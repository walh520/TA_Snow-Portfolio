# Dependencies / 数据约定与依赖

- C++ 摘录使用 Unreal Engine 5.7.4 的 Core 数学、容器和类型；不包含引擎实现源代码。PerlinNoise2D 使用引擎 API。
- HLSL 摘录只包含数学函数，不包含 Engine Shader、场景纹理或资源声明，需由使用方提供编译入口。
- 网格分辨率须大于零，Cells 长度须为 `(Resolution + 1)^2`；所有接触块须采用一致的采样间距和共享世界网格键。CellPosition 仅支持规则网格样本。
- 长度单位 cm，时间单位 s，等效体积单位 cm³；光学消光系数单位 1/m。Shader 的长度积分包含 cm 到 m 的换算。
- ScalarMath 的风响应时间须大于零、Dt 非负；显示圆顶参数及密度噪声尺度须由调用方验证。实际系统的限幅、注册及预算策略不在摘录范围内。
- 分摊体积需使用同一有效接收面的面积加权分母；不能把对任意纹理权重求和替代为实际接收面积积分。

No Actor, renderer, plugin module descriptor, scene capture, SceneWind or TA_ToonVolumetricLighting runtime integration is shipped. This is a numerical source-review package.
