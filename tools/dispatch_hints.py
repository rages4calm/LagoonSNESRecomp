#!/usr/bin/env python3
"""Turn a coverage capture into cfg-ready promotion hints.

Usage:
  python tools/dispatch_hints.py build/coverage-aot [--manifest src/gen/program_manifest.json]

Two things keep a snesrecomp port from matching its interpreter baseline, and
the capture says which is which:

  call_gap/missing_exact_variant   the callee is compiled, but not in the
                                   (m,x) variant the caller was in. That is a
                                   symbols.toml `[[func]]` with entry_m/entry_x.

  indirect_goto / goto_gap         an indirect jump whose target set the
                                   analyzer could not prove. That is an
                                   `indirect_dispatch` directive in the bank cfg
                                   — `idx:`+`tables:` when the table is in ROM,
                                   `ptrtail targets:` when it is enumerated.

Prints both, grouped, with hit counts, ready to paste. Nothing here is applied
automatically: promotion is a human decision between "observed at runtime" and
"trusted as code", and each hint still needs a replay comparison.
"""
import glob
import json
import os
import sys
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load_capture(path):
    files = sorted(glob.glob(os.path.join(path, "**", "*.json"), recursive=True)) \
        if os.path.isdir(path) else [path]
    rows = []
    for f in files:
        d = json.load(open(f, encoding="utf-8"))
        rows.extend(d.get("discoveries", []))
    return rows


def main(argv):
    if not argv or argv[0].startswith("-"):
        sys.exit(__doc__)
    rows = load_capture(argv[0])
    mpath = os.path.join(REPO, "src", "gen", "program_manifest.json")
    if "--manifest" in argv:
        mpath = argv[argv.index("--manifest") + 1]
    nodes = {}
    if os.path.exists(mpath):
        man = json.load(open(mpath, encoding="utf-8"))
        for key, node in man.get("nodes", {}).items():
            # manifest keys look like "028669:M1X0" (uppercase, 6 hex digits)
            addr, mx = key.split(":")
            nodes.setdefault(addr.upper(), []).append(mx)

    variants = defaultdict(lambda: defaultdict(int))   # target -> mx -> hits
    gotos = defaultdict(lambda: defaultdict(int))      # site -> target -> hits
    for r in rows:
        reason, kind = r["fallback_reason"], r["site_kind"]
        tgt, mx = r["target_pc24"].lower(), r["entry_mx"]
        site = r["site_pc24"].lower()
        if reason == "missing_exact_variant":
            variants[tgt][mx] += r["observed_hits"]
        elif kind in ("indirect_goto", "goto_gap"):
            gotos[site][tgt] += r["observed_hits"]

    print("# symbols.toml — callees compiled in the wrong width variant")
    for tgt, mxs in sorted(variants.items(), key=lambda kv: -sum(kv[1].values())):
        have = ",".join(sorted(nodes.get(tgt.lstrip("0"), []))) or "not compiled"
        for mx, hits in sorted(mxs.items(), key=lambda kv: -kv[1]):
            m, x = mx.lstrip("M").split("X")
            addr = tgt.replace("0x", "")
            print(f'[[func]]\nname = "F_{addr.upper()}"\naddr = "{addr[2:]}"'
                  f"\nbank = {int(addr[:2], 16)}\nemit = true\nentry_m = {m}\nentry_x = {x}"
                  f"\nnote = \"call_gap hits={hits}; compiled variants: {have}\"")

    print("\n# recomp/bankNN.cfg — indirect jumps the analyzer could not prove")
    for site, tgts in sorted(gotos.items(), key=lambda kv: -sum(kv[1].values())):
        hits = sum(tgts.values())
        targets = ",".join(sorted(t.replace("0x", "").lstrip("0").rjust(6, "0")
                                  for t, _ in sorted(tgts.items(), key=lambda kv: -kv[1])))
        bank = site.replace("0x", "")[:2]
        print(f"# site {site} hits={hits} observed targets: {targets}")
        print(f"indirect_dispatch {site.replace('0x','')[2:].lstrip('0') or '0'} "
              f"{len(tgts)} ptrtail targets:{targets}   # bank {bank}")
    print("\n# Verify each change by replaying the same route and diffing the")
    print("# present CRCs and scene dumps against the interpreter baseline.")


if __name__ == "__main__":
    main(sys.argv[1:])
