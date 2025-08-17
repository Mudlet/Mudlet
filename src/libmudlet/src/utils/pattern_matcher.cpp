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

#include "pattern_matcher.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>

namespace mudlet {

/**
 * @brief Compiled pattern implementation for different pattern types
 */
class CompiledPatternImpl : public PatternMatcher::CompiledPattern {
public:
    CompiledPatternImpl(const std::string& pattern, PatternType type, bool caseSensitive)
        : mOriginalPattern(pattern), mType(type), mCaseSensitive(caseSensitive) {
        
        switch (type) {
            case PatternType::Regex:
                compileRegex(pattern, caseSensitive);
                break;
            case PatternType::Substring:
                mSearchPattern = caseSensitive ? pattern : toLower(pattern);
                break;
            case PatternType::Exact:
                mSearchPattern = caseSensitive ? pattern : toLower(pattern);
                break;
            case PatternType::BeginOfLine:
                mSearchPattern = caseSensitive ? pattern : toLower(pattern);
                break;
            default:
                break;
        }
    }
    
    PatternMatcher::MatchResult match(const std::string& text) const override {
        switch (mType) {
            case PatternType::Regex:
                return matchRegex(text);
            case PatternType::Substring:
                return matchSubstring(text);
            case PatternType::Exact:
                return matchExact(text);
            case PatternType::BeginOfLine:
                return matchBeginOfLine(text);
            case PatternType::Color:
                return matchColor(text);
            case PatternType::Prompt:
                return matchPrompt(text);
            default:
                return PatternMatcher::MatchResult(false);
        }
    }
    
    PatternType getType() const override { return mType; }
    std::string getPattern() const override { return mOriginalPattern; }

private:
    std::string mOriginalPattern;
    std::string mSearchPattern;
    PatternType mType;
    bool mCaseSensitive;
    std::regex mCompiledRegex;
    bool mRegexValid = false;
    
    void compileRegex(const std::string& pattern, bool caseSensitive) {
        try {
            auto flags = std::regex_constants::ECMAScript;
            if (!caseSensitive) {
                flags |= std::regex_constants::icase;
            }
            mCompiledRegex = std::regex(pattern, flags);
            mRegexValid = true;
        } catch (const std::regex_error& e) {
            Logger::error("Failed to compile regex '{}': {}", pattern, e.what());
            mRegexValid = false;
        }
    }
    
    PatternMatcher::MatchResult matchRegex(const std::string& text) const {
        if (!mRegexValid) {
            return PatternMatcher::MatchResult(false);
        }
        
        std::smatch match;
        if (std::regex_search(text, match, mCompiledRegex)) {
            PatternMatcher::MatchResult result(true);
            result.fullMatch = match.str(0);
            result.startPos = match.position(0);
            result.endPos = result.startPos + match.length(0);
            
            // Extract capture groups
            for (size_t i = 1; i < match.size(); ++i) {
                result.captures.push_back(match.str(i));
            }
            
            return result;
        }
        
        return PatternMatcher::MatchResult(false);
    }
    
    PatternMatcher::MatchResult matchSubstring(const std::string& text) const {
        std::string searchText = mCaseSensitive ? text : toLower(text);
        size_t pos = searchText.find(mSearchPattern);
        
        if (pos != std::string::npos) {
            PatternMatcher::MatchResult result(true);
            result.fullMatch = text.substr(pos, mSearchPattern.length());
            result.startPos = pos;
            result.endPos = pos + mSearchPattern.length();
            return result;
        }
        
        return PatternMatcher::MatchResult(false);
    }
    
    PatternMatcher::MatchResult matchExact(const std::string& text) const {
        std::string compareText = mCaseSensitive ? text : toLower(text);
        
        if (compareText == mSearchPattern) {
            PatternMatcher::MatchResult result(true);
            result.fullMatch = text;
            result.startPos = 0;
            result.endPos = text.length();
            return result;
        }
        
        return PatternMatcher::MatchResult(false);
    }
    
    PatternMatcher::MatchResult matchBeginOfLine(const std::string& text) const {
        std::string compareText = mCaseSensitive ? text : toLower(text);
        
        if (compareText.length() >= mSearchPattern.length() && 
            compareText.substr(0, mSearchPattern.length()) == mSearchPattern) {
            PatternMatcher::MatchResult result(true);
            result.fullMatch = text.substr(0, mSearchPattern.length());
            result.startPos = 0;
            result.endPos = mSearchPattern.length();
            return result;
        }
        
        return PatternMatcher::MatchResult(false);
    }
    
