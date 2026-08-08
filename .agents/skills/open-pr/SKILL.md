---
name: open-pr
description: >-
  Publish the current branch and open a pull request against the upstream Mudlet repository,
  using the project's PR template and the title prefix that Danger enforces. Use when asked to
  open a PR, create a pull request, or push changes upstream.
license: GPL-2.0-or-later
argument-hint: Optional hint about what this PR does (e.g. "adds shimmer blink effect")
user-invocable: true
disable-model-invocation: true
---

## When to use

Use when the user asks to open a pull request, create a PR, or push their changes upstream. This
publishes the current branch to their fork if needed, then opens a PR against upstream
`development`.

## Procedure

1. **Gather context.** Run `git log upstream/development..HEAD --oneline` for the commit messages,
   `git diff upstream/development...HEAD` to see what actually changed, and
   `git branch --show-current` for the branch name.

2. **Choose the title prefix.** `docs/CONTRIBUTING.md` states the rule and Danger enforces it on
   every PR. Choosing between the four is the part that is not written down elsewhere:

   | Prefix | Use for |
   | --- | --- |
   | `Fix: ` | bug fixes visible to users |
   | `Improve: ` | enhancements to existing user-facing functionality |
   | `Add: ` | new user-facing features or capabilities |
   | `Infra: ` | build system, CI, tooling, refactoring, and other non-player-visible changes |

   Distinguishing the last two: user-visible behavior change is `Improve`, everything internal is
   `Infra`, however large the diff.

3. **Write the title.** Use the exact form `Prefix: Summary` — capitalized prefix, colon, single
   space, then a summary whose first word is also capitalized. Keep the summary short and
   understandable to a non-technical reader; it becomes a line in the PTB changelog, which is why
   the casing is worth being consistent about even though Danger's check is case-insensitive and
   does not require the colon. Danger warns on overly long titles, so keep it brief.

   Write in American English, matching the rest of the project's user-facing text — "color" not
   "colour", "standardize" not "standardise".

   ```
   Fix: Profiles named "." or ".." no longer delete every profile when removed
   Improve: OSC 8 hyperlink handling, and a setting to turn it off
   Add: Text-to-speech support for incoming game text
   Infra: Tidy up how CI installs Lua
   ```

   Dependabot raises its own PRs as `Infrastructure: Bump ...`; that is generated upstream and is
   not something to correct by hand.

4. **Draft the body.** Read `.github/PULL_REQUEST_TEMPLATE.md` and fill in its headings — read it
   rather than reproducing it here, so this skill cannot drift from the real template. Note that
   passing `--body` to `gh` bypasses the template file, which is why it has to be read and filled
   in explicitly.

   House style on top of the template: keep each section terse, 1-3 bullet points for the overview
   and a single sentence of motivation. Add a `**Test case:**` line at the end giving brief steps
   to verify the change — not part of the template, but reviewers expect it. No fluff.

5. **Show the draft title and body to the user, and ask whether to open it as a draft PR or as
   ready for review.** Wait for both answers before anything is published — never assume either.
   Draft suits work that is still moving, wants early CI feedback, or needs discussion before
   reviewers spend time on it; ready for review suits a change that is complete and tested.

6. **Publish the branch** to the user's fork if it has no upstream yet:
   `git push -u origin <branch-name>`.

7. **Open the PR** against upstream, adding `--draft` if that is what the user chose:

   ```bash
   gh pr create --repo Mudlet/Mudlet --base development \
     --title "Fix: <short non-technical title>" \
     --body "$(cat <<'EOF'
   <the body from step 4>
   EOF
   )"
   ```

   If `gh` resolves the wrong head, pass `--head <fork-owner>:<branch-name>`, parsing
   `<fork-owner>` from the `origin` remote URL.

   A draft can be marked ready later with `gh pr ready <number>`, so choosing draft is the
   reversible option.

8. **Report the result** — the PR URL on success, the error output on failure.

## Notes

- Before a human signs off on AI-assisted work they must have built and manually tested it; see
  the commit-trailer policy in `docs/ai-instructions.md`. Do not fabricate a `Signed-off-by`.
- Never force-push to a remote branch.
