# Step 2.2 本聊天总结（新聊天最终归档用）

> **用途**：新聊天 AI 读取本文档，结合 `step2.2_诊断总结.md` 的当前代码状态，将本聊天的学习收获补充到 `step2.2_上下半身分离.md` 最终文档中。

---

## 一、本聊天做了什么

| 事项 | 内容 | 状态 |
|------|------|:--:|
| 读完总览 + step2.2 文档，理解目标 | 上下半身分离：移动时上半身独立播动画 | ✅ |
| 确认现有资源 | AnimBP(ABP_GASCharacter)、BS_Locomotion、locomotion.py | ✅ |
| 确认 API | PlaySlotAnimationAsDynamicMontage（在 AnimInstance 上，非 SkelMeshComponent）| ✅ |
| 修改 Montage Slot 名 | MM_Pistol_Fire_Montage: "Arms" → "UpperBody" | ✅ |
| 创建 upper_body.py | Content/Scripts/animation/upper_body.py | ✅ |
| 注册到 nepyinit.py | import animation.upper_body | ✅ |
| AnimBP 手动改造 | 添加 Slot 节点 + LayeredBlendPerBone + 变量 | ✅ |
| AnimBP 多次调试 | Slot 名匹配、BlendWeight 变量、节点连线 | ✅ |
| 发现问题 | MM_Pistol_Fire 动画与 SK_Mannequin 骨架不兼容 | ✅ |
| 替代动画 | MM_Attack_01 可用 | ✅ |
| 最终方案 | BlendPosesByBool + UpperBodyAlpha 变量 | ✅ |

---

## 二、AnimBP 最终有效结构

```
Get Speed ───────────→ [BS_Locomotion] X pin ✅
                          │
                          ├─ Pose → BlendPose_0 (False)
                          │
[SequenceEvaluator]       │
  (MM_Attack_01,          │
   ExplicitTime=0.5) ─────┴─ Pose → BlendPose_1 (True)
                          │
Get UpperBodyAlpha > 0.5 ──→ ActiveValue
                          │
                    [BlendPosesByBool] Pose → [Output Pose] Result
```

- 当 `UpperBodyAlpha > 0.5`：播 MM_Attack_01 全身姿势
- 当 `UpperBodyAlpha <= 0.5`：走 BS_Locomotion

**注意**：这是全身切换，不是真正的上下半身分离。`LayeredBoneBlend` 在本环境中未调试通过（见下文）。

---

## 三、核心学习收获（必须归档到最终文档）

### 3.1 AnimBP 与 Python 的分工（用户重点标记）

> **AnimBP 定义了"骨骼数据如何混合的规则"（静态拓扑），Python 负责"往哪个 Slot/变量 注入什么内容"（运行时输入）。**

详细讲解已在对话中展开，入档内容：

- **AnimBP = 音视频混音台**：混音台的物理旋钮和电路 = AnimBP 中的节点连线；空 XLR 接口 = Slot 节点；DJ 把麦克风插进接口 = Python 调用 API
- **BS_Locomotion 是主动节点**：自己引用 BlendSpace 资产，每帧根据 Speed 采样输出
- **Slot 是被动节点**：没有任何动画引用，等待外部运行时注入；无注入时输出 Reference Pose
- **PlaySlotAnimationAsDynamicMontage**：AnimSequence 无 Slot 概念，通过 SlotNodeName 参数动态指定目标 Slot，引擎内部临时创建 DynamicMontage 包装
- **Montage_Play**：AnimMontage 自带 Slot 配置，引擎根据 Montage 资产的 Slot 名自动路由

### 3.2 "写死 AnimBP vs 动态注入" 的灵活性辨析（用户重点标记）

用户质疑："直接引用的写法也没那么麻烦，Python 代码里看不到灵活性优势"。

**诚实回答**：在当前单角色场景下确实没有压倒性优势。真正的优势在多角色、多技能、数据驱动配置场景：

| 场景 | 写死 AnimBP | 动态注入 |
|------|-----------|---------|
| 固定角色固定动作 | ✅ 直观 | 过度设计 |
| 多个不同英雄用相同 AnimBP 骨架 | 需要子类或分支膨胀 | ✅ 同一 Slot，不同 Python 加载不同路径 |
| 动画配置频繁策划调整 | 改 AnimBP → 编译 | ✅ 改 CSV/Python 常量 |
| 运行时动态组合（武器×姿态 = 8 种） | 状态机爆炸 | ✅ 查表一行 |

结论：**当前仅需 1 个 AnimBP + 少数动作时，写死也可以。但随着规模增长，Slot+动态注入的架构优势才会体现。**

### 3.3 LayeredBlendPerBone 工作原理

**Bone Filter = 逐骨骼选择性混合**：

