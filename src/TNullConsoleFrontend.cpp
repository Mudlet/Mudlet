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
#include "TEvent.h"
#include "TLabelModel.h"

#include <QCoreApplication>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QMovie>
#include <QObject>
#include <QStringList>
#include <QSvgRenderer>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QtMath>

#include <algorithm>
#include <climits>
#include <tuple>

namespace {

// QWIDGETSIZE_MAX, which only <QWidget> defines
constexpr int maximumWidgetSize = (1 << 24) - 1;

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

// The specificity QCss::StyleSelector::matchRule() weighs a selector by that matches a widget of these classes, most
// derived first, in the pseudo-states held, or nothing. A combinator or an attribute selector other than a class is
// not matched.
std::optional<uint> selectorSpecificity(const QStringView selector, const QStringList& classNames, const QString& objectName, const QStringList& heldStates)
{
    qsizetype at = 0;
    // QCss takes a backslash as escaping the character after it, and reads no hex escape
    const auto ident = [&selector, &at]() {
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
        return name;
    };
    QString element;
    if (at < selector.size() && selector.at(at) == u'*') {
        ++at;
    } else {
        element = ident();
    }
    QStringList ids;
    QStringList classes;
    QList<std::pair<QString, bool>> pseudos;
    while (at < selector.size()) {
        const QChar marker = selector.at(at++);
        if (marker == u'#' || marker == u'.') {
            const QString name = ident();
            if (name.isEmpty()) {
                return std::nullopt;
            }
            (marker == u'#' ? ids : classes) << name;
        } else if (marker == u':') {
            if (at < selector.size() && selector.at(at) == u':') {
                ++at;
            }
            const bool negated = at < selector.size() && selector.at(at) == u'!';
            if (negated) {
                ++at;
            }
            const QString name = ident();
            if (name.isEmpty() || (at < selector.size() && selector.at(at) == u'(')) {
                return std::nullopt;
            }
            pseudos.append({name, negated});
        } else {
            return std::nullopt;
        }
    }
    if (element.isEmpty() && ids.isEmpty() && classes.isEmpty() && pseudos.isEmpty() && !selector.startsWith(u'*')) {
        return std::nullopt;
    }
    if ((!element.isEmpty() && !classNames.contains(element)) || (!ids.isEmpty() && ids != QStringList{objectName})) {
        return std::nullopt;
    }
    for (const QString& name : std::as_const(classes)) {
        if (name != classNames.first()) {
            return std::nullopt;
        }
    }
    if (!pseudos.isEmpty() && !knownPseudoState(pseudos.first().first)) {
        return std::nullopt;
    }
    // Selector::pseudoClass(): an unknown state past the first drops the states before it, but not the negated ones
    bool statesHold = true;
    for (const auto& [name, negated] : std::as_const(pseudos)) {
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
    return (element.isEmpty() ? 0 : 1) + uint(classes.size() + pseudos.size()) * 0x10 + uint(ids.size()) * 0x100;
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

// The declarations of a widget's own sheet that reach it in the pseudo-states held, in the order QStyleSheetStyle
// applies them. A sheet that is not a list of rules is QStyleSheetStyle's "* { sheet }"; @-rules apply
// to no widget.
QString sheetDeclarations(const QString& sheet, const QStringList& classNames, const QString& objectName, const QStringList& heldStates)
{
    QString text = sheet;
    for (qsizetype open = indexOutsideQuotes(text, u'/', 0); open >= 0; open = indexOutsideQuotes(text, u'/', open + 1)) {
        if (open + 1 < text.size() && text.at(open + 1) == u'*') {
            const qsizetype close = text.indexOf(qsl("*/"), open + 2);
            text.replace(open, close < 0 ? text.size() - open : close + 2 - open, u' ');
        }
    }
    if (indexOutsideQuotes(text, u'{', 0) < 0) {
        return text;
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
            if (close < 0) {
                break;
            }
            at = close + 1;
            continue;
        }
        // The parser stops at the first rule it cannot read, keeping the rules before it
        if (close < 0 || indexOutsideQuotes(text.left(close), u'{', open + 1) >= 0) {
            break;
        }
        const QString declarations = text.mid(open + 1, close - open - 1);
        for (const QString& selector : text.mid(at, open - at).split(u',')) {
            if (const auto specificity = selectorSpecificity(QStringView(selector).trimmed(), classNames, objectName, heldStates)) {
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

} // namespace

void TConsoleModelSink::printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore)
{
    mModel.buffer.appendFormatted(text, formatting, sourceLinkStore);
    mModel.mirrorToStdOut(text);
}

void TConsoleModelSink::discardAll()
{
    mModel.clear();
}

void TConsoleModelSink::discardLastLine()
{
    mModel.buffer.clearLastLine();
}

void TNullMxpFrameFrontend::createInternalFrame(const QString& name, const QString&, const QString&, const QRect&, bool, bool)
{
    mView.addFrameConsole(name);
}

std::optional<QSize> TNullMxpFrameFrontend::createExternalFrame(const QString& name, const QString&, const QSize& size, bool)
{
    mView.addFrameConsole(name);
    return size;
}

void TNullMxpFrameFrontend::createTabFrame(const QString& name, const QString&, const QString&, const QSize&, bool, bool)
{
    mView.addFrameConsole(name);
}

void TNullMxpFrameFrontend::destroyFrame(const QString& name)
{
    mView.removeFrameConsole(name);
}

TPrintSink* TNullMxpFrameFrontend::sink(const QString& name) const
{
    return mView.frameSink(name);
}

TNullConsoleFrontend::TNullConsoleFrontend(Host* pHost)
: mpHost(pHost)
{
}

TNullConsoleFrontend::~TNullConsoleFrontend() = default;

void TNullConsoleFrontend::dropWindows()
{
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

void TNullConsoleFrontend::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool, bool)
{
    auto pLabel = std::make_unique<TLabelModel>(mpHost, name);
    pLabel->mGeometry = QRect(QPoint(x, y), widgetSize(width, height));
    auto [userWindow, box] = containerFor(windowname, false);
    // A real label takes its console's font as it is made, and keeps it as that font changes
    pLabel->mFont = consoleFont(userWindow).value_or(displayFont());
    mpHost->windowRegistry().registerLabel(name, pLabel.get());
    mLabels[name] = {std::move(pLabel), userWindow, std::move(box)};
    reportWindowVisibility(name);
    // TMainConsole::createLabel()'s colour
    setLabelBackgroundColor(name, QColor(32, 32, 32, 255));
}

void TNullConsoleFrontend::deleteLabel(const QString& name)
{
    removeLabel(name);
}

void TNullConsoleFrontend::removeLabel(const QString& name)
{
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
}

// TLabel::reapplyLinkStyle()
void TNullConsoleFrontend::reapplyLabelLinkStyle(Label& label)
{
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
    TLabelModel& model = *label.pModel;
    if (sheet == model.mStyleSheet && !label.restyleDue && !sheet.contains(qsl("url("), Qt::CaseInsensitive)) {
        return;
    }
    label.restyleDue = false;
    const QFont before = model.mFont;
    model.mStyleSheet = sheet;
    if (sheet.isEmpty()) {
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
        static const QStringList classNames{qsl("TLabel"), qsl("QLabel"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")};
        const QString objectName = qsl("label_%1_%2").arg(mpHost->getName(), model.mName);
        // QStyleSheetStyle::updateStyleSheetFont() asks for the enabled and active states, setGeometry() only for enabled
        const QString declarations = sheetDeclarations(sheet, classNames, objectName, {qsl("enabled"), qsl("active")});
        const QString enabledDeclarations = sheetDeclarations(sheet, classNames, objectName, {qsl("enabled")});
        label.sheetFont = styleSheetFont(declarations);
        label.frame = sheetGeometry(declarations, sheetLengthFont(declarations), 0).frame;
        const SheetGeometry geometry = sheetGeometry(enabledDeclarations, sheetLengthFont(enabledDeclarations), 0);
        label.limits = {geometry.minimum, geometry.maximum};
        label.styled = true;
        polishLabel(label);
        setLabelSize(label, label.limits.bounded(model.mGeometry.size()));
    }
    if (label.textLabel && model.mFont != before) {
        label.sizeHint.reset();
    }
}

TConsoleModel& TNullConsoleFrontend::addSubConsole(const QString& name, const TWindowRegistry::SubConsoleKind kind, const QString& windowname)
{
    // As resolveConsoleModel() and the TConsole constructor set up a sub-console's model
    auto pModel = std::make_unique<TConsoleModel>(mpHost);
    pModel->mConsoleName = name;
    pModel->mScriptAddressable = true;
    pModel->mScrollBarEnabled = false;
    pModel->mUpperPaneFont = TFontAttributes(mpHost->fontsAntiAlias()).makeFont();
    TConsoleModel& model = *pModel;
    mpHost->windowRegistry().registerSubConsole(name, &model, kind);
    auto [userWindow, box] = containerFor(windowname, true);
    mSubConsoles[name] = {std::move(pModel), userWindow, std::move(box)};
    return model;
}

void TNullConsoleFrontend::removeSubConsole(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow) {
        QStringList contents;
        for (const auto& [childName, child] : mSubConsoles) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeSubConsole(childName);
        }
        contents.clear();
        for (const auto& [childName, child] : mLabels) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeLabel(childName);
        }
        QStringList boxes;
        std::erase_if(mScrollBoxes, [&registry, &name, &boxes](const auto& entry) {
            if (entry.second->userWindow != name) {
                return false;
            }
            registry.deregisterScrollBox(entry.first);
            boxes << entry.first;
            return true;
        });
        std::erase_if(mCommandLines, [this, &name, &boxes](const auto& entry) {
            if (entry.second.userWindow != name) {
                return false;
            }
            releaseCommandLine(entry.first, entry.second);
            boxes << entry.first;
            return true;
        });
        std::erase_if(mTextBoxes, [&registry, &name](const auto& entry) {
            if (entry.second.userWindow != name) {
                return false;
            }
            registry.deregisterTextBox(entry.first);
            return true;
        });
        for (const QString& plainName : boxes) {
            republishPlainWindow(plainName);
        }
        registry.deregisterDockWidget(name);
    }
    registry.deregisterSubConsole(name, it->second.pModel.get());
    mRetiredConsoles.push_back(std::move(it->second.pModel));
    mSubConsoles.erase(it);
    queueRelease();
}

void TNullConsoleFrontend::addFrameConsole(const QString& name)
{
    // As TMxpFrameWidgets does for a second build of a name, which TMxpFrameManager never asks for
    removeFrameConsole(name);
    TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, QString());
    mSubConsoles[name].pFrameSink = std::make_unique<TConsoleModelSink>(model);
}

void TNullConsoleFrontend::removeFrameConsole(const QString& name)
{
    if (frameSink(name)) {
        removeSubConsole(name);
    }
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
    addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, windowname);
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(QPoint(x, y), widgetSize(width, height)));
    // As TMainConsole::createMiniConsole() sizes its font, which raises sysFontChangeEvent
    setWindowFontSize(name, 12);
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

void TNullConsoleFrontend::openUserWindow(const QString& name, bool, bool, const QString&)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (!mSubConsoles.count(name)) {
        if (registry.hasSubConsole(name)) {
            // A detached view's, which this has no dock for
            return;
        }
        addSubConsole(name, TWindowRegistry::SubConsoleKind::UserWindow, QString());
        registry.registerDockWidget(name);
        registry.setUserWindowTitle(name, name);
        registry.setUserWindowStyleSheet(name, mpHost->mProfileStyleSheet);
        // There is no layout to dock it into, so it takes the main window's size
        const QSize size = mpHost->mainWindowSize().value_or(QSize());
        registry.setSubConsoleGeometry(name, QRect(QPoint(0, 0), size));
        registry.setUserWindowSize(name, size);
        // As TMainConsole::createUserWindow() sizes its font, which raises sysFontChangeEvent
        setWindowFontSize(name, 10);
    }
    setSubConsoleShown(name, true);
}

