# NEPY 概述与核心概念

> ⚠️ **命名警告**：NEPY 导出的 UE C++ 函数保持 **PascalCase**（如 `ue.LoadClass()`），
> **不要**使用 Python 风格的 snake_case（如 `ue.load_class()` ❌）。
> 常见错误对照见 [命名约定](naming-convention.md)。

## 简介

**NEPY** (NePythonBinding) 是网易自研的 Unreal Engine Python 脚本插件。它利用 UE 自带的类型反射数据，将 C++ 的 `UCLASS`/`USTRUCT`/`UFUNCTION`/`UPROPERTY`/`UENUM` **1:1** 导出至 Python，使开发者能用 Python 编写游戏逻辑。

## 核心设计理念

> **用蓝图做的任何事情，都可以用同样的方法在 Python 里实现。**

NEPY 推荐的最佳使用方式：

| 层级 | 用途 | 为什么 |
|------|------|--------|
| **蓝图** | 对象组装（添加 Component、挂接特效/音频、配置参数） | 可视化组装效率高 |
| **Python** | 玩法逻辑（游戏规则、技能系统、AI 行为） | 可读性好、可版本管理、可热更 |
| **C++** | 性能关键模块、底层基础设施 | 只在必要时用，编译慢 |

## 两种绑定机制

NEPY 提供两种 UE ↔ Python 交互方式：

### 1. 动态绑定（Dynamic Binding）
运行时自动将 UE 反射类型映射为 Python 对象。**无需任何配置**，只要有反射标记就可用。

```python
import ue
# 直接调用 UE API
actor = ue.NewObject(ue.Actor)
actor.SetActorLocation(ue.Vector(100, 200, 300))
```

### 2. Subclassing（静态类定义）
在 Python 中用特殊语法**定义能被 UE 识别的类型**，反其道而行之。详见 [`class-authoring.md`](class-authoring.md)。

```python
@ue.uclass()
class MyCharacter(ue.Character):
    Health = ue.uproperty(100.0)

    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        ue.LogWarning("Hello from Python!")
```

## 三种运行环境

NEPY 会在以下三种环境中加载 `nepyinit.py`，**必须区分处理**：

```python
def on_init():
    if ue.GIsEditor:
        # 编辑器环境（含 PIE）
        pass
    elif ue.IsRunningCommandlet():
        # Commandlet 环境（如打包时）
        pass
    else:
        # 纯游戏环境（打包后的 .exe）
        pass
```

## 脚本目录优先级

NEPY 按以下顺序查找脚本根目录，**第一个存在的目录会被使用**：

```
D:/
├── script              [1] 最高优先级
├── MyGame
    ├── RawScripts      [2] 次优先级（推荐）
    └── Content
        └── Scripts     [3] 最低优先级（自动开启 Redirect）
```

**推荐**：将脚本放在 `<ProjectDir>/RawScripts/`，既方便源码管理，又不触发 Redirect。

### 自定义脚本根目录

在 `DefaultGame.ini` 中配置：

```ini
[NEPY]
PythonScriptPath=MyScripts   ; 相对于 ProjectDir 的路径
NeedRedirect=False
```

## API 文档（类型桩）

NEPY 提供完整的 `.pyi` 文件用于 IDE 代码补全：

| 文件 | 位置 | 说明 |
|------|------|------|
| `__init__.pyi` | `NePythonBinding/Tools/pystubs/ue/` | 完整 UE 引擎 API（约 10MB） |
| `blueprint_doc.pyi` | 同上 | 项目蓝图生成的类 |
| `builtin_doc.pyi` | 同上 | NePy 内置封装类型 |

将其路径添加到 VSCode Python 的 `extraPaths` 即可获得代码提示。

## 日志输出

```python
print("普通输出")            # LogNePython 类别
ue.Log("信息")               # Log 级别
ue.LogWarning("警告")        # Warning 级别
ue.LogError("错误")          # Error 级别
```

## 快速对比：蓝图 vs Python

| 蓝图操作 | Python 等效 |
|---------|------------|
| Spawn Decal at Location | `ue.GameplayStatics.SpawnDecalAtLocation(...)` |
| Get Actor Location | `actor.GetActorLocation()` |
| Set Timer by Function Name | `timer_mgr.SetTimer(callback, 1.0)` |
| Bind Event | `component.OnComponentHit.Add(callback)` |
