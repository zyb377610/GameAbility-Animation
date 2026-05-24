# NEPY 对象生命周期管理

## 核心问题

UE 使用**垃圾回收 (GC)** 管理 C++/蓝图对象，Python 使用**引用计数 + GC**。两者相互独立，跨虚拟机传递对象时需注意生命周期。

---

## 一、结构体与枚举：值拷贝

以 `USTRUCT` 标注的结构体（如 `ue.Vector`、`ue.Rotator`）和 `UENUM` 枚举，在传递给 Python 时**以值拷贝方式传递**。

**这意味着无需关注其生命周期，但修改 Python 中的变量不会影响 UE 端**：

```python
# ❌ 错误：修改不会生效！
actor.GetActorLocation().X = 10

# ✅ 正确：取出来 → 修改 → 设回去
pos = actor.GetActorLocation()
pos.X = 10
actor.SetActorLocation(pos)
```

---

## 二、UObject：Python 引用 = 弱引用

所有派生自 `UObject` 的对象（Actor、Component、Asset 等），在 Python 中的引用都是**弱引用**。对象的真实生命周期由 UE GC 控制。

```python
# Python 中的 actor 只是一个弱引用
actor = world.SpawnActor(...)
# 如果 UE 端销毁了该 Actor，Python 中再访问会报错
```

### 检查对象是否有效

```python
if not actor.IsValid():
    print("Actor has been destroyed")
    return
```

访问已释放的 UObject 会报：
```
underlying UObject is invalid
```

### UObject 何时被释放

| 场景 | 说明 |
|------|------|
| UE GC 回收 | 对象没有被 GC Root 引用时，可能被回收（不一定是每帧触发） |
| 关卡卸载 | 关卡上所有 Actor 被销毁 |
| PIE 结束 | 游戏中所有对象被销毁 |
| `DestroyActor()` | Python/蓝图主动销毁 |
| 其他引擎内部逻辑 | 某些子系统清理 |

---

## 三、OwnByPython — 阻止 GC

当 Python 持有 UObject 引用但 UE 端无其他引用时，对象可能被 GC 回收。例如：
- 用 `ue.LoadClass` 加载蓝图类后稍后才使用
- 将 Actor 暂时从场景移除，稍后加回

**解决方案：调用 `Object.OwnByPython()`**

```python
# 加载蓝图类
bullet_class = ue.LoadClass('/Game/MyBulletBP.MyBulletBP_C')
# ⚠️ 如果不立即使用且不调 OwnByPython，可能被 GC 回收
bullet_class.OwnByPython()

# 稍后安全使用
world.SpawnActor(bullet_class, location, rotation)
```

### 恢复 GC 管理

```python
# 方式1：主动调用
obj.DisownByPython()

# 方式2：Python 层不再持有引用时自动恢复
```

> ⚠️ `OwnByPython` **只能阻止垃圾回收**，不能阻止关卡卸载、PIE 结束、`DestroyActor()` 等情况。

---

## 四、完整示例

```python
@ue.uclass()
class MyRifle(ue.Actor):
    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        # 加载蓝图类，添加 OwnByPython 防止 GC
        self.bullet_class = ue.LoadClass('/Game/MyBulletBP.MyBulletBP_C')
        self.bullet_class.OwnByPython()

    def fire(self):
        # 检查子弹类是否仍有效
        if not self.bullet_class.IsValid():
            ue.LogWarning("Bullet class is invalid!")
            return

        mesh = self.RifleMesh
        spawn_location = mesh.GetSocketLocation('Muzzle')
        spawn_rotation = mesh.GetSocketRotation('Muzzle')
        self.GetWorld().SpawnActor(self.bullet_class, spawn_location, spawn_rotation)
```

## 五、快速参考

| 类型 | Python 中的引用 | 生命周期 | 注意事项 |
|------|:---:|------|------|
| UObject (Actor, Component, Asset) | 弱引用 | UE GC 管理 | 用 `IsValid()` 检查；加载后 `OwnByPython()` |
| UStruct (Vector, Rotator) | 值拷贝 | Python 管理 | 修改不影响 UE 端；需取→改→设 |
| UEnum | 值拷贝 | Python 管理 | 无特殊问题 |
| Python 原生对象 | 强引用 | Python GC | 正常引用计数 |
