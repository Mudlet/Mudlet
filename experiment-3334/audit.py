import os
def load(f):
    d={}
    for l in open(f):
        p=l.split()
        if len(p)==3 and p[0]=="METRIC": d[p[1]]=float(p[2])
    return d
R="/home/vadi/.claude/jobs/3ae74264/tmp/"
ire=["achaea","aetolia","imperian","lusternia","starmourn"]
print("current suboptimal % per run (uniform, samearea):")
for run in ["results","results2","results3"]:
    row=[]
    for m in ire+["sendar"]:
        f=R+run+"/"+m+".txt"
        if not os.path.exists(f): continue
        d=load(f)
        row.append(f"{m} {100*d['uniform_current_suboptimal']/d['uniform_pairs']:.1f}/{100*d['samearea_current_suboptimal']/d['samearea_pairs']:.1f}")
    print(" ",run,"; ".join(row))
print("current worst ratio max:", max(load(R+r+"/"+m+".txt")[f"{c}_current_worst_ratio"] for r in ["results","results2","results3"] for m in ire+["sendar"] if os.path.exists(R+r+"/"+m+".txt") for c in ["uniform","samearea"]))
print("\nchebtie vs current, results3, touched ratio and median ratio:")
tot=0
for m in ire+["sendar","aetherspace"]:
    d=load(R+"results3/"+m+".txt")
    for c in ["uniform","samearea"]:
        tot+=d[c+"_pairs"]
        print(f"  {m:11} {c:9} touched x{d[c+'_chebtie_touched_total']/d[c+'_current_touched_total']:.2f}  median x{d[c+'_chebtie_median_ms']/d[c+'_current_median_ms']:.2f}  chebtie subopt {int(d[c+'_chebtie_suboptimal'])}")
print("  total pairs checked for chebtie:", int(tot))
print("\nAetherspace slowdown vs current (median ratio, touched ratio):")
for run,modes in [("results/aetherspace2.txt",["zero","scaled"]),("results2/aetherspace.txt",["zero","scaled","cheb","chebarea"]),("results3/aetherspace.txt",["zero","chebarea","chebtie"])]:
    d=load(R+run)
    for mo in modes:
        for c in ["uniform","samearea"]:
            print(f"  {run:26} {mo:9} {c:9} median x{d[c+'_'+mo+'_median_ms']/d[c+'_current_median_ms']:.0f}  touched x{d[c+'_'+mo+'_touched_total']/d[c+'_current_touched_total']:.0f}")
