# Investigation Summary: Issue #7313 - Windows Updater Deletes Mudlet

**Issue:** https://github.com/Mudlet/Mudlet/issues/7313

**Investigated:** 2025-10-19

## Problem Analysis

### Root Causes Identified

#### 1. Squirrel.Windows Framework Issues

- **Abandoned project**: Last release in 2020, last commit to master branch in 2022, no active development
- **File locking bugs**: Intermittent "Access denied" errors on `new-mudlet-setup-seen.exe`
- **Double-run vulnerability**: When installer runs twice, it deletes the application (https://github.com/Squirrel/Squirrel.Windows/issues/1717)
- **Shared temp directory pollution**: All Squirrel apps share `%LOCALAPPDATA%\SquirrelTemp`, causing cross-contamination
  - See related issue #6044 where installing MS Teams/Discord launches Mudlet instead
  - MS Teams installer picks up Mudlet's `.nupkg` files and installs to `Microsoft\Mudlet` folder
  - Workaround: Delete `%LOCALAPPDATA%\SquirrelTemp` folder

#### 2. Current Implementation Gaps

**File:** `src/updater.cpp:227-241` (prepareSetupOnWindows)
- No file lock verification before marking update available
- No retry logic when file operations fail
- Moves installer from temp to app directory without checking if accessible

**File:** `src/main.cpp:759-780` (runUpdate)
- Launches installer immediately without validation
- No check if file is locked before attempting to move/execute
- Basic error logging but no recovery mechanism

#### 3. Architectural Problem with Squirrel

- Squirrel changes app path each version (`app-4.18.5` → `app-4.19.0`)
- This breaks:
  - Windows firewall rules
  - Antivirus whitelisting
  - GPU preferences
  - Pinned taskbar shortcuts
  - Other Windows app integrations
- Contributes to update instability

### Symptoms Reported

1. **Update flow fails intermittently (4 in 41 attempts in testing)**
   - Error: `Access to the path 'C:\Users\...\AppData\Local\Mudlet\app-4.18.1\new-mudlet-setup-seen.exe' is denied`
   - Squirrel log shows: `Failed to remove existing directory on full install, is the app still running???`

2. **Mudlet installation disappears after update**
   - User clicks update → restart
   - Mudlet no longer launches
   - Windows asks if they want to delete the shortcut
   - All Mudlet data preserved, only executable deleted

3. **Missing Lua modules after update**
   - Second launch after failed update shows missing lfs, lua-zip, rex_pcre, sqlite3, yajl modules
   - Indicates incomplete/corrupted installation

4. **Affects both Windows 10 and Windows 11**
   - More commonly reported on Windows 11
   - Not consistent - sometimes works, sometimes fails

## Proposed Solutions

### SOLUTION 1: Migrate to Velopack ⭐ RECOMMENDED

**Why Velopack:**
- Active development (successor to Squirrel.Windows → Clowd.Squirrel → Velopack)
- Fixes file locking issues
- **Stable app path**: `{root}\current\YourApp.exe` instead of versioned folders like `app-4.19.0`
- Native C++ support with Qt compatibility (developer asked for Qt guidance)
- **Automatic migration** from Squirrel.Windows - first Velopack update automatically migrates existing installations
- Already requested by Mudlet community in issue #6044
- Free and open source (MIT license)
- Cross-platform (Windows, macOS, Linux)

**Resources:**
- GitHub: https://github.com/velopack/velopack
- Docs: https://docs.velopack.io/
- C++ Guide: https://docs.velopack.io/getting-started/cpp
- Migration Guide: https://docs.velopack.io/migrating/squirrel

**Implementation Steps:**

1. **Install Velopack CLI tool (`vpk`)**
   - Download from releases
   - Add to CI build environment

2. **Replace Clowd.Squirrel in build script**
   - File: `CI/deploy-mudlet-for-windows.sh:217-278`
   - Replace `nuget install Clowd.Squirrel` with Velopack installation
   - Update `pack` command to Velopack syntax

3. **Integrate Velopack C++ library**
   - Download `velopack_libc_{version}.zip` from GitHub releases
   - Add include directory to CMakeLists.txt
   - Link appropriate binary from lib directory
   - Add to vcpkg or as 3rdparty dependency

4. **Update application code**
   - File: `src/main.cpp:759-780`
   - Replace `runUpdate()` logic with Velopack update check
   - Use Velopack's C++ API instead of manual file operations

   ```cpp
   // Example Velopack integration (pseudocode)
   #include "velopack.hpp"

   // At app startup
   VelopackApp::build().run();

   // For updates
   auto updateManager = Velopack::UpdateManager::build();
   auto updateInfo = updateManager->checkForUpdates();
   if (updateInfo) {
       updateManager->downloadUpdates(updateInfo);
       updateManager->applyUpdatesAndRestart(updateInfo);
   }
   ```

5. **Update updater.cpp**
   - File: `src/updater.cpp`
   - Remove Windows-specific Squirrel handling (lines 181-242)
   - Integrate Velopack update mechanism
   - Keep dblsqd feed integration for update notifications

6. **Test migration path**
   - Test Squirrel → Velopack upgrade on existing installations
   - Verify automatic migration works
   - Ensure shortcuts are preserved
   - Confirm no data loss

7. **Update CI/CD pipeline**
   - Modify GitHub Actions workflow
   - Test PTB builds first
   - Roll out to release channel

**Effort:** Medium (2-3 weeks)
**Risk:** Low (automatic migration, well-documented)
**Impact:** High - solves root cause permanently

---

### SOLUTION 2: Add File Lock Detection (Short-term fix)

**Implementation in src/updater.cpp and src/main.cpp:**

1. **Add Windows API file lock checking**
   - Before marking update available
   - Before attempting to move installer file
   - Before launching installer

2. **Implement retry logic**
   - 3 attempts with exponential backoff
   - Wait 5s, 15s, 30s between attempts
   - Log each attempt

3. **Better error reporting**
   - Toast notification if file remains locked
   - Suggest closing other applications
   - Provide manual download link

4. **File accessibility validation**
   ```cpp
   // Pseudocode
   bool isFileAccessible(const QString& filePath) {
       // Try opening file with exclusive write access
       HANDLE hFile = CreateFileW(
           filePath.toStdWString().c_str(),
           GENERIC_WRITE,
           0, // No sharing
           NULL,
           OPEN_EXISTING,
           FILE_ATTRIBUTE_NORMAL,
           NULL
       );

       if (hFile == INVALID_HANDLE_VALUE) {
           DWORD error = GetLastError();
           if (error == ERROR_SHARING_VIOLATION) {
               return false; // File is locked
           }
       } else {
           CloseHandle(hFile);
       }
       return true;
   }
   ```

**Code locations to modify:**
- `src/updater.cpp:prepareSetupOnWindows()` (line 227)
- `src/updater.cpp:finishSetup()` (line 160)
- `src/main.cpp:runUpdate()` (line 759)

**Effort:** Small (3-5 days)
**Risk:** Low
**Impact:** Medium - reduces frequency but doesn't fix root cause

---

### SOLUTION 3: Improve Error Handling & User Experience

**Enhancements:**

1. **User-facing error messages**
   - Clear error dialog when update fails
   - Link to troubleshooting guide
   - Option to download manually

2. **Automatic cleanup on startup**
   - Check for stale installer files
   - Remove old `new-mudlet-setup*.exe` files
   - Clean up corrupted update attempts

3. **Manual download option**
   - Add "Download only" button in update dialog
   - Save installer to Downloads folder
   - Let user run when ready

4. **Better diagnostics**
   - Log file lock errors with details
   - Log Squirrel output more verbosely
   - Include in bug reports

5. **Documentation**
   - Create troubleshooting wiki page
   - Document common errors
   - Provide workarounds

**Effort:** Small (1 week)
**Risk:** Very Low
**Impact:** Low - improves UX but doesn't prevent issues

---

### SOLUTION 4: Clean up nupkg files (Complementary fix)

**Problem:** Mudlet leaves `.nupkg` files in `%LOCALAPPDATA%\SquirrelTemp` after updates

**Investigation findings (from issue #6044):**
- Initial installer does NOT leave `.nupkg` files
- In-app update process DOES leave `.nupkg` files
- These files cause MS Teams installer to malfunction
- One user had 107 `.nupkg` files totaling 4.8GB

**Fix:**
1. After successful update, clean up temp files
2. On app startup, remove old Mudlet `.nupkg` files from SquirrelTemp
3. Add cleanup to `finishSetup()` in updater.cpp

**Effort:** Very Small (1-2 days)
**Risk:** Very Low
**Impact:** Medium - fixes cross-app contamination issue

## Recommendation

**Primary Strategy:**
1. **Immediate:** Implement Solution 2 (file lock detection) as a hotfix
2. **Short-term:** Implement Solution 4 (cleanup nupkg files)
3. **Long-term:** Implement Solution 1 (Velopack migration)

**Rationale:**
- Velopack addresses root causes of both issue #7313 AND #6044
- File lock detection provides immediate relief while migration is underway
- Nupkg cleanup prevents cross-app contamination
- Velopack is actively maintained vs abandoned Squirrel.Windows
- Stable app path in Velopack solves many Windows integration issues
- Automatic migration path makes rollout safe

## Technical Details

### Current Update Flow (Windows)

1. **Update available** → dblsqd feed notifies Mudlet
2. **Download** → Save to temp: `%TEMP%/mudlet-setup-*.exe`
3. **Prepare** → Move to: `%TEMP%/new-mudlet-setup.exe`
4. **On restart** → runUpdate() checks for `new-mudlet-setup.exe`
5. **Execute** → Move to `{app-dir}/new-mudlet-setup-seen.exe`
6. **Launch** → Start installer and quit Mudlet
7. **Installer runs** → Updates to new `app-x.y.z` folder
8. **Cleanup** → Should delete old installer (but fails sometimes)

### Failure Points

- **Step 5**: File lock causes move to fail
- **Step 6**: If launched twice, deletes installation
- **Step 8**: Incomplete cleanup leaves `.nupkg` files

### Related Issues

- **#6044**: Installing Discord/MS Teams launches Mudlet (SquirrelTemp pollution)
- **#4382**: Related to same Squirrel cross-contamination
- **Squirrel.Windows #1582**: Upstream issue about temp directory sharing
- **Squirrel.Windows #1717**: Double-run deletion bug
- **Squirrel.Windows #684**: Fix merged to develop but never released

## Testing Plan

### For Velopack Migration

1. **Unit tests**
   - Test update check mechanism
   - Test download functionality
   - Test migration from Squirrel installation

2. **Integration tests**
   - Fresh install → update cycle
   - Squirrel 4.19.0 → Velopack update
   - Multiple consecutive updates

3. **Platform testing**
   - Windows 10 (21H2, 22H2)
   - Windows 11 (21H2, 22H2, 23H2)
   - Both x64 architecture

4. **Scenario testing**
   - Update while antivirus active
   - Update with firewall enabled
   - Update from PTB to release
   - Update with profiles open

### Success Criteria

- 100% success rate on updates (vs current ~90%)
- No application deletion reports
- Preserved shortcuts and Windows integrations
- No cross-app contamination
- Successful automatic migration from Squirrel

## Resources

### Documentation
- Issue #7313: https://github.com/Mudlet/Mudlet/issues/7313
- Issue #6044: https://github.com/Mudlet/Mudlet/issues/6044
- Velopack Docs: https://docs.velopack.io/
- Velopack C++ Guide: https://docs.velopack.io/getting-started/cpp
- Squirrel Migration: https://docs.velopack.io/migrating/squirrel

### Code References
- `src/updater.cpp` - Update logic
- `src/main.cpp:759-780` - runUpdate() function
- `CI/deploy-mudlet-for-windows.sh:217-278` - Squirrel build process

### Community Input
- Velopack developer (@caesay) requested Qt integration guidance in #6044
- Multiple users requested switch to Velopack/Clowd.Squirrel
- Users report issue affects "every time" on some machines, intermittent on others

## Next Steps

1. **Create spike/POC** (3-5 days)
   - Test Velopack with minimal Qt app
   - Verify C++ integration works
   - Test Squirrel → Velopack migration

2. **Implement file lock checking** (3-5 days)
   - Add Windows API checks
   - Deploy as hotfix in next PTB

3. **Full Velopack integration** (2-3 weeks)
   - Update build scripts
   - Integrate C++ library
   - Update updater.cpp and main.cpp
   - Comprehensive testing

4. **Beta testing** (1-2 weeks)
   - Deploy to PTB channel
   - Monitor error rates
   - Gather user feedback

5. **Production rollout** (1 week)
   - Deploy to release channel
   - Monitor migration success
   - Provide user support

---

**Investigation completed:** 2025-10-19
**Estimated total effort:** 4-6 weeks for complete solution
**Priority:** High - affects user experience and installation reliability
