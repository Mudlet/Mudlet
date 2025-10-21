# Option B Design: Implement String-to-ANSI Color Conversion for tempComplexRegexTrigger

## Overview

This document outlines how to implement proper color support for `tempComplexRegexTrigger()` by converting string color names to ANSI color codes.

## Design Approach

### 1. Color Conversion Strategy

Since ANSI has a limited 256-color palette and string colors can represent any RGB value, we need a color matching algorithm:

```cpp
// New helper function in Host class (Host.h/Host.cpp)
// Converts a string color name or RGB value to the nearest ANSI color code
int Host::colorStringToAnsiCode(const QString& colorString) const
{
    // Parse the color string to a QColor
    QColor targetColor;

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    targetColor.setNamedColor(colorString);
#else
    targetColor = QColor::fromString(colorString);
#endif

    // If the color is invalid, return scmIgnored
    if (!targetColor.isValid()) {
        return TTrigger::scmIgnored;
    }

    // Special case: if the user explicitly wants "default", check for that
    if (colorString.toLower() == "default") {
        return TTrigger::scmDefault;
    }

    // Find the closest ANSI color using Euclidean distance in RGB space
    return findClosestAnsiColor(targetColor);
}

int Host::findClosestAnsiColor(const QColor& targetColor) const
{
    int closestCode = 0;
    double minDistance = std::numeric_limits<double>::max();

    // Check all 256 ANSI colors
    for (int code = 0; code < 256; ++code) {
        QColor ansiColor = getAnsiColor(code, false); // Get the ANSI color as RGB

        if (!ansiColor.isValid()) {
            continue;
        }

        // Calculate Euclidean distance in RGB space
        double distance = calculateColorDistance(targetColor, ansiColor);

        if (distance < minDistance) {
            minDistance = distance;
            closestCode = code;
        }
    }

    return closestCode;
}

double Host::calculateColorDistance(const QColor& c1, const QColor& c2) const
{
    // Euclidean distance in RGB space
    // Could be enhanced with perceptual color difference (Delta E) for better matching
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();

    return std::sqrt(dr*dr + dg*dg + db*db);
}
```

### 2. Modified tempComplexRegexTrigger Implementation

```cpp
// In TLuaInterpreterMudletObjects.cpp, modify the color handling section:

// Lines 2206-2220 - BEFORE (current broken code):
bool colorTrigger;
QString fgColor;
if (lua_isnumber(L, 5)) {
    colorTrigger = false;
} else {
    colorTrigger = true;
    fgColor = lua_tostring(L, 5);  // ⚠️ Read but never used
}

QString bgColor;
if (lua_isnumber(L, 6)) {
    colorTrigger = false;
} else {
    bgColor = lua_tostring(L, 6);  // ⚠️ Read but never used
}

// Lines 2206-2230 - AFTER (proposed fix):
bool colorTrigger = false;
int ansiFgColor = TTrigger::scmIgnored;
int ansiBgColor = TTrigger::scmIgnored;

// Check argument 5 (foreground color)
if (lua_isnumber(L, 5)) {
    // If it's a number, assume it's already an ANSI code (backward compatibility)
    ansiFgColor = lua_tointeger(L, 5);

    // Validate the ANSI code
    if (!(ansiFgColor == TTrigger::scmIgnored ||
          ansiFgColor == TTrigger::scmDefault ||
          (ansiFgColor >= 0 && ansiFgColor <= 255))) {
        return warnArgumentValue(L, __func__, qsl(
            "invalid foreground color code %1, only %2 (ignore), %3 (default) or 0-255 (ANSI) or a color name string allowed")
            .arg(QString::number(ansiFgColor),
                 QString::number(TTrigger::scmIgnored),
                 QString::number(TTrigger::scmDefault)));
    }
    colorTrigger = true;
} else if (lua_isstring(L, 5)) {
    // It's a string color name - convert to ANSI
    QString fgColorName = lua_tostring(L, 5);
    ansiFgColor = host.colorStringToAnsiCode(fgColorName);

    if (ansiFgColor == TTrigger::scmIgnored && fgColorName.toLower() != "ignore") {
        qWarning() << "tempComplexRegexTrigger: Invalid foreground color name"
                   << fgColorName << "- trigger will ignore foreground color";
    }
    colorTrigger = true;
} else if (!lua_isnoneornil(L, 5)) {
    lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #5 type (foreground color as number or string expected, got %s!)",
                    luaL_typename(L, 5));
    return lua_error(L);
}

// Check argument 6 (background color) - similar logic
if (lua_isnumber(L, 6)) {
    ansiBgColor = lua_tointeger(L, 6);

    if (!(ansiBgColor == TTrigger::scmIgnored ||
          ansiBgColor == TTrigger::scmDefault ||
          (ansiBgColor >= 0 && ansiBgColor <= 255))) {
        return warnArgumentValue(L, __func__, qsl(
            "invalid background color code %1, only %2 (ignore), %3 (default) or 0-255 (ANSI) or a color name string allowed")
            .arg(QString::number(ansiBgColor),
                 QString::number(TTrigger::scmIgnored),
                 QString::number(TTrigger::scmDefault)));
    }
    colorTrigger = true;
} else if (lua_isstring(L, 6)) {
    QString bgColorName = lua_tostring(L, 6);
    ansiBgColor = host.colorStringToAnsiCode(bgColorName);

    if (ansiBgColor == TTrigger::scmIgnored && bgColorName.toLower() != "ignore") {
        qWarning() << "tempComplexRegexTrigger: Invalid background color name"
                   << bgColorName << "- trigger will ignore background color";
    }
    colorTrigger = true;
} else if (!lua_isnoneornil(L, 6)) {
    lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #6 type (background color as number or string expected, got %s!)",
                    luaL_typename(L, 6));
    return lua_error(L);
}

// Validate that we have at least one color set
if (colorTrigger && ansiFgColor == TTrigger::scmIgnored && ansiBgColor == TTrigger::scmIgnored) {
    return warnArgumentValue(L, __func__, qsl(
        "cannot create color trigger with both foreground and background colors ignored"));
}
```

