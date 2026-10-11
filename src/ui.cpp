#include "ui.h"

#include "app.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

static const float PI = 3.14159265f;
static constexpr int MACHINE_OFFSET_Y = 0;
static constexpr int RECT_BUTTON_HEIGHT = 66;
static constexpr int FACE_BUTTON_DIAMETER = 78;
static constexpr int LEFT_CONTROL_X = 270;
static constexpr int RIGHT_CONTROL_X = WINDOW_W - LEFT_CONTROL_X;
static SDL_Texture *machineLayer = nullptr;
struct MachineVisualState {
    SDL_JoystickID controller = -1;
    std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> axes{};
    std::array<Uint8, SDL_CONTROLLER_BUTTON_MAX> buttons{};
    std::array<bool, static_cast<std::size_t>(CustomKey::Count)> customHighlights{};
    bool operator==(const MachineVisualState &other) const {
        return controller == other.controller && axes == other.axes &&
               buttons == other.buttons && customHighlights == other.customHighlights;
    }
};
static MachineVisualState cachedMachineState;
static std::deque<std::string> cachedMachineLog;
static bool machineLayerValid = false;
// 中央日志区关于 x=810 对称,与两侧圆形按键保留间距。
static constexpr SDL_Rect EVENT_LOG_AREA = {450, 330 + MACHINE_OFFSET_Y, 720, 540};
static constexpr SDL_Rect RECORD_BUTTON = {1450, 48, 110, 68};
static constexpr SDL_Rect MICROPHONE_PANEL = {360, 400, 900, 280};
static constexpr SDL_Rect MICROPHONE_CLOSE = {1172, 420, 64, 64};

bool UIHandleEvent(AppState &state, const SDL_Event &event) {
    if (state.microphoneModal && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        MicrophoneClose(state);
        return true;
    }
    if (state.microphoneDevice && event.type == SDL_AUDIODEVICEREMOVED &&
        event.adevice.iscapture && event.adevice.which == state.microphoneDevice) {
        MicrophoneClose(state);
        state.microphoneModal = true;
        state.microphoneError = "Microphone disconnected";
        return true;
    }
    SDL_Point point{};
    bool pressed = false;
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT &&
        event.button.which != SDL_TOUCH_MOUSEID) {
        point = {event.button.x, event.button.y};
        pressed = true;
    } else if (event.type == SDL_FINGERDOWN &&
               std::isfinite(event.tfinger.x) && std::isfinite(event.tfinger.y)) {
        point = {static_cast<int>(event.tfinger.x * WINDOW_W),
                 static_cast<int>(event.tfinger.y * WINDOW_H)};
        pressed = true;
    }
    if (pressed) {
        if (state.microphoneModal) {
            if (SDL_PointInRect(&point, &MICROPHONE_CLOSE)) MicrophoneClose(state);
            return true;
        }
        if (SDL_PointInRect(&point, &RECORD_BUTTON)) {
            MicrophoneOpen(state);
            return true;
        }
    }
    // 弹窗期间消费指针事件,避免穿透到底层仪表盘。
    return state.microphoneModal &&
           ((event.type >= SDL_MOUSEMOTION && event.type <= SDL_MOUSEWHEEL) ||
            event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || event.type == SDL_FINGERMOTION);
}

static bool ButtonHeld(SDL_GameController *gc, SDL_GameControllerButton btn) {
    return gc && SDL_GameControllerGetButton(gc, ControllerButtonForDevice(gc, btn)) != 0;
}

static Sint16 AxisVal(SDL_GameController *gc, SDL_GameControllerAxis ax) {
    return gc ? SDL_GameControllerGetAxis(gc, ax) : 0;
}

void DrawCyberGrid(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, C.gridLine.r, C.gridLine.g, C.gridLine.b, C.gridLine.a);
    for (int x = 0; x < WINDOW_W; x += 60)
        SDL_RenderDrawLine(r, x, 0, x, WINDOW_H);
    for (int y = 0; y < WINDOW_H; y += 60)
        SDL_RenderDrawLine(r, 0, y, WINDOW_W, y);
}

void DrawGlowRect(SDL_Renderer *r, SDL_Rect rect, SDL_Color col, int glow) {
    for (int i = glow; i > 0; i--) {
        SDL_Color fc = col;
        fc.a = (Uint8)(40 * i / glow);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, fc.r, fc.g, fc.b, fc.a);
        SDL_Rect gr = {rect.x - i * 2, rect.y - i * 2, rect.w + i * 4, rect.h + i * 4};
        SDL_RenderFillRect(r, &gr);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 255);
    SDL_RenderDrawRect(r, &rect);
}

