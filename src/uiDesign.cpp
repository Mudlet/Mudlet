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

#include "uiDesign.h"

#include "MudletApp.h"
#include "TKeySequenceEdit.h"
#include "utils.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStandardPaths>
#include <QtMath>
#include <QStyle>
#include <QStyleOptionGroupBox>
#include <QStyleOptionViewItem>
#include <QSvgRenderer>
#include <iterator>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QTransform>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <cmath>
#include <tuple>
#include <utility>

namespace uiDesign {

namespace {
constexpr qreal scmStateSaturation = 0.55;
// A red at scmStateSaturation reads as salmon, so the error hue carries its own
constexpr qreal scmErrorStateSaturation = 0.85;
constexpr qreal scmStateLightnessOnDark = 0.58;
constexpr qreal scmStateLightnessOnLight = 0.36;
constexpr qreal scmMarkerHue = 0.13;
constexpr qreal scmMarkerSaturation = 0.9;
constexpr qreal scmMarkerLightnessOnDark = 0.34;
constexpr qreal scmMarkerLightnessOnLight = 0.72;
// The same lightness step reads far larger on a dark page, so it takes a fraction of the light one's lift
constexpr qreal scmCardLiftOnDark = 0.06;
constexpr qreal scmCardLiftOnLight = 0.55;
// macOS's light appearance answers white for Window and Base alike, and a card lifted off a white page lands back
// on it, so the page steps down instead
constexpr int scmMinimumCardLift = 6;
constexpr qreal scmPageDropUnderCard = 0.05;
// A fraction of the card's lift rather than of the room above the page, so it stays small on both themes with one
// number
constexpr qreal scmPaneLiftTowardsCard = 0.2;
// A light page is near white, where the dark page's drop would draw a grey rule rather than a seam
constexpr qreal scmSeparatorDropOnDark = 0.36;
constexpr qreal scmSeparatorDropOnLight = 0.10;
// A palette's pure white or black WindowText glares; a light theme gives up less before small type fades
constexpr qreal scmTextSoftenOnDark = 0.16;
constexpr qreal scmTextSoftenOnLight = 0.10;
// A dark theme's near-black Base reads as a hole in the window, and a check box on a card as a black well in a bright
// ring; lifting a light theme's white Base only muddies it
constexpr qreal scmFieldLiftOnDark = 0.85;
// Only a starting point: on a light theme it lands near 2.3:1, so themeTokens() walks it on to scmQuietMinimumRatio
constexpr qreal scmDisabledTextWeight = 0.32;
constexpr qreal scmDisabledBarTextWeight = 0.20;
constexpr qreal scmMutedTextWeight = 0.70;
constexpr qreal scmAccentTextLiftOnDark = 0.45;
constexpr qreal scmAccentTextDropOnLight = 0.2;
constexpr qreal scmToneWeightStep = 0.02;
constexpr int scmReadabilitySteps = 12;
constexpr int scmInputArrowSize = 8;
constexpr int scmInputStepperArrowSize = 7;
// Twice the body's pressed wash, since the half is drawn over a body already carrying it
constexpr qreal scmSplitHalfPressedWash = 2 * scmSoftWashStrength;
constexpr int scmSidebarItemGutter = 10;
constexpr int scmSidebarItemPadding = scmSidebarItemGutter - scmAccentBarWidth;
constexpr int scmSidebarFocusRingWidth = 1;
// The same on both themes: any less on a light one falls under the 3:1 a control's boundary needs on a white card
constexpr qreal scmChoiceOutlineOnDark = 0.55;
constexpr qreal scmChoiceOutlineOnLight = 0.55;
// Off the radius scale: the box is a glyph, not a box a window is laid out in
constexpr int scmChoiceBoxRadius = 3;
constexpr int scmChoiceLabelSpacing = 6;
// A branch cell's width, which is a view's own indentation step
constexpr int scmTreeBranchBoxSize = 16;
constexpr int scmTreeBranchGlyphSize = 10;
constexpr qreal scmDisabledFieldTowardsPage = 0.6;
constexpr qreal scmDisabledBorderWeight = 0.10;
constexpr qreal scmButtonFaceLiftOnDark = 0.08;
constexpr qreal scmButtonFaceLiftOnLight = 0.04;
constexpr int scmButtonPaddingHorizontal = 12;
// More than a field's 2px, which leaves the glyph touching the frame
constexpr int scmDisclosurePaddingVertical = 5;
constexpr int scmButtonMenuIndicatorInset = 6;
// Keeps the first and last rows inside the frame's rounded corner
constexpr int scmMenuSurfacePadding = 4;
constexpr int scmMenuItemPaddingLeading = scmChoiceIndicatorSize + scmChoiceLabelSpacing;
constexpr int scmMenuItemPaddingTrailing = scmButtonPaddingHorizontal;
// A sub-control is placed from the row's padding box, the edge its highlight starts at, so a mark left there sits on
// the highlight's arc
constexpr int scmMenuItemMarkInset = (scmMenuItemPaddingLeading - scmChoiceIndicatorSize) / 2;

constexpr qreal scmTypeRatio_caption = 0.85;
constexpr qreal scmTypeRatio_title = 1.15;
constexpr qreal scmTypeRatio_display = 1.45;

qreal linearised(const qreal channel)
{
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

qreal relativeLuminance(const QColor& color)
{
    return 0.2126 * linearised(color.redF()) + 0.7152 * linearised(color.greenF()) + 0.0722 * linearised(color.blueF());
}
} // namespace

bool alignInLayoutTree(QLayout* pLayout, const QWidget* pWidget, const Qt::Alignment alignment)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (pItem->widget() == pWidget) {
            pItem->setAlignment(alignment);
            return true;
        }
        if (QLayout* pChildLayout = pItem->layout(); pChildLayout && alignInLayoutTree(pChildLayout, pWidget, alignment)) {
            return true;
        }
    }
    return false;
}

bool removeFromLayoutTree(QLayout* pLayout, QWidget* pWidget)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (pItem->widget() == pWidget) {
            delete pLayout->takeAt(i);
            pLayout->invalidate();
            return true;
        }
        if (QLayout* pChildLayout = pItem->layout(); pChildLayout && removeFromLayoutTree(pChildLayout, pWidget)) {
            return true;
        }
    }
    return false;
}

void detachFromLayout(QWidget* pWidget)
{
    QWidget* pParent = pWidget->parentWidget();
    if (QLayout* pLayout = pParent ? pParent->layout() : nullptr; pLayout) {
        removeFromLayoutTree(pLayout, pWidget);
    }
}

// Layout changes are posted, and no event loop runs before we measure, so flush by hand:
// invalidate() drops the layout's cache, updateGeometry() the widget's size hints.
void invalidateLayoutsUpTo(QWidget* pWidget, const QWidget* pTop)
{
    for (QWidget* pAncestor = pWidget; pAncestor; pAncestor = pAncestor->parentWidget()) {
        if (QLayout* pLayout = pAncestor->layout(); pLayout) {
            pLayout->invalidate();
        }
        pAncestor->updateGeometry();
        if (pAncestor == pTop) {
            return;
        }
    }
}

void markAsShellSurface(QWidget* pWidget)
{
    pWidget->setProperty("settingsSurface", true);
}

void insertGridRowAtTop(QGridLayout* pGrid, QWidget* pWidget)
{
    const int rows = pGrid->rowCount();
    const int columns = std::max(1, pGrid->columnCount());
    QList<std::pair<int, int>> rowProperties;
    rowProperties.reserve(rows);
    for (int row = 0; row < rows; ++row) {
        rowProperties.append({pGrid->rowStretch(row), pGrid->rowMinimumHeight(row)});
    }

    QList<std::tuple<QLayoutItem*, int, int, int, int>> items;
    items.reserve(pGrid->count());
    while (pGrid->count()) {
        int row = 0;
        int column = 0;
        int rowSpan = 1;
        int columnSpan = 1;
        pGrid->getItemPosition(0, &row, &column, &rowSpan, &columnSpan);
        items.append({pGrid->takeAt(0), row, column, rowSpan, columnSpan});
    }

    pGrid->addWidget(pWidget, 0, 0, 1, columns);
    for (const auto& [pItem, row, column, rowSpan, columnSpan] : items) {
        pGrid->addItem(pItem, row + 1, column, rowSpan, columnSpan, pItem->alignment());
    }
    pGrid->setRowStretch(0, 0);
    pGrid->setRowMinimumHeight(0, 0);
    for (int row = 0; row < rows; ++row) {
        pGrid->setRowStretch(row + 1, rowProperties.at(row).first);
        pGrid->setRowMinimumHeight(row + 1, rowProperties.at(row).second);
    }
}

void buildControlSentenceRow(QBoxLayout* pRow, const QString& translatedSentence, QWidget* pControl)
{
    // A screen reader announces a field by its own name and not by the labels
    // beside it, so the control is given the whole sentence it sits in
    pControl->setAccessibleName(QString(translatedSentence).remove(qsl("%1")).simplified());
    buildControlSentenceRow(pRow, translatedSentence, QList<QWidget*>{pControl});
}

void buildControlSentenceRow(QBoxLayout* pRow, const QString& translatedSentence, const QList<QWidget*>& controls)
{
    // Null while the row is a bare layout waiting to be added to another one,
    // and Qt reparents everything in it at that point
    QWidget* pParent = pRow->parentWidget();

    // The gaps between the words and the controls are the row's own spacing, so
    // each run of words is trimmed of what would otherwise double it
    const auto addWords = [pRow, pParent](const QString& words) {
        const QString trimmed = words.trimmed();
        if (!trimmed.isEmpty()) {
            pRow->addWidget(new QLabel(trimmed, pParent));
        }
    };

    QList<bool> placed(controls.size(), false);
    static const QRegularExpression placeholderPattern(qsl("%(\\d+)"));
    qsizetype wordsFrom = 0;
    QRegularExpressionMatchIterator placeholders = placeholderPattern.globalMatch(translatedSentence);
    while (placeholders.hasNext()) {
        const QRegularExpressionMatch placeholder = placeholders.next();
        const int index = placeholder.captured(1).toInt() - 1;
        if (index < 0 || index >= controls.size() || placed.at(index)) {
            continue;
        }
        addWords(translatedSentence.mid(wordsFrom, placeholder.capturedStart() - wordsFrom));
        pRow->addWidget(controls.at(index));
        placed[index] = true;
        wordsFrom = placeholder.capturedEnd();
    }
    addWords(translatedSentence.mid(wordsFrom));

    // A translation that lost a placeholder is a mistake in the .ts file, and
    // the row it produces still has to hold every control it was given
    for (qsizetype index = 0; index < controls.size(); ++index) {
        if (!placed.at(index)) {
            pRow->addWidget(controls.at(index));
        }
    }
}

void makeChevronRow(QAbstractButton* pButton)
{
    pButton->setProperty("settingsChevronRow", true);
    pButton->setCursor(Qt::PointingHandCursor);
    pButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    // Search-result rows are made long after the shell's own focus pass ran
    keepClickFocusOffControls(pButton);
}

void collectFocusableInLayoutOrder(const QLayout* pLayout, QList<QWidget*>& chain)
{
    for (int i = 0, total = pLayout->count(); i < total; ++i) {
        QLayoutItem* pItem = pLayout->itemAt(i);
        if (QWidget* pWidget = pItem->widget(); pWidget) {
            if ((pWidget->focusPolicy() & Qt::TabFocus) == Qt::TabFocus) {
                chain.append(pWidget);
            }
            if (const QLayout* pChildLayout = pWidget->layout(); pChildLayout) {
                collectFocusableInLayoutOrder(pChildLayout, chain);
            }
        } else if (const QLayout* pChildLayout = pItem->layout(); pChildLayout) {
            collectFocusableInLayoutOrder(pChildLayout, chain);
        }
    }
}

QString spotlightStyleSheet(const QColor& accent, const qreal strength)
{
    // Drawn round a card, so it takes a card's corner
    return qsl("#settingsSpotlight { border: 2px solid rgba(%1, %2, %3, %4); border-radius: %6px; background-color: rgba(%1, %2, %3, %5); }")
            .arg(QString::number(accent.red()), QString::number(accent.green()), QString::number(accent.blue()), QString::number(strength, 'f', 3), QString::number(strength * 0.08, 'f', 3))
            .arg(QString::number(scmRadiusPanel));
}

QString foldForSearch(const QString& text)
{
    QString plain;
    plain.reserve(text.size());
    bool inTag = false;
    for (const QChar character : text) {
        if (character == QLatin1Char('<')) {
            inTag = true;
        } else if (character == QLatin1Char('>')) {
            inTag = false;
        } else if (!inTag && character != QLatin1Char('&')) {
            plain.append(character);
        }
    }

    const QString decomposed = plain.normalized(QString::NormalizationForm_KD);
    QString folded;
    folded.reserve(decomposed.size());
    for (const QChar character : decomposed) {
        if (character.category() != QChar::Mark_NonSpacing) {
            folded.append(character);
        }
    }
    return folded.simplified().toCaseFolded();
}

