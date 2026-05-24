# -*- encoding: utf-8 -*-
"""
Step 1.4: GameplayTag 驱动动画

TagToAnimListener 监听 ASC 的 GameplayTag 变化（State.Dead / State.Stunned / State.Hit），
自动将状态写入动画蓝图的 bool 变量，驱动 AnimBP 状态机切换动画。

用法:
    from gas.tag_to_anim import bind_tag_to_anim, TagToAnimListener
    listener = bind_tag_to_anim(asc, mesh_comp)
    # ...
    listener.unbind()

测试:
    from gas.tag_to_anim import test_apply_tag
    test_apply_tag(actor, "State.Hit")   # 模拟添加 Tag
    test_apply_tag(actor, "State.Hit")   # 相同 Tag 不会重复触发
    test_remove_tag(actor, "State.Hit")  # 移除恢复
"""
import ue


# ==================== 1. Tag → AnimBP 变量映射 ====================

# 统一配置：GameplayTag → AnimBP bool 变量名
TAG_VAR_MAP = {
    "State.Dead": "bIsDead",
    "State.Stunned": "bIsStunned",
    "State.Hit": "bIsHit",
}


# ==================== 2. TagToAnimListener ====================

@ue.uclass()
class TagToAnimListener(ue.Object):
    """
    监听 ASC 上 GameplayTag 的添加/移除事件，
    同步更新 SkeletalMeshComponent 对应 AnimInstance 的 bool 变量。

    Attributes:
        _handle: AnyOfTags 绑定的 handle，用于解绑
        _callback: 保留 Python 回调引用防止 GC
        _anim_instance: 缓存的 AnimInstance
        _tag_var_map: tag 名 → AnimBP 变量名
        _asc: 绑定的 ASC 引用
    """

    def __init_default__(self):
        self._handle = None
        self._callback = None
        self._anim_instance = None
        self._asc = None

    @property
    def _tag_var_map(self):
        return TAG_VAR_MAP

    def bind(self, asc, mesh_comp):
        """
        绑定 ASC Tag 变化到 AnimBP 变量。

        Args:
            asc: AbilitySystemComponent 实例
            mesh_comp: SkeletalMeshComponent（如 Character.Mesh）
        """
        self._asc = asc
        tag_var_map = TAG_VAR_MAP

        if mesh_comp:
            try:
                self._anim_instance = mesh_comp.GetAnimInstance()
                if self._anim_instance:
                    print(f"[TagToAnimListener] AnimInstance 已获取: "
                          f"{self._anim_instance}")
                else:
                    print("[TagToAnimListener] 警告: GetAnimInstance() 返回 None, "
                          "确保已指定 AnimBP")
            except Exception as e:
                print(f"[TagToAnimListener] 获取 AnimInstance 失败: {e}")
                self._anim_instance = None

        # 构造 tag 列表
        tags = []
        for tag_name in tag_var_map:
            tag = ue.GameplayTag()
            tag.TagName = tag_name
            tags.append(tag)

        # 创建回调闭包并保留引用防止 GC
        anim_inst_ref = self._anim_instance
        tag_var_map_ref = tag_var_map

        def on_tag_changed(tag: ue.GameplayTag, new_count: int):
            self._on_tag_changed(tag, new_count, anim_inst_ref, tag_var_map_ref)

        self._callback = on_tag_changed

        # 绑定 AnyOfTags（一次绑定监听所有 tag）
        self._handle = (
            ue.AbilitySystemBlueprintLibrary
            .BindEventWrapperToAnyOfGameplayTagsChanged(
                asc,
                tags,
                on_tag_changed,
                True,  # bExecuteImmediatelyIfTagApplied
                ue.EGameplayTagEventType.NewOrRemoved,
            ))
        print(f"[TagToAnimListener] 已绑定 {len(tags)} 个 Tag → AnimBP: "
              f"handle={self._handle}")

        return self

    def _on_tag_changed(self, tag, new_count, anim_inst, tag_var_map):
        """Tag 变化内部回调"""
        tag_name = str(tag.TagName)
        var_name = tag_var_map.get(tag_name)
        if not var_name:
            return

        # 当前使用的 TagListeningPolicy = NewOrRemoved
        # new_count > 0 → tag 新增（set True）
        # new_count == 0 → tag 完全移除（set False）
        is_active = new_count > 0
        print(f"[TagToAnimListener] Tag '{tag_name}' -> {var_name} = {is_active} "
              f"(count={new_count})")

        if anim_inst:
            try:
                setattr(anim_inst, var_name, is_active)
                # 验证写入
                actual = getattr(anim_inst, var_name, None)
                print(f"[TagToAnimListener] 写入后 {var_name} = {actual}")
            except Exception as e:
                # 回退：尝试 set_editor_property
                try:
                    anim_inst.set_editor_property(var_name, is_active)
                    print(f"[TagToAnimListener] set_editor_property 写入成功: "
                          f"{var_name} = {is_active}")
                except Exception as e2:
                    print(f"[TagToAnimListener] 写入 AnimBP 变量失败: "
                          f"setattr={e}, set_editor_property={e2}")
        else:
            print(f"[TagToAnimListener] 跳过: AnimInstance 为 None")

    def unbind(self):
        """解绑所有 Tag 监听"""
        if self._handle:
            ue.AbilitySystemBlueprintLibrary \
                .UnbindAllGameplayTagChangedEventWrappersForHandle(self._handle)
            print(f"[TagToAnimListener] 已解绑 handle={self._handle}")
            self._handle = None
        self._callback = None
        self._anim_instance = None
        self._asc = None


