# Step 4.2: Montage 播放 + AnimNotify 伤害判定

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 4.2 状态改为 ✅。

---

## 一、目标

Python 侧通过 GA 播放技能 Montage，并在关键帧通过 AnimNotify 触发伤害判定（命中窗口）。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.2_弹道技能完整链路.md`
- `docs/05_技能动画表现/01_技能动画表现概述.md`

---

## 三、前置条件

- ✅ Step 1.2 完成：`GA_Fireball` 可播放 Montage + 生成弹道
- 🔲 需准备技能 Montage（`Content/Characters/AM_Attack`），内含 AnimNotify 标记

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/abilities/ga_fireball.py` | 更新 `GA_Fireball` | 绑定 Montage 的 Notify 回调，在命中窗口执行伤害判定 |
| 新增：`Content/Scripts/gas/notify_handler.py` | `NotifyHandler` | 全局 AnimNotify 事件监听器（不依赖 GA 实例） |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/AM_Attack` | AnimMontage | 攻击 Montage，含 `AN_DamageWindow`（AnimNotify） |

---

## 五、关键技术点

### 5.1 AnimNotify 类型确认

`__init__.pyi` 中的 AnimNotify 相关类：

| 类 | 用途 |
|-----|------|
| `AnimNotify` | 基类，有 `Received_Notify(MeshComp, Animation, EventReference) -> bool` |
| `AnimNotifyState` | 持续型 Notify，有 `Received_NotifyBegin / Tick / End` |
| `AnimNotify_GameplayCue` | GAS 专用：`GameplayCue: GameplayCueTag` |
| `AnimNotify_PlayMontageNotify` | Montage Notify：`NotifyName: Name` |
| `AnimNotify_PlayNiagaraEffect` | Niagara 特效 Notify |

### 5.2 方案 A：自定义 AnimNotify 类（Python 子类化）

```python
import ue

class AN_DamageWindow(ue.AnimNotify):
    """
    伤害窗口 Notify：在动画关键帧触发
    放在 Montage 中，Notify 触发时调用 ASC 的伤害判定
    """

    def Received_Notify(self, mesh_comp: ue.SkeletalMeshComponent,
                         animation: ue.AnimSequenceBase,
                         event_reference: ue.AnimNotifyEventReference) -> bool:
        """重载：Notiy 触发时调用"""
        owner = mesh_comp.GetOwner()
        if not owner:
            return False

        asc = owner.get_component_by_class(ue.AbilitySystemComponent)
        if asc:
            # 在这里执行伤害判定逻辑
            # 例如：检查当前激活的 GA，应用伤害 GE
            ue.log("[AN_DamageWindow] 伤害判定触发")
            return True
        return False
```

**关键**：Nepy 是否支持 Python 子类化 `AnimNotify` 并注册到反射系统？需要实测。

### 5.3 方案 B：Montage 事件回调（更可靠）

通过 `AbilityTask_PlayMontageAndWait` 不直接提供 Notify 回调。替代方案：

**使用 `AnimNotify_PlayMontageNotify` + `PlayMontageNotify` 委托**：

`AnimInstance` 上有以下 Montage 相关委托：
- `OnMontageBlendingOut`
- `OnMontageEnded`
- `OnMontageStarted`

```python
def setup_montage_callbacks(ga_instance, anim_instance):
    """绑定 Montage 事件"""
    def on_montage_ended(montage, interrupted):
        if not interrupted:
            # Montage 正常完成 → 技能结束
            ga_instance.EndAbility()

    anim_instance.OnMontageEnded.Add(on_montage_ended)
```

`AnimInstance` API（确认存在）：
- `OnMontageBlendingOut: DynamicMulticastDelegateWrapper[[AnimMontage, bool], None]`
- `OnMontageBlendedIn: DynamicMulticastDelegateWrapper[[AnimMontage], None]`
- `OnMontageStarted: DynamicMulticastDelegateWrapper[[AnimMontage], None]`
- `OnMontageEnded: DynamicMulticastDelegateWrapper[[AnimMontage, bool], None]`
- `OnAllMontageInstancesEnded: DynamicMulticastDelegateWrapper[[], None]`

### 5.4 方案 C：GA 内主动检测（最稳定）

不使用 AnimNotify，在 GA 激活后通过定时器在命中窗口检测：

```python
class GA_Fireball(ue.GameplayAbility):
    def ActivateAbility(self):
        if not self.CommitAbility():
            self.EndAbility()
            return

        # 播放 Montage
        task = ue.AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy(...)
        task.OnCompleted.Add(self._on_montage_done)

        # 延迟 0.5s 后执行伤害判定（命中窗口开始）
        # 使用 AbilityTask 或 world timer
        self._schedule_damage_window()

    def _schedule_damage_window(self):
        # 通过 ASC 的定时机制（若无可简化为 Python 的 threading.Timer）
        import threading
        threading.Timer(0.5, self._do_damage_check).start()

    def _do_damage_check(self):
        # 伤害判定逻辑
        pass

    def _on_montage_done(self):
        self.EndAbility()
```

### 5.5 AbilityTask_WaitGameplayTag 替代方案

也可以在 Montage 中通过 `AnimNotify_GameplayCue` 添加 GameplayCue Tag，然后用 `AbilityTask_WaitGameplayTagAdded` 等待 Tag：

```python
task = ue.AbilityTask_WaitGameplayTagAdded.WaitGameplayTagAdd(
    self,
    ue.GameplayTag("GameplayCue.Attack.HitWindow"),
    OnlyTriggerOnce=True
)
task.Added.Add(self._on_hit_window_open)
```

**API 确认**：
- `AbilityTask_WaitGameplayTagAdded.WaitGameplayTagAdd(OwningAbility, Tag, InOptionalExternalTarget=None, OnlyTriggerOnce=False) -> AbilityTask_WaitGameplayTagAdded`

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| Python 子类化 `AnimNotify.Received_Notify` | Nepy 注册到反射，Montage 中可用 | 待测试（高风险，Nepy 可能不支持） |
| `AnimNotify_GameplayCue` Python 使用 | 在 Montage 中添加 Cue | 待测试 |
| `AbilityTask_WaitGameplayTagAdded` 在 GA 中使用 | 等待 GameplayCue Tag 触发 | 待测试 |
| `DynamicMulticastDelegateWrapper.Add(callback)` | Python 绑定委托 | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 攻击动画挥到特定帧才出伤害 | AnimNotify 命中窗口 |
| 攻击动作中途被打断则无伤害 | Notify 未触发 / Montage 中断 |
| 技能特效在特定帧播放（爆炸、闪光） | AnimNotify_PlayNiagaraEffect |
| 不同技能命中窗口时机不同 | 不同 Montage 不同 Notify 位置 |

---

## 八、验证标准

- [ ] Montage 播放时 AnimNotify 被触发
- [ ] 伤害窗口期正确执行伤害判定
- [ ] Montage 被打断时伤害判定不执行
- [ ] Montage 正常结束 → 技能结束
- [ ] AnimNotify 回调中可获取 ASC 和 Owner

---

## 九、状态

🔲 待开始
