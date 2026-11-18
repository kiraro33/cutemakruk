#!/usr/bin/env python3
"""List the translatable strings of the GUI (tr("...") in projects/gui/src, <string> in projects/gui/ui).

Usage: python tools/makruk/gui_strings.py [--check projects/gui/res/i18n/th.tsv]
Without --check it prints one string per line (escaped as in the TSV: \\n, \\t, \\\\).
With --check it lists the strings that have no Thai translation yet.
Only GUI strings are translated; the library (result texts written to PGN files) stays English.
"""
import html
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
LIT = r'"((?:[^"\\]|\\.)*)"'
TR = re.compile(r'\btr\s*\(\s*((?:' + LIT + r'\s*)+)', re.S)


def unescape_c(s):
    return re.sub(r'\\(.)', lambda m: {"n": "\n", "t": "\t"}.get(m.group(1), m.group(1)), s)


def esc(s):
    return s.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


def strings():
    out = []
    src = os.path.join(ROOT, "projects", "gui", "src")
    for d, _, files in os.walk(src):
        for f in sorted(files):
            if f.endswith((".cpp", ".h")):
                s = open(os.path.join(d, f), encoding="utf-8").read()
                for m in TR.finditer(s):
                    out.append(unescape_c("".join(re.findall(LIT, m.group(1)))))
    ui = os.path.join(ROOT, "projects", "gui", "ui")
    for f in sorted(os.listdir(ui)):
        s = open(os.path.join(ui, f), encoding="utf-8").read()
        for m in re.finditer(r'<string((?:\s[^>]*?)?)(?<!/)>(.*?)</string>', s, re.S):
            if 'notr="true"' not in m.group(1):
                out.append(html.unescape(m.group(2)))
    seen, res = set(), []
    for x in out:
        if x.strip() and x not in seen and re.search("[A-Za-z]", x):
            seen.add(x)
            res.append(x)
    return res


def main():
    ss = strings()
    if len(sys.argv) > 2 and sys.argv[1] == "--check":
        done = set()
        for line in open(sys.argv[2], encoding="utf-8"):
            if "\t" in line and not line.startswith("#"):
                done.add(line.split("\t", 1)[0])
        missing = [s for s in ss if esc(s) not in done]
        for s in missing:
            print(esc(s))
        print("%d strings, %d missing" % (len(ss), len(missing)), file=sys.stderr)
        sys.exit(1 if missing else 0)
    for s in ss:
        print(esc(s))


if __name__ == "__main__":
    main()