void TNullConsoleFrontend::setProfileStyleSheet(const QString& styleSheet)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    // As TMainConsole::setProfileStyleSheet() restyles every dock, a closed one too. The registry
    // ignores a name with no dock, so mini consoles and buffers keep theirs.
    for (const auto& [name, subConsole] : mSubConsoles) {
        registry.setUserWindowStyleSheet(name, styleSheet);
    }
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
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.shown = shown;
    reportWindowVisibility(name);
    return true;
}

bool TNullConsoleFrontend::setSubConsoleShown(const QString& name, const bool shown)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return false;
    }
    it->second.shown = shown;
    if (mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow) {
        reportVisibility();
    } else {
        reportWindowVisibility(name);
    }
    return true;
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

// For a window that holds nothing, such as a label, a mini console or a new scroll box, so that making
// many windows does not report every window each time
void TNullConsoleFrontend::reportWindowVisibility(const QString& name)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (const auto it = mLabels.find(name); it != mLabels.end()) {
        setLabelVisible(it->second, it->second.shown && shownWithin(it->second.userWindow, it->second.box.get()));
    }
    if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end()) {
        registry.setSubConsoleVisible(name, it->second.shown && shownWithin(it->second.userWindow, it->second.box.get()));
    }
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        registry.setPlainWindowVisible(name, it->second->shown && shownWithin(it->second->userWindow, it->second->box.get()));
    } else if (const PlainWindow* window = plainWindow(name)) {
        registry.setPlainWindowVisible(name, window->shown && shownWithin(window->userWindow, window->box.get()));
    }
}

