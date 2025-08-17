#ifndef LIBMUDLET_PATTERN_MATCHER_H
#define LIBMUDLET_PATTERN_MATCHER_H

/***************************************************************************
 *   Copyright (C) 2025 by Rishi Mondal - mavrickrishi@gmail.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "mudlet/core.h"
#include <string>
#include <vector>
#include <memory>
#include <regex>

namespace mudlet {

/**
 * @brief Match result containing match information
 */
struct MatchResult {
    bool matched = false;
    std::string fullMatch;
    std::vector<std::string> captures;
    size_t startPos = 0;
    size_t endPos = 0;
    std::unordered_map<std::string, std::string> namedGroups;
    
    MatchResult() = default;
    MatchResult(bool match) : matched(match) {}
};

/**
 * @brief Pattern matching utility extracted from TTrigger logic
 * 
 * This class encapsulates the core pattern matching algorithms used by
 * triggers and aliases, removing Qt dependencies and providing a clean
 * interface for different matching strategies.
 */
class PatternMatcher {
public:
    /**
     * @brief Match result containing match information
     */
    struct MatchResult {
        bool matched = false;
        std::string fullMatch;
        std::vector<std::string> captures;
        size_t startPos = 0;
        size_t endPos = 0;
        std::unordered_map<std::string, std::string> namedGroups;
        
        MatchResult() = default;
        MatchResult(bool match) : matched(match) {}
    };
    
    /**
     * @brief Compiled pattern for efficient matching
     */
    class CompiledPattern {
    public:
        virtual ~CompiledPattern() = default;
        virtual MatchResult match(const std::string& text) const = 0;
        virtual PatternType getType() const = 0;
        virtual std::string getPattern() const = 0;
    };
    
    /**
     * @brief Create a new pattern matcher instance
     */
    static std::unique_ptr<PatternMatcher> create();
    
    virtual ~PatternMatcher() = default;
    
    /**
     * @brief Compile a pattern for efficient repeated matching
     * @param pattern Pattern string
     * @param type Pattern type
     * @param caseSensitive Whether matching should be case sensitive
     * @return Compiled pattern, or nullptr on compilation failure
     */
    virtual std::unique_ptr<CompiledPattern> compilePattern(
        const std::string& pattern, 
        PatternType type, 
        bool caseSensitive = true) const = 0;
    
    /**
     * @brief Perform a one-time match without compilation
     * @param text Text to match against
     * @param pattern Pattern string
     * @param type Pattern type
     * @param caseSensitive Whether matching should be case sensitive
     * @return Match result
     */
    virtual MatchResult match(
        const std::string& text, 
        const std::string& pattern, 
        PatternType type, 
        bool caseSensitive = true) const = 0;
    
    /**
     * @brief Check if a pattern is valid for the given type
     * @param pattern Pattern string
     * @param type Pattern type
     * @return true if pattern is valid
     */
    virtual bool isValidPattern(const std::string& pattern, PatternType type) const = 0;
    
    /**
     * @brief Get error message for the last compilation/match failure
     * @return Error message, or empty string if no error
     */
    virtual std::string getLastError() const = 0;
    
    /**
     * @brief Escape special characters in a string for use in regex
     * @param text Text to escape
     * @return Escaped text
     */
    static std::string escapeRegex(const std::string& text);
    
    /**
     * @brief Convert a shell-style glob pattern to regex
     * @param glob Glob pattern (with * and ?)
     * @return Regex pattern
     */
    static std::string globToRegex(const std::string& glob);

protected:
    PatternMatcher() = default;
};

/**
 * @brief Color pattern matching for ANSI color sequences
 */
class ColorPatternMatcher {
public:
    /**
     * @brief Color match information
     */
    struct ColorMatch {
        bool matched = false;
        Color foreground;
        Color background;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        size_t startPos = 0;
        size_t endPos = 0;
    };
    
    /**
     * @brief Create a new color pattern matcher
     */
    static std::unique_ptr<ColorPatternMatcher> create();
    
    virtual ~ColorPatternMatcher() = default;
    
    /**
     * @brief Match text against color patterns
     * @param text Text with ANSI color codes
     * @param targetColors Colors to match
     * @return Vector of color matches
     */
    virtual std::vector<ColorMatch> matchColors(
        const std::string& text, 
        const std::vector<Color>& targetColors) const = 0;
    
    /**
     * @brief Extract all colors from text
     * @param text Text with ANSI color codes
     * @return Vector of all color sequences found
     */
    virtual std::vector<ColorMatch> extractAllColors(const std::string& text) const = 0;
    
    /**
     * @brief Check if text contains the specified color
     * @param text Text to check
     * @param color Color to look for
     * @param tolerance Tolerance for color matching (0-100)
     * @return true if color is found
     */
    virtual bool hasColor(
        const std::string& text, 
        const Color& color, 
        int tolerance = 0) const = 0;

protected:
    ColorPatternMatcher() = default;
};

} // namespace mudlet

#endif // LIBMUDLET_PATTERN_MATCHER_H
