# Step 1.3: 技能冷却与消耗

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.3 状态改为 ✅。

---

## 一、目标

给火球技能加上冷却（Cooldown）和法力消耗（Cost），通过 GameplayEffect 的 Duration 策略实现。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.2_弹道技能完整链路.md`
- `docs/01_英雄技能系统/step1.1_GAS角色骨架.md`

---

## 三、前置条件

- ✅ Step 1.2 完成：`GA_Fireball` 可正常激活并打出伤害
- 🔲 `AttrSet_Base` 需要有 `Mana` / `MaxMana` 属性（或另建属性集）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/effects/ge_cooldown.py` | `GE_Cooldown_Fireball(ue.GameplayEffect)` | 冷却 GE：`HasDuration=5s`，给自身添加 `Tag.Cooldown.Fireball` |
| `Content/Scripts/gas/effects/ge_cost.py` | `GE_Cost_Fireball(ue.GameplayEffect)` | 消耗 GE：`Instant`，Modifier 扣减 `Mana` |
| `Content/Scripts/gas/abilities/ga_fireball.py` | 更新 `GA_Fireball` | 设置 `CooldownGameplayEffectClass` 和 `CostGameplayEffectClass` |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| 不需要额外蓝图 | — | 全部由 Python 子类生成 |

---

## 五、关键技术点

### 5.1 冷却 GE 设计

`GE_Cooldown_Fireball`：
- `DurationPolicy = EGameplayEffectDurationType.HasDuration`（值=2）
- `DurationMagnitude` 设置冷却时长（如 5.0 秒）
- 通过 `CooldownGameplayEffectClass` 属性挂在 `GA_Fireball` 上
- `GA_Fireball` 需声明 `GameplayTag`：`Tag.Cooldown.Fireball`
- `CommitAbility()` 时 ASC 自动应用冷却 GE 到自身
- 冷却期间，ASC 检查 `ActivationBlockedTags`（或 `AbilityTags` 对应的冷却 Tag）阻止再次激活

**GA 上关键属性**（`__init__.pyi` 确认）：
```python
CooldownGameplayEffectClass: TSubclassOf[GameplayEffect]
"""
This GameplayEffect represents the cooldown.
It will be applied when the ability is committed and the ability cannot be used
again until it is expired.
"""
```

### 5.2 消耗 GE 设计

`GE_Cost_Fireball`：
- `DurationPolicy = EGameplayEffectDurationType.Instant`（值=0）
- Modifier 扣减 `Mana`（如 -20 点）
- 挂在 `CostGameplayEffectClass` 属性上
- `CommitAbility()` 时自动应用

**GA 上关键属性**：
```python
CostGameplayEffectClass: TSubclassOf[GameplayEffect]
"""
This GameplayEffect represents the cost (mana, stamina, etc) of the ability.
It will be applied when the ability is committed.
"""
```

### 5.3 AttrSet_Base 扩展（如需 Mana）

在 `Content/Scripts/gas/setup_character.py` 中扩展：

```python
class AttrSet_Base(ue.AttributeSet):
    Health: float = 100.0
    MaxHealth: float = 100.0
    Mana: float = 100.0       # 新增
    MaxMana: float = 100.0    # 新增
    AttackPower: float = 10.0
    MoveSpeed: float = 600.0
```

### 5.4 冷却 Tag 阻断机制

`GA_Fireball` 的 `AbilityTags` 和 `ActivationBlockedTags` 配合工作：
- `AbilityTags` 包含 `"Ability.Fireball"`
- 冷却 GE 的 Grant 或 Effect 给自身加 `"Cooldown.Fireball"`
- `ActivationBlockedTags` 包含 `"Cooldown.Fireball"`
- 冷却期间 ASC 检查 `ActivationBlockedTags` → 拒绝激活

### 5.5 Python 中用 Tag 值设置 GE Duration

注意：Python 中 `DurationMagnitude` 的类型为 `GameplayEffectModifierMagnitude`（StructBase）。需要查阅该子结构如何赋值，可能需要：

```python
ge_cd = GE_Cooldown_Fireball()
ge_cd.DurationPolicy = ue.EGameplayEffectDurationType.HasDuration
# DurationMagnitude 通常通过 ScalableFloat 或 SetByCaller 设置
ge_cd.DurationMagnitude.ScalableFloatMagnitude.Value = 5.0  # 待验证
```

**API 搜到的**：
- `GameplayEffect.DurationPolicy: EGameplayEffectDurationType`
- `GameplayEffect.DurationMagnitude: GameplayEffectModifierMagnitude`
- `GameplayEffect.Modifiers: ArrayWrapper[GameplayModifierInfo]`

`GameplayEffectModifierMagnitude` 可能需要进一步搜索其属性。若 Python 设置不便，可改为在蓝图中配置 GE 蓝图。

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `GameplayEffect.DurationMagnitude` 属性赋值 | Python 中能否直接设冷却时长 | 待测试 |
| `GE_Cooldown` 通过 `CommitAbility()` 自动应用 | 冷却 GE 生效，阻止重复激活 | 待测试 |
| `GE_Cost` 扣减 `Mana` 属性 | Modifier 正确引用 AttributeSet 属性 | 待测试 |
| 冷却 Tag 通过 `ActivationBlockedTags` 阻断 | ASC 拒绝冷却期间的激活 | 待测试 |
| `GameplayEffectModifierMagnitude` 结构体 | 成员变量结构 | 需阅读完整 struct |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 技能图标上有倒计时数字 | Cooldown GE HasDuration → UI 监听 Tag 或 GE 剩余时间 |
| 法力条不足时技能图标变灰 | Cost GE 检查 Mana 是否足够 → `CommitAbility` 返回 False |
| 冷却期间按钮不可点 | `TryActivateAbility` 返回 False（ActivationBlockedTags 阻断） |
| 不同技能冷却时间不同 | 不同 GA 挂不同的 Cooldown GE 类 |

---

## 八、验证标准

- [ ] `GE_Cooldown_Fireball` 作为 Python 类可被 `GA_Fireball.CooldownGameplayEffectClass` 引用
- [ ] 激活 `GA_Fireball` 后，`ActivationBlockedTags` 阻止立即再次激活
- [ ] 冷却结束后（Duration 到期），技能可再次激活
- [ ] `GE_Cost_Fireball` 扣减 `Mana` 属性
- [ ] 法力不足时 `CommitAbility()` 返回 False

---

## 九、状态

🔲 待开始
