# Step 3.3 总结：Stride / Slope / Orientation Warping

> 本文档为 Step 3.3 学习过程的问答总结，记录三种动画 Warping 技术的核心原理、数据流和实现架构。
> 本步骤以 AnimBP 节点配置为主，Python 提供运行时数据。

---

## 一、三种 Warping 在 GroundMotion 体系中的位置

```
GroundMotion（地面运动匹配体系）
├── Distance Matching     → 动画进度匹配实际位移（Step 3.1）
│                           起步/停步动画恰好走完对应距离
│
├── Motion Warping        → 动态扭曲 RootMotion 到目标点（Step 3.2）
│                           突进/冲刺精确到达目标位置
│
├── Stride Warping        → 动态拉伸步幅适配不同移速（Step 3.3）★
├── Orientation Warping   → 动态调整身体朝向（Step 3.3）★
└── Slope Warping         → 根据坡度调整脚部 IK（Step 3.3）★
```

- **Step 3.1** 解决"**纵向**"问题：动画播放了多少，位移就走多少
- **Step 3.2** 解决"**终点**"问题：不管目标在哪，动画结束角色恰好到达
- **Step 3.3** 解决"**表面适配**"问题：步幅、坡度、朝向全方位贴合实际运动状态

---

## 二、数据流全景

```
┌─────────────────────────────────────────────────────────────────┐
│                        Python (每帧 Tick)                        │
│                                                                  │
│  Actor.GetVelocity() ──► Speed ──────────────────────┐          │
│                                                        │          │
│  LineTrace(向下射线) ──► SlopeNormal ─────────────────┤          │
│                     ──► SlopeAngle ───────────────────┤          │
│                                                        │          │
│  上一帧Yaw vs 当前Yaw ──► YawDelta ───────────────────┤          │
│                                                        │          │
│  写入 AnimBP 变量 ◄──── setattr(anim_inst, ...) ──────┘          │
└──────────────────────────────┬──────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    AnimBP (动画蓝图)                              │
│                                                                  │
│  [Locomotion State Machine]                                     │
│         │                                                        │
│         ▼                                                        │
│  [BlendSpace / Sequence Evaluator]                              │
│         │                                                        │
│         ▼                                                        │
│  ┌──────────────────┐                                            │
│  │ Stride Warping   │ ← Speed (已有 Step 2.1)                   │
│  │ 步幅拉伸          │   拉伸/压缩腿部骨骼，匹配实际位移            │
│  └────────┬─────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────┐                                            │
│  │ Slope Warping    │ ← SlopeNormal, SlopeAngle (新增)           │
│  │ 斜坡适配          │   旋转脚部 IK，贴合斜面                     │
│  └────────┬─────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────┐                                            │
│  │OrientationWarping│ ← YawDelta (新增)                          │
│  │ 转身朝向          │   脊柱反向旋转，上半身滞后                  │
│  └────────┬─────────┘                                            │
│           │                                                      │
│           ▼                                                      │
│  [Output Pose]                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 三、Stride Warping（步幅拉伸）

### 3.1 解决的问题

在 BlendSpace 中，不同速度混合出的动画，脚步跨幅可能与角色实际位移不匹配。动画是"慢走"的步伐间距，但角色移动速度更快 → 出现**滑步**。

### 3.2 原理

Stride Warping 节点在 AnimBP 管线中，读取动画每帧的脚步位置（通过 Foot IK 骨骼），根据实际 Speed 动态拉伸或压缩腿部骨骼的位移量。

```
步幅缩放系数 = 实际每帧位移 / 动画原始每帧位移
若 > 1.0 → 拉伸腿部（大步快走）
若 < 1.0 → 压缩腿部（小步慢走）
```

### 3.3 Python 侧

Speed 已在 Step 2.1 中实现，无需额外工作。

### 3.4 与 Distance Matching 的关系

| 技术 | 调整对象 | 使用阶段 |
|:--|:--|:--|
| **Distance Matching** | Sequence Evaluator 的 PlayRate（"播到哪一帧"） | 起步/停步过渡 |
| **Stride Warping** | 骨骼位移（"当前帧步幅多大"） | 匀速移动微调 |

两者都在调整"动画播放与位移的对应关系"，需避免双重缩放：
- 起步/停步阶段：Distance Matching 工作，StrideWarping PlayRate 保持 1.0
- 匀速移动阶段：Stride Warping 工作，微调步幅保持 BlendSpace 平滑

---

## 四、Slope Warping（斜坡适配）

### 4.1 解决的问题

平地动画中脚是水平的。上斜坡时不作处理 → 脚穿入地面或悬空。

### 4.2 原理

Slope Warping 节点通过地面法线计算坡度，在 AnimBP 输出姿势阶段对腿部骨骼（foot/leg IK）施加旋转和位移修正，使脚底贴合斜面。

### 4.3 Python 计算：射线检测

从角色位置向下发射不可见射线，获取地面法线：

```python
start = actor.GetActorLocation()
end = start + ue.Vector(0, 0, -200.0)  # 向下探测 200cm

