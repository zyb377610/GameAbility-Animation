# -*- encoding: utf-8 -*-
from __future__ import annotations
import typing
import enum
from . import Object, Class, Enum
from . import ActorComponent, SceneComponent


TObject = typing.TypeVar("TObject", bound=Object, covariant=True)

Name: typing.TypeAlias = str
""" 指示接口参数或属性的 UE 原生类型为 FName，可用于在 subclassing 中定义 FName 类型参数，但不可真正作为 str 的别名使用 """

Text: typing.TypeAlias = str
""" 指示接口参数或属性的 UE 原生类型为 FText，可用于在 subclassing 中定义 FText 类型参数，但不可真正作为 str 的别名使用 """

SelfClass = typing.TypeVar("SelfClass")
""" 指代当前类的类型，可用于在 subclassing 中返回当前类类型 """


class TSubclassOf(Class, typing.Generic[TObject]):
	""" Fake type for uproperty and ufunction. """

	def GetDefaultObject(self) -> TObject:
		pass

	def __set__(self, obj, v: TSubclassOf[TObject] | type[TObject]):
		pass


class SoftPtr:

	def IsValid(self) -> bool:
		""" Test if this points to a live UObject

		@return true if Get() would return a valid non-null pointer
		"""

	def IsNull(self) -> bool:
		""" Test if this can never point to a live UObject

		@return true if this is explicitly pointing to no object
		"""

	def IsPending(self) -> bool:
		""" Test if this does not point to a live UObject, but may in the future

		@return true if this does not point to a real object, but could possibly
		"""

	def IsStale(self) -> bool:
		""" Slightly different than !IsValid(), returns true if this used to point to a UObject, but doesn't any more and has not been assigned or reset in the mean time.

		@return true if this used to point at a real object but no longer does.
		"""

	def Get(self) -> Object | Class:
		""" Dereference the pointer, which may cause it to become valid again. Will not try to load pending outside of game thread

		@return nullptr if this object is gone or the pointer was null, otherwise a valid UObject pointer
		"""

	def GetAssetName(self) -> str:
		""" Returns assetname string, leaving off the /package/path. part """

	def GetLongPackageName(self) -> str:
		""" Returns /package/path string, leaving off the asset name """

	def LoadSynchronous(self) -> Object | Class:
		""" Synchronously load (if necessary) and return the asset object represented by this asset ptr object"""

	def __get__(self, instance, owner=None) -> typing.Self:
		pass

	def __set__(self, instance, value: SoftPtr | Object | None):
		pass


class TSoftObjectPtr(SoftPtr, typing.Generic[TObject]):
	""" Fake type for uproperty and ufunction. """

	def Get(self) -> TObject:
		""" Dereference the pointer, which may cause it to become valid again. Will not try to load pending outside of game thread

		@return nullptr if this object is gone or the pointer was null, otherwise a valid UObject pointer
		"""

	def LoadSynchronous(self) -> TObject:
		""" Synchronously load (if necessary) and return the asset object represented by this asset ptr object"""

	def __set__(self, instance, value: TSoftObjectPtr[TObject] | TObject | None):
		pass


class TSoftClassPtr(SoftPtr, typing.Generic[TObject]):
	""" Fake type for uproperty and ufunction. """

	def Get(self) -> TSubclassOf[TObject]:
		""" Dereference the pointer, which may cause it to become valid again. Will not try to load pending outside of game thread

		@return nullptr if this object is gone or the pointer was null, otherwise a valid UObject pointer
		"""

	def LoadSynchronous(self) -> TSubclassOf[TObject]:
		""" Synchronously load (if necessary) and return the asset object represented by this asset ptr object"""

	def __set__(self, instance, value: TSoftClassPtr[TObject] | TSubclassOf[TObject] | type[TObject] | None):
		pass

class TWeakObjectPtr(typing.Generic[TObject]):
	""" Fake type for uproperty and ufunction. """

	def IsValid(self) -> bool:
		""" Test if this points to a live UObject

		@return true if Get() would return a valid non-null pointer
		"""
	
	def IsStale(self) -> bool:
		""" Slightly different than !IsValid(), returns true if this used to point to a UObject, but doesn't any more and has not been assigned or reset in the mean time.

		@return true if this used to point at a real object but no longer does.
		"""

	def Get(self) -> TObject | None:
		""" Dereference the pointer

		@return nullptr if this object is gone or the pointer was null, otherwise a valid UObject pointer
		"""

	def __get__(self, instance, owner=None) -> typing.Self:
		pass

	def __set__(self, instance, value: TWeakObjectPtr[TObject] | TObject | None):
		pass

