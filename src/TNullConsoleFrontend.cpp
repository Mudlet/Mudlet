/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TNullConsoleFrontend.h"

#include "Host.h"
#include "MudletApp.h"
#include "TAppFrontend.h"
#include "TCommandHistory.h"
#include "TEvent.h"
#include "TLabelModel.h"
#include "TMap.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QMap>
#include <QMovie>
#include <QObject>
#include <QScopedValueRollback>
#include <QStringList>
#include <QSvgRenderer>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTimer>
#include <QtMath>

#include <algorithm>
#include <climits>
#include <tuple>

namespace {

// QWIDGETSIZE_MAX, which only <QWidget> defines
constexpr int maximumWidgetSize = (1 << 24) - 1;

// TMxpFrameWidgets::createInternalFrame()'s allowance for the tab header
constexpr int tabHeaderHeight = 30;

// As QWidget::resize() bounds a size
QSize widgetSize(const int width, const int height)
{
    return QSize(width, height).expandedTo(QSize(0, 0)).boundedTo(QSize(maximumWidgetSize, maximumWidgetSize));
}

// The font properties a sheet's declarations set, read by the CSS value extractor a widget's sheet goes through
std::optional<QFont> styleSheetFont(const QString& sheet)
{
    QTextDocument document;
    document.setHtml(qsl("<span style=\"%1\">x</span>").arg(sheet.toHtmlEscaped()));
    QTextCursor cursor(&document);
    cursor.setPosition(1);
    const QTextCharFormat format = cursor.charFormat();
    static const int fontProperties[] = {QTextFormat::FontFamilies,
                                         QTextFormat::FontStyleName,
                                         QTextFormat::FontPointSize,
                                         QTextFormat::FontPixelSize,
                                         QTextFormat::FontWeight,
                                         QTextFormat::FontItalic,
                                         QTextFormat::FontUnderline,
                                         QTextFormat::FontOverline,
                                         QTextFormat::FontStrikeOut,
                                         QTextFormat::FontCapitalization,
                                         QTextFormat::FontLetterSpacing,
                                         QTextFormat::FontLetterSpacingType,
                                         QTextFormat::FontWordSpacing,
                                         QTextFormat::FontKerning,
                                         QTextFormat::TextUnderlineStyle};
    for (const int property : fontProperties) {
        if (format.hasProperty(property)) {
            return format.font();
        }
    }
    return std::nullopt;
}

// Qt's known pseudo-states (qcssparser.cpp); any other name makes a rule style a sub-control instead
bool knownPseudoState(const QString& name)
{
    static const QStringList known = qsl("active adjoins-item alternate bottom checked closable closed default disabled edit-focus "
                                         "editable enabled exclusive first flat floatable focus has-children has-siblings horizontal "
                                         "hover indeterminate last left maximized middle minimized movable next-selected no-frame "
                                         "non-exclusive off on only-one open pressed previous-selected read-only right selected top "
                                         "unchecked vertical window")
                                             .split(u' ');
    return known.contains(name, Qt::CaseInsensitive);
}

// One basic selector of a QCss::Selector, and how it relates to the one after it: ' ' for an ancestor, '>' for the
// parent, '+' or '~' for a sibling, or nothing for the last
struct BasicSelector
{
    QString element;
    QStringList ids;
    // An attribute's name, its test ('=', '~', '|', '^', '$' or '*', or nothing for any value) and the value tested
    QList<std::tuple<QString, QChar, QString>> attributes;
    QList<std::pair<QString, bool>> pseudos;
    QChar relationToNext;
};

// A selector as QCss::Parser::parseSelector() reads one, or nothing for one it cannot
std::optional<QList<BasicSelector>> parseSelector(const QStringView selector)
{
    qsizetype at = 0;
    // QCss takes a backslash as escaping the character after it, and reads no hex escape. Any name but the one after
    // a '#' is an IDENT, which may not start with a digit; one that does is read as no name
    const auto ident = [&selector, &at](const bool hash = false) {
        const qsizetype from = at;
        QString name;
        while (at < selector.size()) {
            const QChar next = selector.at(at);
            if (next == u'\\' && at + 1 < selector.size()) {
                name += selector.at(at + 1);
                at += 2;
            } else if (next.isLetterOrNumber() || next == u'_' || next == u'-' || next.unicode() >= 0x80) {
                name += next;
                ++at;
            } else {
                break;
            }
        }
        if (!hash && !name.isEmpty()) {
            const qsizetype first = selector.at(from) == u'-' ? from + 1 : from;
            const QChar start = first < at ? selector.at(first) : QChar();
            if (start != u'\\' && start != u'_' && !start.isLetter() && start.unicode() < 0x80) {
                at = from;
                return QString();
            }
        }
        return name;
    };
    const auto skipSpace = [&selector, &at]() {
        const qsizetype from = at;
        while (at < selector.size() && selector.at(at).isSpace()) {
            ++at;
        }
        return at > from;
    };
    QList<BasicSelector> basicSelectors;
    while (true) {
        BasicSelector basic;
        bool named = false;
        if (at < selector.size() && selector.at(at) == u'*') {
            ++at;
            named = true;
        } else {
            basic.element = ident();
            named = !basic.element.isEmpty();
        }
        bool more = false;
        while (at < selector.size()) {
            const QChar marker = selector.at(at);
            if (marker == u'#' || marker == u'.') {
                ++at;
                const QString name = ident(marker == u'#');
                if (name.isEmpty()) {
                    return std::nullopt;
                }
                if (marker == u'#') {
                    basic.ids << name;
                } else {
                    basic.attributes.append({qsl("class"), u'~', name});
                }
            } else if (marker == u'[') {
                ++at;
                skipSpace();
                const QString name = ident();
                skipSpace();
                if (name.isEmpty() || at >= selector.size()) {
                    return std::nullopt;
                }
                QChar test;
                QString value;
                if (selector.at(at) != u']') {
                    if (selector.at(at) != u'=') {
                        test = selector.at(at++);
                        if (!QStringView(u"~|^$*").contains(test) || at >= selector.size() || selector.at(at) != u'=') {
                            return std::nullopt;
                        }
                    } else {
                        test = u'=';
                    }
                    ++at;
                    skipSpace();
                    if (at < selector.size() && (selector.at(at) == u'"' || selector.at(at) == u'\'')) {
                        const QChar quote = selector.at(at++);
                        while (at < selector.size() && selector.at(at) != quote) {
                            if (selector.at(at) == u'\\' && at + 1 < selector.size()) {
                                value += selector.at(at + 1);
                                at += 2;
                            } else {
                                value += selector.at(at++);
                            }
                        }
                        if (at++ >= selector.size()) {
                            return std::nullopt;
                        }
                    } else if (value = ident(); value.isEmpty()) {
                        return std::nullopt;
                    }
                    skipSpace();
                }
                if (at >= selector.size() || selector.at(at) != u']') {
                    return std::nullopt;
                }
                ++at;
                basic.attributes.append({name, test, value});
            } else if (marker == u':') {
                ++at;
                if (at < selector.size() && selector.at(at) == u':') {
                    ++at;
                }
                const bool negated = at < selector.size() && selector.at(at) == u'!';
                if (negated) {
                    ++at;
                }
                QString name = ident();
                if (name.isEmpty()) {
                    return std::nullopt;
                }
                // QCss::Parser::parsePseudo() reads a function of one IDENT, whose pseudo is no state but a sub-control
                if (at < selector.size() && selector.at(at) == u'(') {
                    ++at;
                    skipSpace();
                    const QString argument = ident();
                    skipSpace();
                    if (argument.isEmpty() || at >= selector.size() || selector.at(at) != u')') {
                        return std::nullopt;
                    }
                    ++at;
                    name += u'(' + argument + u')';
                }
                basic.pseudos.append({name, negated});
            } else {
                break;
            }
            more = true;
        }
        if (!named && !more) {
            return std::nullopt;
        }
        const bool spaced = skipSpace();
        if (at < selector.size() && QStringView(u"+>~").contains(selector.at(at))) {
            basic.relationToNext = selector.at(at++);
            skipSpace();
        } else if (spaced) {
            basic.relationToNext = u' ';
        } else if (at < selector.size()) {
            return std::nullopt;
        }
        basicSelectors << basic;
        if (at >= selector.size()) {
            return basicSelectors;
        }
    }
}

std::optional<QString> attributeValue(const QString& name, const TNullConsoleFrontend::SheetNode& node)
{
    // QStyleSheetStyleSelector takes a null value for no attribute; a label's text is null until set, and again once
    // QLabel::clear() has run
    if (name == QLatin1String("objectName")) {
        return node.objectName.isNull() ? std::nullopt : std::optional<QString>(node.objectName);
    }
    for (const auto& [property, value] : node.properties) {
        if (property == name) {
            return value.isNull() ? std::nullopt : std::optional<QString>(value);
        }
    }
    if (name == QLatin1String("class")) {
        return node.classNames.first();
    }
    return std::nullopt;
}

// QCss::StyleSelector::basicSelectorMatches()'s tests of an attribute's value
bool attributeTestHolds(const QChar test, const QString& value, const QString& wanted)
{
    switch (test.unicode()) {
    case 0:
        return true;
    case u'=':
        return value == wanted;
    case u'~':
        return QStringView(value).split(u' ').contains(QStringView(wanted));
    case u'|':
        return value == wanted || value.startsWith(wanted + u'-');
    case u'^':
        return value.startsWith(wanted);
    case u'$':
        return value.endsWith(wanted);
    case u'*':
        return value.contains(wanted);
    default:
        return false;
    }
}

// QCss::StyleSelector::basicSelectorMatches(), which leaves the pseudo-states to the caller
bool basicSelectorMatches(const BasicSelector& basic, const TNullConsoleFrontend::SheetNode& node)
{
    for (const auto& [name, test, wanted] : basic.attributes) {
        const std::optional<QString> value = attributeValue(name, node);
        if (!value || !attributeTestHolds(test, *value, wanted)) {
            return false;
        }
    }
    return (basic.element.isEmpty() || node.classNames.contains(basic.element)) && (basic.ids.isEmpty() || basic.ids == QStringList{node.objectName});
}

// The specificity QCss::StyleSelector::matchRule() weighs a selector by that matches the first widget of the chain in
// the pseudo-states held, or nothing
std::optional<uint> selectorSpecificity(const QList<BasicSelector>& basics, const TNullConsoleFrontend::SheetChain& chain, const QStringList& heldStates)
{
    if (chain.isEmpty()) {
        return std::nullopt;
    }
    const auto matches = [&basics, &chain](const qsizetype index, const qsizetype depth) {
        return basicSelectorMatches(basics.at(index), chain.at(depth));
    };
    // QCss::BasicSelector::MatchNextSelectorIfAncestor and MatchNextSelectorIfIndirectAdjecent, which may try further on
    const auto indirect = [](const QChar relation) {
        return relation == u' ' || relation == u'~';
    };
    bool match = false;
    if (basics.size() == 1) {
        match = basics.first().relationToNext.isNull() && matches(0, 0);
    } else {
        // QCss::StyleSelector::selectorMatches(), which never goes back to try another ancestor for a basic selector
        // it has matched
        qsizetype index = basics.size() - 1;
        qsizetype depth = 0;
        do {
            match = matches(index, depth);
            if (!match && (index == basics.size() - 1 || !indirect(basics.at(index).relationToNext))) {
                break;
            }
            if (match && --index < 0) {
                break;
            }
            // QStyleSheetStyleSelector gives a widget no previous sibling
            const QChar relation = basics.at(index).relationToNext;
            depth = relation == u' ' || relation == u'>' ? depth + 1 : chain.size();
            if (depth >= chain.size()) {
                match = false;
                break;
            }
        } while (match || indirect(basics.at(index).relationToNext));
    }
    if (!match) {
        return std::nullopt;
    }
    // Only the last basic selector's states are the widget's; any other's are not looked at
    const QList<std::pair<QString, bool>>& pseudos = basics.last().pseudos;
    if (!pseudos.isEmpty() && !knownPseudoState(pseudos.first().first)) {
        return std::nullopt;
    }
    // Selector::pseudoClass(): an unknown state past the first drops the states before it, but not the negated ones
    bool statesHold = true;
    for (const auto& [name, negated] : pseudos) {
        if (!knownPseudoState(name)) {
            statesHold = true;
            break;
        }
        const bool held = heldStates.contains(name, Qt::CaseInsensitive);
        if (negated && held) {
            return std::nullopt;
        }
        if (!negated && !held) {
            statesHold = false;
        }
    }
    if (!statesHold) {
        return std::nullopt;
    }
    uint specificity = 0;
    for (const BasicSelector& basic : basics) {
        specificity += (basic.element.isEmpty() ? 0 : 1) + uint(basic.attributes.size() + basic.pseudos.size()) * 0x10 + uint(basic.ids.size()) * 0x100;
    }
    return specificity;
}

const QStringList& widgetClasses()
{
    static const QStringList classes{qsl("QWidget"), qsl("QObject")};
    return classes;
}

TNullConsoleFrontend::SheetNode mainWindowNode()
{
    return {{qsl("mudlet"), qsl("QMainWindow"), qsl("QWidget"), qsl("QObject")}, qsl("main_window"), {}};
}

// A console's frame, core pane and two base frames, as the TConsole constructor nests them
TNullConsoleFrontend::SheetChain consoleFrameChain()
{
    return {{widgetClasses(), qsl("MainFrame"), {}}, {widgetClasses(), QString(), {}}, {widgetClasses(), QString(), {}}, {widgetClasses(), QString(), {}}};
}

// The main console, then the profile's splitter, its container and the frame that holds that
TNullConsoleFrontend::SheetChain mainConsoleChain(const QString& profile)
{
    return {{{qsl("TMainConsole"), qsl("TConsole"), qsl("QWidget"), qsl("QObject")}, profile, {{qsl("HostName"), profile}}},
            {{qsl("QSplitter"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")}, QString(), {}},
            {widgetClasses(), QString(), {}},
            {widgetClasses(), QString(), {}},
            mainWindowNode()};
}

// What QCss::Scanner makes of a backslash in a string and what follows it at index: how much of that it takes, with
// the text it stands for added to text, or nothing where it reads no sheet at all. Up to six hex digits are one UTF-16
// code unit; NUL and the line breaks end the string, and digits past U+FFFF are no escape unless the first is one of
// A to F in upper case, which the scanner reads as itself.
std::optional<qsizetype> sheetEscape(const QStringView value, const qsizetype index, QString* text)
{
    const auto hexDigit = [](const QChar character) {
        const char16_t unit = character.unicode();
        return (unit >= u'0' && unit <= u'9') || (unit >= u'a' && unit <= u'f') || (unit >= u'A' && unit <= u'F');
    };
    qsizetype digits = 0;
    while (digits < 6 && index + digits < value.size() && hexDigit(value.at(index + digits))) {
        ++digits;
    }
    bool ok = false;
    const ushort code = value.mid(index, digits).toUShort(&ok, 16);
    if (ok && (code == 0 || code == 0xA || code == 0xC || code == 0xD)) {
        return std::nullopt;
    }
    if (digits > 0 && !ok && !(value.at(index) >= u'A' && value.at(index) <= u'F')) {
        return std::nullopt;
    }
    if (text) {
        text->append(ok ? QChar(code) : value.at(index));
    }
    return ok ? digits : 1;
}

// Whether QCss::Scanner can read every string in a rule; Qt takes nothing from one it cannot, nor from the rules after it
bool sheetStringsReadable(const QStringView sheet)
{
    QChar quote;
    for (qsizetype at = 0; at < sheet.size(); ++at) {
        const QChar c = sheet.at(at);
        if (c == u'\\') {
            if (quote.isNull() || at + 1 >= sheet.size()) {
                ++at;
                continue;
            }
            const auto taken = sheetEscape(sheet, at + 1, nullptr);
            if (!taken) {
                return false;
            }
            const bool carriageReturn = sheet.at(at + 1) == QChar::CarriageReturn;
            at += *taken;
            // A backslash takes both halves of a CR LF line break
            if (carriageReturn && at + 1 < sheet.size() && sheet.at(at + 1) == QChar::LineFeed) {
                ++at;
            }
        } else if (!quote.isNull()) {
            // A string does not run over a line break that has no backslash ahead of it
            if (c == QChar::LineFeed || c == QChar::CarriageReturn || c == QChar::FormFeed) {
                return false;
            }
            if (c == quote) {
                quote = QChar();
            }
        } else if (c == u'"' || c == u'\'') {
            quote = c;
        }
    }
    return quote.isNull();
}

qsizetype indexOutsideQuotes(const QString& text, const QChar wanted, qsizetype from)
{
    QChar quote;
    for (; from < text.size(); ++from) {
        const QChar c = text.at(from);
        // QCss::Scanner reads a backslash and the character after it as one, in a string or out of one
        if (c == u'\\') {
            ++from;
        } else if (!quote.isNull()) {
            if (c == quote) {
                quote = QChar();
            }
        } else if (c == u'"' || c == u'\'') {
            quote = c;
        } else if (c == wanted) {
            return from;
        }
    }
    return -1;
}

// The comma-separated selectors of a rule, or nothing if QCss::Parser cannot read one of them
std::optional<QList<QList<BasicSelector>>> parseSelectors(const QString& selectors)
{
    QList<QList<BasicSelector>> parsed;
    for (qsizetype from = 0; from <= selectors.size();) {
        const qsizetype comma = indexOutsideQuotes(selectors, u',', from);
        const qsizetype end = comma < 0 ? selectors.size() : comma;
        const auto basics = parseSelector(QStringView(selectors).mid(from, end - from).trimmed());
        if (!basics) {
            return std::nullopt;
        }
        parsed << *basics;
        from = end + 1;
    }
    return parsed;
}

// The declarations of a sheet that reach the first widget of the chain in the pseudo-states held, in the order
// QStyleSheetStyle applies them. A sheet that is not a list of rules is QStyleSheetStyle's "* { sheet }"; @-rules apply
// to no widget.
QString sheetDeclarations(const QString& sheet, const TNullConsoleFrontend::SheetChain& chain, const QStringList& heldStates)
{
    QString text = sheet;
    for (qsizetype open = indexOutsideQuotes(text, u'/', 0); open >= 0; open = indexOutsideQuotes(text, u'/', open + 1)) {
        if (open + 1 < text.size() && text.at(open + 1) == u'*') {
            const qsizetype close = text.indexOf(qsl("*/"), open + 2);
            text.replace(open, close < 0 ? text.size() - open : close + 2 - open, u' ');
        }
    }
    if (indexOutsideQuotes(text, u'{', 0) < 0) {
        return sheetStringsReadable(text) ? text : QString();
    }
    QList<std::pair<uint, QString>> weighted;
    uint order = 0;
    qsizetype at = 0;
    while (true) {
        while (at < text.size() && text.at(at).isSpace()) {
            ++at;
        }
        if (at >= text.size()) {
            break;
        }
        const qsizetype open = indexOutsideQuotes(text, u'{', at);
        if (open < 0) {
            break;
        }
        qsizetype close = indexOutsideQuotes(text, u'}', open + 1);
        if (text.at(at) == u'@') {
            for (qsizetype inner = indexOutsideQuotes(text, u'{', open + 1); close >= 0 && inner >= 0 && inner < close; inner = indexOutsideQuotes(text, u'{', inner + 1)) {
                close = indexOutsideQuotes(text, u'}', close + 1);
            }
            if (close < 0 || !sheetStringsReadable(QStringView(text).mid(at, close - at))) {
                break;
            }
            at = close + 1;
            continue;
        }
        // The parser stops at the first rule it cannot read, a selector included, keeping the rules before it
        if (close < 0 || indexOutsideQuotes(text.left(close), u'{', open + 1) >= 0 || !sheetStringsReadable(QStringView(text).mid(at, close - at))) {
            break;
        }
        const auto parsed = parseSelectors(text.mid(at, open - at));
        if (!parsed) {
            break;
        }
        const QString declarations = text.mid(open + 1, close - open - 1);
        for (const QList<BasicSelector>& basics : *parsed) {
            if (const auto specificity = selectorSpecificity(basics, chain, heldStates)) {
                weighted.append({order + *specificity * 0x100, declarations});
            }
        }
        ++order;
        at = close + 1;
    }
    std::stable_sort(weighted.begin(), weighted.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    QStringList declarations;
    for (const auto& [weight, block] : std::as_const(weighted)) {
        declarations << block;
    }
    return declarations.join(u';');
}

// QStyleSheetStyle::updateStyleSheetFont() asks for the enabled and active states
std::optional<QFont> sheetFontFor(const QString& sheet, const TNullConsoleFrontend::SheetChain& chain)
{
    return styleSheetFont(sheetDeclarations(sheet, chain, {qsl("enabled"), qsl("active")}));
}

// The text a sheet's declarations give a qproperty, as QStyleSheetStyle::setProperties() reads one: its
// last declaration, and the first term of that
std::optional<QString> sheetStringProperty(const QString& declarations, const QStringView property)
{
    std::optional<QString> found;
    for (qsizetype at = 0; at < declarations.size();) {
        const qsizetype semicolon = indexOutsideQuotes(declarations, u';', at);
        const qsizetype end = semicolon < 0 ? declarations.size() : semicolon;
        const QStringView declaration = QStringView(declarations).mid(at, end - at);
        at = end + 1;
        const qsizetype colon = declaration.indexOf(u':');
        if (colon < 0) {
            continue;
        }
        const QStringView name = declaration.left(colon).trimmed();
        const QStringView value = declaration.mid(colon + 1).trimmed();
        if (value.isEmpty() || !name.startsWith(u"qproperty-", Qt::CaseInsensitive) || name.mid(10) != property) {
            continue;
        }
        QString text;
        const QChar quote = value.front();
        if (quote == u'"' || quote == u'\'') {
            for (qsizetype i = 1; i < value.size() && value.at(i) != quote; ++i) {
                if (value.at(i) == u'\\' && i + 1 < value.size()) {
                    const auto taken = sheetEscape(value, i + 1, &text);
                    if (!taken) {
                        return std::nullopt;
                    }
                    i += *taken;
                    continue;
                }
                text.append(value.at(i));
            }
        } else {
            qsizetype length = 0;
            while (length < value.size() && !value.at(length).isSpace()) {
                ++length;
            }
            text = value.left(length).toString();
        }
        found = text;
    }
    return found;
}

// A box length as QCss::ValueExtractor reads one: px, em, ex or a bare number, and any other unit as 0
int sheetLength(const QString& value, const QFont& font)
{
    QStringView number(value);
    std::optional<int> scale;
    if (number.endsWith(u"px", Qt::CaseInsensitive)) {
        scale = 1;
    } else if (number.endsWith(u"em", Qt::CaseInsensitive)) {
        scale = QFontMetrics(font).height();
    } else if (number.endsWith(u"ex", Qt::CaseInsensitive)) {
        scale = QFontMetrics(font).xHeight();
    }
    if (scale) {
        number.chop(2);
    } else if (number.endsWith(u'%')) {
        number.chop(1);
    }
    // lengthValueFromData()'s bounds, as qRound() is undefined outside int
    return qRound(qBound(static_cast<double>(INT_MIN) + 0.1, scale.value_or(1) * number.toDouble(), static_cast<double>(INT_MAX)));
}

// QRect and QSize add in int, which wraps past INT_MAX, and QWidget then holds a size it is given to 0 to QWIDGETSIZE_MAX
int wrapped(const qint64 sum)
{
    return static_cast<int>(static_cast<quint32>(sum));
}

QSize wrappedSum(const QSize& size, const QSize& other)
{
    return QSize(wrapped(qint64{size.width()} + other.width()), wrapped(qint64{size.height()} + other.height()));
}

// The width and height a frame of top, right, bottom and left widths adds
QSize frameExtent(const std::array<int, 4>& frame)
{
    return QSize(wrapped(qint64{frame[1]} + frame[3]), wrapped(qint64{frame[0]} + frame[2]));
}

// What QStyleSheetStyle sizes a widget by from its sheet's declarations
struct SheetGeometry
{
    // The widths SE_ShapedFrameContents takes off each side, as top, right, bottom and left
    std::array<int, 4> frame{};
    // QStyleSheetStyle::setGeometry()'s
    QSize minimum{0, 0};
    QSize maximum{maximumWidgetSize, maximumWidgetSize};
};

// The font QCss::ValueExtractor measures em and ex in: the application's, as the sheet sets it
QFont sheetLengthFont(const QString& declarations)
{
    const std::optional<QFont> font = styleSheetFont(declarations);
    return font ? font->resolve(QFont()) : QFont();
}

// A widget's margin, border and padding, and its size limits. An edge's border style starts as native,
// which keeps a width it is given and otherwise takes the base style's, unless a border radius is set.
SheetGeometry sheetGeometry(const QString& declarations, const QFont& font, const int nativeFrameWidth)
{
    enum class Style { Native, None, Drawn };
    static const QStringList edges{qsl("top"), qsl("right"), qsl("bottom"), qsl("left")};
    static const QStringList borderStyles = qsl("dotted dashed solid double dot-dash dot-dot-dash groove ridge inset outset").split(u' ');
    std::array<int, 4> margins{};
    std::array<int, 4> paddings{};
    std::array<int, 4> borders{};
    std::array<Style, 4> styles{Style::Native, Style::Native, Style::Native, Style::Native};
    bool box = false;
    bool radius = false;
    // QWidget's own size and limits, where -1 is one the sheet does not set
    int width = -1;
    int height = -1;
    int minimumWidth = -1;
    int minimumHeight = -1;
    int maximumWidth = -1;
    int maximumHeight = -1;
    // QCss::ValueExtractor::lengthValues() and Declaration::styleValues(): one to four values, which CSS repeats
    const auto spread = [](const QStringList& values, auto&& read, auto& sides) {
        static constexpr int source[4][4] = {{0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 1}, {0, 1, 2, 3}};
        const qsizetype count = std::min<qsizetype>(values.size(), 4);
        for (int i = 0; i < 4; ++i) {
            sides[i] = read(count ? values.at(source[count - 1][i]) : QString());
        }
    };
    const auto length = [&font](const QString& value) {
        return sheetLength(value, font);
    };
    // An unknown style keeps the width, as only none takes it away
    const auto style = [](const QString& value) {
        if (value.isEmpty() || !value.compare(qsl("none"), Qt::CaseInsensitive)) {
            return Style::None;
        }
        return value.compare(qsl("native"), Qt::CaseInsensitive) ? Style::Drawn : Style::Native;
    };
    const auto numeric = [](const QString& value) {
        const QStringView digits = QStringView(value).sliced(value.startsWith(u'-') || value.startsWith(u'+') ? 1 : 0);
        return !digits.isEmpty() && (digits.front().isDigit() || digits.front() == u'.') && !value.endsWith(u'%');
    };
    // QCss::ValueExtractor::borderValue(): a width, then a style, and the style only where it is one
    const auto shorthand = [&](const QStringList& values, const int edge) {
        qsizetype at = 0;
        borders[edge] = 0;
        styles[edge] = Style::None;
        if (at < values.size() && numeric(values.at(at))) {
            borders[edge] = length(values.at(at++));
        }
        if (at < values.size() && (style(values.at(at)) != Style::Drawn || borderStyles.contains(values.at(at), Qt::CaseInsensitive))) {
            styles[edge] = style(values.at(at));
        }
    };
    qsizetype at = 0;
    while (at < declarations.size()) {
        qsizetype end = indexOutsideQuotes(declarations, u';', at);
        if (end < 0) {
            end = declarations.size();
        }
        const QString declaration = declarations.mid(at, end - at);
        at = end + 1;
        const qsizetype colon = declaration.indexOf(u':');
        if (colon < 0) {
            continue;
        }
        const QString property = declaration.left(colon).trimmed().toLower();
        const QStringList name = property.split(u'-');
        QString value = declaration.mid(colon + 1);
        // QCss::Parser::testPrio(): a trailing "! important", in any case, marks the declaration and is no value
        if (const qsizetype bang = value.lastIndexOf(u'!'); bang >= 0 && !QStringView(value).sliced(bang + 1).trimmed().compare(u"important", Qt::CaseInsensitive)) {
            value.truncate(bang);
        }
        const QStringList values = value.simplified().split(u' ', Qt::SkipEmptyParts);
        const int first = values.isEmpty() ? 0 : length(values.first());
        const qsizetype edge = name.size() > 1 ? edges.indexOf(name.at(1)) : -1;
        const bool padding = name.first() == qsl("padding");
        if (property == qsl("width")) {
            width = first;
        } else if (property == qsl("height")) {
            height = first;
        } else if (property == qsl("min-width")) {
            minimumWidth = first;
        } else if (property == qsl("min-height")) {
            minimumHeight = first;
        } else if (property == qsl("max-width")) {
            maximumWidth = first;
        } else if (property == qsl("max-height")) {
            maximumHeight = first;
        } else if (padding || name.first() == qsl("margin")) {
            auto& sides = padding ? paddings : margins;
            if (name.size() == 1) {
                spread(values, length, sides);
                box = true;
            } else if (name.size() == 2 && edge >= 0) {
                sides[edge] = first;
                box = true;
            }
        } else if (name.first() == qsl("border")) {
            if (name.last() == qsl("radius")) {
                radius = true;
            } else if (name.size() == 1) {
                shorthand(values, 0);
                borders.fill(borders[0]);
                styles.fill(styles[0]);
            } else if (name.size() == 2 && name.at(1) == qsl("width")) {
                spread(values, length, borders);
            } else if (name.size() == 2 && name.at(1) == qsl("style")) {
                spread(values, style, styles);
            } else if (name.size() == 2 && edge >= 0) {
                shorthand(values, edge);
            } else if (name.size() == 3 && edge >= 0 && name.at(2) == qsl("width")) {
                borders[edge] = first;
            } else if (name.size() == 3 && edge >= 0 && name.at(2) == qsl("style")) {
                styles[edge] = values.size() == 1 ? style(values.first()) : Style::None;
            }
        }
    }
    // QRenderRule::fixupBorder()
    for (int i = 0; i < 4; ++i) {
        if (styles[i] == Style::Native && radius) {
            styles[i] = Style::None;
        }
        if (styles[i] == Style::None) {
            borders[i] = 0;
        } else if (styles[i] == Style::Native && !borders[i]) {
            borders[i] = nativeFrameWidth;
        }
    }
    SheetGeometry geometry;
    std::array<int, 4> widths{};
    for (int i = 0; i < 4; ++i) {
        widths[i] = wrapped(qint64{margins[i]} + borders[i] + paddings[i]);
    }
    // The base style frames a native border, and a label has no frame of its own
    if (box || styles[0] != Style::Native) {
        geometry.frame = widths;
    }
    // QRenderRule::boxSize(), which answers -1 for a negative size, and QWidget, which takes that as 0
    const auto boxed = [](const int size, const int edges) {
        return size < 0 ? 0 : std::clamp(wrapped(qint64{size} + edges), 0, maximumWidgetSize);
    };
    const QSize extent = frameExtent(widths);
    const int across = extent.width();
    const int down = extent.height();
    if (minimumWidth != -1) {
        geometry.minimum.setWidth(boxed(std::max(width, minimumWidth), across));
    }
    if (minimumHeight != -1) {
        geometry.minimum.setHeight(boxed(std::max(height, minimumHeight), down));
    }
    if (maximumWidth != -1) {
        geometry.maximum.setWidth(boxed(std::min(width == -1 ? maximumWidgetSize : width, maximumWidth), across));
    }
    if (maximumHeight != -1) {
        geometry.maximum.setHeight(boxed(std::min(height == -1 ? maximumWidgetSize : height, maximumHeight), down));
    }
    return geometry;
}

// QStyleSheetStyle::updateStyleSheetFont()
void applySheetFont(QFont& font, uint& mask, const std::optional<QFont>& sheetFont)
{
    if (!sheetFont) {
        return;
    }
    QFont local = font;
    local.setResolveMask(mask);
    QFont styled = sheetFont->resolve(local);
    styled.setResolveMask(local.resolveMask() | sheetFont->resolveMask());
    if (local.resolveMask() == styled.resolveMask() && local == styled) {
        return;
    }
    font = styled;
    mask = styled.resolveMask();
}

QFont withSheetFont(QFont font, const std::optional<QFont>& sheetFont)
{
    uint mask = font.resolveMask();
    applySheetFont(font, mask, sheetFont);
    return font;
}

// QLabel::sizeHint() for a label with no margin, indent or word wrap, as TLabel's are, inside the frame its sheet gives it
QSize labelSizeHint(const QFont& font, const QString& text, const bool textLabel, const std::array<int, 4>& frame)
{
    const QSize frameSize = frameExtent(frame);
    const QFontMetrics metrics(font);
    if (!textLabel) {
        return wrappedSum(QSize(metrics.averageCharWidth(), metrics.lineSpacing()), frameSize);
    }
    // QLabelPrivate::sizeForWidth(): a framed label with no indent of its own is indented by an x
    const int indent = *std::max_element(frame.begin(), frame.end()) ? std::max(metrics.horizontalAdvance(u'x'), 0) : 0;
    QTextDocument document;
    document.setDefaultFont(font);
    document.setHtml(text);
    QTextOption option = document.defaultTextOption();
    option.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::ManualWrap);
    document.setDefaultTextOption(option);
    QTextFrameFormat frameFormat = document.rootFrame()->frameFormat();
    frameFormat.setMargin(0);
    document.rootFrame()->setFrameFormat(frameFormat);
    document.setTextWidth(-1);
    const QSizeF size = document.size();
    return wrappedSum(QSize(qCeil(size.width()) + indent, qCeil(size.height())), frameSize);
}

// The sheet TCommandLine's constructor gives every command line but the main one
QString commandLineSheet(const QColor& background)
{
    return qsl("QPlainTextEdit{background-color: rgb(%1, %2, %3);}").arg(background.red()).arg(background.green()).arg(background.blue());
}

} // namespace

void TConsoleModelSink::printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore)
{
    mModel.buffer.appendFormatted(text, formatting, sourceLinkStore);
    mModel.mirrorToStdOut(text, TLineFormat(formatting));
}

void TConsoleModelSink::discardAll()
{
    mModel.clear();
    // As TConsole::discardAll() clears through bufferCleared(), which closes its split
    emit mModel.mNotifier.bufferCleared();
}

void TConsoleModelSink::discardLastLine()
{
    mModel.buffer.clearLastLine();
    emit mModel.mNotifier.contentEdited();
}

// TMxpFrameWidgets::frameWidget(): a frame's container, or an external frame's console, which a script's deletion
// of that console, or of the console holding the frame, leaves until deferred deletes run
bool TNullMxpFrameFrontend::hasFrameWidget(const QString& name) const
{
    const auto it = mView.mFrameHolders.find(name);
    return it != mView.mFrameHolders.end() && it->second;
}

QSize TNullMxpFrameFrontend::consoleSize(const QString& name, const QSize& frameSize) const
{
    const int header = mHeaders.contains(name) ? tabHeaderHeight : 0;
    return QSize(frameSize.width(), frameSize.height() - header).expandedTo(QSize(0, 0));
}

// As TMxpFrameWidgets::reportTabAreaSize(), deferred as a page is resized from inside the relayout that moves its header
void TNullMxpFrameFrontend::reportTabArea(const QString& name, const QSize& size)
{
    Host* pHost = mView.mpHost;
    if (!pHost->mMxpFrameManager.setTabAreaSize(name, size)) {
        return;
    }
    QTimer::singleShot(0, pHost, [pHost]() {
        pHost->consoleFrontend()->mxpFrames().reportSize();
        pHost->mMxpFrameManager.relayoutFrames();
    });
}

void TNullMxpFrameFrontend::createInternalFrame(const QString& name, const QString& hostName, const QString&, const QRect& geometry, const bool showHeader, const bool scrolling)
{
    mHeaders.erase(name);
    if (showHeader) {
        mHeaders[name].pages.push_back(name);
    }
    mHosts[name] = hostName;
    mPlaces[name] = geometry.topLeft();
    const QSize size = consoleSize(name, geometry.size());
    if (showHeader) {
        mHeaders[name].area = size;
    }
    mConsoleSizes[name] = size;
    mView.addFrameConsole(name, size, false, hostName, scrolling);
    if (!showHeader) {
        return;
    }
    // TMxpFrameWidgets builds no header in a frame that has no widget
    if (!hasFrameWidget(name)) {
        mHeaders.erase(name);
        return;
    }
    reportTabArea(name, size);
}

std::optional<QSize> TNullMxpFrameFrontend::createExternalFrame(const QString& name, const QString&, const QSize& size, const bool scrolling)
{
    mHeaders.erase(name);
    mHosts[name] = QString();
    mPlaces.erase(name);
    mView.addFrameConsole(name, size, true, QString(), scrolling);
    return size;
}

void TNullMxpFrameFrontend::createTabFrame(const QString& name, const QString&, const QString& parentName, const QSize& size, const bool scrolling, const bool select)
{
    const auto header = mHeaders.find(parentName);
    if (header == mHeaders.end()) {
        return;
    }
    header->second.pages.push_back(name);
    if (select) {
        header->second.current = header->second.pages.size() - 1;
    }
    // A page is in its header's container, wherever that is
    const auto parentHost = mHosts.find(parentName);
    mHosts[name] = parentHost != mHosts.end() ? parentHost->second : QString();
    mPlaces.erase(name);
    mView.addFrameConsole(name, select ? header->second.area : size, false, parentName, scrolling);
    reportPages();
}

bool TNullMxpFrameFrontend::removeFromParentTabs(const QString& name, const QString& parentName)
{
    const auto header = mHeaders.find(parentName);
    if (header == mHeaders.end()) {
        return false;
    }
    const std::vector<QString>& pages = header->second.pages;
    const auto page = std::find(pages.cbegin(), pages.cend(), name);
    if (page == pages.cend()) {
        return false;
    }
    removePage(header->second, static_cast<size_t>(page - pages.cbegin()));
    reportPages();
    return true;
}

// QTabBar::removeTab() brings forward the tab to the right of the one in front, else the one to its left
void TNullMxpFrameFrontend::removePage(Header& header, const size_t index)
{
    const bool front = index == header.current;
    header.pages.erase(header.pages.begin() + static_cast<std::ptrdiff_t>(index));
    if (index < header.current) {
        --header.current;
    } else if (front && header.current >= header.pages.size()) {
        header.current = header.pages.empty() ? 0 : header.pages.size() - 1;
    }
    if (front && !header.pages.empty()) {
        mView.sizeFrameConsole(header.pages[header.current], header.area);
    }
}

QString TNullMxpFrameFrontend::host(const QString& name) const
{
    const auto it = mHosts.find(name);
    return it != mHosts.end() ? it->second : QString();
}

QPoint TNullMxpFrameFrontend::consolePlace(const QString& name) const
{
    QString frame = name;
    int header = 0;
    for (const auto& [headerName, pages] : mHeaders) {
        if (std::find(pages.pages.cbegin(), pages.pages.cend(), name) != pages.pages.cend()) {
            frame = headerName;
            header = tabHeaderHeight;
        }
    }
    const auto place = mPlaces.find(frame);
    return (place != mPlaces.end() ? place->second : QPoint()) + QPoint(0, header);
}

void TNullMxpFrameFrontend::reportPages()
{
    mView.mWindowNotifier.note();
    QStringList names;
    for (const auto& [name, host] : mHosts) {
        names << name;
    }
    for (const QString& name : names) {
        mView.reportWindowVisibility(name);
    }
}

bool TNullMxpFrameFrontend::pageShown(const QString& name) const
{
    const TWindowRegistry& registry = mView.mpHost->windowRegistry();
    QString frame = name;
    // Bounded, so that a host recorded for a name that was since reused cannot loop
    for (size_t depth = 0; !frame.isEmpty() && depth <= mHosts.size(); ++depth) {
        // An external frame's console is the widget the frames in it are built inside
        if (frame != name && mView.externalFrame(frame) && !registry.subConsoleVisible(frame).value_or(true)) {
            return false;
        }
        for (const auto& [headerName, header] : mHeaders) {
            const auto page = std::find(header.pages.cbegin(), header.pages.cend(), frame);
            if (page != header.pages.cend() && static_cast<size_t>(page - header.pages.cbegin()) != header.current) {
                return false;
            }
        }
        const auto host = mHosts.find(frame);
        frame = host != mHosts.end() ? host->second : QString();
    }
    return true;
}

void TNullMxpFrameFrontend::reportFramesWithin(const QString& name)
{
    QStringList within;
    for (const auto& [frame, frameHost] : mHosts) {
        QString host = frameHost;
        for (size_t depth = 0; !host.isEmpty() && depth <= mHosts.size(); ++depth) {
            if (host == name) {
                within << frame;
                break;
            }
            const auto next = mHosts.find(host);
            host = next != mHosts.end() ? next->second : QString();
        }
    }
    for (const QString& frame : within) {
        mView.reportWindowVisibility(frame);
    }
}

void TNullMxpFrameFrontend::destroyFrame(const QString& name)
{
    mHeaders.erase(name);
    mHosts.erase(name);
    mConsoleSizes.erase(name);
    mPlaces.erase(name);
    // QTabWidget takes a deleted page out of its header
    bool removed = false;
    for (auto& [headerName, header] : mHeaders) {
        const auto page = std::find(header.pages.cbegin(), header.pages.cend(), name);
        if (page != header.pages.cend()) {
            removePage(header, static_cast<size_t>(page - header.pages.cbegin()));
            removed = true;
        }
    }
    mView.removeFrameConsole(name);
    if (removed) {
        reportPages();
    }
}

// Only a container that changes size lays its console out again, so one a script resized keeps that size till then
void TNullMxpFrameFrontend::setGeometry(const QString& name, const QRect& geometry)
{
    if (const auto place = mPlaces.find(name); place != mPlaces.end() && place->second != geometry.topLeft()) {
        place->second = geometry.topLeft();
        mView.mWindowNotifier.note();
    }
    const QSize size = consoleSize(name, geometry.size());
    if (const auto laidOut = mConsoleSizes.find(name); laidOut != mConsoleSizes.end() && laidOut->second == size) {
        return;
    }
    mConsoleSizes[name] = size;
    const auto header = mHeaders.find(name);
    if (header == mHeaders.end()) {
        mView.sizeFrameConsole(name, size);
        return;
    }
    header->second.area = size;
    mView.sizeFrameConsole(header->second.pages[header->second.current], size);
    reportTabArea(name, size);
}

// TMxpFrameWidgets shows the frame's widget, which a script can hide only where it is the console itself
void TNullMxpFrameFrontend::showFrame(const QString& name)
{
    if (mView.externalFrame(name)) {
        mView.setSubConsoleShown(name, true);
    }
}

// TMxpFrameWidgets raises the frame's widget, which holds every page of its header; a page's own is in a
// QStackedWidget, where a raise shows nothing
void TNullMxpFrameFrontend::focusFrame(const QString& name)
{
    if (!hasFrameWidget(name)) {
        return;
    }
    const auto header = mHeaders.find(name);
    for (const auto& [headerName, other] : mHeaders) {
        if (headerName != name && std::find(other.pages.cbegin(), other.pages.cend(), name) != other.pages.cend()) {
            return;
        }
    }
    const std::vector<QString> held = header != mHeaders.end() ? header->second.pages : std::vector<QString>{name};
    for (const QString& console : held) {
        if (mView.inOwnFrame(console)) {
            mView.mSubConsoles[console].stackOrder = ++mView.mTopOfStack;
        }
    }
    mView.mWindowNotifier.note();
}

// A view that attaches drops every frame console and makes its own frames
void TNullMxpFrameFrontend::clear()
{
    mHeaders.clear();
    mHosts.clear();
    mConsoleSizes.clear();
    mPlaces.clear();
}

// Nothing lays the main console out, so the main window is all of it
void TNullMxpFrameFrontend::reportSize()
{
    Host* pHost = mView.mpHost;
    const QSize size = pHost->mainWindowSize().value_or(QSize());
    pHost->mMxpFrameManager.setMainConsoleSize(size, size);
}

TPrintSink* TNullMxpFrameFrontend::sink(const QString& name) const
{
    return mView.frameSink(name);
}

// Geometry then shown, as a std::pair<QRect, bool> streams: this order is the file format
QDataStream& operator<<(QDataStream& out, const TNullConsoleFrontend::Placement& placement)
{
    return out << placement.geometry << placement.shown;
}

QDataStream& operator>>(QDataStream& in, TNullConsoleFrontend::Placement& placement)
{
    return in >> placement.geometry >> placement.shown;
}

TNullConsoleFrontend::TNullConsoleFrontend(Host* pHost)
: mpHost(pHost)
{
    // Not TCommandLine's first state, in which Tab on an empty line puts a word of the buffer there, but the one it
    // is in after any send, in which that does nothing
    endMainCommandLineCompletion();
}

TNullConsoleFrontend::~TNullConsoleFrontend() = default;

// For a view taking over, whose command lines are its own: nothing here writes a setting again
void TNullConsoleFrontend::dropWindows()
{
    {
        const QScopedValueRollback dropping(mDroppingWindows, true);
        dropModels();
    }
    mpMainHolder = std::make_unique<QObject>();
    mFrameHolders.clear();
    mMxpFrames.clear();
    // The view's TCommandLine has its own menu items and words for Tab, which go with it
    mMainCommandLine.menuItems.clear();
    mMainCommandLine.suggestions.clear();
    mMainCommandLine.blacklist.clear();
    mMainCommandLineCompletion = TCommandLineCompletion();
    endMainCommandLineCompletion();
    mMainCommandLineWhollySelected = false;
    // The view has its own map widget, if any. An embedded mapper went with the main window's stand-in.
    mMapWidget.reset();
    mpEmbeddedMapperBox.reset();
}

void TNullConsoleFrontend::makeMainCommandLine(const bool savesHistory)
{
    mpHost->windowRegistry().setMainCommandLineSavesHistory(savesHistory);
    resumeMainCommandLine();
}

// Through the main window's stand-in, which the next view to attach replaces, and ~Host() with it
void TNullConsoleFrontend::followView(QObject* view)
{
    QObject::connect(view, &QObject::destroyed, mpMainHolder.get(), [this]() {
        resumeMainCommandLine();
    });
}

// Lays nothing out: the main area keeps what the view made of it until the null view measures it again
void TNullConsoleFrontend::resumeMainText()
{
    mMainConsoleSheetFont.reset();
    mMainTextSheetFont = profileMainTextSheetFont();
}

void TNullConsoleFrontend::resumeMainCommandLine()
{
    // A view detached by setMainConsoleView(nullptr) asks for this again as it goes
    if (!mMainCommandLine.writer) {
        // The key a command line named "main" writes is the main command line's
        mMainCommandLine.writer = makeHistoryWriter(
                mpMainHolder.get(), qsl("main"), std::get<0>(mpHost->getCmdLineSettings(enums::MainCommandLine, QString())), *mpHost->windowRegistry().commandLineSavesHistory(QString()));
    }
}

// As TMainConsole::resetMainConsole() deletes the widgets later, which leaves a command line a console's own one
// displaced from the main window to go on writing its setting
void TNullConsoleFrontend::resetMainConsole()
{
    for (const auto& [name, commandLine] : mCommandLines) {
        if (commandLine.writer) {
            commandLine.writer->deleteLater();
        }
    }
    for (const auto& [name, box] : mScrollBoxes) {
        if (box->holder) {
            box->holder->deleteLater();
        }
    }
    dropModels();
}

void TNullConsoleFrontend::dropModels()
{
    mWindowNotifier.note();
    // Copies, as each call erases the node that holds the name
    while (!mSubConsoles.empty()) {
        const QString name = mSubConsoles.begin()->first;
        removeSubConsole(name);
    }
    while (!mLabels.empty()) {
        const QString name = mLabels.begin()->first;
        removeLabel(name);
    }
    for (const auto& [name, box] : mScrollBoxes) {
        mpHost->windowRegistry().deregisterScrollBox(name);
    }
    mScrollBoxes.clear();
    for (const auto& [name, commandLine] : mCommandLines) {
        releaseCommandLine(name, commandLine);
    }
    mCommandLines.clear();
    for (const auto& [name, textBox] : mTextBoxes) {
        mpHost->windowRegistry().deregisterTextBox(name);
    }
    mTextBoxes.clear();
    mRetiredWindows.clear();
}

QString TNullConsoleFrontend::userWindowOrMain(const QString& windowname) const
{
    return mpHost->windowRegistry().hasDockWidget(windowname) ? windowname : QString();
}

// Not a user window or scroll box that only a detached view has
bool TNullConsoleFrontend::ownsContainer(const QString& windowname) const
{
    const TWindowRegistry& registry = mpHost->windowRegistry();
    return mScrollBoxes.contains(windowname) || mSubConsoles.contains(windowname) || !(registry.hasScrollBox(windowname) || registry.hasDockWidget(windowname));
}

// The user window and scroll box a new window goes into. Only a mini console looks among the
// scroll boxes first, as TMainConsole::createMiniConsole() does.
std::pair<QString, std::shared_ptr<TNullConsoleFrontend::ScrollBox>> TNullConsoleFrontend::containerFor(const QString& windowname, const bool scrollBoxFirst) const
{
    const QString userWindow = userWindowOrMain(windowname);
    const auto it = mScrollBoxes.find(windowname);
    if (it != mScrollBoxes.end() && (scrollBoxFirst || userWindow.isEmpty())) {
        return {it->second->userWindow, it->second};
    }
    return {userWindow, nullptr};
}

void TNullConsoleFrontend::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool, bool clickThrough)
{
    mWindowNotifier.note();
    auto pLabel = std::make_unique<TLabelModel>(mpHost, name);
    // A script names a label for screen readers through Host, which tells only the model
    pLabel->mAccessibilityChanged = [this](TLabelModel::AccessibilityChange) {
        mWindowNotifier.note();
    };
    pLabel->setClickThrough(clickThrough);
    pLabel->mGeometry = QRect(QPoint(x, y), widgetSize(width, height));
    auto [userWindow, box] = containerFor(windowname, false);
    // A real label takes its console's font as it is made, and keeps it as that font changes; under a window's
    // sheet it starts from the application's, as it does on the main console once the profile has had a sheet
    const bool windowStyled = userWindow.isEmpty() ? mProfileSheetSeen : !windowSheet(userWindow).isEmpty();
    pLabel->mFont = windowStyled ? QGuiApplication::font() : consoleFont(userWindow).value_or(displayFont());
    mpHost->windowRegistry().registerLabel(name, pLabel.get());
    mLabels[name] = {std::move(pLabel), userWindow, std::move(box)};
    mLabels[name].stackOrder = ++mTopOfStack;
    mLabels[name].made = ++mLabelsMade;
    mLabels[name].deactivated = ++mDeactivations;
    reportWindowVisibility(name);
    // TMainConsole::createLabel()'s colour
    setLabelBackgroundColor(name, QColor(32, 32, 32, 255));
}

bool TNullConsoleFrontend::setLabelClickThrough(const QString& name, const bool clickThrough)
{
    mWindowNotifier.note();
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->setClickThrough(clickThrough);
    return true;
}

void TNullConsoleFrontend::deleteLabel(const QString& name)
{
    removeLabel(name);
}

void TNullConsoleFrontend::removeLabel(const QString& name)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return;
    }
    mpHost->windowRegistry().deregisterLabel(name, it->second.pModel.get());
    if (it->second.movie) {
        mpHost->getGifTracker()->unregisterGif(it->second.movie.get());
    }
    mRetiredLabels.push_back(std::move(it->second.pModel));
    mLabels.erase(it);
    queueRelease();
}

