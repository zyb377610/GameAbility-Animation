# NEPY 常用编程模式

## 一、资产加载

### LoadClass — 加载蓝图类

```python
# 同步加载蓝图类（注意 _C 后缀）
bullet_bp = ue.LoadClass('/Game/MyBulletBP.MyBulletBP_C')

# ⚠️ 加载后如不立即使用，必须调用 OwnByPython 防止 GC
bullet_bp.OwnByPython()
```

### LoadObject — 加载非蓝图资产

```python
texture = ue.LoadObject(ue.Texture2D, '/Game/Textures/MyTexture')
mesh = ue.LoadObject(ue.StaticMesh, '/Game/Meshes/MyMesh')
```

### 异步加载（避免卡顿）

```python
ue.AsyncLoadClass('/Game/MyBulletBP.MyBulletBP_C')
ue.AsyncLoadObject(ue.StaticMesh, '/Game/Meshes/MyMesh')
```

### PIE 中加载蓝图：FindObject 优先

`ue.LoadObject` 在 PIE 中可能失败。**推荐加载顺序**：

```python
def _load_class(self, path, class_name):
    # 1. FindObject（PIE 可用）
    obj = ue.FindObject(f"/Game/Path/{class_name}.{class_name}_C")
    if obj: return obj

    # 2. LoadClass（同步加载）
    obj = ue.LoadClass(f"/Game/Path/{class_name}.{class_name}_C")
    if obj: return obj

    # 3. Python 类引用（仅当 import 链正常）
    from my_module import MyPythonClass
    return MyPythonClass.Class()
```

### LoadClass vs LoadObject

| API | 用途 | 后缀 |
|-----|------|------|
| `ue.LoadClass(path)` | 加载蓝图**类** | `_C` 后缀 |
| `ue.LoadObject(type, path)` | 加载**资产**（纹理、模型等） | 无 `_C` 后缀 |
| `ue.FindObject(path)` | 从内存查找已加载的对象 | `_C` 后缀 |

---

## 二、SpawnActor — 创建对象实例

### 使用 World.SpawnActor（简单场景）

```python
world = self.GetWorld()
location = mesh.GetSocketLocation('Muzzle')
rotation = mesh.GetSocketRotation('Muzzle')

# 以蓝图类为模板，创建实例并放入场景
bullet = world.SpawnActor(bullet_class, location, rotation)
```

### 使用两阶段 Spawn（需要设属性）

```python
# 阶段1：创建未初始化 Actor
actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(
    self, spawn_class, spawn_transform
)
# 阶段2之间：设置属性
actor.SomeProperty = some_value
# 阶段2：完成初始化
ue.GameplayStatics.FinishSpawningActor(actor, spawn_transform)
```

### 运行时从 Python 类名查找 UClass

```python
# ❌ 不能直接用 Python class
# world.SpawnActor(MyPythonClass, ...)

# ✅ 用 FindClass 获取 UClass
spawn_class = ue.FindClass('MyPythonClassName')
actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(
    self, spawn_class, spawn_transform
)
```

---

## 三、委托绑定

### 绑定 C++ 委托到 Python 回调

```python
@ue.uclass()
class MyBullet(ue.Actor):
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        # 绑定碰撞事件
        collision_comp = self.SphereCollision  # type: ue.SphereComponent
        collision_comp.OnComponentHit.Add(self._on_hit)

    def _on_hit(self, hit_comp, other_actor, other_comp, normal_impulse, hit):
        # type: (ue.PrimitiveComponent, ue.Actor, ue.PrimitiveComponent, ue.Vector, ue.HitResult) -> None
        if other_comp.IsSimulatingPhysics():
            other_comp.AddImpulseAtLocation(
                self.GetVelocity() * 100,
                self.GetActorLocation()
            )
```

### 常用委托

| 委托 | 所在组件 | 触发时机 |
|------|---------|---------|
| `OnComponentHit` | PrimitiveComponent | 物理碰撞 |
| `OnComponentBeginOverlap` | ShapeComponent | 重叠开始 |
| `OnComponentEndOverlap` | ShapeComponent | 重叠结束 |

