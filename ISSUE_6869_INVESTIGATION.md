# Investigation Report: Issue #6869 - De-duplicate Logic for Loading Profiles

## Executive Summary

This investigation confirms the concerns raised in issue #6869. There is significant code duplication between profile loading and profile reset logic, and evidence shows that this has already led to maintenance issues where one code path was updated but the other was not.

## Issue Background

**Issue:** [#6869 - De-duplicate logic for loading profiles](https://github.com/Mudlet/Mudlet/issues/6869)
**Reported by:** vadi2
**Date:** May 27, 2023

The issue identifies code duplication in the logic for re-loading triggers, aliases, timers, and scripts between two methods:
- `mudlet::slot_connectionDialogueFinished()` (src/mudlet.cpp:4326-4416)
- `Host::resetProfile_phase2()` (src/Host.cpp:782-830)

## Code Analysis

### Method: `mudlet::slot_connectionDialogueFinished()`

**Location:** src/mudlet.cpp:4326-4416

**Purpose:** Initializes a newly loaded profile, setting up Lua environment, loading modules/packages, and optionally connecting to the game.

**Key operations:**
1. Sets `mIsProfileLoadingSequence = true`
2. Adds console for new host
3. Sets `mBlockScriptCompile = false`
4. Calls `pHost->mLuaInterpreter.loadGlobal()`
5. Calls `pHost->hideMudletsVariables()`
6. Loads modules in priority order (negative priority first, then rest after scripts)
7. Sets `mBlockStopWatchCreation = false`
8. Calls `pHost->getScriptUnit()->compileAll(true)`
9. Calls `pHost->updateAnsi16ColorsInTable()` ⚠️ **Missing extended ANSI colors**
10. Installs modules and packages
11. Loads map with `pHost->loadMap()`
12. Optionally connects to host via telnet
13. Raises `sysLoadEvent` with argument `1` (indicates fresh profile load)
14. Sets `mIsProfileLoadingSequence = false`

### Method: `Host::resetProfile_phase2()`

**Location:** src/Host.cpp:782-830

**Purpose:** Resets an existing profile by clearing temporary items and recompiling all scripts/triggers/aliases.

**Key operations:**
1. Removes all temp aliases, timers, triggers, keys
2. Removes all non-persistent stopwatches
3. Calls `doCleanup()` on all units (alias, timer, trigger, key)
4. Resets main console
5. Clears event handler and event maps
6. Calls `mLuaInterpreter.initLuaGlobals()`
7. Calls `mLuaInterpreter.loadGlobal()`
8. Sets `mBlockScriptCompile = false`
9. Re-enables all triggers in all units
10. Compiles all units: timer, trigger, alias, action, key, script
11. Sets `mResetProfile = false`
12. Calls `mLuaInterpreter.updateAnsi16ColorsInTable()`
13. Calls `mLuaInterpreter.updateExtendedAnsiColorsInTable()` ✅ **Has extended ANSI colors**
14. Raises `sysLoadEvent` with argument `0` (indicates profile reset)

## Evidence of Maintenance Issues

### Issue 1: Missing `updateExtendedAnsiColorsInTable()` in slot_connectionDialogueFinished

**Finding:** The `resetProfile_phase2()` method calls both:
- `mLuaInterpreter.updateAnsi16ColorsInTable()`
- `mLuaInterpreter.updateExtendedAnsiColorsInTable()`

However, `slot_connectionDialogueFinished()` only calls:
- `pHost->updateAnsi16ColorsInTable()`

**Impact:** This means that when a profile is first loaded, extended ANSI colors (256-color palette) may not be properly initialized in the Lua color_table, but they are correctly initialized during a profile reset. This is exactly the type of inconsistency that issue #6869 warned about.

**Code References:**
- resetProfile_phase2: src/Host.cpp:816-817
- slot_connectionDialogueFinished: src/mudlet.cpp:4365

## Common Logic Identified

Both methods share the following core operations:

1. **Lua Initialization:**
   - Both call `loadGlobal()` on the Lua interpreter
   - Both set `mBlockScriptCompile = false`

2. **Script Compilation:**
   - Both compile scripts (though resetProfile compiles more unit types)
   - slot_connectionDialogueFinished: Only compiles `ScriptUnit`
   - resetProfile_phase2: Compiles all units (Timer, Trigger, Alias, Action, Key, Script)

3. **Color Table Updates:**
   - Both update ANSI 16 colors in Lua table
   - Only resetProfile updates extended ANSI colors (inconsistency!)

4. **Event Notification:**
   - Both raise `sysLoadEvent` (with different boolean flags)

## Key Differences (By Design)

Some differences appear intentional based on the different contexts:

1. **Module/Package Loading:**
   - slot_connectionDialogueFinished: Loads modules and packages
   - resetProfile_phase2: Does NOT reload modules/packages (preserves them)

2. **Map Loading:**
   - slot_connectionDialogueFinished: Loads map from disk
   - resetProfile_phase2: Does NOT reload map (preserves it)

3. **Console Handling:**
   - slot_connectionDialogueFinished: Adds console for new host
   - resetProfile_phase2: Resets existing console

4. **Cleanup Operations:**
   - slot_connectionDialogueFinished: No cleanup (new profile)
   - resetProfile_phase2: Extensive cleanup of temp items and event maps

5. **Connection:**
   - slot_connectionDialogueFinished: Optionally connects to game
   - resetProfile_phase2: No connection logic

## SlySven's Concern

In the GitHub issue comments, SlySven expressed concern about potential complications, noting that some initialization code (like creating the `dlgTriggerEditor` class instance) may not be appropriate for the reset scenario.

**Analysis:** This is a valid concern. The methods serve different purposes:
- slot_connectionDialogueFinished: Full profile initialization from scratch
- resetProfile: Lightweight reset of existing profile

Any refactoring must preserve these semantic differences.

## Recommendations

### Option 1: Extract Common Core Logic (Recommended)

Create a new private helper method that contains the truly common logic:

```cpp
// In Host class
void Host::recompileScriptsAndUpdateLua(bool isFullLoad)
{
    mBlockScriptCompile = false;
    mLuaInterpreter.loadGlobal();

    // Compile all units
    getTimerUnit()->compileAll();
    getTriggerUnit()->compileAll();
    getAliasUnit()->compileAll();
    getActionUnit()->compileAll();
    getKeyUnit()->compileAll();
    getScriptUnit()->compileAll(true);

    // Update color tables - BOTH 16 and extended!
    mLuaInterpreter.updateAnsi16ColorsInTable();
    mLuaInterpreter.updateExtendedAnsiColorsInTable();

    // Raise load event
    TEvent event{};
    event.mArgumentList.append(QLatin1String("sysLoadEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(isFullLoad ? 1 : 0));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);
    raiseEvent(event);
}
```

This method would be called by both:
- `mudlet::slot_connectionDialogueFinished()` (after module/package loading)
- `Host::resetProfile_phase2()` (after cleanup operations)

**Benefits:**
- Ensures both code paths update extended ANSI colors
- Reduces duplication of compilation logic
- Makes future changes easier to maintain
- Preserves distinct initialization/cleanup logic

**Risks:**
- Requires careful testing to ensure no regressions
- Must maintain backward compatibility

### Option 2: Document and Fix Inconsistencies

Instead of major refactoring, simply fix the identified bugs and document the differences:

1. **Immediate Fix:** Add `pHost->updateExtendedAnsiColorsInTable()` to slot_connectionDialogueFinished
2. **Documentation:** Add comments explaining why the methods differ
3. **Testing:** Add tests to catch future divergences

**Benefits:**
- Lower risk
- Faster to implement
- Preserves current architecture

**Risks:**
- Duplication remains
- Future maintenance issues likely

### Option 3: Create Checklist/Template (Supplementary)

Regardless of refactoring, create a developer checklist for "Profile Initialization Operations" to help prevent future divergences.

## Immediate Action Required

**Bug Fix:** Add the missing `updateExtendedAnsiColorsInTable()` call to `slot_connectionDialogueFinished()`:

```cpp
// In mudlet::slot_connectionDialogueFinished(), after line 4365:
pHost->updateAnsi16ColorsInTable();
pHost->updateExtendedAnsiColorsInTable();  // ADD THIS LINE
```

This should be done regardless of which long-term option is chosen.

## Testing Considerations

Any refactoring or fixes should be tested for:
1. Fresh profile loads (both online and offline)
2. Profile resets
3. Module and package loading
4. Extended ANSI color availability in Lua scripts
5. Trigger/alias/timer compilation
6. Event handlers receiving sysLoadEvent correctly

## Conclusion

Issue #6869 is valid and has already manifested as a real bug (missing extended ANSI color initialization). The duplication creates maintenance burden and risk of further divergence. A careful refactoring to extract common logic while preserving necessary differences would improve code quality and reduce future maintenance issues.

---

**Investigation Date:** 2025-10-21
**Mudlet Branch:** claude/investigate-issue-6869-011CULazXdTRAGfho9kV4TBJ
**Base Commit:** 32332f6 Fix: MXP parser blocked on non-escaped '&' and '<' characters
