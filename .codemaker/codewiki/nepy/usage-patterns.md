# NePy 常用编程模式

## 子类化 UE 类

使用 `@ue.uclass()` 装饰器子类化 UE 类，覆盖蓝图可重写方法：

```python
import ue

@ue.uclass()
class MyActor(ue.Actor):
    """自定义 Actor"""
    
    def receive_begin_play(self):
        """覆盖 BeginPlay"""
        ue.log(f"{self.get_name()} begin play from Python!")
        super().receive_begin_play()
    
    def receive_tick(self, delta_seconds: float):
        """覆盖 Tick"""
        pass

@ue.uclass()
class MyComponent(ue.ActorComponent):
    """自定义 Component"""
    
    def receive_begin_play(self):
        pass
```

## 定义 UProperty / UFunction

```python
@ue.uclass()
class MyActor(ue.Actor):
    # UProperty
    health: float = 100.0
    max_health: float = 100.0
    owner_name: str = ""
    target_actor: ue.Actor = None
    enemy_list: ue.ArrayWrapper[ue.Actor] = None
    config_map: ue.MapWrapper[str, float] = None
    
    # UFUNCTION
    def take_damage(self, amount: float) -> None:
        self.health -= amount
        if self.health <= 0:
            self.on_death()
    
    def on_death(self) -> None:
        ue.log(f"{self.get_name()} died!")
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
ue.log("普通日志")
ue.log_warning("警告日志")
ue.log_error("错误日志")
ue.print_string("屏幕打印")  # 打印到游戏屏幕
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
