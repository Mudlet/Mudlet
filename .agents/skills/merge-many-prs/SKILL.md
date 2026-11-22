---
name: merge-many-prs
description: >-
  Read before merging more than a handful of pull requests into development in one sitting,
  by hand or with agents. Gives the trial-merge workflow that tests the whole batch once
  before any of it lands.
---

# Merging many pull requests

PRs that are green on their own can still break `development` together, and a PR's checks can predate workflows that `development` runs today. On one 118-PR day both happened:

- #12664 removed `utils.h` from `dlgMapLabel.cpp`, and #12669 added `qsl()` calls to the same file. Each passed alone; together they broke the build on every platform.
- #12770's last CI run predated the WebAssembly workflow, so its spec failures there only showed up after it merged.

Re-running CI on every PR against the new base would take days. Test the batch once instead:

1. Merge every PR you plan to land into a scratch branch off the current `development`, in the order you will merge them. Resolve conflicts there first: a PR that conflicts with the batch is not ready.
2. Build it and run the Lua specs locally (see the `build-mudlet` skill). This takes minutes and catches PRs that compile or pass only on their own.
3. Push the branch and open a draft PR from it. Pushing alone runs nothing: the build workflows trigger on `pull_request`, and the WebAssembly and headless Lua workflows on pushes to `development` only. The draft runs every workflow once for the whole batch, including any that older PRs never ran.
4. If it fails, halve the batch to find the PR responsible, drop it, and test again.
5. Once the draft is green, close it and merge the individual PRs in the same order. Their own checks may be old or missing; the batch run is what covers them.

Also:

- A fix for `development`'s CI merges only after its own CI is green.
- Updating a PR's branch cancels its in-progress CI (`cancel-in-progress`), so update branches before CI starts, not while you are waiting on it.
