# UE API 调用严谨性

> **激活时机**：任何需要设置 UE 引擎节点属性、调用不熟悉的 C++ API 参数时。
> **来源**：UE 5.6 LayeredBoneBlend.BlendDepth=-1 导致 `1/(-1)` 负权重 → 全部 BlendPoses 静默失效（6小时排查）。

## 规则

1. **API 命名先查 `__init__.pyi`**。NEPY 导出的 C++ 函数保持 PascalCase（`ue.LoadClass()`），不要用 snake_case（`ue.load_class()`）。写完代码后自检所有 `ue.Xxx()` 调用。详见 [命名约定](codewiki/nepy/naming-convention.md)。
2. **参数语义不明 → 先查引擎源码**，不凭直觉猜测。路径: `D:\UE5\UE_5.6\Engine\Source\`，grep 定位函数实现后逐行确认。
3. **`-1` 不是万能值**。UE 源码中 `-1` 可能参与除法、被 Clamp 钳制、或代表 INDEX_NONE。不要假设它等于"无限/全部"。
4. **改一个值 → 验证 → 再改下一个**。不批量修改未经验证的参数。

## 危险值速查

| 值 | 风险 |
|----|------|
| `-1` | 除法分母为负 → 权重/比例反转；INDEX_NONE → "未找到" |
| `0` | 除法分母为零 → INF/NaN；枚举第一项含义不固定 |
| `0.0f` | 浮点零参与 `1/x` → 崩溃或特殊路径 |

## 验证流程

```
猜测参数值 → grep 引擎源码 → 确认语义 → 赋值 → compile → 预览验证
```