void TNullConsoleFrontend::queueRelease()
{
    if (mReleaseQueued) {
        return;
    }
    mReleaseQueued = true;
    // Host's reset drains deferred deletes before it swaps the Lua state, which a timer would miss
    auto pRelease = new QObject();
    QObject::connect(pRelease, &QObject::destroyed, mpHost, [this]() {
        releaseRetired();
    });
    pRelease->deleteLater();
}

void TNullConsoleFrontend::releaseRetired()
{
    mReleaseQueued = false;
    mRetiredConsoles.clear();
    mRetiredLabels.clear();
}

bool TNullConsoleFrontend::setLabelText(const QString& name, const QString& text)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    showLabelText(it->second, it->second.pModel->linkStyledText(text));
    return true;
}

void TNullConsoleFrontend::showLabelText(Label& label, const QString& shown)
{
    mWindowNotifier.note();
    // TLabel::setText() stops the movie even when QLabel::setText() keeps it for being given the same text
    if (label.showsMovie) {
        label.movie->stop();
    }
    // QLabel::setText() ignores the text it already has, and a fresh label's null text equals ""
    if (shown != label.pModel->mText) {
        label.textLabel = true;
        label.showsMovie = false;
        label.pixmapSize.reset();
        label.sizeHint.reset();
    }
    label.pModel->setText(shown);
    updateShownContent(label);
}

