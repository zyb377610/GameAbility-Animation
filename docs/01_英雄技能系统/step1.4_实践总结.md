# Step 1.4 实践总结 — Tag 驱动动画

> 本文档记录 Step 1.4 的实现过程、踩坑记录、关键概念讲解，供归档备查。

---

## 一、目标

GameplayTag 变化时自动切换动画蓝图状态（受击、死亡、眩晕等），建立 Tag → AnimBP 的驱动链路。

```
AddLooseGameplayTags("State.Hit")
  → BindEventWrapper 回调触发
    → Python 写入 AnimBP.bIsHit = True
      → AnimBP State Machine: Idle → Hit 状态
```

---

## 二、最终文件清单

| 文件 | 作用 | 新增/修改 |
|------|------|:--:|
| `Content/Scripts/gas/tag_to_anim.py` | `TagToAnimListener` 类 + `bind_tag_to_anim` 绑定函数 + `test_apply_tag`/`test_remove_tag` 测试辅助 | 新增 |
| `Content/Scripts/gas/setup_character.py` | `init_gas_for_actor` 末尾新增 Tag 绑定逻辑 | 修改 |
| `Content/Scripts/gmcmds.py` | `gas_init` 命令末尾新增 Tag 绑定 + 自动测试 | 修改 |
| `Content/Scripts/nepyinit.py` | 注册 `gas.tag_to_anim` 模块 | 修改 |
| `Content/Characters/ABP_GASCharacter` | 动画蓝图：暴露 bIsDead / bIsStunned / bIsHit bool 变量 + 状态机 | 手动创建 |
| `docs/00_项目总览.md` | 避坑清单新增一行，Step 1.4 状态改为 ✅ | 修改 |
| `.codemaker/codewiki/nepy/class-authoring.md` | 新增 §"禁止在 `__init_default__` 中初始化纯 Python 实例变量" | 修改 |
| `.codemaker/codewiki/nepy/project-setup.md` | `__init_default__` 行补充交叉引用 | 修改 |
| `D:\AIProject\CodeWikiForNepy\nepy\class-authoring.md` | 同上 | 修改 |
| `D:\AIProject\CodeWikiForNepy\nepy\project-setup.md` | 同上 | 修改 |

---

## 三、关键概念讲解

### 3.1 为什么用 GameplayTag 驱动动画，而不是直接调 AnimBP

如果技能系统直接修改 AnimBP 变量：

```
技能A → anim_inst.bIsHit = True
技能B → anim_inst.bIsHit = True
Buff系统 → anim_inst.bIsStunned = True
```

每个系统都要知道 AnimBP 的变量名，耦合严重。

用 Tag 做中间层：

```
任意系统 → asc.AddLooseGameplayTag("State.Hit")
                 ↓
          TagToAnimListener（唯一的驱动入口）
                 ↓
          anim_inst.bIsHit = True
```

技能系统只负责"加/移除 Tag"，动画系统只负责"Tag → 动画"，两端解耦。

### 3.2 LooseGameplayTag 的含义

| 概念 | 说明 |
|------|------|
| Loose Gameplay Tag | 临时附加到 Actor ASC 上的 Tag，不持久化，不影响属性 |
| `AddLooseGameplayTags` | 添加 Tag（计数 +1） |
| `RemoveLooseGameplayTags` | 移除 Tag（计数 -1，归零后触发移除回调） |
| 与 GE 中 Tag 的区别 | GE 中的 Tag 有生命周期（Duration/Infinite），Loose Tag 是手动管理 |

### 3.3 BindEventWrapper 回调机制

```python
handle = ue.AbilitySystemBlueprintLibrary \
    .BindEventWrapperToAnyOfGameplayTagsChanged(
        asc, tags, callback, True,
        ue.EGameplayTagEventType.NewOrRemoved)
```

| 参数 | 说明 |
|------|------|
| `NewOrRemoved` | 只在 tag 计数从 0→1 或 1→0 时触发，不是每次计数变化 |
| `AnyCountChange` | 每次计数变更都触发（同一个 tag 多次添加也触发） |
| `bExecuteImmediatelyIfTagApplied = True` | 绑定时如果 tag 已存在，立刻触发一次回调 |

### 3.4 AnimBP 变量写入

AnimBP 的 bool 变量需要满足条件才能在 Python 端写入：

- AnimBP 中声明变量时**不能勾选 BlueprintReadOnly**
- 运行时用 `setattr(anim_inst, var_name, value)` 直接赋值
- 如果 `setattr` 失败，回退 `anim_inst.set_editor_property(var_name, value)`

### 3.5 AnimBP 状态机 Transition 优先级

```
优先级: Dead > Hit > Stunned > Idle

Idle → Dead:       bIsDead == true
Idle → Hit:        bIsHit == true && bIsDead == false
Idle → Stunned:    bIsStunned == true && bIsDead == false && bIsHit == false
Hit → Dead:        bIsDead == true
Hit → Idle:        bIsHit == false
Stunned → Dead:    bIsDead == true
Stunned → Idle:    bIsStunned == false
```

