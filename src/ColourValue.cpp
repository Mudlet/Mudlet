/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "ColourValue.h"

#include "uiDesign.h"
#include "utils.h"

#include <QAbstractButton>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QToolTip>

#include <algorithm>

namespace uiDesign {

namespace {

constexpr int rowHeight(const ColourRowSize size)
{
    return size == ColourRowSize::Compact ? scmColourWellHeight : scmInputHeight;
}

// The field left showing round the swatch, as in a colour well, so the two read as one kind of thing
constexpr int swatchInset(const ColourRowSize size)
{
    return size == ColourRowSize::Compact ? 2 : 3;
}

constexpr int swatchSide(const ColourRowSize size)
{
    return rowHeight(size) - 2 * (scmInputBorderWidth + swatchInset(size));
}

// ...and the swatch's corner, an input's corner taken in by that inset so the smaller square keeps the same curve
constexpr int swatchRadius(const ColourRowSize size)
{
    return std::max(2, scmRadiusInput - swatchInset(size));
}

constexpr int colourButtonGlyphSize(const ColourRowSize size)
{
    return size == ColourRowSize::Compact ? 14 : 16;
}

// What QLineEdit leaves for its cursor at either end of the text
constexpr int scmCursorRoom = 2;

// Held off 0 and 100 while the colour is neither quite clear nor quite opaque, so the box never says it is
int percentOf(const int alpha)
{
    return std::clamp(qRound(alpha * 100 / 255.0), alpha > 0 ? 1 : 0, alpha < 255 ? 99 : 100);
}

} // namespace

ColourValue::ColourValue(QWidget* pParent, const ColourRowSize size)
: QLineEdit(pParent)
, mSize(size)
{
    // Every prefix of a code is let through, so a code can be typed a character at a time; commit() decides
    setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("#?[0-9A-Fa-f]{0,6}")), this));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    if (mSize == ColourRowSize::Compact) {
        holdHeight(this, rowHeight(mSize));
    }
    keepTextClearOfSwatch();
}

void ColourValue::setColour(const QColor& colour)
{
    if (!colour.isValid()) {
        clear();
        return;
    }
    mColour = colour;
    mShows = Shows::Own;
    setPlaceholderText(mDefaultWord);
    setText(shownCode());
    showOpacity();
    update();
}

void ColourValue::setDefault(const QColor& drawnWith)
{
    mColour = drawnWith;
    mShows = Shows::Default;
    setPlaceholderText(mDefaultWord);
    setText(QString());
    showOpacity();
    update();
}

void ColourValue::clear()
{
    mColour = QColor();
    mShows = Shows::Nothing;
    setPlaceholderText(QString());
    setText(QString());
    showOpacity();
    update();
}

void ColourValue::setDefaultWord(const QString& word)
{
    mDefaultWord = word;
    if (mShows != Shows::Nothing) {
        setPlaceholderText(word);
    }
    updateGeometry();
}

QSpinBox* ColourValue::opacityBox()
{
    if (mpOpacity) {
        return mpOpacity;
    }
    mpOpacity = new QSpinBox(parentWidget());
    mpOpacity->setRange(0, 100);
    //: The sign after the number in the box that sets how opaque a colour is, beside the colour's code in the room properties and the settings; a language that writes the sign apart from the number starts this with a space
    mpOpacity->setSuffix(tr("%"));
    // Taken on Return, on leaving the box and on each step of its arrows, not at every digit typed
    mpOpacity->setKeyboardTracking(false);
    //: Tooltip and screen reader name of the box beside a colour's code that sets how opaque the colour is, as a percentage, in the room properties and the settings
    const QString opacity = tr("Opacity");
    mpOpacity->setToolTip(opacity);
    mpOpacity->setAccessibleName(opacity);
    connect(mpOpacity, &QSpinBox::valueChanged, this, &ColourValue::takeOpacity);
    mpOpacity->installEventFilter(this);
    // As tall as the field beside it, where a styled spin box asks for a few pixels more
    fitNumberBoxes({mpOpacity.data()}, rowHeight(mSize));
    showOpacity();
    return mpOpacity;
}