QString visibleTextOf(const QWidget* pWidget)
{
    if (const auto* pLabel = qobject_cast<const QLabel*>(pWidget); pLabel) {
        return pLabel->text();
    }
    if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pWidget); pGroupBox) {
        return pGroupBox->title();
    }
    if (const auto* pButton = qobject_cast<const QAbstractButton*>(pWidget); pButton) {
        return pButton->text();
    }
    return QString();
}

void collectSearchText(const QWidget* pWidget, QStringList& parts)
{
    parts << pWidget->property(scmProp_searchKeywords).toString() << pWidget->toolTip();
    const auto* pComboBox = qobject_cast<const QComboBox*>(pWidget);
    if (!pComboBox) {
        parts << visibleTextOf(pWidget);
        return;
    }
    // Not a font picker's items: they are the installed fonts, which would make its card match "color" or "mono"
    if (qobject_cast<const QFontComboBox*>(pWidget)) {
        return;
    }
    for (int i = 0, total = pComboBox->count(); i < total; ++i) {
        parts << pComboBox->itemText(i);
    }
}

QString highlightTextOf(const QWidget* pWidget)
{
    const QString text = visibleTextOf(pWidget);
    if (text.isEmpty()) {
        return QString();
    }
    const QString keywords = pWidget->property(scmProp_searchKeywords).toString();
    return keywords.isEmpty() ? text : qsl("%1 %2").arg(text, keywords);
}

bool wordEnoughToSearch(const QStringList& needles)
{
    for (const QString& needle : needles) {
        if (needle.size() >= 2) {
            return true;
        }
        switch (needle.at(0).script()) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
        case QChar::Script_Hangul:
            return true;
        default:
            break;
        }
    }
    return false;
}

void repolish(QWidget* pWidget)
{
    pWidget->style()->unpolish(pWidget);
    pWidget->style()->polish(pWidget);
    pWidget->update();
}

void setSearchMatch(QWidget* pWidget, const QVariant& matched)
{
    pWidget->setProperty("searchMatch", matched);
    repolish(pWidget);
}

StyleSheetFontLift::StyleSheetFontLift(QWidget* pWidget)
{
    // a widget takes nothing from a parent that carries the flag, whatever its own says
    if (auto* pParent = pWidget->parentWidget(); pParent && pParent->testAttribute(Qt::WA_StyleSheet)) {
        pParent->setAttribute(Qt::WA_StyleSheet, false);
        mLifted.append(pParent);
    }
    lift(pWidget);
}

void StyleSheetFontLift::lift(QWidget* pWidget)
{
    if (pWidget->testAttribute(Qt::WA_StyleSheet)) {
        pWidget->setAttribute(Qt::WA_StyleSheet, false);
        mLifted.append(pWidget);
    }
    for (auto* pChild : pWidget->children()) {
        auto* pChildWidget = qobject_cast<QWidget*>(pChild);
        // a popup or a window of its own is a font tree of its own, as it is for Qt
        if (pChildWidget && !pChildWidget->isWindow()) {
            lift(pChildWidget);
        }
    }
}

StyleSheetFontLift::~StyleSheetFontLift()
{
    for (const auto& pWidget : mLifted) {
        if (pWidget) {
            pWidget->setAttribute(Qt::WA_StyleSheet, true);
        }
    }
}

void resolveFontsUnderStyleSheets(QWidget* pRoot)
{
    const StyleSheetFontLift lift(pRoot);
    QEvent fontEvent(QEvent::ApplicationFontChange);
    const auto resolve = [&fontEvent](QWidget* pWidget) {
        // Qt hands its own "qt_" internals the parent's font with its whole resolve mask at polish, which would pin them
        if (pWidget->objectName().startsWith(QLatin1String("qt_"))) {
            pWidget->setFont(QFont());
        } else {
            QCoreApplication::sendEvent(pWidget, &fontEvent);
        }
    };
    if (!pRoot->isWindow()) {
        resolve(pRoot);
    }
    for (auto* pChild : pRoot->findChildren<QWidget*>()) {
        if (pChild->window() == pRoot->window()) {
            resolve(pChild);
        }
    }
}

QColor blend(const QColor& from, const QColor& to, const qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount, from.greenF() + (to.greenF() - from.greenF()) * amount, from.blueF() + (to.blueF() - from.blueF()) * amount);
}

namespace {
QString rgbaText(const QColor& color, const qreal alpha)
{
    return qsl("rgba(%1, %2, %3, %4)").arg(QString::number(color.red()), QString::number(color.green()), QString::number(color.blue()), QString::number(alpha, 'f', 3));
}
} // namespace

QString rgba(const QColor& color, const qreal alpha)
{
    return rgbaText(color, alpha);
}

QString rgba(const QColor& colourWithAlpha)
{
    return rgbaText(colourWithAlpha, colourWithAlpha.alphaF());
}

void paintAccentBar(QPainter* pPainter, const QRect& row, const QColor& accent, const int cornerRadius)
{
    pPainter->save();
    // A rect clip keeps the trailing edge on whole pixels; the antialiased path gives both ends the pill's own arc
    pPainter->setClipRect(QRect(row.left(), row.top(), scmAccentBarWidth, row.height()), Qt::IntersectClip);
    pPainter->setRenderHint(QPainter::Antialiasing);
    QPainterPath pill;
    pill.addRoundedRect(row, cornerRadius, cornerRadius);
    pPainter->fillPath(pill, accent);
    pPainter->restore();
}

qreal contrastRatio(const QColor& first, const QColor& second)
{
    const qreal firstLuminance = relativeLuminance(first);
    const qreal secondLuminance = relativeLuminance(second);
    return (std::max(firstLuminance, secondLuminance) + 0.05) / (std::min(firstLuminance, secondLuminance) + 0.05);
}

QColor readableOn(const QColor& background, const QColor& wanted, const QColor& fallback, const qreal minimumRatio)
{
    if (!wanted.isValid() || !background.isValid()) {
        return wanted.isValid() ? wanted : fallback;
    }
    if (contrastRatio(background, wanted) >= minimumRatio) {
        return wanted;
    }
    // Towards the end of the lightness axis rather than the text colour: the hue says what this is, so only the
    // lightness is spent
    const QColor limit = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black); // theme-fixed: the ends of the lightness axis walked along below, not colours of a theme
    for (int step = 1; step <= scmReadabilitySteps; ++step) {
        const QColor moved = blend(wanted, limit, static_cast<qreal>(step) / scmReadabilitySteps);
        if (contrastRatio(background, moved) >= minimumRatio) {
            return moved;
        }
    }
    return fallback;
}

ThemeTokens themeTokens()
{
    // The application palette, not a widget's: a stylesheet freezes its widget's palette, and a PaletteChange is still
    // undelivered while a window restyles. mudlet::setAppearance() swaps qApp's synchronously.
    return themeTokens(QApplication::palette());
}

ThemeTokens themeTokens(const QPalette& themePalette)
{
    ThemeTokens tokens;
    tokens.accent = themePalette.color(QPalette::Highlight);
    // Window, not Base: a page mixed off Base is darker than the fields on it, which cannot read as depth
    tokens.page = themePalette.color(QPalette::Window);
    tokens.darkPage = tokens.page.lightness() < 128;
    tokens.card = blend(tokens.page, QColor(Qt::white), tokens.darkPage ? scmCardLiftOnDark : scmCardLiftOnLight);
    if (tokens.card.lightness() - tokens.page.lightness() < scmMinimumCardLift) {
        tokens.card = tokens.page;
        tokens.page = blend(tokens.page, QColor(Qt::black), scmPageDropUnderCard);
    }
    // After the step above, which may move the page the words are softened towards
    tokens.text = blend(themePalette.color(QPalette::WindowText), tokens.page, tokens.darkPage ? scmTextSoftenOnDark : scmTextSoftenOnLight);
    tokens.field = tokens.darkPage ? blend(themePalette.color(QPalette::Base), tokens.page, scmFieldLiftOnDark) : themePalette.color(QPalette::Base);
    tokens.pane = blend(tokens.page, tokens.card, scmPaneLiftTowardsCard);
    tokens.separator = blend(tokens.page, QColor(Qt::black), tokens.darkPage ? scmSeparatorDropOnDark : scmSeparatorDropOnLight);
    // From page and text, the one pair a palette must keep apart: Mudlet's light appearance has Window, Base and Mid
    // within three levels, so a border mixed from those is invisible
    tokens.border = blend(tokens.page, tokens.text, tokens.darkPage ? 0.22 : 0.18);
    tokens.mutedText = blend(tokens.page, tokens.text, scmMutedTextWeight);
    // Muted words are still body text, so they are held to scmTextMinimumRatio on every surface
    for (qreal weight = scmMutedTextWeight; weight < 1.0
                                            && (contrastRatio(tokens.mutedText, tokens.page) < scmTextMinimumRatio || contrastRatio(tokens.mutedText, tokens.pane) < scmTextMinimumRatio
                                                || contrastRatio(tokens.mutedText, tokens.card) < scmTextMinimumRatio);) {
        weight = std::min(1.0, weight + scmToneWeightStep);
        tokens.mutedText = blend(tokens.page, tokens.text, weight);
    }
    tokens.disabledText = blend(tokens.page, tokens.text, scmDisabledTextWeight);
    for (qreal weight = scmDisabledTextWeight;
         weight < 1.0
         && (contrastRatio(tokens.disabledText, tokens.page) < scmQuietMinimumRatio || contrastRatio(tokens.disabledText, tokens.pane) < scmQuietMinimumRatio
             || contrastRatio(tokens.disabledText, tokens.card) < scmQuietMinimumRatio || contrastRatio(tokens.disabledText, tokens.field) < scmQuietMinimumRatio);) {
        weight = std::min(1.0, weight + scmToneWeightStep);
        tokens.disabledText = blend(tokens.page, tokens.text, weight);
    }
    // Only the page: every bar is drawn on it, where disabledText is held up by the card it must also read on
    tokens.disabledBarText = blend(tokens.page, tokens.text, scmDisabledBarTextWeight);
    for (qreal weight = scmDisabledBarTextWeight; weight < 1.0 && contrastRatio(tokens.disabledBarText, tokens.page) < scmDisabledBarMinimumRatio;) {
        weight = std::min(1.0, weight + scmToneWeightStep);
        tokens.disabledBarText = blend(tokens.page, tokens.text, weight);
    }
    // Drawn on an accent wash wherever it appears, so walked until it reads on the deepest one over each surface
    {
        const QColor limit = tokens.darkPage ? QColor(Qt::white) : QColor(Qt::black); // theme-fixed: the ends of the lightness axis walked along below, not colours of a theme
        const QColor started = blend(tokens.accent, limit, tokens.darkPage ? scmAccentTextLiftOnDark : scmAccentTextDropOnLight);
        const QList<QColor> washes{
                blend(tokens.page, tokens.accent, scmAccentWashStrength), blend(tokens.pane, tokens.accent, scmAccentWashStrength), blend(tokens.card, tokens.accent, scmAccentWashStrength)};
        const auto readsOnEveryWash = [&washes](const QColor& ink) {
            return std::all_of(washes.cbegin(), washes.cend(), [&ink](const QColor& wash) {
                return contrastRatio(ink, wash) >= scmTextMinimumRatio;
            });
        };
        tokens.accentText = started;
        for (int step = 1; step <= scmReadabilitySteps && !readsOnEveryWash(tokens.accentText); ++step) {
            tokens.accentText = blend(started, limit, static_cast<qreal>(step) / scmReadabilitySteps);
        }
    }
    tokens.marker = QColor::fromHslF(scmMarkerHue, scmMarkerSaturation, tokens.darkPage ? scmMarkerLightnessOnDark : scmMarkerLightnessOnLight);
    // Mixed once as colours and written out of them, so the sheet a rule carries
    // and the fill a painter reaches for cannot come apart
    tokens.hoverWash = tokens.text;
    tokens.hoverWash.setAlphaF(scmHoverWashStrength);
    tokens.accentWash = tokens.accent;
    tokens.accentWash.setAlphaF(scmSoftWashStrength);
    tokens.hoverSoft = rgba(tokens.hoverWash);
    tokens.accentSoft = rgba(tokens.accentWash);
    return tokens;
}

QColor stateColor(const qreal hue, const bool darkPage)
{
    const qreal saturation = qFuzzyCompare(hue, scmStateHue_error) ? scmErrorStateSaturation : scmStateSaturation;
    return QColor::fromHslF(hue, saturation, darkPage ? scmStateLightnessOnDark : scmStateLightnessOnLight);
}