class EnumBase(enum.IntEnum):

	def GetDisplayName(self) -> str:
		""" 获取枚举显示名称 """

	def GetName(self) -> str:
		""" 获取枚举名称 """

	def GetValue(self) -> int:
		""" 获取枚举值 """

	@classmethod
	def Enum(cls) -> Enum:
		""" 枚举类 UE 对象 """


class ArrayWrapper[VT]:
	""" Unreal 数组容器的包装类 """
	@overload
	def __init__(self, target_type: VT) -> None:
		pass

	@overload
	def __init__(self, target_type: VT, iterable: typing.Iterable[VT] = ...) -> None:
		pass

	def IsValid(self) -> bool:
		""" 检测底层是否已被销毁 """
	is_valid = IsValid

	def Copy(self) -> list[VT]:
		""" Make a shallow copy of this list """
	copy = Copy

	def Append(self, Item: VT) -> None:
		""" Append the given value to the end this container (equivalent to 'x.append(v)' in Python) """
	append = Append

	def Count(self, Item: VT) -> int:
		""" Count the number of times that the given value appears in this container (equivalent to 'x.count(v)' in Python) """
	count = Count

	def Extend(self, Item: list[VT]) -> None:
		""" Extend this Unreal array by appending elements from the given iterable (equivalent to TArray::Append in C++) """
	extend = Extend

	def Index(self, Item: VT, Start: int = ..., Stop: int = ...) -> int:
		""" Get the index of the first the given value appears in this container (equivalent to 'x.index(v)' in Python) """
	index = Index

	def Insert(self, Index: int, Item: VT) -> None:
		""" Insert the given value into this container at the given index (equivalent to 'x.insert(i, v)' in Python) """
	insert = Insert
	
	def Pop(self, Index: int = ...) -> VT:
		""" Pop and return the value at the given index (or the end) of this container (equivalent to 'x.pop()' in Python) """
	pop = Pop
	
	def Remove(self, value: VT) -> None:
		""" Remove the given value from this container (equivalent to 'x.remove(v)' in Python) """
	remove = Remove

	def Reverse(self) -> None:
		""" Reverse this container (equivalent to 'x.reverse()' in Python) """
	reverse = Reverse

	def Sort(self, Cmp: typing.Callable = ..., Key: typing.Callable = ..., bReverse: bool = ...) -> None:
		""" Sort this container (equivalent to 'x.sort()' in Python) """
	sort = Sort

	def Clear(self) -> None:
		""" Clear all values in this container (equivalent to 'del x[:]' in Python """
	clear = Clear

	def __len__(self) -> int:
		pass

	def __getitem__(self, Index: int) -> VT:
		pass

	def __setitem__(self, Index: int, Item: VT) -> None:
		pass

	def __delitem__(self, Index: int) -> None:
		pass

	def __contains__(self, Item: object) -> bool:
		pass

	def __iter__(self) -> typing.Iterator[VT]:
		pass

	def __add__(self, x: list[VT]) -> list[VT]:
		pass

	def __iadd__(self, x: list[VT]) -> None:
		pass

	def __mul__(self, n: int) -> list[VT]:
		pass

	def _imul__(self, n: int) -> None:
		pass

	def __get__(self, instance, owner=None) -> typing.Self:
		pass

	def __set__(self, instance, value: list[VT] | ArrayWrapper[VT]):
		pass


