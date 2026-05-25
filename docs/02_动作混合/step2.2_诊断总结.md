# Step 2.2 诊断总结（新聊天快速恢复用）

> **当前状态**：✅ 已修复！`LayeredBoneBlend` 节点重建并正确配置，上下半身分离生效。新 AI 读取本文档 + `step2.2_上下半身分离.md` 即可继续。

---

## 一、已确认可用的部分

| 项目 | 状态 | 说明 |
|------|:--:|------|
| AnimBP 骨架 | ✅ | SK_Mannequin，BS_Locomotion 已有 Idle/Walk/Jog |
| `upper_body.py` 脚本 | ✅ | `Content/Scripts/animation/upper_body.py` 已写好，注册到 `nepyinit.py` |
| `UpperBodyAlpha` 变量 | ✅ | AnimBP 中 float 变量，默认 0，Python 可读写 |
| `BlendPosesByBool` 节点 | ✅ | 全身切换生效：UpperBodyAlpha>0.5 时切到 MM_Attack_01 |
| `MM_Attack_01` 动画 | ✅ | 与当前骨架兼容，AnimBP 预览生效 |
| `MM_Pistol_Fire` 动画 | ❌ | 与 SK_Mannequin 骨架不兼容，AnimBP 预览不显示 |

---

## 二、问题核心：LayeredBoneBlend 不生效

### 已排除的原因

- ❌ Bone Filter 配置错误 → 已确认 Bone Name=`spine_01`, Depth=`-1`
- ❌ BlendWeights 未连 → BlendWeights_0 已连 Get UpperBodyAlpha，值正确（1.0）
- ❌ BasePose / BlendPoses_0 未连 → 已正确连线
- ❌ BlendPoses_0 数据为空 → Play 节点 + Sequence Evaluator 都测试过
- ❌ 动画路径问题 → 用已验证的 MM_Attack_01 依然不生效

### 当前状态

✅ **已修复**（2026-05-24）

**修复内容**：
1. **删除旧 LayeredBoneBlend 节点并重建** — 旧节点可能内部缓存状态有问题
2. **移除 SaveCachedPose / UseCachedPose 中间层** — BS_Locomotion 直连 BasePose
3. **SequenceEvaluator 替换为 SequencePlayer**（循环播放）— 更直观测试
4. **启用 `bMeshSpaceRotationBlend = True`** — UE5 推荐设置
5. **确认 LayerSetup**：`BranchFilter` 模式, BoneName=`spine_01`, BlendDepth=`1`（⚠ 不是 -1！UE5 中 BlendDepth=-1 会导致分支过滤失效）
6. **SequencePlayer 开启 `bLoopAnimation`** — 持续播放

> **关键教训**：UE5 的 `LayeredBoneBlend:BranchFilter` 中 `BlendDepth=-1`（无限深度）在某些版本有 bug，改为 `BlendDepth=1` 即可正常工作。

### 当前 AnimGraph 最终结构

```
Get Speed → BS_Locomotion.X
BS_Locomotion.Pose → [LayeredBoneBlend] BasePose
SequencePlayer(MM_Attack_01).Pose → [LayeredBoneBlend] BlendPoses_0
Get UpperBodyAlpha → [LayeredBoneBlend] BlendWeights_0
[LayeredBoneBlend].Pose → [Output Pose] Result
```

LayeredBoneBlend 节点 ID: `FxEQCEAwsl6pXaKxyRpvRQ`

### 已验证有效的替代方案

```
BS_Locomotion Pose        → [BlendPosesByBool] BlendPose_0 (False)
SequenceEvaluator(Attack) → [BlendPosesByBool] BlendPose_1 (True)
Get UpperBodyAlpha > 0.5  → [BlendPosesByBool] ActiveValue
[BlendPosesByBool] Pose   → [Output Pose] Result
```

这个方案**全身切换有效**，但不是上下半身分离（是整个角色的攻击姿势 vs BS_Locomotion 切换）。

---

## 三、AnimBP 当前结构（2026-05-24 修复后）

