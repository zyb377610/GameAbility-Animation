# Step 2.3 总结：AimOffset + IK

## 本节目标

在 BlendSpace 移动 + 上下半身分离基础上，实现角色瞄准偏移和手部 IK。

---

## 完成内容

### 1. AimOffset（瞄准偏移）

**是什么**：一个 2D BlendSpace，根据 Pitch/Yaw 混合不同瞄准角度的上半身姿势。下半身继续走 BlendSpace 移动，上半身独立瞄准。

**数据流**：
```
鼠标移动 → ControlRotation变化 → Python计算delta →
  AimPitch/AimYaw写入AnimBP → AO_Rifle节点混合姿态 →
  LayeredBoneBlend合并上下半身 → 最终Pose
```

**Python 如何计算 Pitch/Yaw**：
- 每帧读 `PlayerCameraManager.GetCameraRotation()`（摄像机朝向）
- 减法：`摄像机旋转 - 角色旋转` → 得到相对瞄准角度
- 写入 AnimBP 变量 `AimPitch` / `AimYaw`

**蓝图配置**（通过 MCP 完成）：
- `ABP_GASCharacter` AnimGraph 中插入 `AO_Rifle` 节点（BS_Locomotion 和 LayeredBoneBlend 之间）
- 变量 `AimPitch` → AO 的 X 轴，`AimYaw` → AO 的 Y 轴

### 2. IK（反向运动学）

**是什么**：先定手部目标位置，反算手臂骨骼角度。让手精确贴合武器握柄，而不是给每把枪做一套不同位置的手部动画。

**FABRIK 节点配置**：
- TipBone = `hand_r`（末端：右手）
- RootBone = `upperarm_r`（根：右上臂）
- Effector = `IKHandTarget_R`（目标位置，Python 写入）
- Alpha = `IKHandAlpha`（混合权重，0=无IK, 1=纯IK）

**Python 驱动**：`update_hand_ik(alpha)` 计算胸前握持位置，写入 AnimBP。当挂载真实武器后，改为读武器握柄 Socket 坐标即可。

**测试方法**：
```
PIE → @locomotion → 控制台执行：
ctrl.update_hand_ik(alpha=1.0)  # 右手抬起贴合目标
ctrl.update_hand_ik(alpha=0.0)  # 恢复动画默认位置
```

### 3. 输入驱动方案

蓝图 InputAxis 事件因 UE5.6 强制 EnhancedInput 不触发。Python 接管全部输入：
- 移动：`GetInputAxisValue("MoveForward"/"MoveRight")`（原有，在 locomotion.py）
- 瞄准：`GetInputAxisValue("Turn"/"LookUp")` → `AddControllerYawInput/PitchInput`（新增，在 aim_ik.py）

---

## AnimGraph 最终管线

```
Speed → BS_Locomotion
          ↓ Pose
AimPitch/Yaw → AO_Rifle (AimOffset)
          ↓ Pose
LayeredBoneBlend (Base=瞄准, Blend=MM_Attack_01, Weight=UpperBodyAlpha)
          ↓ Pose
FABRIK (IK, TipBone=hand_r, RootBone=upperarm_r, Effector=IKHandTarget_R)
          ↓ Pose
输出
```

---

## 关键文件

| 文件 | 作用 |
|------|------|
| `Content/Scripts/animation/aim_ik.py` | AimIKController：Pitch/Yaw 计算 + IK 控制 + 鼠标输入降级 |
| `Content/Characters/ABP_GASCharacter.uasset` | AnimGraph 含 AO_Rifle + FABRIK 节点 |
| `Content/Characters/Mannequins/Anims/Rifle/AIM/AO_Rifle.uasset` | AimOffset 资产（引擎自带） |
| `Content/Scripts/gmcmds.py` | `@locomotion` 命令同时启动移动+瞄准+IK |

---

## 与漫威争锋对照

| 漫威表现 | 本节实现 |
|----------|---------|
| 角色跑动中瞄准方向 | AimOffset 上下半身分离 |
| 不同武器手部姿势自动适配 | IK 反算手臂贴合武器 |
| 准星随鼠标平滑移动 | Python 实时写 Pitch/Yaw 到 AnimBP |
