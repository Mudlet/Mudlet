#ifndef MUDLET_TLABEL_H
#define MUDLET_TLABEL_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020, 2022-2023 by Stephen Lyons                        *
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

#include "TLabelModel.h"
#include "utils.h"

#include <QColor>
#include <QLabel>
#include <QMovie>
#include <QPixmap>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVideoWidget>

#include <memory>

class Host;
class QContextMenuEvent;
class QMouseEvent;
class QSvgRenderer;

class TLabel : public QLabel
{
    Q_OBJECT

    // Declared first: the references below are initialised from it.
    std::unique_ptr<TLabelModel> mpModel;

public:
    Q_DISABLE_COPY(TLabel)
    explicit TLabel(Host*, const QString&, QWidget* pW = nullptr);
    ~TLabel();

    void setText(const QString& text);
    // A screen reader's or the keyboard's activation, delivered as the left click a mouse would make
    void press();
    // Whether what it shows is an image - a raster, or an SVG with no text over it - which a screen reader
    // is told is a graphic unless the label is a button
    bool showsImage() const;
    // After its image or movie changes, which can make it a graphic, an animation or text
    void imageMayHaveChanged();
    // Whether the player can reach and press it from the keyboard: a button, with the profile's accessibleLabels on
    bool keyboardOperable() const;
    // Whether Tab stops on it with accessibleLabels on: a button, or a label with links to step through
    bool keyboardReachable() const;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void enterEvent(TEnterEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool event(QEvent* event) override;
    QSize sizeHint() const override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void setClickThrough(bool clickthrough);
    void endLinkHover();
    void setBackgroundColor(const QColor& color);
    bool restyle(const QString& sheet);
    void setLinkStyle(const QString& linkColor, const QString& linkVisitedColor, bool underline = true);
    void resetLinkStyle();
    void clearVisitedLinks();
    bool setBackgroundImage(const QString& path);
    void resetBackgroundImage();
    bool setSvgImage(const QString& path);
    void clearSvgImage();
    void setSvgTint(const QColor& color);
    void clearSvgTint();
    void setSvgRotation(double angle);
    void setSvgShear(double shearX, double shearY);
    void resetSvgTransform();
    TLabelModel& model() { return *mpModel; }

    // References aliasing the model above.
    QPointer<Host>& mpHost;
    QString& mName;
    int& mClickFunction;
    int& mDoubleClickFunction;
    int& mReleaseFunction;
    int& mMoveFunction;
    int& mWheelFunction;
    int& mEnterFunction;
    int& mLeaveFunction;
    QMovie* mpMovie = nullptr;
    QSvgRenderer* mpSvgRenderer = nullptr;
    QColor& mSvgTintColor;
    double& mSvgRotation;
    double& mSvgShearX;
    double& mSvgShearY;
    QVideoWidget* mpVideoWidget = nullptr;
    QString& mLinkColor;
    QString& mLinkVisitedColor;
    bool& mLinkUnderline;
    QSet<QString>& mVisitedLinks;

private:
    QPixmap renderSvgPixmap(const QSize& size) const;
    void refreshSvg();
    void stopMovie();
    // No selection flags: they would take the press the click callback needs. The keyboard flag puts
    // links in reach of Tab and Return via the focus policy QLabel derives from these flags.
    static constexpr Qt::TextInteractionFlags scmLinkInteraction = Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard;

    void accessibilityChanged(TLabelModel::AccessibilityChange change);
    // After QLabel::setText(), which has told a screen reader of the new text already
    void syncTextAnnouncedByQt();
    bool mTextAnnouncedByQt = false;
    enum class ShownContent { Text, Graphic, Animation };
    ShownContent mShownContent = ShownContent::Text;
    // A screen reader's press is the label's, not one of a link that happens to sit under its centre
    bool mScreenReaderPress = false;
    // QLabel steps between its links but does not say which has the focus, and past the last it would hand Tab
    // to Qt's own chain, so the label keeps count: -1 before the first
    int mFocusedAnchor = -1;
    int anchorCount() const;
    bool carriesLink() const;
    bool forwardRightClickToConsole(QMouseEvent* event);
    void applyBackgroundColor();
    void showLinkStyledText(const QString& text);
    void reapplyLinkStyle();
    // After every setTextInteractionFlags(), which resets the focus policy
    void updateKeyboardAccess();

    QColor& mBackgroundColor;
    QPixmap mSvgPixmapCache;
    // set when the link styling replaces the palette the stylesheet gave the label
    bool mPaletteSetSinceStyled = false;

private slots:
    void slot_linkActivated(const QString& link);

signals:
    void resized();
};

#endif // MUDLET_TLABEL_H
