# NEPY 调试与日志排查

## 一、Python 调用堆栈

### 获取当前堆栈

```python
import ue

# 获取当前 Python 调用堆栈
call_info = ue.GetCurrentCallInfo()
ue.LogDisplay(call_info)
```

输出包含每帧的：文件名、行号、局部变量、闭包变量、引用计数。

### 获取异常堆栈

```python
try:
    risky_operation()
except Exception as e:
    exception_info = ue.GetTracebackCallInfo(e.__traceback__)
    ue.LogDisplay(exception_info)
```

### VS 断点时查看堆栈

在 Visual Studio 立即窗口输入：

```cpp
NePyCallInfo::PrintCurrentCallInfo()
```

---

## 二、崩溃时自动输出 Python 堆栈

设置 CVAR `nepy.EnableCrashHandler=1`：

```ini
# DefaultEngine.ini
[ConsoleVariables]
nepy.EnableCrashHandler=1
```

或在控制台执行：
```
nepy.EnableCrashHandler 1
```

崩溃时自动输出 Python 调用堆栈到日志文件。

---

## 三、Subclassing 详细日志

设置 CVAR `nepy.subclassing.log`：

```
nepy.subclassing.log 1   # 基本日志
nepy.subclassing.log 2   # 详细日志
```

> ⚠️ CVAR 必须在 NEPY 初始化前设置才生效。

### 关键日志搜索词

| 关键词 | 含义 |
|--------|------|
| `Finalizing Python Generated Class` | 类生成完成 |
| `Finalizing Python Generated Struct` | 结构体生成完成 |
| `Generating Descriptors for Class` | 动态绑定信息 |
| `Generating Descriptors for Struct` | 结构体动态绑定 |
| `Added Function` / `Added Property` | 新增方法/属性详情 |

---

## 四、PyCharm 远程调试

NEPY 支持 attach PyCharm 的 Debugger 到 UE 进程。

### 步骤

1. 在 PyCharm 中配置 **Python Debug Server**（端口默认 5678）
2. 确保 `debuglib` 已放入脚本根目录
3. 在需要打断点的 Python 代码前加入：

```python
import pydevd_pycharm
pydevd_pycharm.settrace('localhost', port=5678, stdoutToServer=True, stderrToServer=True)
```

4. 先在 PyCharm 中启动 Debug Server，然后运行 UE
5. 执行到 `settrace` 时会自动断住

---

## 五、关键日志输出

### 日志 API

```python
print("普通输出")            # LogNePython
ue.Log("信息级别")            # Log 级别
ue.LogWarning("警告级别")     # Warning 级别
ue.LogError("错误级别")       # Error 级别
ue.LogDisplay("显示级别")     # Display 级别
```

### 查看日志位置

- **编辑器**：窗口 → Developer Tools → Output Log
- **文件**：`<Project>/Saved/Logs/<Project>.log`

---

## 六、常见异常排查

| 异常信息 | 原因 | 解决 |
|---------|------|------|
| `underlying UObject is invalid` | UObject 已被 GC 或销毁 | 访问前调用 `IsValid()` |
| `cmd "xxx" not found!` | GM 指令不存在 | 检查 `gmcmds.py` 中的函数名 |
| `TypeError: ...` | Python 参数类型不匹配 | 检查 ufunction 的 params 声明 |
| 类注册后蓝图找不到 | import 链断裂或循环依赖 | 检查 `nepyinit.py` 的 import 顺序 |
| `InitStats` 无效果 | AttributeSet 不是 `@ue.uclass()` | 加上 `@ue.uclass()` 装饰器 |

---

## 七、性能分析 (Python Profile)

NEPY 支持使用 Python 内置的 cProfile 或 PyCharm 的 Python Profiler：

```python
import cProfile

def profile_my_code():
    cProfile.run('my_heavy_function()', sort='cumtime')
```

在 PyCharm 中使用 **Python Profiler** attach 到 UE 进程即可进行性能分析。
