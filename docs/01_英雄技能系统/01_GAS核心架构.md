# 01 - GAS核心架构

> 参照对象：漫威争锋 (Marvel Rivals)
> UE5版本：5.3+

---

## 一、GAS概述

Gameplay Ability System (GAS) 是UE5内置的RPG/Action游戏框架，漫威争锋大量依赖GAS实现英雄技能。

### 核心组件

| 组件 | 作用 | 漫威争锋对应 |
|------|------|-------------|
| `AbilitySystemComponent` (ASC) | 角色身上的技能容器，管理GA/GE/Attribute | 每个英雄挂载的ASC |
| `GameplayAbility` (GA) | 单个技能的逻辑实体 | Q/E/Shift/右键/左键/大招 |
| `GameplayEffect` (GE) | 用于修改属性、施加Buff/Debuff | 伤害、治疗、减速、增伤 |
| `GameplayTag` | 层级标签，用于标记状态和条件 | 状态标记（眩晕/飞行/隐身） |
| `GameplayCue` | 技能的视觉/音效表现 | 技能特效、命中音效 |
| `AttributeSet` | 角色属性集合 | 血量、护盾、弹药、能量 |

---

## 二、待拆解点

1. [ ] 英雄ASC架构：PlayerState上挂ASC vs 角色身上挂ASC
2. [ ] 技能输入绑定：EnhancedInput + GameplayTag映射
3. [ ] 技能CD与消耗：GE-Cost实现
4. [ ] 技能打断与优先级：CancelAbilities / BlockAbilitiesByTag
5. [ ] 被动技能实现：持续生效的GE
6. [ ] 大招充能机制：Attribute + GE驱动

---

## 三、漫威争锋观察要点

- 每个英雄4个主动技能 + 1个被动 + 1个大招 + 1个近战+ 1个主武器
- 技能之间可能存在"连携"（例如浩克与钢铁侠的协同）
- 需要关注技能在不同状态下的可用性（被控制时如何禁用）
- 飞行英雄和地面英雄可能有不同的技能交互规则

---

## 备注

> **实际体验建议**：选择3-5个英雄深入体验，记录每个技能的触发方式、效果表现、CD时间、与其他技能的交互。
