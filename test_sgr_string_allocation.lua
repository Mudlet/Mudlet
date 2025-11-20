-- Test script for SGR string allocation optimization
-- Tests both fast path (single parameter, no semicolons) and slow path (multiple parameters)
-- Run this in Mudlet's main console

echo("\n=== Testing SGR String Allocation Optimization ===\n\n")

-- FAST PATH TESTS: Single parameter sequences (no semicolons, no QStringList allocation)
echo("=== Fast Path: Single Parameter Sequences ===\n")

-- Reset
echo("Testing ESC[0m (reset):\n")
feedTriggers(string.char(27) .. "[1;31m" .. "Bold Red" .. string.char(27) .. "[0m Normal\n")

-- Basic formatting
echo("\nBasic formatting codes:\n")
feedTriggers(string.char(27) .. "[1m" .. "Bold\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[3m" .. "Italic\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[4m" .. "Underline\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[9m" .. "Strikeout\n" .. string.char(27) .. "[0m")

-- Standard colors (30-37)
echo("\nStandard foreground colors:\n")
feedTriggers(string.char(27) .. "[31m" .. "Red " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[32m" .. "Green " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[33m" .. "Yellow " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[34m" .. "Blue\n" .. string.char(27) .. "[0m")

-- Bright colors (90-97)
echo("\nBright foreground colors:\n")
feedTriggers(string.char(27) .. "[91m" .. "Bright Red " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[92m" .. "Bright Green " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[93m" .. "Bright Yellow " .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[94m" .. "Bright Blue\n" .. string.char(27) .. "[0m")

-- Background colors (40-47)
echo("\nStandard background colors:\n")
feedTriggers(string.char(27) .. "[41m" .. "Red BG" .. string.char(27) .. "[0m ")
feedTriggers(string.char(27) .. "[42m" .. "Green BG" .. string.char(27) .. "[0m ")
feedTriggers(string.char(27) .. "[43m" .. "Yellow BG" .. string.char(27) .. "[0m\n")

-- Clear formatting codes
echo("\nClear formatting codes:\n")
feedTriggers(string.char(27) .. "[1m" .. "Bold" .. string.char(27) .. "[22m Clear Bold\n")
feedTriggers(string.char(27) .. "[3m" .. "Italic" .. string.char(27) .. "[23m Clear Italic\n")
feedTriggers(string.char(27) .. "[4m" .. "Underline" .. string.char(27) .. "[24m Clear Underline\n")

-- Default colors
echo("\nDefault color codes:\n")
feedTriggers(string.char(27) .. "[31m" .. "Red" .. string.char(27) .. "[39m Default FG\n")
feedTriggers(string.char(27) .. "[41m" .. "Red BG" .. string.char(27) .. "[49m Default BG\n")

-- Overline
echo("\nOverline:\n")
feedTriggers(string.char(27) .. "[53m" .. "Overlined" .. string.char(27) .. "[55m Normal\n")

-- SLOW PATH TESTS: Multiple parameter sequences (semicolon-separated, uses QStringList)
echo("\n=== Slow Path: Multiple Parameter Sequences ===\n")

-- Combined formatting
echo("Combined formatting:\n")
feedTriggers(string.char(27) .. "[1;3;4m" .. "Bold+Italic+Underline\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[1;31m" .. "Bold Red\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[3;32m" .. "Italic Green\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[4;33m" .. "Underline Yellow\n" .. string.char(27) .. "[0m")

-- Color + background
echo("\nForeground + Background:\n")
feedTriggers(string.char(27) .. "[31;44m" .. "Red on Blue\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[32;43m" .. "Green on Yellow\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[37;40m" .. "White on Black\n" .. string.char(27) .. "[0m")

-- Complex combinations
echo("\nComplex combinations:\n")
feedTriggers(string.char(27) .. "[1;3;4;31;44m" .. "Bold+Italic+Underline Red on Blue\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[1;5;91;104m" .. "Bold+Blink Bright Red on Bright Blue\n" .. string.char(27) .. "[0m")

-- Multiple resets
echo("\nMultiple parameters with resets:\n")
feedTriggers(string.char(27) .. "[0;0;0m" .. "Triple reset\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[1;22;1m" .. "Bold, clear, bold again\n" .. string.char(27) .. "[0m")

-- EDGE CASES
echo("\n=== Edge Cases ===\n")

-- Empty parameter (treated as 0)
echo("Empty parameter:\n")
feedTriggers(string.char(27) .. "[m" .. "Empty (should reset)\n" .. string.char(27) .. "[0m")

-- Single semicolon (two empty parameters)
echo("Parameters with empty:\n")
feedTriggers(string.char(27) .. "[;m" .. "Two empty params\n" .. string.char(27) .. "[0m")
feedTriggers(string.char(27) .. "[1;;31m" .. "Empty middle param\n" .. string.char(27) .. "[0m")

echo("\n=== Performance Comparison Info ===\n")
echo("Fast path (single param): No QStringList allocation - ESC[0m, ESC[1m, ESC[31m, etc.\n")
echo("Slow path (multi param): QStringList allocated - ESC[1;31m, ESC[1;3;4m, etc.\n")
echo("\nMost MUD output uses single-parameter sequences, so the fast path\n")
echo("provides significant memory allocation savings!\n\n")

echo("=== Test Complete ===\n")
echo("If all formatting displays correctly, the optimization works!\n\n")
