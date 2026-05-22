# -*- encoding: utf-8 -*-
"""
Step 1.1: GAS 角色骨架
定义 AttributeSet + 提供运行时初始化函数

用法:
    # 角色 BeginPlay 时调用:
    from gas.setup_character import init_gas_for_actor
    init_gas_for_actor(self)  # self 是 Pawn/Actor 实例
"""
import ue


# ==================== 1. 自定义 AttributeSet ====================

class AttrSet_Base(ue.AttributeSet):
    """基础属性集：血量、最大血量、攻击力、速度"""
    Health: float = 100.0
    MaxHealth: float = 100.0
    AttackPower: float = 10.0
    MoveSpeed: float = 600.0


# ==================== 2. 运行时初始化 ====================

def init_gas_for_actor(actor: ue.Actor):
    """
    给已存在于世界的 Actor 实例初始化 GAS 骨架。
    如果 Actor 蓝图中已有 ASC 组件，则直接使用；
    否则动态添加。

    Args:
        actor: Pawn 或 Character 实例（必须是运行时实例，不能是 CDO）
    """
    if actor.IsClassDefaultObject():
        print("[GAS] 错误: 不能对 CDO 调用，请对运行时 Actor 实例调用")
        return

    # 1) 查找已有的 ASC
    asc = actor.GetComponentByClass(ue.AbilitySystemComponent)

    # 2) 没有则动态添加（运行时实例有 World，AddComponentByClass 有效）
    if not asc:
        asc = actor.AddComponentByClass(
            ue.AbilitySystemComponent,
            False,
            ue.Transform(),
            False
        )
        if asc:
            print("[GAS] ASC 已动态添加")
        else:
            print("[GAS] ASC 添加失败")
            return

    print(f"[GAS] ASC: {asc}")

    # 3) 初始化 AttributeSet — Nepy 绑定需要传 Class 对象而非 Python type
    attr_set_class = AttrSet_Base.static_class() if hasattr(AttrSet_Base, 'static_class') else AttrSet_Base.__class__
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

    print("[GAS] 角色骨架初始化完成")
    return asc