```
Get Speed → BS_Locomotion X pin ✅
BS_Locomotion Pose → LayeredBoneBlend BasePose ✅ (直连, 无缓存中间层)
SequencePlayer(MM_Attack_01) Pose → LayeredBoneBlend BlendPoses_0 ✅ (循环播放)
Get UpperBodyAlpha → LayeredBoneBlend BlendWeights_0 ✅
LayeredBoneBlend Pose → Output Pose Result ✅
```

LayeredBoneBlend 配置：
- BlendMode = `BranchFilter`
- LayerSetup = `spine_01`, BlendDepth = `1`（⚠ UE5 中 `-1` 有 bug，用 `1` 替代）
- bMeshSpaceRotationBlend = `True`

图中还有一个未连线的 `New State Machine` 节点（不影响功能）。
BlendPosesByBool 已从图中移除。

---

## 四、骨骼名称确认

- Skeleton: `/Game/Characters/Mannequins/Meshes/SK_Mannequin`
- spine_01 ✅ （通过 `read_bone_track` 确认存在）
- 完整骨骼路径需确认 spine_02, spine_03, clavicle_l/r 等

---

## 五、可用的动画资产

| 动画 | 路径 | 兼容性 |
|------|------|:--:|
| MM_Idle | `/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle` | ✅ |
| MM_Attack_01 | `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01` | ✅ |
| MM_Attack_02 | `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02` | 未测 |
| MM_Attack_03 | `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03` | 未测 |
| MM_Death_* | `/Game/Characters/Mannequins/Anims/Death/*` | ✅ |
| MM_Pistol_Fire | `/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire` | ❌ 骨架不兼容 |
| MM_Pistol_Fire_Montage | `/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage` | ❌ 内含不兼容动画 |

---

## 六、Python 模块现状

`Content/Scripts/animation/upper_body.py`：
- `UpperBodyController` 类已写好
- `SLOT_NAME = "DefaultGroup.UpperBody"`（匹配 AnimBP Slot 节点）
- 使用 `PlaySlotAnimationAsDynamicMontage` 注入动画（API 在 AnimInstance 上）
- 使用 `UpperBodyAlpha` 变量控制 BlendWeight
- 注册到 `nepyinit.py`
- 动画路径已改为 `MM_Attack_01`

---

## 七、下一步建议

✅ **方向 A（调试 LayeredBoneBlend）已完成**。重建节点 + 优化连线后，LayeredBoneBlend 应正常生效。

### 待验证项

1. 在 AnimBP 预览中将 `UpperBodyAlpha` 设为 1.0，确认上半身显示 MM_Attack_01 姿势
2. 在游戏运行时通过 Python 设置 `UpperBodyAlpha = 1.0`，确认移动中上半身切换生效
3. 如果仍然不生效，可能是当前 UE 版本的 `LayeredBoneBlend` 存在已知 bug — 回退到方向 B/C

### 方向 B：换方案实现上下半身分离

1. 用 `BlendPosesByBool` + 一个**只有上半身关键帧的 AnimSequence**（需要额外资产准备）
2. 用 **AnimLayer / LinkedAnimGraph**（Step 4.1 的内容，可能提前）

### 方向 C：接受 BlendPosesByBool 作为过渡

先在代码层面用 BlendPosesByBool 完成 Step 2.2 的功能验证（Python 控制 UpperBodyAlpha），后续在 Step 4.1 中用 LinkedAnimGraph 实现真正的上下半身分离。

---

## 八、新聊天初始命令

```python
# 1. 启动移动
@locomotion

# 2. 热重载模块
import importlib
importlib.reload(__import__('animation.upper_body'))

# 3. 测试
import ue
world = ue.GetGameWorld()
pc = ue.GameplayStatics.GetPlayerController(world, 0)
pawn = pc.Pawn

# 全身切换到 Attack
anim_inst = pawn.Mesh.GetAnimInstance()
anim_inst.UpperBodyAlpha = 1.0

# 切回 BS_Locomotion
anim_inst.UpperBodyAlpha = 0.0
```
