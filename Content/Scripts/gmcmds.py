# -*- encoding: utf-8 -*-
from ast import literal_eval

def handle_debug_input(cmd_str):
	if not cmd_str.startswith('@'):
		return False

	cmd_tokens = cmd_str[1:].split()
	cmd_name = cmd_tokens[0].strip().lower()
	for func_name, func in globals().items():
		if not callable(func):
			continue
		if getattr(func, '__module__', None) != __name__:
			continue
		if func_name.lower() != cmd_name:
			continue
		args = []
		for token in cmd_tokens[1:]:
			try:
				token = literal_eval(token)
			except:
				pass
			args.append(token)
		func(*args)
		break
	else:
		print('cmd "%s" not found!' % cmd_name)
	return True

def hello1():
	print('hello, nepy!')

def hello2():
	print('hello, nepy! hello2!')

def helloByNum(num):
	print('hello, nepy! num =',num)

def helloByNum1(num):
	print('hello, nepy! num =',num+100)

def debug():
    import os, sys
    dll_path = os.path.dirname(__file__) + '/debuglib'
    if dll_path not in sys.path:
        sys.path.append(dll_path)

	# 开发时期需要调试再取消注释
    import debugpy
    port = 30020
    debugpy.listen(30020, in_process_debug_adapter=True)

def reload():
	import reloader
	reloader.reload()

def uegc():
	import ue
	ue.KismetSystemLibrary.CollectGarbage()

def pygc():
	import gc
	gc.collect()

ticker_handle = None

def tickertest():
    def _tick(dt):
        print('tick:', dt)

    import ue
    global ticker_handle
    if ticker_handle is None:
        print('begin tick!')
        ticker_handle = ue.AddTicker(_tick)
    else:
        print('end tick!')
        ue.RemoveTicker(ticker_handle)
        ticker_handle = None

def timertest():
    import ue
    import time

    begin = time.time()

    def _on_timer():
        print('timer call after %.2fs.' % (time.time() - begin))

    timer_mgr = ue.GetEditorTimerManager()
    timer_mgr.SetTimer(_on_timer, 1, False,)
    print('timer begin.')


# ==================== Step 1.3: 火球技能测试命令（含冷却与消耗） ====================

def fireball():
    """测试火球技能：赋予 GA_Fireball → CommitAbility（检查冷却/消耗） → 发射弹道"""
    import ue
    from gas.setup_character import init_gas_for_actor, AttrSet_Base
    from gas.abilities.ga_fireball import GA_Fireball

    w = ue.GetGameWorld()
    if not w:
        print("[fireball] 错误: 当前没有 World，请先 PIE")
        return

    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[fireball] 错误: 获取不到 Pawn")
        return

    pawn = ctrl.Pawn
    asc = init_gas_for_actor(pawn)
    if not asc:
        return

    # 显示当前状态
    attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
    if attr_set:
        print(f"[fireball] 当前 Mana={attr_set.Mana}/{attr_set.MaxMana}")

    ga_class = GA_Fireball.Class()
    if not ga_class:
        print("[fireball] 错误: GA_Fireball UClass 未找到")
        return

    asc.GiveAbility(ga_class, 1)

    # Step 1.3: 用 try_commit_and_fire 替代直接 do_fireball
    # CommitAbility() 内部检查冷却/消耗，成功则应用 Cost GE + Cooldown GE
    for spec in asc.ActivatableAbilities.Items:
        if spec.Ability and hasattr(spec.Ability, 'try_commit_and_fire'):
            spec.Ability.try_commit_and_fire(asc, pawn)
            break
    else:
        print("[fireball] 错误: 找不到 GA_Fireball 实例")


def fireball_status():
    """查看火球技能冷却和消耗状态"""
    import ue
    from gas.setup_character import init_gas_for_actor, AttrSet_Base

    w = ue.GetGameWorld()
    if not w:
        print("[fireball_status] 错误: 当前没有 World，请先 PIE")
        return

    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[fireball_status] 错误: 获取不到 Pawn")
        return

    pawn = ctrl.Pawn
    asc = init_gas_for_actor(pawn)
    if not asc:
        return

    attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
    if attr_set:
        print(f"[fireball_status] Mana={attr_set.Mana}/{attr_set.MaxMana}, "
              f"Health={attr_set.Health}/{attr_set.MaxHealth}, "
              f"AttackPower={attr_set.AttackPower}")

    # 检查冷却 Tag
    cooldown_tag = ue.GameplayTag()
    cooldown_tag.TagName = "Cooldown.Fireball"
    tag_count = asc.GetGameplayTagCount(cooldown_tag)
    if tag_count > 0:
        print(f"[fireball_status] 冷却中! Tag 'Cooldown.Fireball' 计数={tag_count}")
    else:
        print("[fireball_status] 未在冷却，技能可用")


def gas_init():
    """初始化当前控制 Pawn 的 GAS 骨架"""
    import ue
    from gas.setup_character import init_gas_for_actor

    w = ue.GetGameWorld()
    if not w:
        print("[gas_init] 错误: 当前没有 World，请先 PIE")
        return

    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[gas_init] 错误: 获取不到 Pawn")
        return

    init_gas_for_actor(ctrl.Pawn)
    # Step 1.4: 绑定 Tag → AnimBP
    try:
        from gas.tag_to_anim import bind_tag_to_anim, test_apply_tag
        asc = ctrl.Pawn.GetComponentByClass(ue.AbilitySystemComponent.Class())
        if asc and hasattr(ctrl.Pawn, 'Mesh'):
            ctrl.Pawn._tag_anim_listener = bind_tag_to_anim(asc, ctrl.Pawn.Mesh)
            print("[gas_init] TagToAnimListener 已绑定")
            # 2 秒后自动测试
            def _auto_test():
                test_apply_tag(ctrl.Pawn, "State.Hit")
            ue.KismetSystemLibrary.K2_SetTimerDelegate(_auto_test, 2.0, False)
    except Exception:
        import traceback
        traceback.print_exc()


# ==================== Step 2.1: BlendSpace 移动测试 ====================

def locomotion():
    """
    对当前控制角色启动/停止 LocomotionUpdater + AimIKController。
    第一次调用注册，第二次调用移除（toggle）。
    """
    import ue
    from animation.locomotion import register_locomotion_for_character, \
        unregister_locomotion_for_character, _registry as _locomotion_registry
    from animation.aim_ik import register_aim_ik, unregister_aim_ik, \
        _registry as _aim_ik_registry

    w = ue.GetGameWorld()
    if not w:
        print("[locomotion] 错误: 当前没有 World，请先 PIE")
        return

    ctrl = ue.GameplayStatics.GetPlayerController(w, 0)
    if not ctrl or not ctrl.Pawn:
        print("[locomotion] 错误: 获取不到 Pawn")
        return

    pawn = ctrl.Pawn
    if id(pawn) in _locomotion_registry:
        unregister_locomotion_for_character(pawn)
        unregister_aim_ik(pawn)
        print("[locomotion] LocomotionUpdater + AimIKController 已停止")
    else:
        register_locomotion_for_character(pawn)
        register_aim_ik(pawn)
        print("[locomotion] LocomotionUpdater + AimIKController 已启动，"
              "移动 + 瞄准观察 BlendSpace + AimOffset 效果")