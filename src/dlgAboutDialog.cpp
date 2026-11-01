/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2017-2019, 2022, 2024-2026 by Stephen Lyons  *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


// Debugging value to display ALL licences in the dialog
// #define DEBUG_SHOWALL

#include "dlgAboutDialog.h"

#include "AboutLinkButton.h"
#include "AboutSupporterBanner.h"
#include "FlowLayout.h"
#include "mudlet.h"
#include "MudletApp.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QSysInfo>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

using namespace uiDesign;

namespace {
// The two thresholds differ on purpose: held equal, a one-pixel drag across the
// boundary would flip the column back and forth
constexpr int scmArtColumnWidth = 304;
constexpr int scmArtColumnPadding = 20;
constexpr int scmNarrowArtColumnWidth = 238;
constexpr int scmNarrowArtColumnPadding = 14;
constexpr int scmNarrowBelowWidth = 1000;
constexpr int scmWidenAboveWidth = 1060;
constexpr int scmLinkColumns = 3;
constexpr int scmNarrowLinkColumns = 2;

constexpr int scmMinimumWidth = 900;
constexpr int scmMinimumHeight = 560;
constexpr int scmPreferredWidth = 1080;
constexpr int scmPreferredHeight = 680;

constexpr int scmCardPadding = 16;
constexpr int scmPageSpacing = 16;
constexpr int scmPageMarginTop = 18;
constexpr int scmPageMarginSide = 20;
constexpr int scmPageMarginBottom = 24;

inline constexpr char scmProp_aboutRichText[] = "aboutRichText";

constexpr int scmChipSpacing = 5;
constexpr int scmChipGlyphSize = 18;
// This window's own: a notice elsewhere is uiDesign's shared control, at its own size
constexpr int scmAboutLicenceGlyphSize = 18;
// In characters, so a 25-character address still fits at a 150% interface font
constexpr int scmMoreNameColumnCharacters = 30;

constexpr int scmCopiedMilliseconds = 1600;

constexpr int scmPrimaryButtonHeight = 30;

// Opened up a little so the capitals do not close up
constexpr int scmTierLetterSpacing = 106;

int artworkWidth(const bool narrow)
{
    return (narrow ? scmNarrowArtColumnWidth : scmArtColumnWidth) - 2 * (narrow ? scmNarrowArtColumnPadding : scmArtColumnPadding);
}

// At the application's size, so the stylesheet's percentage is relative to it
QFont fixedPitchFont()
{
    return uiDesign::fixedPitchFont(QApplication::font());
}

// Rasterised for its slot rather than the 128px file, so nothing downsamples it later
QPixmap glyphAt(const QString& file, const int size, const qreal ratio, const QColor& colour)
{
    QPixmap glyph = uiDesign::glyphPixmap(file).scaled(qRound(size * ratio), qRound(size * ratio), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph.setDevicePixelRatio(ratio);
    return uiDesign::tintedGlyph(glyph, colour);
}

} // namespace

dlgAboutDialog::dlgAboutDialog(QWidget* parent)
: QDialog(parent)
{
    setupUi(this);

    mSplash = mudlet::getSplashScreen(MudletApp::release(), MudletApp::publicTest());

    // Settled once rather than per label: themeTokens() walks four colours to their
    // contrast floors, and the shell builds eighty labels
    mInkedLinkColour = themeTokens().accentText;

    mpTimer_copyFeedback = new QTimer(this);
    mpTimer_copyFeedback->setObjectName(qsl("aboutCopyFeedback"));
    mpTimer_copyFeedback->setSingleShot(true);
    mpTimer_copyFeedback->setInterval(scmCopiedMilliseconds);
    connect(mpTimer_copyFeedback, &QTimer::timeout, this, &dlgAboutDialog::slot_restoreCopyButton);

    buildShell();
    applyShellStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgAboutDialog::slot_applyAppearance);

    setMinimumSize(scmMinimumWidth, scmMinimumHeight);
    QSize opening(scmPreferredWidth, scmPreferredHeight);
    if (const QScreen* pScreen = screen(); pScreen) {
        const QSize available = pScreen->availableGeometry().size();
        opening = opening.boundedTo(available);
    }
    resize(opening);
    showPage(qsl("mudlet"));
}

void dlgAboutDialog::buildShell()
{
    mpWidget_shell = new QWidget(this);
    mpWidget_shell->setObjectName(qsl("aboutShell"));
    markAsShellSurface(mpWidget_shell);

    auto* pShellLayout = new QHBoxLayout(mpWidget_shell);
    pShellLayout->setContentsMargins(0, 0, 0, 0);
    pShellLayout->setSpacing(0);
    pShellLayout->addWidget(buildArtColumn());
    pShellLayout->addWidget(buildContentColumn(), 1);

    auto* pRootLayout = layout();
    pRootLayout->setContentsMargins(0, 0, 0, 0);
    pRootLayout->setSpacing(0);
    pRootLayout->addWidget(mpWidget_shell);

    updateArtColumnWidth();
}

QWidget* dlgAboutDialog::buildArtColumn()
{
    mpWidget_artColumn = new QWidget(mpWidget_shell);
    mpWidget_artColumn->setObjectName(qsl("aboutArtColumn"));
    mpWidget_artColumn->setFixedWidth(scmArtColumnWidth);

    auto* pColumn = new QVBoxLayout(mpWidget_artColumn);
    pColumn->setContentsMargins(scmArtColumnPadding, scmArtColumnPadding, scmArtColumnPadding, scmArtColumnPadding);
    pColumn->setSpacing(8);

    detachFromLayout(mudletTitleLabel);
    mudletTitleLabel->setParent(mpWidget_artColumn);
    mudletTitleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    //: Alternative text for the Mudlet artwork on the About dialog, read out by a screen reader
    mudletTitleLabel->setAccessibleName(tr("The Mudlet arch"));
    pColumn->addWidget(mudletTitleLabel);

    auto* pName = new QLabel(qsl("Mudlet"), mpWidget_artColumn);
    pName->setObjectName(qsl("aboutName"));
    pColumn->addWidget(pName);

    auto* pVersion = new QLabel(qsl(APP_VERSION) + MudletApp::buildSuffix(), mpWidget_artColumn);
    pVersion->setObjectName(qsl("aboutVersion"));
    pVersion->setFont(fixedPitchFont());
    pVersion->setWordWrap(true);
    pVersion->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pColumn->addWidget(pVersion);

    auto* pChipRow = new QWidget(mpWidget_artColumn);
    markAsShellSurface(pChipRow);
    auto* pChipLayout = new QHBoxLayout(pChipRow);
    pChipLayout->setContentsMargins(0, 2, 0, 2);
    pChipLayout->setSpacing(6);

    QString channel;
    if (MudletApp::release()) {
        //: Chip on the About dialog naming which kind of build of Mudlet this is
        channel = tr("Release");
    } else if (MudletApp::publicTest()) {
        //: Chip on the About dialog naming which kind of build of Mudlet this is
        channel = tr("Public test build");
    } else {
        //: Chip on the About dialog naming which kind of build of Mudlet this is
        channel = tr("Development build");
    }
    auto* pChannelChip = new QLabel(channel, pChipRow);
    pChannelChip->setObjectName(qsl("aboutChannelChip"));
    pChannelChip->setProperty("aboutChip", true);
    pChannelChip->setProperty("aboutChipLit", true);
    pChipLayout->addWidget(pChannelChip);

    auto* pQtChip = new QLabel(qsl("Qt %1").arg(QString::fromLatin1(qVersion())), pChipRow);
    pQtChip->setObjectName(qsl("aboutQtChip"));
    pQtChip->setProperty("aboutChip", true);
    pChipLayout->addWidget(pQtChip);
    pChipLayout->addStretch(1);
    pColumn->addWidget(pChipRow);

    // PLACEMARKER: Date-stamp needing annual update
    //: %1 is the year the copyright runs to
    auto* pCopyright = new QLabel(tr("Copyright 2008-%1 the Mudlet makers").arg(qsl("2026")), mpWidget_artColumn);
    pCopyright->setObjectName(qsl("aboutCopyright"));
    pCopyright->setWordWrap(true);
    pColumn->addWidget(pCopyright);

    //: Button that puts the build information on the clipboard, to be pasted into a bug report
    QPushButton* pCopy = createCopyButton(qsl("aboutCopyBuildInfo"), tr("Copy build information"), mpWidget_artColumn);
    pColumn->addWidget(pCopy);

    pColumn->addStretch(1);

    auto* pFooter = new QLabel(mpWidget_artColumn);
    pFooter->setObjectName(qsl("aboutFooter"));
    pFooter->setWordWrap(true);
    pFooter->setOpenExternalLinks(false);
    /*: Footer of the About dialog. The link opens the License page of this same
 dialog rather than a web page, so keep the href as it is. */
    setRichText(pFooter, tr("Free software under the <a href=\"license\">GPL 3.0</a>."));
    connect(pFooter, &QLabel::linkActivated, this, [this](const QString&) {
        showPage(qsl("license"));
    });
    pColumn->addWidget(pFooter);

    return mpWidget_artColumn;
}

QWidget* dlgAboutDialog::buildContentColumn()
{
    auto* pContent = new QWidget(mpWidget_shell);
    pContent->setObjectName(qsl("aboutContent"));
    markAsShellSurface(pContent);
    auto* pContentLayout = new QVBoxLayout(pContent);
    pContentLayout->setContentsMargins(0, 0, 0, 0);
    pContentLayout->setSpacing(0);

    auto* pNav = new QWidget(pContent);
    pNav->setObjectName(qsl("aboutNav"));
    markAsShellSurface(pNav);
    auto* pNavLayout = new QHBoxLayout(pNav);
    pNavLayout->setContentsMargins(scmPageMarginSide, 12, scmPageMarginSide, 10);
    pNavLayout->setSpacing(4);

    mpStackedWidget_pages = new QStackedWidget(pContent);
    mpStackedWidget_pages->setObjectName(qsl("aboutStack"));
    markAsShellSurface(mpStackedWidget_pages);

    // Translated in the .ui file's context, which the pages' titles were tabs in until the shell took them over,
    // so every language keeps the words it already has
    //: Title of the About window's page about Mudlet itself
    const QString mudletTitle = QCoreApplication::translate("about_dialog", "Mudlet");
    //: Title of the About window's page thanking the people who fund Mudlet
    const QString supportersTitle = QCoreApplication::translate("about_dialog", "Supporters");
    //: Title of the About window's page showing Mudlet's own licence
    const QString licenseTitle = QCoreApplication::translate("about_dialog", "License");
    //: Title of the About window's page listing the third-party components Mudlet is built with and their licences
    const QString thirdPartyTitle = QCoreApplication::translate("about_dialog", "Third Party");
    const QList<QPair<QString, QPair<QString, QString>>> pages{{qsl("mudlet"), {mudletTitle, qsl(":/icons/about-mudlet.svg")}},
                                                               {qsl("supporters"), {supportersTitle, qsl(":/icons/about-supporters.svg")}},
                                                               {qsl("license"), {licenseTitle, qsl(":/icons/about-license.svg")}},
                                                               {qsl("thirdparty"), {thirdPartyTitle, qsl(":/icons/about-third-party.svg")}}};
    for (const auto& page : pages) {
        auto* pButton = new QToolButton(pNav);
        pButton->setObjectName(qsl("aboutNavButton_%1").arg(page.first));
        pButton->setText(page.second.first);
        pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        pButton->setCheckable(true);
        pButton->setAutoExclusive(true);
        pButton->setFocusPolicy(Qt::StrongFocus);
        pButton->setProperty("aboutNavGlyph", page.second.second);
        const QString key = page.first;
        connect(pButton, &QToolButton::clicked, this, [this, key]() {
            showPage(key);
        });
        mNavButtons.insert(key, pButton);
        pNavLayout->addWidget(pButton);
    }
    pNavLayout->addStretch(1);

    mPageIndexes.insert(qsl("mudlet"), mpStackedWidget_pages->addWidget(buildMudletPage()));
    mPageIndexes.insert(qsl("supporters"), mpStackedWidget_pages->addWidget(buildSupportersPage()));
    mPageIndexes.insert(qsl("license"), mpStackedWidget_pages->addWidget(buildLicensePage()));
    mPageIndexes.insert(qsl("thirdparty"), mpStackedWidget_pages->addWidget(buildThirdPartyPage()));

    pContentLayout->addWidget(pNav);
    pContentLayout->addWidget(mpStackedWidget_pages, 1);
    return pContent;
}

QScrollArea* dlgAboutDialog::createScrollPage(const QString& key)
{
    auto* pScrollArea = new QScrollArea(mpStackedWidget_pages);
    pScrollArea->setObjectName(qsl("aboutPage_%1").arg(key));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    markAsShellSurface(pScrollArea);

    auto* pColumn = new QWidget(pScrollArea);
    pColumn->setObjectName(qsl("aboutColumn_%1").arg(key));
    auto* pColumnLayout = new QVBoxLayout(pColumn);
    pColumnLayout->setContentsMargins(scmPageMarginSide, scmPageMarginTop, scmPageMarginSide, scmPageMarginBottom);
    pColumnLayout->setSpacing(scmPageSpacing);

    pScrollArea->setWidget(pColumn);
    // setWidget() turns autoFillBackground on, which would paint over the page's surface
    pColumn->setAutoFillBackground(false);
    pScrollArea->viewport()->setAutoFillBackground(false);
    markAsShellSurface(pColumn);
    markAsShellSurface(pScrollArea->viewport());
    return pScrollArea;
}

QGroupBox* dlgAboutDialog::createCard(const QString& title, QWidget* pParent)
{
    auto* pCard = new QGroupBox(title, pParent);
    pCard->setProperty(scmProp_aboutCard, true);
    if (title.isEmpty()) {
        pCard->setProperty(scmProp_aboutCardPlain, true);
    }
    return pCard;
}

QWidget* dlgAboutDialog::createSection(const QString& title, const QString& note, QWidget* pParent)
{
    auto* pSection = new QWidget(pParent);
    pSection->setObjectName(qsl("aboutSection"));
    markAsShellSurface(pSection);
    auto* pRow = new QHBoxLayout(pSection);
    pRow->setContentsMargins(0, 22, 0, 10);
    pRow->setSpacing(10);

    auto* pTitle = new QLabel(title, pSection);
    pTitle->setObjectName(qsl("aboutSectionTitle"));
    pRow->addWidget(pTitle);
    if (!note.isEmpty()) {
        auto* pNote = new QLabel(note, pSection);
        pNote->setObjectName(qsl("aboutSectionNote"));
        pRow->addWidget(pNote);
    }
    pRow->addStretch(1);
    return pSection;
}

QWidget* dlgAboutDialog::createSeparatorLine(QWidget* pParent)
{
    auto* pLine = new QWidget(pParent);
    pLine->setObjectName(qsl("aboutSeparatorLine"));
    pLine->setFixedHeight(1);
    return pLine;
}

QLabel* dlgAboutDialog::createParagraph(const QString& text, QWidget* pParent)
{
    auto* pLabel = new QLabel(pParent);
    pLabel->setWordWrap(true);
    pLabel->setOpenExternalLinks(true);
    setRichText(pLabel, text);
    return pLabel;
}

void dlgAboutDialog::setRichText(QLabel* pLabel, const QString& text)
{
    pLabel->setTextFormat(Qt::RichText);
    pLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    // text() will carry a colour in every anchor; the original is kept to re-ink from
    pLabel->setProperty(scmProp_aboutRichText, text);
    pLabel->setText(uiDesign::withLinkColour(text, mInkedLinkColour));
}

void dlgAboutDialog::restyleRichText(const ThemeTokens& tokens)
{
    for (QLabel* pLabel : mpWidget_shell->findChildren<QLabel*>()) {
        const QVariant raw = pLabel->property(scmProp_aboutRichText);
        if (raw.isValid()) {
            pLabel->setText(uiDesign::withLinkColour(raw.toString(), tokens.accentText));
        }
    }
    mInkedLinkColour = tokens.accentText;
}

QPushButton* dlgAboutDialog::createCopyButton(const QString& objectName, const QString& label, QWidget* pParent)
{
    auto* pButton = new QPushButton(label, pParent);
    pButton->setObjectName(objectName);
    pButton->setProperty("aboutButton", true);
    pButton->setProperty("aboutRestingText", label);
    pButton->setAutoDefault(false);
    pButton->setDefault(false);
    connect(pButton, &QPushButton::clicked, this, &dlgAboutDialog::slot_copyBuildInformation);
    mCopyButtons.append(pButton);
    return pButton;
}