QColor errorInk(const ThemeTokens& tokens)
{
    return readableOn(tokens.separator, stateColor(scmStateHue_error, tokens.darkPage), tokens.text, scmTextMinimumRatio);
}

QString scrollBarStyleSheet(const QString& selectorPrefix, const ThemeTokens& tokens, const QColor& surface)
{
    const QColor groove = surface.isValid() ? surface : tokens.page;
    return qsl("%1 QScrollBar:vertical { background-color: %2; width: 12px; margin: 0px; border: none; }"
               "%1 QScrollBar:horizontal { background-color: %2; height: 12px; margin: 0px; border: none; }"
               "%1 QScrollBar::handle:vertical { background-color: %3; border-radius: 5px; min-height: 32px; margin: 1px; }"
               "%1 QScrollBar::handle:horizontal { background-color: %3; border-radius: 5px; min-width: 32px; margin: 1px; }"
               "%1 QScrollBar::handle:hover { background-color: %4; }"
               "%1 QScrollBar::add-line, %1 QScrollBar::sub-line { width: 0px; height: 0px; }"
               "%1 QScrollBar::add-page, %1 QScrollBar::sub-page { background-color: %2; }")
            .arg(selectorPrefix, groove.name(), blend(groove, tokens.text, 0.22).name(), blend(groove, tokens.text, 0.40).name());
}

QString itemRowStyleSheet(const QString& viewSelector, const ThemeTokens& tokens, const QColor& surface, const int rowGutter)
{
    const QColor selectedRow = blend(surface, tokens.accent, scmAccentWashStrength);
    const QString row = viewSelector + qsl("::item");
    return viewSelector + qsl(" { background-color: %1; border: none; outline: none; show-decoration-selected: 1; }").arg(surface.name()) + row
           + qsl(" { border-radius: %1px; border-left: %2px solid transparent; padding: %3px %4px %3px %5px; }")
                     .arg(QString::number(scmRadiusPanel), QString::number(scmAccentBarWidth), QString::number(scmItemRowPaddingVertical))
                     .arg(QString::number(scmItemRowPaddingTrailing), QString::number(rowGutter - scmAccentBarWidth))
           + row + qsl(":hover { background-color: %1; }").arg(tokens.hoverSoft) + row + qsl(":selected { color: %1; background-color: %2; }").arg(tokens.accentText.name(), selectedRow.name());
}

QString sidebarStyleSheet(const QString& listName, const QString& separatorName, const QColor& itemColor, const SidebarMetrics& metrics, const ThemeTokens& tokens)
{
    // A border-left accent bar is drawn as an arc at the pill's corner radius, pinching to nothing at both
    // ends; a gradient is clipped by the radius instead, keeping the bar's width and the rounded corners.
    const auto barStop = [](const int width, const int padding) {
        return static_cast<qreal>(scmAccentBarWidth) / std::max(1, width - 2 * padding);
    };
    // Two stops a ten-thousandth apart rather than one: a gradient is
    // interpolated between its stops, and the bar has to end rather than fade
    const auto pillFill = [&tokens](const qreal stop) {
        return qsl("qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:%2 %1, stop:%3 %4, stop:1 %4)")
                .arg(tokens.accent.name(), QString::number(stop, 'f', 5), QString::number(stop + 0.0001, 'f', 5), tokens.accentSoft);
    };

    const QString list = QLatin1Char('#') + listName;
    const QString separator = QLatin1Char('#') + separatorName;
    const QString accentBar = QString::number(scmAccentBarWidth);
    const QString focusRing = QString::number(scmSidebarFocusRingWidth);
    const QString itemPadding = QString::number(scmSidebarItemPadding);
    const QString focusedItemPadding = QString::number(scmSidebarItemPadding - 2 * scmSidebarFocusRingWidth);
    const QString railProperty = QLatin1StringView(scmProp_rail);
    const QString focusedProperty = QLatin1StringView(scmProp_focused);
    // Or the platform style draws a square selection inside the rounded pill
    return list
           + qsl(" { background: transparent; border: none; outline: none; show-decoration-selected: 1;"
                 " selection-background-color: transparent; selection-color: %1; }")
                     .arg(tokens.accentText.name())
           // The transparent left border stops a selected item's text shifting under its accent bar;
           // outline:none drops a square focus rectangle inside the round pill
           + list + qsl("::item { border-radius: 8px; border-left: %1px solid transparent; padding-left: %2px; color: %3; outline: none; }").arg(accentBar, itemPadding, itemColor.name()) + list
           + qsl("::item:hover { background-color: %1; }").arg(tokens.hoverSoft)
           + list
           // No font-weight: a sheet's font on an ::item never reaches the painter, so SidebarItemDelegate sets the bold
           + qsl("::item:selected { color: %1; background: %2; }").arg(tokens.accentText.name(), pillFill(barStop(metrics.expandedWidth, metrics.padding)))
           // Otherwise keyboard focus looks like selection; an event filter on the list sets scmProp_focused
           + list
           + qsl("[%1=\"true\"]::item:selected { border: %2px solid %3; border-left: %4px solid %3; padding-left: %5px; }")
                     .arg(focusedProperty, focusRing, tokens.accent.name(), accentBar, focusedItemPadding)
           // On a rail the item is only as wide as its icon: no padding, and its own bar fraction
           + list + qsl("[%1=\"true\"]::item { padding-left: 0px; }").arg(railProperty) + list
           + qsl("[%1=\"true\"]::item:selected { background: %2; }").arg(railProperty, pillFill(barStop(metrics.railWidth, metrics.railPadding))) + separator
           + qsl(" { border: none; background-color: %1; margin: 8px %2px; }").arg(tokens.border.name(), QString::number(metrics.separatorInset)) + separator
           + qsl("[%1=\"true\"] { margin: 8px 2px; }").arg(railProperty);
}

int sidebarRowWidth(const QListWidget* pList, const QString& name)
{
    if (!pList) {
        return 0;
    }
    QStyleOptionViewItem option;
    option.initFrom(pList);
    // The chosen row is drawn bold, so it is the bold name that has to fit
    QFont nameFont = pList->font();
    nameFont.setBold(true);
    option.font = nameFont;
    option.fontMetrics = QFontMetrics(nameFont);
    option.text = name;
    option.features |= QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasDecoration;
    // The list's icon size, not scmSidebarIconSize: the editor has a preference that changes it
    option.decorationSize = pList->iconSize();
    option.decorationPosition = QStyleOptionViewItem::Left;
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    QPixmap blank(pList->iconSize());
    blank.fill(Qt::transparent);
    option.icon = QIcon(blank);
    // No widget, so a stylesheet style defers to the style underneath, whose margins the rows are drawn with
    option.widget = nullptr;
    const int fromTheStyle = qApp->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), nullptr).width();
    // What the sheet adds to every row, which no style knows about
    return fromTheStyle + scmAccentBarWidth + scmSidebarItemPadding;
}

bool setSidebarCollapsed(QWidget* pPane, QListWidget* pList, const QString& separatorName, const bool collapsed, const SidebarMetrics& metrics)
{
    const int wanted = collapsed ? metrics.railWidth : metrics.expandedWidth;
    if (collapsed == pList->property(scmProp_rail).toBool() && pPane->width() == wanted) {
        return false;
    }
    const int padding = collapsed ? metrics.railPadding : metrics.padding;
    pPane->setFixedWidth(wanted);
    pPane->layout()->setContentsMargins(padding, metrics.verticalPadding, padding, metrics.verticalPadding);
    pList->setProperty(scmProp_rail, collapsed);
    repolish(pList);
    for (auto* pSeparator : pList->findChildren<QFrame*>(separatorName)) {
        pSeparator->setProperty(scmProp_rail, collapsed);
        repolish(pSeparator);
    }
    pList->doItemsLayout();
    return true;
}

// A stylesheet takes pictures only from files and cannot recolour them, so tinted glyphs are written here. The
// colour is part of each file name: Qt caches a sheet's images by path.
// A folder per build, so a glyph a later build draws differently is drawn again. A folder no build has written to
// for a week goes when another build makes its own
static QString glyphCacheFile(const QString& fileName)
{
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        return QString();
    }
    QString build = MudletApp::scmVersion();
    build.replace(QRegularExpression(qsl("[^A-Za-z0-9._-]")), qsl("_"));
    const QDir glyphs(cacheRoot + qsl("/ui-glyphs"));
    if (!glyphs.exists(build)) {
        const QDateTime weekAgo = QDateTime::currentDateTime().addDays(-7);
        for (const QFileInfo& old : glyphs.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (old.lastModified() < weekAgo) {
                QDir(old.absoluteFilePath()).removeRecursively();
            }
        }
        // Written before the cache had a folder per build
        for (const QString& unversioned : glyphs.entryList(QDir::Files)) {
            QFile::remove(glyphs.absoluteFilePath(unversioned));
        }
        if (!glyphs.mkpath(build)) {
            return QString();
        }
    }
    return glyphs.absoluteFilePath(qsl("%1/%2").arg(build, fileName));
}

// Written at 1x with @2x and higher twins beside it: a stylesheet loads its path through QPixmap, which rounds the
// screen's ratio up to pick a twin and takes it as drawn at that whole ratio. The @2x twin is written whatever the
// screen, so a cache first written at 1x serves a high-DPI screen too. Answers the 1x path, or nothing where the
// cache could not be written
static QString cachedGlyphFile(const QString& stem, const std::function<QPixmap(qreal)>& drawnAt)
{
    const QString filePath = glyphCacheFile(stem + qsl(".png"));
    if (filePath.isEmpty()) {
        return filePath;
    }

    const int most = std::max(2, qCeil(qApp ? qApp->devicePixelRatio() : 1.0));
    for (int at = 1; at <= most; ++at) {
        const QString wanted = at > 1 ? glyphCacheFile(qsl("%1@%2x.png").arg(stem, QString::number(at))) : filePath;
        if (QFileInfo::exists(wanted)) {
            continue;
        }
        const QPixmap glyph = drawnAt(at);
        if (glyph.isNull() || !glyph.save(wanted, "PNG")) {
            return QString();
        }
    }
    return filePath;
}

QString gripGlyphFile(const QColor& color, const bool alongTheBar)
{
    const int columns = alongTheBar ? scmGripDotsAlong : scmGripDotsAcross;
    const int rows = alongTheBar ? scmGripDotsAcross : scmGripDotsAlong;
    const qreal width = (columns - 1) * scmGripDotPitch + scmGripDotDiameter;
    const qreal height = (rows - 1) * scmGripDotPitch + scmGripDotDiameter;
    return cachedGlyphFile(qsl("grip-%1-%2x%3").arg(color.name().mid(1), QString::number(columns), QString::number(rows)), [&color, columns, rows, width, height](const qreal drawnAt) {
        QPixmap grip(qRound(width * drawnAt), qRound(height * drawnAt));
        grip.fill(Qt::transparent);
        QPainter painter(&grip);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.scale(drawnAt, drawnAt);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        for (int column = 0; column < columns; ++column) {
            for (int row = 0; row < rows; ++row) {
                painter.drawEllipse(QRectF(column * scmGripDotPitch, row * scmGripDotPitch, scmGripDotDiameter, scmGripDotDiameter));
            }
        }
        painter.end();
        return grip;
    });
}

// Lucide's 24-unit box leaves a margin that an 8px control arrow would spend half its height on
static QPixmap croppedToItsInk(const QPixmap& glyph)
{
    const QImage image = glyph.toImage().convertToFormat(QImage::Format_ARGB32);
    int left = image.width();
    int right = -1;
    int top = image.height();
    int bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) == 0) {
                continue;
            }
            left = std::min(left, x);
            right = std::max(right, x);
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        }
    }
    if (right < left || bottom < top) {
        return glyph;
    }
    return glyph.copy(QRect(QPoint(left, top), QPoint(right, bottom)));
}

