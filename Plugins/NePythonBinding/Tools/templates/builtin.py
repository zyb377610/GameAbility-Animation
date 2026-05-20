import ${root_module_name} as ue

def bp_func(arg_types=(), ret_types=(), arg_display=(), ret_display=()):
	def _bp_func(func):
		func.bp_arg_types = arg_types
		func.bp_ret_types = ret_types
		func.bp_arg_display = arg_display
		func.bp_ret_display = ret_display
		return func
	return _bp_func

ue.bp_func = bp_func
