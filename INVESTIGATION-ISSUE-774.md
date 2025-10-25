# Investigation Report: Issue #774 - Event Handling Code Quality

**Date:** 2025-10-25
**Issue:** https://github.com/Mudlet/Mudlet/issues/774
**Branch:** claude/investigate-issue-774-011CUUSx5mk2o87uHFSjWLww

## Executive Summary

This investigation examined Qt event handling across multiple Mudlet classes to identify missing or inconsistent use of `accept()` and `ignore()` methods. According to Qt documentation, event handlers should explicitly signal whether they handled an event using these methods to control event propagation.

**Key Finding:** The issue is **partially resolved**. TLabel has excellent event handling. T2DMap and TConsole need improvements. TCommandLine has mostly good handling with some inconsistencies.

## Background

Qt's event system relies on `accept()` and `ignore()` to control whether events propagate up the widget hierarchy. While Qt defaults `isAccepted()` to true, the documentation warns that "subclasses may choose to clear it in their constructor," making explicit handling necessary.

## Detailed Findings by Class

### 1. T2DMap Class (src/T2DMap.cpp)

**Status:** ⚠️ NEEDS IMPROVEMENT

#### Issues Found:

**mouseDoubleClickEvent** (Line 2706)
- ❌ No `accept()` or `ignore()` calls
- ❌ No delegation to parent
- Returns early on conditions but doesn't signal event attitude
- **Recommendation:** Add `event->accept()` after handling, or call `QWidget::mouseDoubleClickEvent(event)` if not handling

**mouseReleaseEvent** (Line 2821)
- ❌ No `accept()` or `ignore()` calls
- Dispatches to interaction handler but doesn't signal result
- **Recommendation:** Consider calling `event->accept()` after successful dispatch or delegating to parent

**mousePressEvent** (Line 2861)
- ❌ No `accept()` or `ignore()` calls
- Dispatches to interaction handler but doesn't signal result
- **Recommendation:** Add `event->accept()` after handling

**mouseMoveEvent** (Line 4118)
- ❌ No `accept()` or `ignore()` calls
- Dispatches to interaction handler but doesn't signal result
- **Recommendation:** Add `event->accept()` after handling

**event** (Line 2831)
- ✅ Returns `QWidget::event(event)` which is correct
- Handles Resize events internally
- **Status:** ACCEPTABLE - delegates properly to parent

### 2. TCommandLine Class (src/TCommandLine.cpp)

**Status:** ⚠️ MOSTLY GOOD with minor issues

#### Issues Found:

**event** (Line 163)
- ✅ Uses `event->accept()` for Copy operation (line 186)
- ❌ Returns `false` in error case without calling `ignore()`
- ✅ Returns `QPlainTextEdit::event(event)` for delegation
- **Recommendation:** Call `event->ignore()` before returning `false` in line 179

**focusInEvent** (Line 603)
- ❌ No `accept()` or `ignore()` calls
- ✅ Calls `QPlainTextEdit::focusInEvent(event)` at end
- **Status:** ACCEPTABLE - delegates to parent

**focusOutEvent** (Line 622)
- ❌ No `accept()` or `ignore()` calls
- ✅ Calls `QPlainTextEdit::focusOutEvent(event)` at end
- **Status:** ACCEPTABLE - delegates to parent

**mousePressEvent** (Line 920)
- ✅ Uses `event->ignore()` for echo suppression case (line 925)
- ✅ Uses `event->accept()` for right-click menu (line 955)
- ✅ Calls `QPlainTextEdit::mousePressEvent(event)` for other cases
- **Status:** EXCELLENT - proper use of event handling

**mouseReleaseEvent** (Line 964)
- ❌ No explicit `accept()` or `ignore()`
- ✅ Calls `QPlainTextEdit::mouseReleaseEvent(event)`
- **Status:** ACCEPTABLE - delegates to parent

### 3. TConsole Class (src/TConsole.cpp)

**Status:** ⚠️ NEEDS IMPROVEMENT

#### Issues Found:

**resizeEvent** (Line 626)
- ❌ No `accept()` or `ignore()` calls
- ❌ Does NOT call `QWidget::resizeEvent(event)`
- Returns early in special case without signaling
- **Recommendation:** Call `QWidget::resizeEvent(event)` at the end, or explicitly accept/ignore

**showEvent** (Line 1079)
- ❌ No explicit `accept()` or `ignore()`
- ✅ Calls `QWidget::showEvent(event)` at end
- **Status:** ACCEPTABLE - delegates to parent

**hideEvent** (Line 1089)
- ❌ No explicit `accept()` or `ignore()`
- ✅ Calls `QWidget::hideEvent(event)` at end
- **Status:** ACCEPTABLE - delegates to parent

