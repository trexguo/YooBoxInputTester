#ifndef YOOBOX_FONT_H
#define YOOBOX_FONT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// 三个预加载字号:标题 / 按键标签 / 提示与日志
struct FontSet {
    TTF_Font *title = nullptr;   // 72 px
    TTF_Font *key   = nullptr;   // 52 px
    TTF_Font *hint  = nullptr;   // 32 px
};

// 加载 res/fonts/font.ttf 的三个字号;任一失败返回 false
bool FontLoad(FontSet &fonts);
void FontUnload(FontSet &fonts);

// 渲染一段 UTF-8 文本到纹理;失败返回 nullptr(调用方负责销毁)
SDL_Texture *RenderText(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color col);

#endif // YOOBOX_FONT_H
