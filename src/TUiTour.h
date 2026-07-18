#ifndef MUDLET_TUITOUR_H
#define MUDLET_TUITOUR_H

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

#include <QWidget>

#include <functional>
#include <vector>

class QFrame;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPaintEvent;
class QPushButton;

class mudlet;

// A one-time spotlight tour over the main window that points out the most
// important parts of the interface to first-time players
class TUiTour : public QWidget
{
    Q_OBJECT

public:
    explicit TUiTour(mudlet* pMainWindow, const bool skipIntroStep = false);

    static bool shouldShowOnFirstProfile();
    static void rememberShown();

    void start();

signals:
    // Emitted on user-driven dismissal only, not when the widget goes down
    // with the application - handlers touch state that no longer exists then
    void signal_tourFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void slot_next();
    void slot_back();
    void slot_finish();

private:
    struct TourStep
    {
        // Returns the area to spotlight in main window coordinates, empty when
        // the target is currently unavailable; a step without a resolver shows
        // a centered card over a fully dimmed window instead
        std::function<QRect()> spotlightResolver;
        QString title;
        QString body;
    };

    void buildSteps();
    void createCard();
    void setStep(int index, int direction);
    void updateCard();
    void positionCard();
    QRect spotlightRect() const;
    void resizeToParent();

    mudlet* mpMainWindow = nullptr;
    bool mSkipIntroStep = false;
    std::vector<TourStep> mSteps;
    int mCurrentStep = 0;

    QFrame* mpCard = nullptr;
    QLabel* mpTitleLabel = nullptr;
    QLabel* mpBodyLabel = nullptr;
    QLabel* mpProgressLabel = nullptr;
    QPushButton* mpSkipButton = nullptr;
    QPushButton* mpBackButton = nullptr;
    QPushButton* mpNextButton = nullptr;
};

#endif // MUDLET_TUITOUR_H
