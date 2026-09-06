## Context

Current state: `main.cpp` is a single-file SDL2 demo that draws a cyber-grid/glow "Hello, Yoobox Y1" screen and prints the *last* input event as one line of text (via `KeyName`). It already opens `SDL_GameController`s at startup, inits video with the `wayland → cocoa → x11` fallback, and runs a poll-and-render loop. Resolution is fixed at 1620×1080; fonts are loaded from `res/fonts/font.ttf` at 72/52/32 px. There is no test harness — "verified" means it compiles (native + cross) and runs.

Note: a Quit/ESC handling requirement and a "no controller" idle-state requirement are both part of `spec.md` already, so the design assumes the app continues to honor ESC-to-quit and now renders an idle "no controller" hint rather than just running with empty indicators. (See proposal.md — Why.)

## Goals / Non-Goals

**Goals:**
- Render the full input state of an SDL game controller (buttons, D-pad, both sticks, both triggers) on the 1620×1080 screen, updating per frame.
- Keep the existing assets, styling (grid/glow), resolution, video-init fallback, and both build paths working.
- Keep the event log for transient/non-controller events (keyboard/touch/mouse, button sawtooth).
- Split the single file into a small set of modules so each concern (app state, rendering widgets, text, entry point) is separately testable and readable.

**Non-Goals:**
- No calibration, remapping, or deadzone-config UI for the sticks.
- No new third-party dependencies.
- No automated test suite — verification stays: it compiles and runs.
- No change to the cross-compile/toolchain path. (`CMakeLists.txt` is updated only to add the new sources/header dir — the SDL2/pkg-config wiring is untouched.)

## Decisions

### 1. Render pressed-state from current controller state, not from accumulated events
Each frame, read button/hat/axis state directly via `SDL_GameControllerGetButton` / `SDL_GameControllerGetAxis` / `SDL_GameControllerGetButton` for the hat and draw active/vs-idle from that. The event log is populated separately from the event queue.
- **Why**: The state functions reflect the true physical state, so simultaneous presses and stick position are always correct on the frame they're read — no bookkeeping to get out of sync, no missed-press drift.
- **Alternative considered**: Map `SDL_CONTROLLERBUTTONDOWN`/`UP` events into a state struct. Rejected — it duplicates state the API already exposes, and a lost UP event (window focus, event queue overflow) would leave a button stuck "on".

### 2. Screen layout
A fixed dashboard for 1620×1080: 
- Left/center: button grid (A/B/X/Y, Start, Select, shoulder buttons, L3/R3) — each an outlined box that fills/highlights when active.
- Right column: D-pad cross (four segments), then two stick graphics (circle + position dot + numeric X/Y), mirrored for left/right stick.
- Bottom strip: trigger level bars (LT, LT/RT) and the recent-event log (last ~6–8 lines) rendered small.
- Top line: status ("YooBox Input Tester" + connected/no-controller).
- **Why**: Keeps arms-length proportions and places the physical layout in a form a test technician reads at a glance; bottom strip is the only scrolling region.
- **Alternative considered**: Single centered canvas — rejected, leaves the wide aspect ratio under-used and mixes log with indicators.

### 3. Reuse existing helpers and styling
Keep `DrawCyberGrid`, `DrawGlowRect`, `RenderText`, the `Colors` struct. Add only small draw helpers (stick gauge, D-pad cross, level bar, active/inactive variants of the existing glow styles).
- **Why**: Minimizes churn, preserves the existing look, and the glow already gives clear active-vs-idle contrast.

### 4. Controller connection handling
A single `SDL_GameController*` for the first connected controller, re-checked at startup and on `SDL_CONTROLLERDEVICEADDED`/`REMOVED` events. When none is connected, draw all indicators idle plus a "no controller" hint. Maintain the existing open-all controllers loop if multiple are present, but drive the dashboard from the first.
- **Why**: Spec requires graceful idle (and graceful removal) behavior. Keeping the existing open-all loop avoids breaking multi-device polling.
- **Alternative considered**: Attempt to render a dashboard per controller — out of scope, extra complexity on a single-device handheld.

### 5. Split the single file into modules
Instead of one `main.cpp`, use a flat `src/` layout with headers alongside sources:
- `src/main.cpp` — entry point, SDL init/teardown, the poll-and-render loop.
- `src/app.h` / `src/app.cpp` — application state + game-controller connection state (open/close, ADDED/REMOVED handling, first-controller selection).
- `src/ui.h` / `src/ui.cpp` — drawing widgets: button grid, D-pad, stick gauges, trigger bars, cyber-grid background, glow rect, and the dashboard layout.
- `src/font.h` / `src/font.cpp` — text rendering + font resource helpers.

`CMakeLists.txt` (both branches) lists these sources and adds `src/` (or an include dir) so the headers resolve from the flat layout.
- **Why**: The single-file form was a demo convenience; the feature set (multiple widgets, controller lifecycle, event log) argues for separating concerns. The flat `src/` layout avoids the cost of a library split in a small app and keeps the edit surface predictable.
- **Alternatives considered**: One library + one executable (rejected — overkill for a single target); keeping everything in `main.cpp` (rejected per explicit request); per-widget files like `button.cpp`/`stick.cpp` (rejected — too granular, would fragment the existing `DrawCyberGrid`/`DrawGlowRect` helpers across many files).

## Risks / Trade-offs

- **Stick deadzone**: small stick drift around center is possible; a small deadzone for rendering (but not for the numeric readout) keeps the dot stable. Numeric values are still shown raw.
- **[Trivial rendering]** A crisp active/inactive distinction requires distinct colors; the existing glow uses alpha blending which needs `SDL_SetRenderDrawBlendMode` toggled — keep it as the current code does.
- **Event log growth**: if not capped, the log string grows unboundedly over a long session. Mitigation: fixed-size ring buffer (last N events) and clamp the rendered string length/font size.
- **No automated verification**: the only automated check is "compiles + starts + stays up". Mitigation: extend the existing smoke-test pattern (background-run + SIGTERM) and rely on manual visual confirmation of each spec scenario.

## Migration Plan

This replaces the demo text readout in place and swaps one `main.cpp` for a `src/` module set plus a `CMakeLists.txt` source-list update. No data or schema migration. Rollback = `git checkout` the moved `main.cpp` and `CMakeLists.txt`, rebuild native/cross. If `README.md` is touched, it's a doc-only change.
