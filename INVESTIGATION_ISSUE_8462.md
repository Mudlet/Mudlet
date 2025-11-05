# Investigation Report: Issue #8462 - Excessive Lag Scrolling Through Triggers with Screen Reader

**Date:** 2025-11-05
**Investigator:** Claude
**Issue:** https://github.com/Mudlet/Mudlet/issues/8462
**Status:** Root cause identified, solutions recommended

---

## Executive Summary

Screen reader users experience approximately **1 second lag per item** when navigating the trigger editor using arrow keys. This investigation identified multiple root causes in the trigger editor's item selection handling code, with the primary bottlenecks being:

1. **Expensive document recreation** on every selection change
2. **Blocking accessibility updates** that wait for screen reader processing
3. **Lack of deferred loading** - content loads immediately on selection rather than activation

---

## Issue Details

### Problem Statement
- **Reporter:** ironcross32
- **Label:** a11y (accessibility)
- **Date Reported:** October 28, 2025
- **Severity:** High - Makes browsing large trigger lists impractical for screen reader users

### User Impact
- Sighted users click directly on desired triggers (instant)
- Screen reader users must navigate sequentially through all items (1 second lag × number of items)
- Loading operations trigger on every arrow key press
- User wants to keep Enter key activation functionality but reduce navigation lag

---

## Technical Investigation

### Architecture Overview

The trigger editor uses `QTreeWidget` to display hierarchical trigger/timer/alias structures. Navigation flow:

```
Arrow Key Press → QTreeWidget selection change
                ↓
          slot_treeSelectionChanged()
                ↓
          slot_xxxSelected() (trigger/timer/alias/etc.)
                ↓
          clearDocument() - Recreates editor [EXPENSIVE]
                ↓
          Load item content into editor [EXPENSIVE]
                ↓
          updatePackageItemAccessibility() [ACCESSIBILITY UPDATE]
                ↓
          QAccessible::updateAccessibility() [BLOCKS ON SCREEN READER]
```

### Root Causes Identified

#### 1. **clearDocument() Called on Every Selection** ⚠️ PRIMARY BOTTLENECK

**Location:** `src/dlgTriggerEditor.cpp:12017`

Called in every selection handler:
- `slot_triggerSelected`: lines 7268, 7409
- `slot_aliasSelected`: lines 7463, 7478
- `slot_keySelected`: lines 7532, 7546
- `slot_timerSelected`: lines 7619, 7629
- `slot_actionSelected`: lines 7708, 7722
- `slot_scriptsSelected`: lines 7951, 7960

**What it does:**
```cpp
void dlgTriggerEditor::clearDocument(edbee::TextEditorWidget* pEditorWidget,
                                     const QString& initialText)
{
    // Creates NEW edbee::CharTextDocument() every time
    auto document = new edbee::CharTextDocument();

    // Disconnects signals
    disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, ...);

    // Sets language grammar (Lua syntax highlighting)
    document->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->get("lua"));

    // Configures entire editor theme and settings
    // - Font configuration
    // - Line separator settings
    // - Bidirectional text settings
    // - Tab settings
    // - Theme colors
    // - Text options

    // Reconnects signals
    connect(document, &edbee::TextDocument::textChanged, ...);
}
```

**Impact:** Complete editor reconfiguration happens **every arrow key press**, even if just browsing.

---

#### 2. **Accessibility Updates Block on Screen Reader** ⚠️ SCREEN READER SPECIFIC

**Location:** `src/mudlet.cpp:6682`

```cpp
void mudlet::announce(const QString& text, const QString& processing, bool isPlain)
{
    QString textToAnnounce;
    if (isPlain) {
        textToAnnounce = text;
    } else {
        QTextDocument convertor;
        convertor.setHtml(text);
        textToAnnounce = convertor.toPlainText();  // HTML parsing overhead
    }

    QAccessibleAnnouncementEvent event(this, textToAnnounce);

    // Set politeness level
    if (processing == QLatin1String("importantall") ||
        processing == QLatin1String("importantmostrecent")) {
        event.setPoliteness(QAccessible::AnnouncementPoliteness::Assertive);
    } else {
        event.setPoliteness(QAccessible::AnnouncementPoliteness::Polite);
    }

    QAccessible::updateAccessibility(&event);  // ⚠️ BLOCKS WAITING FOR SCREEN READER
}
```

