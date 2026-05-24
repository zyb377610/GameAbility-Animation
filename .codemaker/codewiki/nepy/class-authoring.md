# NEPY 类编写完整指南

> 覆盖 Subclassing 全部语法：`@ue.uclass` / `@ue.ustruct` / `@ue.uenum` / `@ue.uproperty` / `@ue.ucomponent` / `@ue.ufunction` / `@ue.udelegate`

> ⚠️ **试验性功能**：Subclassing 机制复杂，稳定性偶有问题，不建议在正式项目中大规模使用。

---

## 一、`@ue.uclass()` — 定义 UE 类

让 Python 类能被 UE 反射系统识别。**必须用，不加则蓝图和 C++ 都无法识别。**

```python
@ue.uclass()
class MyActor(ue.Actor):
    """MyActor 是 AActor 的子类"""
    pass

@ue.uclass()
class MyObject(ue.Object):
    """MyObject 是 UObject 的子类"""
    pass
```

Python 中定义的 UE 类与 C++ 中定义的几乎无区别：可作为蓝图成员变量、蓝图函数参数/返回值、蓝图基类、Python 子类的基类。

### 类说明符 (Class Specifiers)

```python
@ue.uclass(Abstract=True)           # 抽象类，不能直接实例化
class AbstractActor(ue.Actor): pass

@ue.uclass(DisplayName='MyActor')   # 自定义编辑器显示名称
class ActorWithDisplayName(ue.Actor): pass

@ue.uclass(HideCategories=('Navigation', 'Collision'))  # 隐藏属性分类
class CleanActor(ue.Actor): pass
```

### 类元数据 (Meta)

```python
@ue.uclass(meta={'Tooltip': '在Python中定义的Actor类'})
class MyActor(ue.Actor):
    pass
```

> 元数据仅在编辑器中生效，打包后去除。

### 类型注册

**通过 `import` 模块即可注册。** 强烈建议在 `nepyinit.py` 中 import 所有 Subclassing 类，保证编辑器启动和打包时都能正确注册。

### 使用限制

| 限制 | 说明 |
|------|------|
| 禁多继承 UE 类型 | `class X(ue.Actor, ue.Object)` ❌ |
| 允许多继承 Python 类型 | `class X(ue.Actor, MyMixin)` ✅，但 UE 类型必须为第一基类 |
| 禁继承蓝图类 | 蓝图类可被卸载，极其危险 |
| 禁 `__init__` | 无法正常工作，用 `__init_default__` 替代 |
| 禁重名 | Python 中定义的 UE 类型必须全局唯一 |

---

## 二、`@ue.ustruct()` — 定义 UE 结构体

继承 `ue.StructBase`，使用 `@ue.ustruct()` 装饰器：

```python
@ue.ustruct()
class MyStruct(ue.StructBase):
    IntValue = ue.uproperty(int)
    FloatValue = ue.uproperty(float)

    def __init_default__(cdo):
        cdo.IntValue = 3
        cdo.FloatValue = 1.0
```

支持作为：蓝图成员变量、蓝图函数参数/返回值、Python 中 UE 类的成员属性、其他结构体的成员。

### 使用限制

- **禁止 UFUNCTION** — UE 结构体不允许存在 `@ue.ufunction`
- **禁止重名类型**
- **禁止 `__init__`** — 用 `__init_default__` 设置默认值
- **支持普通 Python 方法** — `def foo(self)` 是允许的

### 结构体说明符与元数据

```python
@ue.ustruct(DisplayName='MyStruct')
class StructWithDisplayName(ue.StructBase): pass

@ue.ustruct(meta={'Tooltip': '在Python中定义的结构体'})
class MyStruct(ue.StructBase): pass
```

---

## 三、`@ue.uenum()` — 定义 UE 枚举

继承 `ue.EnumBase`，使用 `@ue.uenum()` 装饰器，用 `ue.uvalue()` 定义枚举值：

```python
@ue.uenum()
class MyEnum(ue.EnumBase):
    Red = ue.uvalue(1)
    Green = ue.uvalue(2)
    Blue = ue.uvalue(3)
```