### 3. Store the Colors in the Trigger

```cpp
// Around line 2283 - BEFORE (current code):
auto pT = new TTrigger("a", patterns, propertyList, multiLine, &host);
pT->setIsFolder(false);
pT->setIsActive(true);
pT->setTemporary(true);
pT->registerTrigger();
// ... no color setup ...

// Around line 2283 - AFTER (proposed fix):
auto pT = new TTrigger("a", patterns, propertyList, multiLine, &host);
pT->setIsFolder(false);
pT->setIsActive(true);
pT->setTemporary(true);

// ✅ NEW: If this is a color trigger, set up the color pattern
if (colorTrigger) {
    if (!pT->setupColorTrigger(ansiFgColor, ansiBgColor)) {
        // Color setup failed - clean up and return error
        delete pT;
        lua_pushnil(L);
        lua_pushstring(L, "Failed to set up color trigger pattern");
        return 2;
    }
}

pT->registerTrigger();
```

## Implementation Files to Modify

### Host.h
Add declarations:
```cpp
public:
    int colorStringToAnsiCode(const QString& colorString) const;

private:
    int findClosestAnsiColor(const QColor& targetColor) const;
    double calculateColorDistance(const QColor& c1, const QColor& c2) const;
```

### Host.cpp
Add the three function implementations shown above.

### TLuaInterpreterMudletObjects.cpp
Modify `tempComplexRegexTrigger()` function as shown above.

## Advantages of Option B

1. **Backward Compatible**: Still accepts numbers (ANSI codes) for users who know them
2. **User Friendly**: Accepts named colors like "red", "blue", "#FF0000", etc.
3. **Functional**: Actually stores and uses the color values
4. **Consistent**: Uses the same underlying color trigger system as `tempAnsiColorTrigger()`

## Challenges and Considerations

### 1. Color Matching Accuracy
- Basic RGB distance may not match human perception
- Could implement CIE Delta E (perceptual color difference) for better results
- ANSI palette is limited - some colors will never match perfectly

### 2. Performance
- Finding closest color requires checking all 256 ANSI colors
- Could optimize with:
  - Caching color conversions in a QMap
  - Using a KD-tree for nearest neighbor search
  - Pre-computing a lookup table

### 3. User Expectations
- Users might expect exact color matches but get approximations
- Documentation should clearly explain the conversion process
- Consider warning users when color conversion results in significant difference

### 4. Special Color Names
Need to handle special cases:
- "default" → `TTrigger::scmDefault`
- "ignore" → `TTrigger::scmIgnored`
- Invalid color names → `TTrigger::scmIgnored` with warning

## Testing Strategy

### Unit Tests
```lua
-- Test with ANSI codes (backward compatibility)
local id1 = tempComplexRegexTrigger("test", ".*", func, 0, 1, 2, 0, 0, "", "", "", 0, 0)

-- Test with named colors
local id2 = tempComplexRegexTrigger("test", ".*", func, 0, "red", "blue", 0, 0, "", "", "", 0, 0)

-- Test with hex colors
local id3 = tempComplexRegexTrigger("test", ".*", func, 0, "#FF0000", "#0000FF", 0, 0, "", "", "", 0, 0)

-- Test with special values
local id4 = tempComplexRegexTrigger("test", ".*", func, 0, "default", "ignore", 0, 0, "", "", "", 0, 0)
```

### Integration Tests
- Verify triggers actually fire on matching colors
- Test color pattern matching with MUD output
- Ensure pattern is correctly stored in mColorPatternList

## Documentation Updates Required

1. Update Lua API documentation to clarify:
   - Arguments 5 & 6 can be ANSI codes (0-255) OR color name strings
   - Color names are converted to nearest ANSI color
   - List of special values: "default", "ignore"
   - Examples with both numbers and strings

2. Add migration guide for existing users

3. Add color conversion reference table showing common color names and their ANSI equivalents

## Estimated Effort

- **Code Implementation**: 4-6 hours
  - Color conversion functions: 2 hours
  - Modify tempComplexRegexTrigger: 1 hour
  - Error handling and validation: 1 hour
  - Code review and refinement: 1-2 hours

- **Testing**: 3-4 hours
  - Unit tests: 1 hour
  - Integration tests: 1-2 hours
  - Manual testing with real MUD: 1 hour

- **Documentation**: 2-3 hours
  - API documentation updates
  - Examples and migration guide
  - Color reference table

**Total**: 9-13 hours of development time

## Recommendation

While Option B is technically feasible and would provide a better user experience, the effort required should be weighed against:

1. The low priority of the original issue
2. The existence of working `tempAnsiColorTrigger()` function
3. The inherent limitations of ANSI color matching
4. The fact that this bug has existed since 2019 without major complaints

If implemented, this should include comprehensive documentation explaining the color conversion process and its limitations.
