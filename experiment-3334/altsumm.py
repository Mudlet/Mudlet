import os
T = "/home/vadi/.claude/jobs/3ae74264/tmp/"
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
        elif l.startswith(("ADV", "FLOATCHECK")):
            other.append(l.strip())
    return d, other


def row(d, c, mo):
    k = f"{c}_{mo}_"
    if k + "suboptimal" not in d:
        return None
    return dict(sub=d[k + "suboptimal"], worst=d[k + "worst_ratio"], fail=d[k + "failed"], touched=d[k + "touched_total"],
                expanded=d[k + "expanded_total"], atF=d[k + "expanded_at_final_f"], med=d[k + "best_median_ms"], p95=d[k + "best_p95_ms"],
                mx=d[k + "best_max_ms"], tot=d[k + "best_total_ms"], hmed=d.get(k + "hstart_ratio_median", 0), hmax=d.get(k + "hstart_ratio_max", 0),
                over=d.get(k + "hstart_over_1", 0), tieD=d.get(k + "tie_route_differs"), tieC=d.get(k + "tie_comparable"))


print("=== MAIN (K=8, farthest-in-SCC): per map, class: mode sub/pairs | median p95 max ms | rooms touched vs current | h(start)/opt median")
for m in MAPS:
    d, _ = load(T + f"results6/main-{m}.txt")
    print(f"\n{m}: rooms {d['map_rooms']:.0f}, largest SCC {d.get('alt_largest_scc', 0):.0f}, landmarks {d['alt_landmarks_built']:.0f}, "
          f"alt build {d['alt_pass_ms']:.1f}ms of initGraph {d['init_graph_ms']:.1f}ms (experiment scale passes {d['scale_pass_ms']:.1f}ms), "
          f"alt tables {d['alt_bytes'] / 1e6:.1f}MB, peak RSS {d.get('peak_rss_kb', 0) / 1024:.0f}MB, busy {d['machine_busy_pct_before']:.0f}/{d['machine_busy_pct_after']:.0f}")
    classes = ["uniform", "samearea"] + [c for c in ["scen_adjacent", "scen_near", "scen_mid", "scen_far", "scen_corner"] if f"{c}_current_suboptimal" in d]
    for c in classes:
        cur = row(d, c, "current")
        pairs = d.get(c + "_pairs", 1)
        for mo in ["current", "zero", "chebtie", "alt", "altnotie"]:
            r = row(d, c, mo)
            if not r:
                continue
            tie = f" tieDiff {r['tieD']:.0f}/{r['tieC']:.0f}" if r["tieC"] is not None else ""
            print(f"  {c:13} {mo:9} sub {r['sub']:.0f}/{pairs:.0f} worst {r['worst']:.3f} | {r['med']:8.3f} {r['p95']:8.3f} {r['mx']:8.3f} ms"
                  f" (med x{r['med'] / cur['med']:.2f}) | touched x{r['touched'] / cur['touched']:.2f} expanded {r['expanded']:.0f} atFinalF {r['atF']:.0f}"
                  f" | h med {r['hmed']:.3f} max {r['hmax']:.3f} >1 {r['over']:.0f}{tie}")

print("\n=== AUDIT (exact d(u,goal) for all u; every edge consistency)")
for m in MAPS + ["aetherspace-teleport"]:
    d, _ = load(T + f"results6/audit-{m}.txt")
    parts = []
    for mo in ["current", "chebtie", "alt"]:
        k = f"audit_{mo}_"
        parts.append(f"{mo}: adm {d[k + 'admissible_violations']:.0f}/{d[k + 'admissible_checked']:.0f} (worst +{d[k + 'worst_excess']:.2f}) "
                     f"cons {d[k + 'consistency_violations']:.0f}/{d[k + 'consistency_checked']:.0f}")
    print(f"  {m:22} " + " | ".join(parts))

print("\n=== TELEPORT (Aetherspace + one teleport inside the big area)")
d, _ = load(T + "results6/teleport-aetherspace.txt")
print(f"  alt build {d['alt_pass_ms']:.0f}ms, chebyshev area scale {d.get('bench_area_chebyshev_scale')}")
for c in ["uniform", "samearea", "scen_mid", "scen_far"]:
    cur = row(d, c, "current")
    for mo in ["current", "chebtie", "alt"]:
        r = row(d, c, mo)
        print(f"  {c:9} {mo:8} sub {r['sub']:.0f} worst {r['worst']:.3f} | {r['med']:8.3f} {r['p95']:8.3f} {r['mx']:8.3f} ms | touched x{r['touched'] / cur['touched']:.2f}")

print("\n=== VARIANTS: ALT median ms / p95 / touched-vs-current, uniform | samearea ; build ms ; table MB")
for var in ["k2", "k4", "main", "k16", "fwdonly", "random"]:
    print(f" {var}:")
    for m in MAPS:
        d, _ = load(T + f"results6/{var}-{m}.txt")
        parts = []
        for c in ["uniform", "samearea"]:
            cur, a = row(d, c, "current"), row(d, c, "alt")
            parts.append(f"{c[:4]} sub {a['sub']:.0f} med {a['med']:.3f} (x{a['med'] / cur['med']:.2f}) p95 {a['p95']:.3f} (x{a['p95'] / cur['p95']:.2f}) touched x{a['touched'] / cur['touched']:.2f}")
        print(f"   {m:11} " + " | ".join(parts) + f" ; build {d['alt_pass_ms']:.1f}ms ; {d['alt_bytes'] / 1e6:.1f}MB ; landmarks {d['alt_landmarks_built']:.0f}")

print("\n=== MAPPING LOOP (edit then getPath; includes graph rebuild)")
for m in MAPS:
    d, _ = load(T + f"results6/mapping-{m}.txt")
    print(f"  {m:11} current median {d['mapping_current_median_ms']:.1f} max {d['mapping_current_max_ms']:.1f} | alt median {d['mapping_alt_median_ms']:.1f} max {d['mapping_alt_max_ms']:.1f} ms (x{d['mapping_alt_median_ms'] / d['mapping_current_median_ms']:.2f})")

print("\n=== ADVERSARIAL / FLOAT (float build)")
for f in ["adversarial", "floatcheck"]:
    _, o = load(T + f"results6/{f}.txt")
    for l in o:
        print("  " + l)
print("\n=== DOUBLE-COST BUILD")
for f in ["adversarial", "floatcheck"]:
    _, o = load(T + f"results7/{f}.txt")
    for l in o:
        print("  " + l)
for m in ["sendar", "achaea", "starmourn", "aetherspace"]:
    d, _ = load(T + f"results7/verify-{m}.txt")
    df, _ = load(T + f"results6/main-{m}.txt")
    parts = []
    for c in ["uniform", "samearea"]:
        for mo in ["current", "alt"]:
            r, rf = row(d, c, mo), row(df, c, mo)
            parts.append(f"{c[:4]} {mo} sub {r['sub']:.0f} med {r['med']:.3f} (float {rf['med']:.3f})")
    print(f"  {m:11} alt tables {d['alt_bytes'] / 1e6:.1f}MB (float {df['alt_bytes'] / 1e6:.1f}MB) peakRSS {d.get('peak_rss_kb', 0) / 1024:.0f}MB (float {df.get('peak_rss_kb', 0) / 1024:.0f}MB) initGraph {d['init_graph_ms']:.0f}ms (float {df['init_graph_ms']:.0f}ms) | " + " ; ".join(parts))
