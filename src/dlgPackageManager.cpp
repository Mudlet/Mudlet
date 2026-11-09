/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022, 2026 by Stephen Lyons - slysven@virginmedia.com   *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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


#include "dlgPackageManager.h"

#include "GripSplitter.h"
#include "MudletApp.h"
#include "TMainConsole.h"
#include "dlgSystemMessageArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPointer>
#include <QProgressDialog>
#include <QSaveFile>
#include <QSettings>
#include <QTabBar>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVersionNumber>

using namespace std::chrono_literals;

#include <algorithm>

// The tab strip's pane inset, so the search field lines up with the first chip
static constexpr int scmPackagesColumnInset = uiDesign::scmTabPaneInset;

// Not the .ui file's 96px: the picture stands beside the name rather than over it
static constexpr int scmPackagesIconSize = 48;
static constexpr int scmPackagesIconGlyphSize = 24;
static constexpr int scmPackagesButtonGlyphSize = 16;
static constexpr int scmPackagesCaptionGap = 12;
// Narrower, the notes and the button row wrap into a column of single words
static constexpr int scmPackagesDetailsMinimumWidth = 320;
static constexpr int scmPackagesHeadlineFloorCharacters = 6;
static constexpr int scmPackagesListShareDenominator = 3;
static constexpr char scmPackagesSplitterKey[] = "packageManagerSplitterState";


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, mpHost(pHost)
{
    setupUi(this);
    connect(lineEdit_searchBar, &QLineEdit::textChanged, this, &dlgPackageManager::slot_searchTextChanged);
    connect(mpHost->mainConsoleView(), &QWidget::destroyed, this, &dlgPackageManager::close);
    packageList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(packageList, &QWidget::customContextMenuRequested, this, &dlgPackageManager::slot_packageContextMenu);
    connect(packageList, &QListWidget::currentItemChanged, this, &dlgPackageManager::slot_itemChanged);
    connect(packageList, &QListWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggleInstallRepoButton);
    connect(packageList, &QListWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggleRemoveButton);
    connect(pushButton_installFile, &QAbstractButton::clicked, this, &dlgPackageManager::slot_installPackageFromFile);
    connect(pushButton_installRepo, &QAbstractButton::clicked, this, &dlgPackageManager::slot_installPackageFromRepository);
    connect(pushButton_remove, &QAbstractButton::clicked, this, &dlgPackageManager::slot_removePackages);
    connect(pushButton_report, &QAbstractButton::clicked, this, &dlgPackageManager::slot_openBugWebsite);
    connect(pushButton_website, &QAbstractButton::clicked, this, &dlgPackageManager::slot_openPackageWebsite);

    //: Package manager - window title
    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));

    pushButton_website->hide();
    pushButton_report->hide();

    setupNavigationButtons();

    packageList->setSortingEnabled(true);

    mpPackageItemDelegate = new PackageItemDelegate(this);
    packageList->setItemDelegate(mpPackageItemDelegate);

    buildShell();

    repositoryPackages = QJsonArray();
    if (!readPackageRepositoryFile()) {
        downloadRepositoryIndex();
    }

    pushButton_installRepo->setEnabled(false);
    mCurrentView = NavigationView::Installed;
    mpNavigationGroup->button(static_cast<int>(NavigationView::Installed))->setChecked(true);
    syncViewBar();
    slot_setPackageList();

    applyPackageManagerShellStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgPackageManager::slot_applyAppearance);

    // Connected last: resetPackageList() dereferences mpNavigationGroup, made by setupNavigationButtons()
    connect(mpHost, &Host::signal_packageListChanged, this, &dlgPackageManager::resetPackageList);

    setAttribute(Qt::WA_DeleteOnClose);
}

