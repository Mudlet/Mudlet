/***************************************************************************
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
#include "TrailingWhitespaceMarker.h"
#include "TriggerHighlighter.h"
#include "edbee/views/texttheme.h"
#include "edbee/models/textdocumentscopes.h"

TriggerHighlighter::TriggerHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    setTheme("Mudlet"); // start with the default theme
}

void TriggerHighlighter::setHighlightingEnabled(bool enabled)
{
    highlightingEnabled = enabled;
    rehighlight();
}

void TriggerHighlighter::highlightBlock(const QString &text)
{
    if (!highlightingEnabled) {
        return;
    }

    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

void TriggerHighlighter::setTheme(const QString& themeName)
{
    highlightingRules.clear();

    auto edbee = edbee::Edbee::instance();
    auto themeManager = edbee->themeManager();
    edbee::TextTheme* theme = themeManager->theme(themeName);

    // set defaults from chosen theme
    edbee::TextThemeRule* defaultRule = new edbee::TextThemeRule("default", "selector", theme->foregroundColor(), theme->backgroundColor(), false, false, false);
    applyFormatting(anchorFormat, defaultRule);
    applyFormatting(charClassFormat, defaultRule);
    applyFormatting(escapeCharFormat, defaultRule);
    applyFormatting(groupFormat, defaultRule);
    applyFormatting(quantifierFormat, defaultRule);

    QList<edbee::TextThemeRule*> rules = theme->rules();

    // override defaults in theme using scopes which map to regex formats
    // check a few as some themes don't provide all of them
    std::map<QString, QTextCharFormat&> scopeMap = {
        {"comment", anchorFormat},
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
        {"variable.function", quantifierFormat}
    };

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
    QColor foreground = rule->foregroundColor();
    QColor background = rule->backgroundColor();

    if (foreground.isValid()) {
        format.setForeground(foreground);
    }

    if (background.isValid()) {
        format.setBackground(background);
    }
    if (rule->bold()) {
        format.setFontWeight(QFont::Bold);
    } else {
        format.setFontWeight(QFont::Normal);
    }
    format.setFontItalic(rule->italic());
    format.setFontUnderline(rule->underline());
}
