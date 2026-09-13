# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## What this is

An SDL2 handheld-gamepad input tester targeting a specific embedded device: the **Yoobox Y1**, a 4.5" RK3326 (ARM aarch64) Linux handheld with a **1620×1080** screen. The window size and layout (grid, glow, text placement) are hardcoded around that resolution.

The entry point is `src/main.cpp`, backed by `src/app.*` (state, controller open/handle, event log), `src/ui.*` (all dashboard rendering), and `src/font.*` (TTF load/render). There is no test suite and no linting — it is a handful of source files plus a CMake build.

The rendered layout is a hardcoded dashboard: device body outline, top trigger row (LB/LT/Fn1/Fn2/RT/RB), left D-pad + right ABXY (circular, radius 66), two stick gauges (radius 70, L3/R3 press), bottom keys (`+`/`Fn3` left, `SELECT`/`START` right), and a central 4:3 "screen" (`{503, 340, 613, 460}`) that fills with the event log. Everything is mirrored about the midline x=810 — keep geometry symmetric when editing.

## Build

Two entirely separate paths exist inside `CMakeLists.txt`, chosen by whether CMAKE_CROSSCOMPILING is set (i.e. whether you pass `-DCMAKE_TOOLCHAIN_FILE=`):

- **Native** (Linux or macOS for development): uses `pkg-config` (`find_package(PkgConfig REQUIRED)` + `pkg_check_modules`). Requires `sdl2` and `SDL2_ttf` present in the pkg-config path (SDL_image is **not** used — the logo was removed, so it is not checked or linked). On macOS from Homebrew, `sdl2` alone is commonly installed but `sdl2_ttf` is not — the configure step fails outright because the check is `REQUIRED`. Fix is `brew install sdl2_ttf` (full walkthrough in README's macOS section). Note that Homebrew's `sdl2` package is now provided by **`sdl2-compat`** (pkg-config Name: `sdl2_compat`), which is why the `SDL2main` filter below matters.
- **Cross** (RK3326 target): the `if(CMAKE_CROSSCOMPILING)` branch reads include/lib dirs straight out of `CMAKE_SYSROOT` and **skips pkg-config entirely**. This is intentional — without it, pkg-config would grab the host's x86_64 SDL2 and the link would be wrong.

```sh
# Native dev build (Linux/macOS)
cmake -S . -B build && cmake --build build -j

# Cross-compile to RK3326 (see README for full toolchain details)
cmake -B build_cross -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
cmake --build build_cross -j
```

There is no test/lint step, so "verify" means: it compiles, and it runs. The binary is a GUI app that blocks in the SDL event loop until you close the window, so it can't be run to completion in an automated check — to confirm it starts (and that `res/` loads), run it in the background and kill it after a couple of seconds:

```sh
./build/YooBoxInputTester > /tmp/run.log 2>&1 & pid=$!; sleep 3; kill $pid; wait $pid; echo "exit=$?"; cat /tmp/run.log
```

`exit=143` (SIGTERM) plus an empty log means it started fine and stayed up; `exit=1` means a fatal error (e.g. font missing).

### macOS specifics

- **Deployment-target linker warning**: with Homebrew's SDL dylibs, linking spits `ld: warning: dylib (...libSDL2*.dylib) was built for newer macOS version (26.0) than being linked (14.0)`. Harmless, but to silence it configure with `-DCMAKE_OSX_DEPLOYMENT_TARGET=26.0` (match the dylib version, or set a lower value if you need older-macOS compat and accept the warning). `CMakeLists.txt` sets no deployment target, so it falls back to the compiler default.
- **Run from the repo root** so `res/` resolves (see assets note below).

## Non-obvious facts to know before editing

- **The toolchain path is hardcoded absolute**: `toolchain.cmake` sets `TOOLCHAIN_DIR` to `/home/ice/distribution/build.ROCKNIX-RK3326.aarch64/toolchain` (a ROCKNIX build tree on another machine, still using user `ice`). Cross-compiling here only works if that exact path exists; otherwise expect missing-compiler or missing-sysroot errors. Hardcoding it also makes the build **non-hermetic**, so guard against surprising it.

- **macOS `SDL2main` filter**: on the native branch, the code strips `SDL2main` out of the SDL2 lib list (`list(FILTER _SDL2_LIBS EXCLUDE REGEX "SDL2main")`). The Homebrew `sdl2-compat` `.pc` declares `-lSDL2main` without shipping that library, so this filter is load-bearing. Don't remove it. The app uses a plain `int main()`, so it genuinely doesn't need `SDL2main`.

- **Headers are included as `<SDL2/SDL.h>`**, but the include dirs point at `.../SDL2` (e.g. `${TOOLCHAIN_SYSROOT}/usr/include/SDL2` on cross, `SDL2_INCLUDE_DIRS` on native). Keep this pairing in mind: the right include path is the SDK's `include/SDL2` directory, and the source uses the `SDL2/`-prefixed form.

- **Only the font is loaded at runtime, relative to the working directory** — `"res/fonts/font.ttf"`. The path is not embedded and there is no resource compiler. A missing font makes `TTF_OpenFont` return null and the app exits with code 1. Run from the repo root so `res/` resolves. (The app is pure SDL2 + SDL2_ttf; `SDL2_image` is not linked at all, so the `res/images/` tree is never read.)

- **Triggers (LT/RT) have no SDL button constant** — SDL exposes them as **axes only** (`SDL_CONTROLLER_AXIS_TRIGGERLEFT/RIGHT`); there is no `SDL_CONTROLLER_BUTTON_TRIGGERLEFT` in the header. The top-row `DrawTopTriggers` draws their analog depth from the axis value, and computes their "pressed" highlight from `|axis| > ~10%` (≈3277/32767), with `btn` set to `SDL_CONTROLLER_BUTTON_INVALID`. Don't bind a shoulder button constant to a trigger (that makes LB and LT light up together), and don't try to call `SDL_GameControllerGetButton` on a trigger.

- **VIDEO driver fallback is hardcoded**: `main()` tries `wayland` → `cocoa` → `x11` via `SDL_setenv` before `SDL_Init`. This is tuned to the target device's base OS (Rocknix) and to desktop testing. If you add more display backends or change how SDL is initialized, keep that fallback chain's ordering in mind.

- **Static linking is only partially supported**: the CMake toolchain file sets `CMAKE_EXE_LINKER_FLAGS_INIT` with `-rpath`/`-L`, but `-static` is not wired up anywhere. README's "static link" section asks for it via `-DCMAKE_EXE_LINKER_FLAGS="-static"` — and that only works if the sysroot actually contains `.a` files. Don't assume static is available.

- **Bottom keys `+` and `Fn3` are currently unbound**: they render but their active state is hardcoded to `false` in `DrawButtonBox` (bindings TBD by the user). `SELECT`/`START` are bound to `SDL_CONTROLLER_BUTTON_BACK`/`START` respectively. When wiring up `+`/`Fn3` later, replace the `false` with a `ButtonHeld(...)` and, if a distinct event is wanted, add it to the `KeyName` mapping in `src/app.cpp`.

## Assets

```
res/
└── fonts/font.ttf      # single TrueType font, used at 3 sizes (72/52/32 px)
```

The `res/images/` tree is no longer read (the logo was dropped); `SDL2_image` is not a dependency.

## Docs

`README.md` is the canonical reference and is far more detailed than this file — especially on **Ubuntu dependency install**, the **full RK3326 cross-compile walkthrough**, and the **deployment step (scp/adb/NFS)**. When in doubt, read the README. The repo uses the `main` branch; there are no feature-branch conventions.
