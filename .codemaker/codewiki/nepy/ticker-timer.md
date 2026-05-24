# NEPY Ticker 与 Timer

## Ticker（每帧回调）

NEPY 导出了引擎的 `FTicker`，可按帧间隔调用 Python 回调。

```python
import ue

def _tick(dt: float):
    print(f"tick: {dt}")

# 添加 Ticker，返回句柄
handle = ue.AddTicker(_tick)

# 移除 Ticker
ue.RemoveTicker(handle)
```

### 结合 GM 指令测试

```python
# gmcmds.py
ticker_handle = None

def tickertest():
    global ticker_handle
    if ticker_handle is None:
        ticker_handle = ue.AddTicker(lambda dt: print(f"tick: {dt}"))
        print("ticker started")
    else:
        ue.RemoveTicker(ticker_handle)
        ticker_handle = None
        print("ticker stopped")
```

## Timer（延迟调用）

通过 `TimerManager` 支持延迟和重复回调。

### 获取 TimerManager

```python
# 编辑器环境（PIE 中不可用）
timer_mgr = ue.GetEditorTimerManager()

# 运行时（从 GameInstance 获取）
game_instance: ue.GameInstance
timer_mgr = game_instance.GetTimerManager()
```

### 使用 Timer

```python
def _on_timer():
    print("Timer fired!")

# 延迟 1 秒后调用一次
timer_mgr.SetTimer(_on_timer, 1.0, False)

# 每 2 秒重复调用
timer_mgr.SetTimer(_on_timer, 2.0, True)
```

---

## 引擎 Tick 顺序（基于 UE5.0）

NEPY 中各个 tick 回调的执行顺序如下：

| 序号 | 阶段 | NEPY 相关回调 |
|------|------|-------------|
| 1 | **OnBeginFrame** | `FCoreDelegates::OnBeginFrame` |
| 2 | **计算 DeltaTime** | `ue.GetDeltaTime()` 可用 |
| 3 | **处理 UI 事件** | UserWidget.OnKeyDown / Subclassing `OnKeyDown` |
| 4 | **OnWorldTickStart** | GameInstanceProxy `on_pre_world_tick(dt)` |
| 5 | **OnWorldPreActorTick** | GameInstanceProxy `on_pre_actor_tick(dt)` |
| 6 | **Actor::Tick** | `ReceiveTick`（⚠️ 暂无法用 Subclassing 覆写） |
| 7 | **TimerManager::Tick** | `game_instance.GetTimerManager().SetTimer(...)` 在此触发 |
| 8 | **OnWorldPostActorTick** | GameInstanceProxy `on_post_actor_tick(dt)` |
| 9 | **UI Tick** | UserWidget.Tick / Subclassing `Tick` |
| 10 | **自增帧编号** | `ue.KismetSystemLibrary.GetFrameCount()` |
| 11 | **FTicker::Tick** | **`nepyinit.on_tick(dt)`** → **GameInstanceProxy `on_tick(dt)`** → **`ue.AddTicker` 注册的回调** |
| 12 | **OnEndFrame** | `FCoreDelegates::OnEndFrame` |

### 关键点

- **`nepyinit.on_tick`** 和 **`ue.AddTicker`** 的回调都在 **第 11 步** 触发，晚于所有 Actor::Tick
- **`GameInstanceProxy.*_tick`** 回调在 Actor Tick 前后（第 4/5/8 步）
- **TimerManager** 回调在 Actor Tick 之后（第 7 步）

## 在 Actor 中设置 Tick

```python
import ue

# 设置 Tick 分组
actor.SetTickGroup(ue.ETickingGroup.TG_PrePhysics)
```

> ⚠️ **目前无法用 Subclassing 覆写 `ReceiveTick`**：UE 对蓝图的优化会使 Python 的 `ReceiveTick` 不生效。解决思路是在 Python 中实现额外 Tick 方法，在蓝图的 Tick 节点里调用。

## 获取 DeltaTime

```python
import ue
dt = ue.GetDeltaTime()
```
