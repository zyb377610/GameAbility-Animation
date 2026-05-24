# -*- encoding: utf-8 -*-
"""
Step 2.1: BlendSpace 移动系统

LocomotionUpdater 每 Tick 计算角色速度（大小 + 方向），
写入 AnimBP 的 Speed 和 Direction 变量，驱动 BlendSpace 平滑切换。

用法（在 GameMode 的 on_tick 或角色 BeginPlay 中注册 Tick）:
    from animation.locomotion import LocomotionUpdater, register_locomotion_for_character
    register_locomotion_for_character(self)  # self = Character/Pown 实例

测试:
    > nepy locomotion_test  # 对当前选中角色启动/停止测试
"""
import ue
import math


# ==================== 1. 方向辅助计算 ====================

def _compute_direction(velocity, forward):
    """
    计算速度方向与角色朝向的水平夹角（度）。

    Args:
        velocity: ue.Vector, 角色速度
        forward: ue.Vector, 角色前方向量

    Returns:
        float, -180 ~ 180 度，正值表示速度在角色右侧
    """
    speed = velocity.Length()
    if speed < 10.0:
        return 0.0  # 几乎静止，方向无意义

    vel_dir = velocity.GetSafeNormal(0.001)
    dot = forward.Dot(vel_dir)
    cross = forward.Cross(vel_dir)
    # cross.Z > 0 → 右侧，cross.Z < 0 → 左侧
    angle = math.degrees(math.atan2(cross.Z, dot))
    return angle


# ==================== 2. LocomotionUpdater ====================

class LocomotionUpdater:
    """
    每帧将角色速度写入 AnimBP 的 Speed/Direction 变量。

    不继承 ue.Object，避免生命周期问题。作为普通 Python 对象，
    挂载到 actor 实例上（actor._locomotion_updater），
    由外部 Tick 驱动。

    Attributes:
        actor: 目标 Character/Pawn 实例
        mesh: 缓存的 SkeletalMeshComponent
        anim_inst: 缓存的 AnimInstance
    """

    def __init__(self, actor):
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
                    print(f"[Locomotion] AnimInstance 已获取: "
                          f"{self.anim_inst}")
                else:
                    print("[Locomotion] 警告: 获取不到 AnimInstance，"
                          "请确保角色 Mesh 指定了 AnimBP")
            else:
                print("[Locomotion] 错误: 获取不到 SkeletalMeshComponent")
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[Locomotion] _init_mesh 失败: {e}")

    def tick(self, delta_time: float):
        """
        每帧 Tick 入口。
        1. 读取 WASD 输入 → CharacterMovement.AddMovementInput
        2. 读取 Actor 实际速度 → 写入 AnimBP Speed/Direction
        """
        if not self.anim_inst or not self.actor:
            return

        try:
            # ---- 1. 驱动移动输入（绕过蓝图 Input） ----
            # 读取轴输入（需要项目设置有 MoveForward / MoveRight 轴映射）
            forward_val = self.actor.GetInputAxisValue("MoveForward")    # W=+1, S=-1
            right_val = self.actor.GetInputAxisValue("MoveRight")        # D=+1, A=-1

            if abs(forward_val) > 0.01 or abs(right_val) > 0.01:
                # 获取玩家控制器旋转
                ctrl = self._get_player_controller()
                yaw = 0.0
                if ctrl:
                    yaw = ctrl.GetControlRotation().Yaw

                # 构建移动方向（基于控制器朝向）
                import math
                rad = math.radians(yaw)
                forward_dir = ue.Vector(math.cos(rad), math.sin(rad), 0.0)
                right_dir = ue.Vector(-math.sin(rad), math.cos(rad), 0.0)

                world_dir = (forward_dir * forward_val + right_dir * right_val)
                if world_dir.Length() > 0.01:
                    world_dir = world_dir.GetSafeNormal(0.001)
                    self.actor.AddMovementInput(world_dir, 1.0, False)

            # ---- 2. 读取移动速度写入 AnimBP ----
            velocity = self.actor.GetVelocity()
            speed = velocity.Length()
            forward = self.actor.GetActorForwardVector()
            direction = _compute_direction(velocity, forward)

            self._write_anim_var('Speed', speed)
            self._write_anim_var('Direction', direction)

            # 每 60 帧打印一次
            if not hasattr(self, '_debug_counter'):
                self._debug_counter = 0
            self._debug_counter += 1
            if self._debug_counter % 60 == 0:
                print(f"[Locomotion] Speed={speed:.1f}, Dir={direction:.1f}, "
                      f"Input(F={forward_val:.1f},R={right_val:.1f})")
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[Locomotion] tick 异常: {e}")

    def _get_player_controller(self):
        """获取控制这个 Pawn 的 PlayerController"""
        w = ue.GetGameWorld()
        if not w:
            return None
        ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
        if ctrl and ctrl.Pawn == self.actor:
            return ctrl
        return None

    def start_ticker(self):
        """自驱动模式：注册 ue.AddTicker，不依赖蓝图 EventTick"""
        if self._ticker_handle is not None:
            return
        self._ticker_handle = ue.AddTicker(self.tick)
        print("[Locomotion] Ticker 已启动（自驱动模式）")

    def stop_ticker(self):
        """停止自驱动 Ticker"""
        if self._ticker_handle is not None:
            ue.RemoveTicker(self._ticker_handle)
            self._ticker_handle = None
            print("[Locomotion] Ticker 已停止")

    def _write_anim_var(self, var_name, value):
        """尝试多种方式写入 AnimInstance 变量"""
        try:
            setattr(self.anim_inst, var_name, value)
        except Exception:
            try:
                self.anim_inst.set_editor_property(var_name, value)
            except Exception as e:
                print(f"[Locomotion] 写入 {var_name} 失败: {e}")


