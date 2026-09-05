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
class QMouseEvent;
class QSvgRenderer;

class TLabel : public QLabel
{
    Q_OBJECT

    // Declared ahead of every other member: the references below are initialised
    // from it, so it has to be constructed first.
    std::unique_ptr<TLabelModel> mpModel;

public:
    Q_DISABLE_COPY(TLabel)
    explicit TLabel(Host*, const QString&, QWidget* pW = nullptr);
    ~TLabel();

    void setText(const QString& text);
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void enterEvent(TEnterEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;
    QSize sizeHint() const override;
    void setClickThrough(bool clickthrough);
    void setBackgroundColor(const QColor& color);
    void setLinkStyle(const QString& linkColor, const QString& linkVisitedColor, bool underline = true);
    void resetLinkStyle();
    void clearVisitedLinks();
    bool setBackgroundImage(const QString& path);
    void resetBackgroundImage();
    bool setSvgImage(const QString& path);
    void clearSvgImage();
    static bool svgCandidate(const QString& path);
    static bool loadSvg(QSvgRenderer& renderer, const QString& path);
    void setSvgTint(const QColor& color);
    void clearSvgTint();
    void setSvgRotation(double angle);
    void setSvgShear(double shearX, double shearY);
    void resetSvgTransform();
    TLabelModel& model() { return *mpModel; }

    // The members below are references aliasing the model above. They stand for
    // the label's identity, callback registry indexes, link colouring and
    // background colour, which live in the core TLabelModel this label owns and
    // the profile's TWindowRegistry indexes by name.
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
    void applyBackgroundColor();

    QColor& mBackgroundColor;
    QPixmap mSvgPixmapCache;

private slots:
    void slot_linkActivated(const QString& link);

signals:
    void resized();
};

#endif // MUDLET_TLABEL_H
