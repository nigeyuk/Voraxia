# VoraxiaPlanet

A multiplayer-first runtime plugin foundation for Voraxia's planetary terrain.

## What this milestone contains

- A valid Unreal Engine runtime plugin descriptor.
- A deliberately minimal, compilable C++ module.
- The final top-level code areas, created early so future work has an agreed home.
- No rendering, terrain generation, replication code, game actors, assets, or editor tooling.

Keeping the first milestone tiny makes a broken plugin boundary easy to diagnose before
we introduce cube-sphere mathematics, streamed terrain, or networking.

## Install

1. Close Unreal Editor and Rider.
2. Copy this entire `VoraxiaPlanet` folder into:
   `YourVoraxiaProject/Plugins/VoraxiaPlanet`
3. Regenerate project files from the `.uproject` file, then open the solution/project in Rider.
4. Build the `VoraxiaEditor` target.
5. Open Voraxia. Unreal should discover the plugin. If it is not enabled automatically,
   enable **Voraxia Planet** under **Edit > Plugins > Voraxia**, then restart when prompted.

For source-control stability, commit the plugin source and descriptor, but never commit its
`Binaries/` or `Intermediate/` directories.

## Intentional directory map

```text
Source/VoraxiaPlanet/
├── Public/
│   ├── Core/         Stable mathematical types and coordinate contracts.
│   ├── Debug/        Runtime diagnostics and debug visualisation APIs.
│   ├── Networking/   Replication contracts, revisions, prediction and resync.
│   ├── Planet/       Planet definitions, cube-sphere topology and gravity.
│   ├── Streaming/    Chunk relevance, lifecycle and asynchronous work control.
│   └── Terrain/      Surface/voxel chunk interfaces and later mesh generation.
└── Private/           Implementations mirroring the public categories.
```

## Non-negotiable rule

Every permanent terrain change must be server-authoritative, revisioned, replicated to relevant
clients, and recoverable by an authoritative chunk snapshot. Clients may generate the untouched
planet locally from shared deterministic inputs, but no client owns final terrain state.
