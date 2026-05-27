# Step 4.1 总结：LinkedAnimGraph 子图分离

> **日期**: 2026-05-27  
> **状态**: ✅ 完成

---

## 一、学到了什么

### 1.1 核心概念对照表

| 概念 | 现实类比 | UE5 中是什么 |
|------|---------|-------------|
| **LinkedAnimGraph 节点** | 墙上的插座 | AnimBP 中的一个 Pose 输入节点，运行时动态链接另一个 AnimBP |
| **AnimLayerInterface** | 插座规格（两孔/三孔） | 纯虚接口类，定义子图必须输出什么样的 Pose |
| **子图 AnimBP** | 电器（电风扇/电暖器） | 实现接口的 AnimBP，运行时被注入到主图的插座上 |
| **Tag** | 插座编号标签 | 字符串，让 Python 能精确定位到某个 LinkedAnimGraph 节点 |
| **LinkAnimGraphByTag** | 插/拔电器动作 | Python API，传入 Class 插上，传入 None 拔掉 |
| **GetLinkedAnimGraphInstanceByTag** | 摸到电器遥控器 | Python API，拿到子图 AnimInstance 引用，可以读写变量 |

### 1.2 为什么需要这个架构

**问题**：所有动画逻辑塞一个 AnimBP → Graph 变蜘蛛网，加一个新武器要改全局

**解法**：
```
                    ABP_GASCharacter（不动）
                          │
            LinkedAnimGraph[Tag="Weapon"]
              ┌───────────┼───────────┐
     ABP_Weapon_Base    ABP_Rifle    ABP_Pistol   ← 各自独立管理
```

- **主图**只管移动（BS_Locomotion），对武器一无所知
- **武器子图**各自管自己的换弹/射击/姿态，互不干扰
- **接口 ALI_Weapon** 保证所有武器子图都有统一的 Pose 输出格式
- **Python 一行切换**：`LinkAnimGraphByTag("Weapon", NewClass)` 

### 1.3 与现有 LayeredBoneBlend 的关系

| | Step 2.2（旧） | Step 4.1（新） |
|---|---|---|
| 上下半身分离方式 | LayeredBoneBlend 固定连一个 Attack 动画 | LayeredBoneBlend 接 LinkedAnimGraph 输出 |
| 上半身动画怎么换 | 改 AnimBP 蓝图连线 | Python 运行时切换子图 |
| 武器动画在哪 | 塞在一个大 AnimBP 里 | 独立 AnimBP，模块化 |
| 加新武器 | 改主图 + 加节点 | 新建子图 + 一行 Python |

---

## 二、创建的全部资产

| 资产 | 路径 | 父类/接口 | 用途 |
|------|------|----------|------|
| **ALI_Weapon** | `Content/Characters/ALI_Weapon` | AnimLayerInterface | 定义 Weapon Layer 接口契约 |
| **ABP_Weapon_Base** | `Content/Characters/ABP_Weapon_Base` | AnimInstance + ALI_Weapon | 武器子图（默认武器） |
| **ABP_Weapon_Alt** | `Content/Characters/ABP_Weapon_Alt` | AnimInstance + ALI_Weapon | 武器子图（备用，用于测试切换） |
| **ABP_GASCharacter** | 已更新 | AnimInstance | 主图，添加 LinkedAnimGraph 节点 |
| **sub_graph.py** | `Content/Scripts/animation/sub_graph.py` | — | SubGraphController + GM 命令 |

### 蓝图配置要点

**ALI_Weapon**：
- 右键 → Animation → Anim Layer Interface → 新建（不是 Blueprint Class）
- 添加 Layer 命名为 `Weapon`

**武器子图**：
- Class Settings → Interfaces → 添加 `ALI_Weapon`
- AnimGraph 中 Weapon Layer → 输出 Pose

**ABP_GASCharacter**：
- 添加 LinkedAnimGraph 节点
- Tag 填 `Weapon`（区分大小写，Python 用此匹配）
- 实例类填 `ABP_Weapon_Base`（默认，可运行时覆盖）
- 连线：LinkedAnimGraph.Output Pose → LayeredBoneBlend.BlendPoses_0

---

## 三、Python 代码架构

### 核心 API

```python
# 插上子图
mesh.LinkAnimGraphByTag("Weapon", ABP_Rifle.Class())
# 引擎：找 Tag="Weapon" 的节点 → 销毁旧子实例 → 创建新子实例 → 注入

# 拔掉子图
mesh.LinkAnimGraphByTag("Weapon", None)
# 引擎：销毁子实例 → LinkedAnimGraph 输出退到参考姿势

# 获取子实例（用来读写子图变量）
weapon_inst = mesh.GetLinkedAnimGraphInstanceByTag("Weapon")
weapon_inst.bIsReloading = True
```

### SubGraphController

```python
class SubGraphController:
    # 链接武器子图
    def link_weapon_subgraph(self, weapon_anim_bp_class) -> bool
    
    # 取消链接
    def unlink_weapon_subgraph(self) -> bool
    
    # 获取子实例引用
    def get_weapon_subgraph(self) -> AnimInstance
    
    # 备用：全局设置所有 LinkedAnimGraph 节点
    def link_via_class_layers(self, anim_bp_class) -> bool
    
    # 打印状态
    def print_status(self)
```

### 模块级注册表

```python
_registry = {}  # {id(mesh): SubGraphController}

def get_or_create_controller(mesh):
    # 惰性创建，避免重复
```