QWidget* dlgAboutDialog::createContactChips(const aboutMaker& maker, QWidget* pParent)
{
    auto* pRow = new QWidget(pParent);
    pRow->setObjectName(qsl("aboutContacts"));
    markAsShellSurface(pRow);
    // Wraps: a QHBoxLayout's minimum is the sum of its items, which pushed the card
    // beside it off the page
    auto* pLayout = new uiDesign::FlowLayout(pRow, scmChipSpacing, scmChipSpacing);
    pLayout->setWidthPolicy(uiDesign::FlowLayout::Width::WidestItem);
    pLayout->setContentsMargins(0, 0, 0, 0);
    // Qt only asks a layout for heightForWidth when its widget's size policy says so
    QSizePolicy rowPolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    rowPolicy.setHeightForWidth(true);
    pRow->setSizePolicy(rowPolicy);

    const QList<QPair<QString, QString>> contacts{{qsl(":/icons/about-github.png"), maker.github}, {qsl(":/icons/toolbar-discord.png"), maker.discord}, {qsl(":/icons/about-mail.svg"), maker.email}};
    for (const auto& contact : contacts) {
        if (contact.second.isEmpty()) {
            continue;
        }
        auto* pChip = new QLabel(pRow);
        pChip->setObjectName(qsl("aboutContactChip"));
        pChip->setProperty("aboutChip", true);
        pChip->setTextFormat(Qt::RichText);
        pChip->setTextInteractionFlags(Qt::TextSelectableByMouse);
        mContactChips.append({pChip, contact.first, contact.second});
        pLayout->addWidget(pChip);
    }
    if (pLayout->count() == 0) {
        pRow->hide();
    }
    return pRow;
}

QWidget* dlgAboutDialog::createMakerCard(const aboutMaker& maker, QWidget* pParent)
{
    QGroupBox* pCard = createCard(maker.name, pParent);
    pCard->setObjectName(qsl("aboutPersonCard"));
    auto* pLayout = new QVBoxLayout(pCard);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(6);
    pLayout->addWidget(createContactChips(maker, pCard));

    auto* pDescription = createParagraph(maker.description, pCard);
    pDescription->setObjectName(qsl("aboutPersonDescription"));
    pLayout->addWidget(pDescription);
    pLayout->addStretch(1);
    return pCard;
}

QWidget* dlgAboutDialog::createMoreRow(const aboutMaker& maker, QWidget* pParent)
{
    auto* pRow = new QWidget(pParent);
    pRow->setObjectName(qsl("aboutMoreRow"));
    markAsShellSurface(pRow);
    // A box, not a grid: a grid spreads a wrapping description over the rows it
    // spans and comes out far taller than the words
    auto* pRowLayout = new QHBoxLayout(pRow);
    pRowLayout->setContentsMargins(0, 9, 0, 9);
    pRowLayout->setSpacing(14);

    auto* pWho = new QWidget(pRow);
    markAsShellSurface(pWho);
    pWho->setFixedWidth(QFontMetrics(pWho->font()).averageCharWidth() * scmMoreNameColumnCharacters);
    auto* pWhoLayout = new QVBoxLayout(pWho);
    pWhoLayout->setContentsMargins(0, 0, 0, 0);
    pWhoLayout->setSpacing(4);
    auto* pName = new QLabel(maker.name, pWho);
    pName->setObjectName(qsl("aboutMoreName"));
    pName->setWordWrap(true);
    pWhoLayout->addWidget(pName);
    pWhoLayout->addWidget(createContactChips(maker, pWho));
    pWhoLayout->addStretch(1);
    pRowLayout->addWidget(pWho, 0, Qt::AlignTop);

    auto* pDescription = createParagraph(maker.description, pRow);
    pDescription->setObjectName(qsl("aboutMoreDescription"));
    pRowLayout->addWidget(pDescription, 1, Qt::AlignTop);
    return pRow;
}

QWidget* dlgAboutDialog::buildMudletPage()
{
    QScrollArea* pPage = createScrollPage(qsl("mudlet"));
    QWidget* pColumn = pPage->widget();
    auto* pColumnLayout = qobject_cast<QVBoxLayout*>(pColumn->layout());

    auto* pLinks = new QWidget(pColumn);
    pLinks->setObjectName(qsl("aboutLinks"));
    markAsShellSurface(pLinks);
    mpLayout_links = new QGridLayout(pLinks);
    mpLayout_links->setContentsMargins(0, 0, 0, 0);
    mpLayout_links->setSpacing(10);

    const QList<QList<QString>> links{//: Name of the link to the Mudlet homepage
                                      {qsl(":/icons/about-homepage.svg"), tr("Homepage"), qsl("www.mudlet.org"), qsl("https://www.mudlet.org/")},
                                      //: Name of the link to the Mudlet forums
                                      {qsl(":/icons/about-forums.svg"), tr("Forums"), qsl("forums.mudlet.org"), qsl("https://forums.mudlet.org/")},
                                      //: Name of the link to the Mudlet wiki
                                      {qsl(":/icons/about-docs.svg"), tr("Documentation"), qsl("wiki.mudlet.org/w/Main_Page"), qsl("https://wiki.mudlet.org/w/Main_Page")},
                                      //: Name of the link to the Mudlet Discord server
                                      {qsl(":/icons/toolbar-discord.png"), tr("Discord"), qsl("mudlet.org/chat"), qsl("https://www.mudlet.org/chat")},
                                      //: Name of the link to Mudlet's source code
                                      {qsl(":/icons/about-github.png"), tr("Source code"), qsl("github.com/Mudlet/Mudlet"), qsl("https://github.com/Mudlet/Mudlet")},
                                      //: Name of the link to Mudlet's issue tracker
                                      {qsl(":/icons/about-bug.svg"), tr("Report a bug"), qsl("github.com/Mudlet/Mudlet/issues"), qsl("https://github.com/Mudlet/Mudlet/issues")}};
    for (const auto& link : links) {
        auto* pButton = new uiDesign::AboutLinkButton(link.at(0), link.at(1), link.at(2), link.at(3), pLinks);
        pButton->setObjectName(qsl("aboutLinkButton"));
        mLinkButtons.append(pButton);
    }
    layOutLinkButtons(scmLinkColumns);
    pColumnLayout->addWidget(pLinks);

    //: Title of the card on the About dialog holding the version, OS and Qt version
    QGroupBox* pBuildCard = createCard(tr("Build information"), pColumn);
    pBuildCard->setObjectName(qsl("aboutBuildCard"));
    auto* pBuildLayout = new QVBoxLayout(pBuildCard);
    pBuildLayout->setContentsMargins(0, 0, 0, 0);
    pBuildLayout->setSpacing(10);

    auto* pHeadRow = new QWidget(pBuildCard);
    markAsShellSurface(pHeadRow);
    auto* pHeadLayout = new QHBoxLayout(pHeadRow);
    pHeadLayout->setContentsMargins(0, 0, 0, 0);
    pHeadLayout->setSpacing(10);
    //: Description under the title of the build information card
    auto* pDescription = new QLabel(tr("What to paste into a bug report."), pHeadRow);
    pDescription->setObjectName(qsl("aboutCardDescription"));
    pHeadLayout->addWidget(pDescription);
    pHeadLayout->addStretch(1);
    //: Button that puts the build information on the clipboard
    pHeadLayout->addWidget(createCopyButton(qsl("aboutCopyButton"), tr("Copy"), pHeadRow));
    pBuildLayout->addWidget(pHeadRow);

    auto* pFacts = new QWidget(pBuildCard);
    pFacts->setObjectName(qsl("aboutBuildFacts"));
    markAsShellSurface(pFacts);
    auto* pFactsGrid = new QGridLayout(pFacts);
    pFactsGrid->setContentsMargins(0, 0, 0, 0);
    pFactsGrid->setHorizontalSpacing(20);
    pFactsGrid->setVerticalSpacing(4);
    pFactsGrid->setColumnStretch(1, 1);
    int factRow = 0;
    for (const auto& fact : buildInfoRows()) {
        auto* pKey = new QLabel(fact.first, pFacts);
        pKey->setObjectName(qsl("aboutBuildKey"));
        pKey->setFont(fixedPitchFont());
        auto* pValue = new QLabel(fact.second, pFacts);
        pValue->setObjectName(qsl("aboutBuildValue"));
        pValue->setFont(fixedPitchFont());
        pValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
        pFactsGrid->addWidget(pKey, factRow, 0, Qt::AlignTop);
        pFactsGrid->addWidget(pValue, factRow, 1, Qt::AlignTop);
        ++factRow;
    }
    pBuildLayout->addWidget(pFacts);
    pColumnLayout->addWidget(pBuildCard);

    const QVector<aboutMaker> allMakers = makers();

    //: Section heading over the cards naming the people who make Mudlet
    pColumnLayout->addWidget(createSection(tr("Credits"), QString(), pColumn));

    auto* pPeople = new QWidget(pColumn);
    pPeople->setObjectName(qsl("aboutPeople"));
    markAsShellSurface(pPeople);
    auto* pPeopleGrid = new QGridLayout(pPeople);
    pPeopleGrid->setContentsMargins(0, 0, 0, 0);
    pPeopleGrid->setSpacing(10);
    pPeopleGrid->setColumnStretch(0, 1);
    pPeopleGrid->setColumnStretch(1, 1);
    int personIndex = 0;
    for (const aboutMaker& maker : allMakers) {
        if (!maker.big) {
            continue;
        }
        pPeopleGrid->addWidget(createMakerCard(maker, pPeople), personIndex / 2, personIndex % 2);
        ++personIndex;
    }
    pColumnLayout->addWidget(pPeople);

    //: Section heading over the compact list of everyone else who has contributed
    const QString moreHeading = tr("More contributors");
    //: Note beside the "More contributors" heading on the About dialog
    const QString moreNote = tr("everyone who left a mark");
    pColumnLayout->addWidget(createSection(moreHeading, moreNote, pColumn));

    auto* pMore = new QWidget(pColumn);
    pMore->setObjectName(qsl("aboutMoreList"));
    markAsShellSurface(pMore);
    auto* pMoreLayout = new QVBoxLayout(pMore);
    pMoreLayout->setContentsMargins(0, 0, 0, 0);
    pMoreLayout->setSpacing(0);
    for (const aboutMaker& maker : allMakers) {
        if (maker.big) {
            continue;
        }
        pMoreLayout->addWidget(createSeparatorLine(pMore));
        pMoreLayout->addWidget(createMoreRow(maker, pMore));
    }
    pMoreLayout->addWidget(createSeparatorLine(pMore));
    pColumnLayout->addWidget(pMore);

    //: Section heading over the closing paragraphs of the Mudlet page
    pColumnLayout->addWidget(createSection(tr("Thanks"), QString(), pColumn));

    QGroupBox* pThanks = createCard(QString(), pColumn);
    pThanks->setObjectName(qsl("aboutThanksCard"));
    auto* pThanksLayout = new QVBoxLayout(pThanks);
    pThanksLayout->setContentsMargins(0, 0, 0, 0);
    pThanksLayout->setSpacing(8);
    const QStringList thanks{
            //: About dialog, thanks: where Mudlet's icons come from
            tr("Many icons are taken from the <b>KDE4 Oxygen icon theme</b>. Most of the rest are from <b>Thorsten Wilms</b>, or from <b>Stephen Lyons</b> combining bits of Thorsten's work "
               "with the other sources. The main toolbar, settings and editor line icons are from the <b>Lucide</b> icon set at <a href=\"https://lucide.dev\">lucide.dev</a>, used under the "
               "ISC licence, and the Discord, GitHub and Patreon marks from <b>Simple Icons</b> at <a href=\"https://simpleicons.org\">simpleicons.org</a>, used under the CC0 1.0 licence."),
            //: About dialog, thanks: two people who shaped the scripting framework
            tr("Special thanks to <b>Brett Duzevich</b> and <b>Ronny Ho</b>. They have contributed many good ideas and thus helped improve the scripting framework substantially."),
            //: About dialog, thanks: the KMuddy project Mudlet's telnet code came from
            tr("Thanks to <b>Tomas Mecir</b> (kmuddy@kmuddy.com) who brought us all together and inspired us with his KMuddy project. Mudlet is using some of the telnet code he wrote for his "
               "KMuddy project (<a href=\"https://cgit.kde.org/kmuddy.git/\">cgit.kde.org/kmuddy.git/</a>)."),
            //: About dialog, thanks: the author of MUSHclient
            tr("Special thanks to <b>Nick Gammon</b> (<a href=\"http://www.gammon.com.au/mushclient/mushclient.htm\">www.gammon.com.au/mushclient/mushclient.htm</a>) for giving us some valued "
               "pieces of advice."),
            //: About dialog, thanks: everyone not named above
            tr("Others too, have made their mark on different aspects of the Mudlet project and if they have not been mentioned here it is by no means intentional! For past contributors you "
               "may see them mentioned in the <b><a href=\"https://launchpad.net/~mudlet-makers/+members\">Mudlet Makers</a></b> list (on our former bug-tracking site), or for on-going "
               "contributors they may well be included in the <b><a href=\"https://github.com/Mudlet/Mudlet/graphs/contributors\">Contributors</a></b> list on GitHub.")};
    for (const QString& paragraph : thanks) {
        auto* pParagraph = createParagraph(paragraph, pThanks);
        pParagraph->setObjectName(qsl("aboutThanksParagraph"));
        pThanksLayout->addWidget(pParagraph);
    }
    pColumnLayout->addWidget(pThanks);
    pColumnLayout->addStretch(1);
    return pPage;
}

QWidget* dlgAboutDialog::buildSupportersPage()
{
    QScrollArea* pPage = createScrollPage(qsl("supporters"));
    QWidget* pColumn = pPage->widget();
    auto* pColumnLayout = qobject_cast<QVBoxLayout*>(pColumn->layout());
    pColumnLayout->setSpacing(10);

    // see https://www.patreon.com/mudlet if you'd like to be added!
    const QStringList mightier_than_swords = {/* active */ "Joshua C. Burt", "StickMUD", "Medievia", /* inactive */ "Qwindor Rousseau", "Maiyannah Bishop", "Stick In the MUD 🎙"};
    const QStringList on_a_plaque = {"demonnic", "Henry Hsiao"};

    const QString introText = mudlet::smSteamMode ? tr(R"(
                            These formidable folks will be fondly remembered forever<br>for their generous financial support on Mudlet's patreon:
                            )")
                                                  : tr(R"(
                            These formidable folks will be fondly remembered forever<br>for their generous financial support on <a href="https://www.patreon.com/mudlet">Mudlet's patreon</a>:
                            )");
    auto* pIntro = new QLabel(pColumn);
    pIntro->setObjectName(qsl("aboutSupportersIntro"));
    pIntro->setWordWrap(true);
    pIntro->setAlignment(Qt::AlignHCenter);
    pIntro->setOpenExternalLinks(true);
    setRichText(pIntro, introText);
    // No layout alignment flag: with one, the box measures the height at the column
    // width while the label draws at its cap, which clipped it to two lines
    pColumnLayout->addWidget(pIntro);

    struct Tier
    {
        QString heading;
        bool swords = false;
        QStringList names;
    };
    const QList<Tier> tiers{//: Heading over the higher tier of Mudlet's Patreon supporters
                            {tr("Mightier than swords"), true, mightier_than_swords},
                            //: Heading over the second tier of Mudlet's Patreon supporters
                            {tr("On a plaque"), false, on_a_plaque}};
    bool firstTier = true;
    for (const Tier& tier : tiers) {
        if (!firstTier) {
            pColumnLayout->addSpacing(8);
        }
        firstTier = false;
        auto* pTier = new QLabel(tier.heading, pColumn);
        pTier->setObjectName(qsl("aboutTier"));
        pTier->setAlignment(Qt::AlignHCenter);
        // Uppercased by the font rather than the words, so a caseless script is left alone
        QFont tierFont = pTier->font();
        tierFont.setCapitalization(QFont::AllUppercase);
        tierFont.setLetterSpacing(QFont::PercentageSpacing, scmTierLetterSpacing);
        pTier->setFont(tierFont);
        pColumnLayout->addWidget(pTier, 0, Qt::AlignHCenter);

        for (const QString& name : tier.names) {
            auto* pBanner = new uiDesign::AboutSupporterBanner(name, tier.swords, pColumn);
            mBanners.append(pBanner);
            pColumnLayout->addWidget(pBanner, 0, Qt::AlignHCenter);
        }
    }

    if (!mudlet::smSteamMode) {
        //: Button on the Supporters page that opens Mudlet's Patreon page
        auto* pPatreon = new QPushButton(tr("Support Mudlet on Patreon"), pColumn);
        pPatreon->setObjectName(qsl("aboutPatreonButton"));
        pPatreon->setProperty("aboutPrimaryButton", true);
        pPatreon->setAutoDefault(false);
        pPatreon->setDefault(false);
        pPatreon->setCursor(Qt::PointingHandCursor);
        connect(pPatreon, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl(qsl("https://www.patreon.com/mudlet")));
        });
        pColumnLayout->addSpacing(12);
        pColumnLayout->addWidget(pPatreon, 0, Qt::AlignHCenter);
    }

    pColumnLayout->addStretch(1);
    return pPage;
}

