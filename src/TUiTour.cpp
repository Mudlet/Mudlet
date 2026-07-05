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

#include "TUiTour.h"

#include "Host.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "mudlet.h"

#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QTextDocumentFragment>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
constexpr int spotlightMargin = 6;
constexpr int spotlightRadius = 8;
constexpr int cardMaxWidth = 420;
constexpr int cardScreenMargin = 12;
constexpr int cardSpotlightGap = 16;
const QLatin1String settingsKeyTourShown("uiTourShown");
}

TUiTour::TUiTour(mudlet* pMainWindow)
: QWidget(pMainWindow)
, mpMainWindow(pMainWindow)
{
    setObjectName(qsl("uiTour"));
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    //: Name of the interface tour overlay, announced by screen readers
    setAccessibleName(tr("Mudlet interface tour"));
    parentWidget()->installEventFilter(this);

    createCard();
    buildSteps();
}

// The tour is meant to run automatically only once ever, and only for players
// new to Mudlet - it can always be revisited via Help menu
bool TUiTour::shouldShowOnFirstProfile()
{
    if (mudlet::self()->experiencedMudletPlayer()) {
        return false;
    }
    return !mudlet::getQSettings()->value(settingsKeyTourShown, false).toBool();
}

void TUiTour::rememberShown()
{
    auto* settings = mudlet::getQSettings();
    settings->setValue(settingsKeyTourShown, true);
    settings->sync();
}

void TUiTour::start()
{
    resizeToParent();
    show();
    raise();
    setFocus();
    setStep(0, 1);
}

