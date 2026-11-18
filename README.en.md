<p align="center"><img src="docs/cutemakruk-logo.png" alt="Cute Makruk" width="320"></p>

<p align="center"><a href="README.md">ภาษาไทย</a> | <b>English</b></p>

# Cute Makruk

Cute Makruk is a Makruk-only fork of
[Cute Chess](https://github.com/cutechess/cutechess) 1.5.1: the command-line tool
(`cutemakruk`, from `cutechess-cli`) and the GUI (`cutemakruk-gui`).
It plays engine matches in Makruk (Thai chess) and ends the games by the
**Thai association rules 2569**, as used by the
[Platoo](https://github.com/kiraro33/platoo-engine) engine:

* counting: board's honour (64 moves after the last Bia leaves the board, not
  restarted by later captures) and pieces' honour (a bare Khun counts from the
  number of pieces + 1 up to 8/16/22/32/44/64 by the strong side's material);
* Khun vs Khun is a draw; there is no other insufficient-material draw;
* 3-fold repetition of the board placement (side to move ignored) is a draw;
* checkmate always wins, also when the count is over;
* the FEN fields 4 and 5 carry the counting state (`- 0`, `128 <plies>` or
  `<2 x limit> <count plies>`), so a position with a running count can be
  given to an engine and written to the PGN.

## Why Cute Makruk: differences from Cute Chess

Cute Chess can play Makruk, but its Makruk rules are not the Thai rules, so match
results had to be fixed afterwards. The problems it had, and what Cute Makruk does:

| | Cute Chess 1.5.1 (`makruk` variant) | Cute Makruk |
|---|---|---|
| Counting | its own counting / "sixty-four move rule", not the Thai board's and pieces' honour | Thai rules 2569: board's honour (64 moves, not restarted by captures) and pieces' honour (8 to 64 by material) |
| Insufficient material | "Draw by insufficient mating material" in positions that are not draws in Thai rules | only Khun vs Khun is a draw |
| Repetition | no repetition draw for Makruk, so long shuffles ran to `-maxmoves` or needed `-draw` adjudication | 3-fold repetition of the board placement is a draw |
| Counting in FEN | not stored | FEN fields 4/5 carry the count, so a book position keeps its running count |
| Results | had to be re-judged by a separate script; in a 1,000-game test 146 results changed | the result is already the Thai result: 0 of 1,000 changed |
| Game length | 322 plies on average, 66.5 min for 1,000 games at 10+0.1 | 194 plies on average, 45.9 min for the same match |
| Variants | 75 chess variants, `standard` by default | Makruk only, the default |
| GUI | chess pieces, English | Makruk pieces, Thai by default (English selectable), Thai board labels |

The numbers come from the same 1,000-game match (same engines, book and seed)
played once with Cute Chess and once with Cute Makruk; both PGNs were checked
position by position against an independent model of the Thai rules.

`makruk` is the only variant and the default one. The binary is `cutemakruk`;
the options are those of `cutechess-cli` (see `cutemakruk --help`).

The GUI `cutemakruk-gui` plays and shows Makruk games (human or engine) with the
same rules, using the Makruk piece images by Ada Joule
([Fulmene/makruk-pieces-image](https://github.com/Fulmene/makruk-pieces-image),
CC BY-SA 4.0; merged into `projects/gui/res/chessboard/makruk-fulmene.svg` by
`tools/makruk/make_pieces.py`, see `makruk-fulmene.txt`). Help > About shows the
version, credits and licence tabs. The GUI is in Thai by default; View > Language
> English (or `cutemakruk-gui --lang en`) switches to English. The Thai texts are in
`projects/gui/res/i18n/th.tsv` (`tools/makruk/gui_strings.py --check` lists missing ones).

Result texts: `White mates` / `Black mates`, `Draw by stalemate`,
`Draw by bare khuns`, `Draw by board's honour counting`,
`Draw by pieces' honour counting`, `Draw by 3-fold repetition`.

Tests (`tools/makruk/`, they need a Platoo checkout for the rule cases):

    python tools/makruk/rule_cases.py --platoo <platoo checkout> --judge build/makrukjudge.exe
    python tools/makruk/compare_readjudicate.py --judge build/makrukjudge.exe games.pgn games_thai.csv

`makrukjudge` is a small driver around the board code: `makrukjudge pos` reads
`<fen>|<uci moves>` lines and prints FEN, result, counting state and legal moves;
`makrukjudge pgn FILE...` prints, for each game, the first position that is over
by the Thai rules.

Build (MSYS2 MinGW64, Qt 6):

    pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-svg mingw-w64-x86_64-qt6-5compat mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=OFF -S . -B build
    cmake --build build --target cli makrukjudge gui

## Credits

* **[Cute Chess](https://github.com/cutechess/cutechess)** by Ilari Pihlajisto,
  Arto Jonsson and contributors: Cute Makruk is a fork of Cute Chess 1.5.1 and
  all credit for the match engine, the GUI and the library goes to them.
* **Makruk piece images** by Ada Joule
  ([Fulmene/makruk-pieces-image](https://github.com/Fulmene/makruk-pieces-image)),
  CC BY-SA 4.0.
* **[Qt](https://www.qt.io)** by The Qt Company and contributors (LGPL v3).

## License

Cute Makruk is free software under the GNU General Public License v3 (see
`COPYING`), like Cute Chess. The Makruk piece images are licensed under
CC BY-SA 4.0. Cute Makruk changes Copyright (C) 2026 Kiraro33 (O.T.).
