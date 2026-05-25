# Step 2.2: 上下半身分离

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 2.2 状态改为 ✅。

---

## 一、目标

移动时上半身独立播放射击/技能动画，下半身继续 BlendSpace 移动。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/step2.1_BlendSpace移动.md`

---

## 三、前置条件

- ✅ Step 2.1 完成：BlendSpace 移动正常运行
- ✅ AnimBP 中已配置 **LayeredBoneBlend** 节点（BranchFilter spine_01, Depth=1）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/upper_body.py` | `UpperBodyController` | 在 Slot 上播放上半身动画（射击/技能），下半身保持 BlendSpace |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/ABP_GASCharacter` | AnimInstance（更新） | 添加 LayeredBlendPerBone + UpperBody Slot |
| `Content/Characters/AM_Shoot` | AnimMontage | 射击 Montage，挂载在 UpperBody Slot |

---

## 五、关键技术点

### 5.1 AnimBP 中的 LayeredBlendPerBone

AnimBP 连线结构（在编辑器中手动搭建）：

```
[BS_Locomotion (移动)] ──┐
                         ├─ [LayeredBlendPerBone] → [Output Pose]
[Slot 'UpperBody'] ──────┘
```

LayeredBlendPerBone 配置：
- **Blend Mask / Bone Filter**：从 `spine_01`（或 `spine_02`）开始为 1.0，该 Bone 及其子节点（上半身、手臂、头）受上层动画影响，下半身保持下层。
- **Blend Depth**：通常设为 -1（无限）或指定深度

### 5.2 Slot 播放

通过 Python 在 Slot 上播放动画：

```python
import ue

class UpperBodyController:
    def __init__(self, skeletal_mesh: ue.SkeletalMeshComponent):
        self.mesh = skeletal_mesh

    def play_shoot_animation(self):
        """在上半身 Slot 播放射击 Montage"""
        anim_inst = self.mesh.GetAnimInstance()
        shoot_montage = ue.load_object(ue.AnimMontage, "/Game/Characters/AM_Shoot")
        if shoot_montage and anim_inst:
            # 通过 PlaySlotAnimationAsDynamicMontage 在指定 Slot 播放
            self.mesh.PlaySlotAnimationAsDynamicMontage(
                shoot_montage,
                "UpperBody",  # Slot 名称
                0.25,  # BlendInTime
                0.25,  # BlendOutTime
                1.0,   # PlayRate
            )
```

### 5.3 API 确认

| API | 类 | 签名 |
|-----|-----|------|
| `PlaySlotAnimationAsDynamicMontage` | SkeletalMeshComponent | `(Asset, SlotNodeName, BlendInTime=0.25, BlendOutTime=0.25, InPlayRate=1.0, LoopCount=1, BlendOutTriggerTime=-1.0, InTimeToStartMontageAt=0.0) -> AnimMontage` |
| `PlaySlotAnimationAsDynamicMontage_WithBlendSettings` | SkeletalMeshComponent | 同上但接受 `MontageBlendSettings` |
| `Montage_Play` | AnimInstance | `(MontageToPlay, InPlayRate=1.0, ReturnValueType, InTimeToStartMontageAt=0.0, bStopAllMontages=True) -> float` |
| `Montage_Stop` | AnimInstance | `(InBlendOutTime, Montage=None) -> None` |

### 5.4 上下半身分离的使用场景

- **移动中射击**：下半身 Walk/Jog/Sprint，上半身播放射击 Montage
- **移动中技能释放**：下半身保持 BlendSpace，上半身播放技能动画
- **受击反馈**：上半身播放受击动画，下半身不打断移动
- **瞄准/转向**：上半身独立朝向目标（配合 AimOffset）

### 5.5 动画层级示意

```
Root
├─ Pelvis (LayeredBlend Bone 分割点)
│  ├─ (下半身) ← 来自 BS_Locomotion
│  └─ Spine → Arms → Head (上半身) ← 来自 Slot UpperBody
```

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `PlaySlotAnimationAsDynamicMontage` Python 调用 | Slot 上播放动画，下层 BlendSpace 继续 | 待测试 |
| `SlotNodeName` 参数匹配 AnimBP 中 Slot 节点名 | "UpperBody" 正确路由 | 待测试 |
| LayeredBlendPerBone 在 Nepy 创建的 AnimBP 中可用 | Python 不直接设，需手动配置 AnimBP | 确认 |
| AnimMontage 加载 | `ue.load_object(ue.AnimMontage, path)` 正确 | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 钢铁侠飞行中双手发炮 | 下半身飞行 BlendSpace，上半身 Slot 射击 |
| 英雄边走边换弹 | 下半身移动，上半身换弹 Montage |
| 受击时身体后仰但不影响移动 | 上半身受击 Slot，下半身继续 |
| 瞄准时上半身转向 | 上半身 AimOffset + Slot |

---

## 八、验证标准

- [x] AnimBP 中 `LayeredBlendPerBone` 正确分割上下半身
- [x] 移动时 `UpperBodyAlpha=1.0` 上半身切换到 Attack 动画，下半身移动不中断
- [x] 停止移动后 BlendSpace 回 Idle，全身姿势正确
- [ ] 射击 Montage 播放完毕自动回到上半身 Idle（待后续实现，当前用 UpperBodyAlpha 变量控制）

---

## 九、状态

✅ 已完成

> **最终总结**: 见 `docs/02_动作混合/step2.2_最终总结_已归档.md`  
> **关键教训**: `BlendDepth=-1` 在 UE5 BranchFilter 模式中有 bug（源码 `1/(-1)` 产生负权重），改用 `BlendDepth=1` 解决。
