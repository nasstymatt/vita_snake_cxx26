#!/usr/bin/env python3
"""Bake a fixed-grid bitmap font atlas from a TTF/OTF into a PNG.

Draws each glyph onto a fully transparent RGBA canvas, so anti-aliased
edges get real partial alpha instead of a keyed-out background color.
No black fringing, no manual erasing.

Glyphs are centred horizontally in their cell but sit on a *shared
baseline* vertically, so '.', 'x' and 'P' line up the way they would in
real text and descenders actually descend. Centring each glyph on its own
ink box instead -- the obvious thing -- leaves periods floating mid-line.

Defaults match the grid vita::ui::fonts already expects (see ui.cppm):
16 cols x 32px cells, ASCII 33.."ascii_start + glyph_count - 1".
"""
import argparse

from PIL import Image, ImageDraw, ImageFont


def fit_font(path: str, box_h: int):
    """Largest em size whose ascent + descent fits box_h.

    Sizing by em alone would overflow: this font reports asc+desc = 33 at
    em=32, so a baseline-aligned 32px cell would clip descenders by a pixel.
    Ask the metrics instead of assuming em == line height.
    """
    for em in range(box_h * 2, 0, -1):
        font = ImageFont.truetype(path, em)
        ascent, descent = font.getmetrics()
        if ascent + descent <= box_h:
            return font, em, ascent, descent
    raise SystemExit(f"no size of {path} fits a {box_h}px line box")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("font", help="path to .ttf/.otf")
    ap.add_argument("-o", "--out", default="assets/font.png")
    ap.add_argument("--ascii-start", type=int, default=33)
    ap.add_argument("--glyph-count", type=int, default=92)
    ap.add_argument("--cols", type=int, default=16)
    ap.add_argument("--cell-size", type=int, default=32)
    ap.add_argument("--padding", type=int, default=0,
                     help="px kept empty above and below the line box, which "
                          "shrinks the font to suit")
    args = ap.parse_args()

    rows = -(-args.glyph_count // args.cols)  # ceil div
    atlas_w = args.cols * args.cell_size
    atlas_h = rows * args.cell_size

    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(atlas)

    box_h = args.cell_size - args.padding * 2
    font, em, ascent, descent = fit_font(args.font, box_h)

    # Centre the line box in whatever room the cell has left over, then put
    # every glyph's baseline at the same offset within it.
    baseline_y = args.padding + (box_h - (ascent + descent)) // 2 + ascent

    for i in range(args.glyph_count):
        ch = chr(args.ascii_start + i)
        col, row = i % args.cols, i // args.cols
        cell_x, cell_y = col * args.cell_size, row * args.cell_size

        # Horizontal placement still centres on the glyph's own ink, which is
        # what makes a proportional face read as evenly spaced in a fixed grid.
        # Only the vertical axis is shared.
        bbox = draw.textbbox((0, 0), ch, font=font)
        x = cell_x + (args.cell_size - (bbox[2] - bbox[0])) // 2 - bbox[0]

        draw.text((x, cell_y + baseline_y), ch, font=font, anchor="ls",
                  fill=(255, 255, 255, 255))

    atlas.save(args.out)
    last = args.ascii_start + args.glyph_count - 1
    print(f"wrote {args.out}: {atlas_w}x{atlas_h}, {args.cols}x{rows} cells "
          f"of {args.cell_size}px, glyphs {args.ascii_start}..{last} "
          f"({chr(args.ascii_start)}..{chr(last)})")
    print(f"  em={em}px (ascent {ascent} + descent {descent} = {ascent + descent} "
          f"in a {box_h}px box), baseline at y={baseline_y} within each cell")


if __name__ == "__main__":
    main()