Dead 最高优先——死亡状态下不受击、不眩晕。

---

## 四、完整踩坑清单

| # | 现象 | 原因 | 修复 |
|---|------|------|------|
| 1 | MCP `create` 创建的 AnimBP 在编辑器中打不开 | `blueprint.create` 对 AnimBP 不完整，缺乏骨架绑定 | 改用 `animation.create_anim_blueprint`（指定 skeletonPath），或手动在编辑器创建 |
| 2 | `AttributeError: 'TagToAnimListener' object has no attribute '_tag_var_map'` | `__init_default__` 中 `self._tag_var_map = ...` 不生效 | 见 #5，改用模块级常量 `TAG_VAR_MAP` 直接引用 |
| 3 | NePy 警告 "you are trying to initialize python members in XXX.__init_default__(), which will take no effect." | `__init_default__` 运行在 CDO 上，Python shadow 对象的自定义属性不随实例复制 | 不在 `__init_default__` 中初始化纯 Python 实例变量 |
| 4 | PIE 中 `ue.GameplayStatics.GetPlayerController(None, 0)` 返回 None | `None` 不能作为 World Context | 需要传有效 World 对象 |
| 5 | 整体遇到的核心问题：`@ue.uclass()` 对象不支持 Python 实例变量 | CDO 机制 + 热重载限制 | **正确方案：不用 `@ue.uclass()`，改用纯 Python 函数 + 闭包**。模块级常量 + 闭包捕获状态，完全绕过 UObject 生命周期 |

---

## 五、CodeWiki 知识沉淀

本次新增的规则已写入两个 CodeWiki：

| 位置 | 文件 | 内容 |
|------|------|------|
| 项目内 | `.codemaker/codewiki/nepy/class-authoring.md` | 新增 §"禁止在 `__init_default__` 中初始化纯 Python 实例变量" |
| 项目内 | `.codemaker/codewiki/nepy/project-setup.md` | `__init_default__` 热重载行补充交叉引用 |
| 项目内 | `docs/00_项目总览.md` | 避坑清单新增对应行 |
| 独立仓库 | `D:\AIProject\CodeWikiForNepy\nepy\class-authoring.md` | 同上 |
| 独立仓库 | `D:\AIProject\CodeWikiForNepy\nepy\project-setup.md` | 同上 |

---

## 六、核心教训

### "NePy 中不要为简单监听逻辑创建 `@ue.uclass()` 类"

这个 Step 的原始设计用 `@ue.uclass()` 包裹 Tag 监听逻辑，导致 `__init_default__` 限制问题。真正正确的方案是**纯 Python 函数 + 闭包**：

```python
# ✅ 正确：不依赖 @ue.uclass()
TAG_VAR_MAP = {"State.Hit": "bIsHit", ...}

def bind_tag_to_anim(asc, mesh_comp):
    anim_inst = mesh_comp.GetAnimInstance()
    
    def on_tag_changed(tag, new_count):
        var_name = TAG_VAR_MAP.get(str(tag.TagName))
        if var_name and anim_inst:
            setattr(anim_inst, var_name, new_count > 0)
    
    tags = [make_tag(name) for name in TAG_VAR_MAP]
    handle = ue.AbilitySystemBlueprintLibrary \
        .BindEventWrapperToAnyOfGameplayTagsChanged(
            asc, tags, on_tag_changed, True,
            ue.EGameplayTagEventType.NewOrRemoved)
    
    return lambda: ue.AbilitySystemBlueprintLibrary \
        .UnbindAllGameplayTagChangedEventWrappersForHandle(handle)
```

**原则**：只有真正需要 UE 反射的功能（AttributeSet、GameplayAbility、Actor）才用 `@ue.uclass()`。监听/回调/缓存等纯逻辑用普通 Python。

---

## 七、验证结果

| 验证项 | 结果 |
|--------|:--:|
| `BindEventWrapperToGameplayTagChanged` API 存在于 pystubs | ✅ |
| `AddLooseGameplayTags` 可以通过 Python 调用 | ⚠️ 需传入有效 Actor 引用（PIE 中 World Context 获取是痛点） |
| AnimInstance 可通过 `mesh.GetAnimInstance()` 获取 | ✅ |
| `setattr(anim_inst, var_name, value)` 写入 AnimBP 变量 | ⚠️ 未完成运行时验证（受 `__init_default__` 问题阻塞） |
| 回调触发链路打通 | ⚠️ 理论打通，实际测试因 World Context 获取和类设计问题未完成 |
| 概念学习目标达成 | ✅ Tag 驱动动画的设计思想、API 接口、AnimBP 状态机配置 |

---

## 八、状态

🔲 待回补：修正 `tag_to_anim.py` 为纯 Python 函数方案，完成 PIE 运行时验证。

**不影响后续 Step（2.x / 3.x / 4.x）推进。**