### 4. TLabel Class (src/TLabel.cpp)

**Status:** ✅ EXCELLENT - No changes needed

#### Findings:

**mousePressEvent** (Line 92)
- ✅ Calls `event->accept()` when handling (line 99)
- ✅ Calls `QWidget::mousePressEvent(event)` when not handling
- **Status:** EXCELLENT

**leaveEvent** (Line 153)
- ✅ Calls `event->accept()` when handling (line 157)
- ✅ Calls `QWidget::leaveEvent(event)` when not handling
- **Status:** EXCELLENT

**enterEvent** (Line 163)
- ✅ Calls `event->accept()` when handling (line 167)
- ✅ Calls `QWidget::enterEvent(event)` when not handling
- **Status:** EXCELLENT

**Additional handlers found:**
- mouseDoubleClickEvent (line 106): ✅ Properly accepts or delegates
- mouseReleaseEvent (line 116): ✅ Proper implementation
- resizeEvent (line 173): ✅ Calls QWidget::resizeEvent(event)

## Recommendations

### Priority 1: T2DMap Mouse Event Handlers

The T2DMap class has the most issues. All mouse event handlers should either:
1. Call `event->accept()` after successfully handling the event, OR
2. Call `event->ignore()` before returning if not handling, OR
3. Delegate to the parent class handler

**Specific actions:**
- Add `event->accept()` to `mouseDoubleClickEvent` after setting variables
- Add `event->accept()` to `mousePressEvent` after dispatch
- Add `event->accept()` to `mouseReleaseEvent` after dispatch
- Add `event->accept()` to `mouseMoveEvent` after dispatch

### Priority 2: TConsole::resizeEvent

The `resizeEvent` handler should call `QWidget::resizeEvent(event)` to ensure proper event chain handling, even if it appears to work without it.

### Priority 3: TCommandLine::event Error Handling

In the error path (line 179), add `event->ignore()` before returning `false` to explicitly signal the event wasn't handled.

## Code Examples

### Example Fix for T2DMap::mouseDoubleClickEvent

**Before:**
```cpp
void T2DMap::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!mpMap||!mpMap->mpRoomDB) {
        // No map loaded!
        return;
    }
    if (mDialogLock || (event->buttons() != Qt::LeftButton)) {
        return;
    }

    mPHighlight = event->pos();
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
}
```

**After:**
```cpp
void T2DMap::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!mpMap||!mpMap->mpRoomDB) {
        // No map loaded!
        event->ignore();
        return;
    }
    if (mDialogLock || (event->buttons() != Qt::LeftButton)) {
        event->ignore();
        return;
    }

    mPHighlight = event->pos();
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
    event->accept();
}
```

### Example Fix for TConsole::resizeEvent

**Before:**
```cpp
void TConsole::resizeEvent(QResizeEvent* event)
{
    if (mType & MainConsole) {
        mBorders = mpHost->borders();
    }
    int x = event->size().width();
    int y = event->size().height();

    if (mType == MainConsole && !x) {
        return;
    }
    // ... rest of implementation
}
```

**After:**
```cpp
void TConsole::resizeEvent(QResizeEvent* event)
{
    if (mType & MainConsole) {
        mBorders = mpHost->borders();
    }
    int x = event->size().width();
    int y = event->size().height();

    if (mType == MainConsole && !x) {
        QWidget::resizeEvent(event);
        return;
    }
    // ... rest of implementation

    QWidget::resizeEvent(event);
}
```

## Impact Assessment

**Severity:** Low to Medium

These issues are unlikely to cause visible bugs in most scenarios due to Qt's default behavior. However:

1. **Event propagation issues:** Events may propagate when they shouldn't, causing unexpected parent widget behavior
2. **Code clarity:** Explicit event handling makes intent clearer and prevents future bugs
3. **Qt guidelines:** Following Qt best practices ensures compatibility with Qt updates

## Testing Recommendations

After fixes are applied:

1. **Mouse interaction testing:** Verify map interaction, label clicks, and command line behavior
2. **Focus testing:** Ensure focus handling still works correctly in command line
3. **Resize testing:** Test console and window resizing behavior
4. **Event propagation:** Verify events don't propagate incorrectly to parent widgets

## Conclusion

Issue #774 identifies real code quality concerns. While TLabel demonstrates excellent event handling practices, T2DMap and TConsole have multiple event handlers that could benefit from explicit `accept()`/`ignore()` calls. TCommandLine is mostly well-implemented with minor improvements possible.

The suggested fixes are straightforward and low-risk, following the excellent example set by TLabel's implementation. Implementing these changes will improve code clarity, prevent potential future bugs, and align with Qt best practices.

---
**Investigation completed by:** Claude Code
**Tools used:** Code analysis, pattern matching, Qt documentation review
