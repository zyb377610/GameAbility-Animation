# Step 3.3: 步幅拉伸 · 斜坡适配 · 转身朝向

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 3.3 状态改为 ✅。

---

## 一、目标

实现步幅拉伸（Stride Warping）、斜坡适配（Slope Warping）、转身朝向调整（Orientation Warping），使角色移动动画在各种地表和方向上表现自然。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/step2.1_BlendSpace移动.md`
- `docs/04_GroundMotion/step3.1_DistanceMatching.md`

---

## 三、前置条件

- ✅ Step 3.1 完成：Distance Matching 可用
- 🔲 AnimBP 中需手动添加 UE5 内置的 **StrideWarping** / **SlopeWarping** / **OrientationWarping** 动画节点

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| 本步骤主要为 AnimBP 节点配置，Python 侧提供实时数据：速度、地面法线、目标朝向等 |
| `Content/Scripts/animation/locomotion.py` | 扩展 `LocomotionUpdater` | 新增 `SlopeNormal` / `SlopeAngle` / `YawDelta` 等变量写入 AnimBP |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/ABP_GASCharacter` | AnimInstance（更新） | 添加 StrideWarping / SlopeWarping / OrientationWarping 节点 |

---

## 五、关键技术点

### 5.1 Stride Warping（步幅拉伸）

**目的**：角色实际移动速度与动画播放率不匹配时，拉伸/压缩腿部动画以匹配步幅，消除滑步。

AnimBP 中配置：
1. 添加 `Stride Warping` 节点
2. 输入参数：
   - **Speed**：当前移动速度
   - **Play Rate**：动画播放率
3. 节点自动根据动画中的脚步骨骼位置进行计算

Python 侧只需持续提供 `Speed` 变量（已在 Step 2.1 中实现）。

### 5.2 Slope Warping（斜坡适配）

**目的**：角色在斜坡上行走时，脚部自动贴合斜面，IK 调整腿部骨骼位置。

Python 计算地面法线：

```python
import ue

def get_ground_info(actor: ue.Actor):
    """获取地面法线和斜坡角度"""
    start = actor.GetActorLocation()
    end = start + ue.Vector(0, 0, -200.0)  # 向下 200cm 射线

    hit_result = ue.SystemLibrary.LineTraceSingle(
        actor.GetWorld(),
        start,
        end,
        ue.ETraceTypeQuery.TraceTypeQuery1,  # 需确认
        False,
        [],
        ue.EDrawDebugTrace.None,
        True
    )

    if hit_result.bBlockingHit:
        normal = hit_result.ImpactNormal
        # 计算地面坡度角
        slope_angle = math.degrees(math.acos(normal.Z))
        return normal, slope_angle
    return ue.Vector(0, 0, 1), 0.0
```

AnimBP 中：
- 添加 `Slope Warping` 节点
- 输入：地面法线 (`SlopeNormal`)、坡度角度 (`SlopeAngle`)
- 输出：自动调整腿部 IK

### 5.3 Orientation Warping（转身朝向）

**目的**：角色快速转身时，下半身先转，上半身跟随，避免"漂移"。

Python 计算朝向差：

```python
def compute_orientation_warping(actor: ue.Actor, prev_yaw: float):
    """计算朝向变化量"""
    cur_yaw = actor.GetActorRotation().Yaw
    yaw_delta = cur_yaw - prev_yaw
    # 归一化
    if yaw_delta > 180.0:
        yaw_delta -= 360.0
    elif yaw_delta < -180.0:
        yaw_delta += 360.0
    return yaw_delta, cur_yaw
```

AnimBP 中：
- 添加 `Orientation Warping` 节点
- 输入：`YawDelta`、`YawRemaining`
- 节点调整 Spine 旋转

### 5.4 三合一 AnimBP 管线

```
[Locomotion State]
  ↓
[Stride Warping       ← Speed]
  ↓
[Slope Warping        ← SlopeNormal, SlopeAngle]
  ↓
[Orientation Warping  ← YawDelta]
  ↓
[Output Pose]
```

Python 每 Tick 更新：
- `Speed`（已有，Step 2.1）
- `SlopeNormal` + `SlopeAngle`（新增）
- `YawDelta`（新增）

### 5.5 API 总结

| API | 来源 | 状态 |
|-----|------|------|
| `SystemLibrary.LineTraceSingle` | 需验证 | 待搜 |
| `Actor.GetActorRotation().Yaw` | Rotator | 待验证 |
| `Vector` 算术 / 构造 | ue.Vector | 待验证 |
| AnimBP StrideWarping/SlopeWarping/OrientationWarping 节点 | UE5 内置 | 手动配置 |

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `ue.SystemLibrary.LineTraceSingle` | Python 中执行射线检测 | 待验证类是否存在 |
| `ue.Vector(0,0,-200)` 构造 | 创建新 Vector | 待验证 |
| `Rotator.Yaw` | 读取 Yaw 分量 | 待验证 |
| `math.acos / math.degrees` | Python 标准库，应可用 | ✅ |
| AnimBP 中 StrideWarping 节点 | 运行时自动工作 | 需手动配置 AnimBP |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 角色在楼梯/斜坡上行走脚步贴合 | Slope Warping |
| 快跑和慢跑步幅自动适配 | Stride Warping |
| 转身时自然不僵硬 | Orientation Warping |
| 不同英雄转身速度不同 | OrientationWarping 参数可调 |

---

## 八、验证标准

- [ ] Python 正确计算地面法线和坡度
- [ ] AnimBP SlopeWarping 在斜坡上调整脚步
- [ ] AnimBP StrideWarping 消除加减速滑步
- [ ] AnimBP OrientationWarping 转身自然过渡
- [ ] 三节点串联工作正常，无冲突

---

## 九、状态

🔲 待开始