void TUiTour::createCard()
{
    mpCard = new QFrame(this);
    mpCard->setObjectName(qsl("uiTourCard"));
    mpCard->setAutoFillBackground(true);
    mpCard->setFrameShape(QFrame::StyledPanel);
    mpCard->setStyleSheet(qsl("QFrame#uiTourCard {"
                              " background-color: palette(window);"
                              " border: 1px solid palette(mid);"
                              " border-radius: 8px;"
                              "}"));

    mpTitleLabel = new QLabel(mpCard);
    mpTitleLabel->setWordWrap(true);
    QFont titleFont = mpTitleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    mpTitleLabel->setFont(titleFont);

    mpBodyLabel = new QLabel(mpCard);
    mpBodyLabel->setWordWrap(true);
    mpBodyLabel->setTextFormat(Qt::RichText);

    mpProgressLabel = new QLabel(mpCard);

    //: Button on the interface tour that dismisses the tour
    mpSkipButton = new QPushButton(tr("Skip tour"), mpCard);
    mpSkipButton->setFlat(true);
    //: Button on the interface tour that goes back to the previous step
    mpBackButton = new QPushButton(tr("Back"), mpCard);
    //: Button on the interface tour that advances to the next step
    mpNextButton = new QPushButton(tr("Next"), mpCard);
    mpNextButton->setDefault(true);

    connect(mpSkipButton, &QPushButton::clicked, this, &TUiTour::slot_finish);
    connect(mpBackButton, &QPushButton::clicked, this, &TUiTour::slot_back);
    connect(mpNextButton, &QPushButton::clicked, this, &TUiTour::slot_next);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(mpProgressLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(mpSkipButton);
    buttonLayout->addWidget(mpBackButton);
    buttonLayout->addWidget(mpNextButton);

    auto* cardLayout = new QVBoxLayout(mpCard);
    cardLayout->setContentsMargins(16, 16, 16, 12);
    cardLayout->setSpacing(8);
    cardLayout->addWidget(mpTitleLabel);
    cardLayout->addWidget(mpBodyLabel);
    cardLayout->addLayout(buttonLayout);
}

void TUiTour::buildSteps()
{
    auto activeConsole = []() -> TMainConsole* {
        Host* pHost = mudlet::self()->getActiveHost();
        return pHost ? pHost->mpConsole.data() : nullptr;
    };
    auto widgetRect = [this](QWidget* widget) -> QRect {
        if (!widget || !widget->isVisible()) {
            return {};
        }
        return {widget->mapTo(mpMainWindow, QPoint(0, 0)), widget->size()};
    };
    auto toolbarButtonRect = [this, widgetRect](const QString& name) -> QRect {
        QToolBar* toolbar = mpMainWindow->mpMainToolBar;
        if (!toolbar || !toolbar->isVisible()) {
            return {};
        }
        return widgetRect(toolbar->findChild<QWidget*>(name));
    };
    auto menuTitleRect = [this](QMenu* menu) -> QRect {
        QMenuBar* menuBar = mpMainWindow->menuBar();
        if (!menu || !menuBar || !menuBar->isVisible()) {
            return {};
        }
        const QRect titleRect = menuBar->actionGeometry(menu->menuAction());
        if (titleRect.isEmpty()) {
            return {};
        }
        return {menuBar->mapTo(mpMainWindow, titleRect.topLeft()), titleRect.size()};
    };

    mSteps.clear();

    mSteps.push_back({nullptr,
                      //: Title of the first step of the interface tour
                      tr("Welcome to Mudlet!"),
                      //: Body of the first step of the interface tour
                      tr("New here? This quick tour points out the most important parts of Mudlet - it takes less than a minute. "
                         "Click anywhere or use the arrow keys to move through it.")});

    mSteps.push_back({[activeConsole, widgetRect]() -> QRect {
                          TMainConsole* console = activeConsole();
                          return console ? widgetRect(console->mpMainDisplay) : QRect();
                      },
                      //: Title of the interface tour step highlighting the main text display
                      tr("The game window"),
                      //: Body of the interface tour step highlighting the main text display
                      tr("Text from the game appears here. Scroll up to review earlier text - the newest text stays visible in a split view while you do.")});

    mSteps.push_back({[activeConsole, widgetRect]() -> QRect {
                          TMainConsole* console = activeConsole();
                          return console ? widgetRect(console->mpCommandLine.data()) : QRect();
                      },
                      //: Title of the interface tour step highlighting the command input line
                      tr("The input line"),
                      //: Body of the interface tour step highlighting the command input line
                      tr("Type game commands here and press Enter to send them. Use the up and down arrow keys to bring back commands you typed before.")});

    // The toolbar is hidden by default, so these two steps point at the
    // equivalent menu when its button isn't on screen
    mSteps.push_back({[this, toolbarButtonRect, menuTitleRect]() -> QRect {
                          const QRect buttonRect = toolbarButtonRect(qsl("triggers_action"));
                          return buttonRect.isEmpty() ? menuTitleRect(mpMainWindow->menuEditor) : buttonRect;
                      },
                      //: Title of the interface tour step highlighting the scripting tools
                      tr("Automate your game"),
                      //: Body of the interface tour step highlighting the scripting tools
                      tr("Triggers, aliases, timers and scripts let Mudlet react to the game for you and shorten what you type. "
                         "You will find them in the script editor, right here - start simple, no programming needed.")});

    mSteps.push_back({[this, toolbarButtonRect, menuTitleRect]() -> QRect {
                          const QRect buttonRect = toolbarButtonRect(qsl("settings_action"));
                          return buttonRect.isEmpty() ? menuTitleRect(mpMainWindow->menuOptions) : buttonRect;
                      },
                      //: Title of the interface tour step highlighting the preferences
                      tr("Make Mudlet yours"),
                      //: Body of the interface tour step highlighting the preferences
                      tr("Fonts, colors, the map, accessibility options and much more can be adjusted in the settings, right here.")});

    mSteps.push_back({nullptr,
                      //: Title of the last step of the interface tour
                      tr("That's it - have fun!"),
                      //: Body of the last step of the interface tour. The tour can be re-run via the named menu entry.
                      tr("For a hands-on lesson, connect to the <b>Mudlet Tutorial</b> game. "
                         "And if you ever want to see this tour again, it lives in Help → Take a UI tour.")});
}

void TUiTour::setStep(int index, int direction)
{
    const int stepCount = static_cast<int>(mSteps.size());
    // A step's target can be off screen right now, e.g. a toolbar button
    // tucked away in the overflow area
    while (index >= 0 && index < stepCount) {
        const TourStep& step = mSteps.at(index);
        if (!step.spotlightResolver || !step.spotlightResolver().isEmpty()) {
            break;
        }
        index += direction;
    }
    if (index < 0) {
        index = 0;
    }
    if (index >= stepCount) {
        slot_finish();
        return;
    }
    mCurrentStep = index;
    updateCard();
    update();
}

void TUiTour::updateCard()
{
    const TourStep& step = mSteps.at(mCurrentStep);
    mpTitleLabel->setText(step.title);
    mpBodyLabel->setText(step.body);
    //: Progress through the interface tour, %1 is the current step number, %2 the total number of steps
    mpProgressLabel->setText(tr("%1 of %2").arg(QString::number(mCurrentStep + 1), QString::number(mSteps.size())));

    const bool lastStep = mCurrentStep == static_cast<int>(mSteps.size()) - 1;
    mpBackButton->setEnabled(mCurrentStep > 0);
    mpSkipButton->setVisible(!lastStep);
    //: Button on the last step of the interface tour that closes it. The other label option is "Next".
    mpNextButton->setText(lastStep ? tr("Finish") : tr("Next"));

    positionCard();

    mudlet::self()->announce(qsl("%1. %2").arg(step.title, QTextDocumentFragment::fromHtml(step.body).toPlainText()));
}

QRect TUiTour::spotlightRect() const
{
    const TourStep& step = mSteps.at(mCurrentStep);
    if (!step.spotlightResolver) {
        return {};
    }
    const QRect targetRect = step.spotlightResolver();
    if (targetRect.isEmpty()) {
        return {};
    }
    return targetRect.adjusted(-spotlightMargin, -spotlightMargin, spotlightMargin, spotlightMargin).intersected(rect());
}

void TUiTour::positionCard()
{
    mpCard->setFixedWidth(qMin(cardMaxWidth, width() - 2 * cardScreenMargin));
    mpCard->adjustSize();

    const QRect within = rect().adjusted(cardScreenMargin, cardScreenMargin, -cardScreenMargin, -cardScreenMargin);
    const QRect spot = spotlightRect();
    QRect cardRect = mpCard->rect();

    if (spot.isEmpty()) {
        cardRect.moveCenter(rect().center());
    } else {
        // Clamped so a spotlight near a window corner still gets its card
        // alongside rather than falling back to the window center
        const int clampedX = qBound(within.left(), spot.center().x() - cardRect.width() / 2, qMax(within.left(), within.right() - cardRect.width()));
        const int clampedY = qBound(within.top(), spot.center().y() - cardRect.height() / 2, qMax(within.top(), within.bottom() - cardRect.height()));

        QRect below = cardRect;
        below.moveLeft(clampedX);
        below.moveTop(spot.bottom() + cardSpotlightGap);

        QRect above = cardRect;
        above.moveLeft(clampedX);
        above.moveBottom(spot.top() - cardSpotlightGap);

        QRect right = cardRect;
        right.moveTop(clampedY);
        right.moveLeft(spot.right() + cardSpotlightGap);

        QRect left = cardRect;
        left.moveTop(clampedY);
        left.moveRight(spot.left() - cardSpotlightGap);

        // When nothing fits (the spotlight covers most of the window) the
        // card floats over the spotlit area
        cardRect.moveCenter(rect().center());
        for (const QRect& candidate : {below, above, right, left}) {
            if (within.contains(candidate)) {
                cardRect = candidate;
                break;
            }
        }
    }

    cardRect.moveLeft(qBound(within.left(), cardRect.left(), qMax(within.left(), within.right() - cardRect.width())));
    cardRect.moveTop(qBound(within.top(), cardRect.top(), qMax(within.top(), within.bottom() - cardRect.height())));
    mpCard->setGeometry(cardRect);
}

void TUiTour::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect spot = spotlightRect();
    QPainterPath dimPath;
    dimPath.addRect(rect());
    if (!spot.isEmpty()) {
        QPainterPath cutout;
        cutout.addRoundedRect(spot, spotlightRadius, spotlightRadius);
        dimPath -= cutout;
    }
    painter.fillPath(dimPath, QColor(0, 0, 0, 180));

    if (!spot.isEmpty()) {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.drawRoundedRect(spot, spotlightRadius, spotlightRadius);
    }
}

