#include "font.h"

constexpr const char *FONT_PATH = "res/fonts/font.ttf";

bool FontLoad(FontSet &fonts) {
    fonts.title = TTF_OpenFont(FONT_PATH, 72);
    fonts.key   = TTF_OpenFont(FONT_PATH, 52);
    fonts.hint  = TTF_OpenFont(FONT_PATH, 32);
    return fonts.title && fonts.key && fonts.hint;
}

void FontUnload(FontSet &fonts) {
    if (fonts.title) TTF_CloseFont(fonts.title);
    if (fonts.key)   TTF_CloseFont(fonts.key);
    if (fonts.hint)  TTF_CloseFont(fonts.hint);
    fonts = FontSet{};
}

SDL_Texture *RenderText(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color col) {
    if (!text || text[0] == '\0') return nullptr;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, col);
    if (!surf) return nullptr;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FreeSurface(surf);
    return tex;
}
