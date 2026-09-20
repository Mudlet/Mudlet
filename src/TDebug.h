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
    // The "[A] ".."[Z] " marking identifying which profile the message came
    // from, NOT the message's category:
    QString mProfileTag;
    QColor mForeground;
    QColor mBackground;
    // When the message arrived, so that one held back while the console is
    // paused is not stamped with the time it was eventually shown:
    QString mTimeStamp;
};

class TDebug
{
    Q_DECLARE_TR_FUNCTIONS(TDebug)

public:
    // Identifies the subsystem a message came from so that the Central Debug
    // Console can drop the high volume ones without losing the rest. The values
    // are persisted in the settings, so do not renumber them:
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

    // The categories that make the console unreadable on a busy profile - off
    // by default, and what the "Quiet" preset in the filter bar restores:
    static const Categories csmNoisyCategories;
    static const Categories csmAllCategories;

    // The master switch that every wants() test sits under. It does not gate
    // passesFilters(), and a handful of messages - the profile-started line and
    // the identifier table in changeHostName() - are emitted without a wants()
    // guard, so they appear whatever this is set to:
    inline static bool smDebugMode = false;

    // Where the composed lines end up. The GUI installs the Central Debug
    // Console; with nothing installed, lines queue as they do before that
    // console exists. TDebug keeps a raw pointer to the sink and a closing
    // profile emits lines from inside teardown, so an implementation has to
    // detach itself before any of it is torn down:
    class Sink
    {
    public:
        // timeStamp is empty for a line shown as it arrives, and the arrival
        // time for one replayed after being held back:
        virtual void printDebugLine(const QString& text, const QColor& foreground, const QColor& background, const QString& timeStamp) = 0;

    protected:
        // Nothing owns a sink through this interface - the Central Debug
        // Console belongs to its widget parent - so deleting through it is a
        // compile error. Clearing the pointer here is only a backstop: a
        // console emits from inside its own teardown, so it detaches earlier.
        ~Sink()
        {
            if (smpSink == this) {
                smpSink = nullptr;
            }
        }
    };

    static void setSink(Sink* pSink) { smpSink = pSink; }
    static Sink* sink() { return smpSink; }

private:
    inline static Sink* smpSink = nullptr;

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

    // Messages held back while the user has the console paused. Bounded because
    // the console's own buffer only holds 10,000 lines - anything beyond that
    // could only be replayed straight into the trimmer:
    inline static QQueue<TDebugMessage> smPausedQueue;
    inline static int smPausedDroppedCount = 0;
    static constexpr int csmPausedQueueLimit = 10000;

    // Filter state, all shared by the single Central Debug Console:
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
    // The trigger, alias, timer, key, button or script this message is about,
    // empty when it is not about one in particular:
    QString mItemName;

public:
    // The category is deliberately not defaulted, so that a new call site
    // cannot silently become unfilterable. The item name is optional because
    // plenty of messages genuinely do not belong to one:
    explicit TDebug(const QColor&, const QColor&, const Category, const QString& itemName = QString());
    ~TDebug() = default;

    static void addHost(Host*, const QString);    // Might need to NOLINT this to prevent a warning about not using a reference
    static void removeHost(Host*, const QString); // Might need to NOLINT this to prevent a warning about not using a reference
    static void changeHostName(const Host*, const QString&);
    static void flushMessageQueue();
    static QString getTag(Host*);

    // Cheap enough to use in place of a bare 'smDebugMode' test, so
    // that the message is never even assembled when it would be filtered out:
    static bool wants(const Category);

    static Categories enabledCategories() { return smEnabledCategories; }
    static void setEnabledCategories(const Categories);
    static void setCategoryEnabled(const Category, const bool);
    static bool categoryEnabled(const Category category) { return smEnabledCategories.testFlag(category); }

    static void setHostEnabled(const Host*, const bool);
    static bool hostEnabled(const Host* pHost) { return !smDisabledHosts.contains(pHost); }
    static void enableAllHosts() { smDisabledHosts.clear(); }
    // Profile identifier ("[A] ") and name for each currently active profile,
    // for the filter bar's profile menu:
    static QList<QPair<const Host*, QString>> activeProfiles();

    static void setTextFilter(const QString&, const Qt::CaseSensitivity);
    static QString textFilter() { return smTextFilter; }
    static Qt::CaseSensitivity textFilterCaseSensitivity() { return smTextFilterCaseSensitivity; }

    static void setItemFilter(const QString& itemName) { smItemFilter = itemName; }
    static QString itemFilter() { return smItemFilter; }

    // Says up front how much is being held back, so an empty-looking console is
    // never mistaken for a broken one:
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
