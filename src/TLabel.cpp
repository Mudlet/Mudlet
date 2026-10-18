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
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "TTextEdit.h"
#include "mudlet.h"
#include "TMainConsole.h"

#include <QAccessible>
#include <QApplication>
#include <QDesktopServices>
#include <QPainter>
#include <QSvgRenderer>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QUrl>
#include <QtEvents>
#include <chrono>
#include <utility>

using namespace std::chrono_literals;

namespace {
void notifyAccessibility(QWidget* widget, QAccessible::Event type)
{
    // updateAccessibility() does nothing while no screen reader has made accessibility active
    QAccessibleEvent event(widget, type);
    QAccessible::updateAccessibility(&event);
}
} // namespace

TLabel::TLabel(Host* pH, const QString& name, QWidget* pW)
: QLabel(pW)
, mpModel(std::make_unique<TLabelModel>(pH, name))
, mpHost(mpModel->mpHost)
, mName(mpModel->mName)
, mClickFunction(mpModel->mClickFunction)
, mDoubleClickFunction(mpModel->mDoubleClickFunction)
, mReleaseFunction(mpModel->mReleaseFunction)
, mMoveFunction(mpModel->mMoveFunction)
, mWheelFunction(mpModel->mWheelFunction)
, mEnterFunction(mpModel->mEnterFunction)
, mLeaveFunction(mpModel->mLeaveFunction)
, mSvgTintColor(mpModel->mSvgTintColor)
, mSvgRotation(mpModel->mSvgRotation)
, mSvgShearX(mpModel->mSvgShearX)
, mSvgShearY(mpModel->mSvgShearY)
, mLinkColor(mpModel->mLinkColor)
, mLinkVisitedColor(mpModel->mLinkVisitedColor)
, mLinkUnderline(mpModel->mLinkUnderline)
, mVisitedLinks(mpModel->mVisitedLinks)
, mBackgroundColor(mpModel->mBackgroundColor)
{
    setMouseTracking(true);
    setObjectName(qsl("label_%1_%2").arg(pH->getName(), mName));

    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setOpenExternalLinks(false);
    mpModel->mAccessibilityChanged = [this](TLabelModel::AccessibilityChange change) {
        accessibilityChanged(change);
    };
    updateKeyboardAccess();

    connect(this, &QLabel::linkActivated, this, &TLabel::slot_linkActivated);

    // QLabel's constructor already took the parent's font, before changeEvent() could see it
    mpModel->mFont = font();
}

TLabel::~TLabel()
{
    // Dying with its container, or in a reset, it takes the focus with it and leaves the keyboard with nobody
    if (hasFocus()) {
        if (QPointer<TMainConsole> pConsole = mainWindowConsole()) {
            QTimer::singleShot(0, pConsole, [pConsole]() {
                if (!QApplication::focusWidget() && QApplication::activeWindow() == pConsole->window()) {
                    pConsole->leaveKeyboardButtons(true);
                }
            });
        }
    }
    // Backstop: TMainConsole deregisters labels it destroys, but one can also die with its parent console.
    if (mpHost) {
        mpHost->windowRegistry().deregisterLabel(mName, mpModel.get());
        // The tracker holds a raw pointer and dereferences every entry when reporting.
        if (mpMovie) {
            mpHost->getGifTracker()->unregisterGif(mpMovie);
        }
    }

    if (mpMovie) {
        mpMovie->deleteLater();
        mpMovie = nullptr;
    }
}

void TLabel::setText(const QString& text)
{
    showLinkStyledText(mpModel->linkStyledText(text));
}

