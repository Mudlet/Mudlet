/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include "TFeatureCallout.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

namespace {
constexpr int arrowHeight = 10;
constexpr int arrowWidth = 18;
constexpr int cornerRadius = 8;
}

TFeatureCallout::TFeatureCallout(QWidget* pAnchor, const QString& title, const QString& body)
: QWidget(pAnchor->window(), Qt::ToolTip | Qt::FramelessWindowHint)
, mpAnchor(pAnchor)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, arrowHeight + 12, 16, 12);
    layout->setSpacing(6);

    auto* titleLabel = new QLabel(title, this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    auto* bodyLabel = new QLabel(body, this);
    bodyLabel->setWordWrap(true);
    bodyLabel->setMaximumWidth(300);
    layout->addWidget(bodyLabel);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch();
    //: Button that dismisses a balloon pointing out a newly added feature
    auto* gotItButton = new QPushButton(tr("Got it"), this);
    gotItButton->setCursor(Qt::PointingHandCursor);
    connect(gotItButton, &QPushButton::clicked, this, &QWidget::close);
    buttonRow->addWidget(gotItButton);
    layout->addLayout(buttonRow);

    // follow the anchor around as docks move and windows resize
    for (QWidget* pWidget = pAnchor; pWidget; pWidget = pWidget->parentWidget()) {
        pWidget->installEventFilter(this);
    }
}

void TFeatureCallout::showAnchored()
{
    if (!mpAnchor) {
        return;
    }
    adjustSize();
    reposition();
    show();
    raise();
}

void TFeatureCallout::reposition()
{
    if (!mpAnchor || !mpAnchor->isVisible()) {
        return;
    }
    const QPoint anchorTopCenter = mpAnchor->mapToGlobal(QPoint(mpAnchor->width() / 2, 0));
    const QPoint anchorBottomCenter = mpAnchor->mapToGlobal(QPoint(mpAnchor->width() / 2, mpAnchor->height()));
    const QRect available = mpAnchor->screen()->availableGeometry();
    mArrowOnTop = anchorBottomCenter.y() + 2 + height() <= available.bottom();
    layout()->setContentsMargins(16, mArrowOnTop ? arrowHeight + 12 : 12, 16, mArrowOnTop ? 12 : arrowHeight + 12);
    adjustSize();
    int x = anchorBottomCenter.x() - width() / 2;
    x = qBound(available.left(), x, available.right() - width());
    mArrowX = qBound(cornerRadius + arrowWidth / 2, anchorBottomCenter.x() - x, width() - cornerRadius - arrowWidth / 2);
    const int y = mArrowOnTop ? anchorBottomCenter.y() + 2 : anchorTopCenter.y() - height() - 2;
    move(x, y);
    update();
}

void TFeatureCallout::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    const qreal cardTop = mArrowOnTop ? arrowHeight : 1;
    path.addRoundedRect(QRectF(1, cardTop, width() - 2, height() - arrowHeight - 1), cornerRadius, cornerRadius);

    QPainterPath arrow;
    if (mArrowOnTop) {
        arrow.moveTo(mArrowX - arrowWidth / 2.0, arrowHeight + 1);
        arrow.lineTo(mArrowX, 1);
        arrow.lineTo(mArrowX + arrowWidth / 2.0, arrowHeight + 1);
    } else {
        arrow.moveTo(mArrowX - arrowWidth / 2.0, height() - arrowHeight - 1);
        arrow.lineTo(mArrowX, height() - 1);
        arrow.lineTo(mArrowX + arrowWidth / 2.0, height() - arrowHeight - 1);
    }
    arrow.closeSubpath();
    path = path.united(arrow);

    painter.setPen(QPen(palette().color(QPalette::Highlight), 1.5));
    painter.setBrush(palette().color(QPalette::Window));
    painter.drawPath(path);
}

bool TFeatureCallout::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::Move:
    case QEvent::Resize:
        reposition();
        break;
    case QEvent::Hide:
        if (watched == mpAnchor) {
            close();
        }
        break;
    case QEvent::MouseButtonPress:
        // the anchor got clicked, so the feature has been discovered - the
        // balloon has served its purpose
        if (watched == mpAnchor) {
            close();
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}
