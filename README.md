# sanitize_metadata

A small Windows command-line tool that resets the HDR10 static metadata on every
HDR-enabled display. Fork of [Kaldaien/sanitize_metadata](https://github.com/Kaldaien/sanitize_metadata)
with bug fixes.

## Changes in this fork

- Leave fullscreen before releasing the swap chain. The original tool was
  terminated by DXGI after the first HDR display, so on multi-monitor systems
  later displays were never processed.
- Send MaxCLL and MaxFALL in whole nits. The original scaled them by 10000 and
  overflowed the 16-bit fields, sending wrapped garbage values.
- Validate the command-line argument and print usage on bad input.
- Skip displays that report a peak luminance of zero unless an override is given.
- Check and report failures from every DXGI call instead of continuing silently.
- Report displays that are skipped because they are not in HDR mode.

## Why

Some HDR monitors and TVs latch onto whatever HDR10 metadata the last fullscreen
application sent them. Games frequently send zero or nonsense values for MaxCLL
and mastering luminance, and the display can stay in a bad tone-mapping state
after the game exits. This tool briefly takes each HDR display fullscreen and
pushes sane metadata derived from the panel's own reported capabilities, which
kicks the display's tone mapper back to a known-good state.

## Usage

```
sanitize_metadata.exe [MaxCLL]
```

- With no argument, the peak luminance reported by Windows for each display is
  used for MaxCLL, MaxFALL and mastering luminance.
- `MaxCLL` is an optional peak brightness in nits (0 < value <= 10000). Pass
  the panel's rated peak if Windows reports something different, for example
  `sanitize_metadata.exe 250`.

Each display in HDR10 mode will go black and switch modes for about a second.
Displays not in HDR mode are reported and left alone.

Example output:

```
Sanitized Display: \\.\DISPLAY1
 MaxCLL=250 nits

Skipped Display: \\.\DISPLAY2 (ColorSpace=0, not HDR10)
```

Exit code is 0 on success and 1 if the argument is invalid.

## Requirements

- Windows 10 or newer with HDR enabled on at least one display
- A GPU with a Direct3D 11 driver

## Building

Open `sanitize_metadata.sln` in Visual Studio 2022 or newer and build the
Release x64 configuration, or from a developer prompt:

```
msbuild sanitize_metadata.sln /p:Configuration=Release /p:Platform=x64
```

The only dependencies are the Windows SDK, ATL and the DXGI/Direct3D 11 system
libraries.

## License

MIT, see [LICENSE.txt](LICENSE.txt).