void TLabel::showLinkStyledText(const QString& text)
{
    // a click-through label passes the mouse on, so its links stay out of reach of the keyboard too
    const bool linksReachable = TLabelModel::containsAnchorTag(text) && !testAttribute(Qt::WA_TransparentForMouseEvents);

    setTextInteractionFlags(linksReachable ? scmLinkInteraction : Qt::TextInteractionFlags(Qt::NoTextInteraction));
    updateKeyboardAccess();

    stopMovie();
    QLabel::setText(text);
    syncTextAnnouncedByQt();
    // Text over an SVG hides it, and new text replaces a raster or movie
    imageMayHaveChanged();
}

void TLabel::press()
{
    const QPointF centre = QRectF(rect()).center();
    QMouseEvent pressEvent(QEvent::MouseButtonPress, centre, mapToGlobal(centre), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    // The click callback may delete the label: deleteLabel() deregisters it at once but destroys it later
    QPointer<TLabel> self(this);
    mScreenReaderPress = true;
    mousePressEvent(&pressEvent);
    if (!self) {
        return;
    }
    if (mpHost && mpHost->windowRegistry().labelModel(mName) == mpModel.get()) {
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, centre, mapToGlobal(centre), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&releaseEvent);
    }
    mScreenReaderPress = false;
}

bool TLabel::keyboardOperable() const
{
    return mpHost && mpHost->mAccessibleLabels && mpModel->actsAsButton();
}

bool TLabel::keyboardReachable() const
{
    return keyboardOperable() || (mpHost && mpHost->mAccessibleLabels && textInteractionFlags().testFlag(Qt::LinksAccessibleByKeyboard));
}

TMainConsole* TLabel::mainWindowConsole() const
{
    TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr;
    return pConsole && window() == pConsole->window() ? pConsole : nullptr;
}

// Past the last link QLabel hands Tab to Qt's own chain, so the label has to know beforehand whether there is
// another. This finds links as QWidgetTextControl::findNextPrevAnchor() does, block by block.
bool TLabel::linkBeyondFocused(bool next) const
{
    if (!textInteractionFlags().testFlag(Qt::LinksAccessibleByKeyboard)) {
        return false;
    }
    const int focused = selectionStart();
    QTextDocument document;
    document.setHtml(text());
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        QString previousHref;
        bool previousWasLink = false;
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            const bool link = format.isAnchor() && format.hasProperty(QTextFormat::AnchorHref);
            // A link with formatting inside it is several fragments with the one href
            if (link && (!previousWasLink || format.anchorHref() != previousHref)) {
                const int start = it.fragment().position();
                if (focused < 0 || (next ? start > focused : start < focused)) {
                    return true;
                }
            }
            previousWasLink = link;
            previousHref = format.anchorHref();
        }
    }
    return false;
}

void TLabel::updateKeyboardAccess()
{
    // A label with a link keeps the focus QLabel gives it for reaching the link. A button takes Tab
    // focus only, or clicking it would take the focus from the command line.
    const bool linksByKeyboard = textInteractionFlags().testFlag(Qt::LinksAccessibleByKeyboard);
    setFocusPolicy(linksByKeyboard ? Qt::StrongFocus : keyboardOperable() ? Qt::TabFocus : Qt::NoFocus);
    // Qt leaves the focus where it is when the policy changes, on a label that can no longer act on a key
    if (hasFocus() && focusPolicy() == Qt::NoFocus) {
        if (TMainConsole* pConsole = mainWindowConsole()) {
            pConsole->focusButtonAfter(this, true);
        } else {
            QWidget::focusNextPrevChild(true);
        }
    }
    update();
}