static QString themedArrowFile(const QColor& color, const bool pointingUp, const int size)
{
    // Not "arrow": caches written by earlier builds hold a stretched shape under that name
    const QString stem = qsl("chevron-%1-%2-%3").arg(pointingUp ? qsl("up") : qsl("down"), color.name().mid(1), QString::number(size));
    return cachedGlyphFile(stem, [&color, pointingUp, size](const qreal drawnAt) {
        QPixmap source = croppedToItsInk(glyphPixmap(qsl(":/icons/editor-chevron-down.svg")));
        if (source.isNull()) {
            return source;
        }
        if (pointingUp) {
            source = source.transformed(QTransform().rotate(180));
        }
        source = tintedGlyph(source, color);
        const int drawnSize = qRound(size * drawnAt);
        // KeepAspectRatio: a chevron is twice as wide as tall, and stretched to a square it reads as a heavy V
        return source.scaled(drawnSize, drawnSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    });
}

QPixmap themedGlyphPixmap(const QString& glyphFile, const QColor& color, const int boxSize, const int glyphSize, const qreal devicePixelRatio)
{
    const QString key = qsl("uiDesign-themedGlyph-%1-%2-%3-%4-%5")
                                .arg(QFileInfo(glyphFile).baseName(), color.name(QColor::HexArgb), QString::number(boxSize), QString::number(glyphSize), QString::number(devicePixelRatio));
    QPixmap glyph;
    if (QPixmapCache::find(key, &glyph)) {
        return glyph;
    }

    QPixmap source = croppedToItsInk(glyphPixmap(glyphFile));
    if (source.isNull()) {
        return QPixmap();
    }
    source = tintedGlyph(source, color);

    // KeepAspectRatio: a dash stretched to a square reads as a rule across the box
    const int drawnSize = qRound(glyphSize * devicePixelRatio);
    const QPixmap mark = source.scaled(drawnSize, drawnSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph = QPixmap(qRound(boxSize * devicePixelRatio), qRound(boxSize * devicePixelRatio));
    glyph.fill(Qt::transparent);
    QPainter painter(&glyph);
    painter.drawPixmap(QPoint((glyph.width() - mark.width()) / 2, (glyph.height() - mark.height()) / 2), mark);
    painter.end();
    glyph.setDevicePixelRatio(devicePixelRatio);

    QPixmapCache::insert(key, glyph);
    return glyph;
}

// The mark carries its own margin: a sub-control scales its image down to its contents, never up, from whichever
// of the 1x and 2x files the platform picked, so only a baked-in margin holds the size.
static QString themedGlyphFile(const QString& glyphFile, const QColor& color, const int boxSize, const int glyphSize)
{
    const QString stem = qsl("%1-%2-%3-%4").arg(QFileInfo(glyphFile).baseName(), color.name().mid(1), QString::number(boxSize), QString::number(glyphSize));
    return cachedGlyphFile(stem, [&glyphFile, &color, boxSize, glyphSize](const qreal drawnAt) {
        return themedGlyphPixmap(glyphFile, color, boxSize, glyphSize, drawnAt);
    });
}

// Painted rather than a Lucide glyph: no Lucide circle survives being drawn at six pixels
static QString dotGlyphFile(const QColor& color, const int boxSize, const int diameter)
{
    const QString stem = qsl("dot-%1-%2-%3").arg(color.name().mid(1), QString::number(boxSize), QString::number(diameter));
    return cachedGlyphFile(stem, [&color, boxSize, diameter](const qreal drawnAt) {
        QPixmap dot(qRound(boxSize * drawnAt), qRound(boxSize * drawnAt));
        dot.fill(Qt::transparent);
        QPainter painter(&dot);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.scale(drawnAt, drawnAt);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        const qreal inset = (boxSize - diameter) / 2.0;
        painter.drawEllipse(QRectF(inset, inset, diameter, diameter));
        painter.end();
        return dot;
    });
}

static QString scopedTo(const QString& selectorPrefix, const QStringList& selectors)
{
    if (selectorPrefix.isEmpty()) {
        return selectors.join(qsl(", "));
    }
    QStringList scopedSelectors;
    scopedSelectors.reserve(selectors.size());
    for (const QString& selector : selectors) {
        scopedSelectors << selectorPrefix + QLatin1Char(' ') + selector;
    }
    return scopedSelectors.join(qsl(", "));
}

QColor disabledFieldColour(const ThemeTokens& tokens)
{
    return blend(tokens.field, tokens.page, scmDisabledFieldTowardsPage);
}

QColor disabledBorderColour(const ThemeTokens& tokens)
{
    return blend(tokens.page, tokens.text, scmDisabledBorderWeight);
}

QString inputStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix)
{
    const QColor disabledField = disabledFieldColour(tokens);
    const QColor disabledBorder = disabledBorderColour(tokens);

    const auto scoped = [&selectorPrefix](const QStringList& selectors) {
        return scopedTo(selectorPrefix, selectors);
    };

    // A styled frame loses its arrows, and macOS then draws a combo box as a line edit and a spin box without steppers,
    // so those two are claimed only once the arrow files exist. Each size gets its own file: a picture a stylesheet
    // scales is drawn at neither.
    const QString downArrow = themedArrowFile(tokens.mutedText, false, scmInputArrowSize);
    const QString upArrow = themedArrowFile(tokens.mutedText, true, scmInputArrowSize);
    const QString stepperDownArrow = themedArrowFile(tokens.mutedText, false, scmInputStepperArrowSize);
    const QString stepperUpArrow = themedArrowFile(tokens.mutedText, true, scmInputStepperArrowSize);
    const bool arrowsAvailable = !downArrow.isEmpty() && !upArrow.isEmpty();

    QStringList fieldTypes{qsl("QLineEdit"), qsl("QPlainTextEdit"), qsl("QTextEdit")};
    if (arrowsAvailable) {
        fieldTypes << qsl("QComboBox") << qsl("QAbstractSpinBox");
    }
    QStringList focusedTypes;
    QStringList disabledTypes;
    for (const QString& fieldType : std::as_const(fieldTypes)) {
        focusedTypes << fieldType + qsl(":focus");
        disabledTypes << fieldType + qsl(":disabled");
    }

    QString rules =
            scoped(fieldTypes)
            + qsl(" { background-color: %1; color: %2; border: %10px solid %3; border-radius: %4px;"
                  " padding: %5px %6px; min-height: %7px;"
                  " selection-background-color: %8; selection-color: %9; }")
                      .arg(tokens.field.name(), tokens.text.name(), tokens.border.name(), QString::number(scmRadiusInput))
                      .arg(QString::number(scmInputPaddingVertical), QString::number(scmInputPaddingHorizontal), QString::number(scmInputContentHeight))
                      .arg(tokens.accent.name(), tokens.accentText.name(), QString::number(scmInputBorderWidth))
            // A frame rather than an outer ring: where the platform draws a ring of its own, two read as a fault
            + scoped(focusedTypes) + qsl(" { border: %2px solid %1; }").arg(tokens.accent.name(), QString::number(scmInputBorderWidth)) + scoped(disabledTypes)
            + qsl(" { color: %1; background-color: %2; border: %4px solid %3; }").arg(tokens.disabledText.name(), disabledField.name(), disabledBorder.name(), QString::number(scmInputBorderWidth))
            // The popup's QFrame bevel is out of reach of the field rules, so it is cleared and the list carries the surface; the
            // window's square corner is opened by letPopupsTakeTheFieldsCorner(). The 2px padding keeps the viewport inside
            // the arc.
            + scoped({qsl("QComboBox QFrame")}) + qsl(" { border: none; margin: 0px; padding: 0px; background: transparent; }") + scoped({qsl("QComboBox QAbstractItemView")})
            + qsl(" { background-color: %1; color: %2; border: %6px solid %3; border-radius: %7px; padding: 2px;"
                  " selection-background-color: %4; selection-color: %5; outline: none; }")
                      .arg(tokens.field.name(), tokens.text.name(), tokens.border.name(), tokens.accent.name(), tokens.accentText.name(), QString::number(scmInputBorderWidth))
                      .arg(QString::number(scmRadiusInput))
            // Both of those hold a QLineEdit of their own, which the rules above
            // would otherwise draw as a second field inside the first
            + scoped({qsl("QComboBox QLineEdit"), qsl("QAbstractSpinBox QLineEdit")}) + qsl(" { background: transparent; border: none; border-radius: 0px; padding: 0px; min-height: 0px; }");

    if (arrowsAvailable) {
        rules += scoped({qsl("QComboBox::drop-down")})
                 + qsl(" { subcontrol-origin: padding; subcontrol-position: center right; width: %1px; border: none; background: transparent; }").arg(QString::number(scmInputDropDownWidth))
                 + scoped({qsl("QComboBox::down-arrow")}) + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(downArrow, QString::number(scmInputArrowSize))
                 + scoped({qsl("QAbstractSpinBox::up-button")})
                 + qsl(" { subcontrol-origin: border; subcontrol-position: top right; width: %1px; border: none; background: transparent; }").arg(QString::number(scmInputStepperWidth))
                 + scoped({qsl("QAbstractSpinBox::down-button")})
                 + qsl(" { subcontrol-origin: border; subcontrol-position: bottom right; width: %1px; border: none; background: transparent; }").arg(QString::number(scmInputStepperWidth))
                 + scoped({qsl("QAbstractSpinBox::up-arrow")}) + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(stepperUpArrow, QString::number(scmInputStepperArrowSize))
                 + scoped({qsl("QAbstractSpinBox::down-arrow")}) + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(stepperDownArrow, QString::number(scmInputStepperArrowSize));

        const QString accentArrow = themedArrowFile(tokens.accent, false, scmInputArrowSize);
        const QString accentStepperDownArrow = themedArrowFile(tokens.accent, false, scmInputStepperArrowSize);
        const QString accentStepperUpArrow = themedArrowFile(tokens.accent, true, scmInputStepperArrowSize);
        if (!accentArrow.isEmpty()) {
            rules += scoped({qsl("QComboBox::down-arrow:hover"), qsl("QComboBox::down-arrow:pressed")})
                     + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(accentArrow, QString::number(scmInputArrowSize));
        }
        if (!accentStepperUpArrow.isEmpty() && !accentStepperDownArrow.isEmpty()) {
            rules += scoped({qsl("QAbstractSpinBox::up-arrow:hover"), qsl("QAbstractSpinBox::up-arrow:pressed")})
                     + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(accentStepperUpArrow, QString::number(scmInputStepperArrowSize))
                     + scoped({qsl("QAbstractSpinBox::down-arrow:hover"), qsl("QAbstractSpinBox::down-arrow:pressed")})
                     + qsl(" { image: url(\"%1\"); width: %2px; height: %2px; }").arg(accentStepperDownArrow, QString::number(scmInputStepperArrowSize));
        }
        rules += scoped({qsl("QAbstractSpinBox::up-button:hover"), qsl("QAbstractSpinBox::down-button:hover")}) + qsl(" { background-color: %1; }").arg(tokens.hoverSoft)
                 + scoped({qsl("QAbstractSpinBox::up-button:pressed"), qsl("QAbstractSpinBox::down-button:pressed")}) + qsl(" { background-color: %1; }").arg(tokens.accentSoft);
    }
    return rules;
}

namespace {
struct ChoiceMarkInks
{
    QColor outline;
    QColor disabledBorder;
    QColor disabledFill;
    // Empty where the cache could not be written, which leaves out the rule that would point at it
    QString tick;
    QString dash;
    QString quietTick;
    QString quietDash;
    QString dot;
    QString quietDot;
};
} // namespace

static ChoiceMarkInks choiceMarkInks(const ThemeTokens& tokens)
{
    ChoiceMarkInks inks;
    inks.outline = blend(tokens.card, tokens.text, tokens.darkPage ? scmChoiceOutlineOnDark : scmChoiceOutlineOnLight);
    inks.disabledBorder = disabledBorderColour(tokens);
    inks.disabledFill = disabledFieldColour(tokens);
    // Against the field fill it is drawn on, not the accent hairline round it
    const QColor markInk = readableOn(tokens.field, tokens.accent, tokens.text, scmQuietMinimumRatio);
    inks.tick = themedGlyphFile(qsl(":/icons/control-check.svg"), markInk, scmChoiceIndicatorSize, scmChoiceGlyphSize);
    inks.dash = themedGlyphFile(qsl(":/icons/control-minus.svg"), markInk, scmChoiceIndicatorSize, scmChoiceGlyphSize);
    inks.quietTick = themedGlyphFile(qsl(":/icons/control-check.svg"), tokens.disabledText, scmChoiceIndicatorSize, scmChoiceGlyphSize);
    inks.quietDash = themedGlyphFile(qsl(":/icons/control-minus.svg"), tokens.disabledText, scmChoiceIndicatorSize, scmChoiceGlyphSize);
    inks.dot = dotGlyphFile(markInk, scmChoiceIndicatorSize, scmChoiceDotDiameter);
    inks.quietDot = dotGlyphFile(tokens.disabledText, scmChoiceIndicatorSize, scmChoiceDotDiameter);
    return inks;
}

// An empty picture leaves its rule out
static QString choiceMarkRules(
        const QString& selector, const int radius, const QString& set, const QString& part, const QString& quietSet, const QString& quietPart, const ChoiceMarkInks& inks, const ThemeTokens& tokens)
{
    QString rules = qsl("%1 { width: %2px; height: %2px; border: %3px solid %4; border-radius: %5px; background-color: %6; }"
                        "%1:hover, %1:focus { border: %3px solid %7; }"
                        "%1:pressed { background-color: %8; }")
                            .arg(selector, QString::number(scmChoiceIndicatorSize), QString::number(scmInputBorderWidth), inks.outline.name(), QString::number(radius), tokens.field.name())
                            .arg(tokens.accent.name(), tokens.accentSoft);
    if (!set.isEmpty()) {
        rules += qsl("%1:checked { border: %2px solid %3; image: url(\"%4\"); }").arg(selector, QString::number(scmInputBorderWidth), tokens.accent.name(), set);
    }
    if (!part.isEmpty()) {
        rules += qsl("%1:indeterminate { border: %2px solid %3; image: url(\"%4\"); }").arg(selector, QString::number(scmInputBorderWidth), tokens.accent.name(), part);
    }
    // Last, so it wins the specificity tie with :checked and :indeterminate
    rules += qsl("%1:disabled { border: %2px solid %3; background-color: %4; }").arg(selector, QString::number(scmInputBorderWidth), inks.disabledBorder.name(), inks.disabledFill.name());
    if (!quietSet.isEmpty()) {
        rules += qsl("%1:disabled:checked { image: url(\"%2\"); }").arg(selector, quietSet);
    }
    if (!quietPart.isEmpty()) {
        rules += qsl("%1:disabled:indeterminate { image: url(\"%2\"); }").arg(selector, quietPart);
    }
    return rules;
}

QString choiceStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix)
{
    const ChoiceMarkInks inks = choiceMarkInks(tokens);
    // Half its size, which Qt clamps to a circle
    const int dotRadius = (scmChoiceIndicatorSize + 1) / 2;
    return choiceMarkRules(scopedTo(selectorPrefix, {qsl("QCheckBox::indicator")}), scmChoiceBoxRadius, inks.tick, inks.dash, inks.quietTick, inks.quietDash, inks, tokens)
           + choiceMarkRules(scopedTo(selectorPrefix, {qsl("QRadioButton::indicator")}), dotRadius, inks.dot, QString(), inks.quietDot, QString(), inks, tokens)
           // A style measures the gap after an indicator off the one it would draw, so it is named here. border: none, or the
           // macOS style trims 11px off a check box's layout rect for its bezel and the next control covers this one's words.
           + scopedTo(selectorPrefix, {qsl("QCheckBox"), qsl("QRadioButton")})
           + qsl(" { spacing: %1px; border: none; }").arg(QString::number(scmChoiceLabelSpacing))
           // Or the platform's focus ring doubles the accent the indicator takes
           + scopedTo(selectorPrefix, {qsl("QCheckBox:focus"), qsl("QRadioButton:focus")}) + qsl(" { outline: none; }");
}