// As TMainConsole::reportVisibility() and reportVisibilityWithin() write it for every window a change reaches
void TNullConsoleFrontend::reportVisibility()
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    for (auto& [name, label] : mLabels) {
        setLabelVisible(label, label.shown && shownWithin(label.userWindow, label.box.get()));
    }
    for (const auto& [name, subConsole] : mSubConsoles) {
        registry.setSubConsoleVisible(name, subConsole.shown && shownWithin(subConsole.userWindow, subConsole.box.get()));
    }
    for (const auto& [name, textBox] : mTextBoxes) {
        registry.setPlainWindowVisible(name, textBox.shown && shownWithin(textBox.userWindow, textBox.box.get()));
    }
    for (const auto& [name, commandLine] : mCommandLines) {
        registry.setPlainWindowVisible(name, commandLine.shown && shownWithin(commandLine.userWindow, commandLine.box.get()));
    }
    // Last, as TMainConsole::plainWindowWidget() takes a box, then a command line, then a text box of a name
    for (const auto& [name, box] : mScrollBoxes) {
        registry.setPlainWindowVisible(name, box->shown && shownWithin(box->userWindow, box->box.get()));
    }
}

bool TNullConsoleFrontend::createScrollBox(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    if (mScrollBoxes.contains(name)) {
        return false;
    }
    auto box = std::make_shared<ScrollBox>();
    std::tie(box->userWindow, box->box) = containerFor(windowname, false);
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
    std::erase_if(mTextBoxes, [&registry, &inside](const auto& entry) {
        if (!inside(entry.second.box.get())) {
            return false;
        }
        registry.deregisterTextBox(entry.first);
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
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        it->second->shown = shown;
        reportVisibility();
        return true;
    }
    PlainWindow* window = plainWindow(name);
    if (!window) {
        return false;
    }
    window->shown = shown;
    reportWindowVisibility(name);
    return true;
}

bool TNullConsoleFrontend::resizePlainWindow(const QString& name, const int width, const int height)
{
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
    if (name.isEmpty()) {
        return {false, qsl("a commandLine cannot have an empty string as its name")};
    }
    if (mpHost->parentWindowMissing(windowname)) {
        return {false, qsl("window '%1' not found").arg(windowname)};
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
    registry.registerCommandLine(name);
    // TMainConsole::plainWindowWidget() takes a scroll box of the same name first
    if (!mScrollBoxes.contains(name)) {
        registry.setPlainWindowGeometry(name, commandLine.geometry);
    }
    reportWindowVisibility(name);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::deleteCommandLine(const QString& name)
{
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
// then finds another window for
void TNullConsoleFrontend::republishPlainWindow(const QString& name)
{
    if (mScrollBoxes.contains(name)) {
        return;
    }
    if (const PlainWindow* window = plainWindow(name)) {
        mpHost->windowRegistry().setPlainWindowGeometry(name, window->geometry);
        reportWindowVisibility(name);
    }
}

// Not the main command line, which TMainConsole::commandLineNamed() takes these names for: that is a
// view's, as the registry's record of its text is
bool TNullConsoleFrontend::ownsCommandLine(const QString& name) const
{
    return !name.isEmpty() && name != QLatin1String("main") && mCommandLines.contains(name);
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
    mpHost->windowRegistry().setCommandLineText(name, plainText(text));
    return true;
}

bool TNullConsoleFrontend::appendCommandLineText(const QString& name, const QString& text)
{
    if (!ownsCommandLine(name)) {
        return false;
    }
    return replaceCommandLineText(name, mpHost->windowRegistry().commandLineText(name).value_or(QString()) + text);
}

std::pair<bool, QString> TNullConsoleFrontend::createTextBox(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    if (name.isEmpty()) {
        return {false, qsl("a text edit cannot have an empty string as its name")};
    }
    if (mpHost->parentWindowMissing(windowname)) {
        return {false, qsl("window '%1' not found").arg(windowname)};
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
    registry.registerTextBox(name);
    if (!mScrollBoxes.contains(name) && !mCommandLines.contains(name)) {
        registry.setPlainWindowGeometry(name, textBox.geometry);
        reportWindowVisibility(name);
    }
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::deleteTextBox(const QString& name)
{
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
    // A text box is never what the name's state is read from while another window has the name
    mpHost->windowRegistry().deregisterTextBox(name);

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
    restyleTextBox(name, it->second);
    return true;
}

// Only the sheet's size limits, which QStyleSheetStyle::setGeometry() sets for the enabled state and
// whether the text edit is read-only
void TNullConsoleFrontend::restyleTextBox(const QString& name, PlainWindow& textBox)
{
    static const QStringList classNames{qsl("TTextBox"), qsl("QPlainTextEdit"), qsl("QAbstractScrollArea"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")};
    const QString declarations =
            sheetDeclarations(textBox.styleSheet, classNames, qsl("textEdit_%1_%2").arg(mpHost->getName(), name), {qsl("enabled"), textBox.readOnly ? qsl("read-only") : qsl("editable")});
    // QFusionStyle's PM_DefaultFrameWidth, which a native border takes
    const SheetGeometry geometry = sheetGeometry(declarations, sheetLengthFont(declarations), 1);
    textBox.limits = {geometry.minimum, geometry.maximum};
    textBox.geometry.setSize(textBox.limits.bounded(textBox.geometry.size()));
    if (!mScrollBoxes.contains(name) && plainWindow(name) == &textBox) {
        mpHost->windowRegistry().setPlainWindowGeometry(name, textBox.geometry);
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

std::pair<bool, QString> TNullConsoleFrontend::setLabelToolTip(const QString& name, const QString& text, double)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    pLabel->setToolTip(text);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCursor(const QString& name, const int shape)
{
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
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int, int)
{
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
    label.movie->start();
    return {true, QString()};
}

// QLabel::clear()
void TNullConsoleFrontend::clearLabelContents(Label& label)
{
    label.pixmapSize.reset();
    label.showsMovie = false;
    label.textLabel = false;
    label.sizeHint.reset();
    label.pModel->mText.clear();
}

bool TNullConsoleFrontend::setLabelBackgroundImage(const QString& name, const QString& path)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    Label& label = it->second;
    if (TLabelModel::svgCandidate(path)) {
        QSvgRenderer renderer;
        if (TLabelModel::loadSvg(renderer, path)) {
            label.svgSize = renderer.defaultSize();
            // TLabel::setSvgImage() takes down a raster background but leaves text and movies
            if (label.pixmapSize) {
                clearLabelContents(label);
            }
            return true;
        }
    }
    const QImage raster(path);
    if (raster.isNull()) {
        return false;
    }
    label.svgSize.reset();
    if (label.showsMovie) {
        label.movie->stop();
    }
    clearLabelContents(label);
    label.pixmapSize = raster.deviceIndependentSize().toSize();
    return true;
}

bool TNullConsoleFrontend::resetLabelBackgroundImage(const QString& name)
{
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
    pMovie->start();
    return true;
}

bool TNullConsoleFrontend::pauseLabelMovie(const QString& name)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    pMovie->setPaused(true);
    return true;
}

bool TNullConsoleFrontend::setLabelMovieFrame(const QString& name, const int frame)
{
    QMovie* pMovie = shownMovie(name);
    return pMovie && pMovie->jumpToFrame(frame);
}

bool TNullConsoleFrontend::setLabelMovieSpeed(const QString& name, const int percent)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    pMovie->setSpeed(percent);
    return true;
}

bool TNullConsoleFrontend::scaleLabelMovie(const QString& name, const bool followLabelSize)
{
    QMovie* pMovie = shownMovie(name);
    if (!pMovie) {
        return false;
    }
    Label& label = mLabels.at(name);
    pMovie->setScaledSize(label.pModel->mGeometry.size());
    label.movieFollowsSize = followLabelSize;
    return true;
}

bool TNullConsoleFrontend::moveLabel(const QString& name, const int x, const int y)
{
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
        inheritLabelFont(label, true);
    }
    label.pModel->mGeometry.moveTo(x, y);
    label.shown = show || (!moved && label.shown);
    reportWindowVisibility(name);
    return true;
}

// A label whose parent changed, or whose parent's parent did: a styled one keeps its font unless it
// is the one restyled, and any other takes its new parent's for what it does not set itself
void TNullConsoleFrontend::inheritLabelFont(Label& label, const bool restyle)
{
    const QFont before = label.pModel->mFont;
    if (!label.styled) {
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
        const bool moved = userWindow != it->second.userWindow || box != it->second.box;
        it->second.userWindow = userWindow;
        it->second.box = std::move(box);
        it->second.shown = show || (!moved && it->second.shown);
        moveSubConsole(name, x, y);
        reportWindowVisibility(name);
        return {true, QString()};
    }
    const auto it = mScrollBoxes.find(name);
    if (it == mScrollBoxes.end()) {
        if (PlainWindow* window = plainWindow(name)) {
            const bool moved = userWindow != window->userWindow || box != window->box;
            window->userWindow = userWindow;
            window->box = std::move(box);
            window->shown = show || (!moved && window->shown);
            // QWidget::setParent() restyles a text edit it moves, in its read-only state now
            if (const auto textBox = mTextBoxes.find(name); moved && textBox != mTextBoxes.end() && &textBox->second == window) {
                restyleTextBox(name, textBox->second);
            }
            movePlainWindow(name, x, y);
            reportWindowVisibility(name);
            return {true, QString()};
        }
        if (registry.hasSubConsole(name) || registry.hasPlainWindow(name)) {
            return noView();
        }
        return {false, qsl("element '%1' not found").arg(name)};
    }
    if (within(box.get(), it->second.get())) {
        return {false, qsl("element '%1' cannot be moved into itself or into one of its own children").arg(name)};
    }
    const bool moved = userWindow != it->second->userWindow || box != it->second->box;
    it->second->userWindow = userWindow;
    it->second->box = std::move(box);
    it->second->shown = show || (!moved && it->second->shown);
    moveScrollBoxContents(it->second.get());
    movePlainWindow(name, x, y);
    reportVisibility();
    return {true, QString()};
}

// What a moved scroll box holds goes with it into its user window
void TNullConsoleFrontend::moveScrollBoxContents(const ScrollBox* box)
{
    for (auto& [name, label] : mLabels) {
        if (within(label.box.get(), box) && label.userWindow != box->userWindow) {
            label.userWindow = box->userWindow;
            inheritLabelFont(label, false);
        }
    }
    for (auto& [name, subConsole] : mSubConsoles) {
        if (within(subConsole.box.get(), box)) {
            subConsole.userWindow = box->userWindow;
        }
    }
    for (auto& [name, inner] : mScrollBoxes) {
        if (within(inner->box.get(), box)) {
            inner->userWindow = box->userWindow;
        }
    }
    for (auto& [name, commandLine] : mCommandLines) {
        if (within(commandLine.box.get(), box)) {
            commandLine.userWindow = box->userWindow;
        }
    }
    for (auto& [name, textBox] : mTextBoxes) {
        if (within(textBox.box.get(), box)) {
            textBox.userWindow = box->userWindow;
        }
    }
}

bool TNullConsoleFrontend::setLabelBackgroundColor(const QString& name, const QColor& color)
{
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
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgTintColor = color;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgRotation(const QString& name, const double angle)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgRotation = angle;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgShear(const QString& name, const double shearX, const double shearY)
{
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
    if (!mSubConsoles.count(name)) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
    geometry.setSize(widgetSize(width, height));
    registry.setSubConsoleGeometry(name, geometry);
    registry.setUserWindowSize(name, geometry.size());
    reportGridSize(name);
    return true;
}

bool TNullConsoleFrontend::moveSubConsole(const QString& name, const int x, const int y)
{
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
    registry.setUserWindowStyleSheet(name, userWindowStyleSheet);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setUserWindowTitle(const QString& name, const QString& text)
{
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

// As TConsole::setConsoleBgColor() leaves a sub-console's model
bool TNullConsoleFrontend::setSubConsoleBackgroundColor(const QString& name, const QColor& color)
{
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
        if (!enabled) {
            pModel->mScrolledBackTo.reset();
        }
    }
    return true;
}

// Lands on the line itself, where TMainConsole::scrollWindowTo() lands its first scroll short by the rows of
// the split screen pane it opens
bool TNullConsoleFrontend::scrollWindowTo(const QString& name, const int line, const bool toEnd)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    const int lastLine = pModel->buffer.getLastLineNumber();
    if (toEnd || line >= lastLine) {
        pModel->mScrolledBackTo.reset();
        return true;
    }
    // A buffer is never painted, so a real one never shows itself scrolled back
    if (!pModel->mScrollingEnabled || mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return true;
    }
    pModel->mScrolledBackTo = line < 0 ? std::max(lastLine + line, 0) : line;
    return true;
}

// As TTextEdit::getColumnCount() and getRowCount() count it
void TNullConsoleFrontend::reportGridSize(const QString& name)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return;
    }
    const QSize size = mpHost->windowRegistry().subConsoleGeometry(name).value_or(QRect()).size();
    const QFontMetricsF metrics(pModel->mUpperPaneFont);
    pModel->mGridSize = QSize(qRound(size.width() / metrics.averageCharWidth()), qRound(size.height() / metrics.lineSpacing()));
}

QFont TNullConsoleFrontend::displayFont() const
{
    return mpHost->displayFontWithNoView();
}

std::optional<QFont> TNullConsoleFrontend::consoleFont(const QString& name) const
{
    if (name.isEmpty() || name == qsl("main")) {
        return {displayFont()};
    }
    if (const TConsoleModel* pModel = subConsoleModel(name)) {
        return {pModel->mUpperPaneFont};
    }
    return {};
}

std::optional<QSize> TNullConsoleFrontend::consoleFontSize(const QString& name) const
{
    const auto font = consoleFont(name);
    if (!font) {
        return {};
    }
    const QFontMetrics metrics(*font);
    return {QSize(metrics.horizontalAdvance(QChar('W')), metrics.height())};
}

std::optional<int> TNullConsoleFrontend::consoleColumnWidth(const QString& name) const
{
    const auto font = consoleFont(name);
    if (!font) {
        return {};
    }
    return {QFontMetrics(*font).averageCharWidth()};
}

// As TConsole::setFont() changes a sub-console's, and tells scripts of it
void TNullConsoleFrontend::setSubConsoleFont(const QString& name, const QFont& font)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return;
    }
    pModel->mUpperPaneFont = font;
    reportGridSize(name);
    if (mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return;
    }
    raiseFontEvent(qsl("sysFontChangeEvent"), name, font);
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
        const QFont before = displayFont();
        const auto result = mpHost->setDisplayFont(font, Host::DisplayFontChange::UserChoice);
        reportDisplayFontChange(before);
        return {result};
    }
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return {};
    }
    QFont font = mpHost->createFontWithSettings(family, pModel->mUpperPaneFont.pointSize());
    if (weight != QFont::Normal) {
        font.setWeight(weight);
    }
    if (TFontAttributes(font) != TFontAttributes(pModel->mUpperPaneFont)) {
        setSubConsoleFont(name, font);
    }
    return {{true, QString()}};
}

bool TNullConsoleFrontend::setWindowFontSize(const QString& name, const int size)
{
    if (name.isEmpty() || name == qsl("main")) {
        const QFont before = displayFont();
        mpHost->setDisplayFontSize(size);
        reportDisplayFontChange(before);
        return true;
    }
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    if (pModel->mUpperPaneFont.pointSize() != size) {
        TFontAttributes font(pModel->mUpperPaneFont);
        font.mPointSize = size;
        setSubConsoleFont(name, font.makeFont());
    }
    return true;
}
