#include "app.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

static const std::size_t LOG_MAX = 8;

static const char *AxisName(Sint16 axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:  return "LeftX";
        case SDL_CONTROLLER_AXIS_LEFTY:  return "LeftY";
        case SDL_CONTROLLER_AXIS_RIGHTX: return "RightX";
        case SDL_CONTROLLER_AXIS_RIGHTY: return "RightY";
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  return "TriggerL";
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return "TriggerR";
        default: return "unknown";
    }
}

static const char *ButtonName(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:              return "A";
        case SDL_CONTROLLER_BUTTON_B:              return "B";
        case SDL_CONTROLLER_BUTTON_X:              return "X";
        case SDL_CONTROLLER_BUTTON_Y:              return "Y";
        case SDL_CONTROLLER_BUTTON_BACK:           return "SELECT";   // UI 底排的 SELECT
        case SDL_CONTROLLER_BUTTON_GUIDE:          return "GUIDE";
        case SDL_CONTROLLER_BUTTON_START:          return "START";    // UI 底排的 START
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:      return "L3";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:     return "R3";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:   return "LB";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:  return "RB";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:        return "DpadUp";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:      return "DpadDown";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:      return "DpadLeft";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:     return "DpadRight";
        default: return nullptr;
    }
}

void AppLog(AppState &st, const std::string &s) {
    st.log.push_back(s);
    while (st.log.size() > LOG_MAX)
        st.log.pop_front();
}

void PointerHandleEvent(AppState &st, const SDL_Event &e) {
    switch (e.type) {
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP:
            if (!std::isfinite(e.tfinger.x) || !std::isfinite(e.tfinger.y)) return;
            st.pointerX = std::min(WINDOW_W - 1,
                static_cast<int>(std::clamp(e.tfinger.x, 0.0f, 1.0f) * WINDOW_W));
            st.pointerY = std::min(WINDOW_H - 1,
                static_cast<int>(std::clamp(e.tfinger.y, 0.0f, 1.0f) * WINDOW_H));
            break;
        case SDL_MOUSEMOTION:
            if (e.motion.which == SDL_TOUCH_MOUSEID) return;
            st.pointerX = e.motion.x;
            st.pointerY = e.motion.y;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (e.button.which == SDL_TOUCH_MOUSEID) return;
            st.pointerX = e.button.x;
            st.pointerY = e.button.y;
            break;
        default:
            return;
    }
    st.pointerVisible = true;
}

bool ExitComboHeld(const AppState &st, bool selectHeld, bool startHeld) {
    return CustomKeyHeld(st, CustomKey::Plus) && selectHeld && startHeld;
}

SDL_GameControllerButton ControllerButtonForDevice(SDL_GameController *gc, SDL_GameControllerButton button) {
    if (!gc) return button;
    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gc);
    const char *name = joystick ? SDL_JoystickName(joystick) : nullptr;
    if (!name || std::string(name) != "play_joystick") return button;
    if (button == SDL_CONTROLLER_BUTTON_X) return SDL_CONTROLLER_BUTTON_Y;
    if (button == SDL_CONTROLLER_BUTTON_Y) return SDL_CONTROLLER_BUTTON_X;
    return button;
}

std::string KeyName(SDL_Event &e, bool suppressMouse) {
    if (suppressMouse && e.type >= SDL_MOUSEMOTION && e.type <= SDL_MOUSEWHEEL)
        return "";
    switch (e.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            const char *name = SDL_GetKeyName(e.key.keysym.sym);
            std::string s = name ? name : "?";
            if (e.key.repeat) s += " (repeat)";
            s += (e.type == SDL_KEYUP) ? " [UP]" : " [DOWN]";
            return s;
        }
        case SDL_TEXTINPUT:
            return std::string("Text: ") + e.text.text;
        case SDL_FINGERDOWN:
            return std::string("Touch DOWN (") + std::to_string((int)(e.tfinger.x * WINDOW_W))
                   + ", " + std::to_string((int)(e.tfinger.y * WINDOW_H)) + ")";
        case SDL_FINGERUP:
            return std::string("Touch UP (") + std::to_string((int)(e.tfinger.x * WINDOW_W))
                   + ", " + std::to_string((int)(e.tfinger.y * WINDOW_H)) + ")";
        case SDL_FINGERMOTION:
            return std::string("Touch MOVE (") + std::to_string((int)(e.tfinger.x * WINDOW_W))
                   + ", " + std::to_string((int)(e.tfinger.y * WINDOW_H)) + ")";
        case SDL_MOUSEBUTTONDOWN:
            return std::string("Mouse DOWN (") + std::to_string(e.button.x) + ", " + std::to_string(e.button.y) + ")";
        case SDL_MOUSEBUTTONUP:
            return std::string("Mouse UP (") + std::to_string(e.button.x) + ", " + std::to_string(e.button.y) + ")";
        case SDL_MOUSEMOTION:
            return std::string("Mouse MOVE (") + std::to_string(e.motion.x) + ", " + std::to_string(e.motion.y) + ")";
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            SDL_GameController *gc = SDL_GameControllerFromInstanceID(e.cbutton.which);
            const auto button = ControllerButtonForDevice(gc,
                static_cast<SDL_GameControllerButton>(e.cbutton.button));
            const char *n = ButtonName(static_cast<Uint8>(button));
            char buf[96];
            std::snprintf(buf, sizeof(buf), "%s [%s]",
                          n ? n : SDL_GameControllerNameForIndex(e.cdevice.which),
                          e.type == SDL_CONTROLLERBUTTONUP ? "UP" : "DOWN");
            return "Btn " + std::string(buf);
        }
        case SDL_CONTROLLERAXISMOTION: {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "%s = %d", AxisName(e.caxis.axis), e.caxis.value);
            return "Axis " + std::string(buf);
        }
        default:
            return "";
    }
}