QString itemCheckMarkStyleSheet(const QString& viewSelector, const ThemeTokens& tokens)
{
    const ChoiceMarkInks inks = choiceMarkInks(tokens);
    return choiceMarkRules(viewSelector + qsl("::indicator"), scmChoiceBoxRadius, inks.tick, inks.dash, inks.quietTick, inks.quietDash, inks, tokens);
}

QString treeBranchStyleSheet(const QString& viewSelector, const ThemeTokens& tokens, const QColor& surface)
{
    const QString closed = themedGlyphFile(qsl(":/icons/about-chevron-right.svg"), tokens.mutedText, scmTreeBranchBoxSize, scmTreeBranchGlyphSize);
    const QString open = themedGlyphFile(qsl(":/icons/editor-chevron-down.svg"), tokens.mutedText, scmTreeBranchBoxSize, scmTreeBranchGlyphSize);
    if (closed.isEmpty() || open.isEmpty()) {
        return QString();
    }

    const QString branch = viewSelector + qsl("::branch");
    // Painted with the surface under :selected and :hover: a tree fills a chosen row's indentation with the palette
    // highlight before it draws a branch
    return branch + qsl(" { background: transparent; border-image: none; image: none; }") + branch + qsl(":selected, ") + branch + qsl(":hover { background-color: %1; }").arg(surface.name()) + branch
           + qsl(":has-children:closed { image: url(\"%1\"); }").arg(closed) + branch + qsl(":has-children:open { image: url(\"%1\"); }").arg(open);
}

static QColor buttonFaceColour(const ThemeTokens& tokens)
{
    return blend(tokens.card, tokens.text, tokens.darkPage ? scmButtonFaceLiftOnDark : scmButtonFaceLiftOnLight);
}

QString buttonStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix)
{
    const QColor face = buttonFaceColour(tokens);
    const auto scoped = [&selectorPrefix](const QStringList& selectors) {
        return scopedTo(selectorPrefix, selectors);
    };

    // A split button must stay a QToolButton, so the opted-in ones join the push button rules
    const QString menuButton = qsl("QToolButton[%1=\"true\"]").arg(QString::fromLatin1(scmProp_menuButton));

    QString rules =
            scoped({qsl("QPushButton"), menuButton})
            + qsl(" { background-color: %1; color: %2; border: %3px solid %4; border-radius: %5px; padding: %6px %7px; min-height: %8px; }")
                      .arg(face.name(), tokens.text.name(), QString::number(scmInputBorderWidth), tokens.border.name(), QString::number(scmRadiusInput))
                      .arg(QString::number(scmInputPaddingVertical), QString::number(scmButtonPaddingHorizontal), QString::number(scmInputContentHeight))
            + scoped({qsl("QPushButton:hover"), menuButton + qsl(":hover")})
            + qsl(" { border: %2px solid %1; background-color: %3; }").arg(tokens.accent.name(), QString::number(scmInputBorderWidth), tokens.hoverSoft)
            + scoped({qsl("QPushButton:pressed"), menuButton + qsl(":pressed")}) + qsl(" { background-color: %1; }").arg(tokens.accentSoft)
            + scoped({qsl("QPushButton:focus"), menuButton + qsl(":focus")}) + qsl(" { border: %2px solid %1; }").arg(tokens.accent.name(), QString::number(scmInputBorderWidth))
            + scoped({qsl("QPushButton:disabled"), menuButton + qsl(":disabled")})
            + qsl(" { color: %1; border: %3px solid %2; background-color: transparent; }").arg(tokens.disabledText.name(), disabledBorderColour(tokens).name(), QString::number(scmInputBorderWidth));

    // Styling a button removes the platform's menu indicator, so this is claimed only once the chevron file exists
    const QString menuChevron = themedArrowFile(tokens.mutedText, false, scmInputArrowSize);
    if (!menuChevron.isEmpty()) {
        rules += scoped({qsl("QPushButton::menu-indicator")})
                 + qsl(" { image: url(\"%1\"); subcontrol-origin: padding; subcontrol-position: center right; right: %2px; width: %3px; height: %3px; }")
                           .arg(menuChevron, QString::number(scmButtonMenuIndicatorInset), QString::number(scmInputArrowSize));
        // The label is laid out across the whole contents rect, so the padding leaves the menu half its width
        rules += scoped({menuButton}) + qsl(" { padding-right: %1px; }").arg(QString::number(scmButtonPaddingHorizontal + scmInputDropDownWidth))
                 + splitButtonMenuHalfStyleSheet(scoped({menuButton}), scmRadiusInput, tokens);

        const QString accentChevron = themedArrowFile(tokens.accent, false, scmInputArrowSize);
        if (!accentChevron.isEmpty()) {
            rules += scoped({qsl("QPushButton::menu-indicator:hover"), qsl("QPushButton::menu-indicator:pressed")})
                     + qsl(" { image: url(\"%1\"); subcontrol-origin: padding; subcontrol-position: center right; right: %2px; width: %3px; height: %3px; }")
                               .arg(accentChevron, QString::number(scmButtonMenuIndicatorInset), QString::number(scmInputArrowSize));
        }
    }

    // Last, to win the specificity tie with the resting rules; its own :hover and :disabled follow, or the shared ones
    // would lose to its resting rule on order alone
    const QString primary = scoped({qsl("QPushButton[%1=\"true\"]").arg(QString::fromLatin1(scmProp_primaryButton))});
    rules += primary + qsl(" { background-color: %1; color: %2; border: %3px solid %4; }").arg(tokens.accentSoft, tokens.accentText.name(), QString::number(scmInputBorderWidth), tokens.accent.name())
             + scoped({qsl("QPushButton[%1=\"true\"]:hover").arg(QString::fromLatin1(scmProp_primaryButton))}) + qsl(" { background-color: %1; }").arg(tokens.hoverSoft)
             + scoped({qsl("QPushButton[%1=\"true\"]:disabled").arg(QString::fromLatin1(scmProp_primaryButton))})
             + qsl(" { color: %1; border: %3px solid %2; background-color: %4; }")
                       .arg(tokens.disabledText.name(), disabledBorderColour(tokens).name(), QString::number(scmInputBorderWidth), disabledFieldColour(tokens).name());
    return rules;
}

// QStyleSheetStyle does not answer hitTestComplexControl for CC_ToolButton, so :hover matches anywhere on the
// button; the half's wash is a step ahead of the body's to still read as its own target. Presses are routed through
// subControlRect(), which the sheet does answer.
QString splitButtonMenuHalfStyleSheet(const QString& buttonSelector, const int cornerRadius, const ThemeTokens& tokens)
{
    const QString chevron = themedArrowFile(tokens.mutedText, false, scmInputArrowSize);
    if (chevron.isEmpty()) {
        return QString();
    }

    QString rules = buttonSelector
                    + qsl("::menu-button { subcontrol-origin: padding; subcontrol-position: center right; width: %1px;"
                          " border: none; border-left: %2px solid %3; background: transparent;"
                          " border-top-right-radius: %4px; border-bottom-right-radius: %4px; }")
                              .arg(QString::number(scmInputDropDownWidth), QString::number(scmInputBorderWidth), tokens.border.name(), QString::number(cornerRadius))
                    + buttonSelector + qsl("::menu-button:hover { background-color: %1; }").arg(tokens.accentSoft) + buttonSelector
                    + qsl("::menu-button:pressed { background-color: %1; }").arg(rgba(tokens.accent, scmSplitHalfPressedWash)) + buttonSelector
                    + qsl("::menu-arrow { image: url(\"%1\"); width: %2px; height: %2px; }").arg(chevron, QString::number(scmInputArrowSize));

    const QString accentChevron = themedArrowFile(tokens.accent, false, scmInputArrowSize);
    if (!accentChevron.isEmpty()) {
        rules += buttonSelector + qsl("::menu-arrow:hover, ") + buttonSelector
                 + qsl("::menu-arrow:pressed { image: url(\"%1\"); width: %2px; height: %2px; }").arg(accentChevron, QString::number(scmInputArrowSize));
    }
    return rules;
}

QString toolBarStyleSheet(const QString& toolBarSelector, const ToolBarSeam seam, const ThemeTokens& tokens)
{
    // Styling the bar reduces the platform's handle to a pair of faint dots
    const QString gripAcross = gripGlyphFile(tokens.mutedText, false);
    const QString gripAlong = gripGlyphFile(tokens.mutedText, true);
    const QString seamEdge = seam == ToolBarSeam::Bottom ? qsl("bottom") : qsl("top");
    const qreal buttonPointSize = QApplication::font("QToolButton").pointSizeF();

    return toolBarSelector + qsl(" { background-color: %1; border: none; border-%2: 1px solid %3; spacing: 2px; padding: 4px 6px; }").arg(tokens.page.name(), seamEdge, tokens.border.name())
           + toolBarSelector
           + qsl("::separator { background-color: %1; width: 1px; margin: 5px 6px; }").arg(tokens.border.name())
           // background-image, not image: a sub-control's image is stretched to fill it
           + toolBarSelector
           + qsl("::handle { background-image: url(%1); background-repeat: no-repeat; background-position: center; width: %2px; margin: 6px 2px; }")
                     .arg(gripAcross, QString::number(scmToolBarGripExtent))
           + toolBarSelector
           + qsl("::handle:vertical { background-image: url(%1); height: %2px; margin: 2px 6px; }").arg(gripAlong, QString::number(scmToolBarGripExtent))
           // The transparent border keeps the label from stepping sideways when
           // a hovered button gains one
           + toolBarSelector
           + qsl(" QToolButton { color: %1; border: 1px solid transparent; border-radius: %2px; padding: %3px %4px; }")
                     .arg(tokens.mutedText.name(), QString::number(scmToolBarButtonRadius), QString::number(scmToolBarButtonPaddingVertical), QString::number(scmToolBarButtonPaddingHorizontal))
           // accentText, to light with the glyph's QIcon::Active ink
           + toolBarSelector + qsl(" QToolButton:hover { color: %1; background-color: %2; }").arg(tokens.accentText.name(), tokens.hoverSoft) + toolBarSelector
           + qsl(" QToolButton:pressed { background-color: %1; }").arg(tokens.accentSoft)
           // accentText, to light with the glyph's QIcon::On ink
           + toolBarSelector + qsl(" QToolButton:checked { color: %1; background-color: %2; }").arg(tokens.accentText.name(), tokens.accentSoft) + toolBarSelector
           + qsl(" QToolButton:disabled { color: %1; }").arg(tokens.disabledBarText.name())
           // Qt hides at most one more button to make room for the overflow button and tightens every gap in the row if that
           // leaves too little, so this one is kept about as narrow as the bar's own
           + toolBarSelector
           + qsl(" QToolBarExtension { padding-left: 0px; padding-right: 0px; }")
           // macOS gives the bar's own buttons a smaller font than the overflow button, and setFont() does not hold under a
           // sheet. By class, not object name: this specificity lets a profile's "QToolButton" font rule, written later, win
           + (buttonPointSize > 0 ? toolBarSelector + qsl(" QToolBarExtension { font-size: %1pt; }").arg(buttonPointSize) : QString());
}

