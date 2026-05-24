# -*- encoding: utf-8 -*-
"""Step 2.1 临时测试：直接驱动 LocomotionUpdater"""
import ue
from animation.locomotion import register_locomotion_for_character, _registry

w = ue.GetGameWorld()
if not w:
    print("[test_locomotion] 错误: 没有 World")
else:
    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[test_locomotion] 错误: 获取不到 Pawn")
    else:
        pawn = ctrl.Pawn
        if id(pawn) in _registry:
            print(f"[test_locomotion] 当前已注册, Speed 每帧更新中")
            # 读取当前 Speed
            try:
                mesh = pawn.Mesh if hasattr(pawn, 'Mesh') else pawn.get_component_by_class(ue.SkeletalMeshComponent.Class())
                if mesh:
                    anim = mesh.GetAnimInstance()
                    if anim:
                        speed_val = anim.Speed if hasattr(anim, 'Speed') else 0
                        print(f"[test_locomotion] AnimBP Speed = {speed_val:.1f}")
            except Exception as e:
                print(f"[test_locomotion] 读取 Speed 失败: {e}")
        else:
            register_locomotion_for_character(pawn)
            print("[test_locomotion] LocomotionUpdater 已注册, 请移动角色")
