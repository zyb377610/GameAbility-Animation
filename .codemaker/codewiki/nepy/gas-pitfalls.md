# NePy GAS 开发踩坑指南

> 记录 NePy 中使用 Gameplay Ability System 的已知限制和正确做法。
> 每次开新聊天做 GAS 相关开发，**必须先读本文档**。

---

## 一、CDO 限制：GA 不能调依赖 ActorInfo 的 API

`GiveAbility` 返回的 GA 实例是 **CDO（Class Default Object）**，没有 World 上下文和 ActorInfo。

| ❌ 不能调 | 原因 | ✅ 替代方案 |
|-----------|------|-------------|
| `CommitAbility()` | 需要 `CurrentActorInfo` | Python 手动检查条件 + `ApplyGameplayEffectSpecToSelf` |
| `EndAbility()` | 同上 | 不需要调用，CDO 不处于激活状态 |
| `K2_ActivateAbility()` | NePy 不支持重载（pystubs 标记 `type_check_only`） | Python 手动方法替代 |
| `GetAvatarActorFromActorInfo()` | CDO 无 ActorInfo | 从外部传入 `asc` 和 `avatar` |

### 正确做法：将 ASC 和 Avatar 通过参数传入

```python
# gmcmds.py — 命令层传入上下文
def fireball():
    pawn = ctrl.Pawn
    asc = init_gas_for_actor(pawn)  # 获取 ASC 实例
    # ...
    for spec in asc.ActivatableAbilities.Items:
        if spec.Ability and hasattr(spec.Ability, 'try_commit_and_fire'):
            spec.Ability.try_commit_and_fire(asc, pawn)  # 传入 ASC + Avatar

# ga_fireball.py — GA 方法接收参数
def try_commit_and_fire(self, asc, avatar):
    # self = CDO, asc = 运行时 ASC 实例, avatar = Pawn
    attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
    # 手动检查冷却、消耗……
```

---

## 二、CommitAbility 无法使用 — 手动模拟完整流程

GAS 标准流程中 `CommitAbility()` 会自动：
1. 检查 `ActivationBlockedTags` → 冷却中不激活
2. 检查 `CostGameplayEffectClass` → 消耗不足不激活
3. Apply Cost GE → 扣资源
4. Apply Cooldown GE → 加冷却 Tag

NePy 中必须在 Python 里手动实现：

```python
def try_commit_and_fire(self, asc, avatar):
    # 1. 手动检查冷却 Tag
    cooldown_tag = ue.GameplayTag()
    cooldown_tag.TagName = "Cooldown.Fireball"
    if asc.GetGameplayTagCount(cooldown_tag) > 0:
        print("冷却中")
        return False

    # 2. 手动检查资源
    attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
    if attr_set.Mana < 20:
        print("Mana 不足")
        return False

    # 3. 手动 Apply Cost GE
    if self.CostGameplayEffectClass:
        ctx = asc.MakeEffectContext()
        spec = asc.MakeOutgoingSpec(self.CostGameplayEffectClass, 1.0, ctx)
        asc.ApplyGameplayEffectSpecToSelf(spec)

    # 4. 手动 Apply Cooldown GE
    if self.CooldownGameplayEffectClass:
        ctx = asc.MakeEffectContext()
        spec = asc.MakeOutgoingSpec(self.CooldownGameplayEffectClass, 1.0, ctx)
        asc.ApplyGameplayEffectSpecToSelf(spec)

    # 5. 执行技能逻辑
    self.do_fireball(asc, avatar)
    return True
```

---

## 三、蓝图资产加载：PIE 中用 FindObject，不是 LoadObject

| 方式 | PIE 中可用？ | 说明 |
|------|:--:|------|
| `ue.LoadObject(cls, path)` | ❌ | 从磁盘加载，PIE 中报 "寻找对象失败" |
| `ue.FindClass("/Game/Path/Name")` | ⚠️ | 不带 `_C` 后缀，有时可用 |
| `ue.FindObject("/Game/Path/Name.Name_C")` | ✅ | 从内存对象注册表查找，PIE 中可靠 |

### 推荐加载顺序