QWidget* dlgAboutDialog::buildLicensePage()
{
    // Not a scroll page: the browser scrolls itself, and nested scroll areas scroll neither well
    auto* pPage = new QWidget(mpStackedWidget_pages);
    pPage->setObjectName(qsl("aboutPage_license"));
    markAsShellSurface(pPage);
    auto* pPageLayout = new QVBoxLayout(pPage);
    pPageLayout->setContentsMargins(scmPageMarginSide, scmPageMarginTop, scmPageMarginSide, scmPageMarginBottom);
    pPageLayout->setSpacing(scmPageSpacing);

    QGroupBox* pNotice = createCard(QString(), pPage);
    pNotice->setObjectName(qsl("aboutLicenseNotice"));
    auto* pNoticeLayout = new QHBoxLayout(pNotice);
    pNoticeLayout->setContentsMargins(0, 0, 0, 0);
    pNoticeLayout->setSpacing(12);

    auto* pNoticeGlyph = new QLabel(pNotice);
    pNoticeGlyph->setObjectName(qsl("aboutLicenseNoticeGlyph"));
    pNoticeGlyph->setFixedSize(18, 18);
    pNoticeLayout->addWidget(pNoticeGlyph, 0, Qt::AlignTop);

    auto* pWords = new QWidget(pNotice);
    markAsShellSurface(pWords);
    auto* pWordsLayout = new QVBoxLayout(pWords);
    pWordsLayout->setContentsMargins(0, 2, 0, 0);
    pWordsLayout->setSpacing(6);
    //: About dialog, License page: the first line of the licence notice
    pWordsLayout->addWidget(createParagraph(tr("<b>Mudlet is free software.</b> Its own source code is released under the "
                                               "<a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.html#SEC1\">GNU General Public License version 2</a> or later."),
                                            pWords));
    /*: About dialog, License page: why the whole of Mudlet is offered under
 version 3. For non-english language versions please append a translation of
 the following to explain why the GPL is NOT reproduced in the relevant
 language: 'As only the English form is considered the official version of the
 license, the following is stated in that language:' */
    pWordsLayout->addWidget(createParagraph(tr("Because it uses components that are only compatible with version 3, the combined work you are running is offered under the GNU General Public "
                                               "License 3.0 only. That licence is reproduced below."),
                                            pWords));
    //: About dialog, License page: who wrote Mudlet first
    auto* pOrigin = createParagraph(tr("Mudlet was originally written by Heiko Köhn, KoehnHeiko@googlemail.com."), pWords);
    pOrigin->setObjectName(qsl("aboutLicenseOrigin"));
    pWordsLayout->addWidget(pOrigin);
    pNoticeLayout->addWidget(pWords, 1);
    pPageLayout->addWidget(pNotice);

    detachFromLayout(textBrowser_license);
    textBrowser_license->setParent(pPage);
    textBrowser_license->setFrameShape(QFrame::NoFrame);
    textBrowser_license->setOpenExternalLinks(true);
    textBrowser_license->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    markAsShellSurface(textBrowser_license->viewport());

    pPageLayout->addWidget(textBrowser_license, 1);
    return pPage;
}

QWidget* dlgAboutDialog::buildThirdPartyPage()
{
    QScrollArea* pPage = createScrollPage(qsl("thirdparty"));
    QWidget* pColumn = pPage->widget();
    auto* pColumnLayout = qobject_cast<QVBoxLayout*>(pColumn->layout());

    auto* pIntro = new QLabel(pColumn);
    pIntro->setObjectName(qsl("aboutThirdPartyIntro"));
    pIntro->setWordWrap(true);
    setRichText(
            pIntro,
            qsl("%1 %2").arg(
                    //: Introduction to the Third party page of the About dialog
                    tr("<b>Mudlet</b> is built upon the shoulders of other projects in the FOSS world; as well as using many GPL components we also make use of some third-party software with other "
                       "licenses."),
                    //: Second half of the introduction to the Third party page, saying that a row opens
                    tr("Open a row to read the licence as shipped.")));
    pColumnLayout->addWidget(pIntro);

    QGroupBox* pList = createCard(QString(), pColumn);
    pList->setObjectName(qsl("aboutThirdPartyList"));
    auto* pListLayout = new QVBoxLayout(pList);
    pListLayout->setContentsMargins(0, 0, 0, 0);
    pListLayout->setSpacing(0);

    bool first = true;
    for (const aboutThirdParty& component : thirdPartyComponents()) {
        if (!first) {
            pListLayout->addWidget(createSeparatorLine(pList));
        }
        first = false;

        auto* pRow = new QWidget(pList);
        pRow->setObjectName(qsl("aboutThirdPartyRow"));
        markAsShellSurface(pRow);
        auto* pRowLayout = new QVBoxLayout(pRow);
        pRowLayout->setContentsMargins(0, 0, 0, 0);
        pRowLayout->setSpacing(0);

        auto* pHead = new QWidget(pRow);
        markAsShellSurface(pHead);
        auto* pHeadLayout = new QHBoxLayout(pHead);
        pHeadLayout->setContentsMargins(0, 0, 14, 0);
        pHeadLayout->setSpacing(10);

        auto* pToggle = new QToolButton(pHead);
        pToggle->setObjectName(qsl("aboutThirdPartyToggle"));
        pToggle->setText(component.name);
        pToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        pToggle->setCheckable(true);
        pToggle->setFocusPolicy(Qt::StrongFocus);
        pToggle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        mThirdPartyToggles.append(pToggle);
        pHeadLayout->addWidget(pToggle);

        auto* pCopyright = new QLabel(pHead);
        pCopyright->setObjectName(qsl("aboutThirdPartyCopyright"));
        pCopyright->setWordWrap(true);
        pCopyright->setOpenExternalLinks(true);
        setRichText(pCopyright, component.copyright);
        pHeadLayout->addWidget(pCopyright, 1);

        auto* pKind = new QLabel(component.licenceKind, pHead);
        pKind->setObjectName(qsl("aboutThirdPartyKind"));
        pKind->setProperty("aboutChip", true);
        pHeadLayout->addWidget(pKind, 0, Qt::AlignRight | Qt::AlignVCenter);
        pRowLayout->addWidget(pHead);

        auto* pBody = new QLabel(pRow);
        pBody->setObjectName(qsl("aboutThirdPartyBody"));
        pBody->setWordWrap(true);
        pBody->setOpenExternalLinks(true);
        setRichText(pBody, component.body);
        pBody->setContentsMargins(40, 2, 14, 14);
        pBody->hide();
        connect(pToggle, &QToolButton::toggled, pBody, &QLabel::setVisible);
        pRowLayout->addWidget(pBody);

        pListLayout->addWidget(pRow);
    }
    pColumnLayout->addWidget(pList);
    pColumnLayout->addStretch(1);
    return pPage;
}

void dlgAboutDialog::showPage(const QString& key)
{
    if (!mPageIndexes.contains(key)) {
        return;
    }
    mpStackedWidget_pages->setCurrentIndex(mPageIndexes.value(key));
    if (QToolButton* pButton = mNavButtons.value(key, nullptr); pButton && !pButton->isChecked()) {
        pButton->setChecked(true);
    }
    if (auto* pScrollArea = qobject_cast<QScrollArea*>(mpStackedWidget_pages->currentWidget()); pScrollArea) {
        pScrollArea->verticalScrollBar()->setValue(0);
    }
}

QList<QPair<QString, QString>> dlgAboutDialog::buildInfoRows()
{
    QList<QPair<QString, QString>> rows;
    //: Key of the build-information row naming which Mudlet this is
    rows.append({tr("Version"), MudletApp::scmVersion()});
    //: Key of the build-information row naming the operating system
    rows.append({tr("OS"), QSysInfo::prettyProductName()});
#if defined(Q_OS_WINDOWS)
    // We only support 64-bit now on Windows but retain what we used to use when
    // we did 32 as well for consistency
    //: Key of the build-information row naming the processor, on Windows
    rows.append({tr("CPU (64-bits)"), QSysInfo::currentCpuArchitecture()});
#else
    //: This is shown for all other OSes than Windows.
    rows.append({tr("CPU"), QSysInfo::currentCpuArchitecture()});
#endif
    if (Q_UNLIKELY(QLatin1String(qVersion()) != QLatin1String(QT_VERSION_STR))) {
        /*: This is shown when the Qt version used at run-time
 is different to that used during compilation - it is not
 the usual case.*/
        rows.append({tr("Qt version (compilation)"), QString::fromLatin1(QT_VERSION_STR)});
        /*: This is shown when the Qt version used at run-time
 is different to that used during compilation - it is not
 the usual case.*/
        rows.append({tr("Qt version (run-time)"), QString::fromLatin1(qVersion())});
    } else {
        /*: This is shown when the same Qt version is used at run-time
 as was used during compilation - it is the usual case.*/
        rows.append({tr("Qt version"), QString::fromLatin1(QT_VERSION_STR)});
    }
    return rows;
}

QString dlgAboutDialog::buildInformationText() const
{
    QStringList lines;
    for (const auto& row : buildInfoRows()) {
        lines << qsl("%1: %2").arg(row.first, row.second);
    }
    return lines.join(QChar::LineFeed);
}

void dlgAboutDialog::slot_copyBuildInformation()
{
    QApplication::clipboard()->setText(buildInformationText());

    auto* pButton = qobject_cast<QPushButton*>(sender());
    if (!pButton) {
        return;
    }
    slot_restoreCopyButton();

    mpButton_showingCopied = pButton;
    //: Shown on the Copy button for a moment after the build information has been put on the clipboard
    pButton->setText(tr("Copied"));
    pButton->setIcon(copyButtonIcon(true, themeTokens()));
    pButton->setProperty("aboutCopied", true);
    repolish(pButton);
    mpTimer_copyFeedback->start();
}

void dlgAboutDialog::slot_restoreCopyButton()
{
    mpTimer_copyFeedback->stop();
    if (!mpButton_showingCopied) {
        return;
    }
    QPushButton* pButton = mpButton_showingCopied;
    mpButton_showingCopied = nullptr;
    pButton->setText(pButton->property("aboutRestingText").toString());
    pButton->setIcon(copyButtonIcon(false, themeTokens()));
    pButton->setProperty("aboutCopied", false);
    repolish(pButton);
}

void dlgAboutDialog::slot_applyAppearance()
{
    applyShellStyle();
}

void dlgAboutDialog::resizeEvent(QResizeEvent* pEvent)
{
    QDialog::resizeEvent(pEvent);
    updateArtColumnWidth();
}

bool dlgAboutDialog::event(QEvent* pEvent)
{
    // Glyphs and artwork are rasterised for the old ratio, and nothing else re-derives a
    // pixmap once it has been set
    if (pEvent->type() == QEvent::DevicePixelRatioChange) {
        applyShellStyle();
    }
    return QDialog::event(pEvent);
}

void dlgAboutDialog::layOutLinkButtons(const int columns)
{
    if (!mpLayout_links) {
        return;
    }
    for (int index = 0; index < mLinkButtons.size(); ++index) {
        mpLayout_links->addWidget(mLinkButtons.at(index), index / columns, index % columns);
    }
    for (int column = 0; column < scmLinkColumns; ++column) {
        mpLayout_links->setColumnStretch(column, column < columns ? 1 : 0);
    }
}

void dlgAboutDialog::updateArtColumnWidth()
{
    if (!mpWidget_artColumn) {
        return;
    }
    const bool narrow = mNarrow ? width() < scmWidenAboveWidth : width() < scmNarrowBelowWidth;
    // A flag rather than testing the label for a picture: a splash that failed to
    // load would re-grid the cards on every resize
    if (narrow == mNarrow && mArtColumnPlaced) {
        return;
    }
    mNarrow = narrow;
    mArtColumnPlaced = true;

    const int padding = narrow ? scmNarrowArtColumnPadding : scmArtColumnPadding;
    mpWidget_artColumn->setFixedWidth(narrow ? scmNarrowArtColumnWidth : scmArtColumnWidth);
    mpWidget_artColumn->layout()->setContentsMargins(padding, padding, padding, padding);
    restyleArtwork(themeTokens());

    layOutLinkButtons(narrow ? scmNarrowLinkColumns : scmLinkColumns);
}

void dlgAboutDialog::restyleArtwork(const ThemeTokens& tokens)
{
    if (mSplash.isNull()) {
        return;
    }
    const int wide = artworkWidth(mNarrow);
    const qreal ratio = devicePixelRatioF();
    QImage scaled = mSplash.scaledToWidth(static_cast<int>(wide * ratio), Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(ratio);

    // A QLabel does not clip a pixmap to a stylesheet's corner radius, so the
    // rounded picture and the hairline round it are painted into the pixmap
    QPixmap rounded(scaled.size());
    rounded.setDevicePixelRatio(ratio);
    rounded.fill(Qt::transparent);
    {
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF frame(0, 0, wide, scaled.height() / ratio);
        QPainterPath corners;
        corners.addRoundedRect(frame, scmRadiusPanel, scmRadiusPanel);
        painter.setClipPath(corners);
        painter.drawImage(frame, scaled);
        painter.setClipping(false);
        painter.setPen(QPen(tokens.border, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(frame.adjusted(0.5, 0.5, -0.5, -0.5), scmRadiusPanel, scmRadiusPanel);
    }
    mudletTitleLabel->setPixmap(rounded);
    mudletTitleLabel->setFixedSize(wide, qRound(scaled.height() / ratio));
}

void dlgAboutDialog::restyleContactChips(const ThemeTokens& tokens)
{
    // Made once per file: 49 chips share three glyphs, and each would cost a load, a
    // tint, a PNG encode and a base64
    QHash<QString, QString> inlined;
    const qreal ratio = devicePixelRatioF();
    for (const ContactChip& chip : mContactChips) {
        if (!chip.pLabel) {
            continue;
        }
        auto glyph = inlined.constFind(chip.glyphFile);
        if (glyph == inlined.constEnd()) {
            glyph = inlined.insert(chip.glyphFile, inlineGlyph(glyphAt(chip.glyphFile, scmChipGlyphSize, ratio, tokens.mutedText)));
        }
        chip.pLabel->setText(qsl("%1 %2").arg(glyph.value(), chip.text.toHtmlEscaped()));
    }
}

QIcon dlgAboutDialog::copyButtonIcon(const bool copied, const ThemeTokens& tokens) const
{
    return QIcon(copied ? tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/about-check.svg")), tokens.accentText) : tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-copy.svg")), tokens.mutedText));
}