class FixedArrayWrapper[VT]:

	def IsValid(self) -> bool:
		pass
	is_valid = IsValid

	def Copy(self) -> list[VT]:
		""" Make a shallow copy of this list """
	copy = Copy

	def Count(self, Item: VT) -> int:
		""" Count the number of times that the given value appears in this container (equivalent to 'x.count(v)' in Python) """
	count = Count

	def Index(self, Item: VT, Start: int = ..., Stop: int = ...) -> int:
		""" Get the index of the first the given value appears in this container (equivalent to 'x.index(v)' in Python) """
	index = Index

	def __len__(self) -> int:
		pass

	def __getitem__(self, Index: int) -> VT:
		pass

	def __setitem__(self, Index: int, Item: VT) -> None:
		pass

	def __contains__(self, Item: object) -> bool:
		pass

	def __iter__(self) -> typing.Iterator[VT]:
		pass

	def __add__(self, x: list[VT]) -> list[VT]:
		pass


class MapWrapper[KT, VT]:

	def IsValid(self) -> bool:
		pass
	is_valid = IsValid

	def Copy(self) -> dict[KT, VT]:
		""" Make a shallow copy of this map """
	copy = Copy

	def Clear(self) -> None:
		""" Remove all values from this container (equivalent to 'x.clear()' in Python) """
	clear = Clear

	def Get[T](self, Key: KT, Default: VT | T = ...) -> VT | T:
		""" Get the item with key K (equivalent to 'x.get(K)' in Python) """
	get = Get

	def Items(self) -> list[tuple[KT, VT]]:
		""" Get a Python list containing the items from this map as key->value pairs """
	items = Items

	def Keys(self) -> list[KT]:
		""" Get a Python list containing the keys from this map """
	keys = Keys

	def Values(self) -> list[VT]:
		""" Get a Python list containing the values from this map """
	values = Values

	def Pop[T](self, Key: KT, Default: VT | T = ...) -> VT | T:
		""" Remove and return the value for key K if present, otherwise return the default, or raise KeyError if no default is given (equivalent to 'x.popitem()' in Python, returns new reference) """
	pop = Pop

	def PopItem(self) -> tuple[KT, VT]:
		""" Remove and return an arbitrary pair from this map (equivalent to 'x.popitem()' in Python, returns new reference) """
	pop_item = PopItem

	def SetDefault(self, Key: KT, Default: VT = ...) -> VT:
		""" Set the item with key K if K isn't in the map and return the value of K (equivalent to 'x.setdefault(K, v)' in Python) """
	set_default = SetDefault

	def Update(self, Items: dict[KT, VT]) -> None:
		""" Update this map from another (equivalent to 'x.update(o)' in Python) """
	update = Update

	def __len__(self) -> int:
		pass

	def __contains__(self, Key: object) -> bool:
		pass

	def __getitem__(self, Key: KT) -> VT:
		pass

	def __setitem__(self, Key: KT, Value: VT) -> None:
		pass

	def __delitem__(self, Key: KT) -> None:
		pass

	def __iter__(self) -> typing.Iterator[KT]:
		pass

	def __get__(self, obj, obj_type=None) -> MapWrapper[KT, VT]:
		pass

	def __set__(self, obj, v: dict[KT, VT] | MapWrapper[KT, VT]):
		pass


class SetWrapper[KT]:
	def IsValid(self) -> bool:
		pass

	def Copy(self) -> set[KT]:
		""" Make a shallow copy of this set """
	copy = Copy

	def Add(self, Value: KT) -> None:
		""" Add the given value to this container (equivalent to 'x.add(v)' in Python) """
	add = Add

	def Discard(self, Value: KT) -> None:
		""" Remove the given value from this container, doing nothing if it's not present (equivalent to 'x.discard(v)' in Python) """
	discard = Discard

	def Remove(self, Value: KT) -> None:
		""" Remove the given value from this container (equivalent to 'x.remove(v)' in Python) """
	remove = Remove

	def Pop(self) -> KT:
		""" Remove and return an arbitrary value from this container (equivalent to 'x.pop()' in Python, returns new reference) """
	pop = Pop

	def Clear(self) -> None:
		""" Remove all values from this container (equivalent to 'x.clear()' in Python) """
	clear = Clear

	def __len__(self) -> int:
		pass

	def __contains__(self, Value: object) -> bool:
		pass

	def __iter__(self) -> typing.Iterator[KT]:
		pass

	def __get__(self, obj, obj_type=None) -> SetWrapper[KT]:
		pass

	def __set__(self, obj, v: set[KT] | SetWrapper[KT]):
		pass


