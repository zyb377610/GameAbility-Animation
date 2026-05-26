# -*- encoding: utf-8 -*-
"""
Step 3.1: Distance Matching — 停步动画的距离匹配

核心原理：
  传统方式：动画固定速度播放 → 驱动角色位移（容易滑步）
  Distance Matching：角色实际位移 → 反驱动动画播放进度（无滑步）

  公式：PlayRate = (DistanceToMatch - CurrentCurveValue) / DeltaTime
     - DistanceToMatch: 角色从停步开始到当前帧，实际累计移动了多远（Python 计算）
     - CurrentCurveValue: 动画当前帧对应的 Distance Curve 值（AnimBP 从曲线读取）
     - PlayRate: 动画需要以多快播放来"追上"实际位移

数据流：
  [角色移动] → [Python: 计算位移增量]
    → [AnimBP.DistanceToMatch] → [读取动画 Distance Curve]
    → [PlayRate = (DistanceToMatch - CurveValue) / dt]
    → [SequenceEvaluator.AdvanceTime(PlayRate * dt)] → [输出匹配的动画帧]

用法：
    from animation.distance_matching import register_distance_matching
    register_distance_matching(pawn)

    # GM 命令: > nepy dm  (toggle 开关)
"""
import ue


# ==================== 1. 运动阶段枚举 ====================

class MotionPhase:
    """角色的运动阶段"""
    IDLE = 0       # 静止
    MOVING = 1     # 移动中
    STOPPING = 2   # 停步过渡中（从移动到静止）


# ==================== 2. DistanceMatchingController ====================

class DistanceMatchingController:
    """
    追踪角色位移，计算 DistanceToMatch 并写入 AnimBP。

    核心职责：
    1. 每帧计算角色位置变化 → 位移增量
    2. 检测运动阶段切换（移动→停步）→ 重置累计距离
    3. 将 DistanceToMatch / DisplacementSinceLastUpdate 写入 AnimBP

    Attributes:
        actor: 目标 Character/Pawn
        mesh: SkeletalMeshComponent（缓存）
        anim_inst: AnimInstance（缓存）
        prev_location: 上一帧位置（用于计算位移增量）
        prev_speed: 上一帧速度（用于检测运动阶段切换）
        accumulated_distance: 当前运动阶段的累计位移(cm)
        current_phase: 当前运动阶段
    """

    # ---- 阶段检测阈值 ----
    MOVING_THRESHOLD = 50.0      # 速度超过此值视为"移动中"(cm/s)
    STOPPING_THRESHOLD = 30.0    # 速度低于此值视为"停步过渡开始"

    def __init__(self, actor):
        self.actor = actor
        self.mesh = None
        self.anim_inst = None
        self.prev_location = None
        self.prev_speed = 0.0
        self.accumulated_distance = 0.0
        self.current_phase = MotionPhase.IDLE
        self._ticker_handle = None
        self._debug_counter = 0

        self._init_mesh()
        self._init_location()

    # ---- 初始化 ----

    def _init_mesh(self):
        """获取 SkeletalMeshComponent 和 AnimInstance 缓存"""
        try:
            if hasattr(self.actor, 'Mesh'):
                self.mesh = self.actor.Mesh
            else:
                self.mesh = self.actor.get_component_by_class(
                    ue.SkeletalMeshComponent.Class())

            if self.mesh:
                self.anim_inst = self.mesh.GetAnimInstance()
                if self.anim_inst:
                    print(f"[DistanceMatching] AnimInstance: {type(self.anim_inst).__name__}")
                else:
                    print("[DistanceMatching] 警告: 获取不到 AnimInstance")
            else:
                print("[DistanceMatching] 错误: 无 SkeletalMeshComponent")
        except Exception:
            import traceback
            traceback.print_exc()

    def _init_location(self):
        """记录初始位置（用于第一帧的位移计算）"""
        try:
            self.prev_location = self.actor.GetActorLocation()
            print(f"[DistanceMatching] 初始位置: "
                  f"({self.prev_location.X:.1f}, {self.prev_location.Y:.1f}, "
                  f"{self.prev_location.Z:.1f})")
        except Exception:
            self.prev_location = ue.Vector(0, 0, 0)

    # ---- 每帧 Tick ----

    def tick(self, delta_time: float):
        """每帧主入口"""
        if not self.anim_inst or not self.actor or not self.prev_location:
            return

        try:
            # --- 步骤1: 计算位移 ---
            cur_location = self.actor.GetActorLocation()
            displacement_vec = cur_location - self.prev_location  # Vector 减法 → 位移向量
            displacement_magnitude = displacement_vec.Length()     # 标量：位移大小(cm)

            # --- 步骤2: 判断前进/后退方向 ---
            forward = self.actor.GetActorForwardVector()
            dot = displacement_vec.Dot(forward)
            signed_distance = displacement_magnitude if dot >= 0 else -displacement_magnitude

            # --- 步骤3: 检测运动阶段变化 ---
            # 用小位移近似速度（避免依赖 GetVelocity，保持独立性）
            if delta_time > 0.0001:
                approx_speed = displacement_magnitude / delta_time
            else:
                approx_speed = 0.0

            phase_changed = self._detect_phase_transition(approx_speed)

            # --- 步骤4: 更新累计距离 ---
            # 注意：这里用 Y 轴分量（UE 坐标系中 Y 是前向），
            # 因为角色的"前向移动距离"在 Y 轴上体现
            # 更准确的做法是在角色前向方向上投影
            y_displacement = displacement_vec.Y  # 前向分量

            if phase_changed:
                # 阶段切换 → 重置累计距离
                old_phase = self.current_phase
                self.accumulated_distance = y_displacement
                if old_phase == MotionPhase.MOVING and self.current_phase == MotionPhase.STOPPING:
                    print(f"[DistanceMatching] ▶ 停步开始！累计距离重置为 "
                          f"{self.accumulated_distance:.2f} cm")
            else:
                # 同一阶段继续累加
                self.accumulated_distance += y_displacement

            # --- 步骤5: 写入 AnimBP ---
            self._write_anim_var('DistanceToMatch', self.accumulated_distance)
            self._write_anim_var('DisplacementSinceLastUpdate', displacement_magnitude)

            # --- 步骤6: 更新缓存 ---
            self.prev_location = cur_location
            self.prev_speed = approx_speed

            # --- 调试日志（每 120 帧一次，避免刷屏）---
            self._debug_counter += 1
            if self._debug_counter % 120 == 0:
                phase_names = {0: 'Idle', 1: 'Moving', 2: 'Stopping'}
                print(f"[DistanceMatching] Phase={phase_names.get(self.current_phase, '?')}, "
                      f"Dist={self.accumulated_distance:.1f}cm, "
                      f"Speed≈{approx_speed:.1f}cm/s, "
                      f"Δ={displacement_magnitude:.1f}cm")

        except Exception:
            import traceback
            traceback.print_exc()

    def _detect_phase_transition(self, current_speed):
        """
        检测运动阶段是否发生变化。

        状态机：
            IDLE ──(speed > MOVING_THRESHOLD)──→ MOVING
            MOVING ──(speed < STOPPING_THRESHOLD)──→ STOPPING
            STOPPING ──(speed < 10)──→ IDLE
            STOPPING ──(speed > MOVING_THRESHOLD)──→ MOVING (急停取消)

        Returns:
            bool: 阶段是否发生变化
        """
        new_phase = self.current_phase

        if self.current_phase == MotionPhase.IDLE:
            if current_speed > self.MOVING_THRESHOLD:
                new_phase = MotionPhase.MOVING
        elif self.current_phase == MotionPhase.MOVING:
            if current_speed < self.STOPPING_THRESHOLD:
                new_phase = MotionPhase.STOPPING
        elif self.current_phase == MotionPhase.STOPPING:
            if current_speed < 10.0:
                new_phase = MotionPhase.IDLE
            elif current_speed > self.MOVING_THRESHOLD:
                new_phase = MotionPhase.MOVING  # 又加速了，取消停步

        changed = (new_phase != self.current_phase)
        self.current_phase = new_phase
        return changed

    # ---- Ticker 自驱动 ----

    def start_ticker(self):
        """注册 UE Ticker，自驱动，不依赖蓝图 EventTick"""
        if self._ticker_handle is not None:
            return
        self._ticker_handle = ue.AddTicker(self.tick)
        print("[DistanceMatching] Ticker 已启动")

    def stop_ticker(self):
        """停止自驱动 Ticker"""
        if self._ticker_handle is not None:
            ue.RemoveTicker(self._ticker_handle)
            self._ticker_handle = None
            print("[DistanceMatching] Ticker 已停止")

    # ---- AnimBP 变量写入 ----

    def _write_anim_var(self, var_name, value):
        """向 AnimInstance 写入变量，尝试多种方式"""
        try:
            setattr(self.anim_inst, var_name, value)
        except Exception:
            try:
                self.anim_inst.set_editor_property(var_name, value)
            except Exception:
                pass  # 静默失败


