# AI Assistant Integration

Mudlet supports multiple AI coding assistants through a centralized instruction system. This document explains how it works and how to use it effectively.

*Note: The centralized instructions may be updated based on common AI assistant errors to improve accuracy and reduce failures.*

## Overview

The project uses a **centralized approach** where all AI assistants read from a single source file, ensuring consistent guidance across different tools.

```
.ai/ai-instructions.md          # Source of truth
├── .github/copilot-instructions.md → symlink (GitHub Copilot)
├── .claude-instructions.md         → symlink (Claude Code)  
└── .cursorrules                    → symlink (Cursor IDE)
```

## Supported AI Tools

- **GitHub Copilot** - Code completion and chat
- **Claude Code** - Anthropic's coding assistant
- **Cursor IDE** - AI-powered code editor

## Benefits

### For Developers

- **Consistent guidance**: All AI tools provide the same project-specific advice
- **Zero maintenance**: Update one file, all tools stay synchronized
- **Easy adoption**: Choose your preferred AI tool without setup complexity
- **Future-proof**: New AI tools can be added with a simple symlink

### For the Project

- **Code quality**: AI assistants understand Mudlet's specific patterns and standards
- **Onboarding**: New contributors get project-specific guidance immediately
- **Consistency**: Reduces variation in coding styles and approaches
- **Documentation**: AI instructions serve as living documentation of best practices

## What AI Assistants Know About Mudlet

The centralized instructions provide AI tools with essential knowledge about:

- **Core Technologies**: C++17, Qt6, Lua 5.1, CMake build system
- **Project Structure**: Directory layout (`src/`, `test/`, `3rdparty/`, etc.)
- **Architecture**: Key classes like `mudlet`, `Host`, `ctelnet`, `TConsole`, `TLuaInterpreter`, `TMap`
- **Coding Standards**: Naming conventions, memory management patterns, smart pointer usage
- **Build System**: Cross-platform commands for macOS, Linux, Windows
- **Debugging**: Available debug flags in both CMake and QMake systems
- **Critical Guidelines**: Thread safety, Lua stack management, GPL headers, communication style

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
rm .github/copilot-instructions.md .claude-instructions.md .cursorrules
git checkout HEAD -- .github/copilot-instructions.md .claude-instructions.md .cursorrules
```

#### Option 2: Manual Copy (Fallback)

If symlinks don't work:

```bash
# Copy to all AI tool locations
cp .ai/ai-instructions.md .github/copilot-instructions.md
cp .ai/ai-instructions.md .claude-instructions.md
cp .ai/ai-instructions.md .cursorrules
```

*Note: With manual copy, you must manually update all copies when `.ai/ai-instructions.md` changes.*

## How It Works

1. **Single Source**: All guidance is written in `.ai/ai-instructions.md`
2. **Symlinks**: Tool-specific files point to the central source
3. **Automatic Sync**: Changes to the source are immediately available to all tools
4. **Cross-Platform**: Works on macOS, Linux, and Windows (with setup)

## Making Changes

### For Maintainers

Edit `.ai/ai-instructions.md` to update guidance for all AI tools simultaneously.

### For Contributors

The AI instructions are designed to help you write code that fits Mudlet's patterns. Let your AI assistant guide you based on the project's standards.

## Examples of AI Assistant Help

With the centralized instructions, AI assistants can help with:

- **Code Generation**: Creates code following Mudlet's naming conventions and smart pointer patterns
- **Error Handling**: Suggests Qt-style error patterns with proper `qWarning()` usage
- **Lua API Functions**: Provides templates for new Lua scripting functions with proper stack management
- **Memory Management**: Recommends appropriate smart pointers (Qt vs STL) and parent-child relationships
- **Cross-Platform Code**: Provides build commands and considers Windows, macOS, and Linux compatibility
- **GPL Compliance**: Includes proper copyright headers for new files

## Technical Details

### File Structure

```text
/Mudlet/
├── .ai/
│   └── ai-instructions.md              # Master instructions (189 lines)
├── .github/
│   └── copilot-instructions.md         # → ../.ai/ai-instructions.md
├── .claude-instructions.md             # → .ai/ai-instructions.md
├── .cursorrules                        # → .ai/ai-instructions.md
└── AI-ASSISTANTS.md                    # This documentation
```

### Symlink Commands

```bash
# How the symlinks were created
ln -s ../.ai/ai-instructions.md .github/copilot-instructions.md
ln -s .ai/ai-instructions.md .claude-instructions.md
ln -s .ai/ai-instructions.md .cursorrules
```

## Adding New AI Tools

To support a new AI assistant:

1. Research the tool's instruction file convention
2. Create a symlink: `ln -s .ai/ai-instructions.md .new-tool-instructions`
3. Add the symlink to Git: `git add .new-tool-instructions`
4. Update this documentation

## Troubleshooting

### Symlinks Not Working

- **Windows**: Enable Developer Mode or use manual copy approach
- **Permissions**: Ensure you have file creation permissions
- **Git Config**: Run `git config core.symlinks true`

### AI Tool Not Reading Instructions

- Verify the instruction file exists and is readable
- Check that the tool supports instruction files
- Confirm the filename matches the tool's expected convention

## Contributing

When contributing to Mudlet with AI assistance:

1. Let your AI tool guide you using the project instructions
2. Review suggestions for alignment with Mudlet's patterns
3. The AI should understand Qt conventions, C++17 features, and Lua integration
4. If AI suggestions don't align with the project, consider updating the instructions

## Questions or Issues

- **Setup problems**: See [.github/README.md](.github/README.md) for detailed setup instructions
- **Instruction updates**: Submit pull requests to modify `.ai/ai-instructions.md`
- **New AI tools**: Open an issue to discuss adding support for additional assistants