# ==================== 3. 便捷工厂函数 ====================

def bind_tag_to_anim(asc, mesh_comp):
    """
    便捷函数：创建 TagToAnimListener 并绑定。

    Args:
        asc: AbilitySystemComponent
        mesh_comp: SkeletalMeshComponent（Character.Mesh）

    Returns:
        TagToAnimListener 实例（调用 .unbind() 解绑）

    Usage:
        listener = bind_tag_to_anim(asc, self.Mesh)
    """
    listener = ue.NewObject(TagToAnimListener.Class())
    listener.bind(asc, mesh_comp)
    return listener


# ==================== 4. 测试/调试辅助 ====================

def test_apply_tag(actor, tag_name: str):
    """
    模拟测试：给 Actor 添加 Loose GameplayTag。
    触发 BindEventWrapper 回调 → 驱动 AnimBP 变量。

    Args:
        actor: Character/Pown 实例
        tag_name: 如 "State.Hit", "State.Dead", "State.Stunned"

    Usage:
        test_apply_tag(self, "State.Hit")
    """
    tag = ue.GameplayTag()
    tag.TagName = tag_name

    container = ue.GameplayTagContainer()
    container.GameplayTags.Append(tag)

    result = ue.AbilitySystemBlueprintLibrary.AddLooseGameplayTags(
        actor, container, False)
    print(f"[TagToAnimListener Test] AddLooseGameplayTags '{tag_name}'"
          f" -> {result}")


def test_remove_tag(actor, tag_name: str):
    """
    模拟测试：移除 Actor 的 Loose GameplayTag。
    触发回调 → 将 AnimBP 变量设回 False。

    Args:
        actor: Character/Pawn 实例
        tag_name: 如 "State.Hit"

    Usage:
        test_remove_tag(self, "State.Hit")
    """
    tag = ue.GameplayTag()
    tag.TagName = tag_name

    container = ue.GameplayTagContainer()
    container.GameplayTags.Append(tag)

    result = ue.AbilitySystemBlueprintLibrary.RemoveLooseGameplayTags(
        actor, container, False)
    print(f"[TagToAnimListener Test] RemoveLooseGameplayTags '{tag_name}'"
          f" -> {result}")
