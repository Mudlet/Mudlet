# Package-repository capture audits

Empirical basis for the starter UI ("Mudlet base UI") capture design: every package in
[mudlet-package-repository](https://github.com/Mudlet/mudlet-package-repository) (216 packages
at commit d2dead2, audited 2026-07-16) was analyzed for how it captures game text and
character data, with file-level provenance for every mechanism.

- `chat-capture-catalog.md` - chat/text capture: mechanism taxonomy, routing patterns,
  per-channel regex library, GMCP dialects, and the layered capture design the starter UI
  implements (79 packages, 281 mechanisms).
- `chat-capture-findings.json` - the raw chat corpus behind the catalog.
- `vitals-capture-catalog.md` - character data (hp/mana/xp/affects/enemy-hp): GMCP dialect
  map, MSDP variables, prompt-shape families, and the maxima problem (63 packages,
  451 mechanisms).
- `vitals-capture-findings.json` - the raw vitals corpus behind the catalog.

Method: per-package analysis with cross-checking against keyword-based recall lists, then a
second pass over any package flagged as capture-likely but reported empty. Counts of
cross-game convergence are by independent game, not package, to compensate for the corpus's
IRE/Achaea skew.
