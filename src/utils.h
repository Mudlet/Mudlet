#ifndef MUDLET_UTILS_H
#define MUDLET_UTILS_H

/***************************************************************************
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@hey.com            *
 *   Copyright (C) 2021, 2023, 2025-2026 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
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

#include <QApplication>
#include <QCryptographicHash>
#include <QEnterEvent>
#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <QScreen>
#include <QWidget>

#include <cstdint>
#include <cstring>

#define qsl(s) QStringLiteral(s)

// user-defined literals to represent kilobytes and megabytes
// C++ standard requires unsigned long long parameter for integer literal operators
constexpr auto operator""_KB(unsigned long long const x) -> int64_t // NOLINT(runtime/int)
{
    return 1024LL * x;
}

constexpr auto operator""_MB(unsigned long long const x) -> int64_t // NOLINT(runtime/int)
{
    return 1024LL * 1024LL * x;
}

using TEnterEvent = QEnterEvent;

using NameGroupMatches = QVector<QPair<QString, QString>>;

// Common enum for specifying insertion mode for tree items
// Used across all editor item types (triggers, aliases, timers, scripts, actions, keys)
enum class TreeItemInsertMode {
    Append,      // Add to end of parent's child list
    AtPosition   // Insert at specific position
};

class utils
{
public:
    // Safe string copy: copies up to destSize-1 bytes and always null-terminates.
    // Returns the number of bytes copied (excluding null terminator).
    static size_t copyString(char* dest, size_t destSize, const char* src, size_t srcLen)
    {
        if (destSize == 0) {
            return 0;
        }
        const size_t copyLen = (srcLen < destSize) ? srcLen : destSize - 1;
        std::memcpy(dest, src, copyLen);
        dest[copyLen] = '\0';
        return copyLen;
    }

    // As copyString(), but for UTF-8 data that has to stay valid UTF-8: the copy
    // stops at the last character that fits whole rather than at the last byte,
    // so no trailing half-character is left behind. Use it wherever a truncated
    // copy is handed on to something that decodes it - Discord discards an
    // entire presence frame whose JSON payload carries an incomplete sequence.
    // Returns the number of bytes copied (excluding the null terminator).
    static size_t copyUtf8String(char* dest, size_t destSize, const char* src, size_t srcLen)
    {
        if (destSize == 0) {
            return 0;
        }
        size_t copyLen = (srcLen < destSize) ? srcLen : destSize - 1;
        // Every byte after the first of a multi-byte character has the form
        // 10xxxxxx, so a cut in front of one is a cut inside a character: walk
        // back to where that character starts. A cut that took everything (or
        // that landed on a character start) needs no adjustment.
        while (copyLen > 0 && copyLen < srcLen && (static_cast<unsigned char>(src[copyLen]) & 0xC0u) == 0x80u) {
            --copyLen;
        }
        std::memcpy(dest, src, copyLen);
        dest[copyLen] = '\0';
        return copyLen;
    }

    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // qsl all over the place:
    static QString richText(const QString& text) { return qsl("<p>%1</p>").arg(text); }

    // Call this in the destructor of a window class that connects any of its
    // own widgets to its own slots - keep it first, so that nothing else the
    // destructor does can deliver a child's signal either.
    //
    // A visible window is taken off the screen while the base-class
    // destructors unwind: ~QDialog hides it explicitly, and any other window
    // class gets closed by ~QWidget. That moves the keyboard focus away from
    // whichever child widget holds it, and an editing widget reacts to the
    // focus-out by emitting - QLineEdit (once its text has been touched, which
    // includes any setText()), QAbstractSpinBox and QKeySequenceEdit all emit
    // editingFinished() there. Qt then tries to deliver that to a slot of a
    // window whose derived part has already been destroyed, which aborts with
    // "Called object is not of the correct type (class destructor may have
    // already run)" (#9574). In a release build the assert is compiled out and
    // the slot runs against destroyed members instead.
    //
    // A window that is being destroyed cannot do anything useful with a
    // signal from its own widgets, so every one of them is severed rather
    // than just the widget types that emit during teardown today. Note that
    // this only reaches connections whose receiver is the window: a
    // connect(child, &Signal, [this]{...}) written without a context object
    // survives it and brings the crash back, so always pass the context:
    static void disconnectChildSignals(QWidget* window)
    {
        for (QObject* child : window->findChildren<QObject*>()) {
            QObject::disconnect(child, nullptr, window, nullptr);
        }
    }

    // Qt 6.9 deprecated QDateTime::setOffsetFromUtc(int) and made it hard to
    // replicate the exact strings that we had before:
    static QString dateStamp() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        if (offset) {
            unsigned hoursOff = abs(offset/3600);
            unsigned minutesOff = (abs(offset) - hoursOff * 3600) / 60;
            return localNow.toString(Qt::ISODate).append(qsl("%1%2:%3")
                                                                 .arg(offset >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                                                                 .arg(hoursOff, 2, 10, QLatin1Char('0'))
                                                                 .arg(minutesOff, 2, 10, QLatin1Char('0')));
        }
        return localNow.toString(Qt::ISODate).append(qsl("+00:00"));
#else
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        localNow.setOffsetFromUtc(offset);
        return localNow.toString(Qt::ISODate);
#endif
    }

    // Return a new QString with path made absolute, resolved against base and cleaned if it was relative
    // Returns path unchanged if it was already absolute or an empty string
    static QString pathResolveRelative(const QString& path, const QString& base)
    {
        if (path.isEmpty()) {
            return path;
        }
        if (QDir::isAbsolutePath(path)) {
            return path;
        }
        return QDir::cleanPath(base + "/" + path);
    }

    struct ConfigDirResolution
    {
        QString path;
        // XDG_CONFIG_HOME is set, but an existing legacy dir was used anyway, so
        // the caller can hint at the migration
        bool migrationPending = false;
        // legacyDefault, when it holds profiles that the chosen dir now hides. The
        // caller has to name it, or those profiles read as gone.
        QString shadowedProfilesPath;
    };

    // How strongly a directory claims to be Mudlet's config root; the stronger
    // claim wins in xdgConfigDir(), so the order is the contract.
    enum class ConfigDirClaim {
        absent = 0,
        // Exists, but holds nothing Mudlet put there - including the stale
        // Mudlet.conf pre-4.19 Mudlet left in $XDG_CONFIG_HOME/mudlet while its
        // profiles stayed in ~/.config/mudlet
        unclaimed = 1,
        settings = 2,
        profiles = 3,
    };

    // A directory that cannot be listed must never read as "nothing here": that
    // inference is what hides profiles, so assume the strongest content instead.
    static bool configDirHoldsProfiles(const QString& dir)
    {
        if (!QDir(dir).exists()) {
            return false;
        }
        if (!QFileInfo(dir).isReadable()) {
            return true;
        }
        const QDir profiles(qsl("%1/profiles").arg(dir));
        if (!profiles.exists()) {
            return false;
        }
        // Counted as mudlet.cpp's anyProfilesExist() does, so the two cannot disagree
        return !QFileInfo(profiles.path()).isReadable() || !profiles.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
    }

    static ConfigDirClaim configDirClaim(const QString& dir)
    {
        if (!QDir(dir).exists()) {
            return ConfigDirClaim::absent;
        }
        if (configDirHoldsProfiles(dir)) {
            return ConfigDirClaim::profiles;
        }
        if (QFileInfo::exists(qsl("%1/Mudlet.ini").arg(dir))) {
            return ConfigDirClaim::settings;
        }
        return ConfigDirClaim::unclaimed;
    }

    // $XDG_CONFIG_HOME/mudlet claims more than it holds, because creating
    // profiles/ there is the deliberate opt-in into an isolated config root. The
    // legacy dir gets no such credit: an empty profiles/ left behind by deleting
    // the last profile would otherwise outrank a config root in active use.
    static ConfigDirClaim xdgConfigDirClaim(const QString& dir)
    {
        if (QDir(qsl("%1/profiles").arg(dir)).exists()) {
            return ConfigDirClaim::profiles;
        }
        return configDirClaim(dir);
    }

    // cleanPath() is not enough: a symlinked ~/.config gives one directory two
    // spellings, and dotfile managers produce exactly that
    static QString configDirIdentity(const QString& dir)
    {
        const QString canonical = QFileInfo(dir).canonicalFilePath();
        return canonical.isEmpty() ? QDir::cleanPath(dir) : canonical;
    }

    // Resolve Mudlet's config root honoring XDG_CONFIG_HOME; the caller handles
    // portable.txt first, which still wins. $XDG_CONFIG_HOME/mudlet takes a tie so
    // that a fresh install lands there.
    static ConfigDirResolution xdgConfigDir(const QString& legacyDefault)
    {
        const QString xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME");
        // The XDG base-dir spec requires an absolute path; a relative (or empty)
        // value must be ignored, which also avoids a surprising CWD-relative root.
        if (xdgConfigHome.isEmpty() || !QDir::isAbsolutePath(xdgConfigHome)) {
            return {legacyDefault, false, QString()};
        }
        const QString xdgTarget = QDir::cleanPath(qsl("%1/mudlet").arg(xdgConfigHome));
        if (xdgConfigDirClaim(xdgTarget) < configDirClaim(legacyDefault)) {
            return {legacyDefault, true, QString()};
        }
        // XDG_CONFIG_HOME=$HOME/.config makes both candidates one directory
        const bool shadowing = configDirIdentity(legacyDefault) != configDirIdentity(xdgTarget) && configDirHoldsProfiles(legacyDefault);
        return {xdgTarget, false, shadowing ? legacyDefault : QString()};
    }

    inline static const auto scmfileSystemUnsafeChars = QRegularExpression(qsl(R"REGEX([/\\:*?"<>|])REGEX"));
    static constexpr int scmMaxPathComponentLength = 50;
    static constexpr int scmPathComponentDigestLength = 16;

    // Sanitize a string for safe use as filename/path component
    // Replaces filesystem-unsafe characters with underscores and limits length.
    // Callers file data under the result, so shortening has to keep distinct
    // inputs distinct: a shortened name carries a digest of the whole input,
    // since plain truncation made two long profile names share - and overwrite -
    // one stored password.
    static QString sanitizeForPath(const QString& input)
    {
        QString sanitized = input;
        // Replace filesystem-unsafe characters with underscores
        sanitized.replace(scmfileSystemUnsafeChars, qsl("_"));
        // Limit length to prevent filesystem issues
        if (sanitized.length() > scmMaxPathComponentLength) {
            const QString digest = QString::fromLatin1(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256).toHex()).left(scmPathComponentDigestLength);
            sanitized = qsl("%1-%2").arg(sanitized.left(scmMaxPathComponentLength - scmPathComponentDigestLength - 1), digest);
        }
        return sanitized;
    }

    // Position a dialog on the same screen as its parent window
    // This improves multi-monitor UX by keeping dialogs with their parent windows
    static void positionDialogOnParentScreen(QWidget* dialog, QWidget* parent)
    {
        if (!dialog || !parent) {
            return;
        }

        // Get the screen containing the parent window
        // Use mapToGlobal to get the actual screen position of the parent widget
        QPoint parentPos = parent->mapToGlobal(parent->rect().center());
        const QScreen* parentScreen = QApplication::screenAt(parentPos);
        if (!parentScreen) {
            // Fallback to parent's screen property if screenAt fails
            parentScreen = parent->screen();
        }

        if (parentScreen) {
            // Get the current screen of the dialog to see if it needs repositioning
            // Use the dialog's current geometry center for more accurate screen detection
            QPoint dialogCenter = dialog->mapToGlobal(dialog->rect().center());
            const QScreen* dialogScreen = QApplication::screenAt(dialogCenter);

            // A hidden dialog reports no screen of its own, so this recentres one that is
            // merely closed even when it holds a position worth keeping - not for windows
            // whose position is remembered
            if (!dialog->isVisible() || !dialogScreen || dialogScreen != parentScreen) {
                centerDialogOnScreen(dialog, parentScreen);
            }
        }
    }

    // Position a dialog on the same screen as the active profile's console
    // This version considers the actual console widget position for better accuracy
    static void positionDialogOnActiveProfileScreen(QWidget* dialog, QWidget* parentWindow, QWidget* activeConsole)
    {
        if (!dialog) {
            return;
        }

        // Prefer the active console position if available, otherwise fall back to parent window
        QWidget* referenceWidget = activeConsole ? activeConsole : parentWindow;
        if (referenceWidget) {
            positionDialogOnParentScreen(dialog, referenceWidget);
        }
    }

    // Centre a dialog on the parent's screen, discarding wherever it currently sits.
    // Only for dialogs whose position is not persisted: a window that restores a
    // remembered position lands back in the middle of the screen on every re-show
    static void forceRepositionDialogOnParentScreen(QWidget* dialog, QWidget* parent)
    {
        if (!dialog || !parent) {
            return;
        }

        // Get the screen containing the parent window
        QPoint parentPos = parent->mapToGlobal(parent->rect().center());
        const QScreen* parentScreen = QApplication::screenAt(parentPos);
        if (!parentScreen) {
            parentScreen = parent->screen();
        }

        if (parentScreen) {
            // Always reposition, regardless of current dialog position
            centerDialogOnScreen(dialog, parentScreen);
        }
    }

    // Centre a dialog whose remembered position has since gone out of reach -
    // the monitor it was left on can have been disconnected. Only the top of
    // the frame is asked about: a window hanging off the top edge with nothing
    // but its lower half showing has no title bar left to drag it back by
    static void keepDialogOnAScreen(QWidget* dialog, QWidget* parent)
    {
        if (!dialog) {
            return;
        }

        constexpr int graspableTitleBarHeight = 30;
        const QRect dialogRect = dialog->frameGeometry();
        const QRect titleBar(dialogRect.x(), dialogRect.y(), dialogRect.width(), qMin(graspableTitleBarHeight, dialogRect.height()));
        const QList<QScreen*> screens = QApplication::screens();
        for (const QScreen* screen : screens) {
            if (screen->availableGeometry().intersects(titleBar)) {
                return;
            }
        }

        const QScreen* fallbackScreen = parent ? parent->screen() : QApplication::primaryScreen();
        centerDialogOnScreen(dialog, fallbackScreen);
    }

    // Position a dialog in the center of the specified screen
    static void centerDialogOnScreen(QWidget* dialog, const QScreen* screen)
    {
        if (!dialog || !screen) {
            return;
        }

        const QRect screenGeometry = screen->availableGeometry();

        // Ensure dialog has a size first
        if (dialog->size().isEmpty()) {
            dialog->adjustSize();
        }

        // Calculate center position
        const QSize dialogSize = dialog->size();
        const QPoint centerPoint = screenGeometry.center();
        const QPoint newPos(
            centerPoint.x() - dialogSize.width() / 2,
            centerPoint.y() - dialogSize.height() / 2);

        // Ensure dialog stays within screen bounds
        QPoint constrainedPos = newPos;
        constrainedPos.setX(qMax(screenGeometry.left(),
                                qMin(newPos.x(), screenGeometry.right() - dialogSize.width())));
        constrainedPos.setY(qMax(screenGeometry.top(),
                                qMin(newPos.y(), screenGeometry.bottom() - dialogSize.height())));

        dialog->move(constrainedPos);
    }
};

#endif // UPDATER_H