```python
def _load_ge(self, bp_name):
    path = f"/Game/Blueprint/GAS/{bp_name}.{bp_name}_C"
    # 1. FindObject（PIE 可用）✓
    ge = ue.FindObject(path)
    if ge: return ge
    # 2. FindClass（不带 _C 后缀）
    ge = ue.FindClass(f"/Game/Blueprint/GAS/{bp_name}")
    if ge: return ge
    # 3. Python 类（仅当 import 链正常时可用）
    from gas.effects.ge_cooldown import GE_Cooldown_Fireball
    return GE_Cooldown_Fireball.Class()
```

---

## 四、GE Modifiers 无法在 Python 中配置

`GameplayEffect.Modifiers` 是 `ArrayWrapper[GameplayModifierInfo]`，NePy 不支持动态添加元素。

**❌ 以下全部无效**：
- `self.Modifiers.Add(...)` — ArrayWrapper 无 Add
- `self.Modifiers = [...]` — 列表赋值导致 null modifier attribute

**✅ 正确做法**：在编辑器中手动创建蓝图 GE 资产，用蓝图可视化配置 Modifiers，Python 代码通过 `FindObject` 加载并 Apply。

### 蓝图 GE 资产配置清单

| GE | DurationPolicy | Modifiers | 额外配置 |
|----|---------------|-----------|---------|
| GE_Damage | Instant | Health: Add -10 | — |
| GE_Cost_Fireball | Instant | Mana: Add -20 | — |
| GE_Cooldown_Fireball | HasDuration (5s) | 不需要 | GE Components → 将标签赋予Actor → 添加到继承 → `Cooldown.Fireball` |

---

## 五、GameplayTag 设置方式

在 Python CDO 中设置 Tag：

```python
# 创建 Tag
tag = ue.GameplayTag()
tag.TagName = "Cooldown.Fireball"

# 添加到 Tag 容器
self.ActivationBlockedTags.GameplayTags.Append(tag)
self.AbilityTags.GameplayTags.Append(tag)

# 查询 Tag 数量
count = asc.GetGameplayTagCount(tag)  # >0 表示拥有此 Tag
```

⚠️ `GameplayTag.TagName` 接受字符串，Tag 必须在引擎的 GameplayTagsManager 中存在或运行时动态创建。

---

## 六、ASC 运行时初始化

每个需要 GAS 的 Actor 必须手动调用初始化函数：

```python
def init_gas_for_actor(actor):
    asc = actor.GetComponentByClass(ue.AbilitySystemComponent.Class())
    if not asc:
        print("需在蓝图中有 ASC 组件")
        return None
    attr_set_class = AttrSet_Base.Class()
    if not asc.GetAttributeSet(attr_set_class):
        asc.InitStats(attr_set_class, None)
    return asc
```

**⚠️ AttributeSet 必须是 `@ue.uclass()` 类**，普通 Python 类即使继承 `ue.AttributeSet` 也不能被 `InitStats` 正确识别。

---

## 七、热重载与 @ue.uclass()

| 改动类型 | `@reload` 生效？ |
|---------|:--:|
| 纯 Python 函数 | ✅ |
| `@ue.uclass()` 类新增方法 | ✅ |
| `@ue.uclass()` 类已有方法实现修改 | ❌ |
| `@ue.uclass()` 类 `__init_default__` 修改 | ❌ |

修改 `@ue.uclass()` 类后：关闭 PIE → 重新 Play。不要依赖 `@reload`。

---

## 八、AttributeSet 属性读取

```python
attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
# ✅ 直接读属性值
mana = attr_set.Mana
health = attr_set.Health

# ⚠️ 修改属性应通过 GE，不要直接赋值
# attr_set.Mana = 50  # 不推荐，绕过 GAS 的预测/复制
```

---

## 九、完整开发 checklist

开新聊天做 GAS 开发时，确认以下事项：

- [ ] 蓝图已配置 `AbilitySystemComponent` 组件
- [ ] `@ue.uclass()` 的 AttributeSet 类在 `nepyinit.on_init()` 中 import
- [ ] 所有 GE 类（Python 或蓝图）在 `nepyinit.on_init()` 中 import
- [ ] GE 的 Modifiers/Tag 在蓝图资产中配置，不在 Python 中
- [ ] GA 不调 `CommitAbility()`，手动实现资源检查逻辑
- [ ] ASC 和 Avatar 通过参数传入 GA 方法
- [ ] 蓝图资产加载用 `ue.FindObject(path_C)` 优先
- [ ] 日志用 `print` 或 `ue.Log/ue.LogWarning/ue.LogError`（大写开头）
- [ ] 修改 `@ue.uclass()` 类后重启 PIE
