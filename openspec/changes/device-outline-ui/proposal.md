## Why

当前仪表盘把按键控件按功能分组平铺在屏幕上(ABXY 在左、D-pad 在右),与 Yoobox Y1 的实际物理布局不符,测试人员无法一眼把屏幕上的控件映射到设备实物。需要按设备正面/顶部轮廓把每个按键摆到对应位置,让界面直接反映物理布局。

## What Changes

- 在仪表盘中以**矢量方式**(SDL 绘制,不依赖图片资产)画出 Yoobox Y1 的**正面轮廓**和**顶部轮廓**。
- 把控件按实际物理位置摆放:
  - 正面:左侧 D-pad、其下方左摇杆;右侧 ABXY 四键(菱形)、其下方右摇杆;底部 Start/Select、L3/R3;中央屏幕区域。
  - 顶部:左右两端 ZL/ZR(模拟扳机)与 L/R(肩键)。
- 每个控件叠加在轮廓上的对应位置;按下时仍在原位高亮/填充。
- 保留现有 `gamepad-input-tester` 的输入事件日志、无控制器 idle 状态、ESC 退出与 cyber-grid 背景风格。
- 推翻当前 `src/ui.cpp` 的 `DrawDashboard` 布局(ABXY 在左、D-pad 在右),改为与实物一致的左 D-pad / 右 ABXY 布局。
- 不引入新依赖,不把两张参考 jpg 拷入 `res/`(轮廓用矢量绘制)。

## Capabilities

### New Capabilities
- `device-outline-ui`: 以设备正面/顶部轮廓为底、按键按实际物理位置摆放并高亮的输入诊断视图。

### Modified Capabilities
- 无。`gamepad-input-tester` 的行为不变——仍是逐帧读取并高亮按键/摇杆/扳机、事件日志、无控制器 idle、ESC 退出;本变更只改呈现方式(布局与轮廓)。因此不产生新 delta,只新增本能力。

## Impact

- **`src/ui.cpp` / `src/ui.h`**: 重写 `DrawDashboard` 的布局;新增矢量绘制正面轮廓与顶部轮廓的辅助函数(圆角矩形为主,辅以顶点/自定义形状);ABXY/D-pad/摇杆/扳机/肩键/Start/Select/L3/R3 各绘制函数的调用参数(坐标)改为轮廓上的物理位置。
- **`src/app.cpp`**: 不变。
- **`src/font.{h,cpp}` / `src/main.cpp` / `CMakeLists.txt`**: 不变。
- **`res/`**: 不变(轮廓为矢量绘制,不新增图片资产)。
- **依赖**: 无新增。SDL2 + SDL2_ttf + SDL2_image 保持唯一要求。
