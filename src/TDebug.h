#ifndef MUDLET_TDEBUG_H
#define MUDLET_TDEBUG_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2021-2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@gmail.com          *
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


#include <QColor>
#include <QCoreApplication>
#include <QList>
#include <QMap>
#include <QQueue>
#include <QSet>
#include <QString>

#include "utils.h"

class Host;

struct TDebugMessage
{
    TDebugMessage(const QString& text, const QString& profileTag, const QColor& foreground, const QColor& background, const QString& timeStamp = QString())
    : mMessage(text)
    , mProfileTag(profileTag)
    , mForeground(foreground)
    , mBackground(background)
    , mTimeStamp(timeStamp)
    {
    }

    QString mMessage;
    // The "[A] ".."[Z] " marking of the source profile, NOT the message's category:
    QString mProfileTag;
    QColor mForeground;
    QColor mBackground;
    // Arrival time, so a message held while paused isn't stamped with when it was shown:
    QString mTimeStamp;
};

class TDebug
{
    Q_DECLARE_TR_FUNCTIONS(TDebug)

public:
    // Lets the Central Debug Console drop the high-volume subsystems. Persisted in the settings, so
    // do not renumber:
    enum class Category : quint32 {
        System = 0x0001,        // profile started/ended, the identifier legend
        Error = 0x0002,         // compile and run-time errors, any subsystem
        Network = 0x0004,       // connection establishment, HTTP transfers
        Protocol = 0x0008,      // GMCP, MSDP, MSSP and MXP events
        GameLine = 0x0010,      // every line arriving from the game
        TriggerMatch = 0x0020,  // "Trigger name=... matched."
        TriggerDetail = 0x0040, // capture groups and multiline match state
        Alias = 0x0080,
        Item = 0x0100,       // housekeeping: compiled OK, expiry, fire counts
        LuaSuccess = 0x0200, // "... ran without errors in Nms"
        LuaWarning = 0x0400,
        Selection = 0x0800, // selectString(), selectSection() and friends
        Map = 0x1000,
        Other = 0x2000,
    };
    Q_DECLARE_FLAGS(Categories, Category)

    // Off by default, and what the filter bar's "Quiet" preset restores:
    static const Categories csmNoisyCategories;
    static const Categories csmAllCategories;

    // Master switch for every wants() test. It doesn't gate passesFilters(), and a few unguarded messages
    // (the profile-started line, changeHostName()'s identifier table) show regardless:
    inline static bool smDebugMode = false;

    // Where composed lines go: the GUI installs the Central Debug Console; with none, lines queue. TDebug
    // keeps a raw pointer and a closing profile emits during teardown, so an implementation must detach
    // before any of it is torn down:
    class Sink
    {
    public:
        // timeStamp is empty for a line shown as it arrives, and the arrival
        // time for one replayed after being held back:
        virtual void printDebugLine(const QString& text, const QColor& foreground, const QColor& background, const QString& timeStamp) = 0;

    protected:
        // Nothing owns a sink through this interface (the console belongs to its widget parent), so deleting
        // through it must not compile. Clearing the pointer is only a backstop: a console detaches earlier.
        ~Sink()
        {
            if (smpSink == this) {
                smpSink = nullptr;
            }
        }
    };

    static void setSink(Sink* pSink) { smpSink = pSink; }
    static Sink* sink() { return smpSink; }

    // Told when the profile identifiers change, so the GUI can keep what shows
    // them - the Central Debug Console's profile menu and the "[A] " prefixes
    // on the profile tabs - in step. With none installed nothing is told:
    class ProfileObserver
    {
    public:
        // A profile gained or lost its identifier, or was renamed:
        virtual void profilesChanged() = 0;
        virtual void profileRenamed(const QString& newName, const QString& tag) = 0;
        // Only raised in debug mode, and before the new profile has a tab:
        virtual void profileAddedInDebugMode() = 0;

    protected:
        ~ProfileObserver()
        {
            if (smpProfileObserver == this) {
                smpProfileObserver = nullptr;
            }
        }
    };

    static void setProfileObserver(ProfileObserver* pObserver) { smpProfileObserver = pObserver; }
    static ProfileObserver* profileObserver() { return smpProfileObserver; }

private:
    inline static Sink* smpSink = nullptr;
    inline static ProfileObserver* smpProfileObserver = nullptr;

    // A shared map that is uses to put a short identifier on each debug message
    // - the first value is used to create a table to display on changes and the
    // second value is the short identifier used:
    inline static QMap<const Host*, QPair<QString, QString>> smIdentifierMap;
    // Used to order identifier in the same application run:
    inline static QQueue<QString> smAvailableIdentifiers;
    inline static bool initialised = false;
    // This is a temporary bodge until we can decouple the Central Debug
    // Console from having to be associated with a Host (Profile) instance,
    // as that prevents it from being created until a profile has - which makes
    // displaying details from that first profile being loaded harder:
    inline static QQueue<TDebugMessage> smMessageQueue;

