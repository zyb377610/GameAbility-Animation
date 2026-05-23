# Step 1.3 实践总结 — 技能冷却与消耗

> 本文档记录 Step 1.3 的完整实现过程、踩坑记录、关键概念讲解，供归档备查。

---

## 一、实现架构

```
@fireball 命令
  │
  ├─ 1. init_gas_for_actor(pawn)        → 获取/注册 ASC + AttributeSet
  ├─ 2. asc.GiveAbility(GA_Fireball, 1) → 授予技能
  ├─ 3. spec.Ability.try_commit_and_fire(asc, pawn) → 手动 Commit 模拟
  │     │
  │     ├─ configure_ge_classes()        → FindObject 加载蓝图 GE 资产
  │     │   ├─ CooldownGameplayEffectClass = BP_GE_Cooldown_Fireball
  │     │   └─ CostGameplayEffectClass = BP_GE_Cost_Fireball
  │     │
  │     ├─ 检查冷却 Tag                  → asc.GetGameplayTagCount("Cooldown.Fireball")
  │     │   └─ >0 → 返回 False（冷却中）
  │     │
  │     ├─ 检查 Mana                     → attr_set.Mana >= 20
  │     │   └─ <20 → 返回 False（Mana 不足）
  │     │
  │     ├─ Apply Cost GE 到自身          → Mana -20
  │     │   ├─ asc.MakeEffectContext()
  │     │   ├─ asc.MakeOutgoingSpec(CostGE, 1.0, ctx)
  │     │   └─ asc.ApplyGameplayEffectSpecToSelf(spec)
  │     │
  │     ├─ Apply Cooldown GE 到自身      → 5s 冷却 + 添加 Tag
  │     │   ├─ asc.MakeEffectContext()
  │     │   ├─ asc.MakeOutgoingSpec(CooldownGE, 1.0, ctx)
  │     │   └─ asc.ApplyGameplayEffectSpecToSelf(spec)
  │     │        → 引擎自动: Duration 到期 → 移除 GE + 移除 Tag
  │     │
  │     └─ do_fireball(asc, avatar)      → 发射弹道（同 Step 1.2）
  │           ├─ Transform 计算
  │           ├─ 两阶段 Spawn 弹道
  │           ├─ FinishSpawning → 绑定碰撞
  │           └─ 碰撞命中 → FindObject(GE_Damage) → ApplyToTarget
  │                 → 目标 Health -10
```

---

## 二、最终文件清单

| 文件 | 作用 | 新增/修改 |
|------|------|:--:|
| `Content/Scripts/gas/effects/__init__.py` | Python 包标记 | 新增 |
| `Content/Scripts/gas/effects/ge_cooldown.py` | `GE_Cooldown_Fireball` Python 类（注册用） | 新增 |
| `Content/Scripts/gas/effects/ge_cost.py` | `GE_Cost_Fireball` Python 类（注册用） | 新增 |
| `Content/Scripts/gas/setup_character.py` | `AttrSet_Base` 新增 `Mana`/`MaxMana`（各 100） | 修改 |
| `Content/Scripts/gas/abilities/ga_fireball.py` | `GA_Fireball` 新增 `try_commit_and_fire`、`configure_ge_classes`、`_load_ge` | 修改 |
| `Content/Scripts/gmcmds.py` | `@fireball` 改为走 `try_commit_and_fire`；新增 `@fireball_status` | 修改 |
| `Content/Scripts/nepyinit.py` | 注册 `gas.effects.*` 模块；修复 `ue.log_warning`→`print` | 修改 |
| `Content/Blueprint/GAS/BP_GE_Cooldown_Fireball` | 冷却 GE 蓝图资产（Duration=5s + 将标签赋予Actor: Cooldown.Fireball） | 手动创建 |
| `Content/Blueprint/GAS/BP_GE_Cost_Fireball` | 消耗 GE 蓝图资产（Instant + Modifier: Mana Add -20） | 手动创建 |
| `Content/Blueprint/GAS/GE_Damage_Fireball` | 伤害 GE（Step 1.2 已有，路径修正） | 不变 |

---

## 三、关键概念讲解

### 3.1 为什么不能用 CommitAbility()：CDO 限制的根因

UE GAS 标准流程中，一次技能激活的生命周期：

```
TryActivateAbilityByClass
  → ASC 创建 GA 运行实例（非 CDO！），绑定 ActorInfo
  → GA::ActivateAbility()
    → CommitAbility()     ← 检查资源/冷却，Apply Cost/Cooldown GE
    → K2_ActivateAbility()  ← 暴露给蓝图的业务逻辑钩子
    → EndAbility()
```

NePy 环境的两个断点：
1. **K2_ActivateAbility 不能重载**：pystubs 标记为 `@typing.type_check_only`，引擎不调度 Python 实现
2. **GiveAbility 返回 CDO**：`asc.ActivatableAbilities.Items` 中的 GA 实例是 CDO，没有 `ActorInfo`，`CommitAbility()` 报 `CurrentActorInfo is null`