// TLabel::imageMayHaveChanged()
void TNullConsoleFrontend::updateShownContent(Label& label)
{
    using ShownContent = TLabelModel::ShownContent;
    // TLabel::showsImage(): text drawn over an SVG is what the label then shows
    const bool showsImage = label.pixmapSize || (label.svgSize && QTextDocumentFragment::fromHtml(label.pModel->mText).toPlainText().trimmed().isEmpty());
    label.pModel->mShownContent = label.showsMovie ? ShownContent::Animation : showsImage ? ShownContent::Graphic : ShownContent::Text;
}

// TLabel::reapplyLinkStyle()
void TNullConsoleFrontend::reapplyLabelLinkStyle(Label& label)
{
    mWindowNotifier.note();
    if (!label.pModel->mText.isEmpty() && TLabelModel::containsAnchorTag(label.pModel->mText)) {
        showLabelText(label, label.pModel->restyledText());
    }
}

std::optional<QSize> TNullConsoleFrontend::getLabelSizeHint(const QString& name) const
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return std::nullopt;
    }
    const Label& label = it->second;
    const QSize frameSize = frameExtent(label.frame);
    // QLabelPrivate::sizeForWidth() answers at least the minimum size, where TLabel's SVG size does not
    const QSize minimum = label.limits.minimum;
    // QLabel measures the movie's current image, re-reading it only as the movie resizes
    if (label.showsMovie && !label.movie->currentPixmap().isNull()) {
        return wrappedSum(label.movie->currentPixmap().deviceIndependentSize().toSize(), frameSize).expandedTo(minimum);
    }
    if (label.pixmapSize) {
        return wrappedSum(*label.pixmapSize, frameSize).expandedTo(minimum);
    }
    // TLabel::sizeHint() answers an SVG's own size only when nothing else shows
    if (label.svgSize && !label.svgSize->isEmpty() && !label.showsMovie && QTextDocumentFragment::fromHtml(label.pModel->mText).toPlainText().trimmed().isEmpty()) {
        return wrappedSum(*label.svgSize, frameSize);
    }
    if (!label.sizeHint) {
        label.sizeHint = labelSizeHint(label.pModel->mFont, label.pModel->mText, label.textLabel, label.frame);
    }
    return label.sizeHint->expandedTo(minimum);
}

// A styled widget's font resolves against the application's, not its parent's
QFont TNullConsoleFrontend::naturalLabelFont(const Label& label) const
{
    return label.styled ? QGuiApplication::font() : consoleFont(label.userWindow).value_or(displayFont());
}

// QWidget::setFont(), which under a sheet also saves the font for each restyle to start from
void TNullConsoleFrontend::setLabelWidgetFont(Label& label, const QFont& font, const bool applySheet)
{
    mWindowNotifier.note();
    if (label.styled) {
        label.savedFont = font;
    }
    label.pModel->mFont = font.resolve(naturalLabelFont(label));
    label.fontMask = font.resolveMask();
    if (applySheet) {
        applySheetFont(label.pModel->mFont, label.fontMask, label.sheetFont);
    }
}

// QStyleSheetStyle::polish(): the first saves the font, every later one starts again from it
void TNullConsoleFrontend::polishLabel(Label& label)
{
    if (label.savedFont) {
        setLabelWidgetFont(label, *label.savedFont, true);
    } else {
        QFont local = label.pModel->mFont;
        local.setResolveMask(label.fontMask);
        label.savedFont = local;
        applySheetFont(label.pModel->mFont, label.fontMask, label.sheetFont);
    }
    label.sizeHint.reset();
}

// TLabel::restyle()
void TNullConsoleFrontend::restyleLabel(Label& label, const QString& sheet)
{
    mWindowNotifier.note();
    TLabelModel& model = *label.pModel;
    const QString inherited = windowSheet(label.userWindow);
    if (sheet == model.mStyleSheet && inherited == label.windowSheet && label.repolished && !label.restyleDue && !sheet.contains(qsl("url("), Qt::CaseInsensitive)) {
        return;
    }
    label.restyleDue = false;
    label.repolished = label.styled && !sheet.isEmpty();
    const QFont before = model.mFont;
    model.mStyleSheet = sheet;
    label.windowSheet = inherited;
    if (sheet.isEmpty() && inherited.isEmpty()) {
        label.sheetChain.clear();
        label.sheetFont.reset();
        label.frame.fill(0);
        label.limits = SizeLimits();
        // Clearing a sheet hands back the saved font and forgets it
        if (label.styled) {
            if (label.savedFont) {
                setLabelWidgetFont(label, *label.savedFont, false);
            }
            label.savedFont.reset();
            label.styled = false;
            label.sizeHint.reset();
        }
    } else {
        label.sheetChain = labelSheetChain(label);
        // The window's sheet first, as a widget's own sheet wins over one it inherits whatever their selectors
        const auto cascade = [&](const QStringList& heldStates) {
            return sheetDeclarations(inherited, label.sheetChain, heldStates) + u';' + sheetDeclarations(sheet, label.sheetChain, heldStates);
        };
        // QStyleSheetStyle::updateStyleSheetFont() asks for the enabled and active states, setGeometry() only for enabled
        const QString declarations = cascade({qsl("enabled"), qsl("active")});
        const QString enabledDeclarations = cascade({qsl("enabled")});
        label.sheetFont = styleSheetFont(declarations);
        label.frame = sheetGeometry(declarations, sheetLengthFont(declarations), 0).frame;
        const SheetGeometry geometry = sheetGeometry(enabledDeclarations, sheetLengthFont(enabledDeclarations), 0);
        label.limits = {geometry.minimum, geometry.maximum};
        label.styled = true;
        polishLabel(label);
        // Qt writes it through QLabel::setText() as it polishes, from the rules that name no state; unlike
        // TLabel::setText(), that leaves a label already holding the text, and the movie it plays, alone
        if (const auto text = sheetStringProperty(cascade({}), u"text"); text && *text != label.pModel->mText) {
            showLabelText(label, *text);
        }
        setLabelSize(label, label.limits.bounded(model.mGeometry.size()));
    }
    if (label.textLabel && model.mFont != before) {
        label.sizeHint.reset();
    }
}

TConsoleModel& TNullConsoleFrontend::addSubConsole(const QString& name, const TWindowRegistry::SubConsoleKind kind, const QString& windowname)
{
    mWindowNotifier.note();
    // As ownConsoleModel() and the TConsole constructor set up a sub-console's model
    auto pModel = std::make_unique<TConsoleModel>(mpHost);
    pModel->mConsoleName = name;
    pModel->mScriptAddressable = true;
    pModel->mScrollBarEnabled = false;
    pModel->mUpperPaneFont = TFontAttributes(mpHost->fontsAntiAlias()).makeFont();
    TConsoleModel& model = *pModel;
    mpHost->windowRegistry().registerSubConsole(name, &model, kind);
    auto [userWindow, box] = containerFor(windowname, true);
    auto pHolder = new QObject(holderFor(userWindow, box.get()));
    mSubConsoles[name] = {std::move(pModel), userWindow, std::move(box)};
    mSubConsoles[name].holder = pHolder;
    mSubConsoles[name].stackOrder = ++mTopOfStack;
    mSubConsoles[name].deactivated = ++mDeactivations;
    mSubConsoles[name].scriptFont = model.mUpperPaneFont;
    // As ~TConsole takes one that goes with the widget holding it, such as a frame nested in a deleted one, out
    // of the registry; the QPointer may already be cleared
    QObject::connect(pHolder, &QObject::destroyed, mpHost, [this, name, pHolder]() {
        if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end() && (!it->second.holder || it->second.holder.data() == pHolder)) {
            removeSubConsole(name);
        }
    });
    // As TConsole connects only these two, the only kinds that can be told not to scroll
    if (kind == TWindowRegistry::SubConsoleKind::MiniConsole || kind == TWindowRegistry::SubConsoleKind::UserWindow) {
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::linesAppended, &model.mNotifier, [this, &model](const int lineCount) {
            reportLinesOverflow(model, lineCount);
        });
        // A trim of the oldest rows, or rows deleted from the end, moves the line it is scrolled back to
        const auto noteScroll = [this, &model]() {
            if (model.mScrolledBackTo) {
                mWindowNotifier.note();
            }
        };
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::linesAppended, &model.mNotifier, noteScroll);
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::contentEdited, &model.mNotifier, noteScroll);
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::linesChanged, &model.mNotifier, noteScroll);
        // As TConsole::applyTimeStamps(), shown or not
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::timeStampsToggled, &model.mNotifier, [this, name]() {
            mWindowNotifier.note();
            // A hidden pane keeps the size it had until the resize it is owed reaches it
            if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end() && (!it->second.paneResizePending || paneShown(it->second))) {
                reportConsoleSize(name);
            }
        });
        const auto noteText = [this, name]() {
            mWindowNotifier.noteText(name);
        };
        // One made again under a name deleted in the same turn can look the same among the windows
        noteText();
        const TConsoleModelNotifier* pNotifier = &model.mNotifier;
        QObject::connect(pNotifier, &TConsoleModelNotifier::linesAppended, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::linesChanged, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::newLinesWritten, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::bufferCleared, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::linkCharactersChanged, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::linkChanged, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::spoilerRevealed, pNotifier, noteText);
        QObject::connect(pNotifier, &TConsoleModelNotifier::contentEdited, pNotifier, noteText);
        // As TConsole::bufferCleared() closes the split, which TConsoleModel::clear() has already forgotten
        QObject::connect(&model.mNotifier, &TConsoleModelNotifier::bufferCleared, &model.mNotifier, [this, name, &model]() {
            mWindowNotifier.note();
            const QSize before = model.mGridSize;
            reportGridSize(name);
            if (model.mGridSize != before) {
                resizePane(name);
            }
        });
    }
    return model;
}

// As TConsole::handleLinesOverflowEvent()
void TNullConsoleFrontend::reportLinesOverflow(const TConsoleModel& model, const int lineCount)
{
    if (model.mScrollingEnabled) {
        return;
    }
    const int linesSpare = model.mGridSize.height() - lineCount;
    if (linesSpare >= 0) {
        return;
    }
    TEvent event{};
    event.mArgumentList.append(qsl("sysWindowOverflowEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(model.mConsoleName);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(-linesSpare));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(event);
}

// As TTextEdit::resizeEvent(), which a hidden pane is sent only as it shows
void TNullConsoleFrontend::resizePane(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    if (!paneShown(it->second) || it->second.sendingResize) {
        it->second.paneResizePending = true;
        return;
    }
    it->second.paneResizePending = false;
    reportConsoleSize(name);
}

// As QWidget::show() sends a pane, and each pane in what it shows, the resize it was owed while hidden
void TNullConsoleFrontend::sendPendingPaneResize(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end() || !it->second.paneResizePending || it->second.sendingResize || !paneShown(it->second)) {
        return;
    }
    it->second.paneResizePending = false;
    reportConsoleSize(name);
}

// As TConsole::raiseMudletResizeEvent()
void TNullConsoleFrontend::reportConsoleSize(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end() || mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return;
    }
    const TConsoleModel& model = *it->second.pModel;
    const QSize grid = model.mGridSize;
    if (!grid.width() || grid == it->second.reportedGrid) {
        return;
    }
    it->second.reportedGrid = grid;
    TEvent event{};
    event.mArgumentList.append(qsl("sysConsoleSizeChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(grid.width()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(grid.height()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(model.mShowTimeStamps ? TBuffer::smTimeStampFormat.size() : 0));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(event);
}

void TNullConsoleFrontend::removeSubConsole(const QString& name, const bool keepWidget)
{
    mWindowNotifier.note();
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    // Its own command line leaves the registry at once, as what a user window holds does here, though the GUI's
    // stays registered until deferred deletes run; its writer waits for those, with the console's holder
    if (const auto line = mCommandLines.find(name); !keepWidget && line != mCommandLines.end() && ownsLine(it->second, line->second)) {
        releaseCommandLine(name, line->second);
        mCommandLines.erase(line);
        republishPlainWindow(name);
    }
    const bool userWindow = registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow;
    const bool shown = it->second.shown;
    QString retiredKey;
    if (userWindow) {
        // Not as the profile goes, where its close has already saved or held where its windows were, nor as a view
        // takes over, which no script asked for
        const bool scriptDeleted = !mpHost->isClosingDown() && !mDroppingWindows;
        if (scriptDeleted) {
            TAppFrontend::instance()->userWindowRemoved(mpHost, name);
        }
        // removeDockWidget() hides a shown dock, which reports what it holds, but a hidden one's report waits for its
        // deferred deletes
        QStringList deferred;
        const QScopedValueRollback deferring(mpDeferredReports, shown ? mpDeferredReports : &deferred);
        if (scriptDeleted) {
            retiredKey = retireWindowContents(name, it->second);
        } else {
            removeWindowContents(name);
        }
        if (!deferred.isEmpty()) {
            auto pRelease = new QObject();
            QObject::connect(pRelease, &QObject::destroyed, mpHost, [this, deferred]() {
                for (const QString& plainName : deferred) {
                    reportWindowVisibility(plainName);
                }
            });
            pRelease->deleteLater();
        }
        registry.deregisterDockWidget(name);
    }
    registry.deregisterSubConsole(name, it->second.pModel.get());
    if (const auto line = mCommandLines.find(name); keepWidget && line != mCommandLines.end() && ownsLine(it->second, line->second)) {
        line->second.consoleShown = it->second.shown;
    }
    if (it->second.holder && !keepWidget) {
        it->second.holder->deleteLater();
    }
    mRetiredConsoles.push_back(std::move(it->second.pModel));
    mSubConsoles.erase(it);
    queueRelease();
    // Its command line, which the console no longer shadows, or as hiding the dock reports its name
    if (keepWidget || (userWindow && shown)) {
        reportWindowVisibility(name);
    }
    if (retiredKey.isEmpty()) {
        return;
    }
    if (shown) {
        reportVisibility(retiredKey, nullptr);
    }
    // TMainConsole::deleteMiniConsole() calls the dock's deleteLater(), which takes what it holds with it
    auto pRelease = new QObject();
    QObject::connect(pRelease, &QObject::destroyed, mpHost, [this, retiredKey]() {
        removeWindowContents(retiredKey);
        mRetiredWindows.erase(retiredKey);
    });
    pRelease->deleteLater();
}

void TNullConsoleFrontend::removeWindowContents(const QString& userWindow)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    QStringList contents;
    for (const auto& [childName, child] : mSubConsoles) {
        if (child.userWindow == userWindow) {
            contents << childName;
        }
    }
    for (const QString& childName : contents) {
        removeSubConsole(childName);
    }
    contents.clear();
    for (const auto& [childName, child] : mLabels) {
        if (child.userWindow == userWindow) {
            contents << childName;
        }
    }
    for (const QString& childName : contents) {
        removeLabel(childName);
    }
    QStringList boxes;
    std::erase_if(mScrollBoxes, [&registry, &userWindow, &boxes](const auto& entry) {
        if (entry.second->userWindow != userWindow) {
            return false;
        }
        registry.deregisterScrollBox(entry.first);
        boxes << entry.first;
        return true;
    });
    std::erase_if(mCommandLines, [this, &userWindow, &boxes](const auto& entry) {
        if (entry.second.userWindow != userWindow) {
            return false;
        }
        releaseCommandLine(entry.first, entry.second);
        boxes << entry.first;
        return true;
    });
    std::erase_if(mTextBoxes, [&registry, &userWindow, &boxes](const auto& entry) {
        if (entry.second.userWindow != userWindow) {
            return false;
        }
        registry.deregisterTextBox(entry.first);
        boxes << entry.first;
        return true;
    });
    for (const QString& plainName : boxes) {
        republishPlainWindow(plainName);
    }
}

// As the GUI's dock, which a script's delete leaves alive until the event loop deletes it, keeps what it holds by
// name and usable, though hidden: they are moved to a key no script can name, which keeps what the window gave them
QString TNullConsoleFrontend::retireWindowContents(const QString& name, const SubConsole& window)
{
    const QString key = QChar(0) + QString::number(++mRetiredWindowCount);
    mRetiredWindows[key] = {name, windowSheet(name), window.pModel->mUpperPaneFont, window.floating};
    const auto retire = [&name, &key](QString& userWindow, ScrollBox* box) {
        if (userWindow != name) {
            return;
        }
        userWindow = key;
        // A box deleted before the window is out of mScrollBoxes, but holds what it held until its own release
        for (; box; box = box->box.get()) {
            if (box->userWindow == name) {
                box->userWindow = key;
            }
        }
    };
    for (auto& [childName, child] : mSubConsoles) {
        retire(child.userWindow, child.box.get());
    }
    for (auto& [childName, child] : mLabels) {
        retire(child.userWindow, child.box.get());
    }
    for (auto& [childName, child] : mScrollBoxes) {
        retire(child->userWindow, child->box.get());
    }
    for (auto& [childName, child] : mCommandLines) {
        retire(child.userWindow, child.box.get());
    }
    for (auto& [childName, child] : mTextBoxes) {
        retire(child.userWindow, child.box.get());
    }
    return key;
}

void TNullConsoleFrontend::addFrameConsole(const QString& name, const QSize& size, const bool external, const QString& hostFrame, const bool scrolling)
{
    // TMxpFrameWidgets builds nothing in a frame that has no widget
    const auto host = mFrameHolders.find(hostFrame);
    QObject* pHost = host != mFrameHolders.end() ? host->second.data() : nullptr;
    if (!hostFrame.isEmpty() && !pHost) {
        return;
    }
    // As TMxpFrameWidgets does for a second build of a name, which TMxpFrameManager never asks for
    removeFrameConsole(name);
    TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, QString());
    SubConsole& console = mSubConsoles[name];
    console.pFrameSink = std::make_unique<TConsoleModelSink>(model);
    model.mScrollingEnabled = scrolling;
    // TMxpFrameWidgets::createExternalFrame() has no container widget but the console itself
    if (external) {
        mFrameHolders[name] = console.holder;
    } else {
        auto pFrame = new QObject(pHost ? pHost : mpMainHolder.get());
        console.holder->setParent(pFrame);
        mFrameHolders[name] = pFrame;
    }
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(QPoint(0, 0), size));
    // An external frame is a mini console of TMainConsole::createMiniConsole()'s, the others take the main font's size
    const int fontSize = mpHost->getDisplayFont().pointSize();
    setWindowFontSize(name, external || fontSize <= 0 ? 12 : fontSize);
    // A sysFontChangeEvent handler can take the console away
    if (!frameSink(name)) {
        return;
    }
    reportGridSize(name);
    setSubConsoleShown(name, true);
}

