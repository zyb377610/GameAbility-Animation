# Step 4.3: 英雄技能全链路整合

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 4.3 状态改为 ✅。

---

## 一、目标

整合前 12 步所有模块，构建一个完整可控英雄：输入 → GA → GE → Tag → Anim → Montage → Motion Warping → 伤害。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/01_英雄技能系统/step1.1_GAS角色骨架.md` ~ `step1.4_Tag驱动动画.md`
- `docs/02_动作混合/step2.1_BlendSpace移动.md` ~ `step2.3_AimOffset_IK.md`
- `docs/04_GroundMotion/step3.1_DistanceMatching.md` ~ `step3.3_StrideSlopeOrientation.md`
- `docs/03_动画子图/step4.1_LinkedAnimGraph.md`
- `docs/05_技能动画表现/step4.2_Montage_AnimNotify.md`

---

## 三、前置条件

- ✅ Step 1.1 → GAS 角色骨架
- ✅ Step 1.2 → 弹道技能链路
- ✅ Step 1.3 → 冷却与消耗
- ✅ Step 1.4 → Tag 驱动动画
- ✅ Step 2.1 → BlendSpace 移动
- ✅ Step 2.2 → 上下半身分离
- ✅ Step 2.3 → AimOffset + IK
- ✅ Step 3.1 → Distance Matching
- ✅ Step 3.2 → Motion Warping
- ✅ Step 3.3 → Stride/Slope/Orientation
- ✅ Step 4.1 → LinkedAnimGraph
- ✅ Step 4.2 → Montage + AnimNotify

---

## 四、具体实现任务

### 4.1 整合脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/gas/hero_controller.py` | `HeroController` | 统一初始化入口：注册属性、给技能、绑定输入、启动动画监听 |
| `Content/Scripts/gas/abilities/ga_attack.py` | `GA_HeroAttack` | 整合攻击技能：冷却 + 消耗 + Montage + MotionWarping + 伤害 |
| `Content/Scripts/gas/abilities/ga_special.py` | `GA_HeroSpecial` | 特殊技能：突进 + MotionWarping |
| `Content/Scripts/animation/locomotion.py` | 更新 `LocomotionUpdater` | 输出 Speed / Direction / Slope / YawDelta |
| `Content/Scripts/animation/upper_body.py` | 更新 `UpperBodyController` | 整合 AimOffset |
| `Content/Scripts/gas/tag_to_anim.py` | 更新 `TagToAnimListener` | 监听全部状态 Tag |

### 4.2 蓝图资产（最终清单）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/BP_GASCharacter` | Character | 挂载 ASC + MotionWarpingComponent + ControlRigComponent |
| `Content/Characters/ABP_GASCharacter` | AnimInstance | 全功能 AnimBP（BlendSpace + Warping + LinkedAnimGraph + AimOffset） |
| `Content/Characters/BS_Locomotion` | BlendSpace | 移动混合空间 |
| `Content/Characters/AO_Aim` | AimOffset | 瞄准偏移 |
| `Content/Characters/AM_Attack` | AnimMontage | 攻击 Montage（带 AnimNotify） |
| `Content/Characters/AM_Dash` | AnimMontage | 突进 Montage（带 MotionWarping Notify） |
| `Content/Characters/ALI_Weapon` | AnimLayerInterface | 武器动画接口 |

---

## 五、关键技术点

### 5.1 全链路流程图

```
[玩家按下攻击键]
  ↓
[InputAction] → [ASC.TryActivateAbilityByClass(GA_HeroAttack)]
  ↓
[GA_HeroAttack.ActivateAbility()]
  ├─ 1. CommitAbility()
  │    ├─ Apply GE_Cost (扣法力)
  │    └─ Apply GE_Cooldown (启动冷却)
  ├─ 2. PlayMontageAndWait(AM_Attack)
  │    └─ MotionWarping 调整位移
  ├─ 3. 等待 AnimNotify 命中窗口
  │    └─ Apply GE_Damage 到目标
  │         └─ Target ASC → Health 减少
  │              └─ UI 监听 Attribute 变化更新血条
  └─ 4. Montage 结束 → EndAbility()
```

### 5.2 数据流汇总

| 数据类型 | 生产者 | 消费者 |
|----------|--------|--------|
| Speed / Direction | `LocomotionUpdater` (Python Tick) | AnimBP BlendSpace |
| AimPitch / AimYaw | `AimIKController` (Python Tick) | AnimBP AimOffset |
| SlopeNormal / SlopeAngle | `LocomotionUpdater` (Raycast) | AnimBP SlopeWarping |
| YawDelta | `LocomotionUpdater` | AnimBP OrientationWarping |
| bIsDead / bIsStunned / bIsHit | `TagToAnimListener` (Tag Callback) | AnimBP State Machine |
| Health / Mana | `GE_Damage` / `GE_Cost` 修改 | UI Widget |
| Cooldown Tag | `GE_Cooldown` 自动管理 | ASC TryActivateAbility 检查 |
| Warp Target | `GA_Dash` 设定 | MotionWarpingComponent |

