T = "/home/vadi/.claude/jobs/3ae74264/tmp/"
s = open(T + "comment_body.md").read()
fixes = [
    ("over 31 maps from 25 games", "over 30 maps from 27 games"),
    ("The exact audit found the heuristic overestimating on every map except none - from 32 rooms",
     "The exact audit found the heuristic overestimating on every map, from 32 rooms"),
    ("0 wrong routes in 18,300 pairs across all 31 maps, plus an Aetherspace variant with one teleport added.",
     "0 wrong routes in 18,300 pairs across all 30 maps and an Aetherspace variant with one teleport added."),
    ("<summary>All 31 maps</summary>", "<summary>All 30 maps</summary>"),
    ("which is +10-75ms on 7-33k-room maps", "which is +10-77ms on 6-33k-room maps"),
    ("so 0.8-8MB on the 6-65k-room maps", "so 0.4-8MB on the 6-65k-room maps"),
    ("to all of them (Aetherspace, Procedural Realms)", "to nearly all of them (Aetherspace, Procedural Realms)"),
]
for a, b in fixes:
    assert s.count(a) == 1, a
    s = s.replace(a, b)
s = s.replace("TABLE", open(T + "comment_table.md").read().rstrip("\n"))
open("/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/experiment-3334/comment-3334.md", "w").write(s)
print(len(s.split()))