// 圆角矩形(纯基础图元,不依赖 SDL_RenderGeometry):
//   fill=true → 用水平条带填充近似圆角;fill=false → 直边 + 折线圆弧描边。
static void DrawRoundedRect(SDL_Renderer *rd, SDL_Rect rect, int r, SDL_Color col, bool fill) {
    if (r <= 0 || r * 2 > rect.w || r * 2 > rect.h)
        r = std::min(rect.w, rect.h) / 2;
    if (r <= 0) { // 退化:纯矩形
        SDL_SetRenderDrawColor(rd, col.r, col.g, col.b, col.a);
        if (fill) SDL_RenderFillRect(rd, &rect);
        else      SDL_RenderDrawRect(rd, &rect);
        return;
    }

    SDL_SetRenderDrawColor(rd, col.r, col.g, col.b, col.a);

    if (fill) {
        // 中央整块
        SDL_Rect core = {rect.x + r, rect.y, rect.w - 2 * r, rect.h};
        SDL_RenderFillRect(rd, &core);
        // 左右整块(横跨全高,不含顶部/底部圆角)
        SDL_Rect sides = {rect.x, rect.y + r, rect.w, rect.h - 2 * r};
        SDL_RenderFillRect(rd, &sides);
        // 四角:逐行条带近似 1/4 圆
        for (int cy = 0; cy < r; ++cy) {
            int dx = r - 1 - cy;                       // 该行距圆心的水平偏移
            int inx = (int)sqrtf((float)(r * r - dx * dx)); // 该行应填充的水平半径
            if (inx <= 0) continue;
            // 左上
            SDL_Rect tl = {rect.x + r - inx, rect.y + cy, inx, 1};
            SDL_RenderFillRect(rd, &tl);
            // 右上
            SDL_Rect tr = {rect.x + rect.w - r, rect.y + cy, inx, 1};
            SDL_RenderFillRect(rd, &tr);
            // 左下
            SDL_Rect bl = {rect.x + r - inx, rect.y + rect.h - 1 - cy, inx, 1};
            SDL_RenderFillRect(rd, &bl);
            // 右下
            SDL_Rect br = {rect.x + rect.w - r, rect.y + rect.h - 1 - cy, inx, 1};
            SDL_RenderFillRect(rd, &br);
        }
    } else {
        // 直边
        SDL_RenderDrawLine(rd, rect.x + r, rect.y, rect.x + rect.w - r, rect.y);
        SDL_RenderDrawLine(rd, rect.x + r, rect.y + rect.h, rect.x + rect.w - r, rect.y + rect.h);
        SDL_RenderDrawLine(rd, rect.x, rect.y + r, rect.x, rect.y + rect.h - r);
        SDL_RenderDrawLine(rd, rect.x + rect.w, rect.y + r, rect.x + rect.w, rect.y + rect.h - r);
        // 四角圆弧(折线)
        struct C { int cx, cy; int sx, sy; }; // 圆心 + 起始方向角(deg)
        C cs[4] = {
            {rect.x + r,         rect.y + r,         180, 0},   // 左上
            {rect.x + rect.w - r, rect.y + r,         270, 0},   // 右上
            {rect.x + rect.w - r, rect.y + rect.h - r,  0, 0},   // 右下
            {rect.x + r,         rect.y + rect.h - r,  90, 0},   // 左下
        };
        for (int c = 0; c < 4; ++c) {
            SDL_Point prev = {cs[c].cx + (int)(r * cosf(cs[c].sx * PI / 180.0f)),
                              cs[c].cy + (int)(r * sinf(cs[c].sx * PI / 180.0f))};
            for (int k = 1; k <= 8; ++k) {
                float a = (cs[c].sx + 90.0f * k / 8.0f) * PI / 180.0f;
                SDL_Point p = {cs[c].cx + (int)(r * cosf(a)), cs[c].cy + (int)(r * sinf(a))};
                SDL_RenderDrawLine(rd, prev.x, prev.y, p.x, p.y);
                prev = p;
            }
        }
    }
}

// 有界 LRU 缓存:静态标签和未变化的日志无需每帧光栅化、上传纹理。
struct CachedText {
    TTF_Font *font;
    std::string text;
    SDL_Color color;
    SDL_Texture *texture;
};
static std::deque<CachedText> textCache;

