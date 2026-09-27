import os
T = "/home/vadi/.claude/jobs/3ae74264/tmp/"


def load(f):
    d = {}
    for l in open(f, errors="replace"):
        p = l.split()
        if len(p) == 3 and p[0] == "METRIC":
            d[p[1]] = float(p[2])
    return d


maps = ["sendar", "achaea", "aetolia", "imperian", "lusternia", "starmourn", "aetherspace"]
print("chebtie correctness across all campaigns (unmutated maps):")
tp = ts = 0
for run in ["results3", "results4", "results5"]:
    rp = rs = 0
    for m in maps:
        f = T + run + "/" + m + ".txt"
        if not os.path.exists(f):
            continue
        d = load(f)
        for c in ["uniform", "samearea"]:
            if c + "_chebtie_suboptimal" in d:
                rp += d[c + "_pairs"]
                rs += d[c + "_chebtie_suboptimal"]
    print(f"  {run}: pairs {rp:.0f} suboptimal {rs:.0f}")
    tp += rp
    ts += rs
print(f"  total: {tp:.0f} pair checks, {ts:.0f} suboptimal")
print("\nchebtie h(start)/optimum > 1 (overestimates) in results4+5:")
for run in ["results4", "results5"]:
    for m in maps:
        d = load(T + run + "/" + m + ".txt")
        for c in ["uniform", "samearea"]:
            o = d.get(c + "_chebtie_hstart_over_1", 0)
            if o:
                print(f"  {run} {m} {c}: {o:.0f} pairs, max ratio {d[c + '_chebtie_hstart_ratio_max']}, suboptimal {d[c + '_chebtie_suboptimal']:.0f}")
print("\nresults5 (gated, warm-up) timing: median/p95/max ms, and ratios vs current")
for m in maps:
    d = load(T + "results5/" + m + ".txt")
    for c in ["uniform", "samearea"]:
        cur = d[c + "_current_best_median_ms"]
        row = []
        for mo in ["current", "zero", "chebarea", "chebtie"]:
            k = c + "_" + mo + "_"
            row.append(f"{mo} {d[k + 'best_median_ms']:.3f}/{d[k + 'best_p95_ms']:.3f}/{d[k + 'best_max_ms']:.3f} (x{d[k + 'best_median_ms'] / cur:.2f}, rooms x{d[k + 'touched_total'] / d[c + '_current_touched_total']:.2f})")
        print(f"  {m:10} {c:9} busy {d['machine_busy_pct_before']:.0f}->{d['machine_busy_pct_after']:.0f}: " + " | ".join(row))
print("\ncurrent suboptimal % (results5):")
for m in maps:
    d = load(T + "results5/" + m + ".txt")
    print(f"  {m}: " + ", ".join(f"{c} {100 * d[c + '_current_suboptimal'] / d[c + '_pairs']:.1f}% worst {d[c + '_current_worst_ratio']}" for c in ["uniform", "samearea"]))
print("\nplateau (results5): expanded, at final f")
for m in ["aetherspace"]:
    d = load(T + "results5/" + m + ".txt")
    for c in ["uniform", "samearea"]:
        for mo in ["chebarea", "chebtie"]:
            k = c + "_" + mo + "_"
            print(f"  {m} {c} {mo}: expanded {d[k + 'expanded_total']:.0f} atFinalF {d[k + 'expanded_at_final_f']:.0f}")
