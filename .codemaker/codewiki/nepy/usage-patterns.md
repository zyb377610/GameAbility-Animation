# NePy 常用编程模式

## 子类化 UE 类

使用 `@ue.uclass()` 装饰器子类化 UE 类。**注意：`@ue.uclass()` 必须用，否则 UE 反射系统不会注册你的类。**

```python
import ue

@ue.uclass()
class MyActor(ue.Actor):
    """自定义 Actor"""
    
    def receive_begin_play(self):
        ue.log(f"{self.get_name()} begin play from Python!")
        super().receive_begin_play()
    
    def receive_tick(self, delta_seconds: float):
        pass

# 方法如需被蓝图/AnimNotify 调用，加 @ue.ufunction()
@ue.uclass()
class MyAnimInstance(ue.AnimInstance):
    
    @ue.ufunction()  # → BlueprintCallable
    def on_some_event(self):
        pass
    
    @ue.ufunction(override=True)  # → 覆盖父类 UFUNCTION
    def ReceiveBeginPlay(self):
        pass
```

## UProperty / UFUNCTION / UComponent 定义

详见 [`class-authoring.md`](class-authoring.md)。这里只给最简示例：

```python
@ue.uclass()
class MyActor(ue.Actor):
    # UProperty — 必须用 ue.uproperty(默认值)
    Health = ue.uproperty(100.0)
    IsAlive = ue.uproperty(True)
    
    # UComponent — 声明子组件
    MyMesh = ue.ucomponent(ue.StaticMeshComponent)
    
    # CDO 初始化 — 用 __init_default__，不用 __init__
    def __init_default__(self):
        self.MyMesh.SetStaticMesh(...)
```

## 委托绑定

```python
@ue.uclass()
class MyActor(ue.Actor):
    def receive_begin_play(self):
        # 绑定碰撞事件
        mesh = self.get_component_by_class(ue.StaticMeshComponent)
        if mesh:
            mesh.on_component_begin_overlap.Add(self.on_overlap)
    
    def on_overlap(self, overlapped_component, other_actor, other_comp, other_body_index, from_sweep, sweep_result):
        ue.log(f"Overlapped with {other_actor.get_name()}")
```

## 资产加载

```python
import ue

# 同步加载资产
texture = ue.load_object(ue.Texture2D, '/Game/Textures/MyTexture')
mesh = ue.load_object(ue.StaticMesh, '/Game/Meshes/MyMesh')

# 使用软引用
@ue.uclass()
class MyActor(ue.Actor):
    soft_mesh: ue.TSoftObjectPtr[ue.StaticMesh] = None
    
    def receive_begin_play(self):
        if self.soft_mesh.IsValid():
            mesh = self.soft_mesh.Get()
        elif not self.soft_mesh.IsNull():
            mesh = self.soft_mesh.LoadSynchronous()
```

## 获取 / 遍历 Actor

```python
import ue

# 通过编辑器子系统获取选中的 Actor
editor_subsystem = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
selected_actors = editor_subsystem.get_selected_level_actors()

# 遍历世界中所有 Actor
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
actor_iterator = ue.PyIterator(ue.Actor, world)
for actor in actor_iterator:
    ue.log(actor.get_name())

# 按标签查找
tagged_actors = ue.PyUtil.get_actors_by_tag(world, "MyTag")
```

## 世界与关卡操作

```python
# 获取编辑器世界
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()

# 在当前关卡中生成 Actor
spawned_actor = world.spawn_actor(MyActor, ue.Vector(0, 0, 100))

# 获取当前关卡
current_level = world.get_current_level()
```

## 蓝图调用

```python
# 蓝图类以 _C 后缀结尾
bp_class = ue.load_class('/Game/Blueprints/BP_MyActor.BP_MyActor_C')

# 蓝图结构体以 _C 后缀
my_struct = ue.MyBlueprintStruct_C()
my_struct.some_field = 10
```

## Vector / Rotator / Transform

```python
# 向量
pos = ue.Vector(100, 200, 300)
length = pos.size()
normalized = pos.get_safe_normal()
dist = ue.Vector.dist(pos1, pos2)

# 旋转
rot = ue.Rotator(0, 90, 0)  # Pitch, Yaw, Roll
forward = rot.vector()  # 获取前方向量

# 变换
transform = ue.Transform()
transform.translation = ue.Vector(100, 0, 0)
transform.rotation = ue.Rotator(0, 45, 0)
transform.scale3d = ue.Vector(1, 1, 2)
```

## 日志输出

```python
print("用 print 即可")           # 输出到 LogNePython
ue.Log("普通日志")               # 大写 Log，Log 级别
ue.LogWarning("警告日志")        # Warning 级别
ue.LogError("错误日志")          # Error 级别
```

## 编辑器工具

```python
import ue

# 使用 Python 编写编辑器工具
@ue.uclass()
class MyEditorUtility(ue.PythonEditorUtility):
    def run(self):
        actors = self.get_selected_actors()
        for actor in actors:
            actor.set_actor_label("Processed")
        ue.log(f"Processed {len(actors)} actors")
```