static SDL_Texture *CachedTextTexture(SDL_Renderer *renderer, TTF_Font *font,
                                      const char *text, SDL_Color color) {
    if (!text || !*text) return nullptr;
    for (auto it = textCache.begin(); it != textCache.end(); ++it) {
        if (it->font == font && it->text == text &&
            it->color.r == color.r && it->color.g == color.g &&
            it->color.b == color.b && it->color.a == color.a) {
            CachedText entry = *it;
            textCache.erase(it);
            textCache.push_back(entry);
            return entry.texture;
        }
    }
    SDL_Texture *texture = RenderText(renderer, font, text, color);
    if (!texture) return nullptr;
    if (textCache.size() >= 128) {
        SDL_DestroyTexture(textCache.front().texture);
        textCache.pop_front();
    }
    textCache.push_back({font, text, color, texture});
    return texture;
}

// 文本居中到 (cx, cy);返回高度;出错返回 0
static int DrawCentered(SDL_Renderer *r, TTF_Font *font, const char *s,
                        SDL_Color col, int cx, int cy) {
    SDL_Texture *t = CachedTextTexture(r, font, s, col);
    if (!t) return 0;
    int w, h;
    SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
    SDL_Rect d = {cx - w / 2, cy - h / 2, w, h};
    SDL_RenderCopy(r, t, nullptr, &d);
    return h;
}

// 一个按键框:发光描边 + (按下时)内部填充。文字用 key 字号。
static void DrawButtonBox(SDL_Renderer *r, TTF_Font *font, const char *label,
                          SDL_Rect box, bool active) {
    SDL_Color col = active ? C.magenta : C.dimCyan;
    DrawGlowRect(r, box, col, active ? 4 : 2);
    if (active) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 70);
        SDL_RenderFillRect(r, &box);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
    DrawCentered(r, font, label, C.white, box.x + box.w / 2, box.y + box.h / 2);
}

// 分区标题
static void DrawSectionTitle(SDL_Renderer *r, const FontSet &fonts, const char *s, int cx, int cy) {
    DrawCentered(r, fonts.hint, s, C.dimCyan, cx, cy);
}

// 顶部状态行
static void DrawStatusLine(SDL_Renderer *r, const FontSet &fonts,
                           SDL_GameController *gc) {
    const char *title = "YooBox Input Tester";
    SDL_Texture *titleTex = CachedTextTexture(r, fonts.title, title, C.cyan);
    int tw = 0, th = 0;
    if (titleTex) {
        SDL_QueryTexture(titleTex, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {WINDOW_W / 2 - tw / 2, 62 - th / 2, tw, th};
        SDL_RenderCopy(r, titleTex, nullptr, &d);
    }
    // 标题右侧小号版本号,底边与标题底边对齐
    int badgeW = 52, badgeH = 26;
    int bx = WINDOW_W / 2 + tw / 2 + 12;
    int by = 62 + th / 2 - badgeH;   // 标题底边(62+th/2)向上取 badgeH,使两者底边相平
    SDL_Rect badge = {bx, by, badgeW, badgeH};
    DrawRoundedRect(r, badge, 8, (SDL_Color){10, 14, 24, 255}, true);
    DrawCentered(r, fonts.hint, "v1.0", C.dimCyan, bx + badgeW / 2, by + badgeH / 2);

    const char *msg = "No controller - plug in a USB gamepad";
    SDL_Color col = C.dimCyan;
    if (gc) {
        const char *name = SDL_GameControllerName(gc);
        msg = name ? name : "Controller connected";
        col = C.magenta;
    }

    SDL_Rect box = {WINDOW_W / 2 - 300, 112, 600, 40};
    DrawGlowRect(r, box, col, 2);
    SDL_Texture *t = CachedTextTexture(r, fonts.hint, msg, col);
    if (t) {
        int tw, th;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {WINDOW_W / 2 - tw / 2, box.y + (box.h - th) / 2, tw, th};
        SDL_RenderCopy(r, t, nullptr, &d);
    }
}

// 圆形按键:填充圆 + 发光描边 + 中心标签。(cx,cy) 为圆心,d 为直径。
static void DrawRoundButton(SDL_Renderer *r, TTF_Font *font, const char *label,
                            int cx, int cy, int d, bool active) {
    const int n = 48, rad = d / 2;
    SDL_Point ring[49];
    for (int i = 0; i < n; ++i) {
        float a = (float)i * 2.0f * PI / n;
        ring[i] = {cx + (int)(rad * cosf(a)), cy + (int)(rad * sinf(a))};
    }
    ring[n] = ring[0];
    // 按压时填充
    if (active) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 70);
        for (int dy = -rad; dy <= rad; ++dy) {
            const int halfWidth = static_cast<int>(std::sqrt(rad * rad - dy * dy));
            SDL_RenderDrawLine(r, cx - halfWidth, cy + dy, cx + halfWidth, cy + dy);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
    // 发光外环
    SDL_Color col = active ? C.magenta : C.dimCyan;
    for (int ringi = 3; ringi >= 1; --ringi) {
        SDL_Point g[49];
        int gr = rad + ringi * 2;
        for (int i = 0; i < n; ++i) {
            float a = (float)i * 2.0f * PI / n;
            g[i] = {cx + (int)(gr * cosf(a)), cy + (int)(gr * sinf(a))};
        }
        g[n] = g[0];
        // 外圈淡色 → 内圈亮
        int alpha = active ? 90 - ringi * 25 : 40 - ringi * 10;
        if (alpha < 0) alpha = 0;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, (Uint8)alpha);
        SDL_RenderDrawLines(r, g, n + 1);
    }
    // 主描边
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 255);
    SDL_RenderDrawLines(r, ring, n + 1);
    DrawCentered(r, font, label, active ? C.white : C.dimCyan, cx, cy);
}

