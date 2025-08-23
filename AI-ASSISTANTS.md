# AI Assistant Integration

Mudlet supports multiple AI coding assistants through a centralized instruction system. This document explains how it works and how to use it effectively.

## Overview

The project uses a **centralized approach** where all AI assistants read from a single source file, ensuring consistent guidance across different tools.

```
.ai/ai-instructions.md          # Source of truth
├── .github/copilot-instructions.md → symlink (GitHub Copilot)
├── CLAUDE.md                        → symlink (Claude Code)  
└── .cursorrules                     → symlink (Cursor IDE)
```

## Supported AI Tools

- **GitHub Copilot**
- **Claude Code**
- **Cursor IDE**

## Getting Started

Choose any supported AI assistant - they all receive the same project guidance through symlinks to `.ai/ai-instructions.md`.

**Windows Users**: May need to enable symlink support - see Windows setup section below.

### Windows Symlink Setup

#### Option 1: Enable Developer Mode (Recommended)

1. Open Windows Settings → Update & Security → For developers
2. Enable "Developer Mode"
3. Configure Git and re-create symlinks:

```cmd
git config core.symlinks true
rm .github/copilot-instructions.md CLAUDE.md .cursorrules
git checkout HEAD -- .github/copilot-instructions.md CLAUDE.md .cursorrules
```