void TLabel::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Held, or with a modifier, it is still not typing, so it stays here rather than going to the command line
        event->accept();
        if ((event->modifiers() & ~Qt::KeypadModifier) != Qt::NoModifier || event->isAutoRepeat()) {
            return;
        }
        if (hasSelectedText()) {
            // QLabel opens the focused link on Return only
            QKeyEvent openLink(event->type(), Qt::Key_Return, Qt::NoModifier);
            QLabel::keyPressEvent(&openLink);
        } else if (keyboardOperable()) {
            press();
        }
        return;
    }
    TMainConsole* pConsole = mainWindowConsole();
    if (pConsole && hasFocus() && mpHost->caretShortcutMatches(event)) {
        event->accept();
        mpHost->setCaretEnabled(true);
        return;
    }
    // Typing, or a keybinding on a function key, carries on in the command line as it does from the main window
    const bool typed = !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) && !event->text().isEmpty() && event->text().front().isPrint();
    const bool functionKey = event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F35;
    if (pConsole && pConsole->mpCommandLine && hasFocus() && (typed || functionKey)) {
        mpHost->setFocusOnHostActiveCommandLine();
        QKeyEvent forwarded(event->type(), event->key(), event->modifiers(), event->text(), event->isAutoRepeat(), event->count());
        qApp->sendEvent(pConsole->mpCommandLine, &forwarded);
        return;
    }
    QLabel::keyPressEvent(event);
}

// Qt's own chain follows the order scripts made labels in, so with accessibleLabels on the main console's labels
// step through the buttons by their place on screen instead - after a label's own links, which QLabel steps through
bool TLabel::focusNextPrevChild(bool next)
{
    TMainConsole* pConsole = mainWindowConsole();
    if (!pConsole || !mpHost->mAccessibleLabels || pConsole->labelWidget(mName) != this) {
        return QLabel::focusNextPrevChild(next);
    }
    // Hiding or disabling the focused label asks it to pass the focus on, not to step to its next link
    if (isVisible() && isEnabled() && linkBeyondFocused(next)) {
        return QLabel::focusNextPrevChild(next);
    }
    pConsole->focusButtonAfter(this, next);
    return true;
}

void TLabel::focusInEvent(QFocusEvent* event)
{
    QLabel::focusInEvent(event);
    // QLabel selects no link on arriving, and Return has nothing to open on a label that is not a button itself
    const bool tabbed = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    if (tabbed && !keyboardOperable() && linkBeyondFocused(event->reason() == Qt::TabFocusReason)) {
        QLabel::focusNextPrevChild(event->reason() == Qt::TabFocusReason);
    }
    update();
}

void TLabel::focusOutEvent(QFocusEvent* event)
{
    QLabel::focusOutEvent(event);
    TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr;
    // Focus that leaves only for a menu, or for the window to lose it, comes back to the label
    const bool windowOnly = event->reason() == Qt::ActiveWindowFocusReason || event->reason() == Qt::PopupFocusReason || event->reason() == Qt::MenuBarFocusReason;
    if (pConsole && !windowOnly && !qobject_cast<TLabel*>(QApplication::focusWidget())) {
        pConsole->forgetCaretOrigin();
    }
    update();
}

bool TLabel::showsImage() const
{
    // A raster replaces the text, but text drawn over the SVG layer is what the label then shows
    return !pixmap().isNull() || (mpSvgRenderer && QTextDocumentFragment::fromHtml(text()).toPlainText().trimmed().isEmpty());
}

void TLabel::imageMayHaveChanged()
{
    const ShownContent shown = movie() ? ShownContent::Animation : showsImage() ? ShownContent::Graphic : ShownContent::Text;
    // A button's role stays a button whatever it shows
    if (std::exchange(mShownContent, shown) != shown && !mpModel->actsAsButton()) {
        mpModel->accessibilityChanged(TLabelModel::AccessibilityChange::Role);
    }
}

void TLabel::syncTextAnnouncedByQt()
{
    mTextAnnouncedByQt = true;
    mpModel->setText(QLabel::text());
    mTextAnnouncedByQt = false;
}

