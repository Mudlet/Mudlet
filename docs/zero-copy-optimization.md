# Zero-Copy Optimization Opportunities in Mudlet

This document outlines opportunities for zero-copy optimizations in Mudlet's data processing paths, along with benchmarking methodology to validate improvements.

## Overview

Zero-copy techniques reduce unnecessary memory allocations and data copying, which can significantly improve performance for I/O-heavy operations like network data processing.

## High Priority Areas

### 1. Network/Telnet Path (`src/ctelnet.cpp`)

This is the hot path for all incoming MUD data.

#### Current Issues

| Location | Problem | Impact |
|----------|---------|--------|
| `processSocketData()` ~line 4652 | Builds `std::string cleandata` char-by-char with `+=` | Multiple reallocations |
| `gotRest()`/`gotPrompt()` | Uses `substr()` creating temporary copies | Unnecessary allocations |
| `encodeAndCookBytes()` | Multiple conversions: `std::string` → `QString` → `QByteArray` → `std::string` | 3-4x copying |

#### Recommended Fixes

1. **Pre-allocate strings**: Use `reserve()` before building strings
2. **Use `std::string_view`**: For read-only access to string portions
3. **Copy spans**: Instead of char-by-char, copy contiguous ranges between special characters
4. **Reduce conversions**: Minimize QString ↔ std::string conversions

### 2. Text Processing (`src/TBuffer.cpp`)

#### Current Issues

| Location | Problem |
|----------|---------|
| `translateToPlainText()` ~line 570 | Copies incoming `std::string` into `localBuffer` |
| Character accumulation (lines 760-1155) | Appends to `mMudLine` char-by-char |

#### Recommended Fixes

1. Use `std::string_view` for incoming data parameter
2. Reserve capacity for `mMudLine` and `mMudBuffer`
3. Batch character operations instead of individual appends

### 3. Lua/C++ Boundary (`src/TLuaInterpreter.cpp`)

#### Current Issues

- `getVerifiedString()` wraps `lua_tostring()` in QString, creating copies
- `lua_pushstring()` calls create temporary `toUtf8()` QByteArrays

## Benchmarking

### Running Benchmarks

```bash
# Build the benchmark
cmake --build build --target TelnetBenchmark

# Run with ctest
QT_QPA_PLATFORM=offscreen ctest -R TelnetBenchmark -V

# Run directly for more options
./build/test/functional_tests/TelnetBenchmark -tickcounter
```

### Baseline Results (December 2025)

Measured on the full `cTelnet::loopbackTest()` → `processSocketData()` → `TBuffer::translateToPlainText()` pipeline.

**Release build (no sanitizers):**

| Data Size | Bytes | Time/Iteration | Throughput |
|-----------|-------|----------------|------------|
| Small | 780 B | 0.28 ms | ~2.7 MB/s |
| Medium | 7,800 B | 2.8 ms | ~2.7 MB/s |
| Large | 78,000 B | 28 ms | ~2.7 MB/s |

**Debug build with AddressSanitizer (for reference):**

| Data Size | Bytes | Time/Iteration | Throughput |
|-----------|-------|----------------|------------|
| Small | 780 B | 3.8 ms | ~200 KB/s |
| Medium | 7,800 B | 38 ms | ~200 KB/s |
| Large | 78,000 B | 363 ms | ~210 KB/s |

Note: ASan adds ~13x overhead. Always benchmark in Release mode for accurate performance measurements.

### Test Data Characteristics

The benchmark uses simulated MUD traffic containing:
- ANSI color codes (`\x1b[1;32m`, `\x1b[0m`)
- Plain text (room descriptions)
- Line endings (`\r\n`)
- Telnet sequences (IAC GA: `\xff\xf9`)

### Profiling Tools

```bash
# CPU profiling with perf
perf record -g ./build/src/mudlet --profile "Test"
perf report

# Memory allocation tracking
heaptrack ./build/src/mudlet --profile "Test"
heaptrack_gui heaptrack.mudlet.*.zst

# Flamegraphs
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

## Implementation Notes

### String Building Patterns

**Before (char-by-char):**
```cpp
std::string cleandata;
for (size_t i = 0; i < size; ++i) {
    cleandata += data[i];  // Potential reallocation each iteration
}
```

**After (span copy with reserve):**
```cpp
std::string cleandata;
cleandata.reserve(size);
size_t spanStart = 0;
for (size_t i = 0; i < size; ++i) {
    if (data[i] == special_char) {
        cleandata.append(data + spanStart, i - spanStart);
        spanStart = i + 1;
    }
}
cleandata.append(data + spanStart, size - spanStart);
```

### Using std::string_view

**Before:**
```cpp
void processData(std::string& data) {
    std::string chunk = data.substr(start, len);  // Creates copy
}
```

**After:**
```cpp
void processData(std::string_view data) {
    std::string_view chunk = data.substr(start, len);  // No copy
}
```

## Related Issues

- GitHub Issue #5780: Buffer optimization TODOs (7 related items in ctelnet.cpp)