// 十字键(D-pad):四个独立方向键围成菱形(上/左/下/右),中心留空。
// 圆形按键,直径 d;与 ABXY 共用相同 d / 间距。
static void DrawDPad(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc,
                     int cx, int cy, int d = 66, const char *title = "D-PAD") {
    if (title) DrawSectionTitle(r, fonts, title, cx, cy - 150);
    struct Dir { const char *label; SDL_GameControllerButton btn; int dx, dy; };
    static const Dir dirs[4] = {
        {"U", SDL_CONTROLLER_BUTTON_DPAD_UP,    0, -1},
        {"L", SDL_CONTROLLER_BUTTON_DPAD_LEFT, -1,  0},
        {"D", SDL_CONTROLLER_BUTTON_DPAD_DOWN,  0,  1},
        {"R", SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1,  0},
    };
    const int off = d;       // 收紧方向键组内间距
    for (int i = 0; i < 4; ++i) {
        int bx = cx + dirs[i].dx * off;
        int by = cy + dirs[i].dy * off;
        DrawRoundButton(r, fonts.hint, dirs[i].label, bx, by, d, ButtonHeld(gc, dirs[i].btn));
    }
}

// ABXY 菱形/十字布局:Y 左、X 右、B 上、A 下(X↔Y、A↔B 对调)。圆形按键,直径 d;与 D-pad 完全一致。
static void DrawABXY(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc,
                     int cx, int cy, int d = 66) {
    struct Btn { const char *label; SDL_GameControllerButton btn; int dx, dy; };
    static const Btn btns[4] = {
        {"X", SDL_CONTROLLER_BUTTON_X,  0, -1},   // 上
        {"Y", SDL_CONTROLLER_BUTTON_Y, -1,  0},   // 左
        {"B", SDL_CONTROLLER_BUTTON_B,  0,  1},   // 下
        {"A", SDL_CONTROLLER_BUTTON_A,  1,  0},   // 右
    };
    const int off = d;       // 与方向键使用相同的紧凑间距
    for (int i = 0; i < 4; ++i) {
        int bx = cx + btns[i].dx * off;
        int by = cy + btns[i].dy * off;
        DrawRoundButton(r, fonts.hint, btns[i].label, bx, by, d, ButtonHeld(gc, btns[i].btn));
    }
}

// 摇杆表:外圈 + 参考线 + 位置点 + 数值。pressBtn 为摇杆下压(L3/R3),按下时点亮中心标签。
static void DrawStickGauge(SDL_Renderer *r, const FontSet &fonts,
                           const char *title, SDL_GameController *gc,
                           SDL_GameControllerAxis ax, SDL_GameControllerAxis ay,
                           int cx, int cy, int radius,
                           const char *pressLabel = nullptr,
                           SDL_GameControllerButton pressBtn = SDL_CONTROLLER_BUTTON_INVALID) {
    if (title && pressLabel)
        DrawSectionTitle(r, fonts, title, cx, cy - radius - 24);

    const int n = 48;
    SDL_Point pts[49];
    for (int i = 0; i < n; ++i) {
        float ang = (float)i * 2.0f * PI / n;
        pts[i] = {cx + (int)(radius * cosf(ang)), cy + (int)(radius * sinf(ang))};
    }
    SDL_SetRenderDrawColor(r, C.dimCyan.r, C.dimCyan.g, C.dimCyan.b, 255);
    pts[n] = pts[0];
    SDL_RenderDrawLines(r, pts, n + 1);

    SDL_SetRenderDrawColor(r, C.dimCyan.r, C.dimCyan.g, C.dimCyan.b, 120);
    SDL_RenderDrawLine(r, cx - radius, cy, cx + radius, cy);
    SDL_RenderDrawLine(r, cx, cy - radius, cx, cy + radius);

    float dx = AxisVal(gc, ax) / 32767.0f;
    float dy = AxisVal(gc, ay) / 32767.0f;
    SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 255);
    SDL_Rect dot = {cx + (int)(dx * radius) - 6, cy + (int)(dy * radius) - 6, 12, 12};
    SDL_RenderFillRect(r, &dot);

    // 摇杆下压指示(如 L3/R3):按下时在圆心亮起标签
    bool pressed = ButtonHeld(gc, pressBtn);
    if (pressLabel && pressed)
        DrawCentered(r, fonts.key, pressLabel, C.magenta, cx, cy);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "X %d  Y %d", AxisVal(gc, ax), AxisVal(gc, ay));
    DrawCentered(r, fonts.hint, buf, C.white, cx, cy + radius + 26);
}