void TLabel::accessibilityChanged(TLabelModel::AccessibilityChange change)
{
    if (change == TLabelModel::AccessibilityChange::Name) {
        // QLabel::setText() announces the text as the new name whenever the widget's own name is empty,
        // so the widget carries a script's name too, and an unnamed label's text is announced once
        if (const QString& name = mpModel->mAccessibleName; name != accessibleName()) {
            setAccessibleName(name);
        } else if (!mTextAnnouncedByQt) {
            notifyAccessibility(this, QAccessible::NameChanged);
        }
        return;
    }
    updateKeyboardAccess();
    if (change == TLabelModel::AccessibilityChange::KeyboardAccess) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    notifyAccessibility(this, QAccessible::RoleChanged);
#endif
    notifyAccessibility(this, QAccessible::ActionChanged);
}

bool TLabel::carriesLink() const
{
    return textFormat() == Qt::RichText && TLabelModel::containsAnchorTag(text());
}

void TLabel::mousePressEvent(QMouseEvent* event)
{
    // QLabel needs the press to note the link so the release can activate it; with links-only flags it
    // leaves the press ignored, so the label's own click callback still runs.
    bool takenByQt = false;
    if (carriesLink() && !mScreenReaderPress) {
        QLabel::mousePressEvent(event);
        takenByQt = event->isAccepted();
    }

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        // The use of accept() here prevents the propagation of the event to
        // any parent, e.g. the containing TConsole
        event->accept();
        mudlet::self()->activateProfile(mpHost);
    } else if (!takenByQt) {
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
    bool takenByQt = false;
    if (carriesLink() && !mScreenReaderPress) {
        QLabel::mouseReleaseEvent(event);
        takenByQt = event->isAccepted();
    }

    auto labelParent = qobject_cast<TConsole*>(parent());
    if (labelParent && labelParent->mpDockWidget && labelParent->mpDockWidget->isFloating()) {
        // move focus back to the active console / command line:
        mudlet::self()->activateProfile(mpHost);
    }

    if (mpHost && mReleaseFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mReleaseFunction, event);
        event->accept();
    } else if (!takenByQt && !mClickFunction && event->button() == Qt::RightButton && forwardRightClickToConsole(event)) {
        event->accept();
    } else if (!takenByQt) {
        QWidget::mouseReleaseEvent(event);
    }
}

// The console builds its context menu in its text pane, which the label covers but is not
// inside, so an ignored right-click, offered only to the label's ancestors, never reaches it
bool TLabel::forwardRightClickToConsole(QMouseEvent* event)
{
    QWidget* pParent = parentWidget();
    if (!pParent) {
        return false;
    }
    // This label, and any other without a click or release callback stacked under it, is left
    // out of the hit test for a moment, so that it finds what they cover
    const QPoint where = pParent->mapFromGlobal(event->globalPosition().toPoint());
    QList<TLabel*> skipped;
    QWidget* pHit = nullptr;
    for (TLabel* pLabel = this; pLabel; pLabel = qobject_cast<TLabel*>(pHit)) {
        if (pLabel->mClickFunction || pLabel->mReleaseFunction) {
            break;
        }
        pLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        skipped.append(pLabel);
        pHit = pParent->childAt(where);
    }
    for (auto* pLabel : skipped) {
        pLabel->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }
    auto* pPane = qobject_cast<TTextEdit*>(pHit);
    if (!pPane) {
        return false;
    }
    // Not the pane's own right-click handling: that acts on a link the label hides
    pPane->showContextMenu(event->globalPosition().toPoint());
    return true;
}

