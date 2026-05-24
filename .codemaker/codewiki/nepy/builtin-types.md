# NePy 内置类型参考

## 容器类型

### ArrayWrapper[VT]
UE TArray 的 Python 封装，行为类似 Python list。

```python
arr: ue.ArrayWrapper[ue.Actor] = some_array_property
arr.Append(item)      # 等价于 list.append
arr.Extend(iterable)  # 等价于 list.extend
arr.Insert(idx, item) # 等价于 list.insert
arr.Remove(item)      # 等价于 list.remove
arr.Pop(idx=-1)       # 等价于 list.pop
arr.Clear()           # 等价于 list.clear()
arr.Copy()            # 浅拷贝为 Python list
arr.Count(item)       # 计数
arr.Index(item)       # 查找索引
arr.Sort()            # 排序
arr.Reverse()         # 反转
arr.IsValid()         # 检测底层是否已销毁
len(arr)              # 获取长度
arr[i]                # 索引访问
arr[i] = val          # 索引赋值
```

### MapWrapper[KT, VT]
UE TMap 的 Python 封装，行为类似 Python dict。

```python
m: ue.MapWrapper[str, ue.Actor] = some_map_property
m.Get(key, default)   # 获取值
m.SetDefault(key, val)# 设置默认值
m.Pop(key, default)   # 弹出
m.PopItem()           # 弹出任意项
m.Update(dict)        # 批量更新
m.Keys()              # 返回所有 key 的 list
m.Values()            # 返回所有 value 的 list
m.Items()             # 返回所有 (key, value) 的 list
m.Clear()             # 清空
m.Copy()              # 浅拷贝为 Python dict
m[key] = val          # 设置
val = m[key]          # 获取
```

### SetWrapper[KT]
UE TSet 的 Python 封装。

```python
s: ue.SetWrapper[str] = some_set_property
s.Add(item)           # 添加
s.Remove(item)        # 移除（不存在则报错）
s.Discard(item)       # 移除（不存在则忽略）
s.Pop()               # 弹出任意元素
s.Clear()             # 清空
s.Copy()              # 浅拷贝为 Python set
```

## 委托 (Delegate)

### DynamicDelegateWrapper
动态单播委托 —— 绑定单个回调。

```python
delegate: ue.DynamicDelegateWrapper = some_component.on_actor_begin_overlap
delegate.Bind(callback)        # 绑定 Python 函数
delegate.Unbind()              # 解绑
delegate.Execute(*args)        # 强制触发（未绑定时报错）
delegate.ExecuteIfBound(*args) # 安全触发
delegate.IsBound()             # 是否已绑定
```

### DynamicMulticastDelegateWrapper
动态多播委托 —— 可绑定多个回调。

```python
delegate: ue.DynamicMulticastDelegateWrapper = some_component.on_component_hit
delegate.Add(callback)         # 添加回调（Python 函数）
delegate.AddUnique(callback)   # 去重添加
delegate.Remove(callback)      # 移除回调
delegate.Clear()               # 清除所有回调
delegate.Broadcast(*args)      # 广播触发
delegate.Contains(callback)    # 是否包含某回调
delegate.GetPythonCallbacks()  # 获取所有 Python 回调

# 动态绑定 UFUNCTION 方法（推荐）
delegate.AddDynamic(obj.Method)              # 绑定对象方法
delegate.AddDynamicUnique(obj.Method)        # 去重绑定
delegate.RemoveDynamic(obj.Method)           # 移除对象方法

# 等效写法（冗余设计）
delegate.AddDynamic(obj, Class.Method)
delegate.AddDynamic(obj, obj.Method)
```

> ⚠️ 数组容器作为委托参数时，回调必须返回该数组：`def callback(arr): ... return arr`

## 智能指针类型

### TSoftObjectPtr[TObject]
软对象指针，支持延迟加载。

```python
ptr: ue.TSoftObjectPtr[ue.Texture2D]
ptr.IsValid()          # 是否指向有效对象
ptr.IsNull()           # 是否为空
ptr.IsPending()        # 是否待加载
ptr.IsStale()          # 是否失效
ptr.Get()              # 获取对象（不强制加载）
ptr.LoadSynchronous()  # 同步加载并返回
ptr.GetAssetName()     # 获取资产名
ptr.GetLongPackageName() # 获取完整包路径
```

### TSoftClassPtr[TObject]
软类指针，用法同 TSoftObjectPtr，Get() 返回 TSubclassOf 类型。

### TWeakObjectPtr[TObject]
弱对象指针。

```python
ptr: ue.TWeakObjectPtr[ue.Actor]
ptr.IsValid()  # 是否有效
ptr.IsStale()  # 是否已失效
ptr.Get()      # 获取对象或 None
```

## 其他类型

### TSubclassOf[TObject]
类引用类型，用于 UProperty/UFuntion 参数。

```python
subclass: ue.TSubclassOf[ue.Actor]
subclass.GetDefaultObject()  # 获取 CDO
```

### Name / Text
- `ue.Name` — str 的类型别名，表示 UE 的 FName
- `ue.Text` — str 的类型别名，表示 UE 的 FText

### FieldPath
属性路径引用。

```python
path = ue.FieldPath()
path.IsValid()  # 属性是否存在
```

### EnumBase
所有 UE 枚举的基类。

```python
my_enum: ue.MyEnum
my_enum.GetDisplayName()  # 获取显示名称
my_enum.GetName()         # 获取枚举名
my_enum.GetValue()        # 获取数值
MyEnum.Enum()             # 获取枚举 UE 对象
```
