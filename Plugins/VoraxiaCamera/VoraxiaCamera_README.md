# Voraxia Camera System

**Module:** `VoraxiaCamera`  
**Primary component:** `UVoraxiaCameraComponent`  
**Status:** Camera v1 feature-complete checkpoint  
**Approach:** C++-first gameplay camera. The Unreal Editor is used chiefly for asset assignment, component placement, and minimal graph wiring.

---

## Purpose

The Voraxia Camera System is a custom third-person camera framework for gameplay, exploration, inspection, and later scanner/tool interactions.

It deliberately avoids a Spring Arm-driven design. The camera component calculates and applies its own camera transform, collision response, focus behaviour, framing changes, and runtime modifiers.

The camera is intended to do more than follow the player:

- Keep a stable third-person view.
- Handle tight geometry without clipping through walls.
- React subtly to movement, speed, and pitch.
- Frame the player differently in special areas.
- Focus on valid world targets.
- Support inspection and scanner-style information gathering.

---

## Design Principles

```text
C++ owns behaviour and state.
The editor connects assets and places level actors.
Blueprint/Event Graph logic is optional, not required.
AnimGraph/Blueprint work should stay as thin as practical.
```

The base camera values remain stable. Runtime systems apply temporary offsets or overrides through the component's framing and modifier layers rather than permanently mutating baseline values.

---

# System Overview

## 1. Core Third-Person Camera

The camera component owns manual camera positioning and rotation.

### Included

- Manual camera transform calculation.
- Yaw and pitch input.
- Configurable pitch limits.
- Soft pitch constraints near minimum and maximum pitch.
- Rotation lag.
- Optional pitch movement follow.
- Optional yaw movement follow.
- Character-facing support through the player character.

### Important framing values

- `CameraDistance`
- `PivotHeight`
- `AdditionalCameraOffset`
- `AdditionalPivotOffset`
- `BaseFOV`
- `MinFOV`
- `MaxFOV`

---

## 2. Runtime Framing

The camera supports temporary blended framing changes without altering permanent defaults.

### Runtime controls

```cpp
SetFraming(...)
ResetFraming(...)
SetCameraDistance(...)
SetPivotHeight(...)
SetCameraOffset(...)
SetPivotOffset(...)
SetFOVOffset(...)
SwapCameraShoulder(...)
```

### Typical uses

- Tight corridors.
- Dramatic reveal areas.
- Inspection locations.
- Hangars or interior spaces.
- Shoulder switching.
- Future cinematic or interaction framing.

All runtime framing values can blend over time and may optionally use a `UCurveFloat`.

---

## 3. Collision and Predictive Avoidance

The camera uses collision sweeps between its pivot and desired camera location.

### Included

- Main camera collision sweep.
- Collision compression and recovery smoothing.
- Minimum distance from pivot.
- Collision safety padding.
- Configurable collision channel.
- Predictive side-feeler probes for nearby surfaces.
- World debug drawing for collision state.

### Current useful tight-space baseline

These values were tuned during wall-clipping testing and are a good starting point:

```text
Min Distance From Pivot: 40
Camera Collision Radius: 24
Collision Safety Padding: 12
Collision Probe Start Offset: 1
Collision Compression Speed: 60
Collision Recovery Speed: 12
```

Treat them as gameplay tuning values, not sacred constants.

---

## 4. Movement Anticipation and Dynamic Modifiers

The camera has subtle motion-aware framing.

### Movement anticipation

The camera can offset itself based on the owning actor's velocity:

- Forward movement offset.
- Backward movement offset.
- Strafe offset.
- Smooth interpolation.
- Optional reduction while the player is actively looking around.

### Dynamic modifiers

The camera can alter distance and FOV dynamically based on:

- Current pitch.
- Owner movement speed.

Examples:

- Pull back slightly when sprinting.
- Expand FOV at speed.
- Change framing subtly at extreme pitch angles.

---

## 5. Focus System

Focus is a rotation-first targeting layer built on top of normal camera behaviour.

It does not discard collision, framing, movement anticipation, or dynamic modifiers.

### Focus targets can be

- Actors.
- Scene components.
- Named sockets.
- Arbitrary world locations.
- Tagged actors selected at runtime.

### Main calls