// Moves and restyles the .ui file's controls but renames none: connections and tests reach them by name
void dlgPackageManager::buildShell()
{
    // The .ui file's 900x600 minimum is more than the columns need; the layout's own floor takes over
    setMinimumSize(0, 0);

    mainVerticalLayout->setContentsMargins(0, 0, 0, 0);
    mainVerticalLayout->setSpacing(0);
    horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
    horizontalLayout_2->setSpacing(0);
    verticalLayout_left->setContentsMargins(scmPackagesColumnInset, scmPackagesColumnInset, scmPackagesColumnInset, scmPackagesColumnInset);
    verticalLayout_right->setContentsMargins(scmPackagesColumnInset, scmPackagesColumnInset, scmPackagesColumnInset, scmPackagesColumnInset);
    uiDesign::markAsShellSurface(leftPanel);
    uiDesign::markAsShellSurface(rightPanel);

    // Out of the .ui file's row first: a splitter takes its panes as its own children
    uiDesign::removeFromLayoutTree(horizontalLayout_2, leftPanel);
    uiDesign::removeFromLayoutTree(horizontalLayout_2, rightPanel);
    mpSplitter = new uiDesign::GripSplitter(Qt::Horizontal, this);
    mpSplitter->setObjectName(qsl("packagesSplitter"));
    uiDesign::markAsShellSurface(mpSplitter);
    mpSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpSplitter->setChildrenCollapsible(false);
    mpSplitter->addWidget(leftPanel);
    mpSplitter->addWidget(rightPanel);
    // Per pane as well: QSplitter::isCollapsible() answers the default for a pane never
    // told, not what setChildrenCollapsible() left
    mpSplitter->setCollapsible(0, false);
    mpSplitter->setCollapsible(1, false);
    mpSplitter->setStretchFactor(0, 0);
    mpSplitter->setStretchFactor(1, 1);
    // No cap on the list column: a drag that stops dead reads as a broken handle
    leftPanel->setMaximumWidth(QWIDGETSIZE_MAX);
    rightPanel->setMinimumWidth(scmPackagesDetailsMinimumWidth);
    mainVerticalLayout->insertWidget(mainVerticalLayout->indexOf(horizontalLayout_2), mpSplitter);
    // Its resize catches a drag, a window resize and a restored split alike, and
    // setSizes() announces nothing of its own
    rightPanel->installEventFilter(this);

    // The .ui file's buttons stay, hidden, as the one path into a view change: slots and
    // tests press them, and the bar clicks them rather than doing the work itself
    headerBar->hide();
    mpViewBar = new QTabBar(leftPanel);
    mpViewBar->setObjectName(qsl("packagesViewBar"));
    mpViewBar->setExpanding(false);
    mpViewBar->setDrawBase(false);
    mpViewBar->setFocusPolicy(Qt::TabFocus);
    uiDesign::markAsShellSurface(mpViewBar);
    //: Package manager - what the row of Explore / Installed / Updates chips is, read out to a screen reader
    mpViewBar->setAccessibleName(tr("Packages to show"));
    {
        // The first addTab() emits currentChanged, which would click Explore before the view is chosen
        const QSignalBlocker held(mpViewBar);
        for (const QAbstractButton* pButton :
             {static_cast<QAbstractButton*>(pushButton_explore), static_cast<QAbstractButton*>(pushButton_installed), static_cast<QAbstractButton*>(pushButton_updates)}) {
            const int index = mpViewBar->addTab(pButton->text());
            mpViewBar->setAccessibleTabName(index, pButton->text());
        }
    }
    uiDesign::prepareTabStrip(mpViewBar);
    // Aligned by the layout: the macOS style answers Qt::AlignCenter for tab placement,
    // while Fusion, the dark appearance's base style, answers the leading edge
    verticalLayout_left->insertWidget(0, mpViewBar, 0, Qt::AlignLeft);
    connect(mpViewBar, &QTabBar::currentChanged, this, [this](const int index) {
        QAbstractButton* pButton = mpNavigationGroup->button(index);
        if (pButton && !pButton->isChecked()) {
            pButton->click();
        }
    });

    // Empty until the style pass inks it
    mpAction_searchGlyph = lineEdit_searchBar->addAction(QIcon(), QLineEdit::LeadingPosition);

    // The row recipe's hover and chosen washes already tell the rows apart
    packageList->setAlternatingRowColors(false);
    // Rows elide, so there is never anything to scroll to sideways
    packageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    label_importStatus->hide();
    mpNotice = new dlgSystemMessageArea(leftPanel);
    mpNotice->setObjectName(qsl("packagesNotice"));
    mpNotice->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
    if (QLayout* pNoticeLayout = mpNotice->layout()) {
        pNoticeLayout->setContentsMargins(0, 0, 0, 0);
    }
    mpNotice->notificationAreaIconLabelError->hide();
    mpNotice->notificationAreaIconLabelInformation->hide();
    mpNotice->notificationAreaIconLabelWarning->show();
    mpNotice->hide();
    verticalLayout_left->insertWidget(verticalLayout_left->indexOf(label_importStatus) + 1, mpNotice);
    connect(mpNotice->messageAreaCloseButton, &QAbstractButton::clicked, mpNotice, &QWidget::hide);

    //: Package manager - button under the list of packages, which opens a file picker
    pushButton_installFile->setText(tr("Install from a file"));
    pushButton_installFile->setSizePolicy(QSizePolicy::Expanding, pushButton_installFile->sizePolicy().verticalPolicy());

    uiDesign::removeFromLayoutTree(horizontalLayout, pushButton_installRepo);
    uiDesign::removeFromLayoutTree(horizontalLayout, pushButton_remove);
    QLayoutItem* pTrailingSpacer = horizontalLayout_4->takeAt(horizontalLayout_4->indexOf(horizontalSpacer));
    horizontalLayout_4->insertWidget(0, pushButton_installRepo);
    horizontalLayout_4->insertWidget(1, pushButton_remove);
    if (pTrailingSpacer) {
        horizontalLayout_4->insertItem(2, pTrailingSpacer);
    }

    pushButton_website->setFlat(false);
    pushButton_report->setFlat(false);
    //: Package manager - button that opens the package's own web page
    const QString websiteWord = tr("Website");
    //: Package manager - button that opens the page an issue with the package is reported on
    const QString reportWord = tr("Report an issue");
    pushButton_website->setText(websiteWord);
    pushButton_report->setText(reportWord);
    // A narrow row leaves these two as glyphs alone, so a screen reader needs the word here
    pushButton_website->setAccessibleName(websiteWord);
    pushButton_report->setAccessibleName(reportWord);
    mActionRowLinks = {{pushButton_website, websiteWord, 0, 0}, {pushButton_report, reportWord, 0, 0}};

    label_icon->setFixedSize(scmPackagesIconSize, scmPackagesIconSize);
    // The pixmap arrives cut and scaled at the device pixel ratio; label scaling would undo both
    label_icon->setScaledContents(false);
    label_icon->clear();

    QFont nameFont = font();
    nameFont.setPointSize(uiDesign::typeSize(uiDesign::TypeStep::Title));
    nameFont.setWeight(QFont::DemiBold);
    label_packageName->setFont(nameFont);

    // A QLabel's minimum is its whole text, which let the package shown decide how narrow the window could go
    for (QLabel* pLabel : {label_packageName, label_author}) {
        pLabel->setMinimumWidth(QFontMetrics(pLabel->font()).averageCharWidth() * scmPackagesHeadlineFloorCharacters);
    }

    // The .ui file pins it 100px wide, which a number beside the name does not need
    label_version->setMinimumWidth(0);
    label_version->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignBottom);
    // Free to grow down, so its box is the row's full depth and AlignBottom has an effect
    label_version->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // Maximum, so the version stands right after the name rather than at the column's end
    label_packageName->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    label_author->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    label_author->setFont(font());
    label_version->setFont(font());

    uiDesign::removeFromLayoutTree(horizontalLayout_3, label_version);
    horizontalLayout_3->setSpacing(scmPackagesCaptionGap);
    horizontalLayout_3->addStretch(1);

    mpLayout_headlineRow = new QHBoxLayout();
    mpLayout_headlineRow->setObjectName(qsl("packagesHeadlineRow"));
    mpLayout_headlineRow->setContentsMargins(0, 0, 0, 0);
    mpLayout_headlineRow->setSpacing(scmPackagesCaptionGap);
    const int namePlace = verticalLayout_details->indexOf(label_packageName);
    uiDesign::removeFromLayoutTree(verticalLayout_details, label_packageName);
    mpLayout_headlineRow->addWidget(label_packageName);
    mpLayout_headlineRow->addWidget(label_version);
    mpLayout_headlineRow->addStretch(1);
    verticalLayout_details->insertLayout(namePlace, mpLayout_headlineRow);

    if (QLayoutItem* pCaptionRow = verticalLayout_details->takeAt(verticalLayout_details->indexOf(horizontalLayout_3))) {
        verticalLayout_details->addItem(pCaptionRow);
    }
    // The notes field's hairline is already the seam; this rule would double it
    line->hide();
}

void dlgPackageManager::clearPackageDetails()
{
    label_icon->clear();
    label_icon->setStyleSheet(QString());
    showPlainDescription(QString());
    mPackageNameShown.clear();
    label_packageName->clear();
    label_packageName->setToolTip(QString());
    label_title->clear();
    label_author->clear();
    label_version->clear();
    pushButton_website->hide();
    pushButton_report->hide();
}

void dlgPackageManager::showPackageIcon(const QPixmap& face)
{
    if (face.isNull()) {
        showPackagePlaceholder();
        return;
    }

    label_icon->setStyleSheet(QString());

    const qreal ratio = label_icon->devicePixelRatioF();
    QPixmap cut(QSize(scmPackagesIconSize, scmPackagesIconSize) * ratio);
    cut.setDevicePixelRatio(ratio);
    cut.fill(Qt::transparent);

    QPainter painter(&cut);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath corner;
    corner.addRoundedRect(QRectF(0, 0, scmPackagesIconSize, scmPackagesIconSize), uiDesign::scmRadiusInput, uiDesign::scmRadiusInput);
    painter.setClipPath(corner);
    painter.drawPixmap(QRect(0, 0, scmPackagesIconSize, scmPackagesIconSize), face);
    painter.end();

    label_icon->setPixmap(cut);
}

void dlgPackageManager::showPackagePlaceholder()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    label_icon->setStyleSheet(qsl("#label_icon { background-color: %1; border: %2px solid %3; border-radius: %4px; }")
                                      .arg(tokens.card.name(), QString::number(uiDesign::scmInputBorderWidth), tokens.border.name(), QString::number(uiDesign::scmRadiusInput)));

    const qreal ratio = label_icon->devicePixelRatioF();
    QPixmap glyph = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/packages-package.svg")), tokens.mutedText)
                            .scaled(QSize(scmPackagesIconGlyphSize, scmPackagesIconGlyphSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph.setDevicePixelRatio(ratio);
    label_icon->setPixmap(glyph);
}

void dlgPackageManager::downloadIcon(const QString& packageName)
{
    QString iconPath;

    if (packageLookup.contains(packageName)) {
        QJsonObject packageObj = packageLookup.value(packageName);
        if (packageObj.contains(qsl("icon"))) {
            iconPath = packageObj[qsl("icon")].toString();
        } else {
            showPackagePlaceholder();
            return;
        }
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/") + iconPath));
    request.setTransferTimeout(10000);
    QNetworkReply* reply = manager->get(request);
    reply->setProperty("packageName", packageName);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        slot_onIconDownloaded(reply);
    });
}