# ==================== 3. 模块级注册表 ====================

_registry = {}  # {id(actor): DistanceMatchingController}


def register_distance_matching(actor):
    """
    为角色注册 DistanceMatchingController。
    调用后会自驱动（内部注册 Ticker），不依赖蓝图 EventTick。

    Args:
        actor: Character/Pawn 运行时实例

    Returns:
        DistanceMatchingController 或 None
    """
    if actor.IsClassDefaultObject():
        print("[DistanceMatching] 错误: 不能对 CDO 调用")
        return None

    key = id(actor)
    if key in _registry:
        print(f"[DistanceMatching] 该角色已注册，跳过")
        return _registry[key]

    controller = DistanceMatchingController(actor)
    controller.start_ticker()
    _registry[key] = controller
    print(f"[DistanceMatching] 已注册到 {actor}")
    return controller


def unregister_distance_matching(actor):
    """移除 DistanceMatchingController，停止 Ticker"""
    key = id(actor)
    if key in _registry:
        _registry[key].stop_ticker()
        del _registry[key]
        print(f"[DistanceMatching] 已移除")
    else:
        print(f"[DistanceMatching] 该角色未注册")


# ==================== 4. GM 命令入口 ====================

def dm():
    """
    GM 命令: > nepy dm
    对当前玩家控制的 Pawn 启动/停止 Distance Matching（Toggle）
    """
    w = ue.GetGameWorld()
    if not w:
        print("[dm] 错误: 当前没有 World，请先 PIE")
        return

    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[dm] 错误: 获取不到 Pawn")
        return

    pawn = ctrl.Pawn
    key = id(pawn)
    if key in _registry:
        unregister_distance_matching(pawn)
        print("[dm] DistanceMatching 已停止")
    else:
        register_distance_matching(pawn)
        print("[dm] DistanceMatching 已启动"
              "（移动角色后急停，观察停步动画是否无滑步）")
