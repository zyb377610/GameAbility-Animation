# -*- encoding: utf-8 -*-
"""
Step 1.3: 消耗 GameplayEffect
GE_Cost_Fireball — Instant，扣减 Mana 20 点

⚠️ 注意：NePy 不支持在 Python 中配置 GameplayEffect.Modifiers（ArrayWrapper 无法赋值）。
Modifiers 需在编辑器蓝图中手动配置。
此 Python 类仅提供 UClass 注册，实际 Modifiers 通过蓝图 GE_Cost_Fireball 资产配置。

蓝图配置步骤：
1. 在编辑器中基于此类创建蓝图 GE 资产
2. DurationPolicy = Instant
3. Modifiers → Add → Attribute = AttrSet_Base.Mana, ModifierOp = Add, Magnitude = -20 (ScalableFloat)
"""
import ue


@ue.uclass()
class GE_Cost_Fireball(ue.GameplayEffect):
    """火球技能消耗 GE：Instant, Modifiers 在蓝图中配置"""

    def __init_default__(self):
        self.DurationPolicy = ue.EGameplayEffectDurationType.Instant