void ControllersOpenAll(AppState &st) {
    st.mainController = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i)) continue;
        SDL_GameController *c = SDL_GameControllerOpen(i);
        if (!st.mainController) st.mainController = c;
    }
}

bool ControllersHandleEvent(AppState &st, SDL_Event &e) {
    switch (e.type) {
        case SDL_CONTROLLERDEVICEADDED: {
            SDL_GameController *c = SDL_GameControllerOpen(e.cdevice.which);
            if (c && !st.mainController) st.mainController = c;
            return true;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            SDL_GameController *c = SDL_GameControllerFromInstanceID(e.cdevice.which);
            if (c) {
                if (c == st.mainController) st.mainController = nullptr;
                SDL_GameControllerClose(c);
            }
            return true;
        }
        default:
            return false;
    }
}

bool CustomKeyHeld(const AppState &st, CustomKey key) {
    const auto index = static_cast<std::size_t>(key);
    return index < st.customKeys.size() && st.customKeys[index];
}

bool CustomKeyHighlighted(const AppState &st, CustomKey key) {
    const auto index = static_cast<std::size_t>(key);
    if (index >= st.customKeys.size()) return false;
    return st.customKeys[index] ||
           (st.customKeyHighlightUntil[index] != 0 &&
            !SDL_TICKS_PASSED(SDL_GetTicks(), st.customKeyHighlightUntil[index]));
}

void CustomKeysClose(AppState &st) {
#ifdef __linux__
    if (st.customInputFd >= 0) close(st.customInputFd);
    if (st.fn2AdcFd >= 0) close(st.fn2AdcFd);
#endif
    st.customInputFd = -1;
    st.customInputDropped = false;
    st.fn2AdcFd = -1;
    st.fn2AdcHeld = st.fn2EvdevHeld = false;
    st.customKeys.fill(false);
    st.customKeyHighlightUntil.fill(0);
}