**Called from:** `src/dlgTriggerEditor.cpp` lines 7429, 7498, 7566, 7993, 8098, 8173

**Impact:** For package items, this announces on first selection, but:
- `QAccessible::updateAccessibility()` waits for screen reader to process
- Screen readers need time to recalculate accessibility tree
- Adds latency on top of clearDocument() overhead

---

#### 3. **Package Item Accessibility Updates on Every Selection**

**Location:** `src/dlgTriggerEditor.cpp:7407-7432` (and similar in other slot functions)

```cpp
QString packageName = pT->packageName(pT);
if (!packageName.isEmpty()) {
    QString currentDesc = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
    updatePackageItemAccessibility(pItem, currentDesc);  // Updates every time
    showWarning(tr("This item is part of a package..."), false);

    static bool firstPackageAnnounced = false;
    if (!firstPackageAnnounced) {
        mudlet::self()->announce(tr("Package item. Copy before editing..."));
        firstPackageAnnounced = true;  // Only announces once
    }
}
```

**Problem:** While `announce()` is guarded by `firstPackageAnnounced`, the `updatePackageItemAccessibility()` call still:
- Updates `Qt::AccessibleDescriptionRole` on every selection
- May trigger screen reader re-evaluation even without announce

---

#### 4. **Additional Trigger-Specific Overhead**

For triggers specifically, additional expensive operations:

**showPatternItems()** - `src/dlgTriggerEditor.cpp:1398`
- Shows/hides up to 50 pattern widgets
- Calls `updatePatternPlaceholders()`, `updatePatternTabOrder()`, `updatePatternNavigationHint()`
- Adjusts visibility on multiple UI elements

Called at line 7288 in `slot_triggerSelected()`

---

### Performance Hotspots Summary

| Operation | File:Line | Impact | Frequency |
|-----------|-----------|--------|-----------|
| `clearDocument()` | dlgTriggerEditor.cpp:12017 | ⚠️⚠️⚠️ HIGH | Every selection |
| `QAccessible::updateAccessibility()` | mudlet.cpp:6702 | ⚠️⚠️ MEDIUM-HIGH | First package item |
| `updatePackageItemAccessibility()` | dlgTriggerEditor.cpp:378 | ⚠️ MEDIUM | Every package item |
| `showPatternItems()` | dlgTriggerEditor.cpp:1398 | ⚠️ MEDIUM | Every trigger |

**Cumulative Effect:** 1000+ milliseconds per navigation with screen reader active

---

## External Research Findings

### Qt Accessibility Performance Issues

#### 1. **QAccessible::setActive() Known Issue**
- **Source:** Qt Forum thread (Qt 5.15 / Windows 10)
- **Problem:** User reported 10-second hangs with ~1500 accessibility activation observers
- **Root Cause:** 1500 ToolTips registering with QAccessible (PopUps treated as Windows)
- **Lesson:** Large numbers of UI elements can kill `setActive()` performance

#### 2. **Qt 5.11+ UI Automation Backend**
- **Source:** Qt Blog, NVDA GitHub issues
- **Change:** Qt 5.11 replaced MSAA with Microsoft UI Automation
- **Impact:** New backend has different performance characteristics with NVDA/JAWS
- **Known Issues:** Some NVDA compatibility problems, though many resolved in recent versions

#### 3. **QTreeWidget Accessibility Issues**
- **Source:** NVDA GitHub issue #12476
- **Problem:** NVDA doesn't read QTreeView on tab focus
- **Cause:** Qt doesn't send gainFocus event consistently for tree views
- **Related:** Focus change notifications sometimes ignored by NVDA

### Qt Best Practices for Accessibility Performance

#### 1. **Use QAccessible::isActive() for Optimization**
**Source:** Qt Documentation (Qt 5.15, Qt 6.x)

```cpp
if (QAccessible::isActive()) {
    // Only do expensive accessibility work when screen reader is active
    QAccessible::updateAccessibility(&event);
}
```

