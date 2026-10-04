# 自定义按键实测

2026-10-04，通过 SSH 在掌机上使用 `evtest` 监测 `/dev/input/event*`。
设备树报告 `rockchip,rk3562`。

以下是 Linux evdev 原始键码，不是 SDL 按钮编号或 SDL scancode。

| 实体按键 | 输入设备 | EV_KEY 键码 | Linux 名称 | 结果 |
| --- | --- | --- | --- | --- |
| Fn1 | `/dev/input/event0` (`play_joystick`) | 615 | `KEY_RIGHT_DOWN` | 捕获到按下和松开 |
| Fn2 | `ffaa0000.saradc` 通道 3；evdev 待核实 | 614（暂定） | `KEY_RIGHT_UP` | ADC 实测可靠变化，但多次 evdev 复测不产生事件；614 保留为暂定映射 |
| Fn3 | `/dev/input/event0` (`play_joystick`) | 102 | `KEY_HOME` | 捕获到按下和松开 |
| + | `/dev/input/event0` (`play_joystick`) | 464 | `KEY_FN` | 捕获到按下和松开 |

已识别按键的按下值为 `1`，松开值为 `0`。Fn1、Fn2、Fn3 和 + 已通过 Linux evdev 接入按键高亮和事件日志。
程序按 `play_joystick` 设备名称查找输入节点，不依赖固定的 `event0` 编号。
本轮 Fn2 三组事件持续约 20、39、19 毫秒；部署后的程序日志也已捕获 `Fn2 [DOWN]` / `Fn2 [UP]`。
为避免按下和松开在同一帧内处理后看不到高亮，自定义键的显示至少保留 250 毫秒；实际按键状态和事件日志仍按原始输入更新。
用户反馈 Fn2 未显示后的复测中，其他按键正常上报，但未捕获到新的 614 事件；Fn2 上报的稳定性仍待确认。
随后按一次 A、再按 Fn2 三次的对照测试中，`event0` 捕获到 305 (`BTN_EAST`) 的按下/松开，各输入节点均没有对应 Fn2 的新按键事件。监测正常，问题发生在测试程序读取之前；614 与实体 Fn2 的对应关系仍需进一步验证。

## ADC 原始信号对照

三次 Fn2 长按均使 `ffaa0000.saradc` 的 `in_voltage3_raw` 从约 1023 降到 683–684，持续约 3 秒，松开后恢复；期间没有 evdev 按键事件。Fn1 对照使该 ADC 的通道 2 降到约 681–682，并正常产生 615 事件。

测试程序在设备兼容标识包含 `rockchip,rk3562` 且 ADC 名称匹配时启用 Fn2 ADC 读取：650–720 判定按下，保持范围 620–750；其他值判定松开。设备树中的 START 也使用通道 3，其低电平按下范围与 Fn2 区分。日志注明 `ADC3`，ADC 与 evdev 状态取并集。此处是本机实测适配，不是通用 ROCKNIX 按键映射。
