@SlySven would appreciate your review on this one - you noticed back in 2020 that the route-finding doesn't always find the cheapest route, and I think I've found why and a way to fix it.

**Still reproduces on development.** The reporter's map still gives 7 steps for `getPath(2, 183)` and 4 for the way back.

**Cause.** The A* heuristic in `src/TAstar.h` is the straight-line distance between room coordinates. That's only safe if no exit costs less than the distance it covers on the map - but on real maps exits often span several grid units at cost 1 (on the reporter's map most span 2 units, some up to 14). The heuristic then overestimates, and A* settles the goal before it has looked at the cheaper route. So the early exit on the first route found isn't the problem by itself - it's correct once the heuristic never overestimates.

**How I measured.** A benchmark harness over 7 maps: the reporter's map (174 rooms), the Achaea, Aetolia, Imperian, Lusternia and Starmourn crowdmaps (6k-31k rooms), and Aetherspace (2.3M rooms, 8-way grid, every exit 1 unit long). Per map, 300 random pairs and 300 same-area pairs (100 + 100 on Aetherspace), checked against the true cheapest cost from plain Dijkstra. Timings are best-of-5 with the modes in shuffled order, on an idle machine. On top of that, an exact audit: a reverse Dijkstra from 30 sample goals (3 on Aetherspace) gives every room's true distance to the goal, and checks the heuristic never goes over it.

**Today:**

| map | wrong routes, random pairs | wrong routes, same area | worst |
|---|---|---|---|
| reporter's (sendar) | 84/300 | 84/300 | 2.43x |
| Achaea | 30/300 | 15/300 | 1.40x |
| Aetolia | 12/300 | 18/300 | 2.33x |
| Imperian | 16/300 | 24/300 | 1.78x |
| Lusternia | 21/300 | 26/300 | 2.40x |
| Starmourn | 13/300 | 15/300 | 1.44x |
| Aetherspace | 0/100 | 0/100 | - |

Across all my runs the crowdmaps sat at 3-13%, and the worst route I saw was 9.5x the cheapest (Lusternia). Aetherspace is fine because every exit is 1 unit long, so the heuristic is close to exact there. The exact audit found the heuristic overestimating on every map, from 1,110 rooms (Imperian) to 2.9M (Aetherspace).

**What I tried first:**

1. Scaling the straight-line distance down until it can't overestimate - correct within an area, but 31-47x slower on the crowdmaps.
2. Chebyshev distance scaled per area - fast, but still wrong when the cheapest path leaves the area and comes back (1 wrong route in 11,400, and a small crafted map gets cost 6 instead of 3). One teleport exit inside a big area also collapses the scale and makes it ~1,000x slower.
3. Plain Dijkstra - always correct, ~1,100x slower on Aetherspace.
4. ALT (landmarks + the triangle inequality) - correct everywhere and 7-25x faster on the crowdmaps, but on Aetherspace the landmarks give weak bounds (p95 85x slower than today) and take 23s to build.

**Proposal: the higher of two lower bounds that can't overestimate.**

- **ALT per strongly connected component.** Every group of rooms that can all reach each other gets 8 landmarks, and each room stores its distance to and from its group's landmarks. For a room `u` and goal `g` in the same group, `d(u, g) >= d(L, g) - d(L, u)` and `d(u, g) >= d(u, L) - d(g, L)` for every landmark `L`.
- **Chebyshev distance x area scale, but only in sealed areas** - areas with no exits in from other areas, or none out to them. A path between two rooms of such an area can never leave it, so the bound holds. The scale is the lowest cost / Chebyshev length over the area's exits.
- **No landmarks where the Chebyshev bound is already tight** - a group entirely inside one sealed area whose scale is at least half its cheapest exit. That's what keeps Aetherspace fast: all 5 of its areas are sealed, so it builds no landmarks at all.
- Ties on f go to the smaller h.

The max of two bounds that never overestimate never overestimates either.

**Correctness:** 0 wrong routes in 3,900 pairs across the 7 maps plus an Aetherspace variant with one teleport added. The exact audit found 0 overestimates on every map. The crafted map gets the optimal cost 3.

**Speed** (median / p95 per `getPath`, random pairs):

| map | today | proposal |
|---|---|---|
| Achaea | 1.73 / 4.05ms | 0.13 / 1.06ms |
| Aetolia | 1.34 / 2.58ms | 0.08 / 0.44ms |
| Imperian | 1.23 / 2.37ms | 0.06 / 0.38ms |
| Lusternia | 1.22 / 2.21ms | 0.05 / 0.47ms |
| Starmourn | 0.24 / 0.65ms | 0.03 / 0.15ms |
| Aetherspace | 0.27 / 0.62ms | 0.37 / 0.89ms |
| Aetherspace + teleport | 0.21 / 0.71ms (2/50 same-area routes wrong) | 0.31 / 17.3ms (0 wrong) |

**What it costs:**

- **Graph rebuilds get slower.** The landmarks are rebuilt with the graph, so the first `getPath` after a map edit pays for it. An edit + `getPath` goes from 91 to 149ms on Achaea, 49 to 90ms on Imperian, 9.5 to 21.6ms on Starmourn, and 31 to 34s on Aetherspace. With 4 landmarks instead of 8, ALT alone cost 1.35-1.5x per edit instead of 1.6-2.1x - I haven't run the hybrid with 4 yet.
- **Memory:** 8 landmarks x 2 directions x 4 bytes = 64 bytes per room, 0.4-2.0MB on the crowdmaps. The prototype still allocates the table for rooms that got no landmarks (149MB on Aetherspace) - easy to avoid, not done yet.
- **Different routes of the same cost.** Where today's route was already the cheapest, the new one often picks another route of the same cost: 35/216 on the reporter's map, 45-211 out of ~280 on the crowdmaps, and 99/100 on Aetherspace, where a grid has many equally short routes. Speedwalks on grid maps will visibly take different routes, even though none of them are longer.

**Questions for you:**

1. Is +40-60ms on the first `getPath` after an edit acceptable on 20-30k-room maps, or should the landmark build be deferred or made incremental?
2. Does the sealed-area argument hold up? It's computed on the graph `initGraph()` builds, so locked rooms and locked exits are already out of it - is there anything else that could let a path leave an area and come back that I've missed?
3. Are you OK with the equal-cost route changes?

Side note: with `float` costs, a weight near `INT_MAX` can't be told apart from its neighbours, so extreme weights change routing even under plain Dijkstra. Switching to `double` fixes it, but that's #10673's territory rather than this one.

The prototype and the benchmark harness are on the `BRANCH-LINK` branch.
