---
description: Create a PR with Mudlet's template format
allowed-tools: Bash, Read, Glob, Grep
---

Create a pull request for the current branch. Follow these steps:

1. Run `git log development..HEAD --oneline` and `git diff development...HEAD` to understand all changes
2. Determine the appropriate title prefix:
   - `add:` for new user-facing features
   - `improve:` for enhancements to existing user-facing features
   - `fix:` for bug fixes visible to users
   - `infrastructure:` for internal/non-player-visible changes (refactoring, build system, code cleanup, developer tooling)
3. Write a title that a non-technical user can understand (appears in changelogs)

Create the PR using `gh pr create` with this exact template format:

```
gh pr create --title "<prefix>: <short non-technical title>" --body "$(cat <<'EOF'
#### Brief overview of PR changes/additions
<1-3 terse bullet points describing what changed>

#### Motivation for adding to Mudlet
<1 sentence explaining why this change matters>

#### Other info (issues closed, discussion etc)
<Reference any issues with "Fixes #123" or "Closes #123" if applicable>

**Test case:** <Brief steps to verify the change works>
EOF
)"
```

Keep all descriptions terse and to the point. No fluff.
