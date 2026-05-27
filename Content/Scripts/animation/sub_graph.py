# -*- encoding: utf-8 -*-
"""
Step 4.1: 动画子图（LinkedAnimGraph）— SubGraphController

核心思路：
  主 AnimBP 中通过 LinkedAnimGraph 节点引用武器子图，运行时用 Python
  动态 Link/Unlink，实现"移动 + 武器动画"的模块化分离。

架构：
  主 AnimBP (ABP_GASCharacter)
    ├─ BS_Locomotion → LayeredBoneBlend.BasePose   (下半身移动)
    └─ LinkedAnimGraph[Tag="Weapon", InstanceClass=ABP_Weapon_Base]
         └─ → LayeredBoneBlend.BlendPoses_0        (上半身武器)

  Python 控制:
    - LinkAnimGraphByTag("Weapon", ABP_Rifle)     → 切步枪子图
    - LinkAnimGraphByTag("Weapon", ABP_Pistol)    → 切手枪子图
    - LinkAnimGraphByTag("Weapon", None)          → 清空，上半身回退

技术要点：
  1. LinkedAnimGraph 节点通过 Tag 匹配 → Python 侧必须 Tag 名完全一致
  2. 子图和主图是独立的 AnimInstance，各自有自己的变量空间
  3. 向子图传数据两种方式：
     a. GetLinkedAnimGraphInstanceByTag 获取子实例 → 直接写属性
     b. GameplayTag 驱动（双方都读同一个 Tag 状态）
  4. LinkAnimClassLayers 是全局的（作用于所有 LinkedAnimGraph 节点），
     LinkAnimGraphByTag 是精确的（只影响指定 Tag 的节点）

用法：
    from animation.sub_graph import SubGraphController

    mesh = character.Mesh  # SkeletalMeshComponent
    ctrl = SubGraphController(mesh)

    # 链接武器子图
    ctrl.link_weapon_subgraph(ABP_Rifle.Class())

    # 获取子实例，写入变量
    weapon_inst = ctrl.get_weapon_subgraph()
    if weapon_inst:
        weapon_inst.bIsReloading = True

    # 清空武器子图
    ctrl.unlink_weapon_subgraph()

测试命令（Nepy 控制台）：
    > nepy sub_graph_link     # 对选中的角色链接 ABP_Weapon_Base
    > nepy sub_graph_unlink   # 取消链接
    > nepy sub_graph_status   # 查看子图状态
"""

import ue


# ==================== 1. SubGraphController ====================