因此 Step 1.2 和 Step 1.3 都采用外部手动调用的策略——将 `asc` 和 `avatar` 作为参数传入，绕过引擎的实例化/回调机制。

### 3.2 手动 Commit 模拟 vs 引擎 CommitAbility 对比

| | 引擎 CommitAbility() | 我们的 try_commit_and_fire() |
|------|------|------|
| 冷却检查 | 检查 `ActivationBlockedTags` | `asc.GetGameplayTagCount("Cooldown.Fireball")` |
| 消耗检查 | `CommitAbilityCost` → Apply Cost GE | 手动读 `attr_set.Mana`；手动 Apply Cost GE |
| 冷却启动 | `CommitAbilityCooldown` → Apply Cooldown GE | 手动 Apply Cooldown GE |
| 失败处理 | 自动调用 `EndAbility` + 通知客户端 | 返回 `False`，调用方自行处理 |

效果等价，只是检查步骤在 Python 侧显式实现。

### 3.3 GE 的正确理解：数据模板 vs 执行逻辑

**GE 是什么**：一个数据配置模板，不是可执行的函数。

```
BP_GE_Cost_Fireball（蓝图资产 = 数据模板）
├── 持续时间策略 = 瞬时（Instant）
└── 修饰符
    └── [0] 属性 = AttrSet_Base.Mana
            运算 = 加（Add）
            大小 = -20.0
```

**GE 如何生效**：三步流程

| 步骤 | API | 作用 |
|------|-----|------|
| 构建上下文 | `asc.MakeEffectContext()` | 记录"谁引发了效果" |
| 创建 Spec | `asc.MakeOutgoingSpec(GE类, 等级, 上下文)` | 把模板实例化为数据包 |
| 应用 Spec | `asc.ApplyGameplayEffectSpecToSelf(spec)` | ASC 解析 Spec → 修改属性 → 管理 Duration |

我们只负责构造 Spec 并 Apply，**ASC 内部负责解析 Modifiers 和修改属性值**。

### 3.4 Cost GE 和 Cooldown GE 的设计区别

| | Cost GE | Cooldown GE |
|------|------|------|
| Duration 策略 | Instant（立即生效即结束） | HasDuration（持续 5 秒） |
| 修改属性 | Mana -20 | 不修改属性 |
| Tag | 不需要 | 将标签赋予Actor: Cooldown.Fireball |
| 生命周期 | Apply → 立即扣 Mana → 结束 | Apply → 加 Tag → 5s 后引擎自动移除 |

冷却为什么用 Tag 机制：`asc.GetGameplayTagCount` 查询 Tag 比维护定时器更可靠。5 秒后引擎自动移除 GE 和 Tag，不需要手动清理。

### 3.5 蓝图资产加载：FindObject vs LoadObject

| 方式 | PIE 中可用？ | 原因 |
|------|:--:|------|
| `ue.LoadObject(cls, path)` | ❌ | 从磁盘加载，PIE 运行时环境不可用 |
| `ue.FindObject(path_C)` | ✅ | 从内存对象注册表查找 |
| `ue.FindClass(path)`（不带 _C） | ⚠️ | 有时可用，不稳定 |
| Python 类 `.Class()` | ⚠️ | 仅当 import 链完整且未被 @reload 破坏时可用 |

Step 1.2 中 `ue.LoadObject` 能用是因为它在编辑器模式下测试的。Step 1.3 在 PIE 中验证时发现 LoadObject 失败，最终 `FindObject` 成功。

### 3.6 GA 与 ASC 的角色分离

```python
def try_commit_and_fire(self, asc, avatar):
    # self   = GA_Fireball CDO（技能模板，持有 GE 类引用）
    # asc    = 运行时 ASC 实例（角色的技能容器，执行 GE Apply）
    # avatar = Pawn（角色实体，提供 Transform）
```

三者分离是绕过 CDO 限制的核心设计：
- CDO 提供 `CooldownGameplayEffectClass` / `CostGameplayEffectClass` 引用（这些是类级别的属性，CDO 可以持有）
- ASC 提供 `MakeEffectContext` / `MakeOutgoingSpec` / `ApplyToSelf`（这些需要运行时上下文）
- Avatar 提供位置/朝向信息

---

## 四、完整踩坑清单（Step 1.3 新增）

