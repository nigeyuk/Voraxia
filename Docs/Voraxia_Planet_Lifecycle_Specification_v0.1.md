# Voraxia Planet Lifecycle Specification

**Version:** 0.1  
**Status:** Living architecture specification  
**Date:** 29 June 2026  
**Owner:** Coding Custard Studios  
**Applies to:** `VoraxiaPlanet` runtime architecture, future terrain, voxel, PCG, persistence, gravity, atmosphere, and multiplayer systems.

## 1. Purpose and authority

This document is the canonical technical and design contract for a Voraxia planet from creation through player arrival, mining, base construction, unloading, and return visits. It is deliberately broader than the current implementation. It separates locked decisions, working prototypes, planned systems, and open questions so that future code does not accidentally promote a sketch into a network contract.

When this document conflicts with informal chat notes or comments, this document wins once the change has been reviewed and versioned in source control.

### Status vocabulary

- **Locked decision:** an architectural rule that new systems must respect.
- **Implemented and validated:** working code or a tested prototype.
- **Planned:** intended direction, not yet a supported gameplay feature.
- **Open decision:** deliberately unresolved and not safe to assume.

## 2. Non-negotiable principles

1. **Determinism first.** Untouched planetary content must be reproducible from stable inputs, primarily planet identity, seed, generator version, and global coordinates.
2. **Server authority.** The server decides the authoritative planet state, active terrain state, edits, structures, and gameplay-relevant object state.
3. **Persistent deltas, not full worlds.** Untouched terrain is regenerated. Player changes, structures, and other exceptions are saved as compact persistent state.
4. **One physical source of truth.** Terrain, gravity, atmosphere, life support, flight, PCG, and environmental hazards query the same authoritative planet profile rather than maintaining incompatible local values.
5. **Global fields before local presentation.** Planet generation is evaluated in global planet space, not per cube face, so terrain and other fields remain continuous across face boundaries.
6. **Simulation-led gameplay.** Planet gravity and atmosphere are gameplay inputs. A 1.2 g world must create materially different movement, vehicle, thrust, and flight demands from a 1.0 g world.
7. **Streaming is not persistence.** A chunk becoming unloaded means it is dormant, not deleted.
8. **PCG populates the world.** Procedural generation supplies the bulk of environmental content. Hand-authored material is reserved for important landmarks, missions, factions, and deliberate exceptions.

## 3. Canonical units and coordinate contracts

### 3.1 Canonical units

| Domain | Canonical unit | Rule |
|---|---:|---|
| Persistent planet coordinates | metres, double precision | Never store planet-scale state in Unreal centimetres. |
| Planet radius, terrain height, atmosphere altitude | metres | Use SI-style values in authoritative data. |
| Gravity | m/s² | Current baseline setting uses `SurfaceGravityMetresPerSecondSquared`; 1 g gameplay baseline is 9.81 m/s². |
| Gameplay / Unreal local world | centimetres | Convert at the presentation or local-simulation boundary. |
| Long-range display | kilometres | Display only. Do not use as storage units. |

### 3.2 Stable identifiers

Every planet has an immutable `PlanetId`. A planet's `Seed` and `GeneratorVersion` are persistent generation contracts. Never regenerate an ID or silently change an algorithm for an existing generator version after worlds can be saved or played.

A terrain chunk is identified by planet identity, cube face, quadtree level, and X/Y address. This address is a stable logical identity, not a render-mesh identifier.

## 4. Planet data model

### 4.1 Definition asset

A `UVoraxiaPlanetDefinition` describes durable design-time planet identity and physical baseline. It includes, at minimum:

- `PlanetId`
- display name
- seed
- generator version
- reference radius in metres
- maximum terrain elevation and editable depth
- surface gravity

### 4.2 Replicated runtime state

At runtime the server validates the definition and creates compact `FVoraxiaPlanetRuntimeState`. Clients reconstruct deterministic base content from this replicated state. Vertex buffers, dynamic meshes, and untouched voxel payloads are not replicated.

### 4.3 Planned physical profile expansion

The runtime state will gain a versioned physical/environment profile before gravity, flight, atmospheric entry, or life-support systems become gameplay-critical. Planned fields include:

- atmosphere top altitude
- surface pressure and density model
- gas mixture / composition
- temperature profile
- toxicity and corrosiveness
- weather capability and wind model parameters
- gravitational falloff model or mass-equivalent parameters

These are planned architecture, not yet implemented fields.

## 5. Planet lifecycle overview