class SubGraphController:
    """
    管理 LinkedAnimGraph 子图的动态链接。

    纯 Python 对象，挂载在 SkeletalMeshComponent 上使用。
    模块级注册表管理生命周期（与 locomotion.py / upper_body.py 相同模式）。

    Attributes:
        mesh: 目标 SkeletalMeshComponent
        current_weapon_class: 当前链接的武器子图类（None = 未链接）
    """

    def __init__(self, mesh):
        """
        Args:
            mesh: ue.SkeletalMeshComponent，角色的 Mesh 组件
        """
        self.mesh = mesh
        self.current_weapon_class = None

    # ---- 武器子图链接 / 取消 ----

    def link_weapon_subgraph(self, weapon_anim_bp_class):
        """
        链接武器子图到主 AnimBP 的 LinkedAnimGraph[Tag="Weapon"] 节点。

        原理：
          1. 引擎遍历主 AnimBP 中所有 LinkedAnimGraph 节点
          2. 找到 Tag == "Weapon" 的那个
          3. 创建 weapon_anim_bp_class 的新 AnimInstance 作为子实例
          4. 子实例的 AnimGraph 输出自动注入到主图

        Args:
            weapon_anim_bp_class: 武器子图类（如 ABP_Weapon_Base.Class()）
                                  必须是 AnimInstance 的子类
        Returns:
            bool, 是否链接成功
        """
        if not self.mesh:
            print("[SubGraph] 错误: mesh 为空")
            return False

        try:
            # LinkAnimGraphByTag 签名:
            #   (InTag: Name, InClass: TSubclassOf[AnimInstance] | type[AnimInstance] | None) -> None
            self.mesh.LinkAnimGraphByTag("Weapon", weapon_anim_bp_class)
            self.current_weapon_class = weapon_anim_bp_class

            cls_name = weapon_anim_bp_class.GetName() if weapon_anim_bp_class else "Unknown"
            print(f"[SubGraph] 武器子图已链接: {cls_name} (Tag='Weapon')")
            return True
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[SubGraph] 链接失败: {e}")
            return False

    def unlink_weapon_subgraph(self):
        """
        取消链接武器子图。

        传入 None → 引擎销毁子实例，主 AnimBP 的 LinkedAnimGraph 输出
        退回到参考姿势（或默认值），上半身回到 BasePose 状态。

        Returns:
            bool, 是否取消成功
        """
        if not self.mesh:
            print("[SubGraph] 错误: mesh 为空")
            return False

        try:
            self.mesh.LinkAnimGraphByTag("Weapon", None)
            self.current_weapon_class = None
            print("[SubGraph] 武器子图已取消链接")
            return True
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[SubGraph] 取消链接失败: {e}")
            return False

    # ---- 获取子实例 ----

    def get_weapon_subgraph(self):
        """
        获取当前链接的武器子图 AnimInstance。

        通过这个引用可以向子图中写入变量，例如：
            weapon = ctrl.get_weapon_subgraph()
            weapon.bIsShooting = True

        Returns:
            AnimInstance 或 None（如果未链接或 Tag 不匹配）
        """
        if not self.mesh:
            return None

        try:
            # GetLinkedAnimGraphInstanceByTag 签名:
            #   (InTag: Name) -> AnimInstance
            inst = self.mesh.GetLinkedAnimGraphInstanceByTag("Weapon")
            return inst
        except Exception as e:
            print(f"[SubGraph] 获取子实例失败: {e}")
            return None

    # ---- 备用方式：LinkAnimClassLayers ----

    def link_via_class_layers(self, anim_bp_class):
        """
        备用方式：通过 LinkAnimClassLayers 全局设置所有 LinkedAnimGraph 节点。

        与 LinkAnimGraphByTag 的区别：
          - LinkAnimClassLayers: 一次性设置所有 LinkedAnimGraph 节点的实现类
          - LinkAnimGraphByTag: 精确指定某个 Tag 对应的子图（推荐）

        Args:
            anim_bp_class: AnimInstance 子类（或 None 清空）
        Returns:
            bool
        """
        if not self.mesh:
            print("[SubGraph] 错误: mesh 为空")
            return False

        try:
            self.mesh.LinkAnimClassLayers(anim_bp_class)
            cls_name = anim_bp_class.GetName() if anim_bp_class else "None"
            print(f"[SubGraph] LinkAnimClassLayers 已设置: {cls_name}")
            return True
        except Exception as e:
            import traceback
            traceback.print_exc()
            print(f"[SubGraph] LinkAnimClassLayers 失败: {e}")
            return False

    # ---- 状态查询 ----

    def print_status(self):
        """打印当前链接状态"""
        print("=" * 50)
        print("[SubGraph] 状态报告")
        print(f"  Mesh: {self.mesh}")
        print(f"  当前武器子图: {self.current_weapon_class}")

        weapon_inst = self.get_weapon_subgraph()
        if weapon_inst:
            print(f"  子实例: {weapon_inst}")
            print(f"  子实例类: {weapon_inst.__class__.__name__}")
        else:
            print("  子实例: None (未链接)")

        print("=" * 50)


# ==================== 2. 模块级注册表 ====================

_registry = {}  # {id(mesh): SubGraphController}


def get_or_create_controller(mesh):
    """
    获取或创建 SubGraphController（惰性初始化）。

    Args:
        mesh: ue.SkeletalMeshComponent

    Returns:
        SubGraphController 实例
    """
    key = id(mesh)
    if key not in _registry:
        _registry[key] = SubGraphController(mesh)
    return _registry[key]


# ==================== 3. GM 测试命令 ====================

# 武器注册表：{编号: (路径, 名称)}
_WEAPON_REGISTRY = {
    0: ("/Game/Characters/ABP_Weapon_Base", "ABP_Weapon_Base"),
    1: ("/Game/Characters/ABP_Weapon_Alt",  "ABP_Weapon_Alt"),
}


