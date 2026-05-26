# Step 3.1 Distance Matching — 关键概念记录

> 记录于 Step 3.1b 完成后，在开始 3.1c AnimBP 连线之前。
> 内容源自实现过程中遇到的疑问和解答。

---

## 一、滑步的本质

**滑步（Foot Sliding）= 动画根骨骼速度 ≠ 胶囊体速度。**

脚相对于地面的速度 = 胶囊体世界速度 + 动画根骨骼局部速度。二者大小相等、方向相反时，脚相对地面速度为 0，不滑步。

| 情况 | 现象 | 类比 |
|------|------|------|
| 动画位移 > 实际位移 | 脚向后空转 | 冰面上走路 |
| 实际位移 > 动画位移 | 脚粘地，身体被拖 | 旱冰鞋被推 |

---

## 二、Distance Matching 的解决原理

### 为什么改播放速度能消除滑步

假设胶囊体以 300cm/s 减速，动画根骨默认速度 400cm/s：

```
改前: 300 + (-400) = -100cm/s → 脚向后滑
改后: 300 + (-400×0.75) = 0cm/s → 脚不滑！
```

**改 PlayRate = 改变动画根骨骼移动速率，使之与胶囊体速率匹配。**

### PlayRate 不是恒定值，每帧动态计算

```
每帧逻辑:
  差值 = DistanceToMatch(角色实际位移) - CurveValue(动画当前位移)
  PlayRate = 差值 / DeltaTime
  
  差距大 → PlayRate大 → 加速追
  差距小 → PlayRate小 → 减速等
  差距零 → PlayRate≈0 → 暂停
```

PlayRate 像弹簧，每帧根据"曲线值和实际位移的差值"自我调节。

---

## 三、以谁为准？移动 vs 技能

| 场景 | 谁说了算 | UE 机制 |
|------|---------|---------|
| 日常移动 (Locomotion) | 实际位移为王 | Distance Matching — 动画适应物理 |
| 技能动作 (Abilities) | 动画位移为王 | Root Motion — 物理跟随动画 |

---

## 四、动画不播完是否正常

**正常。Distance Matching 把动画从"时间轴播放器"变成了"距离查询表"。**

- 角色只经历了 80cm 减速 → 动画播到 80cm 对应的帧 → 脚站稳了，后面不需要
- 角色经历了 200cm 减速（全速急停）→ 动画最大只有 138cm → 需要 Stride Warping 拉伸（Step 3.3）

动画完整性不重要，位移匹配才重要。

---

## 五、序列评估器 vs 序列播放器

| | Sequence Player | Sequence Evaluator |
|--|----------------|-------------------|
| 时间推进 | 自动（PlayRate） | 需显式控制 |
| 适用场景 | 固定速度播放 | Distance Matching（时间由外部驱动） |
| 关键 API | — | SetExplicitTime / AdvanceTime |

---

## 六、制动距离（松手后为什么还有位移）

松手 ≠ 立刻静止。`CharacterMovementComponent` 的 `GroundFriction` 和 `BrakingDecelerationWalking` 决定了制动过程。全速奔跑松手后，角色可能因惯性滑行 80~138cm。这股制动距离就是 `DistanceToMatch` 的数据源。
