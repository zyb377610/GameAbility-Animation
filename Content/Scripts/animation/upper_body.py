# -*- encoding: utf-8 -*-
"""
Step 2.2: 上下半身分离 — 上半身动画控制

核心思路：
  AnimBP 中 LayeredBlendPerBone 把骨骼分两半：
    - spine_01 及以上（上半身+手臂+头）→ 来自 Slot 'UpperBody'
    - spine_01 以下（pelvis + 腿）→ 来自 BS_Locomotion（BlendSpace 移动）

  本模块的 UpperBodyController 负责在运行时向 Slot 注入动画。
  不注入时 Slot 输出参考姿势，LayeredBlendPerBone 自动退回到 Base 层 =
  全身 BlendSpace，上半身也随之正常。

技术要点：
  1. PlaySlotAnimationAsDynamicMontage — 不依赖 AnimBP 蓝图连线，
     运行时动态指定 Slot 名即可注入动画。返回 AnimMontage 对象。
  2. Slot 名 "UpperBody" 必须与 AnimBP 中 Slot 节点名完全一致（区分大小写）。
  3. BlendInTime / BlendOutTime 控制图层混合速度：
     - BlendIn:  上半身从 Base 动画平滑过渡到 Slot 动画
     - BlendOut: Slot 动画结束 → 平滑退回 Base 动画
  4. 下半身始终保持 Base 层（BlendSpace 移动），不受上半身 Slot 影响。

用法：
    from animation.upper_body import UpperBodyController

    # 获取角色的 SkeletalMeshComponent
    mesh = character.Mesh  # 或 character.get_component_by_class(ue.SkeletalMeshComponent.Class())

    # 创建控制器
    upper = UpperBodyController(mesh)

    # 播放射击动画（上半身 Slot）
    upper.play_shoot_animation()

    # 播放受击动画
    upper.play_hit_animation()
"""

import ue


