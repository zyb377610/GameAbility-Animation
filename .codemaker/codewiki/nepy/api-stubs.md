# NePy API Stubs 结构指南

## 文件位置

所有类型桩文件位于：`Plugins/NePythonBinding/Tools/pystubs/ue/`

## `__init__.pyi` — 主 API 文件

这是核心文件，约 10MB，包含完整的 UE 引擎 Python API。按以下分区分段：

| 分区 | 行号范围 | 说明 |
|------|---------|------|
| `# region enums` | 第 6 行开始 | 所有 UE 枚举类型，每个枚举继承自 `EnumBase`，有 docstring 说明 |
| `# region structs` | 第 39541 行开始 | 所有 UE 结构体（FVector、FRotator 等），包含属性声明和方法签名 |
| `# region classes` | 第 137389 行开始 | 所有 UE 类（Actor、Component 等），包含属性、方法和继承关系 |
| `# region top module` | 第 325201 行开始 | 顶层模块函数（如 `load_object`、`find_object` 等全局函数） |
| `# region builtin` | 第 325602 行开始 | 重新导出内置类型 |
| `# region blueprint` | 第 325606 行开始 | 重新导出自定义蓝图类型 |

### 搜索 API

使用 `grep_search` 在 `__init__.pyi` 中搜索：

```
# 搜索类定义
grep_search: "class ClassName\b" 在 __init__.pyi

# 搜索枚举
grep_search: "class EnumName\(EnumBase\)" 在 __init__.pyi

# 搜索结构体
grep_search: "class StructName\b" 在 structs 区域之后
```

## `blueprint_doc.pyi` — 蓝图类型

| 分区 | 行号 | 说明 |
|------|------|------|
| `# region Blueprint Classes` | 第 14 行 | 项目中所有蓝图生成的类 |
| `# region Blueprint Structs` | 第 2716 行 | 蓝图结构体 |
| `# region Blueprint Enums` | 第 2730 行 | 蓝图枚举 |

从 `__init__.pyi` 和 `builtin_doc.pyi` 导入基础类型，然后定义项目特定的蓝图类。

## `builtin_doc.pyi` — 内置类型

NePy 提供的内置封装类型：

- `ArrayWrapper[VT]` — UE TArray 的 Python 封装，支持 len/getitem/setitem 等 Python 标准操作
- `FixedArrayWrapper[VT]` — 固定大小数组封装
- `MapWrapper[KT, VT]` — UE TMap 封装
- `SetWrapper[KT]` — UE TSet 封装
- `DynamicDelegateWrapper` — 动态单播委托
- `DynamicMulticastDelegateWrapper` — 动态多播委托
- `TSoftObjectPtr` / `TSoftClassPtr` / `TWeakObjectPtr` — 智能指针封装
- `SoftPtr` — 软引用基类
- `FieldPath` — 属性路径
- `EnumBase` — 枚举基类
- `Name` / `Text` — UE FName / FText 类型别名（实际为 str）
- `TSubclassOf` — 类引用类型
- `uclass()` / `ustruct()` / `uenum()` — 子类化装饰器

## `internal.pyi` — 内部类型

小型文件，定义内部辅助类：`ValueDef`、`PropertyDef`、`FunctionDef`、`ComponentDef`、`DelegateDef`、`ParamDef`、`ObjectRefDef` 以及各类装饰器定义。
