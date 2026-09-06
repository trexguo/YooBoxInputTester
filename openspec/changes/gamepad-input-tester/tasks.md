## 1. Project skeleton + build

- [x] 1.1 Create `src/main.cpp`, `src/app.{h,cpp}`, `src/ui.{h,cpp}`, `src/font.{h,cpp}` by moving the existing `main.cpp` bodies (app state & controller open loop → `app`, text/font helpers → `font`, drawing helpers → `ui`, entry point + loop → `main`). Verify the split files compile
- [x] 1.2 Update `CMakeLists.txt` (both native and cross branches) to list the new source files and add the `src/` include dir, and verify the native build compiles (`cmake --build build -j`)

## 2. Controller connection state

- [x] 2.1 In `app`, expose a single `SDL_GameController*` referencing the first connected controller (keep the open-all loop for multi-device), and verify the native build compiles with a `no controller` idle path
- [x] 2.2 Handle `SDL_CONTROLLERDEVICEADDED` / `SDL_CONTROLLERDEVICEREMOVED`: on add, open and (if none was driving the dashboard) adopt the new controller; on remove, close the disconnected one and drop to the no-controller idle state. Verify by running the build and confirming it stays up with no controller attached (background run + kill per CLAUDE.md)

## 3. Draw helpers (ui)

- [x] 3.1 Add a stick-gauge draw helper (circle outline + position dot + numeric X/Y label) and verify it renders both sticks in the neutral position with `x=0, y=0` on a fresh run
- [x] 3.2 Add a D-pad (hat) draw helper that highlights the active direction segment(s) and renders all segments idle at center; verify the four segments draw on a fresh run
- [x] 3.3 Add a trigger level-bar draw helper and verify it renders LT/RT at minimum level on a fresh run
- [x] 3.4 Add an active/inactive variant of the existing glow-rect style for buttons (fill/highlight when the button is held) and verify the build compiles

## 4. Per-frame state rendering (main loop)

- [x] 4.1 Each frame, read every game button via `SDL_GameControllerGetButton` and light the corresponding on-screen indicator; verify all buttons render idle when none are held on a fresh run
- [x] 4.2 Each frame, read the hat via `SDL_GameControllerGetButton(..., SDL_CONTROLLER_BUTTON_DPAD_*)` and drive the D-pad helper; verify the D-pad highlights direction segments as each is held
- [x] 4.3 Each frame, read both stick axes (`LEFTX/LEFTY`, `RIGHTX/RIGHTY`) and drive the stick gauges with a small render deadzone (numeric readout raw); verify the dot and numbers track a deflected stick
- [x] 4.4 Each frame, read both trigger axes and drive the level bars; verify they track partial trigger travel
- [x] 4.5 Compose the 1620×1080 dashboard (status line, button grid, D-pad, two stick gauges, trigger bars, log strip) from `ui` and verify the full screen renders on a fresh run

## 5. Event log

- [x] 5.1 Maintain a bounded ring buffer (last ~8 events) for button/hat/axis plus existing keyboard/touch/mouse, render it in the bottom strip, and verify the log shows new events and drops the oldest beyond the size

## 6. Build + verification

- [x] 6.1 Native build compiles cleanly and the app starts, stays up, and exits on ESC (background run + kill per CLAUDE.md; confirm `exit=143` and empty log)
- [x] 6.2 Cross build for RK3326 compiles (`cmake --build build_cross -j` using the README toolchain; if the toolchain path is unavailable on this machine, note it and verify only the native path)
- [ ] 6.3 Manually confirm each spec scenario is observable: button press+release, simultaneous multi-button, D-pad direction/diagonal/release, stick deflection+centered, trigger press+release, event log append, no-controller idle, ESC quits