```cpp
SetFocusActor(...)
SetFocusComponent(...)
SetFocusWorldLocation(...)
ClearFocus(...)
FocusDefaultTaggedActor(...)
```

### Input modes

The player character supports:

```text
Hold
    Press focus -> focus target
    Release focus -> clear target

Toggle
    Press focus -> focus target
    Press again -> clear target
```

### Standard target tag

```text
CameraFocusTarget
```

Add it under an actor's **Actor > Tags** field.

---

## 6. Focus Target Selection v2

Tagged focus selection is no longer just "nearest tagged target."

Candidates are evaluated in this order:

```text
Tagged
Focusable, if it implements the focusable interface
Within search range
In front of the active camera
Visible by line trace, if enabled
Scored by centre-of-view alignment
Distance used as a secondary tie-breaker
```

### Important settings

```text
FocusTargetSearchDistance
bRequireFocusTargetInFront
FocusTargetMinForwardDot

bRequireFocusTargetLineOfSight
FocusTargetLineOfSightChannel

FocusTargetAlignmentScoreWeight
FocusTargetDistanceScoreWeight
```

### Debug settings

```text
bLogFocusTargetSelection
bDrawFocusTargetSelectionDebug
FocusTargetSelectionDebugDuration
```

Debug colour convention:

```text
Green  = selected target
Yellow = valid candidate
Red    = rejected target
```

---

## 7. Focusable Interface

`IVoraxiaFocusableInterface` lets a target tell the camera how it should be treated.

### Interface responsibilities

```text
CanBeFocused
GetFocusDisplayName
GetFocusLocation
```

### Why it exists

The camera should not need to know what an asteroid, terminal, resource node, or future interactable is.

A focusable target can:

- Reject focus.
- Provide a player-facing name.
- Provide a custom point to look at instead of its actor origin.

This is especially useful for large irregular asteroids, sockets, doors, consoles, or targets with dedicated scan points.

---

## 8. Scannable Interface

`IVoraxiaScannableInterface` provides scanner-facing information for an active focus target.

### Interface responsibilities

```text
CanBeScanned
GetScanDisplayName
GetScanSummary
GetScanComposition
GetScanTimeSeconds
```

### Composition entry

`FVoraxiaScanCompositionEntry` contains:

```text
MaterialId
DisplayName
Percentage
```

### Current state

The camera can detect whether a focused actor is scannable, retrieve its scan data, write it to the Output Log, and show it in the Slate debug panel.

A C++ test asteroid was used to validate the full path:

```text
Focus target acquired
Focusable name/location returned
Scannable target detected
Summary returned
Composition array returned
Data logged and shown in Slate
```

---

## 9. Camera Zones

`AVoraxiaCameraZone` is a level-placed framing trigger.

### Behaviour

```text
Player enters zone
    -> SetFraming(...)

Player leaves zone
    -> ResetFraming(...)
```

### Zone controls

- Box trigger bounds.
- Player-controlled pawn filtering.
- Camera distance.
- Pivot height.
- Camera offset.
- Pivot offset.
- FOV offset.
- Blend in time.
- Blend out time.
- Optional blend curve.
- Optional framing reset on exit.

### Intended current use

One active zone at a time is the happy path.

---

## 10. Debug Tools

The system includes development-time debugging support.

### World debug

- Camera collision state.
- Predictive avoidance probes.
- Pivot and camera locations.
- Focus target sphere and line.
- Focus target selection candidate lines.

### Slate debug panel

The Slate panel can show:

```text
State
Collision
Focus active state
Focus alpha
Focus target
Focus location
Scan status
Scan name
Scan time
Scan summary
Scan composition
Distance values
Rotation values
FOV
Camera and pivot locations
Camera debug summary
```

### Useful debug toggles

```text
bDrawFocusDebug
bDrawCameraCollisionDebug
bDrawCameraStateDebug
bShowSlateDebugPanel

bLogFocusTargetSelection
bDrawFocusTargetSelectionDebug
```

Use them generously while building, then disable them for ordinary gameplay testing.

---

# Basic Setup Checklist

## Player

1. Add `UVoraxiaCameraComponent` to the player character.
2. Add a `UCameraComponent`.
3. Assign the target camera, or allow automatic camera discovery.
4. Bind look input to:

