# CPU Performance Optimization Issues

This document contains detailed issue descriptions for CPU optimization opportunities in Mudlet. Each can be created as a separate GitHub issue.

---

## Issue 1: Optimize Text Rendering with Line Metrics Caching

**Title:** `improve: Cache grapheme rendering metrics in TTextEdit to reduce CPU usage`

**Labels:** `enhancement`, `performance`, `renderer`

**Description:**

### Summary
The text rendering system in `TTextEdit::drawLine()` performs expensive grapheme-by-grapheme calculations on every paint event, including boundary finding, width calculations, and two separate rendering passes (background and foreground). This creates significant CPU overhead during text display.

### Current Behavior
- `QTextBoundaryFinder` runs on every line render
- Character width calculated per grapheme without caching
- Two separate loops: one for backgrounds, one for foregrounds
- No caching of computed line metrics

### Performance Impact
- High CPU usage during scrolling and text updates
- Repeated calculations for unchanged lines
- Estimated 15-25% improvement potential with caching

### Proposed Solution
Implement a line render cache:
```cpp
struct LineRenderCache {
    int lineNumber;
    QString lineHash; // or revision number
    QVector<QRect> textRects;
    QVector<QColor> fgColors;
    QVector<QString> graphemes;
    QVector<int> charWidths;
};
```

### Implementation Steps
1. Add cache structure to TTextEdit
2. Invalidate cache when line content changes
3. Use dirty-line tracking from needUpdate()
4. Reuse cached metrics when available

### Files Affected
- `src/TTextEdit.cpp` (drawLine, paintEvent)
- `src/TTextEdit.h`

### Expected Improvement
15-25% reduction in rendering CPU usage

---

## Issue 2: Optimize ANSI/SGR Sequence Parsing

**Title:** `improve: Optimize SGR color code parsing with lookup tables and caching`

**Labels:** `enhancement`, `performance`, `parser`

**Description:**

### Summary
The `TBuffer::decodeSGR()` function is a massive switch statement (1424-2200+ lines) that processes every ANSI escape code through extensive string splitting and parsing. This runs on every piece of colored text from the MUD server.

### Current Behavior
- Giant switch statement for all SGR codes
- `sequence.split(QChar(';'))` and `split(QChar(':'))` on every sequence
- Repeated `QStringList::at()` calls in loops
- No caching of common sequences
- No fast paths for frequent codes

### Performance Impact
- High CPU usage during colorful MUD output
- Estimated 10-20% improvement with optimizations

### Proposed Solution
1. **Lookup table for simple colors** (30-37, 40-47, 90-97, 100-107):
   ```cpp
   static const QHash<int, QColor> simpleFgColors = {
       {30, mBlack}, {31, mRed}, {32, mGreen}, ...
   };
   ```

2. **Cache common sequences**:
   ```cpp
   QHash<QString, ParsedSGR> sgrCache;
   if (sgrCache.contains(sequence)) {
       applyCachedSGR(sgrCache[sequence]);
       return;
   }
   ```

3. **Reserve QStringList capacity**:
   ```cpp
   QStringList params;
   params.reserve(estimatedCount);
   params = sequence.split(';');
   ```

### Implementation Steps
1. Profile most common SGR sequences
2. Create lookup table for simple codes
3. Implement LRU cache (size ~100)
4. Add fast path detection
5. Reserve container capacity

### Files Affected
- `src/TBuffer.cpp` (decodeSGR, decodeSGR38, decodeSGR48)
- `src/TBuffer.h`

### Expected Improvement
10-20% reduction in ANSI parsing CPU usage

---

## Issue 3: Optimize MUD Data Processing with Buffering

**Title:** `improve: Batch character processing in translateToPlainText`

**Labels:** `enhancement`, `performance`, `network`

**Description:**

### Summary
`TBuffer::translateToPlainText()` processes incoming MUD data character-by-character through multiple encoding checks and repeated QString::append() calls, causing significant CPU overhead and memory reallocations.

### Current Behavior
- Byte-by-byte processing through multiple conditionals
- No pre-allocation of output buffers
- Repeated `mMudLine.append()` calls (20+ locations)
- QString reallocation overhead on every append

### Performance Impact
- High CPU usage during high-volume MUD text
- Memory allocation churn
- Estimated 20% improvement with buffering

### Proposed Solution

1. **Reserve buffer capacity**:
   ```cpp
   void TBuffer::translateToPlainText(std::string& incoming, const bool isFromServer) {
       mMudLine.reserve(incoming.length() * 1.2); // Account for expansion
       mMudBuffer.reserve(incoming.length() * 1.2);
       // ... process
   }
   ```

