# -*- encoding: utf-8 -*-
"""
Step 1.3: 冷却 GameplayEffect
GE_Cooldown_Fireball — HasDuration, 5 秒冷却
通过 TargetTagsGameplayEffectComponent 给自身添加 "Cooldown.Fireball" Tag
配合 GA_Fireball.ActivationBlockedTags 阻止冷却期间激活

⚠️ GEComponents 数组配置为 Python 实验性代码。
如果 GEComponents 在 Python 中无法正确添加（ArrayWrapper 限制），
则需在编辑器中手动创建蓝图 GE_Cooldown_Fireball 资产，
并配置 DurationPolicy=HasDuration, Duration=5s, GrantedTags 添加 Cooldown.Fireball。
"""
import ue


@ue.uclass()
class GE_Cooldown_Fireball(ue.GameplayEffect):
    """火球技能冷却 GE：持续 5 秒"""

    def __init_default__(self):
        self.DurationPolicy = ue.EGameplayEffectDurationType.HasDuration
        # Duration 5 秒
        self.DurationMagnitude.ScalableFloatMagnitude.Value = 5.0

        # 尝试通过 TargetTagsGameplayEffectComponent 添加冷却 Tag
        try:
            tag_component = ue.TargetTagsGameplayEffectComponent()
            cooldown_tag = ue.GameplayTag()
            cooldown_tag.TagName = "Cooldown.Fireball"
            tag_component.InheritableGrantedTagsContainer.Added.GameplayTags.Append(
                cooldown_tag)
            self.GEComponents.Append(tag_component)
            print("[GE_Cooldown_Fireball] TargetTagsGameplayEffectComponent 配置成功")
        except Exception as e:
            print(f"[GE_Cooldown_Fireball] 警告: GEComponents 配置失败 ({e})"
                  " — 需在编辑器中手动创建蓝图 GE 资产")
