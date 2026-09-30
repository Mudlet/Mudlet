import os
T = "/home/vadi/.claude/jobs/3ae74264/tmp/"
MAPS = ["sendar", "achaea", "aetolia", "imperian", "lusternia", "starmourn", "aetherspace"]


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
    return d


def row(d, c, mo):
    k = f"{c}_{mo}_"
    if not d or k + "suboptimal" not in d:
        return None
    return dict(sub=d[k + "suboptimal"], worst=d[k + "worst_ratio"], fail=d[k + "failed"], touched=d[k + "touched_total"],
                med=d[k + "best_median_ms"], p95=d[k + "best_p95_ms"], mx=d[k + "best_max_ms"], tot=d[k + "best_total_ms"])


def fmt(r, cur):
    return (f"sub {r['sub']:.0f} worst {r['worst']:.3f} fail {r['fail']:.0f} | med {r['med']:.3f} p95 {r['p95']:.3f} max {r['mx']:.3f} ms"
            f" (med x{r['med'] / cur['med']:.2f} p95 x{r['p95'] / cur['p95']:.2f}) touched x{r['touched'] / cur['touched']:.3f}")


print("=== MAIN per-SCC (results8) vs largest-SCC (results6), K=8")
for m in MAPS:
    d, o = load(T + f"results8/main-{m}.txt"), load(T + f"results6/main-{m}.txt")
    print(f"\n{m}: rooms {d['map_rooms']:.0f}, SCCs {d.get('alt_components', 0):.0f}, with landmarks {d.get('alt_components_with_landmarks', 0):.0f}, "
          f"landmarks {d['alt_landmarks_built']:.0f} (old {o['alt_landmarks_built']:.0f}); build {d['alt_pass_ms']:.1f}ms (old {o['alt_pass_ms']:.1f}) "
          f"of initGraph {d['init_graph_ms']:.1f}ms (old {o['init_graph_ms']:.1f}); tables {d['alt_bytes'] / 1e6:.1f}MB (old {o['alt_bytes'] / 1e6:.1f}); "
          f"peak RSS {d.get('peak_rss_kb', 0) / 1024:.0f}MB; busy {d['machine_busy_pct_before']:.0f}/{d['machine_busy_pct_after']:.0f}")
    classes = ["uniform", "samearea"] + [c for c in ["scen_adjacent", "scen_near", "scen_mid", "scen_far", "scen_corner"] if f"{c}_current_suboptimal" in d]
    for c in classes:
        cur = row(d, c, "current")
        pairs = d.get(c + "_pairs", 0)
        for mo in ["current", "zero", "chebtie", "alt"]:
            r = row(d, c, mo)
            if r:
                print(f"  {c:13} {mo:8} /{pairs:.0f} {fmt(r, cur)}")
        ro, co = row(o, c, "alt"), row(o, c, "current")
        if ro:
            print(f"  {c:13} OLD-alt  /{o.get(c + '_pairs', 0):.0f} {fmt(ro, co)}")

print("\n=== AUDIT")
for m in MAPS + ["aetherspace-teleport"]:
    d = load(T + f"results8/audit-{m}.txt")
    parts = []
    for mo in ["current", "chebtie", "alt"]:
        k = f"audit_{mo}_"
        parts.append(f"{mo}: adm {d[k + 'admissible_violations']:.0f}/{d[k + 'admissible_checked']:.0f} (+{d[k + 'worst_excess']:.2f}) "
                     f"cons {d[k + 'consistency_violations']:.0f}/{d[k + 'consistency_checked']:.0f} goalReach {d.get(k + 'consistency_violations_goal_reachable', -1):.0f}")
    print(f"  {m:22} " + " | ".join(parts))

print("\n=== TELEPORT aetherspace")
d = load(T + "results8/teleport-aetherspace.txt")
print(f"  alt build {d['alt_pass_ms']:.0f}ms")
for c in ["uniform", "samearea", "scen_mid", "scen_far"]:
    cur = row(d, c, "current")
    for mo in ["current", "chebtie", "alt"]:
        r = row(d, c, mo)
        if r:
            print(f"  {c:9} {mo:8} {fmt(r, cur)}")

print("\n=== K variants (alt vs current, same run)")
for var in ["k4", "main", "k16"]:
    print(f" {var}:")
    for m in MAPS:
        d = load(T + f"results8/{var}-{m}.txt")
        parts = []
        for c in ["uniform", "samearea"]:
            cur, a = row(d, c, "current"), row(d, c, "alt")
            parts.append(f"{c[:4]} sub {a['sub']:.0f} med {a['med']:.3f} (x{a['med'] / cur['med']:.2f}) p95 {a['p95']:.3f} (x{a['p95'] / cur['p95']:.2f}) max {a['mx']:.1f}")
        print(f"   {m:11} " + " | ".join(parts) + f" ; build {d['alt_pass_ms']:.1f}ms ; {d['alt_bytes'] / 1e6:.1f}MB ; lm {d['alt_landmarks_built']:.0f}")

print("\n=== MAPPING LOOP (edit then getPath) per-SCC | old")
for m in MAPS:
    d, o = load(T + f"results8/mapping-{m}.txt"), load(T + f"results6/mapping-{m}.txt")
    if not d:
        print(f"  {m:11} MISSING")
        continue
    print(f"  {m:11} current med {d['mapping_current_median_ms']:.1f} max {d['mapping_current_max_ms']:.1f} | alt med {d['mapping_alt_median_ms']:.1f} max {d['mapping_alt_max_ms']:.1f} "
          f"(x{d['mapping_alt_median_ms'] / d['mapping_current_median_ms']:.2f}) | old x{o['mapping_alt_median_ms'] / o['mapping_current_median_ms']:.2f}")
