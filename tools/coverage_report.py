#!/usr/bin/env python3
"""Summarize snesrecomp coverage captures (tier2 manifests).

Usage:
  python tools/coverage_report.py build/coverage-cov [build/coverage-aot ...] [--site 0080E6]

Prints interpreted work, the fallback sites that remain (what still runs in the
interpreter and why), and the hottest interpreted instructions. `--site` filters
the fallback list to one dispatch site so its observed target set can be read
off directly when authoring an `indirect_dispatch` directive.
"""
import glob
import json
import os
import sys


def load(path):
    if os.path.isdir(path):
        files = sorted(glob.glob(os.path.join(path, "**", "*.json"), recursive=True))
        files = [f for f in files if "tier2 bundle" in open(f, encoding="utf-8", errors="ignore").read(200)
                 or True]
        if not files:
            sys.exit(f"no capture json under {path}")
        return [json.load(open(f, encoding="utf-8")) for f in files]
    return [json.load(open(path, encoding="utf-8"))]


def main(argv):
    want_site = None
    if "--site" in argv:
        i = argv.index("--site")
        want_site = argv[i + 1].lower().lstrip("$").rjust(6, "0")
        del argv[i:i + 2]
    if not argv:
        sys.exit(__doc__)
    for path in argv:
        for d in load(path):
            costs = {}
            for c in d.get("costs", []):
                if c.get("record_kind") == "instruction":
                    pc = c["target_pc24"].lower()
                    ins, cyc = costs.get(pc, (0, 0))
                    costs[pc] = (ins + c["interpreted_instructions"],
                                 cyc + c["guest_cycles"])
            total_ins = sum(v[0] for v in costs.values())
            total_cyc = sum(v[1] for v in costs.values())
            print(f"== {os.path.basename(path)}  capture={d['capture_id']}")
            print(f"   interpreted: {total_ins:,} instructions, {total_cyc:,} guest cycles"
                  f"   tier hits={d['total_tier_hits']:,}  distinct fallback sites={d['distinct_sites']}")
            disc = d["discoveries"]
            if want_site:
                disc = [x for x in disc if x["site_pc24"].lstrip("0x").lower().endswith(want_site[-4:])]
            by_site = {}
            for x in disc:
                by_site.setdefault(x["site_pc24"], []).append(x)
            print(f"   fallback sites ({len(by_site)}):")
            for site, rows in sorted(by_site.items(),
                                     key=lambda kv: -sum(r["observed_hits"] for r in kv[1])):
                kinds = {r["site_kind"] + "/" + r["fallback_reason"] for r in rows}
                hits = sum(r["observed_hits"] for r in rows)
                print(f"     {site}  hits={hits:<8} {sorted(kinds)}")
                for r in sorted(rows, key=lambda r: -r["observed_hits"])[:12]:
                    print(f"        -> {r['target_pc24']} {r['entry_mx']} hits={r['observed_hits']}"
                          f" frames {r['first_frame']}..{r['last_frame']}")
            if not want_site:
                print("   hottest interpreted instructions:")
                for pc, (ins, cyc) in sorted(costs.items(), key=lambda kv: -kv[1][1])[:10]:
                    print(f"     {pc}  {ins:,} insns / {cyc:,} cycles")


if __name__ == "__main__":
    main(sys.argv[1:])
