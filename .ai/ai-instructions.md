# AI Assistant instructions for Mudlet

## Project overview

Mudlet is a cross-platform MUD client built with Qt6 and C++20, providing scripting capabilities in Lua 5.1. The project emphasizes "powerful simplicity" - clean interface with deep customization options.

## Core technologies

- **C++20** with Qt6 (minimum 6.4.0)
- **CMake** build system (minimum 3.25.1)  
- **Lua 5.1** scripting engine
- **Cross-platform**: Windows, macOS, Linux

## Project structure

- `src/` - main application source code
- `test/` - Qt Test unit tests for C++ core
- `src/mudlet-lua/tests/` - Busted unit tests for Lua functionality
- `3rdparty/` - External dependencies and libraries
- `translations/` - Internationalization files
- `.github/workflows/` - Github Actions workflows

## Essential Coding Standards

### C++ Conventions

```cpp
// Class names: PascalCase with 'T' prefix for main classes
class TConsole : public QWidget

// Member variables: camelCase with 'm' prefix
QString mProfileName;

// Qt signals/slots: camelCase
signals:
    void profileChanged(const QString& name);
```

### String Handling

```cpp
// Use qsl() macro for string literals (defined as QStringLiteral)
QString objectName = qsl("timer(Host:%1)(TTimerId:%2)").arg(hostName, timerName);

// Prefer QString for UI, tr() for user-visible strings
QString displayText = tr("Connection failed: %1").arg(errorMessage);
```

### Memory Management

- Use Qt's parent-child system for automatic cleanup
- Use smart pointers (QSharedPointer, QScopedPointer) when ownership is unclear
- Follow RAII principles

**Smart Pointer Patterns:**

```cpp
// Qt smart pointers for Qt objects
QSharedPointer<Host> host;              // Shared ownership
QScopedPointer<VarUnit> varUnit;        // Single ownership, auto-delete

// STL smart pointers for cross-platform code
std::shared_ptr<TMediaPlayer> player;   // Shared ownership
std::unique_ptr<QTimer> timer;          // Single ownership
std::weak_ptr<TMediaPlayer> weakRef;    // Non-owning reference
```

## Key Architecture Points

### Core Classes (src/ directory)

- `mudlet.h/cpp` - Main application
- `Host.h/cpp` - Game connection management
- `ctelnet.h/cpp` - Telnet protocol handling
- `TConsole.h/cpp` - Text display and input
- `TLuaInterpreter.h/cpp` - Lua scripting engine
- `TMap.h/cpp` - Mapping system

### Lua API Development

```cpp
// Standard Lua function template
int TLuaInterpreter::functionName(lua_State* L)
{
    const QString param = getVerifiedString(L, __func__, 1, "parameter name");
    // ... implementation
    
    lua_pushboolean(L, true);
    return 1; // number of return values
}
```

## Common Patterns

### Error Handling

```cpp
// Qt-style error handling
if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open file:" << file.errorString();
    return false;
}
```

### UI Components

- Dialog classes use `dlg*.h/cpp` naming
- Follow Qt's Model-View pattern
- Use Qt's signal/slot mechanism for communication

## Critical Guidelines

1. **Qt Object Lifetime** - Remember parent-child relationships
2. **Thread Safety** - UI operations must happen on main thread
3. **Lua Stack Management** - Balance pushes/pops in Lua API functions
4. **Cross-platform** - Test on Windows, macOS, Linux when possible
5. **Internationalization** - Use `tr()` for all user-visible strings
6. **Testing** - Tests located in `test/` directory, follow Qt Test framework patterns
7. **Communication** - Provide concise, structured responses; prioritize code examples over lengthy explanations
8. **File Headers** - New `.h/.cpp` files require GNU General Public License copyright headers (see existing files for format)

## Build System Notes

- **Primary**: CMake (handles platform-specific configurations)
- **Legacy**: QMake in `src/mudlet.pro` (for version management)
- Use `.clang-format` configuration in `src/` for code style

### Debugging Options

Both `src/CMakeLists.txt` and `src/mudlet.pro` contain commented debugging defines for development (search "Debugging code inclusions"):

- `DEBUG_TELNET` - Telnet protocol debugging
- `DEBUG_UTF8_PROCESSING` - UTF-8 decoding messages
- `DEBUG_SGR_PROCESSING` - ANSI color sequence debugging
- `DEBUG_WINDOW_HANDLING` - UI window operations
- And others for encoding, MXP, map autosave, etc.

**Usage**: Uncomment relevant `target_compile_definitions(mudlet PRIVATE DEBUG_XXX)` lines (CMake) or `DEFINES+=DEBUG_XXX` lines (QMake) when debugging specific areas. **Important**: Do not commit uncommented debug lines to git.

### Building on macOS

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_macOS

**Essential Build Commands:**

```bash
# Build
cd /path/to/Mudlet/build
cmake ../../Mudlet -DCMAKE_PREFIX_PATH=`brew --prefix qt6`
make -j `sysctl -n hw.ncpu`

# Run
cd /path/to/Mudlet/build
./src/mudlet.app/Contents/MacOS/mudlet
```

### Building on Linux

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_Linux

**Essential Build Commands:**

```bash
# Build
cd /path/to/Mudlet/build
cmake ../../Mudlet
make -j $(nproc)

# Run
cd /path/to/Mudlet/build
./src/mudlet
```

### Building on Windows

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_Windows

**Essential Build Commands:**

```cmd
REM Build
cd \path\to\Mudlet\build
cmake ..\..\Mudlet -G "Visual Studio 17 2022"
cmake --build . --config Release --parallel

REM Run
cd \path\to\Mudlet\build
.\src\Release\mudlet.exe
```
