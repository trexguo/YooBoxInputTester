## 1. Vector outline helpers

- [x] 1.1 Add a rounded-rectangle draw helper (`DrawRoundedRect`, fill + outline via SDL primitives) to `src/ui.cpp` and verify the native build compiles (`cmake --build build -j`)
- [x] 1.2 Size/place the two outline views on the 1620×1080 canvas (thin top-edge strip + large front-face panel, with the event log at the bottom) and verify the native build compiles

## 2. Top outline (shoulder controls)

- [x] 2.1 Draw the top-edge outline strip with the left trigger (LT/ZL) and right trigger (RT/ZR) at the two ends, each with an analog depth bar, and verify the build compiles and the strip renders on a fresh run
- [x] 2.2 Add the L/R shoulder button indicators adjacent to each trigger on the top outline and verify they render on a fresh run

## 3. Front outline (face controls)

- [x] 3.1 Draw the front-face body outline (rounded rect silhouette) with a centered screen, and verify the build compiles and the silhouette renders on a fresh run
- [x] 3.2 Place the D-pad to the left of the screen and the ABXY cluster to the right of the screen (mirroring the physical layout), driven by the existing `DrawDPad`/`DrawButtonBox` helpers, and verify the build compiles
- [x] 3.3 Place the left stick below the D-pad and the right stick below the ABXY cluster via the existing `DrawStickGauge`, and verify they render with the numeric X/Y readout on a fresh run
- [x] 3.4 Place the Start/Select and L3/R3 indicators near the bottom of the front face, and verify the build compiles

## 4. Dashboard composition

- [x] 4.1 Reorder `DrawDashboard` to compose the status line, top outline, front outline (D-pad, ABXY, both sticks, Start/Select/L3/R3), and the event log in the new physical-layout arrangement, and verify the full screen renders on a fresh run
- [x] 4.2 Preserve the existing event log, no-controller idle hint, and ESC-to-quit behavior and verify with the smoke-test pattern (background run + kill per CLAUDE.md: the app starts, stays up, `exit=143`/137, empty log)
