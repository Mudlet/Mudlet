import os
R="/home/vadi/.claude/jobs/3ae74264/tmp/results2/"
for m in ["sendar","achaea","aetolia","imperian","lusternia","starmourn","aetherspace"]:
    f=R+m+".txt"
    if not os.path.exists(f): print(m,"MISSING"); continue
    d={}
    for l in open(f):
        p=l.split()
        if len(p)==3 and p[0]=="METRIC": d[p[1]]=p[2]
        elif "FAIL" in l: print(m, l.strip())
    print(f"\n== {m}: rooms {d.get('map_rooms')} euclid-scale {d.get('heuristic_scale')} cheb-scale {d.get('chebyshev_scale')}")
    for c in ["uniform","samearea"]:
        print(f"  {c}: pairs {d.get(c+'_pairs')}")
        for mo in ["current","zero","scaled","cheb","chebarea"]:
            k=f"{c}_{mo}_"
            g=lambda x: d.get(k+x,"-")
            print(f"    {mo:9} subopt {g('suboptimal'):>4} worst {g('worst_ratio'):>6} fail {g('failed')} touched {g('touched_total'):>10} maxT {g('touched_max'):>8} total_ms {g('total_ms'):>10} med {g('median_ms'):>8} p95 {g('p95_ms'):>8} max {g('max_ms')}")
for mode in (3,4):
    f=R+f"aether-scen-{mode}.txt"
    if os.path.exists(f):
        print(f"aether scenarios mode {mode}:", " ".join(l.split()[1]+"="+l.split()[2] for l in open(f) if l.startswith("METRIC path_") and ("_ms" in l or "_steps" in l)), [l.strip() for l in open(f) if "FAIL" in l])
