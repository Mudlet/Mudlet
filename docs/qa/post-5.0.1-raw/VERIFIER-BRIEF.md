# Verifier brief (independent replication of a QA batch)

You verify the findings of other agents. You have only their reports; they have
finished and cannot be asked. Your standard: nothing is confirmed until you have
reproduced it yourself, from the report's steps, on a fresh Mudlet instance.

Read first: <scratchpad>/qa/AGENT-BRIEF.md (environment, launch recipe, rules -
all of them apply to you, including the display number you are given and the
ban on building or editing in /home/user/Mudlet).

## For each finding (F-*) in the reports you are given
1. Run its repro exactly as written (its script file if it has one). Record what you observed.
2. Verdict: CONFIRMED (reproduced as described), PARTIAL (reproduced, but the
   severity, cause or description is wrong - say how), NOT REPRODUCED (steps
   followed, behaviour absent - include your own log/screenshot), or CANNOT RUN
   (say what was missing). A finding with no replayable steps is NOT REPRODUCED
   unless you can reconstruct it from the description in under 5 minutes.
3. For CONFIRMED and PARTIAL: is the commit blamed actually the one that
   introduced it? Check with `git show <hash>` and, when cheap, whether 5.0.1's
   code (`git show Mudlet-5.0.1:<path>`) had the behaviour, so Regression vs New
   bug is right.

## For the Coverage table of each report
Sample every row marked "Fixed & verified" and check its cited evidence exists
(screenshot file present and shows what is claimed; log line present). Re-run at
least one third of them, prioritising the crash/hang/data-loss commits and any
row whose evidence is thin. A row you re-run that does not hold up becomes a new
finding of yours (F-V-<n>) with full evidence.

## Cross-check against GitHub before you record a finding
- grep <scratchpad>/qa/recent-issues.md (issues touched since the 5.0.1 branch point) and <scratchpad>/qa/open-prs.md (open PRs).
- Then run one semantic search per finding with the GitHub tool: load it with ToolSearch (`select:mcp__github__search_issues`) and call it with owner Mudlet, repo Mudlet, a plain-words query. Also search pull requests the same way (`select:mcp__github__search_pull_requests`).
- A match does not cancel the finding - you are confirming it is live on this tree - but tag its Status with `Known: #N` (open issue), `Closed: #N` (an issue that claims it is fixed - that makes yours a Regression or Fix incomplete) or `Fix pending: PR #N`.

## Report
Write <scratchpad>/qa/verify-<BATCH>.md:

```
# Verification of batch <N>: <areas>

## Verdicts
| Finding | Reported severity | Verdict | Your severity | Notes |

## Coverage audit
| Report | Rows | Evidenced | Re-run | Held up | Did not hold up (ids) |

## New findings
F-V-<n> ... (same format as the agent findings, with replayable steps)

## Details
One subsection per finding with what you ran and what you saw (log excerpts, screenshot paths).
```

Your final message to the coordinator: the report path, and one line per finding with its verdict. Nothing else.