class DynamicDelegateWrapper[**P, R]:

	def IsValid(self) -> bool:
		""" 检测底层UObject是否已被销毁 """

	def IsBound(self) -> bool:
		""" 此委托是否已绑定过回调函数（Python或C++） """

	def IsBoundTo(self, Callback: typing.Callable[P, R]) -> bool:
		""" 此委托是否已绑定至某Python回调函数 """

	def Bind(self, Callback: typing.Callable[P, R]) -> None:
		""" 向此委托绑定一个Python回调函数 """

	def Unbind(self) -> None:
		""" 从此委托移除一个Python回调函数 """

	def Clear(self) -> None:
		""" 从此委托移除一个Python回调函数 """

	def GetPythonCallback(self) -> typing.Callable[P, R] | None:
		""" 返回此委托上绑定的Python回调函数 """

	def Execute(self, *Args: P.args, **Kwargs: P.kwargs) -> R:
		""" 触发委托，如果未绑定回调函数则报错 """

	def ExecuteIfBound(self, *Args: P.args, **Kwargs: P.kwargs) -> R:
		""" 如果委托绑定了回调函数，则触发委托 """


class DynamicDelegateArg[**P, R]:

	def IsBound(self) -> bool:
		""" 此委托是否已绑定过回调函数（Python或C++） """

	def Execute(self, *Args: P.args, **Kwargs: P.kwargs) -> R:
		""" 触发委托，如果未绑定回调函数则报错 """

	def ExecuteIfBound(self, *Args: P.args, **Kwargs: P.kwargs) -> R:
		""" 如果委托绑定了回调函数，则触发委托 """


class DynamicMulticastDelegateWrapper[**P, R: None]:
	def IsValid(self) -> bool:
		""" 检测底层UObject是否已被销毁 """

	def IsBound(self) -> bool:
		""" 此委托是否已绑定过回调函数（Python或C++） """

	def Contains(self, Callback: typing.Callable[P, None]) -> bool:
		""" 此委托是否已绑定过某Python回调函数 """

	def Add(self, Callback: typing.Callable[P, None]) -> None:
		""" 向此委托绑定一个Python回调函数 """

	def AddUnique(self, Callback: typing.Callable[P, None]) -> None:
		""" 向此委托绑定一个Python回调函数，重复绑定无效 """

	def Remove(self, Callback: typing.Callable[P, None]) -> None:
		""" 从此委托移除一个Python回调函数 """

	def Clear(self) -> None:
		""" 清除委托上绑定的所有Python回调函数 """

	def GetPythonCallbacks(self) -> list[typing.Callable[P, None]]:
		""" 返回此委托上绑定的Python回调函数列表 """

	def Broadcast(self, *Args: P.args, **Kwargs: P.kwargs) -> None:
		""" 触发委托 """


class FieldPath(object):
	""" 属性路径 """

	def IsValid(self) -> bool:
		""" 检测属性路径引用的属性是否存在 """


def uclass[TC: type](
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_ClassSpecifiers]
) -> typing.Callable[[TC], TC]:
	pass


def ustruct[TC: type](
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_TypeSpecifiers]
) -> typing.Callable[[TC], TC]:
	pass


def uenum[TC](
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_TypeSpecifiers]
) -> typing.Callable[[TC], TC]:
	pass


def uvalue(
	value: int,
	*,
	meta: dict[str, typing.Any] = ...,
) -> int:
	pass


