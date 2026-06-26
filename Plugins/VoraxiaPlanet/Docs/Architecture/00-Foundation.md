# VoraxiaPlanet: Foundation Contract

## Current milestone

This is a **compile-baseline plugin skeleton**. It deliberately contains no gameplay or terrain
implementation. The only acceptance criterion is that it is detected and builds as a runtime
plugin inside Voraxia.

## Architectural rules already locked

1. **Multiplayer-first.** Networking determines the data model from the beginning.
2. **Server authority.** The server owns all permanent terrain, resource, and construction state.
3. **Replicated changes, not replicated meshes.** Clients construct pristine terrain from common
   deterministic inputs. The server replicates authoritative terrain operations and, when needed,
   compact chunk snapshots.
4. **Revisioned chunks.** Each editable chunk will have an ID, monotonically increasing revision,
   ordered authoritative operations, and a resynchronisation path.
5. **Stable planet coordinates.** Persistent locations will not use ordinary Unreal world
   coordinates as their canonical address.
6. **Runtime first.** Editor-only tools are postponed until a runtime need makes them worthwhile.

## First future implementation slice

`Core/` will receive plain, serialisable data types only:

- Planet identity
- Cube-face identity
- Chunk identity
- Planet-local double-precision positions
- Terrain-operation identity and revision metadata

No UObjects, Actors, mesh components, threading, or generation logic enter until those types and
their network serialisation are agreed.