class UpperBodyController:
    """
    上半身动画控制器。

    通过 PlaySlotAnimationAsDynamicMontage 在 AnimBP 的 'UpperBody' Slot
    上动态注入动画。下半身由 BlendSpace 驱动不受影响。

    注意：本类为纯 Python 对象，不继承 ue.uclass。
    避免 __init_default__ 限制，状态用模块级注册表管理
    （与 locomotion.py 相同策略）。
    """

    # ---------------------------------------------------------------
    # 动画路径常量（集中管理，修改方便）
    # ---------------------------------------------------------------
    # 注意：PlaySlotAnimationAsDynamicMontage 接受 AnimSequence，
    #       Montage_Play 接受 AnimMontage。两者路由方式不同：
    #       - AnimSequence → 通过 SlotNodeName 参数指定目标 Slot
    #       - AnimMontage   → 通过 Montage 资产自带的 Slot 名路由
    # 这里用 AnimSequence 直接注入到 UpperBody Slot，避免 Montage
    # 的 Slot 名匹配问题。
    ANIM_SHOOT = "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"

    # 可选：受击动画（可按需替换为实际资源路径）
    ANIM_HIT = "/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01"

    # AnimBP 中配置的 Slot 节点名
    # 引擎匹配 SlotNodeName 时使用 GroupName.SlotName 格式
    SLOT_NAME = "DefaultGroup.UpperBody"

    # ---------------------------------------------------------------
    # 初始化
    # ---------------------------------------------------------------

    def __init__(self, mesh: ue.SkeletalMeshComponent):
        """
        Args:
            mesh: 角色的 SkeletalMeshComponent 实例。
                  AnimBP 必须挂载在此 Component 上（通过 Mesh.AnimClass）。
        """
        self.mesh = mesh
        self._current_montage = None  # 当前播放的 Montage 引用

    # ---------------------------------------------------------------
    # 核心方法：在 Slot 上播放动画
    # ---------------------------------------------------------------

    def _play_on_slot(self, anim_path: str, blend_in: float = 0.25,
                      blend_out: float = 0.25, play_rate: float = 1.0):
        """
        在 UpperBody Slot 播放指定动画序列。

        PlaySlotAnimationAsDynamicMontage 签名：
          (Asset: AnimSequenceBase, SlotNodeName, BlendInTime, BlendOutTime,
           InPlayRate, LoopCount, BlendOutTriggerTime,
           InTimeToStartMontageAt) -> AnimMontage

        原理：
          1. 加载 AnimSequence 资产
          2. 在 AnimInstance 上以 'UpperBody' Slot 名动态创建 Montage 播放
          3. AnimBP 中的 Slot 节点 + LayeredBlendPerBone 将动画注入上半身
          4. 动画结束 → Blend Out → 上半身回到 Base 层

        为什么用 AnimSequence 而非 AnimMontage：
          - PlaySlotAnimationAsDynamicMontage 的 Asset 参数类型是
            AnimSequenceBase（动画序列），不是 AnimMontage
          - AnimMontage 自带 Slot 配置，用 Montage_Play，Slot 名必须匹配
          - AnimSequence 无 Slot 概念，通过 SlotNodeName 参数直接指定目标

        Args:
            anim_path: AnimSequence 资产路径
            blend_in: 混合入时间（秒）
            blend_out: 混合出时间（秒）
            play_rate: 播放速率
        """
        # 1. 加载 AnimSequence
        anim = ue.LoadObject(ue.AnimSequence, anim_path)
        if not anim:
            print(f"[UpperBody] 错误: 找不到动画: {anim_path}")
            return

        # 2. 在 AnimInstance 上动态创建 Montage 并播放
        anim_inst = self.mesh.GetAnimInstance()
        if not anim_inst:
            print("[UpperBody] 错误: 获取不到 AnimInstance")
            return

        self._current_montage = anim_inst.PlaySlotAnimationAsDynamicMontage(
            anim,              # Asset: AnimSequenceBase
            self.SLOT_NAME,    # SlotNodeName: 必须匹配 AnimBP 中 Slot 节点名
            blend_in,          # BlendInTime
            blend_out,         # BlendOutTime
            play_rate,         # InPlayRate
            1,                 # LoopCount
            -1.0,              # BlendOutTriggerTime (-1 = 动画播完自动 BlendOut)
            0.0                # InTimeToStartMontageAt
        )

        # 3. 设置 AnimBP 的 UpperBodyAlpha 变量 = 1.0
        #    告知 LayeredBoneBlend 将 BlendPoses 的权重启用
        self._set_anim_var('UpperBodyAlpha', 1.0)

        print(f"[UpperBody] Slot '{self.SLOT_NAME}' 播放: "
              f"{anim.GetName()} "
              f"(BlendIn={blend_in}s, BlendOut={blend_out}s)")

    # ---------------------------------------------------------------
    # 便捷方法
    # ---------------------------------------------------------------

    def play_shoot_animation(self):
        """播放射击动画（上半身），下半身继续移动。"""
        self._play_on_slot(self.ANIM_SHOOT, blend_in=0.15, blend_out=0.2)

    def play_hit_animation(self, blend_in: float = 0.1, blend_out: float = 0.3):
        """
        播放受击动画（上半身），下半身不打断移动。

        漫威对照：
          - 角色被击中身体后仰但继续移动
          - 受击动画只影响上半身，腿部 Keep Walking

        Args:
            blend_in: 混合入时间（秒），受击反应需要快速体现
            blend_out: 混合出时间（秒），受击恢复稍慢
        """
        self._play_on_slot(self.ANIM_HIT, blend_in=blend_in,
                           blend_out=blend_out)

    def play_anim(self, anim_path: str, blend_in: float = 0.25,
                  blend_out: float = 0.25, play_rate: float = 1.0):
        """
        播放任意 AnimSequence 到上半身 Slot 的通用接口。

        Args:
            anim_path: AnimSequence 资产完整路径
            blend_in: 混合入时间
            blend_out: 混合出时间
            play_rate: 播放速率
        """
        self._play_on_slot(anim_path, blend_in=blend_in,
                           blend_out=blend_out, play_rate=play_rate)

    # ---------------------------------------------------------------
    # 状态查询
    # ---------------------------------------------------------------

    def is_playing(self) -> bool:
        """上半身是否正在播放动画。"""
        if self._current_montage and self.mesh:
            anim_inst = self.mesh.GetAnimInstance()
            if anim_inst:
                try:
                    # Montage_IsPlaying 检查 Montage 是否活跃
                    return anim_inst.Montage_IsPlaying(self._current_montage)
                except Exception:
                    pass
        return False

    def stop(self, blend_out: float = 0.25):
        """
        强制停止当前上半身动画。

        Args:
            blend_out: 混合出时间（秒）
        """
        if self._current_montage and self.mesh:
            anim_inst = self.mesh.GetAnimInstance()
            if anim_inst:
                try:
                    anim_inst.Montage_Stop(blend_out, self._current_montage)
                    print(f"[UpperBody] 已停止上半身动画 "
                          f"(BlendOut={blend_out}s)")
                except Exception as e:
                    print(f"[UpperBody] 停止失败: {e}")
        self._current_montage = None
        # 重置 BlendWeight，让上半身完全回到 Base 层
        self._set_anim_var('UpperBodyAlpha', 0.0)

    # ---------------------------------------------------------------
    # 内部辅助
    # ---------------------------------------------------------------

    def _set_anim_var(self, var_name, value):
        """写入 AnimInstance 变量（与 locomotion.py 相同策略）"""
        if not self.mesh:
            return
        anim_inst = self.mesh.GetAnimInstance()
        if not anim_inst:
            return
        try:
            setattr(anim_inst, var_name, value)
        except Exception:
            try:
                anim_inst.set_editor_property(var_name, value)
            except Exception as e:
                print(f"[UpperBody] 写入 {var_name} 失败: {e}")


# ==================== 模块级注册表 ====================
# 与 locomotion.py 相同策略：避免 Nepy uobject 的属性赋值限制，
# 用 id(actor) 做 key 管理 UpperBodyController 实例。

_registry = {}  # {id(actor): UpperBodyController}


def get_upper_body_controller(actor) -> UpperBodyController:
    """
    获取或创建角色的 UpperBodyController。

    Args:
        actor: Character/Pawn 运行时实例

    Returns:
        UpperBodyController 实例
    """
    key = id(actor)
    if key not in _registry:
        mesh = None
        if hasattr(actor, 'Mesh'):
            mesh = actor.Mesh
        else:
            mesh = actor.get_component_by_class(
                ue.SkeletalMeshComponent.Class())
        _registry[key] = UpperBodyController(mesh)
    return _registry[key]