void TUiTour::keyPressEvent(QKeyEvent* event)
{
    const bool leftToRight = QGuiApplication::isLeftToRight();
    const Qt::Key nextKey = leftToRight ? Qt::Key_Right : Qt::Key_Left;
    const Qt::Key backKey = leftToRight ? Qt::Key_Left : Qt::Key_Right;

    if (event->key() == Qt::Key_Escape) {
        event->accept();
        slot_finish();
    } else if (event->key() == backKey || event->key() == Qt::Key_PageUp) {
        event->accept();
        slot_back();
    } else if (event->key() == nextKey || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_PageDown) {
        event->accept();
        slot_next();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void TUiTour::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
    slot_next();
}

bool TUiTour::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parent() && (event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest)) {
        resizeToParent();
    }
    return QWidget::eventFilter(watched, event);
}

void TUiTour::resizeToParent()
{
    setGeometry(parentWidget()->rect());
    positionCard();
    update();
}

void TUiTour::slot_next()
{
    if (mCurrentStep >= static_cast<int>(mSteps.size()) - 1) {
        slot_finish();
        return;
    }
    setStep(mCurrentStep + 1, 1);
}

void TUiTour::slot_back()
{
    if (mCurrentStep > 0) {
        setStep(mCurrentStep - 1, -1);
    }
}

void TUiTour::slot_finish()
{
    close();
}
