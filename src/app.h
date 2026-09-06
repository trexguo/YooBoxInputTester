#ifndef YOOBOX_APP_H
#define YOOBOX_APP_H

#include <SDL2/SDL.h>
#include <deque>
#include <string>

// 目标设备固定分辨率(Yoobox Y1 1620×1080),各绘制模块共用
static const int WINDOW_W = 1620;
static const int WINDOW_H = 1080;

// 应用状态:控制器连接状态 + 最近事件日志。
// 仪表盘只由一个"主"控制器驱动,但保留 open-all 多设备逻辑。
struct AppState {
    SDL_GameController *mainController = nullptr; // 驱动仪表盘的那个
    std::deque<std::string> log;                  // 末 8 条事件
};

// 注册一段输入描述到有限日志(末 8 条,超出丢弃最旧)
void AppLog(AppState &st, const std::string &s);

// 把 SDL_Event 转成一段可读文本;没有匹配的输入类型返回空串
std::string KeyName(SDL_Event &e);

// 打开所有已连接手柄,并把第一个设为主控;已打开的保留
void ControllersOpenAll(AppState &st);

// 处理手柄的热插拔事件(进入事件循环后每帧调用),返回是否命中 controller 事件
bool ControllersHandleEvent(AppState &st, SDL_Event &e);

#endif // YOOBOX_APP_H
