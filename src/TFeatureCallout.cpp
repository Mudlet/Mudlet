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

#include "TFeatureCallout.h"

#include "mudlet.h"
#include "MudletApp.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

using namespace std::chrono_literals;

namespace {
constexpr int arrowHeight = 10;
constexpr int arrowWidth = 18;
constexpr int cornerRadius = 8;
// Stop showing a balloon the player keeps ignoring
constexpr int maximumAppearances = 5;
// Several features shipping in one release must not stack balloons
constexpr int maximumPerSession = 2;

QString dismissedKey(const QString& featureId)
{
    return qsl("whatsNew/%1/dismissed").arg(featureId);
}

QString shownCountKey(const QString& featureId)
{
    return qsl("whatsNew/%1/shownCount").arg(featureId);
}

QRect anchorRectFor(QWidget* pAnchor, QAction* pAnchorMenu)
{
    if (auto* pMenuBar = qobject_cast<QMenuBar*>(pAnchor); pMenuBar && pAnchorMenu) {
        return pMenuBar->actionGeometry(pAnchorMenu);
    }
    return pAnchor->rect();
}
} // namespace

TFeatureCallout::TFeatureCallout(const QString& featureId, QWidget* pAnchor, const QString& title, const QString& body, QAction* pAnchorMenu)
: QWidget(pAnchor->window(), Qt::ToolTip | Qt::FramelessWindowHint)
, mFeatureId(featureId)
, mpAnchor(pAnchor)
, mpAnchorMenu(pAnchorMenu)
, mAnnouncement(qsl("%1. %2").arg(title, body))
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
    connect(gotItButton, &QPushButton::clicked, this, [this]() {
        markDismissed();
        close();
    });
    buttonRow->addWidget(gotItButton);
    layout->addLayout(buttonRow);

    // follow the anchor around as docks move and windows resize
    for (QWidget* pWidget = pAnchor; pWidget; pWidget = pWidget->parentWidget()) {
        pWidget->installEventFilter(this);
    }

    mApplicationActive = qGuiApp->applicationState() == Qt::ApplicationActive;
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &TFeatureCallout::slot_applicationStateChanged);
}

void TFeatureCallout::maybeShow(const QString& featureId, QWidget* pAnchor, const QString& title, const QString& body)
{
    maybeShowImpl(featureId, pAnchor, nullptr, title, body);
}

void TFeatureCallout::maybeShow(const QString& featureId, QMenuBar* pMenuBar, QAction* pMenu, const QString& title, const QString& body)
{
    maybeShowImpl(featureId, pMenuBar, pMenu, title, body);
}

void TFeatureCallout::maybeShowImpl(const QString& featureId, QWidget* pAnchor, QAction* pAnchorMenu, const QString& title, const QString& body)
{
    if (!pAnchor) {
        return;
    }
    auto* settings = MudletApp::getQSettings();
    // players installing today experience the current interface as the
    // baseline, so nothing in it is "new" to them
    if (MudletApp::firstLaunch()) {
        settings->setValue(dismissedKey(featureId), true);
        return;
    }
    if (settings->value(dismissedKey(featureId), false).toBool()) {
        return;
    }
    if (settings->value(shownCountKey(featureId), 0).toInt() >= maximumAppearances) {
        return;
    }
    if (smSessionShown.contains(featureId) || smSessionShown.size() >= maximumPerSession) {
        return;
    }

    // let the widget holding the anchor settle into its final place first
    QTimer::singleShot(800ms, pAnchor, [featureId, pAnchor, pAnchorMenu = QPointer<QAction>(pAnchorMenu), title, body]() {
        if (!pAnchor->isVisible() || !pAnchor->rect().contains(anchorRectFor(pAnchor, pAnchorMenu))) {
            return;
        }
        if (MudletApp::getQSettings()->value(dismissedKey(featureId), false).toBool()) {
            return;
        }
        // the session budget is claimed here rather than up front, so an
        // anchor that stayed hidden for the whole delay does not use it up -
        // which is why the check runs again in here
        if (smSessionShown.contains(featureId) || smSessionShown.size() >= maximumPerSession) {
            return;
        }
        smSessionShown.insert(featureId);
        auto* pCallout = new TFeatureCallout(featureId, pAnchor, title, body, pAnchorMenu);
        pCallout->showAnchored();
    });
}

