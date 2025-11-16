/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020, 2023 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "TLabel.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "mudlet.h"

#include <QDesktopServices>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTimer>
#include <QUrl>
#include <QtEvents>


TLabel::TLabel(Host* pH, const QString& name, QWidget* pW)
: QLabel(pW)
, mpHost(pH)
, mName(name)
{
    setMouseTracking(true);
    setObjectName(qsl("label_%1_%2").arg(pH->getName(), mName));

    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    setOpenExternalLinks(false);

    connect(this, &QLabel::linkActivated, this, &TLabel::slot_linkActivated);
}

TLabel::~TLabel()
{
    if (mpMovie) {
        mpMovie->deleteLater();
        mpMovie = nullptr;
    }
}

void TLabel::setText(const QString& text)
{
    // If we have link styling configured and the text contains HTML links,
    // we need to inject inline styles because QTextDocument doesn't use
    // widget stylesheets or QPalette for link colors when a stylesheet exists
    if ((!mLinkColor.isEmpty() || !mLinkVisitedColor.isEmpty()) && text.contains(qsl("<a "))) {
        QString styledText = text;

        // Replace all <a href="..."> tags with <a href="..." style="...">
        // Note: This regex is intentionally strict (lowercase, href first, no spacing around =)
        // because Mudlet's HTML generation (via echo(), setLabelText(), etc.) consistently
        // uses this format. User-provided HTML outside this pattern will still render as
        // clickable links (Qt handles that), but won't receive custom styling.
        QRegularExpression anchorRegex(qsl("<a\\s+href=([\"'][^\"']*[\"'])([^>]*)>"));
        QRegularExpressionMatchIterator it = anchorRegex.globalMatch(styledText);

        // Process matches in reverse order to avoid offset issues
        QList<QRegularExpressionMatch> matches;
        while (it.hasNext()) {
            matches.prepend(it.next());
        }

        for (const auto& match : matches) {
            QString fullMatch = match.captured(0);
            QString hrefPart = match.captured(1);  // The href="..." part
            QString otherAttrs = match.captured(2); // Other attributes

            // Extract the actual URL from hrefPart (remove quotes)
            QString url = hrefPart;
            url.remove(0, 1); // Remove opening quote
            url.chop(1);      // Remove closing quote

            bool isVisited = mVisitedLinks.contains(url);

            QString linkStyle;
            if (isVisited && !mLinkVisitedColor.isEmpty()) {
                linkStyle += qsl("color: %1; ").arg(mLinkVisitedColor);
            } else if (!mLinkColor.isEmpty()) {
                linkStyle += qsl("color: %1; ").arg(mLinkColor);
            }

            if (!mLinkUnderline) {
                linkStyle += qsl("text-decoration: none; ");
            }

            if (!linkStyle.isEmpty()) {
                linkStyle = linkStyle.trimmed();

                QString replacement;
                if (otherAttrs.contains(qsl("style="))) {
                    // Already has a style attribute - merge our styles
                    // This is complex, so for now just prepend our styles
                    replacement = qsl("<a href=%1 style=\"%2\"").arg(hrefPart, linkStyle);
                    // Intentionally overwrites any existing style attribute rather than merging
                    // to keep implementation simple for the common case (labels without pre-existing inline styles)
                    otherAttrs.remove(QRegularExpression(qsl("style=([\"'][^\"']*[\"'])")));
                    replacement += otherAttrs + qsl(">");
                } else {
                    replacement = qsl("<a href=%1 style=\"%2\"%3>").arg(hrefPart, linkStyle, otherAttrs);
                }

                styledText.replace(match.capturedStart(), match.capturedLength(), replacement);
            }
        }

        QLabel::setText(styledText);
    } else {
        QLabel::setText(text);
    }
}

void TLabel::setClick(const int func)
{
    releaseFunc(mClickFunction, func);
    mClickFunction = func;
}

void TLabel::setDoubleClick(const int func)
{
    releaseFunc(mDoubleClickFunction, func);
    mDoubleClickFunction = func;
}

void TLabel::setRelease(const int func)
{
    releaseFunc(mReleaseFunction, func);
    mReleaseFunction = func;
}

void TLabel::setMove(const int func)
{
    releaseFunc(mMoveFunction, func);
    mMoveFunction = func;
}

void TLabel::setWheel(const int func)
{
    releaseFunc(mWheelFunction, func);
    mWheelFunction = func;
}

void TLabel::setEnter(const int func)
{
    releaseFunc(mEnterFunction, func);
    mEnterFunction = func;
}

void TLabel::setLeave(const int func)
{
    releaseFunc(mLeaveFunction, func);
    mLeaveFunction = func;
}