void dlgPackageManager::downloadRepositoryIndex()
{
    const QString outputPath = MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json"));
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(qsl("https://raw.githubusercontent.com/Mudlet/mudlet-package-repository/refs/heads/main/packages/mpkg.packages.json")));
    request.setTransferTimeout(20000);
    QNetworkReply* reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, outputPath, manager, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - network request failed:" << reply->errorString();
        } else {
            const QByteArray data = reply->readAll();
            // A captive portal or proxy can answer with a page of its own and a success status
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject() || !doc.object().value(qsl("packages")).isArray()) {
                qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - the download is not a package index, discarding it";
            } else {
                // Opened only once a whole index is in hand, so a failed, refused or abandoned download never
                // touches the previous one. On Unix, a profile directory that cannot take the temporary file
                // falls back to rewriting the index in place, which a write failing partway leaves truncated.
                QSaveFile file(outputPath);
                file.setDirectWriteFallback(true);
                if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
                    qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - failed to save the repository index:" << file.errorString();
                }
            }
        }
        reply->deleteLater();
        manager->deleteLater();
        // A successful read refreshes the updates list and badge itself
        if (!readPackageRepositoryFile()) {
            populatePackagesWithUpdates();
            updateUpdatesBadge();
        } else if (mCurrentView != NavigationView::Installed) {
            // Explore and Updates are listed from the index, so what is on show predates this one
            slot_setPackageList();
        }
    });
}

void dlgPackageManager::fillPackageDetails(const QString& name, const QString& title, const QString& author, const QString& version)
{
    label_title->setText(title);
    //: Package manager - who wrote the package, on the caption line under its name. %1 is the author's name
    label_author->setText(author.isEmpty() ? QString() : tr("by %1").arg(author));
    // Before showPackageName(): the name is cut to what the version leaves of the row
    label_version->setText(version);
    showPackageName(name);
}

void dlgPackageManager::showPackageName(const QString& name)
{
    mPackageNameShown = name;
    // Measured off the row, not the label: the label keeps the last package's width
    // until the layout runs again
    int room = label_packageName->width();
    if (mpLayout_headlineRow && mpLayout_headlineRow->geometry().width() > 0) {
        room = mpLayout_headlineRow->geometry().width() - scmPackagesCaptionGap - label_version->sizeHint().width();
    }
    const QFontMetrics metrics(label_packageName->font());
    const QString elidedName = metrics.elidedText(name, Qt::ElideRight, std::max(0, room));
    if (elidedName.isEmpty() && !name.isEmpty()) {
        // elidedText() returns nothing below an ellipsis's width; a blank heading reads as "no package selected"
        return;
    }
    label_packageName->setText(elidedName);
    label_packageName->setToolTip(elidedName == name ? QString() : name);
}

// Kept for relayDescription(): a QTextDocument applies its default stylesheet only
// when its text is set, so a theme change has to set the text again
void dlgPackageManager::showDescription(const QString& markdown)
{
    mDescription = markdown;
    mDescriptionIsMarkdown = true;
    packageDescription->setMarkdown(markdown);
}

void dlgPackageManager::showPlainDescription(const QString& text)
{
    mDescription = text;
    mDescriptionIsMarkdown = false;
    packageDescription->setPlainText(text);
}

void dlgPackageManager::relayDescription()
{
    if (mDescriptionIsMarkdown) {
        packageDescription->setMarkdown(mDescription);
    } else {
        packageDescription->setPlainText(mDescription);
    }
}

bool dlgPackageManager::hasNewerVersion(const QString& installed, const QString& repo) const
{
    const QVersionNumber installedVersion = QVersionNumber::fromString(installed);
    const QVersionNumber repoVersion = QVersionNumber::fromString(repo);
    return repoVersion > installedVersion;
}

QString dlgPackageManager::updateVersionText(const QString& installedVersion, const QString& repoVersion) const
{
    //: Package manager - the version beside a package's name in the Updates view: the one installed, then the one waiting. %1 is the installed version, %2 the new one
    return tr("%1 → %2").arg(installedVersion, repoVersion);
}

QString dlgPackageManager::updateRowVersion(const QString& packageName) const
{
    const QString installedVersion = mpHost ? mpHost->mPackageInfo.value(packageName).value(qsl("version")) : QString();
    const QString repoVersion = packageLookup.value(packageName).value(qsl("version")).toString();
    if (installedVersion.isEmpty() || repoVersion.isEmpty()) {
        return {};
    }
    return updateVersionText(installedVersion, repoVersion);
}

void dlgPackageManager::setRowVersion(QListWidgetItem* pItem, const QString& version)
{
    if (!pItem || version.isEmpty()) {
        return;
    }
    pItem->setData(PackageItemDelegate::cPackageVersionRole, version);
}

void dlgPackageManager::populatePackagesWithUpdates()
{
    mPackagesWithUpdates.clear();

    for (const QString& packageName : std::as_const(mpHost->mInstalledPackages)) {
        if (!matchesRepositoryPackage(packageName)) {
            continue;
        }

        const QJsonObject repoPackage = packageLookup.value(packageName);
        const QString repoVersion = repoPackage.value(qsl("version")).toString();
        const auto packageInfo = mpHost->mPackageInfo.value(packageName);
        const QString installedVersion = packageInfo.value(qsl("version"));

        if (!installedVersion.isEmpty() && !repoVersion.isEmpty() && hasNewerVersion(installedVersion, repoVersion)) {
            mPackagesWithUpdates.append(packageName);
        }
    }
}

bool dlgPackageManager::readPackageRepositoryFile()
{
    QFile file(MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qWarning() << "Repository file is empty";
        return false;
    }

    const QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in repository file";
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("packages") || !obj["packages"].isArray()) {
        qWarning() << "Repository file corrupt: missing 'packages' array";
        return false;
    }

    repositoryPackages = obj[qsl("packages")].toArray();
    packageLookup.clear();
    for (const QJsonValue& val : std::as_const(repositoryPackages)) {
        QJsonObject pkg = val.toObject();
        packageLookup.insert(pkg["mpackage"].toString(), pkg);
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();

    return true;
}

