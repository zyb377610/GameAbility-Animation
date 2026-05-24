# NEPY GM 指令框架

## 原理

NEPY 的 PythonConsole 只能执行单条 Python 语句。通过 `nepyinit.py` 的 `on_debug_input` 回调，可以**拦截输入并路由到自定义 GM 指令**，同时保留 Python 求值能力。

- 输入以 `@` 开头 → 执行 GM 指令
- 否则 → 执行普通 Python 语句

## 快速搭建

### 步骤 1：创建 gmcmds.py

在脚本根目录新增 `gmcmds.py`：

```python
# -*- encoding: utf-8 -*-
from ast import literal_eval

def handle_debug_input(cmd_str):
    if not cmd_str.startswith('@'):
        return False       # 不以 @ 开头 = 不处理，走 Python eval

    cmd_tokens = cmd_str[1:].split()
    cmd_name = cmd_tokens[0].strip().lower()

    for func_name, func in globals().items():
        if not callable(func):
            continue
        if getattr(func, '__module__', None) != __name__:
            continue
        if func_name.lower() != cmd_name:
            continue

        args = []
        for token in cmd_tokens[1:]:
            try:
                token = literal_eval(token)   # 尝试解析 Python 字面量
            except:
                pass                          # 解析失败则保留字符串
            args.append(token)
        func(*args)
        break
    else:
        print(f'cmd "{cmd_name}" not found!')
    return True                              # 已处理

def hello():
    print('hello, nepy!')
```

### 步骤 2：接入 on_debug_input

在 `nepyinit.py` 中添加：

```python
def on_debug_input(cmd_str):
    import gmcmds
    return gmcmds.handle_debug_input(cmd_str)
```

### 步骤 3：测试

在 PythonConsole 输入：

| 输入 | 效果 |
|------|------|
| `@hello` | 调用 `hello()` 函数 |
| `@hi` | 提示 `cmd "hi" not found!` |
| `print(1+1)` | 正常执行 Python 语句 |

## 带参数的 GM 指令

```python
def hello2(msg):
    print(f'hello from {msg}!')

# 输入: @hello2 world
# 输出: hello from world!

def spawn_npc(npc_id, count=1):
    print(f"Spawning {count} NPC of type {npc_id}")

# 输入: @spawn_npc "Orc" 5
```

参数用空格分隔，自动尝试 `literal_eval` 解析数字、字符串等。

## 常用 GM 指令示例

```python
import ue

def reload():
    """热重载所有脚本"""
    import reloader
    reloader.reload()

def actors():
    """列出当前关卡所有 Actor"""
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    for actor in ue.PyIterator(ue.Actor, world):
        print(actor.get_name())

def gc():
    """手动触发 GC"""
    ue.SystemLibrary.CollectGarbage()

def crash():
    """测试崩溃日志（需要开启 nepy.EnableCrashHandler=1）"""
    raise RuntimeError("Intentional crash!")
```

## 与其他系统的集成

GM 指令是调试入口，通常还会对接：
- **`reload`** → 触发 `reloader.reload()`（[热重载](hot-reload.md)）
- **`reload_monitor`** → 文件变更自动 reload（[热重载](hot-reload.md)）
- **Ticker/Timer 测试** → 启停 tick 回调（[Ticker与Timer](ticker-timer.md)）
- **GAS 测试** → 触发技能、设置属性（[GAS踩坑](gas-pitfalls.md)）
