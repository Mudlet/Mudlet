/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2022, 2024-2025 by Stephen Lyons             *
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


#include "dlgPackageExporter.h"
#include "ui_dlgPackageExporter.h"

#include "ChipRow.h"
#include "ExporterTreeDelegate.h"
#include "GripSplitter.h"
#include "Host.h"
#include "LuaLiteral.h"
#include "MudletApp.h"
#include "dlgSystemMessageArea.h"
#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "XMLexport.h"

#include <QSaveFile>
#include <QtConcurrentRun>
#include <QAbstractItemModel>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QFontMetrics>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QResizeEvent>
#include <QSettings>
#include <QShowEvent>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtMath>
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

static constexpr int scmExporterColumnInset = uiDesign::scmTabPaneInset;
// Matches PackageItemDelegate::cRowGutter, so the two windows' rows line up
static constexpr int scmExporterRowGutter = uiDesign::scmAccentBarWidth + 5;
static constexpr int scmExporterIconSize = 48;
// Set on the icon box while an image it would take is dragged over it
static constexpr char scmProp_iconDropTarget[] = "exporterIconDropTarget";
static constexpr int scmExporterIconGlyphSize = 24;
static constexpr int scmExporterButtonGlyphSize = 16;
// No version string anybody writes needs more room than this
static constexpr int scmExporterVersionWidth = 120;
// Only until the first layout: fitAssetsList() measures the height off the rows from then on
static constexpr int scmExporterDropZoneHeight = 64;
static constexpr int scmExporterDescriptionLines = 4;
static constexpr int scmExporterDropZoneLines = 2;
// A monospaced face reads larger than the proportional words beside it at one size
static constexpr qreal scmExporterPathFontScale = 0.9;
static constexpr int scmExporterTreeShareDenominator = 3;
static constexpr int scmExporterWidth = 880;
static constexpr int scmExporterHeight = 640;
static constexpr char scmExporterSplitterKey[] = "packageExporterSplitterState";

// We are now using code that won't work with really old versions of libzip;
// some of the error handling was improved in 1.0 . Unfortunately libzip 1.7.0
// (and one or two other recent versions) forgot to include the version defines
// and thus broke a test depending on them:
#if defined(LIBZIP_VERSION_MAJOR) && (LIBZIP_VERSION_MAJOR < 1)
#error Mudlet requires a version of libzip of at least 1.0
#endif

