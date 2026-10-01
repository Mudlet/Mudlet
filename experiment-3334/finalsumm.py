import glob
import os

T = "/home/vadi/.claude/jobs/3ae74264/tmp/results13/"


def load(f):
    d = {}
    if not os.path.exists(f):
        return None
    for l in open(f, errors="replace"):
        p = l.split()
        if len(p) == 3 and p[0] == "METRIC":
            try:
                d[p[1]] = float(p[2])
            except ValueError:
                pass
        if "FAIL!" in l:
            d["FAIL"] = 1
    return d


def g(d, k, default=0.0):
    return d.get(k, default) if d else default


names = sorted(os.path.basename(f)[5:-4] for f in glob.glob(T + "main-*.txt"))
names.append("aetherspace-teleport")
print("map | rooms | today wrong /pairs, worst | mode8 wrong | median today -> mode8 | p95 today -> mode8 | audit overest. today / mode8 | edit+getPath today -> mode8 | landmark SCCs / skipped (tight largest)")
tot_pairs = tot_wrong8 = tot_wrong0 = 0
worse = []
for m in names:
    d = load(T + (f"main-{m}.txt" if m != "aetherspace-teleport" else "teleport-aetherspace.txt"))
    a = load(T + f"audit-{m}.txt")
    mp = load(T + f"mapping-{m}.txt") if m != "aetherspace-teleport" else None
    if not d:
        print(f"{m}: MISSING")
        continue
    pairs = g(d, "uniform_pairs") + g(d, "samearea_pairs")
    w0 = g(d, "uniform_current_suboptimal") + g(d, "samearea_current_suboptimal")
    w8 = g(d, "uniform_altgeo_suboptimal") + g(d, "samearea_altgeo_suboptimal")
    worst = max(g(d, "uniform_current_worst_ratio", 1), g(d, "samearea_current_worst_ratio", 1))
    tot_pairs += pairs
    tot_wrong8 += w8
    tot_wrong0 += w0
    cells = []
    for stat in ["best_median_ms", "best_p95_ms"]:
        parts = []
        for c in ["uniform", "samearea"]:
            c0, c8 = g(d, f"{c}_current_{stat}"), g(d, f"{c}_altgeo_{stat}")
            parts.append(f"{c0:.3f}->{c8:.3f}")
            if c0 > 0 and c8 > 1.2 * c0 and c8 - c0 > 0.05:
                worse.append(f"{m} {c} {stat} {c0:.3f}->{c8:.3f}")
        cells.append(" / ".join(parts))
    aud = f"{g(a, 'audit_current_admissible_violations'):.0f} / {g(a, 'audit_altgeo_admissible_violations', -1):.0f}" if a else "-"
    edit = f"{g(mp, 'mapping_current_median_ms'):.1f}->{g(mp, 'mapping_alt_median_ms'):.1f} (x{g(mp, 'mapping_alt_median_ms') / max(g(mp, 'mapping_current_median_ms'), 1e-9):.2f})" if mp else "-"
    fail = " FAIL" if d.get("FAIL") or (a and a.get("FAIL")) or (mp and mp.get("FAIL")) else ""
    print(f"{m} | {g(d, 'map_rooms'):.0f} | {w0:.0f}/{pairs:.0f}, {worst:.2f}x | {w8:.0f} | {cells[0]} | {cells[1]} | {aud} | {edit} | "
          f"{g(d, 'alt_components_with_landmarks'):.0f} / {g(d, 'geo_scc_skipped'):.0f} ({g(d, 'geo_tight_largest', -1):.2f}){fail}")
print(f"\nTOTAL pairs {tot_pairs:.0f}: today wrong {tot_wrong0:.0f}, mode 8 wrong {tot_wrong8:.0f}")
print("mode 8 slower than today by >20% and >0.05ms:")
for w in worse:
    print("  " + w)