// As TMxpFrameWidgets lays a frame's console out in its container, which a console a script moved out has left
void TNullConsoleFrontend::sizeFrameConsole(const QString& name, const QSize& size)
{
    if (!inOwnFrame(name)) {
        return;
    }
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(QPoint(0, 0), size));
    mWindowNotifier.note();
    reportGridSize(name);
}

bool TNullConsoleFrontend::inOwnFrame(const QString& name) const
{
    const auto frame = mFrameHolders.find(name);
    if (!frameSink(name) || frame == mFrameHolders.end() || !frame->second) {
        return false;
    }
    const QObject* pHolder = mSubConsoles.at(name).holder;
    return pHolder && (pHolder == frame->second || pHolder->parent() == frame->second);
}

bool TNullConsoleFrontend::externalFrame(const QString& name) const
{
    const auto frame = mFrameHolders.find(name);
    const auto console = mSubConsoles.find(name);
    return frame != mFrameHolders.end() && frame->second && console != mSubConsoles.end() && frame->second == console->second.holder;
}

// TMxpFrameWidgets::destroyFrame() deletes the frame's widgets at once, but a console a script moved out of a frame
// with a container of its own lives on, out of the registry, with its command line
void TNullConsoleFrontend::removeFrameConsole(const QString& name)
{
    QPointer<QObject> pFrame;
    if (const auto it = mFrameHolders.find(name); it != mFrameHolders.end()) {
        pFrame = it->second;
        mFrameHolders.erase(it);
    }
    if (frameSink(name)) {
        const QObject* pHolder = mSubConsoles.at(name).holder;
        removeSubConsole(name, pHolder && pHolder != pFrame && pHolder->parent() != pFrame);
    }
    delete pFrame.data();
}

TConsoleModelSink* TNullConsoleFrontend::frameSink(const QString& name) const
{
    const auto it = mSubConsoles.find(name);
    return it == mSubConsoles.end() ? nullptr : it->second.pFrameSink.get();
}

void TNullConsoleFrontend::createBuffer(const QString& name)
{
    TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::Buffer, QString());
    model.setWrapAt(mpHost->mWrapAt);
    model.setIndentCount(mpHost->mWrapIndentCount);
    model.setHangingIndentCount(mpHost->mWrapHangingIndentCount);
    reportGridSize(name);
}

void TNullConsoleFrontend::addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    const TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, windowname);
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(QPoint(x, y), widgetSize(width, height)));
    // As TMainConsole::createMiniConsole() sizes its font, which raises sysFontChangeEvent, whose handlers can
    // delete the console, or make another under its name
    setWindowFontSize(name, 12);
    const auto made = mSubConsoles.find(name);
    if (made == mSubConsoles.end() || made->second.pModel.get() != &model) {
        return;
    }
    applyUserWindowSheetFont(name, windowSheet(made->second.userWindow));
    setSubConsoleShown(name, true);
}

void TNullConsoleFrontend::deleteMiniConsole(const QString& name)
{
    if (mSubConsoles.count(name)) {
        mpHost->windowRegistry().forgetUserWindowSize(name);
        removeSubConsole(name);
    }
}

void TNullConsoleFrontend::closeSubConsole(const QString& name)
{
    if (!mSubConsoles.count(name)) {
        return;
    }
    if (mpHost->isClosingDown()) {
        removeSubConsole(name);
    } else {
        setSubConsoleShown(name, false);
    }
}

bool TNullConsoleFrontend::openUserWindow(const QString& name, const bool loadLayout, const bool autoDock, const QString& area)
{
    mWindowNotifier.note();
    TWindowRegistry& registry = mpHost->windowRegistry();
    const bool make = !mSubConsoles.count(name);
    if (make) {
        if (registry.hasSubConsole(name)) {
            // A detached view's, which this has no dock for
            return true;
        }
        addSubConsole(name, TWindowRegistry::SubConsoleKind::UserWindow, QString());
        registry.registerDockWidget(name);
        registry.setUserWindowTitle(name, name);
        applyUserWindowSheet(name, mpHost->mProfileStyleSheet);
        // There is no layout to dock it into, so it takes the main window's size
        const QSize size = mpHost->mainWindowSize().value_or(QSize());
        registry.setSubConsoleGeometry(name, QRect(QPoint(0, 0), size));
        registry.setUserWindowSize(name, size);
    }
    const QPointer<const QObject> window = &mSubConsoles.at(name).pModel->mNotifier;
    // A handler run as it is made, as the layout loads or as it shows can delete it, or open another of its name in its
    // place, as TMainConsole::placeUserWindow() finds
    const auto gone = [this, &name, &window]() {
        const auto it = mSubConsoles.find(name);
        return !window || it == mSubConsoles.end() || &it->second.pModel->mNotifier != window;
    };
    if (make) {
        // As TMainConsole::createUserWindow() sizes its font, which raises sysFontChangeEvent
        setWindowFontSize(name, 10);
        if (gone()) {
            return false;
        }
    }
    // As TMainConsole::placeUserWindow(): once per window and before its show, so the first show after it, the load's or
    // this one, reports the size restored
    if (bool& layoutRequested = mSubConsoles.at(name).layoutRequested; loadLayout && !layoutRequested) {
        layoutRequested = true;
        TAppFrontend::instance()->loadWindowLayout();
        if (gone()) {
            return false;
        }
    }
    setSubConsoleShown(name, true);
    if (gone()) {
        return false;
    }
    // As TMainConsole::placeUserWindow(), after the show
    SubConsole& dock = mSubConsoles.at(name);
    dock.autoDock = autoDock;
    static const QMap<QString, Qt::DockWidgetArea> dockAreas{{qsl("r"), Qt::RightDockWidgetArea},
                                                             {qsl("right"), Qt::RightDockWidgetArea},
                                                             {qsl("l"), Qt::LeftDockWidgetArea},
                                                             {qsl("left"), Qt::LeftDockWidgetArea},
                                                             {qsl("t"), Qt::TopDockWidgetArea},
                                                             {qsl("top"), Qt::TopDockWidgetArea},
                                                             {qsl("b"), Qt::BottomDockWidgetArea},
                                                             {qsl("bottom"), Qt::BottomDockWidgetArea}};
    if (area == QLatin1String("f") || area == QLatin1String("floating")) {
        dock.floating = true;
    } else if (const auto docked = dockAreas.constFind(area); docked != dockAreas.cend()) {
        dock.floating = false;
        dock.dockArea = *docked;
    }
    return true;
}

void TNullConsoleFrontend::setProfileStyleSheet(const QString& styleSheet)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    // As TMainConsole::setProfileStyleSheet() restyles every dock, a closed one too. The registry
    // ignores a name with no dock, so mini consoles and buffers keep theirs.
    for (const auto& [name, subConsole] : mSubConsoles) {
        if (registry.hasDockWidget(name)) {
            applyUserWindowSheet(name, styleSheet);
        }
    }
    mProfileSheetSeen = mProfileSheetSeen || !styleSheet.isEmpty();
    restyleMainText();
    restyleWindowContents(QString());
}

// The profile's sheet is on the main console, and a user window's on its dock
QString TNullConsoleFrontend::windowSheet(const QString& userWindow) const
{
    if (const auto retired = mRetiredWindows.find(userWindow); retired != mRetiredWindows.end()) {
        return retired->second.sheet;
    }
    return userWindow.isEmpty() ? mpHost->mProfileStyleSheet : mpHost->windowRegistry().userWindowStyleSheet(userWindow).value_or(QString());
}

