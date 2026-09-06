## Why

The current `main.cpp` (the SDL2 "Hello, Yoobox Y1" demo) reports input as a single line of text describing the *last* event only. That is unusable for testing a handheld's game controls: you cannot see two buttons pressed at once, cannot tell a hat (D-pad) direction from a plain button event, and cannot read a stick's live position. What is needed is a real diagnostic view — the physical input layout on screen, with every button, trigger, D-pad direction, and stick continuously reflecting current state.

## What Changes

- Replace the single last-event text readout with a **live input state screen** tailored to the 1620×1080 Y1 display.
- Add a **button grid** rendering each game controller button (`A/B/X/Y`, `Start`, `Select`, shoulder `LB/RB`, `L3/R3` and any others the controller reports) that lights up while held.
- Add a **D-pad (hat) indicator** showing the current hat direction as an active cross segment.
- Add **two analog stick indicators** (`left`, `right`) plotting the stick position on a 2-D center/circle graphic, plus a live numeric readout of X/Y axis values.
- Add **trigger level bars** for `LT`/`RT` showing pressed depth (analog value, not just on/off).
- Add a compact **live event log** (last several events) as a secondary readout, since the visual state alone hides transient taps; the existing keyboard/touch/mouse/gamepad event text continues to be logged there.
- Keep the existing `Esc` quits, resources (`res/fonts/font.ttf`), background styling, and both build paths (native + RK3326 cross) working unchanged.

## Capabilities

### New Capabilities
- `gamepad-input-tester`: The hardware input diagnostic screen for the Yoobox Y1 — renders live state for controller buttons, D-pad, both analog sticks and triggers, plus a recent-event log, driven from `SDL_GameController`.

### Modified Capabilities
- None. No existing spec files reference this behavior; this capability is introduced fresh.

## Impact

- **`main.cpp` → `src/` module set**: the demo's single file is split into `src/main.cpp` (entry point + loop), `src/app.{h,cpp}` (app + controller connection state), `src/ui.{h,cpp}` (drawing widgets for buttons, D-pad, sticks, triggers, background) and `src/font.{h,cpp}` (text/font helpers). The text readout/render section is replaced by the state-mirroring layout; the `SDL_GameController*` open loop, `SDL_GameControllerUpdate` polling, video-init fallback and render-loop structure carry over.
- **`CMakeLists.txt`**: both branches list the new source files and add the `src/` include dir; the SDL2/pkg-config wiring, `SDL2main` filter, and cross-compile branch logic are unchanged.
- **`res/fonts/font.ttf`**: reused at existing sizes (and possibly one additional size for the log).
- **Dependencies**: no new libraries. SDL2 + SDL2_ttf + SDL2_image remain the only requirements.
- **Build**: `toolchain.cmake` unchanged. Both native (Linux/macOS) and RK3326 cross builds must still compile and link.
- **`README.md`**: likely a short note that the app is now an input tester (optional, out of spec scope).