void TLabel::mouseMoveEvent(QMouseEvent* event)
{
    // QLabel is what switches to the pointing hand over a link. The flag answers what carriesLink() does
    // without scanning the whole text on every move. With no move callback the move is ignored below, so
    // one with a button held (or any, if the parent tracks the mouse) goes on to the label's parent - not
    // to a sibling such as an embedded map.
    if (textInteractionFlags() & Qt::LinksAccessibleByMouse) {
        QLabel::mouseMoveEvent(event);
    }

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

// QLabel only notices the pointer leaving a link from a move, and then restores the cursor it saved when
// the hover began. A made-up move where no link can be ends the hover once the pointer has left the label,
// or before a script sets a cursor that the saved copy would later overwrite.
void TLabel::endLinkHover()
{
    if (textInteractionFlags() & Qt::LinksAccessibleByMouse) {
        const QPointF nowhere(-1, -1);
        QMouseEvent away(QEvent::MouseMove, nowhere, mapToGlobal(nowhere), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QLabel::mouseMoveEvent(&away);
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    endLinkHover();
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

// The SVG is a layer of its own rather than the label's content, because QLabel
// keeps text, a pixmap and a movie in a single slot where each replaces the last.
// Qt paints a QFrame's palette and stylesheet backgrounds before it delivers the
// paint event, so drawing here puts the SVG over the label's background colour
// and under whatever the label is showing.
void TLabel::paintEvent(QPaintEvent* event)
{
    // QLabel draws its content inside contentsRect(), so the layer belongs there
    // too rather than over a stylesheet border or in its padding
    if (const QRect area = contentsRect(); mpSvgRenderer && !area.isEmpty()) {
        // the cache has to be compared in device pixels with the rounding the
        // render uses, or a fractional ratio leaves the test never matching and
        // the document re-rendered on every paint
        const qreal dpr = devicePixelRatioF();
        if (mSvgPixmapCache.size() != area.size() * dpr || !qFuzzyCompare(mSvgPixmapCache.devicePixelRatio(), dpr)) {
            mSvgPixmapCache = renderSvgPixmap(area.size());
        }
        QPainter painter(this);
        painter.drawPixmap(area.topLeft(), mSvgPixmapCache);
    }

    QLabel::paintEvent(event);

    // Drawn here rather than left to the style: a script's stylesheet gives labels no focus style
    if (hasFocus() && keyboardOperable()) {
        QPainter painter(this);
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.drawRect(QRectF(rect()).adjusted(1, 1, -1, -1));
    }
}

// QLabel takes its hint from the text, pixmap or movie in its content slot, and
// the SVG is none of those - it is a layer of this class's own, which QLabel's
// hint cannot see. Geyser's autoAdjustSize() sizes a label from the hint, so a
// label showing nothing but an SVG has to answer with the document plus the same
// extras QLabel would add around a raster of that size. A label that does carry
// content keeps QLabel's answer, because the SVG scales to fit whatever size the
// content asks for and its own size says nothing. Geyser.Label:new always echoes
// an empty rich-text div, so what counts as visible text is what the document
// renders rather than whether the string is empty.
QSize TLabel::sizeHint() const
{
    if (!mpSvgRenderer) {
        return QLabel::sizeHint();
    }

    const bool showsContent = !pixmap().isNull() || movie() || !QTextDocumentFragment::fromHtml(text()).toPlainText().trimmed().isEmpty();
    const QSize documentSize = mpSvgRenderer->defaultSize();
    // a document without width, height or viewBox has no size to offer
    if (showsContent || documentSize.isEmpty()) {
        return QLabel::sizeHint();
    }
    return documentSize.grownBy(contentsMargins()) + QSize(2 * margin(), 2 * margin());
}

bool TLabel::setBackgroundImage(const QString& path)
{
    if (TLabelModel::svgCandidate(path) && setSvgImage(path)) {
        return true;
    }

    // the label keeps what it is showing when the file is not an image at all,
    // and the caller's error message is then the truth
    const QPixmap raster(path);
    if (raster.isNull()) {
        return false;
    }

    clearSvgImage();
    stopMovie();
    setPixmap(raster);
    // setPixmap() empties the text
    mpModel->setText(text());
    imageMayHaveChanged();
    return true;
}

bool TLabel::setSvgImage(const QString& path)
{
    // the new document has to prove readable before the current one goes, or a
    // mistyped path takes the SVG down with it
    auto* renderer = new QSvgRenderer(this);
    if (!TLabelModel::loadSvg(*renderer, path)) {
        delete renderer;
        return false;
    }

    // the tint and the transforms belong to the label rather than to the document,
    // so they carry over to the new image - and one set before any SVG arrived
    // applies as soon as it does
    delete mpSvgRenderer;
    mpSvgRenderer = renderer;
    // a raster background is the label's content, so it would otherwise stay on
    // top of the new SVG layer - text and movies are left alone
    if (!pixmap().isNull()) {
        QLabel::clear();
    }
    refreshSvg();
    updateGeometry();
    imageMayHaveChanged();
    return true;
}

void TLabel::resetBackgroundImage()
{
    // the SVG layer and whatever image sits in QLabel's content slot go together;
    // text is not an image and stays
    clearSvgImage();
    if (!pixmap().isNull() || movie()) {
        stopMovie();
        clear();
        mpModel->setText(text());
    }
    if (const QString sheet = styleSheet(), withoutImage = TLabelModel::styleSheetWithoutBackgroundImage(sheet); withoutImage != sheet) {
        restyle(withoutImage);
    }
    imageMayHaveChanged();
}

void TLabel::stopMovie()
{
    if (auto* pMovie = movie()) {
        // the Host hangs on to this movie to reuse it, so an unstopped one would
        // go on decoding frames for a label that no longer shows it
        pMovie->stop();
    }
}

QPixmap TLabel::renderSvgPixmap(const QSize& size) const
{
    const qreal dpr = devicePixelRatioF();
    QPixmap svgPixmap(size * dpr);
    svgPixmap.fill(Qt::transparent);
    QPainter painter(&svgPixmap);

    if (!qFuzzyIsNull(mSvgRotation) || !qFuzzyIsNull(mSvgShearX) || !qFuzzyIsNull(mSvgShearY)) {
        const qreal cx = svgPixmap.width() / 2.0;
        const qreal cy = svgPixmap.height() / 2.0;
        painter.translate(cx, cy);
        painter.rotate(mSvgRotation);
        painter.shear(mSvgShearX, mSvgShearY);
        painter.translate(-cx, -cy);
    }

    // QSvgRenderer honours its aspect ratio mode only for a document with an
    // explicit viewBox, so the fit is done here
    QRectF targetRect(svgPixmap.rect());
    if (const QSizeF documentSize(mpSvgRenderer->defaultSize()); !documentSize.isEmpty()) {
        const QSizeF fitted = documentSize.scaled(QSizeF(svgPixmap.size()), Qt::KeepAspectRatio);
        targetRect = QRectF(QPointF((svgPixmap.width() - fitted.width()) / 2.0, (svgPixmap.height() - fitted.height()) / 2.0), fitted);
    }
    mpSvgRenderer->render(&painter, targetRect);

    if (mSvgTintColor.isValid()) {
        painter.resetTransform();
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(svgPixmap.rect(), mSvgTintColor);
    }

    painter.end();
    svgPixmap.setDevicePixelRatio(dpr);
    return svgPixmap;
}

// the tint and the transforms are the label's, not the document's, so only their
// own reset functions clear them
void TLabel::clearSvgImage()
{
    if (!mpSvgRenderer) {
        return;
    }
    delete mpSvgRenderer;
    mpSvgRenderer = nullptr;
    refreshSvg();
    // the size hint answered from the document while there was one
    updateGeometry();
}

void TLabel::refreshSvg()
{
    mSvgPixmapCache = QPixmap();
    update();
}

void TLabel::setSvgTint(const QColor& color)
{
    mSvgTintColor = color;
    refreshSvg();
}

void TLabel::clearSvgTint()
{
    mSvgTintColor = QColor();
    refreshSvg();
}

void TLabel::setSvgRotation(double angle)
{
    mSvgRotation = angle;
    refreshSvg();
}

void TLabel::setSvgShear(double shearX, double shearY)
{
    mSvgShearX = shearX;
    mSvgShearY = shearY;
    refreshSvg();
}

void TLabel::resetSvgTransform()
{
    mSvgRotation = 0.0;
    mSvgShearX = 0.0;
    mSvgShearY = 0.0;
    refreshSvg();
}

// Labels have their own right-click handling, so skip QLabel's "Copy Link Location" menu:
// QWidget's handler just passes the event on.
void TLabel::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);
}

void TLabel::setClickThrough(bool clickthrough)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, clickthrough);
    mpModel->setClickThrough(clickthrough);

    // If clickthrough is enabled, text interaction (including hyperlinks) won't work
    // So we need to disable text interaction when clickthrough is on
    if (clickthrough) {
        setTextInteractionFlags(Qt::NoTextInteraction);
    } else {
        // Re-enable text interaction only if the current text has hyperlinks
        setTextInteractionFlags(TLabelModel::containsAnchorTag(text()) ? scmLinkInteraction : Qt::TextInteractionFlags(Qt::NoTextInteraction));
    }
    updateKeyboardAccess();
}