### 5.3 完整初始化序列

```python
# Content/Scripts/gas/hero_controller.py
import ue

class HeroController:
    """
    一键初始化英雄的全部系统
    """

    @staticmethod
    def setup_hero(actor: ue.Actor):
        asc = actor.get_component_by_class(ue.AbilitySystemComponent)
        if not asc:
            ue.log_error("[Hero] ASC 未找到，请先执行 Step 1.1")
            return

        # 1. 初始化属性
        asc.InitStats(AttrSet_Base, None)

        # 2. 授予技能
        asc.GiveAbility(GA_HeroAttack, Level=1, InputID=0)   # 左键
        asc.GiveAbility(GA_Dash, Level=1, InputID=1)          # 右键

        # 3. 绑定 Tag → Anim
        from gas.tag_to_anim import bind_all_tags
        bind_all_tags(actor)

        # 4. 启动动画更新
        from animation.locomotion import LocomotionUpdater
        updater = LocomotionUpdater(actor)
        # 通过 world timer 或 Tick 驱动

        ue.log("[Hero] 英雄系统初始化完成")
```

### 5.4 输入绑定

输入绑定在蓝图中更稳定（InputAction → ASC Ability Activation），Python 可选方案：
- `ASC.TryActivateAbilityByClass(GA_HeroAttack)` 直接在 Python 输入回调中调用
- 蓝图输入事件 → Python 函数

### 5.5 技能 GA 模板

所有技能公共模式：

```python
class GA_Base(ue.GameplayAbility):
    """技能基类：封装通用模式"""

    def activate_with_montage(self, montage_path: str, warp_target_name: str = None):
        if not self.CommitAbility():
            self.EndAbility()
            return

        montage = ue.load_object(ue.AnimMontage, montage_path)
        if not montage:
            ue.log_error(f"[GA] Montage 加载失败: {montage_path}")
            self.EndAbility()
            return

        # 设置 Motion Warping Target（如果有）
        if warp_target_name:
            self._set_warp_target(warp_target_name)

        # 播放 Montage
        task = ue.AbilityTask_PlayMontageAndWait.CreatePlayMontageAndWaitProxy(
            self, "None", montage, 1.0
        )
        task.OnCompleted.Add(self._on_montage_completed)
        task.OnInterrupted.Add(self._on_montage_interrupted)
        task.OnCancelled.Add(self._on_montage_cancelled)

    def _on_montage_completed(self):
        self.EndAbility()

    def _on_montage_interrupted(self):
        self.EndAbility()

    def _on_montage_cancelled(self):
        self.EndAbility()
```

### 5.6 最终 Dir 结构

```
Content/Scripts/
├── gas/
│   ├── setup_character.py      # Step 1.1
│   ├── hero_controller.py      # Step 4.3 (整合)
│   ├── tag_to_anim.py          # Step 1.4
│   ├── notify_handler.py       # Step 4.2
│   ├── abilities/
│   │   ├── ga_fireball.py      # Step 1.2
│   │   ├── ga_dash.py          # Step 3.2
│   │   ├── ga_attack.py        # Step 4.3
│   │   └── ga_special.py       # Step 4.3
│   └── effects/
│       ├── ge_damage.py        # Step 1.2
│       ├── ge_cooldown.py      # Step 1.3
│       └── ge_cost.py          # Step 1.3
└── animation/
    ├── locomotion.py           # Step 2.1 + 3.3
    ├── upper_body.py           # Step 2.2
    ├── aim_ik.py               # Step 2.3
    ├── distance_matching.py    # Step 3.1
    └── sub_graph.py            # Step 4.1
```

---

## 六、待验证 API（汇总）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| 所有前 12 步列出的待验证 API | — | 需要逐步验证 |
| 多 GA 同时授予 | 不同 InputID 正确区分 | 待测试 |
| Python 输入回调中调用 `TryActivateAbilityByClass` | 正常激活 | 待测试 |
| 蓝图 InputAction 绑定到 ASC | 蓝图设置 → ASC 自动激活对应 InputID 的 GA | 待测试 |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 完整英雄：移动 + 普攻 + 技能 + 大招 | 全部 GA + GE + Anim 链 |
| 技能之间互不冲突 | GA 的 `BlockAbilitiesWithTag` + `CancelAbilitiesWithTag` |
| 连招流畅 | Montage Section 跳转 + Notify 驱动 |
| UI 实时反馈（血条、技能图标冷却） | Attribute 监听 + GE Cooldown |

---

## 八、验证标准

- [ ] `HeroController.setup_hero()` 一键初始化无报错
- [ ] 攻击键 → GA 激活 → Montage 播放 → 伤害判定 → 目标扣血
- [ ] 移动 + 攻击同时进行（上下半身分离）
- [ ] 冷却期间无法重复释放
- [ ] 法力不足时技能释放失败
- [ ] 受击 Tag → AnimBP 受击状态
- [ ] AimOffset 瞄准正常工作
- [ ] 斜坡上走路脚步贴近地面

---

## 九、状态

🔲 待开始
