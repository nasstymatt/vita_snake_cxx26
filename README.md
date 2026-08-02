# Snake

Snake for the PS Vita. Uses bleeding-edge C++26, so it needs a very recent
toolchain.

## Requirements

- [VitaSDK](https://vitasdk.org) with GCC 15.2.0 or newer
- CMake 3.28+
- [vitaGL](https://github.com/Rinnegatamante/vitaGL), built from source (see below)
- [Vita3K](https://vita3k.org), to run it on desktop
- Python 3 with Pillow, only if you regenerate the font atlas

`$VITASDK` must be set, usually to `/usr/local/vitasdk`.

## vitaGL

Build vitaGL yourself. The `libvitaGL.a` that ships with VitaSDK crashes this
app at startup (currently no clue why).

```sh
git clone https://github.com/Rinnegatamante/vitaGL.git ~/.local/opt/vitaGL
cd ~/.local/opt/vitaGL && make
```

`~/.local/opt/vitaGL` is where the build looks by default. Anywhere else:

```sh
cmake -DVITAGL_DIR=/path/to/vitaGL ..
```

## Build and run

```sh
make          # produces build/snake.vpk
make debug    # builds, then launches it in Vita3K
```

`make` rebuilds from scratch every time, which keeps stale module caches from
causing confusing errors.

The Vita3K path is hardcoded at the top of the Makefile. Edit `vita3k :=` if
yours differs.

On hardware: copy `build/snake.vpk` across and install it with VitaShell.

## Controls

D-pad steers and moves through menus, ✕ confirms, Start pauses.

## Font atlas

`assets/font.png` is baked from `assets/font.otf` and compiled into the binary.
Only regenerate it if you change the font:

```sh
python3 tools/gen_font_atlas.py assets/font.otf -o assets/font.png
```

The grid it produces (16×6 cells of 32px) is also hardcoded in
`src/vita/ui.cppm`. Change one, change the other.

## Layout

```
src/vita/     engine: math, process, gfx, ui, ctrl, platform
src/snake/    the game: rules, assets, rendering, scenes
tools/        font atlas generator
```