void TLabel::setBackgroundColor(const QColor& color)
{
    mBackgroundColor = color;

    if (!restyle(TLabelModel::styleSheetWithBackgroundColor(styleSheet(), color))) {
        applyBackgroundColor();
    }
}

// QWidget::setStyleSheet() repolishes and repaints even when handed the sheet it
// already has. Re-applying one still matters when it names a file, as that is what
// makes Qt reload an image rewritten on disk, and after the link styling replaced the
// palette, as that is what restores the colours the sheet sets.
bool TLabel::restyle(const QString& sheet)
{
    if (sheet == styleSheet() && !mPaletteSetSinceStyled && !sheet.contains(qsl("url("), Qt::CaseInsensitive)) {
        return false;
    }
    mPaletteSetSinceStyled = false;
    setStyleSheet(sheet);
    return true;
}

// Qt hands a widget back the palette it saved when it first styled it, so every
// restyle - this label's own, an ancestor's, or the application's - drops the colour
// and leaves a label that fills its background on Qt's near-white default
void TLabel::changeEvent(QEvent* event)
{
    QLabel::changeEvent(event);

    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        applyBackgroundColor();
    }
    if (event->type() == QEvent::StyleChange) {
        mpModel->mStyleSheet = styleSheet();
    } else if (event->type() == QEvent::FontChange) {
        mpModel->mFont = font();
    }
}