QString disclosureButtonStyleSheet(const QString& buttonSelector, const ThemeTokens& tokens)
{
    return buttonSelector
           + qsl(" { color: %1; border: %2px solid %3; border-radius: %4px; padding: %5px %6px; background-color: %7; }")
                     .arg(tokens.mutedText.name(), QString::number(scmInputBorderWidth), tokens.border.name(), QString::number(scmRadiusInput), QString::number(scmDisclosurePaddingVertical))
                     .arg(QString::number(scmButtonPaddingHorizontal), buttonFaceColour(tokens).name())
           + buttonSelector + qsl(":hover { color: %1; background-color: %2; }").arg(tokens.accentText.name(), tokens.hoverSoft) + buttonSelector
           + qsl(":checked { color: %1; border: %2px solid %3; background-color: %4; }").arg(tokens.accentText.name(), QString::number(scmInputBorderWidth), tokens.accent.name(), tokens.accentSoft)
           + buttonSelector + qsl(":focus { border-color: %1; }").arg(tokens.accent.name());
}

// No font here: a menu is read in the font of whatever it hangs from
QString menuStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix)
{
    const auto scoped = [&selectorPrefix](const QStringList& selectors) {
        return scopedTo(selectorPrefix, selectors);
    };

    QString rules =
            scoped({qsl("QMenu")})
            + qsl(" { background-color: %1; color: %2; border: %3px solid %4; border-radius: %5px; padding: %6px; }")
                      .arg(tokens.card.name(), tokens.text.name(), QString::number(scmInputBorderWidth), tokens.border.name(), QString::number(scmRadiusInput))
                      .arg(QString::number(scmMenuSurfacePadding))
            + scoped({qsl("QMenu::item")})
            + qsl(" { background: transparent; border-radius: %1px; padding: %2px %3px %2px %4px; }")
                      .arg(QString::number(scmRadiusChip), QString::number(scmMenuItemPaddingVertical), QString::number(scmMenuItemPaddingTrailing), QString::number(scmMenuItemPaddingLeading))
            + scoped({qsl("QMenu::item:selected")}) + qsl(" { background-color: %1; color: %2; }").arg(tokens.accentSoft, tokens.accentText.name()) + scoped({qsl("QMenu::item:disabled")})
            + qsl(" { color: %1; }").arg(tokens.disabledText.name())
            // Inset like the rows rather than full width: it parts the rows either side of it, not the frame
            + scoped({qsl("QMenu::separator")})
            + qsl(" { height: 1px; background-color: %1; margin: %2px %3px; }").arg(tokens.separator.name(), QString::number(scmMenuSurfacePadding), QString::number(scmMenuItemPaddingLeading));

    // Exclusive rows would come out as boxes rather than circles; none of these menus has one
    const ChoiceMarkInks inks = choiceMarkInks(tokens);
    rules += choiceMarkRules(scoped({qsl("QMenu::indicator")}), scmChoiceBoxRadius, inks.tick, QString(), inks.quietTick, QString(), inks, tokens)
             + scoped({qsl("QMenu::indicator"), qsl("QMenu::icon")}) + qsl(" { left: %1px; }").arg(QString::number(scmMenuItemMarkInset));
    return rules;
}

QString tabBarStyleSheet(const QString& tabWidgetSelector, const ThemeTokens& tokens)
{
    // The bar is both a sub-control of the tab widget (::tab-bar) and a QTabBar descendant, so both selectors are used
    const QString bar = tabWidgetSelector + qsl(" QTabBar");

    QString rules = tabWidgetSelector
                    + qsl("::pane { border: none; background: transparent; margin: %1px; }").arg(QString::number(scmTabPaneInset))
                    // On the pane's inset, so the chips and the field below start on the same line
                    + tabWidgetSelector + qsl("::tab-bar { left: %1px; }").arg(QString::number(scmTabStripInset)) + bar
                    + qsl(" { background: transparent; border: none; }")
                    // The transparent border keeps the word still when :focus colours it
                    + bar
                    + qsl("::tab { background: transparent; color: %1; border: %2px solid transparent; border-radius: %3px;"
                          " padding: %4px %5px; margin-right: %6px; }")
                              .arg(tokens.mutedText.name(), QString::number(scmInputBorderWidth), QString::number(scmRadiusChip), QString::number(scmTabPaddingVertical))
                              .arg(QString::number(scmTabPaddingHorizontal), QString::number(scmTabGap))
                    + bar + qsl("::tab:hover { background-color: %1; color: %2; }").arg(tokens.hoverSoft, tokens.accentText.name()) + bar
                    + qsl("::tab:selected { background-color: %1; color: %2; }").arg(tokens.accentSoft, tokens.accentText.name())
                    // Focus as the accent border, the way every other control in the design shows it
                    + bar + qsl("::tab:focus { border-color: %1; }").arg(tokens.accent.name()) + bar + qsl("::tab:disabled { color: %1; }").arg(tokens.disabledText.name());

    // Left out if the cache cannot be written, so the tab keeps the platform's own cross
    const QString quietCross = themedGlyphFile(qsl(":/icons/editor-clear.svg"), tokens.mutedText, scmTabCloseBoxSize, scmTabCloseGlyphSize);
    const QString litCross = themedGlyphFile(qsl(":/icons/editor-clear.svg"), tokens.accentText, scmTabCloseBoxSize, scmTabCloseGlyphSize);
    if (!quietCross.isEmpty()) {
        rules += bar + qsl("::close-button { image: url(\"%1\"); subcontrol-position: right; width: %2px; height: %2px; }").arg(quietCross, QString::number(scmTabCloseBoxSize));
        if (!litCross.isEmpty()) {
            rules += bar + qsl("::close-button:hover { image: url(\"%1\"); width: %2px; height: %2px; }").arg(litCross, QString::number(scmTabCloseBoxSize));
        }
    }
    return rules;
}

namespace {
// Qt's close button sizes itself in its constructor from PM_TabCloseIndicatorWidth, and QStyleSheetStyle never
// reads the ::close-button size for it (20px under Fusion, 14 under macOS). ChildPolished arrives before that
// resize(), so a fixed size set then holds.
class TabCloseBoxKeeper : public QObject
{
public:
    // Named rather than found by type: without Q_OBJECT - which it has no
    // signals to want - a qobject_cast for this class matches every QObject
    static constexpr char scmName[] = "uiDesignTabCloseBoxKeeper";

    explicit TabCloseBoxKeeper(QObject* pParent)
    : QObject(pParent)
    {
        setObjectName(QLatin1StringView(scmName));
    }

    void keep(QAbstractButton* pButton)
    {
        pButton->setFixedSize(scmTabCloseBoxSize, scmTabCloseBoxSize);
        pButton->installEventFilter(this);
    }

    // With a ::tab box, SE_TabBarTabRightButton places the button on the tab's raw rect, outside the chip.
    // layoutTab() applies it with move(), so each QEvent::Move puts it back; moving sends another, hence the compare.
    void placeInsideThePadding(QWidget* pButton)
    {
        auto* pTabBar = qobject_cast<QTabBar*>(parent());
        if (!pTabBar || !pButton || pButton->parentWidget() != pTabBar) {
            return;
        }
        for (int i = 0; i < pTabBar->count(); ++i) {
            for (const QTabBar::ButtonPosition side : {QTabBar::LeftSide, QTabBar::RightSide}) {
                if (pTabBar->tabButton(i, side) != pButton) {
                    continue;
                }
                // The chip is the tab rect less scmTabGap on its trailing side; Qt has already centred the button vertically
                const QRect tab = pTabBar->tabRect(i);
                const int padding = scmInputBorderWidth + scmTabPaddingHorizontal;
                QRect wanted = pButton->geometry();
                wanted.moveLeft(side == QTabBar::RightSide ? tab.right() - scmTabGap - padding - pButton->width() + 1 : tab.left() + padding);
                wanted = QStyle::visualRect(pTabBar->layoutDirection(), tab, wanted);
                if (pButton->pos() != wanted.topLeft()) {
                    pButton->move(wanted.topLeft());
                }
                return;
            }
        }
    }

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override
    {
        if (pEvent->type() == QEvent::ChildPolished) {
            QObject* pChild = static_cast<QChildEvent*>(pEvent)->child();
            auto* pButton = qobject_cast<QAbstractButton*>(pChild);
            if (pButton && !qobject_cast<QToolButton*>(pChild)) {
                keep(pButton);
            }
        } else if (pEvent->type() == QEvent::Move) {
            placeInsideThePadding(qobject_cast<QWidget*>(pWatched));
        }
        return QObject::eventFilter(pWatched, pEvent);
    }
};
} // namespace

void prepareTabStrip(QTabBar* pTabBar)
{
    if (!pTabBar) {
        return;
    }

    // The macOS style fills a document-mode bar with a band of its own that background: transparent cannot remove; it
    // is the bar's base
    pTabBar->setDrawBase(false);

    QObject* pFound = pTabBar->findChild<QObject*>(QLatin1StringView(TabCloseBoxKeeper::scmName), Qt::FindDirectChildrenOnly);
    auto* pKeeper = pFound ? static_cast<TabCloseBoxKeeper*>(pFound) : new TabCloseBoxKeeper(pTabBar);
    if (!pFound) {
        pTabBar->installEventFilter(pKeeper);
    }

    // Crosses made before the filter was installed are caught here; the StyleChange makes the bar re-lay its tabs from
    // their new size()
    bool anyCut = false;
    for (int i = 0; i < pTabBar->count(); ++i) {
        for (const QTabBar::ButtonPosition side : {QTabBar::LeftSide, QTabBar::RightSide}) {
            auto* pCross = qobject_cast<QAbstractButton*>(pTabBar->tabButton(i, side));
            if (!pCross || qobject_cast<QToolButton*>(pCross)) {
                continue;
            }
            anyCut = anyCut || pCross->size() != QSize(scmTabCloseBoxSize, scmTabCloseBoxSize);
            pKeeper->keep(pCross);
            pKeeper->placeInsideThePadding(pCross);
        }
    }
    if (anyCut) {
        QEvent styleChanged(QEvent::StyleChange);
        QApplication::sendEvent(pTabBar, &styleChanged);
    }
}

void keepClickFocusOffControls(QWidget* pRoot)
{
    if (!pRoot) {
        return;
    }
    const auto leaveItToTheKeyboard = [](QWidget* pControl) {
        // A control asking for no focus at all is left saying so: the point
        // here is which way focus arrives, not whether it can
        if (pControl->focusPolicy() == Qt::NoFocus) {
            return;
        }
        const auto* pCard = qobject_cast<QGroupBox*>(pControl);
        if (!qobject_cast<QCheckBox*>(pControl) && !qobject_cast<QRadioButton*>(pControl) && !qobject_cast<QPushButton*>(pControl) && !(pCard && pCard->isCheckable())) {
            return;
        }
        pControl->setFocusPolicy(Qt::TabFocus);
    };
    leaveItToTheKeyboard(pRoot);
    for (QWidget* pControl : pRoot->findChildren<QWidget*>()) {
        leaveItToTheKeyboard(pControl);
    }
}

namespace {
// A popup window is filled before its rounded frame is drawn, so its square corners show unless it paints nothing.
// A stylesheet cannot clear a QFrame subclass's background, and Qt::WA_WindowPropagation hands the popup the
// field's brushes, so Window and Base are cleared; the words' ink still follows the appearance.
void namePopupSurfaceAsNothing(QWidget* pPopup)
{
    QPalette unpainted = pPopup->palette();
    unpainted.setBrush(QPalette::Window, Qt::transparent);
    unpainted.setBrush(QPalette::Base, Qt::transparent);
    pPopup->setPalette(unpainted);
}

// An appearance change re-resolves every palette after the shell has restyled, so the brushes are cleared again on
// each Show
class PopupSurfaceKeeper : public QObject
{
public:
    // Named rather than found by type: without Q_OBJECT - which it has no
    // signals to want - a qobject_cast for this class matches every QObject
    static constexpr char scmName[] = "uiDesignPopupSurfaceKeeper";