| # | 现象 | 原因 | 修复 |
|---|------|------|------|
| 1 | `ue.log_warning` AttributeError | NePy 不存在此函数，正确 API 是 `ue.Log`/`ue.LogWarning`/`ue.LogError`（大写 L） | 改用 `print` |
| 2 | `ue.log` AttributeError | 同上 | 同上 |
| 3 | `CommitAbility()` → `CurrentActorInfo` ensure | CDO 无 ActorInfo 上下文 | 手动模拟：检查 Tag + Mana + Apply GE |
| 4 | `ue.LoadObject` 在 PIE 中返回 None | PIE 环境不从磁盘加载资产 | 改用 `ue.FindObject(path_C)` |
| 5 | `ue.FindClass` 加载蓝图 GE 不稳定 | 不带 `_C` 后缀时有时不可用 | 优先用 `FindObject` |
| 6 | Python `GE_Cooldown_Fireball` 的 `GEComponents.Append` 可能失败 | ArrayWrapper 限制，同 Modifiers | 蓝图资产配置 Tag |
| 7 | `@reload` 后 `ga_fireball.py` 新代码不生效 | `@ue.uclass()` 已有方法变更需重启 PIE | 关闭 PIE → 重新 Play |
| 8 | `@reload` 后 `try_commit_and_fire` 方法丢失 | 编辑时缩进混用（Tab/空格）+ old_string 漂移导致函数头缺失 | 精确匹配替换，统一 4 空格 |
| 9 | 蓝图 GE 路径 `/Game/Blueprints/` vs `/Game/Blueprint/` 不匹配 | 项目实际路径是 `Blueprint`（单数） | 修正路径 |
| 10 | `GE_Damage_Fireball` 加载失败（同 #4） | 路径错误 + LoadObject PIE 不可用 | 修正路径 + FindObject |
| 11 | `gmcmds.debug()` 在 PIE 中 `ModuleNotFoundError` | 纯 py 执行环境和 NePy 环境不同 | NePy 的 `@` 命令在 `nepyinit.on_init` 中注册，不需要额外调用 `gmcmds.debug()` |
| 12 | 冷却期间 `@fireball` 不报冷却中（第一版 Tag 未添加） | Python CDO 中 `TargetTagsGameplayEffectComponent` 配置失败 | 蓝图资产中配置"将标签赋予Actor" |

---

## 五、验证结果

| 验证项 | 结果 |
|--------|:--:|
| `AttrSet_Base` 新增 Mana/MaxMana（各 100） | ✅ |
| `try_commit_and_fire` 手动检查冷却 Tag | ✅ |
| 连续 `@fireball` 在冷却期间被拒绝 | ✅ |
| 冷却 5 秒到期后技能可再次激活 | ✅ |
| Cost GE 扣减 Mana（每次 -20） | ✅ |
| Mana 不足时 `@fireball` 被拒绝 | ✅ |
| `@fireball_status` 显示当前属性状态 | ✅ |
| 弹道发射和 GE 伤害（沿用 Step 1.2 逻辑） | ✅ |
| 目标 Health 连续下降（100→90→80→70→60） | ✅ |

### 完整测试序列

```
@fireball  # 1: Mana 100→80, 弹道 ✅
@fireball  # 2: Mana 80→60, 弹道 ✅
@fireball  # 3: Mana 60→40, 弹道 ✅
@fireball  # 4-9: 冷却中 (6次被挡) ✅
@fireball  # 10: 冷却到期, Mana 40→20 ✅
@fireball  # 11: Mana 20→0 ✅
@fireball  # 12: 冷却中 ✅
@fireball  # 13: Mana 不足 (0/100) ✅
```

---

## 六、运行时命令

```
@fireball         → 测试火球技能完整链路（含冷却+消耗）
@fireball_status  → 查看当前 Mana/Health/冷却状态
@gas_init         → 仅初始化当前 Pawn 的 GAS 骨架
@reload           → 热重载所有 Python 模块
```

---

## 七、与 Step 1.2 的对比

| | Step 1.2 | Step 1.3 |
|------|------|------|
| GE 数量 | 1（伤害） | 3（伤害 + 消耗 + 冷却） |
| GA 入口方法 | `do_fireball` | `try_commit_and_fire` → `do_fireball` |
| 冷却 | 无 | 5s Duration + Cooldown.Fireball Tag |
| 消耗 | 无 | Mana -20/次 |
| GE 加载 | `ue.LoadObject`（编辑器可用） | `ue.FindObject`（PIE 可用） |
| 核心策略 | 手动调 `do_fireball` 绕过 `K2_ActivateAbility` | 手动模拟 `CommitAbility` 全部流程 |

---

## 八、CodeWiki 知识沉淀

Step 1.3 的踩坑经验已同步写入两个 CodeWiki：

| 位置 | 文件 | 内容 |
|------|------|------|
| 项目内 | `.codemaker/codewiki/nepy/gas-pitfalls.md` | GAS 开发踩坑指南（CDO、Commit、蓝图加载、GE Modifiers、Tag 等） |
| 项目内 | `.codemaker/codewiki/nepy/project-setup.md` | 修正日志 API，热重载第 5 条 |
| 项目内 | `.codemaker/codewiki/nepy/usage-patterns.md` | 修正日志 API |
| 独立仓库 | `D:\AIProject\CodeWikiForNepy\nepy\gas-pitfalls.md` | 同上 |
| 独立仓库 | `D:\AIProject\CodeWikiForNepy\nepy\project-setup.md` | 同上 |
| 独立仓库 | `D:\AIProject\CodeWikiForNepy\nepy\usage-patterns.md` | 同上 |
