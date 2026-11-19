# PixelForge 3DS

An original pixel-art drawing tool for Nintendo 3DS, developed in C using **libctru**. The bottom touch screen hosts a 32×32 pixel canvas and 16-color palette; the upper screen shows controls and save status.

## Controls

| Input | Action |
|---|---|
| Stylus on canvas | Draw |
| Stylus on color palette | Choose color |
| D-pad | Move cursor |
| A | Paint |
| B | Erase to white |
| L / R | Previous / next color |
| X | Show / hide grid |
| Y | Save `drawing.ppm` |
| SELECT | Load `drawing.ppm` |
| START | Exit |

## Save file

Saved as **32×32 binary PPM (P6)** at:

`sdmc:/3ds/PixelForge3DS/drawing.ppm`

You can open PPM files with GIMP or ImageMagick and convert them to PNG. The file keeps full RGB color. **Only one save slot** is included in this initial version (saving overwrites the previous drawing).

## Build without a PC (GitHub Actions)

1. Create a **public or private GitHub repository** using the GitHub site on your phone or tablet.
2. Upload the files and folders from this project, **including `.github/workflows/build.yml`**, keeping the paths intact.
3. Open **Actions** and select **Build PixelForge 3DS** → **Run workflow** (or push a commit).
4. Open the successful workflow run and download the `PixelForge3DS-3dsx` build artifact.
5. Extract `PixelForge3DS.3dsx`, copy it into `/3ds/` on your 3DS SD card and launch it via the Homebrew Launcher on your own homebrew-enabled console.

A GitHub Actions build is **not guaranteed** until tested on GitHub; if dependency package names or container images change, the workflow may need updates.

## Build locally

With the devkitPro **3ds-dev** package installed:

```sh
make
```

Output: `PixelForge3DS.3dsx`. `make clean` removes generated binary files.

## Notes

- Source is provided, but it has **not been compiled or tested on an actual Nintendo 3DS** in this environment.
- It uses direct framebuffer drawing (no Citro2D dependency), so the on-screen art is designed for the 3DS's native 320×240 touch display.
- A `.cia` installer is not provided. The project targets the Homebrew Launcher `.3dsx` format.