@typing.overload
def uproperty[T: EnumBase](
	default: T,
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> T:
	""" 定义一个枚举类型的 UProperty，并设置默认值 """

@typing.overload
def uproperty[T: (int, bool, float, str)](
	default: T,
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> T:
	""" 定义一个 UProperty，并设置默认值 """


@typing.overload
def uproperty[T](
	type: type[T],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> T:
	""" 定义一个 UProperty """


@typing.overload
def uproperty[VT](
	type: list[type[VT]],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> ArrayWrapper[VT]:
	""" 定义一个 TArray 容器类型的 UProperty """


@typing.overload
def uproperty[KT, VT](
	type: dict[type[KT], type[VT]],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> MapWrapper[KT, VT]:
	""" 定义一个 TMap 容器类型的 UProperty """


@typing.overload
def uproperty[KT](
	type: set[type[KT]],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_PropertySpecifiers]
) -> SetWrapper[KT]:
	""" 定义一个 TSet 容器类型的 UProperty """


def uparam(type, *, meta: dict[str, typing.Any] = ..., **kwargs: typing.Unpack[_CommonSpecifiers]):
	""" 为 ufunction 参数添加修饰 """


@typing.overload
def ufunction[F: typing.Callable](
	*,
	override: bool = ...,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_FunctionSpecifiers]
) -> typing.Callable[[F], F]:
	""" 重写父类定义的 UFunction """


@typing.overload
def ufunction[F: typing.Callable](
	*,
	ret: tuple | typing.Any = ...,
	params: tuple | typing.Any = ...,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_FunctionSpecifiers]
) -> typing.Callable[[F], F]:
	""" 定义一个 UFunction """


@typing.overload
def ucomponent[T: SceneComponent](
	cls: type[T] | TSubclassOf[T],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_BasePropertySpecifiers]
) -> T:
	""" 为 Actor 定义一个 UProperty 并在构造 Actor 时创建同名的 SceneComponent 对象，并 Attach 到根节点。 """


@typing.overload
def ucomponent[T: SceneComponent](
	cls: type[T] | TSubclassOf[T],
	*,
	attach: str,
	attach_socket: str = ...,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_BasePropertySpecifiers]
) -> T:
	""" 为 Actor 定义一个 UProperty 并在构造 Actor 时创建同名的 SceneComponent 对象，并 Attach 到父节点。 """


@typing.overload
def ucomponent[T: SceneComponent](
	cls: type[T] | TSubclassOf[T],
	*,
	root: bool,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_BasePropertySpecifiers]
) -> T:
	""" 为 Actor 定义一个 UProperty 并在构造 Actor 时创建同名的 SceneComponent 对象，并作为 Actor 根节点。 """


@typing.overload
def ucomponent[T: ActorComponent](
	cls: type[T] | TSubclassOf[T],
	*,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_BasePropertySpecifiers]
) -> T:
	""" 为 Actor 定义一个 UProperty 并在构造 Actor 时创建同名的 ActorComponent 对象 """


@typing.overload
def ucomponent[T: ActorComponent](
	cls: type[T] | TSubclassOf[T],
	*,
	override: str,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_BasePropertySpecifiers]
) -> T:
	""" 为 Actor 定义一个新的 UProperty，指向父类对应名称的 ActorComponent 对象，并覆盖它的对象类型 """


type ParamTypeAndName[T] = tuple[type[T], str]
type DelegateParamDefineWithNoParam = tuple[()]
type DelegateParamDefineWithOneParam[T1] = tuple[ParamTypeAndName[T1]]
type DelegateParamDefineWithTwoParams[T1, T2] = tuple[ParamTypeAndName[T1], ParamTypeAndName[T2]]
type DelegateParamDefineWithThreeParams[T1, T2, T3] = tuple[ParamTypeAndName[T1], ParamTypeAndName[T2], ParamTypeAndName[T3]]
type DelegateParamDefineWithFourParams[T1, T2, T3, T4] = tuple[ParamTypeAndName[T1], ParamTypeAndName[T2], ParamTypeAndName[T3], ParamTypeAndName[T4]]
type DelegateParamDefineWithFiveParams[T1, T2, T3, T4, T5] = tuple[ParamTypeAndName[T1], ParamTypeAndName[T2], ParamTypeAndName[T3], ParamTypeAndName[T4], ParamTypeAndName[T5]]


@typing.overload
def udelegate(
	*,
	params: DelegateParamDefineWithNoParam = ...,
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[], None]:
	""" 定义一个 Delegate 类型的 UProperty """


@typing.overload
def udelegate[T1](
	*,
	params: DelegateParamDefineWithOneParam[T1],
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[T1], None]:
	""" 定义一个 Delegate 类型的 UProperty """


@typing.overload
def udelegate[T1, T2](
	*,
	params: DelegateParamDefineWithTwoParams[T1, T2],
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[T1, T2], None]:
	""" 定义一个 Delegate 类型的 UProperty """


@typing.overload
def udelegate[T1, T2, T3](
	*,
	params: DelegateParamDefineWithThreeParams[T1, T2, T3],
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[T1, T2, T3], None]:
	""" 定义一个 Delegate 类型的 UProperty """


