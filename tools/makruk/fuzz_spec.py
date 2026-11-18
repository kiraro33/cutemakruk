#!/usr/bin/env python3
"""Random-playout fuzz: plays capture-biased random Makruk games with the legal moves of
makrukjudge and compares, after every ply, cutemakruk's result, counting state and FEN
with the spec model of Platoo's rule_tests.py (rules v2.0 = Thai rules 2569).

  python tools/makruk/fuzz_spec.py --platoo <platoo checkout> --judge build/makrukjudge.exe --games 200
"""

import argparse
import os
import random
import subprocess
import sys

RESULT = {"*": "ongoing", "1-0": "white_win", "0-1": "black_win", "1/2-1/2": "draw"}
REASON = (("mates", "checkmate"), ("stalemate", "stalemate"), ("bare khuns", "insufficient"),
          ("counting", "counting"), ("repetition", "repetition"))


def reason_of(desc):
    return next((v for k, v in REASON if k in desc), "none")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--platoo", required=True)
    p.add_argument("--judge", required=True)
    p.add_argument("--games", type=int, default=100)
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--maxply", type=int, default=600)
    a = p.parse_args()
    sys.path.insert(0, os.path.join(a.platoo, "makruk", "tools"))
    import rule_tests as rt
    from perft_diff import placement

    rng = random.Random(a.seed)
    proc = subprocess.Popen([os.path.abspath(a.judge), "pos"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            text=True, bufsize=1)

    def ask(fen, moves):
        proc.stdin.write("%s|%s\n" % (fen, " ".join(moves)))
        proc.stdin.flush()
        return dict(kv.split("=", 1) for kv in proc.stdout.readline().rstrip("\n").split("|"))

    fails, plies, ends = 0, 0, {}
    for g in range(a.games):
        fen = rt.START_FEN
        board, stm, st, full = rt.read_fen_state(fen)
        hist = [placement(board)]
        moves = []
        while True:
            d = ask(fen, moves)
            plies += 1
            legal = d["legal"].split()
            ksq = next(sq for sq, pc in board.items() if pc == ("K" if stm == "w" else "k"))
            in_check = rt.attacked_by(board, ksq, rt.other(stm))
            want = rt.result_model(st, board, stm, len(legal), in_check, hist.count(hist[-1]))
            got = (RESULT[d["result"]], reason_of(d["reason"]))
            m = rt.COUNTING_RE.match("Counting: " + d["counting"])
            got_cs = {"countType": "none"} if m.group(1) else {
                "countType": m.group(2), "countSide": m.group(3), "countLimit": int(m.group(4)),
                "countNext": int(m.group(5))}
            want_fen = rt.write_fen_model(board, stm, st, full)
            bad = []
            if got != want:
                bad.append("result %s want %s" % (got, want))
            if got_cs != st.as_dict():
                bad.append("counting %s want %s" % (got_cs, st.as_dict()))
            if d["fen"] != want_fen:
                bad.append("fen %s want %s" % (d["fen"], want_fen))
            if bad:
                fails += 1
                print("game %d ply %d: %s (moves %s)" % (g + 1, len(moves), "; ".join(bad), " ".join(moves)))
                break
            if want[0] != "ongoing" or len(moves) >= a.maxply:
                key = want[1] if want[0] != "ongoing" else "maxply"
                ends[key] = ends.get(key, 0) + 1
                break
            caps = [mv for mv in legal if mv[2:4] in board]
            mv = rng.choice(caps if caps and rng.random() < 0.7 else legal)
            stm, st = rt.do_move_model(board, stm, st, mv[:4])
            if stm == "w":
                full += 1
            hist.append(placement(board))
            moves.append(mv)
    proc.stdin.close()
    proc.wait()
    print("%d games, %d positions checked, %d failures; game ends: %s" % (
        a.games, plies, fails, ", ".join("%s %d" % kv for kv in sorted(ends.items()))))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