```
Root
├─ pelvis          ← 权重 0（保持 Base 层 = BlendSpace）
│  ├─ thigh_l/r    ← 权重 0
│  └─ spine_01     ← 权重 1.0（Branch Filter 起点）
│     ├─ spine_02  ← 递归继承权重 1.0
│     ├─ spine_03  ← 上层动画生效
│     ├─ clavicle_l/r → arms → hands ← 上层动画
│     └─ neck → head                   ← 上层动画
```

**Blend Depth 含义**：
- 从 Branch Filter 的 Bone Name 开始，向下覆盖多少层子骨骼
- `1` = 只覆盖该骨骼本身
- `2` = 该骨骼 + 直接子骨骼
- `-1` = 无限深度，覆盖所有子孙骨骼（我们需要的）

**Branch Filter vs Per Bone Filter**：
- Branch Filter：指定一个起始骨 + Depth，覆盖整条分支，简单
- Per Bone Filter：逐骨指定 0~1 权重，可精细控制（如左臂 0.8、右臂 0.5），繁琐

### 3.4 Slot 命名规则

Slot 节点全名 = `GroupName.SlotName`。
- AnimBP 编辑器默认：`DefaultGroup.DefaultSlot`
- 改名后：`DefaultGroup.UpperBody`
- Python 代码中 `SlotNodeName` 参数需用全名 `"DefaultGroup.UpperBody"`
- Slot Group 可在 AnimBP 的 **Asset Details → Slot Groups** 或 **Window → Anim Slot Manager** 中管理

### 3.5 踩坑记录

| 坑 | 现象 | 原因 |
|----|------|------|
| PlaySlotAnimationAsDynamicMontage AttributeError | `'SkeletalMeshComponent' object has no attribute` | API 挂在 **AnimInstance** 上，不是 SkeletalMeshComponent。以 Nepy 绑定源码（.cpp）为准，pystubs 可能误导 |
| Montage_Play + AnimMontage 不播放 | 无报错无效果 | `PlaySlotAnimationAsDynamicMontage` 只接受 AnimSequence，AnimMontage 需用 `Montage_Play` |
| MM_Pistol_Fire 动画预览不显示 | SequenceEvaluator 输出为空 | 动画骨架与 SK_Mannequin 不兼容，来自不同的 Mixamo 导出 |
| Slot 名称不匹配 | 播放无效果 | AnimBP 中 Slot 名为 `DefaultSlot`，代码传 `"UpperBody"` → 不一致 |
| LayeredBoneBlend 不生效 | 配置正确但输出始终 = BasePose | 本环境未解决，改用 BlendPosesByBool 作为替代 |

### 3.6 API 速查

| API | 所在类 | 用途 |
|-----|--------|------|
| `PlaySlotAnimationAsDynamicMontage(Asset, SlotNodeName, BlendInTime, ...)` | **AnimInstance** | AnimSequence 动态注入指定 Slot |
| `Montage_Play(MontageToPlay, InPlayRate, ...)` | AnimInstance | 播放 AnimMontage（Slot 由 Montage 资产决定） |
| `Montage_Stop(BlendOutTime, Montage)` | AnimInstance | 停止 Montage |
| `Montage_IsPlaying(Montage)` | AnimInstance | 检查 Montage 是否活跃 |
| `LoadObject(Class, Path)` | ue 模块 | 加载资产 |
| `GetAnimInstance()` | SkeletalMeshComponent | 获取 AnimInstance |

---

## 四、相关文件清单

| 文件 | 行数 | 说明 |
|------|------|------|
| `Content/Scripts/animation/upper_body.py` | 267 | 上半身动画控制器（完整） |
| `Content/Scripts/animation/locomotion.py` | - | Step 2.1 移动系统（参考） |
| `Content/Scripts/nepyinit.py` | - | 已注册 animation.upper_body |
| `Content/Characters/ABP_GASCharacter` | - | AnimBP（含 UpperBodyAlpha 变量、BlendPosesByBool、SequenceEvaluator） |
| `docs/02_动作混合/step2.2_上下半身分离.md` | - | 原始任务文档 |
| `docs/02_动作混合/step2.2_诊断总结.md` | - | 本聊天产出的诊断文档 |
| `docs/00_项目总览.md` | - | 项目总览 |

---

## 五、给新聊天的任务

1. 读取 `step2.2_上下半身分离.md`（原始任务）
2. 读取 `step2.2_诊断总结.md`（当前代码状态）
3. 读取本文档（学习收获），将**第三节「核心学习收获」**的内容补充到 `step2.2_上下半身分离.md` 的"关键技术点"和"原理讲解"部分
4. 根据用户在新聊天中提供的 `LayeredBoneBlend` 解决方案，更新最终文档中关于该节点的配置说明
5. 更新 `docs/00_项目总览.md` 中 Step 2.2 的状态为 ✅
