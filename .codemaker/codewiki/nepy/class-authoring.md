# NePy 类编写完整指南

> 覆盖 `@ue.uclass` / `@ue.uproperty` / `@ue.ucomponent` / `@ue.ufunction` / `__init_default__` 的完整语法和对比。

---

## 一、`@ue.uclass()` — 注册 Python 类为 UE UClass

**必须用。** 不加这个装饰器，UE 反射系统不识别你的类，`InitStats`/`FindClass`/蓝图引用全部失效。

```python
import ue

@ue.uclass()
class MyCharacter(ue.Character):
    pass
```

| 要点 | 说明 |
|------|------|
| 无参数 | `@ue.uclass()` 不加任何参数 |
| 父类 | 必须继承自 UE C++ 类（`ue.Actor`, `ue.Character`, `ue.AttributeSet`, `ue.GameplayAbility` 等）或已有的 `@ue.uclass()` 类 |
| 注册时机 | import 模块时触发，所以必须在 `nepyinit.py` 的 `on_init()` 中 import |

---

## 二、`ue.uproperty(默认值)` — 暴露属性到 UE 反射

**语法：`属性名 = ue.uproperty(默认值)`**

必须写在 class body 层级，**不能**写在 `__init__` 或 `__init_default__` 中。

### 支持的类型和默认值写法

| Python 类型 | 写法 | 说明 |
|-------------|------|------|
| `float` | `Health = ue.uproperty(100.0)` | 健康值、伤害、速度等 |
| `int` | `AmmoCount = ue.uproperty(30)` | 弹药、计数 |
| `bool` | `IsAlive = ue.uproperty(True)` | 状态标记 |
| `str` | `MontagePath = ue.uproperty('')` | 动画路径、名称 |
| `ue.Vector` | `LastLocation = ue.uproperty(ue.Vector)` | 只传类型，无默认值 |
| `ue.Rotator` | `LastRotation = ue.uproperty(ue.Rotator)` | 只传类型，无默认值 |

```python
@ue.uclass()
class AttrSet_Base(ue.AttributeSet):
    Health = ue.uproperty(100.0)
    MaxHealth = ue.uproperty(100.0)
    AttackPower = ue.uproperty(10.0)
    MoveSpeed = ue.uproperty(600.0)
```

### ⚠️ 常见错误

```python
# ❌ 错误：纯 Python 类型注解不会注册到 UE 反射
health: float = 100.0

# ❌ 错误：写在 __init__ 里不会注册
def __init__(self):
    self.Health = ue.uproperty(100.0)

# ✅ 正确：class body 层级用 ue.uproperty()
Health = ue.uproperty(100.0)
```

---

## 三、`ue.ucomponent(类, attach='父组件名')` — 声明子组件

在 `@ue.uclass()` 类中声明蓝图可见的子组件。

```python
@ue.uclass()
class MyCharacter(ue.Character):
    # 独立组件（会作为 Actor 的直接子组件）
    SpringArm = ue.ucomponent(ue.SpringArmComponent)
    
    # 附着到其他组件
    Camera = ue.ucomponent(ue.CameraComponent, attach='SpringArm')
    
    # 碰撞体
    CollisionSphere = ue.ucomponent(ue.SphereComponent)
    
    # 可渲染组件
    WeaponMesh = ue.ucomponent(ue.SkeletalMeshComponent)
    BulletMesh = ue.ucomponent(ue.StaticMeshComponent, attach='CollisionSphere')
```

| 参数 | 说明 |
|------|------|
| 第一个参数 | 组件类（如 `ue.SpringArmComponent`） |
| `attach='Name'` | 父组件在本类中的属性名（字符串） |

---

## 四、`@ue.ufunction()` — 标记方法为 UFUNCTION

### 两种模式

```python
@ue.uclass()
class MyActor(ue.Actor):

    @ue.ufunction()            # BlueprintCallable + AnimNotify 可调用
    def MyCustomEvent(self):
        """蓝图、AnimNotify 可以调这个方法"""
        pass

    @ue.ufunction(override=True)  # 覆盖父类已有 UFUNCTION
    def ReceiveBeginPlay(self):
        """覆盖 Actor::ReceiveBeginPlay"""
        pass
```

| 装饰器 | 用途 |
|--------|------|
| `@ue.ufunction()` | 新增 BlueprintCallable 方法。AnimNotify 可调用。 |
| `@ue.ufunction(override=True)` | 覆盖父类的 UFUNCTION。常用：`ReceiveBeginPlay`, `ReceiveTick`, `ReceiveAnyDamage`, `ReceiveEndPlay` |