void dlgAboutDialog::applyShellStyle()
{
    if (!mpWidget_shell) {
        return;
    }
    // From the application palette: assigning a stylesheet freezes this dialog's (see themeTokens())
    const ThemeTokens tokens = themeTokens();

    const QString cardIndicatorRules = cardIndicatorStyleSheet(scmProp_aboutCard, tokens);
    const int cardTitleHeight = measuredCardTitleHeight(mpWidget_shell, cardIndicatorRules, scmProp_aboutCard);
    const CardMetrics cardMetrics{
            .cardProperty = scmProp_aboutCard, .plainProperty = scmProp_aboutCardPlain, .padding = scmCardPadding, .titleHeight = cardTitleHeight, .flattenNestedGroupBoxes = false};

    const QString captionSize = QString::number(typeSize(TypeStep::Caption));
    const QString titleSize = QString::number(typeSize(TypeStep::Title));

    mpWidget_shell->setStyleSheet(
            qsl("#aboutShell, #aboutContent { background-color: %1; }"
                "#aboutArtColumn { background-color: %2; border-right: 1px solid %3; }"
                // So a profile's Lua stylesheet painting every QWidget cannot colour the scaffolding
                "QWidget[settingsSurface=\"true\"] { background-color: transparent; border: none; }"
                "#aboutName { font-size: %5pt; font-weight: bold; }"
                "#aboutVersion { font-size: %6pt; color: %4; }"
                "#aboutCopyright { font-size: %6pt; color: %4; }"
                "#aboutFooter { font-size: %6pt; color: %4; }"
                "#textBrowser_license { background-color: %1; border: none; }")
                    .arg(tokens.page.name(), tokens.pane.name(), tokens.separator.name(), tokens.mutedText.name(), titleSize, captionSize)
            + qsl("QLabel[aboutChip=\"true\"] { border: 1px solid %1; border-radius: %2px; padding: 1px 7px; font-size: %9pt; color: %3; }"
                  "QLabel[aboutChipLit=\"true\"] { background-color: %4; color: %5; border: 1px solid transparent; }"
                  "#aboutNav { border-bottom: 1px solid %1; }"
                  "#aboutNav QToolButton { border: 1px solid transparent; border-radius: 6px; padding: 6px 12px; color: %3; background-color: transparent; }"
                  "#aboutNav QToolButton:hover { background-color: %6; color: %7; }"
                  "#aboutNav QToolButton:checked { background-color: %4; color: %5; font-weight: bold; }"
                  "#aboutNav QToolButton:focus { border: 1px solid %8; }")
                      .arg(tokens.border.name(),
                           QString::number(scmRadiusChip),
                           tokens.mutedText.name(),
                           tokens.accentSoft,
                           tokens.accentText.name(),
                           tokens.hoverSoft,
                           tokens.text.name(),
                           tokens.accent.name(),
                           captionSize)
            + cardStyleSheet(cardMetrics, tokens) + cardIndicatorRules
            + qsl("#aboutSectionTitle { font-size: %6pt; font-weight: bold; }"
                  "#aboutSectionNote { font-size: %7pt; color: %1; }"
                  "#aboutCardDescription { font-size: %7pt; color: %1; }"
                  "#aboutPersonDescription { color: %1; }"
                  "#aboutMoreName { font-weight: bold; }"
                  "#aboutMoreDescription { color: %1; }"
                  "#aboutThanksParagraph { color: %1; }"
                  "#aboutSupportersIntro { color: %1; }"
                  "#aboutTier { font-size: %7pt; color: %1; }"
                  "#aboutThirdPartyIntro { color: %1; }"
                  "#aboutThirdPartyCopyright { font-size: %7pt; color: %1; }"
                  "#aboutThirdPartyBody { font-size: %7pt; color: %1; }"
                  "#aboutLicenseOrigin { font-size: %7pt; color: %1; }"
                  "#aboutBuildKey { font-size: %7pt; color: %1; }"
                  "#aboutBuildValue { font-size: %7pt; color: %2; }"
                  "#aboutSeparatorLine { background-color: %3; }"
                  "#aboutThirdPartyToggle { border: 1px solid transparent; padding: 9px 14px; text-align: left; background-color: transparent; color: %2; font-weight: bold; }"
                  "#aboutThirdPartyToggle:hover { background-color: %4; }"
                  "#aboutThirdPartyToggle:focus { border: 1px solid %5; }")
                      .arg(tokens.mutedText.name(), tokens.text.name(), tokens.border.name(), tokens.hoverSoft, tokens.accent.name(), titleSize, captionSize)
            + buttonStyleSheet(tokens) + qsl("QPushButton[aboutButton=\"true\"][aboutCopied=\"true\"] { color: %1; border: 1px solid %2; }").arg(tokens.accentText.name(), tokens.accent.name())
            + qsl("QPushButton[aboutPrimaryButton=\"true\"] { background-color: %1; color: %2; border: 1px solid transparent; border-radius: %3px; padding: 5px 16px;"
                  " min-height: %4px; font-weight: bold; }"
                  "QPushButton[aboutPrimaryButton=\"true\"]:hover { background-color: %5; }"
                  "QPushButton[aboutPrimaryButton=\"true\"]:focus { border: 1px solid %6; }")
                      .arg(tokens.accentSoft, tokens.accentText.name(), QString::number(scmRadiusInput), QString::number(scmPrimaryButtonHeight), tokens.hoverSoft, tokens.accent.name())
            + scrollBarStyleSheet(qsl("QScrollArea[settingsSurface=\"true\"]"), tokens) + scrollBarStyleSheet(qsl("#textBrowser_license"), tokens));

    keepClickFocusOffControls(mpWidget_shell);

    // After the sheet: assigning a stylesheet re-polishes the subtree back to the
    // palette it was first polished with, discarding any ink written before it
    for (QToolButton* pButton : std::as_const(mNavButtons)) {
        pButton->setIcon(tintedIcon(pButton->property("aboutNavGlyph").toString(), tokens));
    }
    for (QToolButton* pToggle : mThirdPartyToggles) {
        pToggle->setIcon(tintedIcon(qsl(":/icons/about-chevron-right.svg"), qsl(":/icons/about-chevron-down.svg"), tokens));
    }
    for (uiDesign::AboutLinkButton* pLink : mLinkButtons) {
        pLink->applyTokens(tokens);
    }
    for (uiDesign::AboutSupporterBanner* pBanner : mBanners) {
        pBanner->applyTokens(tokens);
    }
    for (QPushButton* pButton : mCopyButtons) {
        pButton->setIcon(copyButtonIcon(pButton == mpButton_showingCopied, tokens));
    }
    restyleContactChips(tokens);
    restyleArtwork(tokens);
    if (auto* pNoticeGlyph = findChild<QLabel*>(qsl("aboutLicenseNoticeGlyph")); pNoticeGlyph) {
        pNoticeGlyph->setPixmap(glyphAt(qsl(":/icons/about-mudlet.svg"), scmAboutLicenceGlyphSize, devicePixelRatioF(), tokens.accentText));
    }
    if (auto* pPatreon = findChild<QPushButton*>(qsl("aboutPatreonButton")); pPatreon) {
        pPatreon->setIcon(QIcon(tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/about-patreon.png")), tokens.accentText)));
    }

    // Only when it moved: re-parsing eighty labels for the colour they already
    // carry is the costliest step of construction
    if (tokens.accentText != mInkedLinkColour) {
        restyleRichText(tokens);
    }

    // Half a megabyte of rich text: only re-parsed when one of its head's three inks changed
    const QString licenceInk = qsl("%1/%2/%3").arg(tokens.text.name(), tokens.mutedText.name(), tokens.accentText.name());
    if (licenceInk != mLicenceInkKey) {
        mLicenceInkKey = licenceInk;
        setLicenseText(tokens);
    }
    update();
}

void dlgAboutDialog::setLicenseText(const ThemeTokens& tokens)
{
    // h2 to h4 name the body size outright: Qt's rich text engine otherwise gives each
    // heading a larger size of its own
    const QString htmlHead = qsl("<head><style type=\"text/css\">"
                                 "body { margin: 0; }"
                                 "h1 { text-align: center; font-size: %4pt; font-weight: bold; color: %1; }"
                                 "h2 { text-align: center; font-size: %5pt; font-weight: bold; color: %2; }"
                                 "h3 { text-align: center; font-size: %5pt; font-weight: bold; color: %2; }"
                                 "h4 { font-size: %5pt; font-weight: bold; color: %1; }"
                                 "p { color: %1; }"
                                 "li { color: %1; }"
                                 "a { color: %3; }"
                                 "tt { white-space: pre-wrap; }"
                                 "</style></head>")
                                     .arg(tokens.text.name(), tokens.mutedText.name(), tokens.accentText.name(), QString::number(typeSize(TypeStep::Title)), QString::number(typeSize(TypeStep::Body)));

    // clang-format off
    /* Only the introductory text at the top is to be translated - the Licence
     * itself MUST NOT be translated as only the English Language version is
     * legally definitive - any translations are NOT so:
     */

    /* IMPORTANT: The Lua-code-formatter has a GPL 3.0 ONLY LICENCE - THAT and some Apache 2.0
     * third-party components means that the Mudlet application must also have a GPL 3.0 only
     * (not lower and not higher) licence when all parts are packaged together in an installable
     * form or a Linux AppImage.*/

    QString gplText(qsl("<h1>GNU GENERAL PUBLIC LICENSE</h1>"
                          "<h2>Version 3, 29 June 2007</h2>"
                          "<h3 style='text-align: center;'>Copyright © 2007 Free Software Foundation, Inc.<br>"
                          "<a href=\"https://fsf.org\">https://fsf.org</a></h3>"
                          "<p>Everyone is permitted to copy and distribute verbatim copies of this license "
                          "document, but changing it is not allowed.</p>"
                          "<h4>Preamble</h4>"
                          "<p>The GNU General Public License is a free, copyleft license for software and "
                          "other kinds of works.</p>"
                          "<p>The licenses for most software and other practical works are designed to take away your freedom to "
                          "share and change the works. By contrast, the GNU General Public License is intended to "
                          "guarantee your freedom to share and change free software--to make sure the "
                          "software is free for all its users. We, the Free Software Foundation, use the "
                          "GNU General Public License for most of our software; it applies also to any "
                          "other work released this way by its authors. You can apply it to your programs, "
                          "too.</p>"
                          "<p>When we speak of free software, we are referring to freedom, not price. Our "
                          "General Public Licenses are designed to make sure that you have the freedom to "
                          "distribute copies of free software (and charge for this service if you wish), "
                          "that you receive source code or can get it if you want it, that you can change "
                          "the software or use pieces of it in new free programs; and that you know you can "
                          "do these things.</p>"
                          "<p>To protect your rights, we need to prevent others from denying you these "
                          "rights or asking you to surrender the rights. Therefore, you have certain "
                          "responsibilities if you distribute copies of the software, or if you modify it: "
                          "responsibilities to respect the freedom of others.</p>"
                          "<p>For example, if you distribute copies of such a program, whether gratis or "
                          "for a fee, you must pass on to the recipients the same freedoms that you "
                          "received. You must make sure that they, too, receive or can get the source code. "
                          "And you must show them these terms so they know their rights.</p>"
                          "<p>Developers that use the GNU GPL protect your rights with two steps: (1) "
                          "assert copyright on the software, and (2) offer you this License giving you "
                          "legal permission to copy, distribute and/or modify it.</p>"
                          "<p>For the developers' and authors' protection, the GPL clearly explains that "
                          "there is no warranty for this free software. For both users' and authors' sake, "
                          "the GPL requires that modified versions be marked as changed, so that their "
                          "problems will not be attributed erroneously to authors of previous versions.</p>"
                          "<p>Some devices are designed to deny users access to install or run modified "
                          "versions of the software inside them, although the manufacturer can do so. This "
                          "is fundamentally incompatible with the aim of protecting users' freedom to "
                          "change the software. The systematic pattern of such abuse occurs in the area of "
                          "products for individuals to use, which is precisely where it is most "
                          "unacceptable. Therefore, we have designed this version of the GPL to prohibit "
                          "the practice for those products. If such problems arise substantially in other "
                          "domains, we stand ready to extend this provision to those domains in future "
                          "versions of the GPL, as needed to protect the freedom of users.</p>"
                          "<p>Finally, every program is threatened constantly by software patents. States "
                          "should not allow patents to restrict development and use of software on "
                          "general-purpose computers, but in those that do, we wish to avoid the special "
                          "danger that patents applied to a free program could make it effectively "
                          "proprietary. To prevent this, the GPL assures that patents cannot be used to "
                          "render the program non-free.</p>"
                          "<p>The precise terms and conditions for copying, distribution and modification "
                          "follow.</p>"
                          "<h2 style='text-align: left;'>TERMS AND CONDITIONS</h2>"
                          "<h3 style='text-align: left;'>0. Definitions.</h3>"
                          "<p>“This License” refers to version 3 of the GNU General Public License.</p>"
                          "<p>“Copyright” also means copyright-like laws that apply to other kinds of works, such "
                          "as semiconductor masks.</p>"
                          "<p>“The Program” refers to any copyrightable work licensed under this License. Each "
                          "licensee is addressed as “you”. “Licensees” and “recipients” may be individuals or "
                          "organizations.</p>"
                          "<p>To “modify” a work means to copy from or adapt all or part of the work in a fashion "
                          "requiring copyright permission, other than the making of an exact copy. The resulting "
                          "work is called a “modified version” of the earlier work or a work “based on” the "
                          "earlier work.</p>"
                          "<p>A “covered work” means either the unmodified Program or a work based on the Program.</p>"
                          "<p>To “propagate” a work means to do anything with it that, without permission, would make "
                          "you directly or secondarily liable for infringement under applicable copyright law, "
                          "except executing it on a computer or modifying a private copy. Propagation includes "
                          "copying, distribution (with or without modification), making available to the public, and "
                          "in some countries other activities as well.</p>"
                          "<p>To “convey” a work means any kind of propagation that enables other parties to make "
                          "or receive copies. Mere interaction with a user through a computer network, with no "
                          "transfer of a copy, is not conveying.</p>"
                          "<p>An interactive user interface displays “Appropriate Legal Notices” to the extent that "
                          "it includes a convenient and prominently visible feature that (1) displays an appropriate "
                          "copyright notice, and (2) tells the user that there is no warranty for the work (except "
                          "to the extent that warranties are provided), that licensees may convey the work under "
                          "this License, and how to view a copy of this License. If the interface presents a list of "
                          "user commands or options, such as a menu, a prominent item in the list meets this "
                          "criterion.</p>"
                          "<h3 style='text-align: left;'>1. Source Code.</h3>"
                          "<p>The “source code” for a work means the preferred form of the work for making "
                          "modifications to it. “Object code” means any non-source form of a work.</p>"
                          "<p>A “Standard Interface” means an interface that either is an official standard "
                          "defined by a recognized standards body, or, in the case of interfaces specified "
                          "for a particular programming language, one that is widely used among developers "
                          "working in that language.</p>"
                          "<p>The “System Libraries” of an executable work include anything, other than the "
                          "work as a whole, that (a) is included in the normal form of packaging a Major "
                          "Component, but which is not part of that Major Component, and (b) serves only to "
                          "enable use of the work with that Major Component, or to implement a Standard "
                          "Interface for which an implementation is available to the public in source code "
                          "form. A “Major Component”, in this context, means a major essential component "
                          "(kernel, window system, and so on) of the specific operating system (if any) on "
                          "which the executable work runs, or a compiler used to produce the work, or an "
                          "object code interpreter used to run it.</p>"
                          "<p>The “Corresponding Source” for a work in object code form means all the "
                          "source code needed to generate, install, and (for an executable work) run the "
                          "object code and to modify the work, including scripts to control those "
                          "activities. However, it does not include the work's System Libraries, or "
                          "general-purpose tools or generally available free programs which are used "
                          "unmodified in performing those activities but which are not part of the work. "
                          "For example, Corresponding Source includes interface definition files associated "
                          "with source files for the work, and the source code for shared libraries and "
                          "dynamically linked subprograms that the work is specifically designed to "
                          "require, such as by intimate data communication or control flow between those "
                          "subprograms and other parts of the work.</p>"
                          "<p>The Corresponding Source need not include anything that users can regenerate "
                          "automatically from other parts of the Corresponding Source.</p>"
                          "<p>The Corresponding Source for a work in source code form is that same work.</p>"
                          "<h3 style='text-align: left;'>2. Basic Permissions.</h3>"
                          "<p>All rights granted under this License are granted for the term of copyright "
                          "on the Program, and are irrevocable provided the stated conditions are met. This "
                          "License explicitly affirms your unlimited permission to run the unmodified "
                          "Program. The output from running a covered work is covered by this License only "
                          "if the output, given its content, constitutes a covered work. This License "
                          "acknowledges your rights of fair use or other equivalent, as provided by "
                          "copyright law.</p>"
                          "<p>You may make, run and propagate covered works that you do not convey, without "
                          "conditions so long as your license otherwise remains in force. You may convey "
                          "covered works to others for the sole purpose of having them make modifications "
                          "exclusively for you, or provide you with facilities for running those works, "
                          "provided that you comply with the terms of this License in conveying all "
                          "material for which you do not control copyright. Those thus making or running "
                          "the covered works for you must do so exclusively on your behalf, under your "
                          "direction and control, on terms that prohibit them from making any copies of "
                          "your copyrighted material outside their relationship with you.</p>"
                          "<p>Conveying under any other circumstances is permitted solely under the "
                          "conditions stated below. Sublicensing is not allowed; section 10 makes it "
                          "unnecessary.</p>"
                          "<h3 style='text-align: left;'>3. Protecting Users' Legal Rights From Anti-Circumvention Law.</h3>"
                          "<p>No covered work shall be deemed part of an effective technological measure "
                          "under any applicable law fulfilling obligations under article 11 of the WIPO "
                          "copyright treaty adopted on 20 December 1996, or similar laws prohibiting or "
                          "restricting circumvention of such measures.</p>"
                          "<p>When you convey a covered work, you waive any legal power to forbid "
                          "circumvention of technological measures to the extent such circumvention is "
                          "effected by exercising rights under this License with respect to the covered "
                          "work, and you disclaim any intention to limit operation or modification of the "
                          "work as a means of enforcing, against the work's users, your or third parties' "
                          "legal rights to forbid circumvention of technological measures.</p>"
                          "<h3 style='text-align: left;'>4. Conveying Verbatim Copies.</h3>"
                          "<p>You may convey verbatim copies of the Program's source code as you receive "
                          "it, in any medium, provided that you conspicuously and appropriately publish on "
                          "each copy an appropriate copyright notice; keep intact all notices stating that "
                          "this License and any non-permissive terms added in accord with section 7 apply "
                          "to the code; keep intact all notices of the absence of any warranty; and give "
                          "all recipients a copy of this License along with the Program.</p>"
                          "<p>You may charge any price or no price for each copy that you convey, and you "
                          "may offer support or warranty protection for a fee.</p>"
                          "<h3 style='text-align: left;'>5. Conveying Modified Source Versions.</h3>"
                          "<p>You may convey a work based on the Program, or the modifications to produce "
                          "it from the Program, in the form of source code under the terms of section 4, "
                          "provided that you also meet all of these conditions:</p>"
                          "<ul><li>a) The work must carry prominent notices stating that you modified it, and "
                          "giving a relevant date.</li>"
                          "<li>b) The work must carry prominent notices stating that it is released under "
                          "this License and any conditions added under section 7. This requirement modifies "
                          "the requirement in section 4 to “keep intact all notices”.</li>"
                          "<li>c) You must license the entire work, as a whole, under this License to "
                          "anyone who comes into possession of a copy. This License will therefore apply, "
                          "along with any applicable section 7 additional terms, to the whole of the work, "
                          "and all its parts, regardless of how they are packaged. This License gives no "
                          "permission to license the work in any other way, but it does not invalidate such "
                          "permission if you have separately received it.</li>"
                          "<li>d) If the work has interactive user interfaces, each must display Appropriate "
                          "Legal Notices; however, if the Program has interactive interfaces that do not "
                          "display Appropriate Legal Notices, your work need not make them do so.</li></ul>"
                          "<p>A compilation of a covered work with other separate and independent works, "
                          "which are not by their nature extensions of the covered work, and which are not "
                          "combined with it such as to form a larger program, in or on a volume of a "
                          "storage or distribution medium, is called an “aggregate” if the compilation and "
                          "its resulting copyright are not used to limit the access or legal rights of the "
                          "compilation's users beyond what the individual works permit. Inclusion of a "
                          "covered work in an aggregate does not cause this License to apply to the other "
                          "parts of the aggregate.</p>"
                          "<h3 style='text-align: left;'>6. Conveying Non-Source Forms.</h3>"
                          "<p>You may convey a covered work in object code form under the terms of sections "
                          "4 and 5, provided that you also convey the machine-readable Corresponding Source "
                          "under the terms of this License, in one of these ways:</p>"
                          "<ul><li>a) Convey the object code in, or embodied in, a physical product (including "
                          "a physical distribution medium), accompanied by the Corresponding Source fixed "
                          "on a durable physical medium customarily used for software interchange.</li>"
                          "<li>b) Convey the object code in, or embodied in, a physical product (including a "
                          "physical distribution medium), accompanied by a written offer, valid for at "
                          "least three years and valid for as long as you offer spare parts or customer "
                          "support for that product model, to give anyone who possesses the object code "
                          "either (1) a copy of the Corresponding Source for all the software in the "
                          "product that is covered by this License, on a durable physical medium "
                          "customarily used for software interchange, for a price no more than your "
                          "reasonable cost of physically performing this conveying of source, or (2) access "
                          "to copy the Corresponding Source from a network server at no charge.</li>"
                          "<li>c) Convey individual copies of the object code with a copy of the written "
                          "offer to provide the Corresponding Source. This alternative is allowed only "
                          "occasionally and noncommercially, and only if you received the object code with "
                          "such an offer, in accord with subsection 6b.</li>"
                          "<li>d) Convey the object code by offering access from a designated place (gratis "
                          "or for a charge), and offer equivalent access to the Corresponding Source in the "
                          "same way through the same place at no further charge. You need not require "
                          "recipients to copy the Corresponding Source along with the object code. If the "
                          "place to copy the object code is a network server, the Corresponding Source may "
                          "be on a different server (operated by you or a third party) that supports "
                          "equivalent copying facilities, provided you maintain clear directions next to "
                          "the object code saying where to find the Corresponding Source. Regardless of "
                          "what server hosts the Corresponding Source, you remain obligated to ensure that "
                          "it is available for as long as needed to satisfy these requirements.</li>"
                          "<li>e) Convey the object code using peer-to-peer transmission, provided you "
                          "inform other peers where the object code and Corresponding Source of the work "
                          "are being offered to the general public at no charge under subsection 6d.</li></ul>"
                          "<p>A separable portion of the object code, whose source code is excluded from "
                          "the Corresponding Source as a System Library, need not be included in conveying "
                          "the object code work.</p>"
                          "<p>A “User Product” is either (1) a “consumer product”, which means any tangible "
                          "personal property which is normally used for personal, family, or household "
                          "purposes, or (2) anything designed or sold for incorporation into a dwelling. In "
                          "determining whether a product is a consumer product, doubtful cases shall be "
                          "resolved in favor of coverage. For a particular product received by a particular "
                          "user, “normally used” refers to a typical or common use of that class of "
                          "product, regardless of the status of the particular user or of the way in which "
                          "the particular user actually uses, or expects or is expected to use, the "
                          "product. A product is a consumer product regardless of whether the product has "
                          "substantial commercial, industrial or non-consumer uses, unless such uses "
                          "represent the only significant mode of use of the product.</p>"
                          "<p>“Installation Information” for a User Product means any methods, procedures, "
                          "authorization keys, or other information required to install and execute "
                          "modified versions of a covered work in that User Product from a modified version "
                          "of its Corresponding Source. The information must suffice to ensure that the "
                          "continued functioning of the modified object code is in no case prevented or "
                          "interfered with solely because modification has been made.</p>"
                          "<p>If you convey an object code work under this section in, or with, or "
                          "specifically for use in, a User Product, and the conveying occurs as part of a "
                          "transaction in which the right of possession and use of the User Product is "
                          "transferred to the recipient in perpetuity or for a fixed term (regardless of "
                          "how the transaction is characterized), the Corresponding Source conveyed under "
                          "this section must be accompanied by the Installation Information. But this "
                          "requirement does not apply if neither you nor any third party retains the "
                          "ability to install modified object code on the User Product (for example, the "
                          "work has been installed in ROM).</p>"
                          "<p>The requirement to provide Installation Information does not include a "
                          "requirement to continue to provide support service, warranty, or updates for a "
                          "work that has been modified or installed by the recipient, or for the User "
                          "Product in which it has been modified or installed. Access to a network may be "
                          "denied when the modification itself materially and adversely affects the "
                          "operation of the network or violates the rules and protocols for communication "
                          "across the network.</p>"
                          "<p>Corresponding Source conveyed, and Installation Information provided, in "
                          "accord with this section must be in a format that is publicly documented (and "
                          "with an implementation available to the public in source code form), and must "
                          "require no special password or key for unpacking, reading or copying."
                          "<h3 style='text-align: left;'>7. Additional Terms.</h3>"
                          "<p>“Additional permissions” are terms that supplement the terms of this License "
                          "by making exceptions from one or more of its conditions. Additional permissions "
                          "that are applicable to the entire Program shall be treated as though they were "
                          "included in this License, to the extent that they are valid under applicable "
                          "law. If additional permissions apply only to part of the Program, that part may "
                          "be used separately under those permissions, but the entire Program remains "
                          "governed by this License without regard to the additional permissions.</p>"
                          "<p>When you convey a copy of a covered work, you may at your option remove any "
                          "additional permissions from that copy, or from any part of it. (Additional "
                          "permissions may be written to require their own removal in certain cases when "
                          "you modify the work.) You may place additional permissions on material, added by "
                          "you to a covered work, for which you have or can give appropriate copyright "
                          "permission.</p>"
                          "<p>Notwithstanding any other provision of this License, for material you add to "
                          "a covered work, you may (if authorized by the copyright holders of that "
                          "material) supplement the terms of this License with terms:.</p>"
                          "<ul><li>a) Disclaiming warranty or limiting liability differently from the terms "
                          "of sections 15 and 16 of this License; or</li>"
                          "<li>b) Requiring preservation of specified reasonable legal notices or author "
                          "attributions in that material or in the Appropriate Legal Notices displayed by "
                          "works containing it; or</li>"
                          "<li>c) Prohibiting misrepresentation of the origin of that material, or "
                          "requiring that modified versions of such material be marked in reasonable ways "
                          "as different from the original version; or</li>"
                          "<li>d) Limiting the use for publicity purposes of names of licensors or authors "
                          "of the material; or</li>"
                          "<li>e) Declining to grant rights under trademark law for use of some trade "
                          "names, trademarks, or service marks; or</li>"
                          "<li>f) Requiring indemnification of licensors and authors of that material by "
                          "anyone who conveys the material (or modified versions of it) with contractual "
                          "assumptions of liability to the recipient, for any liability that these "
                          "contractual assumptions directly impose on those licensors and authors.</li></ul>"
                          "<p>All other non-permissive additional terms are considered “further "
                          "restrictions” within the meaning of section 10. If the Program as you received "
                          "it, or any part of it, contains a notice stating that it is governed by this "
                          "License along with a term that is a further restriction, you may remove that "
                          "term. If a license document contains a further restriction but permits "
                          "relicensing or conveying under this License, you may add to a covered work "
                          "material governed by the terms of that license document, provided that the "
                          "further restriction does not survive such relicensing or conveying.</p>"
                          "<p>If you add terms to a covered work in accord with this section, you must "
                          "place, in the relevant source files, a statement of the additional terms that "
                          "apply to those files, or a notice indicating where to find the applicable terms.</p>"
                          "<p>Additional terms, permissive or non-permissive, may be stated in the form of "
                          "a separately written license, or stated as exceptions; the above requirements "
                          "apply either way.</p>"
                          "<h3 style='text-align: left;'>8. Termination.</h3>"
                          "<p>You may not propagate or modify a covered work except as expressly provided "
                          "under this License. Any attempt otherwise to propagate or modify it is void, and "
                          "will automatically terminate your rights under this License (including any "
                          "patent licenses granted under the third paragraph of section 11).</p>"
                          "<p>However, if you cease all violation of this License, then your license from a "
                          "particular copyright holder is reinstated (a) provisionally, unless and until "
                          "the copyright holder explicitly and finally terminates your license, and (b) "
                          "permanently, if the copyright holder fails to notify you of the violation by "
                          "some reasonable means prior to 60 days after the cessation.</p>"
                          "<p>Moreover, your license from a particular copyright holder is reinstated "
                          "permanently if the copyright holder notifies you of the violation by some "
                          "reasonable means, this is the first time you have received notice of violation "
                          "of this License (for any work) from that copyright holder, and you cure the "
                          "violation prior to 30 days after your receipt of the notice.</p>"
                          "<p>Termination of your rights under this section does not terminate the licenses "
                          "of parties who have received copies or rights from you under this License. If "
                          "your rights have been terminated and not permanently reinstated, you do not "
                          "qualify to receive new licenses for the same material under section 10.</p>"
                          "<h3 style='text-align: left;'>9. Acceptance Not Required for Having Copies.</h3>"
                          "<p>You are not required to accept this License in order to receive or run a copy "
                          "of the Program. Ancillary propagation of a covered work occurring solely as a "
                          "consequence of using peer-to-peer transmission to receive a copy likewise does "
                          "not require acceptance. However, nothing other than this License grants you "
                          "permission to propagate or modify any covered work. These actions infringe "
                          "copyright if you do not accept this License. Therefore, by modifying or "
                          "propagating a covered work, you indicate your acceptance of this License to do "
                          "so.</p>"
                          "<h3 style='text-align: left;'>10. Automatic Licensing of Downstream Recipients.</h3>"
                          "<p>Each time you convey a covered work, the recipient automatically receives a "
                          "license from the original licensors, to run, modify and propagate that work, "
                          "subject to this License. You are not responsible for enforcing compliance by "
                          "third parties with this License.</p>"
                          "<p>An “entity transaction” is a transaction transferring control of an "
                          "organization, or substantially all assets of one, or subdividing an "
                          "organization, or merging organizations. If propagation of a covered work results "
                          "from an entity transaction, each party to that transaction who receives a copy "
                          "of the work also receives whatever licenses to the work the party's predecessor "
                          "in interest had or could give under the previous paragraph, plus a right to "
                          "possession of the Corresponding Source of the work from the predecessor in "
                          "interest, if the predecessor has it or can get it with reasonable efforts.</p>"
                          "<p>You may not impose any further restrictions on the exercise of the rights "
                          "granted or affirmed under this License. For example, you may not impose a "
                          "license fee, royalty, or other charge for exercise of rights granted under this "
                          "License, and you may not initiate litigation (including a cross-claim or "
                          "counterclaim in a lawsuit) alleging that any patent claim is infringed by "
                          "making, using, selling, offering for sale, or importing the Program or any "
                          "portion of it.</p>"
                          "<h3 style='text-align: left;'>11. Patents.</h3>"
                          "<p>A “contributor” is a copyright holder who authorizes use under this License "
                          "of the Program or a work on which the Program is based. The work thus licensed "
                          "is called the contributor's “contributor version”.</p>"
                          "<p>A contributor's “essential patent claims” are all patent claims owned or "
                          "controlled by the contributor, whether already acquired or hereafter acquired, "
                          "that would be infringed by some manner, permitted by this License, of making, "
                          "using, or selling its contributor version, but do not include claims that would "
                          "be infringed only as a consequence of further modification of the contributor "
                          "version. For purposes of this definition, “control” includes the right to grant "
                          "patent sublicenses in a manner consistent with the requirements of this License.</p>"
                          "<p>Each contributor grants you a non-exclusive, worldwide, royalty-free patent "
                          "license under the contributor's essential patent claims, to make, use, sell, "
                          "offer for sale, import and otherwise run, modify and propagate the contents of "
                          "its contributor version.</p>"
                          "<p>In the following three paragraphs, a “patent license” is any express "
                          "agreement or commitment, however denominated, not to enforce a patent (such as "
                          "an express permission to practice a patent or covenant not to sue for patent "
                          "infringement). To “grant” such a patent license to a party means to make such an "
                          "agreement or commitment not to enforce a patent against the party.</p>"
                          "<p>If you convey a covered work, knowingly relying on a patent license, and the "
                          "Corresponding Source of the work is not available for anyone to copy, free of "
                          "charge and under the terms of this License, through a publicly available network "
                          "server or other readily accessible means, then you must either (1) cause the "
                          "Corresponding Source to be so available, or (2) arrange to deprive yourself of "
                          "the benefit of the patent license for this particular work, or (3) arrange, in a "
                          "manner consistent with the requirements of this License, to extend the patent "
                          "license to downstream recipients. “Knowingly relying” means you have actual "
                          "knowledge that, but for the patent license, your conveying the covered work in a "
                          "country, or your recipient's use of the covered work in a country, would "
                          "infringe one or more identifiable patents in that country that you have reason "
                          "to believe are valid.</p>"
                          "<p>If, pursuant to or in connection with a single transaction or arrangement, "
                          "you convey, or propagate by procuring conveyance of, a covered work, and grant a "
                          "patent license to some of the parties receiving the covered work authorizing "
                          "them to use, propagate, modify or convey a specific copy of the covered work, "
                          "then the patent license you grant is automatically extended to all recipients of "
                          "the covered work and works based on it.</p>"
                          "<p>A patent license is “discriminatory” if it does not include within the scope "
                          "of its coverage, prohibits the exercise of, or is conditioned on the "
                          "non-exercise of one or more of the rights that are specifically granted under "
                          "this License. You may not convey a covered work if you are a party to an "
                          "arrangement with a third party that is in the business of distributing software, "
                          "under which you make payment to the third party based on the extent of your "
                          "activity of conveying the work, and under which the third party grants, to any "
                          "of the parties who would receive the covered work from you, a discriminatory "
                          "patent license (a) in connection with copies of the covered work conveyed by you "
                          "(or copies made from those copies), or (b) primarily for and in connection with "
                          "specific products or compilations that contain the covered work, unless you "
                          "entered into that arrangement, or that patent license was granted, prior to 28 "
                          "March 2007.</p>"
                          "<p>Nothing in this License shall be construed as excluding or limiting any "
                          "implied license or other defenses to infringement that may otherwise be "
                          "available to you under applicable patent law.</p>"
                          "<h3 style='text-align: left;'>12. No Surrender of Others' Freedom.</h3>"
                          "<p>If conditions are imposed on you (whether by court order, agreement or "
                          "otherwise) that contradict the conditions of this License, they do not excuse "
                          "you from the conditions of this License. If you cannot convey a covered work so "
                          "as to satisfy simultaneously your obligations under this License and any other "
                          "pertinent obligations, then as a consequence you may not convey it at all. For "
                          "example, if you agree to terms that obligate you to collect a royalty for "
                          "further conveying from those to whom you convey the Program, the only way you "
                          "could satisfy both those terms and this License would be to refrain entirely "
                          "from conveying the Program.</p>"
                          "<h3 style='text-align: left;'>13. Use with the GNU Affero General Public License.</h3>"
                          "<p>Notwithstanding any other provision of this License, you have permission to "
                          "link or combine any covered work with a work licensed under version 3 of the GNU "
                          "Affero General Public License into a single combined work, and to convey the "
                          "resulting work. The terms of this License will continue to apply to the part "
                          "which is the covered work, but the special requirements of the GNU Affero "
                          "General Public License, section 13, concerning interaction through a network "
                          "will apply to the combination as such.</p>"
                          "<h3 style='text-align: left;'>14. Revised Versions of this License.</h3>"
                          "<p>The Free Software Foundation may publish revised and/or new versions of the "
                          "GNU General Public License from time to time. Such new versions will be similar "
                          "in spirit to the present version, but may differ in detail to address new "
                          "problems or concerns.</p>"
                          "<p>Each version is given a distinguishing version number. If the Program "
                          "specifies that a certain numbered version of the GNU General Public License “or "
                          "any later version” applies to it, you have the option of following the terms and "
                          "conditions either of that numbered version or of any later version published by "
                          "the Free Software Foundation. If the Program does not specify a version number "
                          "of the GNU General Public License, you may choose any version ever published by "
                          "the Free Software Foundation.</p>"
                          "<p>If the Program specifies that a proxy can decide which future versions of the "
                          "GNU General Public License can be used, that proxy's public statement of "
                          "acceptance of a version permanently authorizes you to choose that version for "
                          "the Program.</p>"
                          "<p>Later license versions may give you additional or different permissions. "
                          "However, no additional obligations are imposed on any author or copyright holder "
                          "as a result of your choosing to follow a later version.</p>"
                          "<h3 style='text-align: left;'>15. Disclaimer of Warranty.</h3>"
                          "<p>THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE "
                          "LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER "
                          "PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER "
                          "EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF "
                          "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE "
                          "QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE "
                          "DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.</p>"
                          "<h3 style='text-align: left;'>16. Limitation of Liability.</h3>"
                          "<p>IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL "
                          "ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM "
                          "AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, "
                          "INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE "
                          "THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED "
                          "INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE "
                          "PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY "
                          "HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.</p>"
                          "<h3 style='text-align: left;'>17. Interpretation of Sections 15 and 16.</h3>"
                          "<p>If the disclaimer of warranty and limitation of liability provided above "
                          "cannot be given local legal effect according to their terms, reviewing courts "
                          "shall apply local law that most closely approximates an absolute waiver of all "
                          "civil liability in connection with the Program, unless a warranty or assumption "
                          "of liability accompanies a copy of the Program in return for a fee.</p>"
                          "<p>END OF TERMS AND CONDITIONS</p>"
                          "<h2 style='text-align: left;'>How to Apply These Terms to Your New Programs</h2>"
                          "<p>If you develop a new program, and you want it to be of the greatest possible "
                          "use to the public, the best way to achieve this is to make it free software "
                          "which everyone can redistribute and change under these terms.</p>"
                          "<p>To do so, attach the following notices to the program. It is safest to attach "
                          "them to the start of each source file to most effectively state the exclusion of "
                          "warranty; and each file should have at least the “copyright” line and a pointer "
                          "to where the full notice is found.</p>"
                          "<p style='text-align: center;'><tt>&lt;one line to give the program's name and a brief idea of what it does.&gt;<br>"
                          "Copyright (C) &lt;year&gt;  &lt;name of author&gt;</tt></p>"
                          "<p style='text-align: center;'><tt>This program is free software: you can redistribute it and/or modify<br>"
                          "it under the terms of the GNU General Public License as published by<br>"
                          "the Free Software Foundation, either version 3 of the License, or<br>"
                          "(at your option) any later version.</tt></p>"
                          "<p style='text-align: center;'><tt>This program is distributed in the hope that it will be useful,<br>"
                          "but WITHOUT ANY WARRANTY; without even the implied warranty of<br>"
                          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the<br>"
                          "GNU General Public License for more details.</tt></p>"
                          "<p style='text-align: center;'><tt>You should have received a copy of the GNU General Public License<br>"
                          "along with this program.  If not, see &lt;https://www.gnu.org/licenses/&gt;.</tt>"
                          "<p>Also add information on how to contact you by electronic and paper mail.</p>"
                          "<p>If the program does terminal interaction, make it output a short notice like "
                          "this when it starts in an interactive mode:</p>"
                          "<p style='text-align: center;'><tt>&lt;program&gt;  Copyright (C) &lt;year&gt;  &lt;name of author&gt;<br>"
                          "This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.<br>"
                          "This is free software, and you are welcome to redistribute it<br>"
                          "under certain conditions; type `show c' for details.</tt></p>"
                          "<p>The hypothetical commands `show w' and `show c' should show the appropriate "
                          "parts of the General Public License. Of course, your program's commands might be "
                          "different; for a GUI interface, you would use an “about box”.</p>"
                          "<p>You should also get your employer (if you work as a programmer) or school, if "
                          "any, to sign a “copyright disclaimer” for the program, if necessary. For more "
                          "information on this, and how to apply and follow the GNU GPL, see &lt;<a href=\"https://www.gnu.org/licenses/\">https://www.gnu.org/licenses/</a>&gt;.</p>"
                          "<p>The General Public License does not permit incorporating your program into "
                          "proprietary programs. If your program is a subroutine library, you may "
                          "consider it more useful to permit linking proprietary applications with the "
                          "library. If this is what you want to do, use the GNU Lesser General Public "
                          "instead of this License. But first, please read "
                          "&lt;<a href=\"https://www.gnu.org/licenses/why-not-lgpl.html\">https://www.gnu.org/licenses/why-not-lgpl.html</a>&gt;.</p>"));
    // clang-format on

    // A theme change must not throw the reader back to the top
    const int wasAt = textBrowser_license->verticalScrollBar()->value();
    // A QTextDocument ignores the head's "a { color }" rule when inking anchors
    textBrowser_license->setHtml(qsl("<html>%1<body>%2</body></html>").arg(htmlHead, uiDesign::withLinkColour(gplText, tokens.accentText)));
    textBrowser_license->verticalScrollBar()->setValue(wasAt);
}

QVector<aboutMaker> dlgAboutDialog::makers()
{
    // clang-format off
    // theme-fixed: a Discord handle carries a # and four digits, which reads as a hex colour and is not one
    QVector<aboutMaker> aboutMakers; // [big?, name, discord, github, email, description]
    aboutMakers.append({true, qsl("Heiko Köhn"), QString(), QString(), qsl("KoehnHeiko@googlemail.com"),
                        //: about:Heiko
                        tr("Original author, original project lead, Mudlet core coding, retired.")});
    aboutMakers.append({true, qsl("Vadim Peretokin"), qsl("Vadi#3695"), qsl("vadi2"), qsl("vadim.peretokin@mudlet.org"),
                        //: about:Vadi
                        tr("GUI design and initial feature planning. He is responsible for the project homepage and the user manual. "
                           "Maintainer of the Windows, macOS, Ubuntu and generic Linux installers. "
                           "Maintains the Mudlet wiki, Lua API, and handles project management, public relations &amp; user help. "
                           "With the project from the very beginning and is an official spokesman of the project. "
                           "Since the retirement of Heiko, he has become the head of the Mudlet project.")});
    aboutMakers.append({true, qsl("Stephen Lyons"), qsl("SlySven#2703"), qsl("SlySven"), qsl("slysven@virginmedia.com"),
                        //: about:SlySven
                        tr("After joining in 2013, he has been poking various bits of the C++ code and GUI with a pointy stick; "
                           "subsequently trying to patch over some of the holes made/found. "
                           "Most recently he has been working on I18n and L10n for Mudlet 4.0.0 so if you are playing Mudlet in a language "
                           "other than American English you will be seeing the results of him getting fed up with the spelling differences "
                           "between what was being used and the British English his brain wanted to see.")});
    aboutMakers.append({true, qsl("Damian Monogue"), qsl("demonnic#4307"), qsl("demonnic"), qsl("demonnic@gmail.com"),
                        //: about:demonnic
                        tr("Former maintainer of the early Windows and Apple OSX packages. "
                           "He also administers our server and helps the project in many ways.")});
    aboutMakers.append({true, qsl("Florian Scheel"), qsl("keneanung#2803"), qsl("keneanung"), qsl("keneanung@googlemail.com"),
                        //: about:keneanung
                        tr("Contributed many improvements to Mudlet's db: interface, event system, "
                           "and has been around the project for a very long while assisting users.")});
    aboutMakers.append({true, qsl("Leris"), qsl("Leris#5152"), qsl("Kebap"), qsl("kebap_spam@gmx.net"),
                        //: about:Leris
                        tr("Does a ton of work in making Mudlet, the website and the wiki accessible to you "
                           "regardless of the language you speak - and promoting our genre!")});
    aboutMakers.append({true, qsl("Piotr Wilczynski"), QString(), qsl("Delwing"), qsl("delwing@gmail.com"),
                        //: about:Delwing
                        tr("Joined in 2020, reworking much of the 2D mapper and adding many Lua API features. "
                           "Outside the client they build Mudlet Web, the documentation extract that powers "
                           "autocompletion in code editors, and the tools that share Mudlet maps online.")});
    aboutMakers.append({true, qsl("Zooka"), QString(), qsl("ZookaOnGit"), QString(),
                        //: about:Zooka
                        tr("Joined in 2023 and works across the whole client - script editor, preferences, package manager "
                           "and mapper - along with many Lua API additions. Wrote the Mudlet Tutorial profile and "
                           "maintains the Mudlet package repository.")});
    aboutMakers.append({false, qsl("Ahmed Charles"), QString(), qsl("ahmedcharles"), qsl("acharles@outlook.com"),
                        //: about:ahmedcharles
                        tr("Contributions to the Travis integration, CMake and Visual C++ build, "
                           "a lot of code quality and memory management improvements.")});
    aboutMakers.append({false, qsl("Chris Mitchell"), qsl("Chris7#6113"), qsl("Chris7"), qsl("chris.mit7@gmail.com"),
                        //: about:Chris7
                        tr("Developed a shared module system that allows script packages to be shared among profiles, "
                           "a UI for viewing Lua variables, improvements in the mapper and all around.")});
    aboutMakers.append({false, qsl("Ben Carlsen"), QString(), QString(), qsl("arkholt@gmail.com"),
                        //: about:Ben Carlsen
                        tr("Developed the first version of our Mac OSX installer. "
                           "He is the former maintainer of the Mac version of Mudlet.")});
    aboutMakers.append({false, qsl("Ben Smith"), QString(), QString(), QString(),
                        //: about:Ben Smith
                        tr("Joined in December 2009 though he's been around much longer. "
                           "Contributed to the Lua API and is the former maintainer of the Lua API.")});
    aboutMakers.append({false, qsl("Blaine von Roeder"), QString(), QString(), QString(),
                        //: about:Blaine von Roeder
                        tr("Joined in December 2009. He has contributed to the Lua API, submitted small bugfix patches "
                           "and has helped with release management of 1.0.5.")});
    aboutMakers.append({false, qsl("Bruno Bigras"), QString(), QString(), qsl("bruno@burnbox.net"),
                        //: about:Bruno Bigras
                        tr("Developed the original cmake build script and he has committed a number of patches.")});
    aboutMakers.append({false, qsl("Carter Dewey"), QString(), QString(), qsl("eldarerathis@gmail.com"),
                        //: about:Carter Dewey
                        tr("Contributed to the Lua API.")});
    aboutMakers.append({false, qsl("Erik Pettis"), qsl("Etomyutikos#9266"), qsl("Oneymus"), QString(),
                        //: about:Oneymus
                        tr("Developed the Vyzor GUI Manager for Mudlet.")});
    aboutMakers.append({false, qsl("Harrison"), QString(), qsl("Harrison-Teeg"), qsl("harrison.martin@gmail.com"),
                        //: about:Harrison
                        tr("Brought the 3D mapper back to life with camera controls, lighting and proper geometry "
                           "for z-squished rooms, and has fixed a number of console and command line annoyances.")});
    aboutMakers.append({false, qsl("ItsTheFae"), qsl("TheFae#9971"), qsl("Kae"), QString(),
                        //: about:TheFae
                        tr("Worked wonders in rejuvenating our Website in 2017 but who prefers a little anonymity - "
                           "if you are a <i>SpamBot</i> you will not get onto our Fora now. They have also made some useful "
                           "C++ core code contributions and we look forward to future reviews on and work in that area.")});
    aboutMakers.append({false, qsl("Ian Adkins"), qsl("Dicene#1533"), qsl("dicene"), qsl("ieadkins@gmail.com"),
                        //: about:Dicene
                        tr("Joining us 2017 they have given us some useful C++ and Lua contributions.")});
    aboutMakers.append({false, qsl("James Younquist"), QString(), QString(), qsl("daemacles@yahoo.com"),
                        //: about:James Younquist
                        tr("Contributed the Geyser layout manager for Mudlet in March 2010. "
                           "It is written in Lua and aims at simplifying user GUI scripting.")});
    aboutMakers.append({false, qsl("John Dahlström"), QString(), QString(), qsl("email@johndahlstrom.se"),
                        //: about:John Dahlström
                        tr("Helped develop and debug the Lua API.")});
    aboutMakers.append({false, qsl("John McKisson"), QString(), qsl("jmckisson"), qsl("john.mckisson@gmail.com"),
                        //: about:John McKisson
                        tr("Implemented MMCP, so Mudlet can join MudMaster chat networks, and has contributed "
                           "a range of console and Lua API fixes.")});
    aboutMakers.append({false, qsl("Karsten Bock"), QString(), qsl("Beliaar"), QString(),
                        //: about:Beliaar
                        tr("Contributed several improvements and new features for Geyser.")});
    aboutMakers.append({false, qsl("Leigh Stillard"), QString(), QString(), qsl("leigh.stillard@gmail.com"),
                        //: about:Leigh Stillard
                        tr("The original author of our Windows installer.")});
    aboutMakers.append({false, qsl("Maksym Grinenko"), QString(), QString(), qsl("maksym.grinenko@gmail.com"),
                        //: about:Maksym Grinenko
                        tr("Worked on the manual, forum help and helps with GUI design and documentation.")});
    aboutMakers.append({false, qsl("Manuel Wegmann"), QString(), qsl("Edru2"), QString(),
                        //: about:Edru2
                        tr("Built much of the GUI toolkit you script with between 2020 and 2022: Adjustable Containers, "
                           "Geyser's ScrollBox, animated labels and Geyser in UserWindows - plus the dark theme toggle "
                           "and the Package Exporter rework.")});
    aboutMakers.append({false, qsl("Mike Conley"), QString(), qsl("mpconley"), qsl("sousesider@gmail.com"),
                        //: about:Mike Conley
                        tr("Joined in 2018 and looks after nearly everything Mudlet plays or negotiates - MCMP media, "
                           "sound and video, closed captioning, MXP, OSC 8 hyperlinks and text encodings - plus "
                           "multi-window support with drag-and-drop tabs.")});
    aboutMakers.append({false, qsl("Stephen Hansen"), QString(), QString(), qsl("me+mudlet@ixokai.io"),
                        //: about:Stephen Hansen
                        tr("Developed a database Lua API that allows for far easier use of databases and one of the original OSX installers.")});
    aboutMakers.append({false, qsl("Thorsten Wilms"), QString(), QString(), qsl("t_w_@freenet.de"),
                        //: about:Thorsten Wilms
                        tr("Designed our beautiful logo, our splash screen, the about dialog, our website, several icons and badges. "
                           "Visit his homepage at <a href=\"http://thorwil.wordpress.com/\">thorwil.wordpress.com</a>.")});
    aboutMakers.append({false, qsl("Tim Johnson"), QString(), qsl("atari2600tim"), QString(),
                        //: about:Tim Johnson
                        tr("Joined in 2020 and made Mudlet work far better with screen readers, alongside secure IRC "
                           "connections, Discord improvements, and a batch of editor shortcuts and Lua configuration functions.")});
    // clang-format on
    return aboutMakers;
}

QVector<aboutThirdParty> dlgAboutDialog::thirdPartyComponents()
{
    // clang-format off
    // The Licences themselves MUST NOT be translated: only the English form is legally definitive
    // This one needs something about the name of the original copyright holder
    // and possible contributors as it includes a %1 placeholder in the text
    // to represent something that varies between different products using it:
    // There are a second & third %2/%3 placeholder that contains:
    // %2 either "COPYRIGHT HOLDERS AND CONTRIBUTORS" or "AUTHOR"
    // %3 either "COPYRIGHT HOLDERS OR CONTRIBUTORS" or "AUTHOR"
    // depending on the particular situation:
    QString BSD3Clause_Body(
            qsl("<h4>The [3-Clause] BSD Licence</h4>"
                           "<p>Redistribution and use in source and binary forms, with or without "
                           "modification, are permitted provided that the following conditions are met:"
                           "<ul><li>Redistributions of source code must retain the above copyright notice, "
                           "this list of conditions and the following disclaimer.</li>"
                           "<li>Redistributions in binary form must reproduce the above copyright notice, "
                           "this list of conditions and the following disclaimer in the documentation "
                           "and/or other materials provided with the distribution.</li>"
                           "<li>%1 be used to "
                           "endorse or promote products derived from this software without specific prior "
                           "written permission.</li></ul></p>"
                           "<p>THIS SOFTWARE IS PROVIDED BY THE %2 &quot;AS "
                           "IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
                           "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
                           "ARE DISCLAIMED. IN NO EVENT SHALL THE %3 BE "
                           "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR "
                           "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF "
                           "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS "
                           "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN "
                           "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING "
                           "IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE "
                           "POSSIBILITY OF SUCH DAMAGE.</p>"));

    // There are a first & second %1/%2 placeholder that contains:
    // %1 either "COPYRIGHT HOLDERS AND CONTRIBUTORS" or "AUTHOR"
    // %2 either "COPYRIGHT HOLDERS OR CONTRIBUTORS" or "AUTHOR"
    // depending on the particular situation:
    QString BSD2Clause_Body(
            qsl("<h4>The [2-Clause] BSD Licence</h4>"
                           "<p>Redistribution and use in source and binary forms, with or without "
                           "modification, are permitted provided that the following conditions are met:</p>"
                           "<ol><li>Redistributions of source code must retain the above copyright notice, "
                           "this list of conditions and the following disclaimer.</li>"
                           "<li>Redistributions in binary form must reproduce the above copyright notice, "
                           "this list of conditions and the following disclaimer in the documentation "
                           "and/or other materials provided with the distribution.</li></ol></p>"
                           "<p>THIS SOFTWARE IS PROVIDED BY THE %1 &quot;AS "
                           "IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
                           "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
                           "ARE DISCLAIMED. IN NO EVENT SHALL THE %2 BE LIABLE "
                           "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL "
                           "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR "
                           "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER "
                           "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, "
                           "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE "
                           "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</p>"));

#if defined(INCLUDE_UPDATER) || defined(INCLUDE_OPENSSL3) || defined(DEBUG_SHOWALL)
    // This uses curly double quotes “ = &#8220; and ” = &#8221;
    QString APACHE2_Body(
            qsl("<h4>Apache Licence</h4>"
                "<h4 style='text-align: center;'>Version 2.0, January 2004<br>"
                "<a href='https://www.apache.org/licenses/'>https://www.apache.org/licenses/</a></h4>"
                "<p>TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION</p>"
                "<h4>1. Definitions.</h4>"
                "<p>“License” shall mean the terms and conditions for use, reproduction, and "
                "distribution as defined by Sections 1 through 9 of this document.</p>"
                "<p>“Licensor” shall mean the copyright owner or entity authorized by the "
                "copyright owner that is granting the License.</p>"
                "<p>“Legal Entity” shall mean the union of the acting entity and all other "
                "entities that control, are controlled by, or are under common control with "
                "that entity. For the purposes of this definition, “control” means (i) the "
                "power, direct or indirect, to cause the direction or management of such entity, "
                "whether by contract or otherwise, or (ii) ownership of fifty percent (50%) or "
                "more of the outstanding shares, or (iii) beneficial ownership of such entity.</p>"
                "<p>“You” (or “Your”) shall mean an individual or Legal Entity exercising "
                "permissions granted by this License.</p>"
                "<p>“Source” form shall mean the preferred form for making modifications, "
                "including but not limited to software source code, documentation source, and "
                "configuration files.</p>"
                "<p>“Object” form shall mean any form resulting from mechanical transformation "
                "or translation of a Source form, including but not limited to compiled object "
                "code, generated documentation, and conversions to other media types.</p>"
                "<p>“Work” shall mean the work of authorship, whether in Source or Object form, "
                "made available under the License, as indicated by a copyright notice that is "
                "included in or attached to the work (an example is provided in the Appendix "
                "below).</p>"
                "<p>“Derivative Works” shall mean any work, whether in Source or Object form, "
                "that is based on (or derived from) the Work and for which the editorial "
                "revisions, annotations, elaborations, or other modifications represent, as a "
                "whole, an original work of authorship. For the purposes of this License, "
                "Derivative Works shall not include works that remain separable from, or merely "
                "link (or bind by name) to the interfaces of, the Work and Derivative Works "
                "thereof.</p>"
                "<p>“Contribution” shall mean any work of authorship, including the original "
                "version of the Work and any modifications or additions to that Work or "
                "Derivative Works thereof, that is intentionally submitted to Licensor for "
                "inclusion in the Work by the copyright owner or by an individual or Legal "
                "Entity authorized to submit on behalf of the copyright owner. For the purposes "
                "of this definition, “submitted” means any form of electronic, verbal, or "
                "written communication sent to the Licensor or its representatives, including "
                "but not limited to communication on electronic mailing lists, source code "
                "control systems, and issue tracking systems that are managed by, or on behalf "
                "of, the Licensor for the purpose of discussing and improving the Work, but "
                "excluding communication that is conspicuously marked or otherwise designated "
                "in writing by the copyright owner as “Not a Contribution.”</p>"
                "<p>“Contributor” shall mean Licensor and any individual or Legal Entity on "
                "behalf of whom a Contribution has been received by Licensor and subsequently "
                "incorporated within the Work.</p>"
                "<h4>2. Grant of Copyright License.</h4>"
                "<p>Subject to the terms and conditions of this License, each Contributor hereby "
                "grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, "
                "irrevocable copyright license to reproduce, prepare Derivative Works of, publicly "
                "display, publicly perform, sublicense, and distribute the Work and such "
                "Derivative Works in Source or Object form.</p>"
                "<h4>3. Grant of Patent License.</h4>"
                "<p>Subject to the terms and conditions of this License, each Contributor hereby "
                "grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, "
                "irrevocable (except as stated in this section) patent license to make, have made, "
                "use, offer to sell, sell, import, and otherwise transfer the Work, where such "
                "license applies only to those patent claims licensable by such Contributor that "
                "are necessarily infringed by their Contribution(s) alone or by combination of "
                "their Contribution(s) with the Work to which such Contribution(s) was submitted. "
                "If You institute patent litigation against any entity (including a cross-claim or "
                "counterclaim in a lawsuit) alleging that the Work or a Contribution incorporated "
                "within the Work constitutes direct or contributory patent infringement, then any "
                "patent licenses granted to You under this License for that Work shall terminate "
                "as of the date such litigation is filed.</p>"
                "<h4>4. Redistribution.</h4>"
                "<p>You may reproduce and distribute copies of the Work or Derivative Works thereof "
                "in any medium, with or without modifications, and in Source or Object form, "
                "provided that You meet the following conditions:</p>"
                "<ol type='a'><li>You must give any other recipients of the Work or Derivative Works a "
                "copy of this License; and</li>"
                "<li>You must cause any modified files to carry prominent notices stating that You "
                "changed the files; and</li>"
                "<li>You must retain, in the Source form of any Derivative Works that You distribute, "
                "all copyright, patent, trademark, and attribution notices from the Source form of "
                "the Work, excluding those notices that do not pertain to any part of the "
                "Derivative Works; and</li>"
                "<li>If the Work includes a “NOTICE” text file as part of its "
                "distribution, then any Derivative Works that You distribute must include a "
                "readable copy of the attribution notices contained within such NOTICE file, "
                "excluding those notices that do not pertain to any part of the Derivative "
                "Works, in at least one of the following places: within a NOTICE text file "
                "distributed as part of the Derivative Works; within the Source form or "
                "documentation, if provided along with the Derivative Works; or, within a "
                "display generated by the Derivative Works, if and wherever such third-party "
                "notices normally appear. The contents of the NOTICE file are for informational "
                "purposes only and do not modify the License. You may add Your own attribution "
                "notices within Derivative Works that You distribute, alongside or as an "
                "addendum to the NOTICE text from the Work, provided that such additional "
                "attribution notices cannot be construed as modifying the License.</li></ol>"
                "<p>You may add Your own copyright statement to Your modifications and may provide "
                "additional or different license terms and conditions for use, reproduction, or "
                "distribution of Your modifications, or for any such Derivative Works as a whole, "
                "provided Your use, reproduction, and distribution of the Work otherwise complies "
                "with the conditions stated in this License.</p>"
                "<h4>5. Submission of Contributions.</h4>"
                "<p>Unless You explicitly state otherwise, any Contribution intentionally submitted "
                "for inclusion in the Work by You to the Licensor shall be under the terms and "
                "conditions of this License, without any additional terms or conditions. "
                "Notwithstanding the above, nothing herein shall supersede or modify the terms of "
                "any separate license agreement you may have executed with Licensor regarding such "
                "Contributions.</p>"
                "<h4>6. Trademarks.</h4>"
                "<p>This License does not grant permission to use the trade names, trademarks, "
                "service marks, or product names of the Licensor, except as required for "
                "reasonable and customary use in describing the origin of the Work and reproducing "
                "the content of the NOTICE file.</p>"
                "<h4>7. Disclaimer of Warranty.</h4>"
                "<p>Unless required by applicable law or agreed to in writing, Licensor provides "
                "the Work (and each Contributor provides its Contributions) on an “AS IS” BASIS, "
                "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, "
                "including, without limitation, any warranties or conditions of TITLE, "
                "NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR PURPOSE. You are "
                "solely responsible for determining the appropriateness of using or redistributing "
                "the Work and assume any risks associated with Your exercise of permissions under "
                "this License.</p>"
                "<h4>8. Limitation of Liability.</h4>"
                "<p>In no event and under no legal theory, whether in tort (including negligence), "
                "contract, or otherwise, unless required by applicable law (such as deliberate and "
                "grossly negligent acts) or agreed to in writing, shall any Contributor be liable "
                "to You for damages, including any direct, indirect, special, incidental, or "
                "consequential damages of any character arising as a result of this License or out "
                "of the use or inability to use the Work (including but not limited to damages for "
                "loss of goodwill, work stoppage, computer failure or malfunction, or any and all "
                "other commercial damages or losses), even if such Contributor has been advised of "
                "the possibility of such damages.</p>"
                "<h4>9. Accepting Warranty or Additional Liability.</h4>"
                "<p>While redistributing the Work or Derivative Works thereof, You may choose to "
                "offer, and charge a fee for, acceptance of support, warranty, indemnity, or other "
                "liability obligations and/or rights consistent with this License. However, in "
                "accepting such obligations, You may act only on Your own behalf and on Your sole "
                "responsibility, not on behalf of any other Contributor, and only if You agree to "
                "indemnify, defend, and hold each Contributor harmless for any liability incurred "
                "by, or claims asserted against, such Contributor by reason of your accepting any "
                "such warranty or additional liability.</p>"
                "<p>END OF TERMS AND CONDITIONS</p>"));
#endif // defined(INCLUDE_UPDATER)

    QString MIT_Body(
            qsl("<h4>The MIT License</h4>"
                           "<p>Permission is hereby granted, free of charge, to any person obtaining a copy "
                           "of this software and associated documentation files (the &quot;Software&quot;), "
                           "to deal in the Software without restriction, including without limitation the "
                           "rights to use, copy, modify, merge, publish, distribute, sublicense, and/or "
                           "sell copies of the Software, and to permit persons to whom the Software is "
                           "furnished to do so, subject to the following conditions:</p>"
                           "<p>The above copyright notice and this permission notice shall be included in "
                           "all copies or substantial portions of the Software.</p>"
                           "<p>THE SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY KIND, "
                           "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
                           "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO "
                           "EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES "
                           "OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, "
                           "ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER "
                           "DEALINGS IN THE SOFTWARE.</p>"));

    QString ISC_Body(
            qsl("<h4>The ISC License</h4>"
                           "<p>Permission to use, copy, modify, and/or distribute this software for any "
                           "purpose with or without fee is hereby granted, provided that the above "
                           "copyright notice and this permission notice appear in all copies.</p>"
                           "<p>THE SOFTWARE IS PROVIDED &quot;AS IS&quot; AND THE AUTHOR DISCLAIMS ALL "
                           "WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF "
                           "MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY "
                           "SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER "
                           "RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, "
                           "NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE "
                           "USE OR PERFORMANCE OF THIS SOFTWARE.</p>"));

#if defined(INCLUDE_FONTS) || defined(DEBUG_SHOWALL)
    QString UbuntuFontText(
                qsl("<h3>UBUNTU FONT LICENCE Version 1.0</h3>"
                               "<p>PREAMBLE</p>"
                               "<p>This licence allows the licensed fonts to be used, studied, modified and "
                               "redistributed freely. The fonts, including any derivative works, can be "
                               "bundled, embedded, and redistributed provided the terms of this licence are "
                               "met. The fonts and derivatives, however, cannot be released under any other "
                               "licence. The requirement for fonts to remain under this licence does not "
                               "require any document created using the fonts or their derivatives to be "
                               "published under this licence, as long as the primary purpose of the document is "
                               "not to be a vehicle for the distribution of the fonts.</p>"
                               "<p>DEFINITIONS</p>"
                               "<p>&quot;Font Software&quot; refers to the set of files released by the "
                               "Copyright Holder(s) under this licence and clearly marked as such. This may "
                               "include source files, build scripts and documentation.</p>"
                               "<p>&quot;Original Version&quot; refers to the collection of Font Software "
                               "components as received under this licence.</p>"
                               "<p>&quot;Modified Version&quot; refers to any derivative made by adding to, "
                               "deleting, or substituting -- in part or in whole -- any of the components of "
                               "the Original Version, by changing formats or by porting the Font Software to a "
                               "new environment.</p>"
                               "<p>&quot;Copyright Holder(s)&quot; refers to all individuals and companies who "
                               "have a copyright ownership of the Font Software.</p>"
                               "<p>&quot;Substantially Changed&quot; refers to Modified Versions which can be "
                               "easily identified as dissimilar to the Font Software by users of the Font "
                               "Software comparing the Original Version with the Modified Version.</p>"
                               "<p>To &quot;Propagate&quot; a work means to do anything with it that, without "
                               "permission, would make you directly or secondarily liable for infringement "
                               "under applicable copyright law, except executing it on a computer or modifying "
                               "a private copy. Propagation includes copying, distribution (with or without "
                               "modification and with or without charging a redistribution fee), making "
                               "available to the public, and in some countries other activities as well.</p>"
                               "<p>PERMISSION &amp; CONDITIONS</p>"
                               "<p>This licence does not grant any rights under trademark law and all such "
                               "rights are reserved.</p>"
                               "<p>Permission is hereby granted, free of charge, to any person obtaining a copy "
                               "of the Font Software, to propagate the Font Software, subject to the below "
                               "conditions:"
                               "<ol style=\"1\"><li>Each copy of the Font Software must contain the above "
                               "copyright notice and this licence. These can be included either as stand-alone "
                               "text files, human-readable headers or in the appropriate machine-readable "
                               "metadata fields within text or binary files as long as those fields can be "
                               "easily viewed by the user.</li>"
                               "<li>The font name complies with the following:"
                               "<ol type=\"a\"><li>The Original Version must retain its name, unmodified.</li>"
                               "<li>Modified Versions which are Substantially Changed must be renamed to avoid "
                               "use of the name of the Original Version or similar names entirely.</li>"
                               "<li>Modified Versions which are not Substantially Changed must be renamed to "
                               "both (i) retain the name of the Original Version and (ii) add additional naming "
                               "elements to distinguish the Modified Version from the Original Version. The "
                               "name of such Modified Versions must be the name of the Original Version, with "
                               "&quot;derivative X&quot; where X represents the name of the new work, appended "
                               "to that name.</li></ol></li>"
                               "<li>The name(s) of the Copyright Holder(s) and any contributor to the Font "
                               "Software shall not be used to promote, endorse or advertise any Modified "
                               "Version, except (i) as required by this licence, (ii) to acknowledge the "
                               "contribution(s) of the Copyright Holder(s) or (iii) with their explicit written "
                               "permission.</li>"
                               "<li>The Font Software, modified or unmodified, in part or in whole, must be "
                               "distributed entirely under this licence, and must not be distributed under any "
                               "other licence. The requirement for fonts to remain under this licence does not "
                               "affect any document created using the Font Software, except any version of the "
                               "Font Software extracted from a document created using the Font Software may "
                               "only be distributed under this licence.</p>"
                               "<p>TERMINATION</li></ol>"
                               "<p>This licence becomes null and void if any of the above conditions are not "
                               "met.</p>"
                               "<p>DISCLAIMER</p>"
                               "<p>THE FONT SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY "
                               "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF "
                               "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF "
                               "COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE COPYRIGHT "
                               "HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING ANY "
                               "GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WHETHER IN AN "
                               "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF THE USE OR "
                               "INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE FONT "
                               "SOFTWARE.</p>"));

    QString SILOpenFontText(
                qsl("<h3>SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007</h3>"
                               "<p>PREAMBLE</p>"
                               "<p>The goals of the Open Font License (OFL) are to stimulate worldwide "
                               "development of collaborative font projects, to support the font "
                               "creation efforts of academic and linguistic communities, and to "
                               "provide a free and open framework in which fonts may be shared and "
                               "improved in partnership with others.</p>"
                               "<p>The OFL allows the licensed fonts to be used, studied, modified and "
                               "redistributed freely as long as they are not sold by themselves. The "
                               "fonts, including any derivative works, can be bundled, embedded, "
                               "redistributed and/or sold with any software provided that any reserved "
                               "names are not used by derivative works. The fonts and derivatives, "
                               "however, cannot be released under any other type of license. The "
                               "requirement for fonts to remain under this license does not apply to "
                               "any document created using the fonts or their derivatives.</p>"
                               "<p>DEFINITIONS</p>"
                               "<p>&quot;Font Software&quot; refers to the set of files released by the "
                               "Copyright Holder(s) under this licence and clearly marked as such. This may "
                               "include source files, build scripts and documentation.</p>"
                               "<p>&quot;Reserved Font Name&quot; refers to any names specified as such "
                               "after the copyright statement(s).</p>"
                               "<p>&quot;Original Version&quot; refers to the collection of Font Software "
                               "components as distributed by the Copyright Holder(s).</p>"
                               "<p>&quot;Modified Version&quot; refers to any derivative made by adding to, "
                               "deleting, or substituting -- in part or in whole -- any of the components of "
                               "the Original Version, by changing formats or by porting the Font Software to a "
                               "new environment.</p>"
                               "<p>&quot;Author(s)&quot; refers to any designer, engineer, programmer, technical "
                               "writer or other person who contributed to the Font Software.</p>"
                               "<p>PERMISSION &amp; CONDITIONS</p>"
                               "<p>Permission is hereby granted, free of charge, to any person obtaining "
                               "a copy of the Font Software, to use, study, copy, merge, embed, "
                               "modify, redistribute, and sell modified and unmodified copies of the "
                               "Font Software, subject to the following conditions:"
                               "<ol style=\"1\"><li> Neither the Font Software nor any of its individual components, "
                               "in Original or Modified Versions, may be sold by itself.</li>"
                               "<li>Original or Modified Versions of the Font Software may be bundled, "
                               "redistributed and/or sold with any software, provided that each copy "
                               "contains the above copyright notice and this license. These can be "
                               "included either as stand-alone text files, human-readable headers or "
                               "in the appropriate machine-readable metadata fields within text or "
                               "binary files as long as those fields can be easily viewed by the user.</li>"
                               "<li>No Modified Version of the Font Software may use the Reserved Font "
                               "Name(s) unless explicit written permission is granted by the "
                               "corresponding Copyright Holder. This restriction only applies to the "
                               "primary font name as presented to the users.</li>"
                               "<li>The name(s) of the Copyright Holder(s) or the Author(s) of the Font "
                               "Software shall not be used to promote, endorse or advertise any "
                               "Modified Version, except to acknowledge the contribution(s) of the "
                               "Copyright Holder(s) and the Author(s) or with their explicit written "
                               "permission.</li>"
                               "<li>The Font Software, modified or unmodified, in part or in whole, "
                               "must be distributed entirely under this license, and must not be "
                               "distributed under any other license. The requirement for fonts to "
                               "remain under this license does not apply to any document created using "
                               "the Font Software</p>"
                               "<p>TERMINATION</li></ol>"
                               "<p>This licence becomes null and void if any of the above conditions are not "
                               "met.</p>"
                               "<p>DISCLAIMER</p>"
                               "<p>THE FONT SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY "
                               "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF "
                               "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF "
                               "COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE COPYRIGHT "
                               "HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING ANY "
                               "GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WHETHER IN AN "
                               "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF THE USE OR "
                               "INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE FONT "
                               "SOFTWARE.</p>"));
#endif // defined(INCLUDE_FONTS)

    QVector<aboutThirdParty> components;

    // Names and SPDX identifiers are not translated: a name is what its authors call it
    components.append({qsl("Communi IRC Library"),
                       //: about:third-party Communi
                       tr("IRC support. Copyright © 2008-2020 The Communi Project"),
                       qsl("BSD-3-Clause"),
                       BSD3Clause_Body.arg(qsl("Neither the name of the Communi Project nor the names of its contributors may"),
                                           qsl("COPYRIGHT HOLDERS AND CONTRIBUTORS"),
                                           qsl("COPYRIGHT HOLDERS OR CONTRIBUTORS"))
                               //: about:third-party the Konversation code inside Communi
                               + tr("<p>Parts of <tt>irctextformat.cpp</tt> code come from Konversation and are copyrighted to:<br>"
                                    "Copyright © 2002 Dario Abatianni &lt;eisfuchs@tigress.com&gt;<br>"
                                    "Copyright © 2004 Peter Simonsson &lt;psn@linux.se&gt;<br>"
                                    "Copyright © 2006-2008 Eike Hein &lt;hein@kde.org&gt;<br>"
                                    "Copyright © 2004-2009 Eli Mackenzie &lt;argonel@gmail.com&gt;</p>")});

    components.append({qsl("Lua 5.1"),
                       //: about:third-party Lua
                       tr("The language Mudlet is scripted in. Copyright © 1994–2017 Lua.org, PUC-Rio."), qsl("MIT"), MIT_Body});

    components.append({qsl("LuaFileSystem"),
                       //: about:third-party LuaFileSystem
                       tr("File system access for Lua. Copyright © 2003-2020, Kepler Project"), qsl("MIT"), MIT_Body});

    components.append({qsl("Lua_yajl"),
                       //: about:third-party lua_yajl
                       tr("Lua 5.1 interface to yajl. Author: Brian Maher &lt;maherb at brimworks dot com&gt;. Copyright © 2009 Brian Maher"), qsl("MIT"), MIT_Body});

    components.append({qsl("Luautf8"),
                       //: about:third-party luautf8
                       tr("A UTF-8 support module for Lua. Copyright © 2018 Xavier Wang"), qsl("MIT"), MIT_Body});

    components.append({qsl("LuaSql-Sqlite3"),
                       //: about:third-party LuaSql
                       tr("Database connectivity for the Lua programming language (Sqlite3 component). Copyright © 2003-2019, The Kepler Project"), qsl("MIT"), MIT_Body});

    components.append({qsl("Lrexlib-pcre2"),
                       //: about:third-party lrexlib
                       tr("Regular expression library binding (PCRE2 flavour). Copyright © Reuben Thomas 2000-2020, Copyright © Shmuel Zeigerman 2004-2020"), qsl("MIT"), MIT_Body});

#if defined(Q_OS_MACOS) || defined(DEBUG_SHOWALL)
    components.append({qsl("LuaZip"),
                       //: about:third-party LuaZip
                       tr("Reading files inside zip files. Author: Danilo Tuler. Copyright © 2003-2007 Kepler Project"), qsl("MIT"), MIT_Body});
#endif // defined(Q_OS_MACOS)

    components.append({qsl("Edbee"),
                       //: about:third-party edbee
                       tr("The multi-feature editor widget the script editor is built on. Copyright © 2012-2026 by Rick Blommers"),
                       qsl("MIT"),
                       MIT_Body
                               //: about:third-party edbee supplement
                               + qsl("<p>%1</p>").arg(tr("The <b>edbee-lib</b> widget itself incorporates another component with a licence that must be noted as well, it is Oniguruma - listed "
                                                         "below."))});

    components.append({qsl("Oniguruma"),
                       //: about:third-party Oniguruma
                       tr("Regular expressions inside edbee. Copyright © 2002-2021 K.Kosako &lt;kkosako0@gmail.com&gt;. All rights reserved."),
                       qsl("BSD-2-Clause"),
                       BSD2Clause_Body.arg(qsl("COPYRIGHT HOLDERS AND CONTRIBUTORS"), qsl("COPYRIGHT HOLDERS OR CONTRIBUTORS"))});

#if defined(INCLUDE_UPDATER) || defined(DEBUG_SHOWALL)
    components.append({qsl("Dblsqd"),
                       //: about:third-party dblsqd
                       tr("The updater, as a derived work. Copyright © 2017 Philipp Medien"), qsl("Apache-2.0"), APACHE2_Body});
#if defined(Q_OS_MACOS)
    components.append({qsl("Sparkle"),
                       //: about:third-party Sparkle
                       tr("The macOS updater. Copyright © 2006-2013 Andy Matuschak, "
                          "Copyright © 2009-2013 Elgato Systems GmbH, "
                          "Copyright © 2011-2014 Kornel Lesiński, "
                          "Copyright © 2015-2017 Mayur Pawashe, "
                          "Copyright © 2014 C.W. Betts, "
                          "Copyright © 2014 Petroules Corporation, "
                          "Copyright © 2014 Big Nerd Ranch. All rights reserved."),
                       qsl("MIT"),
                       MIT_Body
                               //: about:third-party the components bundled inside Sparkle
                               + tr("<h4>bspatch.c and bsdiff.c, from bsdiff 4.3 <a href=\"http://www.daemonology.net/bsdiff/\">http://www.daemonology.net/bsdiff</a>:</h4>"
                                    "<p>Copyright © 2003-2005 Colin Percival.</p>"
                                    "<h4>sais.c and sais.c, from sais-lite (2010/08/07) "
                                    "<a href=\"https://sites.google.com/site/yuta256/sais\">https://sites.google.com/site/yuta256/sais</a>:</h4>"
                                    "<p>Copyright © 2008-2010 Yuta Mori.</p>"
                                    "<h4>SUDSAVerifier.m:</h4>"
                                    "<p>Copyright © 2011 Mark Hamlin.<br>All rights reserved.</p>")
                               + BSD2Clause_Body.arg(QLatin1String("AUTHOR"), QLatin1String("AUTHOR"))
                               + BSD2Clause_Body.arg(QLatin1String("AUTHOR AND CONTRIBUTORS"), QLatin1String("AUTHOR OR CONTRIBUTORS"))});
#endif // defined(Q_OS_MACOS)
#endif // defined(INCLUDE_UPDATER)

#if defined(INCLUDE_FONTS) || defined(DEBUG_SHOWALL)
    components.append({qsl("Ubuntu Font Family"),
                       //: about:third-party Ubuntu font
                       tr("A font family bundled with Mudlet"), qsl("UFL-1.0"), UbuntuFontText});
    components.append({qsl("Noto Color Emoji"),
                       //: about:third-party Noto font
                       tr("A font bundled with Mudlet"), qsl("OFL-1.1"), SILOpenFontText});
#endif // defined(INCLUDE_FONTS)

    components.append({qsl("Discord Rich Presence"),
                       //: about:third-party Discord RPC
                       tr("The RPC library that tells Discord what you are playing. Copyright © 2017 Discord, Inc."),
                       qsl("MIT"),
                       // A picture rather than chrome, so no colour of the design applies
                       qsl(R"(<p><img src=":/icons/Discord-Logo+Wordmark-Color_438x120px.png" width="219" height="60"></p>)") + MIT_Body});

    components.append({qsl("QtKeyChain"),
                       //: about:third-party QtKeychain
                       tr("Platform-independent Qt API for storing passwords securely. Copyright © 2011-2026 Frank Osterfeld &lt;frank.osterfeld@gmail.com&gt;."),
                       qsl("BSD-3-Clause"),
                       BSD3Clause_Body.arg(QLatin1String("The name of the author may not"), QLatin1String("AUTHOR"), QLatin1String("AUTHOR"))});

    components.append({qsl("singleshot_connect.h"),
                       //: about:third-party KDToolBox
                       tr("Part of <a href=\"https://github.com/KDAB/KDToolBox\">KDToolBox</a>. Copyright © 2020-2021 Klarälvdalens Datakonsult AB, a KDAB Group company, "
                          "&lt;info@kdab.com&gt;."),
                       qsl("MIT"),
                       MIT_Body});

    // Although this is only effective on Windows it is bundled in ALL builds
    components.append({qsl("utf8_filenames.lua"),
                       //: about:third-party utf8_filenames
                       tr("Modifies standard Lua functions so that they work with UTF-8 filenames on Windows. Copyright © 2019 Egor-Skriptunoff. "
                          "<a href=\"https://gist.github.com/Egor-Skriptunoff/2458547aa3b9210a8b5f686ac08ecbf0\">Github GIST</a>"),
                       qsl("MIT"),
                       MIT_Body});

#if defined(WITH_SENTRY) || defined(DEBUG_SHOWALL)
    components.append({qsl("Sentry Native"),
                       //: about:third-party Sentry
                       tr("Crash reporting SDK. Copyright © 2019 Sentry (https://sentry.io) and individual contributors. All rights reserved."), qsl("MIT"), MIT_Body});
#endif

    //: about:third-party name of the sword model the 3D mapper uses
    components.append({tr("Sword 3D model"),
                       //: about:third-party sword model
                       tr("Used by the 3D mapper. Model by minghauLoh, obtained from Sketchfab"),
                       //: Licence chip for a component whose licence is none of the usual ones
                       tr("Other"),
                       //: about:third-party where the 3D mapper's sword model came from
                       tr("<p>Model obtained from <a href=\"https://sketchfab.com/3d-models/sword-07463a2658e04d6ab8a42b5639a35d63\">Sketchfab</a><br>"
                          "Author: <a href=\"https://sketchfab.com/minghau\">minghauLoh</a><br>"
                          "Licensed under <a href=\"https://creativecommons.org/licenses/by/4.0/\">CC BY 4.0</a></p>")});

#if defined(INCLUDE_OPENSSL3) || defined(DEBUG_SHOWALL)
    components.append({qsl("OpenSSL 3.x"),
                       //: about:third-party OpenSSL
                       tr("Open Source Toolkit for Secure Transport Layer Security. Copyright © 1995-2026 The OpenSSL Project Authors. All Rights Reserved"),
                       qsl("Apache-2.0"),
                       APACHE2_Body});
#endif

    //: about:third-party the speech recognition libraries Mudlet can load. None are bundled - the user installs them - so this names them rather than reproducing their licences
    components.append({tr("Speech recognition backends"),
                       //: about:third-party Vosk, sherpa-onnx and the ONNX Runtime
                       tr("Mudlet can drive Vosk, sherpa-onnx and the ONNX Runtime for speech recognition. None of them ship with Mudlet: you install them yourself."),
                       qsl("Apache-2.0 AND MIT"),
                       //: about:third-party where the speech recognition backends come from and why their licences are not reproduced here
                       tr("<p>Mudlet can drive <a href=\"https://alphacephei.com/vosk\">Vosk</a> (Apache 2.0), "
                          "<a href=\"https://github.com/k2-fsa/sherpa-onnx\">sherpa-onnx</a> (Apache 2.0), and the "
                          "<a href=\"https://onnxruntime.ai\">ONNX Runtime</a> (MIT) it runs on.<br>"
                          "None of them ship with Mudlet: you install them yourself, and each one's licence "
                          "travels with the files you download.</p>")});

    components.append({qsl("Lucide"),
                       //: about:third-party Lucide. The copyright line is required by the ISC licence, so keep the name and year as they are
                       tr("Mudlet's line icons, from <a href=\"https://lucide.dev\">lucide.dev</a>. Copyright © 2026 Lucide Icons and Contributors"),
                       qsl("ISC"),
                       ISC_Body
                               //: about:third-party Lucide supplement: introduces the second licence, which covers the icons Lucide itself inherited from the Feather project
                               + tr("<h4>Icons derived from the Feather project</h4>"
                                    "<p>Those of Lucide's icons that it inherited from the Feather project are additionally covered by the following licence.<br>"
                                    "Copyright © 2013-present Cole Bemis</p>")
                               + MIT_Body});

    // clang-format on
    return components;
}