QString ColourValue::shownCode() const
{
    return mShows == Shows::Own ? mColour.name(QColor::HexRgb).toUpper() : QString();
}

bool ColourValue::edited() const
{
    return text() != shownCode();
}

bool ColourValue::opacityEdited() const
{
    return mpOpacity && mpOpacity->cleanText() != mpOpacity->locale().toString(mpOpacity->value());
}

void ColourValue::revert()
{
    setText(shownCode());
}

void ColourValue::commit()
{
    if (!edited()) {
        return;
    }
    QString typed = text().trimmed();
    if (typed.isEmpty()) {
        if (mShows == Shows::Own && !mDefaultWord.isEmpty()) {
            emit defaultEntered();
        }
        // Whoever took the signal has shown the default by now; nobody did if the field has no default word
        if (edited()) {
            revert();
        }
        return;
    }
    if (typed.startsWith(QLatin1Char('#'))) {
        typed.remove(0, 1);
    }
    if (typed.size() == 3) {
        typed = QString(typed.at(0)).repeated(2) + QString(typed.at(1)).repeated(2) + QString(typed.at(2)).repeated(2);
    }
    QColor typedColour = typed.size() == 6 ? QColor::fromString(QString(QLatin1Char('#') + typed)) : QColor();
    if (!typedColour.isValid()) {
        revert();
        return;
    }
    // Only the colour is typed here; how see-through it is stays as it was
    typedColour.setAlpha(mColour.isValid() ? mColour.alpha() : 255);
    if (mShows == Shows::Own && typedColour == mColour) {
        revert();
        return;
    }
    setColour(typedColour);
    emit colourEntered(typedColour);
}

void ColourValue::showOpacity()
{
    if (!mpOpacity) {
        return;
    }
    const QSignalBlocker blocker(mpOpacity);
    mpOpacity->setValue(mColour.isValid() ? percentOf(mColour.alpha()) : 100);
    // Items that do not agree on a colour have no one opacity to set until a colour is given them
    mpOpacity->setEnabled(isEnabled() && mShows != Shows::Nothing);
}

void ColourValue::takeOpacity(const int percent)
{
    if (mShows == Shows::Nothing) {
        return;
    }
    QColor colour = mColour;
    colour.setAlpha(qRound(percent * 255 / 100.0));
    if (mShows == Shows::Own && colour == mColour) {
        return;
    }
    setColour(colour);
    emit colourEntered(colour);
}

void ColourValue::keepTextClearOfSwatch()
{
    // The text starts where the input recipe's padding would put it beside the swatch
    const int swatchRoom = swatchInset(mSize) + swatchSide(mSize);
    if (layoutDirection() == Qt::RightToLeft) {
        setTextMargins(0, 0, swatchRoom, 0);
    } else {
        setTextMargins(swatchRoom, 0, 0, 0);
    }
}

namespace {
bool takesOrDropsAnEdit(const QEvent* pEvent)
{
    const int key = static_cast<const QKeyEvent*>(pEvent)->key();
    return key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Escape;
}
} // namespace

bool ColourValue::event(QEvent* pEvent)
{
    // Return and Escape belong to a typed code before they are a dialog's default button or its way out
    if (pEvent->type() == QEvent::ShortcutOverride && edited() && takesOrDropsAnEdit(pEvent)) {
        pEvent->accept();
        return true;
    }
    if (pEvent->type() == QEvent::ToolTip) {
        if (auto* pHelp = static_cast<QHelpEvent*>(pEvent); onSwatch(pHelp->pos())) {
            const QString words = mpChoose->toolTip().isEmpty() ? mpChoose->accessibleName() : mpChoose->toolTip();
            QToolTip::showText(pHelp->globalPos(), words, this, swatchTarget());
            return true;
        }
    }
    return QLineEdit::event(pEvent);
}