void TLabel::mousePressEvent(QMouseEvent* event)
{
    // If the label has rich text with potential hyperlinks, let QLabel handle the event first
    // QLabel will emit linkActivated if a link was clicked
    if (!text().isEmpty() && textFormat() == Qt::RichText && text().contains(qsl("<a "))) {
        QLabel::mousePressEvent(event);
        // If QLabel didn't accept the event, then it wasn't a link click
        if (event->isAccepted()) {
            return;
        }
    }

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        // The use of accept() here prevents the propagation of the event to
        // any parent, e.g. the containing TConsole
        event->accept();
        mudlet::self()->activateProfile(mpHost);
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (mpHost && mDoubleClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mDoubleClickFunction, event);
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
    // If the label has rich text with potential hyperlinks, let QLabel handle the event first
    if (!text().isEmpty() && textFormat() == Qt::RichText && text().contains(qsl("<a "))) {
        QLabel::mouseReleaseEvent(event);
        // If QLabel accepted the event, it was handling a link click
        if (event->isAccepted()) {
            return;
        }
    }

    auto labelParent = qobject_cast<TConsole*>(parent());
    if (labelParent && labelParent->mpDockWidget && labelParent->mpDockWidget->isFloating()) {
        // move focus back to the active console / command line:
        mudlet::self()->activateProfile(mpHost);
    }

    if (mpHost && mReleaseFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mReleaseFunction, event);
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void TLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (mpHost && mMoveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mMoveFunction, event);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void TLabel::wheelEvent(QWheelEvent* event)
{

    if (mpHost && mWheelFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mWheelFunction, event);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    if (mpHost && mLeaveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mLeaveFunction, event);
        event->accept();
    } else {
        QWidget::leaveEvent(event);
    }
}

void TLabel::enterEvent(TEnterEvent* event)
{
    if (mpHost && mEnterFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mEnterFunction, event);
        event->accept();
    } else {
        QWidget::enterEvent(event);
    }
}

void TLabel::resizeEvent(QResizeEvent* event)
{
    emit resized();
    QWidget::resizeEvent(event);
}


// This function deferences previous functions in the Lua registry.
// This allows the functions to be safely overwritten.
void TLabel::releaseFunc(const int existingFunction, const int newFunction)
{
    if (newFunction != existingFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(existingFunction);
    }
}

void TLabel::setClickThrough(bool clickthrough)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, clickthrough);

    // If clickthrough is enabled, text interaction (including hyperlinks) won't work
    // So we need to disable text interaction when clickthrough is on
    if (clickthrough) {
        setTextInteractionFlags(Qt::NoTextInteraction);
    } else {
        // Re-enable text interaction for clickable hyperlinks
        setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
}

void TLabel::setLinkStyle(const QString& linkColor, const QString& linkVisitedColor, bool underline)
{
    mLinkColor = linkColor;
    mLinkVisitedColor = linkVisitedColor;
    mLinkUnderline = underline;

    // Set QPalette as a fallback (works if no stylesheet is set on the widget)
    QPalette palette = this->palette();

    if (!linkColor.isEmpty()) {
        QColor color(linkColor);
        palette.setColor(QPalette::Active, QPalette::Link, color);
        palette.setColor(QPalette::Inactive, QPalette::Link, color);
    }

    if (!linkVisitedColor.isEmpty()) {
        QColor color(linkVisitedColor);
        palette.setColor(QPalette::Active, QPalette::LinkVisited, color);
        palette.setColor(QPalette::Inactive, QPalette::LinkVisited, color);
    }

    setPalette(palette);

    // Note: Widget stylesheets don't affect QTextDocument rendering
    // Link colors are applied via inline styles in setText()

    // Force update to re-render with new styles
    update();
}

void TLabel::resetLinkStyle()
{
    setPalette(QPalette());

    mLinkColor.clear();
    mLinkVisitedColor.clear();
    mLinkUnderline = true;

    // Force update to re-render with new styles
    update();
}

void TLabel::clearVisitedLinks()
{
    mVisitedLinks.clear();

    QString currentText = text();
    if (!currentText.isEmpty() && currentText.contains(qsl("<a "))) {
        setText(currentText);
    }
}

void TLabel::slot_linkActivated(const QString& link)
{
    if (!mpHost) {
        return;
    }

    if (!mLinkVisitedColor.isEmpty()) {
        mVisitedLinks.insert(link);

        // Refresh the label to update link colors
        // We need to re-apply the current text to trigger the styling update
        QString currentText = text();
        if (!currentText.isEmpty() && currentText.contains(qsl("<a "))) {
            setText(currentText);
        }
    }

    // Check for custom schemes by looking for the colon separator
    const int colonPos = link.indexOf(':');

    if (colonPos > 0) {
        const QString scheme = link.left(colonPos).toLower(); // RFC 3986: schemes are case-insensitive
        const QString payload = link.mid(colonPos + 1); // Everything after the colon

        // Handle custom Mudlet URL schemes for Lua commands
        if (scheme == qsl("send")) {
            // send: scheme - send the command to the MUD immediately
            mpHost->send(payload);
            return;
        }

        if (scheme == qsl("prompt")) {
            // prompt: scheme - put text in command line and wait for user to press enter
            if (mpHost->mpConsole && mpHost->mpConsole->mpCommandLine) {
                QPointer<TCommandLine> commandLine = mpHost->mpConsole->mpCommandLine;
                commandLine->setPlainText(payload);
                QTextCursor cursor = commandLine->textCursor();
                cursor.movePosition(QTextCursor::End);
                commandLine->setTextCursor(cursor);
                // Defer the focus operation to avoid issues with QPointer manipulation
                // during the signal handler execution
                QTimer::singleShot(0, commandLine.data(), [commandLine]() {
                    if (commandLine) {
                        commandLine->setFocus();
                    }
                });
            }
            return;
        }

        if (scheme == qsl("http") || scheme == qsl("https")) {
            QDesktopServices::openUrl(QUrl(link));
            return;
        }

        // Unknown scheme - ignore safely to prevent unintended Lua execution
        // Only links without a scheme should be treated as Lua code
        return;
    }

    // No scheme - treat as Lua code to execute
    mpHost->mLuaInterpreter.compileAndExecuteScript(link);
}
