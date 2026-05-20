# 04 - GroundMotion概述

> 参照对象：漫威争锋 (Marvel Rivals)
> UE5版本：5.3+

---

## 一、GroundMotion核心概念

GroundMotion 是指角色运动动画与地面之间匹配的技术集合。UE5通过 Distance Matching、Motion Warping、Stride Warping 等机制确保动画播放与实际位移精确对齐，消除"滑步"等现象。

### 核心组件

| 组件 | 作用 | 漫威争锋对应 |
|------|------|-------------|
| `Distance Matching` | 动画播放进度与角色位移距离对齐 | 起步/停步动画完美对应实际移动距离 |
| `Motion Warping` | 在动画播放中动态调整角色位置和旋转 | 近战突进锁定目标位置 |
| `Stride Warping` | 动态调整步幅拉伸 | 不同移速下脚步与地面贴合 |
| `Orientation Warping` | 动态调整身体朝向 | 转身时身体自然旋转 |
| `Slope Warping` | 根据地面坡度调整IK | 斜坡行走时脚步贴合 |
| `Root Motion` | 动画驱动角色位移 | 技能位移（翻滚、冲刺、摆荡） |

---

## 二、待拆解点

1. [ ] Distance Matching实现原理与Anim Curves
2. [ ] MotionWarpingComponent 配置与WarpTarget设置
3. [ ] MotionWarping AnimNotifyState 标记Warp窗口
4. [ ] Stride Warping的参数与插件配置
5. [ ] Orientation Warping 与 AimOffset 的区别与配合
6. [ ] Slope Warping 的IK方案
7. [ ] Root Motion vs 代码驱动位移的选择场景

---

## 三、漫威争锋观察要点

- **蜘蛛侠摆荡**：Motion Warping + 物理绳索的混合实现
- **黑豹/铁拳冲刺**：MotionWarping锁定目标，动画与位移精确对齐
- **起步/停步**：观察是否有滑步现象（推断Distance Matching是否到位）
- **斜坡表现**：斜坡移动时脚步是否有浮空或穿插
- **浩克跳跃**：大距离跳跃的Root Motion处理

---

## 备注

> **实现建议**：优先实现Distance Matching和MotionWarping，这是GroundMotion中最核心的两个机制。先做起步停步，再做突进技能。
