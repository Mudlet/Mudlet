---
name: open-pr
description: >-
  Publish the current branch and open a pull request against the upstream Mudlet repository,
  using the project's PR template and the title prefix that Danger enforces. Use when asked to
  open a PR, create a pull request, or push changes upstream - whether that is asked for by a
  person, or reached at the end of an agent's task.
license: GPL-2.0-or-later
argument-hint: Optional hint about what this PR does (e.g. "adds shimmer blink effect")
user-invocable: true
---

## When to use

Use when the user asks to open a pull request, create a PR, or push their changes upstream. This
publishes the current branch to their fork if needed, then opens a PR against upstream
`development`.

Agents opening a pull request at the end of their own task use this too. The procedure is the same
either way; only steps 5 and 6 have a person in them, and each says what to do when there is nobody
to ask.

Either way the branch has to be finished before this starts - reviewed, formatted, fully committed.
Opening the pull request is the last step of the work rather than a checkpoint partway through it,
because every push afterwards re-runs the automated reviewers over the whole diff.

## Procedure

1. **Gather context.** Run `git status --porcelain` first. If anything is uncommitted, stop: ask
   the user to commit what belongs in the PR, or to confirm explicitly that it should be left out.
   A pull request is built from commits, so uncommitted work is silently absent from it.

   Then run `git log upstream/development..HEAD --oneline` for the commit messages,
   `git diff upstream/development...HEAD` to see what actually changed, and
   `git branch --show-current` for the branch name. An empty commit list means there is nothing to
   open a pull request for.

   `git branch --show-current` prints nothing on a detached HEAD. If it is empty, stop and ask the
   user to check out a named branch — every later step needs that name, and an empty one produces
   a malformed push and an unusable `--head`.

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
   to verify the change — not part of the template, but reviewers expect it. No fluff; about one
   screen in total.

   For AI-assisted work, close the body with the `Assisted-by: AGENT_NAME:MODEL_VERSION` trailer
   described in `docs/CONTRIBUTING.md`, naming the model that did the work. It goes in the body as
   well as in the commits because a squash merge drops commit trailers.

5. **Settle draft versus ready for review.**

   With a person in the conversation, show them the draft title and body and ask which of the two
   to open. Wait for both answers before anything is published — never assume either.

   Running unattended, do not stall on the wording: open ready for review, which is what a finished
   branch wants, and report the title and body along with the URL. Choose draft instead when the
   branch is knowingly unfinished — work still moving, a question that needs answering before
   reviewers spend their time, or an experiment put up only for CI feedback. `gh pr ready <number>`
   promotes a draft later, so draft is the reversible choice whenever it is a close call.

6. **Confirm the fork before pushing anything.** `origin` is not guaranteed to be the user's fork,
   and pushing to the wrong remote is awkward to undo, so establish this before the push rather
   than after. Derive the head explicitly too — a checkout commonly has several remotes, including
   other people's forks, and a head inferred by `gh` can point at the wrong one.

   ```bash
   BRANCH=$(git branch --show-current)
   FORK_OWNER=$(printf '%s' "$(git remote get-url --push origin)" \
     | sed -E 's#\.git$##; s#^[a-zA-Z+]+://##; s#^[^@/]+@##;
               s#^[^/:]+(:[0-9]+)?/##; s#^[^/:]+:##; s#/[^/]*$##')
   ```

   Use `--push`: a remote can carry a separate `pushurl`, and it is the push URL the branch
   actually lands on, so the head must be derived from the same URL `git push` will use.

   This handles the `https://`, `ssh://` and `git@host:owner/repo` forms, with or without an
   explicit port. Check the result before using it: if `$FORK_OWNER` is empty or still contains
   `/`, `:` or `@`, the URL was not in a form this understands — stop and report it rather than
   building a malformed `--head`.

   Then check the owner itself. `$FORK_OWNER` must not be `Mudlet`: that is upstream, and pushing a
   branch straight there is the mistake this step exists to catch. Compare it against
   `gh api user --jq .login`, the account `gh pr create` will act as — a match is confirmation
   enough to carry on unattended. If the two differ, as they legitimately do when the fork belongs
   to an organisation, show both values and ask before pushing.

7. **Publish the branch** with `git push -u origin "$BRANCH"`. Do this every time, not only when
   the branch lacks an upstream — a branch that already tracks `origin` can still hold local
   commits that have not been pushed, and those would be missing from the pull request.

   The push must succeed before continuing. If it fails, stop and report the error; do not open a
   pull request against a branch whose commits are not on the fork.

8. **Open the PR** against upstream, adding `--draft` if that is what step 5 settled on:

   ```bash
   gh pr create --repo Mudlet/Mudlet --base development \
     --head "${FORK_OWNER}:${BRANCH}" \
     --title "Fix: <short non-technical title>" \
     --body "$(cat <<'EOF'
   <the body from step 4>
   EOF
   )"
   ```

   A draft can be marked ready later with `gh pr ready <number>`, so choosing draft is the
   reversible option.

9. **Report the result** — the PR URL on success, the error output on failure.

## Notes

- Before a human signs off on AI-assisted work they must have built and manually tested it; see
  the commit-trailer policy in `docs/ai-instructions.md`. Do not fabricate a `Signed-off-by`.
- Never force-push to a remote branch.