dlgPackageExporter::dlgPackageExporter(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(std::make_unique<Ui::dlgPackageExporter>())
, mpHost(pHost)
{
    ui->setupUi(this);
    ui->splitter_metadataAssets->hide();
    ui->pushButton_removeIcon->hide();

    mFreshVersion = ui->lineEdit_version->text();

    mpExportSelection = ui->treeWidget_exportSelection;
    mpSelectionText = ui->groupBox_exportSelection;

    mpTriggers = new QTreeWidgetItem({tr("Triggers")});
    mpAliases = new QTreeWidgetItem({tr("Aliases")});
    mpTimers = new QTreeWidgetItem({tr("Timers")});
    mpScripts = new QTreeWidgetItem({tr("Scripts")});
    mpKeys = new QTreeWidgetItem({tr("Keys")});
    mpButtons = new QTreeWidgetItem({tr("Buttons")});

    mpExportSelection->addTopLevelItem(mpTriggers);
    mpExportSelection->addTopLevelItem(mpAliases);
    mpExportSelection->addTopLevelItem(mpTimers);
    mpExportSelection->addTopLevelItem(mpScripts);
    mpExportSelection->addTopLevelItem(mpKeys);
    mpExportSelection->addTopLevelItem(mpButtons);

    mpExportSelection->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpExportSelection, &QTreeWidget::itemChanged, this, &dlgPackageExporter::slot_recountItems);
    connect(mpExportSelection, &QTreeWidget::customContextMenuRequested, this, &dlgPackageExporter::slot_rightClickOnItems);

    mCancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    // make sure the Cancel button doesn't close the dialog
    ui->buttonBox->addButton(mCancelButton, QDialogButtonBox::ResetRole);
    mCancelButton->setVisible(false);
    mCloseButton = ui->buttonBox->button(QDialogButtonBox::Close);

    // This button has the ApplyRole which applies current changes but does NOT
    // cause the dialog to close:
    mExportButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    //: Text for button to perform the package export on the items the user has selected.
    mExportButton->setText(tr("Export"));

    // reset mPackagePathFileName and mXmlPathFileName from possible previous use
    mPackagePathFileName.clear();
    mXmlPathFileName.clear();
    connect(ui->addFiles, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_addFiles);
    connect(mExportButton, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_exportPackage);
    connect(ui->pushButton_packageLocation, &QPushButton::clicked, this, &dlgPackageExporter::slot_openPackageLocation);
    connect(ui->lineEdit_packageName, &QLineEdit::textChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    connect(ui->lineEdit_author, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->lineEdit_title, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->lineEdit_version, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    // For the start-over button, which checkToEnableExportButton() also refreshes
    connect(ui->lineEdit_helpUrl, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->textEdit_description, &QTextEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(this, &dlgPackageExporter::signal_exportLocationChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    slot_updateLocationPlaceholder();
    connect(ui->packageList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgPackageExporter::slot_packageChanged);
    connect(ui->pushButton_addDependency, &QPushButton::clicked, this, &dlgPackageExporter::slot_addDependency);
    connect(ui->pushButton_removeDependency, &QPushButton::clicked, this, &dlgPackageExporter::slot_removeDependency);
    connect(ui->pushButton_addIcon, &QPushButton::clicked, this, &dlgPackageExporter::slot_importIcon);
    connect(ui->pushButton_removeIcon, &QPushButton::clicked, this, &dlgPackageExporter::slot_removeIcon);
    connect(mCancelButton, &QPushButton::clicked, this, &dlgPackageExporter::slot_cancelExport);

    ui->listWidget_addedFiles->installEventFilter(this);
    ui->comboBox_dependencies->installEventFilter(this);
    ui->textEdit_description->installEventFilter(this);
    ui->widget_icon->setAcceptDrops(true);
    ui->widget_icon->installEventFilter(this);

    // default package template - prevent clearing of text upon click
    dlgPackageExporter* te_parent = static_cast<dlgPackageExporter*>(topLevelWidget());
    te_parent->mPlainDescription = ui->textEdit_description->toPlainText();

    buildShell();

    populateDependencies();
    syncDependencyChips();

    listTriggers();
    listAliases();
    listKeys();
    listScripts();
    listActions();
    listTimers();

    //: Title of the window. The %1 will be replaced by the current profile's name
    setWindowTitle(tr("Package Exporter - %1").arg(mpHost->getName()));

    // Set the previous details if saved
    QSettings& settings = *MudletApp::getQSettings();
    auto packageAuthor = settings.value(qsl("packageAuthor"), QString()).toString();
    mFreshAuthor = packageAuthor;
    if (!packageAuthor.isEmpty()) {
        ui->lineEdit_author->setText(packageAuthor);
    }

    // Ensure this dialog goes away if the Host (profile) is closed while we are
    // open - as this is parented to the mudlet instance rather than the Host
    // of the profile it would otherwise be left around if only the profile was
    // being closed:
    connect(mpHost, &QObject::destroyed, this, &dlgPackageExporter::slot_cancelExport);
    connect(mpHost, &QObject::destroyed, this, &dlgPackageExporter::close);

    applyPackageExporterShellStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgPackageExporter::slot_applyAppearance);

    // Last: the description must already hold its template
    mFreshDescription = ui->textEdit_description->toPlainText();
    refreshStartOverButton();
}

dlgPackageExporter::~dlgPackageExporter()
{
    // The files list's model still emits while ~QWidget deletes the children, after
    // this class is torn down, so its connections to member slots are cut here
    if (ui && ui->listWidget_addedFiles) {
        if (QAbstractItemModel* pModel = ui->listWidget_addedFiles->model()) {
            disconnect(pModel, nullptr, this, nullptr);
        }
    }

    // An export still in flight loses its finished handler along with this dialog
    if (!mExportRunning) {
        return;
    }
    QApplication::restoreOverrideCursor();
    if (!mpZipWatcher) {
        releaseExportLock();
        return;
    }
    // Cancelled or not, the zip may still be reading its staging folder, so the lock is held until it ends
    auto* watcher = mpZipWatcher;
    watcher->setParent(qApp);
    connect(watcher,
            &QFutureWatcherBase::finished,
            qApp,
            [watcher, cancelled = mpZipCancelled, isModule = mIsModuleCreationMode, packageName = mPackageName, packagePathFileName = mPackagePathFileName]() {
                releaseExportLock();
                if (auto [isOk, errorMsg] = watcher->result(); !isOk) {
                    if (!cancelled->load()) {
                        qWarning().noquote().nospace() << "dlgPackageExporter WARNING - exporting \"" << packageName << "\" failed after its dialog was closed: " << errorMsg;
                    }
                } else if (isModule) {
                    qWarning().noquote().nospace() << "dlgPackageExporter WARNING - module \"" << packageName << "\" was written to \"" << packagePathFileName
                                                   << "\" but not installed, as its dialog was closed before the export finished";
                }
                watcher->deleteLater();
            });
}

void dlgPackageExporter::releaseExportLock()
{
    mExportingPackage = false;
    recheckEveryExportButton();
}

// Every open exporter's Export button follows the shared lock, not only the one that took or released it
void dlgPackageExporter::recheckEveryExportButton()
{
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (auto* exporter = qobject_cast<dlgPackageExporter*>(widget)) {
            exporter->checkToEnableExportButton();
        }
    }
}

// Every .ui control keeps its object name and connections: the other exporter tests press them
void dlgPackageExporter::buildShell()
{
    resize(scmExporterWidth, scmExporterHeight);

    // So the column tones reach the window's edges and the seam runs its whole height
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(0);

    ui->label_or->hide();
    //: Package exporter - prompt in the empty name field
    ui->lineEdit_packageName->setPlaceholderText(tr("Package name"));
    uiDesign::markAsShellSurface(ui->widget_newOrExistingPackageName);
    ui->horizontalLayout_packageName->setContentsMargins(scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset);

    mpButton_startOver = new QPushButton(ui->widget_newOrExistingPackageName);
    mpButton_startOver->setObjectName(qsl("exporterStartOverButton"));
    //: Package exporter - button on the top row that puts every field, tick, chip, picture and file back to how the window opened; disabled while there is nothing to clear
    mpButton_startOver->setText(tr("Clear all fields and selections"));
    //: Package exporter - tooltip on the button that clears every field, tick and file
    mpButton_startOver->setToolTip(tr("Puts every field, tick and file back to how the window opened"));
    connect(mpButton_startOver, &QAbstractButton::clicked, this, &dlgPackageExporter::requestStartOver);
    ui->horizontalLayout_packageName->insertWidget(0, mpButton_startOver);
    ui->horizontalLayout_packageName->insertStretch(1, 1);

    buildSelectionColumn();
    buildDetailsColumn();
    buildFoot();

    uiDesign::removeFromLayoutTree(ui->verticalLayout, ui->splitter_metadata);
    ui->splitter_metadata->hide();
    ui->frame_metadata->hide();
    ui->pushButton_openInfos->hide();

    mpSplitter = new uiDesign::GripSplitter(Qt::Horizontal, this);
    mpSplitter->setObjectName(qsl("exporterSplitter"));
    uiDesign::markAsShellSurface(mpSplitter);
    mpSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpSplitter->setChildrenCollapsible(false);
    mpSplitter->addWidget(mpSelectionColumn);
    mpSplitter->addWidget(mpDetailsColumn);
    // Per pane as well: QSplitter::isCollapsible() answers a pane never told with
    // the default, not with what setChildrenCollapsible() set
    mpSplitter->setCollapsible(0, false);
    mpSplitter->setCollapsible(1, false);
    mpSplitter->setStretchFactor(0, 0);
    mpSplitter->setStretchFactor(1, 1);
    ui->verticalLayout->insertWidget(1, mpSplitter);
    ui->verticalLayout->addWidget(mpFoot);

    showModeWords();
    showSelectionCount(0);
    slot_updateLocationPlaceholder();
}

void dlgPackageExporter::buildSelectionColumn()
{
    mpSelectionColumn = new QWidget(this);
    mpSelectionColumn->setObjectName(qsl("exporterSelectionColumn"));
    uiDesign::markAsShellSurface(mpSelectionColumn);
    auto* pColumn = new QVBoxLayout(mpSelectionColumn);
    pColumn->setContentsMargins(scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset);

    mpLabel_selectionHeading = new QLabel(mpSelectionColumn);
    mpLabel_selectionHeading->setObjectName(qsl("exporterSelectionHeading"));
    mpLabel_selectionCount = new QLabel(mpSelectionColumn);
    mpLabel_selectionCount->setObjectName(qsl("exporterSelectionCount"));
    auto* pHeadingRow = new QHBoxLayout();
    pHeadingRow->setContentsMargins(0, 0, 0, 0);
    pHeadingRow->addWidget(mpLabel_selectionHeading);
    pHeadingRow->addWidget(mpLabel_selectionCount);
    pHeadingRow->addStretch(1);
    pColumn->addLayout(pHeadingRow);

    uiDesign::removeFromLayoutTree(ui->horizontalLayout, ui->treeWidget_exportSelection);
    pColumn->addWidget(ui->treeWidget_exportSelection, 1);

    // The item-row recipe already tells rows apart; banding would be a second cue
    ui->treeWidget_exportSelection->setAlternatingRowColors(false);
    // Rows elide, so there is nothing to scroll to sideways
    ui->treeWidget_exportSelection->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpTreeDelegate = new ExporterTreeDelegate(this);
    ui->treeWidget_exportSelection->setItemDelegate(mpTreeDelegate);

    mpLabel_selectionHint = new QLabel(mpSelectionColumn);
    mpLabel_selectionHint->setObjectName(qsl("exporterSelectionHint"));
    mpLabel_selectionHint->setWordWrap(true);
    //: Package exporter - caption under the tree of things to export
    mpLabel_selectionHint->setText(tr("Ticking a folder takes everything in it. Right-click a folder to take the folder alone.")
                                   + QChar(' ')
                                   //: Package exporter - second sentence of the caption under the tree of things to export, about a folder that carries the package's own name
                                   + tr("A folder named after the package is made again on install, so only what is inside it goes in."));
    pColumn->addWidget(mpLabel_selectionHint);
}

void dlgPackageExporter::buildDetailsColumn()
{
    mpDetailsColumn = new QWidget(this);
    mpDetailsColumn->setObjectName(qsl("exporterDetailsColumn"));
    uiDesign::markAsShellSurface(mpDetailsColumn);
    auto* pColumn = new QVBoxLayout(mpDetailsColumn);
    pColumn->setContentsMargins(scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset);

    mpLabel_detailsHeading = new QLabel(mpDetailsColumn);
    mpLabel_detailsHeading->setObjectName(qsl("exporterDetailsHeading"));
    mpLabel_detailsHint = new QLabel(mpDetailsColumn);
    mpLabel_detailsHint->setObjectName(qsl("exporterDetailsHint"));
    //: Package exporter - note beside the heading over the package's details, saying none of the fields under it are required
    mpLabel_detailsHint->setText(tr("Everything here is optional"));
    auto* pHeadingRow = new QHBoxLayout();
    pHeadingRow->setContentsMargins(0, 0, 0, 0);
    pHeadingRow->addWidget(mpLabel_detailsHeading);
    pHeadingRow->addWidget(mpLabel_detailsHint);
    pHeadingRow->addStretch(1);
    pColumn->addLayout(pHeadingRow);

    const auto stackedField = [](QWidget* pLabel, QWidget* pField) {
        auto* pStack = new QVBoxLayout();
        pStack->setContentsMargins(0, 0, 0, 0);
        pStack->addWidget(pLabel);
        pStack->addWidget(pField);
        return pStack;
    };

    uiDesign::removeFromLayoutTree(ui->horizontalLayout_packageName, ui->lineEdit_packageName);
    mpLabel_nameCaption = new QLabel(mpDetailsColumn);
    mpLabel_nameCaption->setObjectName(qsl("exporterNameLabel"));
    mpLabel_nameCaption->setBuddy(ui->lineEdit_packageName);
    pColumn->addLayout(stackedField(mpLabel_nameCaption, ui->lineEdit_packageName));

    for (QWidget* pWidget : {static_cast<QWidget*>(ui->label_author),
                             static_cast<QWidget*>(ui->lineEdit_author),
                             static_cast<QWidget*>(ui->label_version),
                             static_cast<QWidget*>(ui->lineEdit_version),
                             static_cast<QWidget*>(ui->label_shortDescription),
                             static_cast<QWidget*>(ui->lineEdit_title),
                             static_cast<QWidget*>(ui->label_helpUrl),
                             static_cast<QWidget*>(ui->lineEdit_helpUrl),
                             static_cast<QWidget*>(ui->label_icon),
                             static_cast<QWidget*>(ui->widget_icon),
                             static_cast<QWidget*>(ui->label_requiredPackages),
                             static_cast<QWidget*>(ui->label_detailedDescription),
                             static_cast<QWidget*>(ui->textEdit_description)}) {
        uiDesign::removeFromLayoutTree(ui->gridLayout_metadata, pWidget);
    }

    //: Package exporter - prompt in the empty author field
    ui->lineEdit_author->setPlaceholderText(tr("Your name"));
    ui->lineEdit_version->setFixedWidth(scmExporterVersionWidth);
    auto* pAuthorRow = new QHBoxLayout();
    pAuthorRow->setContentsMargins(0, 0, 0, 0);
    pAuthorRow->addLayout(stackedField(ui->label_author, ui->lineEdit_author), 1);
    pAuthorRow->addLayout(stackedField(ui->label_version, ui->lineEdit_version), 0);
    pColumn->addLayout(pAuthorRow);

    //: Package exporter - word over the field holding the package's one-line summary
    ui->label_shortDescription->setText(tr("In one line"));
    //: Package exporter - prompt in the empty one-line summary field
    ui->lineEdit_title->setPlaceholderText(tr("What it does, for the package manager's list"));
    pColumn->addLayout(stackedField(ui->label_shortDescription, ui->lineEdit_title));

    //: Package exporter - word over the field holding the address of the package's help page
    ui->label_helpUrl->setText(tr("Help page"));
    buildIconRow();
    auto* pHelpRow = new QHBoxLayout();
    pHelpRow->setContentsMargins(0, 0, 0, 0);
    pHelpRow->addLayout(stackedField(ui->label_helpUrl, ui->lineEdit_helpUrl), 1);
    pHelpRow->addLayout(stackedField(ui->label_icon, ui->widget_icon), 0);
    pColumn->addLayout(pHelpRow);

    //: Package exporter - word over the row of packages this one needs to be installed alongside
    ui->label_requiredPackages->setText(tr("Needs these packages"));
    mpChipRow_dependencies = new uiDesign::ChipRow(mpDetailsColumn);
    mpChipRow_dependencies->setObjectName(qsl("exporterDependencies"));
    mpChipRow_dependencies->setAddWords(
            //: Package exporter - button at the end of the row of required packages, which opens a field for another one
            tr("Add package"),
            //: Package exporter - tooltip on the button that opens the field for another package this one needs
            tr("Name another package this one needs"),
            //: Package exporter - prompt in the field a required package's name is typed into
            tr("package name"));
    mpChipRow_dependencies->setChipWords(
            //: Package exporter - accessible name of one package in the row of packages this one needs; %1 is that package's name
            tr("Required package %1"),
            //: Package exporter - tooltip on the cross that takes one package off the row of packages this one needs; %1 is that package's name
            tr("No longer needs %1"));
    connect(mpChipRow_dependencies, &uiDesign::ChipRow::itemsChanged, this, &dlgPackageExporter::slot_dependencyChipsChanged);
    // Hidden, not removed: the export reads comboBox_dependencies and the other
    // exporter tests drive these
    ui->DependencyList->hide();
    ui->pushButton_addDependency->hide();
    ui->pushButton_removeDependency->hide();
    ui->comboBox_dependencies->hide();
    pColumn->addLayout(stackedField(ui->label_requiredPackages, mpChipRow_dependencies));

    //: Package exporter - word over the package's full description
    ui->label_detailedDescription->setText(tr("Description"));
    mpLabel_descriptionHint = new QLabel(mpDetailsColumn);
    mpLabel_descriptionHint->setObjectName(qsl("exporterDescriptionHint"));
    //: Package exporter - note beside the Description word, saying where the words are read and what they may be written in
    mpLabel_descriptionHint->setText(tr("shown in the package manager, Markdown works"));
    auto* pDescriptionHead = new QHBoxLayout();
    pDescriptionHead->setContentsMargins(0, 0, 0, 0);
    pDescriptionHead->addWidget(ui->label_detailedDescription);
    pDescriptionHead->addWidget(mpLabel_descriptionHint);
    pDescriptionHead->addStretch(1);
    ui->textEdit_description->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* pDescription = new QVBoxLayout();
    pDescription->setContentsMargins(0, 0, 0, 0);
    pDescription->addLayout(pDescriptionHead);
    pDescription->addWidget(ui->textEdit_description, 1);
    pColumn->addLayout(pDescription, 1);

    buildAssetsRow();
    pColumn->addWidget(ui->widget_assets);
    mpDetailsColumn->installEventFilter(this);
}

void dlgPackageExporter::buildIconRow()
{
    ui->Icon->setFixedSize(scmExporterIconSize, scmExporterIconSize);
    // The pixmap arrives scaled; scaling it again in the label undoes that
    ui->Icon->setScaledContents(false);
    ui->Icon->setAlignment(Qt::AlignCenter);
    ui->Icon->show();

    //: Package exporter - button that opens a file picker for the package's picture
    ui->pushButton_addIcon->setText(tr("Choose an icon"));
    ui->pushButton_addIcon->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->pushButton_removeIcon->setText(QString());
    ui->pushButton_removeIcon->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->pushButton_removeIcon->setFixedSize(uiDesign::scmInputHeight, uiDesign::scmInputHeight);
    //: Package exporter - the button that takes the chosen picture off the package again, read out to a screen reader and shown as its tooltip
    const QString removeIconWord = tr("Remove the icon");
    ui->pushButton_removeIcon->setAccessibleName(removeIconWord);
    ui->pushButton_removeIcon->setToolTip(removeIconWord);

    // A disabled label is announced as unavailable; the caption tone quietens it instead
    ui->label->setEnabled(true);
    //: Package exporter - note beside the icon box saying what size of picture to hand it, and that an image file can be dropped on the box. Keep the line break: the note stands beside a box two lines tall
    ui->label->setText(tr("512 x 512 works best\nor drop an image here"));

    // The .ui pads the row for a drop outline drawn around it; the box itself shows the drop now, and the padding
    // set the row two pixels off the help page field beside it
    ui->horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
    uiDesign::removeFromLayoutTree(ui->horizontalLayout_2, ui->Icon);
    ui->horizontalLayout_2->insertWidget(0, ui->Icon);
    ui->horizontalLayout_2->addStretch(1);
}

void dlgPackageExporter::buildAssetsRow()
{
    //: Package exporter - word over the list of files shipped inside the package
    ui->label_assets->setText(tr("Files to ship with it"));
    mpLabel_assetsHint = new QLabel(ui->widget_assets);
    mpLabel_assetsHint->setObjectName(qsl("exporterAssetsHint"));
    //: Package exporter - note beside the Files word, giving examples of what a package ships
    mpLabel_assetsHint->setText(tr("images, sounds, fonts"));
    uiDesign::removeFromLayoutTree(ui->verticalLayout_4, ui->label_assets);
    auto* pAssetsHead = new QHBoxLayout();
    pAssetsHead->setContentsMargins(0, 0, 0, 0);
    pAssetsHead->addWidget(ui->label_assets);
    pAssetsHead->addWidget(mpLabel_assetsHint);
    pAssetsHead->addStretch(1);
    ui->verticalLayout_4->insertLayout(0, pAssetsHead);
    // Fixed: a growing row would take spare column height, and fitAssetsList()
    // assumes only the description and the list share it
    ui->widget_assets->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // A QListWidget's size hint asks for about 200px, taken out of the description
    ui->listWidget_addedFiles->setFixedHeight(scmExporterDropZoneHeight);
    mpLabel_assetsEmpty = new QLabel(ui->listWidget_addedFiles->viewport());
    mpLabel_assetsEmpty->setObjectName(qsl("exporterAssetsEmpty"));
    mpLabel_assetsEmpty->setAlignment(Qt::AlignCenter);
    //: Package exporter - prompt on the empty zone the files to ship are dropped on
    mpLabel_assetsEmpty->setText(tr("Drop files or folders here"));
    // Or the prompt swallows the drop it is inviting
    mpLabel_assetsEmpty->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* pViewportLayout = new QVBoxLayout(ui->listWidget_addedFiles->viewport());
    pViewportLayout->setContentsMargins(0, 0, 0, 0);
    pViewportLayout->addWidget(mpLabel_assetsEmpty);
    // Through the model: QListWidget has no signal for an added item. The model
    // emits while ~QWidget tears the window down, after this class is gone, so the
    // lambda holds the two widgets as QPointers rather than `this`.
    const QPointer<QLabel> pPrompt = mpLabel_assetsEmpty;
    const QPointer<QListWidget> pFiles = ui->listWidget_addedFiles;
    const auto promptFollowsTheList = [pPrompt, pFiles]() {
        if (pPrompt && pFiles) {
            pPrompt->setVisible(pFiles->count() == 0);
        }
    };
    QAbstractItemModel* pModel = ui->listWidget_addedFiles->model();
    connect(pModel, &QAbstractItemModel::rowsInserted, mpLabel_assetsEmpty, promptFollowsTheList);
    connect(pModel, &QAbstractItemModel::rowsRemoved, mpLabel_assetsEmpty, promptFollowsTheList);
    connect(pModel, &QAbstractItemModel::modelReset, mpLabel_assetsEmpty, promptFollowsTheList);
    // Member slots, so the destructor cuts these for the same reason
    connect(pModel, &QAbstractItemModel::rowsInserted, this, &dlgPackageExporter::refreshStartOverButton);
    connect(pModel, &QAbstractItemModel::rowsRemoved, this, &dlgPackageExporter::refreshStartOverButton);
    connect(pModel, &QAbstractItemModel::modelReset, this, &dlgPackageExporter::refreshStartOverButton);
    connect(pModel, &QAbstractItemModel::rowsInserted, this, &dlgPackageExporter::fitAssetsList);
    connect(pModel, &QAbstractItemModel::rowsRemoved, this, &dlgPackageExporter::fitAssetsList);
    connect(pModel, &QAbstractItemModel::modelReset, this, &dlgPackageExporter::fitAssetsList);
    showAssetsPrompt();

    //: Package exporter - button that opens a file picker for the files to ship with the package
    ui->addFiles->setText(tr("Add files"));
    ui->addFiles->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // Leading edge, not centred: the row starts where the field above it does
    for (QSpacerItem* pSpacer : {ui->horizontalSpacer_2, ui->horizontalSpacer}) {
        if (const int at = ui->horizontalLayout_3->indexOf(pSpacer); at >= 0) {
            delete ui->horizontalLayout_3->takeAt(at);
        }
    }
    mpLabel_assetsDropHint = new QLabel(ui->widget_addFilesButton);
    mpLabel_assetsDropHint->setObjectName(qsl("exporterAssetsDropHint"));
    //: Package exporter - note beside the Add files button, saying the list takes a drop as well
    mpLabel_assetsDropHint->setText(tr("or drop them on the list"));
    ui->horizontalLayout_3->setSpacing(uiDesign::scmNoticeSpacing);
    ui->horizontalLayout_3->addWidget(mpLabel_assetsDropHint);
    ui->horizontalLayout_3->addStretch(1);
}

void dlgPackageExporter::buildFoot()
{
    mpFoot = new QWidget(this);
    mpFoot->setObjectName(qsl("exporterFoot"));
    uiDesign::markAsShellSurface(mpFoot);
    auto* pFootLayout = new QVBoxLayout(mpFoot);
    pFootLayout->setContentsMargins(scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset, scmExporterColumnInset);

    mpLabel_saveCaption = new QLabel(mpFoot);
    mpLabel_saveCaption->setObjectName(qsl("exporterSaveCaption"));
    mpLabel_savePath = new QLabel(mpFoot);
    mpLabel_savePath->setObjectName(qsl("exporterSavePath"));
    // A QLabel's minimum width is its whole text, which would hold the window that
    // wide; fitSavePath() elides it to the room instead
    mpLabel_savePath->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    // lineEdit_filePath stays hidden: tests read the export location off its placeholder
    uiDesign::removeFromLayoutTree(ui->horizontalLayout_4, ui->pushButton_packageLocation);
    uiDesign::removeFromLayoutTree(ui->verticalLayout, ui->widget_exportLocation);
    ui->widget_exportLocation->hide();
    ui->lineEdit_filePath->hide();

    auto* pSaveRow = new QHBoxLayout();
    pSaveRow->setContentsMargins(0, 0, 0, 0);
    pSaveRow->addWidget(mpLabel_saveCaption);
    pSaveRow->addWidget(mpLabel_savePath, 1);
    pSaveRow->addWidget(ui->pushButton_packageLocation);
    pFootLayout->addLayout(pSaveRow);

    // infoLabel stays hidden and written to: the exporter tests read results off it
    uiDesign::removeFromLayoutTree(ui->verticalLayout, ui->infoLabel);
    ui->infoLabel->hide();
    mpNotice = new dlgSystemMessageArea(mpFoot);
    mpNotice->setObjectName(qsl("exporterNotice"));
    mpNotice->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
    if (QLayout* pNoticeLayout = mpNotice->layout()) {
        pNoticeLayout->setContentsMargins(0, 0, 0, 0);
    }
    mpNotice->hide();
    mpNotice->notificationAreaMessageBox->setOpenExternalLinks(true);
    mpNotice->notificationAreaMessageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(mpNotice->messageAreaCloseButton, &QAbstractButton::clicked, mpNotice, &QWidget::hide);
    pFootLayout->addWidget(mpNotice);

    uiDesign::removeFromLayoutTree(ui->verticalLayout, ui->buttonBox);
    pFootLayout->addWidget(ui->buttonBox);

    mExportButton->setProperty(uiDesign::scmProp_primaryButton, true);
    uiDesign::repolish(mExportButton);
}

void dlgPackageExporter::showModeWords()
{
    if (!mpLabel_selectionHeading || !mpLabel_nameCaption) {
        return;
    }
    if (mIsModuleCreationMode) {
        //: Package exporter - heading over the tree of things to include, while a module is being made
        mpLabel_selectionHeading->setText(tr("Select items to include in module"));
        //: Package exporter - heading over the module's own details
        mpLabel_detailsHeading->setText(tr("About the module"));
        //: Package exporter - word over the first field of the form, which the module is named in
        mpLabel_nameCaption->setText(tr("Module name"));
        //: Package exporter - caption in the foot, before the path the module is written to
        mpLabel_saveCaption->setText(tr("Saves into the profile as"));
    } else {
        //: This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.
        mpLabel_selectionHeading->setText(tr("Select what to export"));
        //: Package exporter - heading over the package's own details
        mpLabel_detailsHeading->setText(tr("About the package"));
        //: Package exporter - word over the first field of the form, which the package is named in
        mpLabel_nameCaption->setText(tr("Package name"));
        //: Package exporter - caption in the foot, before the path the package is written to
        mpLabel_saveCaption->setText(tr("Saves to"));
    }
    //: Package exporter - button in the foot that opens a picker for where the package is written
    ui->pushButton_packageLocation->setText(tr("Change folder"));
}

void dlgPackageExporter::showSelectionCount(const int itemsToExport)
{
    if (!mpLabel_selectionCount) {
        return;
    }
    // Separate strings: with no translation loaded, Qt does not pluralise a numerus form
    if (itemsToExport == 1) {
        //: Package exporter - chip beside the heading over the tree while exactly one thing is ticked
        mpLabel_selectionCount->setText(tr("1 item"));
    } else if (itemsToExport > 1) {
        //: Package exporter - chip beside the heading over the tree, saying how many things are ticked. %n is that count, two or more.
        mpLabel_selectionCount->setText(tr("%n items", nullptr, itemsToExport));
    } else {
        //: Package exporter - chip beside the heading over the tree while nothing at all is ticked
        mpLabel_selectionCount->setText(tr("nothing yet"));
    }
    mpLabel_selectionCount->setProperty("exporterCountEmpty", itemsToExport <= 0);
    uiDesign::repolish(mpLabel_selectionCount);
}

// comboBox_dependencies is what the export writes, so it stays the one record of
// what the package needs and the chip row is a view over it
void dlgPackageExporter::syncDependencyChips()
{
    if (!mpChipRow_dependencies) {
        return;
    }
    QStringList required;
    required.reserve(ui->comboBox_dependencies->count());
    for (int index = 0; index < ui->comboBox_dependencies->count(); ++index) {
        required << ui->comboBox_dependencies->itemText(index);
    }
    mpChipRow_dependencies->setItems(required);
    refreshDependencySuggestions();
    refreshStartOverButton();
}

void dlgPackageExporter::refreshDependencySuggestions()
{
    if (!mpChipRow_dependencies) {
        return;
    }
    QList<uiDesign::Suggestion> offers;
    offers.reserve(std::max(0, ui->DependencyList->count() - 1));
    // Row zero of that picker is its own prompt rather than a package
    for (int index = 1; index < ui->DependencyList->count(); ++index) {
        //: Package exporter - word beside a package name the exporter offers as something this package could need
        offers.append({ui->DependencyList->itemText(index), tr("installed")});
    }
    mpChipRow_dependencies->setSuggestions(offers);
}

// Mirrors the hidden "-" button's bookkeeping on DependencyList
void dlgPackageExporter::slot_dependencyChipsChanged()
{
    const QStringList required = mpChipRow_dependencies->items();
    QStringList wasRequired;
    wasRequired.reserve(ui->comboBox_dependencies->count());
    for (int index = 0; index < ui->comboBox_dependencies->count(); ++index) {
        wasRequired << ui->comboBox_dependencies->itemText(index);
    }
    for (const QString& name : std::as_const(wasRequired)) {
        if (!required.contains(name) && ui->DependencyList->findText(name) < 0) {
            ui->DependencyList->addItem(name);
        }
    }
    for (const QString& name : std::as_const(required)) {
        if (const int at = ui->DependencyList->findText(name); at > 0) {
            ui->DependencyList->removeItem(at);
        }
    }

    const QSignalBlocker held(ui->comboBox_dependencies);
    ui->comboBox_dependencies->clear();
    ui->comboBox_dependencies->addItems(required);
    refreshDependencySuggestions();
    refreshStartOverButton();
}

// Text kept because a QLabel bakes a link's colour in at setText()
void dlgPackageExporter::showNotice(const QString& richText, const uiDesign::NoticeKind kind)
{
    if (!mpNotice) {
        return;
    }
    mNoticeRichText = richText;
    mpNotice->notificationAreaIconLabelError->setVisible(kind == uiDesign::NoticeKind::Error);
    mpNotice->notificationAreaIconLabelWarning->setVisible(kind == uiDesign::NoticeKind::Warning);
    mpNotice->notificationAreaIconLabelInformation->setVisible(kind == uiDesign::NoticeKind::Information);
    mpNotice->notificationAreaMessageBox->setText(uiDesign::withLinkColour(richText, uiDesign::themeTokens().accentText));
    mpNotice->show();
}

// Elided from the left: the last folders and the file name are what a reader needs
void dlgPackageExporter::fitSavePath()
{
    if (!mpLabel_savePath) {
        return;
    }
    mpLabel_savePath->setToolTip(mSavePathShown);
    const int room = mpLabel_savePath->width();
    if (room <= 0) {
        mpLabel_savePath->setText(mSavePathShown);
        return;
    }
    mpLabel_savePath->setText(QFontMetrics(mpLabel_savePath->font()).elidedText(mSavePathShown, Qt::ElideLeft, room));
}

// frameWidth() ignores a stylesheet border, so measure against the viewport
static int chromeAround(const QAbstractScrollArea* pArea)
{
    const int measured = pArea->height() - pArea->viewport()->height();
    return measured > 0 ? measured : 2 * uiDesign::scmInputBorderWidth;
}

// The description and this list are the column's only flexing rows
void dlgPackageExporter::fitAssetsList()
{
    if (mFittingAssets || !ui || !ui->listWidget_addedFiles || !ui->textEdit_description) {
        return;
    }
    QListWidget* pFiles = ui->listWidget_addedFiles;
    QTextEdit* pDescription = ui->textEdit_description;
    // Nothing to measure before layout, and a guess here would be undone by the first pass
    if (!pFiles->isVisible() || !pDescription->isVisible() || pFiles->height() <= 0 || pDescription->height() <= 0) {
        return;
    }

    // Nothing styles the items, so an empty list's row is the font's line height
    const int rowHeight = pFiles->count() > 0 ? pFiles->sizeHintForRow(0) : QFontMetrics(pFiles->font()).height();
    if (rowHeight <= 0) {
        return;
    }
    const int listChrome = chromeAround(pFiles);
    const int wanted = (pFiles->count() + 1) * rowHeight + listChrome;
    const int floorOfList = scmExporterDropZoneLines * rowHeight + listChrome;
    const int floorOfDescription = QFontMetrics(pDescription->font()).lineSpacing() * scmExporterDescriptionLines + qRound(2 * pDescription->document()->documentMargin()) + chromeAround(pDescription);
    const int room = pDescription->height() + pFiles->height();
    const int cap = std::max(floorOfList, room - floorOfDescription);
    const int listHeight = std::clamp(wanted, floorOfList, cap);

    pDescription->setMinimumHeight(floorOfDescription);
    if (pFiles->minimumHeight() == listHeight && pFiles->maximumHeight() == listHeight) {
        return;
    }
    mFittingAssets = true;
    pFiles->setFixedHeight(listHeight);
    mFittingAssets = false;
}

// Leaves carry no glyph: the unit heading they stand under already says what they are
static void markTreeFolders(QTreeWidgetItem* pItem, const QIcon& folderGlyph, const int topFolderMark)
{
    for (int child = 0; child < pItem->childCount(); ++child) {
        QTreeWidgetItem* pChild = pItem->child(child);
        pChild->setIcon(0, pChild->childCount() > 0 || pChild->data(0, Qt::UserRole) == topFolderMark ? folderGlyph : QIcon());
        markTreeFolders(pChild, folderGlyph, topFolderMark);
    }
}

// Walked here rather than set where the rows are built, so an appearance change re-inks them
void dlgPackageExporter::restyleTreeIcons(const uiDesign::ThemeTokens& tokens)
{
    const QStringList unitGlyphs{qsl(":/icons/editor-triggers.svg"),
                                 qsl(":/icons/editor-aliases.svg"),
                                 qsl(":/icons/editor-timers.svg"),
                                 qsl(":/icons/editor-scripts.svg"),
                                 qsl(":/icons/editor-keys.svg"),
                                 qsl(":/icons/editor-buttons.svg")};
    const QIcon folderGlyph = uiDesign::tintedIcon(qsl(":/icons/editor-folder.svg"), tokens);

    // QTreeWidget::itemChanged fires for an icon too, and its slot hands each row's
    // check state down to the children
    const QSignalBlocker held(mpExportSelection);
    for (int top = 0; top < mpExportSelection->topLevelItemCount() && top < unitGlyphs.size(); ++top) {
        QTreeWidgetItem* pTop = mpExportSelection->topLevelItem(top);
        pTop->setIcon(0, uiDesign::tintedIcon(unitGlyphs.at(top), tokens));
        markTreeFolders(pTop, folderGlyph, isTopFolder);
    }
    mpExportSelection->setIconSize(QSize(scmExporterButtonGlyphSize, scmExporterButtonGlyphSize));
}

void dlgPackageExporter::showIconPlaceholder(const uiDesign::ThemeTokens& tokens)
{
    const qreal ratio = ui->Icon->devicePixelRatioF();
    QPixmap glyph = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/packages-package.svg")), tokens.mutedText)
                            .scaled(QSize(scmExporterIconGlyphSize, scmExporterIconGlyphSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    glyph.setDevicePixelRatio(ratio);
    ui->Icon->setPixmap(glyph);
}

void dlgPackageExporter::showChosenIcon()
{
    const qreal ratio = ui->Icon->devicePixelRatioF();
    // Through a QIcon, so the .ico and .icns files the picker offers are read at all
    QPixmap chosen = mPackageIconPath.isEmpty() ? QPixmap() : QIcon(mPackageIconPath).pixmap(QSize(scmExporterIconSize, scmExporterIconSize) * ratio);
    ui->Icon->clear();
    if (chosen.isNull()) {
        showIconPlaceholder(uiDesign::themeTokens());
        ui->pushButton_removeIcon->hide();
    } else {
        chosen = chosen.scaled(QSize(scmExporterIconSize, scmExporterIconSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        chosen.setDevicePixelRatio(ratio);
        ui->Icon->setPixmap(chosen);
        ui->pushButton_removeIcon->show();
    }
    refreshStartOverButton();
}

void dlgPackageExporter::showAssetsPrompt()
{
    if (mpLabel_assetsEmpty) {
        mpLabel_assetsEmpty->setVisible(ui->listWidget_addedFiles->count() == 0);
    }
}

// Not on the dialog: mudlet replaces a dialog's sheet with the profile's Lua stylesheet on show
void dlgPackageExporter::applyPackageExporterShellStyle()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QString captionSize = QString::number(uiDesign::typeSize(uiDesign::TypeStep::Caption));
    const QString hairline = QString::number(uiDesign::scmInputBorderWidth);

    ui->widget_newOrExistingPackageName->setStyleSheet(qsl("#widget_newOrExistingPackageName { background-color: %1; }").arg(tokens.page.name())
                                                       + uiDesign::inputStyleSheet(tokens, qsl("#widget_newOrExistingPackageName"))
                                                       + uiDesign::buttonStyleSheet(tokens, qsl("#widget_newOrExistingPackageName")));

    // A transparent hairline on the filled count chip, so it does not shift when emptied
    const QString leftRules =
            qsl("#exporterSelectionColumn { background-color: %1; }").arg(tokens.pane.name()) + qsl("#exporterSelectionHeading { color: %1; background: transparent; }").arg(tokens.mutedText.name())
            + qsl("#exporterSelectionHint { color: %1; background: transparent; font-size: %2pt; }").arg(tokens.mutedText.name(), captionSize)
            + qsl("#exporterSelectionCount { color: %1; background-color: %2; border: %3px solid transparent; border-radius: %4px; padding: 0px %5px; }")
                      .arg(tokens.accentText.name(), tokens.accentSoft, hairline, QString::number(uiDesign::scmRadiusChip))
                      .arg(QString::number(uiDesign::scmItemRowPaddingTrailing))
            + qsl("#exporterSelectionCount[exporterCountEmpty=\"true\"] { color: %1; background: transparent; border: %2px solid %3; }").arg(tokens.mutedText.name(), hairline, tokens.border.name());
    mpSelectionColumn->setStyleSheet(leftRules);
    mpLabel_selectionCount->setFont(uiDesign::chipFont(mpLabel_selectionCount));

    const QString treeSelector = qsl("QTreeWidget#treeWidget_exportSelection");
    ui->treeWidget_exportSelection->setStyleSheet(uiDesign::itemRowStyleSheet(treeSelector, tokens, tokens.pane, scmExporterRowGutter) + uiDesign::itemCheckMarkStyleSheet(treeSelector, tokens)
                                                  + uiDesign::treeBranchStyleSheet(treeSelector, tokens, tokens.pane) + uiDesign::scrollBarStyleSheet(treeSelector, tokens, tokens.pane));
    // In the palette too: the view draws row text from it, and the platform's
    // answers white on the accent's wash
    QPalette rowInks = ui->treeWidget_exportSelection->palette();
    rowInks.setColor(QPalette::Text, tokens.text);
    rowInks.setColor(QPalette::HighlightedText, tokens.accentText);
    ui->treeWidget_exportSelection->setPalette(rowInks);

    // Named one by one rather than every QLabel: the chips and the drop-zone prompt
    // are labels too, and neither is a caption
    const QStringList captionLabels{qsl("#exporterNameLabel"),
                                    qsl("#label_author"),
                                    qsl("#label_version"),
                                    qsl("#label_shortDescription"),
                                    qsl("#label_helpUrl"),
                                    qsl("#label_icon"),
                                    qsl("#label_requiredPackages"),
                                    qsl("#label_detailedDescription"),
                                    qsl("#label_assets"),
                                    qsl("#label"),
                                    qsl("#exporterDetailsHint"),
                                    qsl("#exporterDescriptionHint"),
                                    qsl("#exporterAssetsHint"),
                                    qsl("#exporterAssetsDropHint")};
    const QString rightRules =
            qsl("#exporterDetailsColumn { background-color: %1; }").arg(tokens.page.name()) + uiDesign::inputStyleSheet(tokens, qsl("#exporterDetailsColumn"))
            + uiDesign::buttonStyleSheet(tokens, qsl("#exporterDetailsColumn")) + uiDesign::choiceStyleSheet(tokens, qsl("#exporterDetailsColumn")) + captionLabels.join(qsl(", "))
            + qsl(" { color: %1; background: transparent; font-size: %2pt; }").arg(tokens.mutedText.name(), captionSize)
            + qsl("#exporterDetailsHeading { color: %1; background: transparent; }").arg(tokens.mutedText.name())
            // Dashed, not the input recipe's solid hairline: the dashed edge says it takes a drop
            + qsl("#Icon { background-color: %1; border: %2px dashed %3; border-radius: %4px; }").arg(tokens.card.name(), hairline, tokens.border.name(), QString::number(uiDesign::scmRadiusInput))
            + qsl("#Icon[%1=\"true\"] { background-color: %2; border-color: %3; }").arg(QLatin1String(scmProp_iconDropTarget), tokens.accentSoft, tokens.accent.name())
            + qsl("#exporterDetailsColumn QListWidget#listWidget_addedFiles { background-color: %1; color: %2; border: %3px dashed %4; border-radius: %5px; }")
                      .arg(tokens.field.name(), tokens.text.name(), hairline, tokens.border.name(), QString::number(uiDesign::scmRadiusInput))
            // A label, not a placeholder, so held to the text contrast floor
            + qsl("#exporterAssetsEmpty { color: %1; background: transparent; }").arg(uiDesign::readableOn(tokens.field, tokens.disabledText, tokens.text, uiDesign::scmTextMinimumRatio).name())
            + uiDesign::scrollBarStyleSheet(qsl("#textEdit_description"), tokens, tokens.field)
            + uiDesign::scrollBarStyleSheet(qsl("#listWidget_addedFiles"), tokens, tokens.field)
            // Last: the chip rules and the caption rule are equally specific, so the later wins
            + uiDesign::ChipRow::styleSheetFor(tokens);
    mpDetailsColumn->setStyleSheet(rightRules);

    // A QTextDocument takes its colours only from its own default stylesheet
    ui->textEdit_description->document()->setDefaultStyleSheet(qsl("body { color: %1; }"
                                                                   "h1, h2, h3, h4, h5, h6 { color: %1; }"
                                                                   "h1 { font-size: %6pt; }"
                                                                   "h2 { font-size: %7pt; }"
                                                                   "h3, h4, h5, h6 { font-size: %8pt; }"
                                                                   "a { color: %2; }"
                                                                   "pre, code { background-color: %3; color: %1; border: %4px solid %5; }")
                                                                       .arg(tokens.text.name(), tokens.accentText.name(), tokens.page.name(), hairline, tokens.border.name())
                                                                       .arg(QString::number(uiDesign::typeSize(uiDesign::TypeStep::Display)),
                                                                            QString::number(uiDesign::typeSize(uiDesign::TypeStep::Title)),
                                                                            QString::number(uiDesign::typeSize(uiDesign::TypeStep::Body))));

    const QString footRules = qsl("#exporterFoot { background-color: %1; border-top: %2px solid %3; }").arg(tokens.page.name(), hairline, tokens.separator.name())
                              + uiDesign::buttonStyleSheet(tokens, qsl("#exporterFoot"))
                              + qsl("#exporterSaveCaption { color: %1; background: transparent; font-size: %2pt; }").arg(tokens.mutedText.name(), captionSize)
                              + qsl("#exporterSavePath { color: %1; background: transparent; }").arg(tokens.text.name());
    mpFoot->setStyleSheet(footRules);
    mpLabel_savePath->setFont(uiDesign::fixedPitchFont(font(), scmExporterPathFontScale));
    fitSavePath();

    // The handle paints each neighbour's tone up to its hairline and cannot read a
    // stylesheet fill, so each column names its own
    mpSelectionColumn->setProperty(uiDesign::scmProp_paneTone, tokens.pane);
    mpDetailsColumn->setProperty(uiDesign::scmProp_paneTone, tokens.page);
    if (mpSplitter && mpSplitter->handle(1)) {
        mpSplitter->handle(1)->update();
    }

    // Glyphs are inked per appearance and do not follow a theme change by themselves
    restyleTreeIcons(tokens);
    showChosenIcon();
    mExportButton->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-export.svg"), tokens));
    ui->addFiles->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-add.svg"), tokens));
    ui->pushButton_removeIcon->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));
    for (QAbstractButton* pButton : {static_cast<QAbstractButton*>(mExportButton.data()), static_cast<QAbstractButton*>(ui->addFiles), static_cast<QAbstractButton*>(ui->pushButton_removeIcon)}) {
        pButton->setIconSize(QSize(scmExporterButtonGlyphSize, scmExporterButtonGlyphSize));
    }
    if (QToolButton* pClose = mpNotice ? mpNotice->messageAreaCloseButton : nullptr) {
        pClose->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));
        pClose->setIconSize(QSize(scmExporterButtonGlyphSize, scmExporterButtonGlyphSize));
    }
    if (mpNotice && !mNoticeRichText.isEmpty()) {
        mpNotice->notificationAreaMessageBox->setText(uiDesign::withLinkColour(mNoticeRichText, tokens.accentText));
    }

    if (mpChipRow_dependencies) {
        mpChipRow_dependencies->restyleGlyphs(tokens);
        mpChipRow_dependencies->restyleSuggestions(tokens);
    }
    if (mpTreeDelegate) {
        mpTreeDelegate->restyle(tokens);
    }

    for (QWidget* pContainer : {ui->widget_newOrExistingPackageName, mpSelectionColumn, mpDetailsColumn, mpFoot}) {
        uiDesign::keepClickFocusOffControls(pContainer);
        uiDesign::letPopupsTakeTheFieldsCorner(pContainer);
    }

    // Last: it measures off the fonts and borders set above
    fitAssetsList();
}

void dlgPackageExporter::slot_applyAppearance()
{
    applyPackageExporterShellStyle();
}

void dlgPackageExporter::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // macOS drops geometry set before a native window is shown; first show only, or a reshow undoes a drag
    if (!mSplitterRestored) {
        mSplitterRestored = true;
        restoreColumnSplit();
    }
    fitSavePath();
    fitAssetsList();
}

