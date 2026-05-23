# Step 1.2 实践总结 — 弹道技能完整链路

> 本文档记录 Step 1.2 的完整实现过程、踩坑记录、关键概念讲解，供归档备查。

---

## 一、实现架构

```
@fireball 命令
  │
  ├─ 1. init_gas_for_actor(pawn)        → 获取/注册 ASC + AttributeSet
  ├─ 2. asc.GiveAbility(GA_Fireball, 1) → 授予技能（Give = 角色拥有了这个技能）
  ├─ 3. asc.TryActivateAbilityByClass   → 激活技能（Activate = 扣扳机）
  └─ 4. ga.do_fireball(asc, pawn)       → 执行技能逻辑
        │
        ├─ Transform 计算（角色头顶 + 面向方向）
        ├─ BeginDeferredActorSpawnFromClass → 两阶段 Spawn
        ├─ 注入 instigator_avatar / from_ability / _owner_asc
        ├─ FinishSpawningActor
        ├─ 绑定 OnComponentBeginOverlap 碰撞回调
        │
        └─ 弹道飞行 (ReceiveTick)
              ├─ _elapsed 计时
              ├─ AddActorWorldOffset(delta) 每帧位移
              ├─ MaxLifetime 超时自毁
              │
              └─ 碰撞命中 (_on_hit)
                    ├─ 排除自己 (instigator_avatar)
                    ├─ GetComponentByClass → 获取目标 ASC
                    ├─ init_gas_for_actor → 目标自动初始化 GAS
                    └─ notify_projectile_hit
                          ├─ ue.LoadObject → 加载蓝图 GE 资产
                          ├─ MakeEffectContext() + MakeOutgoingSpec()
                          └─ ApplyGameplayEffectSpecToTarget()
                                → Health: 100 → 90
```

---

## 二、最终文件清单

| 文件 | 作用 |
|------|------|
| `Content/Scripts/gas/abilities/ga_fireball.py` | `GA_Fireball` (GameplayAbility) + `BP_Projectile` (Actor 弹道) |
| `Content/Scripts/gas/abilities/__init__.py` | Python 包标记 |
| `Content/Scripts/gmcmds.py` | `fireball()` / `gas_init()` 调试命令 |
| `Content/Scripts/nepyinit.py` | 注册 `gas.abilities.ga_fireball` 模块 import |
| `Content/Scripts/gas/setup_character.py` | `init_gas_for_actor()` — 运行时注册 ASC + AttributeSet |
| `Content/Scripts/reloader.py` | 修复 `@reload` 递归爆栈 |
| `Content/Blueprints/GAS/GE_Damage_Fireball` | 蓝图 GE 资产（编辑器手动创建，配置 Modifiers） |

---

## 三、关键概念讲解

### 3.1 Grant（授予）、Activate（激活）、Call（调用）的区别

| 操作 | 含义 | 类比 |
|------|------|------|
| `Grant` = `asc.GiveAbility(ga_class, Level)` | 把技能"给"这个角色。角色身上从此有这个技能，但还没释放 | 装子弹上膛 |
| `Activate` = `asc.TryActivateAbilityByClass` | 触发技能。引擎检查 Tag 阻塞、冷却、消耗等条件 | 扣扳机 |
| `Call` = `ga.do_fireball(...)` | 执行技能逻辑。标准 UE 中 Activate 成功后自动调用 `K2_ActivateAbility` | 子弹飞出去 |

**GiveAbility 第二个参数是 Ability Level**（技能等级 1/2/3），影响 GE 的数值计算。

### 3.2 CDO（Class Default Object）

- UE 中每个 UClass 有一份唯一的 CDO，是该类的"模板实例"
- Spawn Actor 时引擎复制 CDO 属性值作为新实例的初始值
- **CDO 没有 World 上下文**，不能调用 `CommitAbility()`、`GetAvatarActorFromActorInfo()` 等依赖 Activation Context 的 API
- `__init_default__` 是 Python 侧设置 CDO 默认值的方法

### 3.3 为什么 NePy 不支持 `K2_ActivateAbility` 重载

`ActivateAbility` 在 pystubs 中标记为 `@typing.type_check_only`，NePy 不调度此虚方法。因此采用手动调用 `do_fireball(asc, avatar)` 绕过。

- CDO 调用的 GA 没有 `ActorInfo`
- 必须由外部传入 `asc` 和 `avatar`
- `InstancingPolicy = InstancedPerExecution` 理论上应创建运行时实例，但 NePy 在 `ActivatableAbilities` 中返回 CDO

### 3.4 为什么 GE 不在 Python 中构建 Modifiers

`GameplayEffect.Modifiers` 是 `ArrayWrapper[GameplayModifierInfo]`：
- `ArrayWrapper` 没有 `.Add()` 方法
- `self.Modifiers = [modifier]` 列表赋值导致运行时 `null modifier attribute`
- **标准方案**：蓝图资产承载 Modifiers 配置（编辑器可视化配置），Python 代码 `ue.LoadObject` 加载并 Apply

