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
#include "TAppFrontend.h"
#include "TEvent.h"
#include "TLabelModel.h"
#include "TMap.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
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

// The sheet TCommandLine's constructor gives every command line but the main one
QString commandLineSheet(const QColor& background)
{
    return qsl("QPlainTextEdit{background-color: rgb(%1, %2, %3);}").arg(background.red()).arg(background.green()).arg(background.blue());
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

// TMxpFrameWidgets::frameWidget(): a frame's container, or an external frame's console, which a script's deletion
// of that console, or of the console holding the frame, leaves until deferred deletes run
bool TNullMxpFrameFrontend::hasFrameWidget(const QString& name) const
{
    const auto it = mView.mFrameHolders.find(name);
    return it != mView.mFrameHolders.end() && it->second;
}

void TNullMxpFrameFrontend::createInternalFrame(const QString& name, const QString& hostName, const QString&, const QRect&, bool, bool)
{
    mView.addFrameConsole(name, false, hostName);
}

std::optional<QSize> TNullMxpFrameFrontend::createExternalFrame(const QString& name, const QString&, const QSize& size, bool)
{
    mView.addFrameConsole(name, true);
    return size;
}

void TNullMxpFrameFrontend::createTabFrame(const QString& name, const QString&, const QString& parentName, const QSize&, bool, bool)
{
    mView.addFrameConsole(name, false, parentName);
}

void TNullMxpFrameFrontend::destroyFrame(const QString& name)
{
    mView.removeFrameConsole(name);
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
    // The view's TCommandLine has its own menu items, which go with it
    mMainCommandLine.menuItems.clear();
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

void TNullConsoleFrontend::resumeMainCommandLine()
{
    // A view detached by setMainConsoleView(nullptr) asks for this again as it goes
    if (!mMainCommandLine.writer) {
        // The key a command line named "main" writes is the main command line's
        mMainCommandLine.writer = makeHistoryWriter(mpMainHolder.get(), qsl("main"), *mpHost->windowRegistry().commandLineSavesHistory(QString()));
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

void TNullConsoleFrontend::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool, bool clickThrough)
{
    auto pLabel = std::make_unique<TLabelModel>(mpHost, name);
    pLabel->setClickThrough(clickThrough);
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

bool TNullConsoleFrontend::setLabelClickThrough(const QString& name, const bool clickThrough)
{
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
    if (sheet == model.mStyleSheet && label.repolished && !label.restyleDue && !sheet.contains(qsl("url("), Qt::CaseInsensitive)) {
        return;
    }
    label.restyleDue = false;
    label.repolished = label.styled && !sheet.isEmpty();
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
    auto pHolder = new QObject(holderFor(userWindow, box.get()));
    mSubConsoles[name] = {std::move(pModel), userWindow, std::move(box)};
    mSubConsoles[name].holder = pHolder;
    // As ~TConsole takes one that goes with the widget holding it, such as a frame nested in a deleted one, out
    // of the registry; the QPointer may already be cleared
    QObject::connect(pHolder, &QObject::destroyed, mpHost, [this, name, pHolder]() {
        if (const auto it = mSubConsoles.find(name); it != mSubConsoles.end() && (!it->second.holder || it->second.holder.data() == pHolder)) {
            removeSubConsole(name);
        }
    });
    return model;
}

void TNullConsoleFrontend::removeSubConsole(const QString& name, const bool keepWidget)
{
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
    if (userWindow) {
        // Not as the profile goes, where its close has already saved or held where its windows were, nor as a view
        // takes over, which no script asked for
        if (!mpHost->isClosingDown() && !mDroppingWindows) {
            TAppFrontend::instance()->userWindowRemoved(mpHost, name);
        }
        // removeDockWidget() hides a shown dock, which reports what it holds, but a hidden one's report waits for its
        // deferred deletes
        QStringList deferred;
        const QScopedValueRollback deferring(mpDeferredReports, shown ? mpDeferredReports : &deferred);
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
        std::erase_if(mTextBoxes, [&registry, &name, &boxes](const auto& entry) {
            if (entry.second.userWindow != name) {
                return false;
            }
            registry.deregisterTextBox(entry.first);
            boxes << entry.first;
            return true;
        });
        for (const QString& plainName : boxes) {
            republishPlainWindow(plainName);
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
}

void TNullConsoleFrontend::addFrameConsole(const QString& name, const bool external, const QString& hostFrame)
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
    // TMxpFrameWidgets::createExternalFrame() has no container widget but the console itself
    if (external) {
        mFrameHolders[name] = console.holder;
        return;
    }
    auto pFrame = new QObject(pHost ? pHost : mpMainHolder.get());
    console.holder->setParent(pFrame);
    mFrameHolders[name] = pFrame;
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

bool TNullConsoleFrontend::openUserWindow(const QString& name, const bool loadLayout, bool, const QString&)
{
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
        registry.setUserWindowStyleSheet(name, mpHost->mProfileStyleSheet);
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
    return !gone();
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

std::pair<bool, QString> TNullConsoleFrontend::createMapper(const QString& windowname, int, int, int, int)
{
    // As TMainConsole::createMapper(), which only takes a user window: not a scroll box
    const bool wantsMainConsole = windowname.isEmpty() || !windowname.compare(QLatin1String("main"), Qt::CaseInsensitive);
    if (!wantsMainConsole && !mpHost->windowRegistry().hasDockWidget(windowname)) {
        return {false, qsl("window '%1' not found").arg(windowname)};
    }
    if (mMapWidget && mMapWidget->shown) {
        return {false, qsl("cannot create mapper. Do you already use a map window?")};
    }
    if (!mpEmbeddedMapper && !ownsContainer(windowname)) {
        // A detached view's user window, which only that view could make a mapper in
        return noView();
    }
    if (mMapWidget) {
        // As TMainConsole::createMapper() deleting the hidden map dock leaves QMainWindow its placeholder
        TAppFrontend::instance()->mapWidgetRemoved(mpHost);
        mMapWidget.reset();
    }
    if (!mpEmbeddedMapper) {
        mpEmbeddedMapper = new TNullMapView(mpHost, holderFor(windowname, nullptr));
        mEmbeddedMapperWindow = userWindowOrMain(windowname);
        mpEmbeddedMapperBox.reset();
        // dlgMapper shows itself when a map load asks it to
        QObject::connect(mpHost->mpMap.data(), &TMap::signal_mapperShowRequested, mpEmbeddedMapper, [this]() {
            showEmbeddedMapper();
        });
        copyPlayerRoomStyleToMap();
        mpHost->loadMapForNewMapper(true);
        TEvent mapOpenEvent{};
        mapOpenEvent.mArgumentList.append(QLatin1String("mapOpenEvent"));
        mapOpenEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(mapOpenEvent);
    }
    showEmbeddedMapper();
    return {true, QString()};
}

void TNullConsoleFrontend::createMapperDock(const QString& title, const QString&)
{
    mMapWidget = MapWidget{title, QRect(), false, std::make_unique<TNullMapView>(mpHost)};
    copyPlayerRoomStyleToMap();
}

// As a map load or a script shows the dlgMapper, which leaves a hidden window holding it hidden
void TNullConsoleFrontend::showEmbeddedMapper()
{
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
    if (mMapWidget) {
        mMapWidget->geometry = QRect(QPoint(0, 0), mpHost->mainWindowSize().value_or(QSize()));
    }
    TAppFrontend::instance()->loadWindowLayout();
    placeMapWidget(QString(), -1, -1, -1, -1);
}

std::pair<bool, QString> TNullConsoleFrontend::placeMapWidget(const QString& area, const int x, const int y, const int width, const int height)
{
    if (!mMapWidget) {
        return {false, qsl("cannot create map widget. Do you already use an embedded mapper?")};
    }

    // Shown before the area is checked, as TMainConsole::placeMapWidget() does
    mMapWidget->shown = true;
    mMapWidget->pView->setShown(true);
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
    if (!mMapWidget) {
        return;
    }
    mMapWidget->geometry = placement.geometry;
    mMapWidget->shown = placement.shown;
    mMapWidget->pView->setShown(placement.shown);
}

bool TNullConsoleFrontend::hideMapWidget()
{
    if (!mMapWidget || !mMapWidget->shown) {
        return false;
    }
    mMapWidget->shown = false;
    mMapWidget->pView->setShown(false);
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
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    // As QWidget::setVisible() to what a widget already is, which reports nothing, so leaves what a window still
    // sending its show's resize holds its earlier answer
    if (it->second.shown == shown) {
        return true;
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
        reportWindowVisibility(name);
        return true;
    }
    it->second.shown = shown;
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
        const QPointer<const QObject> window = &it->second.pModel->mNotifier;
        raiseUserWindowResize(name);
        // Its handlers can delete the window, or open another of its name
        if (const auto again = mSubConsoles.find(name); window && again != mSubConsoles.end() && &again->second.pModel->mNotifier == window) {
            again->second.resizePending = false;
            again->second.sendingResize = false;
        }
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
        registry.setSubConsoleVisible(name, it->second.shown && shownWithin(it->second.userWindow, it->second.box.get()));
    }
    if (const auto it = mScrollBoxes.find(name); it != mScrollBoxes.end()) {
        registry.setPlainWindowVisible(name, it->second->shown && shownWithin(it->second->userWindow, it->second->box.get()));
    } else if (const PlainWindow* window = plainWindow(name)) {
        registry.setPlainWindowVisible(name, window->shown && window->layerShown && window->consoleShown && shownWithin(window->userWindow, window->box.get()));
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
}

bool TNullConsoleFrontend::createScrollBox(const QString& windowname, const QString& name, const int x, const int y, const int width, const int height)
{
    if (mScrollBoxes.contains(name)) {
        return false;
    }
    auto box = std::make_shared<ScrollBox>();
    std::tie(box->userWindow, box->box) = containerFor(windowname, false);
    box->holder = new QObject(holderFor(box->userWindow, box->box.get()));
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
    // TCommandLine's constructor takes the setting a profile save last wrote for the name
    const bool savesHistory = std::get<1>(mpHost->getCmdLineSettings(enums::SubCommandLine, name));
    commandLine.writer = makeHistoryWriter(holderFor(commandLine.userWindow, commandLine.box.get()), name, savesHistory);
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
TNullConsoleFrontend::HistoryWriter* TNullConsoleFrontend::makeHistoryWriter(QObject* parent, const QString& name, const bool savesHistory)
{
    auto pWriter = new HistoryWriter(parent);
    pWriter->savesHistory = savesHistory;
    Host* pHost = mpHost;
    QObject::connect(mpHost, &Host::signal_saveCommandLinesHistory, pWriter, [pHost, pWriter, name]() {
        pHost->setCmdLineSettings(enums::SubCommandLine, pWriter->savesHistory, name);
    });
    return pWriter;
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
    } else if (!mpHost->passwordEntryWanted()) {
        // TMainConsole writes to the hidden-input box over it while the game asks for one
        mpHost->windowRegistry().setMainCommandLineText(plainText(text));
    }
    return true;
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
    const bool savesHistory = std::get<1>(mpHost->getCmdLineSettings(enums::ConsoleCommandLine, name));
    PlainWindow& commandLine = mCommandLines[name];
    commandLine.inConsole = true;
    commandLine.userWindow = console->second.userWindow;
    commandLine.box = console->second.box;
    // A child of the console, as TConsole::setCmdVisible() makes it
    commandLine.writer = makeHistoryWriter(console->second.holder ? console->second.holder.data() : mpMainHolder.get(), name, savesHistory);
    console->second.ownLine = commandLine.writer;
    TWindowRegistry& registry = mpHost->windowRegistry();
    registry.registerCommandLine(name);
    registry.setCommandLineSavesHistory(name, savesHistory);
    registry.setCommandLineStyleSheet(name, commandLineSheet(mpHost->mCommandLineBgColor));
    // As TMainConsole::registerSubCommandLine() reports the name, and so the console's own window
    reportWindowVisibility(name);
    return true;
}

// Unlike showWindow(), this finds the command line when a scroll box has the same name
bool TNullConsoleFrontend::setCommandLineVisible(const QString& name, const bool visible)
{
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
    static const QStringList textBoxClasses{qsl("TTextBox"), qsl("QPlainTextEdit"), qsl("QAbstractScrollArea"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")};
    static const QStringList commandLineClasses{qsl("TCommandLine"), qsl("QPlainTextEdit"), qsl("QAbstractScrollArea"), qsl("QFrame"), qsl("QWidget"), qsl("QObject")};
    const QString objectName = (commandLine ? qsl("commandLine_%1_%2") : qsl("textEdit_%1_%2")).arg(mpHost->getName(), name);
    const QString declarations =
            sheetDeclarations(window.styleSheet, commandLine ? commandLineClasses : textBoxClasses, objectName, {qsl("enabled"), window.readOnly ? qsl("read-only") : qsl("editable")});
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
    updateShownContent(label);
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
    }
    updateShownContent(label);
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
        QObject* pParent = holderFor(userWindow, box.get());
        // A frame's console also moves when it leaves the frame for the window the frame is in
        const bool moved = userWindow != it->second.userWindow || box != it->second.box || (it->second.holder && it->second.holder->parent() != pParent);
        it->second.userWindow = userWindow;
        it->second.box = std::move(box);
        it->second.shown = show || (!moved && it->second.shown);
        if (it->second.holder) {
            it->second.holder->setParent(pParent);
        }
        if (const auto line = mCommandLines.find(name); line != mCommandLines.end() && ownsLine(it->second, line->second)) {
            line->second.userWindow = it->second.userWindow;
            line->second.box = it->second.box;
        }
        moveSubConsole(name, x, y);
        reportWindowVisibility(name);
        return {true, QString()};
    }
    const auto it = mScrollBoxes.find(name);
    if (it == mScrollBoxes.end()) {
        if (PlainWindow* window = plainWindow(name)) {
            // A console's own command line leaves the console's layer for the window it is moved into
            const bool moved = window->inConsole || userWindow != window->userWindow || box != window->box;
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
            if (const auto textBox = mTextBoxes.find(name); moved && textBox != mTextBoxes.end() && &textBox->second == window) {
                restylePlainWindow(name, textBox->second, false);
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
            mEmbeddedMapperShown = show || (!moved && mEmbeddedMapperShown);
            mEmbeddedMapperWindow = userWindow;
            mpEmbeddedMapperBox = std::move(box);
            reportEmbeddedMapperVisibility();
            return {true, QString()};
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
    if (it->second->holder) {
        it->second->holder->setParent(holderFor(it->second->userWindow, it->second->box.get()));
    }
    moveScrollBoxContents(it->second.get());
    movePlainWindow(name, x, y);
    reportVisibility(QString(), it->second.get());
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
    if (within(mpEmbeddedMapperBox.get(), box)) {
        mEmbeddedMapperWindow = box->userWindow;
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
    // A hidden dock's console takes the size as it next shows
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end() || !it->second.shown || it->second.consoleSize == geometry.size() || registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
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