// The opacity box keeps the field's rule: Return takes a typed number and Escape drops it, and only with nothing
// typed do they go on to the dialog
bool ColourValue::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched != mpOpacity || (pEvent->type() != QEvent::ShortcutOverride && pEvent->type() != QEvent::KeyPress) || !takesOrDropsAnEdit(pEvent) || !opacityEdited()) {
        return QLineEdit::eventFilter(pWatched, pEvent);
    }
    pEvent->accept();
    if (pEvent->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(pEvent)->key() == Qt::Key_Escape) {
            mpOpacity->setValue(mpOpacity->value());
        } else {
            mpOpacity->interpretText();
        }
    }
    return true;
}

void ColourValue::keyPressEvent(QKeyEvent* pEvent)
{
    // With nothing typed, both go on to the dialog as they would from any other field
    if (edited()) {
        switch (pEvent->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            commit();
            pEvent->accept();
            return;
        case Qt::Key_Escape:
            revert();
            pEvent->accept();
            return;
        default:
            break;
        }
    }
    QLineEdit::keyPressEvent(pEvent);
}

void ColourValue::focusOutEvent(QFocusEvent* pEvent)
{
    // A context menu takes the focus while the code is still being edited
    if (pEvent->reason() != Qt::PopupFocusReason) {
        commit();
    }
    QLineEdit::focusOutEvent(pEvent);
}

void ColourValue::changeEvent(QEvent* pEvent)
{
    QLineEdit::changeEvent(pEvent);
    if (pEvent->type() == QEvent::EnabledChange) {
        showOpacity();
        hoverSwatch(false);
    } else if (pEvent->type() == QEvent::LayoutDirectionChange) {
        keepTextClearOfSwatch();
    }
}

QSize ColourValue::sizeHint() const
{
    ensurePolished();
    const QFontMetrics metrics = fontMetrics();
    int widestDigit = 0;
    for (const QChar digit : qsl("0123456789ABCDEF")) {
        widestDigit = std::max(widestDigit, metrics.horizontalAdvance(digit));
    }
    const int words = std::max(metrics.horizontalAdvance(QLatin1Char('#')) + 6 * widestDigit, metrics.horizontalAdvance(mDefaultWord));
    const QMargins margins = textMargins();
    QStyleOptionFrame option;
    initStyleOption(&option);
    const int height = mSize == ColourRowSize::Compact ? rowHeight(mSize) : QLineEdit::sizeHint().height();
    const QSize contents(margins.left() + scmCursorRoom + words + scmCursorRoom + margins.right(), height);
    return {style()->sizeFromContents(QStyle::CT_LineEdit, &option, contents, this).width(), height};
}

QSize ColourValue::minimumSizeHint() const
{
    return sizeHint();
}

