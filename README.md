# Animated GIF for Unreal Engine

> Native animated‑GIF support for UE 5.6 - decode a `.gif` into a real,
> runtime‑updating GPU texture driven by the file's own frame timing, and use it
> in UMG, Slate, and materials.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.6-0E1128?logo=unrealengine)
![Platform](https://img.shields.io/badge/platform-Win64-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

Unreal has no built‑in GIF support: `ImageWrapper` doesn't list GIF and the Media
Framework doesn't decode it. The usual workaround is to import every frame as a
separate `UTexture2D` and flip through them on a timer. This plugin does it
properly instead: a GIF is decoded once into fully‑composited frames and played
back on a single `UTexture2DDynamic` (or a `UTextureRenderTarget2D`) that is
rewritten on the GPU only when the displayed frame actually changes, using the
real per‑frame delays, disposal/compositing and loop count from the file.

<!-- TODO: add a short screen capture / screenshots here (asset editor, UMG widget). -->

## Features

- **Native runtime texture** - playback writes to a live `UTexture` via
  `RHIUpdateTexture2D`; no per‑frame asset swapping, no timers.
- **Correct GIF playback** - per‑frame delays, all four disposal methods,
  transparency, interlacing and the `NETSCAPE2.0` loop count.
- **Two sources** - import `.gif` as a `UGifAsset`, **or** load from a file / raw
  bytes at runtime (sync helpers + an async Blueprint node).
- **UMG widget** - `Animated GIF` widget with play/pause/stop, looping, play rate,
  tint, an in‑designer preview toggle, and the stock *Image* palette icon.
- **Materials / 3D** - bind the live texture to a material, or link a
  **Render Target** asset that the player drives (drop it straight into a material
  graph; it updates live like a Scene Capture target).
- **Dedicated asset editor** - checkerboard preview, transport bar with a frame
  scrubber, a details panel, and a toolbar.
- **Animated Content Browser thumbnails.**
- **Pause / time‑dilation aware** by default, with per‑instance overrides for
  HUD / menu / loading‑screen GIFs.
- **Extensible decoder seam** (`IAnimatedImageDecoder`) so APNG/WebP can be added
  without touching consumers.

## Requirements

- **Unreal Engine 5.6** 
- Developed and tested on **Windows (Win64)**. It uses only cross‑platform engine
  APIs, so other platforms should build, but they are currently untested.

## Installation

### From a release

The release archive ships compiled binaries for
**UE 5.6 / Win64**.

1. Download the latest archive from
   [Releases](https://github.com/PsinaDev/UnrealAnimatedGif/releases).
2. Extract the `AnimatedGif` folder into your project's `Plugins/` directory:
   `YourProject/Plugins/AnimatedGif/` (create the `Plugins` folder if it doesn't
   exist). To make it available to **all** projects instead, extract into
   `<UE_5.6>/Engine/Plugins/Marketplace/AnimatedGif/`.
3. Launch the project. Enable **Animated GIF** under **Edit → Plugins → UI** if it
   isn't already, then restart.

> The prebuilt binaries are version‑locked to **UE 5.6 (Win64)**. For another
> engine version or platform, use the source install below.

### From source

For C++ projects, or to build for another engine version / platform.

1. Copy the `AnimatedGif` folder into your project's `Plugins/` directory:
   `YourProject/Plugins/AnimatedGif/`.
2. Right‑click your `.uproject` → **Generate Visual Studio project files**.
3. Build the project from your IDE (or let the editor compile it on launch).
4. Enable **Animated GIF** under **Edit → Plugins → UI** if needed, then restart.

> `stb_image.h` is bundled under `Source/ThirdParty/stb_image/` — no external
> dependencies to fetch.


## Quick start

1. Drag a `.gif` into the Content Browser → it becomes a **GIF Asset**.
2. Add an **Animated GIF** widget to a UMG widget (Palette → *Image*) and set its
   **Gif Asset**. It auto‑plays and loops by default.

That's it.

## Usage

### Importing GIFs

Drag‑and‑drop a `.gif` into the Content Browser. You get a `UGifAsset` with an
animated thumbnail and reimport support. Double‑click it to open the **GIF editor**:
an animated preview (checkerboard background to show transparency), a play/pause/stop
toolbar, a frame scrubber with a time readout, and a details panel.

### UMG widget

Add the **Animated GIF** widget and set **Gif Asset**. Properties:

| Property | Meaning |
|---|---|
| `Gif Asset` | The GIF to play. |
| `Auto Play` | Start on construct (runtime). |
| `Looping` | `false` → play once and stop; `true` → use the loop count. |
| `Loop Count Override` | `-1` = use the asset's loop count; `≥ 0` overrides it (`0` = forever). |
| `Play Rate` | Speed multiplier. |
| `Color And Opacity` | Tint. |
| `Preview In Designer` | Animate inside the UMG designer (off by default). |

Blueprint: `Set Gif Asset`, `Play`, `Pause`, `Stop`, `Get Player`.

### Runtime loading (no asset)

**Blueprint** - use the latent node **Load Animated Gif From File** /
**Load Animated Gif From Bytes** (outputs `On Loaded(Player)` / `On Failed(Error)`);
decoding happens on a worker thread.

**C++**

```cpp
// Async (preferred): decode off the game thread, player created on it.
UAsyncAction_LoadAnimatedGif* Load =
    UAsyncAction_LoadAnimatedGif::LoadAnimatedGifFromFile(this, FilePath);
Load->OnLoaded.AddDynamic(this, &AMyActor::HandleGifLoaded);
Load->Activate();

// Synchronous helpers (small GIFs / tools):
UGifPlayer* Player =
    UAnimatedGifFunctionLibrary::CreateGifPlayerFromBytes(this, Bytes);
```

Keep the returned `UGifPlayer` in a `UPROPERTY` to stop it being garbage‑collected
(the playback subsystem only roots it while it is actively playing).

### Materials & render targets

A player exposes its live texture three ways:

```cpp
Image->SetBrushResourceObject(Player->GetTexture());          // any output (UTexture)
Image->SetBrushFromTextureDynamic(Player->GetDynamicTexture()); // internal texture
MID->SetTextureParameterValue("Gif", Player->GetTexture());   // material parameter
```

For a **designer‑friendly material workflow**, create a **Render Target** asset,
assign it to the GIF asset's **Linked Render Target**, and drop that render target
into a material graph as a normal Texture Sample. Then play the GIF with a player
(`CreateGifPlayer`, or just open the asset editor) - the player writes frames
straight into the render target, which updates live in the editor and at runtime.

> Why a Render Target and not a Media Texture? A `UMediaTexture` can't be fed
> without a full `UMediaPlayer` backend and needs an External‑Texture material
> node. A render target is an ordinary sampleable 2D texture and reuses the same
> upload path.

## Settings

**Project Settings → Animated GIF**:

| Setting | Default | Meaning |
|---|---|---|
| `Min Frame Delay Seconds` | `0.02` | Frames faster than this are clamped… |
| `Fallback Frame Delay Seconds` | `0.10` | …to this (browser behaviour for 0 ms‑delay GIFs). |

Per‑player flags: `Looping`, `Loop Count`, `Play Rate`, `Ignore Pause`,
`Ignore Time Dilation`.

## API overview

| Class | Role |
|---|---|
| `UGifAsset` | Imported asset: metadata + decoded BGRA frames (bulk data). Optional `Linked Render Target`. |
| `UGifPlayer` | Playhead + GPU sink. `Play/Pause/Stop/SeekToFrame/SeekToTime`, `GetTexture/GetDynamicTexture/GetRenderTarget`, `OnLooped/OnFinished`. |
| `UAnimatedGifImage` | UMG widget. |
| `UAnimatedGifFunctionLibrary` | `CreateGifPlayer`, `CreateGifPlayerFromBytes`, `LoadGifPlayerFromFile`. |
| `UAsyncAction_LoadAnimatedGif` | Async load‑from‑file/bytes Blueprint node. |
| `UGifPlaybackSubsystem` | Per‑world playback clock (pause/time‑dilation aware). |
| `UAnimatedGifSettings` | Project settings. |

## How it works

A GIF is decoded once (at import, or off‑thread at runtime) into
`FAnimatedImageData` - an immutable, ref‑counted buffer of fully‑composited BGRA8
frames plus per‑frame delays and the loop count. A `UGifPlayer` owns a clock and a
GPU texture; each tick it maps elapsed time to a frame index and, **only when the
index changes**, uploads that frame with `RHIUpdateTexture2D`. In‑world players are
ticked by a `UTickableWorldSubsystem` (so playback respects pause and time
dilation); world‑less players (e.g. loading screens) fall back to the core ticker.

The decoder is the vendored single‑header **stb_image** (public domain), compiled
with only the GIF path enabled. On top of it the plugin swizzles RGBA → BGRA at
decode time, parses the `NETSCAPE2.0` loop‑count extension (which stb ignores), and
applies a one‑line fix to stb's disposal‑method‑3 handling (marked in the header so
it survives re‑vendoring).

## Project structure

```
AnimatedGif/
├─ AnimatedGif.uplugin
└─ Source/
   ├─ AnimatedGif/         Runtime module (asset, player, widget, subsystem, decoder)
   ├─ AnimatedGifEditor/   Editor module (factory, asset editor, thumbnail, asset def)
   └─ ThirdParty/stb_image/  Vendored decoder (header-only)
```

| Module | Type |
|---|---|
| `AnimatedGif` | Runtime |
| `AnimatedGifEditor` | Editor |
| `stb_image` | External (ThirdParty) |


## Third‑party

- [**stb_image**](https://github.com/nothings/stb) by Sean Barrett - public domain
  (or MIT). Bundled under `Source/ThirdParty/stb_image/` with a small local patch
  to disposal‑method‑3 handling.

## License

Released under the **MIT License** - see [`LICENSE`](LICENSE). The bundled
`stb_image.h` is public domain / MIT and retains its own license header.

## Credits

Created by [**PsinaDev**](https://github.com/PsinaDev).
