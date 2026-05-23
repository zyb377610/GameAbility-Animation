# -*- encoding: utf-8 -*-
"""
Step 1.1: GAS 角色骨架
定义 AttrSet_Base（纯 Python 类，供 InitStats 初始化属性默认值），
以及运行时给 Actor 实例注册 ASC + AttributeSet 的辅助函数。

用法: 角色 BeginPlay 时调用 init_gas_for_actor(self)
"""
import ue


# ==================== 1. 自定义 AttributeSet ====================

@ue.uclass()
class AttrSet_Base(ue.AttributeSet):
    """基础属性集：血量、最大血量、攻击力、速度"""
    Health = ue.uproperty(100.0)
    MaxHealth = ue.uproperty(100.0)
    AttackPower = ue.uproperty(10.0)
    MoveSpeed = ue.uproperty(600.0)


# ==================== 2. 运行时初始化 ====================

def init_gas_for_actor(actor: ue.Actor):
    """
    给运行时 Actor 实例初始化 GAS 骨架。
    AttributeSet 通过 ASC.InitStats() 注册。

    Args:
        actor: Pawn 或 Character 运行时实例（不能是 CDO）

    Usage:
        from gas.setup_character import init_gas_for_actor
        init_gas_for_actor(self)  # 在 ReceiveBeginPlay 中
    """
    if actor.IsClassDefaultObject():
        print("[GAS] 错误: 不能对 CDO 调用，请对运行时 Actor 实例调用")
        return

    # 1) 查找 ASC（优先用蓝图已有组件）
    asc = None
    # 尝试多种方式获取
    asc_class = ue.AbilitySystemComponent.Class()
    if asc_class:
        try:
            asc = actor.GetComponentByClass(asc_class)
        except Exception:
            pass
    if not asc:
        asc = actor.GetComponentByClass(ue.AbilitySystemComponent)
    if not asc and hasattr(actor, 'AbilitySystem'):
        asc = actor.AbilitySystem
        if asc:
            print("[GAS] ASC 通过属性名获取")
    if not asc:
        print("[GAS] 未能获取 ASC，请确保蓝图中有 AbilitySystemComponent 组件")
        return

    print(f"[GAS] ASC: {asc}")

    # 2) 注册 AttributeSet
    attr_set_class = AttrSet_Base.Class()
    attr_set = asc.GetAttributeSet(attr_set_class)
    if not attr_set:
        asc.InitStats(attr_set_class, None)
        attr_set = asc.GetAttributeSet(attr_set_class)
        print("[GAS] AttributeSet 已通过 InitStats 注册")
    else:
        print("[GAS] AttributeSet 已存在")

    if attr_set:
        print(f"[GAS] Health={attr_set.Health}, MaxHealth={attr_set.MaxHealth}, "
              f"AttackPower={attr_set.AttackPower}, MoveSpeed={attr_set.MoveSpeed}")
    else:
        print("[GAS] 警告: 获取不到 AttributeSet")

    print("[GAS] 角色骨架初始化完成")
    return asc
