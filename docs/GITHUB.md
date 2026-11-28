# .github Directory

This directory contains GitHub-specific configuration files and templates that help manage the Mudlet project's development workflow.

## Files and Their Purpose

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

- **`copilot-instructions.md`** - symlink to the root `AGENTS.md`, for the Copilot surfaces that don't read `AGENTS.md` yet
- **`codeql/`** - CodeQL security analysis configuration

## AI Assistant Integration

For which AI assistants read which files, and Windows symlink setup, see **[AI-ASSISTANTS.md](AI-ASSISTANTS.md)**.
