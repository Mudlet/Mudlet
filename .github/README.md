# .github Directory

This directory contains GitHub-specific configuration files and templates that help manage the Mudlet project's development workflow.

## Files and Their Purpose

### Community & Contribution Files

- **`CODE_OF_CONDUCT.md`** - Community standards and behavior expectations
- **`CONTRIBUTING.md`** - Guidelines for contributing to the project
- **`SUPPORT.md`** - Information on getting help and support
- **`FUNDING.yml`** - Sponsorship and funding information for GitHub's sponsor button

### Issue & PR Management

- **`ISSUE_TEMPLATE/`** - Templates for bug reports and feature requests
  - `01-bug-report.md` - Standard bug report template
  - `02-feature-request.md` - Feature request template  
  - `config.yml` - Issue template configuration
- **`PULL_REQUEST_TEMPLATE.md`** - Default template for pull requests
- **`CODEOWNERS`** - Automatically assigns reviewers to PRs (currently assigns @Mudlet/mudlet-makers)

### Automation & CI/CD

- **`workflows/`** - GitHub Actions workflow definitions
  - `build-mudlet.yml` / `build-mudlet-win.yml` - Build automation for different platforms
  - `codeql-analysis.yml` - Security analysis
  - `codespell-analysis.yml` - Spell checking
  - `clangtidy-*.yml` - C++ code quality analysis
  - `dangerjs.yml` - PR validation and automation
  - `generate-*.yml` - Various content generation workflows
  - `update-*.yml` - Automated update workflows for dependencies and translations
  - And many more specialized automation workflows

### Configuration Files

- **`dependabot.yml`** - Dependabot configuration for automated dependency updates
- **`pr-labeler.yml`** - Automatic labeling of pull requests based on file changes
- **`repo-metadata.yml`** - Repository metadata configuration
- **`codespell-wordlist.txt`** - Custom dictionary for spell checking

### Developer Tools

- **`copilot-instructions.md`** - GitHub Copilot instructions (symlinked from `../.ai/ai-instructions.md`)
- **`codeql/`** - CodeQL security analysis configuration

## AI Assistant Instructions

Multiple AI tools can access centralized project guidance through symlinks to a single source file.

### Centralized AI Instructions

All AI assistant instructions are centralized in the `.ai/ai-instructions.md` file, with tool-specific symlinks:

- **`.github/copilot-instructions.md`** → `.ai/ai-instructions.md` (GitHub Copilot)
- **`.claude-instructions.md`** → `.ai/ai-instructions.md` (Claude Code)
- **`.cursorrules`** → `.ai/ai-instructions.md` (Cursor IDE)

This approach provides:

- **Single source of truth**: One file to maintain for all AI assistants
- **Consistency**: All AI tools receive the same project guidance
- **Easy maintenance**: Updates are automatically reflected across all tools

### Windows Users: Symlink Support

Windows users may need to enable symlink support for Git. For detailed setup instructions, see the [AI Assistant Integration Guide](../AI-ASSISTANTS.md#3-windows-users-symlink-setup).

### What's Included

The centralized instructions cover:

- C++17/Qt6/Lua 5.1 coding standards  
- Key architectural patterns
- Common development pitfalls
- Build system basics (CMake primary, QMake legacy)
