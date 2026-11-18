#!/usr/bin/env python3
"""Checks what the GUI shows besides the rules: promoted Bias (own image) and the running count.

Usage: python tools/makruk/display_cases.py --judge build/makrukjudge.exe
Each case is "<fen>|<moves>" for `makrukjudge pos` ("undo" takes back a move) with the expected
promoted squares and count ("pieces b 3/64" = pieces' honour, Black counts, 3 of 64).
"""
import argparse
import os
import subprocess
import sys

PROMO = "8/8/2k5/8/4P3/8/8/4K3 w - - 0 1|e4e5 c6d7 e5e6"
CASES = [
    ("no count, nothing promoted", "8/8/2k5/8/4P3/8/8/4K3 w - - 0 1|", "", "none"),
    ("Bia promotes, Black is bare: pieces' honour starts", PROMO, "e6", "pieces b 3/64"),
    ("the promoted Bia moves", PROMO + " d7c7 e6f7", "f7", "pieces b 4/64"),
    ("undo puts it back", PROMO + " d7c7 e6f7 undo", "e6", "pieces b 4/64"),
    ("undo the promotion", PROMO + " undo", "", "none"),
    ("a captured promoted Bia is gone (both bare: no count)", PROMO + " d7e6", "", "none"),
    ("board's honour: no Bias, both sides have pieces",
     "4k3/8/8/3r4/8/8/3R4/4K3 w - - 0 1|d2d3", "", "board w 0/64"),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--judge", required=True)
    args = ap.parse_args()
    judge = os.path.abspath(args.judge)
    lines = "\n".join(c[1] for c in CASES) + "\n"
    out = subprocess.run([judge, "pos"], input=lines, capture_output=True, text=True).stdout
    fails = 0
    for (name, _, promoted, display), line in zip(CASES, out.splitlines()):
        d = dict(kv.split("=", 1) for kv in line.split("|"))
        ok = d.get("promoted") == promoted and d.get("display") == display
        fails += not ok
        print("%s  %s  (promoted=%r display=%r)" % ("PASS" if ok else "FAIL", name,
                                                    d.get("promoted"), d.get("display")))
    print("%d cases: %d PASS, %d FAIL" % (len(CASES), len(CASES) - fails, fails))
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
