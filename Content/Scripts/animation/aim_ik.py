# -*- encoding: utf-8 -*-
"""
Step 2.3: AimOffset + IK — 瞄准偏移与简易 IK

核心原理:
  AimOffset 是 2D BlendSpace，根据 Pitch/Yaw 混合不同瞄准角度的姿势。
  配合 Step 2.2 的 LayeredBlendPerBone（上下半身分离），上半身独立瞄准，
  下半身继续 BlendSpace 移动。

数据流:
  摄像机朝向 → Python 计算 Pitch/Yaw delta → 写入 AnimBP →
  AimOffset 节点混合姿势 → LayeredBlendPerBone 合并上下半身 → 最终 Pose

用法:
    from animation.aim_ik import AimIKController

    # 方案 A：手动 Tick（在 nepyinit.py on_tick 中调用）
    ctrl = AimIKController(actor)
    ctrl.tick(delta_time)

    # 方案 B：自驱动 Ticker
    ctrl.start_ticker()
"""

import ue
import math


# ==================== 1. AimIKController ====================

class AimIKController:
    """
    瞄准偏移控制器。
    
    每帧获取摄像机朝向，计算相对于角色身体朝向的 Pitch/Yaw 差值，
    写入 AnimBP 驱动 AimOffset 节点。

    关键设计决策：
    - 纯 Python 对象，不继承 ue.uclass（避免 __init_default__ 限制）
    - 用模块级 _registry 管理实例（与 locomotion.py / upper_body.py 一致）
    - 摄像机旋转用 PlayerCameraManager.GetCameraRotation()，
      比 ControlRotation 更准确（ControlRotation 可能被 lag/平滑处理过）
    """

    # ---- 多摄像机方案的默认 PlayerIndex ----
    DEFAULT_PLAYER_INDEX = 0

    def __init__(self, actor):
        """
        Args:
            actor: Character/Pawn 运行时实例
        """
        self.actor = actor
        self.mesh = None
        self.anim_inst = None
        self._ticker_handle = None
        self._init_mesh()

    def _init_mesh(self):
        """获取并缓存 SkeletalMeshComponent 和 AnimInstance"""
        try:
            if hasattr(self.actor, 'Mesh'):
                self.mesh = self.actor.Mesh
            else:
                self.mesh = self.actor.get_component_by_class(
                    ue.SkeletalMeshComponent.Class())

            if self.mesh:
                self.anim_inst = self.mesh.GetAnimInstance()
                if self.anim_inst:
                    print(f"[AimIK] AnimInstance 已获取: {self.anim_inst}")
                else:
                    print("[AimIK] 警告: 获取不到 AnimInstance")
            else:
                print("[AimIK] 错误: 获取不到 SkeletalMeshComponent")
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[AimIK] _init_mesh 失败: {e}")

    # ---------------------------------------------------------------
    # 核心 Tick：Pitch/Yaw 计算 & 写入 AnimBP
    # ---------------------------------------------------------------

    def tick(self, delta_time: float):
        """
        每帧 Tick 入口。

        步骤:
          1. 通过 PlayerCameraManager 获取当前摄像机朝向
          2. 计算 cam_rot - actor_rot 得到角色需要瞄准的相对角度
          3. 归一化 Pitch/Yaw 到合理范围 (-90~90)
          4. 写入 AnimBP 的 AimPitch / AimYaw 变量

        Args:
            delta_time: 帧间隔时间（秒），当前实现不需要但保留以兼容统一 Tick 接口
        """
        if not self.anim_inst or not self.actor:
            return

        try:
            # ---- 0. 鼠标输入降级驱动（蓝图 InputAxis 不触发时的后备方案） ----
            #      GetInputAxisValue 直接读轴值缓存，不依赖 InputComponent 委托
            self._drive_mouse_input(delta_time)

            # ---- 1. 获取 Actor 旋转 ----
            actor_rot = self.actor.GetActorRotation()

            # ---- 2. 获取摄像机旋转 ----
            cam_rot = self._get_camera_rotation()
            if cam_rot is None:
                return

            # ---- 3. 计算 delta = 摄像机朝向 - 角色朝向 ----
            #      Rotator.__sub__ 已由 Nepy 支持（pystubs line 120182）
            delta = cam_rot - actor_rot
            # .GetNormalized() 将角度规整到 [-180, 180] 范围
            delta = delta.GetNormalized()

            # ---- 4. 提取 Pitch 和 Yaw ----
            pitch = delta.Pitch   # 上下：正=抬头，负=低头
            yaw = delta.Yaw       # 左右：正=右转，负=左转

            # 限制 AimOffset 的典型范围 (-90~90)
            # AimOffset 资产中的采样点通常覆盖这个范围，超出则 clamping
            pitch = max(-90.0, min(90.0, pitch))
            yaw = max(-90.0, min(90.0, yaw))

            # ---- 5. 写入 AnimBP 变量 ----
            self._write_anim_var('AimPitch', pitch)
            self._write_anim_var('AimYaw', yaw)

            # 每 60 帧打印一次调试信息
            if not hasattr(self, '_debug_counter'):
                self._debug_counter = 0
            self._debug_counter += 1
            if self._debug_counter % 60 == 0:
                print(f"[AimIK] Pitch={pitch:.1f}, Yaw={yaw:.1f}")

        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[AimIK] tick 异常: {e}")

    # ---------------------------------------------------------------
    # 摄像机旋转获取
    # ---------------------------------------------------------------

    def _get_camera_rotation(self):
        """
        获取当前玩家摄像机的世界旋转。

        使用 GameplayStatics.GetPlayerCameraManager 获取摄像机管理器，
        然后调用 GetCameraRotation() 获取实际摄像机朝向。

        为什么不用 GetControlRotation()？
          - ControlRotation 是玩家控制的"期望朝向"，可能被输入平滑处理
          - GetCameraRotation 是实际渲染用的摄像机朝向，更准确
          - 对于 AimOffset，两个差别不大，但 GetCameraRotation 更"物理正确"

        Returns:
            Rotator | None: 摄像机世界旋转，失败返回 None
        """
        world = self.actor.GetWorld()
        if not world:
            return None

        cam_mgr = ue.GameplayStatics.GetPlayerCameraManager(
            world, self.DEFAULT_PLAYER_INDEX)
        if not cam_mgr:
            return None

        cam_rot = cam_mgr.GetCameraRotation()
        return cam_rot

    # ---------------------------------------------------------------
    # 鼠标输入降级方案（蓝图 InputAxis 不触发时的后备）
    # ---------------------------------------------------------------

    # 鼠标灵敏度缩放系数（与 DefaultInput.ini 中 MouseX/Y 的 Sensitivity 对应）
    MOUSE_SENSITIVITY = 0.07

    def _drive_mouse_input(self, delta_time: float):
        """
        读取 Turn/LookUp 轴输入，直接驱动 PlayerController 的 ControlRotation。

        如果蓝图 InputAxis 节点正常触发（重启编辑器后 DefaultInput.ini 生效），
        本方法和蓝图的效果叠加。但因为 InputAxis 每次只触发一次且增量相同，
        叠加后每帧只多处理一次，视觉上无区别。

        为什么需要这个降级方案：
          - 蓝图 InputAxis 依赖 InputComponentClass = InputComponent（旧版）
          - DefaultInput.ini 中若仍是 EnhancedInputComponent，蓝图事件不触发
          - GetInputAxisValue 绕过 InputComponent，直接从 PlayerInput 读缓存
          - 本方法确保无论 InputComponent 类型如何，鼠标都能驱动瞄准
        """
        turn_val = self.actor.GetInputAxisValue("Turn")       # MouseX
        lookup_val = self.actor.GetInputAxisValue("LookUp")   # MouseY

        if abs(turn_val) < 0.001 and abs(lookup_val) < 0.001:
            return

        # AddControllerYawInput / AddControllerPitchInput 是 Pawn 的方法（不是 Controller）
        # pystubs line 147783 / 147791
        self.actor.AddControllerYawInput(turn_val)
        self.actor.AddControllerPitchInput(lookup_val)

    def _get_player_controller(self):
        """获取控制此 Actor 的 PlayerController"""
        world = self.actor.GetWorld()
        if not world:
            return None
        ctrl = ue.GameplayStatics.GetPlayerController(world, self.DEFAULT_PLAYER_INDEX)
        if ctrl and ctrl.Pawn == self.actor:
            return ctrl
        return None

    # ---------------------------------------------------------------
    # IK 手部目标计算（简易方案，可扩展）
    # ---------------------------------------------------------------

    def update_hand_ik(self, alpha: float = 0.0):
        """
        更新右手 IK 目标位置。

        当前实现为基础版本：将 IK 目标放在角色前方胸前位置。
        当挂载武器网格后，应改为读取武器握柄 Socket 世界坐标。

        Args:
            alpha: IK 混合权重 (0.0 ~ 1.0)，0=禁用，1=完全 IK
        """
        if not self.anim_inst:
            return

        # 写入 IK 混合权重
        self._write_anim_var('IKHandAlpha', alpha)

        if alpha <= 0.0:
            return

        # 计算基础 IK 目标位置（胸前偏右）
        actor_loc = self.actor.GetActorLocation()
        actor_forward = self.actor.GetActorForwardVector()
        actor_right = self.actor.GetActorRightVector()
        actor_up = self.actor.GetActorUpVector()

        # 目标：角色前方 60cm, 右侧 25cm, 上方 120cm（近似步枪握持位置）
        target_loc = actor_loc + actor_forward * 60.0 + actor_right * 25.0 + actor_up * 120.0

        # 骨骼空间转换：世界坐标 → mesh 组件空间
        if self.mesh:
            # K2_GetComponentToWorld 是蓝图暴露的 C++ GetComponentTransform 等效函数
            mesh_transform = self.mesh.K2_GetComponentToWorld()
            local_target = mesh_transform.InverseTransformPosition(target_loc)

            # 构建 Transform: (位置=local_target, 旋转=零, 缩放=1)
            # 用构造函数传参：FTransform(Quat, Vector, Vector)
            target_transform = ue.Transform()
            target_transform.Translation = local_target
            target_transform.Scale3D = ue.Vector(1.0, 1.0, 1.0)

            self._write_anim_var('IKHandTarget_R', target_transform)

    # ---------------------------------------------------------------
    # 自驱动 Ticker（可选）
    # ---------------------------------------------------------------

    def start_ticker(self):
        """自驱动模式：注册 ue.AddTicker，不依赖外部 Tick"""
        if self._ticker_handle is not None:
            return
        self._ticker_handle = ue.AddTicker(self.tick)
        print("[AimIK] Ticker 已启动（自驱动模式）")

    def stop_ticker(self):
        """停止自驱动 Ticker"""
        if self._ticker_handle is not None:
            ue.RemoveTicker(self._ticker_handle)
            self._ticker_handle = None
            print("[AimIK] Ticker 已停止")

    # ---------------------------------------------------------------
    # AnimBP 变量写入（与 locomotion.py / upper_body.py 相同策略）
    # ---------------------------------------------------------------

    def _write_anim_var(self, var_name, value):
        """
        尝试多种方式写入 AnimInstance 变量。

        策略:
          1. setattr() — 直接属性赋值（最常见路径）
          2. set_editor_property() — 编辑器属性接口（备用）
        """
        try:
            setattr(self.anim_inst, var_name, value)
        except Exception:
            try:
                self.anim_inst.set_editor_property(var_name, value)
            except Exception as e:
                print(f"[AimIK] 写入 {var_name} 失败: {e}")


