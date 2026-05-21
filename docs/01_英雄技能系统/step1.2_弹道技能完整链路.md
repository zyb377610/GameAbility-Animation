# Step 1.2: 弹道技能完整链路

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.2 状态改为 ✅。

---

## 一、目标

用 Python 子类化 `GameplayAbility` + `GameplayEffect`，创建一个火球弹道技能，实现从输入触发到命中伤害的完整链路。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.1_GAS角色骨架.md`

---

## 三、前置条件

- ✅ Step 1.1 完成：`BP_GASCharacter` 已有 ASC + `AttrSet_Base`
- 🔲 角色蓝图需配置 **InputAction** 绑定（可在蓝图或 Python 中绑定输入到 ASC 的 `AbilityActivated`）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/abilities/ga_fireball.py` | `GA_Fireball(ue.GameplayAbility)` | 火球技能类：播放动画、生成弹道 Actor、等待命中、应用伤害 GE |
| `Content/Scripts/gas/effects/ge_damage.py` | `GE_Damage_Fireball(ue.GameplayEffect)` | 伤害 GameplayEffect：`Instant` 策略，Modifier 扣减 `Health` |
| `Content/Scripts/gas/abilities/ga_fireball.py` | `BP_Projectile(ue.Actor)` | 弹道 Actor：移动、碰撞、命中后 Apply GE 到 Target |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/BP_GASCharacter` | Character（Step 1.1） | 添加 InputAction 绑定到 ASC |
| `Content/Blueprints/GAS/GE_Damage_Fireball` | 由 Python 生成的 GE 蓝图 | 伤害效果蓝图资产（如 Python 可直接创建设置，则不需要） |
| `Content/Blueprints/GAS/BP_Projectile` | Actor | 弹道可视化 Actor（Mesh + Collision） |

---

## 五、关键技术点

### 5.1 GameplayAbility 子类化（Nepy）

```python
class GA_Fireball(ue.GameplayAbility):
    # 类属性 = CDO 默认值
    AbilityTags: ue.GameplayTagContainer       # "Ability.Fireball"
    ActivationOwnedTags: ue.GameplayTagContainer
    CooldownGameplayEffectClass: ue.TSubclassOf[ue.GameplayEffect]  # 本步先不设
    CostGameplayEffectClass: ue.TSubclassOf[ue.GameplayEffect]      # 本步先不设

    def ActivateAbility(self):
        """重载：技能主逻辑"""
        # 1. CommitAbility() — 扣除消耗、启动冷却
        if not self.CommitAbility():
            self.EndAbility()
            return

        # 2. 播放 Montage
        task = ue.AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy(
            self, "None", montage_asset, 1.0)
        task.OnCompleted.Add(self.on_montage_completed)

        # 3. 生成弹道 Actor
        # 4. 等待命中 → Apply GE
```

**关键 API**（确认在 `__init__.pyi` 中存在）：

| API | 签名 |
|-----|------|
| `ue.AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy` | `(OwningAbility, TaskInstanceName, MontageToPlay, Rate=1.0, StartSection="None", bStopWhenAbilityEnds=True, AnimRootMotionTranslationScale=1.0, StartTimeSeconds=0.0, bAllowInterruptAfterBlendOut=False) -> AbilityTask_PlayMontageAndWait` |
| `GameplayAbility.CommitAbility()` | `def CommitAbility(self) -> bool` — 提交技能（扣除消耗+启动 CD） |
| `GameplayAbility.EndAbility()` | `def EndAbility(self) -> None` — 结束技能 |
| `GameplayAbility.ActivateAbility()` | `@typing.type_check_only` 虚方法，Python 可重载 |

### 5.2 GameplayEffect 伤害（Instant）

```python
class GE_Damage_Fireball(ue.GameplayEffect):
    DurationPolicy: ue.EGameplayEffectDurationType = ue.EGameplayEffectDurationType.Instant  # type: ignore
    # Modifiers 数组在构造时设置：扣减 Health 10 点
    Modifiers: ue.ArrayWrapper[ue.GameplayModifierInfo]