可作为蓝图成员变量、函数参数/返回值、Python UE 类的属性类型。

### 枚举说明符与元数据

```python
@ue.uenum(DisplayName='MyEnum')
class EnumWithDisplayName(ue.EnumBase): pass

@ue.uenum(meta={'DisplayName': '我的枚举'})
class MyEnum(ue.EnumBase):
    Red = ue.uvalue(1)
    Green = ue.uvalue(2, meta={'Tooltip': '是绿色的'})
```

### 类型注册

与类相同，通过 `import` 注册，建议在 `nepyinit.py` 中 import。

---

## 四、`ue.uproperty()` — 定义属性

### 基本语法

```python
@ue.uclass()
class MyActor(ue.Actor):
    # 基础类型 — 值即默认值
    BoolValue = ue.uproperty(True)
    IntValue = ue.uproperty(42)
    FloatValue = ue.uproperty(3.14)
    StringValue = ue.uproperty("hello")

    # UE 类型 — 传类型即可
    EnumValue = ue.uproperty(ue.EAxis)
    StructValue = ue.uproperty(ue.Vector)
    ObjectValue = ue.uproperty(ue.Object)

    # 智能指针
    ObjectClassValue = ue.uproperty(ue.TSubclassOf[ue.Object])
    SoftObjectValue = ue.uproperty(ue.TSoftObjectPtr[ue.Object])
    SoftClassValue = ue.uproperty(ue.TSoftClassPtr[ue.Object])
    WeakObjectValue = ue.uproperty(ue.TWeakObjectPtr[ue.Object])

    # 容器
    StringArray = ue.uproperty([str])
    StringSet = ue.uproperty({str})
    StringIntMap = ue.uproperty({str: int})
```

### 自引用类型

当属性类型就是定义类本身时，用 `ue.SelfClass`：

```python
@ue.uclass()
class MyActor(ue.Actor):
    ObjectValue = ue.uproperty(ue.SelfClass)
    ObjectClassValue = ue.uproperty(ue.TSubclassOf[ue.SelfClass])
```

### 属性默认值 — `__init_default__`

**用 `__init_default__` 而非 `__init__`**。该函数在类构建时（import 时）调用一次，设置到 CDO 上。`__init_default__` 的优先级高于声明时的默认值。

```python
@ue.uclass()
class MyActor(ue.Actor):
    IntValue = ue.uproperty(1)  # 声明默认值

    def __init_default__(cdo):
        cdo.IntValue = 2         # 覆盖为 2
        cdo.BoolValue = True
        cdo.StructValue = ue.Vector(0, 1, 0)
        cdo.StringArrayValue = ['hi', 'nepy']
        cdo.StringSetValue = {'hi', 'nepy'}
        cdo.StringIntMapValue = {'hi': 1, 'nepy': 2}
```

### ⚠️ 禁止在 `__init_default__` 中初始化纯 Python 实例变量

```python
def __init_default__(self):
    # ❌ 不生效或热重载后丢失
    self._cache = {}
    self._handle = None

    # ✅ 正确：仅设置 ue.uproperty / ue.ucomponent 的 UE 默认值
    self.CollisionComponent.SetSphereRadius(10.0)
```

需要 Python 状态时，在 `ReceiveBeginPlay` 中懒初始化。

### 属性说明符 (Property Specifiers)

```python
Value1 = ue.uproperty(int, VisibleAnywhere=True)  # 只读显示
Value2 = ue.uproperty(int, DisplayName='MyProp')  # 自定义显示名
```

> 默认行为（`EditAnywhere` + `BlueprintReadWrite`）可在 "项目设置 → 插件 → NePythonBinding" 中修改。

### 属性访问器 (BlueprintGetter / BlueprintSetter)

```python
@ue.uclass()
class MyActor(ue.Actor):
    IntValue = ue.uproperty(int,
        BlueprintGetter='IntValueGetter',
        BlueprintSetter='IntValueSetter')

    @ue.ufunction(ret=int, BlueprintGetter=True)
    def IntValueGetter(self):
        return self._IntValue

    @ue.ufunction(params=(int,), BlueprintSetter=True)
    def IntValueSetter(self, value):
        self._IntValue = value
```