#ifdef __linux__
namespace {
struct CustomBinding { CustomKey key; unsigned short code; const char *name; };
const CustomBinding customBindings[] = {
    {CustomKey::Fn1, 615, "Fn1"}, // KEY_RIGHT_DOWN:设备实测
    {CustomKey::Fn2, 614, "Fn2"}, // KEY_RIGHT_UP:暂定映射,本机可靠输入来自 ADC3
    {CustomKey::Fn3, KEY_HOME, "Fn3"},
    {CustomKey::Plus, KEY_FN, "+"},
};

void SetCustomKey(AppState &st, const CustomBinding &binding, bool held, bool log,
                  const char *source = nullptr) {
    auto &current = st.customKeys[static_cast<std::size_t>(binding.key)];
    if (current == held) return; // 不重复记录自动连发
    current = held;
    // 短脉冲可能在一帧内完成按下和松开;仅延长显示,不改变实际按键状态。
    if (held && log) {
        Uint32 until = SDL_GetTicks() + 250;
        st.customKeyHighlightUntil[static_cast<std::size_t>(binding.key)] = until ? until : 1;
    }
    if (log) {
        char text[80];
        if (source)
            std::snprintf(text, sizeof(text), "Btn %s [%s] (%s)",
                          binding.name, held ? "DOWN" : "UP", source);
        else
            std::snprintf(text, sizeof(text), "Btn %s [%s] (evdev %u)",
                          binding.name, held ? "DOWN" : "UP", binding.code);
        AppLog(st, text);
        SDL_Log("%s", text);
    }
}

bool SyncCustomKeys(AppState &st, bool log) {
    unsigned char held[(KEY_MAX + 8) / 8] = {};
    if (ioctl(st.customInputFd, EVIOCGKEY(sizeof(held)), held) < 0) return false;
    for (const auto &binding : customBindings) {
        bool down = (held[binding.code / 8] & (1u << (binding.code % 8))) != 0;
        if (binding.key == CustomKey::Fn2) {
            st.fn2EvdevHeld = down;
            down = down || st.fn2AdcHeld;
        }
        SetCustomKey(st, binding, down, log);
    }
    return true;
}

// 本机实测 Fn2:ffaa0000.saradc 通道3,松开约1023,按下683–684。
// START 同通道使用较低电平,因此仅识别 Fn2 的窄范围并加回滞。
bool Fn2AdcPressed(int value, bool held) {
    return held ? value >= 620 && value <= 750 : value >= 650 && value <= 720;
}

void OpenFn2Adc(AppState &st) {
    char compatible[128] = {};
    int fd = open("/sys/firmware/devicetree/base/compatible", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    const ssize_t count = read(fd, compatible, sizeof(compatible));
    close(fd);
    bool target = false;
    for (ssize_t i = 0; i < count; ++i)
        if ((i == 0 || compatible[i - 1] == '\0') &&
            count - i >= 16 && std::memcmp(compatible + i, "rockchip,rk3562", 15) == 0)
            target = true;
    if (!target) return;
    DIR *dir = opendir("/sys/bus/iio/devices");
    if (!dir) return;
    while (auto *entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "iio:device", 10) != 0) continue;
        const std::string base = std::string("/sys/bus/iio/devices/") + entry->d_name;
        fd = open((base + "/name").c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;
        char name[64] = {};
        const ssize_t size = read(fd, name, sizeof(name) - 1);
        close(fd);
        if (size <= 0 || std::strcmp(name, "ffaa0000.saradc\n") != 0) continue;
        st.fn2AdcFd = open((base + "/in_voltage3_raw").c_str(), O_RDONLY | O_CLOEXEC);
        if (st.fn2AdcFd >= 0) {
            SDL_Log("Fn2: monitoring %s ADC3 (measured range 650-720)", name);
            break;
        }
    }
    closedir(dir);
}

void PollFn2Adc(AppState &st) {
    if (st.fn2AdcFd < 0) return;
    char raw[32] = {};
    const ssize_t size = pread(st.fn2AdcFd, raw, sizeof(raw) - 1, 0);
    if (size < 0 && errno == EINTR) return;
    char *end = nullptr;
    const long value = size > 0 ? std::strtol(raw, &end, 10) : -1;
    const bool valid = size > 0 && end != raw && (*end == '\n' || *end == '\0') &&
                       value >= 0 && value <= 1023;
    st.fn2AdcHeld = valid && Fn2AdcPressed(static_cast<int>(value), st.fn2AdcHeld);
    SetCustomKey(st, customBindings[1], st.fn2AdcHeld || st.fn2EvdevHeld, true, "ADC3");
    if (!valid) {
        close(st.fn2AdcFd);
        st.fn2AdcFd = -1;
        SDL_Log("Fn2: ADC3 read failed; disabled ADC monitoring");
    }
}

void OpenCustomKeys(AppState &st) {
    DIR *dir = opendir("/dev/input");
    if (!dir) return;
    while (auto *entry = readdir(dir)) {
        if (std::strncmp(entry->d_name, "event", 5) != 0) continue;
        const std::string path = std::string("/dev/input/") + entry->d_name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        char name[128] = {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
            std::strcmp(name, "play_joystick") == 0) {
            st.customInputFd = fd;
            SyncCustomKeys(st, false);
            OpenFn2Adc(st);
            SDL_Log("Custom keys: monitoring %s (%s)", name, path.c_str());
            break;
        }
        close(fd);
    }
    closedir(dir);
}
} // namespace
#endif

void CustomKeysPoll(AppState &st) {
#ifdef __linux__
    if (st.customInputFd < 0) {
        const Uint32 now = SDL_GetTicks();
        if (!SDL_TICKS_PASSED(now, st.customInputNextScan)) return;
        st.customInputNextScan = now + 2000;
        OpenCustomKeys(st);
        if (st.customInputFd < 0) return;
    }
    PollFn2Adc(st);
    input_event events[32];
    // 限制单帧读取量,避免摇杆持续上报时阻塞渲染。
    for (int batch = 0; batch < 32; ++batch) {
        const ssize_t bytes = read(st.customInputFd, events, sizeof(events));
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (bytes <= 0) {
            for (const auto &binding : customBindings) SetCustomKey(st, binding, false, true);
            CustomKeysClose(st);
            break;
        }
        for (std::size_t i = 0; i < static_cast<std::size_t>(bytes) / sizeof(input_event); ++i) {
            const auto &event = events[i];
            if (event.type == EV_SYN && event.code == SYN_DROPPED) {
                st.customInputDropped = true;
                continue;
            }
            if (st.customInputDropped) {
                if (event.type == EV_SYN && event.code == SYN_REPORT) {
                    if (!SyncCustomKeys(st, true)) {
                        CustomKeysClose(st);
                        return;
                    }
                    st.customInputDropped = false;
                }
                continue;
            }
            if (event.type != EV_KEY) continue;
            for (const auto &binding : customBindings) {
                if (event.code != binding.code) continue;
                bool down = event.value != 0;
                if (binding.key == CustomKey::Fn2) {
                    st.fn2EvdevHeld = down;
                    down = down || st.fn2AdcHeld;
                }
                SetCustomKey(st, binding, down, true);
            }
        }
    }
#else
    (void)st;
#endif
}