```text
Definition authored
  -> server validates and creates runtime state
  -> deterministic global fields become evaluable everywhere
  -> representations stream according to distance and activity
  -> local terrain and voxel chunks activate near interest sources
  -> players mine, build, and change persistent state
  -> changed chunks save deltas or snapshots
  -> distant regions become dormant
  -> returning players reload base generation plus persistent changes
```

A planet is never fully voxelised or fully active. It is always mathematically defined, selectively represented, and persistently altered only where gameplay creates a difference from the base world.

## 6. Creation and authoritative activation

### 6.1 Authoring

A planet definition is created for a new world. The permanent ID is generated once. Seed, radius, generator version, physical profile, and planet archetype are selected intentionally.

### 6.2 Server activation

On authoritative startup:

1. Validate the definition.
2. Create `FVoraxiaPlanetRuntimeState`.
3. Replicate compact runtime state to clients.
4. Make deterministic planet fields available to all local systems.
5. Begin representation and chunk streaming only when players, ships, events, or other interest sources need the world.

### 6.3 Client activation

Clients receive runtime state, then independently rebuild deterministic presentation from the same contracts. They never invent a different seed, generator version, terrain algorithm, gravity, or atmosphere profile.

## 7. Global deterministic generation

### 7.1 Current terrain foundation

The current macro terrain generator samples global 3D unit direction from the planet centre and returns deterministic values such as height, continentalness, and mountainness. It is intentionally independent of UObjects, mesh ownership, rendering, networking, and editor state.

The global-direction input is critical. It ensures that terrain remains continuous across all cube-sphere face boundaries.

### 7.2 Planned planet fields

The same global evaluation pattern should expand into versioned fields:

- macro elevation and basins
- continentalness and mountain belts
- geology / rock strata class
- cave potential and depth bands
- resource potential
- temperature and moisture proxies
- climate / biome classification
- hazard and anomaly potential
- landmark suitability

Each field must have a documented input contract, deterministic seed handling, generator-version behavior, and clear distinction between visual-only data and gameplay-authoritative data.

### 7.3 Generator versioning

A generator version is immutable once a real planet exists. New algorithms require a new version branch. Clients that do not understand a version must fail closed rather than rendering invented terrain that disagrees with the server.

## 8. Representation by distance and activity

### 8.1 Far orbital representation

Far from the surface, a planet is represented by a low-cost shell, atmosphere, clouds, broad lighting, and large-scale landmark cues. It must communicate planetary identity without requiring terrain collision or voxels.

### 8.2 Atmospheric approach

As an interest source approaches, higher-detail surface representations stream in. Atmospheric density, drag, thermal behavior, and gravity queries become increasingly relevant. The planet must transition visually and physically without a fake "new map" cutover.

### 8.3 Surface terrain representation

Near a landed player or low-flying ship, detailed surface chunks provide mesh, collision, PCG inputs, and gameplay-relevant resource/feature placement.

### 8.4 Local volumetric terrain representation

True 3D density/voxel chunks are materialised only where required for digging, caves, terrain deformation, collision, construction integration, or other local interactions. This is a smaller, activity-aware volume within the broader surface streaming region.

### 8.5 Interest sources

The streaming system uses interest sources rather than players alone:

- players
- low-flying or landed ships
- active mining tools / excavation machines
- bases and structures requiring nearby terrain state
- gameplay events and mission locations
- other server-authoritative active entities

Streaming shapes may be asymmetric and predictive. A fast ship needs terrain prepared ahead of its path; an underground player needs a deeper vertical neighbourhood; a stationary base needs durable, fast-recoverable terrain around it.

## 9. Gravity, atmosphere, and simulation-led physics

### 9.1 Gravity

Gravity is a queryable local environment value, not a fixed Unreal-world Z-axis assumption. The initial surface baseline derives from the planet runtime state and points toward the planet centre. A future gravity model may include altitude falloff, but must remain stable, versioned, and consistent across client and server.

A planet at 1.2 g should impose a local acceleration of approximately `1.2 * 9.81 m/s²` at its defined surface baseline. This affects character movement, falling, rover traction, vehicle braking, landing gear load, ship hover thrust, take-off ability, fuel cost, and payload planning.

### 9.2 Flight and propulsion

Ship systems must query local gravity and atmosphere. Hovering requires upward force greater than local mass times local gravity. Aerodynamic lift, drag, control surfaces, engine performance, and re-entry behavior depend on atmospheric density and composition, not a universal Earth-like default.