@typing.overload
def udelegate[T1, T2, T3, T4](
	*,
	params: DelegateParamDefineWithFourParams[T1, T2, T3, T4],
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[T1, T2, T3, T4], None]:
	""" 定义一个 Delegate 类型的 UProperty """


@typing.overload
def udelegate[T1, T2, T3, T4, T5](
	*,
	params: DelegateParamDefineWithFiveParams[T1, T2, T3, T4, T5],
	meta: dict[str, typing.Any] = ...,
	**kwargs: typing.Unpack[_DelegatePropertySpecifiers]
) -> DynamicMulticastDelegateWrapper[[T1, T2, T3, T4, T5], None]:
	""" 定义一个 Delegate 类型的 UProperty """

def ref(
	target_type: type[T]
) -> type[T]:
	""" 定义参数类型为引用 """

__all__ = [
	'Name',
	'Text',
	
	'SelfClass',

	'TSubclassOf',
	'SoftPtr',
	'TSoftObjectPtr',
	'TSoftClassPtr',
	'TWeakObjectPtr',

	'EnumBase',
	'ArrayWrapper',
	'FixedArrayWrapper',
	'MapWrapper',
	'SetWrapper',
	'DynamicDelegateWrapper',
	'DynamicDelegateArg',
	'DynamicMulticastDelegateWrapper',
	'FieldPath',

	'uclass',
	'ustruct',
	'uenum',
	'uvalue',
	'uproperty',
	'uparam',
	'ucomponent',
	'udelegate',
	'ufunction',

	'ref',
]


class _CommonSpecifiers(typing.TypedDict, total=False):

	DisplayName: str
	""" The name to display for this class, property, or function instead of auto-generating it from the name. """


class _TypeSpecifiers(_CommonSpecifiers, total=False):

	BlueprintType: typing.Literal[True]
	""" Exposes this struct or enum as a type that can be used for variables in blueprints """

	NotBlueprintType: typing.Literal[True]
	""" Nepy script difference. Marks this struct or enum without 'BlueprintType' Specifier even if it added by default value. """


class _ClassSpecifiers(_CommonSpecifiers, total=False):

	# BlueprintType: typing.Literal[True]
	# """ Exposes this class as a type that can be used for variables in blueprints. This is inherited by subclasses unless overridden. """

	# NotBlueprintType: typing.Literal[True]
	# """ Prevents this class from being used for variables in blueprints. This is inherited by subclasses unless overridden. """

	# Blueprintable: typing.Literal[True]
	# """ Exposes this class as an acceptable base class for creating blueprints. The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses. """

	# NotBlueprintable: typing.Literal[True]
	# """ Specifies that this class is *NOT* an acceptable base class for creating blueprints. The default is NotBlueprintable, unless inherited otherwise. This is inherited by subclasses. """

	EditInlineNew: typing.Literal[True]
	""" These affect the behavior of the property editor.
	Class can be constructed from editinline New button. """

	NotEditInlineNew: typing.Literal[True]
	""" Class can't be constructed from editinline New button. """

	NotPlaceable: typing.Literal[True]
	""" This class cannot be placed in the editor (it cancels out an inherited placeable flag). """

	DefaultToInstanced: typing.Literal[True]
	""" All instances of this class are considered "instanced". Instanced classes (components) are duplicated upon construction. This flag is inherited by subclasses. """

	HideDropdown: typing.Literal[True]
	""" Class not shown in editor drop down for class selection. """

	Abstract: typing.Literal[True]
	""" Class is abstract and can't be instantiated directly. """

	Deprecated: typing.Literal[True]
	""" This class is deprecated and objects of this class won't be saved when serializing.  This flag is inherited by subclasses. """

	Transient: typing.Literal[True]
	""" This class can't be saved; null it out at save time.  This flag is inherited by subclasses. """

	Config: str
	""" Load object configuration at construction time.  These flags are inherited by subclasses.
	Class containing config properties. Usage config=ConfigName or config=inherit (inherits config name from base class). """

	PerObjectConfig: typing.Literal[True]
	""" Handle object configuration on a per-object basis, rather than per-class. """

	ShowCategories: str | tuple[str, ...] | list[str]
	""" Shows the specified categories in a property viewer. Usage: showCategories=CategoryName or showCategories=(category0, category1, ...) """

	HideCategories: str | tuple[str, ...] | list[str]
	""" Hides the specified categories in a property viewer. Usage: hideCategories=CategoryName or hideCategories=(category0, category1, ...) """

	ClassGroup: str
	""" This keyword is used to set the actor group that the class is show in, in the editor. """

	AutoExpandCategories: str | tuple[str, ...] | list[str]
	""" Specifies which categories should be automatically expanded in a property viewer. """

	AutoCollapseCategories: str | tuple[str, ...] | list[str]
	""" Specifies which categories should be automatically collapsed in a property viewer. """

	AdvancedClassDisplay: typing.Literal[True]
	""" All the properties of the class are hidden in the main display by default, and are only shown in the advanced details section. """

	CollapseCategories: typing.Literal[True]
	""" Display properties in the editor without using categories. """

	DontCollapseCategories: typing.Literal[True]
	""" Display properties in the editor using categories (default behaviour). """


