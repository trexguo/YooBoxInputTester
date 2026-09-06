## Purpose

Provides a Yoobox Y1 handheld diagnostic view that mirrors the device's actual physical layout, so a tester can map the on-screen control indicators directly to the real buttons, sticks, and triggers.

## ADDED Requirements

### Requirement: Render the device front outline

The system SHALL render a vector-drawn outline of the Yoobox Y1 handheld front face, showing the body contour, the centered screen, and the positions of the D-pad (left), right ABXY button cluster, left and right analog sticks, and the Start/Select and L3/R3 buttons. The outline SHALL be drawn from vector shapes rather than an image asset.

#### Scenario: Front outline shown on a fresh run

- **WHEN** the app starts and renders the dashboard
- **THEN** a front-face outline is visible with the D-pad on the left, the ABXY cluster on the right, the screen centered, and both analog sticks below the D-pad and ABXY cluster respectively

### Requirement: Render the device top outline

The system SHALL render a vector-drawn outline of the Yoobox Y1 handheld top edge that places the left shoulder controls (ZL/L) and right shoulder controls (ZR/R) at their physical left/right positions, and SHALL reflect the pressed depth of the analog triggers on the top outline.

#### Scenario: Top outline shown on a fresh run

- **WHEN** the app starts
- **THEN** a top-edge outline is visible with the left and right trigger/shoulder controls at the two ends

### Requirement: Place controls at their physical positions

The system SHALL position each control indicator on the outline at the location that corresponds to the physical control on the device: the D-pad to the left of the screen with the left stick below it, the ABXY cluster to the right of the screen with the right stick below it, Start/Select and L3/R3 near the bottom, and the shoulder controls on the top edge.

#### Scenario: Control positions reflect physical layout

- **WHEN** a control indicator is rendered
- **THEN** it appears at the matching position on the appropriate outline instead of in a generic grouped grid

### Requirement: Highlight controls in place

The system SHALL indicate the active state of each control at its outline position, so that pressing a physical control lights the corresponding indicator where it is drawn, and releasing it returns that indicator to idle.

#### Scenario: Button pressed on the outline

- **WHEN** the user presses and holds a game controller button
- **THEN** the matching on-outline indicator is shown active at its position

#### Scenario: Button released on the outline

- **WHEN** the user releases the previously held game controller button
- **THEN** the matching on-outline indicator returns to idle

#### Scenario: Stick deflected on the outline

- **WHEN** the user deflects an analog stick
- **THEN** the stick indicator on the outline moves proportionally to the deflection and shows the updated numeric X/Y values at that position

### Requirement: Preserve existing diagnostic behavior

The system SHALL keep the existing controller input tester behavior: a recent event log, an idle/no-controller state that keeps running instead of exiting, and Escape-to-quit.

#### Scenario: No controller connected

- **WHEN** no game controller is connected
- **THEN** the outlines and all control indicators render idle and a "no controller" hint is shown

#### Scenario: Esc quits

- **WHEN** the user presses the Escape key
- **THEN** the system exits cleanly