void dlgPackageExporter::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    fitSavePath();
}

void dlgPackageExporter::restoreColumnSplit()
{
    if (!mpSplitter) {
        return;
    }

    QSettings* pSettings = MudletApp::getQSettings();
    const QByteArray saved = pSettings ? pSettings->value(QString::fromLatin1(scmExporterSplitterKey)).toByteArray() : QByteArray();
    if (!saved.isEmpty() && mpSplitter->restoreState(saved)) {
        // restoreState() also restores the serialized handle width, which a state
        // saved by another Mudlet version may have different
        mpSplitter->setHandleWidth(uiDesign::GripSplitter::scmHandleThickness);
        return;
    }

    const int across = mpSplitter->width() - uiDesign::GripSplitter::scmHandleThickness;
    if (across <= 0) {
        return;
    }
    const int tree = std::max(mpSelectionColumn->minimumWidth(), across / scmExporterTreeShareDenominator);
    mpSplitter->setSizes({tree, across - tree});
}

void dlgPackageExporter::setModuleCreationMode(bool isModule)
{
    mIsModuleCreationMode = isModule;
    if (isModule) {
        // Update the window title for module creation
        setWindowTitle(tr("Create Module - %1").arg(mpHost->getName()));

        // Update UI elements for module creation
        //: Package exporter - prompt in the empty name field while a module is being made
        ui->lineEdit_packageName->setPlaceholderText(tr("Module name"));
        //: Screen reader name of the field the module to create is named in, which has no visible label of its own
        ui->lineEdit_packageName->setAccessibleName(tr("Module name"));
        //: Screen reader description of the field the module to create is named in
        ui->lineEdit_packageName->setAccessibleDescription(tr("Name to create the module under"));

        // Update button text to be module-specific
        mExportButton->setText(tr("Create Module"));

        showModeWords();

        // Update the groupbox title for module creation
        ui->groupBox_exportSelection->setTitle(tr("Select items to include in module"));

        // Hide the package list dropdown and "or" label for cleaner module creation
        ui->packageList->setVisible(false);
        ui->label_or->setVisible(false);

        // Update the description button text to be module-specific
        // IMPORTANT: Must use setText not setPlaceholderText for buttons
        ui->pushButton_openInfos->setText(tr("Add module description, icon, and assets (optional)"));

        // Update the export location widget to be more descriptive
        ui->lineEdit_filePath->setPlaceholderText(tr("Module location"));
        //: Screen reader name of the read-only field showing where the module will be saved, which has no visible label of its own
        ui->lineEdit_filePath->setAccessibleName(tr("Module save location"));

        // Update info label if visible
        ui->infoLabel->setText(QString());

        // Update metadata labels to be module-specific
        ui->label_shortDescription->setText(tr("Module description"));
        ui->lineEdit_title->setPlaceholderText(tr("Brief description of your module"));
        ui->lineEdit_author->setPlaceholderText(tr("Module author (recommended)"));
        ui->lineEdit_version->setPlaceholderText(tr("Module version (recommended)"));

        // Update dependencies and assets labels
        ui->label_requiredPackages->setText(tr("Module dependencies"));
        ui->label_assets->setText(tr("Include module assets (images, sounds, fonts)"));
        ui->addFiles->setText(tr("Select files to include in module"));

        // Clear dropdown lists that have package-specific items
        // Temporarily disconnect the signal to prevent unwanted selections when clearing
        disconnect(ui->packageList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgPackageExporter::slot_packageChanged);
        ui->packageList->clear();
        ui->DependencyList->clear();
        // Add only module-relevant items to dependency list
        ui->DependencyList->addItem(tr("Select module dependencies"));
        ui->DependencyList->addItems(mpHost->mInstalledPackages);
        auto modules = mpHost->mInstalledModules;
        for (const auto& [moduleName, moduleData] : modules.asKeyValueRange()) {
            ui->DependencyList->addItem(moduleName);
        }
        // Reconnect the signal (though it's not needed in module creation mode)
        connect(ui->packageList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgPackageExporter::slot_packageChanged);
        refreshDependencySuggestions();

        // Clear the default package template text and set module-specific template
        ui->textEdit_description->clear();
        ui->textEdit_description->setPlainText(tr("(optional)\n\n"
                                                  "This module description is shown in the Module Manager. The editor supports Commonmark markdown.\n\n"
                                                  "### Description\n\n"
                                                  "A full description of what this module does. If the module is game-specific, mention that here.\n\n"
                                                  "### Usage\n\n"
                                                  "If this module uses aliases, show a few examples and expected output.\n\n"
                                                  "`> alias_1`\n\n"
                                                  "    output of alias_1  -- indent by four spaces\n"
                                                  "    more output        -- for code blocks\n\n"
                                                  "### See Also\n\n"
                                                  "Further reading material, e.g., links to documentation or forum posts.\n\n"
                                                  "* https://wiki.mudlet.org/w/Manual:Modules"));

        // In this mode startOver() puts back the module template, not the package one
        mFreshDescription = ui->textEdit_description->toPlainText();
        refreshStartOverButton();

        // Focus on the module name input
        ui->lineEdit_packageName->setFocus();
    }
}

