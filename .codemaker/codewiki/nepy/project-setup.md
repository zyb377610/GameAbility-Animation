# NEPY 项目搭建与生命周期

## 一、`nepyinit.py` — 脚本入口

`nepyinit.py` 是 NEPY 的入口文件，类似 C/C++ 的 `main` 函数。NEPY 插件加载后立刻加载它。

### 完整回调函数清单

```python
# -*- encoding: utf-8 -*-
import ue

def on_init():
    """NEPY 插件初始化时调用（StartupModule）。"""
    """在此 import 所有业务模块、Subclassing 类。"""
    pass

def on_shutdown():
    """NEPY 插件关闭时调用（ShutdownModule）。"""
    """在此执行清理工作。"""
    pass

def on_post_engine_init():
    """可选。引擎完全初始化后调用（OnPostEngineInit）。"""
    """比 on_init 晚，此时引擎各子系统已就绪。"""
    pass

def on_tick(delta_seconds: float):
    """可选。每帧调用一次。"""
    pass

def on_debug_input(cmd_str: str) -> bool:
    """可选。PythonConsole 输入时首先回调这里。"""
    """返回 True 表示已自行处理，不再 eval。"""
    return False
```

### 环境区分

```python
def on_init():
    if ue.GIsEditor:
        # 编辑器环境
        import reload_monitor
        reload_monitor.start()
    elif ue.IsRunningCommandlet():
        # Commandlet（如打包）—— 只做类型注册
        pass
    else:
        # 纯游戏环境
        pass

    # 以下在所有环境都需要执行
    import MySubclassingClasses  # Subclassing 注册
```

### ⚠️ import 即注册

**`@ue.uclass()` 在模块 import 时触发 UE 类型注册。** 必须确保所有 Subclassing 类在 `on_init()` 中被 import。建议把 import 放在 `nepyinit.py` 顶层或 `on_init()` 最早位置。

### 旧版兼容

旧版 NEPY 使用 `ue_site.py` 作为入口，功能相同。若两者同时存在，优先使用 `ue_site.py`。

---

## 二、`ue.GRuntimeDelegates` — 全局委托

NEPY 提供了运行时委托容器，可在 Python/C++ 中灵活访问：

```python
def on_tick_handler(delta_seconds):
    print(f"Tick: {delta_seconds}")

# 添加
ue.GRuntimeDelegates.OnTick.Add(on_tick_handler)

# 移除
ue.GRuntimeDelegates.OnTick.Remove(on_tick_handler)
```

---

## 三、GameInstance 代理模式

`GameInstance` 是全局单例，游戏开始时创建、结束时销毁。与 `nepyinit` 的区别：

```
nepyinit.init → GameInstance.init → 游戏循环 → GameInstance.shutdown → nepyinit.shutdown
```

在编辑器中，GameInstance **仅 PIE 期间存在**；`nepyinit` 在整个编辑器生命周期存在。

### 创建 GameInstance 代理

**步骤 1**：编写 Python 代理类

```python
# gameinstance.py
import ue

class GameInstanceProxy(object):
    def init(self):
        self.game_instance = self.uobject  # type: ue.GameInstance
        ue.LogWarning(f"GameInstance init! {self.game_instance}")

    def shutdown(self):
        ue.LogWarning("GameInstance shutdown!")

    def on_pre_world_tick(self, delta_seconds: float):
        pass

    def on_pre_actor_tick(self, delta_seconds: float):
        pass

    def on_post_actor_tick(self, delta_seconds: float):
        pass

    def on_tick(self, delta_seconds: float):
        pass
```

**步骤 2**：创建蓝图，派生自 `NePyGameInstance`

**步骤 3**：在蓝图属性面板设置：
- `PythonModule` → `gameinstance`
- `PythonClass` → `GameInstanceProxy`

**步骤 4**：在项目设置中将 GameInstance 设为该蓝图。

### GameInstance 回调 API 一览

| 方法 | 调用时机 |
|------|---------|
| `init()` | `UGameInstance::Init()` |
| `on_start()` | `UGameInstance::OnStart()`，比 `init` 稍晚 |
| `shutdown()` | `UGameInstance::Shutdown()` |
| `on_pre_world_tick(dt)` | World 开始 Tick |
| `on_pre_actor_tick(dt)` | 所有 Actor 之前 |
| `on_post_actor_tick(dt)` | 所有 Actor 之后 |
| `on_tick(dt)` | 渲染 Tick 之后 |

> 各 tick 回调的精确执行顺序参见 [`ticker-timer.md`](ticker-timer.md#引擎tick顺序)。

---

## 四、项目骨架模板

```
RawScripts/                  # 脚本根目录
├── nepyinit.py              # 入口（必须）
├── reloader.py              # 热重载引擎
├── reload_monitor.py        # 文件变更监听
├── gmcmds.py                # GM 指令
├── gameinstance.py          # GameInstance 代理
├── characters/
│   └── player_character.py  # @ue.uclass Character
├── weapons/
│   └── weapon_base.py       # @ue.uclass Weapon
├── gas/
│   ├── attr_set_base.py     # @ue.uclass AttributeSet
│   └── ga_fireball.py       # @ue.uclass GameplayAbility
└── utils/
    └── helpers.py           # 纯 Python 工具函数
```

### nepyinit.py 推荐 import 顺序

1. **编辑器工具** — `reload_monitor`, `gmcmds`（仅在 `ue.GIsEditor` 时）
2. **基础设施** — `gameinstance`
3. **基础 Subclassing 类** — `character`, `attribute_set`
4. **派生 Subclassing 类** — 具体武器、技能
5. **其他子系统**

**原因**：父类必须在子类之前 import，否则注册时找不到父类 UClass。

---

## 五、懒加载 import

循环依赖时，把 import 放到函数体内：

```python
def some_function(self):
    from weapons.weapon_base import WeaponBase
    if isinstance(self.current_weapon, WeaponBase):
        ...
```