// 事件日志:固定字号、固定行高,超出宽度直接裁切。
static void DrawLog(SDL_Renderer *r, const FontSet &fonts, const std::deque<std::string> &log) {
    SDL_Rect screen = EVENT_LOG_AREA;
    // 底衬,让文字在屏幕上可读
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_RenderFillRect(r, &screen);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    const int padding = 12;
    const int gap = 8;
    const int blockPaddingX = 12;
    const int blockWidth = screen.w - 2 * padding;
    const int textWidth = blockWidth - 2 * blockPaddingX;
    const int contentTop = screen.y + padding;
    const int contentHeight = screen.h - 2 * padding;
    const int visibleRows = 8;
    const int blockHeight = (contentHeight - gap * (visibleRows - 1)) / visibleRows;
    DrawSectionTitle(r, fonts, "EVENT LOG", WINDOW_W / 2, 270 + MACHINE_OFFSET_Y);
    if (log.empty()) {
        DrawCentered(r, fonts.eventLog, "(waiting for input)", C.dimCyan,
                     WINDOW_W / 2, contentTop + contentHeight / 2);
        return;
    }

    struct LogLine { SDL_Texture *texture; int width; int height; };
    std::deque<LogLine> lines;
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        if (static_cast<int>(lines.size()) >= visibleRows) break;
        SDL_Texture *texture = CachedTextTexture(r, fonts.eventLog, it->c_str(), C.white);
        if (!texture) continue;
        int width = 0, height = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
        lines.push_front({texture, width, height});
    }
    int y = contentTop;
    int blockIndex = 0;
    for (const auto &line : lines) {
        SDL_Rect block = {screen.x + padding, y, blockWidth,
                          blockHeight};
        const SDL_Color background = blockIndex++ % 2 == 0
            ? SDL_Color{22, 36, 48, 255} : SDL_Color{34, 50, 64, 255};
        DrawRoundedRect(r, block, 8, background, true);
        const int visibleWidth = std::min(line.width, textWidth);
        SDL_Rect source = {0, 0, visibleWidth, line.height};
        SDL_Rect destination = {WINDOW_W / 2 - visibleWidth / 2,
                                y + (blockHeight - line.height) / 2, visibleWidth, line.height};
        SDL_RenderCopy(r, line.texture, &source, &destination);
        y += block.h + gap;
    }
}

