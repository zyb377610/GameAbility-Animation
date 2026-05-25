# Step 2.2 最终总结（归档）

> **归档日期**: 2026-05-24  
> **状态**: ✅ 完成  
> **AnimBP 资产**: `/Game/Characters/ABP_GASCharacter`

---

## 一、目标达成摘要

| 任务 | 状态 |
|------|:--:|
| AnimBP 中 `LayeredBoneBlend` 正确分割上下半身 | ✅ |
| `UpperBodyAlpha` 控制上半身在 BS_Locomotion 和 Attack 动画之间混合 | ✅ |
| 移动时下半身 BlendSpace 不中断 | ✅ |
| BlendDepth=-1 bug 定位并修复 | ✅ |

---

## 二、最终 AnimGraph 结构

```
Get Speed ────────────────→ BS_Locomotion.X (速度驱动)

BS_Locomotion.Pose ───────→ LayeredBoneBlend.BasePose        (下半身: Idle/Walk/Jog)
SequencePlayer(MM_Attack_01).Pose → LayeredBoneBlend.BlendPoses_0  (上半身: 攻击动画)
Get UpperBodyAlpha ───────→ LayeredBoneBlend.BlendWeights_0  (混合权重: 0=纯移动, 1=纯攻击)

LayeredBoneBlend.Pose ────→ Output Pose.Result
```

图中还有 3 个未连线节点（MM_Attack_02, MM_Jump, MM_Fall_Loop），不影响功能，可供后续扩展使用。

---

## 三、LayeredBoneBlend 最终配置

| 属性 | 值 | 说明 |
|------|-----|------|
| BlendMode | `BranchFilter` | 按骨骼分支过滤 |
| LayerSetup.BoneName | `spine_01` | 上半身分割点 |
| LayerSetup.BlendDepth | `1` | ⚠ 关键！不能是 -1 |
| bMeshSpaceRotationBlend | `True` | 网格空间旋转混合 |
| BasePose | BS_Locomotion | 下半身源 |
| BlendPoses_0 | MM_Attack_01（循环播放） | 上半身源 |
| BlendWeights_0 | UpperBodyAlpha (float, 0~1) | 混合权重 |

---

## 四、关键问题与修复过程

### 问题1: LayeredBoneBlend 完全不生效

**现象**: 连线正确、变量正确，但混合结果始终等于 BasePose，BlendPoses_0 被完全忽略。

**排查历程**:
1. ❌ 排除 BoneName 配置错误（`read_bone_track` 确认 spine_01 存在）
2. ❌ 排除 BlendWeights 未接入（验证 UpperBodyAlpha=1.0 已传入）
3. ❌ 排除动画不兼容（MM_Attack_01 与当前骨架匹配）
4. ❌ 排除缓存中间层干扰（删除 SaveCachedPose/UseCachedPose）
5. ❌ 重建 LayeredBoneBlend 节点并设置 bMeshSpaceRotationBlend=True
6. ✅ **根因**: `BlendDepth=-1` 导致权重全为 0

### 问题2: BlendDepth=-1 的源码级根因

查看 UE 5.6 源码 `AnimationRuntime.cpp:2398-2442`:

```cpp
const float IncreaseWeightPerDepth = 
    (BranchFilter.BlendDepth != 0) ? (1.f / ((float)BranchFilter.BlendDepth)) : 1.f;
```

| BlendDepth | IncreaseWeightPerDepth | 效果 |
|------------|----------------------|------|
| 0 | 1.0 | 无除法，每层权重 = Depth+1 |
| 1 | 1/1 = 1.0 | 每层权重 = Depth+1，全部达到 1.0 |
| 2 | 1/2 = 0.5 | 第2层达到 1.0，前层有过渡 |
| N | 1/N | 第N层达到 1.0 |
| **-1** | **1/(-1) = -1.0** | **每层权重为负 → Clamp(0,1) → 全部为 0** |