void dlgPackageExporter::preselectTrigger(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the trigger ID from the editor tree item
    const int triggerId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (triggerId <= 0) {
        return;
    }

    // Find the matching trigger in our trigger map by ID
    for (auto it = triggerMap.begin(); it != triggerMap.end(); ++it) {
        if (it.value() == triggerId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::preselectTimer(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the timer ID from the editor tree item
    const int timerId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (timerId <= 0) {
        return;
    }

    // Find the matching timer in our timer map by ID
    for (auto it = timerMap.begin(); it != timerMap.end(); ++it) {
        if (it.value() == timerId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::preselectAlias(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the alias ID from the editor tree item
    const int aliasId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (aliasId <= 0) {
        return;
    }

    // Find the matching alias in our alias map by ID
    for (auto it = aliasMap.begin(); it != aliasMap.end(); ++it) {
        if (it.value() == aliasId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::preselectScript(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the script ID from the editor tree item
    const int scriptId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (scriptId <= 0) {
        return;
    }

    // Find the matching script in our script map by ID
    for (auto it = scriptMap.begin(); it != scriptMap.end(); ++it) {
        if (it.value() == scriptId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::preselectAction(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the action ID from the editor tree item
    const int actionId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (actionId <= 0) {
        return;
    }

    // Find the matching action in our action map by ID
    for (auto it = actionMap.begin(); it != actionMap.end(); ++it) {
        if (it.value() == actionId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::preselectKey(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    // Get the key ID from the editor tree item
    const int keyId = item->data(0, Qt::UserRole).toInt();

    // Don't select anything if the ID is invalid (0 or negative)
    if (keyId <= 0) {
        return;
    }

    // Find the matching key in our key map by ID
    for (auto it = keyMap.begin(); it != keyMap.end(); ++it) {
        if (it.value() == keyId) {
            it.key()->setCheckState(0, Qt::Checked);
            break;
        }
    }
}

void dlgPackageExporter::appendToDetails(const QString& what, const QString& value)
{
    if (value.isEmpty()) {
        return;
    }
    if (mPackageComment.isEmpty()) {
        // Insert a leading linefeed at the start as some zip utilities prefix
        // the first line with, say something like "Comment: " which does not
        // look so good when immediately followed by "Package name:". For
        // similar layout reasons, indent all the comment entries by, say,
        // four spaces in the next chunk of code.
        mPackageComment.append(QChar::LineFeed);
    }
    if (!what.compare(QLatin1String("mpackage"))) {
        // No point in using an internal variable name in an external, user
        // viewable aspect, use something that is more sensible:
        mPackageComment.append(qsl("    Package name: %1\n").arg(value));
    } else {
        mPackageComment.append(qsl("    %1: %2\n").arg(what, value));
    }
    // Only a value the plain [[...]] would end early, reopen, or strip a leading newline from gets
    // the escalated form: the package repository's upload site reads the plain one as text
    const bool plainLiteralHoldsIt =
            !value.contains(qsl("]]")) && !value.contains(qsl("[[")) && !value.endsWith(QLatin1Char(']')) && !value.startsWith(QLatin1Char('\n')) && !value.startsWith(QLatin1Char('\r'));
    mPackageConfig.append(qsl("%1 = %2\n").arg(what, plainLiteralHoldsIt ? qsl("[[%1]]").arg(value) : LuaLiteral::quote(value)));
}

void dlgPackageExporter::recurseTree(QTreeWidgetItem* pItem, QList<QTreeWidgetItem*>& treeList)
{
    treeList.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseTree(pItem->child(i), treeList);
    }
}

std::pair<bool, QString> dlgPackageExporter::writeFileToZip(const QString& archiveFileName, const QString& fileSystemFileName, zip* archive)
{
    struct zip_source* s = zip_source_file(archive, fileSystemFileName.toUtf8().constData(), 0, -1);
    if (s == nullptr) {
        return {false,
                //: This error message will appear when a file is to be placed into the package but the code cannot open it.
                tr("Failed to open file \"%1\" to place into package. Error message was: \"%2\".").arg(fileSystemFileName.toHtmlEscaped(), zip_strerror(archive))};
    }

    if (zip_file_add(archive, archiveFileName.toUtf8().constData(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) == -1) {
        zip_source_free(s);
        s = nullptr;
        return {false,
                //: This error message will appear when a file is to be placed into the package but cannot be done for some reason.
                tr("Failed to add file \"%1\" to package. Error message was: \"%3\".").arg(archiveFileName.toHtmlEscaped(), zip_strerror(archive))};
    }

    return {true, QString()};
}

void dlgPackageExporter::populateDependencies()
{
    ui->DependencyList->clear();
    ui->DependencyList->addItem(tr("add dependencies"));
    const QSignalBlocker blocker(ui->packageList);
    const QString previousSelection = ui->packageList->currentText();
    ui->packageList->clear();
    //: First item in package selection dropdown - choosing one of the packages under it fills this form in from a package that is already installed, and ticks what it holds in the tree
    ui->packageList->addItem(tr("Start from an installed package"));
    ui->packageList->addItems(mpHost->mInstalledPackages);

    ui->DependencyList->addItems(mpHost->mInstalledPackages);
    auto modules = mpHost->mInstalledModules;
    for (const auto& [moduleName, moduleData] : modules.asKeyValueRange()) {
        ui->packageList->addItem(moduleName);
        ui->DependencyList->addItem(moduleName);
    }

    const int previousIndex = ui->packageList->findText(previousSelection);
    if (previousIndex >= 0) {
        ui->packageList->setCurrentIndex(previousIndex);
    }
    refreshDependencySuggestions();
}

void dlgPackageExporter::slot_addDependency()
{
    auto text = ui->DependencyList->currentText();
    auto index = ui->DependencyList->currentIndex();
    if (text.isEmpty() || index == 0) {
        return;
    }
    ui->comboBox_dependencies->addItem(text);
    ui->DependencyList->removeItem(index);
    ui->comboBox_dependencies->setCurrentIndex(ui->comboBox_dependencies->count() - 1);
    syncDependencyChips();
}

void dlgPackageExporter::slot_removeDependency()
{
    auto text = ui->comboBox_dependencies->currentText();
    auto index = ui->comboBox_dependencies->currentIndex();
    if (text.isEmpty()) {
        return;
    }
    ui->DependencyList->addItem(text);
    ui->comboBox_dependencies->removeItem(index);
    syncDependencyChips();
}

void dlgPackageExporter::slot_packageChanged(int index)
{
    if (index == 0) {
        // Before uncheckAllChildren(): nothing may change until requestStartOver() has its answer
        requestStartOver();
        return;
    }

    const QString packageName = ui->packageList->currentText();
    uncheckAllChildren();
    ui->lineEdit_packageName->setText(packageName);

    //check package/module items
    QTreeWidgetItem* top = mpTriggers;
    QList<QTreeWidgetItem*> trigList;
    recurseTree(top, trigList);
    for (auto item : std::as_const(trigList)) {
        if (const auto* pTrigger = triggerOf(triggerMap, item); pTrigger && pTrigger->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpTimers;
    QList<QTreeWidgetItem*> timerList;
    recurseTree(top, timerList);
    for (auto item : std::as_const(timerList)) {
        if (const auto* pTimer = timerOf(timerMap, item); pTimer && pTimer->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpAliases;
    QList<QTreeWidgetItem*> aliasList;
    recurseTree(top, aliasList);
    for (auto item : std::as_const(aliasList)) {
        if (const auto* pAlias = aliasOf(aliasMap, item); pAlias && pAlias->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpButtons;
    QList<QTreeWidgetItem*> actionList;
    recurseTree(top, actionList);
    for (auto item : std::as_const(actionList)) {
        if (const auto* pAction = actionOf(actionMap, item); pAction && pAction->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpScripts;
    QList<QTreeWidgetItem*> scriptList;
    recurseTree(top, scriptList);
    for (auto item : std::as_const(scriptList)) {
        if (const auto* pScript = scriptOf(scriptMap, item); pScript && pScript->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpKeys;
    QList<QTreeWidgetItem*> keyList;
    recurseTree(top, keyList);
    for (auto item : std::as_const(keyList)) {
        if (const auto* pKey = keyOf(keyMap, item); pKey && pKey->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    refreshFolderMarks();

    const QString packagePath{MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName())};
    //fill package metadata
    mPackageDataPath.clear();
    QMap<QString, QString> const packageInfo = mpHost->mPackageInfo.value(packageName);
    ui->lineEdit_author->setText(packageInfo.value(qsl("author")));
    const QString icon{packageInfo.value(qsl("icon"))};
    if (!icon.isEmpty()) {
        useIcon(qsl("%1/%2/.mudlet/Icon/%3").arg(packagePath, packageName, icon));
    } else {
        slot_removeIcon();
    }
    ui->lineEdit_title->setText(packageInfo.value(qsl("title")));
    mPlainDescription = packageInfo.value(qsl("description"));
    QString description{mPlainDescription};
    description.replace(QLatin1String("$packagePath"), qsl("%1/%2").arg(packagePath, packageName));
    ui->textEdit_description->setMarkdown(description);
    const QString version = packageInfo.value(qsl("version"));
    ui->lineEdit_version->setText(version);
    ui->lineEdit_helpUrl->setText(packageInfo.value(qsl("helpURL")));
    populateDependencies(); // available dependencies, as opposed to required ones which is next
    const QStringList dependencies = packageInfo.value(qsl("dependencies")).split(QLatin1Char(','));
    ui->comboBox_dependencies->clear();
    if (!dependencies.at(0).isEmpty()) {
        ui->comboBox_dependencies->addItems(dependencies);
    }
    syncDependencyChips();
    //get files and folders from package
    ui->listWidget_addedFiles->clear();
    const QFileInfo info(qsl("%1/%2/").arg(packagePath, packageName));
    if (info.exists()) {
        mPackageDataPath = qsl("%1/%2/.mudlet").arg(packagePath, packageName);
        QDirIterator it(info.absoluteFilePath(), QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
        QStringList ignore;
        // .mudlet is rebuilt on export from the icon and description, see copyPackageDataToTmp()
        ignore << QLatin1String("config.lua") << qsl("%1.xml").arg(packageName) << qsl(".mudlet");
        while (it.hasNext()) {
            const QFileInfo f(it.next());
            if (ignore.contains(f.fileName(), Qt::CaseInsensitive)) {
                continue;
            }
            ui->listWidget_addedFiles->addItem(f.absoluteFilePath());
        }
    }

    mLoadedWords.name = ui->lineEdit_packageName->text();
    mLoadedWords.author = ui->lineEdit_author->text();
    mLoadedWords.title = ui->lineEdit_title->text();
    mLoadedWords.helpUrl = ui->lineEdit_helpUrl->text();
    mLoadedWords.version = ui->lineEdit_version->text();
    mLoadedWords.description = ui->textEdit_description->toPlainText();
    mPackageListWasAt = index;
    refreshStartOverButton();
}

bool dlgPackageExporter::formIsFresh() const
{
    // <= 0: in module mode the picker is emptied, which leaves it at -1
    return ui->lineEdit_packageName->text().isEmpty() && ui->packageList->currentIndex() <= 0 && ui->lineEdit_author->text() == mFreshAuthor && ui->lineEdit_title->text().isEmpty()
           && ui->lineEdit_helpUrl->text().isEmpty() && ui->lineEdit_version->text() == mFreshVersion && ui->textEdit_description->toPlainText() == mFreshDescription && mDescriptionImages.isEmpty()
           && ui->comboBox_dependencies->count() == 0 && mPackageIconPath.isEmpty() && ui->listWidget_addedFiles->count() == 0 && countCheckedItems() == 0;
}

bool dlgPackageExporter::formHasTypedWords() const
{
    const auto typedHere = [](const QString& reads, const QString& fresh, const QString& loaded) {
        return reads != fresh && reads != loaded;
    };
    return typedHere(ui->lineEdit_packageName->text(), QString(), mLoadedWords.name) || typedHere(ui->lineEdit_author->text(), mFreshAuthor, mLoadedWords.author)
           || typedHere(ui->lineEdit_title->text(), QString(), mLoadedWords.title) || typedHere(ui->lineEdit_helpUrl->text(), QString(), mLoadedWords.helpUrl)
           || typedHere(ui->lineEdit_version->text(), mFreshVersion, mLoadedWords.version) || typedHere(ui->textEdit_description->toPlainText(), mFreshDescription, mLoadedWords.description);
}

void dlgPackageExporter::startOver()
{
    ui->lineEdit_packageName->clear();
    {
        // Or the picker re-enters slot_packageChanged()
        const QSignalBlocker held(ui->packageList);
        ui->packageList->setCurrentIndex(0);
    }
    ui->lineEdit_author->setText(mFreshAuthor);
    ui->lineEdit_title->clear();
    ui->lineEdit_helpUrl->clear();
    ui->lineEdit_version->setText(mFreshVersion);
    ui->textEdit_description->setPlainText(mFreshDescription);
    mPlainDescription = mFreshDescription;
    mDescriptionImages.clear();
    ui->comboBox_dependencies->clear();
    populateDependencies();
    syncDependencyChips();
    mPackageIconPath.clear();
    mPackageDataPath.clear();
    showChosenIcon();
    ui->listWidget_addedFiles->clear();
    uncheckAllChildren();
    refreshFolderMarks();
    mLoadedWords = LoadedWords();
    mPackageListWasAt = 0;
    if (mpNotice) {
        mpNotice->hide();
    }
    mNoticeRichText.clear();
    slot_updateLocationPlaceholder();
    refreshStartOverButton();
}

void dlgPackageExporter::requestStartOver()
{
    if (formHasTypedWords()) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        //: Title of the dialog asking whether to clear every field of the package exporter
        msgBox.setWindowTitle(tr("Start over?"));
        //: Package exporter - asked before every field, tick and file on the form is cleared
        msgBox.setText(tr("Clear everything and start over?"));
        //: Package exporter - shown under the 'clear everything' question, saying what is lost by answering it
        msgBox.setInformativeText(tr("The words you typed here will be lost."));
        // On the buttons: QMessageBox::setButtonText() is deprecated since Qt 6.2
        //: Package exporter - the answer to the 'clear everything' question that goes ahead and clears the form
        msgBox.button(QMessageBox::Yes)->setText(tr("Clear everything"));
        //: Package exporter - the answer to the 'clear everything' question that leaves the form as it stands
        msgBox.button(QMessageBox::No)->setText(tr("Keep editing"));
        if (msgBox.exec() != QMessageBox::Yes) {
            // From the picker's first entry the picker has already moved: put it back
            // on the package whose words are still on the form
            if (ui->packageList->currentIndex() == 0 && mPackageListWasAt > 0) {
                const QSignalBlocker held(ui->packageList);
                ui->packageList->setCurrentIndex(mPackageListWasAt);
            }
            refreshStartOverButton();
            return;
        }
    }
    startOver();
}

void dlgPackageExporter::refreshStartOverButton()
{
    if (!mpButton_startOver) {
        return;
    }
    mpButton_startOver->setEnabled(!formIsFresh());
}

void dlgPackageExporter::slot_updateLocationPlaceholder()
{
    const auto packageName = ui->lineEdit_packageName->text();
    const QString where = packageName.isEmpty() ? getActualPath() : qsl("%1/%2.mpackage").arg(getActualPath(), packageName);

    checkToEnableExportButton();

    ui->lineEdit_filePath->setPlaceholderText(tr("Export to %1").arg(where));
    // Without "Export to": the caption beside the path says that
    mSavePathShown = where;
    fitSavePath();
}

void dlgPackageExporter::checkToEnableExportButton()
{
    QStringList missingFields;
    if (ui->lineEdit_packageName->text().isEmpty()) {
        //: package name will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
        missingFields << tr("package name");
    }
    // intentionally disabled for now until mpkg gains wider adoption
    // if (ui->lineEdit_author->text().isEmpty()) {
    //     //: package author will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("author");
    // }
    // if (ui->lineEdit_title->text().isEmpty()) {
    //     //: package title will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("title");
    // }
    // if (ui->lineEdit_version->text().isEmpty()) {
    //     //: package version will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("version");
    // }
    // if (ui->textEdit_description->toPlainText().isEmpty() || mExportingPackage) {
    //     //: package description will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("description");
    // }

    if (!missingFields.isEmpty()) {
        mExportButton->setEnabled(false);
        mExportButton->setToolTip(tr("Required field missing: %1").arg(missingFields.join(qsl(", "))));
    } else {
        mExportButton->setEnabled(!mExportingPackage);
        mExportButton->setToolTip(tr("Export package"));
    }

    // Every field reaches here on each keystroke
    refreshStartOverButton();
}

void dlgPackageExporter::slot_importIcon()
{
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Icon"), lastDir, tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)"));
    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);
    if (!mIsModuleCreationMode) {
        // browsing for an icon must not redirect where the module gets saved
        mPackagePath = lastDir;
        emit signal_exportLocationChanged(mPackagePath);
    }
    useIcon(fileName);
}

void dlgPackageExporter::useIcon(const QString& fileName)
{
    mPackageIconPath = fileName;
    showChosenIcon();
}

QString dlgPackageExporter::iconFileFromDrop(const QDropEvent* event)
{
    // The same file types slot_importIcon() offers in its picker
    static const QStringList iconSuffixes{qsl("png"), qsl("jpg"), qsl("jpeg"), qsl("bmp"), qsl("tif"), qsl("ico"), qsl("icns")};
    if (!(event->possibleActions() & Qt::CopyAction) || !event->mimeData() || !event->mimeData()->hasUrls()) {
        return QString();
    }
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() != 1 || !urls.first().isLocalFile()) {
        return QString();
    }
    const QFileInfo info(urls.first().toLocalFile());
    if (!info.isFile() || !iconSuffixes.contains(info.suffix(), Qt::CaseInsensitive)) {
        return QString();
    }
    return info.absoluteFilePath();
}

// Shown on the icon box, where the picture lands, in the accent the files drop zone's dashed edge stands for
void dlgPackageExporter::setIconDropHighlight(const bool highlight)
{
    if (mIconDropHighlight != highlight) {
        mIconDropHighlight = highlight;
        ui->Icon->setProperty(scmProp_iconDropTarget, highlight);
        // A property selector is matched when the sheet is applied, not when the property changes
        ui->Icon->style()->unpolish(ui->Icon);
        ui->Icon->style()->polish(ui->Icon);
        ui->Icon->update();
    }
}

void dlgPackageExporter::slot_removeIcon()
{
    mPackageIconPath.clear();
    showChosenIcon();
}

void dlgPackageExporter::showRenderedDescription()
{
    //$packagePath allows to put images in a package which will then be displayed in the description
    //during package creation it uses the profile folder. But once the package is created it will use
    //profile folder/packagename
    QString plainText{mPlainDescription};
    const QString profilePath{MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName())};
    //$packagePath will be replaced by the resource path if an existing package is selected
    if (ui->packageList->currentIndex() != 0) {
        const QString packageName = ui->packageList->currentText();
        plainText.replace(QLatin1String("$packagePath"), qsl("%1/%2").arg(profilePath, packageName));
    } else {
        plainText.replace(QLatin1String("$packagePath"), profilePath);
    }
    for (int i = mDescriptionImages.size() - 1; i >= 0; i--) {
        QString fname = mDescriptionImages.at(i);
        const QFileInfo info(fname);
        fname = QUrl::toPercentEncoding(fname).constData();
        plainText.replace(qsl("$%1").arg(info.fileName()), fname);
    }
    ui->textEdit_description->setMarkdown(plainText);
}

bool dlgPackageExporter::eventFilter(QObject* obj, QEvent* evt)
{
    // Queued: the fit has to measure after the layout has run for the new size
    if (obj == mpDetailsColumn && evt->type() == QEvent::Resize) {
        QMetaObject::invokeMethod(this, &dlgPackageExporter::fitAssetsList, Qt::QueuedConnection);
        return false;
    }

    if (evt->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        //add dependencies by writing them and pressing enter
        //delete them by pressing delete. Scroll through them by using Up and Down keys
        if (obj == ui->comboBox_dependencies) {
            if (keyEvent->key() == Qt::Key_Delete) {
                slot_removeDependency();
                return true;
            }
        }
    }

    //Focus handling returns false so that underlying class functions still work and the cursor is visible for example
    //description focus handling
    if (obj == ui->textEdit_description) {
        if (evt->type() == QEvent::FocusIn) {
            ui->textEdit_description->setCurrentCharFormat(QTextCharFormat());
            ui->textEdit_description->setPlainText(mPlainDescription);
            return false;
        }

        if (evt->type() == QEvent::FocusOut) {
            mPlainDescription = ui->textEdit_description->toPlainText();
            showRenderedDescription();
            return false;
        }
    }

    if (obj == ui->widget_icon) {
        switch (evt->type()) {
        case QEvent::DragEnter:
        case QEvent::DragMove: {
            auto* moveEvent = static_cast<QDragMoveEvent*>(evt);
            if (iconFileFromDrop(moveEvent).isEmpty()) {
                moveEvent->ignore();
                setIconDropHighlight(false);
                return true;
            }
            // A move would tell the source to delete the file it handed over
            moveEvent->setDropAction(Qt::CopyAction);
            moveEvent->accept();
            setIconDropHighlight(true);
            return true;
        }
        case QEvent::DragLeave:
            setIconDropHighlight(false);
            return true;
        case QEvent::Drop: {
            setIconDropHighlight(false);
            auto* dropEvent = static_cast<QDropEvent*>(evt);
            const QString fileName = iconFileFromDrop(dropEvent);
            if (fileName.isEmpty()) {
                dropEvent->ignore();
                return true;
            }
            dropEvent->setDropAction(Qt::CopyAction);
            dropEvent->accept();
            useIcon(fileName);
            return true;
        }
        default:
            break;
        }
    }

    //added files listWidget events
    if (obj == ui->listWidget_addedFiles) {
        if (evt->type() == QEvent::DragEnter) {
            QDragEnterEvent* enterEvent = static_cast<QDragEnterEvent*>(evt);
            if (enterEvent->mimeData()->hasUrls()) {
                enterEvent->acceptProposedAction();
            }
            return true;
        }

        if (evt->type() == QEvent::Drop) {
            QDropEvent* dropEvent = static_cast<QDropEvent*>(evt);
            for (const auto& url : dropEvent->mimeData()->urls()) {
                const QString fname = url.toLocalFile();
                const QFileInfo info(fname);
                if (info.exists()) {
                    ui->listWidget_addedFiles->addItem(fname);
                }
            }
            return true;
        }

        if (evt->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
            if (keyEvent->key() == Qt::Key_Delete) {
                delete ui->listWidget_addedFiles->currentItem();
            }
        }
    }

    return false;
}

void dlgPackageExporter::copy_directory(const QString& fromDir, const QString& toDir, bool overwrite)
{
    QDirIterator it(fromDir);
    QDir targetDir(toDir);
    if (overwrite && targetDir.exists()) {
        targetDir.removeRecursively();
    }
    targetDir.mkdir(toDir);
    while (it.hasNext()) {
        const QFileInfo f(it.next());
        if (f.fileName() == QLatin1String(".") || f.fileName() == QLatin1String("..") || f.isSymLink()) {
            continue;
        }
        if (f.isDir()) {
            copy_directory(f.filePath(), qsl("%1/%2").arg(targetDir.absolutePath(), f.fileName()), false);
        } else {
            QFile::copy(f.filePath(), targetDir.filePath(f.fileName()));
        }
    }
}

void dlgPackageExporter::slot_exportPackage()
{
    // The native windows dialog does not support displaying files - and as this
    // code will clobber/overwrite an existing package with the same
    // name it is highly desirable to show the files.
    // Although the Qt Documentation says only that the Windows platform needs
    // to NOT use the native dialog to show files, it has also shown to be
    // required for KDE on Linux - so has been used for all platforms:
    mPackageName = ui->lineEdit_packageName->text();
    if (mPackageName.isEmpty()) {
        displayResultMessage(tr("Please enter the package name."), false);
        return;
    }

    // Check if any items are selected for export
    bool hasSelectedItems = false;

    // Check triggers
    if (mpTriggers) {
        QTreeWidgetItemIterator it(mpTriggers);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    // Check timers if no triggers selected
    if (!hasSelectedItems && mpTimers) {
        QTreeWidgetItemIterator it(mpTimers);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    // Check aliases if no triggers/timers selected
    if (!hasSelectedItems && mpAliases) {
        QTreeWidgetItemIterator it(mpAliases);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    // Check actions if no triggers/timers/aliases selected
    if (!hasSelectedItems && mpButtons) {
        QTreeWidgetItemIterator it(mpButtons);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    // Check scripts if no triggers/timers/aliases/actions selected
    if (!hasSelectedItems && mpScripts) {
        QTreeWidgetItemIterator it(mpScripts);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    // Check keys if no other items selected
    if (!hasSelectedItems && mpKeys) {
        QTreeWidgetItemIterator it(mpKeys);
        while (*it) {
            if ((*it)->checkState(0) == Qt::Checked && !packageFolder(*it)) {
                hasSelectedItems = true;
                break;
            }
            ++it;
        }
    }

    if (!hasSelectedItems) {
        if (mIsModuleCreationMode) {
            displayResultMessage(tr("Cannot create empty module. Please select at least one trigger, timer, alias, script, action, or key to include in the module."), false);
        } else {
            displayResultMessage(tr("Cannot create empty package. Please select at least one item to include in the package."), false);
        }
        return;
    }

    // if packageName changed allow to create a new package in the same path
    if (mIsModuleCreationMode) {
        // Modules default to the profile directory unless the user picked a save location
        const QString moduleDir = mPackagePath.isEmpty() ? MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName()) : mPackagePath;
        mPackagePathFileName = qsl("%1/%2.mpackage").arg(moduleDir, mPackageName);
    } else {
        mPackagePathFileName = qsl("%1/%2.mpackage").arg(getActualPath(), mPackageName);
    }

    const auto anotherExportRunning = [this]() {
        if (!mExportingPackage) {
            return false;
        }
        //: Shown in the package exporter when an export is asked for while another package or module is still being exported
        displayResultMessage(tr("Another package is still being exported. Please wait for it to finish."), false);
        return true;
    };
    if (anotherExportRunning()) {
        return;
    }

    // Confirm any overwrite BEFORE we create or truncate anything. Previously
    // the destination file was clobbered by the zip step regardless of the
    // user's answer, so declining still destroyed the existing file. Asking
    // here means cancelling leaves the existing package file (and any installed
    // module of the same name) completely untouched.
    const bool moduleAlreadyInstalled = mIsModuleCreationMode && mpHost->mInstalledModules.contains(mPackageName);
    if (QFileInfo::exists(mPackagePathFileName) || moduleAlreadyInstalled) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (mIsModuleCreationMode) {
            //: Title of the dialog asking whether to replace a module that already exists when creating a module
            msgBox.setWindowTitle(tr("Overwrite module?"));
            //: %1 is the name of the module that already exists
            msgBox.setText(tr("A module named \"%1\" already exists.").arg(mPackageName.toHtmlEscaped()));
        } else {
            //: Title of the dialog asking whether to replace a package file that already exists when exporting
            msgBox.setWindowTitle(tr("Overwrite package?"));
            //: %1 is the file name of the package file that would be overwritten
            msgBox.setText(tr("A file named \"%1\" already exists.").arg(QFileInfo(mPackagePathFileName).fileName().toHtmlEscaped()));
        }
        //: Shown under the 'a file/module already exists' text when exporting a package or creating a module
        msgBox.setInformativeText(tr("Do you want to overwrite it?"));
        if (msgBox.exec() != QMessageBox::Yes) {
            // Leave the existing file/module intact and abort the export.
            return;
        }
    }

    // The overwrite prompt above runs an event loop, in which another exporter may have started
    if (anotherExportRunning()) {
        return;
    }

    // QT Docs say that QStandardPaths::writableLocation(QStandardPaths::TempLocation)
    // "Returns a directory where temporary files can be stored. The returned
    // value might be application-specific, shared among other applications for
    // this user, or even system-wide. The returned path is never empty."
    // which will return something like:
    // Windows: "C:/Users/<USER>/AppData/Local/Temp"
    // Linux: "/tmp"
    // MacOS: randomly generated by the OS
    // To avoid confusion if the user looks in that part of their file-system we
    // will append a "/mudlet" suffix so they can see that we are interested in
    // those files:
    const QString stagingDirName = qsl("%1/mudlet/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), mPackageName);
    QDir packageDir = QDir(stagingDirName);
    if (!packageDir.exists()) {
        packageDir.mkpath(stagingDirName);
    } else {
        packageDir.removeRecursively();
        packageDir.mkpath(stagingDirName);
    }

    const QString tempPath = qsl("%1/").arg(stagingDirName);

    // Made before Cancel is shown and the event loop runs, so that an early Cancel reaches the zip
    mpZipCancelled = std::make_shared<std::atomic<bool>>(false);
    mExportingPackage = true;
    mExportRunning = true;
    QApplication::setOverrideCursor(Qt::BusyCursor);
    recheckEveryExportButton();

#if defined(LIBZIP_SUPPORTS_CANCELLING)
    mCancelButton->setVisible(true);
#endif

    mCloseButton->setVisible(false);
    displayResultMessage(tr("Exporting package..."), true);
    const QPointer<dlgPackageExporter> self(this);
    qApp->processEvents();
    if (!self) {
        return;
    }

    auto plainDescription = copyNewImagesToTmp(tempPath);
    copyPackageDataToTmp(tempPath);

    QStringList assetPaths;
    for (int i = 0; i < ui->listWidget_addedFiles->count(); ++i) {
        assetPaths << ui->listWidget_addedFiles->item(i)->text();
    }

    // start copying assets in the background
    auto assetsFuture = QtConcurrent::run(dlgPackageExporter::copyAssetsToTmp, assetPaths, tempPath);

    const QFileInfo iconFile = copyIconToTmp(tempPath);

    mXmlPathFileName = qsl("%1/%2.xml").arg(stagingDirName, mPackageName);
    writeConfigFile(stagingDirName, iconFile, plainDescription);

    QFile checkWriteability(mXmlPathFileName);
    if (!checkWriteability.open(QIODevice::WriteOnly)) {
        displayResultMessage(tr("Failed to export. Could not open the folder \"%1\" for writing. Do you have the necessary permissions and free disk-space to write to that folder?")
                                     .arg(mXmlPathFileName.toHtmlEscaped()),
                             false);
        assetsFuture.cancel();
        releaseExportLock();
        endExportRun();
        return;
    }
    checkWriteability.close();

    // This gets reset anytime that something goes wrong so that the export gets
    // aborted and shows an error message rather than an okay one:
    bool isOk = true;

    QList<QTreeWidgetItem*> trigList;
    QList<QTreeWidgetItem*> timerList;
    QList<QTreeWidgetItem*> aliasList;
    QList<QTreeWidgetItem*> actionList;
    QList<QTreeWidgetItem*> scriptList;
    QList<QTreeWidgetItem*> keyList;
    exportXml(isOk, trigList, timerList, aliasList, actionList, scriptList, keyList);
    markExportItems(trigList, timerList, aliasList, actionList, scriptList, keyList);

    if (isOk) {
        // ensure assets have been copied before we start writing the zip
        // this will freeze the main thread, so it's not the perfect way - ideally
        // only start this after assets copy + xml writing is complete
        assetsFuture.waitForFinished();
        cleanupUnusedImages(tempPath, plainDescription);
        if (auto [success, message] = assetsFuture.result(); !success) {
            displayResultMessage(message, false);
            isOk = false;
        } else {
            auto future = QtConcurrent::run(dlgPackageExporter::zipPackage, stagingDirName, mPackagePathFileName, mXmlPathFileName, mPackageName, mPackageComment, mpZipCancelled);
            auto watcher = new QFutureWatcher<std::pair<bool, QString>>(this);
            mpZipWatcher = watcher;
            connect(watcher, &QFutureWatcher<std::pair<bool, QString>>::finished, this, [=, this]() {
                releaseExportLock();

                if (auto [isOk, errorMsg] = future.result(); !isOk) {
                    displayResultMessage(errorMsg, false);
                } else {
                    // If in module creation mode, automatically install the module
                    // - unless its profile has closed meanwhile
                    if (mIsModuleCreationMode && mpHost) {
                        // If a module of this name is already installed, the user
                        // confirmed overwriting it before the export began (see the
                        // overwrite prompt in slot_exportPackage), so remove the old
                        // copy to make way for the freshly-exported one. Testing the
                        // installed-modules list is language-independent, unlike
                        // matching the English text of the install error message.
                        if (mpHost->mInstalledModules.contains(mPackageName)) {
                            if (!mpHost->uninstallPackage(mPackageName, enums::PackageModuleType::ModuleFromUI)) {
                                // Uninstall can refuse (e.g. a profile save is in
                                // progress). The freshly-exported file is safe on
                                // disk, but the old module is still registered, so
                                // installing the new copy would fail as "already
                                // installed". Report that honestly instead of
                                // proceeding to a misleading success message.
                                displayResultMessage(tr("Module \"%1\" exported but failed to uninstall existing version").arg(mPackageName.toHtmlEscaped()), false);
                                mCancelButton->setVisible(false);
                                mCloseButton->setVisible(true);
                                endExportRun();
                                mpZipWatcher = nullptr;
                                watcher->deleteLater();
                                return;
                            }
                        }
                        // Quiet: the failure branch below says it was exported but not installed, which the console line can't
                        auto [installSuccess, installMessage] = mpHost->installPackage(mPackagePathFileName, enums::PackageModuleType::ModuleFromUI, true);
                        if (installSuccess) {
                            const QString savedDir = QFileInfo(mPackagePathFileName).absolutePath();
                            const QString savedDirLink = qsl("<a href=\"%1\">%2</a>").arg(QUrl::fromLocalFile(savedDir).toString(QUrl::FullyEncoded).toHtmlEscaped(), savedDir.toHtmlEscaped());
                            // Items with broken Lua are not reported here: the export worked, and the editor behind this
                            // dialog already shows the errors against the items.
                            // Show embedded success message (better UX than popup)
                            //: %1 is the module name, %2 is a clickable link to the folder the module file was saved in
                            displayResultMessage(tr("Module \"%1\" created and installed successfully! Saved to: %2. You can now close this dialog.").arg(mPackageName.toHtmlEscaped(), savedDirLink),
                                                 true);

                            // Clear the form to allow creating another module
                            ui->lineEdit_packageName->clear();
                            ui->lineEdit_packageName->setFocus();
                        } else {
                            displayResultMessage(tr("Module \"%1\" exported but installation failed: %2").arg(mPackageName.toHtmlEscaped(), installMessage.toHtmlEscaped()), false);
                        }
                    } else {
                        displayResultMessage(tr("Package \"%1\" exported to: %2").arg(mPackageName.toHtmlEscaped(), qsl("<a href=\"file:///%1\">%1</a>").arg(getActualPath().toHtmlEscaped())), true);
                    }
                }
                mCancelButton->setVisible(false);
                mCloseButton->setVisible(true);
                endExportRun();

                // Clean up the watcher
                mpZipWatcher = nullptr;
                watcher->deleteLater();
            });
            watcher->setFuture(future);
        }
    }

    if (isOk) {
        displayResultMessage(tr("Exporting package..."), true);
    } else {
        releaseExportLock();
        mCancelButton->setVisible(false);
        mCloseButton->setVisible(true);
        endExportRun();
    }

    // save settings for future reuse
    QSettings& settings = *MudletApp::getQSettings();
    settings.setValue("packageAuthor", ui->lineEdit_author->text());
}

//copy the newly-added description image files
QString dlgPackageExporter::copyNewImagesToTmp(const QString& tempPath) const
{
    QStringList newImagesList;
    //don't change the original plain description here as it may still be needed, for example if creating another package
    QString plainDescription = mPlainDescription;
    for (int i = mDescriptionImages.size() - 1; i >= 0; i--) {
        const QString fname = mDescriptionImages.at(i);
        const QFileInfo info(fname);
        if (plainDescription.contains(qsl("$%1").arg(info.fileName()))) {
            newImagesList.append(fname);
        }
    }

    if (!newImagesList.isEmpty()) {
        //Create description image dir
        const QString descriptionImagesDirName = qsl("%1.mudlet/description_images/").arg(tempPath);
        const QDir descriptionImageDir = QDir(descriptionImagesDirName);
        if (!descriptionImageDir.exists()) {
            descriptionImageDir.mkpath(descriptionImagesDirName);
        }
        for (int i = newImagesList.size() - 1; i >= 0; i--) {
            const QFileInfo imageFile(newImagesList.at(i));
            if (imageFile.exists()) {
                QString imageDir = descriptionImagesDirName;
                imageDir.append(imageFile.fileName());
                QFile::copy(imageFile.absoluteFilePath(), imageDir);
            }
            //replaces spaces with %20 in image file name to create a compatible url
            const QString imageName = QUrl::toPercentEncoding(imageFile.fileName()).constData();
            //replace temporary path with the path that is now inside the package; angle brackets
            //keep the Markdown link destination intact once $packagePath expands to a path with spaces
            plainDescription.replace(qsl("$%1").arg(imageFile.fileName()), qsl("<$packagePath/.mudlet/description_images/%1>").arg(imageName));
        }
    }
    return plainDescription;
}

// purge images from tmp which are no longer used by the description
void dlgPackageExporter::cleanupUnusedImages(const QString& tempPath, const QString& plainDescription)
{
    static const QRegularExpression imagesInUsePattern(R"(\$packagePath\/\.mudlet\/description_images\/(.+?)\.)");
    QStringList imagesInUse;
    QRegularExpressionMatchIterator i = imagesInUsePattern.globalMatch(plainDescription);
    while (i.hasNext()) {
        auto match = i.next();
        // The description stores the image path percent-encoded (e.g. a space
        // becomes %20), but the copied file on disk keeps its decoded name, so
        // decode before comparing. Otherwise an in-use image whose filename
        // contains a space or other special character is treated as unused and
        // deleted, shipping a package with a missing image.
        imagesInUse << QUrl::fromPercentEncoding(match.captured(1).remove(QChar('\"')).toUtf8());
    }

    // iterate through all images in folder, if our list doesn't contain it - remove
    QDirIterator allImagesCopied(qsl("%1.mudlet/description_images").arg(tempPath), QDir::Files);
    while (allImagesCopied.hasNext()) {
        const QFileInfo copiedImage(allImagesCopied.next());
        if (!imagesInUse.contains(copiedImage.baseName())) {
            if (!QFile(copiedImage.absoluteFilePath()).remove()) {
                qDebug() << "couldn't remove unused image" << copiedImage.fileName();
            }
        }
    }
}

void dlgPackageExporter::markExportItems(QList<QTreeWidgetItem*>& trigList,
                                         QList<QTreeWidgetItem*>& timerList,
                                         QList<QTreeWidgetItem*>& aliasList,
                                         QList<QTreeWidgetItem*>& actionList,
                                         QList<QTreeWidgetItem*>& scriptList,
                                         QList<QTreeWidgetItem*>& keyList)
{ //now fix all the stuff we weren't exporting
    //trigger, timer, alias, action, script, keys
    for (auto item : std::as_const(trigList)) {
        if (auto* pTrigger = triggerOf(triggerMap, item)) {
            pTrigger->exportItem = true;
        }
        if (auto* pTrigger = triggerOf(modTriggerMap, item)) {
            pTrigger->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(timerList)) {
        if (auto* pTimer = timerOf(timerMap, item)) {
            pTimer->exportItem = true;
        }
        if (auto* pTimer = timerOf(modTimerMap, item)) {
            pTimer->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(actionList)) {
        if (auto* pAction = actionOf(actionMap, item)) {
            pAction->exportItem = true;
        }
        if (auto* pAction = actionOf(modActionMap, item)) {
            pAction->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(scriptList)) {
        if (auto* pScript = scriptOf(scriptMap, item)) {
            pScript->exportItem = true;
        }
        if (auto* pScript = scriptOf(modScriptMap, item)) {
            pScript->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(keyList)) {
        if (auto* pKey = keyOf(keyMap, item)) {
            pKey->exportItem = true;
        }
        if (auto* pKey = keyOf(modKeyMap, item)) {
            pKey->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(aliasList)) {
        if (auto* pAlias = aliasOf(aliasMap, item)) {
            pAlias->exportItem = true;
        }
        if (auto* pAlias = aliasOf(modAliasMap, item)) {
            pAlias->mModuleMasterFolder = true;
        }
    }
}
void dlgPackageExporter::exportXml(bool& isOk,
                                   QList<QTreeWidgetItem*>& trigList,
                                   QList<QTreeWidgetItem*>& timerList,
                                   QList<QTreeWidgetItem*>& aliasList,
                                   QList<QTreeWidgetItem*>& actionList,
                                   QList<QTreeWidgetItem*>& scriptList,
                                   QList<QTreeWidgetItem*>& keyList)
{
    XMLexport writer(mpHost);
    //write trigs
    QTreeWidgetItem* top = mpTriggers;
    recurseTree(top, trigList);
    for (auto item : std::as_const(trigList)) {
        auto* pTrigger = triggerOf(triggerMap, item);
        if (!pTrigger) {
            continue;
        }
        // A dash only says something inside is ticked, and XMLimport remakes a package folder
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pTrigger->exportItem = false;
        } else if (pTrigger->mModuleMasterFolder) {
            pTrigger->mModuleMasterFolder = false;
            modTriggerMap.insert(item, pTrigger->getID());
        }
    }
    top = mpTimers;
    recurseTree(top, timerList);
    for (auto item : std::as_const(timerList)) {
        auto* pTimer = timerOf(timerMap, item);
        if (!pTimer) {
            continue;
        }
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pTimer->exportItem = false;
        } else if (pTimer->mModuleMasterFolder) {
            pTimer->mModuleMasterFolder = false;
            modTimerMap.insert(item, pTimer->getID());
        }
    }
    top = mpAliases;
    recurseTree(top, aliasList);
    for (auto item : std::as_const(aliasList)) {
        auto* pAlias = aliasOf(aliasMap, item);
        if (!pAlias) {
            continue;
        }
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pAlias->exportItem = false;
        } else if (pAlias->mModuleMasterFolder) {
            pAlias->mModuleMasterFolder = false;
            modAliasMap.insert(item, pAlias->getID());
        }
    }
    top = mpButtons;
    recurseTree(top, actionList);
    for (auto item : std::as_const(actionList)) {
        auto* pAction = actionOf(actionMap, item);
        if (!pAction) {
            continue;
        }
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pAction->exportItem = false;
        } else if (pAction->mModuleMasterFolder) {
            pAction->mModuleMasterFolder = false;
            modActionMap.insert(item, pAction->getID());
        }
    }
    top = mpScripts;
    recurseTree(top, scriptList);
    for (auto item : std::as_const(scriptList)) {
        auto* pScript = scriptOf(scriptMap, item);
        if (!pScript) {
            continue;
        }
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pScript->exportItem = false;
        } else if (pScript->mModuleMasterFolder) {
            pScript->mModuleMasterFolder = false;
            modScriptMap.insert(item, pScript->getID());
        }
    }
    top = mpKeys;
    recurseTree(top, keyList);
    for (auto item : std::as_const(keyList)) {
        auto* pKey = keyOf(keyMap, item);
        if (!pKey) {
            continue;
        }
        const bool goesIn = item->checkState(0) == Qt::Checked && !packageFolder(item);
        if (!goesIn) {
            pKey->exportItem = false;
        } else if (pKey->mModuleMasterFolder) {
            pKey->mModuleMasterFolder = false;
            modKeyMap.insert(item, pKey->getID());
        }
    }

    if (!writer.exportPackage(mXmlPathFileName, false, true)) {
        //: This error message is shown when all the Mudlet items cannot be written to the 'packageName'.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened
        displayResultMessage(tr("Failed to export. Could not write Mudlet items to the file \"%1\".").arg(mXmlPathFileName.toHtmlEscaped()), false);
        // Although we have failed, we must not just abort here. We need to reset
        // the selected "for export or not"-flags first. So note that we have failed:
        isOk = false;
        // After the following we will then drop through to the end of the
        // method to set up a means to close the dialogue after the user has
        // seen the error message...
    }
}

QString dlgPackageExporter::normalizedHelpUrl() const
{
    QString url = ui->lineEdit_helpUrl->text().trimmed();
    // anchored so a "://" buried in a query string does not count as a scheme
    static const QRegularExpression schemePattern(qsl("^[a-zA-Z][a-zA-Z0-9+.-]*://"));
    if (!url.isEmpty() && !url.contains(schemePattern)) {
        // scheme-less URLs silently fail to open in a browser later
        url.prepend(qsl("https://"));
    }
    return url;
}

void dlgPackageExporter::writeConfigFile(const QString& stagingDirName, const QFileInfo& iconFile, const QString& packageDescription)
{
    QStringList dependencies;
    for (int index = 0; index < ui->comboBox_dependencies->count(); index++) {
        dependencies << ui->comboBox_dependencies->itemText(index);
    }

    mPackageConfig.clear();
    mPackageComment.clear();
    appendToDetails(qsl("mpackage"), mPackageName);
    appendToDetails(qsl("author"), ui->lineEdit_author->text());
    appendToDetails(qsl("icon"), iconFile.fileName());
    appendToDetails(qsl("title"), ui->lineEdit_title->text());
    appendToDetails(qsl("description"), packageDescription);
    appendToDetails(qsl("version"), ui->lineEdit_version->text());
    appendToDetails(qsl("helpURL"), normalizedHelpUrl());
    appendToDetails(qsl("dependencies"), dependencies.join(","));
    const auto iso8601timestamp = utils::dateStamp();
    mPackageConfig.append(qsl("created = \"%1\"\n").arg(iso8601timestamp));
    mPackageComment.append(qsl("    created: %1\n").arg(iso8601timestamp));

    const QString luaConfig = qsl("%1/config.lua").arg(stagingDirName);
    QSaveFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << mPackageConfig;
        if (!configFile.commit()) {
            qDebug() << "dlgPackageExporter::writeConfigFile: error saving package config data: " << configFile.errorString();
        }
    }
}

QFileInfo dlgPackageExporter::copyIconToTmp(const QString& tempPath) const
{
    const QFileInfo iconFile(mPackageIconPath);
    if (iconFile.exists()) {
        QString iconDirName = qsl("%1.mudlet/Icon/").arg(tempPath);
        const QDir iconDir = QDir(iconDirName);
        if (!iconDir.exists()) {
            iconDir.mkpath(iconDirName);
        }
        iconDirName.append(iconFile.fileName());
        QFile::copy(mPackageIconPath, iconDirName);
    }
    return iconFile;
}

// Leaves the old icon behind, as copyIconToTmp() supplies the one the package has now;
// description images the description no longer uses are dropped by cleanupUnusedImages()
void dlgPackageExporter::copyPackageDataToTmp(const QString& tempPath) const
{
    if (mPackageDataPath.isEmpty() || !QFileInfo(mPackageDataPath).isDir()) {
        return;
    }
    const QString stagedDataPath = qsl("%1.mudlet").arg(tempPath);
    QDir().mkpath(stagedDataPath);
    QDirIterator it(mPackageDataPath, QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
    while (it.hasNext()) {
        const QFileInfo entry(it.next());
        if (entry.isSymLink() || entry.fileName() == QLatin1String("Icon")) {
            continue;
        }
        const QString target = qsl("%1/%2").arg(stagedDataPath, entry.fileName());
        if (entry.isDir()) {
            copy_directory(entry.absoluteFilePath(), target, false);
        } else {
            QFile::copy(entry.absoluteFilePath(), target);
        }
    }
}

std::pair<bool, QString> dlgPackageExporter::copyAssetsToTmp(const QStringList& assetPaths, const QString& tempPath)
{
    for (const auto& assetPath : assetPaths) {
        const QFileInfo asset(assetPath);
        QString filePath = tempPath;
        filePath.append(asset.fileName());
        if (!asset.exists()) {
            return {false, tr("%1 doesn't seem to exist anymore - can you double-check it?").arg(asset.absoluteFilePath().toHtmlEscaped())};
        }
        if (asset.isFile()) {
            QFile::remove(filePath);
            if (!QFile::copy(asset.absoluteFilePath(), filePath)) {
                return {false, tr("cannot copy %1 to the temporary location %2 - can you double-check it?").arg(asset.absoluteFilePath().toHtmlEscaped(), tempPath.toHtmlEscaped())};
            }
        } else if (asset.isDir()) {
            copy_directory(asset.absoluteFilePath(), filePath, false);
        }
    }

    return {true, QString{}};
}

std::pair<bool, QString> dlgPackageExporter::zipPackage(const QString& stagingDirName,
                                                        const QString& packagePathFileName,
                                                        const QString& xmlPathFileName,
                                                        const QString& packageName,
                                                        const QString& packageComment,
                                                        [[maybe_unused]] std::shared_ptr<std::atomic<bool>> cancelled)
{
    bool isOk = true;
    QString error;
    // zip error code:
    int ze = 0;


    // ZIP_CREATE creates the archive if it does not exist.
    // ZIP_TRUNCATE zaps any contents in a previously existing file.
    zip* archive = zip_open(packagePathFileName.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &ze);

    if (!archive) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, ze);
        /*: This zipError message is shown when the libzip library code is unable
 to open the file that was to be the end result of the export process.
 As this may be an existing file anywhere in the computer's
 file-system(s) it is possible that permissions on the directory or an
 existing file that is to be overwritten may be a source of problems
 here.*/
        const QString errMsg = tr("Failed to open package file. Error is: \"%1\".").arg(zip_error_strerror(&zipError));
        zip_error_fini(&zipError);
        return {false, errMsg};
    }
    // Opened/created archive file successfully
#if defined(Q_OS_WINDOWS)
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    /*
     * From Qt Docs:
     * Note: On NTFS file systems, ownership and permissions checking is disabled by
     * default for performance reasons. To enable it, include the following line:
     */
    extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
    /*
     * Permission checking is then turned on and off by incrementing and
     * decrementing qt_ntfs_permission_lookup by 1:
     */
    qt_ntfs_permission_lookup++;
#else
    /*
     * The prior system has been deprecated in Qt 6.6 and the following is
     * a low-level call to provide similar functionality. Ideally
     * the higher level QNtfsPermissionCheckGuard class would be used instead.
     */
    qEnableNtfsPermissionChecks();
#endif
#endif // defined(Q_OS_WINDOWS)
    // QDir::Hidden is needed so the .mudlet staging directory (description
    // images and other assets) makes it into the archive on Unix systems:
    QDirIterator stagingFile(stagingDirName, QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    // relative names to use in archive:
    QStringList directoryEntries;
    // Key is relative name to use in archive
    // Value is fullName in file-system:
    QMap<QString, QString> fileEntries;
    while (stagingFile.hasNext() && isOk) {
        const QString itEntry = stagingFile.next();
        Q_UNUSED(itEntry)
        // Dot and DotDot entries are no use to us so skip them
        if (!(stagingFile.fileName().compare(qsl(".")) && stagingFile.fileName().compare(qsl("..")))) {
            // Dot and DotDot entries are no use to us so skip them
            continue;
        }

        const QFileInfo stagingFileInfo(stagingFile.fileInfo());
        if (!stagingFileInfo.isReadable()) {
            qWarning() << "dlgPackageExporter::slot_exportPackage() skipping file: " << stagingFile.fileName() << "it is NOT readable!";
            continue;
        }

        if (stagingFileInfo.isSymLink()) {
            qWarning() << "dlgPackageExporter::slot_exportPackage() skipping file: " << stagingFile.fileName() << "it is a Symlink - avoided to prevent file-system loops!";
            continue;
        }

        QString nameInArchive = stagingFile.filePath();
        nameInArchive.remove(qsl("%1/").arg(stagingDirName));

        if (stagingFileInfo.isDir()) {
            directoryEntries.append(nameInArchive);
        } else if (stagingFileInfo.isFile()) {
            fileEntries.insert(nameInArchive, stagingFile.filePath());
        }
    }

#if defined(Q_OS_WINDOWS)
    // Turn off permission checking on NTFS file systems
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    qt_ntfs_permission_lookup--;
#else
    qDisableNtfsPermissionChecks();
#endif
#endif

    if (directoryEntries.count() > 1) {
        std::sort(directoryEntries.begin(), directoryEntries.end());
    }

    QStringListIterator itDirectoryName(directoryEntries);
    while (itDirectoryName.hasNext() && isOk) {
        const QString directoryName = itDirectoryName.next();
        // zip_dir_add(...) returns the index of the
        // added directory item in the archive or -1 on error:
        if (zip_dir_add(archive, directoryName.toStdString().c_str(), ZIP_FL_ENC_UTF_8) == -1) {
            const QString errorMsg = tr("Failed to add directory \"%1\" to package. Error is: \"%2\".").arg(directoryName.toHtmlEscaped(), zip_strerror(archive));
            zip_discard(archive);
            return {false, errorMsg};
        }
    }

    // Process the config and the file containing the Mudlet triggers,
    // etc. specially so they are inserted first and last respectively:
    if (isOk) {
        if (fileEntries.contains(qsl("config.lua"))) {
            if (!writeFileToZip(qsl("config.lua"), fileEntries.value(qsl("config.lua")), archive).first) {
                /* isOk = false; */
            } else {
                fileEntries.remove(qsl("config.lua"));
            }
        }
    }

    QString xmlFileName = packageName;
    xmlFileName.append(QLatin1String(".xml"));
    if (isOk) {
        QMapIterator<QString, QString> itFileName(fileEntries);
        while (itFileName.hasNext() && isOk) {
            itFileName.next();
            if (itFileName.key() == xmlFileName) {
                continue;
            }

            if (std::tie(isOk, error) = writeFileToZip(itFileName.key(), itFileName.value(), archive); !isOk) {
                zip_discard(archive);
                break;
            }
        }
    }

    if (isOk) {
        if (fileEntries.contains(xmlFileName) && fileEntries.value(xmlFileName) == xmlPathFileName) {
            std::tie(isOk, error) = writeFileToZip(xmlFileName, xmlPathFileName, archive);

            // If successful will get to HERE...

        } else {
            return {false,
                    tr("Required file \"%1\" was not found in the staging area. "
                       "This area contains the Mudlet items chosen for the package, "
                       "which you selected to be included in the package file. "
                       "This suggests there may be a problem with that directory: "
                       "\"%2\" - "
                       "Do you have the necessary permissions and free disk-space?")
                            .arg(xmlPathFileName, QDir(stagingDirName).canonicalPath())};
        }
    }

    if (isOk) {
        zip_set_archive_comment(archive, packageComment.toUtf8().constData(), static_cast<zip_uint16_t>(packageComment.toUtf8().length()));

#if defined(LIBZIP_SUPPORTS_CANCELLING)
        auto cancel_callback = [](zip*, void* state) -> int {
            return static_cast<std::atomic<bool>*>(state)->load();
        };
        zip_register_cancel_callback_with_state(archive, cancel_callback, nullptr, cancelled.get());
#endif

        // THIS is the point that the archive gets created from the
        // source materials - it may take a short while!
        // If it fails to write out the new file 'archive' is left
        // unchanged (and we can still access it to get the error
        // details):
        ze = zip_close(archive);
        if (ze) {
            // libzip's C interface around the error message isn't trivial - so copy it over into Qt land where things are simpler
            const QString zipError{zip_strerror(archive)};
            if (zipError == qsl("Operation cancelled")) {
                zip_discard(archive);
                return {false, tr("Export cancelled.")};
            }

            /*: This error message is displayed at the final stage of exporting
 a package when all the sourced files are finally put into the
 archive. Unfortunately this may be the point at which something
 breaks because a problem was not spotted/detected in the process
 earlier...*/
            const QString errorMsg = tr("Failed to zip up the package. Error is: \"%1\".").arg(zipError);
            zip_discard(archive);
            // In libzip 0.11 a function was added to clean up
            // (deallocate) the memory associated with an archive
            // - which would normally occur upon a successful close
            // - before that version the memory just leaked away...
            return {false, errorMsg};
        }

    } else {
        zip_discard(archive);
    }

    return {isOk, error};
}

void dlgPackageExporter::slot_addFiles()
{
    QFileDialog* fDialog = new QFileDialog;
    fDialog->setFileMode(QFileDialog::Directory);
    fDialog->setOption(QFileDialog::DontUseNativeDialog);

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();
    fDialog->setDirectory(lastDir);

    QStringList selectedFiles;
    //change file dialog children functions to support multiple folder+file selection
    //as qt doesn't seem to support that out of the box
    QDialogButtonBox* dialogBox = fDialog->findChild<QDialogButtonBox*>();
    QPushButton* button = dialogBox->button(QDialogButtonBox::Open);
    QListView* dialogListView = fDialog->findChild<QListView*>("listView");

    if (dialogListView) {
        dialogListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //button would be disabled if no folder is selected
        connect(dialogListView, &QListView::clicked, this, [=] {
            button->setEnabled(true);
        });
    }
    QTreeView* dialogTreeView = fDialog->findChild<QTreeView*>();
    if (dialogTreeView) {
        dialogTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(dialogTreeView, &QTreeView::clicked, this, [=] {
            button->setEnabled(true);
        });
    }
    connect(button, &QPushButton::clicked, this, [=] {
        fDialog->QDialog::accept();
    });
    if (fDialog->exec()) {
        selectedFiles = fDialog->selectedFiles();
    }
    if (!selectedFiles.isEmpty()) {
        ui->listWidget_addedFiles->addItems(selectedFiles);

        lastDir = fDialog->directory().absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);
        if (!mIsModuleCreationMode) {
            // adding asset files must not redirect where the module gets saved
            mPackagePath = lastDir;
            emit signal_exportLocationChanged(mPackagePath);
        }
    }
    fDialog->deleteLater();
}

void dlgPackageExporter::slot_openPackageLocation()
{
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString chosenPath = QFileDialog::getExistingDirectory(nullptr, tr("Where do you want to save the package?"), lastDir, QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);

    if (chosenPath.isEmpty()) {
        // The user cancelled the picker - keep any previously chosen location
        // rather than silently reverting the export path to the profile folder.
        return;
    }

    mPackagePath = chosenPath;
    settings.setValue("lastFileDialogLocation", mPackagePath);
    emit signal_exportLocationChanged(mPackagePath);
}

//only uncheck children
void uncheckRecursive(QTreeWidgetItem* item)
{
    item->setCheckState(0, Qt::Unchecked);
    item->setData(0, dlgPackageExporter::scmForcedInRole, false);
    for (int i = 0; i < item->childCount(); i++) {
        uncheckRecursive(item->child(i));
    }
}

void dlgPackageExporter::uncheckAllChildren()
{
    for (int i = 0; i < mpExportSelection->topLevelItemCount(); i++) {
        for (int j = 0; j < mpExportSelection->topLevelItem(i)->childCount(); j++) {
            uncheckRecursive(mpExportSelection->topLevelItem(i)->child(j));
        }
    }
}

int dlgPackageExporter::countRecursive(QTreeWidgetItem* item, int count) const
{
    count = count + (item->checkState(0) == Qt::Checked && !packageFolder(item) ? 1 : 0);
    for (int i = 0; i < item->childCount(); i++) {
        count = countRecursive(item->child(i), count);
    }
    return count;
}

int dlgPackageExporter::countCheckedItems() const
{
    int count = 0;
    for (int i = 0; i < mpExportSelection->topLevelItemCount(); i++) {
        count = count + (mpExportSelection->topLevelItem(i)->checkState(0) == Qt::Checked ? 1 : 0);
        for (int j = 0; j < mpExportSelection->topLevelItem(i)->childCount(); j++) {
            count = countRecursive(mpExportSelection->topLevelItem(i)->child(j), count);
        }
    }

    return count;
}

void dlgPackageExporter::checkChildren(QTreeWidgetItem* item) const
{
    if (!mCheckChildren) {
        return;
    }
    // A dash is derived, never handed down: clicking a dashed folder asks for all of it
    const auto checkState = item->checkState(0) == Qt::PartiallyChecked ? Qt::Checked : item->checkState(0);

    for (int i = 0; i < item->childCount(); i++) {
        item->child(i)->setCheckState(0, checkState);
    }
}

// The editor and scripts can delete items while this dialog is open, so the
// tree keeps their ids and each use looks the item up again
TTrigger* dlgPackageExporter::triggerOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getTriggerUnit()->getTrigger(map.value(item)) : nullptr;
}

TTimer* dlgPackageExporter::timerOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getTimerUnit()->getTimer(map.value(item)) : nullptr;
}

TAlias* dlgPackageExporter::aliasOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getAliasUnit()->getAlias(map.value(item)) : nullptr;
}

TAction* dlgPackageExporter::actionOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getActionUnit()->getAction(map.value(item)) : nullptr;
}

TScript* dlgPackageExporter::scriptOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getScriptUnit()->getScript(map.value(item)) : nullptr;
}

TKey* dlgPackageExporter::keyOf(const QMap<QTreeWidgetItem*, int>& map, QTreeWidgetItem* item) const
{
    return map.contains(item) ? mpHost->getKeyUnit()->getKey(map.value(item)) : nullptr;
}

bool dlgPackageExporter::packageFolder(const QTreeWidgetItem* pItem) const
{
    if (!pItem || !pItem->parent() || pItem->parent()->parent() || pItem->data(0, Qt::UserRole) != isTopFolder || pItem->data(0, scmForcedInRole).toBool()) {
        return false;
    }
    // Only the picked installed package's: a folder that merely shares the typed name is the user's own, with patterns
    // or CSS XMLimport would not make again
    return ui->packageList->currentIndex() > 0 && pItem->text(0) == ui->packageList->currentText();
}

static bool everythingUnderIsTicked(const QTreeWidgetItem* pItem)
{
    for (int child = 0; child < pItem->childCount(); ++child) {
        const QTreeWidgetItem* pChild = pItem->child(child);
        if (pChild->checkState(0) != Qt::Checked || !everythingUnderIsTicked(pChild)) {
            return false;
        }
    }
    return true;
}

bool dlgPackageExporter::refreshFolderMark(QTreeWidgetItem* pItem)
{
    bool tickedInside = false;
    for (int child = 0; child < pItem->childCount(); ++child) {
        tickedInside = refreshFolderMark(pItem->child(child)) || tickedInside;
    }

    if (packageFolder(pItem)) {
        const bool allOfIt = pItem->childCount() > 0 && everythingUnderIsTicked(pItem);
        pItem->setCheckState(0, allOfIt ? Qt::Checked : (tickedInside ? Qt::PartiallyChecked : Qt::Unchecked));
        // XMLimport makes this folder again, so its own tick counts for nothing above
        return tickedInside;
    }

    if (pItem->childCount() == 0) {
        return pItem->checkState(0) == Qt::Checked;
    }

    // A folder ticked itself keeps it: that is the extra nesting a right-click asks for
    if (pItem->checkState(0) == Qt::Checked) {
        return true;
    }
    pItem->setCheckState(0, tickedInside ? Qt::PartiallyChecked : Qt::Unchecked);
    return tickedInside;
}

void dlgPackageExporter::refreshFolderMarks()
{
    if (!mpExportSelection) {
        return;
    }
    // Or slot_recountItems() runs checkChildren(), which hands the derived state down
    const QSignalBlocker held(mpExportSelection);
    for (int top = 0; top < mpExportSelection->topLevelItemCount(); ++top) {
        QTreeWidgetItem* pTop = mpExportSelection->topLevelItem(top);
        for (int child = 0; child < pTop->childCount(); ++child) {
            refreshFolderMark(pTop->child(child));
        }
    }
}

void dlgPackageExporter::recurseTriggers(TTrigger* trig, QTreeWidgetItem* qTrig)
{
    std::list<Tree<TTrigger>*>* childList = trig->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TTrigger>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TTrigger*>(*it);
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        triggerMap.insert(pItem, pChild->getID());
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qTrig->addChild(pItem);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::listTriggers()
{
    TriggerUnit* tu = mpHost->getTriggerUnit();
    std::list<TTrigger*>::const_iterator it;
    std::list<TTrigger*> tList = tu->getTriggerRootNodeList();
    QTreeWidgetItem* top = mpTriggers;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        top->addChild(pItem);
        triggerMap.insert(pItem, pChild->getID());
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::recurseAliases(TAlias* item, QTreeWidgetItem* qItem)
{
    std::list<Tree<TAlias>*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TAlias>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TAlias*>(*it);
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qItem->addChild(pItem);
        aliasMap.insert(pItem, pChild->getID());
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::listAliases()
{
    AliasUnit* tu = mpHost->getAliasUnit();
    std::list<TAlias*>::const_iterator it;
    std::list<TAlias*> tList = tu->getAliasRootNodeList();
    QTreeWidgetItem* top = mpAliases;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        top->addChild(pItem);
        aliasMap.insert(pItem, pChild->getID());
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::recurseScripts(TScript* item, QTreeWidgetItem* qItem)
{
    std::list<Tree<TScript>*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TScript>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TScript*>(*it);
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild->getID());
        qItem->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::listScripts()
{
    ScriptUnit* tu = mpHost->getScriptUnit();
    std::list<TScript*>::const_iterator it;
    std::list<TScript*> tList = tu->getScriptRootNodeList();
    QTreeWidgetItem* top = mpScripts;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        scriptMap.insert(pItem, pChild->getID());
        top->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::recurseKeys(TKey* item, QTreeWidgetItem* qItem)
{
    std::list<Tree<TKey>*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TKey>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TKey*>(*it);
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild->getID());
        qItem->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::listKeys()
{
    KeyUnit* tu = mpHost->getKeyUnit();
    std::list<TKey*>::const_iterator it;
    std::list<TKey*> tList = tu->getKeyRootNodeList();
    QTreeWidgetItem* top = mpKeys;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TKey* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        keyMap.insert(pItem, pChild->getID());
        top->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::recurseActions(TAction* item, QTreeWidgetItem* qItem)
{
    std::list<Tree<TAction>*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TAction>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TAction*>(*it);
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild->getID());
        qItem->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::listActions()
{
    ActionUnit* tu = mpHost->getActionUnit();
    std::list<TAction*>::const_iterator it;
    std::list<TAction*> tList = tu->getActionRootNodeList();
    QTreeWidgetItem* top = mpButtons;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        actionMap.insert(pItem, pChild->getID());
        top->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::recurseTimers(TTimer* item, QTreeWidgetItem* qItem)
{
    std::list<Tree<TTimer>*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<Tree<TTimer>*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        auto* pChild = static_cast<TTimer*>(*it);
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild->getID());
        qItem->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::listTimers()
{
    TimerUnit* tu = mpHost->getTimerUnit();
    std::list<TTimer*>::const_iterator it;
    std::list<TTimer*> tList = tu->getTimerRootNodeList();
    QTreeWidgetItem* top = mpTimers;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        timerMap.insert(pItem, pChild->getID());
        top->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::displayResultMessage(const QString& html, const bool isSuccessMessage)
{
    if (!isSuccessMessage) {
        if (mDeleteWhenExportEnds) {
            // Closed mid-export, so this hidden dialog goes without anyone seeing its label
            qWarning().noquote().nospace() << "dlgPackageExporter WARNING - exporting \"" << mPackageName
                                           << "\" failed after its dialog was closed: " << QTextDocumentFragment::fromHtml(html).toPlainText();
        }
        ui->infoLabel->setText(qsl("<p>%1</p>").arg(html));
        showNotice(html, uiDesign::NoticeKind::Error);
        return;
    }

    const QString told = qsl("<p>%1</p>"
                             "<p>%2</p>")
                                 .arg(html,
                                      /*:
                            Only the text outside of the 'a' (HTML anchor) tags PLUS the verb
                            'upload' in between them in the source text, (associated with uploading
                            the resulting package to the Mudlet forums) should be translated.
                            */
                                      tr("Why not <a href=\"https://packages.mudlet.org/upload\">upload</a> your package for other Mudlet users?"));
    ui->infoLabel->setText(told);
    showNotice(told, uiDesign::NoticeKind::Information);
}

void dlgPackageExporter::slot_recountItems(QTreeWidgetItem* item)
{
    checkChildren(item);
    if (!mRecountPending) {
        mRecountPending = true;
        QTimer::singleShot(0ms, this, [this]() {
            mRecountPending = false;

            refreshFolderMarks();
            const int itemsToExport = countCheckedItems();
            if (itemsToExport) {
                //: This is the text shown at the top of a groupbox when there is %n (one or more) items to export in the Package exporter dialogue; the initial (and when there is no items selected) is a separate text.
                mpSelectionText->setTitle(tr("Select what to export (%n item(s))", nullptr, itemsToExport));
            } else {
                //: This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.
                mpSelectionText->setTitle(tr("Select what to export"));
            }
            // The hidden box's title above is still written: the dialog's tests read the count off it
            showSelectionCount(itemsToExport);
            refreshStartOverButton();
        });
    }
}

// allow the top-folder to be force-selected for the package with right-click
// normally undesired as it'll add unnecessary nesting
void dlgPackageExporter::slot_rightClickOnItems(const QPoint& point)
{
    auto item = mpExportSelection->itemAt(point);
    if (!item || item->parent() == nullptr) {
        return;
    }
    mCheckChildren = false;
    // The package's own folder goes in whole, its patterns or CSS with it, only this way
    if (packageFolder(item)) {
        item->setData(0, scmForcedInRole, true);
        item->setCheckState(0, Qt::Checked);
    } else if (item->checkState(0) == Qt::Checked) {
        item->setData(0, scmForcedInRole, false);
        item->setCheckState(0, Qt::Unchecked);
    } else {
        item->setCheckState(0, Qt::Checked);
    }
    mCheckChildren = true;
}

QString dlgPackageExporter::getActualPath() const
{
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    return mPackagePath.isEmpty() ? lastDir : mPackagePath;
}

void dlgPackageExporter::slot_cancelExport()
{
    if (mpZipCancelled) {
        mpZipCancelled->store(true);
    }

    mCancelButton->setVisible(false);
    mCloseButton->setVisible(true);
}

//Description Class TextEdit
dlgPackageExporterDescription::dlgPackageExporterDescription(QWidget* pW)
: QTextEdit(pW)
{
}

dlgPackageExporterDescription::~dlgPackageExporterDescription() {}

// In lines of the current font rather than pixels, so a stylesheet or a larger
// font still leaves room to write
QSize dlgPackageExporterDescription::minimumSizeHint() const
{
    constexpr int minimumLines = 10;
    const int textHeight = minimumLines * fontMetrics().lineSpacing() + qCeil(2 * document()->documentMargin());
    // contentsMargins() already holds the frame's width
    const QMargins margins = viewportMargins() + contentsMargins();
    return {QTextEdit::minimumSizeHint().width(), textHeight + margins.top() + margins.bottom()};
}

bool dlgPackageExporterDescription::canInsertFromMimeData(const QMimeData* source) const
{
    if (source->hasUrls()) {
        return true;
    }
    return QTextEdit::canInsertFromMimeData(source);
}

void dlgPackageExporterDescription::insertFromMimeData(const QMimeData* source)
{
    dlgPackageExporter* my_parent = static_cast<dlgPackageExporter*>(topLevelWidget());
    if (source->hasUrls()) {
        QTextCursor myCursor = textCursor();
        QStringList accepted_types;
        accepted_types << "jpeg" << "jpg" << "png" << "gif" << "bmp" << "svg";
        for (const auto& url : source->urls()) {
            const QString fname = url.toLocalFile();
            const QFileInfo info(fname);
            if (info.exists() && accepted_types.contains(info.suffix().trimmed(), Qt::CaseInsensitive)) {
                if (!my_parent->mDescriptionImages.contains(fname)) {
                    my_parent->mDescriptionImages.append(fname);
                }
                const QString imgSrc = qsl("![Image]($%1)").arg(info.fileName());
                myCursor.insertText(imgSrc);
            }
        }
        my_parent->mPlainDescription = toPlainText();
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}

// Unfocused, the editor shows rendered markdown whose positions don't map back onto the
// markdown source, so show the source while files are dragged over it and drop into that
void dlgPackageExporterDescription::dragEnterEvent(QDragEnterEvent* event)
{
    if (!hasFocus() && event->mimeData()->hasUrls()) {
        mShowingSourceForDrop = true;
        setPlainText(static_cast<dlgPackageExporter*>(topLevelWidget())->mPlainDescription);
    }
    QTextEdit::dragEnterEvent(event);
}

void dlgPackageExporterDescription::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTextEdit::dragLeaveEvent(event);
    restoreRenderedDescription();
}

void dlgPackageExporterDescription::dropEvent(QDropEvent* event)
{
    QTextEdit::dropEvent(event);
    restoreRenderedDescription();
}

void dlgPackageExporterDescription::restoreRenderedDescription()
{
    if (!mShowingSourceForDrop) {
        return;
    }
    mShowingSourceForDrop = false;
    if (!hasFocus()) {
        static_cast<dlgPackageExporter*>(topLevelWidget())->showRenderedDescription();
    }
}

void dlgPackageExporter::endExportRun()
{
    mExportRunning = false;
    QApplication::restoreOverrideCursor();
    if (mDeleteWhenExportEnds) {
        deleteLater();
    }
}

// Close, Escape and the window's own close button all come through here
void dlgPackageExporter::done(int result)
{
    // The export reports, and installs a module, through this dialog, so it outlives the closing until the export ends
    if (mExportRunning && mpHost && testAttribute(Qt::WA_DeleteOnClose)) {
        setAttribute(Qt::WA_DeleteOnClose, false);
        mDeleteWhenExportEnds = true;
        // Heard directly, as whether close() reaches a hidden window's closeEvent() depends on its platform window
        connect(mpHost, &QObject::destroyed, this, &QObject::deleteLater);
    }
    QDialog::done(result);
}

void dlgPackageExporter::closeEvent(QCloseEvent* event)
{
    if (mpSplitter) {
        if (QSettings* pSettings = MudletApp::getQSettings()) {
            pSettings->setValue(QString::fromLatin1(scmExporterSplitterKey), mpSplitter->saveState());
        }
    }
    if (mpHost) {
        emit packageExporterClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
}
