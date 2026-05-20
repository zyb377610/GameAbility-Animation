# -*- encoding: utf-8 -*-

# 对于二元运算符函数重载，传入的参数与普通方法不一样，需要特殊处理
BINARY_OPERATORS = {
	'operator+',
	'operator-',
	'operator*',
	'operator/',
	'operator%',
	'operator|',
	'operator^',
	'operator&',
	'operator<<',
	'operator>>',
}

# 相等性判断的运算符要特殊处理不同类型的输入参数
EQUALITY_OPERATORS = {
	'operator==',
	'operator!=',
}

OPERATOR_OVERLOADS = [
	{
		'cpp_name': 'operator+',
		'py_name': '__add__',
		'arg_count': 1,
		'number_method_name': 'nb_add',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator-',
		'py_name': '__sub__',
		'arg_count': 1,
		'number_method_name': 'nb_subtract',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator*',
		'py_name': '__mul__',
		'arg_count': 1,
		'number_method_name': 'nb_multiply',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator/',
		'py_name': '__div__',
		'arg_count': 1,
		'number_method_name': 'nb_divide',
		'number_method_type': 'binaryfunc',
		'py3_name': '__truediv__',
		'py3_number_method_name': 'nb_true_divide',
	},

	{
		'cpp_name': 'operator+=',
		'py_name': '__iadd__',
		'arg_count': 1,
		'number_method_name': 'nb_inplace_add',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator-=',
		'py_name': '__isub__',
		'arg_count': 1,
		'number_method_name': 'nb_inplace_subtract',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator*=',
		'py_name': '__imul__',
		'arg_count': 1,
		'number_method_name': 'nb_inplace_multiply',
		'number_method_type': 'binaryfunc',
	},
	{
		'cpp_name': 'operator/=',
		'py_name': '__idiv__',
		'arg_count': 1,
		'number_method_name': 'nb_inplace_divide',
		'number_method_type': 'binaryfunc',
		'py3_name': '__itruediv__',
		'py3_number_method_name': 'nb_inplace_true_divide',
	},

	{
		'cpp_name': 'operator<',
		'py_name': '__lt__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_lt',
	},
	{
		'cpp_name': 'operator<=',
		'py_name': '__le__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_le',
	},
	{
		'cpp_name': 'operator==',
		'py_name': '__eq__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_eq',
	},
	{
		'cpp_name': 'operator!=',
		'py_name': '__ne__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_ne',
	},
	{
		'cpp_name': 'operator>',
		'py_name': '__gt__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_gt',
	},
	{
		'cpp_name': 'operator>=',
		'py_name': '__ge__',
		'arg_count': 1,
		'cmp_method_flag': 'has_cmp_ge',
	},
]

OPERATOR_BY_CPP_NAME = {i['cpp_name']: i for i in OPERATOR_OVERLOADS}
OPERATOR_BY_PY_NAME = {i['py_name']: i for i in OPERATOR_OVERLOADS}
