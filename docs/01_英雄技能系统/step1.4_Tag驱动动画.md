# Step 1.4: Tag 驱动动画

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.4 状态改为 ✅。

---

## 一、目标

GameplayTag 变化时自动切换动画蓝图状态（受击、死亡、眩晕等），实现 Tag → AnimBP 的驱动链路。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.1_GAS角色骨架.md`
- `docs/01_英雄技能系统/step1.2_弹道技能完整链路.md`

---

## 三、前置条件

- ✅ Step 1.1~1.2 完成：ASC + AttributeSet 就绪，Tag 系统可用
- 🔲 需准备 **AnimBP**（使用第三人称模板的 Animation Blueprint）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/tag_to_anim.py` | `TagToAnimListener` | 监听 ASC Tag 变化，将状态写入 AnimBP 变量 |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/ABP_GASCharacter` | AnimInstance | 动画蓝图，暴露 `bIsDead` / `bIsStunned` / `bIsHit` 等 bool 变量 |

---

## 五、关键技术点

### 5.1 ASC Tag 变化事件监听

UE5 提供 `AbilitySystemBlueprintLibrary` 的 Tag 绑定方法（已确认存在）：

```python
import ue

def on_tag_changed(tag: ue.GameplayTag, new_count: int):
    tag_name = str(tag.TagName)
    # 根据 tag 名设置 AnimBP 变量
    anim_instance = skeletal_mesh.GetAnimInstance()
    if tag_name == "State.Dead":
        anim_instance.set_editor_property("bIsDead", new_count > 0)
    elif tag_name == "State.Stunned":
        anim_instance.set_editor_property("bIsStunned", new_count > 0)
    elif tag_name == "State.Hit":
        anim_instance.set_editor_property("bIsHit", new_count > 0)

# 绑定
from ue import AbilitySystemBlueprintLibrary
AbilitySystemBlueprintLibrary.BindEventWrapperToGameplayTagChanged(
    asc,
    ue.GameplayTag("State.Dead"),
    on_tag_changed,
    bExecuteImmediatelyIfTagApplied=True,
    TagListeningPolicy=ue.EGameplayTagEventType.NewOrRemoved
)
```

### 5.2 API 确认清单

| API | 来源类 | 签名概览 |
|-----|--------|---------|
| `BindEventWrapperToGameplayTagChanged` | `AbilitySystemBlueprintLibrary` | `(ASC, Tag, Callable[[GameplayTag, int], None], bExecuteImmediately, TagListeningPolicy) -> GameplayTagChangedEventWrapperSpecHandle` |
| `BindEventWrapperToAnyOfGameplayTagsChanged` | `AbilitySystemBlueprintLibrary` | `(ASC, list[GameplayTag], Callable, ...) -> Handle` |
| `BindEventWrapperToAnyOfGameplayTagContainerChanged` | `AbilitySystemBlueprintLibrary` | `(ASC, TagContainer, Callable, ...) -> Handle` |
| `UnbindGameplayTagChangedEventWrapperForHandle` | `AbilitySystemBlueprintLibrary` | `(Tag, Handle) -> None` |
| `AddLooseGameplayTags` | `AbilitySystemBlueprintLibrary` | `(Actor, GameplayTagContainer, bShouldReplicate=False) -> bool` |
| `RemoveLooseGameplayTags` | `AbilitySystemBlueprintLibrary` | `(Actor, GameplayTagContainer, bShouldReplicate=False) -> bool` |
| `GetAnimInstance()` | `SkeletalMeshComponent` | `() -> AnimInstance` |

**`EGameplayTagEventType` 枚举**：
- `NewOrRemoved = 0` — 仅在 tag 新出现或完全移除时触发
- `AnyCountChange = 1` — 每次计数变化都触发

### 5.3 AnimBP 变量设置

通过 Python 直接写 AnimInstance 属性：

```python
anim_inst = mesh.GetAnimInstance()
anim_inst.set_editor_property("bIsDead", True)
# 或者在运行时：
anim_inst.bIsDead = True  # 如果 AnimBP 暴露为 BlueprintReadWrite
```

**注意**：`set_editor_property` 是编辑器工具方法，运行时更推荐直接属性赋值（前提是 AnimBP 中变量标记为 `BlueprintReadWrite`）。

### 5.4 Tag 驱动状态机

AnimBP 内部应建好 **State Machine**，根据 `bIsDead` / `bIsStunned` / `bIsHit` 等 bool 变量切换状态：
- Idle/Walk/Run → 正常 Locomotion
- Hit → 播放受击动画（可用 BlendPose 或 Slot）
- Dead → 死亡动画（Ragdoll 或倒地动画）
- Stunned → 眩晕 Loop

### 5.5 激活流程

```
[GE Apply] → [ASC Add Loose Tag "State.Hit"]
→ [BindEventWrapper 触发回调] → [Python: anim_inst.bIsHit = True]
→ [AnimBP State Machine: 切换到 Hit 状态]
```

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `BindEventWrapperToGameplayTagChanged` 在 Python 中回调 | Tag 变化时 Python 函数被调用 | 待测试 |
| `GameplayTag.__init__(str)` | `ue.GameplayTag("State.Dead")` 构造 Tag | 待测试 |
| `GetAnimInstance()` 运行时赋值变量 | `anim_inst.bIsDead = True` 是否立即反映 | 待测试（可能需 `BlueprintReadWrite` 标记） |
| `set_editor_property` 在运行时可用 | 运行时也能通过此方法设置 AnimBP 属性 | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 英雄受击时有受击动画（身体后仰） | Tag "State.Hit" → AnimBP Hit 状态 |
| 英雄死亡后倒地 / 消失动画 | Tag "State.Dead" → AnimBP Dead 状态 |
| 被控制技能命中后眩晕晃动 | Tag "State.Stunned" → AnimBP Stunned 状态 |
| 不同英雄受击动画不同 | 不同 AnimBP 内 Hit 状态动画不同 |
| Buff / Debuff 触发特殊表现 | Tag 驱动的 AnimNotify / GameplayCue |

---

## 八、验证标准

- [ ] `AddLooseGameplayTags` 添加 `"State.Hit"` Tag 到角色
- [ ] `BindEventWrapperToGameplayTagChanged` 回调被触发
- [ ] 回调中成功获取 `AnimInstance`
- [ ] `bIsHit` 变量从 Python 写入 AnimBP
- [ ] AnimBP 状态机切换到对应状态
- [ ] Tag 移除后变量重置为 False

---

## 九、状态

🔲 待开始