    explicit PopupSurfaceKeeper(QObject* pParent)
    : QObject(pParent)
    {
        setObjectName(QLatin1StringView(scmName));
    }

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override
    {
        if (pEvent->type() == QEvent::Show) {
            if (auto* pPopup = qobject_cast<QWidget*>(pWatched)) {
                namePopupSurfaceAsNothing(pPopup);
            }
        }
        return QObject::eventFilter(pWatched, pEvent);
    }
};
} // namespace

void letPopupsTakeTheFieldsCorner(QWidget* pRoot)
{
    if (!pRoot) {
        return;
    }
    const auto openTheCorner = [](QComboBox* pComboBox) {
        // view() makes the container on the first call, which is what lets this
        // run before the box has ever been dropped down
        QAbstractItemView* pList = pComboBox->view();
        if (!pList) {
            return;
        }
        // The container window, not the list: the platform makes its surface for the window
        QWidget* pPopup = pList->window();
        if (!pPopup || pPopup == pComboBox->window()) {
            return;
        }
        pPopup->setAttribute(Qt::WA_TranslucentBackground);
        namePopupSurfaceAsNothing(pPopup);
        if (!pPopup->findChild<QObject*>(QLatin1StringView(PopupSurfaceKeeper::scmName), Qt::FindDirectChildrenOnly)) {
            pPopup->installEventFilter(new PopupSurfaceKeeper(pPopup));
        }
    };
    const auto openTheMenusCorner = [](QMenu* pMenu) {
        pMenu->setAttribute(Qt::WA_TranslucentBackground);
        namePopupSurfaceAsNothing(pMenu);
        if (!pMenu->findChild<QObject*>(QLatin1StringView(PopupSurfaceKeeper::scmName), Qt::FindDirectChildrenOnly)) {
            pMenu->installEventFilter(new PopupSurfaceKeeper(pMenu));
        }
    };
    if (auto* pComboBox = qobject_cast<QComboBox*>(pRoot)) {
        openTheCorner(pComboBox);
    }
    for (QComboBox* pComboBox : pRoot->findChildren<QComboBox*>()) {
        openTheCorner(pComboBox);
    }
    if (auto* pMenu = qobject_cast<QMenu*>(pRoot)) {
        openTheMenusCorner(pMenu);
    }
    for (QMenu* pMenu : pRoot->findChildren<QMenu*>()) {
        openTheMenusCorner(pMenu);
    }
}

QPixmap glyphPixmap(const QString& file)
{
    const QString key = qsl("uiDesign-glyph-%1-%2").arg(file, QString::number(scmGlyphStrokeWidth));
    QPixmap glyph;
    if (QPixmapCache::find(key, &glyph)) {
        return glyph;
    }

    if (!file.endsWith(QLatin1StringView(".svg"), Qt::CaseInsensitive)) {
        glyph = QPixmap(file);
        QPixmapCache::insert(key, glyph);
        return glyph;
    }

    QFile source(file);
    if (!source.open(QIODevice::ReadOnly)) {
        return glyph;
    }
    QByteArray svg = source.readAll();
    // Lucide writes the weight into every file as stroke-width="2"; swapping it
    // here is what makes the weight a token instead of a property of the asset
    svg.replace(QByteArrayLiteral(R"(stroke-width="2")"), QByteArrayLiteral(R"(stroke-width=")") + QByteArray::number(scmGlyphStrokeWidth) + '"');

    QSvgRenderer renderer(svg);
    if (!renderer.isValid()) {
        return glyph;
    }
    glyph = QPixmap(scmGlyphRasterSize, scmGlyphRasterSize);
    glyph.fill(Qt::transparent);
    QPainter painter(&glyph);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    QPixmapCache::insert(key, glyph);
    return glyph;
}

QPixmap glyphFillPixmap(const QString& file)
{
    if (!file.endsWith(QLatin1StringView(".svg"), Qt::CaseInsensitive)) {
        return QPixmap();
    }
    const QString key = qsl("uiDesign-glyph-fill-%1").arg(file);
    QPixmap fill;
    if (QPixmapCache::find(key, &fill)) {
        return fill;
    }

    QString shapeFile = file;
    shapeFile.insert(shapeFile.size() - 4, qsl("-fill"));
    QFile source(QFile::exists(shapeFile) ? shapeFile : file);
    if (!source.open(QIODevice::ReadOnly)) {
        return fill;
    }
    QByteArray svg = source.readAll();
    // Lucide sets both on the root element and nowhere else, so these two swaps turn every line into its shape
    svg.replace(QByteArrayLiteral(R"(fill="none")"), QByteArrayLiteral(R"(fill="black")"));
    svg.replace(QByteArrayLiteral(R"(stroke="currentColor")"), QByteArrayLiteral(R"(stroke="none")"));

    QSvgRenderer renderer(svg);
    if (!renderer.isValid()) {
        return fill;
    }
    fill = QPixmap(scmGlyphRasterSize, scmGlyphRasterSize);
    fill.fill(Qt::transparent);
    QPainter painter(&fill);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    QPixmapCache::insert(key, fill);
    return fill;
}

QColor identityInk(const IdentityColour& identity, const ThemeTokens& tokens)
{
    if (identity.hue < 0.0) {
        return tokens.mutedText;
    }
    const QColor wanted = QColor::fromHslF(identity.hue, identity.saturation, identity.lightness);
    // The card second: a split button's menu is drawn on it, and walking only ever moves away from both
    return readableOn(tokens.card, readableOn(tokens.page, wanted, tokens.mutedText, scmQuietMinimumRatio), tokens.mutedText, scmQuietMinimumRatio);
}

QColor identityFill(const IdentityColour& identity, const ThemeTokens& tokens)
{
    QColor fill = QColor::fromHslF(identity.hue, identity.saturation, identity.lightness);
    fill.setAlphaF(tokens.darkPage ? scmIdentityFillOnDark : scmIdentityFillOnLight);
    return fill;
}

QPixmap tintedGlyph(const QPixmap& source, const QColor& color)
{
    QPixmap glyph = source;
    QPainter painter(&glyph);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(glyph.rect(), color);
    painter.end();
    return glyph;
}

namespace {
QIcon tintedIconDisabledIn(const QString& glyphOff, const QString& glyphOn, const ThemeTokens& tokens, const QColor& disabledInk)
{
    const QPixmap sourceOff = glyphPixmap(glyphOff);
    const QPixmap sourceOn = glyphOn == glyphOff ? sourceOff : glyphPixmap(glyphOn);

    QIcon icon;
    icon.addPixmap(tintedGlyph(sourceOff, tokens.mutedText), QIcon::Normal, QIcon::Off);
    const QPixmap accentOff = tintedGlyph(sourceOff, tokens.accentText);
    icon.addPixmap(accentOff, QIcon::Active, QIcon::Off);
    icon.addPixmap(accentOff, QIcon::Selected, QIcon::Off);
    const QPixmap accentOn = glyphOn == glyphOff ? accentOff : tintedGlyph(sourceOn, tokens.accentText);
    icon.addPixmap(accentOn, QIcon::Normal, QIcon::On);
    icon.addPixmap(accentOn, QIcon::Active, QIcon::On);
    icon.addPixmap(accentOn, QIcon::Selected, QIcon::On);
    const QPixmap disabledOff = tintedGlyph(sourceOff, disabledInk);
    icon.addPixmap(disabledOff, QIcon::Disabled, QIcon::Off);
    icon.addPixmap(glyphOn == glyphOff ? disabledOff : tintedGlyph(sourceOn, disabledInk), QIcon::Disabled, QIcon::On);
    return icon;
}

void restyleGlyphsDisabledIn(const QList<ActionGlyph>& glyphs, const ThemeTokens& tokens, const QColor& disabledInk)
{
    for (const ActionGlyph& glyph : glyphs) {
        if (glyph.pAction) {
            glyph.pAction->setIcon(tintedIconDisabledIn(glyph.glyphOff, glyph.glyphOn.isEmpty() ? glyph.glyphOff : glyph.glyphOn, tokens, disabledInk));
        }
    }
}

} // namespace

void addPaintDots(QPixmap& glyph, const QString& file, const ThemeTokens& tokens)
{
    QString dotsFile = file;
    dotsFile.insert(dotsFile.size() - 4, qsl("-dots"));
    QFile source(dotsFile);
    if (!source.exists() || !source.open(QIODevice::ReadOnly)) {
        return;
    }
    QSvgRenderer renderer(source.readAll());
    if (!renderer.isValid()) {
        return;
    }
    const qreal scale = static_cast<qreal>(glyph.width()) / renderer.viewBoxF().width();
    QPainter painter(&glyph);
    for (int dot = 0; dot < static_cast<int>(std::size(scmIdentityPaintDots)); ++dot) {
        const QString id = qsl("dot-%1").arg(dot);
        if (!renderer.elementExists(id)) {
            continue;
        }
        QPixmap shape(glyph.size());
        shape.fill(Qt::transparent);
        {
            QPainter shapePainter(&shape);
            shapePainter.setRenderHint(QPainter::Antialiasing);
            const QRectF bounds = renderer.boundsOnElement(id);
            renderer.render(&shapePainter, id, QRectF(bounds.topLeft() * scale, bounds.size() * scale));
        }
        painter.drawPixmap(0, 0, tintedGlyph(shape, identityInk(scmIdentityPaintDots[dot], tokens)));
    }
}

QPixmap identityGlyph(const QString& file, const IdentityColour& identity, const ThemeTokens& tokens)
{
    QPixmap glyph = tintedGlyph(glyphPixmap(file), identityInk(identity, tokens));
    const QPixmap fill = glyphFillPixmap(file);
    if (!fill.isNull()) {
        QPixmap washed = tintedGlyph(fill, identityFill(identity, tokens));
        QPainter painter(&washed);
        painter.drawPixmap(0, 0, glyph);
        painter.end();
        glyph = washed;
    }
    addPaintDots(glyph, file, tokens);
    return glyph;
}

QIcon identityIcon(const QString& glyphOff, const QString& glyphOn, const IdentityColour& identity, const ThemeTokens& tokens, const QColor& disabledInk)
{
    const QPixmap colouredOff = identityGlyph(glyphOff, identity, tokens);
    const QPixmap colouredOn = glyphOn == glyphOff ? colouredOff : identityGlyph(glyphOn, identity, tokens);
    QIcon icon;
    for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Active, QIcon::Selected}) {
        icon.addPixmap(colouredOff, mode, QIcon::Off);
        icon.addPixmap(colouredOn, mode, QIcon::On);
    }
    const QPixmap disabledOff = tintedGlyph(glyphPixmap(glyphOff), disabledInk);
    icon.addPixmap(disabledOff, QIcon::Disabled, QIcon::Off);
    icon.addPixmap(glyphOn == glyphOff ? disabledOff : tintedGlyph(glyphPixmap(glyphOn), disabledInk), QIcon::Disabled, QIcon::On);
    return icon;
}

QIcon tintedIcon(const QString& glyphOff, const QString& glyphOn, const ThemeTokens& tokens)
{
    return tintedIconDisabledIn(glyphOff, glyphOn, tokens, tokens.disabledText);
}

QIcon tintedIcon(const QString& glyph, const ThemeTokens& tokens)
{
    return tintedIcon(glyph, glyph, tokens);
}

void restyleActionGlyphs(const QList<ActionGlyph>& glyphs, const ThemeTokens& tokens)
{
    restyleGlyphsDisabledIn(glyphs, tokens, tokens.disabledText);
}

void restyleToolBarGlyphs(const QList<ActionGlyph>& glyphs, const ThemeTokens& tokens, const bool coloured)
{
    for (const ActionGlyph& glyph : glyphs) {
        if (!glyph.pAction) {
            continue;
        }
        const QString glyphOn = glyph.glyphOn.isEmpty() ? glyph.glyphOff : glyph.glyphOn;
        glyph.pAction->setIcon(coloured && glyph.identity.hue >= 0.0 ? identityIcon(glyph.glyphOff, glyphOn, glyph.identity, tokens, tokens.disabledBarText)
                                                                     : tintedIconDisabledIn(glyph.glyphOff, glyphOn, tokens, tokens.disabledBarText));
    }
}

QString noticeStyleSheet(const QString& frameSelector, const ThemeTokens& tokens)
{
    return qsl("%1 { background-color: %2; border: 1px solid %3; border-radius: %5px; }"
               "%1 QLabel { background: transparent; color: %4; }")
            .arg(frameSelector, tokens.accentSoft, tokens.accent.name(), tokens.mutedText.name(), QString::number(scmRadiusPanel));
}

