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