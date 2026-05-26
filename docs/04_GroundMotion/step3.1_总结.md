# Step 3.1 总结：Distance Matching 起步/停步

> 完成日期：2026-05-26
> 对应文档：`docs/04_GroundMotion/step3.1_DistanceMatching.md`
> 关键概念记录：`docs/04_GroundMotion/step3.1_关键概念记录.md`

---

## 一、实际完成内容

| 子步骤 | 内容 | 产出 |
|--------|------|------|
| 3.1a | 给停步动画添加 Distance Curve | `StopWalking_Anim` 新增 `Distance` 曲线（10个关键帧，0~138cm） |
| 3.1b | Python 位移追踪 + 写入 AnimBP | `Content/Scripts/animation/distance_matching.py` |
| 3.1c | AnimBP 连线配置 | `ABP_GASCharacter` 新增 4 个变量 + SequenceEvaluator分支 |
| 3.1d | GM 命令接入 | `@dm` toggle 开关 |

---

## 二、新增文件

| 文件 | 说明 |
|------|------|
| `Content/Scripts/animation/distance_matching.py` | DistanceMatchingController：位移追踪、运动阶段检测、AnimBP变量写入 |
| `docs/04_GroundMotion/step3.1_关键概念记录.md` | 实现过程中的关键概念问答记录 |

## 三、修改文件

| 文件 | 修改内容 |
|------|---------|
| `Content/Scripts/nepyinit.py` | 新增 `import animation.distance_matching` |
| `Content/Scripts/gmcmds.py` | 新增 `dm()` GM 命令 |
| `Content/Characters/ABP_GASCharacter` | 新增变量：`DistanceToMatch`, `DisplacementSinceLastUpdate`, `IsStopping`, `StopAnimTime`；AnimGraph 新增 SequenceEvaluator + BlendPosesByBool 分支 |

---

## 四、架构一览

```
┌─ Python 层 ─────────────────────────────────────────────┐
│  DistanceMatchingController                              │
│    ├── 每帧计算: displacement = cur_pos - prev_pos       │
│    ├── 运动阶段状态机: IDLE → MOVING → STOPPING          │
│    ├── 累计: distance_to_match += displacement.Y         │
│    └── 写入: anim_inst.DistanceToMatch = 累计值          │
├──────────────────────────────────────────────────────────┤
│  AnimBP EventGraph                                       │
│    ├── IsStopping = (Speed > 5) AND (Speed < 50)        │
│    └── StopAnimTime = Clamp(DistanceToMatch/138, 0,1)*1.53│
├──────────────────────────────────────────────────────────┤
│  AnimBP AnimGraph                                        │
│    ├── BlendPosesByBool(IsStopping)                      │
│    │   ├── False → BlendSpace 正常移动                    │
│    │   └── True  → SequenceEvaluator(ExplicitTime=StopAnimTime)│
│    └── → LayeredBoneBlend → Output Pose                  │
└──────────────────────────────────────────────────────────┘
```

---

## 五、核心学习成果

### 5.1 Distance Matching 原理

**问题**：动画根骨骼位移 ≠ 胶囊体实际位移 → 滑步。

**解决**：以胶囊体实际位移为基准，动态调整动画播放进度（Explicit Time），使动画根骨骼位移 = 胶囊体位移。

**核心公式**：`比例 = 实际位移 / 动画最大位移 → 目标时间 = 比例 × 动画总时长`

### 5.2 简化实现 vs 标准实现

| | 本 Step 实现 | 标准 Distance Matching |
|--|-------------|----------------------|
| 时间映射 | 线性映射（位移/总位移→时间/总时长） | Distance Curve 反向查询 |
| 驱动方式 | SetExplicitTime（直接设时间） | PlayRate 弹簧式追赶 |
| 复杂度 | 低 | 高（需曲线反查+每帧迭代） |
| 学习价值 | 理解核心数据流 | 理解精确匹配 |

### 5.3 数据流关键认知

- **Python 职责**：提供"角色实际走了多远"（DistanceToMatch）
- **AnimBP 职责**：把距离映射为动画播放时间
- **SequenceEvaluator**：区别于 SequencePlayer，时间由外部显式控制
- **状态切换**：BlendPosesByBool 实现停步动画和正常移动的平滑过渡

### 5.4 运动阶段状态机

```
        speed > 50
IDLE ──────────────→ MOVING
  ↑                    │
  │ speed < 10         │ speed < 30
  │                    ↓
  └──────────────── STOPPING
        还有: speed > 50 → MOVING (取消停步)
```

阶段切换时重置 `DistanceToMatch`，确保每段运动独立匹配。

---

## 六、遇到的坑

| 问题 | 原因 | 解决 |
|------|------|------|
| SequenceEvaluator 无 PlayRate 引脚 | UE5 中 SE 只有 ExplicitTime | 改用线性映射直接设时间 |
| GetCurveValue 需要 Target 引脚 | 需引用动画资产 | 简化方案跳过曲线查表，用线性映射替代 |
| UE 5.6 中 SequencePlayer 无法 SetPlaybackPosition | 新版 AnimBP 限制更严格 | 用变量桥接（EventGraph→变量→AnimGraph） |
| Convert to Sequence Evaluator 节点找不到 | 引用获取链路复杂 | 放弃直接获取节点引用，改为变量传递 |
| BlendPosesByBool blend time 未设置 | 默认 blend 时间可能不理想 | BlendIn=0.05s, BlendOut=0.15s |
| `Vector.__sub__` / `Vector.Length()` / `Vector.Dot()` | 需确认 Nepy 绑定 | 已验证 pystubs，全部存在 |

---

## 七、与后续步骤的关系

| 步骤 | 本 Step 成果的用途 |
|------|-------------------|
| Step 3.2 MotionWarping | 停步逻辑可复用；MotionWarping 是主动式位移，DM 是被动式跟踪 |
| Step 3.3 Stride/Orientation Warping | 本 Step 的简化线性映射可升级为精确曲线匹配 + PlayRate 版本 |

---

## 八、验证标准

- [x] Python 每帧计算位移增量（DistanceMatchingController.Tick）
- [x] `DistanceToMatch` 成功写入 AnimBP（setattr 方式）
- [x] AnimBP EventGraph 中 IsStopping / StopAnimTime 正确计算
- [x] AnimGraph 中 BlendPosesByBool 正确切换 SequenceEvaluator
- [x] GM 命令 `@dm` 可启动/停止
- [x] 停步时停步动画被触发（区别于之前直接切 Idle）

---

## 九、待优化项

- [ ] 将线性映射升级为 Distance Curve 精确反查（需要更复杂的蓝图逻辑或曲线查表）
- [ ] 实现 PlayRate 弹簧式追赶（替代 SetExplicitTime 直接设值，过渡更平滑）
- [ ] 添加起步动画支持（需准备 StartWalk 动画资产）