void dlgPackageManager::resetPackageList()
{
    if (!mpHost) {
        return;
    }

    clearPackageDetails();
    mCurrentView = NavigationView::Installed;
    mpNavigationGroup->button(static_cast<int>(NavigationView::Installed))->setChecked(true);
    syncViewBar();
    packageList->clear();

    for (const QString& packageName : std::as_const(mpHost->mInstalledPackages)) {
        packageList->addItem(installedPackageItem(packageName));
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();
    packageList->setCurrentRow(0);
    slot_toggleInstallRepoButton();
    slot_toggleRemoveButton();
}

// A package's toolbars are main-window docks, and showing one (a floating one above all)
// raises the main window over this dialog. Deferred past the install event and the docks' show.
void dlgPackageManager::comeBackToFront()
{
    QTimer::singleShot(0ms, this, [this]() {
        if (!isVisible()) {
            return;
        }
        raise();
        activateWindow();
    });
}

// A row inside a selection keeps it: Remove and Install act on every selected row,
// so a right-click that threw the rest away would be a trap
void dlgPackageManager::slot_packageContextMenu(const QPoint& pos)
{
    QListWidgetItem* pItem = packageList->itemAt(pos);
    if (!pItem) {
        return;
    }
    packageList->setCurrentItem(pItem, pItem->isSelected() ? QItemSelectionModel::NoUpdate : QItemSelectionModel::ClearAndSelect);

    // QPointer: exec() runs an event loop, and the profile closing meanwhile deletes
    // this window and the menu parented to it
    QPointer<QMenu> pMenu = buildPackageContextMenu(pItem);
    if (!pMenu) {
        return;
    }
    pMenu->exec(packageList->viewport()->mapToGlobal(pos));
    if (pMenu) {
        pMenu->deleteLater();
    }
}

QMenu* dlgPackageManager::buildPackageContextMenu(QListWidgetItem* pItem)
{
    if (!pItem || !mpHost) {
        return nullptr;
    }

    auto* pMenu = new QMenu(this);
    pMenu->setObjectName(qsl("packagesContextMenu"));
    // Built on demand, so it takes the recipe here rather than from the style pass
    pMenu->setStyleSheet(uiDesign::menuStyleSheet(uiDesign::themeTokens()));
    uiDesign::letPopupsTakeTheFieldsCorner(pMenu);

    if (mCurrentView == NavigationView::Installed) {
        //: Package manager - the list's right-click menu, over an installed package: takes it off the profile
        QAction* pRemove = pMenu->addAction(tr("Remove"));
        pRemove->setObjectName(qsl("packagesMenuRemove"));
        connect(pRemove, &QAction::triggered, this, &dlgPackageManager::slot_removePackages);
        return pMenu;
    }

    QAction* pInstall = pMenu->addAction(mCurrentView == NavigationView::Updates
                                                 //: Package manager - the list's right-click menu in the Updates view: brings the package up to date
                                                 ? tr("Update")
                                                 //: Package manager - the list's right-click menu in the Explore view: fetches the package
                                                 : tr("Install"));
    pInstall->setObjectName(qsl("packagesMenuInstall"));
    connect(pInstall, &QAction::triggered, this, &dlgPackageManager::slot_installPackageFromRepository);
    return pMenu;
}

void dlgPackageManager::setupNavigationButtons()
{
    mpNavigationGroup = new QButtonGroup(this);
    mpNavigationGroup->setExclusive(true);

    mpNavigationGroup->addButton(pushButton_explore, static_cast<int>(NavigationView::Explore));
    mpNavigationGroup->addButton(pushButton_installed, static_cast<int>(NavigationView::Installed));
    mpNavigationGroup->addButton(pushButton_updates, static_cast<int>(NavigationView::Updates));
    syncViewBar();

    connect(mpNavigationGroup, &QButtonGroup::idClicked, this, &dlgPackageManager::slot_navigationButtonClicked);
}

// Blocked, or the bar would click the button that has just changed the view
void dlgPackageManager::syncViewBar()
{
    if (!mpViewBar) {
        return;
    }
    const QSignalBlocker held(mpViewBar);
    mpViewBar->setCurrentIndex(static_cast<int>(mCurrentView));
}

void dlgPackageManager::slot_installPackageFromFile()
{
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value(qsl("lastFileDialogLocation"), QDir::homePath()).toString();

    //: Package manager - import packages from file dialog (multi-select enabled)
    //: Package manager - file filter for supported package types (mpackage, zip, xml)
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import Mudlet Package"), lastDir, tr("Mudlet Packages (*.mpackage *.zip *.xml)"));
    if (fileNames.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileNames.first()).absolutePath();
    settings.setValue(qsl("lastFileDialogLocation"), lastDir);

    QStringList failedPackages;

    for (const QString& fileName : fileNames) {
        if (mpHost->installPackage(fileName, enums::PackageModuleType::Package).first) {
            mpHost->waitForProfileSave();
        } else {
            const QString baseName = QFileInfo(fileName).fileName();
            failedPackages << baseName;
            qWarning() << "dlgPackageManager::slot_installPackageFromFile() ERROR - failed to import" << baseName;
        }
    }

    resetPackageList();
    comeBackToFront();

    if (!failedPackages.isEmpty()) {
        //: Package manager - status message shown when some packages failed to import. %1 is a comma-separated list of package names
        showImportStatus(tr("Failed to import: %1").arg(failedPackages.join(qsl(", "))));
    }
}

void dlgPackageManager::slot_installPackageFromRepository()
{
    // Names, not items: a warning below runs an event loop, in which a package
    // installed or removed by a script rebuilds the list and frees its items
    QStringList selected;
    for (const QListWidgetItem* item : packageList->selectedItems()) {
        selected << item->text();
    }
    if (selected.isEmpty()) {
        return;
    }

    //: Package manager - cancel button text for download progress dialog
    auto progress = new QProgressDialog(tr("Downloading packages..."), tr("Cancel"), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setAutoClose(true);
    progress->setMinimumDuration(0);
    progress->show();

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    auto pendingDownloads = std::make_shared<QHash<QString, QString>>();
    auto remainingDownloads = std::make_shared<int>(selected.size());
    auto activeReplies = std::make_shared<QList<QNetworkReply*>>();
    auto cancelled = std::make_shared<bool>(false);
    bool repoError = false;

    // Tears down the progress dialog and network manager, so must run exactly once: from the reply
    // that takes the count to zero, or after the loop if it is already zero. The guard keeps that true
    // if a refusal below ever counts itself before its message box, whose event loop can finish a download
    auto batchFinished = std::make_shared<bool>(false);
    auto finishBatch = [this, pendingDownloads, manager, progress, batchFinished]() {
        if (*batchFinished) {
            return;
        }
        *batchFinished = true;

        QStringList failedPackages;

        for (auto it = pendingDownloads->begin(); it != pendingDownloads->end(); ++it) {
            const QString& packageName = it.key();
            const QString& filePath = it.value();

            if (mpHost) {
                // Before both calls: during a save an uninstall is refused and an install waits for
                // the save to finish - long after the archive below is deleted.
                mpHost->waitForProfileSave();
                bool readyToInstall = true;
                if (mpHost->mInstalledPackages.contains(packageName)) {
                    readyToInstall = mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package);
                    if (!readyToInstall) {
                        // Installing over a still-listed package fails as "already installed", so the update would vanish unreported
                        failedPackages << packageName;
                        qWarning() << "dlgPackageManager::slot_installPackageFromRepository() ERROR - could not remove the installed" << packageName << "to update it";
                    }
                }
                if (readyToInstall && !mpHost->installPackage(filePath, enums::PackageModuleType::Package).first) {
                    failedPackages << packageName;
                    qWarning() << "dlgPackageManager::slot_installPackageFromRepository() ERROR - failed to install" << packageName;
                }
            }
            QFile::remove(filePath);
        }

        progress->reset();
        // QProgressDialog::closeEvent emits canceled(), running the cancel handler below. Harmless only
        // because it is after the loop: nothing is left to abort, and its files are already gone
        progress->close();
        progress->deleteLater();
        manager->deleteLater();

        resetPackageList();
        comeBackToFront();

        if (!failedPackages.isEmpty()) {
            //: Package manager - status message shown when some packages downloaded from the repository failed to install. %1 is a comma-separated list of package names
            showImportStatus(tr("Failed to install: %1").arg(failedPackages.join(qsl(", "))));
        }
    };

    QObject::connect(progress, &QProgressDialog::canceled, [activeReplies, pendingDownloads, manager, progress, cancelled]() {
        *cancelled = true;
        for (QNetworkReply* reply : *activeReplies) {
            if (reply) {
                reply->abort();
            }
        }
        for (const QString& filePath : pendingDownloads->values()) {
            QFile::remove(filePath);
        }
        pendingDownloads->clear();
        manager->deleteLater();
        progress->deleteLater();
    });

    for (const QString& packageName : std::as_const(selected)) {
        QJsonObject foundObj;
        if (packageLookup.contains(packageName)) {
            foundObj = packageLookup.value(packageName);
        }

        if (foundObj.isEmpty()) {
            //: Package manager: package couldn't be downloaded
            QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' not found in repository").arg(packageName));
            repoError = true;
            (*remainingDownloads.get())--;
            continue;
        }

        QString remoteFileName = foundObj.value("filename").toString();
        remoteFileName = QFileInfo(remoteFileName).fileName();
        if (remoteFileName.isEmpty()) {
            //: Package manager: package couldn't be downloaded
            QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' not found in repository").arg(packageName));
            repoError = true;
            (*remainingDownloads.get())--;
            continue;
        }

        const QByteArray encoded = QUrl::toPercentEncoding(remoteFileName);
        const QString outDir = MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName());
        const QString outPath = outDir + QDir::separator() + remoteFileName;
        QNetworkRequest request(QUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/packages/%1").arg(QString::fromUtf8(encoded))));
        request.setTransferTimeout(30000);
        QNetworkReply* reply = manager->get(request);

        // Parented so closing the dialog mid-download frees the file and its handle on the half-written
        // package: the reply handler below never runs once the dialog, its context object, is gone
        QFile* file = new QFile(outPath, this);
        if (!file->open(QIODevice::WriteOnly)) {
            qWarning() << "dlgPackageManager::slot_installPackageFromRepository() ERROR - could not open" << outPath << "for writing:" << file->errorString();
            //: Package manager: the downloaded package could not be written to the profile folder. %1 is the package name, %2 the reason given by the operating system
            QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' could not be saved to your profile folder: %2").arg(packageName, file->errorString()));
            (*remainingDownloads.get())--;
            file->deleteLater();
            reply->deleteLater();
            continue;
        }

        // Track only once the file is open: the failure above deletes the reply, and a cancel calls
        // abort() on these raw pointers
        activeReplies->append(reply);

        QObject::connect(reply, &QNetworkReply::readyRead, [file, reply]() {
            const QByteArray data = reply->readAll();
            if (file->write(data) != data.size()) {
                qWarning() << "dlgPackageManager::slot_installMultiple() ERROR - failed to write downloaded data:" << file->errorString();
                reply->abort();
            }
        });

        pendingDownloads->insert(packageName, outPath);

        QObject::connect(reply, &QNetworkReply::finished, this, [reply, file, this, outPath, packageName, pendingDownloads, remainingDownloads, cancelled, activeReplies, finishBatch]() {
            const QByteArray data = reply->readAll();
            if (!data.isEmpty() && file->write(data) != data.size()) {
                qWarning() << "dlgPackageManager::slot_installMultiple() ERROR - failed to write final data:" << file->errorString();
            }
            file->close();
            reply->deleteLater();
            file->deleteLater();

            activeReplies->removeOne(reply);

            if (*cancelled) {
                QFile::remove(outPath);
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                //: Package manager: network error, package couldn't be downloaded
                QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' could not be downloaded due to a network error").arg(packageName));
                pendingDownloads->remove(packageName);
            }

            // Exactly once per download past the cancel check: only zero ends the batch, so a second
            // decrement steps over it and leaves the progress dialog up for the rest of the session
            if (--(*remainingDownloads.get()) == 0) {
                finishBatch();
            }
        });
    }

    // Can already be zero: refused selections never download, and replies can finish inside a
    // refusal's modal box before the loop ends. Only the reply handler checks otherwise, so without
    // this the batch is never installed and the progress dialog never closes
    if (*remainingDownloads.get() == 0) {
        finishBatch();
    }

    if (repoError) {
        downloadRepositoryIndex();
    }
}

void dlgPackageManager::slot_itemChanged(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    clearPackageDetails();

    QString packageName = pItem->text();

    if (mCurrentView == NavigationView::Installed) {
        auto packageInfo{mpHost->mPackageInfo.value(packageName)};
        if (packageInfo.isEmpty()) {
            showPackageName(packageName);
            showPackagePlaceholder();
            //: Package manager - shown in the details pane for an installed package that came with no details, such as a plain .xml package. %1 is the package name
            showPlainDescription(tr("'%1' was installed without any details to show: it has no title, author, version or description.").arg(packageName));
            return;
        }

        if (matchesRepositoryPackage(packageName)) {
            pushButton_website->show();
            pushButton_report->show();
        }

        QString description = packageInfo.value(qsl("description"));
        if (!description.isEmpty()) {
            QString packageDir = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), packageName);
            description.replace(QLatin1String("$packagePath"), packageDir);
            showDescription(description);
        }

        auto iconName = packageInfo.value(qsl("icon"));
        if (!iconName.isEmpty()) {
            const auto iconDir = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName, iconName));
            showPackageIcon(QPixmap(iconDir));
        } else {
            showPackagePlaceholder();
        }

        fillPackageDetails(packageName, packageInfo.value(qsl("title")), packageInfo.value(qsl("author")), packageInfo.value(qsl("version")));

    } else if (mCurrentView == NavigationView::Explore) {
        if (packageLookup.contains(packageName)) {
            pushButton_website->show();
            pushButton_report->show();
            downloadIcon(packageName);

            QJsonObject packageObj = packageLookup.value(packageName);
            fillPackageDetails(
                    packageObj.value(qsl("mpackage")).toString(), packageObj.value(qsl("title")).toString(), packageObj.value(qsl("author")).toString(), packageObj.value(qsl("version")).toString());
            showDescription(packageObj.value(qsl("description")).toString());
        } else {
            showMissingFromIndex(packageName);
        }
    } else if (mCurrentView == NavigationView::Updates) {
        if (packageLookup.contains(packageName)) {
            pushButton_website->show();
            pushButton_report->show();
            downloadIcon(packageName);

            QJsonObject packageObj = packageLookup.value(packageName);
            const auto packageInfo = mpHost->mPackageInfo.value(packageName);
            const QString installedVersion = packageInfo.value(qsl("version"));
            const QString repoVersion = packageObj.value(qsl("version")).toString();

            fillPackageDetails(packageObj.value(qsl("mpackage")).toString(), packageObj.value(qsl("title")).toString(), packageObj.value(qsl("author")).toString(), repoVersion);

            label_version->setText(updateVersionText(installedVersion, repoVersion));
            // Cut again: fillPackageDetails() cut the name to fit beside one number, not two
            showPackageName(mPackageNameShown);

            showDescription(packageObj.value(qsl("description")).toString());
        } else {
            showMissingFromIndex(packageName);
        }
    }
}