// QWidget::event() doesn't pass ToolTipChange on to changeEvent()
bool TLabel::event(QEvent* event)
{
    // The caret shortcut has to beat an application-wide QShortcut on the same key, as in TTextEdit::event()
    if (event->type() == QEvent::ShortcutOverride && mainWindowConsole() && hasFocus() && mpHost->caretShortcutMatches(static_cast<QKeyEvent*>(event))) {
        event->accept();
        return true;
    }
    if (event->type() == QEvent::ToolTipChange) {
        mpModel->setToolTip(toolTip());
    }
    const bool handled = QLabel::event(event);
    // A qproperty-text in a style sheet reaches QLabel::setText() directly, as the style is applied
    if (event->type() == QEvent::Polish || event->type() == QEvent::StyleChange) {
        syncTextAnnouncedByQt();
    }
    return handled;
}

void TLabel::applyBackgroundColor()
{
    // the equality test is also what stops setPalette() below recursing back in
    // through changeEvent()
    if (!mBackgroundColor.isValid() || palette().color(QPalette::Window) == mBackgroundColor) {
        return;
    }

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, mBackgroundColor);
    setPalette(palette);
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
    mPaletteSetSinceStyled = true;

    // Widget stylesheets don't affect QTextDocument rendering, so links already shown are re-styled inline
    reapplyLinkStyle();
    update();
}

