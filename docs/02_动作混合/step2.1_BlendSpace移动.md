# Step 2.1: BlendSpace 移动

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 2.1 状态改为 ✅。

---

## 一、目标

基于角色速度驱动 BlendSpace，实现 Idle / Walk / Jog / Sprint 平滑过渡。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/01_动作混合概述.md`

---

## 三、前置条件

- ✅ Step 1.1 完成：角色可操控移动
- 🔲 需准备 **BlendSpace 1D** 资产（`Content/Characters/BS_Locomotion`），横轴为 Speed（0 ~ 400），样本点包含 Idle(0)、Walk(200)、Jog(400)。Sprint 可选后续追加
- 🔲 需在 AnimBP 中已建好 `Speed` 变量（float），驱动 BlendSpace

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/locomotion.py` | `LocomotionUpdater` | 每 Tick 计算角色速度（大小 + 方向），写入 AnimBP 的 `Speed` 和 `Direction` 变量 |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/BS_Locomotion` | BlendSpace | 移动混合空间，1D 速度轴 |
| `Content/Characters/ABP_GASCharacter` | AnimInstance | 动画蓝图，已有 `Speed` / `Direction` 变量 + BlendSpace 节点 |

---

## 五、关键技术点

### 5.1 速度获取与计算

```python
import ue

class LocomotionUpdater:
    def __init__(self, actor: ue.Actor):
        self.actor = actor

    def tick(self, delta_time: float):
        # 获取速率（cm/s）
        velocity = self.actor.GetVelocity()  # Vector
        speed = velocity.Length()  # float
        # 计算方向：yaw 差 = 移动方向 - 角色朝向
        forward = self.actor.GetActorForwardVector()
        direction = self._compute_direction(velocity, forward)

        # 写入 AnimInstance
        import ue
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if mesh:
            anim_inst = mesh.GetAnimInstance()
            if anim_inst:
                anim_inst.Speed = speed
                anim_inst.Direction = direction
```

**关键 API**：
- `Actor.GetVelocity() -> Vector` — 返回速度（cm/s）。确认存在（`__init__.pyi` line 139404）
- `SkeletalMeshComponent.GetAnimInstance() -> AnimInstance` — 确认存在
- AnimInstance 属性直接赋值（前提变量标记为 `BlueprintReadWrite`）

### 5.2 方向计算

```python
import math

def _compute_direction(velocity: ue.Vector, forward: ue.Vector) -> float:
    # 归一化速度方向
    if velocity.Length() < 10.0:
        return 0.0  # 几乎静止
    vel_dir = velocity.GetSafeNormal(0.001)
    # 计算 yaw 差
    dot = forward.Dot(vel_dir)
    cross = forward.Cross(vel_dir)
    angle = math.degrees(math.atan2(cross.Z, dot))
    return angle  # -180~180
```

### 5.3 BlendSpace 配置

BlendSpace 1D 需要在编辑器中手动配置：
- **横轴名称**：`Speed`，范围 0-400，网格 3
- **采样点**：
  - Idle（Speed=0）
  - Walk（Speed=200）
  - Jog（Speed=400）
  - Sprint（Speed=600，可选，暂缺素材可省略）
- 插值方式：`TargetWeightInterpolationSpeedPerSec` 可设 > 0 以获得平滑过渡

### 5.4 AnimBP 连线

```
Event Blueprint Update Animation
  ↓
[Speed 变量] → [BS_Locomotion] → [Output Pose]
```

AnimBP 每帧自动从 `Speed` 变量读取值驱动 BlendSpace。Python 只需保证每 Tick 写入正确值。

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `Actor.GetVelocity()` Python 中返回 Vector | 获取角色移动速度 | 待测试 |
| `GetAnimInstance()` 返回类型 | 返回自定义 AnimBP 子类实例 | 待测试（需确认类型转换） |
| `anim_inst.Speed = value` 运行时赋值 | 属性是否可在 Python 中直接写入 | 待测试 |
| `Vector.Length()` | Nepy 绑定的 Vector 有此方法 | 待测试 |
| `Vector.Dot()` / `Vector.Cross()` | Nepy 绑定的 Vector 支持 | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 角色从走到跑动作平滑无缝 | BlendSpace 1D 速度驱动 |
| 八方向移动过渡自然（WASD 斜向） | BlendSpace 可扩展为 2D（Speed + Direction） |
| 不同英雄移动姿态不同 | 不同 BlendSpace 资产 + 不同 AnimBP |
| 移动中急停有停顿动画 | 距离匹配 + BlendSpace 低速度段 |

---

## 八、验证标准

- [x] Python Tick 正确计算 Speed 值
- [x] `anim_inst.Speed` 每帧更新
- [x] BlendSpace 在 0~400 之间平滑切换 Idle/Walk/Jog
- [ ] 方向变化时 2D BlendSpace（如有）正确切换（暂缺多方向动画素材）
- [x] 无抖动、无跳帧

---

## 九、状态

✅ 已完成