// What a widget made in a user window, a scroll box or the main window's area is inside, from its parent out, as
// probed in the GUI: TMainConsole::parentWidgetFor() puts it in a box's widget, a user window's console or the main
// console's frame
TNullConsoleFrontend::SheetChain TNullConsoleFrontend::containerSheetChain(const QString& userWindow, const ScrollBox* box) const
{
    const QString profile = mpHost->getName();
    if (box) {
        SheetChain chain{{{qsl("TScrollBoxWidget"), qsl("QWidget"), qsl("QObject")}, QString(), {}},
                         {widgetClasses(), qsl("qt_scrollarea_viewport"), {}},
                         {{qsl("TScrollBox"), qsl("QScrollArea"), qsl("QAbstractScrollArea"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")}, parentName(userWindow, box), {}}};
        return chain + containerSheetChain(box->userWindow, box->box.get());
    }
    if (userWindow.isEmpty()) {
        return consoleFrameChain() + mainConsoleChain(profile);
    }
    // What a deleted user window held keeps the name and floating its dock had
    QString window = userWindow;
    bool floating = false;
    if (const auto retired = mRetiredWindows.find(userWindow); retired != mRetiredWindows.end()) {
        window = retired->second.name;
        floating = retired->second.floating;
    } else if (const auto dock = mSubConsoles.find(userWindow); dock != mSubConsoles.end()) {
        floating = dock->second.floating;
    }
    return {{{qsl("TConsole"), qsl("QWidget"), qsl("QObject")}, qsl("dockWindowConsole_%1_%2").arg(profile, window), {}},
            {{qsl("TDockWidget"), qsl("QDockWidget"), qsl("QWidget"), qsl("QObject")}, qsl("dockWindow_%1_%2").arg(profile, window), {{qsl("floating"), floating ? qsl("true") : qsl("false")}}},
            mainWindowNode()};
}

// A user window's console, which is in its dock, or a mini console, which TMainConsole::createMiniConsole() names
TNullConsoleFrontend::SheetChain TNullConsoleFrontend::consoleSheetChain(const QString& name) const
{
    if (name == qsl("main")) {
        return mainConsoleChain(mpHost->getName());
    }
    const auto console = mSubConsoles.find(name);
    const SheetNode node{{qsl("TConsole"), qsl("QWidget"), qsl("QObject")}, name, {}};
    if (console == mSubConsoles.end()) {
        return {node};
    }
    if (mpHost->windowRegistry().hasDockWidget(name)) {
        return containerSheetChain(name, nullptr);
    }
    return SheetChain{node} + containerSheetChain(console->second.userWindow, console->second.box.get());
}

// The upper pane of a console, in its splitter, layer, display and frame, as the TConsole constructor lays them out
TNullConsoleFrontend::SheetChain TNullConsoleFrontend::paneSheetChain(const QString& name) const
{
    const QString profile = mpHost->getName();
    const SheetChain chain{{{qsl("TTextEdit"), qsl("QWidget"), qsl("QObject")}, qsl("upperPane_%1_%2").arg(profile, name), {}},
                           {{qsl("TSplitter"), qsl("QSplitter"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")}, qsl("splitter_%1_%2").arg(profile, name), {}},
                           {widgetClasses(), qsl("layer"), {}},
                           {widgetClasses(), qsl("MainDisplay"), {}}};
    return chain + consoleFrameChain() + consoleSheetChain(name);
}

TNullConsoleFrontend::SheetChain TNullConsoleFrontend::labelSheetChain(const Label& label) const
{
    const SheetNode node{
            {qsl("TLabel"), qsl("QLabel"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")}, qsl("label_%1_%2").arg(mpHost->getName(), label.pModel->mName), {{qsl("text"), label.pModel->mText}}};
    return SheetChain{node} + containerSheetChain(label.userWindow, label.box.get());
}

// A console's own command line is in the console's command line layer; any other in the window it was made in
TNullConsoleFrontend::SheetChain TNullConsoleFrontend::plainWindowSheetChain(const QString& name, const PlainWindow& window, const bool commandLine) const
{
    const SheetNode node{{commandLine ? qsl("TCommandLine") : qsl("TTextBox"), qsl("QPlainTextEdit"), qsl("QAbstractScrollArea"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")},
                         (commandLine ? qsl("commandLine_%1_%2") : qsl("textEdit_%1_%2")).arg(mpHost->getName(), name),
                         {}};
    if (window.inConsole) {
        return SheetChain{node, {widgetClasses(), QString(), {}}} + consoleSheetChain(name);
    }
    return SheetChain{node} + containerSheetChain(window.userWindow, window.box.get());
}

// As Qt restyles the widgets inside one given a sheet: the labels, and the text of the mini consoles
void TNullConsoleFrontend::restyleWindowContents(const QString& userWindow)
{
    for (auto& [name, label] : mLabels) {
        if (label.userWindow == userWindow) {
            // Qt repolishes them all even for the sheet they had, which matches a label's text again
            label.restyleDue = true;
            restyleLabel(label, label.pModel->mStyleSheet);
        }
    }
    QStringList miniConsoles;
    for (const auto& [name, console] : mSubConsoles) {
        if (console.userWindow == userWindow && mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::MiniConsole) {
            miniConsoles << name;
        }
    }
    const QString sheet = windowSheet(userWindow);
    for (const QString& name : std::as_const(miniConsoles)) {
        applyUserWindowSheetFont(name, sheet);
    }
}

std::pair<bool, QString> TNullConsoleFrontend::createMapper(const QString& windowname, const int x, const int y, const int width, const int height)
{
    mWindowNotifier.note();
    // As TMainConsole::createMapper(), which only takes a user window: not a scroll box
    const bool wantsMainConsole = windowname.isEmpty() || !windowname.compare(QLatin1String("main"), Qt::CaseInsensitive);
    const auto refusal = [&]() -> std::optional<std::pair<bool, QString>> {
        if (!wantsMainConsole && !mpHost->windowRegistry().hasDockWidget(windowname)) {
            return std::pair{false, qsl("window '%1' not found").arg(windowname)};
        }
        if (mMapWidget && mMapWidget->shown) {
            return std::pair{false, qsl("cannot create mapper. Do you already use a map window?")};
        }
        if (!mpEmbeddedMapper && !ownsContainer(windowname)) {
            // A detached view's user window, which only that view could make a mapper in
            return noView();
        }
        return std::nullopt;
    };
    if (const auto refused = refusal()) {
        return *refused;
    }
    removeHiddenMapWidget();
    const bool makesMapper = !mpEmbeddedMapper;
    if (makesMapper) {
        TNullMapView::announceEditing(mpHost);
        // A handler of that may have opened a map or closed the window
        if (const auto refused = refusal()) {
            return *refused;
        }
        removeHiddenMapWidget();
    }
    if (!mpEmbeddedMapper) {
        mpEmbeddedMapper = new TNullMapView(mpHost, holderFor(windowname, nullptr));
        mEmbeddedMapperWindow = userWindowOrMain(windowname);
        mpEmbeddedMapperBox.reset();
        mEmbeddedMapperStackOrder = ++mTopOfStack;
        // dlgMapper shows itself when a map load asks it to
        QObject::connect(mpHost->mpMap.data(), &TMap::signal_mapperShowRequested, mpEmbeddedMapper, [this]() {
            showEmbeddedMapper();
        });
        // The window it is in goes with a deferred delete, after that window's own change may have been told
        QObject::connect(mpEmbeddedMapper, &QObject::destroyed, &mWindowNotifier, [this]() {
            mWindowNotifier.note();
        });
        copyPlayerRoomStyleToMap();
        mpHost->loadMapForNewMapper(true);
    }
    // Raised too where a handler of the mode event made the mapper, as the GUI's second createMapper() does
    if (makesMapper) {
        TEvent mapOpenEvent{};
        mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mapOpenEvent);
    }
    // TMainConsole::createMapper() moves and resizes the one there is, as Geyser.Mapper does by calling it again
    mEmbeddedMapperGeometry = QRect(QPoint(x, y), widgetSize(width, height));
    showEmbeddedMapper();
    return {true, QString()};
}

// The map widget has no view while its mode event runs
void TNullConsoleFrontend::MapWidget::setShown(const bool show)
{
    shown = show;
    if (pView) {
        pView->setShown(show);
    }
}

// As TMainConsole::createMapper() deleting the hidden map dock leaves QMainWindow its placeholder
void TNullConsoleFrontend::removeHiddenMapWidget()
{
    if (!mMapWidget) {
        return;
    }
    TAppFrontend::instance()->mapWidgetRemoved(mpHost);
    // Deferred, as the GUI's dock is: a map info contributor or a mouse press handler drawing this view may be
    // the caller
    if (TNullMapView* pOldView = mMapWidget->pView.release()) {
        pOldView->setParent(mpMainHolder.get());
        pOldView->deleteLater();
    }
    mMapWidget.reset();
}

void TNullConsoleFrontend::createMapperDock(const QString& title, const QString&)
{
    mWindowNotifier.note();
    // The widget is there before its map, as the GUI's dock is while dlgMapper's constructor raises the event
    mMapWidget = MapWidget{title, QRect(), false, nullptr, ++mTopOfStack};
    TNullMapView::announceEditing(mpHost);
    // A handler of that may have made a mapper in its place
    if (!mMapWidget || mMapWidget->pView) {
        return;
    }
    mMapWidget->pView = std::make_unique<TNullMapView>(mpHost);
    mMapWidget->pView->setShown(mMapWidget->shown);
    copyPlayerRoomStyleToMap();
}

// As a map load or a script shows the dlgMapper, which leaves a hidden window holding it hidden
void TNullConsoleFrontend::showEmbeddedMapper()
{
    mWindowNotifier.note();
    mEmbeddedMapperShown = true;
    reportEmbeddedMapperVisibility();
}

void TNullConsoleFrontend::reportEmbeddedMapperVisibility()
{
    if (mpEmbeddedMapper) {
        mpEmbeddedMapper->setShown(mEmbeddedMapperShown && shownWithin(mEmbeddedMapperWindow, mpEmbeddedMapperBox.get()));
    }
}

// As TMainConsole does as it makes a dlgMapper, for the map to have the profile's player room style
void TNullConsoleFrontend::copyPlayerRoomStyleToMap()
{
    TMap* pMap = mpHost->mpMap.data();
    mpHost->getPlayerRoomStyleDetails(
            pMap->mPlayerRoomStyle, pMap->mPlayerRoomOuterDiameterPercentage, pMap->mPlayerRoomInnerDiameterPercentage, pMap->mPlayerRoomOuterColor, pMap->mPlayerRoomInnerColor);
}

// Docked, then the layout loaded, then shown, as TMainConsole::showNewMapperDock() does, so a load's handler that
// places the map widget leaves it where it put it
void TNullConsoleFrontend::showNewMapperDock()
{
    mWindowNotifier.note();
    if (mMapWidget) {
        mMapWidget->geometry = QRect(QPoint(0, 0), mpHost->mainWindowSize().value_or(QSize()));
    }
    TAppFrontend::instance()->loadWindowLayout();
    placeMapWidget(QString(), -1, -1, -1, -1);
}

std::pair<bool, QString> TNullConsoleFrontend::placeMapWidget(const QString& area, const int x, const int y, const int width, const int height)
{
    mWindowNotifier.note();
    if (!mMapWidget) {
        return {false, qsl("cannot create map widget. Do you already use an embedded mapper?")};
    }

    // Shown before the area is checked, as TMainConsole::placeMapWidget() does
    mMapWidget->setShown(true);
    if (area.isEmpty()) {
        return {true, QString()};
    }
    if (area == QLatin1String("f") || area == QLatin1String("floating")) {
        if (x != -1 && y != -1) {
            mMapWidget->geometry.moveTo(x, y);
        }
        if (width != -1 && height != -1) {
            mMapWidget->geometry.setSize(widgetSize(width, height));
        }
        return {true, QString()};
    }
    static const QStringList dockAreas{qsl("r"), qsl("right"), qsl("l"), qsl("left"), qsl("t"), qsl("top"), qsl("b"), qsl("bottom")};
    if (dockAreas.contains(area)) {
        // There is no layout to dock it into, so it takes the main window's size, as a docked user window does
        mMapWidget->geometry = QRect(QPoint(0, 0), mpHost->mainWindowSize().value_or(QSize()));
        return {true, QString()};
    }
    return {false, qsl(R"(docking option "%1" not available. available docking options are "t" top, "b" bottom, "r" right, "l" left and "f" floating)").arg(area)};
}

bool TNullConsoleFrontend::setMapWidgetTitle(const QString& title)
{
    if (!mMapWidget || !mMapWidget->shown) {
        return false;
    }
    mMapWidget->title = title;
    return true;
}

std::optional<QString> TNullConsoleFrontend::mapWidgetTitle() const
{
    if (!mMapWidget || !mMapWidget->shown) {
        return std::nullopt;
    }
    return mMapWidget->title;
}

std::optional<QRect> TNullConsoleFrontend::mapWidgetGeometry() const
{
    if (!mMapWidget || !mMapWidget->shown) {
        return std::nullopt;
    }
    return mMapWidget->geometry;
}

std::optional<TNullConsoleFrontend::Placement> TNullConsoleFrontend::mapWidgetPlacement() const
{
    if (!mMapWidget) {
        return std::nullopt;
    }
    return Placement{mMapWidget->geometry, mMapWidget->shown};
}

void TNullConsoleFrontend::restoreMapWidgetPlacement(const Placement& placement)
{
    mWindowNotifier.note();
    if (!mMapWidget) {
        return;
    }
    mMapWidget->geometry = placement.geometry;
    mMapWidget->setShown(placement.shown);
}

bool TNullConsoleFrontend::hideMapWidget()
{
    mWindowNotifier.note();
    if (!mMapWidget || !mMapWidget->shown) {
        return false;
    }
    mMapWidget->setShown(false);
    return true;
}

TLabelModel* TNullConsoleFrontend::labelModel(const QString& name) const
{
    const auto it = mLabels.find(name);
    return it == mLabels.end() ? nullptr : it->second.pModel.get();
}

TConsoleModel* TNullConsoleFrontend::subConsoleModel(const QString& name) const
{
    const auto it = mSubConsoles.find(name);
    return it == mSubConsoles.end() ? nullptr : it->second.pModel.get();
}

bool TNullConsoleFrontend::setLabelShown(const QString& name, const bool shown)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    // As QWidget::setVisible() to what a widget already is, which reports nothing, so leaves what a window still
    // sending its show's resize holds its earlier answer
    if (it->second.shown == shown) {
        return true;
    }
    if (!shown) {
        it->second.deactivated = ++mDeactivations;
    }
    it->second.shown = shown;
    reportWindowVisibility(name);
    return true;
}

bool TNullConsoleFrontend::subConsoleShown(const QString& name) const
{
    const auto it = mSubConsoles.find(name);
    return it != mSubConsoles.end() && it->second.shown;
}

// As TMainConsole::reportDockGeometry() reports each dock's name, leaving alone what it holds
void TNullConsoleFrontend::reportUserWindowVisibility()
{
    const TWindowRegistry& registry = mpHost->windowRegistry();
    for (const auto& [name, subConsole] : mSubConsoles) {
        if (registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow) {
            reportWindowVisibility(name);
        }
    }
}

bool TNullConsoleFrontend::setSubConsoleShown(const QString& name, const bool shown)
{
    mWindowNotifier.note();
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return false;
    }
    // As for a label, even while the window's own show is still sending it the resize
    if (it->second.shown == shown) {
        return true;
    }
    if (mpHost->windowRegistry().subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
        it->second.shown = shown;
        sendPendingPaneResize(name);
        reportWindowVisibility(name);
        return true;
    }
    it->second.shown = shown;
    const QPointer<const QObject> window = &it->second.pModel->mNotifier;
    if (shown) {
        // The dock's layout gives its console a size it took while hidden only now, before the console shows
        const QSize size = mpHost->windowRegistry().subConsoleGeometry(name).value_or(QRect()).size();
        if (size != it->second.consoleSize) {
            it->second.consoleSize = size;
            it->second.resizePending = true;
        }
    }
    if (shown && it->second.resizePending) {
        // As QWidgetPrivate::show_helper(), which sends the pending resize before the window reports itself visible,
        // though what it holds answers as isVisibleTo() once a change reaches it (see reportVisibility()), and clears it
        // only once that is sent
        it->second.sendingResize = true;
        raiseUserWindowResize(name);
        // Its handlers can delete the window, or open another of its name
        if (const auto again = mSubConsoles.find(name); window && again != mSubConsoles.end() && &again->second.pModel->mNotifier == window) {
            again->second.resizePending = false;
            again->second.sendingResize = false;
        }
    }
    // Its pane is sent the resize it is owed only as it shows, after the dock's
    if (const auto again = mSubConsoles.find(name); shown && window && again != mSubConsoles.end() && &again->second.pModel->mNotifier == window) {
        sendPendingPaneResize(name);
    }
    reportVisibility(name, nullptr);
    // A handler that deletes the window leaves showWindow() with none to show
    return mSubConsoles.contains(name);
}

// As TConsole::resizeEvent() reports a user window
void TNullConsoleFrontend::raiseUserWindowResize(const QString& name)
{
    const QSize size = mpHost->windowRegistry().subConsoleGeometry(name).value_or(QRect()).size();
    mpHost->getLuaInterpreter()->call(qsl("handleWindowResizeEvent"), qsl("WindowResizeEvent"));

    TEvent event{};
    event.mArgumentList.append(qsl("sysUserWindowResizeEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(size.width()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(size.height()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
}

// QWidget::isVisibleTo() the main console, for which a window inside a hidden user window or scroll
// box is hidden too
bool TNullConsoleFrontend::shownWithin(const QString& userWindow, const ScrollBox* box) const
{
    for (; box; box = box->box.get()) {
        if (!box->shown) {
            return false;
        }
    }
    if (userWindow.isEmpty()) {
        return true;
    }
    const auto it = mSubConsoles.find(userWindow);
    return it != mSubConsoles.end() && it->second.shown;
}

// As TMainConsole::reportVisibility() for one name, so that making many windows does not report every window each time
void TNullConsoleFrontend::reportWindowVisibility(const QString& name)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (const auto it = mLabels.find(name); it != mLabels.end()) {
        setLabelVisible(it->second, it->second.shown && shownWithin(it->second.userWindow, it->second.box.get()));
    }
    if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end()) {
        const bool onPageShown = !inOwnFrame(name) || mMxpFrames.pageShown(name);
        registry.setSubConsoleVisible(name, it->second.shown && onPageShown && shownWithin(it->second.userWindow, it->second.box.get()));
    }
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        registry.setPlainWindowVisible(name, it->second->shown && shownWithin(it->second->userWindow, it->second->box.get()));
    } else if (const PlainWindow* window = plainWindow(name)) {
        registry.setPlainWindowVisible(name, window->shown && window->layerShown && window->consoleShown && shownWithin(window->userWindow, window->box.get()));
    }
    // After its own, which the frames in it read
    if (externalFrame(name)) {
        mMxpFrames.reportFramesWithin(name);
    }
}

// As TMainConsole::reportVisibility() and reportVisibilityWithin() report a change, to the user window or scroll box
// given: each name it holds, so a window of that name elsewhere too, even one whose show is still sending its resize
void TNullConsoleFrontend::reportVisibility(const QString& changedWindow, const ScrollBox* changedBox)
{
    reportEmbeddedMapperVisibility();
    const auto holds = [&changedWindow, changedBox](const QString& userWindow, const ScrollBox* box) {
        return changedBox ? within(box, changedBox) : userWindow == changedWindow;
    };
    QStringList names;
    if (!changedBox) {
        names << changedWindow;
    }
    for (const auto& [name, label] : mLabels) {
        if (holds(label.userWindow, label.box.get())) {
            names << name;
        }
    }
    for (const auto& [name, subConsole] : mSubConsoles) {
        if (holds(subConsole.userWindow, subConsole.box.get())) {
            names << name;
        }
    }
    for (const auto& [name, textBox] : mTextBoxes) {
        if (holds(textBox.userWindow, textBox.box.get())) {
            names << name;
        }
    }
    for (const auto& [name, commandLine] : mCommandLines) {
        if (holds(commandLine.userWindow, commandLine.box.get())) {
            names << name;
        }
    }
    for (const auto& [name, box] : mScrollBoxes) {
        if (holds(box->userWindow, box.get())) {
            names << name;
        }
    }
    for (const QString& name : names) {
        reportWindowVisibility(name);
    }
    for (const QString& name : names) {
        sendPendingPaneResize(name);
    }
}

bool TNullConsoleFrontend::createScrollBox(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    mWindowNotifier.note();
    if (mScrollBoxes.contains(name)) {
        return false;
    }
    auto box = std::make_shared<ScrollBox>();
    std::tie(box->userWindow, box->box) = containerFor(windowname, false);
    box->holder = new QObject(holderFor(box->userWindow, box->box.get()));
    box->stackOrder = ++mTopOfStack;
    box->deactivated = ++mDeactivations;
    mScrollBoxes[name] = box;
    TWindowRegistry& registry = mpHost->windowRegistry();
    registry.registerScrollBox(name);
    registry.setPlainWindowGeometry(name, QRect(QPoint(x, y), widgetSize(width, height)));
    // A new box holds nothing yet
    reportWindowVisibility(name);
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::deleteScrollBox(const QString& name)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a scrollbox cannot have an empty string as its name")};
    }
    const auto it = mScrollBoxes.find(name);
    if (it == mScrollBoxes.end()) {
        if (mpHost->windowRegistry().hasScrollBox(name)) {
            // One the current view has no record of, such as a detached view's
            return {false, qsl("scrollbox name '%1' could not be deleted").arg(name)};
        }
        return {false, qsl("scrollbox name '%1' not found").arg(name)};
    }
    const std::shared_ptr<ScrollBox> box = it->second;
    if (box->holder) {
        box->holder->deleteLater();
    }
    mScrollBoxes.erase(it);
    mpHost->windowRegistry().deregisterScrollBox(name);
    republishPlainWindow(name);
    // TMainConsole::deleteScrollBox() calls deleteLater(), so what the box holds stays until then
    auto pRelease = new QObject();
    QObject::connect(pRelease, &QObject::destroyed, mpHost, [this, box]() {
        removeScrollBoxContents(box.get());
    });
    pRelease->deleteLater();

    TEvent event{};
    event.mArgumentList.append(qsl("sysScrollBoxDeleted"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
    return {true, QString()};
}

bool TNullConsoleFrontend::within(const ScrollBox* container, const ScrollBox* box)
{
    for (; container; container = container->box.get()) {
        if (container == box) {
            return true;
        }
    }
    return false;
}

void TNullConsoleFrontend::removeScrollBoxContents(const ScrollBox* box)
{
    const auto inside = [box](const ScrollBox* container) {
        return within(container, box);
    };
    QStringList contents;
    for (const auto& [name, label] : mLabels) {
        if (inside(label.box.get())) {
            contents << name;
        }
    }
    for (const QString& name : contents) {
        removeLabel(name);
    }
    contents.clear();
    for (const auto& [name, subConsole] : mSubConsoles) {
        if (inside(subConsole.box.get())) {
            contents << name;
        }
    }
    for (const QString& name : contents) {
        removeSubConsole(name);
    }
    contents.clear();
    std::erase_if(mCommandLines, [this, &inside, &contents](const auto& entry) {
        if (!inside(entry.second.box.get())) {
            return false;
        }
        releaseCommandLine(entry.first, entry.second);
        contents << entry.first;
        return true;
    });
    TWindowRegistry& registry = mpHost->windowRegistry();
    std::erase_if(mTextBoxes, [&registry, &inside, &contents](const auto& entry) {
        if (!inside(entry.second.box.get())) {
            return false;
        }
        registry.deregisterTextBox(entry.first);
        contents << entry.first;
        return true;
    });
    std::erase_if(mScrollBoxes, [&registry, &inside, &contents](const auto& entry) {
        if (!inside(entry.second->box.get())) {
            return false;
        }
        registry.deregisterScrollBox(entry.first);
        contents << entry.first;
        return true;
    });
    for (const QString& name : contents) {
        republishPlainWindow(name);
    }
}

bool TNullConsoleFrontend::setPlainWindowShown(const QString& name, const bool shown)
{
    mWindowNotifier.note();
    // As for a label
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        if (it->second->shown != shown) {
            it->second->shown = shown;
            reportVisibility(QString(), it->second.get());
        }
        return true;
    }
    PlainWindow* window = plainWindow(name);
    if (!window) {
        return false;
    }
    if (window->shown != shown) {
        window->shown = shown;
        reportWindowVisibility(name);
    }
    return true;
}

bool TNullConsoleFrontend::resizePlainWindow(const QString& name, const int width, const int height)
{
    mWindowNotifier.note();
    const bool box = mScrollBoxes.contains(name);
    PlainWindow* window = box ? nullptr : plainWindow(name);
    if (!box && !window) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.plainWindowGeometry(name).value_or(QRect());
    // QWidget::resize() bounds a size only for a widget not yet created or with size limits, and a
    // scroll box is created once made and has no limits, where a command line or text box has them
    geometry.setSize(box ? QSize(width, height) : window->limits.bounded(widgetSize(width, height)));
    registry.setPlainWindowGeometry(name, geometry);
    if (window) {
        window->geometry = geometry;
    }
    return true;
}

bool TNullConsoleFrontend::movePlainWindow(const QString& name, const int x, const int y)
{
    mWindowNotifier.note();
    const bool box = mScrollBoxes.contains(name);
    PlainWindow* window = box ? nullptr : plainWindow(name);
    if (!box && !window) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.plainWindowGeometry(name).value_or(QRect());
    geometry.moveTo(x, y);
    registry.setPlainWindowGeometry(name, geometry);
    if (window) {
        window->geometry = geometry;
    }
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::createCommandLine(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a commandLine cannot have an empty string as its name")};
    }
    if (mpHost->parentWindowMissing(windowname)) {
        return {false, qsl("window '%1' not found").arg(windowname)};
    }
    if (mpHost->windowRegistry().hasLabel(name)) {
        return {false, qsl("label with the name '%1' already exists").arg(name)};
    }
    if (!ownsContainer(windowname)) {
        return noView();
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.hasCommandLine(name)) {
        return {false, qsl("couldn't create commandLine")};
    }
    PlainWindow& commandLine = mCommandLines[name];
    // TMainConsole::parentWidgetFor() takes a scroll box before a user window
    std::tie(commandLine.userWindow, commandLine.box) = containerFor(windowname, true);
    commandLine.geometry = QRect(QPoint(x, y), widgetSize(width, height));
    commandLine.stackOrder = ++mTopOfStack;
    // TCommandLine's constructor takes the file and the setting a profile save last wrote for the name
    const auto [fileName, savesHistory] = mpHost->getCmdLineSettings(enums::SubCommandLine, name);
    commandLine.writer = makeHistoryWriter(holderFor(commandLine.userWindow, commandLine.box.get()), name, fileName, savesHistory);
    registry.registerCommandLine(name);
    registry.setCommandLineSavesHistory(name, savesHistory);
    registry.setCommandLineStyleSheet(name, commandLineSheet(mpHost->mCommandLineBgColor));
    // TMainConsole::plainWindowWidget() takes a scroll box of the same name first
    if (!mScrollBoxes.contains(name)) {
        registry.setPlainWindowGeometry(name, commandLine.geometry);
    }
    reportWindowVisibility(name);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::deleteCommandLine(const QString& name)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a command line cannot have an empty string as its name")};
    }
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        if (mpHost->windowRegistry().hasCommandLine(name)) {
            // One the current view has no record of, such as a detached view's
            return {false, qsl("command line name '%1' could not be deleted").arg(name)};
        }
        return {false, qsl("command line name '%1' not found").arg(name)};
    }
    // TMainConsole::deleteCommandLine() deletes it later
    if (it->second.writer) {
        it->second.writer->deleteLater();
    }
    releaseCommandLine(name, it->second);
    mCommandLines.erase(it);
    republishPlainWindow(name);

    TEvent event{};
    event.mArgumentList.append(qsl("sysCommandLineDeleted"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
    return {true, QString()};
}

bool TNullConsoleFrontend::setCommandLineAction(const QString& name, const int func)
{
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        return false;
    }
    // As TCommandLine::releaseFunc()
    if (it->second.action && it->second.action != func) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(it->second.action);
    }
    it->second.action = func;
    return true;
}

bool TNullConsoleFrontend::resetCommandLineAction(const QString& name)
{
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        return false;
    }
    if (it->second.action) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(it->second.action);
        it->second.action = 0;
    }
    return true;
}

// Its Lua function is freed as a label's is, where TCommandLine leaves it to the Lua state's close
void TNullConsoleFrontend::releaseCommandLine(const QString& name, const PlainWindow& commandLine)
{
    if (commandLine.action) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(commandLine.action);
    }
    mpHost->windowRegistry().deregisterCommandLine(name);
}

// The QObject standing for the widget a window made there would be a child of
QObject* TNullConsoleFrontend::holderFor(const QString& userWindow, const ScrollBox* box) const
{
    if (box && box->holder) {
        return box->holder;
    }
    if (const auto it = mSubConsoles.find(userWindow); it != mSubConsoles.end() && it->second.holder) {
        return it->second.holder;
    }
    return mpMainHolder.get();
}

// Connected as the TCommandLine would be, so a save writes in the order the GUI's do: names that differ only
// in a '/' or '\' share a key
TNullConsoleFrontend::HistoryWriter* TNullConsoleFrontend::makeHistoryWriter(QObject* parent, const QString& name, const QString& fileName, const bool savesHistory)
{
    auto pWriter = new HistoryWriter(parent);
    pWriter->savesHistory = savesHistory;
    pWriter->fileName = fileName;
    Host* pHost = mpHost;
    QString error;
    if (!TCommandHistory::read(MudletApp::getMudletPath(enums::profileDataItemPath, pHost->getName(), fileName), pWriter->history, error)) {
        pWriter->readFailed = true;
        pWriter->readError = error;
        // A read that failed part-way holds the lines before the failure, which are not to be offered either
        pWriter->history.clear();
        qWarning().nospace().noquote() << "TNullConsoleFrontend::makeHistoryWriter() WARNING - unable to read the command history of the command line called: " << name << " reason: " << error;
    }
    QObject::connect(mpHost, &Host::signal_saveCommandLinesHistory, pWriter, [pHost, pWriter, name]() {
        pHost->setCmdLineSettings(enums::SubCommandLine, pWriter->savesHistory, name);
        // As TCommandLine::slot_saveHistory(), which leaves a file it no longer writes as it was
        const int saveSize = pHost->getCommandLineHistorySaveSize();
        if (!saveSize || !pWriter->savesHistory) {
            return;
        }
        if (pWriter->readFailed) {
            qWarning().nospace().noquote() << "TNullConsoleFrontend::makeHistoryWriter() WARNING - not saving the command history of the command line called: " << name
                                           << ", as its file could not be read";
            return;
        }
        QString writeError;
        if (!TCommandHistory::write(MudletApp::getMudletPath(enums::profileDataItemPath, pHost->getName(), pWriter->fileName), pWriter->history, saveSize, writeError)) {
            qWarning().nospace().noquote() << "TNullConsoleFrontend::makeHistoryWriter() WARNING - unable to save the command history of the command line called: " << name
                                           << " reason: " << writeError;
        }
    });
    return pWriter;
}

void TNullConsoleFrontend::addMainCommandLineHistory(const QString& sent)
{
    // The line as it stands, which an alias that clears it leaves nothing of
    const QString command = mMainCommandLineRewritten ? mpHost->windowRegistry().commandLineText(QString()).value_or(QString()) : sent;
    mMainCommandLineRewritten = false;
    // As TCommandLine::enterCommand(), past the hidden-input box
    if (command.isEmpty() || !mMainCommandLine.writer || (mpHost->isRemoteEchoingActive() && !mpHost->disablePasswordMasking())) {
        return;
    }
    TCommandHistory::add(mMainCommandLine.writer->history, command);
}

QStringList TNullConsoleFrontend::mainCommandLineHistory() const
{
    return mMainCommandLine.writer ? mMainCommandLine.writer->history : QStringList();
}

std::optional<QString> TNullConsoleFrontend::mainCommandLineHistoryReadError() const
{
    if (!mMainCommandLine.writer || !mMainCommandLine.writer->readFailed) {
        return std::nullopt;
    }
    return mMainCommandLine.writer->readError;
}

// In the order TMainConsole::plainWindowWidget() takes them after a scroll box
TNullConsoleFrontend::PlainWindow* TNullConsoleFrontend::plainWindow(const QString& name)
{
    if (const auto it = mCommandLines.find(name); it != mCommandLines.end()) {
        return &it->second;
    }
    if (const auto it = mTextBoxes.find(name); it != mTextBoxes.end()) {
        return &it->second;
    }
    return nullptr;
}

// What the registry has for a name a window gave up, which TMainConsole::plainWindowWidget()
// then finds another window for, and as TMainConsole::reportWindowState() reports the name to whatever has it
void TNullConsoleFrontend::republishPlainWindow(const QString& name)
{
    mWindowNotifier.note();
    if (!mScrollBoxes.contains(name)) {
        if (const PlainWindow* window = plainWindow(name)) {
            mpHost->windowRegistry().setPlainWindowGeometry(name, window->geometry);
        }
    }
    if (mpDeferredReports) {
        *mpDeferredReports << name;
    } else {
        reportWindowVisibility(name);
    }
}

// As TMainConsole::commandLineNamed() takes them
static bool mainCommandLine(const QString& name)
{
    return name.isEmpty() || name == QLatin1String("main");
}

bool TNullConsoleFrontend::ownsCommandLine(const QString& name) const
{
    return mainCommandLine(name) || mCommandLines.contains(name);
}

TNullConsoleFrontend::PlainWindow* TNullConsoleFrontend::commandLineNamed(const QString& name)
{
    if (mainCommandLine(name)) {
        return &mMainCommandLine;
    }
    const auto it = mCommandLines.find(name);
    return it == mCommandLines.end() ? nullptr : &it->second;
}

// What QPlainTextEdit::toPlainText() gives back, which turns line breaks into newlines and
// non-breaking spaces into spaces
static QString plainText(const QString& text)
{
    QTextDocument document;
    document.setPlainText(text);
    return document.toPlainText();
}

bool TNullConsoleFrontend::replaceCommandLineText(const QString& name, const QString& text)
{
    if (!ownsCommandLine(name)) {
        return false;
    }
    if (!mainCommandLine(name)) {
        mpHost->windowRegistry().setCommandLineText(name, plainText(text));
        // Told even where Mudlet's copy is unchanged, as the app's line may hold what it has yet to pass on
        mWindowNotifier.noteCommandLine(name);
    } else if (!mpHost->passwordEntryWanted()) {
        // TMainConsole writes to the hidden-input box over it while the game asks for one
        mMainCommandLineTyped = false;
        mMainCommandLineAllSelected = false;
        mMainCommandLineRewritten = true;
        mMainCommandLineWhollySelected = false;
        endMainCommandLineCompletion();
        const QString plain = plainText(text);
        mpHost->windowRegistry().setMainCommandLineText(plain);
        emit mpHost->signal_commandLineTextRequested(plain);
    }
    return true;
}

void TNullConsoleFrontend::typeCommandLine(const QString& name, const QString& text)
{
    mpHost->windowRegistry().setCommandLineText(name, plainText(text));
}

bool TNullConsoleFrontend::commandLineShown(const QString& name) const
{
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        return false;
    }
    const PlainWindow& line = it->second;
    const auto console = mSubConsoles.find(name);
    const bool own = line.inConsole && console != mSubConsoles.end() && ownsLine(console->second, line);
    return line.shown && line.layerShown && line.consoleShown && (!own || console->second.shown) && shownWithin(line.userWindow, line.box.get());
}

std::optional<int> TNullConsoleFrontend::commandLineAction(const QString& name) const
{
    const auto it = mCommandLines.find(name);
    if (mainCommandLine(name) || it == mCommandLines.end()) {
        return std::nullopt;
    }
    return it->second.action;
}

const QObject* TNullConsoleFrontend::commandLineInstance(const QString& name) const
{
    const auto it = mCommandLines.find(name);
    return mainCommandLine(name) || it == mCommandLines.end() ? nullptr : it->second.writer.data();
}

void TNullConsoleFrontend::echoSentCommandLine(const QString& name, QString command)
{
    const auto it = mCommandLines.find(name);
    TConsoleModel* pModel = subConsoleModel(name);
    if (it == mCommandLines.end() || !it->second.inConsole || it->second.action || !pModel || mpHost->mCommandEchoMode == Host::CommandEchoMode::Never) {
        return;
    }
    pModel->printCommand(command);
}

void TNullConsoleFrontend::commandLineSent(const QString& name)
{
    const auto it = mCommandLines.find(name);
    if (mainCommandLine(name) || it == mCommandLines.end()) {
        return;
    }
    // The line as it stands, which a script that clears it leaves nothing of
    const QString text = mpHost->windowRegistry().commandLineText(name).value_or(QString());
    if (!text.isEmpty() && it->second.writer && (!mpHost->isRemoteEchoingActive() || mpHost->disablePasswordMasking())) {
        TCommandHistory::add(it->second.writer->history, text);
    }
    if (mpHost->mAutoClearCommandLineAfterSend) {
        replaceCommandLineText(name, QString());
    }
}

bool TNullConsoleFrontend::holdMainCommandLineForSend(const QString& line)
{
    mMainCommandLineRewritten = false;
    // A line typed while the game hides input went through the hidden-input box, past the main line
    if (mpHost->passwordEntryWanted()) {
        return false;
    }
    mMainCommandLineTyped = false;
    mMainCommandLineAllSelected = false;
    mMainCommandLineWhollySelected = false;
    endMainCommandLineCompletion();
    mpHost->windowRegistry().setMainCommandLineText(plainText(line));
    return true;
}

void TNullConsoleFrontend::mainCommandLineSent()
{
    // TCommandLine::enterCommand() selects a line it keeps
    if (!mpHost->mAutoClearCommandLineAfterSend) {
        mMainCommandLineAllSelected = true;
        mMainCommandLineWhollySelected = true;
        return;
    }
    if (mpHost->passwordEntryWanted()) {
        mMainCommandLineTyped = false;
        // The app's line is set aside until hidden input ends, as the GUI's line is behind its box. Only set here, as
        // a send that began inside this one's aliases may have emptied the line already.
        if (!mpHost->windowRegistry().commandLineText(QString()).value_or(QString()).isEmpty()) {
            mMainCommandLineClearPending = true;
        }
        mpHost->windowRegistry().setMainCommandLineText(QString());
    } else {
        replaceCommandLineText(QString(), QString());
    }
}

void TNullConsoleFrontend::setCommandLineText(const QString& text)
{
    replaceCommandLineText(QString(), text);
    // As TMainConsole::setCommandLineText()
    if (!mpHost->passwordEntryWanted()) {
        mMainCommandLineAllSelected = true;
        mMainCommandLineWhollySelected = true;
    }
}

bool TNullConsoleFrontend::selectCommandLineText(const QString& name)
{
    if (!ownsCommandLine(name)) {
        return false;
    }
    if (mainCommandLine(name) && !mpHost->passwordEntryWanted()) {
        mMainCommandLineAllSelected = true;
        mMainCommandLineWhollySelected = true;
    }
    return true;
}

void TNullConsoleFrontend::typeMainCommandLine(const QString& text, const bool playerTyped)
{
    const QString plain = plainText(text);
    // Mudlet sees each new line, never the key that made it, so any change ends Tab's cycle and the history walk, as
    // Backspace does on the GUI's line; the line a completion left, handed back, is no change
    if (plain != mpHost->windowRegistry().commandLineText(QString()).value_or(QString()) && plain != mMainCommandLineCompleted) {
        endMainCommandLineCompletion();
        // The GUI's line shows a recall all selected
        mMainCommandLineWhollySelected = !playerTyped;
    }
    writeMainCommandLine(plain, playerTyped);
}

void TNullConsoleFrontend::writeMainCommandLine(const QString& plain, const bool playerTyped)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    const QString current = registry.commandLineText(QString()).value_or(QString());
    // As TCommandLine::slot_contentsChange(): typing on into a line a script set leaves it the script's, but typing
    // over it replaces it with a line of the player's. Mudlet does not see the app's caret or selection, so any
    // change to a line the GUI would have selected counts, as does one that no longer starts with the line, erring
    // towards moving it into hidden input.
    if (plain.isEmpty() || !playerTyped) {
        mMainCommandLineTyped = false;
    } else if (current.isEmpty() || !plain.startsWith(current) || (mMainCommandLineAllSelected && plain != current)) {
        mMainCommandLineTyped = true;
    }
    // So typing into it counts, erring towards secrecy as for a line the GUI would have selected
    if (!playerTyped) {
        mMainCommandLineAllSelected = true;
    } else if (plain != current) {
        mMainCommandLineAllSelected = false;
    }
    mMainCommandLineRewritten = true;
    registry.setMainCommandLineText(plain);
}

void TNullConsoleFrontend::endMainCommandLineCompletion()
{
    mMainCommandLineCompletion.reset();
    mMainCommandLineCompletion.resetHistoryWalk();
    mMainCommandLineCompleted.reset();
}

std::optional<QString> TNullConsoleFrontend::tabCompleteMainCommandLine(const bool forward)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    const auto completion = mMainCommandLineCompletion.tabComplete(
            registry.commandLineText(QString()).value_or(QString()), forward, mpHost->mainConsoleModel().buffer, mMainCommandLine.suggestions, mMainCommandLine.blacklist);
    if (!completion) {
        return std::nullopt;
    }
    writeMainCommandLine(plainText(completion->line), false);
    mMainCommandLineWhollySelected = false;
    const QString shown = registry.commandLineText(QString()).value_or(QString());
    mMainCommandLineCompletion.setShownLine(shown);
    mMainCommandLineCompleted = shown;
    return shown;
}

