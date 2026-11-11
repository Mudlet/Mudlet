/***************************************************************************
 *   Copyright (C) 2024 by Zooka                                           *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "Host.h"
#include "MudletApp.h"
#include "TrailingWhitespaceMarker.h"
#include "TriggerHighlighter.h"
#include "uiDesign.h"
#include "edbee/edbee.h"
#include "edbee/views/texttheme.h"
#include "edbee/models/textdocumentscopes.h"

namespace {
// Below body text's 4.5, since the colour is a hint rather than the message, but high enough
// that a theme's mid-grey comment colour cannot survive on a mid-grey field
constexpr qreal scmMinimumTokenContrast = 3.5;
} // namespace

TriggerHighlighter::TriggerHighlighter(QTextDocument* parent)
: QSyntaxHighlighter(parent)
{
    setTheme(MudletApp::scmEditorThemeNameLight); // start with the default theme
}

void TriggerHighlighter::setHighlightingEnabled(bool enabled)
{
    highlightingEnabled = enabled;
    rehighlight();
}

void TriggerHighlighter::highlightBlock(const QString& text)
{
    if (!highlightingEnabled) {
        return;
    }

    for (const HighlightingRule& rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

void TriggerHighlighter::setTheme(const QString& themeName)
{
    mThemeName = themeName;
    rebuildRules();
}

void TriggerHighlighter::setFieldColors(const QColor& background, const QColor& text)
{
    if (mFieldBackground == background && mFieldText == text) {
        return;
    }
    mFieldBackground = background;
    mFieldText = text;
    rebuildRules();
}

void TriggerHighlighter::rebuildRules()
{
    highlightingRules.clear();

    auto edbee = edbee::Edbee::instance();
    auto themeManager = edbee->themeManager();
    edbee::TextTheme* theme = themeManager->theme(mThemeName);
    if (!theme) {
        // A theme whose file is gone is never registered; edbee's own editor paints with the
        // fallback then, so the rows follow it
        theme = themeManager->fallbackTheme();
    }

    // set defaults from chosen theme
    edbee::TextThemeRule defaultRule("default", "selector", theme->foregroundColor(), QColor(), false, false, false);
    applyFormatting(anchorFormat, &defaultRule);
    applyFormatting(charClassFormat, &defaultRule);
    applyFormatting(escapeCharFormat, &defaultRule);
    applyFormatting(groupFormat, &defaultRule);
    applyFormatting(quantifierFormat, &defaultRule);

    QList<edbee::TextThemeRule*> rules = theme->rules();

    // override defaults in theme using scopes which map to regex formats
    // check a few as some themes don't provide all of them
    std::map<QString, QTextCharFormat&> scopeMap = {{"comment", anchorFormat},
                                                    {"keyword", charClassFormat},
                                                    {"keyword.control", charClassFormat},
                                                    {"keyword.operator", charClassFormat},
                                                    {"constant", escapeCharFormat},
                                                    {"constant.numeric", escapeCharFormat},
                                                    {"constant.language", escapeCharFormat},
                                                    {"constant.character.escape", escapeCharFormat},
                                                    {"string", groupFormat},
                                                    {"variable", quantifierFormat},
                                                    {"variable.language", quantifierFormat},
                                                    {"variable.parameter", quantifierFormat},
                                                    {"variable.function", quantifierFormat}};

    for (auto& rule : rules) {
        QString scope = rule->scopeSelector()->toString();
        auto it = scopeMap.find(scope);
        if (it != scopeMap.end()) {
            applyFormatting(it->second, rule);
        }
    }

    const QRegularExpression anchorPattern(R"([$^])", QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpression charClassPattern(R"(\[.*?\])", QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpression escapePattern(R"(\\[.*?])", QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpression groupPattern(R"(\((?:\?[:=!])?.*?\))", QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpression quantifierPattern(R"([?+*]|\{\d+,?\d*\})", QRegularExpression::UseUnicodePropertiesOption);

    highlightingRules.append({anchorPattern, anchorFormat});
    highlightingRules.append({charClassPattern, charClassFormat});
    highlightingRules.append({escapePattern, escapeCharFormat});
    highlightingRules.append({quantifierPattern, quantifierFormat});
    highlightingRules.append({groupPattern, groupFormat});

    rehighlight();
}

void TriggerHighlighter::applyFormatting(QTextCharFormat& format, edbee::TextThemeRule* rule)
{
    // Falls back to the field's text colour: an unreadable pattern is worse than an unhighlighted one
    const QColor foreground = mFieldBackground.isValid() ? uiDesign::readableOn(mFieldBackground, rule->foregroundColor(), mFieldText, scmMinimumTokenContrast) : rule->foregroundColor();

    if (foreground.isValid()) {
        format.setForeground(foreground);
    }

    // Never the theme's background: a code theme's dark block would draw a pattern row as a black
    // box on a light form
    format.clearBackground();
    if (rule->bold()) {
        format.setFontWeight(QFont::Bold);
    } else {
        format.setFontWeight(QFont::Normal);
    }
    format.setFontItalic(rule->italic());
    format.setFontUnderline(rule->underline());
}