# ==================== 3. 模块级注册表（避免 Nepy uobject 不支持属性赋值） ====================

# 使用 id(actor) 作为 key，因为 Nepy uobject 不支持直接挂 Python 属性
_registry = {}  # {id(actor): LocomotionUpdater}


def register_locomotion_for_character(actor):
    """
    为角色实例挂载 LocomotionUpdater。

    调用后，需要在 GameMode 的 on_tick() 中遍历所有 Pawn 调用 tick。
    推荐方式：在 nepyinit.py 的 on_tick() 中统一驱动。

    Args:
        actor: Character/Pawn 运行时实例

    Usage:
        from animation.locomotion import register_locomotion_for_character
        register_locomotion_for_character(self)  # 在 ReceiveBeginPlay 中
    """
    if actor.IsClassDefaultObject():
        print("[Locomotion] 错误: 不能对 CDO 调用")
        return None

    updater = LocomotionUpdater(actor)
    updater.start_ticker()  # 自驱动，不依赖蓝图 EventTick
    _registry[id(actor)] = updater
    print(f"[Locomotion] LocomotionUpdater 已注册到 {actor}")
    return updater


def unregister_locomotion_for_character(actor):
    """移除 LocomotionUpdater，停止 Ticker"""
    key = id(actor)
    if key in _registry:
        _registry[key].stop_ticker()
        del _registry[key]
        print(f"[Locomotion] LocomotionUpdater 已移除")
    else:
        print(f"[Locomotion] 该角色未注册")


# ==================== 4. 全局 Tick 驱动 ====================
# 推荐: 在 nepyinit.py 的 on_tick() 中调用 tick_all_characters()

def tick_all_characters(delta_time: float):
    """
    遍历所有已注册 LocomotionUpdater 的角色并驱动 tick。

    应在 nepyinit.py 的 on_tick(dt) 中调用。
    """
    try:
        # 清理已销毁的角色
        dead_keys = []
        for key, updater in list(_registry.items()):
            try:
                # 通过 actor 属性检查是否存活
                _ = updater.actor
                updater.tick(delta_time)
            except Exception:
                dead_keys.append(key)

        for key in dead_keys:
            del _registry[key]
    except Exception:
        pass  # 静默失败，避免刷屏