void TLabel::resetLinkStyle()
{
    // starting from a fresh palette would take the background colour with the link colours
    QPalette palette;
    if (mBackgroundColor.isValid()) {
        palette.setColor(QPalette::Window, mBackgroundColor);
    }
    setPalette(palette);
    mPaletteSetSinceStyled = true;

    mLinkColor.clear();
    mLinkVisitedColor.clear();
    mLinkUnderline = true;

    // the styles written into the anchors would otherwise stay
    reapplyLinkStyle();
    update();
}

void TLabel::clearVisitedLinks()
{
    mVisitedLinks.clear();
    reapplyLinkStyle();
}

void TLabel::reapplyLinkStyle()
{
    // a pixmap or movie takes the text out of QLabel's content slot
    if (const QString currentText = text(); !currentText.isEmpty() && TLabelModel::containsAnchorTag(currentText)) {
        showLinkStyledText(mpModel->restyledText());
    }
}

void TLabel::slot_linkActivated(const QString& link)
{
    if (!mpHost) {
        return;
    }

    if (!mLinkVisitedColor.isEmpty()) {
        mVisitedLinks.insert(link);
        reapplyLinkStyle();
    }

    // Check for custom schemes by looking for the colon separator; a browser
    // also drops the whitespace around an href before reading its scheme
    const QString href = link.trimmed();
    const int colonPos = href.indexOf(':');

    if (colonPos > 0) {
        const QString scheme = href.left(colonPos).toLower(); // RFC 3986: schemes are case-insensitive
        // Untrimmed: a prompt: or send: payload can end in a space the player means to keep
        const QString payload = link.mid(link.indexOf(':') + 1);

        // Handle custom Mudlet URL schemes for Lua commands
        if (scheme == qsl("send")) {
            // send: scheme - send the command to the MUD immediately
            mpHost->send(payload);
            return;
        }

        if (scheme == qsl("prompt")) {
            // prompt: scheme - put text in command line and wait for user to press enter
            if (mpHost->mainConsoleView() && mpHost->mainConsoleView()->mpCommandLine) {
                QPointer<TCommandLine> commandLine = mpHost->mainConsoleView()->mpCommandLine;
                mpHost->mainConsoleView()->printToCommandLine(payload);
                // Defer the focus operation to avoid issues with QPointer manipulation
                // during the signal handler execution
                QTimer::singleShot(0ms, commandLine.data(), [commandLine]() {
                    if (commandLine) {
                        commandLine->setFocus();
                    }
                });
            }
            return;
        }

        if (scheme == qsl("http") || scheme == qsl("https")) {
            QDesktopServices::openUrl(QUrl(href));
            return;
        }

        // After a scheme-shaped prefix only a Lua method call - tel:go(), pkg.ui:show "x" - is Lua;
        // ftp://host, mailto:a@b.c or about:blank are URLs that Lua would only report as syntax errors.
        static const QRegularExpression schemeName(qsl("^[a-z][a-z0-9+.-]*$"));
        // Lua allows whitespace and --[[ ]] comments on either side of the method name
        static const QRegularExpression methodCall(qsl("^(?:\\s|--\\[(=*)\\[.*?\\]\\1\\])*[A-Za-z_][A-Za-z0-9_]*(?:\\s|--\\[(=*)\\[.*?\\]\\2\\])*[(\"'{[]"));
        if (scheme == qsl("javascript") || (schemeName.match(scheme).hasMatch() && !methodCall.match(payload).hasMatch())) {
            //: Printed in the Errors console when a link in a label is clicked but neither handled nor run; %1 is the link, %2 its scheme
            const QString note = tr("[LINK:] The label link \"%1\" was not run: %2: is not a scheme Mudlet handles, and the link is not a Lua command.").arg(link, scheme);
            emit mpHost->signal_errorConsolePrint(note + QChar::LineFeed, QColor(255, 128, 0), QColor(Qt::black));
            return;
        }
    }

    mpHost->mLuaInterpreter.compileAndExecuteScript(link);
}