void dlgPackageManager::slot_navigationButtonClicked(int buttonId)
{
    mCurrentView = static_cast<NavigationView>(buttonId);
    syncViewBar();
    lineEdit_searchBar->clear();
    slot_setPackageList();
}

void dlgPackageManager::slot_applyAppearance()
{
    applyPackageManagerShellStyle();
}

void dlgPackageManager::slot_onIconDownloaded(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    const QString requestedPackage = reply->property("packageName").toString();
    const QListWidgetItem* currentItem = packageList->currentItem();

    if (!currentItem || currentItem->text() != requestedPackage) {
        reply->deleteLater();
        reply->manager()->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(imageData);
        showPackageIcon(pixmap);
    } else {
        showPackagePlaceholder();
    }
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void dlgPackageManager::slot_openBugWebsite()
{
    const QListWidgetItem* currentItem = packageList->currentItem();
    if (!currentItem) {
        return;
    }

    mudlet::self()->openWebPage(qsl("https://github.com/Mudlet/mudlet-package-repository/issues/new?template=package-bug-or-issue.md&title=[Package%20Bug]%20") + currentItem->text());
}

// The index is keyed by name alone and anyone can make a package of the same name. The author is
// copied into the index from the package's config.lua and rarely changes between versions
bool dlgPackageManager::matchesRepositoryPackage(const QString& packageName) const
{
    const auto repositoryPackage = packageLookup.constFind(packageName);
    if (repositoryPackage == packageLookup.cend() || !mpHost->mPackageInfo.contains(packageName)) {
        return false;
    }
    return mpHost->mPackageInfo.value(packageName).value(qsl("author")) == repositoryPackage->value(qsl("author")).toString();
}

QString dlgPackageManager::packageHelpUrl(const QString& packageName) const
{
    // a help URL set by the package's author takes precedence over the generic repository website
    if (matchesRepositoryPackage(packageName)) {
        const QString url = mpHost->mPackageInfo.value(packageName).value(qsl("helpURL"));
        if (!url.isEmpty()) {
            return url;
        }
    }
    return packageLookup.value(packageName).value(qsl("helpURL")).toString();
}

void dlgPackageManager::slot_openPackageWebsite()
{
    const QListWidgetItem* currentItem = packageList->currentItem();
    if (!currentItem) {
        return;
    }

    mudlet::self()->openWebPage(packageWebsiteUrl(currentItem->text()));
}

// The site addresses a package by slug: lower case, each run of non-alphanumerics one
// hyphen ("Achaean System" is /packages/achaean-system)
QString dlgPackageManager::packageWebsiteUrl(const QString& packageName) const
{
    const QString helpUrl = packageHelpUrl(packageName);
    if (!helpUrl.isEmpty()) {
        return helpUrl;
    }

    QString slug;
    bool pendingHyphen = false;
    for (const QChar c : packageName.toLower()) {
        if (c.isLetterOrNumber()) {
            if (pendingHyphen && !slug.isEmpty()) {
                slug.append(QLatin1Char('-'));
            }
            pendingHyphen = false;
            slug.append(c);
        } else {
            pendingHyphen = true;
        }
    }
    return qsl("https://packages.mudlet.org/packages/") + slug;
}

// Returns what to tell the user about refused removals, empty if none. Split from the slot so the
// wording can be tested: the dialog has no Lua entry point and the box below blocks.
QString dlgPackageManager::removePackages(const QStringList& packageNames)
{
    QStringList refusedWhileSaving;
    QStringList noLongerInstalled;
    for (const QString& package : packageNames) {
        if (mpHost->uninstallPackage(package, enums::PackageModuleType::Package)) {
            continue;
        }
        // Otherwise the package went after the row was drawn (a sibling in this selection can remove it
        // from its sysUninstall handler), so do not blame a save that is not running.
        if (mpHost->currentlySavingProfile()) {
            refusedWhileSaving << package;
        } else {
            noLongerInstalled << package;
        }
    }

    QStringList sentences;
    if (!refusedWhileSaving.isEmpty()) {
        //: %1 is a comma separated list of the packages that are still installed
        sentences << tr("These could not be removed while the profile is being saved: %1. Please try again in a moment.").arg(refusedWhileSaving.join(qsl(", ")));
    }
    if (!noLongerInstalled.isEmpty()) {
        //: %1 is a comma separated list of the packages that turned out not to be installed any more
        sentences << tr("These are no longer installed, so there was nothing to remove: %1.").arg(noLongerInstalled.join(qsl(", ")));
    }
    return sentences.join(qsl(" "));
}

void dlgPackageManager::slot_removePackages()
{
    const QList<QListWidgetItem*> selectedItems = packageList->selectedItems();
    QStringList selectedPackages;

    for (QListWidgetItem* item : selectedItems) {
        selectedPackages << item->text();
    }

    const QString msg = removePackages(selectedPackages);
    if (!msg.isEmpty()) {
        //: Title of the dialog that says why a package the user asked to remove was not removed
        QMessageBox::warning(this, tr("Removal failed"), msg);
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();
    comeBackToFront();
}

QListWidgetItem* dlgPackageManager::installedPackageItem(const QString& packageName) const
{
    auto* item = new QListWidgetItem(packageName);
    const auto packageInfo = mpHost->mPackageInfo.value(packageName);
    if (const QString title = packageInfo.value(qsl("title")); !title.isEmpty()) {
        item->setData(Qt::UserRole, title);
    }
    setRowVersion(item, packageInfo.value(qsl("version")));
    if (const QString iconName = packageInfo.value(qsl("icon")); !iconName.isEmpty()) {
        item->setIcon(QIcon(MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName, iconName))));
    }
    return item;
}

