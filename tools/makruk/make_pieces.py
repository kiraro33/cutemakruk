#!/usr/bin/env python3
"""Build projects/gui/res/chessboard/makruk-fulmene.svg from the Fulmene Makruk piece images.

Source: https://github.com/Fulmene/makruk-pieces-image (Ada Joule, CC BY-SA 4.0).
Usage:  git clone https://github.com/Fulmene/makruk-pieces-image src
        python make_pieces.py src projects/gui/res/chessboard/makruk-fulmene.svg

Each piece file (<piece>_w.svg for White, <piece>_b.svg for Black) becomes one <g> whose id is the
graphical symbol the GUI asks for (Board::representation: K E F R N P, lower case for Black). The ids
inside each file are prefixed so the gradients and filters of different pieces do not clash.
Changes from the source: files merged into one, ids renamed, Inkscape metadata removed, an invisible
frame added around the Bia so that it is drawn smaller than the other pieces.
"""
import re
import sys

PIECES = [("K", "khun"), ("F", "met"), ("E", "khon"), ("R", "rua"), ("N", "ma"), ("P", "bia")]
STEP = 120.0  # piece files are 95.25 x 95.25 user units; keep the pieces apart
# The GUI scales every element to 80% of a square by its bounds. The Bia disc would then be as
# big as the Khun, so it gets an invisible square frame (centred on the disc, which sits in the
# middle of its 95.25 source frame) that makes it draw at about half a square.
BIA_FRAME = 76.0


def body(path, prefix):
    s = open(path, encoding="utf-8").read()
    s = re.sub(r"<\?xml[^>]*\?>|<!--.*?-->", "", s, flags=re.S)
    s = re.sub(r"<sodipodi:namedview\b[^>]*[^/]>.*?</sodipodi:namedview>", "", s, flags=re.S)
    s = re.sub(r"<sodipodi:namedview\b[^>]*/>", "", s, flags=re.S)
    s = re.sub(r"<metadata\b.*?</metadata>", "", s, flags=re.S)
    inner = re.search(r"<svg\b[^>]*>(.*)</svg>", s, flags=re.S).group(1)
    inner = re.sub(r'\b(id)="([^"]*)"', lambda m: 'id="%s%s"' % (prefix, m.group(2)), inner)
    inner = re.sub(r"url\(#([^)]*)\)", lambda m: "url(#%s%s)" % (prefix, m.group(1)), inner)
    inner = re.sub(r'(xlink:href|href)="#([^"]*)"',
                   lambda m: '%s="#%s%s"' % (m.group(1), prefix, m.group(2)), inner)
    # drop editor-only attributes (inkscape:*, sodipodi:*)
    inner = re.sub(r'\s(inkscape|sodipodi):[\w-]+="[^"]*"', "", inner)
    return inner


def main(src, out):
    parts = []
    for row, (side, suffix) in enumerate((("w", str.upper), ("b", str.lower))):
        for col, (sym, name) in enumerate(PIECES):
            gid = suffix(sym)
            frame = ""
            if sym == "P":
                o = (95.25 - BIA_FRAME) / 2
                frame = ('<rect x="%g" y="%g" width="%g" height="%g" fill="#000000" fill-opacity="0"/>'
                         % (o, o, BIA_FRAME, BIA_FRAME))
            parts.append('<g id="%s" transform="translate(%g,%g)">%s%s</g>' % (
                gid, col * STEP, row * STEP, frame,
                body("%s/%s_%s.svg" % (src, name, side), "%s%s_" % (name, side))))
    svg = ('<?xml version="1.0" encoding="UTF-8"?>\n'
           '<!-- Makruk pieces by Ada Joule (github.com/Fulmene/makruk-pieces-image), CC BY-SA 4.0,\n'
           '     https://creativecommons.org/licenses/by-sa/4.0/ . Merged into one file for cutemakruk\n'
           '     by tools/makruk/make_pieces.py (ids renamed, editor metadata removed). -->\n'
           '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"'
           ' version="1.1" width="%g" height="%g" viewBox="0 0 %g %g">\n<title>Makruk pieces (Fulmene)</title>\n'
           % (len(PIECES) * STEP, 2 * STEP, len(PIECES) * STEP, 2 * STEP)
           + "\n".join(parts) + "\n</svg>\n")
    open(out, "w", encoding="utf-8", newline="\n").write(svg)
    print("wrote %s (%d bytes)" % (out, len(svg)))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
