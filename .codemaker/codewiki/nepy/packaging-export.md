# NEPY 打包与扩展导出

## 一、打包发布

### 三种打包方式

| 方式 | 说明 | 适用场景 |
|------|------|---------|
| **PAK 打包** | 脚本编译/加密后打入 PAK | 正式发布（推荐） |
| **离散文件** | `.py` 文件原样放置 | 快速测试、开发调试 |
| **补丁** | 增量更新脚本 | 线上热修复 |

### PAK 打包

脚本放在 `Content/Scripts/` 目录下，后续编译为 `.ues` 二进制并打入 PAK。需要开启 Redirect 功能（`Content/Scripts` 目录自动开启）。

```ini
[NEPY]
NeedRedirect=True
```

### 离散文件打包

脚本保留为 `.py` 文件，与游戏可执行文件一同分发。

### 符号链接技巧

可将 `Content/Scripts/` 通过符号链接指向 `RawScripts/`，开发时使用 `RawScripts`，打包时使用 `Content/Scripts` 下的加密脚本。

---

## 二、导出更多 C++ 接口

NEPY 默认只导出引擎核心部分。可通过 `ExportConfig.py` 导出额外的引擎模块、引擎插件或项目插件。

### ExportConfig.py 位置

```
NePythonBinding/Tools/BindingGenerator/ExportConfig.py
```

### 核心配置：ExportModules

```python
ExportModules = [
    {
        'module': 'niagara',          # 自定义模块名，影响生成路径
        'gen_path': EngineBindingGeneratePath,  # 代码生成目录
        'packages': [
            {
                'name': '/Script/Niagara',     # UE 模块名
                'source_dir': os.path.join(EnginePath, 'Plugins', 'FX', 'Niagara', 'Source', 'Niagara'),
            }
        ]
    }
]
```

| 配置项 | 说明 |
|--------|------|
| `module` | 自定义名字，代码生成到 `NePy/Auto/<module>/` |
| `gen_path` | 生成目录（引擎用 `EngineBindingGeneratePath`，游戏用 `GameBindingGeneratePath`） |
| `packages/name` | `/Script` + `.Build.cs` 的文件名 |
| `packages/source_dir` | 模块源码目录 |

### 配置编译依赖

在 `NePythonBinding.Build.cs` 中添加：

```cs
PrivateDependencyModuleNames.AddRange(new string[] {
    "Niagara",    // 新增
});
```

### 配置插件依赖（仅插件）

在 `NePythonBinding.uplugin` 中添加：

```json
"Plugins": [
    { "Name": "Niagara", "Enabled": true }
]
```

### 运行导出

```powershell
python BindingGenerator.py -p        # 导出
python BindingGenerator.py -p -c     # 清理旧文件后导出
```

### 排除项 (Exclude)

```python
{
    'exclude': {
        'classes': {
            'NiagaraDataInterfaceGrid3DCollection': True,  # 排除单个类
            re.compile(r'^MaterialExpression'): True,      # 正则批量排除
        },
        'structs': {
            'Matrix': True,
        }
    }
}
```

可排除单个类中的属性、方法，或为类添加 `__dict__` 支持。

### 导出失败排查

| 问题 | 原因 | 检查点 |
|------|------|--------|
| 类型未导出 | 缺少反射标记 | 加 `UCLASS`/`USTRUCT`/`UENUM` |
| 类不可见 | 缺少可见性标记 | 加 Module API Specifier 或 `MinimalAPI` + `BlueprintType` |
| 方法未导出 | 缺少 `BlueprintCallable` | 加 `BlueprintCallable` Function Specifier |
| 编译错误 | include 路径不对 | 确认 `source_dir` 正确 |
| 类型导出但编译失败 | 缺少依赖 | 添加 `.Build.cs` 和 `.uplugin` 依赖 |

### 启用调试日志

```powershell
python BindingGenerator.py -p -l 2    # -l 2 打开调试日志
```

---

## 三、游戏模块导出

游戏自身的 C++ 模块需要通过不同方式导出：

```python
{
    'module': 'my_game',
    'gen_path': GameBindingGeneratePath,    # 注意：用 Game 路径
    'packages': [
        {
            'name': '/Script/MyGame',
            'source_dir': os.path.join(GamePath, 'Source', 'MyGame'),
        }
    ]
}
```

更多配置项见 `ExportConfig.py` 源码注释。

---

## 四、快速参考

| 操作 | 命令 |
|------|------|
| 导出引擎接口 | `python BindingGenerator.py -p` |
| 清理+导出 | `python BindingGenerator.py -p -c` |
| 调试模式导出 | `python BindingGenerator.py -p -l 2` |
| 生成 pystub 文件 | ExportConfig.py 中配置 `PythonStubDir` |