bool dlgPackageManager::installedPackageMatches(const QString& packageName, const QString& searchText) const
{
    const auto packageInfo = mpHost->mPackageInfo.value(packageName);
    return packageName.contains(searchText, Qt::CaseInsensitive) || packageInfo.value(qsl("title")).contains(searchText, Qt::CaseInsensitive)
           || packageInfo.value(qsl("description")).contains(searchText, Qt::CaseInsensitive) || packageInfo.value(qsl("author")).contains(searchText, Qt::CaseInsensitive);
}

void dlgPackageManager::slot_searchTextChanged(const QString& searchText)
{
    packageList->clear();

    if (mCurrentView == NavigationView::Installed) {
        // The list it filters is mInstalledPackages: a package installed without a config.lua has no details, but is listed
        for (const QString& packageName : std::as_const(mpHost->mInstalledPackages)) {
            if (installedPackageMatches(packageName, searchText)) {
                packageList->addItem(installedPackageItem(packageName));
            }
        }
    } else if (mCurrentView == NavigationView::Explore) {
        for (const QJsonValue& value : std::as_const(repositoryPackages)) {
            const QJsonObject pkg = value.toObject();
            const QString name = pkg[qsl("mpackage")].toString();
            const QString title = pkg[qsl("title")].toString();
            const QString description = pkg[qsl("description")].toString();
            const QString author = pkg[qsl("author")].toString();

            if (name.contains(searchText, Qt::CaseInsensitive) || title.contains(searchText, Qt::CaseInsensitive) || description.contains(searchText, Qt::CaseInsensitive)
                || author.contains(searchText, Qt::CaseInsensitive)) {
                QListWidgetItem* item = new QListWidgetItem(name);
                if (!title.isEmpty()) {
                    item->setData(Qt::UserRole, title);
                }
                setRowVersion(item, pkg[qsl("version")].toString());
                if (pkg.contains(qsl("icon"))) {
                    const QPixmap pixmap(pkg[qsl("icon")].toString());
                    item->setIcon(QIcon(pixmap));
                }
                packageList->addItem(item);
            }
        }
    } else if (mCurrentView == NavigationView::Updates) {
        for (const QString& packageName : std::as_const(mPackagesWithUpdates)) {
            if (installedPackageMatches(packageName, searchText)) {
                auto* item = installedPackageItem(packageName);
                setRowVersion(item, updateRowVersion(packageName));
                packageList->addItem(item);
            }
        }
    }
}

void dlgPackageManager::slot_setPackageList()
{
    if (!mpHost) {
        return;
    }

    if (!lineEdit_searchBar->text().isEmpty()) {
        clearPackageDetails();
        slot_searchTextChanged(lineEdit_searchBar->text());
        return;
    }

    packageList->clear();
    clearPackageDetails();

    if (mCurrentView == NavigationView::Installed) {
        for (const QString& packageName : std::as_const(mpHost->mInstalledPackages)) {
            packageList->addItem(installedPackageItem(packageName));
        }
    } else if (mCurrentView == NavigationView::Explore) {
        // No early return: the button toggles at the end must still run for this view
        if (readPackageRepositoryFile()) {
            for (const QJsonValue& packageVal : std::as_const(repositoryPackages)) {
                auto item = new QListWidgetItem();
                const QJsonObject packageObj = packageVal.toObject();
                const QString packageName = packageObj.value("mpackage").toString();
                const QString title = packageObj.value("title").toString();
                item->setText(packageName);
                if (!title.isEmpty()) {
                    item->setData(Qt::UserRole, title);
                }
                setRowVersion(item, packageObj.value(qsl("version")).toString());
                packageList->addItem(item);
            }
        } else {
            //: Package manager - shown in the details pane of the Explore view when the list of packages in the repository has not been downloaded or cannot be read
            showPlainDescription(tr(
                    "The list of packages in the repository is not available: it is still downloading, or the download failed. If it does not appear here, reopen the package manager to try again."));
        }
    } else if (mCurrentView == NavigationView::Updates) {
        populatePackagesWithUpdates();

        if (mPackagesWithUpdates.isEmpty()) {
            //: Package manager - message shown in description area when no updates are available
            showPlainDescription(tr("All packages are up to date."));
        }

        for (const QString& packageName : std::as_const(mPackagesWithUpdates)) {
            auto* item = installedPackageItem(packageName);
            setRowVersion(item, updateRowVersion(packageName));
            packageList->addItem(item);
        }
    }

    packageList->setCurrentRow(0);
    // The buttons follow the view, which no selection signal reports
    slot_toggleInstallRepoButton();
    slot_toggleRemoveButton();
}

