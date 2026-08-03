# Platform builds and debugging defines

## Building on macOS

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_macOS

**Essential build commands:**

```bash
# Build
cd /path/to/Mudlet/build
# wait up to 10mins for a build
cmake ../../Mudlet -DCMAKE_PREFIX_PATH=`brew --prefix qt6`
make -j `sysctl -n hw.ncpu`

# Run Mudlet - use absolute path to avoid directory confusion
/path/to/Mudlet/build/src/mudlet.app/Contents/MacOS/mudlet
```

## Building on Windows

For complete setup instructions, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Compiling_on_Windows

## Debugging options

`src/CMakeLists.txt` contains commented debugging defines for development (search "Debugging code inclusions"):

- `DEBUG_TELNET` - Telnet protocol debugging
- `DEBUG_UTF8_PROCESSING` - UTF-8 decoding messages
- `DEBUG_SGR_PROCESSING` - ANSI color sequence debugging
- `DEBUG_WINDOW_HANDLING` - UI window operations
- And others for encoding, MXP, map autosave, etc.

**Usage**: Uncomment the relevant `target_compile_definitions(${LIB_MUDLET_TARGET} PUBLIC DEBUG_XXX)` lines when debugging specific areas. **Important**: Do not commit uncommented debug lines to git.
