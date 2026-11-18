#!/usr/bin/env python3
"""Compares cutemakruk's Thai result of each PGN game (makrukjudge pgn) with the
CSV of Platoo's readjudicate.py (--csv): result, reason and the ply where the game
is over by the Thai rules 2569.

  python tools/makruk/compare_readjudicate.py --judge build/makrukjudge.exe games.pgn games_thai.csv
"""

import argparse
import csv
import io
import os
import subprocess
import sys
from collections import Counter

TOKEN = (("mates", "checkmate"), ("stalemate", "stalemate"), ("bare khuns", "bare-khuns"),
         ("counting", "counting"), ("repetition", "repetition"))


def token(desc):
    for k, v in TOKEN:
        if k in desc:
            return v
    return "?" + desc


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--judge", required=True)
    p.add_argument("pgn")
    p.add_argument("csv", help="readjudicate.py --csv output for the same PGN")
    p.add_argument("--show", type=int, default=10, help="differences to print")
    a = p.parse_args()
    out = subprocess.run([os.path.abspath(a.judge), "pgn", a.pgn], capture_output=True, text=True,
                         check=True).stdout
    mine = {int(r["game"]): r for r in csv.DictReader(io.StringIO(out))}
    with open(a.csv, encoding="utf-8") as fh:
        ref = {int(r["game"]): r for r in csv.DictReader(fh)}
    stats = Counter()
    shown = 0
    for g in sorted(set(mine) | set(ref)):
        m, r = mine.get(g), ref.get(g)
        if m is None or r is None or m["error"] or r["category"] == "error":
            stats["skipped (missing / replay error)"] += 1
            continue
        if r["thai_result"] == "*":
            ok = m["result"] == "*"
            kind = "not over by Thai rules"
        else:
            ok = (m["result"], token(m["reason"]), m["ply"]) == (r["thai_result"], r["thai_reason"], r["thai_ply"])
            kind = "over by Thai rules"
        stats["%s: %s" % (kind, "same" if ok else "DIFFERENT")] += 1
        if not ok and shown < a.show:
            shown += 1
            print("game %d: cutemakruk %s %s ply %s | readjudicate %s %s ply %s" % (
                g, m["result"], m["reason"], m["ply"], r["thai_result"], r["thai_reason"], r["thai_ply"]))
    print("%s: %d games" % (os.path.basename(a.pgn), len(set(mine) | set(ref))))
    for k, v in sorted(stats.items()):
        print("  %5d  %s" % (v, k))
    return 1 if any("DIFFERENT" in k for k in stats) else 0


if __name__ == "__main__":
    sys.exit(main())