QPixmap noticeGlyph(const NoticeKind kind, const ThemeTokens& tokens, const qreal devicePixelRatio)
{
    // Line glyphs, not the .ui files' bitmaps: tinting keeps only the alpha channel, and theirs is a solid disc or
    // triangle
    QString glyphFile;
    QColor glyphColour;
    switch (kind) {
    case NoticeKind::Warning:
        glyphFile = qsl(":/icons/editor-notice-warning.svg");
        glyphColour = stateColor(scmStateHue_warning, tokens.darkPage);
        break;
    case NoticeKind::Error:
        glyphFile = qsl(":/icons/editor-notice-error.svg");
        glyphColour = errorInk(tokens);
        break;
    case NoticeKind::Information:
        glyphFile = qsl(":/icons/editor-notice-info.svg");
        glyphColour = tokens.accentText;
        break;
    }

    QPixmap glyph = tintedGlyph(glyphPixmap(glyphFile), glyphColour).scaled(QSize(scmNoticeGlyphSize, scmNoticeGlyphSize) * devicePixelRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph.setDevicePixelRatio(devicePixelRatio);
    return glyph;
}

void applyNoticeGlyph(QLabel* pLabel, const NoticeKind kind, const ThemeTokens& tokens)
{
    if (!pLabel) {
        return;
    }
    pLabel->setPixmap(noticeGlyph(kind, tokens, pLabel->devicePixelRatioF()));
    pLabel->setMargin(0);
    pLabel->setFixedSize(scmNoticeGlyphSize, scmNoticeGlyphSize);
}

QString cardStyleSheet(const CardMetrics& metrics, const ThemeTokens& tokens)
{
    const QString cardProperty = QLatin1StringView(metrics.cardProperty);
    const QString padding = QString::number(metrics.padding);
    QString rules =
            qsl("QGroupBox[%1=\"true\"] { background-color: %2; border: 1px solid %3; border-radius: %4px;"
                " padding: %5px %6px %6px %6px; font-weight: bold; }"
                "QGroupBox[%1=\"true\"]::title { subcontrol-origin: padding; subcontrol-position: top left;"
                " left: %6px; top: %6px; padding: 0px; }"
                "QGroupBox[%1=\"true\"] > * { font-weight: normal; }")
                    .arg(cardProperty, tokens.card.name(), tokens.border.name(), QString::number(scmRadiusPanel), QString::number(metrics.padding + metrics.titleHeight + scmCardTitleGap), padding);
    if (metrics.plainProperty) {
        rules += qsl("QGroupBox[%1=\"true\"] { padding-top: %2px; }").arg(QLatin1StringView(metrics.plainProperty), padding);
    }
    if (metrics.flattenNestedGroupBoxes) {
        rules += qsl("QGroupBox[%1=\"true\"] QGroupBox { border: none; background: transparent; margin-top: 20px; padding: 0px 0px 0px 8px; font-weight: bold; }"
                     "QGroupBox[%1=\"true\"] QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 0px; padding: 0px; }")
                         .arg(cardProperty);
    }
    return rules;
}

QString cardIndicatorStyleSheet(const char* cardProperty, const ThemeTokens& tokens)
{
    const ChoiceMarkInks inks = choiceMarkInks(tokens);
    return choiceMarkRules(qsl("QGroupBox[%1=\"true\"]::indicator").arg(QLatin1StringView(cardProperty)), scmChoiceBoxRadius, inks.tick, inks.dash, inks.quietTick, inks.quietDash, inks, tokens);
}

static QRect measuredCardTitleRect(QWidget* pParent, const QString& indicatorRules, const char* cardProperty, const bool checkable)
{
    QGroupBox box(pParent);
    box.setProperty(cardProperty, true);
    box.setCheckable(checkable);
    // Never shown, but a box with no title has no label to place
    box.setTitle(qsl("Aa"));
    // Not the shell's stylesheet, which is the string being built; bold, as the title's line height is measured
    box.setStyleSheet(indicatorRules + qsl("QGroupBox[%1=\"true\"] { font-weight: bold; }").arg(QString::fromLatin1(cardProperty)));
    QStyleOptionGroupBox option;
    option.initFrom(&box);
    option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel;
    if (checkable) {
        option.subControls |= QStyle::SC_GroupBoxCheckBox;
        option.state |= QStyle::State_On;
    }
    option.text = box.title();
    option.textAlignment = Qt::AlignLeft;
    option.lineWidth = 0;
    option.midLineWidth = 0;
    return box.style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, &box);
}

int measuredCardTitleHeight(QWidget* pParent, const QString& indicatorRules, const char* cardProperty)
{
    // A checkable title is as tall as its indicator when the type is smaller than the box
    return qMax(measuredCardTitleRect(pParent, indicatorRules, cardProperty, true).height(), measuredCardTitleRect(pParent, indicatorRules, cardProperty, false).height());
}

QString inlineGlyph(const QPixmap& glyph)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    glyph.save(&buffer, "PNG");
    return qsl(R"(<img src="data:image/png;base64,%1" width="18" height="18">)").arg(QString::fromLatin1(png.toBase64()));
}

QString withLinkColour(const QString& richText, const QColor& colour)
{
    // Every anchor this is asked about is written "<a href=", so a plain
    // replace reaches all of them without parsing the document
    return QString(richText).replace(qsl("<a "), qsl("<a style=\"color: %1\" ").arg(colour.name()));
}

QFont fixedPitchFont(const QFont& base, const qreal scale)
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    // A platform with no theme of its own, as offscreen on Windows, answers with its proportional default. The bundled
    // face where the fonts folder registered it, or else any fixed-pitch family there is: registering one here would
    // change the font database under any font picker open at the time.
    if (!QFontInfo(font).fixedPitch()) {
        static const QString fixedFamily = []() {
            if (const QString bundled = qsl("Bitstream Vera Sans Mono"); QFontDatabase::hasFamily(bundled)) {
                return bundled;
            }
            const QStringList families = QFontDatabase::families();
            const auto found = std::find_if(families.cbegin(), families.cend(), [](const QString& family) {
                return QFontDatabase::isFixedPitch(family);
            });
            return found == families.cend() ? QString() : *found;
        }();
        font = QFont(fixedFamily);
        font.setStyleHint(QFont::TypeWriter);
    }
    if (base.pointSizeF() > 0.0) {
        font.setPointSizeF(base.pointSizeF() * scale);
    } else {
        font.setPixelSize(std::max(1, qRound(base.pixelSize() * scale)));
    }
    return font;
}

int typeSize(const TypeStep step)
{
    const QFont applicationFont = QApplication::font();
    // A font set in pixels answers -1 for its point size, so the size it is
    // actually rendered at is asked of QFontInfo instead
    qreal base = applicationFont.pointSizeF();
    if (base <= 0.0) {
        base = QFontInfo(applicationFont).pointSizeF();
    }

    qreal ratio = 1.0;
    switch (step) {
    case TypeStep::Caption:
        ratio = scmTypeRatio_caption;
        break;
    case TypeStep::Body:
        break;
    case TypeStep::Title:
        ratio = scmTypeRatio_title;
        break;
    case TypeStep::Display:
        ratio = scmTypeRatio_display;
        break;
    }
    return std::max(1, qRound(base * ratio));
}

QVariant controlValue(const QObject* pControl)
{
    if (const auto* pGroupBox = qobject_cast<const QGroupBox*>(pControl)) {
        return pGroupBox->isCheckable() ? QVariant(pGroupBox->isChecked()) : QVariant();
    }
    if (const auto* pCheckBox = qobject_cast<const QCheckBox*>(pControl)) {
        // The check state rather than isChecked(), for the tri-state boxes
        return QVariant::fromValue(pCheckBox->checkState());
    }
    if (const auto* pButton = qobject_cast<const QAbstractButton*>(pControl)) {
        if (qobject_cast<const QPushButton*>(pControl) || qobject_cast<const QToolButton*>(pControl)) {
            return {};
        }
        return pButton->isChecked();
    }
    if (const auto* pFontComboBox = qobject_cast<const QFontComboBox*>(pControl)) {
        return pFontComboBox->currentFont();
    }
    if (const auto* pComboBox = qobject_cast<const QComboBox*>(pControl)) {
        return pComboBox->currentIndex();
    }
    if (const auto* pSpinBox = qobject_cast<const QSpinBox*>(pControl)) {
        return pSpinBox->value();
    }
    if (const auto* pDoubleSpinBox = qobject_cast<const QDoubleSpinBox*>(pControl)) {
        return pDoubleSpinBox->value();
    }
    if (const auto* pDateTimeEdit = qobject_cast<const QDateTimeEdit*>(pControl)) {
        return pDateTimeEdit->dateTime();
    }
    if (const auto* pLineEdit = qobject_cast<const QLineEdit*>(pControl)) {
        return pLineEdit->text();
    }
    return {};
}

bool beingTypedInto(const QObject* pControl)
{
    // A spin box types into a line edit of its own, while emitting valueChanged() for each digit that makes a number
    if (const auto* pSpinBox = qobject_cast<const QAbstractSpinBox*>(pControl)) {
        pControl = pSpinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
    }
    const auto* pLineEdit = qobject_cast<const QLineEdit*>(pControl);
    return pLineEdit && pLineEdit->hasFocus() && pLineEdit->isModified();
}

SettingsSnapshot::SettingsSnapshot(const QWidget& owner, const QMap<QString, QKeySequence>& shortcuts)
: mOwner(owner)
, mCurrentShortcuts(shortcuts)
{
}

bool SettingsSnapshot::carriesValue(const QObject* pControl)
{
    return controlValue(pControl).isValid();
}

void SettingsSnapshot::take()
{
    const QHash<const QObject*, QVariant> previous = mValues;
    mValues.clear();
    for (const auto* pWidget : mOwner.findChildren<QWidget*>()) {
        const QVariant value = controlValue(pWidget);
        if (!value.isValid()) {
            continue;
        }
        // The preceding apply skipped a half-typed field, so its last populated value stands until the edit finishes
        if (const auto it = previous.constFind(pWidget); beingTypedInto(pWidget) && it != previous.constEnd()) {
            mValues.insert(pWidget, *it);
            continue;
        }
        mValues.insert(pWidget, value);
    }
    mShortcuts = mCurrentShortcuts;
}

void SettingsSnapshot::take(const QObject* pControl)
{
    mValues.insert(pControl, controlValue(pControl));
}

bool SettingsSnapshot::dirty(const QObject* pControl) const
{
    // The debounce is shared, so the apply about to read this was likely started by another control's edit
    if (beingTypedInto(pControl)) {
        return false;
    }
    const auto it = mValues.constFind(pControl);
    if (it == mValues.constEnd()) {
        return true;
    }
    return *it != controlValue(pControl);
}

// For a setting spread over several controls (borders, Discord privacy flags), any one changing triggers the
// write, still composed per control: a clean control contributes the Host's current value, not what it
// shows, which a script may have changed since. Members that are separate settings take their own guard.
bool SettingsSnapshot::anyDirty(const QList<const QObject*>& controls) const
{
    for (const auto* pControl : controls) {
        if (dirty(pControl)) {
            return true;
        }
    }
    return false;
}

bool SettingsSnapshot::shortcutsDirty() const
{
    return mCurrentShortcuts != mShortcuts;
}

bool SettingsSnapshot::shortcutDirty(const QString& key) const
{
    return mCurrentShortcuts.value(key) != mShortcuts.value(key);
}

bool SettingsSnapshot::pendingEdits(const QTimer* pApplyTimer, const QLineEdit* pSearchField) const
{
    // Until the apply runs the controls hold the user's values, and its closing refresh re-reads them anyway
    if (pApplyTimer && pApplyTimer->isActive()) {
        return true;
    }
    for (const auto* pWidget : mOwner.findChildren<QWidget*>()) {
        if (pWidget == pSearchField || !carriesValue(pWidget)) {
            continue;
        }
        // dirty() is false for a field being typed into, exactly the edit that must not be overwritten here
        if (beingTypedInto(pWidget) || dirty(pWidget)) {
            return true;
        }
    }
    if (shortcutsDirty()) {
        return true;
    }
    // A shortcut editor holds a capture until editingFinished, so differing from its last commit is an edit
    for (auto it = mEditors.cbegin(), end = mEditors.cend(); it != end; ++it) {
        if (it.value() && it.value()->keySequence() != mCurrentShortcuts.value(it.key())) {
            return true;
        }
    }
    return false;
}

TKeySequenceEdit* SettingsSnapshot::editorFor(const QString& key) const
{
    return mEditors.value(key).data();
}

void SettingsSnapshot::addEditor(const QString& key, TKeySequenceEdit* pEditor)
{
    mEditors.insert(key, pEditor);
}

} // namespace uiDesign