def _get_mesh_from_pawn():
    """获取当前控制角色的 SkeletalMeshComponent，复用逻辑"""
    world = ue.GetGameWorld()
    if not world:
        print("[SubGraph] 世界不存在，请先 PIE")
        return None

    pc = ue.GameplayStatics.GetPlayerController(world, 0)
    if not pc:
        print("[SubGraph] 没有 PlayerController")
        return None

    pawn = pc.Pawn
    if not pawn:
        print("[SubGraph] 没有 Pawn")
        return None

    mesh = pawn.Mesh if hasattr(pawn, 'Mesh') else None
    if not mesh:
        mesh = pawn.get_component_by_class(ue.SkeletalMeshComponent.Class())
    if not mesh:
        print("[SubGraph] 角色无 SkeletalMeshComponent")
        return None
    return mesh


def sub_graph_link_test(weapon_index=0):
    """
    Nepy 控制台测试命令：切换武器子图。

    用法：
        > nepy sub_graph_link         # 默认武器 0 (ABP_Weapon_Base)
        @sub_graph_link 0              # 装备 ABP_Weapon_Base
        @sub_graph_link 1              # 装备 ABP_Weapon_Alt

    Args:
        weapon_index: int, 武器编号 (0 或 1)
    """
    try:
        mesh = _get_mesh_from_pawn()
        if not mesh:
            return

        # 查找武器
        if weapon_index not in _WEAPON_REGISTRY:
            print(f"[SubGraph] 未知武器编号: {weapon_index}，可选: {list(_WEAPON_REGISTRY.keys())}")
            return

        asset_path, weapon_name = _WEAPON_REGISTRY[weapon_index]

        # 加载蓝图 → 获取生成类
        weapon_bp = ue.LoadObject(ue.Blueprint, asset_path)
        if not weapon_bp:
            print(f"[SubGraph] 错误: 找不到蓝图 {asset_path}")
            return
        weapon_bp_class = weapon_bp.GeneratedClass
        if not weapon_bp_class:
            print(f"[SubGraph] 错误: 无法获取 {asset_path} 的生成类")
            return

        # 链接
        ctrl = get_or_create_controller(mesh)
        success = ctrl.link_weapon_subgraph(weapon_bp_class)
        if success:
            print(f"[SubGraph] 已装备武器 {weapon_index}: {weapon_name}")
            ctrl.print_status()
    except Exception as e:
        import traceback
        traceback.print_exc()
        print(f"[SubGraph] 切换武器异常: {e}")


def sub_graph_unlink_test():
    """取消链接测试命令"""
    try:
        world = ue.GetGameWorld()
        if not world:
            print("[SubGraph] 世界不存在")
            return

        pc = ue.GameplayStatics.GetPlayerController(world, 0)
        pawn = pc.Pawn if pc else None
        if not pawn:
            print("[SubGraph] 没有 Pawn")
            return

        mesh = pawn.Mesh if hasattr(pawn, 'Mesh') else None
        if not mesh:
            print("[SubGraph] 角色无 SkeletalMeshComponent")
            return

        ctrl = get_or_create_controller(mesh)
        ctrl.unlink_weapon_subgraph()
        ctrl.print_status()
    except Exception as e:
        import traceback
        traceback.print_exc()
        print(f"[SubGraph] 测试异常: {e}")


def sub_graph_status_test():
    """查看子图状态"""
    try:
        world = ue.GetGameWorld()
        if not world:
            print("[SubGraph] 世界不存在")
            return

        pc = ue.GameplayStatics.GetPlayerController(world, 0)
        pawn = pc.Pawn if pc else None
        mesh = pawn.Mesh if pawn and hasattr(pawn, 'Mesh') else None
        if not mesh:
            print("[SubGraph] 角色无 SkeletalMeshComponent")
            return

        ctrl = get_or_create_controller(mesh)
        ctrl.print_status()
    except Exception as e:
        import traceback
        traceback.print_exc()
        print(f"[SubGraph] 状态查询异常: {e}")