success, hit = ue.LineTraceSingle(
    actor.GetWorld(),
    start, end,
    ue.ETraceTypeQuery.TraceTypeQuery1,
    False, [], ue.EDrawDebugTrace.NONE, True
)
```

### 4.4 法线与坡度的数学关系

```
SlopeNormal = (Nx, Ny, Nz)    # 碰撞点法线
SlopeAngle  = acos(Nz) * 180 / π

纯平地: Normal = (0, 0, 1)     → Angle = acos(1.0)   = 0°
30°斜坡: Normal = (0, ~0.5, ~0.866) → Angle = acos(0.866) = 30°
45°斜坡: Normal = (0, ~0.707, ~0.707) → Angle = acos(0.707) = 45°
```

Nz 越小 → 坡度越大。`Max Slope Angle` 设为 45°~60° 防止在墙壁等极端角度下 IK 变形。

### 4.5 射线通道两种方案

| 方案 | API | 优缺点 |
|:--|:--|:--|
| **A. TraceTypeQuery1** | `LineTraceSingle(..., TraceTypeQuery1, ...)` | 默认 Visibility 通道，无需额外配置，但依赖地面 Mesh 有 Block 响应 |
| **B. Profile 名称** | `LineTraceSingleByProfile(..., "WorldStatic", ...)` | 语义明确，如 A 方案打不中可切换 |

---

## 五、Orientation Warping（转身朝向）

### 5.1 解决的问题

角色快速转向时，整个身体同步旋转显僵硬。真实的人转身是**下半身先转，上半身（脊柱）滞后跟随**。

### 5.2 原理

Orientation Warping 节点根据每帧的朝向变化量（YawDelta），对 Spine（脊柱）骨骼施加反向旋转补偿。

```
下半身（Root Bone）→ 跟随实际朝向旋转
上半身（Spine）     → 被反向拉住，滞后跟随
效果：自然扭转过渡
```

### 5.3 Python 计算：YawDelta

```python
def _compute_yaw_delta(cur_yaw, prev_yaw):
    delta = cur_yaw - prev_yaw
    if delta > 180.0:       # 跨过 +180 边界
        delta -= 360.0
    elif delta < -180.0:    # 跨过 -180 边界
        delta += 360.0
    return delta             # [-180, 180]
