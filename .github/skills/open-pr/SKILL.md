---
name: open-pr
description: >-
  Publish the current branch and open a pull request to the upstream Mudlet repository.
  Generates a concise, non-technical PR title and description following the project template.
argument-hint: Optional hint about what this PR does (e.g. "adds shimmer blink effect")
user-invocable: true
disable-model-invocation: true
---

## When to Use

Use this skill when the user asks to open a pull request, create a PR, or push their changes upstream. The skill publishes the current branch if needed and opens a PR against the upstream `development` branch.

## Procedure

1. **Gather context**: Run `git diff upstream/development...HEAD` to understand what changed. Also run `git log upstream/development..HEAD --oneline` for commit messages and `git branch --show-current` for the branch name.

2. **Determine the PR title prefix** based on the nature of the changes:
   - `Fix: ` — for bug fixes
   - `Improve: ` — for enhancements to existing functionality
   - `Add: ` — for new features or capabilities
   - `Infrastructure: ` — for build system, CI, tooling, or project configuration changes

3. **Generate the PR title**: After the prefix, write a short, non-technical summary that anyone can understand. This title appears in PTB changelogs.

4. **Generate the PR body** using this exact template format:

   ```
   #### Brief overview of PR changes/additions

   <1-3 sentences summarizing what changed, written for a non-technical audience>

   #### Motivation for adding to Mudlet

   <1-2 sentences explaining why this change is valuable>

   #### Other info (issues closed, discussion etc)

   <Any related issues, discussions, or notes — or "None" if not applicable>
   ```

5. **Show the draft title and body to the user** and ask for confirmation before proceeding.

6. **Publish the branch** if it has not been pushed to the user's fork yet:
   - Run `git push -u origin <branch-name>` to push and set the upstream tracking branch.

7. **Open the pull request** using the `mcp_github_create_pull_request` tool:
   - `owner`: `Mudlet`
   - `repo`: `Mudlet`
   - `base`: `development`
   - `head`: `<fork-owner>:<branch-name>` (e.g. `mpconley:feature/my-branch`)
   - `title`: The generated PR title
   - `body`: The generated PR body

   To determine `<fork-owner>`, parse it from the `origin` remote URL.

8. **Report the result**: Show the user the PR URL on success, or the error details on failure.