// 顶部按键:位于机身外框上方。LB(左肩)、LT(左扳机)对称于 R2→RB(右肩)、RT(右扳机),
// 中间为 Fn1 / Fn2。扳机仍用轴判断按下,外观与其他矩形按钮一致。
static void DrawTopTriggers(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc,
                            const AppState &state) {
    struct Key {
        const char *label;
        SDL_GameControllerAxis ax;   // 非触发键用 SDL_CONTROLLER_AXIS_INVALID(不画深度)
        SDL_GameControllerButton btn;   // 触发键无独立按钮,置 INVALID,按下状态改由轴向判断
        CustomKey custom = CustomKey::Count;
    };
    static const Key keys[6] = {
        {"LB",  SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"LT",  SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_BUTTON_INVALID},
        {"Fn1", SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_INVALID, CustomKey::Fn1},
        {"Fn2", SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_INVALID, CustomKey::Fn2},
        {"RT",  SDL_CONTROLLER_AXIS_TRIGGERRIGHT, SDL_CONTROLLER_BUTTON_INVALID},
        {"RB",  SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
    };
    const int bw = 104, bh = RECT_BUTTON_HEIGHT, gap = 16;
    const int y0 = 226 + MACHINE_OFFSET_Y;     // 顶排随机身整体下移
    // 左组(LB,LT)中心=270;fn1 在 LT 右侧留一段距离,fn2 在屏中线对称位置
    const int leftCX = LEFT_CONTROL_X;
    const int pairW = 2 * bw + gap;           // 104+16+104 = 224
    const int l2Right = leftCX + pairW / 2;   // LT 右缘
    const int fnGap = 40;                     // fn1 与 LT 的间距
    const int fnCX1 = l2Right + fnGap + bw / 2;   // fn1 中心(在 LT 右侧留一段距离)
    const int fnCX2 = WINDOW_W - fnCX1;           // fn1 的对称位置中心(关于 x=810)
    int bx[6] = {
        leftCX - pairW / 2,                    // LB
        leftCX - pairW / 2 + bw + gap,         // LT
        fnCX1 - bw / 2,                        // fn1
        fnCX2 - bw / 2,                        // fn2
        WINDOW_W - leftCX - pairW / 2,         // RB (右对影)
        WINDOW_W - leftCX - pairW / 2 + bw + gap, // RT
    };
    for (int i = 0; i < 6; ++i) {
        SDL_Rect b = {bx[i], y0, bw, bh};
        SDL_SetRenderDrawColor(r, 10, 14, 24, 255);
        SDL_RenderFillRect(r, &b);
        // 触发键在无独立按钮时,按下状态由轴向超过阈值判断(避免与肩键共用同一按钮常量而联动)
        bool a = false;
        if (keys[i].custom != CustomKey::Count)
            a = CustomKeyHighlighted(state, keys[i].custom);
        else if (keys[i].btn != SDL_CONTROLLER_BUTTON_INVALID)
            a = ButtonHeld(gc, keys[i].btn);
        else if (keys[i].ax != SDL_CONTROLLER_AXIS_INVALID) {
            // SDL 控制器扳机轴范围为 0..32767,背景按真实行程连续填充。
            const float depth = std::clamp(AxisVal(gc, keys[i].ax) / 32767.0f, 0.0f, 1.0f);
            DrawGlowRect(r, b, C.dimCyan, 2);
            SDL_Rect inner = {b.x + 1, b.y + 1, b.w - 2, b.h - 2};
            const int fillHeight = static_cast<int>(std::lround(depth * inner.h));
            if (fillHeight > 0) {
                SDL_Rect fill = {inner.x, inner.y + inner.h - fillHeight, inner.w, fillHeight};
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 140);
                SDL_RenderFillRect(r, &fill);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            }
            DrawCentered(r, fonts.hint, keys[i].label, C.white, b.x + b.w / 2, b.y + b.h / 2);
            continue;
        }
        DrawButtonBox(r, fonts.hint, keys[i].label, b, a);
    }
}

// 正面轮廓:主体圆角矩形 + 中央屏幕(内为事件日志)+ 底缘小键。
static void DrawFrontOutline(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc,
                             const AppState &state) {
    SDL_Rect body = {90, 300 + MACHINE_OFFSET_Y, WINDOW_W - 180, 600};
    DrawRoundedRect(r, body, 46, (SDL_Color){34, 38, 58, 255}, true);
    DrawRoundedRect(r, body, 46, C.dimCyan, false);

    // 顶部扳机键(LT / L2 / RT / R2)
    DrawTopTriggers(r, fonts, gc, state);

    // 日志屏幕在机身内垂直居中。
    SDL_Rect screen = EVENT_LOG_AREA;
    DrawRoundedRect(r, screen, 20, (SDL_Color){6, 6, 12, 255}, true);

    // 两组圆形按键:直径与间距完全一致,左右列中线对齐(内移后仍对称)
    DrawDPad(r, fonts, gc, LEFT_CONTROL_X, 444 + MACHINE_OFFSET_Y, FACE_BUTTON_DIAMETER, nullptr);
    DrawABXY(r, fonts, gc, RIGHT_CONTROL_X, 444 + MACHINE_OFFSET_Y, FACE_BUTTON_DIAMETER);

    DrawStickGauge(r, fonts, nullptr, gc,                     // 左摇杆(D-pad 下)
                   SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, LEFT_CONTROL_X, 670 + MACHINE_OFFSET_Y, 78,
                   "L3", SDL_CONTROLLER_BUTTON_LEFTSTICK);
    DrawStickGauge(r, fonts, nullptr, gc,                     // 右摇杆(ABXY 下)→ 与左摇杆镜像
                   SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY, RIGHT_CONTROL_X, 670 + MACHINE_OFFSET_Y, 78,
                   "R3", SDL_CONTROLLER_BUTTON_RIGHTSTICK);

    // 底缘:两列各自紧凑居中 —— 左列(`+`/`Fn3`)中心=270,右列(`SELECT`/`START`)中心=1350
    const int keyw = 100, keyh = RECT_BUTTON_HEIGHT, keygap = 14;
    const int keyy = 812 + MACHINE_OFFSET_Y;
    SDL_Rect plus = {LEFT_CONTROL_X - keyw - keygap / 2, keyy, keyw, keyh};  // +   左列左
    SDL_Rect fn3  = {LEFT_CONTROL_X + keygap / 2,        keyy, keyw, keyh};  // Fn3 左列右
    SDL_Rect stan = {RIGHT_CONTROL_X - keyw - keygap / 2, keyy, keyw, keyh}; // SELECT 右列左
    SDL_Rect seln = {RIGHT_CONTROL_X + keygap / 2,        keyy, keyw, keyh}; // START 右列右
    DrawButtonBox(r, fonts.key, "+",    plus,
                  ButtonHeld(gc, SDL_CONTROLLER_BUTTON_GUIDE) || CustomKeyHighlighted(state, CustomKey::Plus));
    DrawButtonBox(r, fonts.hint, "Fn3",  fn3,  CustomKeyHighlighted(state, CustomKey::Fn3));
    DrawButtonBox(r, fonts.hint, "SELECT",  stan, ButtonHeld(gc, SDL_CONTROLLER_BUTTON_BACK));
    DrawButtonBox(r, fonts.hint, "START", seln, ButtonHeld(gc, SDL_CONTROLLER_BUTTON_START));

    // 事件日志画在屏幕区域上
    DrawLog(r, fonts, state.log);
}