```

**为什么需要归一化？** Yaw 范围是 `[-180, 180]`。角色从 179° 转到 -179°，实际转了 +2°，但 `-179 - 179 = -358`，归一化后才得到正确的 +2°。

### 5.4 Rotation Interp Speed 参数

控制上半身跟随的"弹簧感"：

| 值 | 效果 | 适合角色 |
|:--|:--|:--|
| 3~5 | 上半身跟随很慢，明显"扭腰" | 重型角色（坦克） |
| 10 | 适中，自然过渡 | 通用 |
| 15~20 | 跟随很快，几乎看不出扭腰 | 敏捷角色（刺客） |

> 漫威争锋中不同英雄转身速度不同，本质就是调整这个参数。

---

## 六、节点顺序的设计逻辑

三个节点必须严格按 **Stride → Slope → Orientation** 顺序串联：

```
Stride 最先  → 先让步伐走得对（位移量修正）
Slope 第二   → 在步伐正确的基础上，脚旋转贴合斜面
Orientation 最后 → 调脊柱（不涉及脚），不会干扰前两步结果
```

**如果顺序反过来：**
- Orientation 先做 → Spine 转了 → Stride/Slope 基于错误姿势修正 → 脚可能又离地

---

## 七、Python 侧改动清单

扩展现有 `Content/Scripts/animation/locomotion.py` 中的 `LocomotionUpdater`：

### 7.1 新增函数

| 函数 | 用途 |
|:--|:--|
| `_get_ground_info(actor)` | 射线检测，返回 `(SlopeNormal, SlopeAngle)` |
| `_compute_yaw_delta(cur, prev)` | 计算归一化朝向变化量 |

### 7.2 LocomotionUpdater 改动点

| 改动 | 说明 |
|:--|:--|
| 构造中加 `self._prev_yaw` | 记录上一帧 Yaw，初始化为 `actor.GetActorRotation().Yaw` |
| tick() 末尾加地面检测 | 调用 `_get_ground_info`，写入 `SlopeNormal_X/Y/Z` 和 `SlopeAngle` |
| tick() 末尾加朝向差计算 | 调用 `_compute_yaw_delta`，写入 `YawDelta` |

### 7.3 SlopeNormal 传递方式

AnimBP 中 Vector 类型变量可能不被 Nepy `setattr` 支持，**稳妥方案是将 Vector 拆成三个 Float 变量**：

| AnimBP 变量名 | 写入值 |
|:--|:--|
| `SlopeNormal_X` | `normal.X` |
| `SlopeNormal_Y` | `normal.Y` |
| `SlopeNormal_Z` | `normal.Z` |

AnimBP 侧用 `Make Vector` 节点将三个 float 合成 Vector 再接入 Slope Warping 节点。

### 7.4 API 确认状态

| API | 签名 | 状态 |
|:--|:--|:--|
| `ue.LineTraceSingle` | `(World, Start, End, TraceChannel, ...) -> (bool, HitResult)` | ✅ pystubs 确认 |
| `ue.ETraceTypeQuery.TraceTypeQuery1` | 枚举值 0 | ✅ |
| `ue.EDrawDebugTrace.NONE` | 枚举值 0 | ✅ |
| `HitResult.bBlockingHit` | bool | ✅ |
| `HitResult.ImpactNormal` | Vector | ✅ |
| `Rotator.Yaw` | float 属性 | ✅ |
| `ue.Vector(x, y, z)` | 构造函数（现有代码已用） | ✅ |
| `setattr(anim_inst, var_name, value)` | Nepy 运行时写入 | ✅ 已有模式 |

---

## 八、AnimBP 侧改动清单

在 `ABP_GASCharacter` 中操作：

### 8.1 新增变量

| 变量名 | 类型 | 默认值 |
|:--|:--|:--|
| `SlopeNormal_X` | Float | 0.0 |
| `SlopeNormal_Y` | Float | 0.0 |
| `SlopeNormal_Z` | Float | 1.0 |
| `SlopeAngle` | Float | 0.0 |
| `YawDelta` | Float | 0.0 |

### 8.2 三个 Warping 节点配置

| 节点 | 关键引脚 | 接法 |
|:--|:--|:--|
| **Stride Warping** | Input Pose, Speed, Play Rate, Output Pose | Speed 连 Speed 变量，Play Rate 建议 1.0 |
| **Slope Warping** | Input Pose, Slope Normal, Slope Angle, Output Pose | Slope Normal 用 MakeVector 合成三个 float |
| **Orientation Warping** | Input Pose, Yaw Delta, Rotation Interp Speed, Output Pose | Yaw Delta 连 YawDelta 变量，Interp Speed 建议 10 |

---

## 九、潜在问题与排查

| 问题 | 现象 | 排查方向 |
|:--|:--|:--|
| 射线打不中地面 | SlopeAngle 始终为 0 | 临时开启 `ForOneFrame` 调试线 → 检查地面 Collision Preset → 换用 `LineTraceSingleByProfile` |
| SlopeNormal 写入失败 | 变量值不变 | 确认 AnimBP 中变量名完全一致 → 测试 AnimBP 是否支持自定义变量被外部写入 |
| Orientation 效果不明显 | 转身无扭腰效果 | print YawDelta 确认有值 → 减小 RotationInterpSpeed 到 3~5 |
| Stride 与 DistanceMatching 冲突 | 起步/停步滑步 | AnimBP 中判断移动状态，起步/停步时 StrideWarping PlayRate 保持 1.0 |
| 三个节点顺序不对 | 姿势变形 | 严格按 Stride→Slope→Orientation 连线 |

---

## 十、验证标准

- [ ] Python 正确计算地面法线和坡度（控制台可见 Slope 值）
- [ ] AnimBP SlopeWarping 在斜坡上调整脚步（脚不再穿模或悬空）
- [ ] AnimBP StrideWarping 消除加减速滑步（速度变化脚底不打滑）
- [ ] AnimBP OrientationWarping 转身自然（按 A/D 转身时上半身有滞后感）
- [ ] 三节点串联无冲突（同时走路+上坡+转视角表现自然）

---

## 十一、与漫威争锋的对照

| 漫威观察 | UE5 对应 |
|:--|:--|
| 角色在楼梯/斜坡上行走脚步贴合 | Slope Warping |
| 快跑和慢跑步幅自动适配 | Stride Warping |
| 转身时自然不僵硬 | Orientation Warping |
| 不同英雄转身速度不同 | OrientationWarping 的 Rotation Interp Speed 参数 |

---

## 十二、GroundMotion 三阶段总结

```
Phase 3 完整技术栈：

Step 3.1  Distance Matching
          └── 纵向：动画进度 = 实际位移

Step 3.2  Motion Warping
          └── 终点：动态扭曲 RootMotion 精确到达目标

Step 3.3  Stride + Slope + Orientation Warping
          └── 表面：步幅/坡度/朝向全方位贴合运动状态
```

三者协同工作，构建了完整的"动画驱动角色运动与地面精确匹配"体系。

---

> **归档时间**：2026-05-26
> **参考文档**：`step3.3_StrideSlopeOrientation.md`, `step3.2_总结.md`, `01_GroundMotion概述.md`, `step2.1_BlendSpace移动.md`
