# TA Snow

积雪数学与 Shader 核心节选

从 TA_Snow 积雪系统中整理的数值实现与 Shader 数学函数，展示雪层状态、接触压实、体积转移、落雪沉积和粉雪光学的实现思路。

[GitHub 仓库](https://github.com/walh520/TA_Snow-Portfolio) · [雪地与沙地作品演示](https://www.bilibili.com/video/BV1v8H76wEix/) · [作品总集](https://www.bilibili.com/video/BV1MVak6jEPv/)

## 项目内容

本仓库面向 UE 5.7.4，提供数学代码、必要数据结构和 Shader 函数节选，适合阅读核心公式与数值实现。Actor、表面捕获、GPU 回读、网格生成、渲染调度及工程资产位于本地完整项目中，公开节选需自行集成调用与编译入口。

演示视频展示完整雪地与沙地作品，包含场景交互及当前仓库之外的系统。

## 实现与贡献

代码节选自 NiTong 的 Toon 项目 TA_Snow 实现。项目工作包括状态定义、求解与插值、显示/落地/天气/风公式接口，以及 Shader 数学函数的整理。

实现参考压实、面积积分、指数响应、噪声、Beer–Lambert 与 Henyey–Greenstein 等既有方法。雪雾积分与双相密度平流沿用同项目 Light Beam / Realtime Fog 的相关实现，详细来源见 [ATTRIBUTION.md](ATTRIBUTION.md)。

## 核心内容

- **雪层状态**：使用等效未压实雪厚、压实度和覆盖率表达二维顶面样本。
- **接触压实**：扫掠接触选择高度连通层，再执行压实与横向体积转移。
- **体积再分配**：接收环按网格面积和边角权重归一化；有效接收环存在时才移出雪量。
- **厚度与形状**：固定种子厚度、反对角线三角插值，以及独立于质量状态的显示高度。
- **沉积与环境响应**：有限支撑落地核、天气参考方程和固定风速的一阶指数响应。
- **粉雪表现**：Beer–Lambert 消光、HG 相函数、value noise 与双相密度平流。

## 数学设计

| 设计 | 实现方式 |
| --- | --- |
| 二维顶面样本 | 表达局部雪层厚度、覆盖和压实状态。 |
| 面积加权接收环 | 在离散网格上分配接触移出的雪量，按有效面积归一化。 |
| 显示高度与质量状态分离 | 调整视觉轮廓，同时保持求解状态的语义。 |
| 独立数学节选 | 将状态、公式与 Shader 函数集中展示，便于阅读核心实现与开展集成研究。 |

主要公式与约定：

- 雪量以等效未压实厚度表示，可见高度约为 `max(0, Baseline + MassOffset) / (1 + 2 × Compaction)`。压实改变几何高度，质量状态与显示高度分别维护。
- 接触外推按接收环权重、节点面积权重和单元面积归一化，守恒范围是该离散体积转移过程。
- 落点核 `exp(-4.5u²) × (1-u²)²` 在有效半径内求值，沉积量使用同一连通表面的加权面积归一化。
- 固定目标风速的一阶响应使用指数解析式，适用于目标风速固定的响应模型。
- Beer–Lambert 段积分显式将 cm 转成 m；HG、value noise 与双相位细节用于光学和视觉近似。

## 与本地系统的关系

本地 `TA_Snow` 是包含 Runtime 模块、WeatherActor、表面捕获、动态网格与 FX 的项目插件。本仓库抽取其中的 Solver/Shape、标量公式与粉雪数学函数，集中呈现数值核心。

本地完整系统的处理分工如下：

- **CPU 积雪求解**：维护雪层与压实状态，选择连通表面和接触，执行体积再分配、天气更新、落雪沉积及 ProceduralMesh 更新。
- **表面捕获与回读**：TopSurface 使用 SceneCapture 和异步 GPU texture readback，建立 CPU 可用的可见顶面快照。
- **GPU 粉雪表现**：通过 Vertex/Pixel Shader 与 Raster Pass 求值并积分光学，采用程序化视觉表现，无逐粒子回读。
- **脱离雪块**：使用 CPU 重力/阻尼推进并在落点沉积，运动模型聚焦雪块脱离与回落。
- **空中飘雪**：由独立的 GPUAgents Snowfall 路径处理，与 CPU 地面积雪求解分工配合。

TopSurface 使用单一可见顶面近似，适用范围是局部表面雪层。调度队列与时间预算采用软预算，实际开销需在目标场景测量。

本地实现入口：

- `Plugins/TA_Snow/Source/TASnowRuntime/Private/TASnowWeatherActor.cpp`：状态、沉积与网格
- `Plugins/TA_Snow/Source/TASnowRuntime/Private/TASnowTopSurface.cpp`：捕获与回读
- `Plugins/TA_Snow/Source/TASnowRuntime/Private/TASnowMovingCaps.cpp`：脱离与落地
- `Plugins/TA_Snow/Source/TASnowRuntime/Private/TASnowFX.cpp`：渲染调度
- `Plugins/TA_Snow/Shaders/Private/TASnowFX.usf`：光学与粒子表现

这些集成路径属于本地工程，未包含在当前公开节选中。

## 公开代码阅读入口

1. [TASnowMathTypes.h](Plugins/TA_Snow/Source/Core/TASnowMathTypes.h)：状态、网格与面积权重
2. [TASnowSolver.cpp](Plugins/TA_Snow/Source/Core/TASnowSolver.cpp)：扫掠压实与接收环转移
3. [TASnowShape.cpp](Plugins/TA_Snow/Source/Core/TASnowShape.cpp)：种子厚度和三角插值
4. [TASnowScalarMath.h](Plugins/TA_Snow/Source/Core/TASnowScalarMath.h)：显示、落地、天气和风公式
5. [TASnowPowderMath.ush](Plugins/TA_Snow/Shaders/Math/TASnowPowderMath.ush)：光学积分与密度细节

## 依赖与调用约定

C++ 使用 UE 5.7.4 Core 数学、容器与类型，PerlinNoise2D 调用引擎 API。HLSL 提供独立函数，Engine Shader、场景纹理、资源声明和编译入口由集成方提供。

完整调用约定见 [DEPENDENCIES.md](DEPENDENCIES.md)：

- `Resolution > 0`，Cells 长度为 `(Resolution + 1)^2`
- 接触块采用一致采样间距和共享世界网格键，CellPosition 适用于规则网格
- 长度单位 cm，时间 s，等效体积 cm³，消光系数 1/m；Shader 光学积分包含 cm 到 m 的换算
- 风响应时间大于 0，`Dt ≥ 0`；覆盖率、显示参数和噪声尺度由调用方校验
- 体积分摊以同一有效接收面的面积加权结果为分母

本地 Runtime 另使用 Landscape、ProceduralMeshComponent、TAWorldInteractionRuntime、Renderer/RenderCore/RHI 和 Renderer 私有头文件，描述文件声明 TA_WorldInteraction。可选水面检测通过反射接口接入，TA_Water 和 TA_GPUAgents 均非其硬依赖。

## 验证状态

公开数学节选与上述本地系统更新尚未执行编译、自动化测试、ShaderCompileWorker、Editor/PIE、GPU Capture 或性能测量。当前没有完整系统的 FPS、精度或稳定性实测结论。

集成验证需结合调用约定检查大位移接触、连通层选择、离散体积转移及显示结果；完整工程中的捕获、回读、天气和水面更新也需运行验证。

## 来源与许可

采用 [LICENSE-SOURCE-AVAILABLE.txt](LICENSE-SOURCE-AVAILABLE.txt) 所列的源码可阅（source-available）条款。项目实现、既有方法、同项目来源与 Epic API 的署名说明见 [ATTRIBUTION.md](ATTRIBUTION.md)。公开内容为数学节选，未再分发引擎实现、引擎 Shader 或资产。