    PatternMatcher::MatchResult matchColor(const std::string& text) const {
        // Color pattern matching implementation
        // For now, just return false - this will be implemented in Phase 3
        return PatternMatcher::MatchResult(false);
    }
    
    PatternMatcher::MatchResult matchPrompt(const std::string& text) const {
        // Prompt detection implementation
        // Basic heuristic: line that doesn't end with newline and is relatively short
        if (text.length() > 0 && text.length() < 200 && text.back() != '\n') {
            PatternMatcher::MatchResult result(true);
            result.fullMatch = text;
            result.startPos = 0;
            result.endPos = text.length();
            return result;
        }
        
        return PatternMatcher::MatchResult(false);
    }
    
    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
};

/**
 * @brief PatternMatcher implementation
 */
class PatternMatcherImpl : public PatternMatcher {
public:
    std::unique_ptr<CompiledPattern> compilePattern(
        const std::string& pattern, 
        PatternType type, 
        bool caseSensitive) const override {
        
        if (!isValidPattern(pattern, type)) {
            mLastError = "Invalid pattern for type";
            return nullptr;
        }
        
        auto compiled = std::make_unique<CompiledPatternImpl>(pattern, type, caseSensitive);
        mLastError.clear();
        return compiled;
    }
    
    PatternMatcher::MatchResult match(
        const std::string& text, 
        const std::string& pattern, 
        PatternType type, 
        bool caseSensitive) const override {
        
        auto compiled = compilePattern(pattern, type, caseSensitive);
        if (!compiled) {
            return PatternMatcher::MatchResult(false);
        }
        
        return compiled->match(text);
    }
    
    bool isValidPattern(const std::string& pattern, PatternType type) const override {
        if (pattern.empty()) {
            return false;
        }
        
        switch (type) {
            case PatternType::Regex:
                return isValidRegex(pattern);
            case PatternType::Substring:
            case PatternType::Exact:
            case PatternType::BeginOfLine:
            case PatternType::Color:
            case PatternType::Prompt:
                return true;
            default:
                return false;
        }
    }
    
    std::string getLastError() const override {
        return mLastError;
    }

private:
    mutable std::string mLastError;
    
    bool isValidRegex(const std::string& pattern) const {
        try {
            std::regex test(pattern);
            return true;
        } catch (const std::regex_error& e) {
            mLastError = e.what();
            return false;
        }
    }
};

// Static utility methods
std::string PatternMatcher::escapeRegex(const std::string& text) {
    static const std::string specialChars = "\\^$.|?*+()[]{}";
    std::string result;
    result.reserve(text.length() * 2);
    
    for (char c : text) {
        if (specialChars.find(c) != std::string::npos) {
            result += '\\';
        }
        result += c;
    }
    
    return result;
}

std::string PatternMatcher::globToRegex(const std::string& glob) {
    std::string result = "^";
    result.reserve(glob.length() * 2);
    
    for (size_t i = 0; i < glob.length(); ++i) {
        char c = glob[i];
        switch (c) {
            case '*':
                result += ".*";
                break;
            case '?':
                result += ".";
                break;
            case '.':
            case '^':
            case '$':
            case '+':
            case '{':
            case '}':
            case '[':
            case ']':
            case '(':
            case ')':
            case '|':
            case '\\':
                result += '\\';
                result += c;
                break;
            default:
                result += c;
                break;
        }
    }
    
    result += "$";
    return result;
}

// Factory method
std::unique_ptr<PatternMatcher> PatternMatcher::create() {
    return std::make_unique<PatternMatcherImpl>();
}

/**
 * @brief ColorPatternMatcher implementation
 */
class ColorPatternMatcherImpl : public ColorPatternMatcher {
public:
    std::vector<ColorMatch> matchColors(
        const std::string& text, 
        const std::vector<Color>& targetColors) const override {
        
        // Stub implementation for Phase 1
        // Full ANSI color parsing will be implemented in Phase 2
        std::vector<ColorMatch> matches;
        return matches;
    }
    
    std::vector<ColorMatch> extractAllColors(const std::string& text) const override {
        // Stub implementation for Phase 1
        std::vector<ColorMatch> colors;
        return colors;
    }
    
    bool hasColor(
        const std::string& text, 
        const Color& color, 
        int tolerance) const override {
        
        // Stub implementation for Phase 1
        return false;
    }
};

// Factory method for ColorPatternMatcher
std::unique_ptr<ColorPatternMatcher> ColorPatternMatcher::create() {
    return std::make_unique<ColorPatternMatcherImpl>();
}

} // namespace mudlet