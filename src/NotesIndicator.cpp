/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Development Team                         *
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


#include "NotesIndicator.h"

#include "utils.h"

#include <QApplication>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleHints>

#include <algorithm>
#include <cmath>

namespace {
constexpr int kHoverDelta = 2;
constexpr int kIconSizeAnimationDurationMs = 120;
constexpr int kHoverAnimationDurationMs = 140;

[[nodiscard]] double srgbToLinear(const int channel)
{
    const double c = channel / 255.0;
    if (c <= 0.04045) {
        return c / 12.92;
    }
    return std::pow((c + 0.055) / 1.055, 2.4);
}

[[nodiscard]] double relativeLuminance(const QColor& color)
{
    const double r = srgbToLinear(color.red());
    const double g = srgbToLinear(color.green());
    const double b = srgbToLinear(color.blue());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

[[nodiscard]] double contrastRatio(const QColor& a, const QColor& b)
{
    const double l1 = relativeLuminance(a);
    const double l2 = relativeLuminance(b);
    const double lighter = std::max(l1, l2);
    const double darker = std::min(l1, l2);
    return (lighter + 0.05) / (darker + 0.05);
}
}

NotesIndicator::NotesIndicator(QWidget* pParent)
: QPushButton(pParent)
{
    setObjectName(qsl("notesIndicator"));

    QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    policy.setHorizontalStretch(0);
    policy.setVerticalStretch(0);
    policy.setHeightForWidth(false);
    setSizePolicy(policy);

    setFocusPolicy(Qt::NoFocus);
    setFlat(true);
    setText(QString());

    mpIconSizeAnimation = new QPropertyAnimation(this, "displayIconSize", this);
    mpIconSizeAnimation->setDuration(kIconSizeAnimationDurationMs);
    mpIconSizeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    mpHoverAnimation = new QPropertyAnimation(this, "hoverProgress", this);
    mpHoverAnimation->setDuration(kHoverAnimationDurationMs);
    mpHoverAnimation->setEasingCurve(QEasingCurve::OutCubic);

    setSize(mBaseIconSize);

    connect(this, &QPushButton::clicked, this, [this]() {
        if (isClickable()) {
            emit notesButtonClicked();
        }
    });
}

void NotesIndicator::setState(State state)
{
    if (mState == state) {
        return;
    }

    mState = state;
    updateIcon();

    if (!isClickable()) {
        mIsPressed = false;
        setDown(false);
    }

    if (!underMouse() || !isClickable()) {
        unsetCursor();
        animateHoverProgressTo(0.0);
        animateIconSizeTo(mBaseIconSize);
        return;
    }

    setCursor(Qt::PointingHandCursor);
    animateHoverProgressTo(1.0);
    animateIconSizeTo(mBaseIconSize + kHoverDelta);
}

void NotesIndicator::setSize(int size)
{
    if (size <= 0) {
        return;
    }

    const int maxIconSize = size + kHoverDelta;
    const bool alreadyConfigured = (mBaseIconSize == size) && (width() == maxIconSize) && (height() == maxIconSize) && !mIcons.isEmpty();
    if (alreadyConfigured) {
        return;
    }

    mBaseIconSize = size;

    setFixedSize(maxIconSize, maxIconSize);
    setDisplayIconSize(mBaseIconSize);

    loadIcons();
    updateIcon();
}

void NotesIndicator::setNoteCount(int count)
{
    if (mNoteCount == count) {
        return;
    }

    mNoteCount = count;
    updateIcon();
}

bool NotesIndicator::isClickable() const
{
    return mState != State::Empty;
}

void NotesIndicator::loadIcons()
{
    mIcons.clear();

    const QIcon closedFolder = style()->standardIcon(QStyle::SP_DirIcon);
    const QIcon openFolder = style()->standardIcon(QStyle::SP_DirOpenIcon);

    mIcons.insert(State::Empty, closedFolder);
    mIcons.insert(State::HasContent, makeIconWithBadge(openFolder, badgeColorForState(State::HasContent)));
    mIcons.insert(State::Modified, makeIconWithBadge(openFolder, badgeColorForState(State::Modified)));
    mIcons.insert(State::HasUnread, makeIconWithBadge(openFolder, badgeColorForState(State::HasUnread)));
}

void NotesIndicator::updateIcon()
{
    if (mIcons.isEmpty()) {
        loadIcons();
    }

    updateToolTip();

    if (mIcons.contains(mState)) {
        setIcon(mIcons.value(mState));
    }

    update();
}

void NotesIndicator::updateToolTip()
{
    if (mState == State::Empty || mNoteCount <= 0) {
        //: Notes indicator tooltip when there are no notes
        setToolTip(tr("No notes"));
        return;
    }

    const QString countString = (mNoteCount == 1) ? tr("1 note") : tr("%1 notes").arg(mNoteCount);

    QString extraContext;
    switch (mState) {
    case State::HasContent:
        extraContext = QString();
        break;
    case State::Modified:
        //: Notes indicator tooltip context when notes have been modified
        extraContext = tr("modified");
        break;
    case State::HasUnread:
        //: Notes indicator tooltip context when notes contain unread changes
        extraContext = tr("unread");
        break;
    case State::Empty:
        extraContext = QString();
        break;
    }

    //: Notes indicator tooltip; used as "%1 - Click to view" or "%1 (%2) - Click to view". %1 is a count, %2 is a short context like "unread".
    const QString clickToView = tr("Click to view");

    if (extraContext.isEmpty()) {
        setToolTip(tr("%1 - %2").arg(countString, clickToView));
        return;
    }

    setToolTip(tr("%1 (%2) - %3").arg(countString, extraContext, clickToView));
}

void NotesIndicator::mousePressEvent(QMouseEvent* pEvent)
{
    if (!isClickable()) {
        pEvent->ignore();
        return;
    }

    if (pEvent->button() == Qt::LeftButton) {
        mIsPressed = true;
        animateIconSizeTo(mBaseIconSize + kHoverDelta - 1);
    }

    QPushButton::mousePressEvent(pEvent);
}

void NotesIndicator::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (!mIsPressed) {
        QPushButton::mouseReleaseEvent(pEvent);
        return;
    }