std::optional<QString> TNullConsoleFrontend::historyCompleteMainCommandLine()
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    const QStringList history = mainCommandLineHistory();
    const QString line = registry.commandLineText(QString()).value_or(QString());
    // TCommandLine::historyMove() recalls a whole line instead for an empty line or none in the history
    if (line.isEmpty() || history.isEmpty()) {
        return std::nullopt;
    }
    const auto completion = mMainCommandLineCompletion.historyComplete(line, true, history, true);
    if (!completion.line && !completion.tailCut) {
        return std::nullopt;
    }
    // Only Tab's cycle: the walk goes on from here
    mMainCommandLineCompletion.reset();
    writeMainCommandLine(plainText(completion.line.value_or(completion.typed)), false);
    mMainCommandLineWhollySelected = false;
    const QString shown = registry.commandLineText(QString()).value_or(QString());
    if (completion.line) {
        mMainCommandLineCompletion.setHistoryCompletion(shown);
    }
    mMainCommandLineCompleted = shown;
    return shown;
}

bool TNullConsoleFrontend::hiddenInputOpened()
{
    // Only true inside the recompute that a WILL ECHO makes, which emits the change this runs from
    if (!mpHost->passwordEntryOpensWithThePrompt() || !mMainCommandLineTyped) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    bool typedAhead = false;
    for (const QChar character : registry.commandLineText(QString()).value_or(QString())) {
        if (character != QChar::CarriageReturn && character != QChar::LineFeed) {
            typedAhead = true;
            break;
        }
    }
    if (!typedAhead) {
        return false;
    }
    mMainCommandLineTyped = false;
    mMainCommandLineAllSelected = false;
    registry.wipeMainCommandLineText();
    mpHost->passwordEntryEdited();
    return true;
}

void TNullConsoleFrontend::hiddenInputClosed()
{
    if (mMainCommandLineClearPending) {
        mMainCommandLineClearPending = false;
        emit mpHost->signal_commandLineTextRequested(QString());
    }
}

bool TNullConsoleFrontend::appendCommandLineText(const QString& name, const QString& text)
{
    if (!ownsCommandLine(name)) {
        return false;
    }
    return replaceCommandLineText(name, mpHost->windowRegistry().commandLineText(name).value_or(QString()) + text);
}

bool TNullConsoleFrontend::addCommandLineMenuItem(const QString& name, const QString& label, const QString& eventName)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->menuItems.insert_or_assign(label, eventName);
    return true;
}

std::optional<bool> TNullConsoleFrontend::removeCommandLineMenuItem(const QString& name, const QString& label)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return std::nullopt;
    }
    return pCommandLine->menuItems.erase(label) != 0;
}

bool TNullConsoleFrontend::addCommandLineSuggestion(const QString& name, const QString& word)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->suggestions.insert(word);
    return true;
}

bool TNullConsoleFrontend::removeCommandLineSuggestion(const QString& name, const QString& word)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->suggestions.remove(word);
    return true;
}

bool TNullConsoleFrontend::clearCommandLineSuggestions(const QString& name)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->suggestions.clear();
    return true;
}

bool TNullConsoleFrontend::addCommandLineBlacklistWord(const QString& name, const QString& word)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->blacklist.insert(word);
    return true;
}

bool TNullConsoleFrontend::removeCommandLineBlacklistWord(const QString& name, const QString& word)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->blacklist.remove(word);
    return true;
}

bool TNullConsoleFrontend::clearCommandLineBlacklist(const QString& name)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    pCommandLine->blacklist.clear();
    return true;
}

bool TNullConsoleFrontend::setCommandLineSavesHistory(const QString& name, const bool savesHistory)
{
    PlainWindow* pCommandLine = commandLineNamed(name);
    if (!pCommandLine) {
        return false;
    }
    if (pCommandLine->writer) {
        pCommandLine->writer->savesHistory = savesHistory;
    }
    if (pCommandLine == &mMainCommandLine) {
        mpHost->windowRegistry().setMainCommandLineSavesHistory(savesHistory);
    } else {
        mpHost->windowRegistry().setCommandLineSavesHistory(name, savesHistory);
    }
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::setCmdLineStyleSheet(const QString& name, const QString& styleSheet)
{
    // The main window's layout sizes the main command line, which is not modelled, so its sheet sets no limits here
    if (mainCommandLine(name)) {
        mpHost->windowRegistry().setMainCommandLineStyleSheet(styleSheet);
        return {true, QString()};
    }
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        return {false, qsl("command-line name '%1' not found").arg(name)};
    }
    it->second.styleSheet = styleSheet;
    mpHost->windowRegistry().setCommandLineStyleSheet(name, styleSheet);
    restylePlainWindow(name, it->second, true);
    return {true, QString()};
}

// TConsole::setCmdVisible() makes a console's own command line the first time it is shown, and
// TMainConsole::registerSubCommandLine() puts it in place of any other command line of the name
bool TNullConsoleFrontend::setWindowCommandLineVisible(const QString& name, const bool visible)
{
    mWindowNotifier.note();
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end()) {
        return false;
    }
    const auto it = mCommandLines.find(name);
    if (it != mCommandLines.end() && ownsLine(console->second, it->second)) {
        it->second.layerShown = visible;
        // As QWidget::setVisible() to what the command line already is, which reports nothing
        if (it->second.shown != visible) {
            it->second.shown = visible;
            reportWindowVisibility(name);
        }
        // As TConsole::setCmdVisible() resizes the console either way
        reportGridSize(name);
        resizePane(name);
        return true;
    }
    // A deleted one is still held until deferred deletes run, and nothing is made in its place
    if (console->second.ownLine || !visible) {
        return true;
    }
    if (it != mCommandLines.end()) {
        // Deregistered, not deleted, so its writer and Lua function stay
        mpHost->windowRegistry().deregisterCommandLine(name);
        mCommandLines.erase(it);
    }
    const auto [fileName, savesHistory] = mpHost->getCmdLineSettings(enums::ConsoleCommandLine, name);
    PlainWindow& commandLine = mCommandLines[name];
    commandLine.inConsole = true;
    commandLine.stackOrder = ++mTopOfStack;
    commandLine.userWindow = console->second.userWindow;
    commandLine.box = console->second.box;
    // A child of the console, as TConsole::setCmdVisible() makes it
    commandLine.writer = makeHistoryWriter(console->second.holder ? console->second.holder.data() : mpMainHolder.get(), name, fileName, savesHistory);
    console->second.ownLine = commandLine.writer;
    TWindowRegistry& registry = mpHost->windowRegistry();
    registry.registerCommandLine(name);
    registry.setCommandLineSavesHistory(name, savesHistory);
    registry.setCommandLineStyleSheet(name, commandLineSheet(mpHost->mCommandLineBgColor));
    // As TMainConsole::registerSubCommandLine() reports the name, and so the console's own window
    reportWindowVisibility(name);
    reportGridSize(name);
    resizePane(name);
    return true;
}

// Unlike showWindow(), this finds the command line when a scroll box has the same name
bool TNullConsoleFrontend::setCommandLineVisible(const QString& name, const bool visible)
{
    mWindowNotifier.note();
    const auto it = mCommandLines.find(name);
    if (it == mCommandLines.end()) {
        return false;
    }
    // As QWidget::setVisible() to what the command line already is, which reports nothing
    if (it->second.shown != visible) {
        it->second.shown = visible;
        reportWindowVisibility(name);
    }
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::createTextBox(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a text edit cannot have an empty string as its name")};
    }
    if (mpHost->parentWindowMissing(windowname)) {
        return {false, qsl("window '%1' not found").arg(windowname)};
    }
    if (mpHost->windowRegistry().hasLabel(name)) {
        return {false, qsl("label with the name '%1' already exists").arg(name)};
    }
    if (!ownsContainer(windowname)) {
        return noView();
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.hasTextBox(name)) {
        return {false, qsl("couldn't create text edit")};
    }
    PlainWindow& textBox = mTextBoxes[name];
    std::tie(textBox.userWindow, textBox.box) = containerFor(windowname, true);
    textBox.geometry = QRect(QPoint(x, y), widgetSize(width, height));
    textBox.stackOrder = ++mTopOfStack;
    registry.registerTextBox(name);
    if (!mScrollBoxes.contains(name) && !mCommandLines.contains(name)) {
        registry.setPlainWindowGeometry(name, textBox.geometry);
        reportWindowVisibility(name);
    }
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::deleteTextBox(const QString& name)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a text edit cannot have an empty string as its name")};
    }
    const auto it = mTextBoxes.find(name);
    if (it == mTextBoxes.end()) {
        if (mpHost->windowRegistry().hasTextBox(name)) {
            // One the current view has no record of, such as a detached view's
            return {false, qsl("text edit name '%1' could not be deleted").arg(name)};
        }
        return {false, qsl("text edit name '%1' not found").arg(name)};
    }
    mTextBoxes.erase(it);
    mpHost->windowRegistry().deregisterTextBox(name);
    republishPlainWindow(name);

    TEvent event{};
    event.mArgumentList.append(qsl("sysTextEditDeleted"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
    return {true, QString()};
}

bool TNullConsoleFrontend::setTextBoxText(const QString& name, const QString& text)
{
    if (!mTextBoxes.contains(name)) {
        return false;
    }
    mpHost->windowRegistry().setTextBoxText(name, plainText(text));
    return true;
}

bool TNullConsoleFrontend::setTextBoxFont(const QString& name, const QFont& font)
{
    mWindowNotifier.note();
    if (!mTextBoxes.contains(name)) {
        return false;
    }
    mpHost->windowRegistry().setTextBoxFont(name, font);
    return true;
}

bool TNullConsoleFrontend::setTextBoxReadOnly(const QString& name, const bool readOnly)
{
    const auto it = mTextBoxes.find(name);
    if (it == mTextBoxes.end()) {
        return false;
    }
    it->second.readOnly = readOnly;
    return true;
}

bool TNullConsoleFrontend::setTextBoxStyleSheet(const QString& name, const QString& styleSheet)
{
    const auto it = mTextBoxes.find(name);
    if (it == mTextBoxes.end()) {
        return false;
    }
    it->second.styleSheet = styleSheet;
    restylePlainWindow(name, it->second, false);
    return true;
}

// Only the sheet's size limits, which QStyleSheetStyle::setGeometry() sets for the enabled state and
// whether a text edit is read-only, which a command line never is
void TNullConsoleFrontend::restylePlainWindow(const QString& name, PlainWindow& window, const bool commandLine)
{
    mWindowNotifier.note();
    const QString declarations = sheetDeclarations(window.styleSheet, plainWindowSheetChain(name, window, commandLine), {qsl("enabled"), window.readOnly ? qsl("read-only") : qsl("editable")});
    // QFusionStyle's PM_DefaultFrameWidth, which a native border takes
    const SheetGeometry geometry = sheetGeometry(declarations, sheetLengthFont(declarations), 1);
    window.limits = {geometry.minimum, geometry.maximum};
    window.geometry.setSize(window.limits.bounded(window.geometry.size()));
    if (!mScrollBoxes.contains(name) && plainWindow(name) == &window) {
        mpHost->windowRegistry().setPlainWindowGeometry(name, window.geometry);
    }
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelStyleSheet(const QString& name, const QString& stylesheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    restyleLabel(it->second, stylesheet);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelToolTip(const QString& name, const QString& text, const double duration)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    pLabel->setToolTip(text);
    mLabels.at(name).toolTipDuration = duration;
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCursor(const QString& name, const int shape)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    if (!labelModel(name)) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    // The shapes Qt::CursorShape has, bar the bitmap ones, and -1 to unset it
    if (shape < -1 || shape > 21) {
        return {false, qsl("cursor shape '%1' not found. see https://doc.qt.io/qt-5/qt.html#CursorShape-enum").arg(shape)};
    }
    Label& label = mLabels.at(name);
    label.cursor = shape == -1 ? std::nullopt : std::optional(static_cast<Qt::CursorShape>(shape));
    label.customCursor.clear();
    label.customCursorHotSpot = QPoint();
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCustomCursor(const QString& name, const QString& pixMapLocation, const int hotX, const int hotY)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    if (pixMapLocation.isEmpty()) {
        return {false, qsl("custom cursor location cannot be an empty string")};
    }
    if (!labelModel(name)) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    // A QPixmap needs a GUI application; it loads files through QImage regardless
    if (QImage(pixMapLocation).isNull()) {
        return {false, qsl("couldn't find custom cursor, is the location \"%1\" correct?").arg(pixMapLocation)};
    }
    Label& label = mLabels.at(name);
    label.cursor = Qt::BitmapCursor;
    label.customCursor = pixMapLocation;
    label.customCursorHotSpot = QPoint(hotX, hotY);
    return {true, QString()};
}

bool TNullConsoleFrontend::setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, const bool underline)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    // TLabel sets the link colours in its palette, which the next restyle has to override
    it->second.restyleDue = true;
    TLabelModel* pLabel = it->second.pModel.get();
    pLabel->mLinkColor = linkColor;
    pLabel->mLinkVisitedColor = linkVisitedColor;
    pLabel->mLinkUnderline = underline;
    reapplyLabelLinkStyle(it->second);
    return true;
}

bool TNullConsoleFrontend::resetLabelLinkStyle(const QString& name)
{
    return setLabelLinkStyle(name, QString(), QString(), true);
}

bool TNullConsoleFrontend::clearLabelVisitedLinks(const QString& name)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.pModel->mVisitedLinks.clear();
    reapplyLabelLinkStyle(it->second);
    return true;
}

// QWidget::resize() takes a negative size as nothing
bool TNullConsoleFrontend::resizeLabel(const QString& name, const int width, const int height)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    setLabelSize(it->second, it->second.limits.bounded(widgetSize(width, height)));
    return true;
}

void TNullConsoleFrontend::setLabelSize(Label& label, const QSize& size)
{
    mWindowNotifier.note();
    if (size == label.pModel->mGeometry.size()) {
        return;
    }
    label.pModel->mGeometry.setSize(size);
    // QWidget holds a hidden widget's resize event, and so TLabel::resized, back until it shows
    if (!label.pModel->mVisible) {
        label.resizePending = true;
    } else if (label.movieFollowsSize) {
        label.movie->setScaledSize(size);
    }
}