> 设置访问器后，直接访问属性会调用访问器。用 `_属性名` 访问原始值。

---

## 五、`ue.ucomponent()` — 定义组件

```python
@ue.uclass()
class MyActor(ue.Actor):
    SceneComp = ue.ucomponent(ue.SceneComponent)
    MeshComp = ue.ucomponent(ue.StaticMeshComponent)

    # 指定根组件
    Root = ue.ucomponent(ue.BillboardComponent, root=True)

    # 指定附着关系（ComponentName 即 Python 变量名）
    ChildComp = ue.ucomponent(ue.BoxComponent, attach='Root')

    # Override 父类组件类型
    OverriddenComp = ue.ucomponent(ue.StaticMeshComponent, override="SceneComp")
```

| 参数 | 说明 |
|------|------|
| 第一个参数 | 组件类（支持几乎所有 ActorComponent 子类） |
| `root=True` | 指定为根组件 |
| `attach='CompName'` | 附着到另一个组件上 |
| `override='CompName'` | 覆写父类同名组件的类型 |

> ⚠️ 当前版本 `reload` 还有问题，修改 RootComponent 后只能创建新蓝图。

### 组件继承

子类自动继承父类所有组件属性（含 C++ Native 父类的组件）。

---

## 六、`@ue.ufunction()` — 定义方法

### 基本用法

```python
@ue.uclass()
class MyActor(ue.Actor):

    @ue.ufunction()                            # 无参无返回值
    def FuncA(self):
        pass

    @ue.ufunction(params=(int, float))         # 指定参数类型
    def FuncB(self, IntParam, FloatParm):
        pass

    @ue.ufunction(params=(int, float), ret=bool)  # 指定返回值类型
    def FuncC(self, IntParam, FloatParm):
        return True

    @ue.ufunction(params=(int, float), ret=(bool, str))  # 多返回值
    def FuncD(self, IntParam, FloatParm):
        return True, 'hi'
```

> 默认带有 `BlueprintCallable` 标记，可在蓝图中调用。可在插件设置中修改此默认行为。

### 参数和返回值支持的类型

```python
@ue.ufunction(params=(
    bool,               # 布尔值
    int,                # 整数
    float,              # 浮点数
    str,                # 字符串
    ue.EAxis,           # 枚举值
    ue.Object,          # 对象引用
    ue.Vector,          # 结构体
    [str],              # 数组
    {str},              # 集合
    {str: int}          # 字典
), ret=bool)
def Foo(self, BoolParam, IntParam, FloatParam, StringParam, EnumParam, ObjectParam, StructParam, StringArrayParam, StringSetParam, StringIntMapParam):
    return True
```

### 方法覆写 `override=True`

用于覆写 C++ 基类的 `BlueprintImplementableEvent` 或 `BlueprintNativeEvent`：

```python
@ue.uclass()
class MyActor(ue.Actor):
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        ue.LogWarning("BeginPlay!")

# 调用基类方法
@ue.uclass()
class MyChildActor(MyActor):
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        super(MyChildActor, self).ReceiveBeginPlay()
        ue.LogWarning("Child BeginPlay!")
```

> ⚠️ 覆写时不可再用 `params` 或 `ret`。

### BlueprintEvent — 让蓝图覆写 Python 方法

```python
@ue.ufunction(params=(int, float), ret=bool, BlueprintEvent=True)
def FuncC(self, IntParam, FloatParm):
    return True
```

蓝图子类可覆写此方法，调用 Parent 节点执行 Python 基类逻辑。

### 静态方法与 Pure 方法

```python
# 静态方法
@ue.ufunction(params=(int, float), ret=(bool, str))
@staticmethod()
def StaticFunc(IntParam, FloatParam):
    return True, 'hi'

# Pure 方法（蓝图中无执行 Pin）
@ue.ufunction(ret=bool, BlueprintPure=True)
def PureFunc(self):
    return True
```

### 引用参数 `ue.ref()`

等价于 C++ 的 `UPARAM(ref)`，用于参数既是输入也是输出：

