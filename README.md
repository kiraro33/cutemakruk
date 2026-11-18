# cutemakruk

cutemakruk is a Makruk-only fork of the command-line tool of
[Cute Chess](https://github.com/cutechess/cutechess) 1.5.1 (`cutechess-cli`).
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

`makruk` is the only variant and the default one. The binary is `cutemakruk`;
the options are those of `cutechess-cli` (see `cutemakruk --help`).

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
    cmake --build build --target cli makrukjudge

cutemakruk is free software under the GNU General Public License v3 (see
`COPYING`), like Cute Chess. All credit for the match engine goes to the Cute Chess
authors (Ilari Pihlajisto, Arto Jonsson and contributors); the original README
follows. The GUI is not maintained in this fork.

---

# Cute Chess (original README)

Cute Chess is a graphical user interface, command-line interface and a library
for playing chess. Cute Chess is written in C++ using the [Qt
framework](https://www.qt.io/).

## Installation

### Binaries

See the [Releases](https://github.com/cutechess/cutechess/releases) page.

### Building from source

Cute Chess requires Qt 6.8 or greater, a compiler with C++17 support and `cmake`.
Cute Chess depends on the following Qt modules:

* qt-widgets
* qt-svg
* qt-concurrent
* qt-printsupport
* qt5-compat
* qt-testlib (optional: unit tests)

Run these commands:

    cmake -S . -B build
    cmake --build build

Documentation is available as plain-text, HTML and Unix manual pages in the
`docs/` directory.

For detailed build instruction for various operating systems, see the
[Building from source](https://github.com/cutechess/cutechess/wiki/Building-from-source)
wiki page.

## Running

The `cutechess` program is the graphical user interface of Cute Chess.
It can be run either from command-line or from your desktop environment's
application launcher.

The `cutechess-cli` program is the command-line interface for playing
games between chess engines. For example, to play ten games between two Sloppy
engines (assuming `sloppy` is in PATH) with a time control of 40 moves in 60
seconds:

    $ cutechess-cli -engine cmd=sloppy -engine cmd=sloppy -each proto=xboard tc=40/60 -rounds 10

See `cutechess-cli -help` for descriptions of the supported options or manuals
for full documentation.

## License

Cute Chess is released under the GPLv3+ license except for the components in
the `projects/lib/components` and `projects/gui/components` directories which
are released under the MIT License.

## Credits

Cute Chess was written by Ilari Pihlajisto, Arto Jonsson and [contributors](https://github.com/cutechess/cutechess/graphs/contributors)