    // Used as a tag for system (non-profile) messages:
    // Changed to use U+2731 {HEAVY ASTERIX} instead of an asterix:
    inline static const QString csmTagSystemMessage = qsl("[\u2731] ");
    // If something has gone wrong and it is not possible to work out which
    // profile it is from, don't use a tag:
    inline static const QString csmTagFault = QString();
    // Used as a tag for messages on the 27th and above currently active
    // profiles:
    inline static const QString csmTagOverflow = qsl("[?] ");

    // Held while paused, capped at the console's own 10,000-line buffer - more would only be trimmed on replay:
    inline static QQueue<TDebugMessage> smPausedQueue;
    inline static int smPausedDroppedCount = 0;
    static constexpr int csmPausedQueueLimit = 10000;

    static Categories smEnabledCategories;
    inline static QSet<const Host*> smDisabledHosts;
    inline static QString smTextFilter;
    inline static Qt::CaseSensitivity smTextFilterCaseSensitivity = Qt::CaseInsensitive;
    // When set, only messages about this trigger/alias/timer/key/button/script
    // get through - plus system messages, so profile starts and ends still show:
    inline static QString smItemFilter;
    inline static bool smPaused = false;
    // Whether the last non-continuation message got through the filters, so
    // that its continuation fragments can follow it rather than being orphaned:
    inline static bool smLastMessagePassed = true;
    // A message whose head passed every filter but the text one, kept in case a
    // continuation fragment of the same message matches instead:
    inline static bool smHeadHeld = false;
    inline static QString smHeldHead;
    inline static QColor smHeldHeadForeground;
    inline static QColor smHeldHeadBackground;

    QString msg;
    QColor fgColor;
    QColor bgColor;
    Category mCategory;
    // The trigger/alias/timer/key/button/script this is about, or empty:
    QString mItemName;

public:
    // Category deliberately not defaulted, so a new call site can't silently become unfilterable:
    explicit TDebug(const QColor&, const QColor&, const Category, const QString& itemName = QString());
    ~TDebug() = default;

    static void addHost(Host*, const QString);    // Might need to NOLINT this to prevent a warning about not using a reference
    static void removeHost(Host*, const QString); // Might need to NOLINT this to prevent a warning about not using a reference
    static void changeHostName(const Host*, const QString&);
    static void flushMessageQueue();
    static QString getTag(Host*);

    // Use instead of a bare smDebugMode test, so a filtered-out message is never assembled:
    static bool wants(const Category);

    static Categories enabledCategories() { return smEnabledCategories; }
    static void setEnabledCategories(const Categories);
    static void setCategoryEnabled(const Category, const bool);
    static bool categoryEnabled(const Category category) { return smEnabledCategories.testFlag(category); }

    static void setHostEnabled(const Host*, const bool);
    static bool hostEnabled(const Host* pHost) { return !smDisabledHosts.contains(pHost); }
    static void enableAllHosts() { smDisabledHosts.clear(); }
    static QList<QPair<const Host*, QString>> activeProfiles();

    static void setTextFilter(const QString&, const Qt::CaseSensitivity);
    static QString textFilter() { return smTextFilter; }
    static Qt::CaseSensitivity textFilterCaseSensitivity() { return smTextFilterCaseSensitivity; }

    static void setItemFilter(const QString& itemName) { smItemFilter = itemName; }
    static QString itemFilter() { return smItemFilter; }

    // Says what is filtered out, so an empty-looking console isn't mistaken for a broken one:
    static void announceFilters();
    static int hiddenCategoryCount();

    static void setPaused(const bool);
    static bool paused() { return smPaused; }
    static int pausedMessageCount() { return smPausedQueue.count(); }
    static int pausedDroppedCount() { return smPausedDroppedCount; }
    static int pausedMessageLimit() { return csmPausedQueueLimit; }
    static void discardPausedMessages();

    // Used to flush/print out the accumulated message:
    TDebug& operator>>(Host*);

    // Used to append the argument type to the message:
    TDebug& operator<<(const QString&);
    TDebug& operator<<(const QChar&);
    TDebug& operator<<(const int&);
    // These should all be used with a preceding operator<<(const QString&)
    // that provides an opening '(' to match the one that these all append
    // after the content they add to the message:
    TDebug& operator<<(const QMap<QString, QString>&);
    TDebug& operator<<(const QMap<QString, int>&);
    TDebug& operator<<(const QMap<int, QString>&);
    TDebug& operator<<(const QMap<int, int>&);
    TDebug& operator<<(const QList<QString>&);
    TDebug& operator<<(const QList<int>&);

    // Prepend this to any continuation message to suppress the insertion of the
    // profile identifying marking.  This is a Unicode NON-character code which
    // is explicitly undisplayable but can be embedded for our own internal purposes:
    inline static const QChar csmContinue = QChar(0xFFFF);

private:
    TDebug() = default;

    static QString displayNewTable();
    static QString deduceProfileTag(QString&, Host*);
    bool passesFilters(const Host*);
    QString displayLine(Host*);
    static QString composeLine(const QString& profileTag, const QString& text);
    static void emitLine(const QString& line, const QColor& foreground, const QColor& background);
    static void drainPausedQueue();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TDebug::Categories)

#endif // MUDLET_TDEBUG_H
