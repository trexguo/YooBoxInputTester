## Purpose

Provides a live diagnostic view for the Yoobox Y1 handheld's game controls — game buttons, D-pad, both analog sticks and triggers — showing their current state on the device's 1620×1080 display so hardware input can be verified at a glance.

## ADDED Requirements

### Requirement: Maintain a connected game controller
The system SHALL detect and maintain a connection to at least one SDL game controller when present, and SHALL keep reading its button, hat, and axis state continuously. When no controller is connected the system SHALL continue running and display an idle/no-controller state rather than exiting.

#### Scenario: Controller present at startup
- **WHEN** a game controller is connected before the app starts
- **THEN** the system opens it and begins rendering its input state

#### Scenario: No controller present
- **WHEN** no game controller is connected
- **THEN** the system stays running with all indicators idle and a "no controller" hint visible

### Requirement: Render game button state
The system SHALL render an on-screen indicator for each reported game controller button (including A/B/X/Y, Start, Select, and any shoulder/click buttons reported by the controller). The indicator SHALL appear in an active state while its button is held and in an idle state while it is released.

#### Scenario: Button press
- **WHEN** the user presses and holds a game controller button
- **THEN** that button's on-screen indicator is rendered in the active state

#### Scenario: Button release
- **WHEN** the user releases the previously held game controller button
- **THEN** that button's on-screen indicator returns to the idle state

#### Scenario: Simultaneous multi-button press
- **WHEN** two or more game controller buttons are held at the same time
- **THEN** every held button's indicator is rendered in the active state in the same frame

### Requirement: Render D-pad direction state
The system SHALL render an on-screen D-pad (hat) indicator that reflects the current hat direction, with the active segment(s) highlighted for the direction currently held. A centered/neutral hat SHALL render as idle.

#### Scenario: D-pad direction held
- **WHEN** the user holds a D-pad direction on the controller
- **THEN** the on-screen D-pad indicator highlights that direction's segment

#### Scenario: D-pad diagonal held
- **WHEN** the user holds a D-pad diagonal on the controller
- **THEN** the on-screen D-pad indicator highlights both constituent direction segments

#### Scenario: D-pad released
- **WHEN** the user releases the D-pad back to center
- **THEN** the on-screen D-pad indicator renders as idle

### Requirement: Render analog stick position
The system SHALL render a position indicator for each connected analog stick that plots the live axis position on a 2-D center/circle graphic, and SHALL display the numeric X/Y axis values for that stick. The indicator SHALL move as the stick is deflected and return to center when released.

#### Scenario: Stick deflection
- **WHEN** the user deflects an analog stick
- **THEN** the on-screen position indicator moves proportionally to the deflection and the numeric X/Y values update

#### Scenario: Stick centered
- **WHEN** the user releases an analog stick back to its neutral position
- **THEN** the on-screen position indicator returns to the center of its graphic

### Requirement: Render trigger level
The system SHALL render a level indicator for each analog trigger that represents the trigger's pressed depth (analog level, not only on/off). The level SHALL increase as the trigger is pressed fully and decrease as it is released.

#### Scenario: Trigger pressed
- **WHEN** the user presses an analog trigger
- **THEN** the on-screen level indicator shows a depth matching the trigger's value

#### Scenario: Trigger released
- **WHEN** the user releases the analog trigger
- **THEN** the on-screen level indicator returns to its minimum level

### Requirement: Maintain a recent event log
The system SHALL maintain an on-screen log of the most recent input events (button, hat, axis, and any keyboard/touch/mouse events), displaying at least the last few events, with older events discarded so the log stays bounded.

#### Scenario: New input event
- **WHEN** an input event occurs
- **THEN** it is appended to the on-screen log and the oldest event beyond the log size is discarded

#### Scenario: Esc quits
- **WHEN** the user presses the Escape key
- **THEN** the system exits cleanly.