### 9.3 Atmosphere

A planet's atmosphere is a physical gameplay profile. At minimum, planned systems distinguish pressure, density, composition, temperature, toxicity, corrosiveness, and weather/wind potential. These values feed life support, suit requirements, combustion constraints, external equipment wear, flight, parachute/braking behavior, and atmospheric entry.

## 10. PCG population and world features

### 10.1 Role of PCG

The deterministic planet generator defines what a planet is. PCG determines how that planet is populated. PCG consumes terrain and environmental fields to place the bulk of scenery and environmental storytelling that cannot be handcrafted by a two-person team.

### 10.2 PCG candidates

PCG can populate:

- rocks, boulders, scree, cliff dressing, debris
- vegetation, alien flora, ice, lava, crystal, and other biome dressing
- ore indicators and non-critical resource dressing
- fauna habitats and ambient zones
- wreck fragments, abandoned machinery, minor camps, relay towers
- caves entrances, surface anomalies, hazard dressing
- small industrial and scavenger sites

### 10.3 Persistent features

Major faction sites, mission locations, rare resources, large ruins, landmark wrecks, dungeons, and important settlements are not disposable scatter results. They require stable generated identities and server-owned persistent state. PCG may dress their surroundings but does not replace their authority.

### 10.4 Respecting edits

PCG must not respawn decoration through player bases, tunnels, cleared areas, or persistent structures. It queries terrain edit state, reserved/base volumes, ownership rules, and feature exclusions before populating a chunk.

## 11. Mining, voxels, caves, and construction

### 11.1 Base density contract

Future local voxel terrain combines:

```text
Base procedural density
+ macro surface height
+ geology / cave / ore fields
+ player and event edits
= authoritative solid-or-empty result
```

This contract is the hinge between visible macro terrain and a mineable, cave-capable world. It must be defined before full voxel implementation begins.

### 11.2 Mining

Mining targets active local density chunks. The server validates tools, range, permissions, and edit operations; it commits authoritative terrain/resource changes. Clients rebuild local mesh/collision from generated base data plus current revisions and edits.

### 11.3 Cave bases

A player may land, excavate a cave base, leave the planet, and later return. The cave, access tunnels, terrain openings, and base structures remain correct because their changes are persisted. They do not need to remain permanently loaded in memory while no one is nearby.

### 11.4 Construction boundary

Terrain edits and player structures are distinct persistent layers. Terrain stores altered density/material state. Structures store placement, ownership, inventory, power, health, and gameplay state. Both layers reference stable planet/chunk/local coordinate identities.

## 12. Persistence, dormancy, and return visits

### 12.1 Untouched chunks

An untouched chunk is regenerated from the planet's deterministic base fields. No saved mesh or voxel payload is required.

### 12.2 Edited chunks

The server persists the difference from the base world. Typical data includes density edits, material edits, resource depletion, feature state, and a revision number.

### 12.3 Delta versus snapshot policy

- **Edit deltas:** efficient for modest changes, such as mining cuts and small tunnels.
- **Baked snapshots:** appropriate for heavily modified chunks or large cave bases where replaying a huge history becomes inefficient.

A chunk may be promoted from delta replay to snapshot storage according to a documented compaction policy. The snapshot remains tied to the planet ID, generator version, chunk identity, and persistence schema version.

### 12.4 Dormancy

When no interest source requires a chunk, active runtime mesh/collision/simulation can unload. Before unloading, dirty state is durably committed. A dormant chunk is not erased. It is simply absent from active memory and can be reconstructed.

### 12.5 Reload sequence

When a player returns:

1. Server selects the chunk due to interest/streaming rules.
2. Base terrain is regenerated deterministically.
3. Saved deltas or a baked snapshot are loaded.
4. Persistent structures and gameplay entities are restored.
5. The server establishes the current chunk revision.
6. Clients receive the needed state and rebuild presentation locally.

## 13. Multiplayer authority and replication

### 13.1 Server owns

- planet runtime state
- active chunk decisions
- terrain edit validation and commit
- chunk revisions
- persistent structures and gameplay objects
- resource depletion and harvest state
- save/compaction operations

### 13.2 Clients derive locally

- untouched terrain density and surface from approved seed/version/coordinates
- visual meshes and local presentation
- PCG scenery that is explicitly safe to derive locally
- debug/inspection visualisation

### 13.3 Replication policy

Replicate compact facts, not full terrain meshes:

- runtime definition/profile data
- active chunk identifiers and revisions
- edit operations or compact authoritative edit state
- snapshots where needed
- gameplay-relevant object state near a client

Clients must be able to request or receive recovery state if a chunk revision is missed or local reconstruction fails.

## 14. State machines

### 14.1 Planet lifecycle

| State | Meaning | Exit condition |
|---|---|---|
| Defined | Design-time definition exists with permanent ID. | Server validates and activates it. |
| Authoritative | Server runtime state exists and is replicable. | Interest sources require representation. |
| Represented | Far shell / atmosphere / broad visual layer is available. | Players or ships approach. |
| Streamed | Surface or voxel chunks are loaded around active interests. | Chunks become active or leave range. |
| Active | Terrain, collision, features, and simulation are live. | Edits commit or interest leaves. |
| Persisted | Dirty chunk/structure changes are durably stored. | Region may become dormant. |
| Dormant | No active runtime representation is required. | Interest source returns. |
| Reloading | Base world plus persistent changes are reconstructed. | Chunks become active again. |

### 14.2 Chunk lifecycle

```text
Procedural-only
  -> requested by interest source
  -> generated / loaded
  -> active
  -> edited (optional)
  -> persisted
  -> dormant
  -> regenerated plus edits on next request
```

## 15. Current implementation baseline

### Implemented and validated

- VoraxiaPlanet plugin foundation and developer settings.
- Planet definition assets and compact replicated runtime state.
- Persistent planet identity, seed, generator version, radius, terrain limits, and surface gravity fields.
- Cube-sphere coordinate conventions and chunk addressing.
- Local Dynamic Mesh preview for individual patches, adjacent regions, and whole-planet diagnostic previews.
- Outward-facing triangle winding and seam validation across cube faces/chunks.
- Live editor preview controls.
- Deterministic macro terrain generator sampled from global direction.
- Macro terrain radial preview displacement with visual exaggeration.
- Debug colour modes for chunk address, terrain height, and terrain slope.

### Planned, not implemented

- actual orbital/far shell runtime representation
- atmosphere and gravity runtime queries
- physical flight, drag, lift, re-entry, and life-support effects
- terrain density/voxel contract and chunk implementation
- terrain edits, snapshots, persistence, compaction, and recovery
- streamed collision and active chunk scheduler
- PCG production pipeline and persistent feature layer
- structures/base integration

## 16. Validation and test matrix

Every implementation phase must include deterministic and multiplayer checks appropriate to its scope.

| Area | Required checks |
|---|---|
| Cube-sphere terrain | all face orientations, adjacent chunk seams, whole-planet visual pass |
| Generator | same seed/version/input produces same output on server and client; unsupported version fails closed |
| Streaming | chunks activate/deactivate at correct distances and activity states; no invalid gaps during movement |
| Voxels | edit then unload/reload; adjacent edited chunk boundary; cave/surface opening continuity |
| Persistence | mine/build, leave planet, restart/rejoin, return and verify exact result |
| Multiplayer | two clients observe same edits, stale revisions recover, unauthorised edits rejected |
| PCG | deterministic local scenery remains stable; persistent features survive; no spawn through player bases |
| Physics | gravity direction/strength follows planet profile; atmosphere changes flight/life-support behavior predictably |

## 17. Open decisions

The following are intentionally unresolved and must not be assumed by code:

- exact density function and voxel resolution/chunk dimensions
- edit-delta representation and snapshot compaction threshold
- gravity falloff model and practical physics limits near/inside planets
- atmospheric equations, altitude bands, and thermal model fidelity
- planetary archetype schema and feature-library format
- climate, hydrology, ocean, and weather simulation depth
- PCG framework boundary: Unreal PCG integration, custom runtime generation, or hybrid
- base keep-alive rules and offline simulation policy
- persistence backend and save migration strategy
- public modding or world-authoring hooks

## 18. Immediate next steps

1. Bank the current macro terrain and diagnostic-preview work.
2. Refine macro terrain fields and define the procedural terrain/feature vision.
3. Draft the physical planet profile schema before any ship/atmosphere implementation.
4. Define the base density contract before voxel code.
5. Design chunk persistence and revision semantics before mining/building systems.
6. Introduce PCG in layers, beginning with non-persistent scenery driven by stable terrain/environment fields.

## 19. Change-control rule

Changes to planet identity, generator versioning, coordinate contracts, physics units, chunk identity, multiplayer authority, or persistence semantics are architecture changes. They require an explicit amendment to this document and a corresponding code/versioning plan before implementation.
