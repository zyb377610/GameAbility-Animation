# NEPY API 命名约定（必读！）

> ⚠️ **最常见的低级错误**：用 Python 的 snake_case 调用 UE 函数。

## 核心规则：UE 反射出的函数名保持原样

NEPY 通过反射将 C++ API 1:1 导出到 Python，**函数名保持 C++ 的 PascalCase**。

## ✅ 正确 vs ❌ 错误速查表

| ❌ 错误 (Python snake_case) | ✅ 正确 (C++ PascalCase) |
|---------------------------|------------------------|
| `ue.load_class(path)` | `ue.LoadClass(path)` |
| `ue.load_object(type, path)` | `ue.LoadObject(type, path)` |
| `ue.find_object(path)` | `ue.FindObject(path)` |
| `ue.find_class(name)` | `ue.FindClass(name)` |
| `ue.new_object(cls)` | `ue.NewObject(cls)` |
| `ue.async_load_class(path)` | `ue.AsyncLoadClass(path)` |
| `ue.async_load_object(type, path)` | `ue.AsyncLoadObject(type, path)` |
| `ue.log(message)` | `ue.Log(message)` |
| `ue.log_warning(message)` | `ue.LogWarning(message)` |
| `ue.log_error(message)` | `ue.LogError(message)` |
| `ue.get_world()` | `self.GetWorld()` |
| `ue.get_actor_location()` | `actor.GetActorLocation()` |
| `ue.set_actor_location(v)` | `actor.SetActorLocation(v)` |
| `ue.gameplay_statics.xxx()` | `ue.GameplayStatics.Xxx()` |
| `ue.py_iterator(cls, world)` | `ue.PyIterator(cls, world)` |
| `ue.py_util.xxx()` | `ue.PyUtil.xxx()` |

## 例外（真正的 snake_case API）

以下 API 本身就是 snake_case，不需要转换：

- `ue.uclass()` / `ue.ustruct()` / `ue.uenum()`
- `ue.uproperty()` / `ue.ucomponent()` / `ue.ufunction()` / `ue.udelegate()`
- `ue.uvalue()` / `ue.uparam()` / `ue.uref()`
- `ue.get_editor_subsystem()`
- `ue.register_debug_channel()`

## 自检方法

写完代码后扫描所有 `ue.Xxx()` 调用，自问：
> 这个函数名是 C++ 里就存在的 UE 反射函数吗？→ 用 PascalCase
> 这个函数名是 NEPY 专门为 Python 提供的吗？→ 可能是 snake_case

如果不确定，到 `Plugins/NePythonBinding/Tools/pystubs/ue/__init__.pyi` 中搜索确认。
