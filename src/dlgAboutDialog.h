#ifndef MUDLET_DLGABOUTDIALOG_H
#define MUDLET_DLGABOUTDIALOG_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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


#include "ui_about_dialog.h"
#include "uiDesign.h"

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QVector>

class QGridLayout;
class QGroupBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QTimer;
class QToolButton;

namespace uiDesign {
class AboutLinkButton;
class AboutSupporterBanner;
} // namespace uiDesign

struct aboutMaker
{
    bool big = false;
    QString name;
    QString discord;
    QString github;
    QString email;
    QString description;
};

struct aboutThirdParty
{
    QString name;
    QString copyright;
    QString licenceKind;
    QString body;
};

class dlgAboutDialog : public QDialog, public Ui::about_dialog
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgAboutDialog)
    explicit dlgAboutDialog(QWidget* parent = nullptr);

    // Public so a test can check the rows against this build without repeating the #if blocks
    static QVector<aboutThirdParty> thirdPartyComponents();
    static QList<QPair<QString, QString>> buildInfoRows();

    // getSplashScreen() answers a random Easter egg on 1 April, so asking it again
    // does not say what is on screen
    const QImage& splashImage() const { return mSplash; }

protected:
    void resizeEvent(QResizeEvent* pEvent) override;
    bool event(QEvent* pEvent) override;

private slots:
    void showPage(const QString& key);
    void slot_copyBuildInformation();
    void slot_restoreCopyButton();
    void slot_applyAppearance();

private:
    // Rich text bakes the tinted glyph in, so the chip is rebuilt on a theme change
    struct ContactChip
    {
        QPointer<QLabel> pLabel;
        QString glyphFile;
        QString text;
    };

    void buildShell();
    QWidget* buildArtColumn();
    QWidget* buildContentColumn();
    QWidget* buildMudletPage();
    QWidget* buildSupportersPage();
    QWidget* buildLicensePage();
    QWidget* buildThirdPartyPage();

    QScrollArea* createScrollPage(const QString& key);
    QGroupBox* createCard(const QString& title, QWidget* pParent);
    QWidget* createSection(const QString& title, const QString& note, QWidget* pParent);
    QWidget* createContactChips(const aboutMaker& maker, QWidget* pParent);
    QWidget* createMakerCard(const aboutMaker& maker, QWidget* pParent);
    QWidget* createMoreRow(const aboutMaker& maker, QWidget* pParent);
    QWidget* createSeparatorLine(QWidget* pParent);
    QPushButton* createCopyButton(const QString& objectName, const QString& label, QWidget* pParent);
    QLabel* createParagraph(const QString& text, QWidget* pParent);
    // Every label that can hold a link must go through here: anchors bake their colour in
    void setRichText(QLabel* pLabel, const QString& text);

    void applyShellStyle();
    void restyleArtwork(const uiDesign::ThemeTokens& tokens);
    void restyleContactChips(const uiDesign::ThemeTokens& tokens);
    void restyleRichText(const uiDesign::ThemeTokens& tokens);
    QIcon copyButtonIcon(const bool copied, const uiDesign::ThemeTokens& tokens) const;
    void updateArtColumnWidth();
    void layOutLinkButtons(const int columns);
    void setLicenseText(const uiDesign::ThemeTokens& tokens);
    QString buildInformationText() const;

    static QVector<aboutMaker> makers();

    QWidget* mpWidget_shell = nullptr;
    QWidget* mpWidget_artColumn = nullptr;
    QStackedWidget* mpStackedWidget_pages = nullptr;
    QGridLayout* mpLayout_links = nullptr;
    QTimer* mpTimer_copyFeedback = nullptr;
    QPointer<QPushButton> mpButton_showingCopied;

    QMap<QString, int> mPageIndexes;
    QMap<QString, QToolButton*> mNavButtons;
    QList<uiDesign::AboutLinkButton*> mLinkButtons;
    QList<uiDesign::AboutSupporterBanner*> mBanners;
    QList<QToolButton*> mThirdPartyToggles;
    QList<QPushButton*> mCopyButtons;
    QList<ContactChip> mContactChips;

    // Kept unscaled, to redraw at the column's other width and the theme's hairline
    QImage mSplash;
    bool mNarrow = false;
    bool mArtColumnPlaced = false;
    QColor mInkedLinkColour;
    QString mLicenceInkKey;
};

#endif // MUDLET_DLGABOUTDIALOG_H