static void DrawMicrophone(SDL_Renderer *r, const FontSet &fonts, const AppState &state) {
    DrawButtonBox(r, fonts.hint, "Mic", RECORD_BUTTON, state.microphoneModal);
    if (!state.microphoneModal) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 175);
    SDL_RenderFillRect(r, nullptr);
    // 圆角阴影和轻微外沿光晕,避免矩形发光填充遮住仪表内容。
    SDL_Rect shadow = {MICROPHONE_PANEL.x - 8, MICROPHONE_PANEL.y + 12,
                       MICROPHONE_PANEL.w + 16, MICROPHONE_PANEL.h + 8};
    DrawRoundedRect(r, shadow, 32, (SDL_Color){0, 0, 0, 100}, true);
    for (int inset = 8; inset >= 2; inset -= 2) {
        SDL_Rect halo = {MICROPHONE_PANEL.x - inset, MICROPHONE_PANEL.y - inset,
                         MICROPHONE_PANEL.w + inset * 2, MICROPHONE_PANEL.h + inset * 2};
        DrawRoundedRect(r, halo, 28 + inset, (SDL_Color){0, 200, 220, 12}, false);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    DrawRoundedRect(r, MICROPHONE_PANEL, 28, (SDL_Color){16, 22, 36, 255}, true);
    DrawRoundedRect(r, MICROPHONE_PANEL, 28, (SDL_Color){38, 94, 110, 255}, false);
    DrawCentered(r, fonts.hint, "Microphone Test", C.white,
                 MICROPHONE_PANEL.x + MICROPHONE_PANEL.w / 2,
                 MICROPHONE_CLOSE.y + MICROPHONE_CLOSE.h / 2);
    DrawRoundedRect(r, MICROPHONE_CLOSE, 16, (SDL_Color){26, 36, 52, 255}, true);
    DrawRoundedRect(r, MICROPHONE_CLOSE, 16, (SDL_Color){52, 80, 94, 255}, false);
    DrawCentered(r, fonts.hint, "X", C.white,
                 MICROPHONE_CLOSE.x + MICROPHONE_CLOSE.w / 2,
                 MICROPHONE_CLOSE.y + MICROPHONE_CLOSE.h / 2);

    const SDL_Rect track = {424, 510, 772, 64};
    DrawRoundedRect(r, track, 14, (SDL_Color){7, 12, 22, 255}, true);
    DrawRoundedRect(r, track, 14, (SDL_Color){30, 50, 68, 255}, false);
    constexpr int segments = 36, segmentWidth = 16, gap = 5;
    const float activeSegments = std::clamp(state.microphoneLevel, 0.0f, 1.0f) * segments;
    for (int i = 0; i < segments; ++i) {
        SDL_Rect segment = {track.x + 10 + i * (segmentWidth + gap), track.y + 10,
                            segmentWidth, track.h - 20};
        DrawRoundedRect(r, segment, 4, (SDL_Color){21, 33, 48, 255}, true);
        const float fraction = std::clamp(activeSegments - i, 0.0f, 1.0f);
        if (fraction == 0.0f) continue;
        SDL_Color color = i >= 33 ? C.magenta :
                          i >= 28 ? C.yellow : (SDL_Color){0, 218, 204, 255};
        // 最后一段渐亮,保留连续音量反馈而不是仅有整段跳变。
        color.r = static_cast<Uint8>(21 + (color.r - 21) * fraction);
        color.g = static_cast<Uint8>(33 + (color.g - 33) * fraction);
        color.b = static_cast<Uint8>(48 + (color.b - 48) * fraction);
        DrawRoundedRect(r, segment, 4, color, true);
    }
    SDL_SetRenderDrawColor(r, 64, 96, 112, 255);
    for (int x : {track.x + 10, WINDOW_W / 2, track.x + track.w - 10})
        SDL_RenderDrawLine(r, x, 584, x, 592);
    const SDL_Color labelColor = {128, 156, 172, 255};
    DrawCentered(r, fonts.log, "-60 dBFS", labelColor, 470, 615);
    DrawCentered(r, fonts.log, "-30 dBFS", labelColor, WINDOW_W / 2, 615);
    DrawCentered(r, fonts.log, "0 dBFS", labelColor, 1160, 615);
}

