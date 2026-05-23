# Step 1.3: 技能冷却与消耗

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.3 状态改为 ✅。

---

## 一、目标

给火球技能加上冷却（Cooldown）和法力消耗（Cost），通过 GameplayEffect 的 Duration 策略实现。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `.codemaker/codewiki/nepy/gas-pitfalls.md` ⚠️ **必读**：NePy GAS 开发踩坑指南（CDO限制、CommitAbility替代、蓝图加载等）
- `docs/01_英雄技能系统/step1.2_弹道技能完整链路.md`
- `docs/01_英雄技能系统/step1.1_GAS角色骨架.md`
- `docs/01_英雄技能系统/step1.3_实践总结.md`（本步实际实现记录）

---

## 三、前置条件

- ✅ Step 1.2 完成：`GA_Fireball` 可正常激活并打出伤害
- ✅ `AttrSet_Base` 有 `Mana` / `MaxMana` 属性

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/effects/ge_cooldown.py` | `GE_Cooldown_Fireball(ue.GameplayEffect)` | 冷却 GE Python 类（UClass 注册用，实际配置在蓝图） |
| `Content/Scripts/gas/effects/ge_cost.py` | `GE_Cost_Fireball(ue.GameplayEffect)` | 消耗 GE Python 类（UClass 注册用，实际配置在蓝图） |
| `Content/Scripts/gas/abilities/ga_fireball.py` | 更新 `GA_Fireball` | 新增 `try_commit_and_fire`、`configure_ge_classes`、`_load_ge` |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Blueprint/GAS/BP_GE_Cost_Fireball` | `GameplayEffect` | 消耗 GE：Instant + Modifier Mana Add -20 |
| `Content/Blueprint/GAS/BP_GE_Cooldown_Fireball` | `GameplayEffect` | 冷却 GE：HasDuration 5s + GE Components → 将标签赋予Actor → 添加到继承 → Cooldown.Fireball |

> ⚠️ 实际实现中，GE Modifiers 和 Tag 无法在 Python 中配置（ArrayWrapper 限制），必须通过蓝图资产承载。Python 类仅用于 UClass 注册。

---

## 五、关键技术点

### 5.1 冷却 GE 设计

`GE_Cooldown_Fireball`：
- `DurationPolicy = HasDuration`，`DurationMagnitude` = 5.0 秒
- 通过 GE Components → 将标签赋予Actor → 添加到继承 → `Cooldown.Fireball`
- 挂在 `GA_Fireball.CooldownGameplayEffectClass` 上
- 冷却期间，`asc.GetGameplayTagCount("Cooldown.Fireball")` > 0 → 拒绝激活

### 5.2 消耗 GE 设计

`GE_Cost_Fireball`：
- `DurationPolicy = Instant`
- Modifier: `AttrSet_Base.Mana`, `Add`, `-20`
- 挂在 `GA_Fireball.CostGameplayEffectClass` 上

### 5.3 AttrSet_Base 扩展

```python
class AttrSet_Base(ue.AttributeSet):
    Health = ue.uproperty(100.0)
    MaxHealth = ue.uproperty(100.0)
    Mana = ue.uproperty(100.0)       # 新增
    MaxMana = ue.uproperty(100.0)    # 新增
    AttackPower = ue.uproperty(10.0)
    MoveSpeed = ue.uproperty(600.0)
```

### 5.4 手动 Commit 模拟（关键）

由于 NePy CDO 限制，不能用 `CommitAbility()`，改为手动模拟：

```python
def try_commit_and_fire(self, asc, avatar):
    # 1. 检查冷却 Tag
    if asc.GetGameplayTagCount(cooldown_tag) > 0: return False
    # 2. 检查 Mana
    if attr_set.Mana < 20: return False
    # 3. Apply Cost GE
    asc.ApplyGameplayEffectSpecToSelf(cost_spec)
    # 4. Apply Cooldown GE
    asc.ApplyGameplayEffectSpecToSelf(cooldown_spec)
    # 5. 发射
    self.do_fireball(asc, avatar)
```

### 5.5 蓝图资产加载

PIE 中 `ue.LoadObject` 不可用，改用 `ue.FindObject(path_C)`。

---

## 六、已验证 API

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `GameplayEffect.DurationMagnitude.ScalableFloatMagnitude.Value = 5.0` | Python 中设冷却时长 | ✅ 蓝图配置更可靠 |
| `asc.GetGameplayTagCount(tag)` | 查询冷却状态 | ✅ |
| `asc.ApplyGameplayEffectSpecToSelf(spec)` | 对自身应用 GE | ✅ |
| `asc.MakeOutgoingSpec(ge_class, 1.0, ctx)` | 构造 GE Spec | ✅ |
| `ue.FindObject(path_C)` | PIE 中加载蓝图资产 | ✅ |
| `attr_set.Mana` | 读取属性值 | ✅ |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 技能图标上有倒计时数字 | Cooldown GE HasDuration → UI 监听 Tag 或 GE 剩余时间 |
| 法力条不足时技能图标变灰 | Cost GE 检查 Mana 是否足够 → 拒绝激活 |
| 冷却期间按钮不可点 | `GetGameplayTagCount` 检查冷却 Tag → 拒绝 |
| 不同技能冷却时间不同 | 不同 GA 挂不同的 Cooldown GE 类 |

---

## 八、验证标准

- [x] `GE_Cooldown_Fireball` 作为 Python 类可被 `GA_Fireball.CooldownGameplayEffectClass` 引用
- [x] 冷却期间拒绝再次激活（连续 6 次被挡）
- [x] 冷却 5 秒到期后技能可再次激活
- [x] `GE_Cost_Fireball` 扣减 `Mana` 属性（每次 -20）
- [x] 法力不足时（Mana < 20）拒绝激活

---

## 九、状态

✅ 已完成并 PIE 验证通过（2026-05-23）

> 详细实现记录见 [`step1.3_实践总结.md`](step1.3_实践总结.md)