void TNullConsoleFrontend::setLabelVisible(Label& label, const bool visible)
{
    label.pModel->mVisible = visible;
    if (visible && label.resizePending) {
        label.resizePending = false;
        if (label.movieFollowsSize) {
            label.movie->setScaledSize(label.pModel->mGeometry.size());
        }
    }
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelMovie(const QString& name, const QString& moviePath)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    // Host::setMovie() found the name registered, so the label belongs to a detached view
    if (it == mLabels.end()) {
        return noView();
    }
    if (const QMovie candidate(moviePath); !candidate.isValid()) {
        return {false, qsl("no valid movie found at '%1'").arg(moviePath)};
    }
    Label& label = it->second;
    if (!label.movie) {
        label.movie = std::make_unique<QMovie>();
        // A movie that plays to its end stops by itself
        QObject::connect(label.movie.get(), &QMovie::stateChanged, &mWindowNotifier, [this]() {
            mWindowNotifier.note();
        });
        mpHost->getGifTracker()->registerGif(label.movie.get());
        label.movie->setCacheMode(QMovie::CacheAll);
    }
    label.movie->setFileName(moviePath);
    label.movie->stop();
    // QLabel::setMovie() clears the text and leaves the label measuring the movie
    label.showsMovie = true;
    label.pixmapSize.reset();
    label.textLabel = false;
    label.sizeHint.reset();
    label.pModel->mText.clear();
    updateShownContent(label);
    label.movie->start();
    return {true, QString()};
}

// QLabel::clear()
void TNullConsoleFrontend::clearLabelContents(Label& label)
{
    mWindowNotifier.note();
    label.pixmapSize.reset();
    label.showsMovie = false;
    label.textLabel = false;
    label.sizeHint.reset();
    label.pModel->mText.clear();
}

bool TNullConsoleFrontend::setLabelBackgroundImage(const QString& name, const QString& path)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    Label& label = it->second;
    if (TLabelModel::svgCandidate(path)) {
        QSvgRenderer renderer;
        if (TLabelModel::loadSvg(renderer, path)) {
            label.svgSize = renderer.defaultSize();
            label.svgPath = path;
            // TLabel::setSvgImage() takes down a raster background but leaves text and movies
            if (label.pixmapSize) {
                clearLabelContents(label);
            }
            updateShownContent(label);
            return true;
        }
    }
    const QImage raster(path);
    // TLabel::setBackgroundImage() takes an empty path to mean no image
    if (raster.isNull() && !path.isEmpty()) {
        return false;
    }
    label.svgSize.reset();
    if (label.showsMovie) {
        label.movie->stop();
    }
    clearLabelContents(label);
    if (!raster.isNull()) {
        label.pixmapSize = raster.deviceIndependentSize().toSize();
        label.pixmapPath = path;
    }
    updateShownContent(label);
    return true;
}

bool TNullConsoleFrontend::resetLabelBackgroundImage(const QString& name)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    Label& label = it->second;
    label.svgSize.reset();
    // Text is not an image, so it stays
    if (label.pixmapSize || label.showsMovie) {
        if (label.showsMovie) {
            label.movie->stop();
        }
        clearLabelContents(label);
    }
    if (const QString withoutImage = TLabelModel::styleSheetWithoutBackgroundImage(label.pModel->mStyleSheet); withoutImage != label.pModel->mStyleSheet) {
        restyleLabel(label, withoutImage);
    }
    updateShownContent(label);
    return true;
}

// TConsole::setWindowBackgroundImage() loads a 'cover' image to scale it, and leaves the other modes to a stylesheet
bool TNullConsoleFrontend::setWindowBackgroundImage(const QString& imgPath, const int mode)
{
    if (mode == 5) {
        return !QImage(imgPath).isNull();
    }
    return mode >= 1 && mode <= 4;
}

QMovie* TNullConsoleFrontend::shownMovie(const QString& name) const
{
    const auto it = mLabels.find(name);
    return it != mLabels.end() && it->second.showsMovie ? it->second.movie.get() : nullptr;
}

std::optional<bool> TNullConsoleFrontend::labelShowsMovie(const QString& name) const
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return std::nullopt;
    }
    return {it->second.showsMovie};
}

bool TNullConsoleFrontend::startLabelMovie(const QString& name)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    mWindowNotifier.note();
    pMovie->start();
    return true;
}

bool TNullConsoleFrontend::pauseLabelMovie(const QString& name)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    mWindowNotifier.note();
    pMovie->setPaused(true);
    return true;
}

bool TNullConsoleFrontend::setLabelMovieFrame(const QString& name, const int frame)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    mWindowNotifier.note();
    return pMovie->jumpToFrame(frame);
}

bool TNullConsoleFrontend::setLabelMovieSpeed(const QString& name, const int percent)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    mWindowNotifier.note();
    pMovie->setSpeed(percent);
    return true;
}

bool TNullConsoleFrontend::scaleLabelMovie(const QString& name, const bool followLabelSize)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    mWindowNotifier.note();
    Label& label = mLabels.at(name);
    pMovie->setScaledSize(label.pModel->mGeometry.size());
    label.movieFollowsSize = followLabelSize;
    return true;
}

bool TNullConsoleFrontend::moveLabel(const QString& name, const int x, const int y)
{
    mWindowNotifier.note();
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mGeometry.moveTo(x, y);
    return true;
}

// QWidget::setParent(QWidget*) returns at once for the parent a widget already has; otherwise it hides
// a widget that has been created, which then only shows again when asked. Every window here is taken
// as created, as one is once any window it is in has been shown.
bool TNullConsoleFrontend::reparentLabel(const QString& windowname, const QString& name, const int x, const int y, const bool show)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end() || !ownsContainer(windowname)) {
        return false;
    }
    // TMainConsole::parentWidgetFor() takes a scroll box before a user window
    auto [userWindow, box] = containerFor(windowname, true);
    Label& label = it->second;
    const bool moved = userWindow != label.userWindow || box != label.box;
    // QWidget::setParent() restyles a label it moves
    if (moved) {
        label.userWindow = userWindow;
        label.box = std::move(box);
        label.stackOrder = ++mTopOfStack;
        label.deactivated = ++mDeactivations;
        inheritLabelFont(label, true);
    }
    label.pModel->mGeometry.moveTo(x, y);
    label.shown = show || (!moved && label.shown);
    reportWindowVisibility(name);
    return true;
}

// Whether the rules over a label differ from those it was last restyled under
bool TNullConsoleFrontend::labelSheetsMoved(const Label& label) const
{
    return windowSheet(label.userWindow) != label.windowSheet || (label.styled && labelSheetChain(label) != label.sheetChain);
}

bool TNullConsoleFrontend::paneSheetsMoved(const QString& name, const SubConsole& console) const
{
    return paneSheetChain(name) != console.paneChain;
}

// A label whose parent changed, or whose parent's parent did: a styled one keeps its font unless it
// is the one restyled, and any other takes its new parent's for what it does not set itself
void TNullConsoleFrontend::inheritLabelFont(Label& label, const bool restyle)
{
    const QFont before = label.pModel->mFont;
    if (restyle && labelSheetsMoved(label)) {
        label.restyleDue = true;
        restyleLabel(label, label.pModel->mStyleSheet);
    } else if (!label.styled) {
        QFont local = label.pModel->mFont;
        local.setResolveMask(label.fontMask);
        label.pModel->mFont = local.resolve(naturalLabelFont(label));
    } else if (restyle) {
        polishLabel(label);
    }
    if (label.textLabel && label.pModel->mFont != before) {
        label.sizeHint.reset();
    }
}

std::pair<bool, QString> TNullConsoleFrontend::reparentWindow(const QString& windowname, const QString& name, const int x, const int y, const bool show)
{
    mWindowNotifier.note();
    if (!ownsContainer(windowname)) {
        return noView();
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    auto [userWindow, box] = containerFor(windowname, true);
    if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end()) {
        // A buffer is a top-level widget until then, whose size QWidget::show() may then change
        if (registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::MiniConsole) {
            return noView();
        }
        QObject* pParent = holderFor(userWindow, box.get());
        const bool otherWindow = userWindow != it->second.userWindow;
        // A frame's console also moves when it leaves the frame for the window the frame is in
        const bool moved = otherWindow || box != it->second.box || (it->second.holder && it->second.holder->parent() != pParent);
        if (moved) {
            it->second.stackOrder = ++mTopOfStack;
        }
        it->second.userWindow = userWindow;
        it->second.box = std::move(box);
        it->second.shown = show || (!moved && it->second.shown);
        if (otherWindow || paneSheetsMoved(name, it->second)) {
            applyUserWindowSheetFont(name, windowSheet(userWindow));
        }
        if (it->second.holder) {
            it->second.holder->setParent(pParent);
        }
        if (const auto line = mCommandLines.find(name); line != mCommandLines.end() && ownsLine(it->second, line->second)) {
            line->second.userWindow = it->second.userWindow;
            line->second.box = it->second.box;
            if (moved && !line->second.styleSheet.isEmpty()) {
                restylePlainWindow(name, line->second, true);
            }
        }
        moveSubConsole(name, x, y);
        reportWindowVisibility(name);
        sendPendingPaneResize(name);
        return {true, QString()};
    }
    const auto it = mScrollBoxes.find(name);
    if (it == mScrollBoxes.end()) {
        if (PlainWindow* window = plainWindow(name)) {
            // A console's own command line leaves the console's layer for the window it is moved into
            const bool moved = window->inConsole || userWindow != window->userWindow || box != window->box;
            if (moved) {
                window->stackOrder = ++mTopOfStack;
            }
            window->inConsole = false;
            window->layerShown = true;
            window->consoleShown = true;
            window->userWindow = userWindow;
            window->box = std::move(box);
            window->shown = show || (!moved && window->shown);
            if (window->writer) {
                window->writer->setParent(holderFor(window->userWindow, window->box.get()));
            }
            // QWidget::setParent() restyles a text edit it moves, in its read-only state now
            if (moved) {
                restylePlainWindow(name, *window, mCommandLines.contains(name));
            }
            movePlainWindow(name, x, y);
            reportWindowVisibility(name);
            return {true, QString()};
        }
        if (registry.hasSubConsole(name) || registry.hasPlainWindow(name)) {
            return noView();
        }
        if (namesEmbeddedMapper(name)) {
            mpEmbeddedMapper->setParent(holderFor(userWindow, box.get()));
            const bool moved = userWindow != mEmbeddedMapperWindow || box != mpEmbeddedMapperBox;
            if (moved) {
                mEmbeddedMapperStackOrder = ++mTopOfStack;
            }
            mEmbeddedMapperShown = show || (!moved && mEmbeddedMapperShown);
            mEmbeddedMapperWindow = userWindow;
            mpEmbeddedMapperBox = std::move(box);
            mEmbeddedMapperGeometry.moveTo(x, y);
            reportEmbeddedMapperVisibility();
            return {true, QString()};
        }
        return {false, qsl("element '%1' not found").arg(name)};
    }
    if (within(box.get(), it->second.get())) {
        return {false, qsl("element '%1' cannot be moved into itself or into one of its own children").arg(name)};
    }
    const bool moved = userWindow != it->second->userWindow || box != it->second->box;
    if (moved) {
        it->second->stackOrder = ++mTopOfStack;
    }
    const QString fromWindow = it->second->userWindow;
    it->second->userWindow = userWindow;
    it->second->box = std::move(box);
    it->second->shown = show || (!moved && it->second->shown);
    if (it->second->holder) {
        it->second->holder->setParent(holderFor(it->second->userWindow, it->second->box.get()));
    }
    moveScrollBoxContents(it->second.get(), fromWindow);
    movePlainWindow(name, x, y);
    reportVisibility(QString(), it->second.get());
    return {true, QString()};
}

// What a moved scroll box holds goes with it into its user window
void TNullConsoleFrontend::moveScrollBoxContents(const ScrollBox* box, const QString& fromWindow)
{
    // QWidget::setParent() restyles the box and all inside it under the sheet of the window it went into, and
    // under the rules that name what it is now inside, but only where a sheet was or is over it: with none,
    // QWidgetPrivate::inheritStyle() leaves what it holds unpolished
    const auto underSheet = [this](const QString& userWindow) {
        return !mpHost->mProfileStyleSheet.isEmpty() || !windowSheet(userWindow).isEmpty();
    };
    const bool repolish = underSheet(fromWindow) || underSheet(box->userWindow);
    for (auto& [name, inner] : mScrollBoxes) {
        if (within(inner->box.get(), box)) {
            inner->userWindow = box->userWindow;
        }
    }
    for (auto& [name, label] : mLabels) {
        if (!within(label.box.get(), box)) {
            continue;
        }
        const bool otherWindow = label.userWindow != box->userWindow;
        label.userWindow = box->userWindow;
        const bool restyle = repolish && labelSheetsMoved(label);
        if (otherWindow || restyle) {
            inheritLabelFont(label, restyle);
        }
    }
    QStringList restyled;
    for (auto& [name, subConsole] : mSubConsoles) {
        if (within(subConsole.box.get(), box)) {
            subConsole.userWindow = box->userWindow;
            if (mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::MiniConsole) {
                restyled << name;
            }
        }
    }
    const QString sheet = windowSheet(box->userWindow);
    for (const QString& name : std::as_const(restyled)) {
        if (const auto console = mSubConsoles.find(name); console != mSubConsoles.end() && paneSheetsMoved(name, console->second)) {
            applyUserWindowSheetFont(name, sheet);
        }
    }
    for (auto& [name, commandLine] : mCommandLines) {
        if (within(commandLine.box.get(), box)) {
            commandLine.userWindow = box->userWindow;
            if (repolish && !commandLine.styleSheet.isEmpty()) {
                restylePlainWindow(name, commandLine, true);
            }
        }
    }
    for (auto& [name, textBox] : mTextBoxes) {
        if (within(textBox.box.get(), box)) {
            textBox.userWindow = box->userWindow;
            if (repolish && !textBox.styleSheet.isEmpty()) {
                restylePlainWindow(name, textBox, false);
            }
        }
    }
    if (within(mpEmbeddedMapperBox.get(), box)) {
        mEmbeddedMapperWindow = box->userWindow;
    }
}

bool TNullConsoleFrontend::setLabelBackgroundColor(const QString& name, const QColor& color)
{
    mWindowNotifier.note();
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    TLabelModel& model = *it->second.pModel;
    model.mBackgroundColor = color;
    restyleLabel(it->second, TLabelModel::styleSheetWithBackgroundColor(model.mStyleSheet, color));
    return true;
}

// TLabel stamps its palette with the colour it was last given, which is what the real view reads
std::optional<QColor> TNullConsoleFrontend::getLabelBackgroundColor(const QString& name) const
{
    const TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return std::nullopt;
    }
    return pLabel->mBackgroundColor;
}

bool TNullConsoleFrontend::setLabelSvgTint(const QString& name, const QColor& color)
{
    mWindowNotifier.note();
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgTintColor = color;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgRotation(const QString& name, const double angle)
{
    mWindowNotifier.note();
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgRotation = angle;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgShear(const QString& name, const double shearX, const double shearY)
{
    mWindowNotifier.note();
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgShearX = shearX;
    pLabel->mSvgShearY = shearY;
    return true;
}

bool TNullConsoleFrontend::setLabelFont(const QString& name, const QFont& font)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    Label& label = it->second;
    const QFont before = label.pModel->mFont;
    setLabelWidgetFont(label, font, label.styled);
    if (label.textLabel && label.pModel->mFont != before) {
        label.sizeHint.reset();
    }
    return true;
}

// A user window has no frame here, so its console is its dock's size
bool TNullConsoleFrontend::resizeSubConsole(const QString& name, const int width, const int height)
{
    mWindowNotifier.note();
    if (!mSubConsoles.count(name)) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
    const QSize before = geometry.size();
    geometry.setSize(widgetSize(width, height));
    registry.setSubConsoleGeometry(name, geometry);
    registry.setUserWindowSize(name, geometry.size());
    reportGridSize(name);
    // TMxpFrameWidgets::createExternalFrame() reports the resizes of an external frame's console
    if (externalFrame(name)) {
        mpHost->mMxpFrameManager.setWindowSize(name, geometry.size());
    }
    // After the frame's report, which a handler of the pane's report resizing it again would otherwise outdate
    if (geometry.size() != before) {
        resizePane(name);
    }
    // A hidden dock's console takes the size as it next shows, and a handler of the pane's report may have resized it
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end() || !it->second.shown || it->second.consoleSize == geometry.size() || registry.subConsoleGeometry(name).value_or(QRect()).size() != geometry.size()
        || registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
        return true;
    }
    it->second.consoleSize = geometry.size();
    // A console its show is still sending the resize to is not yet visible, so this only marks it pending again, which
    // that show then clears with no event, unless a show nested in it, after a hide, sends it at this size
    if (it->second.sendingResize) {
        it->second.resizePending = true;
    } else {
        raiseUserWindowResize(name);
    }
    return true;
}

bool TNullConsoleFrontend::moveSubConsole(const QString& name, const int x, const int y)
{
    mWindowNotifier.note();
    if (!mSubConsoles.count(name)) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
    geometry.moveTo(x, y);
    registry.setSubConsoleGeometry(name, geometry);
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a userwindow cannot have an empty string as its name")};
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (!mSubConsoles.count(name) || !registry.hasDockWidget(name)) {
        return {false, qsl("userwindow name '%1' not found").arg(name)};
    }
    applyUserWindowSheet(name, userWindowStyleSheet);
    return {true, QString()};
}

// What a sheet on the dock changes that scripts read back, with no widget for Qt to polish
void TNullConsoleFrontend::applyUserWindowSheet(const QString& name, const QString& sheet)
{
    mpHost->windowRegistry().setUserWindowStyleSheet(name, sheet);
    // As TDockWidget::event() takes the title a qproperty-windowTitle gives the dock, which is outside its console
    if (const auto title = sheetStringProperty(sheetDeclarations(sheet, containerSheetChain(name, nullptr).mid(1), {}), u"windowTitle")) {
        mWindowNotifier.note();
        mpHost->windowRegistry().setUserWindowTitle(name, *title);
    }
    applyUserWindowSheetFont(name, sheet);
    restyleWindowContents(name);
}

void TNullConsoleFrontend::applyUserWindowSheetFont(const QString& name, const QString& sheet)
{
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end()) {
        return;
    }
    // As TTextEdit::changeEvent() takes the font a sheet gives the window's text for the console's, with no
    // sysFontChangeEvent; clearing it hands back the font from before, as for a label
    console->second.paneChain = paneSheetChain(name);
    const std::optional<QFont> sheetFont = sheetFontFor(sheet, console->second.paneChain);
    TConsoleModel& model = *console->second.pModel;
    QFont font = console->second.fontBeforeSheet.value_or(model.mUpperPaneFont);
    console->second.fontBeforeSheet = sheetFont ? std::optional<QFont>(font) : std::nullopt;
    uint mask = font.resolveMask();
    applySheetFont(font, mask, sheetFont);
    if (font != model.mUpperPaneFont) {
        mWindowNotifier.note();
        model.mUpperPaneFont = font;
        reportGridSize(name);
    }
}

std::pair<bool, QString> TNullConsoleFrontend::setUserWindowTitle(const QString& name, const QString& text)
{
    mWindowNotifier.note();
    if (name.isEmpty()) {
        return {false, qsl("a user window cannot have an empty string as its name")};
    }
    if (!mSubConsoles.count(name)) {
        return {false, qsl("user window name '%1' not found").arg(name)};
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
        return {false, qsl("\"%1\" is not a user window").arg(name)};
    }
    // The real view's default title, so translated in its context
    //: Default title of a user window; %1 is the profile's name and %2 the window's
    registry.setUserWindowTitle(name, text.isEmpty() ? QCoreApplication::translate("TMainConsole", "User window - %1 - %2").arg(mpHost->getName(), name) : text);
    return {true, QString()};
}

void TNullConsoleFrontend::setBorderColor(const QColor& color)
{
    mpHost->mainConsoleModel().mBorderColor = color;
}

void TNullConsoleFrontend::applyBorders()
{
    mpHost->layOutMainAreaWithNoView(Host::MainAreaChange::borders);
}

// As TConsole::setConsoleBgColor() leaves a sub-console's model
bool TNullConsoleFrontend::setSubConsoleBackgroundColor(const QString& name, const QColor& color)
{
    mWindowNotifier.note();
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mBgColor = color;
    pModel->buffer.updateColors();
    return true;
}

bool TNullConsoleFrontend::setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mCommandBgColor = color;
    return true;
}

bool TNullConsoleFrontend::setSubConsoleCommandForegroundColor(const QString& name, const QColor& color)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mCommandFgColor = color;
    return true;
}

bool TNullConsoleFrontend::setWindowScrollBarVisible(const QString& name, const bool visible)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    pModel->mScrollBarEnabled = visible;
    mWindowNotifier.note();
    return true;
}

// Nothing reads a horizontal scroll bar back, so there is nothing to record
bool TNullConsoleFrontend::setWindowHorizontalScrollBarVisible(const QString& name, bool)
{
    return mpHost->consoleModelNamed(name) != nullptr;
}

// As TConsole::setScrolling(), which leaves the main console and buffers scrolling
bool TNullConsoleFrontend::setWindowScrolling(const QString& name, const bool enabled)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    const auto kind = mpHost->windowRegistry().subConsoleKind(name);
    if (kind == TWindowRegistry::SubConsoleKind::MiniConsole || kind == TWindowRegistry::SubConsoleKind::UserWindow) {
        pModel->mScrollingEnabled = enabled;
        if (!enabled && pModel->mScrolledBackTo) {
            mWindowNotifier.note();
            pModel->mScrolledBackTo.reset();
            reportGridSize(name);
            resizePane(name);
        }
    }
    return true;
}

bool TNullConsoleFrontend::scrollWindowTo(const QString& name, const int line, const bool toEnd)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    mWindowNotifier.note();
    const bool wasSplit = pModel->mScrolledBackTo.has_value();
    const int lastLine = pModel->buffer.getLastLineNumber();
    // A buffer is never painted, so a real one never shows itself scrolled back
    if (toEnd || line >= lastLine) {
        pModel->mScrolledBackTo.reset();
        // As TMainConsole::scrollWindowTo() puts a scrolled back pane at the end, which incoming text under way then leaves it at
        if (wasSplit && pModel == &mpHost->mainConsoleModel() && mIncomingTexts > 0) {
            holdNewLinesAtEnd();
        }
    } else if (pModel->mScrollingEnabled && mpHost->windowRegistry().subConsoleKind(name) != TWindowRegistry::SubConsoleKind::Buffer) {
        pModel->mScrolledBackTo = line < 0 ? std::max(lastLine + line, 0) : line;
    }
    // As TConsole's lower pane showing or hiding resizes the upper one
    if (pModel->mScrolledBackTo.has_value() != wasSplit) {
        reportGridSize(name);
        resizePane(name);
    }
    return true;
}