    mIsPressed = false;

    if (!isClickable()) {
        unsetCursor();
        animateIconSizeTo(mBaseIconSize);
        animateHoverProgressTo(0.0);
        QPushButton::mouseReleaseEvent(pEvent);
        return;
    }

    if (rect().contains(pEvent->pos())) {
        animateIconSizeTo(mBaseIconSize + kHoverDelta);
        animateHoverProgressTo(1.0);
    } else {
        unsetCursor();
        animateIconSizeTo(mBaseIconSize);
        animateHoverProgressTo(0.0);
    }

    QPushButton::mouseReleaseEvent(pEvent);
}

void NotesIndicator::enterEvent(QEnterEvent* pEvent)
{
    QPushButton::enterEvent(pEvent);

    mIsHovered = true;

    if (!isClickable()) {
        return;
    }

    setCursor(Qt::PointingHandCursor);
    animateHoverProgressTo(1.0);

    if (mIsPressed) {
        animateIconSizeTo(mBaseIconSize + kHoverDelta - 1);
    } else {
        animateIconSizeTo(mBaseIconSize + kHoverDelta);
    }
}

void NotesIndicator::leaveEvent(QEvent* pEvent)
{
    QPushButton::leaveEvent(pEvent);

    mIsHovered = false;
    mIsPressed = false;

    unsetCursor();
    animateHoverProgressTo(0.0);
    animateIconSizeTo(mBaseIconSize);
}

void NotesIndicator::changeEvent(QEvent* pEvent)
{
    QPushButton::changeEvent(pEvent);

    switch (pEvent->type()) {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::StyleChange:
    case QEvent::ThemeChange:
        loadIcons();
        updateIcon();
        break;

    default:
        break;
    }
}

