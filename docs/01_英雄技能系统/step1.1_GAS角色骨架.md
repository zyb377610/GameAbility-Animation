# Step 1.1: Python GAS 角色骨架

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 1.1 状态改为 ✅。

---

## 一、目标

用一个 Python 脚本，在 UE 编辑器中**一键创建一个自带 GAS（AbilitySystemComponent + AttributeSet）的可操控角色**，为后续所有技能、动画步骤提供基础 Actor。

---

## 二、重要：AI 需要读取的依赖文件

| 文件 | 用途 |
|------|------|
| `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi` | 查 GAS 相关 API（`AbilitySystemComponent`, `GameplayAbility`, `AttributeSet`, `GameplayEffect` 的方法签名） |
| `Plugins/NePythonBinding/Tools/pystubs/ue/builtin_doc.pyi` | 查 `Name`, `Text`, `TSubclassOf` 等类型别名用法 |
| **不需要** 读完整个文件，按需搜索即可 |

> **注意**：本项目全部用 Python + Nepy，不写 C++。所有操作通过 `import ue` 完成。

---

## 三、前置条件

- ✅ 项目已能正常打开
- ✅ Nepy 插件加载成功（打开编辑器后 Python Console 可用）
- ✅ `Content/Scripts/nepyinit.py` 可正常执行
- 🔲 **需先创建**：一个第三人称角色蓝图（用UE默认的 Mannequin + 第三人称动画蓝图），路径 `Content/Characters/BP_GASCharacter`

### 蓝图预准备（手动在编辑器做一次）

1. 在 `Content/Characters/` 下创建 Blueprint Class，父类选 `Character`
2. 命名为 `BP_GASCharacter`
3. Mesh 设为 SK_Mannequin，AnimBP 选 UE 默认的第三人称动画蓝图
4. 这个蓝图**不需要手动挂任何组件**——Python 脚本会动态添加 ASC

---

## 四、具体实现任务

### 4.1 Python 脚本：`Content/Scripts/gas/setup_character.py`

创建 `Content/Scripts/gas/` 目录，编写以下脚本：

```python
# -*- encoding: utf-8 -*-
"""
Step 1.1: GAS 角色骨架
一键创建 / 配置带 ASC + AttributeSet 的角色
"""
import ue


# ==================== 1. 自定义 AttributeSet ====================

class AttrSet_Base(ue.AttributeSet):
    """基础属性集：血量、最大血量、攻击力、速度"""
    Health: float = 100.0
    MaxHealth: float = 100.0
    AttackPower: float = 10.0
    MoveSpeed: float = 600.0


# ==================== 2. 工具函数 ====================

def setup_gas_character(bp_path="/Game/Characters/BP_GASCharacter"):
    """
    给指定蓝图添加 AbilitySystemComponent，
    并注册自定义 AttributeSet 和初始化属性。
    调用方式：在 Python Console 中执行
        from gas.setup_character import setup_gas_character
        setup_gas_character()
    """
    # 1) 加载蓝图
    bp_class = ue.load_class(bp_path)
    if not bp_class:
        ue.log_error(f"[GAS] 蓝图加载失败: {bp_path}")
        return

    cdo = bp_class.get_default_object()
    if not cdo:
        ue.log_error("[GAS] CDO 获取失败")
        return

    # 2) 检查是否已添加 ASC
    existing_asc = cdo.get_component_by_class(ue.AbilitySystemComponent)
    if existing_asc:
        ue.log_warning("[GAS] ASC 已存在，跳过添加")
    else:
        # 在蓝图中添加 ASC 组件
        asc = cdo.add_component_by_class(
            ue.AbilitySystemComponent, False, ue.Transform(), False
        )
        if asc:
            ue.log("[GAS] AbilitySystemComponent 添加成功")
        else:
            ue.log_error("[GAS] ASC 添加失败")
            return

    # 3) 初始化属性 —— 首次运行时 AttributeSet 会自动创建
    asc = cdo.get_component_by_class(ue.AbilitySystemComponent)
    if asc:
        attr_set = asc.get_attribute_set(AttrSet_Base)
        if not attr_set:
            ue.log("[GAS] AttributeSet 首次创建，使用默认值")
        else:
            ue.log(f"[GAS] AttributeSet 已存在 Health={attr_set.Health}")

    ue.log("[GAS] 角色骨架搭建完成")
    return cdo


# ==================== 3. 蓝图保存（可选） ====================

def save_blueprint(bp_path="/Game/Characters/BP_GASCharacter"):
    """保存蓝图资产"""
    bp = ue.load_object(ue.Blueprint, bp_path)
    if bp and hasattr(bp, 'save'):
        bp.save()
        ue.log("[GAS] 蓝图已保存")
```

