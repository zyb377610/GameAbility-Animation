# NePy (NePythonBinding) 插件概述

## 简介

NePy 是一个为 Unreal Engine 5 提供 Python 脚本支持的插件，支持在 UE 编辑器内和运行时执行 Python 代码，并且支持热更新。

## 插件位置

插件根目录：`Plugins/NePythonBinding/`

## API 文档（类型桩）

NePy 提供了完整的 Python 类型桩（`.pyi`）文件，用于 IDE 代码补全和类型检查，同时也是最完整的 API 参考文档。

文档位置：`Plugins/NePythonBinding/Tools/pystubs/ue/`

包含以下文件：

| 文件 | 说明 |
|------|------|
| `__init__.pyi` | 完整的 UE 引擎 API（类、枚举、结构体、顶层函数），约 10MB |
| `blueprint_doc.pyi` | 项目中蓝图生成的类、结构体和枚举 |
| `builtin_doc.pyi` | NePy 内置封装类型（容器、委托、装饰器等） |
| `internal.pyi` | 内部辅助定义 |

## 基本使用

```python
import ue

# 获取资产
obj = ue.load_object(klass, '/Game/Path/To/Asset')

# 调用 UE 函数
actor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_selected_level_actors()[0]
actor.set_actor_location(ue.Vector(100, 200, 300), False, False)

# 子类化 UE 类
@ue.uclass()
class MyActor(ue.Actor):
    def receive_begin_play(self):
        ue.Log("Hello from Python!")
```

## 热更新

NePy 支持运行时热更新 Python 脚本，修改代码后无需重启编辑器即可生效。具体机制参考插件自身的 Python 运行时模块。