2. **Create fast paths for common encodings**:
   ```cpp
   if (mEncoding == "UTF-8" && !mUSE_IRE_DRIVER_BUGFIX && !mMXP_MODE) {
       // Fast path for clean UTF-8
       QString converted = QString::fromUtf8(incoming.data(), incoming.length());
       // Handle in bulk
   }
   ```

3. **Batch character processing** for ASCII ranges

### Implementation Steps
1. Add buffer reservation at function start
2. Implement fast path for UTF-8 without MXP
3. Profile and optimize common encoding paths
4. Test with various MUD encodings

### Files Affected
- `src/TBuffer.cpp` (translateToPlainText)
- `src/TBuffer.h`

### Expected Improvement
20% reduction in text processing CPU usage

---

## Issue 4: Implement Spatial Indexing for 3D Map Rendering

**Title:** `improve: Add Z-plane indexing for 3D mapper to reduce room iteration`

**Labels:** `enhancement`, `performance`, `mapper`, `3d`

**Description:**

### Summary
The legacy OpenGL 3D mapper (`GLWidget::paintGL()`) has nested while loops that iterate through ALL rooms in an area on every frame, checking each room's Z-coordinate. This is extremely inefficient for large maps.

### Current Behavior
- Nested while loops iterate every room
- No spatial partitioning
- Checks every room for Z-plane matching
- Immediate mode OpenGL (deprecated)
- Per-room state changes

### Performance Impact
- Significant CPU usage on large maps
- Frame drops during rotation/navigation
- Estimated 50%+ improvement potential

### Proposed Solution

1. **Z-plane indexing**:
   ```cpp
   // In TArea or TMap
   std::unordered_map<int, std::vector<TRoom*>> roomsByZPlane;
   
   // Only iterate visible Z-planes
   for (int z = visibleZMin; z <= visibleZMax; ++z) {
       for (TRoom* room : roomsByZPlane[z]) {
           // render only visible rooms
       }
   }
   ```

2. **Viewport culling** to skip off-screen rooms

3. **Batch OpenGL state changes**

4. **Consider migrating to VBOs** (separate issue)

### Implementation Steps
1. Add Z-plane index structure to TArea/TMap
2. Maintain index on room add/remove/move
3. Update paintGL to use index
4. Add viewport culling
5. Batch state changes

### Files Affected
- `src/glwidget.cpp` (paintGL)
- `src/glwidget.h`
- `src/TMap.h`
- `src/TArea.h`

### Expected Improvement
50%+ reduction in 3D map rendering CPU usage for large maps

---

## Issue 5: Consolidate Timer System with Priority Queue

**Title:** `improve: Replace individual QTimers with centralized timer scheduler`

**Labels:** `enhancement`, `performance`, `timers`, `architecture`

**Description:**

### Summary
Every `TTimer` creates its own `QTimer` object, potentially resulting in hundreds of independent Qt timers firing. This creates unnecessary overhead and prevents optimization opportunities like timer coalescing.

### Current Behavior
- One QTimer per TTimer instance
- Independent firing for each timer
- No coalescing of near-simultaneous timers
- High overhead for many timers

### Performance Impact
- CPU overhead from Qt event loop handling hundreds of timers
- Missed optimization opportunities
- Estimated 10-15% improvement for timer-heavy profiles

### Proposed Solution

Centralized timer scheduler with priority queue:
```cpp
class TimerScheduler {
    struct TimerEvent {
        qint64 fireTime;
        TTimer* timer;
        bool operator<(const TimerEvent& other) const {
            return fireTime > other.fireTime; // Min-heap
        }
    };
    
    std::priority_queue<TimerEvent> timerQueue;
    QTimer* masterTimer; // Single timer
    
    void scheduleTimer(TTimer* timer, qint64 delay);
    void onMasterTimeout();
};
```

### Implementation Steps
1. Create TimerScheduler class
2. Migrate TTimer to use scheduler
3. Implement timer coalescing (50ms window)
4. Use Qt::CoarseTimer for non-critical timers
5. Test with timer-heavy scripts

### Files Affected
- New: `src/TimerScheduler.h`, `src/TimerScheduler.cpp`
- `src/TTimer.cpp`
- `src/TTimer.h`
- `src/TimerUnit.cpp`

### Expected Improvement
10-15% reduction in timer overhead for timer-heavy profiles

### Breaking Changes
**None** - Internal implementation detail

---

## Issue 6: Optimize Update/Repaint Cascades

