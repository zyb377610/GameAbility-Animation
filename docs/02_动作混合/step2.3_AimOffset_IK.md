# Step 2.3: AimOffset + IK

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 2.3 状态改为 ✅。

---

## 一、目标

实现瞄准偏移（AimOffset）+ 简易手部 IK，角色可看向目标方向且手部贴合武器。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/step2.1_BlendSpace移动.md`
- `docs/02_动作混合/step2.2_上下半身分离.md`

---

## 三、前置条件

- ✅ Step 2.1 完成：BlendSpace 移动正常
- 🔲 需准备 AimOffset 资产（`Content/Characters/AO_Aim`），含 Pitch（-90~90）/ Yaw（-90~90）
- 🔲 AnimBP 中需已连接 AimOffset 节点到上半身管线

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/aim_ik.py` | `AimIKController` | 计算 Pitch/Yaw 写入 AnimBP 变量；可选简单 IK 调整手持武器位置 |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/AO_Aim` | AimOffset | 瞄准偏移资产（Pitch/Yaw 2D） |
| `Content/Characters/ABP_GASCharacter` | AnimInstance（更新） | 添加 `AimPitch` / `AimYaw` 变量 + AimOffset 节点 |
| 可选：`Content/Characters/CR_HandIK` | ControlRig | 手部 IK ControlRig |

---

## 五、关键技术点

### 5.1 AimOffset 参数计算

```python
import ue

class AimIKController:
    def __init__(self, actor: ue.Actor):
        self.actor = actor

    def tick(self, delta_time: float):
        # 获取摄像机/瞄准方向
        camera_manager = ue.GameplayStatics.GetPlayerCameraManager(self.actor.GetWorld(), 0)
        if not camera_manager:
            return
        cam_rot = camera_manager.GetCameraRotation()  # Rotator

        # 转换为相对于角色朝向的 Pitch/Yaw
        actor_rot = self.actor.GetActorRotation()
        delta = cam_rot - actor_rot
        pitch = delta.Pitch  # 归一化到 -90~90
        yaw = delta.Yaw      # 归一化到 -90~90

        # 写入 AnimBP
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if mesh:
            anim_inst = mesh.GetAnimInstance()
            if anim_inst:
                anim_inst.AimPitch = pitch
                anim_inst.AimYaw = yaw
```

### 5.2 AimOffset 节点配置

AnimBP 中：
```
[上半身 Pose]
  ↓
[AimOffset: AO_Aim] (参数: AimPitch, AimYaw)
  ↓
[LayeredBlendPerBone / 合并到最终 Pose]
```

AimOffset 资产配置（手动）：
- **横轴**：Yaw（-90~90 度）
- **纵轴**：Pitch（-90~90 度）
- 采样点：多个角度方向的瞄准姿态

### 5.3 简易 IK（ControlRig 方案）

`ue.ControlRigComponent` API 确认存在：

```python
class ControlRigComponent(PrimitiveComponent):
    ControlRigClass: TSubclassOf[ControlRig]
    ControlRig: ControlRig

    # 通过 MappedElements 驱动骨骼
    UserDefinedElements: ArrayWrapper[ControlRigComponentMappedElement]
    MappedElements: ArrayWrapper[ControlRigComponentMappedElement]

    bUpdateRigOnTick: bool
    bResetTransformBeforeTick: bool
```

IK 流程（简易方案）：
1. 在编辑器中创建 ControlRig 蓝图 `CR_HandIK`，包含 TwoBoneIK 节点
2. 角色蓝图中添加 `ControlRigComponent`，`ControlRigClass` 设为 `CR_HandIK`
3. Python 控制 IK 目标点（如武器握柄位置）

```python
ctrl_rig_comp = actor.get_component_by_class(ue.ControlRigComponent)
if ctrl_rig_comp and ctrl_rig_comp.ControlRig:
    # 设置 IK 目标骨骼位置
    ctrl_rig_comp.ControlRig.set_editor_property("IKTarget_Hand_R", target_location)
```

### 5.4 更轻量方案：FABRIK / TwoBoneIK 直接在 AnimBP

如果 ControlRig 过于复杂，AnimBP 内可直接用 FABRIK 或 TwoBoneIK 节点，Python 只需设置目标位置（通过 AnimBP 变量传递 `Vector`）。

### 5.5 API 总结

| API | 来源 |
|-----|------|
| `GameplayStatics.GetPlayerCameraManager(World, PlayerIndex) -> PlayerCameraManager` | ue |
| `PlayerCameraManager.GetCameraRotation() -> Rotator` | 需验证是否暴露 |
| `Actor.GetActorRotation() -> Rotator` | Actor |
| `Rotator.Pitch / .Yaw` | 需验证 Nepy 绑定 |
| `ControlRigComponent` | 完整存在（line 159426） |

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `GameplayStatics.GetPlayerCameraManager` | Python 获取摄像机管理器 | 待测试 |
| `Rotator` 算术减法 `cam_rot - actor_rot` | 返回 delta Rotator | 待测试（Nepy 可能不支持，需手动计算） |
| `Rotator.Pitch / .Yaw` | 读取分量 | 待测试 |
| `ControlRigComponent.ControlRig` 属性访问 | 获取 ControlRig 实例 | 待测试 |
| AimOffset 参数 `set_editor_property` | 运行时写入 Pitch/Yaw | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 角色瞄准时上半身转向准星 | AimOffset Pitch/Yaw |
| 瞄准时手臂和枪口对准目标 | IK (TwoBoneIK / FABRIK) |
| 不同英雄瞄准姿态不同 | 不同 AimOffset 资产 |
| 准星随鼠标移动平滑跟随 | AnimBP 参数实时写入 |

---

## 八、验证标准

- [ ] Python 计算 Pitch/Yaw 差值
- [ ] `AimPitch` / `AimYaw` 成功写入 AnimBP
- [ ] AimOffset 驱动角色上半身转向
- [ ] 移动中瞄准偏移不干扰下半身 BlendSpace
- [ ] （可选）IK 使手部贴合武器位置

---

## 九、状态

🔲 待开始