**Qt Docs Quote:**
> "If determining the parameters of the call is expensive you can test
> QAccessible::isActive() to avoid unnecessary computation."

**Caveat:** `isActive()` returns true once a screen reader connects and doesn't go back to false when closed - use for "has ever been active" checks.

#### 2. **Defer Content Loading with itemActivated Signal**
**Source:** Qt Documentation, Qt Forum discussions

**Key Distinction:**
- `itemSelectionChanged()` - Fires on arrow key navigation
- `itemActivated()` - Fires only on Enter/Return key or double-click

**Recommendation:** Use `itemActivated()` for expensive content loading, reserve `itemSelectionChanged()` for lightweight UI updates.

#### 3. **Debounce Rapid Navigation**
**Source:** Qt Forum thread on fast key presses

Delay content loading until key presses settle down during rapid navigation.

---

## Recommended Solutions

### Solution 1: Defer Content Loading Until Activation (RECOMMENDED)

**Approach:** Implement two-phase loading
- **Phase 1 (Selection):** Minimal UI updates only
- **Phase 2 (Activation):** Full content loading with clearDocument()

**Implementation Strategy:**

```cpp
// In constructor, connect to both signals
connect(treeWidget_triggers, &QTreeWidget::currentItemChanged,
        this, &dlgTriggerEditor::slot_triggerSelectionChanged);
connect(treeWidget_triggers, &QTreeWidget::itemActivated,
        this, &dlgTriggerEditor::slot_triggerActivated);

// Lightweight selection handler - no clearDocument()
void dlgTriggerEditor::slot_triggerSelectionChanged(QTreeWidgetItem* pItem)
{
    if (!pItem) return;

    // Store current selection
    mpCurrentTriggerItem = pItem;

    // Show minimal preview or summary info (optional)
    // updateTriggerPreview(pItem);

    // Update accessibility with summary only
    if (QAccessible::isActive()) {
        // Lightweight accessibility update
    }
}

// Full loading only on activation (Enter key or double-click)
void dlgTriggerEditor::slot_triggerActivated(QTreeWidgetItem* pItem)
{
    if (!pItem) return;

    // Now do the expensive operations
    clearDocument(mpSourceEditorEdbee);
    // ... full content loading ...
}
```

**Benefits:**
- ✅ Browsing with arrow keys becomes instant
- ✅ Enter key still activates/deactivates (preserves user workflow)
- ✅ Only loads content when user commits to editing
- ✅ Screen reader announces selection without content load delay

**User Experience:**
- Arrow keys: Browse triggers instantly, hear trigger names
- Enter key: Load trigger content for editing, toggle enabled/disabled
- Esc key (optional): Return to browse mode without loading

**Estimated Improvement:** 1000ms → <50ms per arrow key press

---

### Solution 2: Guard Expensive Operations with QAccessible::isActive()

**Approach:** Only perform expensive accessibility work when screen reader is active

```cpp
void mudlet::announce(const QString& text, const QString& processing, bool isPlain)
{
    // Early return if no screen reader active
    if (!QAccessible::isActive()) {
        return;
    }

    // ... existing implementation ...
    QAccessible::updateAccessibility(&event);
}
```

**Apply to:**
- `mudlet::announce()` - src/mudlet.cpp:6682
- `updatePackageItemAccessibility()` calls
- Any accessibility-specific UI updates

**Benefits:**
- ✅ No performance impact for non-screen-reader users
- ✅ Simple to implement (add single if-check)
- ✅ Can be combined with Solution 1

**Limitation:** Still has lag for screen reader users - this is a mitigation, not a fix

---

### Solution 3: Cache Editor State to Avoid Recreating

**Approach:** Reuse editor instances instead of recreating on every selection

**Current Problem:**
```cpp
void clearDocument(...) {
    auto document = new edbee::CharTextDocument();  // NEW every time
    // ... reconfigure everything ...
}
```

**Proposed:**
```cpp
// Store last-configured editor state
struct EditorCache {
    int itemId;
    edbee::TextDocument* document;
    QString content;
};
QMap<EditorViewType, EditorCache> mEditorCache;

void loadDocumentCached(...) {
    auto& cache = mEditorCache[mCurrentView];
    if (cache.itemId == currentItemId) {
        // Reuse existing document
        pEditorWidget->setTextDocument(cache.document);
        return;
    }
    // Otherwise, create new
    clearDocument(pEditorWidget, content);
    cache.itemId = currentItemId;
    cache.document = pEditorWidget->textDocument();
}
```

