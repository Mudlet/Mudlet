# Copilot Instructions for Mudlet

## Project Overview

Mudlet is a cross-platform MUD (Multi-User Dungeon) client built with Qt6 and C++17. It provides a modern interface for playing text-based RPGs with advanced scripting capabilities in Lua. The project emphasizes "powerful simplicity" - delivering a clean, uncluttered interface while providing deep customization and scripting features.

## Core Technologies

- **Primary Language**: C++17
- **GUI Framework**: Qt6 (minimum 6.4.0)
- **Build System**: CMake (minimum 3.25.1)
- **Scripting Language**: Lua 5.1
- **Platform Support**: Windows, macOS, Linux

## Architecture Overview

### Key Components

- **Core Classes**: Located in `src/` directory
  - `mudlet.h/cpp` - Main application class
  - `Host.h/cpp` - Game connection management
  - `TConsole.h/cpp` - Text display and input handling
  - `TLuaInterpreter.h/cpp` - Lua scripting engine
  - `TMap.h/cpp` - Mapping system
  - `TBuffer.h/cpp` - Text buffer management

- **UI Components**: Dialog classes (dlg*.h/cpp files)
  - Profile management, scripting editors, preferences
  - All dialogs should follow Qt's Model-View pattern

- **Scripting System**: 
  - Lua API implementations in `TLuaInterpreter*.cpp` files
  - Action/Alias/Trigger/Timer systems for game automation

### Directory Structure

```
src/                   # Main source code
├── mudlet-lua/        # Lua API implementations
├── ui/                # UI form files (.ui)
├── icons/             # Application icons
└── fonts/             # Bundled fonts

3rdparty/              # Third-party dependencies
templates/             # Profile templates
translations/          # Internationalization files
CI/                    # Continuous integration scripts
```

## Design Philosophy

### UI Design Principles

1. **Clarity over features**: Default to hiding advanced options
2. **Progressive disclosure**: Casual players see simple interface, power users access advanced features
3. **Pure players first**: Interface should be clean and uncluttered for gameplay
4. **Script-focused developers**: Provide rich API and customization options

### Code Style Guidelines

- Follow existing `.clang-format` configuration in `src/`
- Use descriptive class and variable names (Hungarian notation is used historically)
- Prefer Qt containers and QString over STL equivalents
- Use Qt's signal/slot mechanism for inter-object communication
- All public APIs should be documented with Doxygen comments

## Coding Standards

### C++ Conventions

```cpp
// Class naming: PascalCase with 'T' prefix for main classes
class TConsole : public QWidget

// Member variables: camelCase with 'm' prefix
QString mProfileName;

// Constants: ALL_CAPS
static const int MAX_BUFFER_SIZE = 1000000;

// Qt signals/slots: camelCase
signals:
    void profileChanged(const QString& name);
```

### Lua API Conventions

- All Lua functions should have comprehensive error checking
- Use `lua_error()` for fatal errors, return `nil + error message` for recoverable errors
- Follow existing pattern in `TLuaInterpreter*.cpp` files
- Document all Lua functions with proper parameter types and return values

## Common Patterns

### Error Handling

```cpp
// Qt-style error handling
if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open file:" << file.errorString();
    return false;
}
```

### String Handling

```cpp
// Prefer QString for UI strings, std::string for internal processing
QString displayText = tr("Connection failed: %1").arg(errorMessage);

// Use qsl() macro for string literals (defined as QStringLiteral)
QString objectName = qsl("timer(Host:%1)(TTimerId:%2)").arg(hostName, timerName);
QString funcName = qsl("Timer%1").arg(QString::number(mID));

// Use qsl() for compile-time string constants for better performance
const QString errorMsg = qsl("Discord API is not available");
```

### Memory Management

- Use Qt's parent-child system for automatic memory management
- Use smart pointers (QSharedPointer, QScopedPointer) when ownership is unclear
- Follow RAII principles

## Testing

- Unit tests are located in `test/` directory
- Use Qt Test framework for new tests
- Integration tests should cover Lua API functionality
- UI tests should use Qt's accessibility framework

## Build System

### CMake Configuration

- Main CMakeLists.txt handles platform-specific configurations
- Use CMake's find_package for dependencies
- Follow existing patterns for adding new source files

### Legacy QMake Support

- `src/mudlet.pro` is maintained for compatibility and version management
- When updating version numbers, both CMakeLists.txt and mudlet.pro must be updated
- QMake build is secondary to CMake; CMake is the primary build system

### Dependencies

Key third-party libraries:
- Qt6 (Core, GUI, Widgets, Network, Multimedia, etc.)
- Lua 5.1
- PCRE (regular expressions)
- Hunspell (spell checking)
- Various others in `3rdparty/` directory

## Lua Scripting Integration

### Adding New Lua Functions

1. Declare in appropriate `TLuaInterpreter*.h` file
2. Implement in corresponding `.cpp` file
3. Register in `initLuaGlobals()` or similar initialization function
4. Add to `lua-function-list.json` for auto-completion
5. Document in wiki/manual

### Lua Function Template

```cpp
// In TLuaInterpreterUI.cpp
int TLuaInterpreter::createLabel(lua_State* L)
{
    const QString labelName = getVerifiedString(L, __func__, 1, "label name");
    // ... implementation
    
    lua_pushboolean(L, true);
    return 1; // number of return values
}
```

## Internationalization

- Use `tr()` for all user-visible strings
- Translation files are in `translations/` directory
- Follow Qt's internationalization guidelines
- Test with different locales, especially for text layout

## Platform-Specific Code

### macOS
- Objective-C++ files (.mm) for macOS-specific features
- Use `#ifdef Q_OS_MACOS` for platform-specific code

### Windows
- Use Windows API judiciously, prefer Qt alternatives
- Handle file path differences (backslash vs forward slash)

### Linux
- Support multiple distributions
- Handle different desktop environments appropriately

## Common Pitfalls

1. **Qt Object Lifetime**: Remember parent-child relationships
2. **Signal/Slot Connections**: Use new connect syntax when possible
3. **String Encoding**: Always specify encoding when reading files
4. **Thread Safety**: Most Qt UI operations must happen on main thread
5. **Lua Stack Management**: Always balance pushes/pops in Lua API functions

## Performance Considerations

- Text rendering is performance-critical (TBuffer/TConsole classes)
- Lua script execution should be non-blocking where possible
- Use Qt's model/view framework for large datasets
- Profile memory usage, especially for long-running sessions

## Security Guidelines

- Validate all user input, especially in Lua scripts
- Be cautious with file operations from Lua
- Network connections should have proper timeout handling
- Sanitize data when interfacing with external processes

## Contributing Guidelines

1. Follow existing code style and patterns
2. Add appropriate error handling and logging
3. Update documentation for public API changes
4. Test on multiple platforms when possible
5. Consider backward compatibility for Lua scripts
6. Update translations if adding user-visible strings

## When Making Changes

- **Small fixes**: Follow existing patterns closely
- **New features**: Consider UI design philosophy and user types
- **API changes**: Maintain backward compatibility when possible
- **Performance changes**: Profile before and after
- **UI changes**: Consider accessibility and different screen sizes

For more detailed information, see:
- Build instructions: http://wiki.mudlet.org/w/Compiling_Mudlet
- User documentation: https://wiki.mudlet.org/
- API documentation: Generated from source via Doxygen