---

## 五、`__init_default__()` — CDO 初始化

**用 `__init_default__` 替代 `__init__`。**

Nepy 中 `@ue.uclass()` 类的构造函数是 `__init_default__`，它在类注册为 CDO（Class Default Object）时调用一次，不是每次创建实例时调用。

```python
@ue.uclass()
class MyActor(ue.Actor):
    CollisionSphere = ue.ucomponent(ue.SphereComponent)

    def __init_default__(self):
        """CDO 初始化 — 只调用一次"""
        self.CollisionSphere.SetSphereRadius(10.0)
        self.CollisionSphere.SetCollisionProfileName("OverlapAll")
```

⚠️ **不要用 `__init__`** — Nepy 的 `@ue.uclass()` 类不支持 Python 标准的 `__init__`。

### ⚠️ 禁止在 `__init_default__` 中初始化纯 Python 实例变量

**这是最常见的踩坑点。** `self.xxx = value` 只在 CDO 创建时执行一次，且热重载/实例复制时会丢失。

```python
@ue.uclass()
class MyClass(ue.Object):
    def __init_default__(self):
        # ❌ 错误：以下赋值均不生效（或只在 CDO 上短暂存在）
        self._cache = {}
        self._handle = None
        self._callback = None
        self._listener_ref = some_object  # 热重载后丢失

        # ✅ 正确：仅对 ue.ucomponent 配置默认值
        self.CollisionComponent.SetSphereRadius(10.0)
```

**为什么会失败？**

1. `__init_default__` 在 CDO 上执行，CDO 是 C++ 对象模板，Python shadow 对象上的 `self.xxx` 不被序列化
2. 每次从 CDO 复制实例时，Python 成员不会被复制到新实例
3. NePy 会打印警告：
   > `you are trying to initialize python members in 'XXX.__init_default__()', which will take no effect.`

**替代方案：**

| 场景 | 正确做法 |
|------|---------|
| 需要实例级 Python 状态 | 在 `ReceiveBeginPlay` / 普通方法中赋值，**首次访问时懒初始化** |
| 需要缓存/回调引用 | 用模块级变量 + 闭包，**不依赖 `@ue.uclass()` 类** |
| 简单配置常量 | 用 `ue.uproperty()` 声明（写在 class body 层级） |
| 不需要 UE 反射的状态 | 考虑这段逻辑是否真的需要放在 `@ue.uclass()` 类里；很多场景用**纯 Python 函数 + 闭包**更简洁可靠 |

```python
# ✅ 懒初始化模式
@ue.uclass()
class MyActor(ue.Actor):
    def get_cache(self):
        if not hasattr(self, '_cache'):
            self._cache = {}
        return self._cache

# ✅ 更推荐：不需要 UE 反射的场景，直接用普通 Python
def create_tag_listener(asc, mesh):
    """纯 Python 函数，闭包持有状态，不需要 @ue.uclass()"""
    anim_inst = mesh.GetAnimInstance()
    def on_changed(tag, count):
        setattr(anim_inst, "bIsHit", count > 0)
    handle = bind_tag_event(asc, on_changed)
    return handle  # 调用者负责管理生命周期
```

### vs. 普通 `ReceiveBeginPlay`

| 方法 | 调用时机 | 用途 |
|------|---------|------|
| `__init_default__` | 类注册时（一次） | **仅**设置组件默认值、碰撞配置 |
| `ReceiveBeginPlay` | 每个实例开始游戏时 | 绑定委托、初始化运行时状态、懒初始化 Python 变量 |

---

## 六、汇总示例

```python
import ue

@ue.uclass()
class BP_GASCharacter(ue.Character):
    # === UProperties ===
    Health = ue.uproperty(100.0)
    MaxHealth = ue.uproperty(100.0)
    IsDead = ue.uproperty(False)

    # === UComponents ===
    SpringArm = ue.ucomponent(ue.SpringArmComponent)
    Camera = ue.ucomponent(ue.CameraComponent, attach='SpringArm')

    # === CDO Init ===
    def __init_default__(self):
        self.SpringArm.TargetArmLength = 300.0

    # === Overrides ===
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        ue.log(f"[{self.get_name()}] BeginPlay")

    # === BlueprintCallable ===
    @ue.ufunction()
    def TakeDamage(self, Amount: float):
        self.Health -= Amount
        if self.Health <= 0:
            self.IsDead = True
```
