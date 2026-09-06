#include "app.h"

#include <cstdio>

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
        case SDL_CONTROLLER_BUTTON_GUIDE:          return "Fn";       // UI 顶排的 Fn1/Fn2 共用此值
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

std::string KeyName(SDL_Event &e) {
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
            const char *n = ButtonName(e.cbutton.button);
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
