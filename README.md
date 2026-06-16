# Voraxia

Blank C++ Unreal project skeleton.

## What this contains

- `Voraxia.uproject`
- Minimal runtime C++ module named `Voraxia`
- Minimal `AVoraxiaGameModeBase`
- `Config/` files
- `Content/_Voraxia/` project content root
- Placeholder `.gitkeep` files so empty folders survive Git

## First use

1. Extract this folder somewhere sensible, for example your Unreal projects directory.
2. Associate `Voraxia.uproject` with your Unreal Engine version if prompted.
3. Right-click `Voraxia.uproject` and generate project files, or open it directly with Rider/Unreal if your setup supports that.
4. Build the `VoraxiaEditor` target.
5. Open `Voraxia.uproject`.

## Note about Content

The `_Voraxia` folders contain `.gitkeep` files only. These are not Unreal assets and are only there so Git retains the folder layout before real `.uasset` / `.umap` files exist.