void NotesIndicator::paintEvent(QPaintEvent* pEvent)
{
    Q_UNUSED(pEvent)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (mHoverProgress > 0.0 && isClickable()) {
        QColor highlight = palette().color(QPalette::Highlight);

        const bool dark = isDarkTheme();
        const bool highContrast = isHighContrastTheme();

        int alpha = dark ? 70 : 55;
        if (highContrast) {
            alpha = dark ? 120 : 95;
        }
        if (mIsPressed) {
            alpha = std::min(255, alpha + 35);
        }

        highlight.setAlphaF((alpha / 255.0) * mHoverProgress);

        painter.setPen(Qt::NoPen);
        painter.setBrush(highlight);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    }

    const QIcon iconToDraw = mIcons.value(mState, QIcon());
    if (iconToDraw.isNull()) {
        return;
    }

    const bool highContrast = isHighContrastTheme();
    painter.setOpacity((mState == State::Empty) ? (highContrast ? 0.65 : 0.45) : 1.0);

    const int maxIconSize = mBaseIconSize + kHoverDelta;
    const int iconSize = std::clamp(mDisplayIconSize, 1, maxIconSize);

    const QRect iconRect((width() - iconSize) / 2, (height() - iconSize) / 2, iconSize, iconSize);
    iconToDraw.paint(&painter, iconRect, Qt::AlignCenter, QIcon::Normal);
}

void NotesIndicator::animateIconSizeTo(int targetSize)
{
    if (!mpIconSizeAnimation) {
        return;
    }

    const int maxIconSize = mBaseIconSize + kHoverDelta;
    targetSize = std::clamp(targetSize, 1, maxIconSize);

    if (mDisplayIconSize == targetSize) {
        return;
    }

    mpIconSizeAnimation->stop();
    mpIconSizeAnimation->setStartValue(mDisplayIconSize);
    mpIconSizeAnimation->setEndValue(targetSize);
    mpIconSizeAnimation->start();
}

void NotesIndicator::animateHoverProgressTo(qreal targetProgress)
{
    if (!mpHoverAnimation) {
        return;
    }

    targetProgress = std::clamp(targetProgress, 0.0, 1.0);

    if (qFuzzyCompare(mHoverProgress, targetProgress)) {
        return;
    }

    mpHoverAnimation->stop();
    mpHoverAnimation->setStartValue(mHoverProgress);
    mpHoverAnimation->setEndValue(targetProgress);
    mpHoverAnimation->start();
}

void NotesIndicator::setDisplayIconSize(int size)
{
    if (mDisplayIconSize == size) {
        return;
    }

    mDisplayIconSize = size;
    setIconSize(QSize(size, size));
    update();
}

void NotesIndicator::setHoverProgress(qreal progress)
{
    if (qFuzzyCompare(mHoverProgress, progress)) {
        return;
    }

    mHoverProgress = progress;
    update();
}

QColor NotesIndicator::badgeColorForState(State state) const
{
    const bool dark = isDarkTheme();

    switch (state) {
    case State::HasContent:
        return palette().color(QPalette::Highlight);

    case State::Modified:
        return dark ? QColor(255, 200, 80) : QColor(225, 160, 0);

    case State::HasUnread:
        return dark ? QColor(90, 220, 90) : QColor(0, 170, 0);

    case State::Empty:
        break;
    }

    return palette().color(QPalette::WindowText);
}

QIcon NotesIndicator::makeIconWithBadge(const QIcon& baseIcon, const QColor& badgeColor) const
{
    const int size = mBaseIconSize + kHoverDelta;

    QPixmap pixmap = baseIcon.pixmap(QSize(size, size), devicePixelRatioF(), QIcon::Normal, QIcon::Off);
    if (pixmap.isNull()) {
        return baseIcon;
    }

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const bool dark = isDarkTheme();
    const bool highContrast = isHighContrastTheme();

    const int badgeRadius = std::max(3, size / 5);
    const QPoint badgeCenter(size - badgeRadius - 1, size - badgeRadius - 1);

    QColor outline = dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
    outline.setAlpha(highContrast ? 255 : 170);

    QColor fill = badgeColor;
    fill.setAlpha(highContrast ? 255 : 235);

    painter.setPen(QPen(outline, 1));
    painter.setBrush(fill);
    painter.drawEllipse(badgeCenter, badgeRadius, badgeRadius);

    return QIcon(pixmap);
}

bool NotesIndicator::isDarkTheme() const
{
    const auto scheme = qApp->styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark) {
        return true;
    }
    if (scheme == Qt::ColorScheme::Light) {
        return false;
    }

    return palette().color(QPalette::Window).lightnessF() < 0.5;
}

bool NotesIndicator::isHighContrastTheme() const
{
    const QColor background = palette().color(QPalette::Window);
    const QColor foreground = palette().color(QPalette::WindowText);

    return contrastRatio(background, foreground) >= 7.0;
}
