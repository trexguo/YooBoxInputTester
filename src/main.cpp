// YooBox Input Tester — 入口与主循环
//
// 针对 Yoobox Y1(RK3326,1620×1080)的手柄输入诊断界面。
// 使用 SDL_GameController 逐帧读取手柄状态,渲染实时仪表盘;
// 键盘 / 触摸 / 鼠标 / 手柄事件写入底部事件日志。

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "app.h"
#include "font.h"
#include "ui.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // 按目标设备基系统(Rocknix)顺序尝试显示后端;桌面测试回退链保持不变
    SDL_setenv("SDL_VIDEODRIVER", "wayland", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_setenv("SDL_VIDEODRIVER", "cocoa", 1);
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
            SDL_setenv("SDL_VIDEODRIVER", "x11", 1);
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
                SDL_Log("SDL_Init failed: %s", SDL_GetError());
                return 1;
            }
        }
    }
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow(
        "YooBox Input Tester",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) { SDL_Log("CreateWindow failed: %s", SDL_GetError()); return 1; }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { SDL_Log("CreateRenderer failed: %s", SDL_GetError()); return 1; }

    FontSet fonts;
    if (!FontLoad(fonts)) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return 1;
    }

    SDL_StartTextInput();

    AppState state;
    ControllersOpenAll(state);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;

            // 手柄热插拔(ADDED/REMOVED 只更新连接状态,不入日志)
            ControllersHandleEvent(state, event);
            // 手柄按钮/轴、键盘、触摸、鼠标事件 → 事件日志
            std::string name = KeyName(event);
            if (!name.empty())
                AppLog(state, name);
        }

        SDL_SetRenderDrawColor(renderer, C.bg.r, C.bg.g, C.bg.b, 255);
        SDL_RenderClear(renderer);
        DrawCyberGrid(renderer);

        DrawDashboard(renderer, fonts, state.mainController, state.log);

        SDL_RenderPresent(renderer);
    }

    FontUnload(fonts);
    SDL_StopTextInput();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
