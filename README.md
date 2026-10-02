# TA Snow — Snow Mathematics and Shader Kernels

This repository contains numerical source excerpts from the TA_Snow snow system for Unreal Engine 5.7.4. The published scope is compression and displacement mathematics, deterministic thickness construction, interpolation, display-height shaping, landing kernels, weather reference equations, fixed-wind response, and powder optics/noise functions.

## 中文简介

TA_Snow 的数学模型以二维顶面样本上的等效未压实雪厚、压实度和覆盖率表示积雪状态。求解先选择接触高度层，再对扫掠区域执行压实与横向质量转移；接收环使用面积加权归一化。Shader 摘录提供消光积分、相函数、三维插值噪声和双相密度平流。

本仓库仅发布数学代码及必要数据结构。Actor、场景深度捕获、GPU 回读、网格生成、渲染管线、任务调度、注册逻辑、资产、工程、工具及测试均不包含。目录布局沿用 TARibbon-Portfolio 的插件源码展示方式，但这里不是可安装的完整插件。

## Repository map

```text
Plugins/TA_Snow/
├── Source/Core/
│   ├── TASnowMathTypes.h      # Numerical state and grid quadrature
│   ├── TASnowSolver.cpp       # Swept compression and conservative rim transfer
│   ├── TASnowShape.cpp        # Seeded thickness and triangle interpolation
│   └── TASnowScalarMath.h     # Display, landing, weather and wind formulas
└── Shaders/Math/
    └── TASnowPowderMath.ush    # Optical integration and advected density detail
```

## Mathematical model / 数学模型

- **雪厚与压实**：可见厚度为 `max(0, baseline + massOffset) / (1 + 2 * compaction)`。压实改变几何厚度；横向推雪改变等效体积状态。
- **扫掠接触**：点到运动线段距离定义压力区域，宽压力核通过 SmoothStep 控制；压实与推雪速率采用指数响应。较大位置跳变不连接成长扫掠带。
- **表面连续性**：以接触样本为起点，四邻域局部高度差限制决定接收层，避免将不同平台上的样本作为同一质量接收面。平坦范围采用直接路径。
- **质量转移**：移出体积按网格面积及边角梯形权重累计，再按接收环权重归一化。跨块共享边界参与面积积分；没有有效接收环时不移出质量。守恒描述指离散体积公式，不代表质量/能量完整物理模型。
- **初始厚度**：固定种子生成二维 Perlin 场偏移；厚度限制在零与最大值之间。背景与精细样本使用相同反对角线三角插值。
- **显示轮廓**：小片采用指数饱和圆顶；普通区域采用边缘因子。显示高度不回写质量状态。
- **落地分摊**：有限支撑高斯核乘以边缘衰减项，按有效接收面积归一化体积。这里只包含核与归一化公式，不含落地检测或事件队列。
- **天气与风**：带符号厚度偏移和噪声保留率描述参考状态；固定风通过一阶指数响应及其时间积分产生连续位移。
- **雪雾光学**：Beer–Lambert 消光与 HG 相函数用于分段散射累积；三线性 value noise、双相平流交叉淡化和细节八度用于密度扰动。

## Source extraction / 源码摘录

Solver 与 Shape 保留 Toon 当前数学实现，仅调整本地 include。数学类型移除 UObject、网格、缓存和调度成员，只保留求解与插值所需的状态。ScalarMath 从顶面显示、落地、天气和风响应公式中提取函数接口。PowderMath 保留相关 HLSL 函数，移除场景资源、VS/PS 入口和绑定参数。

这些代码是数学审阅摘录，不是完整系统的行为或性能证明。调用端需满足有效网格分辨率、状态数组长度、非负时间、正响应时间、有效覆盖率和面积积分等约定，详见 [DEPENDENCIES.md](DEPENDENCIES.md)。

## Verification status

Publication scope and source extraction were inspected. No compilation, automated tests, ShaderCompileWorker, Editor/PIE execution, GPU capture or performance measurement was run for this publication. The reduced excerpts must be integrated and validated independently.

## License and attribution

See [LICENSE-SOURCE-AVAILABLE.txt](LICENSE-SOURCE-AVAILABLE.txt) and [ATTRIBUTION.md](ATTRIBUTION.md). Established numerical and optical methods are not claimed as original inventions.