class _FunctionSpecifiers(_CommonSpecifiers, total=False):

	Category: str
	""" Specifies the category of the function when displayed in blueprint editing tools.
	Usage: Category=CategoryName or Category="MajorCategory,SubCategory" """

	CallInEditor: typing.Literal[True]
	""" This function can be called in the editor on selected instances via a button in the details panel. """

	NotBlueprintCallable: typing.Literal[True]
	""" Nepy script only. Marks this function without any 'BlueprintCallable' specifier even if it added by default value. """

	BlueprintCallable: typing.Literal[True]
	""" This function can be called from blueprint code and should be exposed to the user of blueprint editing tools. """

	BlueprintEvent: typing.Literal[True]
	""" Nepy script only. Similar to BlueprintNativeEvent, this ufunction can be overridden by a blueprint, but also has a default implementation.
	The difference is that it does not require the default implementation function to have an '_Implementation' suffix. """

	BlueprintAuthorityOnly: typing.Literal[True]
	""" This function will not execute from blueprint code if running on something without network authority """

	BlueprintCosmetic: typing.Literal[True]
	""" This function is cosmetic and will not run on dedicated servers """

	BlueprintPure: typing.Literal[True]
	""" This function fulfills a contract of producing no side effects, and additionally implies BlueprintCallable. """

	BlueprintGetter: typing.Literal[True]
	""" This function is used as the get accessor for a blueprint exposed property. Implies BlueprintPure and BlueprintCallable. """

	BlueprintSetter: typing.Literal[True]
	""" This function is used as the set accessor for a blueprint exposed property. Implies BlueprintCallable. """

	Server: typing.Literal[True]
	""" This function is replicated, and executed on servers.  Provide a body named [FunctionName]_Implementation instead of [FunctionName];
	the autogenerated code will include a thunk that calls the implementation method when necessary. """

	Client: typing.Literal[True]
	""" This function is replicated, and executed on clients.  Provide a body named [FunctionName]_Implementation instead of [FunctionName];
	the autogenerated code will include a thunk that calls the implementation method when necessary. """

	NetMulticast: typing.Literal[True]
	""" This function is both executed locally on the server and replicated to all clients, regardless of the Actor's NetOwner """

	Reliable: typing.Literal[True]
	""" Replication of calls to this function should be done on a reliable channel.
	Only valid when used in conjunction with Client, Server, or NetMulticast """

	Exec: typing.Literal[True]
	""" This function is executable from the command line. """

	FieldNotify: typing.Literal[True]
	""" Generate a field entry for the NotifyFieldValueChanged interface. """


