/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgSystemMessageArea.h"

#include "mudlet.h"
#include "uiDesign.h"

#include <QResizeEvent>


dlgSystemMessageArea::dlgSystemMessageArea(QWidget* pParentWidget)
: QWidget(pParentWidget)
{
    // init generated dialog
    setupUi(this);

    if (QLayout* pNoticeLayout = frame_notificationArea->layout()) {
        pNoticeLayout->setContentsMargins(uiDesign::scmNoticePaddingHorizontal, uiDesign::scmNoticePaddingVertical, uiDesign::scmNoticePaddingHorizontal, uiDesign::scmNoticePaddingVertical);
        pNoticeLayout->setSpacing(uiDesign::scmNoticeSpacing);
    }
    // The .ui spacer is sized for a 64px picture; it only has to keep the close button on the first line
    verticalSpacer_closeButton->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    slot_applyAppearance();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgSystemMessageArea::slot_applyAppearance);
}

// A wrapping QLabel's own hint is its height at a narrower width of its choosing, so answer the
// layout's height-for-width at the width the notice really has
QSize dlgSystemMessageArea::sizeHint() const
{
    return heightAtOwnWidth(QWidget::sizeHint());
}

QSize dlgSystemMessageArea::minimumSizeHint() const
{
    return heightAtOwnWidth(QWidget::minimumSizeHint());
}

QSize dlgSystemMessageArea::heightAtOwnWidth(const QSize& fallback) const
{
    QLayout* pLayout = layout();
    if (!pLayout || !pLayout->hasHeightForWidth() || width() <= 0) {
        return fallback;
    }
    const int height = pLayout->totalHeightForWidth(width());
    return height > 0 ? QSize(fallback.width(), height) : fallback;
}

// A new width wraps to a different number of lines, which changes both hints
void dlgSystemMessageArea::resizeEvent(QResizeEvent* pEvent)
{
    QWidget::resizeEvent(pEvent);
    if (pEvent->oldSize().width() != pEvent->size().width()) {
        updateGeometry();
    }
}

// Nothing else may set a sheet on this frame: whichever setStyleSheet() lands last wins
void dlgSystemMessageArea::slot_applyAppearance()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    frame_notificationArea->setStyleSheet(uiDesign::noticeStyleSheet(qsl("QFrame#frame_notificationArea"), tokens));
    // Overrides the 64px full-colour bitmap the .ui file sets on each label
    uiDesign::applyNoticeGlyph(notificationAreaIconLabelInformation, uiDesign::NoticeKind::Information, tokens);
    uiDesign::applyNoticeGlyph(notificationAreaIconLabelWarning, uiDesign::NoticeKind::Warning, tokens);
    uiDesign::applyNoticeGlyph(notificationAreaIconLabelError, uiDesign::NoticeKind::Error, tokens);
    // On the label itself, not via the frame's descendant rule: the area is hidden while styled, and
    // the polish when a notice shows it writes the application's ink into the label's palette
    notificationAreaMessageBox->setStyleSheet(qsl("color: %1;").arg(tokens.mutedText.name()));
}
