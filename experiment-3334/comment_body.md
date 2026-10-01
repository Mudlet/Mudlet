@SlySven would appreciate your review on this one - you noticed back in 2020 that the route-finding doesn't always find the cheapest route, and I think I've found why and a way to fix it.

**Still reproduces on development.** The reporter's map still gives 7 steps for `getPath(2, 183)` and 4 for the way back.

**Cause.** The A* heuristic in `src/TAstar.h` is the straight-line distance between room coordinates. That's only safe if no exit costs less than the distance it covers on the map - but on real maps exits often span several grid units at cost 1 (on the reporter's map most span 2 units, some up to 14). The heuristic then overestimates, and A* settles the goal before it has looked at the cheaper route. So the early exit on the first route found isn't the problem by itself - it's correct once the heuristic never overestimates.

**How I measured.** A benchmark harness over 31 maps from 25 games: the reporter's map, the IRE crowdmaps (Achaea, Aetolia, Imperian, Lusternia, Starmourn), 23 public maps from other games (Aardwolf, Arkadia, Wheel of Time, Materia Magica, KillerMUD, Dark Mists, Procedural Realms, the 89k-room map from #8553 and more - found in mapper repos and issue attachments), and Aetherspace (2.3M rooms, 8-way grid, every exit 1 unit long). Per map, 300 random pairs and 300 same-area pairs (100 + 100 on Aetherspace), checked against the true cheapest cost from plain Dijkstra. Timings are best-of-5 with the modes in shuffled order, on an idle machine. On top of that, an exact audit: a reverse Dijkstra from 30 sample goals (3 on Aetherspace) gives every room's true distance to the goal, and checks the heuristic never goes over it.

**Today:** 2,146 of 18,300 routes are longer than the cheapest one (12%). It depends a lot on how a map was drawn - 2/600 on The Forest's Edge, 164/600 on the reporter's map, 439/600 on Wheel of Time. The worst I saw was 9.5x the cheapest (Lusternia, in an earlier run). Aetherspace is fine because every exit is 1 unit long, so the heuristic is close to exact there. The exact audit found the heuristic overestimating on every map except none - from 32 rooms (The Forest's Edge) to 2.4M (Aetherspace).

**What I tried first:**

1. Scaling the straight-line distance down until it can't overestimate - correct within an area, but 31-47x slower on the crowdmaps.
2. Chebyshev distance scaled per area - fast, but still wrong when the cheapest path leaves the area and comes back (1 wrong route in 11,400, and a small crafted map gets cost 6 instead of 3). One teleport exit inside a big area also collapses the scale and makes it ~1,000x slower.
3. Plain Dijkstra - always correct, ~1,100x slower on Aetherspace.
4. ALT (landmarks + the triangle inequality) - correct everywhere and 7-25x faster on the crowdmaps, but on Aetherspace the landmarks give weak bounds (p95 85x slower than today) and take 23s to build.

**Proposal: the higher of two lower bounds that can't overestimate.**

- **ALT per strongly connected component.** Every group of rooms that can all reach each other gets 8 landmarks, and each room stores its distance to and from its group's landmarks. For a room `u` and goal `g` in the same group, `d(u, g) >= d(L, g) - d(L, u)` and `d(u, g) >= d(u, L) - d(g, L)` for every landmark `L`.
- **Chebyshev distance x area scale, but only in sealed areas** - areas with no exits in from other areas, or none out to them. A path between two rooms of such an area can never leave it, so the bound holds. The scale is the lowest cost / Chebyshev length over the area's exits.
- **No landmarks where the Chebyshev bound is already tight.** For a group inside one sealed area, the Dijkstra that picks its first landmark also measures how much of the true distance the Chebyshev bound covers. At 90% or more, the group gets no landmarks. Aetherspace is an open grid (100%), so it builds none - Procedural Realms is a grid full of walls (60%), so its big groups still get them.
- Ties on f go to the smaller h.

The max of two bounds that never overestimate never overestimates either.

**Correctness:** 0 wrong routes in 18,300 pairs across all 31 maps, plus an Aetherspace variant with one teleport added. The crafted map gets the optimal cost 3. The exact audit found 0 overestimates everywhere except LegendMUD, where `float` costs let the landmark bound go over by up to 7 (61 out of 2.6M checks) - its exits have very large weights, and subtracting two large `float` distances loses the last few units. With `double` costs that's 0 too, so the real version should switch `cost` to `double`.

**Speed and cost per map** (median / p95 per `getPath` on random pairs, then the first `getPath` after a map edit):

<details>
<summary>All 31 maps</summary>

TABLE

</details>

In short: 1.5-25x faster on the median and 1.1-9x on p95 for every map over 5k rooms except Aetherspace, where it's 1.2-1.4x slower but still under 1ms. With a teleport added to Aetherspace, today's heuristic gets 2/100 routes wrong and the proposal's p95 goes to 8ms (same-area pairs up to 43ms).

**What it costs:**

- **Graph rebuilds get slower.** The landmarks are rebuilt with the graph, so the first `getPath` after a map edit pays for it - 1.6-2.2x on most maps, which is +10-75ms on 7-33k-room maps, +133ms on Materia Magica (65k rooms) and +450ms on the 89k-room map from #8553. The grid maps pay less: 1.3x on Procedural Realms, 1.1x on Aetherspace (35 to 39s, mostly the existing graph build). With 4 landmarks instead of 8, ALT alone cost 1.35-1.5x per edit instead of 1.6-2.1x - I haven't run the hybrid with 4 yet.
- **Memory:** 8 landmarks x 2 directions per room in a group with landmarks - 64 bytes with `float`, 128 with `double`, so 0.8-8MB on the 6-65k-room maps. The prototype still allocates the table for rooms that got no landmarks (149MB on Aetherspace) - easy to avoid, not done yet.
- **Different routes of the same cost.** Where today's route was already the cheapest, the new one often picks another route of the same cost - from 12% of those pairs (Realms of Despair) to all of them (Aetherspace, Procedural Realms), where a grid has many equally short routes. Speedwalks will visibly take different routes, even though none of them are longer.

Side note: TorilMUD's public map (46k rooms) has every room but one locked, so pathfinding skips it entirely - left it out.

The prototype and the benchmark harness are on the `BRANCH-LINK` branch.