`BlendDepth` 的语义是**控制权重沿骨骼链的渐变速率**（在第几层满权重），不是限制覆盖范围。BranchFilter 选中 spine_01 后，不管 Depth 设多少，spine_01 及以下所有子骨骼都会被覆盖。

当 Depth=-1 时，`1/(-1)` 产生负权重增量，被 `FMath::Clamp(..., 0.f, 1.f)` 钳制为 0，导致 BlendPoses 完全无效。

**修复**: 将 `BlendDepth` 从 `-1` 改为 `1`。

---

## 五、BlendDepth 语义解释

`BlendDepth` = 权重在第几层达到满值 1.0。

以 spine_01 → spine_02 → spine_03 → clavicle_l → upperarm_l 为例：

| BlendDepth | spine_01(深0) | spine_02(深1) | spine_03(深2) | clavicle_l(深3) |
|------------|:--:|:--:|:--:|:--:|
| 1 | 1.0 | 1.0 | 1.0 | 1.0 |
| 2 | 0.5 | 1.0 | 1.0 | 1.0 |
| 3 | 0.33 | 0.67 | 1.0 | 1.0 |
| -1 | **0** | **0** | **0** | **0** |

公式: `每层权重增量 = (深度+1) / BlendDepth`，最终 Clamp 到 [0, 1]。

---

## 六、相关资产

| 资产 | 路径 | 用途 |
|------|------|------|
| AnimBP | `/Game/Characters/ABP_GASCharacter` | 主动画蓝图 |
| BS_Locomotion | `/Game/Characters/BS_Locomotion` | 移动混合空间 (Idle/Walk/Jog) |
| MM_Attack_01 | `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01` | 攻击动画 |
| upper_body.py | `Content/Scripts/animation/upper_body.py` | Python 控制器 |

---

## 七、AnimBP 变量

| 变量名 | 类型 | 默认值 | 用途 |
|--------|------|--------|------|
| Speed | double | 0 | 驱动 BS_Locomotion |
| Direction | double | 0 | 移动方向（预留） |
| UpperBodyAlpha | double | 0 | 上半身混合权重 (0=纯移动, 1=纯攻击) |
| bIsDead | bool | false | 死亡标记（预留） |
| bIsStunned | bool | false | 眩晕标记（预留） |
| bIsHit | bool | false | 受击标记（预留） |

---

## 八、Python 测试命令

```python
# 获取 AnimInstance
import ue
world = ue.GetGameWorld()
pc = ue.GameplayStatics.GetPlayerController(world, 0)
pawn = pc.Pawn
anim_inst = pawn.Mesh.GetAnimInstance()

# 上半身切到攻击
anim_inst.UpperBodyAlpha = 1.0

# 切回纯移动
anim_inst.UpperBodyAlpha = 0.0

# 半混合
anim_inst.UpperBodyAlpha = 0.5
```

---

## 九、锚点转移

- **LayeredBoneBlend** ✅ 已完成 — 后续可用 Step 4.1 的 LinkedAnimGraph 替代
- **Slot 播放 + Montage** 🔲 待 Step 2.3/后续实现
- **Python 运行时控制** 🔲 `UpperBodyAlpha` 变量已就绪，API 待测试

---

## 十、对比漫威争锋

| 漫威观察 | 本项目实现 |
|----------|----------|
| 钢铁侠飞行中攻击 | 🔲 下半身 BlendSpace + 上半身 Slot（本 Step 已就绪基础设施） |
| 英雄边走边换弹 | 🔲 需 UpperBody Slot Montage |
| 受击不打断移动 | 🔲 bIsHit + 上半身受击 Slot |
| 瞄准时上半身转向 | 🔲 Step 2.3 AimOffset |

---

**归档签名**: 上下半身分离核心功能通过 `LayeredBoneBlend + BranchFilter(spine_01, Depth=1) + UpperBodyAlpha` 实现。⚠ 关键教训: UE5 BranchFilter 模式不能用 BlendDepth=-1，会因源码中 `1/(-1)` 产生负权重导致完全失效。