**Benefits:**
- ✅ Reduces editor reconfiguration overhead
- ✅ Preserves undo history when returning to items
- ✅ Natural LRU cache (most recently edited items stay fast)

**Challenges:**
- ⚠️ Memory overhead (multiple document instances)
- ⚠️ Need to invalidate cache when item content changes externally
- ⚠️ More complex implementation

---

### Solution 4: Debounce Selection Updates During Rapid Navigation

**Approach:** Delay expensive operations until navigation settles

```cpp
QTimer* mSelectionDebounceTimer;

void dlgTriggerEditor::slot_triggerSelected(QTreeWidgetItem* pItem)
{
    // Cancel any pending load
    mSelectionDebounceTimer->stop();

    // Store selection
    mpCurrentTriggerItem = pItem;

    // Delay expensive loading
    mSelectionDebounceTimer->start(150);  // 150ms delay
}

void dlgTriggerEditor::slot_debouncedLoad()
{
    // Now do expensive clearDocument() and content load
    if (mpCurrentTriggerItem) {
        loadTriggerContentExpensive(mpCurrentTriggerItem);
    }
}
```

**Benefits:**
- ✅ Fast arrow key navigation (loads only after pause)
- ✅ No workflow change required
- ✅ Works with existing signal connections

**Limitations:**
- ⚠️ Still delays of 150ms + load time after navigation stops
- ⚠️ Doesn't fully solve screen reader lag
- ⚠️ Can feel "laggy" if delay too long, "choppy" if too short

---

## Recommended Implementation Plan

### Phase 1: Quick Wins (Immediate)

1. **Add QAccessible::isActive() guards** (Solution 2)
   - File: `src/mudlet.cpp:6682` - `announce()` function
   - File: `src/dlgTriggerEditor.cpp` - `updatePackageItemAccessibility()` calls
   - **Effort:** 1-2 hours
   - **Impact:** No performance penalty for non-screen-reader users

### Phase 2: Defer Content Loading (Medium-term)

2. **Implement itemActivated-based loading** (Solution 1)
   - Refactor selection handlers to split into lightweight/heavyweight paths
   - Connect `itemActivated()` signal for all tree widgets
   - Move `clearDocument()` calls to activation handlers
   - **Effort:** 4-8 hours (all 6 editor types)
   - **Impact:** 95%+ reduction in screen reader navigation lag

3. **Add visual/auditory feedback** for browse vs edit modes
   - Optional status bar indicator: "Browsing triggers (press Enter to edit)"
   - Accessibility announcement: "Selected [trigger name]. Press Enter to edit."
   - **Effort:** 2 hours
   - **Impact:** Better UX clarity

### Phase 3: Advanced Optimizations (Optional)

4. **Implement editor state caching** (Solution 3)
   - Cache recently edited documents
   - LRU eviction policy (e.g., keep 10 most recent)
   - **Effort:** 8-16 hours
   - **Impact:** Further speedup for returning to recently edited items

5. **Debouncing as fallback** (Solution 4)
   - For users who prefer auto-load behavior
   - Add preference setting: "Auto-load item content after [150]ms"
   - **Effort:** 4 hours
   - **Impact:** Provides compromise between instant load and deferred load

---

## Testing Recommendations

### Test Cases

#### TC1: Screen Reader Navigation Performance
- **Setup:** NVDA or JAWS active, trigger list with 100+ items
- **Test:** Use arrow keys to navigate through 20 triggers
- **Measure:** Time per navigation (should be <100ms)
- **Expected:** No perceptible lag with Solutions 1+2 implemented

#### TC2: Enter Key Activation
- **Setup:** Screen reader active
- **Test:** Navigate to trigger, press Enter
- **Verify:**
  - Content loads in editor
  - Changes can be made
  - Press Enter again toggles enabled/disabled state
- **Expected:** Same workflow as before, just on Enter instead of arrow keys