```cpp
VoraxiaCameraComponent->AddYawInput(...)
VoraxiaCameraComponent->AddPitchInput(...)
```

5. Bind focus input to the player focus handlers.
6. Choose Hold or Toggle focus mode in the player character settings.

## Focus target

1. Place an actor in the level.
2. Add this actor tag:

```text
CameraFocusTarget
```

3. Optionally implement `IVoraxiaFocusableInterface`.
4. Optionally implement `IVoraxiaScannableInterface`.
5. Ensure line-of-sight collision is configured appropriately for the selected trace channel.

## Camera zone

1. Place `AVoraxiaCameraZone`.
2. Resize `ZoneBounds`.
3. Confirm overlap collision uses a Trigger-style setup.
4. Set zone framing values.
5. Walk into the volume and test the blend.
6. Walk out and confirm reset behaviour.

---

# Validation Checklist

## Core camera

- [ ] Camera follows player.
- [ ] Look input rotates camera correctly.
- [ ] Pitch constraints feel smooth.
- [ ] Sprint and speed modifiers behave sensibly.
- [ ] Camera does not clip through basic walls.

## Focus

- [ ] Tagged target can be focused.
- [ ] Hold mode clears on release.
- [ ] Toggle mode clears on second press.
- [ ] Focusable display name appears.
- [ ] Custom focus location appears in debug draw.
- [ ] Behind-wall candidate is rejected when line of sight is enabled.
- [ ] Camera chooses the target closest to the centre of view.

## Scan

- [ ] Scannable focused target reports `Scan: Yes`.
- [ ] Output Log lists scan name, summary, time, and composition.
- [ ] Slate debug panel shows scan readout.
- [ ] Non-scannable targets remain focusable without scan data.

## Zones

- [ ] Zone overlap fires.
- [ ] Entering applies framing.
- [ ] Leaving resets framing.
- [ ] Trigger collision works on placed instances.

---

# Remaining Camera Work

The camera is in a good stopping state. These are future improvements, not blockers.

## High-value next work

### Player-facing scanner UI

Current scan data is debug-visible, not yet player-facing.

Potential future elements:

```text
Focus reticle
Focused target name
Scan prompt
Scan progress bar
Composition panel
Unknown / partial / complete scan states
```

### Real asteroid integration

The test actor proves the system, but real asteroid/resource data still needs to feed the scannable interface.

Future work:

```text
Expose asteroid composition from actual asteroid/resource data
Decide whether composition is fixed, procedural, sampled, or partially hidden
Connect discovered material data to mining/resource systems
Persist scan discovery state
```

### Scan gameplay rules

These are design decisions before implementation:

```text
Instant scan or timed scan?
Scan range?
Line-of-sight requirement?
Can scanning continue while moving?
Does focus begin scanning automatically?
Can scan data be incomplete or uncertain?
Does scan knowledge persist per asteroid?
```

## Medium-priority future work

### Camera zone stacking and priority

Current camera zones are deliberately simple.

Future extension:

```text
Zone priority
Nested zone restoration
Per-zone tags or profiles
Active-zone debug display
Multiple overlapping framing influences
```

### Focus polish

Possible later additions:

```text
Target cycling
Soft target retention
Aim-assist style target bias
Focus loss conditions
Better screen-space reticle behaviour
Target-selection HUD feedback
```

### Camera polish

Possible later additions:

```text
Per-surface collision tuning
Camera shake layer
Contextual shoulder preference
Cinematic override layer
Photo/debug camera mode
Accessibility controls for lag, inversion, FOV, and focus mode
```

---

# Explicitly Out of Scope for This Camera Checkpoint

These are separate systems and should not be forced into the camera module:

```text
Animation Blueprint
Foot IK
Hand IK
Tool/weapon hand placement
Full scanner gameplay UI
Voxel asteroid composition generation
Mining/resource extraction rules
```

The camera is ready to support those systems when they arrive.

---

# Suggested Git Checkpoint

```bash
git add Plugins/VoraxiaCamera/README.md
git commit -m "Document Voraxia camera system"
```

---

## Camera v1 Summary

```text
Manual third-person camera
+ collision
+ dynamic movement feel
+ runtime framing
+ focus
+ target selection
+ scan data contracts
+ debug visualisation
+ camera zones
= a solid exploration and inspection camera foundation
```
