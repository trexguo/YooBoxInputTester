#ifndef YOOBOX_UI_H
#define YOOBOX_UI_H

#include <SDL2/SDL.h>
#include <deque>
#include <string>

#include "font.h"

// 仪表盘所用的固定调色板(沿用原 demo 的 cyber-grid 风格)
struct Colors {
    SDL_Color bg       = {10, 10, 26, 255};
    SDL_Color cyan     = {0, 255, 255, 255};
    SDL_Color magenta  = {255, 0, 255, 255};
    SDL_Color yellow   = {255, 255, 0, 255};
    SDL_Color dimCyan  = {0, 180, 180, 255};
    SDL_Color white    = {220, 220, 220, 255};
    SDL_Color gridLine = {0, 60, 80, 60};
};

// 全局调色板实例(C++17 内联变量,各翻译单元共享一个)
inline const Colors C;

// 背景 cyber-grid + 发光矩形(原 demo 的直接搬移)
void DrawCyberGrid(SDL_Renderer *r);
void DrawGlowRect(SDL_Renderer *r, SDL_Rect rect, SDL_Color col, int glow);

// 按实际状态绘制仪表盘各组件的核心:
//   - 顶部状态行(标题 + 控制器连接状态)
//   - 按键网格(A/B/X/Y, Start/Select, LB/RB, L3/R3)
//   - D-pad 十字
//   - 左右摇杆表 + 数值
//   - LT/RT 扳机水平条
//   - 底部事件日志
// `mainController` 可为 null(无控制器 → 全部 idle + 提示)。
// `log` 为最近事件(末 ~8 条)。
void DrawDashboard(SDL_Renderer *r, const FontSet &fonts,
                   SDL_GameController *mainController,
                   const std::deque<std::string> &log);

#endif // YOOBOX_UI_H
