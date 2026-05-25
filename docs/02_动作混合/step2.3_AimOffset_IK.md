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
- ✅ AimOffset 资产（`Content/Characters/Mannequins/Anims/Rifle/AIM/AO_Rifle`）
- ✅ AnimBP 中已连接 AimOffset 节点（AO_Rifle）到 BlendSpace → LayeredBoneBlend 管线
- ✅ FABRIK IK 节点已接入 AnimGraph（hand_r → upperarm_r）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/aim_ik.py` | `AimIKController` | 计算 Pitch/Yaw 写入 AnimBP 变量；可选简单 IK 调整手持武器位置 |

### 4.2 蓝图资产

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

AnimBP 中已经完成：
```
BS_Locomotion
  ↓ Pose
AO_Rifle (AimOffset, 参数: AimPitch/X, AimYaw/Y)
  ↓ Pose
LayeredBoneBlend (BasePose=瞄准, BlendPoses_0=上半身蒙太奇)
  ↓ Pose
FABRIK (右手IK)
  ↓ Pose
输出
```

加入 AO 资产后的实际效果：
- `AO_Rifle` 提供瞄准姿态（BasePose 从 BS_Locomotion 递进）
- 上下半身分离通过 LayeredBoneBlend 在上半身混合 Montage（如挥拳）
- 瞄准偏移在上半身骨骼上叠加

### 5.3 IK（FABRIK）

AnimBP 中已有 FABRIK 节点，配置：
- TipBone = `hand_r`（右手）
- RootBone = `upperarm_r`（右上臂）
- Effector = `IKHandTarget_R`（Python 写入目标位置）
- Alpha = `IKHandAlpha`（混合权重 0~1）

Python 驱动 `update_hand_ik(alpha)` 计算胸前握持位置，FABRIK 反算手臂骨骼。

### 5.4 输入驱动方案

蓝图 InputAxis 因 UE5.6 强制 EnhancedInput 无法走旧版路线，Python 接管全部输入：
- 移动：`GetInputAxisValue("MoveForward"/"MoveRight")` 直接读键盘轴值
- 瞄准：`GetInputAxisValue("Turn"/"LookUp")` → `AddControllerYawInput/AddControllerPitchInput`

以上在 `aim_ik.py` 的 `_drive_mouse_input()` 中实现。

---

## 六、API 验证结果

| API 调用 | 结果 |
|---------|------|
| `GameplayStatics.GetPlayerCameraManager` | ✅ 可用 |
| `Rotator` 算术减法 `cam_rot - actor_rot` | ✅ Nepy 支持 |
| `Rotator.Pitch / .Yaw` | ✅ 可用 |
| `Rotator.GetNormalized()` | ✅ 可用 |
| `AddControllerYawInput/AddControllerPitchInput` | ✅ 在 Pawn 上（非 Controller） |
| `K2_GetComponentToWorld` | ✅ 可用 |
| AnimBP 变量写入 `_write_anim_var` | ✅ 可用 |

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

- [x] Python 计算 Pitch/Yaw 差值
- [x] `AimPitch` / `AimYaw` 成功写入 AnimBP
- [x] AimOffset 驱动角色上半身转向
- [x] 移动中瞄准偏移不干扰下半身 BlendSpace
- [x] （可选）IK 使手部贴合武器位置

---

## 九、状态

✅ 已完成