### 注意：只能使用动态委托

只有以 `DECLARE_DYNAMIC` 开头的委托才能在 Python 中使用。静态委托绑定的是编译期函数，无法绑定 Python/蓝图回调。

---

## 四、输入处理

### 轴映射（Axis）- 持续输入

在项目设置中配置输入映射后：

```python
@ue.uclass()
class MyCharacter(ue.Character):
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        self.InputComponent.BindAxis('MoveForward', self._move_forward)
        self.InputComponent.BindAxis('MoveRight', self._move_right)
        self.InputComponent.BindAxis('TurnRight', self._turn_right)
        self.InputComponent.BindAxis('LookUp', self._look_up)

    def _move_forward(self, value):
        if value != 0:
            self.AddMovementInput(self.GetActorForwardVector(), value)

    def _move_right(self, value):
        if value != 0:
            self.AddMovementInput(self.GetActorRightVector(), value)

    def _turn_right(self, value):
        self.AddControllerYawInput(value * self.MouseSpeed * ue.GetDeltaTime())

    def _look_up(self, value):
        self.AddControllerPitchInput(value * self.MouseSpeed * ue.GetDeltaTime())
```

### 操作映射（Action）- 单次触发

```python
self.InputComponent.BindAction('Jump', ue.EInputEvent.IE_Pressed, self._jump)
self.InputComponent.BindAction('Fire', ue.EInputEvent.IE_Pressed, self._fire)
```

---

## 五、类型判断

### IsA vs isinstance

```python
# ✅ 推荐：UE 原生方式，热重载安全
if other_actor.IsA(MyCharacter):
    ...

# ⚠️ Python isinstance 在热重载后可能失效
if isinstance(other_actor, MyCharacter):
    ...
```

### 判断 UE 对象有效性

```python
if not actor.IsValid():
    print("Actor has been destroyed")
    return
```

---

## 六、Vector / Rotator / Transform

```python
# 向量
pos = ue.Vector(100, 200, 300)
length = pos.Size()
normalized = pos.GetSafeNormal()
dist = ue.Vector.Dist(pos1, pos2)

# 旋转 (Pitch, Yaw, Roll)
rot = ue.Rotator(0, 90, 0)
forward = rot.Vector()

# 变换
transform = ue.Transform()
transform.Translation = ue.Vector(100, 0, 0)
transform.Rotation = ue.Rotator(0, 45, 0)
transform.Scale3D = ue.Vector(1, 1, 2)
```

### ⚠️ 结构体是值拷贝

```python
# ❌ 错误：不会生效！
actor.GetActorLocation().X = 10

# ✅ 正确：取出来 → 修改 → 设回去
pos = actor.GetActorLocation()
pos.X = 10
actor.SetActorLocation(pos)
```

详见 [对象生命周期管理](object-lifecycle.md)。

---

## 七、遍历 Actor

```python
# PyIterator 遍历世界中的所有 Actor
world = self.GetWorld()
for actor in ue.PyIterator(ue.Actor, world):
    print(actor.GetName())

# 按标签查找
tagged_actors = ue.PyUtil.get_actors_by_tag(world, "MyTag")

# 编辑器：获取选中 Actor
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
selected = editor.get_selected_level_actors()
```

---

## 八、完整示例：从零创建可操控角色

参见 [`class-authoring.md`](class-authoring.md) 的汇总示例，以及 [Ticker与Timer](ticker-timer.md) 的 Tick 顺序说明。

核心模式：
1. `@ue.uclass()` 定义 Character → import 注册
2. 创建蓝图 → 添加 Component（Camera、Mesh）
3. 在 `ReceiveBeginPlay` 中绑定输入（`BindAxis` / `BindAction`）
4. 在蓝图子类中添加变量（如 `MouseSpeed`），Python 中通过 `self.MouseSpeed` 访问