# ==================== 2. 模块级注册表 ====================
# 与 locomotion.py / upper_body.py 一致的模式：
# 用 id(actor) 做 key，避免 Nepy uobject 的 Python 属性赋值限制

_registry = {}  # {id(actor): AimIKController}


def register_aim_ik(actor) -> AimIKController:
    """
    为角色注册 AimIKController。

    调用方式（在角色 BeginPlay 或 nepyinit.py 中）:
        from animation.aim_ik import register_aim_ik
        register_aim_ik(self)  # self = Character 实例

    Args:
        actor: Character/Pawn 运行时实例

    Returns:
        AimIKController 实例
    """
    if actor.IsClassDefaultObject():
        print("[AimIK] 错误: 不能对 CDO 调用")
        return None

    key = id(actor)
    if key not in _registry:
        ctrl = AimIKController(actor)
        ctrl.start_ticker()  # 自驱动模式
        _registry[key] = ctrl
        print(f"[AimIK] AimIKController 已注册到 {actor}")
    else:
        ctrl = _registry[key]
        # 如果已存在但 Ticker 没启动（如重置场景），重新启动
        if ctrl._ticker_handle is None:
            ctrl.start_ticker()
    return _registry[key]


def unregister_aim_ik(actor):
    """移除 AimIKController，停止 Ticker"""
    key = id(actor)
    if key in _registry:
        _registry[key].stop_ticker()
        del _registry[key]
        print(f"[AimIK] AimIKController 已移除")


# ==================== 3. 全局 Tick 驱动 ====================

def tick_all_aim_ik(delta_time: float):
    """
    遍历所有已注册 AimIKController 并驱动 tick。
    
    如果使用自驱动 Ticker 则不需要调用此函数。
    如果所有 AimIKController 都用 start_ticker() 了，
    这个函数是备用的集中 Tick 方案。

    在 nepyinit.py 的 on_tick(dt) 中调用:
        from animation.aim_ik import tick_all_aim_ik
        tick_all_aim_ik(dt)
    """
    try:
        dead_keys = []
        for key, ctrl in list(_registry.items()):
            try:
                _ = ctrl.actor
                ctrl.tick(delta_time)
            except Exception:
                dead_keys.append(key)

        for key in dead_keys:
            del _registry[key]
    except Exception:
        pass