#### TC3: Non-Screen Reader Performance
- **Setup:** No screen reader active
- **Test:** Navigate and edit triggers
- **Verify:** No performance regression
- **Expected:** Same or better performance (isActive() guards prevent unused work)

#### TC4: Accessibility Announcements
- **Setup:** Screen reader active
- **Test:** Navigate to package item
- **Verify:** Announcement plays once, not repeatedly
- **Expected:** "Package item. Copy before editing to preserve changes" announced first time only

### Automated Testing

Create integration tests in `test/` directory:

```cpp
// Test that clearDocument is NOT called on selection change
void TestTriggerEditor::testSelectionWithoutLoad()
{
    int clearDocumentCallCount = 0;
    // Mock or spy on clearDocument

    // Simulate arrow key navigation
    triggerEditor->treeWidget_triggers->setCurrentItem(item1);
    QCOMPARE(clearDocumentCallCount, 0);  // Should not be called

    // Simulate activation
    emit triggerEditor->treeWidget_triggers->itemActivated(item1);
    QCOMPARE(clearDocumentCallCount, 1);  // Now should be called
}
```

---

## Related Issues and Files

### Files Modified
- `src/dlgTriggerEditor.cpp` - Main implementation file
  - Lines 7251-8004: All slot_xxxSelected() functions
  - Line 12017: clearDocument() function
- `src/dlgTriggerEditor.h` - Header file
  - Lines 271-276: Slot function declarations
  - Line 378: updatePackageItemAccessibility declaration
- `src/mudlet.cpp` - Accessibility announcements
  - Lines 6682-6703: announce() function
- `src/TTreeWidget.h` / `src/TTreeWidget.cpp` - Tree widget implementation

### Related Qt Classes
- `QTreeWidget` - Tree widget base
- `QAccessible` - Accessibility framework
- `QAccessibleAnnouncementEvent` - Screen reader announcements
- `edbee::TextEditorWidget` - Code editor widget

### Similar Issues in Qt Ecosystem
- NVDA Issue #12476: QTreeView focus issues
- NVDA Issue #8604: Qt 5.11+ UI Automation compatibility
- Qt Forum: QAccessible::setActive() performance with many widgets

---

## Conclusion

Issue #8462 stems from **architectural assumptions** that don't match screen reader user workflows:

1. **Assumption:** Selection = Intent to edit → Load content immediately
2. **Reality:** Screen reader users select many items while browsing → Only edit occasionally

The **primary fix** (Solution 1) aligns the architecture with screen reader usage patterns by deferring expensive operations until explicit activation. This is the recommended approach used by many accessible applications.

**Recommended Priority:**
- ✅ **MUST FIX:** Implement Solution 1 (defer loading) + Solution 2 (isActive guards)
- ⚙️ **SHOULD FIX:** Add user preference for auto-load delay (Solution 4 as option)
- 💡 **NICE TO HAVE:** Implement Solution 3 (caching) for power users with many triggers

**Estimated Total Effort:** 8-12 hours development + 4 hours testing

**Expected User Impact:** Navigation lag reduced from **1000ms → <50ms** per item (95%+ improvement)

---

## References

### External Resources
- Qt Accessibility Documentation: https://doc.qt.io/qt-6/accessible.html
- Qt Forum Thread: QAccessible::setActive performance issues
- NVDA GitHub: https://github.com/nvaccess/nvda/issues/12476
- Qt Blog: Qt 5.11 UI Automation backend changes

### Internal Resources
- Issue #8462: https://github.com/Mudlet/Mudlet/issues/8462
- CLAUDE.md: Project coding standards and architecture
- src/TAccessibleTextEdit.cpp: Existing accessibility implementation (already has performance notes)

### Performance Notes in Codebase
The codebase already has performance awareness in accessibility code:
- `src/TAccessibleTextEdit.cpp:52`: "performance note - this is called extremely frequently"
- `src/TAccessibleTextEdit.cpp:237`: "should be cached"
- Similar notes on lines 84, 252, 267

This investigation extends that performance awareness to the trigger editor UI navigation.

---

**Investigation Status:** ✅ Complete
**Next Steps:** Present findings to maintainers, discuss solution preferences, begin implementation
