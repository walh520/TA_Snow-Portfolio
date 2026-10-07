# TA Snow

积雪数学与 Shader 核心节选

> 公开仓库仅数学/Shader 节选；本地有 Actor、表面捕获与网格链路，两者均未在本轮构建或运行验收。

[GitHub 仓库](https://github.com/walh520/TA_Snow-Portfolio)

## 简介与公开范围

TA_Snow 积雪系统的数值实现节选，面向 UE 5.7.4 的数学审阅。包含压实与位移、确定性厚度、插值、显示高度、落地核、天气参考方程、固定风响应，以及粉雪光学和噪声函数。

只发布数学代码和必要数据结构。Actor、深度捕获、GPU 回读、网格生成、渲染管线、调度、注册、资产、工程、工具与测试均不包含；不是可安装的完整插件。

## 本地工程与公开快照

当前本地 `TA_Snow` 是含 Runtime 模块、WeatherActor、表面捕获、动态网格和 FX 的项目插件；GitHub 仓库仍只是从该系统抽出的数学/Shader 节选，不能将二者的交付范围混写。

公开 Solver/Shape 与本地实现除首行 include 适配外相同；粉雪节选中的 8 个函数体与本地 TASnowFX.usf 在忽略空白/注释后相同。标量公式另外沿运行时调用点核对。这个结果说明节选来源对应，不能代替编译、物理正确性或视觉验收。

## 演示

[雪地与沙地作品演示](https://www.bilibili.com/video/BV1v8H76wEix/) · [作品总集](https://www.bilibili.com/video/BV1MVak6jEPv/)

视频展示完整作品，可能包含沙地、场景交互与当前仓库未公开的系统。此处只对应 TA_Snow 数学/Shader 节选，不能据此承诺复现完整雪沙效果。

## 实现与贡献

[ATTRIBUTION.md](ATTRIBUTION.md) 说明代码节选自 NiTong 的 Toon 项目 TA_Snow 数学实现。项目工作集中在状态定义、求解与插值实现、显示/落地/天气/风公式接口及 Shader 数学函数抽取。压实、面积积分、指数响应、噪声、Beer–Lambert 与 Henyey–Greenstein 等已有方法不作为个人发明。雪雾积分与双相密度平流还记录了同项目 Light Beam / Realtime Fog 来源。

## 核心功能

- 以等效未压实雪厚、压实度、覆盖率表达二维顶面样本。
- 扫掠接触先选择高度连通层，再压实并执行横向体积转移。
- 接收环按网格面积与边角权重归一化；没有有效接收环时不移出质量。
- 固定种子厚度与反对角线三角插值；显示高度不回写质量状态。
- 有限支撑落地核、天气参考式与固定风指数响应。
- Beer–Lambert 消光、HG 相函数、value noise 与双相密度平流。

本地工程分工：

- CPU：维护雪层/压实状态，选择连通表面与接触，执行体积再分配、天气更新、落雪沉积以及 ProceduralMesh 更新。
- GPU → CPU：TopSurface 通过 SceneCapture 和异步 GPU texture readback 建立可见顶面的 CPU 表面快照；所以不能把整个积雪系统说成无回读的纯 GPU 求解。
- GPU 表现：粉雪 FX 通过 Vertex/Pixel Shader 与 Raster Pass 求值、积分光学；不是持久三维流体模拟。局部“无逐粒子回读”描述仅对应这条 FX 路径。
- 本地支持脱离雪块的 CPU 重力/阻尼推进与落点沉积，但不等于完整刚体雪崩。
- GPUAgents 的 Snowfall 是另一条空中雪片路径，不能与地面积雪求解合并宣传成同一个 GPU 系统。

## 方案与取舍

| 选择 | 目的与边界 |
| --- | --- |
| 二维顶面样本表示 | 适合局部高度与压实研究，不是完整三维雪体。 |
| 面积加权接收环 | 约束离散体积转移；“守恒”仅针对该离散公式，不是完整质量/能量物理模型。 |
| 显示高度与质量状态分离 | 可调整轮廓且保持求解状态语义；视觉造型不反向修改质量。 |
| 发布纯数学节选 | 便于阅读核心公式；调用方须补齐编译入口、资源和运行集成。 |

数学对应关系：

- 雪量状态使用等效未压实厚度；可见高度约为 `max(0, Baseline + MassOffset) / (1 + 2 × Compaction)`。压实改变几何高度，不能把显示高度直接当雪体积。
- 接触外推按接收环权重、节点面积权重和单元面积归一化；只有有效接收环才移出雪量。“守恒”指该离散体积转移，不是完整热力学或三维流动。
- 落点核 `exp(-4.5u²) × (1-u²)²` 只在有效半径内使用；沉积除以同一连通表面上的加权面积，不能以简单权重和替代面积积分。
- 固定目标风速的一阶响应采用指数解析式；它不等于求解随时间变化的真实湍流。
- Shader 的 Beer–Lambert 段积分显式把 cm 转成 m；HG、value noise 和双相位细节控制是光学/表现近似。

本地 TopSurface 是单一可见顶面近似，不能表达多层洞穴/全三维雪体。队列和时间预算属于软预算，不能据此给出帧时上限。

## 代码阅读入口

1. [TASnowMathTypes.h](Plugins/TA_Snow/Source/Core/TASnowMathTypes.h)：状态、网格与面积权重。
2. [TASnowSolver.cpp](Plugins/TA_Snow/Source/Core/TASnowSolver.cpp)：扫掠压实与接收环转移。
3. [TASnowShape.cpp](Plugins/TA_Snow/Source/Core/TASnowShape.cpp)：种子厚度和三角插值。
4. [TASnowScalarMath.h](Plugins/TA_Snow/Source/Core/TASnowScalarMath.h)：显示、落地、天气和风公式。
5. [TASnowPowderMath.ush](Plugins/TA_Snow/Shaders/Math/TASnowPowderMath.ush)：光学积分与密度细节。

对应本地完整系统（未作为当前公开包交付）：`Plugins/TA_Snow/Source/TASnowRuntime/Private/` 下的 `TASnowWeatherActor.cpp`（状态/沉积/网格）、`TASnowTopSurface.cpp`（捕获/回读）、`TASnowMovingCaps.cpp`（脱离/落地）、`TASnowFX.cpp`（渲染调度），以及 `Shaders/Private/TASnowFX.usf`（光学/粒子表现）。

## 验证与性能

原发布只检查了源码抽取与发布范围。未执行编译、自动化测试、ShaderCompileWorker、Editor/PIE、GPU Capture 或性能测量。本次整理也没有重新验证节选。

数学代码可阅不等于完整系统行为或性能证明；集成后必须独立验证。这里没有完整插件的 FPS、精度或稳定性结论。

本地状态文档同样保留未编译、未自动化/Editor/PIE/性能验收的说明。旧历史 Evidence 不能验证后来加入的捕获、回读、天气或水面改动；本轮没有产生新运行结果。

## 依赖与运行方式

C++ 使用 UE 5.7.4 Core 数学、容器与类型，PerlinNoise2D 调用引擎 API；HLSL 仅包含函数，没有 Engine Shader、场景纹理、资源声明或编译入口。需由集成方提供调用与编译环境，不能直接“启用插件运行”。

[DEPENDENCIES.md](DEPENDENCIES.md) 中的调用约定须保留：

- Resolution > 0，Cells 长度为 `(Resolution + 1)^2`；接触块采用一致采样间距和共享世界网格键，CellPosition 只适用于规则网格。
- 长度 cm，时间 s，等效体积 cm³，消光系数 1/m；Shader 光学积分涉及 cm 到 m 换算。
- 风响应时间 > 0、Dt ≥ 0；覆盖率、显示参数和噪声尺度由调用方验证。
- 分摊体积采用同一有效接收面的面积加权分母，不用任意纹理权重和代替面积积分。

本地 Runtime 另依赖 Landscape、ProceduralMeshComponent、TAWorldInteractionRuntime、Renderer/RenderCore/RHI，并使用 Renderer 私有头；描述文件声明 TA_WorldInteraction。它没有对 TA_GPUAgents 的反向硬依赖。可选水面检测通过反射接口，不能写成必须链接 TA_Water。以上是本地工程依赖，不能替代公开节选自身的调用契约。

## 限制与来源许可

未包含 Actor、renderer、插件模块描述、scene capture、SceneWind 或 TA_ToonVolumetricLighting 的运行集成。大位移跳变、接触层选择和离散守恒均应按源码约定解读；摘录不覆盖完整工程的限幅、注册和预算策略。

遵循 [LICENSE-SOURCE-AVAILABLE.txt](LICENSE-SOURCE-AVAILABLE.txt) 与 [ATTRIBUTION.md](ATTRIBUTION.md)。UE API 属于 Epic；本仓库说明不再分发引擎实现、引擎 Shader 与资产。称为源码可阅数学节选。