void dlgPackageManager::slot_toggleInstallRepoButton()
{
    pushButton_installRepo->setVisible(mCurrentView != NavigationView::Installed);

    if (mCurrentView == NavigationView::Explore || mCurrentView == NavigationView::Updates) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_installRepo->setEnabled(selectionCount);
        pushButton_installRepo->setIcon(mCurrentView == NavigationView::Updates ? mUpdateGlyph : mInstallGlyph);

        if (mCurrentView == NavigationView::Updates) {
            if (selectionCount > 1) {
                //: Message on button in package manager to bring several chosen packages up to date. %n is the number of selected packages.
                pushButton_installRepo->setText(tr("Update %n packages", nullptr, selectionCount));
            } else {
                //: Message on button in package manager to bring the chosen package up to date; also its wording with nothing chosen, where the button is disabled.
                pushButton_installRepo->setText(tr("Update"));
            }
            //: Tooltip for button in package manager when in Updates view
            pushButton_installRepo->setToolTip(tr("Update selected packages"));
        } else {
            if (selectionCount > 1) {
                //: Message on button in package manager to fetch several chosen packages. %n is the number of selected packages.
                pushButton_installRepo->setText(tr("Install %n packages", nullptr, selectionCount));
            } else {
                //: Message on button in package manager to fetch the chosen package; also its wording with nothing chosen, where the button is disabled.
                pushButton_installRepo->setText(tr("Install"));
            }
            //: Tooltip for button in package manager when in Explore view
            pushButton_installRepo->setToolTip(tr("Install package from repository"));
        }
    } else {
        //: Message on button in package manager initially and when the view is the "Installed" one
        pushButton_installRepo->setText(tr("Install"));
        pushButton_installRepo->setEnabled(false);
        pushButton_installRepo->setIcon(mInstallGlyph);
        //: Tooltip for button in package manager when in Explore view
        pushButton_installRepo->setToolTip(tr("Install package from repository"));
    }
    scheduleActionRowFit();
}

void dlgPackageManager::slot_toggleRemoveButton()
{
    pushButton_remove->setVisible(mCurrentView == NavigationView::Installed);

    if (mCurrentView == NavigationView::Installed) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_remove->setEnabled(selectionCount);
        if (selectionCount > 1) {
            //: Message on button in package manager to take several chosen packages off the profile. %n is the number of selected packages.
            pushButton_remove->setText(tr("Remove %n packages", nullptr, selectionCount));
        } else {
            //: Message on button in package manager to take the chosen package off the profile; also its wording with nothing chosen, where the button is disabled.
            pushButton_remove->setText(tr("Remove"));
        }
    } else {
        //: Message on button in package manager initially and when the view is NOT the "Installed" one
        pushButton_remove->setText(tr("Remove"));
        pushButton_remove->setEnabled(false);
    }
    scheduleActionRowFit();
}

// No hide timer: which packages failed to install must stay until the reader closes it
void dlgPackageManager::showImportStatus(const QString& message)
{
    if (!mpNotice) {
        return;
    }
    mpNotice->notificationAreaIconLabelError->hide();
    mpNotice->notificationAreaIconLabelInformation->hide();
    mpNotice->notificationAreaIconLabelWarning->show();
    mpNotice->notificationAreaMessageBox->setText(message);
    mpNotice->show();
}

// Every index read relists Explore and Updates, so no row should get here; a blank pane would read as a broken dialog
void dlgPackageManager::showMissingFromIndex(const QString& packageName)
{
    qWarning() << "dlgPackageManager::showMissingFromIndex() WARNING - package not found in the repository index:" << packageName;
    showPackageName(packageName);
    showPackagePlaceholder();
    //: Package manager - shown in the details pane when the selected package is not in the downloaded package index. %1 is the package name
    showPlainDescription(tr("The details of '%1' could not be found in the package index. It may have been renamed or removed; try reopening the package manager.").arg(packageName));
}

void dlgPackageManager::updateUpdatesBadge()
{
    if (mPackagesWithUpdates.size()) {
        //: Package manager - navigation button showing one or more available updates
        pushButton_updates->setText(tr("Updates (%1)").arg(mPackagesWithUpdates.size()));
    } else {
        //: Package manager - navigation button for when there are no updates
        pushButton_updates->setText(tr("Updates"));
    }
    // The hidden button is what slots and tests read; the chip is what the reader sees
    if (mpViewBar && mpViewBar->count() > static_cast<int>(NavigationView::Updates)) {
        const int updates = static_cast<int>(NavigationView::Updates);
        mpViewBar->setTabText(updates, pushButton_updates->text());
        mpViewBar->setAccessibleTabName(updates, pushButton_updates->text());
    }
}

// Sheets go on the columns, never the dialog: mudlet replaces a dialog's sheet with the
// profile's Lua one on show, and these container-scoped rules beat the profile's
void dlgPackageManager::applyPackageManagerShellStyle()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

    const QString leftRules = qsl("#leftPanel { background-color: %1; }").arg(tokens.pane.name())
                              // Only its QTabBar rules apply; ::pane and ::tab-bar miss on a plain widget
                              + uiDesign::tabBarStyleSheet(qsl("#leftPanel"), tokens) + uiDesign::inputStyleSheet(tokens, qsl("#leftPanel")) + uiDesign::buttonStyleSheet(tokens, qsl("#leftPanel"));
    leftPanel->setStyleSheet(leftRules);

    packageList->setStyleSheet(uiDesign::itemRowStyleSheet(qsl("QListWidget#packageList"), tokens, tokens.pane, PackageItemDelegate::cRowGutter)
                               + uiDesign::scrollBarStyleSheet(qsl("QListWidget#packageList"), tokens, tokens.pane));
    // Mirrored in the palette, which is what a view is asked for its row inks; the
    // platform's answers white on the accent's wash
    QPalette rowInks = packageList->palette();
    rowInks.setColor(QPalette::Text, tokens.text);
    rowInks.setColor(QPalette::HighlightedText, tokens.accentText);
    packageList->setPalette(rowInks);

    const QColor versionInk = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage), tokens.text, uiDesign::scmTextMinimumRatio);
    // Bottom-aligned in a box as deep as the name's, the two baselines sit their descents
    // apart; the lift puts the number on the name's baseline
    const int baselineLift = std::max(0, QFontMetrics(label_packageName->font()).descent() - QFontMetrics(label_version->font()).descent());
    label_version->setContentsMargins(0, 0, 0, baselineLift);

    const QString rightRules = qsl("#rightPanel { background-color: %1; }").arg(tokens.page.name()) + uiDesign::inputStyleSheet(tokens, qsl("#rightPanel"))
                               + uiDesign::buttonStyleSheet(tokens, qsl("#rightPanel")) + qsl("#label_packageName, #label_title { color: %1; background: transparent; }").arg(tokens.text.name())
                               + qsl("#label_author { color: %1; background: transparent; font-size: %2pt; font-weight: normal; }")
                                         .arg(tokens.mutedText.name(), QString::number(uiDesign::typeSize(uiDesign::TypeStep::Caption)))
                               + qsl("#label_version { color: %1; background: transparent; font-weight: normal; }").arg(versionInk.name())
                               + uiDesign::scrollBarStyleSheet(qsl("#packageDescription"), tokens, tokens.field);
    rightPanel->setStyleSheet(rightRules);

    // The handle paints each neighbour's tone up to its hairline, and cannot read
    // a colour a stylesheet filled
    leftPanel->setProperty(uiDesign::scmProp_paneTone, tokens.pane);
    rightPanel->setProperty(uiDesign::scmProp_paneTone, tokens.page);
    if (mpSplitter && mpSplitter->handle(1)) {
        mpSplitter->handle(1)->update();
    }

    // A QTextDocument takes colours only from its default stylesheet; withLinkColour() is for a QLabel
    packageDescription->document()->setDefaultStyleSheet(
            qsl("body { color: %1; }"
                "h1, h2, h3, h4, h5, h6 { color: %1; }"
                // On the window's type scale: the document's own h1 is twice the body text
                "h1 { font-size: %6pt; }"
                "h2 { font-size: %7pt; }"
                "h3, h4, h5, h6 { font-size: %8pt; }"
                "a { color: %2; }"
                "pre, code { background-color: %3; color: %1; border: %4px solid %5; }")
                    .arg(tokens.text.name(), tokens.accentText.name(), tokens.page.name(), QString::number(uiDesign::scmInputBorderWidth), tokens.border.name())
                    .arg(QString::number(uiDesign::typeSize(uiDesign::TypeStep::Display)),
                         QString::number(uiDesign::typeSize(uiDesign::TypeStep::Title)),
                         QString::number(uiDesign::typeSize(uiDesign::TypeStep::Body))));
    relayDescription();

    // Tinted pixmaps keep the old appearance's ink until taken again here
    if (mpAction_searchGlyph) {
        mpAction_searchGlyph->setIcon(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/settings-search.svg")), tokens.mutedText)));
    }
    mInstallGlyph = uiDesign::tintedIcon(qsl(":/icons/packages-install.svg"), tokens);
    mUpdateGlyph = uiDesign::tintedIcon(qsl(":/icons/packages-update.svg"), tokens);
    pushButton_installFile->setIcon(uiDesign::tintedIcon(qsl(":/icons/packages-install-file.svg"), tokens));
    pushButton_remove->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-delete.svg"), tokens));
    pushButton_website->setIcon(uiDesign::tintedIcon(qsl(":/icons/about-homepage.svg"), tokens));
    pushButton_report->setIcon(uiDesign::tintedIcon(qsl(":/icons/about-bug.svg"), tokens));
    for (QAbstractButton* pButton : {static_cast<QAbstractButton*>(pushButton_installFile),
                                     static_cast<QAbstractButton*>(pushButton_installRepo),
                                     static_cast<QAbstractButton*>(pushButton_remove),
                                     static_cast<QAbstractButton*>(pushButton_website),
                                     static_cast<QAbstractButton*>(pushButton_report)}) {
        pButton->setIconSize(QSize(scmPackagesButtonGlyphSize, scmPackagesButtonGlyphSize));
    }
    // After the icons and their size, which the measurement includes
    measureActionRowButtons();
    // The toggles hand Install the glyph just tinted for its view
    slot_toggleInstallRepoButton();
    slot_toggleRemoveButton();

    if (QToolButton* pClose = mpNotice ? mpNotice->messageAreaCloseButton : nullptr) {
        pClose->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));
        pClose->setIconSize(QSize(scmPackagesButtonGlyphSize, scmPackagesButtonGlyphSize));
    }

    // A sheet on label_icon is the one sign the placeholder glyph is showing
    if (!label_icon->styleSheet().isEmpty()) {
        showPackagePlaceholder();
    }

    mpPackageItemDelegate->restyle(tokens);

    uiDesign::keepClickFocusOffControls(leftPanel);
    uiDesign::keepClickFocusOffControls(rightPanel);
    uiDesign::letPopupsTakeTheFieldsCorner(leftPanel);
    uiDesign::letPopupsTakeTheFieldsCorner(rightPanel);
}