```

**EGameplayEffectDurationType 枚举**：
- `Instant = 0` — 立即生效
- `Infinite = 1` — 永久
- `HasDuration = 2` — 有限时长

### 5.3 ASC 的 GiveAbility 与激活

```python
asc = cdo.get_component_by_class(ue.AbilitySystemComponent)
# 授予技能
spec_handle = asc.GiveAbility(GA_Fireball, Level=1)
# 通过 Tag 激活
asc.TryActivateAbilitiesByTag(tag_container)
# 或直接按 class 激活
asc.TryActivateAbilityByClass(GA_Fireball)
```

**API 搜到的**：
- `GiveAbility(AbilityClass, Level=0, InputID=-1) -> GameplayAbilitySpecHandle`
- `TryActivateAbilityByClass(InAbilityToActivate, bAllowRemoteActivation=True) -> bool`
- `TryActivateAbilitiesByTag(GameplayTagContainer, bAllowRemoteActivation=True) -> bool`

### 5.4 Apply GE to Target

```python
# 在 ASC 上创建 EffectContext，然后生成 Spec
context = asc.MakeEffectContext()
spec_handle = asc.MakeOutgoingSpec(GE_Damage_Fireball, 1.0, context)
# 应用到目标 ASC
source_asc.ApplyGameplayEffectSpecToTarget(spec_handle, target_asc)
```

**API**：
- `MakeEffectContext() -> GameplayEffectContextHandle`
- `MakeOutgoingSpec(GameplayEffectClass, Level, Context) -> GameplayEffectSpecHandle`
- `ApplyGameplayEffectSpecToSelf(SpecHandle) -> ActiveGameplayEffectHandle`
- `ApplyGameplayEffectSpecToTarget(SpecHandle, Target) -> ActiveGameplayEffectHandle`

### 5.5 弹道 Actor（Python 脚本内嵌）

```python
class BP_Projectile(ue.Actor):
    def __init__(self):
        # 在 __init__ 添加 SphereComponent 做碰撞
        self.sphere = self.add_component_by_class(...)
        self.sphere.on_component_begin_overlap.Add(self.on_hit)

    def on_hit(self, overlap_comp, other_actor, ...):
        target_asc = other_actor.get_component_by_class(ue.AbilitySystemComponent)
        if target_asc:
            source_asc.ApplyGameplayEffectSpecToTarget(spec_handle, target_asc)
        self.DestroyActor()
```

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `GameplayAbility.ActivateAbility()` 重载 | Python 子类能否重载此蓝图事件 | 待测试 |
| `AbilityTask_PlayMontageAndWait` 的 `OnCompleted.Add()` | Python 回调绑定是否正常 | 待测试 |
| `MakeEffectContext()` 是否绑定 Instigator | 上下文中包含施法者引用 | 待测试 |
| `ApplyGameplayEffectSpecToTarget()` 跨 Actor | 弹道命中后 Apply 到其他 Actor 的 ASC | 待测试 |
| `GameplayEffect.Modifiers` 在 Python 中赋值 | 是否支持 `ArrayWrapper` 初始化 | 待测试（可能需要蓝图中设置） |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 钢铁侠掌心炮 — 按下左键，手臂伸直发射 | GA 输入绑定 → Montage → 弹道 |
| 惩罚者射击 — 持续按住连发 | GA + InputAction Hold |
| 弹道命中敌人闪红、跳伤害数字 | GE Instant → 修改 Health Attribute |
| 技能释放时有专属动画 | `AbilityTask_PlayMontageAndWait` |
| 弹道有飞行速度 | Python Tick + 移动 Component |

---

## 八、验证标准

- [ ] `GA_Fireball` 可被 `GiveAbility` + `TryActivateAbilityByClass` 激活
- [ ] `ActivateAbility()` 被调用，日志确认
- [ ] Montage 播放，`OnCompleted` / `OnBlendOut` 回调触发
- [ ] 弹道 Actor 生成并飞行
- [ ] 命中后 `ApplyGameplayEffectSpecToTarget` 被调用
- [ ] 目标 `Health` 属性被扣减（`GetAttributeSet` 验证）

---

## 九、状态

🔲 待开始
