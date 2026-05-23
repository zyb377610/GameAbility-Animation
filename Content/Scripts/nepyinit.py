# -*- encoding: utf-8 -*-
import ue
import traceback


def on_init():
    ue.log_warning('[GameAbilityAnim] Nepy initialized.')

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


def on_shutdown():
    ue.log_warning('[GameAbilityAnim] Nepy shutdown.')


def on_debug_input(cmd_str):
    import gmcmds
    return gmcmds.handle_debug_input(cmd_str)


def on_tick(dt: float):
    """
    每帧全局回调。
    脚本注册 GameMode / Tick 后由此驱动。
    """
    pass
