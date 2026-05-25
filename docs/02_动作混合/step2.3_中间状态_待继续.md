# Step 2.3 中间状态文档 — 已修复

## 当前目标

实现 AimOffset + IK，已完成。

---

## 已完成的修复和新增

### 1. DefaultInput.ini 修复（根因）
- `DefaultPlayerInputClass` 从 `EnhancedInput.EnhancedPlayerInput` → `Engine.PlayerInput`
- `DefaultInputComponentClass` 从 `EnhancedInput.EnhancedInputComponent` → `Engine.InputComponent`
- **需要重启编辑器才能生效**

### 2. ABP_GASCharacter 变量新增
| 变量 | 类型 | 用途 |
|------|------|------|
| AimPitch | float | AimOffset 上下偏移 |
| AimYaw | float | AimOffset 左右偏移 |
| IKHandAlpha | float | FABRIK 混合权重 (0~1) |
| IKHandTarget_R | Transform | 右手 IK 目标位置 |

### 3. AnimGraph 管线更新
```
BS_Locomotion [Speed → X]
  ↓ Pose
AO_Rifle (AimOffset) [AimPitch → X, AimYaw → Y]
  ↓ Pose
LayeredBoneBlend [UpperBodyAlpha, BlendPoses_0=MM_Attack_01]
  ↓ Pose
FABRIK [IKHandAlpha, IKHandTarget_R, TipBone=hand_r, RootBone=upperarm_r]
  ↓ Pose
Root (输出)
```

### 4. Python 脚本修改

#### `aim_ik.py`
- 新增 `_drive_mouse_input()` — 降级方案：Python 直接读 `GetInputAxisValue("Turn"/"LookUp")` 并调用 `AddControllerYawInput/AddControllerPitchInput`，确保蓝图 InputAxis 不触发时鼠标仍能驱动 ControlRotation
- 新增 `update_hand_ik(alpha)` — 计算右手 IK 目标位置并写入 AnimBP

#### `gmcmds.py` `@locomotion` 命令
- 现在同时注册/取消 LocomotionUpdater + AimIKController

---

## 待用户验证

1. **重启编辑器**（DefaultInput.ini 需重启才能被 UInputSettings 重新缓存）
2. PIE 后输入 `@locomotion` 启动移动+瞄准系统
3. 移动 WASD 观察 BlendSpace 移动 + 鼠标移动观察 AimOffset 效果
4. 可选：在 Python 控制台调用 `aim_ik.AimIKController.update_hand_ik(... 1.0)` 测试 IK

---

## 不再需要排查的问题

- ~~蓝图 InputAxis 事件不触发~~ → DefaultInput.ini 修复 + Python 降级方案双重保障
- ~~AimOffset 节点未接入~~ → 已完成
- ~~IK 未配置~~ → FABRIK 节点已添加到管线

---

## 关键文件路径

| 文件 | 路径 |
|------|------|
| Python 瞄准脚本 | `Content/Scripts/animation/aim_ik.py` |
| 角色蓝图 | `Content/Characters/BP_GASCharacter.uasset` |
| 输入配置 | `Config/DefaultInput.ini` |
| AnimBP | `Content/Characters/ABP_GASCharacter.uasset` |
| AimOffset 资产 | `Content/Characters/Mannequins/Anims/Rifle/AIM/AO_Rifle.uasset` |
| 项目总览 | `docs/00_项目总览.md` |