---

### 4.2 待确认的技术点（当前聊天中验证）

以下 API 在写脚本时需要通过 UE Python Console 实测：

| # | 待确认的 API 调用 | 预期行为 | 实际结果 |
|---|-------------------|---------|---------|
| 1 | `add_component_by_class(ue.AbilitySystemComponent, ...)` 是否能在 Python 中添加组件 | 成功添加 | 待测试 |
| 2 | `asc.get_attribute_set(AttrSet_Base)` 是否会自动创建实例 | 首次返回 None，需调用 `InitStats` | 待测试 |
| 3 | Python 继承的 `ue.AttributeSet` 子类是否能被 ASC 正确识别 | 能用 `GetAttributeSet` 获取 | 待测试 |

> **如果某个 API 不可用**：改用蓝图方式——在 BP_GASCharacter 蓝图中手动拖入 ASC 组件，Python 脚本只负责属性初始化和技能注册。

---

## 五、关键技术点

### 5.1 Nepy 子类化 UE 类型

```python
class AttrSet_Base(ue.AttributeSet):
    Health: float = 100.0    # 自动绑定为 FGameplayAttributeData
    MaxHealth: float = 100.0
```

Nepy 会在 Python 类定义时自动生成对应的 UE Native 类型，并注册到反射系统。

### 5.2 ASC 组件挂载

方式 A（Python 动态添加）：
```python
cdo.add_component_by_class(ue.AbilitySystemComponent, False, Transform(), False)
```

方式 B（蓝图预置）：如果方式 A 不支持，改为在 BP_GASCharacter 蓝图中手动拖 ASC 组件。

### 5.3 AttributeSet 注册

ASC 通常通过以下两种方式之一识别 AttributeSet：
- `InitStats(AttrSetClass, DataTable)` — 从 DataTable 初始化
- `GetOrCreateAttributeSubobject()` — UE5 内置方式

---

## 六、漫威争锋对照

| 漫威争锋观察点 | UE5 对应 |
|---------------|---------|
| 每个英雄有血量、护盾值 | `AttributeSet` 中的 `Health`, `MaxHealth`, `Shield` |
| 英雄受到伤害时血量条变化 | GE 修改 Attribute → UI 绑定额 |
| 不同英雄属性基础值不同 | 不同的 Python 子类 AttributeSet 默认值不同 |
| 属性变更表现（闪红、数字飘字） | GE + GameplayCue 或手动监听 Attribute 变化 |

---

## 七、验证标准

- [ ] 在 Python Console 中执行 `from gas.setup_character import setup_gas_character; setup_gas_character()` 无报错
- [ ] 日志输出 `[GAS] AbilitySystemComponent 添加成功`
- [ ] 在蓝图中能看到 ASC 组件
- [ ] `asc.get_attribute_set(AttrSet_Base)` 返回非 None
- [ ] AttributeSet 默认值正确（Health=100 等）

---

## 八、当前 Step 状态

| 状态 | 说明 |
|:--:|------|
| 🔲 待开始 | 下一步：在编辑器中测试 Python API 可用性，修正脚本中的 API 调用 |

---

## 九、完成后

1. 更新 `docs/00_项目总览.md` 中 Step 1.1 状态为 ✅
2. 如果有 API 与预期不符，**务必在本文件中记录**，方便下一步 AI 知道实际情况
3. 提交 Git
