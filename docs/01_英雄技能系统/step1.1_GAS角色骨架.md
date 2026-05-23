# Step 1.1: Python GAS 角色骨架

> **AI 新聊天请注意**：读取本文件 + `docs/00_项目总览.md`（含通用 AI 规则）即可了解全部任务。
> **写 NePy 代码前必须先读** `.codemaker/codewiki/nepy/class-authoring.md` 和 `.codemaker/codewiki/nepy/project-setup.md`。
> 完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.1 状态改为 ✅。

---

## 一、目标

用一个 Python 脚本定义 `AttrSet_Base`（UE5 GAS AttributeSet 子类），并提供运行时初始化函数，在角色 BeginPlay 时为实例动态注册 ASC + AttributeSet。

---

## 二、重要：AI 需要读取的依赖文件

| 文件 | 用途 |
|------|------|
| `.codemaker/codewiki/nepy/class-authoring.md` | 🔑 NePy 类编写规范（@ue.uclass/@ue.uproperty 等） |
| `.codemaker/codewiki/nepy/project-setup.md` | 🔑 nepyinit 生命周期、import 即注册 |
| `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi` | GAS API 签名 (`InitStats`, `GetAttributeSet`, `FindClass` 等) |
| `Plugins/NePythonBinding/Tools/pystubs/ue/builtin_doc.pyi` | 内置类型用法 |

> **本项目全部 Python + Nepy，不写 C++。**

---

## 三、前置条件

| 条件 | 状态 |
|------|:--:|
| 项目可打开，Nepy 加载成功 | ✅ |
| `Content/Scripts/nepyinit.py` 正常 | ✅ |
| `Content/Characters/BP_GASCharacter` 蓝图已创建 | 🔲 |
| 蓝图中已手动添加 AbilitySystemComponent 组件 | 🔲 |

### 蓝图预准备（手动在编辑器做一次）

1. `Content/Characters/` 下创建 Blueprint Class → 父类 `Character` → 命名 `BP_GASCharacter`
2. 打开蓝图，Components 面板：
   - **Mesh** → SkeletalMesh 选 `SKM_Manny_Simple`, Anim Class 选 `ABP_Unarmed`
   - **+ Add Component** → 添加 `AbilitySystemComponent`
3. **Compile → Save**

---

## 四、已验证的关键事实（不要重复踩坑）

| # | 事实 | 来源 |
|---|------|------|
| 1 | `@ue.uclass()` 的 AttributeSet 子类**无需**也用不上（Nepy 不支持注册为独立 UClass），`AttrSet_Base` 直接用普通 `class AttrSet_Base(ue.AttributeSet)` 定义 | 实测 |
| 2 | `InitStats(AttrSet_Base.Class(), None)` 需要的参数是 `AttrSet_Base.Class()`（调用返回 UClass） | 实测 |
| 3 | CDO 上 `AddComponentByClass` 不可用（world == nullptr）→ 放弃在 CDO 上操作 | 实测 |
| 4 | `ue.log_error` **不存在** → 用 `ue.log_warning` 或 `print` | 实测 |
| 5 | `GetAttributeSet(AttrSet_Base.Class())` 必须传 `AttrSet_Base.Class()` 而非类本身 | 实测 |
| 6 | API 用 PascalCase（`InitStats`, `GetAttributeSet`），不是 snake_case | 实测 |
| 7 | 蓝图已有的 ASC 组件运行时通过 `GetComponentByClass` 可正常获取 | 实测 |
| 8 | `nepyinit.py` 的 `on_init()` 中必须 `import gas.setup_character` | 实测 |

---

## 五、已有实现

### 5.1 脚本：`Content/Scripts/gas/setup_character.py`

已就绪。定义了 `AttrSet_Base` 和 `init_gas_for_actor()`。

### 5.2 nepyinit.py 中的 import

`Content/Scripts/nepyinit.py` 的 `on_init()` 中已加入：

```python
import gas.setup_character
```

---

## 六、验证标准

PIE 运行后，在 Console 中执行：

```python
import ue
w = ue.GetGameWorld()
ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
pawn = ctrl.Pawn

from gas.setup_character import init_gas_for_actor
init_gas_for_actor(pawn)
```

预期输出：

```
[GAS] ASC: <AbilitySystemComponent 'AbilitySystem' at ...>
[GAS] AttributeSet 已通过 InitStats 注册
[GAS] Health=100.0, MaxHealth=100.0, AttackPower=10.0, MoveSpeed=600.0
[GAS] 角色骨架初始化完成
```

- [x] 无报错
- [x] ASC 获取成功
- [x] AttributeSet 注册成功
- [x] 默认值正确

---

## 七、漫威争锋对照

| 漫威争锋观察点 | UE5 对应 |
|---------------|---------|
| 每个英雄有血量、护盾值 | `AttributeSet` 中的 `Health`, `MaxHealth` |
| 不同英雄属性基础值不同 | 不同的 AttributeSet 子类默认值不同 |
| 属性变更表现（闪红、数字飘字） | GE + GameplayCue（后续 Step） |

---

## 八、当前 Step 状态

| 状态 | 说明 |
|:--:|------|
| ✅ 已验证 | Step 1.1 完成，可进入 Step 1.2 |
