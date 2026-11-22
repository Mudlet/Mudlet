# AI Assistant Integration

Mudlet's instructions for AI coding assistants live in one file, [`AGENTS.md`](../AGENTS.md) at the repository root, in the [AGENTS.md](https://agents.md) format. Task-specific instructions live in `.agents/skills/<name>/SKILL.md`, in the [Agent Skills](https://agentskills.io) format.

| Path | What it is | Why it exists |
|------|------------|---------------|
| `AGENTS.md` | The instructions (regular file) | Read natively by Claude Code, Cursor, OpenAI Codex, and Copilot's cloud agent, CLI, github.com code review and VS Code chat |
| `.agents/skills/` | The skills (regular files) | Read natively by Cursor, Codex and Copilot |
| `.claude/skills` | Symlink to `.agents/skills` | Claude Code only discovers project skills in `.claude/skills` |
| `.github/copilot-instructions.md` | Symlink to `AGENTS.md` | Copilot Chat on github.com, and Copilot Chat and code review in Visual Studio, JetBrains, Xcode and VS Code, don't read `AGENTS.md` |

Don't add a `CLAUDE.md`: Claude Code reads `AGENTS.md` only when no `CLAUDE.md` exists, and tools that read both (Cursor, Copilot code review) would get the instructions twice. For personal Claude-specific notes use `~/.claude/CLAUDE.md` or `.claude/rules/`, which don't stop `AGENTS.md` loading; a `CLAUDE.local.md` in the checkout does. Claude Code needs v2.1.277 or later to read `AGENTS.md`.

The legacy `.cursorrules` file is gone; Cursor reads `AGENTS.md` itself.

## Windows symlink setup

The two symlinks above need Git's symlink support on Windows; without it they check out as small text files containing the target path. `AGENTS.md` itself is a regular file and works either way.

1. Open Windows Settings → System → For developers (Update & Security → For developers on Windows 10)
2. Enable "Developer Mode"
3. Configure Git and re-create the symlinks:

```cmd
git config core.symlinks true
rm .github/copilot-instructions.md .claude/skills
git checkout HEAD -- .github/copilot-instructions.md .claude/skills
```