class _BasePropertySpecifiers(_CommonSpecifiers, total=False):

	Category: str
	""" Specifies the category of the property. Usage: Category=CategoryName. """

	Config: typing.Literal[True]
	""" Load object configuration at construction time.  These flags are inherited by subclasses.
	Class containing config properties. Usage config=ConfigName or config=inherit (inherits config name from base class). """

	NotEditable: typing.Literal[True]
	""" Nepy script only. Marks this property without any edit specifier. """

	EditAnywhere: typing.Literal[True]
	""" Indicates that this property can be edited by property windows in the editor """

	EditInstanceOnly: typing.Literal[True]
	""" Indicates that this property can be edited by property windows, but only on instances, not on archetypes """

	EditDefaultsOnly: typing.Literal[True]
	""" Indicates that this property can be edited by property windows, but only on archetypes """

	VisibleAnywhere: typing.Literal[True]
	""" Indicates that this property is visible in property windows, but cannot be edited at all """

	VisibleInstanceOnly: typing.Literal[True]
	""" Indicates that this property is only visible in property windows for instances, not for archetypes, and cannot be edited """

	VisibleDefaultsOnly: typing.Literal[True]
	""" Indicates that this property is only visible in property windows for archetypes, and cannot be edited """

	GlobalConfig: typing.Literal[True]
	""" Same as above but load config from base class, not subclass. """

	Transient: typing.Literal[True]
	""" This class can't be saved; null it out at save time.  This flag is inherited by subclasses. """

	DuplicateTransient: typing.Literal[True]
	""" Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.) """

	NonPIEDuplicateTransient: typing.Literal[True]
	""" Property should always be reset to the default value unless it's being duplicated for a PIE session """

	TextExportTransient: typing.Literal[True]
	""" Property shouldn't be exported to text format (e.g. copy/paste) """

	NoClear: typing.Literal[True]
	""" Hide clear button in the editor. """

	EditFixedSize: typing.Literal[True]
	""" Indicates that elements of an array can be modified, but its size cannot be changed. """

	Replicated: typing.Literal[True]
	""" Property is relevant to network replication. """

	ReplicatedUsing: str
	""" Property is relevant to network replication. Notify actors when a property is replicated (usage: ReplicatedUsing=FunctionName). """

	NotReplicated: typing.Literal[True]
	""" Skip replication (only for struct members and parameters in service request functions). """

	Interp: typing.Literal[True]
	""" Interpolatable property for use with cinematics. Always user-settable in the editor. """

	NonTransactional: typing.Literal[True]
	""" Property isn't transacted. """

	Instanced: typing.Literal[True]
	""" Property is a component reference. Implies EditInline and Export. """

	BlueprintHidden: typing.Literal[True]
	""" Nepy script only. Marks this property without any blueprint specifier. """

	BlueprintReadWrite: typing.Literal[True]
	""" This property can be read or written from a blueprint. """

	BlueprintReadOnly: typing.Literal[True]
	""" This property can be read by blueprints, but not modified. """

	BlueprintGetter: str
	""" This property has an accessor to return the value. Implies BlueprintReadWrite. (usage: BlueprintGetter=FunctionName). """

	BlueprintSetter: str
	""" This property has an accessor to set the value. Implies BlueprintReadWrite. (usage: BlueprintSetter=FunctionName). """

	AssetRegistrySearchable: typing.Literal[True]
	""" The AssetRegistrySearchable keyword indicates that this property and it's value will be automatically added
	to the asset registry for any asset class instances containing this as a member variable.  It is not legal
	to use on struct properties or parameters. """

	SimpleDisplay: typing.Literal[True]
	""" Properties appear visible by default in a details panel """

	AdvancedDisplay: typing.Literal[True]
	""" Properties are in the advanced dropdown in a details panel """

	SaveGame: typing.Literal[True]
	""" Property should be serialized for save games.
	This is only checked for game-specific archives with ArIsSaveGame set """

	SkipSerialization: typing.Literal[True]
	""" Property shouldn't be serialized, can still be exported to text """


class _PropertySpecifiers(_BasePropertySpecifiers, total=False):

	FieldNotify: bool | str | tuple[str, ...] | list[str]
	""" Generate a field entry for the NotifyFieldValueChanged interface. """


class _DelegatePropertySpecifiers(_BasePropertySpecifiers, total=False):

	BlueprintCallable: typing.Literal[True]
	""" MC Delegates only.  Property should be exposed for calling in blueprint code """

	BlueprintAuthorityOnly: typing.Literal[True]
	""" MC Delegates only. This delegate accepts (only in blueprint) only events with BlueprintAuthorityOnly. """

	BlueprintAssignable: typing.Literal[True]
	""" MC Delegates only.  Property should be exposed for assigning in blueprints. """
