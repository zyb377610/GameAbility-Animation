# NEPY 热重载机制

## 工作原理

NEPY 不提供内建 Reload 方案（因为 Python 自带的 `reload` 不靠谱）。官方提供的是 **func_code reloader**：保持旧模块/旧对象不变，仅替换函数的字节码。

## 步骤 1：接入 reloader.py

在脚本根目录创建 `reloader.py`（完整代码见官方文档），核心接口：

```python
import reloader

reloader.init_last_reload_time()     # 初始化基准时间
reloader.reload()                     # 全量 reload
reloader.reload(module_names)         # 只 reload 指定模块
reloader.reload(modified_only=True)   # 增量 reload（只更新修改过的文件）
```

在 `nepyinit.py` 中初始化：

```python
def on_init():
    if ue.GIsEditor:
        import reloader
        reloader.init_last_reload_time()
```

## 步骤 2：接入 GM 指令触发 reload

在 `gmcmds.py` 中添加：

```python
def reload():
    import reloader
    reloader.reload()
```

之后在 PythonConsole 输入 `@reload` 即可触发。

## 步骤 3：文件修改时自动 Reload

创建 `reload_monitor.py`（完整代码见官方文档），利用 Windows `ReadDirectoryChangesW` 监听 `.py` 文件变更：

在 `nepyinit.py` 中启用：

```python
def on_init():
    if ue.GIsEditor:
        import reload_monitor
        reload_monitor.start()
```

保存 `.py` 文件后 0.1 秒 debounce 自动触发 `reloader.reload()`。

## ⚠️ 热重载的限制

### func_code reloader 的局限

| 行为 | 说明 |
|------|------|
| 新增模块/类/方法 | ✅ 生效 |
| 删除旧的模块/类/方法 | ❌ 不会删除 |
| 修改函数实现（纯 Python） | ✅ 通过替换 `__code__` 生效 |
| 修改模块变量/类变量 | ❌ 不会更新 |
| 递归闭包函数 | 最大 5 层深度 |

### `@reload` 对 `@ue.uclass()` 的生效情况

| 改动类型 | `@reload` 生效？ | 建议 |
|----------|:---:|------|
| 纯 Python 函数（如 `gmcmds.py` 的命令） | ✅ | 放心用 |
| `@ue.uclass()` 类**新增方法** | ✅ | 可用 |
| `@ue.uclass()` 类**已有方法实现修改** | ❌ | 重启 PIE |
| `@ue.uclass()` 类的 `__init_default__` 修改 | ❌ | 重启 PIE |
| `@ue.uclass()` 新增属性/组件 | ❌ | 重启编辑器 |

### `isinstance` 可能失效

热重载后类的内存地址变了，`isinstance(obj, SomeClass)` 可能返回 `False`。

**替代方案**：用 `hasattr` 或 UE 的 `IsA()`：

```python
# ❌ 可能失效
if isinstance(weapon, RangedWeapon):
    ...

# ✅ 安全
if hasattr(weapon, '_is_reloading'):
    ...

# ✅ 更安全（UE 类型）
if weapon.IsA(RangedWeapon):
    ...
```

### 模块级可变状态

```python
# ⚠️ 热重载时重置为空
_active_instances: list = []
_cached_references: dict = {}
```

## 最佳实践

1. **修改 `@ue.uclass()` 类后**：关闭 PIE（Stop）→ 重新 Play，不要依赖 `@reload`
2. **发现 Reload 异常**：重启编辑器
3. **新 Subclassing 类**：首次注册需重启编辑器
4. **纯 Python 逻辑**：可以用 `@reload` 快速迭代
