#!/usr/bin/env python3
"""Runs Platoo's Makruk rule test cases (makruk/tests/rules/*.json, rules v2.0 =
Thai association rules 2569) through cutemakruk's board code (makrukjudge pos).

Checked per case: every move of `moves` is legal, legal_moves, result + reason,
count_state (through the R5.8 FEN reader of rule_tests.py), fen and placement.
Not checked: `go` (engine search output, not a board rule).

  python tools/makruk/rule_cases.py --platoo C:/path/to/platoo-checkout --judge build/makrukjudge.exe
"""

import argparse
import os
import subprocess
import sys
from collections import Counter

REASON = (("mates", "checkmate"), ("stalemate", "stalemate"), ("bare khuns", "insufficient"),
          ("counting", "counting"), ("repetition", "repetition"))
RESULT = {"*": "ongoing", "1-0": "white_win", "0-1": "black_win", "1/2-1/2": "draw"}


def reason_of(desc):
    for k, v in REASON:
        if k in desc:
            return v
    return "none"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--platoo", required=True, help="Platoo checkout (has makruk/tools, makruk/tests/rules)")
    p.add_argument("--judge", required=True, help="makrukjudge executable")
    p.add_argument("--quiet", action="store_true")
    a = p.parse_args()
    sys.path.insert(0, os.path.join(a.platoo, "makruk", "tools"))
    import rule_tests as rt
    from perft_diff import norm_move

    cases, errors = rt.load_cases([os.path.join(a.platoo, "makruk", "tests", "rules")])
    if errors:
        print("\n".join(errors))
        return 2
    lines = ["%s|%s" % (rt.fen_of(c), " ".join(c["moves"])) for c in cases]
    out = subprocess.run([os.path.abspath(a.judge), "pos"], input="\n".join(lines) + "\n", capture_output=True, text=True,
                         check=True).stdout.splitlines()
    assert len(out) == len(cases), (len(out), len(cases))
    stats = Counter()
    checks = Counter()
    for c, line in zip(cases, out):
        ex = c["expect"]
        fails = []
        if line.startswith("error="):
            fails.append(line)
        else:
            d = dict(kv.split("=", 1) for kv in line.split("|"))
            legal = {norm_move(m) for m in d["legal"].split()}
            lm = ex.get("legal_moves")
            if lm:
                if "exact" in lm and legal != {norm_move(m) for m in lm["exact"]}:
                    fails.append("legal_moves exact")
                if "must_include" in lm and not {norm_move(m) for m in lm["must_include"]} <= legal:
                    fails.append("legal_moves must_include")
                if "must_exclude" in lm and {norm_move(m) for m in lm["must_exclude"]} & legal:
                    fails.append("legal_moves must_exclude")
                if "count" in lm and len(legal) != lm["count"]:
                    fails.append("legal_moves count %d != %d" % (len(legal), lm["count"]))
                checks["legal_moves"] += 1
            got = (RESULT[d["result"]], reason_of(d["reason"]))
            checks["result"] += 1
            if got != (ex["result"], ex["reason"]):
                fails.append("result %s/%s want %s/%s (%s)" % (got + (ex["result"], ex["reason"], d["reason"])))
            if "count_state" in ex:
                checks["count_state"] += 1
                m = rt.COUNTING_RE.match("Counting: " + d["counting"])
                got = {"countType": "none"} if m.group(1) else {
                    "countType": m.group(2), "countSide": m.group(3), "countLimit": int(m.group(4)),
                    "countNext": int(m.group(5))}
                diffs = rt.cmp_count_state(ex["count_state"], got, rt.COUNT_KEYS)
                if diffs:
                    fails.append("count_state " + "; ".join(diffs))
            if "fen" in ex:
                checks["fen"] += 1
                if d["fen"] != ex["fen"]:
                    fails.append("fen %s want %s" % (d["fen"], ex["fen"]))
            if "placement" in ex:
                checks["placement"] += 1
                if " ".join(d["fen"].split()[:2]) != ex["placement"]:
                    fails.append("placement")
        stats["FAIL" if fails else "PASS"] += 1
        if fails or not a.quiet:
            print("%-5s %s %s" % ("FAIL" if fails else "PASS", c["id"], " | ".join(fails)))
    print("\n%d cases: %d PASS, %d FAIL; checks: %s" % (len(cases), stats["PASS"], stats["FAIL"],
                                                      ", ".join("%s %d" % kv for kv in sorted(checks.items()))))
    return 1 if stats["FAIL"] else 0


if __name__ == "__main__":
    sys.exit(main())
