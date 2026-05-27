# Step 4.1: LinkedAnimGraph 子图分离

> **AI 新聊天请注意**：读取本文件即可了解这一步的全部任务。完成后更新"验证标准"checkbox，并将 `docs/00_项目总览.md` 中 Step 4.1 状态改为 ✅。

---

## 一、目标

用 LinkedAnimGraph 分离移动子图（Locomotion）和武器子图（Weapon），实现模块化动画蓝图架构。

---

## 二、AI 需要读取的依赖文件

- `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi`
- `docs/02_动作混合/step2.1_BlendSpace移动.md`
- `docs/02_动作混合/step2.2_上下半身分离.md`
- `docs/03_动画子图/01_动画子图概述.md`

---

## 三、前置条件

- ✅ Step 2.1~2.2 完成：移动 BlendSpace + 上下半身分离就绪
- 🔲 需创建 AnimLayerInterface 蓝图（`Content/Characters/ALI_Weapon`）
- 🔲 需创建 Weapon AnimBP（`Content/Characters/ABP_Weapon_Base`）

---

## 四、具体实现任务

### 4.1 Python 脚本

| 文件路径 | 类/函数 | 用途 |
|----------|---------|------|
| `Content/Scripts/animation/sub_graph.py` | `SubGraphController` | 动态 Link/Unlink 武器子图到主动画蓝图 |

### 4.2 蓝图资产（手动创建）

| 路径 | 父类 | 用途 |
|------|------|------|
| `Content/Characters/ALI_Weapon` | AnimLayerInterface | 定义武器子图接口（函数签名） |
| `Content/Characters/ABP_Weapon_Base` | AnimInstance（实现 ALI_Weapon） | 武器动画子图（换弹、射击姿势等） |
| `Content/Characters/ABP_GASCharacter` | AnimInstance（更新） | 主 AnimBP，通过 LinkedAnimGraph 节点引入子图 |

---

## 五、关键技术点

### 5.1 LinkedAnimGraph 架构

```
主 AnimBP (ABP_GASCharacter)
├─ Locomotion (BlendSpace + Warping)
├─ LinkedAnimGraph [Tag="Weapon"]
│   └─ 调用 ALI_Weapon 接口
└─ Output Pose

武器子图 (ABP_Weapon_Base)
├─ 实现 ALI_Weapon 接口
├─ 换弹动画
├─ 射击姿态
└─ Output Pose
```

### 5.2 Python Link/Unlink

```python
import ue

class SubGraphController:
    """
    管理 LinkedAnimGraph 子图的动态链接
    """

    def __init__(self, actor: ue.Actor):
        self.actor = actor

    def link_weapon_subgraph(self, weapon_anim_bp_class):
        """链接武器子图"""
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if not mesh:
            return
        # 方式 1：LinkAnimGraphByTag
        mesh.LinkAnimGraphByTag("Weapon", weapon_anim_bp_class)

    def unlink_weapon_subgraph(self):
        """取消链接武器子图"""
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if not mesh:
            return
        mesh.LinkAnimGraphByTag("Weapon", None)  # None = 清空

    def link_via_class_layers(self, in_class):
        """方式 2：LinkAnimClassLayers（链接到所有 LinkedAnimGraph 节点）"""
        mesh = self.actor.get_component_by_class(ue.SkeletalMeshComponent)
        if not mesh:
            return
        mesh.LinkAnimClassLayers(in_class)
```

### 5.3 API 确认

**从 `__init__.pyi` 中确认的 API**：

| API | 类 | 签名 |
|-----|-----|------|
| `LinkAnimGraphByTag` | SkeletalMeshComponent | `(InTag: Name, InClass: TSubclassOf[AnimInstance] \| type[AnimInstance] \| None) -> None` |
| `LinkAnimClassLayers` | SkeletalMeshComponent | `(InClass: TSubclassOf[AnimInstance] \| type[AnimInstance] \| None) -> None` |
| `GetLinkedAnimGraphInstanceByTag` | SkeletalMeshComponent | `(InTag: Name) -> AnimInstance` |

### 5.4 AnimLayerInterface

`AnimLayerInterface` 是 UE5 的接口类，通过编辑器创建：
1. 右键 Content Browser → Blueprint Class → Anim Layer Interface
2. 在接口中添加函数（如 `GetWeaponPose`），函数签名匹配子图输出

主 AnimBP 中的 LinkedAnimGraph 节点：
- 选择 `AnimLayerInterface` 类型
- Tag 设置为匹配名称（如 "Weapon"）

### 5.5 使用场景

- **换武器**：切换不同武器 AnimBP 子图（手枪 / 步枪 / 近战）
- **载具**：Link 载具动画子图
- **状态覆盖**：受伤时 Link 受伤动画子图

---

## 六、待验证 API（当前不确认，务必列出）

| API 调用 | 预期行为 | 实际结果 |
|---------|---------|---------|
| `SkeletalMeshComponent.LinkAnimGraphByTag` | Python 中动态 Link 子图 | 待测试 |
| `SkeletalMeshComponent.LinkAnimClassLayers` | Python 中动态 Link | 待测试 |
| `LinkAnimGraphByTag("Weapon", None)` | 清空子图 Link | 待测试 |
| `GetLinkedAnimGraphInstanceByTag` | 获取已链接的子图 AnimInstance | 待测试 |
| `AnimLayerInterface` 在 Python 中引用 | `ue.AnimLayerInterface` 类是否存在 | 确认存在（line 259731） |

---

## 七、漫威争锋对照

| 漫威观察 | UE5 对应 |
|----------|---------|
| 不同英雄拿不同武器，动画表现不同 | LinkedAnimGraph 切换武器子图 |
| 拿武器/空手切换时动画过渡自然 | Link/Unlink 子图 |
| 武器换弹动画独立于移动 | Weapon 子图自行管理 |
| 英雄姿态切换（战斗/非战斗） | 不同子图 Stack |

---

## 八、验证标准

- [x] Python `LinkAnimGraphByTag("Weapon", class)` 链接成功
- [x] 子图动画覆盖主 AnimBP 对应部位
- [x] `GetLinkedAnimGraphInstanceByTag` 返回非 None
- [x] `LinkAnimGraphByTag("Weapon", None)` 取消链接还原
- [x] 移动 + 武器子图同时工作（下半身移动，上半身武器动画）
- [x] 武器切换：`@sub_graph_link 0` / `@sub_graph_link 1` 动态切换子图

---

## 九、状态

✅ 已完成