void dlgPackageManager::closeEvent(QCloseEvent* event)
{
    if (mpSplitter) {
        if (QSettings* pSettings = MudletApp::getQSettings()) {
            pSettings->setValue(QString::fromLatin1(scmPackagesSplitterKey), mpSplitter->saveState());
        }
    }
    if (mudlet::self() && !mudlet::self()->isGoingDown() && mpHost && !mpHost->isClosingDown()) {
        emit packageManagerClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
}

void dlgPackageManager::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // Not in the constructor: on macOS a native window created before it is shown drops the
    // geometry it is handed. First show only, so a hide and re-show keeps a seam dragged since.
    if (!mSplitterRestored) {
        mSplitterRestored = true;
        restoreColumnSplit();
    }
    scheduleHeadlineFit();
    scheduleActionRowFit();
}

// Deferred: the layout settles the rows' new widths only after the resize event
bool dlgPackageManager::eventFilter(QObject* pWatched, QEvent* pEvent)
{
    if (pWatched == rightPanel && pEvent->type() == QEvent::Resize) {
        scheduleHeadlineFit();
        scheduleActionRowFit();
    }
    return QDialog::eventFilter(pWatched, pEvent);
}

void dlgPackageManager::scheduleHeadlineFit()
{
    if (mPackageNameShown.isEmpty()) {
        return;
    }
    QTimer::singleShot(0ms, this, [this]() {
        showPackageName(mPackageNameShown);
    });
}

// Asked of the buttons, since style padding, font and translation move both numbers, and taken
// again each style pass. The glyph-only width becomes the floor: a QPushButton's
// minimumSizeHint() is its whole sizeHint(), which would hold the window open.
void dlgPackageManager::measureActionRowButtons()
{
    for (ActionRowLink& link : mActionRowLinks) {
        QPushButton* pButton = link.pButton;
        pButton->ensurePolished();
        pButton->setMinimumWidth(0);
        pButton->setText(link.word);
        link.worded = pButton->sizeHint().width();
        pButton->setText(QString());
        link.glyphOnly = pButton->sizeHint().width();
        pButton->setMinimumWidth(link.glyphOnly);
        // Left glyph-only if already so, or a narrow window flashes the words on every appearance change
        if (mActionRowWordsShown) {
            pButton->setText(link.word);
        }
    }
}

int dlgPackageManager::actionRowWordedNeed() const
{
    if (!horizontalLayout_4) {
        return 0;
    }
    int need = 0;
    int items = 0;
    for (int index = 0, count = horizontalLayout_4->count(); index < count; ++index) {
        const QLayoutItem* pItem = horizontalLayout_4->itemAt(index);
        QWidget* pWidget = pItem->widget();
        if (pWidget && pWidget->isHidden()) {
            // A layout leaves a hidden widget out altogether, its gap with it
            continue;
        }
        ++items;
        if (!pWidget) {
            // The spacer asks for no width once the row is short of it
            continue;
        }
        // The worded width whatever the button shows now, or the two thresholds would collapse into one
        int wanted = pWidget->sizeHint().width();
        for (const ActionRowLink& link : mActionRowLinks) {
            if (link.pButton == pWidget && link.worded > 0) {
                wanted = link.worded;
                break;
            }
        }
        need += wanted;
    }
    const QMargins rowMargins = horizontalLayout_4->contentsMargins();
    return need + std::max(0, items - 1) * horizontalLayout_4->spacing() + rowMargins.left() + rowMargins.right();
}

// Words come back only a gap's width above where they were dropped, so a drag across the
// breakpoint settles instead of flickering
void dlgPackageManager::fitActionRowToItsColumn()
{
    if (mActionRowFitting || mActionRowLinks.isEmpty() || !rightPanel || !verticalLayout_right) {
        return;
    }
    // Both hidden: nothing to fit until a package that shows either is chosen
    const bool anyOnShow = std::any_of(mActionRowLinks.cbegin(), mActionRowLinks.cend(), [](const ActionRowLink& link) {
        return !link.pButton->isHidden();
    });
    if (!anyOnShow) {
        return;
    }

    const QMargins columnMargins = verticalLayout_right->contentsMargins();
    const int room = rightPanel->width() - columnMargins.left() - columnMargins.right();
    const int need = actionRowWordedNeed();
    if (room <= 0 || need <= 0) {
        return;
    }
    const bool wordsWanted = mActionRowWordsShown ? room >= need : room >= need + horizontalLayout_4->spacing();
    if (wordsWanted == mActionRowWordsShown) {
        return;
    }

    mActionRowFitting = true;
    mActionRowWordsShown = wordsWanted;
    for (const ActionRowLink& link : mActionRowLinks) {
        link.pButton->setText(wordsWanted ? link.word : QString());
    }
    mActionRowFitting = false;
}

void dlgPackageManager::scheduleActionRowFit()
{
    if (mActionRowFitPending) {
        return;
    }
    mActionRowFitPending = true;
    QTimer::singleShot(0ms, this, [this]() {
        mActionRowFitPending = false;
        fitActionRowToItsColumn();
    });
}

void dlgPackageManager::restoreColumnSplit()
{
    if (!mpSplitter) {
        return;
    }

    QSettings* pSettings = MudletApp::getQSettings();
    const QByteArray saved = pSettings ? pSettings->value(QString::fromLatin1(scmPackagesSplitterKey)).toByteArray() : QByteArray();
    if (!saved.isEmpty() && mpSplitter->restoreState(saved)) {
        // restoreState() also restores the saved handle width, which may be another version's
        mpSplitter->setHandleWidth(uiDesign::GripSplitter::scmHandleThickness);
        return;
    }

    const int across = mpSplitter->width() - uiDesign::GripSplitter::scmHandleThickness;
    if (across <= 0) {
        return;
    }
    const int list = std::max(leftPanel->minimumWidth(), across / scmPackagesListShareDenominator);
    mpSplitter->setSizes({list, across - list});
}