void TFeatureCallout::dismiss(const QString& featureId)
{
    auto* settings = MudletApp::getQSettings();
    if (!settings->value(dismissedKey(featureId), false).toBool()) {
        settings->setValue(dismissedKey(featureId), true);
    }
    for (QWidget* pWidget : QApplication::topLevelWidgets()) {
        if (auto* pCallout = qobject_cast<TFeatureCallout*>(pWidget); pCallout && pCallout->mFeatureId == featureId) {
            pCallout->close();
        }
    }
}

void TFeatureCallout::showAnchored()
{
    if (!anchorOnScreen()) {
        return;
    }
    if (!mApplicationActive) {
        mWaitingForActivation = true;
        return;
    }
    place();
}

void TFeatureCallout::place()
{
    adjustSize();
    reposition();
    show();
    raise();
    if (!mAnnounced) {
        mAnnounced = true;
        // an appearance is only spent once the balloon is really on screen
        auto* settings = MudletApp::getQSettings();
        settings->setValue(shownCountKey(mFeatureId), settings->value(shownCountKey(mFeatureId), 0).toInt() + 1);
        mudlet::self()->announce(mAnnouncement);
    }
}

void TFeatureCallout::slot_applicationStateChanged(const Qt::ApplicationState state)
{
    mApplicationActive = state == Qt::ApplicationActive;
    if (!mApplicationActive) {
        if (isVisible()) {
            mWaitingForActivation = true;
            hide();
            // hide() was measured not to unmap the tooltip window on X11,
            // leaving the balloon on top of the other application anyway.
            // Dropping the native window does, and show() builds a fresh one -
            // done on every platform because it costs nothing to recreate:
            destroy();
        }
        return;
    }
    if (!mWaitingForActivation) {
        return;
    }
    mWaitingForActivation = false;
    if (!anchorOnScreen()) {
        close();
        return;
    }
    place();
}

void TFeatureCallout::markDismissed()
{
    MudletApp::getQSettings()->setValue(dismissedKey(mFeatureId), true);
}

QRect TFeatureCallout::anchorRect() const
{
    return anchorRectFor(mpAnchor, mpAnchorMenu);
}

// A menu folded into the bar's overflow extension keeps a geometry past the bar's edge
bool TFeatureCallout::anchorOnScreen() const
{
    return mpAnchor && mpAnchor->isVisible() && mpAnchor->rect().contains(anchorRect());
}

void TFeatureCallout::reposition()
{
    // e.g. a narrowed window folded the menu into the bar's overflow; not a
    // dismissal, the player never engaged with the balloon
    if (!anchorOnScreen()) {
        close();
        return;
    }
    const QRect target = anchorRect();
    const QPoint anchorTopCenter = mpAnchor->mapToGlobal(QPoint(target.left() + target.width() / 2, target.top()));
    const QPoint anchorBottomCenter = mpAnchor->mapToGlobal(QPoint(target.left() + target.width() / 2, target.top() + target.height()));
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
        // any hidden ancestor means the anchor is no longer on screen; not a
        // dismissal though - the player never engaged with the balloon
        close();
        break;
    case QEvent::WindowStateChange:
        if (watched->isWidgetType() && static_cast<QWidget*>(watched)->isMinimized()) {
            close();
        }
        break;
    case QEvent::MouseButtonPress:
        // the anchor (or the menu it points at) got clicked, so the feature
        // has been discovered - the balloon has served its purpose
        if (watched == mpAnchor && anchorRect().contains(static_cast<QMouseEvent*>(event)->position().toPoint())) {
            markDismissed();
            close();
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}
