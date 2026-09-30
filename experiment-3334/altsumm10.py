import os
T = "/home/vadi/.claude/jobs/3ae74264/tmp/results10/"
MAPS = ["sendar", "achaea", "aetolia", "imperian", "lusternia", "starmourn", "aetherspace"]


def load(f):
    d, other = {}, []
    if not os.path.exists(f):
        return None, []
    for l in open(f, errors="replace"):
        p = l.split()
        if len(p) == 3 and p[0] == "METRIC":
            try:
                d[p[1]] = float(p[2])
            except ValueError:
                pass
        elif l.startswith("ADV"):
            other.append(l.strip())
    return d, other


def row(d, c, mo):
    k = f"{c}_{mo}_"
    if not d or k + "suboptimal" not in d:
        return None
    return dict(sub=d[k + "suboptimal"], worst=d[k + "worst_ratio"], fail=d[k + "failed"], touched=d[k + "touched_total"],
                med=d[k + "best_median_ms"], p95=d[k + "best_p95_ms"], mx=d[k + "best_max_ms"])


_, adv = load(T + "adversarial.txt")
print("=== ADVERSARIAL")
for l in adv:
    print("  " + l)

for kind in ["main", "teleport"]:
    print(f"\n=== {kind.upper()}")
    for m in (MAPS if kind == "main" else ["aetherspace"]):
        d, _ = load(T + f"{kind}-{m}.txt")
        if not d:
            print(f"{m}: MISSING")
            continue
        print(f"\n{m}: rooms {d['map_rooms']:.0f}; areas {d.get('geo_areas', 0):.0f} sealed {d.get('geo_sealed_areas', 0):.0f}; SCCs {d.get('alt_components', 0):.0f} "
              f"with landmarks {d.get('alt_components_with_landmarks', 0):.0f} skipped {d.get('geo_scc_skipped', 0):.0f} ({d.get('geo_rooms_skipped', 0):.0f} rooms); "
              f"alt build {d['alt_pass_ms']:.1f}ms geo {d.get('geo_pass_ms', 0):.1f}ms; initGraph {d['init_graph_ms']:.1f}ms; peak RSS {d.get('peak_rss_kb', 0) / 1024:.0f}MB; "
              f"busy {d['machine_busy_pct_before']:.0f}/{d['machine_busy_pct_after']:.0f}")
        classes = ["uniform", "samearea"] + [c for c in ["scen_near", "scen_mid", "scen_far", "scen_corner"] if f"{c}_current_suboptimal" in d]
        for c in classes:
            cur = row(d, c, "current")
            for mo in ["current", "alt", "altgeo"]:
                r = row(d, c, mo)
                if r:
                    print(f"  {c:9} {mo:7} sub {r['sub']:.0f}/{d.get(c + '_pairs', 0):.0f} worst {r['worst']:.3f} fail {r['fail']:.0f} | med {r['med']:.3f} p95 {r['p95']:.3f} max {r['mx']:.3f} ms"
                          f" (med x{r['med'] / cur['med']:.2f} p95 x{r['p95'] / cur['p95']:.2f}) touched x{r['touched'] / cur['touched']:.3f}")

print("\n=== AUDIT")
for m in MAPS + ["aetherspace-teleport"]:
    d, _ = load(T + f"audit-{m}.txt")
    if not d:
        print(f"  {m}: MISSING")
        continue
    parts = []
    for mo in ["current", "alt", "altgeo"]:
        k = f"audit_{mo}_"
        if k + "admissible_checked" in d:
            parts.append(f"{mo}: adm {d[k + 'admissible_violations']:.0f}/{d[k + 'admissible_checked']:.0f} (+{d[k + 'worst_excess']:.2f}) "
                         f"cons {d[k + 'consistency_violations']:.0f}/{d[k + 'consistency_checked']:.0f} goalReach {d.get(k + 'consistency_violations_goal_reachable', -1):.0f}")
    print(f"  {m:22} " + " | ".join(parts))

print("\n=== MAPPING LOOP (mode 8, K=8, geo skip 0.5)")
for m in MAPS:
    d, _ = load(T + f"mapping-{m}.txt")
    if not d:
        print(f"  {m:11} MISSING")
        continue
    print(f"  {m:11} current med {d['mapping_current_median_ms']:.1f} | altgeo med {d['mapping_alt_median_ms']:.1f} max {d['mapping_alt_max_ms']:.1f} "
          f"(x{d['mapping_alt_median_ms'] / d['mapping_current_median_ms']:.2f}) pass {d['mapping_alt_pass_median_ms']:.1f}ms")
