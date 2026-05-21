# Step 3.1: Distance Matching 起步/停步

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 3.1 状态改为 ✅。

---

## 一、目标

实现起步/停步动画的距离匹配（Distance Matching），角色移动距离精确对应动画位移。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/step2.1_BlendSpace移动.md`
- `docs/04_GroundMotion/01_GroundMotion概述.md`

---

## 三、前置条件

- ✅ Step 2.1 完成：BlendSpace 移动正常
- 🔲 需准备带有 **Distance Curve** 的动画资产（起步/停步/循环跑）
- 🔲 AnimBP 中使用 **Sequence Evaluator** + **Distance Curve** 计算 PlayRate

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/distance_matching.py` | `DistanceMatchingController` | 计算角色位移增量，写入 AnimBP 变量驱动 Sequence Evaluator |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/ABP_GASCharacter` | AnimInstance（更新） | 添加 `DistanceToMatch` / `DisplacementSinceLastUpdate` 等变量 |
| `Content/Characters/AS_StartWalk` | AnimSequence（带 Distance Curve） | 起步动画 |
| `Content/Characters/AS_StopWalk` | AnimSequence（带 Distance Curve） | 停步动画 |

---

## 五、关键技术点

### 5.1 Distance Matching 原理

```
[角色速度变化] → [Python: 计算位移 delta_distance]
→ [AnimBP: DistanceToMatch 变量]
→ [Sequence Evaluator: 用 Distance Curve 反查时间点]
→ [播放对应帧, 腿部落脚点匹配实际位移]
```

### 5.2 Python 位移计算

```python
import ue

class DistanceMatchingController:
    def __init__(self, actor: ue.Actor):
        self.actor = actor
        self.prev_location = actor.GetActorLocation()

    def tick(self, delta_time: float):
        cur_location = self.actor.GetActorLocation()
        displacement = cur_location - self.prev_location  # 位移向量
        distance = displacement.Length()  # 标量位移(cm)

        # 判断方向：向前还是向后移动
        forward = self.actor.GetActorForwardVector()
        dot = displacement.Dot(forward)
        signed_distance = distance if dot >= 0 else -distance

        # 写入 AnimBP
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if mesh:
            anim_inst = mesh.GetAnimInstance()
            if anim_inst:
                anim_inst.DistanceToMatch = signed_distance
                anim_inst.DisplacementSinceLastUpdate = distance

        self.prev_location = cur_location
```

### 5.3 AnimBP 端配置（手动）

AnimBP 中 Sequence Evaluator 连线：
1. 节点类型：`Sequence Evaluator`（显式时间驱动）
2. 输入动画：`AS_StartWalk`、`AS_Loop`、`AS_StopWalk`
3. 使用 **Distance Curve Modify** 节点读取动画中的 Distance Curve
4. 比较 `DistanceToMatch` 与曲线值，反算 PlayRate
5. 核心逻辑公式：`PlayRate = (DistanceToMatch - CurrentCurveValue) / DeltaTime`

> **注意**：AnimBP 中的距离匹配逻辑主要在动画蓝图的 `Event Blueprint Update Animation` 中实现，Python 负责提供位移数据。

### 5.4 落地脚匹配

距离匹配的关键价值：
- 起步：脚步滑步消除（动画位移 = 角色实际位移）
- 停步：脚步落点准确（不会出现停步后还滑一段）
- 转身：配合 Orientation Warping 使脚底不旋转

### 5.5 API 确认

| API | 来源 | 状态 |
|-----|------|------|
| `Actor.GetActorLocation() -> Vector` | Actor | 确认存在 |
| `Actor.GetActorForwardVector() -> Vector` | Actor | 确认存在 |
| `Vector.Length()` | Vector | 待验证 Nepy |
| `Vector.Dot()` | Vector | 待验证 Nepy |
| `SkeletalMeshComponent.GetAnimInstance()` | SkeletalMeshComponent | 确认存在 |

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `Vector` 算术减法 | `cur - prev` 返回位移 Vector | 待测试 |
| `Vector.Length()` | 返回浮点 | 待测试 |
| `Vector.Dot(forward)` | 返回标量点积 | 待测试 |
| AnimBP 中 Distance Curve 节点 | Python 无需操作，AnimBP 手动配置 | 确认蓝图可用 |
| 运行时写入 `anim_inst.DistanceToMatch` | Sequence Evaluator 正确读取 | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 角色起步无滑步 | Distance Matching 对齐动画位移 |
| 急停时脚步落点精确 | 停步动画 Distance Curve 反算 |
| 不同移动速度动画播放率自动适配 | Sequence Evaluator PlayRate 动态计算 |
| 快速转向脚底有旋转过渡 | 配合 Orientation Warping（Step 3.3） |

---

## 八、验证标准

- [ ] Python 每帧计算位移增量（Delta Distance）
- [ ] `DistanceToMatch` 成功写入 AnimBP
- [ ] 起步时无滑步（脚底贴合地面）
- [ ] 停步时脚部落点准确
- [ ] 加速/减速时 PlayRate 动态调节

---

## 九、状态

🔲 待开始
