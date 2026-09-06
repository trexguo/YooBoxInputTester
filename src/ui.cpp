#include "ui.h"

#include "app.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

static const float PI = 3.14159265f;

static bool ButtonHeld(SDL_GameController *gc, SDL_GameControllerButton btn) {
    return gc && SDL_GameControllerGetButton(gc, btn) != 0;
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

// 文本居中到 (cx, cy);返回高度;出错返回 0
static int DrawCentered(SDL_Renderer *r, TTF_Font *font, const char *s,
                        SDL_Color col, int cx, int cy) {
    SDL_Texture *t = RenderText(r, font, s, col);
    if (!t) return 0;
    int w, h;
    SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
    SDL_Rect d = {cx - w / 2, cy - h / 2, w, h};
    SDL_RenderCopy(r, t, nullptr, &d);
    SDL_DestroyTexture(t);
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
    SDL_Texture *titleTex = RenderText(r, fonts.title, title, C.cyan);
    int tw = 0, th = 0;
    if (titleTex) {
        SDL_QueryTexture(titleTex, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {WINDOW_W / 2 - tw / 2, 62 - th / 2, tw, th};
        SDL_RenderCopy(r, titleTex, nullptr, &d);
        SDL_DestroyTexture(titleTex);
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
    SDL_Texture *t = RenderText(r, fonts.hint, msg, col);
    if (t) {
        int tw, th;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {WINDOW_W / 2 - tw / 2, box.y + (box.h - th) / 2, tw, th};
        SDL_RenderCopy(r, t, nullptr, &d);
        SDL_DestroyTexture(t);
    }
}

// 圆形按键:填充圆 + 发光描边 + 中心标签。(cx,cy) 为圆心,d 为直径。
static void DrawRoundButton(SDL_Renderer *r, TTF_Font *font, const char *label,
                            int cx, int cy, int d, bool active) {
    const int n = 48, rad = d / 2;
    SDL_Point ring[48];
    for (int i = 0; i < n; ++i) {
        float a = (float)i * 2.0f * PI / n;
        ring[i] = {cx + (int)(rad * cosf(a)), cy + (int)(rad * sinf(a))};
    }
    // 按压时填充
    if (active) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, C.magenta.r, C.magenta.g, C.magenta.b, 70);
        SDL_Rect fill = {cx - rad, cy - rad, d, d};
        SDL_RenderFillRect(r, &fill);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
    // 发光外环
    SDL_Color col = active ? C.magenta : C.dimCyan;
    for (int ringi = 3; ringi >= 1; --ringi) {
        SDL_Point g[48];
        int gr = rad + ringi * 2;
        for (int i = 0; i < n; ++i) {
            float a = (float)i * 2.0f * PI / n;
            g[i] = {cx + (int)(gr * cosf(a)), cy + (int)(gr * sinf(a))};
        }
        // 外圈淡色 → 内圈亮
        int alpha = active ? 90 - ringi * 25 : 40 - ringi * 10;
        if (alpha < 0) alpha = 0;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, (Uint8)alpha);
        SDL_RenderDrawLines(r, g, n);
    }
    // 主描边
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, 255);
    SDL_RenderDrawLines(r, ring, n);
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
    const int off = d + 8;   // 相邻方向键中心间距(紧凑)
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
    const int off = d + 8;   // 相邻按键中心间距(紧凑)
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
    SDL_Point pts[48];
    for (int i = 0; i < n; ++i) {
        float ang = (float)i * 2.0f * PI / n;
        pts[i] = {cx + (int)(radius * cosf(ang)), cy + (int)(radius * sinf(ang))};
    }
    SDL_SetRenderDrawColor(r, C.dimCyan.r, C.dimCyan.g, C.dimCyan.b, 255);
    SDL_RenderDrawLines(r, pts, n);

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

// 事件日志:渲染在中央"屏幕"区域内,内容垂直/水平居中,显示最近 LOG_MAX 条。
static void DrawLog(SDL_Renderer *r, const FontSet &fonts, const std::deque<std::string> &log) {
    SDL_Rect screen = {503, 340, 613, 460};   // 与 DrawFrontOutline 的屏幕一致;中心 x=810
    // 底衬,让文字在屏幕上可读
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_RenderFillRect(r, &screen);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // 取要显示的最近 8 条
    std::deque<std::string> vis;
    int start = (int)log.size() - 8;
    if (start < 0) start = 0;
    for (auto it = log.begin() + start; it != log.end(); ++it) vis.push_back(*it);

    const int ln = 30;
    int n = (int)vis.size();
    if (n == 0) n = 1;                       // 空时显示提示占一行
    const int headH = 34;                    // 标题行高
    const int totalH = headH + 10 + n * ln;  // 标题 + 间距 + 行数
    int top = screen.y + (screen.h - totalH) / 2;

    DrawSectionTitle(r, fonts, "EVENT LOG", WINDOW_W / 2, top + headH / 2);
    int y = top + headH + 10 + ln / 2;
    if (vis.empty()) {
        DrawCentered(r, fonts.hint, "(waiting for input)", C.dimCyan, WINDOW_W / 2, y);
    } else {
        for (int i = 0; i < (int)vis.size(); ++i) {
            DrawCentered(r, fonts.hint, vis[i].c_str(), C.white, WINDOW_W / 2, y);
            y += ln;
        }
    }
}

// 顶部按键:位于机身外框上方。LB(左肩)、LT(左扳机)对称于 R2→RB(右肩)、RT(右扳机),
// 中间为 Fn1 / Fn2。肩键=数字按下;扳机=模拟深度条 + 按下高亮。
static void DrawTopTriggers(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc) {
    struct Key {
        const char *label;
        SDL_GameControllerAxis ax;   // 非触发键用 SDL_CONTROLLER_AXIS_INVALID(不画深度)
        SDL_GameControllerButton btn;   // 触发键无独立按钮,置 INVALID,按下状态改由轴向判断
    };
    static const Key keys[6] = {
        {"LB",  SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {"LT",  SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_BUTTON_INVALID},
        {"Fn1", SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_GUIDE},
        {"Fn2", SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_GUIDE},
        {"RT",  SDL_CONTROLLER_AXIS_TRIGGERRIGHT, SDL_CONTROLLER_BUTTON_INVALID},
        {"RB",  SDL_CONTROLLER_AXIS_INVALID,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
    };
    const int bw = 104, bh = 66, gap = 16;
    const int y0 = 226;                       // 位于机身顶部(y=300)之上 → 外框之外
    // 左组(LB,LT)中心=290;fn1 在 LT 右侧留一段距离,fn2 在屏中线对称位置
    const int leftCX = 290;
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
        // 模拟触发深度(仅触发键):从底向上填充
        if (keys[i].ax != SDL_CONTROLLER_AXIS_INVALID) {
            int pct = std::clamp((int)(AxisVal(gc, keys[i].ax) * 100 / 32767), 0, 100);
            SDL_SetRenderDrawColor(r, 0, 30, 40, 180);
            SDL_RenderFillRect(r, &b);
            if (pct > 0) {
                SDL_SetRenderDrawColor(r, C.yellow.r, C.yellow.g, C.yellow.b, 255);
                SDL_Rect fill = {b.x, b.y + b.h - b.h * pct / 100, b.w, b.h * pct / 100};
                SDL_RenderFillRect(r, &fill);
            }
        } else {
            // 功能键:纯底色
            SDL_SetRenderDrawColor(r, 10, 14, 24, 255);
            SDL_RenderFillRect(r, &b);
        }
        // 触发键在无独立按钮时,按下状态由轴向超过阈值判断(避免与肩键共用同一按钮常量而联动)
        bool a;
        if (keys[i].btn != SDL_CONTROLLER_BUTTON_INVALID)
            a = ButtonHeld(gc, keys[i].btn);
        else
            a = std::abs(AxisVal(gc, keys[i].ax)) > 3277;   // ~10% 行程视为按下
        SDL_Color col = a ? C.magenta : C.dimCyan;
        DrawGlowRect(r, b, col, a ? 4 : 2);
        DrawCentered(r, fonts.hint, keys[i].label, a ? C.white : C.dimCyan,
                     b.x + b.w / 2, b.y + b.h / 2);
    }
}

// 正面轮廓:主体圆角矩形 + 中央屏幕(内为事件日志)+ 底缘小键。
static void DrawFrontOutline(SDL_Renderer *r, const FontSet &fonts, SDL_GameController *gc,
                             const std::deque<std::string> &log) {
    SDL_Rect body = {90, 300, WINDOW_W - 180, 600};   // 缩窄:左右各留 90,高度略收
    DrawRoundedRect(r, body, 46, (SDL_Color){34, 38, 58, 255}, true);
    DrawRoundedRect(r, body, 46, C.dimCyan, false);

    // 顶部扳机键(LT / L2 / RT / R2)
    DrawTopTriggers(r, fonts, gc);

    // 中央屏幕(4:3,中心与整机中线 x=810 对正;下缘避开底排按键 y=812)
    SDL_Rect screen = {503, 340, 613, 460};
    DrawRoundedRect(r, screen, 20, (SDL_Color){6, 6, 12, 255}, true);

    // 两组圆形按键:直径与间距完全一致,左右列中线对齐(内移后仍对称)
    DrawDPad(r, fonts, gc, 290, 444, 66, nullptr);            // 左:D-pad 四圆键
    DrawABXY(r, fonts, gc, 1330, 444, 66);                    // 右:ABXY 四圆键 → 与左 D-pad 镜像

    DrawStickGauge(r, fonts, nullptr, gc,                     // 左摇杆(D-pad 下)
                   SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, 290, 650, 70,
                   "L3", SDL_CONTROLLER_BUTTON_LEFTSTICK);
    DrawStickGauge(r, fonts, nullptr, gc,                     // 右摇杆(ABXY 下)→ 与左摇杆镜像
                   SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY, 1330, 650, 70,
                   "R3", SDL_CONTROLLER_BUTTON_RIGHTSTICK);

    // 底缘:两列各自紧凑居中 —— 左列(`+`/`Fn3`)中心=290,右列(`SELECT`/`START`)中心=1330
    const int keyw = 100, keygap = 14;
    SDL_Rect plus = {290 - keyw - keygap / 2, 812, keyw, 44};   // +   左列左
    SDL_Rect fn3  = {290 + keygap / 2,        812, keyw, 44};   // Fn3 左列右
    SDL_Rect stan = {1330 - keyw - keygap / 2, 812, keyw, 44};  // SELECT 右列左
    SDL_Rect seln = {1330 + keygap / 2,        812, keyw, 44};  // START 右列右
    DrawButtonBox(r, fonts.hint, "+",    plus, false);          // 键值待定,当前不绑定
    DrawButtonBox(r, fonts.hint, "Fn3",  fn3,  false);          // 键值待定,当前不绑定
    DrawButtonBox(r, fonts.hint, "SELECT",  stan, ButtonHeld(gc, SDL_CONTROLLER_BUTTON_BACK));
    DrawButtonBox(r, fonts.hint, "START", seln, ButtonHeld(gc, SDL_CONTROLLER_BUTTON_START));

    // 事件日志画在屏幕区域上
    DrawLog(r, fonts, log);
}

void DrawDashboard(SDL_Renderer *r, const FontSet &fonts,
                   SDL_GameController *mainController,
                   const std::deque<std::string> &log) {
    DrawStatusLine(r, fonts, mainController);
    DrawFrontOutline(r, fonts, mainController, log);
}
