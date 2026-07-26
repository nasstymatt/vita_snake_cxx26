#!/usr/bin/env python3
"""Bake a fixed-grid bitmap font atlas from a TTF/OTF into a PNG.

Draws each glyph onto a fully transparent RGBA canvas, so anti-aliased
edges get real partial alpha instead of a keyed-out background color.
No black fringing, no manual erasing.

Defaults match the grid vita::ui::fonts already expects (see ui.cppm):
16 cols x 32px cells, ASCII 33.."ascii_start + glyph_count - 1".
"""
import argparse

from PIL import Image, ImageDraw, ImageFont


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("font", help="path to .ttf/.otf")
    ap.add_argument("-o", "--out", default="assets/font.png")
    ap.add_argument("--ascii-start", type=int, default=33)
    ap.add_argument("--glyph-count", type=int, default=92)
    ap.add_argument("--cols", type=int, default=16)
    ap.add_argument("--cell-size", type=int, default=32)
    ap.add_argument("--padding", type=int, default=4,
                     help="px kept empty on each side of a glyph's cell")
    args = ap.parse_args()

    rows = -(-args.glyph_count // args.cols)  # ceil div
    atlas_w = args.cols * args.cell_size
    atlas_h = rows * args.cell_size

    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(atlas)

    glyph_px = args.cell_size - args.padding * 2
    font = ImageFont.truetype(args.font, glyph_px)

    for i in range(args.glyph_count):
        ch = chr(args.ascii_start + i)
        col, row = i % args.cols, i // args.cols
        cell_x, cell_y = col * args.cell_size, row * args.cell_size

        bbox = draw.textbbox((0, 0), ch, font=font)
        w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
        x = cell_x + (args.cell_size - w) // 2 - bbox[0]
        y = cell_y + (args.cell_size - h) // 2 - bbox[1]

        draw.text((x, y), ch, font=font, fill=(255, 255, 255, 255))

    atlas.save(args.out)
    last = args.ascii_start + args.glyph_count - 1
    print(f"wrote {args.out}: {atlas_w}x{atlas_h}, {args.cols}x{rows} cells "
          f"of {args.cell_size}px, glyphs {args.ascii_start}..{last} "
          f"({chr(args.ascii_start)}..{chr(last)})")


if __name__ == "__main__":
    main()