void ColourValue::paintEvent(QPaintEvent* pEvent)
{
    QLineEdit::paintEvent(pEvent);
    if (!mColour.isValid()) {
        return;
    }

    const ThemeTokens tokens = themeTokens();
    const bool available = isEnabled();
    const qreal half = scmInputBorderWidth / 2.0;
    const int radius = swatchRadius(mSize);
    const QRect swatch = swatchRect();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath swatchCorner;
    swatchCorner.addRoundedRect(QRectF(swatch), radius, radius);
    painter.setClipPath(swatchCorner);
    if (mColour.alpha() < 255) {
        painter.drawTiledPixmap(swatch, checkerboardTile(devicePixelRatioF()));
    }
    // Unavailable, the colour keeps its hue and loses most of its saturation, as a colour well's does
    const QColor shown = available ? mColour : QColor::fromHsl(mColour.hslHue(), mColour.hslSaturation() / 4, mColour.lightness(), mColour.alpha());
    painter.fillRect(swatch, shown);
    painter.setClipping(false);
    // So a swatch as light as the field under it still has an edge
    painter.setPen(QPen(available ? tokens.border : disabledBorderColour(tokens), scmInputBorderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(swatch).adjusted(half, half, -half, -half), radius - half, radius - half);
    // Just outside the swatch, in the inset between it and the field's frame
    if (mSwatchHovered && swatchClickable()) {
        painter.setPen(QPen(tokens.accent, scmInputBorderWidth));
        painter.drawRoundedRect(QRectF(swatch).adjusted(-half, -half, half, half), radius + half, radius + half);
    }
}

void ColourValue::setChooseButton(QAbstractButton* pChoose)
{
    mpChoose = pChoose;
    setMouseTracking(true);
}

QRect ColourValue::swatchRect() const
{
    const int inset = scmInputBorderWidth + swatchInset(mSize);
    const int side = swatchSide(mSize);
    return QStyle::visualRect(layoutDirection(), rect(), QRect(inset, (height() - side) / 2, side, side));
}

QRect ColourValue::swatchTarget() const
{
    const int inset = swatchInset(mSize);
    return swatchRect().adjusted(-inset, -inset, inset, inset);
}

// Only a swatch on show stands for Choose: a field showing no colour has none to click
bool ColourValue::swatchClickable() const
{
    return mpChoose && mpChoose->isEnabled() && isEnabled() && mColour.isValid();
}

bool ColourValue::onSwatch(const QPoint& position) const
{
    return swatchClickable() && swatchTarget().contains(position);
}

void ColourValue::hoverSwatch(const bool over)
{
    if (over == mSwatchHovered) {
        return;
    }
    mSwatchHovered = over;
    setCursor(over ? Qt::PointingHandCursor : Qt::IBeamCursor);
    update();
}

void ColourValue::mousePressEvent(QMouseEvent* pEvent)
{
    if (pEvent->button() == Qt::LeftButton && onSwatch(pEvent->position().toPoint())) {
        mSwatchPressed = true;
        pEvent->accept();
        return;
    }
    QLineEdit::mousePressEvent(pEvent);
}

void ColourValue::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (mSwatchPressed && pEvent->button() == Qt::LeftButton) {
        mSwatchPressed = false;
        pEvent->accept();
        // As a button does: only a release over what was pressed acts
        if (onSwatch(pEvent->position().toPoint())) {
            mpChoose->click();
        }
        return;
    }
    QLineEdit::mouseReleaseEvent(pEvent);
}

// A double click on the swatch would otherwise select the code beside it
void ColourValue::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
    if (onSwatch(pEvent->position().toPoint())) {
        pEvent->accept();
        return;
    }
    QLineEdit::mouseDoubleClickEvent(pEvent);
}

void ColourValue::mouseMoveEvent(QMouseEvent* pEvent)
{
    hoverSwatch(onSwatch(pEvent->position().toPoint()));
    // A drag begun on the swatch selects no text
    if (mSwatchPressed) {
        pEvent->accept();
        return;
    }
    QLineEdit::mouseMoveEvent(pEvent);
}

void ColourValue::leaveEvent(QEvent* pEvent)
{
    hoverSwatch(false);
    QLineEdit::leaveEvent(pEvent);
}

void setColourButtonGlyph(QAbstractButton* pButton, const ColourButton kind, const ThemeTokens& tokens, const ColourRowSize size)
{
    const int side = rowHeight(size);
    pButton->setText(QString());
    pButton->setFixedSize(side, side);
    if (size == ColourRowSize::Compact) {
        holdHeight(pButton, side);
    }
    pButton->setIcon(tintedIcon(kind == ColourButton::Choose ? qsl(":/icons/control-choose-colour.svg") : qsl(":/icons/control-reset.svg"), tokens));
    const int glyph = colourButtonGlyphSize(size);
    pButton->setIconSize(QSize(glyph, glyph));
}

} // namespace uiDesign