**Title:** `improve: Consolidate update requests with dirty flag tracking`

**Labels:** `enhancement`, `performance`, `rendering`

**Description:**

### Summary
Multiple code paths trigger repaints through `forceUpdate()`, `needUpdate()`, and `showNewLines()`, and `TMap::update()` creates a new lambda on every call. This can cause redundant update scheduling and object allocation.

### Current Behavior
- `TMap::update()` creates QTimer lambda on every call
- Multiple update request paths
- No consolidation of rapid updates
- Potential for redundant repaints

### Performance Impact
- Lambda allocation overhead
- Redundant paint events
- Estimated 5-10% improvement

### Proposed Solution

1. **Use persistent dirty flags**:
   ```cpp
   bool mNeedsUpdate = false;
   QTimer* mUpdateTimer; // Reuse, don't create new
   
   void scheduleUpdate() {
       if (!mNeedsUpdate) {
           mNeedsUpdate = true;
           mUpdateTimer->start();
       }
   }
   ```

2. **Batch updates** at event loop boundaries

3. **Eliminate lambda creation** in TMap::update()

### Implementation Steps
1. Add dirty flag to TMap, TTextEdit
2. Replace lambda with slot/method
3. Consolidate update request paths
4. Test with high-frequency updates

### Files Affected
- `src/TMap.cpp` (update)
- `src/TTextEdit.cpp` (forceUpdate, needUpdate, showNewLines)
- `src/TConsole.cpp`

### Expected Improvement
5-10% reduction in update overhead

---

## Issue 7: Pre-allocate Container Capacity

**Title:** `improve: Reserve capacity for QVector/QString before bulk operations`

**Labels:** `enhancement`, `performance`, `memory`, `good first issue`

**Description:**

### Summary
Throughout the codebase, QVector and QString containers grow dynamically during loops without capacity reservation, causing repeated reallocations and memory copying.

### Current Behavior
- `QVector::append()` without reserve
- `QString::append()` without reserve
- Growth happens during hot loops
- Memory allocation churn

### Performance Impact
- Reallocation overhead
- Memory fragmentation
- Estimated 5-15% improvement in parsing code

### Proposed Solution

Add `.reserve()` calls before known-size operations:
```cpp
// Example in drawLine
QVector<QColor> fgColors;
QVector<QRect> textRects;
QVector<QString> graphemes;
QVector<int> charWidths;

// Reserve based on line length
const int estimatedSize = lineText.size() + 10;
fgColors.reserve(estimatedSize);
textRects.reserve(estimatedSize);
graphemes.reserve(estimatedSize);
charWidths.reserve(estimatedSize);
```

### Implementation Steps
1. Search for append patterns in hot paths
2. Add reserve calls where size is predictable
3. Profile before/after
4. Focus on:
   - TTextEdit::drawLine
   - TBuffer parsing functions
   - Container building in loops

### Files Affected
- `src/TTextEdit.cpp`
- `src/TBuffer.cpp`
- Various parsing functions

### Expected Improvement
5-15% reduction in allocation overhead

### Good First Issue
This is a good entry point for contributors:
- Clear pattern to follow
- Low risk changes
- Measurable impact
- Doesn't require deep architectural knowledge

---

## Priority Recommendations

**Quick Wins (High ROI):**
1. Issue #7 - Container pre-allocation (good first issue)
2. Issue #2 - SGR sequence optimization
3. Issue #3 - Buffer reservation

**High Impact:**
1. Issue #4 - 3D map spatial indexing (50%+ improvement)
2. Issue #1 - Text rendering cache (15-25% improvement)
3. Issue #3 - MUD data buffering (20% improvement)

**Architectural Improvements:**
1. Issue #5 - Timer system consolidation
2. Issue #6 - Update cascade optimization

## Testing Strategy

For each optimization:
1. Add performance benchmarks
2. Profile before/after with real MUD data
3. Test with various scenarios:
   - High ANSI color usage
   - Large 3D maps
   - Many active timers
   - High-volume text streams
4. Verify no regressions in functionality

## Profiling Tools

Recommended profiling approach:
```cpp
// Add to hot functions
QElapsedTimer perfTimer;
perfTimer.start();
// ... function code ...
qint64 elapsed = perfTimer.elapsed();
if (elapsed > 10) { // Log slow operations
    qDebug() << __func__ << "took" << elapsed << "ms";
}
```

Or use:
- Instruments (macOS)
- perf (Linux)
- Visual Studio Profiler (Windows)
- valgrind/callgrind

---

*Generated from CPU performance analysis of Mudlet codebase*
