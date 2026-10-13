# 自定义按键实测

2026-10-04，通过 SSH 在掌机上使用 `evtest` 监测 `/dev/input/event*`。
设备树报告 `rockchip,rk3562`。

以下是 Linux evdev 原始键码，不是 SDL 按钮编号或 SDL scancode。

| 实体按键 | 输入设备 | EV_KEY 键码 | Linux 名称 | 结果 |
| --- | --- | --- | --- | --- |
| Fn1 | `/dev/input/event0` (`play_joystick`) | 615 | `KEY_RIGHT_DOWN` | 捕获到按下和松开 |
| Fn2 | `/dev/input/event0` (`play_joystick`) | 614 | `KEY_RIGHT_UP` | 2026-10-13 新版固件复测，多次短按及长按均捕获到按下和松开 |
| Fn3 | `/dev/input/event0` (`play_joystick`) | 102 | `KEY_HOME` | 捕获到按下和松开 |
| + | `/dev/input/event0` (`play_joystick`) | 464 | `KEY_FN` | 捕获到按下和松开 |

已识别按键的按下值为 `1`，松开值为 `0`。Fn1、Fn2、Fn3 和 + 已通过 Linux evdev 接入按键高亮和事件日志。
程序按 `play_joystick` 设备名称查找输入节点，不依赖固定的 `event0` 编号。
为避免按下和松开在同一帧内处理后看不到高亮，自定义键的显示至少保留 250 毫秒；实际按键状态仍按原始输入更新。加号由 SDL GUIDE 事件写入日志，evdev 负责其按住状态和短按高亮。

## 最新固件复测（2026-10-13）

设备报告 AisRock `70be24-Dirty-20261012`，构建分支 `RK3562_ROY01C`。只按右上角 Fn2，进行了多次短按及约 2 秒长按，`evtest /dev/input/event0` 均捕获到 `EV_KEY` / `614 (KEY_RIGHT_UP)`：按下为 `1`，松开为 `0`。

Fn2 现已确认使用 evdev 键码 614；程序移除 ADC 读取、阈值回滞和 ADC/evdev 状态合并，直接使用原始按键事件。

## 旧固件测量记录

此前固件的复测曾出现 Fn2 不产生 evdev 事件的情况。当时三次 Fn2 长按使 `ffaa0000.saradc` 的 `in_voltage3_raw` 从约 1023 降到 683–684，松开后恢复，程序因此曾加入 ADC 补充逻辑。该逻辑已在新版固件确认 evdev 上报后移除。
