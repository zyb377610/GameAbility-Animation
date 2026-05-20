这个项目旨在研究UE5中技能的实现、动作混合、动画子图以及groundmotion的实现。
最终的结果应该是形成一个文档，包含以下内容：
1. 技能实现：详细介绍UE5中技能的实现方法，包括技能的设计、技能的触发机制、技能的效果实现等方面的内容。
2. 动作混合：介绍UE5中动作混合的概念和实现方法，包括动作混合的原理、动作混合的应用场景、动作混合的实现步骤等方面的内容。
3. 动画子图：介绍UE5中动画子图的概念和实现方法，包括动画子图的定义、动画子图的应用场景、动画子图的实现步骤等方面的内容。
4. Groundmotion实现：介绍UE5中groundmotion的概念和实现方法，包括groundmotion的定义、groundmotion的应用场景、groundmotion的实现步骤等方面的内容。
通过这个项目，我们希望能够深入了解UE5中技能的实现、动作混合、动画子图以及groundmotion的实现方法，并能够将这些知识应用到实际的游戏开发中去。

---

## 文档导航

| 模块 | 入口文档 |
|------|---------|
| 项目总览 | [00_项目总览.md](./00_项目总览.md) |
| 英雄技能系统 (GAS) | [01_GAS核心架构.md](./01_英雄技能系统/01_GAS核心架构.md) |
| 动作混合 | [01_动作混合概述.md](./02_动作混合/01_动作混合概述.md) |
| 动画子图 | [01_动画子图概述.md](./03_动画子图/01_动画子图概述.md) |
| GroundMotion | [01_GroundMotion概述.md](./04_GroundMotion/01_GroundMotion概述.md) |
| 技能动画表现 | [01_技能动画表现概述.md](./05_技能动画表现/01_技能动画表现概述.md) |
| 工程实践 | [01_漫威争锋观察记录.md](./06_工程实践/01_漫威争锋观察记录.md) |

## 目录结构

```
GameAbility&Animation/
├── README.md
├── 00_项目总览.md                        # 项目总览、阶段规划、技术栈图
│
├── 01_英雄技能系统/                       # GAS框架拆解
│   └── 01_GAS核心架构.md
│
├── 02_动作混合/                           # BlendSpace、LayeredBlendPerBone、IK
│   └── 01_动作混合概述.md
│
├── 03_动画子图/                           # LinkedAnimGraph、AnimLayer、SubAnimInstance
│   └── 01_动画子图概述.md
│
├── 04_GroundMotion/                       # DistanceMatching、MotionWarping、StrideWarping
│   └── 01_GroundMotion概述.md
│
├── 05_技能动画表现/                        # Montage、AnimNotify、GameplayTag驱动
│   └── 01_技能动画表现概述.md
│
└── 06_工程实践/                           # 实际体验观察记录
    └── 01_漫威争锋观察记录.md
```