```python
@ue.ufunction(params=(ue.ref(int), float, ue.ref(int)))
def FuncA(self, IntParamRef, FloatParam, IntParam2Ref):
    # ue.ref 参数作为额外返回值，追加在常规返回值之后
    return IntParamRef * 1, IntParam2Ref * 2
```

### 参数元数据 `ue.uparam()`

```python
@ue.ufunction(
    params=(ue.uparam(str, DisplayName='名称'),),
    ret=(ue.uparam(ue.Actor, DisplayName='Npc对象')))
def SpawnNpc(self, Name):
    ...
```

### 方法说明符 (Function Specifiers)

```python
@ue.ufunction(CallInEditor=True)            # 编辑器中作为按钮
def Foo(self): pass

@ue.ufunction(Server=True, Reliable=True)   # 网络：服务器执行，可靠传输
def Bar(self): pass
```

### 方法元数据

```python
@ue.ufunction(params=(int, float), ret=bool, meta={'Tooltip': '在Python中定义的方法'})
def Bar(self, IntParam, FloatParm):
    return True
```

---

## 七、`ue.udelegate()` — 定义委托

### 委托作为成员属性（多播委托）

```python
@ue.uclass()
class MyActor(ue.Actor):
    DelegateProp = ue.udelegate(params=((int, 'Param1'), (int, 'Param2')))
```

**Python 中使用**：
```python
actor.DelegateProp.Add(callback)
actor.DelegateProp.Broadcast(123, 456)
```

**动态绑定**：
```python
# 推荐写法
actor.DelegateProp.AddDynamic(o.Func)

# 等效写法
actor.DelegateProp.AddDynamic(o, Class.Func)
actor.DelegateProp.AddDynamic(o, o.Func)

# 移除同理
actor.DelegateProp.RemoveDynamic(o.Func)
```

**蓝图中使用**：Python 中定义的委托成员类似蓝图 Event Dispatcher，可在蓝图子类中绑定。

### 委托作为方法参数（单播委托）

```python
@ue.uclass()
class MyActor(ue.Actor):
    @ue.ufunction(params=(ue.udelegate(params=(int, int))))
    def FuncTakesDelegateParam(self, delegate):
        delegate.ExecuteIfBound(123, 456)
```

蓝图可向此函数传递委托参数。

### 委托元数据

```python
DelegateProp = ue.udelegate(
    params=((int, 'Param1'), (int, 'Param2')),
    meta={'DisplayName': '在Python中定义的委托'})
```

> ⚠️ 数组容器作为委托参数时，必须在回调返回值中追加此数组参数。

---

## 八、汇总示例

```python
import ue

# 枚举
@ue.uenum()
class EWeaponType(ue.EnumBase):
    Melee = ue.uvalue(1)
    Range = ue.uvalue(2)

# 结构体
@ue.ustruct()
class FWeaponStats(ue.StructBase):
    Damage = ue.uproperty(float)
    FireRate = ue.uproperty(float)

# 类
@ue.uclass()
class BP_GASCharacter(ue.Character):
    # 属性
    Health = ue.uproperty(100.0)
    WeaponType = ue.uproperty(EWeaponType)
    Stats = ue.uproperty(FWeaponStats)

    # 组件
    SpringArm = ue.ucomponent(ue.SpringArmComponent)
    Camera = ue.ucomponent(ue.CameraComponent, attach='SpringArm')

    # 委托
    OnHealthChanged = ue.udelegate(params=((float, 'OldValue'), (float, 'NewValue')))

    # CDO 初始化
    def __init_default__(self):
        self.SpringArm.TargetArmLength = 300.0
        self.Stats = FWeaponStats()
        self.Stats.Damage = 50.0

    # 覆写
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        ue.Log(f"[{self.get_name()}] BeginPlay, Health={self.Health}")

    # BlueprintCallable
    @ue.ufunction(params=(float,), ret=bool)
    def TakeDamage(self, Amount: float) -> bool:
        old_health = self.Health
        self.Health -= Amount
        self.OnHealthChanged.Broadcast(old_health, self.Health)
        return self.Health <= 0
```
