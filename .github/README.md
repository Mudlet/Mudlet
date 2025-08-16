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

- **`copilot-instructions.template.md`** - Template for GitHub Copilot instructions
- **`copilot-instructions.md`** - Personal GitHub Copilot instructions (git-ignored)
- **`codeql/`** - CodeQL security analysis configuration

## GitHub Copilot Instructions

The `copilot-instructions.md` file provides context-aware guidance to GitHub Copilot about Mudlet's architecture, coding standards, and best practices. This helps Copilot generate more accurate and project-appropriate suggestions.

### Setting Up Your Personal Copilot Instructions

1. Copy the template: `cp copilot-instructions.template.md copilot-instructions.md`
2. Customize the instructions based on your development focus areas
3. The file is git-ignored, so your personal customizations won't be committed

### Why It's Git-Ignored

- **Personal preferences**: Different developers may want to emphasize different aspects
- **Multiple contributors**: Avoids conflicts when multiple people modify instructions
- **Flexibility**: Allows experimentation without affecting others
- **Template preservation**: Keeps the base template clean and up-to-date

The template file (`copilot-instructions.template.md`) contains comprehensive guidance about:

- Project architecture and design philosophy
- Coding standards and conventions
- Common patterns and best practices
- Build system usage
- Testing approaches
- Platform-specific considerations

Feel free to modify your personal copy to better suit your development workflow and areas of focus within the Mudlet codebase.