bool TNullConsoleFrontend::windowFollowsNewLines(const QString& name)
{
    const TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    return pModel && !pModel->mScrolledBackTo;
}

// As TTextEdit::getColumnCount() and getRowCount() count it
void TNullConsoleFrontend::reportGridSize(const QString& name)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return;
    }
    QSize size = mpHost->windowRegistry().subConsoleGeometry(name).value_or(QRect()).size();
    // TConsole::layoutMainDisplay() takes its own command line, 31 pixels tall, and the 1px foot seam over it off the pane
    const auto console = mSubConsoles.find(name);
    const auto commandLine = mCommandLines.find(name);
    if (console != mSubConsoles.end() && commandLine != mCommandLines.end() && ownsLine(console->second, commandLine->second) && commandLine->second.shown) {
        size.setHeight(std::max(0, size.height() - 31 - 1));
    }
    // TConsole's splitter gives a scrolled-back console's upper pane 6 of every 8 pixels its 3px handle leaves,
    // rounded half up: measured in the GUI, as its constructor's 6:1 stretch factors suggest otherwise. A change to
    // that split must change this too
    const auto kind = mpHost->windowRegistry().subConsoleKind(name);
    if (pModel->mScrolledBackTo && (kind == TWindowRegistry::SubConsoleKind::MiniConsole || kind == TWindowRegistry::SubConsoleKind::UserWindow)) {
        size.setHeight((std::max(0, size.height() - 3) * 6 + 4) / 8);
    }
    const QFontMetricsF metrics(pModel->mUpperPaneFont);
    pModel->mGridSize = QSize(qRound(size.width() / metrics.averageCharWidth()), qRound(size.height() / metrics.lineSpacing()));
    mWindowNotifier.note();
}

QFont TNullConsoleFrontend::displayFont() const
{
    return mpHost->displayFontWithNoView();
}

QFont TNullConsoleFrontend::mainTextFont() const
{
    return withSheetFont(withSheetFont(displayFont(), mMainConsoleSheetFont), mMainTextSheetFont);
}

std::optional<QFont> TNullConsoleFrontend::profileMainTextSheetFont() const
{
    return sheetFontFor(mpHost->mProfileStyleSheet, paneSheetChain(qsl("main")));
}

// As a polish gives the main console's text the font the profile's sheet names for it, and lays it out again
void TNullConsoleFrontend::restyleMainText()
{
    const QFont before = mainTextFont();
    mMainTextSheetFont = profileMainTextSheetFont();
    if (mainTextFont() != before) {
        mpHost->layOutMainAreaWithNoView(Host::MainAreaChange::font);
    }
}

// As TConsole::refreshView() gives the text the main console's font, over which a sheet that styles the console lies,
// and which the text keeps once that sheet has gone
void TNullConsoleFrontend::changeDisplayFont(const std::function<void()>& change)
{
    const QFont before = displayFont();
    const std::optional<QFont> consoleSheetFont = mMainConsoleSheetFont;
    mMainConsoleSheetFont = sheetFontFor(mpHost->mProfileStyleSheet, consoleSheetChain(qsl("main")));
    change();
    if (TFontAttributes(displayFont()) == TFontAttributes(before)) {
        mMainConsoleSheetFont = consoleSheetFont;
    }
    reportDisplayFontChange(before);
}

std::optional<QFont> TNullConsoleFrontend::consoleFont(const QString& name) const
{
    if (name.isEmpty() || name == qsl("main")) {
        return {displayFont()};
    }
    if (const TConsoleModel* pModel = subConsoleModel(name)) {
        return {pModel->mUpperPaneFont};
    }
    if (const auto retired = mRetiredWindows.find(name); retired != mRetiredWindows.end()) {
        return {retired->second.font};
    }
    return {};
}

// The font a console's text is drawn in, which for the main console a profile sheet can make other than its own
std::optional<QFont> TNullConsoleFrontend::consoleTextFont(const QString& name) const
{
    if (name.isEmpty() || name == qsl("main")) {
        return {mainTextFont()};
    }
    return consoleFont(name);
}

std::optional<QSize> TNullConsoleFrontend::consoleFontSize(const QString& name) const
{
    const auto font = consoleTextFont(name);
    if (!font) {
        return {};
    }
    const QFontMetrics metrics(*font);
    return {QSize(metrics.horizontalAdvance(QChar('W')), metrics.height())};
}

std::optional<int> TNullConsoleFrontend::consoleColumnWidth(const QString& name) const
{
    const auto font = consoleTextFont(name);
    if (!font) {
        return {};
    }
    return {QFontMetrics(*font).averageCharWidth()};
}

// The sheet over a sub-console's widget: a buffer has no widget for one to reach
QString TNullConsoleFrontend::consoleWindowSheet(const QString& name) const
{
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end() || mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return QString();
    }
    return windowSheet(mpHost->windowRegistry().hasDockWidget(name) ? name : console->second.userWindow);
}

// As TConsole::refreshView() gives its text the console's font, over which a sheet that styles the console lies
QFont TNullConsoleFrontend::underConsoleSheet(const QString& name, const QFont& font) const
{
    return withSheetFont(font, sheetFontFor(consoleWindowSheet(name), consoleSheetChain(name)));
}

// As TConsole::setFont() changes a sub-console's, and tells scripts of it
void TNullConsoleFrontend::setSubConsoleFont(const QString& name, const QFont& font)
{
    mWindowNotifier.note();
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end() || !console->second.pModel) {
        return;
    }
    console->second.scriptFont = font;
    // The text keeps that font once the sheet has gone, until a script sets another
    const QFont ownFont = underConsoleSheet(name, font);
    console->second.pModel->mUpperPaneFont = ownFont;
    // A sheet's font stays over the one a script then sets
    if (console->second.fontBeforeSheet) {
        console->second.fontBeforeSheet = ownFont;
        applyUserWindowSheetFont(name, consoleWindowSheet(name));
    }
    reportGridSize(name);
    if (mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return;
    }
    raiseFontEvent(qsl("sysFontChangeEvent"), name, ownFont);
}

// As TConsole::setFont() and Host::updateConsolesFont() tell scripts of a new main console font
void TNullConsoleFrontend::reportDisplayFontChange(const QFont& before)
{
    const QFont font = displayFont();
    if (TFontAttributes(font) == TFontAttributes(before)) {
        return;
    }
    raiseFontEvent(qsl("sysFontChangeEvent"), qsl("main"), font);
    // Read afresh, as Host::updateConsolesFont() does: a sysFontChangeEvent handler may have changed it again
    raiseFontEvent(qsl("sysSettingChanged"), qsl("main window font"), displayFont());
    mpHost->reportDisplayFontChangeToEditor();
}

void TNullConsoleFrontend::raiseFontEvent(const QString& eventName, const QString& subject, const QFont& font)
{
    TEvent event{};
    event.mArgumentList.append(eventName);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(subject);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(font.family());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(font.pointSize()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(event);
}

std::optional<std::pair<bool, QString>> TNullConsoleFrontend::setWindowFontFamily(const QString& name, const QString& family, const QFont::Weight weight)
{
    if (name.isEmpty() || name == qsl("main")) {
        QFont font = mpHost->createFontWithSettings(family, mpHost->getDisplayFont().pointSize());
        if (weight != QFont::Normal) {
            font.setWeight(weight);
        }
        std::pair<bool, QString> result;
        changeDisplayFont([this, &font, &result]() {
            result = mpHost->setDisplayFont(font, Host::DisplayFontChange::UserChoice);
        });
        return {result};
    }
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end() || !console->second.pModel) {
        return {};
    }
    // TMainConsole::setWindowFontFamily() keeps the size of the widget's font, which a sheet that styles the
    // console lies over, and TConsole::setFont() compares the new font with the one scripts gave it
    const QFont scriptFont = console->second.scriptFont;
    QFont font = mpHost->createFontWithSettings(family, underConsoleSheet(name, scriptFont).pointSize());
    if (weight != QFont::Normal) {
        font.setWeight(weight);
    }
    if (TFontAttributes(font) != TFontAttributes(scriptFont)) {
        setSubConsoleFont(name, font);
    }
    return {{true, QString()}};
}

bool TNullConsoleFrontend::setWindowFontSize(const QString& name, const int size)
{
    if (name.isEmpty() || name == qsl("main")) {
        changeDisplayFont([this, size]() {
            mpHost->setDisplayFontSize(size);
        });
        return true;
    }
    const auto console = mSubConsoles.find(name);
    if (console == mSubConsoles.end() || !console->second.pModel) {
        return false;
    }
    // As TConsole::setFontSize() compares with and builds from the font scripts gave it
    if (console->second.scriptFont.pointSize() != size) {
        TFontAttributes font(console->second.scriptFont);
        font.mPointSize = size;
        setSubConsoleFont(name, font.makeFont());
    }
    return true;
}

// Timed as TMainConsole times a pass for its latency box. With no window to be minimized, whether the app's is is the
// session's to say
bool TNullConsoleFrontend::startIncomingText()
{
    mIncomingTextTimer.restart();
    ++mIncomingTexts;
    if (!mNewLinesHeldAt) {
        holdNewLinesAtEnd();
    }
    return mpHost->mAlertOnNewData;
}

// Where TTextEdit::showNewLines() leaves the pane: one past the last line, which windowScroll() caps
void TNullConsoleFrontend::holdNewLinesAtEnd()
{
    TBuffer& buffer = mpHost->mainConsoleModel().buffer;
    mNewLinesHeldAt = buffer.linesTrimmedTotal() + buffer.size();
}

// The GUI's pane goes to the end as it is now, and lines the incoming text still has to add leave it there
void TNullConsoleFrontend::followNewLines()
{
    if (mIncomingTexts > 0) {
        holdNewLinesAtEnd();
    } else {
        mNewLinesHeldAt.reset();
    }
}

// As TConsole::bufferCleared() puts the pane on what is left, which later lines then leave it at
void TNullConsoleFrontend::holdNewLinesAfterClear()
{
    if (mIncomingTexts > 0 || mNewLinesHeldAt) {
        holdNewLinesAtEnd();
    }
}

std::optional<int> TNullConsoleFrontend::newLinesHeldAt() const
{
    if (!mNewLinesHeldAt) {
        return std::nullopt;
    }
    return static_cast<int>(std::max<qint64>(*mNewLinesHeldAt - mpHost->mainConsoleModel().buffer.linesTrimmedTotal(), 0));
}

// As TConsole::showCommandEcho(), which follows only a command that made lines of its own
void TNullConsoleFrontend::showCommandEcho(const TConsoleModel::CommandEcho& echo)
{
    if (echo.kind == TConsoleModel::CommandEcho::Kind::NewLines) {
        showNewLines();
    }
}

void TNullConsoleFrontend::alertNewData()
{
    emit mpHost->mainConsoleModel().mNotifier.newDataAlerted();
}

void TNullConsoleFrontend::finishIncomingText(bool)
{
    --mIncomingTexts;
    TConsoleModel& model = mpHost->mainConsoleModel();
    model.mIncomingTextSeconds = mIncomingTextTimer.elapsed() / 1000.0;
    emit model.mNotifier.incomingTextProcessed();
}

bool TNullConsoleFrontend::restack(const QString& name, const bool raise)
{
    mWindowNotifier.note();
    const auto order = [this, raise]() {
        return raise ? ++mTopOfStack : --mBottomOfStack;
    };
    if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end()) {
        it->second.stackOrder = order();
        return true;
    }
    if (const auto it = mLabels.find(name); it != mLabels.end()) {
        it->second.stackOrder = order();
        return true;
    }
    if (namesEmbeddedMapper(name)) {
        mEmbeddedMapperStackOrder = order();
        return true;
    }
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        it->second->stackOrder = order();
        return true;
    }
    if (const auto it = mCommandLines.find(name); it != mCommandLines.end()) {
        it->second.stackOrder = order();
        return true;
    }
    if (const auto it = mTextBoxes.find(name); it != mTextBoxes.end()) {
        it->second.stackOrder = order();
        return true;
    }
    return false;
}

QString TNullConsoleFrontend::parentName(const QString& userWindow, const ScrollBox* box) const
{
    if (!box) {
        return userWindow;
    }
    for (const auto& [name, candidate] : mScrollBoxes) {
        if (candidate.get() == box) {
            return name;
        }
    }
    return userWindow;
}

bool TNullConsoleFrontend::listsConsole(const std::optional<TWindowRegistry::SubConsoleKind> kind)
{
    return kind == TWindowRegistry::SubConsoleKind::MiniConsole || kind == TWindowRegistry::SubConsoleKind::UserWindow;
}

TConsoleModel* TNullConsoleFrontend::listedConsoleModel(const QString& name) const
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end() || !listsConsole(mpHost->windowRegistry().subConsoleKind(name)) || inRetiredWindow(it->second.userWindow, it->second.box.get())) {
        return nullptr;
    }
    return it->second.pModel.get();
}

// What a deleted user window held, which is in no window an app can show until deferred deletes take it
bool TNullConsoleFrontend::inRetiredWindow(const QString& userWindow, const ScrollBox* box) const
{
    if (mRetiredWindows.contains(userWindow)) {
        return true;
    }
    for (; box; box = box->box.get()) {
        if (mRetiredWindows.contains(box->userWindow)) {
            return true;
        }
    }
    return false;
}

std::vector<TNullConsoleFrontend::WindowRecord> TNullConsoleFrontend::windowRecords() const
{
    using Kind = WindowRecord::Kind;
    const TWindowRegistry& registry = mpHost->windowRegistry();
    const auto consoleKind = [&registry](const QString& name, const SubConsole& console) {
        if (console.pFrameSink) {
            return Kind::mxpFrame;
        }
        return registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow ? Kind::userWindow : Kind::miniConsole;
    };
    // As parentName() gives the user window for a box deleted with deleteScrollBox() that its windows still hold
    const auto kindOfParent = [this](const ScrollBox* box) {
        const bool listed = box && std::any_of(mScrollBoxes.cbegin(), mScrollBoxes.cend(), [box](const auto& entry) {
                                return entry.second.get() == box;
                            });
        return listed ? Kind::scrollBox : Kind::userWindow;
    };
    std::vector<WindowRecord> records;
    for (const auto& [name, console] : mSubConsoles) {
        const auto kind = registry.subConsoleKind(name);
        if (!listsConsole(kind) || inRetiredWindow(console.userWindow, console.box.get())) {
            continue;
        }
        // A frame's console that a script moved out of its frame is where the script put it
        const bool framed = inOwnFrame(name);
        const QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
        QString frameHost = framed ? mMxpFrames.host(name) : QString();
        QPoint framePlace = framed ? mMxpFrames.consolePlace(name) : QPoint();
        // A frame in one whose console a script moved out stays in that one's container, which did not move
        for (size_t depth = 0; !frameHost.isEmpty() && mSubConsoles.contains(frameHost) && !inOwnFrame(frameHost) && depth <= mSubConsoles.size(); ++depth) {
            framePlace += mMxpFrames.consolePlace(frameHost);
            frameHost = mMxpFrames.host(frameHost);
        }
        records.push_back({name,
                           consoleKind(name, console),
                           framed ? frameHost : parentName(console.userWindow, console.box.get()),
                           framed ? QRect(framePlace, geometry.size()) : geometry,
                           console.shown,
                           console.shown && (!framed || mMxpFrames.pageShown(name)) && shownWithin(console.userWindow, console.box.get()),
                           console.stackOrder,
                           console.pModel->mBgColor,
                           console.pModel->mUpperPaneFont,
                           kind == TWindowRegistry::SubConsoleKind::UserWindow || (framed && externalFrame(name))});
        WindowRecord& record = records.back();
        // As Host::windowScroll() bounds it, which a deleteLine() can leave past the end
        if (console.pModel->mScrolledBackTo) {
            record.scrolledBackTo = std::clamp(*console.pModel->mScrolledBackTo, 0, console.pModel->buffer.getLastLineNumber());
        }
        record.grid = console.pModel->mGridSize;
        record.scrollBar = console.pModel->mScrollBarEnabled;
        record.timeStamps = console.pModel->mShowTimeStamps;
        record.parentKind = framed ? (frameHost.isEmpty() ? Kind::userWindow : Kind::mxpFrame) : kindOfParent(console.box.get());
        if (kind == TWindowRegistry::SubConsoleKind::UserWindow) {
            record.autoDock = console.autoDock;
            record.dockArea = console.dockArea;
            record.floating = console.floating;
            record.title = registry.userWindowTitle(name).value_or(QString());
        }
    }
    for (const auto& [name, label] : mLabels) {
        if (inRetiredWindow(label.userWindow, label.box.get())) {
            continue;
        }
        records.push_back({name,
                           Kind::label,
                           parentName(label.userWindow, label.box.get()),
                           label.pModel->mGeometry,
                           label.shown,
                           label.shown && shownWithin(label.userWindow, label.box.get()),
                           label.stackOrder,
                           label.pModel->mBackgroundColor,
                           label.pModel->mFont});
        records.back().parentKind = kindOfParent(label.box.get());
    }
    for (const auto& [name, box] : mScrollBoxes) {
        if (inRetiredWindow(box->userWindow, box->box.get())) {
            continue;
        }
        records.push_back({name,
                           Kind::scrollBox,
                           parentName(box->userWindow, box->box.get()),
                           registry.plainWindowGeometry(name).value_or(QRect()),
                           box->shown,
                           box->shown && shownWithin(box->userWindow, box->box.get()),
                           box->stackOrder,
                           QColor(),
                           QFont()});
        records.back().parentKind = kindOfParent(box->box.get());
    }
    for (const auto& [name, line] : mCommandLines) {
        const auto console = mSubConsoles.find(name);
        const bool own = line.inConsole && console != mSubConsoles.end() && ownsLine(console->second, line);
        if (own ? inRetiredWindow(console->second.userWindow, console->second.box.get()) : inRetiredWindow(line.userWindow, line.box.get())) {
            continue;
        }
        const bool visible = line.shown && line.layerShown;
        const bool shown = commandLineShown(name);
        records.push_back({name, Kind::commandLine, own ? name : parentName(line.userWindow, line.box.get()), line.geometry, visible, shown, line.stackOrder, QColor(), QFont()});
        records.back().parentKind = own ? consoleKind(name, console->second) : kindOfParent(line.box.get());
    }
    for (const auto& [name, textBox] : mTextBoxes) {
        if (inRetiredWindow(textBox.userWindow, textBox.box.get())) {
            continue;
        }
        records.push_back({name,
                           Kind::textBox,
                           parentName(textBox.userWindow, textBox.box.get()),
                           textBox.geometry,
                           textBox.shown,
                           textBox.shown && shownWithin(textBox.userWindow, textBox.box.get()),
                           textBox.stackOrder,
                           QColor(),
                           registry.textBoxFont(name).value_or(QFont())});
        records.back().parentKind = kindOfParent(textBox.box.get());
    }
    if (mpEmbeddedMapper) {
        records.push_back({qsl("mapper"),
                           Kind::mapper,
                           parentName(mEmbeddedMapperWindow, mpEmbeddedMapperBox.get()),
                           mEmbeddedMapperGeometry,
                           mEmbeddedMapperShown,
                           mEmbeddedMapperShown && shownWithin(mEmbeddedMapperWindow, mpEmbeddedMapperBox.get()),
                           mEmbeddedMapperStackOrder,
                           QColor(),
                           QFont()});
        records.back().parentKind = kindOfParent(mpEmbeddedMapperBox.get());
    } else if (mMapWidget) {
        records.push_back({qsl("mapper"), Kind::mapper, QString(), mMapWidget->geometry, mMapWidget->shown, mMapWidget->shown, mMapWidget->stackOrder, QColor(), QFont(), true});
    }
    return records;
}

std::optional<TNullConsoleFrontend::LabelRecord> TNullConsoleFrontend::labelRecord(const QString& name) const
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return std::nullopt;
    }
    const Label& label = it->second;
    LabelRecord record;
    if (label.pixmapSize) {
        record.image = label.pixmapPath;
    } else if (label.svgSize) {
        record.image = label.svgPath;
        record.svg = true;
    }
    if (label.showsMovie && label.movie) {
        record.movie = label.movie->fileName();
        // One that played to its end shows its last frame, as a paused one shows its own
        record.moviePaused = label.movie->state() != QMovie::Running;
        record.movieFrame = record.moviePaused ? label.movie->currentFrameNumber() : -1;
        record.movieSpeed = label.movie->speed();
        record.movieSize = label.movie->scaledSize();
    }
    record.toolTipDuration = label.toolTipDuration;
    record.cursor = label.cursor;
    record.customCursor = label.customCursor;
    record.customCursorHotSpot = label.customCursorHotSpot;
    record.made = label.made;
    return record;
}

bool TNullConsoleFrontend::holdsLabelModel(const TLabelModel* pModel) const
{
    return pModel
           && (std::any_of(mLabels.cbegin(),
                           mLabels.cend(),
                           [pModel](const auto& entry) {
                               return entry.second.pModel.get() == pModel;
                           })
               || std::any_of(mRetiredLabels.cbegin(), mRetiredLabels.cend(), [pModel](const auto& retired) {
                      return retired.get() == pModel;
                  }));
}

std::optional<qint64> TNullConsoleFrontend::deactivated(const QString& name, const bool label) const
{
    if (label) {
        const auto it = mLabels.find(name);
        return it == mLabels.end() ? std::nullopt : std::optional(it->second.deactivated);
    }
    if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end()) {
        return it->second.deactivated;
    }
    const auto it = mScrollBoxes.find(name);
    return it == mScrollBoxes.end() ? std::nullopt : std::optional(it->second->deactivated);
}

bool TNullConsoleFrontend::userWindow(const QString& name) const
{
    return mSubConsoles.count(name) && mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow;
}

bool TNullConsoleFrontend::placeUserWindow(const QString& name, const QRect& geometry, const bool floating)
{
    mWindowNotifier.note();
    if (!userWindow(name)) {
        return false;
    }
    mSubConsoles.at(name).floating = floating;
    const QPointer<const QObject> window = &mSubConsoles.at(name).pModel->mNotifier;
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect moved = registry.subConsoleGeometry(name).value_or(QRect());
    moved.moveTo(geometry.topLeft());
    registry.setSubConsoleGeometry(name, moved);
    // Its handlers can delete the window, or open another of its name, which this did not place, in its place
    const bool resized = resizeSubConsole(name, geometry.width(), geometry.height());
    const auto it = mSubConsoles.find(name);
    return resized && window && userWindow(name) && &it->second.pModel->mNotifier == window;
}
