import os
R = "/home/vadi/.claude/jobs/3ae74264/tmp/results5/"


def load(f):
    d = {}
    other = []
    for l in open(f, errors="replace"):
        p = l.split()
        if len(p) == 3 and p[0] == "METRIC":
            d[p[1]] = p[2]
        elif l.strip():
            other.append(l.strip())
    return d, other


for m in ["sendar", "achaea", "aetolia", "imperian", "lusternia", "starmourn", "aetherspace", "aetherspace-teleport"]:
    f = R + m + ".txt"
    if not os.path.exists(f):
        print(m, "MISSING")
        continue
    d, other = load(f)
    print(f"\n== {m}: rooms {d.get('map_rooms')} busy {d.get('machine_busy_pct_before')}->{d.get('machine_busy_pct_after')} load {d.get('loadavg_before')}->{d.get('loadavg_after')}")
    print(f"   init_graph_ms {d.get('init_graph_ms')} scale_pass_ms {d.get('scale_pass_ms')} cheb_global {d.get('chebyshev_scale_global')} bench_area_scale {d.get('bench_area_chebyshev_scale')}")
    print(f"   edges {d.get('edges_total')} special {d.get('edges_special')} cross_area {d.get('edges_cross_area')} diagonal {d.get('edges_diagonal')} cost>1 {d.get('edges_cost_above_1')}")
    print("   " + " ".join(k + "=" + v for k, v in d.items() if k.startswith("ns_ew_spacing")))
    for o in other:
        if "Totals" in o or "FAIL" in o:
            print("   ", o)
    classes = ["uniform", "samearea"] + [c for c in ["scen_adjacent", "scen_near", "scen_mid", "scen_far", "scen_corner"] if any(k.startswith(c + "_") for k in d)]
    for c in classes:
        extra = f" pairs {d.get(c + '_pairs')} distinct {d.get(c + '_pairs_distinct')} unreachable {d.get(c + '_unreachable_skipped')}" if not c.startswith("scen") else f" optimum {d.get(c + '_optimum')}"
        print(f"  {c}:{extra}")
        for mo in ["current", "zero", "chebarea", "chebtie"]:
            k = f"{c}_{mo}_"
            if k + "suboptimal" not in d:
                continue
            g = lambda x: d.get(k + x, "-")
            print(f"    {mo:9} subopt {g('suboptimal'):>3} worst {g('worst_ratio'):>6} fail {g('failed')} touched {g('touched_total'):>10} expanded {g('expanded_total'):>10} atFinalF {g('expanded_at_final_f'):>9} med {g('best_median_ms'):>8} p95 {g('best_p95_ms'):>8} max {g('best_max_ms'):>8} total {g('best_total_ms'):>10} hRatio med {g('hstart_ratio_median'):>6} max {g('hstart_ratio_max'):>7} >1: {g('hstart_over_1'):>4} tieDiff {g('tie_route_differs')}/{g('tie_comparable')}")
