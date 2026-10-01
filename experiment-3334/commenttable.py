import glob
import os

R = "/home/vadi/.claude/jobs/3ae74264/tmp/results13/"


def load(f):
    d = {}
    for l in open(f, errors="replace"):
        p = l.split()
        if len(p) == 3 and p[0] == "METRIC":
            try:
                d[p[1]] = float(p[2])
            except ValueError:
                pass
    return d


games = {"sendar": "reporter's map (Sendar)", "achaea": "Achaea", "aetolia": "Aetolia", "imperian": "Imperian", "lusternia": "Lusternia",
         "starmourn": "Starmourn", "aetherspace": "Aetherspace", "aardwolf": "Aardwolf (starter)", "aardwolf_2": "Aardwolf",
         "arkadia": "Arkadia", "arkadia_2": "Arkadia (other snapshot)", "barsawia": "Barsawia", "blackmud": "BlackMUD",
         "darkcastle": "Dark Castle", "darkmists": "Dark Mists", "genesis": "Genesis", "killermud": "KillerMUD",
         "killermud_2": "KillerMUD (other map)", "legendmud": "LegendMUD", "luminarimud": "LuminariMUD",
         "materiamagica": "Materia Magica", "medievia": "Medievia", "proceduralrealms": "Procedural Realms",
         "realmsofdespair": "Realms of Despair", "realmsofthedragon": "Realms of the Dragon", "ronin": "Ronin",
         "tfe": "The Forest's Edge", "valena_issue8553": "map from #8553", "warlock": "Warlock", "wotmud": "Wheel of Time",
         "wotmud_2": "Wheel of Time (zoned)"}


def fm(x):
    return f"{x:.2f}" if x >= 0.1 else f"{x:.3f}"


def ed(x):
    if x >= 1000:
        return f"{x / 1000:.1f}s"
    return f"{x:.0f}ms" if x >= 10 else f"{x:.1f}ms"


rows = []
for f in glob.glob(R + "main-*.txt"):
    m = os.path.basename(f)[5:-4]
    d = load(f)
    mp = load(R + f"mapping-{m}.txt")
    pairs = d["uniform_pairs"] + d["samearea_pairs"]
    w0 = d["uniform_current_suboptimal"] + d["samearea_current_suboptimal"]
    worst = max(d["uniform_current_worst_ratio"], d["samearea_current_worst_ratio"])
    rows.append((d["map_rooms"],
                 f"| {games[m]} | {d['map_rooms']:,.0f} | {w0:.0f}/{pairs:.0f} | {worst:.2f}x | "
                 f"{fm(d['uniform_current_best_median_ms'])} / {fm(d['uniform_current_best_p95_ms'])}ms | "
                 f"{fm(d['uniform_altgeo_best_median_ms'])} / {fm(d['uniform_altgeo_best_p95_ms'])}ms | "
                 f"{ed(mp['mapping_current_median_ms'])} → {ed(mp['mapping_alt_median_ms'])} |"))
print("| map | rooms | wrong routes today | worst today | today, median / p95 | proposal, median / p95 | edit + `getPath`, today → proposal |")
print("|---|---|---|---|---|---|---|")
for _, r in sorted(rows):
    print(r)
t = load(R + "teleport-aetherspace.txt")
print(f"| Aetherspace + one teleport | {t['map_rooms']:,.0f} | {t['uniform_current_suboptimal'] + t['samearea_current_suboptimal']:.0f}/"
      f"{t['uniform_pairs'] + t['samearea_pairs']:.0f} | {max(t['uniform_current_worst_ratio'], t['samearea_current_worst_ratio']):.2f}x | "
      f"{fm(t['uniform_current_best_median_ms'])} / {fm(t['uniform_current_best_p95_ms'])}ms | "
      f"{fm(t['uniform_altgeo_best_median_ms'])} / {fm(t['uniform_altgeo_best_p95_ms'])}ms | - |")
