# -*- encoding: utf-8 -*-
import ue
import traceback


def _force_legacy_input_classes():
    """
    运行时强制将 DefaultInputComponentClass 从 EnhancedInput 切回旧版。

    原因：UE5 默认使用 EnhancedInput，但本项目的蓝图 InputAxis 节点
    需要旧版 InputComponent。DefaultInput.ini 的修改可能在编辑器运行时
    被覆盖，因此在 Python 侧做一次运行时强制设置作为兜底。

    Pawn::CreatePlayerInputComponent() 读取 UInputSettings 单例，
    每次 PIE 的 Pawn 被控制时触发，所以必须在下次 PIE 前设置好。
    """
    try:
        # 方案A: 直接用 UInputSettings 的 CDO
        input_settings = ue.InputSettings()
        if input_settings is None:
            # 方案B: 通过 GetMutableDefault 获取
            input_settings = ue.GetMutableDefault(ue.InputSettings.Class())
        if input_settings is None:
            print('[GameAbilityAnim] 无法获取 InputSettings 单例')
            return

        input_settings.DefaultPlayerInputClass = ue.PlayerInput.Class()
        input_settings.DefaultInputComponentClass = ue.InputComponent.Class()
        print('[GameAbilityAnim] DefaultInputComponentClass 已强制设为 InputComponent')
    except Exception:
        traceback.print_exc()


def on_init():
    print('[GameAbilityAnim] Nepy initialized.')

    # ---- 强制旧版输入类（运行时兜底，不依赖 DefaultInput.ini） ----
    _force_legacy_input_classes()

    # ---- 编辑器辅助工具 ----
    if ue.GIsEditor:
        try:
            import reload_monitor
            reload_monitor.start()
        except Exception:
            traceback.print_exc()
        try:
            import gmcmds
            gmcmds.debug()
        except Exception:
            traceback.print_exc()

    # ---- GAS / Animation 脚本模块 ----
    try:
        import gas.setup_character
    except Exception:
        traceback.print_exc()
    try:
        import gas.abilities.ga_fireball
    except Exception:
        traceback.print_exc()
    try:
        import gas.effects.ge_cooldown
    except Exception:
        traceback.print_exc()
    try:
        import gas.effects.ge_cost
    except Exception:
        traceback.print_exc()
    try:
        import gas.tag_to_anim
    except Exception:
        traceback.print_exc()

    # ---- 动画脚本模块 ----
    try:
        import animation.locomotion
    except Exception:
        traceback.print_exc()
    try:
        import animation.upper_body
    except Exception:
        traceback.print_exc()
    try:
        import animation.aim_ik
    except Exception:
        traceback.print_exc()
    try:
        import animation.distance_matching
    except Exception:
        traceback.print_exc()
    try:
        import animation.sub_graph
    except Exception:
        traceback.print_exc()


def on_shutdown():
    print('[GameAbilityAnim] Nepy shutdown.')


def on_debug_input(cmd_str):
    import gmcmds
    return gmcmds.handle_debug_input(cmd_str)


def on_tick(dt: float):
    """
    每帧全局回调。
    脚本注册 GameMode / Tick 后由此驱动。
    """
    # Step 2.1: 驱动所有角色的 LocomotionUpdater
    try:
        from animation.locomotion import tick_all_characters
        tick_all_characters(dt)
    except Exception:
        pass