void DrawDashboard(SDL_Renderer *r, const FontSet &fonts, const AppState &state) {
    DrawStatusLine(r, fonts, state.mainController);
    MachineVisualState visual;
    if (state.mainController) {
        visual.controller = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(state.mainController));
        for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
            visual.axes[i] = SDL_GameControllerGetAxis(state.mainController, static_cast<SDL_GameControllerAxis>(i));
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
            visual.buttons[i] = SDL_GameControllerGetButton(state.mainController, static_cast<SDL_GameControllerButton>(i));
    }
    for (std::size_t i = 0; i < visual.customHighlights.size(); ++i)
        visual.customHighlights[i] = CustomKeyHighlighted(state, static_cast<CustomKey>(i));
    const bool machineChanged = !machineLayerValid || !(visual == cachedMachineState)
                                || state.log != cachedMachineLog;
    // 将整组机身 UI 作为透明图层等比放大,保留所有内部几何关系。
    if (!machineLayer) {
        machineLayer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                        SDL_TEXTUREACCESS_TARGET, WINDOW_W, WINDOW_H);
        if (machineLayer) SDL_SetTextureBlendMode(machineLayer, SDL_BLENDMODE_BLEND);
    }
    SDL_Texture *previousTarget = SDL_GetRenderTarget(r);
    if (machineLayer && machineChanged && SDL_SetRenderTarget(r, machineLayer) == 0) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
        DrawFrontOutline(r, fonts, state.mainController, state);
        SDL_SetRenderTarget(r, previousTarget);
        cachedMachineState = visual;
        cachedMachineLog = state.log;
        machineLayerValid = true;
    }
    if (machineLayer && machineLayerValid) {
        constexpr float scale = 1.05f;
        // 保留完整透明画布,避免裁掉画在 x+w / y+h 上的外框边线。
        const SDL_Rect source = {0, 0, WINDOW_W, WINDOW_H};
        const SDL_FRect destination = {
            810.0f + (source.x - 810.0f) * scale,
            600.0f + (source.y - 600.0f) * scale,
            source.w * scale, source.h * scale};
        SDL_RenderCopyF(r, machineLayer, &source, &destination);
    } else {
        DrawFrontOutline(r, fonts, state.mainController, state);
    }
    if (state.pointerVisible) {
        // 放大十字标记,中心仍对应实际输入坐标。
        const int radius = 30, thickness = 5;
        SDL_Rect horizontal = {state.pointerX - radius, state.pointerY - thickness / 2,
                               radius * 2 + 1, thickness};
        SDL_Rect vertical = {state.pointerX - thickness / 2, state.pointerY - radius,
                             thickness, radius * 2 + 1};
        SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 255);
        SDL_RenderFillRect(r, &horizontal);
        SDL_RenderFillRect(r, &vertical);
    }
    // 机身底部约 y=915,下方空白处只展示当前一条日志及 SDL 详情。
    DrawCentered(r, fonts.log, state.currentLog.empty() ? "(waiting for input)" : state.currentLog.c_str(),
                 C.white, WINDOW_W / 2, 963);
    DrawCentered(r, fonts.hint, "同时按住加号键、SELECT 和 START 退出程序。",
                 C.dimCyan, WINDOW_W / 2, WINDOW_H - 64);
    DrawCentered(r, fonts.hint, "Hold +, SELECT and START together to exit.",
                 C.dimCyan, WINDOW_W / 2, WINDOW_H - 24);
    DrawMicrophone(r, fonts, state);
}

void UIUnload() {
    for (const auto &entry : textCache) SDL_DestroyTexture(entry.texture);
    textCache.clear();
    if (machineLayer) SDL_DestroyTexture(machineLayer);
    machineLayer = nullptr;
    machineLayerValid = false;
    cachedMachineLog.clear();
}
