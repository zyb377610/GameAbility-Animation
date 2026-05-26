# Step 3.2: Motion Warping 突进技能

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 3.2 状态改为 ✅。

---

## 一、目标

实现突进技能（Dash）锁定目标位置，通过 Motion Warping 动态调整 RootMotion 使角色精确到达目标点。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.2_弹道技能完整链路.md`
- `docs/04_GroundMotion/01_GroundMotion概述.md`

---

## 三、前置条件

- ✅ Step 1.1~1.2 完成：GAS + 技能系统就绪
- 🔲 角色需挂载 **MotionWarpingComponent**（在蓝图中手动添加）
- 🔲 需准备带有 **RootMotion** 的突进 Montage（如向前冲刺动画）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/abilities/ga_dash.py` | `GA_Dash(ue.GameplayAbility)` | 突进技能：Play Montage + 设置 Motion Warping Target |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/BP_GASCharacter` | Character（更新） | 添加 MotionWarpingComponent |
| `Content/Characters/AM_Dash` | AnimMontage | 突进动画（含 RootMotion），带 MotionWarping SyncPoint Notify |

---

## 五、关键技术点

### 5.1 MotionWarpingComponent

`MotionWarpingComponent` **不在** `__init__.pyi` 中（搜索无结果）。这意味着：

- **方案 A（推荐）**：在蓝图中手动添加 `MotionWarpingComponent`，通过 `AnimNotifyState_MotionWarping` 设置 Warp Target
- **方案 B**：通过 Python 尝试 `add_component_by_class`，但类可能未暴露到 Nepy

**结论**：Motion Warping 的核心配置在蓝图端完成：
1. 蓝图中添加 MotionWarpingComponent
2. 突进 Montage 中添加 `MotionWarpingAnimNotifyState`
3. Python 侧在技能激活时设置 Warp Target 位置

### 5.2 Python 侧逻辑

```python
import ue

class GA_Dash(ue.GameplayAbility):
    """
    突进技能：向锁定目标方向冲刺
    必须在蓝图中配合 MotionWarpingComponent
    """
    AbilityTags: ue.GameplayTagContainer  # "Ability.Dash"

    def ActivateAbility(self):
        if not self.CommitAbility():
            self.EndAbility()
            return

        # 获取 Avatar Actor
        avatar = self.GetAvatarActorFromActorInfo()
        if not avatar:
            self.EndAbility()
            return

        # 获取 MotionWarpingComponent（从蓝图中预先挂载）
        warp_comp = avatar.get_component_by_class(ue.MotionWarpingComponent)  # type: ignore
        if warp_comp:
            # 设置 Warp Target：向前 600 单位
            forward = avatar.GetActorForwardVector()
            target_location = avatar.GetActorLocation() + forward * 600.0
            target_rotation = avatar.GetActorRotation()

            # 添加/更新 Warp Target
            # 注意：具体 API 签名需要验证
            warp_comp.AddOrUpdateWarpTargetFromLocation(
                "DashTarget",     # Warp Target Name
                target_location    # 目标位置
            )

        # 播放突进 Montage
        dash_montage = ue.load_object(ue.AnimMontage, "/Game/Characters/AM_Dash")
        task = ue.AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy(
            self, "None", dash_montage, 1.0, "None", True, 1.0
        )
        task.OnCompleted.Add(self._on_dash_completed)

    def _on_dash_completed(self):
        self.EndAbility()
```

### 5.3 MotionWarpingAnimNotifyState

Montage 编辑器中：
1. 在 Montage 上右键 → Add Notify State → `Motion Warping`
2. 设置 Warp Target Name（如 `"DashTarget"`）
3. 设置 Warp Type：`Simple Warp`（位移）或 `Rotation Only` 等

### 5.4 找不到 API 的应对

如果 Python 无法直接操作 `MotionWarpingComponent`（Nepy 未暴露），方案为：

- 所有 Motion Warping 配置在蓝图中完成（添加 Component、设置 NotifyState）
- Python 侧通过 `GA_Dash` 技能管理技能生命周期（播放 Montage、等待完成）
- Warp Target 的更新可通过蓝图事件系统间接实现

### 5.5 API 确认

| API | 来源 | 状态 |
|-----|------|------|
| `AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy` | AbilityTask | ✅ 确认 |
| `GameplayAbility.ActivateAbility()` | GameplayAbility | ✅ 确认 |
| `GameplayAbility.CommitAbility()` | GameplayAbility | ✅ 确认 |
| `GameplayAbility.EndAbility()` | GameplayAbility | ✅ 确认 |
| `MotionWarpingComponent` | **未找到** | ❌ 未在 pystubs 中 |

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `add_component_by_class(ue.MotionWarpingComponent)` | 是否能动态添加 MotionWarpingComponent | 待测试（类可能不存在） |
| `MotionWarpingComponent.AddOrUpdateWarpTargetFromLocation` | 设置 Warp Target | 待测试（需先确认类存在） |
| `GetAvatarActorFromActorInfo()` | GA 中获取 Avatar Actor | 待测试 |
| Montage RootMotion 是否通过 Nepy 传递 | 已确认 Montage 播放正常（Step 1.2 验证） | 继测 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 黑豹冲刺锁定目标位置 | Motion Warping → Target Location |
| 蜘蛛侠荡秋千到达精确落点 | Motion Warping 多阶段 Warp |
| 突进技能中角色不会穿过或停太远 | Warp 确保终点精确 |
| 突进动画播放率随距离调整 | MotionWarping 自动调整 RootMotion Scale |

---

## 八、验证标准

- [ ] `GA_Dash` 激活后播放突进 Montage
- [ ] MotionWarpingComponent 在蓝图中正常工作
- [ ] 角色冲刺到目标位置（偏移误差 < 10cm）
- [ ] Montage 播放完毕后技能正常结束
- [ ] 可配合冷却（Step 1.3 的 GE_Cooldown）

---

## 九、状态

✅ 已完成（原理学习归档，总结见 step3.2_总结.md）