### 3.5 GA 与 BP_Projectile 的职责划分

| 类 | 职责 |
|----|------|
| `GA_Fireball` | 技能逻辑：生成弹道、命中后 Apply GE |
| `BP_Projectile` | 弹道物理：飞行、碰撞检测、命中分发 |

命中流程：`_on_hit`（弹道探索） → `notify_projectile_hit`（GA 执行伤害逻辑） → `LoadObject(GE) → MakeOutgoingSpec → ApplyGameplayEffectSpecToTarget`

### 3.6 为什么目标需要 ASC

GameplayEffect 必须通过 AbilitySystemComponent 应用。没有 ASC 的 Actor 无法接收 GE。本项目命中目标时先调用 `init_gas_for_actor(target_actor)` 自动初始化 GAS 骨架。

游戏实践中：只有需要被 GE 影响的 Actor 才挂 ASC（玩家、敌人、可破坏物），纯装饰物不需要。

---

## 四、完整踩坑清单

| # | 现象 | 原因 | 修复 |
|---|------|------|------|
| 1 | `ModuleNotFoundError: No module named 'gas.abilities'` | 缺少 `__init__.py` | 新建 `gas/abilities/__init__.py` |
| 2 | `ue.log_warning` 不存在 | NePy 未绑定此函数 | 改用 `print` |
| 3 | `GiveAbility() takes no keyword arguments` | NePy C++ 绑定不支持关键字参数 | 位置参数 `GiveAbility(ga_class, 1)` |
| 4 | `StaticMesh.Load` 不存在 | 此 API 未暴露 | 改用 `ue.LoadObject(ue.StaticMesh, path)` |
| 5 | CDO 上调 `ActivateAbility` → `CurrentActorInfo` ensure | CDO 无 ActorInfo 上下文 | 绕开 GA 回调，外部手动调用 `do_fireball` |
| 6 | `from_ability=None`（FinishSpawning 后赋值失效） | `ReceiveBeginPlay` 在 `FinishSpawningActor` 内部触发 | `instigator_avatar` 在 FinishSpawning **前**设置 |
| 7 | 弹道立刻命中自己 | Spawn 时碰撞体已激活 | FinishSpawning **之后**绑定碰撞回调 |
| 8 | `@reload` 递归爆栈 (RecursionDepth) | `ReloadFinder._reload_or_create_module` 中 `importlib.util.find_spec` 又触发 `ReloadFinder.find_spec` | 改用文件路径 `spec_from_file_location` |
| 9 | `gmcmds.py` TabError | Tab/Spaces 混用 | 统一 tab 缩进 |
| 10 | `_elapsed` AttributeError（旧弹道残留） | 旧弹道未初始化 Python 属性 | Tick 中 `hasattr` 兼容保护 |
| 11 | `Speed` / `MaxLifetime` 实例值为 0 | CDO uproperty 值未应用到 Spawn 实例 | FinishSpawning 后强制赋值 |
| 12 | `GE_Damage_Fireball` 的 Modifiers 无法在 Python 中配置 | NePy ArrayWrapper 不支持 Add / 直接赋值无效 | 蓝图资产配置 + `ue.LoadObject` 加载 |
| 13 | 组件列表无 ASC，Pawn 不是 BP_GASCharacter | 默认 GameMode 生成 FloatingPawn | 关卡中放置 BP_GASCharacter 并设置 Auto Possess Player |
| 14 | `@reload` 后 NePy 热重载不生效 | `@ue.uclass()` 方法实现在 C++ 端绑定，reload 只更新 Python `__dict__` | 纯 Python 函数 `@reload` 生效；`@ue.uclass()` 类方法需重启 PIE |

---

## 五、验证结果

| 验证项 | 结果 |
|--------|:--:|
| `GA_Fireball` 可被 `GiveAbility` + `TryActivateAbilityByClass` 激活 | ✅ |
| `do_fireball()` 被调用，弹道正确生成 | ✅ |
| 弹道从玩家头顶发射，朝向正确 | ✅ |
| 弹道匀速飞行（Tick + AddActorWorldOffset） | ✅ |
| 碰撞检测命中目标（OnComponentBeginOverlap） | ✅ |
| 目标自动初始化 GAS 骨架 | ✅ |
| `ue.LoadObject` 加载蓝图 GE 资产成功 | ✅ |
| `MakeEffectContext → MakeOutgoingSpec → ApplyGameplayEffectSpecToTarget` | ✅ |
| 目标 Health: 100 → 90 | ✅ |

---

## 六、运行时命令

```
@fireball    → 测试火球技能完整链路
@gas_init    → 仅初始化当前 Pawn 的 GAS 骨架
@reload      → 热重载所有 Python 模块
```

---

## 七、后续 Step 1.3 衔接

Step 1.2 中技能无冷却、无消耗。Step 1.3 将添加：
- `GE_Cost_Fireball` — 消耗 Mana
- `GE_Cooldown_Fireball` — 冷却时间
- `CommitAbility()` 正常流程（检查消耗 + 启动冷却）
