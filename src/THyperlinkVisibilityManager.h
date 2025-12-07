/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#ifndef MUDLET_THYPERLINKVISIBILITYMANAGER_H
#define MUDLET_THYPERLINKVISIBILITYMANAGER_H

#include <QObject>
#include <QMap>
#include <QPoint>
#include <QTimer>
#include <QPointer>
#include <QString>

class TBuffer;
class TConsole;

// Forward declaration for visibility settings
namespace Mudlet {
struct HyperlinkStyling;
}

struct TrackedHyperlink {
    int linkId = 0;
    int lineNumber = 0;
    int startColumn = 0;
    int length = 0;
    QString originalText;
    qint64 creationTimeMs = 0;
    qint64 timerActivatedMs = 0;  // When the timer was activated (by click); 0 = not yet activated
    
    enum class Action {
        None,
        Conceal,
        Reveal
    };

    Action action = Action::None;
    quint32 delayMs = 0;
    bool onPrompt = false;
    bool deletesEntireLine = false;
    bool isConcealed = false;
};

class THyperlinkVisibilityManager : public QObject
{
    Q_OBJECT

public:
    explicit THyperlinkVisibilityManager(TConsole* pConsole, QObject* parent = nullptr);
    ~THyperlinkVisibilityManager() override;

    // Returns true if the link should start concealed (text should be replaced with spaces)
    bool registerHyperlink(int linkId, int lineNumber, int startColumn, int length,
                          const QString& originalText, const Mudlet::HyperlinkStyling& styling);
    void onLinkClicked(int linkId);
    void onCommandLineTextChanged();
    void onCommandLineSubmitted();
    void concealLink(int linkId);
    void revealLink(int linkId);
    void processPromptTriggeredLinks();
    bool isLinkConcealed(int linkId) const;
    void removeLinksOnLine(int lineNumber);
    void adjustLineNumbers(int deletedLineStart, int deletedLineCount);
    void clear();

signals:
    void visibilityChanged();

private slots:
    void slot_checkTimers();

private:
    void startTimerIfNeeded();
    void stopTimerIfNotNeeded();
    void performConcealment(TrackedHyperlink& link);
    void performReveal(TrackedHyperlink& link);

    QPointer<TConsole> mpConsole;
    QMap<int, TrackedHyperlink> mTrackedLinks;
    QTimer* mpTimer = nullptr;
    bool mHasTimerBasedLinks = false;
};

#endif // MUDLET_THYPERLINKVISIBILITYMANAGER_H