与 `locomotion.py` / `upper_body.py` 一致的注册表模式，解决 Nepy 热重载后 uobject 属性失效的问题。

### GM 命令

| 命令 | 效果 |
|------|------|
| `@sub_graph_link 0` | 装备 ABP_Weapon_Base |
| `@sub_graph_link 1` | 装备 ABP_Weapon_Alt |
| `@sub_graph_unlink` | 卸下武器 |
| `@sub_graph_status` | 查看子图链接状态 |

### 武器注册表

```python
_WEAPON_REGISTRY = {
    0: ("/Game/Characters/ABP_Weapon_Base", "ABP_Weapon_Base"),
    1: ("/Game/Characters/ABP_Weapon_Alt",  "ABP_Weapon_Alt"),
}
```

加新武器：建新 AnimBP + 在这里加一行，其他代码不用动。

### 加载蓝图的正确姿势

```python
# ✅ 正确
weapon_bp = ue.LoadObject(ue.Blueprint, "/Game/Characters/ABP_Weapon_Base")
weapon_bp_class = weapon_bp.GeneratedClass  # 属性，不是方法

# ❌ 错误
ue.load_object(...)         # 不存在，实际是 LoadObject（PascalCase）
weapon_bp.GeneratedClass()  # GeneratedClass 是属性不是方法，不能加括号
```

---

## 四、运行时完整链路

```
玩家输入 → GM 命令 @sub_graph_link 0
    │
    ├─ gmcmds.sub_graph_link(0)
    │     └─ sub_graph.sub_graph_link_test(0)
    │           ├─ _get_mesh_from_pawn() → 拿到角色的 SkeletalMeshComponent
    │           ├─ _WEAPON_REGISTRY[0] → 查到蓝图路径
    │           ├─ ue.LoadObject(ue.Blueprint, path) → 加载蓝图资产
    │           ├─ blueprint.GeneratedClass → 获取 UClass（ABP_Weapon_Base_C）
    │           └─ SubGraphController.link_weapon_subgraph(class)
    │                 └─ mesh.LinkAnimGraphByTag("Weapon", class)
    │
    ├─ 引擎侧：
    │     ├─ 遍历主 AnimBP 中所有 LinkedAnimGraph 节点
    │     ├─ 匹配 Tag="Weapon" 的节点
    │     ├─ 销毁旧子实例（如果有）
    │     ├─ 创建 ABP_Weapon_Base_C 的新实例
    │     └─ 子实例 AnimGraph 输出 → ALI_Weapon 接口 → 注入主图
    │
    └─ 渲染：
          BS_Locomotion.Pose ──→ LayeredBoneBlend.BasePose  （下半身移动）
          武器子图.Pose ──────→ LayeredBoneBlend.BlendPoses_0 （上半身武器）
                      ↓
                  Output Pose（全身动画）
```

---

## 五、碰到的问题与解决

| 问题 | 原因 | 解决 |
|------|------|------|
| 找不到 `Anim Layer Interface` 创建入口 | UE5 菜单分类不同，不在 Blueprint Class 下 | 右键 → Animation → Anim Layer Interface |
| `ue.load_object` 报 AttributeError | Nepy API 是 PascalCase，不是 snake_case | 改为 `ue.LoadObject` |
| `ue.load_class` 报 AttributeError | 同样大小写问题，且 LoadClass 路径格式不同 | 改用 `ue.LoadObject` + `GeneratedClass` |
| `GeneratedClass()` 报 TypeError | `GeneratedClass` 是属性不是方法 | 去掉括号 |
| LinkedAnimGraph 节点 UI 与文档预期不同 | UE5.6 只有 Tag 和实例类，没有 Layer 字段 | Tag 填 "Weapon"，实例类填具体 AnimBP |

### 底层教训

**Nepy API 命名规则**：所有 UE API 都是 PascalCase（`LoadObject`、`LinkAnimGraphByTag`、`GeneratedClass`），即使直觉上像 Python 标准库的命名（`load_object`）也不存在。

---

## 六、项目进度更新

| Phase | Step | 状态 |
|-------|------|:--:|
| Phase 1-3 | Step 1.1 ~ 3.3 | ✅ |
| Phase 4 | Step 4.1 LinkedAnimGraph | ✅ |
| Phase 4 | Step 4.2 Montage + AnimNotify | 🔲 |
| Phase 4 | Step 4.3 英雄技能全链路 | 🔲 |

---

## 七、与漫威争锋对照

| 漫威观察 | 本步实现 |
|----------|---------|
| 不同英雄拿不同武器，动画表现不同 | `@sub_graph_link 0/1` 切换武器子图 |
| 拿武器/空手切换动画过渡自然 | LinkAnimGraphByTag 自动处理 BlendIn/Out |
| 武器换弹动画独立于移动 | Weapon 子图自行管理，下半身移动不受影响 |
| 英雄姿态切换（战斗/非战斗） | 不同子图 Class 运行时切换 |
| 人类体型英雄可能共享 Locomotion | 主图 BS_Locomotion 可复用到多角色 |

---

## 八、下一步

**Step 4.2: 技能 Montage + AnimNotify** — 在武器子图中用 Python 播放 Montage，通过 AnimNotify 事件与 GAS 技能系统联动。

相关文档：`docs/05_技能动画表现/step4.2_Montage_AnimNotify.md`
