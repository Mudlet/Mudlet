# QA agent brief (read fully before doing anything)

You are one QA agent in a campaign testing Mudlet's `development` branch against
the 5.0.1 release. The coordinator gives you ONE area. Your job: verify every
commit in that area behaves as its commit message claims, and hunt for
regressions a real user would hit around it. You do not fix anything.

Repository: /home/user/Mudlet (branch claude/qa-plan-5-0-1-i6fj5x, tree identical
to development at dbbf040c3). Plan: /home/user/Mudlet/docs/qa/post-5.0.1-qa-plan.md
(read the "Environment" section and your area's section).
Scratchpad root for all your output: /tmp/claude-0/-home-user-Mudlet/3123fe1e-ea5a-5043-97a2-95f34f40ac2d/scratchpad/qa
Baseline automated results: <scratchpad>/qa/baseline.md (pre-existing failures are listed there; do not re-report them, but do say if one is in your area).

## Hard rules
- Never edit, create or delete files inside /home/user/Mudlet. Never run cmake, ninja or make THERE, and never git checkout/stash/reset there: three agents share that tree and its binaries. `git log`/`git show`/`git diff` are fine.
- If you genuinely need a build (to bisect a regression against 5.0.1, or to check a suspected fix), use your own worktree; ccache is shared and warm, so an app-only build of the current tree is mostly link time:
  `git -C /home/user/Mudlet worktree add --detach /home/user/worktrees/<AREA> <ref>`
  `cd /home/user/worktrees/<AREA> && git submodule update --init --recursive`
  `cmake --preset linux-debug-nosan -DUSE_ALTERNATE_LINKER=mold && nice cmake --build --preset linux-debug-nosan -j 2 --target mudlet`
  Build ONLY the `mudlet` target (the tests are 13 GB), use `-j 2` at most, and `rm -rf` the worktree's build directory when you are done - the disk has about 10 GB free. A 5.0.1 build is a cold compile (~20 min at -j 2); do it only if a finding hinges on it, and say so in the report.
- Work only in your own scratch directory: <scratchpad>/qa/work-<AREA>/ (create it). Screenshots go to <scratchpad>/qa/shots-<AREA>/.
- Use ONLY the X display number assigned to you. Start it yourself:
  `Xvfb :<N> -screen 0 1280x800x24 >/dev/null 2>&1 & sleep 2; DISPLAY=:<N> openbox >/dev/null 2>&1 & sleep 1`
- Every Mudlet you launch gets a throwaway HOME (`HOME=$(mktemp -d)`) and these env vars:
  `DISPLAY=:<N> QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 DBUS_SESSION_BUS_ADDRESS=disabled:`
  Launch into a profile with Lua ready:
  `./build-linux-debug-nosan/src/mudlet --profile "Mudlet self-test" --offline --mirror > <work>/mudlet.log 2>&1 &`
  `--mirror` puts every console's text on stdout, so read results from the log, not from pixels.
  Give it ~10 s to come up. Verified facts about this launch (coordinator checked it):
  - The window opens at about 960x620 in the top-left area of the 1280x800 screen; the input line is the
    white box at the bottom, at roughly (440, 704). CLICK IT before typing, or the keystrokes go elsewhere:
    `xdotool mousemove 440 704 click 1; xdotool type --delay 15 'lua print(1+1)'; xdotool key Return`
  - On a fresh HOME an "interface tour" popup appears over the console shortly after start (it showed
    "4 of 6" with no interaction - E2 investigates that). Dismiss it by clicking "Skip tour" (about (360, 277))
    or pressing Escape, and screenshot to confirm it is gone before you type anything.
  - `--mirror` writes console lines as `main| <text>`; `print(a, b)` puts each argument on its own line.
  Long scripts: write them to a file and type `lua dofile("/abs/path.lua")`.
- Screenshot after every interaction: `import -window root <shots>/NN-what.png`, then READ the PNG (the Read tool shows images) and take coordinates from what you see. Never guess coordinates.
- ctest: run from /home/user/Mudlet as `QT_QPA_PLATFORM=offscreen ctest --preset linux-debug-nosan -R '<regex>' --output-on-failure 2>&1 | tail -60`. Other agents run ctest at the same time; that is fine.
- Lua specs: `.claude/scripts/run-lua-tests.sh` runs everything (~6 min) and is safe concurrently. For a subset, make a directory holding symlinks to the spec files you want and run `TESTS_DIRECTORY=<thatdir> .claude/scripts/run-lua-tests.sh`. Read the busted summary at the end of its output.
- Some Lua test helpers only exist when `MUDLET_TEST_MODE=1` is set in the environment; set it on your manual launches too if a helper such as `feedTelnet` is missing.
- The machine has 4 cores shared with two other agents. Do not run more than one Mudlet plus one ctest at a time.
- PROCESS HYGIENE (two batch-1 agents killed each other's Mudlet and X server; do not repeat it): every process you start in the background gets its PID recorded at once - `cmd ... & echo $! >> <work>/pids.txt` - and when you clean up you kill ONLY the PIDs in that file (`kill $(cat <work>/pids.txt) 2>/dev/null`). Never kill by name or pattern: no `pkill`, no `killall`, no `kill $(pgrep ...)`. Other agents' Mudlets, Xvfbs, openboxes and python servers look identical to yours. If a Mudlet you started refuses to die, `kill -9` that PID only.
- Time-box yourself: aim to finish in about 45 minutes of wall clock. Breadth over depth on the long tail; depth on anything that crashes, hangs, or loses data.

## Method, per commit in scope
1. `git log -1 --format=%B <hash>` - read the whole message; most have a "Test case:" section. `git show --stat <hash>` for the files.
2. Run that test case by hand in the real UI or via Lua. Confirm the fix is present.
3. Do the two or three adjacent things a user would do next (undo it, do it twice, do it with the editor open, do it on a second profile, resize the window, etc.).
4. Run the ctest classes and spec files the plan names for your area. Compare with baseline.md.
5. Anything you cannot run here (needs Windows/macOS, a microphone, a real game) is still recorded, with the reason.

## Evidence standard
Every claim you make will be re-run by a separate verifier agent who has only your report. So:
- A finding needs a REPLAYABLE repro: a script file under your work dir (Lua and/or shell, with the exact xdotool commands and the launch line) plus the expected and actual outcome, and either a screenshot path or a quoted log excerpt. "I saw X" without a script is not a finding.
- A crash needs the last 30 lines of mudlet.log and, if present, the Qt fatal/backtrace (run the binary under `gdb -batch -ex run -ex bt --args ...` if it reproduces).
- A "Fixed & verified" coverage row must cite its evidence: the screenshot file name or the log line that shows the fixed behaviour. A row without evidence must say "Claimed, not evidenced" instead - that is acceptable and honest; an evidenced row that does not hold up is not.
- Say what you actually observed, not what the commit message says should happen. If the two differ, that is a finding.
- If a behaviour is arguable, record it as a Note rather than a bug.

## Cross-check against GitHub before you record a finding
- grep <scratchpad>/qa/recent-issues.md (issues touched since the 5.0.1 branch point) and <scratchpad>/qa/open-prs.md (open PRs).
- Then run one semantic search per finding with the GitHub tool: load it with ToolSearch (`select:mcp__github__search_issues`) and call it with owner Mudlet, repo Mudlet, a plain-words query. Also search pull requests the same way (`select:mcp__github__search_pull_requests`).
- A match does not cancel the finding - you are confirming it is live on this tree - but tag its Status with `Known: #N` (open issue), `Closed: #N` (an issue that claims it is fixed - that makes yours a Regression or Fix incomplete) or `Fix pending: PR #N`.

## Report
Write <scratchpad>/qa/findings-<AREA>.md in exactly this shape:

```
# <AREA> findings

## Summary
3-6 lines: what you covered, what broke, what you could not run.

## Findings
| ID | Severity | Commit | Title | Status |
(Severity: Blocker / Major / Minor / Cosmetic. Status: Regression / New bug / Fix incomplete / Note)

### F-<AREA>-1: <title>
Steps / Expected / Actual / Evidence / Commits

## Coverage
| Commit | Subject | Verdict | How verified |
(Verdict: Fixed & verified / Verified no regression / Could not test (reason) / Bug: F-<AREA>-n)
Every commit in your scope appears here exactly once.

## Automated
ctest: x passed / y failed (names of failures). Specs: successes / failures / errors (names).
```

Your final message to the coordinator is only: the path of the report, the count of findings by severity, and one line per Blocker/Major. Nothing else.
