# NePy 项目搭建与运行

> 覆盖 `nepyinit.py` 生命周期、import 链触发类注册、`FindClass` + 两阶段 Spawn、热重载注意事项。

---

## 一、`nepyinit.py` 生命周期

`nepyinit.py` 是 NePy 的入口文件，位于 `Content/Scripts/nepyinit.py`。NePy 加载时自动调用其中的钩子函数。

```python
# -*- encoding: utf-8 -*-
import ue
import traceback

def on_init():
    """NePy 初始化时调用。在此 import 所有业务模块。"""
    ue.Log('[MyProject] Nepy initialized.')
    
    # 编辑器工具
    if ue.GIsEditor:
        try:
            import reload_monitor
            reload_monitor.start()
        except Exception:
            traceback.print_exc()
        try:
            import gmcmds
            gmcmds.debug()
        except Exception:
            traceback.print_exc()
    
    # === 导入业务模块（触发 @ue.uclass() 注册）===
    import core.game_mode
    import characters.player_character
    import gas.setup_character
    # ... 其他模块

def on_shutdown():
    """NePy 关闭时调用"""
    ue.Log('[MyProject] NePy shutdown.')

def on_tick(dt: float):
    """每帧全局回调"""
    pass
```

### ⚠️ 关键：import 即注册

**`@ue.uclass()` 在 import 模块时触发注册。** 如果你的 Python 类没有被 import（或 import 链断了），UE 反射系统就不知道这个类的存在。

检查清单：
- [ ] 所有 `@ue.uclass()` 类所在的 `*.py` 文件都在 `on_init()` 中 import 了
- [ ] 没有循环 import 导致部分模块 import 失败
- [ ] `on_init()` 中 import 语句没有放在 `if` 条件里（除非确实需要条件导入）

---

## 二、`ue.FindClass()` + 两阶段 Spawn

运行时动态创建 Python 定义的 `@ue.uclass()` 类实例，必须用 `ue.FindClass()` + 两阶段 Spawn。

```python
import ue

# ❌ 错误：不能直接用 Python class 做 SpawnActor
# ue.GameplayStatics.BeginDeferredActorSpawnFromClass(world, MyPythonClass, ...)

# ✅ 正确：用 FindClass 获取 UClass
spawn_class = ue.FindClass('MyPythonClassName')
actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(
    self, spawn_class, spawn_transform
)
# 在两阶段之间设置属性
actor.SomeProperty = some_value
ue.GameplayStatics.FinishSpawningActor(actor, spawn_transform)
```

| API | 说明 |
|-----|------|
| `ue.FindClass('ClassName')` | 按 Python 类名字符串查找 UClass |
| `BeginDeferredActorSpawnFromClass` | 创建未初始化的 Actor |
| `FinishSpawningActor` | 完成初始化 |

---

## 三、两层属性：`ue.uproperty` vs Python 注解

`@ue.uclass()` 类中，属性的存储方式分两种：

| 写法 | 存储位置 | 用途 |
|------|---------|------|
| `Health = ue.uproperty(100.0)` | UE 反射系统 | 需蓝图访问、编辑器配置、网络复制的属性 |
| `_cache: float = 0.0` | Python 运行时 | 纯 Python 内部状态，不需 UE 可见 |

**两者可以共存，互不干扰。**

```python
@ue.uclass()
class MyActor(ue.Actor):
    # UE 属性 — 蓝图可见
    Health = ue.uproperty(100.0)
    
    # Python 运行时属性 — 蓝图不可见
    _dirty: bool = False
    _targets: list = []
    
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        self._dirty = True
```

---

## 四、热重载注意事项

NePy 支持修改 `.py` 文件后无需重启编辑器。但有些陷阱：

### 1. `isinstance` 可能失效

重载后类的内存地址变了，`isinstance(obj, SomeClass)` 可能返回 `False`，即使对象原来确实是该类的实例。

✅ **用 `hasattr` 替代：**
```python
# ❌ 热重载后可能失效
if isinstance(weapon, RangedWeapon):
    ...

# ✅ 安全
if hasattr(weapon, '_is_reloading'):
    ...
```

### 2. 手动强制 reload

有时自动热重载没触发，手动执行：
```python
import importlib
import my_module
importlib.reload(my_module)
```

### 3. `reload_monitor.py` 机制

参考项目中的 `reload_monitor.py` 通过 Windows `ReadDirectoryChangesW` 监听 `.py` 文件变更，debounce 0.1s 后自动触发 `reloader.reload()`。

### 4. 模块级可变状态

```python
# ⚠️ 重载时这些会被重置为空！
_active_instances: list = []
_cached_references: dict = {}
```

热重载后需要重新初始化这些模块级状态。

### 5. `@ue.uclass()` 类方法变更需重启 PIE ⚠️

**关键坑**：`@reload` 只更新 Python 模块的 `__dict__`，`@ue.uclass()` 类中已有方法的 C++ 端绑定不会更新。

| 改动类型 | `@reload` 生效？ |
|---------|:--:|
| 纯 Python 函数（如 `gmcmds.py` 中的命令） | ✅ |
| `@ue.uclass()` 类中**新增方法** | ✅ |
| `@ue.uclass()` 类中**已有方法的实现修改** | ❌ 需重启 PIE |
| `@ue.uclass()` 类的 `__init_default__` 修改 | ❌ 需重启 PIE，且**不要在其中初始化纯 Python 实例变量**（详见 `class-authoring.md` §五） |

✅ **最佳实践**：修改 `@ue.uclass()` 类后直接关闭 PIE（编辑器 Stop）再重新 Play，不要依赖 `@reload`。

---

## 五、懒加载 import（避免循环依赖）

当模块间存在循环引用时，把 import 放到函数体内：

```python
# ❌ 顶层 import 可能导致循环
# from weapons.weapon_base import WeaponBase

def some_function(self):
    # ✅ 延迟 import
    from weapons.weapon_base import WeaponBase
    if isinstance(self.current_weapon, WeaponBase):
        ...
```

---

## 六、日志输出

```python
print("用 print 即可")           # 输出到 LogNePython，简单直接
ue.Log("普通日志")               # 同 LogNePython，Log 级别
ue.LogWarning("警告日志")        # Warning 级别
ue.LogError("错误日志")          # Error 级别
# 注意：是 ue.Log / ue.LogWarning / ue.LogError，大写开头，不是 ue.log
```

---

## 七、完整项目骨架模板

```
Content/Scripts/
├── nepyinit.py          # 入口，on_init 中 import 所有模块
├── reload_monitor.py    # 热重载监听
├── gmcmds.py            # 调试命令（@hello 等）
├── core/
│   ├── game_mode.py     # @ue.uclass GameMode
│   └── game_instance.py # GameInstance 代理
├── gas/
│   ├── __init__.py
│   └── setup_character.py  # AttrSet_Base, 初始化函数
├── characters/
│   └── player_character.py # @ue.uclass Character
├── weapons/
│   └── weapon_base.py      # @ue.uclass Weapon
└── utils/
    └── helpers.py          # 纯 Python 工具函数（不需要 @ue.uclass）
```

### nepyinit.py 中的 import 顺序

1. 编辑器工具（`reload_monitor`, `gmcmds`）
2. 基础设施（`core.*`）
3. 基础类（`characters.base_character`）
4. 派生类（`characters.player_character`, `characters.enemy.*`）
5. 其他子系统（`weapons.*`, `gas.*`, `ai.*`）

**原因**：父类必须在子类之前 import，否则 `@ue.uclass()` 注册时找不到父类 UClass。
