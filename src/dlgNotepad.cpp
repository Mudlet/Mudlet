/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018, 2025 by Stephen Lyons                        *
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


#include "dlgNotepad.h"

#include "MudletApp.h"
#include "Host.h"
#include "HostDialogs.h"
#include "mudlet.h"
#include "ToolBarOverflow.h"
#include "uiDesign.h"
#include "utils.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QSaveFile>
#include <QScrollBar>
#include <QShortcut>
#include <QTabBar>
#include <QStringConverter>
#include <QTabBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>

#include <algorithm>
#include <chrono>

using namespace std::chrono;

const QString jsonNotesFileName{qsl("notes.json")};
const QString local8BitEncodedNotesFileName{qsl("notes.txt")};
const QString utf8EncodedNotesFileName{qsl("notes_utf8.txt")};

// Visible across a page of notes, yet short of the full pen that marks the cursor's match
constexpr qreal scmNotepadQuietMatchWash = 0.35;

// Six lines or fewer reads as a sequence of commands; a longer note sent with no prefix puts prose on the command line
constexpr int scmNotepadWarnAtLines = 6;
// QCommonStyle hard-codes this gap between a tool button's icon and its text; no style metric reports it
constexpr int scmQtToolButtonPictureGap = 4;

// Tighter than the bar's own spacing, which parts one control from the next
constexpr int scmNotepadReadingGap = 5;

constexpr int scmNotepadReadingDotDiameter = 8;

constexpr int scmNotepadProgressHeight = 4;
constexpr int scmNotepadProgressWidth = 90;

// No pseudo-state expresses the warning, so the hairline rule selects on this property
constexpr char scmProp_notepadFieldState[] = "notepadFieldState";

constexpr int scmNotepadFieldCueSize = 20;

// slot_sendNextLine() skips an empty line, so counting it would promise more than the game will see
static int sendableLineCount(const QStringList& lines)
{
    int count = 0;
    for (const QString& line : lines) {
        if (!line.isEmpty()) {
            ++count;
        }
    }
    return count;
}

// QTextCursor::selectedText() separates lines with U+2029 rather than newlines
static QStringList linesOf(QString text)
{
    return text.replace(QChar::ParagraphSeparator, QChar::LineFeed).split(QChar::LineFeed);
}

dlgNotepad::dlgNotepad(Host* pH)
: mpHost(pH)
{
    setupUi(this);

    // Never written to, and an empty one leaves a strip of native drawing under the styled toolbar
    statusbar->hide();

    setupSendStrip();
    setupAddTabButton();

    // Before restore(), so every tab's close button is caught as it is made
    uiDesign::prepareTabStrip(tabWidget->tabBar());

    tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabWidget->tabBar(), &QWidget::customContextMenuRequested, this, &dlgNotepad::slot_tabContextMenu);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &dlgNotepad::slot_tabCloseRequested);

    connect(action_stop, &QAction::triggered, this, &dlgNotepad::slot_stopSending);
    connect(action_sendAll, &QAction::triggered, this, &dlgNotepad::slot_sendAll);
    connect(action_sendLine, &QAction::triggered, this, &dlgNotepad::slot_sendLine);
    connect(action_sendSelection, &QAction::triggered, this, &dlgNotepad::slot_sendSelection);
    connect(lineEdit_prependText, &QLineEdit::textChanged, this, &dlgNotepad::updateNoPrefixWarning);

    if (mpHost) {
        restore();
        restoreSettings();
        connect(mpHost, &Host::signal_profileStyleSheetChanged, this, &dlgNotepad::setStyleSheet);
        connect(mpHost, &Host::signal_profileStyleSheetChanged, this, &dlgNotepad::setTabsStyleSheet);
        connect(mpHost, &Host::signal_consoleFontChanged, this, &dlgNotepad::setFont);
    }

    setupFindBar();
    connect(tabWidget, &QTabWidget::currentChanged, this, &dlgNotepad::slot_currentTabChanged);

    updateSendStrip();
    applyNotepadShellStyle();
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgNotepad::slot_applyAppearance);

    startTimer(2min);
}

// QToolBar makes its own QToolButton for each action, so the text-beside-icon style and
// the accessible name go on that button rather than on the action
void dlgNotepad::setupSendStrip()
{
    //: Button at the foot of the notepad that sends the line the cursor is on to the game
    action_sendLine->setText(tr("Send line"));
    //: Tooltip of the notepad's Send line button
    action_sendLine->setToolTip(tr("Send the line the cursor is on to the game, with the prefix in front of it."));
    //: Tooltip of the notepad's Send selection button
    action_sendSelection->setToolTip(tr("Send the selected lines one at a time, the prefix in front of each."));
    //: Tooltip of the notepad's Send all button
    action_sendAll->setToolTip(tr("Send every line of this note one at a time, the prefix in front of each."));

    //: Button at the foot of the notepad that stops a send that is running
    action_stop = new QAction(tr("Stop"), this);
    //: Tooltip of the notepad's Stop button
    action_stop->setToolTip(tr("Stop sending. Lines already sent stay sent."));

    mNotepadActionGlyphs = {{action_sendLine, qsl(":/icons/notepad-send-line.svg"), QString()},
                            {action_sendSelection, qsl(":/icons/notepad-send-selection.svg"), QString()},
                            {action_sendAll, qsl(":/icons/notepad-send-all.svg"), QString()},
                            {action_stop, qsl(":/icons/notepad-stop.svg"), QString()}};

    for (QAction* pAction : toolBar->actions()) {
        if (pAction->isSeparator()) {
            action_sendControlsSeparator = pAction;
            break;
        }
    }

    //: Word leading the field in the notepad that says what is put in front of every line sent
    label_prependText = new QLabel(tr("Before each line"), this);
    label_prependText->setObjectName(qsl("notepadPrefixLabel"));
    action_prependTextLabel = toolBar->addWidget(label_prependText);

    lineEdit_prependText = new QLineEdit(this);
    lineEdit_prependText->setObjectName(qsl("notepadPrefixField"));
    //: Example of what to type into the notepad's prefix field - a game command that takes the rest of the line
    lineEdit_prependText->setPlaceholderText(tr("e.g. say"));
    //: Accessible name of the notepad's prefix field, announced by a screen reader
    lineEdit_prependText->setAccessibleName(tr("Text to put before each line"));
    //: Tooltip of the notepad's prefix field. "say" and "tell" are game commands and are examples rather than words to translate.
    mPrefixFieldToolTip = tr("Put this in front of every line sent, for example say or tell Morquin.");
    lineEdit_prependText->setToolTip(mPrefixFieldToolTip);
    lineEdit_prependText->setClearButtonEnabled(true);

    // Inside the field rather than on the bar, so the cue takes no room and shows at any width
    action_prefixWarning = new QAction(this);
    action_prefixWarning->setObjectName(qsl("notepadPrefixWarning"));
    action_prefixWarning->setVisible(false);
    lineEdit_prependText->addAction(action_prefixWarning, QLineEdit::TrailingPosition);
    // QLineEdit's default Expanding policy would take the room the warning needs; the spacer takes the slack
    lineEdit_prependText->setSizePolicy(QSizePolicy::Preferred, lineEdit_prependText->sizePolicy().verticalPolicy());
    action_prependText = toolBar->addWidget(lineEdit_prependText);

    mpLabel_sendingDot = new QLabel(this);
    mpLabel_sendingDot->setObjectName(qsl("notepadSendingDot"));
    mpLabel_sendingDot->setFixedSize(scmNotepadReadingDotDiameter, scmNotepadReadingDotDiameter);
    action_sendingDot = toolBar->addWidget(mpLabel_sendingDot);

    mpLabel_sendingText = new QLabel(this);
    mpLabel_sendingText->setObjectName(qsl("notepadSendingText"));
    action_sendingText = toolBar->addWidget(mpLabel_sendingText);

    mpProgressBar_sending = new QProgressBar(this);
    mpProgressBar_sending->setObjectName(qsl("notepadSendingBar"));
    // The label beside it already gives the count
    mpProgressBar_sending->setTextVisible(false);
    mpProgressBar_sending->setFixedHeight(scmNotepadProgressHeight);
    mpProgressBar_sending->setFixedWidth(scmNotepadProgressWidth);
    action_sendingBar = toolBar->addWidget(mpProgressBar_sending);

    toolBar->addAction(action_stop);

    auto* pSpacer = new QWidget(this);
    pSpacer->setObjectName(qsl("notepadStripSpacer"));
    pSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    uiDesign::markAsShellSurface(pSpacer);
    toolBar->addWidget(pSpacer);

    mpWidget_noPrefixNote = new QWidget(this);
    mpWidget_noPrefixNote->setObjectName(qsl("notepadNoPrefixNote"));
    uiDesign::markAsShellSurface(mpWidget_noPrefixNote);
    auto* pNoteLayout = new QHBoxLayout(mpWidget_noPrefixNote);
    pNoteLayout->setContentsMargins(0, 0, 0, 0);
    pNoteLayout->setSpacing(scmNotepadReadingGap);
    mpLabel_noPrefixDot = new QLabel(mpWidget_noPrefixNote);
    mpLabel_noPrefixDot->setObjectName(qsl("notepadNoPrefixDot"));
    mpLabel_noPrefixDot->setFixedSize(scmNotepadReadingDotDiameter, scmNotepadReadingDotDiameter);
    pNoteLayout->addWidget(mpLabel_noPrefixDot);
    mpLabel_noPrefixText = new QLabel(mpWidget_noPrefixNote);
    mpLabel_noPrefixText->setObjectName(qsl("notepadNoPrefixText"));
    pNoteLayout->addWidget(mpLabel_noPrefixText);
    action_noPrefixNote = toolBar->addWidget(mpWidget_noPrefixNote);
    toolBar->installEventFilter(this);

    // Words beside the glyphs: the glyphs alone do not tell the sends apart
    const int glyphSize = qRound(toolBar->fontMetrics().height() * 0.9);
    toolBar->setIconSize(QSize(glyphSize, glyphSize));
    const QList<QPair<QAction*, QString>> announced{//: Accessible name of the notepad's Send line button, announced by a screen reader
                                                    {action_sendLine, tr("Send the current line")},
                                                    //: Accessible name of the notepad's Send selection button, announced by a screen reader
                                                    {action_sendSelection, tr("Send the selected lines")},
                                                    //: Accessible name of the notepad's Send all button, announced by a screen reader
                                                    {action_sendAll, tr("Send all lines")},
                                                    //: Accessible name of the notepad's Stop button, announced by a screen reader
                                                    {action_stop, tr("Stop sending")}};
    for (const auto& button : announced) {
        if (auto* pButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(button.first))) {
            pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            pButton->setAccessibleName(button.second);
        }
    }

    setSending(false);
}

// The platform's tool button font, smaller than a label's on macOS. Read off the application, not a
// button, which only takes it when polished on first show. Applied through the bar's sheet, since a
// stylesheet polish re-resolves the font from the parent and drops one set directly.
// Not a uiDesign::typeSize() step: that scale follows the application font, not the tool button font
int dlgNotepad::stripWordPointSize() const
{
    const qreal buttonSize = QApplication::font("QToolButton").pointSizeF();
    return std::max(1, qRound(buttonSize > 0.0 ? buttonSize : toolBar->font().pointSizeF()));
}

void dlgNotepad::setupAddTabButton()
{
    mpAddTabButton = new QToolButton(this);
    mpAddTabButton->setObjectName(qsl("notepadAddTab"));
    mpAddTabButton->setToolTip(tr("Add new note tab (Ctrl+T)"));
    mpAddTabButton->setAutoRaise(false);
    connect(mpAddTabButton, &QToolButton::clicked, this, &dlgNotepad::slot_addTabClicked);
    tabWidget->setCornerWidget(mpAddTabButton, Qt::TopRightCorner);

    auto* newTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(newTabShortcut, &QShortcut::activated, this, &dlgNotepad::slot_addTabClicked);
}

// closeTab() keeps the last note, so a sole tab carries no cross. Toggling the whole bar rather
// than one tab's button also gives the tab its width back.
void dlgNotepad::updateTabClosability()
{
    tabWidget->setTabsClosable(tabWidget->count() > 1);
}

void dlgNotepad::setupFindBar()
{
    mpFindBar = new QWidget(this);
    mpFindBar->setObjectName(qsl("notepadFindBar"));
    auto* layout = new QHBoxLayout(mpFindBar);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    mpFindLineEdit = new QLineEdit(mpFindBar);
    mpFindLineEdit->setObjectName(qsl("notepadFindField"));
    //: Placeholder text for the search field in notepad
    mpFindLineEdit->setPlaceholderText(tr("Find"));
    mpFindLineEdit->setClearButtonEnabled(true);
    mpFindLineEdit->installEventFilter(this);

    mpFindPrevButton = new QToolButton(mpFindBar);
    mpFindPrevButton->setObjectName(qsl("notepadFindPrevious"));
    mpFindPrevButton->setToolTip(tr("Find previous"));
    mpFindPrevButton->setAutoRaise(false);

    mpFindNextButton = new QToolButton(mpFindBar);
    mpFindNextButton->setObjectName(qsl("notepadFindNext"));
    mpFindNextButton->setToolTip(tr("Find next"));
    mpFindNextButton->setAutoRaise(false);

    mpFindCloseButton = new QToolButton(mpFindBar);
    mpFindCloseButton->setObjectName(qsl("notepadFindClose"));
    mpFindCloseButton->setToolTip(tr("Close find bar"));
    mpFindCloseButton->setAutoRaise(false);

    layout->addWidget(mpFindLineEdit, 1);
    layout->addWidget(mpFindPrevButton);
    layout->addWidget(mpFindNextButton);
    layout->addWidget(mpFindCloseButton);

    verticalLayout->addWidget(mpFindBar);
    mpFindBar->hide();

    connect(mpFindLineEdit, &QLineEdit::textChanged, this, &dlgNotepad::slot_findTextChanged);
    connect(mpFindPrevButton, &QToolButton::clicked, this, &dlgNotepad::slot_findPrevious);
    connect(mpFindNextButton, &QToolButton::clicked, this, &dlgNotepad::slot_findNext);
    connect(mpFindCloseButton, &QToolButton::clicked, this, &dlgNotepad::slot_hideFindBar);

    mpFindShortcut = new QShortcut(QKeySequence::Find, this);
    connect(mpFindShortcut, &QShortcut::activated, this, &dlgNotepad::slot_showFindBar);
}

void dlgNotepad::slot_addTabClicked()
{
    addTab();
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void dlgNotepad::setFont(const QFont& font)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            textEdit->setFont(font);
        }
    }
}

void dlgNotepad::setTabsStyleSheet(const QString& styleSheet)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            textEdit->setStyleSheet(styleSheet);
        }
    }
}

void dlgNotepad::slot_applyAppearance()
{
    applyNotepadShellStyle();
}

// Sheets go on widgets, not the window: a profile's Lua stylesheet replaces the window's on every show
void dlgNotepad::applyNotepadShellStyle()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

    const QString centralRules =
            qsl("QWidget#centralwidget { background-color: %1; }").arg(tokens.page.name()) + uiDesign::tabBarStyleSheet(qsl("#tabWidget"), tokens)
            + uiDesign::inputStyleSheet(tokens, qsl("#centralwidget"))
            + qsl("QWidget#notepadFindBar { background-color: %1; border-top: %2px solid %3; }").arg(tokens.page.name(), QString::number(uiDesign::scmInputBorderWidth), tokens.border.name())
            // Borderless: a frame round a glyph this small reads as a second control
            + qsl("QToolButton#notepadAddTab, QWidget#notepadFindBar QToolButton"
                  " { border: none; border-radius: %1px; background: transparent; padding: 2px; }"
                  "QToolButton#notepadAddTab:hover, QWidget#notepadFindBar QToolButton:hover { background-color: %2; }")
                      .arg(QString::number(uiDesign::scmRadiusChip), tokens.hoverSoft);
    centralwidget->setStyleSheet(centralRules);

    const QColor warningColour = uiDesign::stateColor(uiDesign::scmStateHue_warning, tokens.darkPage);
    const QColor warningInk = uiDesign::readableOn(tokens.page, warningColour, tokens.text, uiDesign::scmTextMinimumRatio);

    const QString toolBarRules = uiDesign::toolBarStyleSheet(qsl("QToolBar#toolBar"), uiDesign::ToolBarSeam::Top, tokens) + uiDesign::inputStyleSheet(tokens, qsl("QToolBar#toolBar"))
                                 + qsl("QToolBar#toolBar QLabel { color: %1; }").arg(tokens.mutedText.name())
                                 + qsl("QToolBar#toolBar QLabel, QToolBar#toolBar QLineEdit { font-size: %1pt; }").arg(QString::number(stripWordPointSize()))
                                 // After the shared rules, in the border shorthand they write, so it wins the
                                 // specificity tie against QLineEdit:focus
                                 + qsl("QToolBar#toolBar QLineEdit[%1=\"warning\"] { border: %2px solid %3; }")
                                           .arg(QString::fromLatin1(scmProp_notepadFieldState), QString::number(uiDesign::scmInputBorderWidth), warningColour.name())
                                 // Named down to the label, or the QLabel rule above wins on specificity
                                 + qsl("QToolBar#toolBar QLabel#notepadSendingText { color: %1; padding: 0px %2px; }").arg(tokens.accentText.name(), QString::number(scmNotepadReadingGap))
                                 + qsl("QToolBar#toolBar QLabel#notepadNoPrefixText { color: %1; }").arg(warningInk.name())
                                 + qsl("#notepadSendingBar { border: none; background-color: %1; border-radius: %3px; }"
                                       "#notepadSendingBar::chunk { background-color: %2; border-radius: %3px; }")
                                           .arg(tokens.accentSoft, tokens.accent.name(), QString::number(scmNotepadProgressHeight / 2));
    toolBar->setStyleSheet(toolBarRules);
    uiDesign::ToolBarOverflow::restyle(toolBar, tokens);
    alignPrefixLeadWord();

    mpLabel_sendingDot->setStyleSheet(qsl("#notepadSendingDot { background-color: %1; border-radius: %2px; }").arg(tokens.accent.name(), QString::number(scmNotepadReadingDotDiameter / 2)));
    mpLabel_noPrefixDot->setStyleSheet(qsl("#notepadNoPrefixDot { background-color: %1; border-radius: %2px; }").arg(warningColour.name(), QString::number(scmNotepadReadingDotDiameter / 2)));

    if (action_prefixWarning) {
        const qreal glyphRatio = lineEdit_prependText->devicePixelRatioF();
        QPixmap cue = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-notice-warning.svg")), warningColour)
                              .scaled(QSize(scmNotepadFieldCueSize, scmNotepadFieldCueSize) * glyphRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        cue.setDevicePixelRatio(glyphRatio);
        action_prefixWarning->setIcon(QIcon(cue));
    }

    uiDesign::restyleToolBarGlyphs(mNotepadActionGlyphs, tokens);
    mpAddTabButton->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-add.svg"), tokens));
    mpFindPrevButton->setIcon(uiDesign::tintedIcon(qsl(":/icons/notepad-chevron-up.svg"), tokens));
    mpFindNextButton->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-chevron-down.svg"), tokens));
    mpFindCloseButton->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));

    uiDesign::keepClickFocusOffControls(centralwidget);
    uiDesign::keepClickFocusOffControls(toolBar);
    uiDesign::letPopupsTakeTheFieldsCorner(centralwidget);
    uiDesign::letPopupsTakeTheFieldsCorner(toolBar);

    mCurrentMatchInk = tokens.marker;
    mOtherMatchInk = uiDesign::blend(tokens.field, tokens.marker, scmNotepadQuietMatchWash);
    if (mpFindBar && mpFindBar->isVisible()) {
        highlightAllMatches();
    }

    // A theme change can move the font, and with it the room the warning's words need
    fitNoPrefixNote();
}

dlgNotepad::~dlgNotepad()
{
    if (auto* pDialogs = mpHost ? HostDialogs::find(mpHost) : nullptr; pDialogs && pDialogs->mpNotePad) {
        save();
        pDialogs->mpNotePad = nullptr;
    }
}

int dlgNotepad::addTab(const QString& name, const QString& content)
{
    // The profile's sheet, set on the note itself, beats the central widget's rules where they collide
    auto* textEdit = new QPlainTextEdit(this);
    if (mpHost) {
        textEdit->setFont(mpHost->getDisplayFont());
        textEdit->setStyleSheet(mpHost->mProfileStyleSheet);
    }
    if (!content.isEmpty()) {
        textEdit->setPlainText(content);
    }

    connect(textEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::slot_textChanged);
    // Only the matches on screen are marked, so they are marked afresh whenever what is on screen changes
    auto rehighlightIfShown = [this, textEdit]() {
        if (textEdit == currentTextEdit()) {
            scheduleRehighlight();
        }
    };
    connect(textEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, rehighlightIfShown);
    connect(textEdit->verticalScrollBar(), &QScrollBar::rangeChanged, this, rehighlightIfShown);
    textEdit->viewport()->installEventFilter(this);
    connect(textEdit, &QPlainTextEdit::textChanged, this, &dlgNotepad::updateSendStrip);
    connect(textEdit, &QPlainTextEdit::selectionChanged, this, &dlgNotepad::updateSelectionReach);

    //: Default name for a new note tab
    const QString tabName = name.isEmpty() ? tr("New Note") : name;
    const int index = tabWidget->addTab(textEdit, tabName);
    updateTabClosability();
    return index;
}

// save() keeps no copy of a closed note, so one with text is asked about first
bool dlgNotepad::okToDiscardNotes(const QList<int>& indices)
{
    int withText = 0;
    for (const int index : indices) {
        if (index < 0 || index >= tabWidget->count()) {
            continue;
        }
        auto* pNote = qobject_cast<QPlainTextEdit*>(tabWidget->widget(index));
        if (pNote && !pNote->toPlainText().trimmed().isEmpty()) {
            ++withText;
        }
    }

    if (!withText) {
        return true;
    }

    QString ask;
    QString consequence;
    QString destroy;
    if (indices.size() == 1) {
        //: Dialog asking the reader to confirm closing a note that has text in it. %1 is the note's name.
        ask = tr("Close the note \"%1\"?").arg(tabWidget->tabText(indices.constFirst()));
        //: Body of the dialog asking the reader to confirm closing a note that has text in it
        consequence = tr("Its text will be lost. Mudlet keeps no copy of a closed note.");
        //: Button on that dialog which goes ahead and closes the one note it asked about
        destroy = tr("Close Note");
    } else {
        //: Dialog asking the reader to confirm closing every note but the one they picked
        ask = tr("Close the other notes?");
        //: Body of that dialog. %n is how many of the notes being closed have text in them.
        consequence = tr("Text in %n of them will be lost.", "", withText);
        //: Button on that dialog which goes ahead and closes every note but the one the reader picked
        destroy = tr("Close Notes");
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    // A sheet on this window on macOS rather than a box over the whole application
    box.setWindowModality(Qt::WindowModal);
    box.setText(ask);
    box.setInformativeText(consequence);
    box.addButton(destroy, QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);
    box.exec();

    return box.buttonRole(box.clickedButton()) == QMessageBox::DestructiveRole;
}

void dlgNotepad::closeTab(int index)
{
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }

    if (tabWidget->count() <= 1) {
        return;
    }

    if (!okToDiscardNotes({index})) {
        return;
    }

    save();

    QWidget* widget = tabWidget->widget(index);
    tabWidget->removeTab(index);
    delete widget;
    updateTabClosability();

    mNeedToSave = true;
}

// Removed from the back, since removing a tab renumbers the ones after it
void dlgNotepad::closeOtherTabs(int keptIndex)
{
    if (keptIndex < 0 || keptIndex >= tabWidget->count() || tabWidget->count() <= 1) {
        return;
    }

    QList<int> others;
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (i != keptIndex) {
            others.append(i);
        }
    }

    if (!okToDiscardNotes(others)) {
        return;
    }

    save();

    for (int i = tabWidget->count() - 1; i >= 0; --i) {
        if (i != keptIndex) {
            QWidget* widget = tabWidget->widget(i);
            tabWidget->removeTab(i);
            delete widget;
        }
    }
    updateTabClosability();

    mNeedToSave = true;
}

void dlgNotepad::renameTab(int index)
{
    if (index < 0 || index >= tabWidget->count()) {
        return;
    }

    const QString currentName = tabWidget->tabText(index);
    bool ok = false;
    //: Dialog title for renaming a note tab
    const QString newName = QInputDialog::getText(this,
                                                  tr("Rename Note Tab"),
                                                  //: Label for the input field when renaming a note tab
                                                  tr("New name:"),
                                                  QLineEdit::Normal,
                                                  currentName,
                                                  &ok);
    if (ok && !newName.isEmpty() && newName != currentName) {
        tabWidget->setTabText(index, newName);
        mNeedToSave = true;
    }
}

void dlgNotepad::slot_tabCloseRequested(int index)
{
    closeTab(index);
}

void dlgNotepad::slot_tabContextMenu(const QPoint& pos)
{
    const int tabIndex = tabWidget->tabBar()->tabAt(pos);

    QMenu menu(this);
    menu.setStyleSheet(uiDesign::menuStyleSheet(uiDesign::themeTokens()));
    uiDesign::letPopupsTakeTheFieldsCorner(&menu);

    //: Context menu action to create a new note tab
    QAction* newTabAction = menu.addAction(tr("New Tab"));
    connect(newTabAction, &QAction::triggered, this, [this]() {
        addTab();
        tabWidget->setCurrentIndex(tabWidget->count() - 1);
    });

    if (tabIndex >= 0) {
        menu.addSeparator();

        //: Context menu action to rename a note tab
        QAction* renameAction = menu.addAction(tr("Rename Tab"));
        connect(renameAction, &QAction::triggered, this, [this, tabIndex]() {
            renameTab(tabIndex);
        });

        if (tabWidget->count() > 1) {
            //: Context menu action to close a note tab
            QAction* closeAction = menu.addAction(tr("Close Tab"));
            connect(closeAction, &QAction::triggered, this, [this, tabIndex]() {
                closeTab(tabIndex);
            });

            menu.addSeparator();

            //: Context menu action to close all note tabs except the clicked one
            QAction* closeOthersAction = menu.addAction(tr("Close Other Tabs"));
            connect(closeOthersAction, &QAction::triggered, this, [this, tabIndex]() {
                closeOtherTabs(tabIndex);
            });
        }
    }

    menu.exec(tabWidget->tabBar()->mapToGlobal(pos));
}

QPlainTextEdit* dlgNotepad::currentTextEdit() const
{
    return qobject_cast<QPlainTextEdit*>(tabWidget->currentWidget());
}

void dlgNotepad::save()
{
    const QString directoryPath = MudletApp::getMudletPath(enums::profileHomePath, mpHost->getName());
    const QString fileName = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonNotesFileName);

    const QDir dir;
    if (!dir.exists(directoryPath)) {
        dir.mkpath(directoryPath);
    }

    QJsonObject root;
    root.insert(qsl("version"), 1);

    QJsonArray tabsArray;
    for (int i = 0; i < tabWidget->count(); ++i) {
        QJsonObject tabObj;
        tabObj.insert(qsl("name"), tabWidget->tabText(i));

        if (auto* textEdit = qobject_cast<QPlainTextEdit*>(tabWidget->widget(i))) {
            tabObj.insert(qsl("content"), textEdit->toPlainText());
        }

        tabsArray.append(tabObj);
    }

    root.insert(qsl("tabs"), tabsArray);
    root.insert(qsl("activeTab"), tabWidget->currentIndex());

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "dlgNotepad::save: failed to open file for writing:" << file.errorString();
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qDebug() << "dlgNotepad::save: error saving notepad contents:" << file.errorString();
    }

    mNeedToSave = false;
}

bool dlgNotepad::migrateOldNotesFile()
{
    QString oldFileName = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), utf8EncodedNotesFileName);
    bool useUtf8 = true;

    if (!QFile::exists(oldFileName)) {
        oldFileName = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), local8BitEncodedNotesFileName);
        useUtf8 = false;

        if (!QFile::exists(oldFileName)) {
            return false;
        }
    }

    QFile file(oldFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "dlgNotepad::migrateOldNotesFile: failed to open file for reading:" << file.errorString();
        return false;
    }

    QTextStream fileStream(&file);
    if (!useUtf8) {
        fileStream.setEncoding(QStringEncoder::Encoding::System);
    }

    const QString content = fileStream.readAll();
    file.close();

    //: Name for the migrated notes tab when upgrading from single-note to tabbed notepad
    addTab(tr("Notes"), content);

    return true;
}

void dlgNotepad::restore()
{
    const QString fileName = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), jsonNotesFileName);

    if (QFile::exists(fileName)) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "dlgNotepad::restore: failed to open file for reading:" << file.errorString();
            //: Default name for the first note tab
            addTab(tr("Notes"));
            return;
        }

        const QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "dlgNotepad::restore: JSON parse error:" << parseError.errorString();
            addTab(tr("Notes"));
            return;
        }

        if (!doc.isObject()) {
            qDebug() << "dlgNotepad::restore: JSON root is not an object";
            addTab(tr("Notes"));
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonArray tabsArray = root.value(qsl("tabs")).toArray();

        if (tabsArray.isEmpty()) {
            addTab(tr("Notes"));
        } else {
            for (const QJsonValue& tabValue : tabsArray) {
                const QJsonObject tabObj = tabValue.toObject();
                const QString name = tabObj.value(qsl("name")).toString();
                const QString content = tabObj.value(qsl("content")).toString();
                addTab(name, content);
            }

            const int activeTab = root.value(qsl("activeTab")).toInt(0);
            if (activeTab >= 0 && activeTab < tabWidget->count()) {
                tabWidget->setCurrentIndex(activeTab);
            }
        }
    } else if (!migrateOldNotesFile()) {
        addTab(tr("Notes"));
    }
}

void dlgNotepad::slot_textChanged()
{
    mNeedToSave = true;
}

void dlgNotepad::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    if (!mNeedToSave) {
        return;
    }

    save();
}

void dlgNotepad::slot_sendAll()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    const QString allText = textEdit->toPlainText();
    const QStringList lines = allText.split('\n');
    startSendingLines(lines);
}

void dlgNotepad::slot_sendLine()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QTextCursor cursor = textEdit->textCursor();
    cursor.select(QTextCursor::LineUnderCursor);
    const QString line = cursor.selectedText();

    if (!line.isEmpty()) {
        startSendingLines(QStringList{line});
    }
}

void dlgNotepad::slot_sendSelection()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QString selectedText = textEdit->textCursor().selectedText();

    if (!selectedText.isEmpty()) {
        const QStringList lines = selectedText.replace(QChar(0x2029), qsl("\n")).split('\n');
        startSendingLines(lines);
    }
}

void dlgNotepad::startSendingLines(const QStringList& lines)
{
    mLinesToSend = lines;
    mCurrentLineIndex = 0;
    mLinesSent = 0;
    mLinesToSendCount = sendableLineCount(lines);

    if (!mSendTimer) {
        mSendTimer = new QTimer(this);
        connect(mSendTimer, &QTimer::timeout, this, &dlgNotepad::slot_sendNextLine);
    }

    setSending(true);
    updateSendingReading();
    //: Announced when the notepad starts sending a note to the game. %n is how many lines will go.
    mudlet::self()->announce(mLinesToSendCount == 1 ? tr("Sending 1 line") : tr("Sending %n lines", "", mLinesToSendCount));
    mSendTimer->start(300ms);
}

void dlgNotepad::slot_sendNextLine()
{
    if (mCurrentLineIndex >= mLinesToSend.size()) {
        finishSending(false);
        return;
    }

    const QString line = mLinesToSend[mCurrentLineIndex++];
    if (!line.isEmpty() && mpHost) {
        mpHost->send(lineEdit_prependText->text() + line);
        ++mLinesSent;
        updateSendingReading();
    }

    if (mCurrentLineIndex >= mLinesToSend.size()) {
        finishSending(false);
    }
}

void dlgNotepad::slot_stopSending()
{
    finishSending(true);
}

void dlgNotepad::updateSendingReading()
{
    if (!mpLabel_sendingText || !mpProgressBar_sending) {
        return;
    }
    //: Reading on the notepad's send strip while lines are going to the game. %1 is how many have gone, %2 how many there are in all.
    mpLabel_sendingText->setText(tr("Sending %1 of %2").arg(QString::number(mLinesSent), QString::number(mLinesToSendCount)));
    // A zero range makes QProgressBar draw its busy indicator
    mpProgressBar_sending->setMaximum(qMax(1, mLinesToSendCount));
    mpProgressBar_sending->setValue(mLinesSent);
}

void dlgNotepad::finishSending(const bool stopped)
{
    if (mSendTimer) {
        mSendTimer->stop();
    }

    if (mSending) {
        if (stopped) {
            //: Announced when a send from the notepad was stopped part way. %1 is how many lines went before it stopped, %2 how many there were in all.
            mudlet::self()->announce(tr("Stopped after %1 of %2 lines").arg(QString::number(mLinesSent), QString::number(mLinesToSendCount)));
        } else {
            //: Announced when the notepad has sent a note to the game. %n is how many lines went.
            mudlet::self()->announce(mLinesSent == 1 ? tr("Sent 1 line") : tr("Sent %n lines", "", mLinesSent));
        }
    }

    mLinesToSend.clear();
    mCurrentLineIndex = 0;
    setSending(false);
}

// Stop is hidden rather than disabled: while idle there is nothing to stop
void dlgNotepad::setSending(const bool sending)
{
    mSending = sending;

    action_sendLine->setVisible(!sending);
    action_sendSelection->setVisible(!sending);
    action_sendAll->setVisible(!sending);
    if (action_sendControlsSeparator) {
        action_sendControlsSeparator->setVisible(!sending);
    }
    if (action_prependTextLabel) {
        action_prependTextLabel->setVisible(!sending);
    }
    if (action_prependText) {
        action_prependText->setVisible(!sending);
    }

    if (action_sendingDot) {
        action_sendingDot->setVisible(sending);
    }
    if (action_sendingText) {
        action_sendingText->setVisible(sending);
    }
    if (action_sendingBar) {
        action_sendingBar->setVisible(sending);
    }
    if (action_stop) {
        action_stop->setVisible(sending);
    }

    updateNoPrefixWarning();
}

void dlgNotepad::updateSendStrip()
{
    auto* pTextEdit = currentTextEdit();
    mNoteSendableLines = pTextEdit ? sendableLineCount(linesOf(pTextEdit->toPlainText())) : 0;
    updateSendReach();
    updateNoPrefixWarning();
}

void dlgNotepad::updateSendReach()
{
    const int inTheNote = mNoteSendableLines;

    // Singular as its own string: with no catalogue loaded Qt applies no plural form and shows "1 lines"
    //: Button at the foot of the notepad. %n is how many lines of the note would go to the game, one at a time.
    action_sendAll->setText(inTheNote == 1 ? tr("Send all (1 line)") : tr("Send all (%n lines)", "", inTheNote));
    action_sendAll->setEnabled(inTheNote > 0);

    updateSelectionReach();
    alignPrefixLeadWord();
}

void dlgNotepad::updateSelectionReach()
{
    auto* pTextEdit = currentTextEdit();
    const int inTheSelection = pTextEdit ? sendableLineCount(linesOf(pTextEdit->textCursor().selectedText())) : 0;
    if (inTheSelection > 0) {
        //: Button at the foot of the notepad. %n is how many of the note's lines are selected.
        action_sendSelection->setText(inTheSelection == 1 ? tr("Send selection (1 line)") : tr("Send selection (%n lines)", "", inTheSelection));
    } else {
        //: Button at the foot of the notepad, with nothing selected to say a count of
        action_sendSelection->setText(tr("Send selection"));
    }
    action_sendSelection->setEnabled(inTheSelection > 0);
}

// A tool button's sizeHint() slack lands after its word, on the seam's side, while a label's word sits at its edge
void dlgNotepad::alignPrefixLeadWord()
{
    const auto* pSendAll = qobject_cast<const QToolButton*>(toolBar->widgetForAction(action_sendAll));
    if (!pSendAll || !label_prependText) {
        return;
    }
    const int frame = uiDesign::scmInputBorderWidth + uiDesign::scmToolBarButtonPaddingHorizontal;
    const int picture = pSendAll->iconSize().width() + scmQtToolButtonPictureGap;
    const int word = pSendAll->fontMetrics().horizontalAdvance(pSendAll->text());
    const int afterTheWord = std::max(0, pSendAll->sizeHint().width() - 2 * frame - picture - word);
    const QMargins held(uiDesign::scmToolBarButtonPaddingHorizontal + afterTheWord, 0, uiDesign::scmToolBarButtonPaddingHorizontal, 0);
    if (label_prependText->contentsMargins() == held) {
        return;
    }
    label_prependText->setContentsMargins(held);
    // The margin takes room from the warning, which was fitted to the old width
    fitNoPrefixNote();
}

void dlgNotepad::updateNoPrefixWarning()
{
    if (!mpWidget_noPrefixNote || !action_noPrefixNote) {
        return;
    }

    const int inTheNote = mNoteSendableLines;
    const bool warn = !mSending && lineEdit_prependText->text().isEmpty() && inTheNote > scmNotepadWarnAtLines;

    mNoPrefixWarningApplies = warn;
    if (warn) {
        //: Warning at the foot of the notepad, shown when a long note would be sent with nothing in front of its lines. %n is how many lines the note holds.
        mNoPrefixNoteText = tr("No prefix, so %n lines go to the game as commands", "", inTheNote);
        mpWidget_noPrefixNote->setToolTip(mNoPrefixNoteText);
        fitNoPrefixNote();
    } else {
        action_noPrefixNote->setVisible(false);
    }

    if (action_prefixWarning) {
        action_prefixWarning->setToolTip(warn ? mNoPrefixNoteText : QString());
        action_prefixWarning->setVisible(warn);
    }
    lineEdit_prependText->setToolTip(warn ? mNoPrefixNoteText : mPrefixFieldToolTip);

    const QString state = warn ? qsl("warning") : QString();
    if (lineEdit_prependText->property(scmProp_notepadFieldState).toString() != state) {
        lineEdit_prependText->setProperty(scmProp_notepadFieldState, state.isEmpty() ? QVariant() : QVariant(state));
        uiDesign::repolish(lineEdit_prependText);
    }
}

// QToolBar moves what does not fit into its extension menu, so the words come whole or not at all
void dlgNotepad::fitNoPrefixNote()
{
    if (!mpLabel_noPrefixText || !action_noPrefixNote || !mNoPrefixWarningApplies || mNoPrefixNoteText.isEmpty()) {
        return;
    }
    // QToolBar leaves what it hid out of its sizeHint(), so the reading is shown before measuring
    action_noPrefixNote->setVisible(true);
    mpWidget_noPrefixNote->show();
    mpLabel_noPrefixText->clear();
    // Layouts cache their size hints until invalidated
    uiDesign::invalidateLayoutsUpTo(mpLabel_noPrefixText, toolBar);
    const int roomForTheWords = toolBar->width() - toolBar->sizeHint().width();

    //: Short form of the notepad's no-prefix warning, on the strip where there is no room for the whole sentence, which stays on the tooltip
    const QStringList forms{mNoPrefixNoteText, tr("No prefix")};
    for (const QString& words : forms) {
        mpLabel_noPrefixText->setText(words);
        if (mpLabel_noPrefixText->sizeHint().width() <= roomForTheWords) {
            return;
        }
    }

    // The action, not the widget: QToolBar shows a widget it has room for again on its next layout
    mpLabel_noPrefixText->clear();
    action_noPrefixNote->setVisible(false);
}

void dlgNotepad::saveSettings()
{
    if (!mpHost) {
        return;
    }

    mpHost->writeProfileIniData(qsl("Notepad/WindowState"), QString::fromLatin1(saveState().toBase64()));
    // A key older versions wrote, which nothing reads
    mpHost->removeProfileIniData(qsl("Notepad/SendControlsVisible"));
}

void dlgNotepad::restoreSettings()
{
    if (!mpHost) {
        return;
    }

    const QString windowStateStr = mpHost->readProfileIniData(qsl("Notepad/WindowState"));
    if (!windowStateStr.isEmpty()) {
        restoreState(QByteArray::fromBase64(windowStateStr.toLatin1()));
    }
}

void dlgNotepad::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

// Under "follow the system" a system theme change emits no signal_appearanceChanged; the style or
// palette change Qt sends is the only notice
void dlgNotepad::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        applyNotepadShellStyle();
    }
}

bool dlgNotepad::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Resize) {
        if (auto* textEdit = currentTextEdit(); textEdit && obj == textEdit->viewport()) {
            scheduleRehighlight();
        }
    }
    if (obj == toolBar && event->type() == QEvent::Resize) {
        fitNoPrefixNote();
        return QMainWindow::eventFilter(obj, event);
    }

    if (obj == mpFindLineEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                slot_findPrevious();
            } else {
                slot_findNext();
            }
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            slot_hideFindBar();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void dlgNotepad::slot_showFindBar()
{
    mpFindBar->show();
    mpFindLineEdit->setFocus();
    mpFindLineEdit->selectAll();
    highlightAllMatches();
}

void dlgNotepad::slot_hideFindBar()
{
    mpFindBar->hide();
    clearSearchHighlights();
    if (auto* textEdit = currentTextEdit()) {
        textEdit->setFocus();
    }
}

void dlgNotepad::slot_findNext()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit || mpFindLineEdit->text().isEmpty()) {
        return;
    }

    if (!textEdit->find(mpFindLineEdit->text())) {
        textEdit->moveCursor(QTextCursor::Start);
        textEdit->find(mpFindLineEdit->text());
    }
    highlightAllMatches();
}

void dlgNotepad::slot_findPrevious()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit || mpFindLineEdit->text().isEmpty()) {
        return;
    }

    if (!textEdit->find(mpFindLineEdit->text(), QTextDocument::FindBackward)) {
        textEdit->moveCursor(QTextCursor::End);
        textEdit->find(mpFindLineEdit->text(), QTextDocument::FindBackward);
    }
    highlightAllMatches();
}

void dlgNotepad::slot_findTextChanged(const QString& text)
{
    Q_UNUSED(text)
    auto* textEdit = currentTextEdit();
    if (textEdit && !mpFindLineEdit->text().isEmpty()) {
        QTextCursor cursor = textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        textEdit->setTextCursor(cursor);
        textEdit->find(mpFindLineEdit->text());
    }
    highlightAllMatches();
}

void dlgNotepad::slot_currentTabChanged(int index)
{
    Q_UNUSED(index)
    updateSendStrip();
    if (mpFindBar && mpFindBar->isVisible()) {
        highlightAllMatches();
    }
}

void dlgNotepad::highlightAllMatches()
{
    auto* textEdit = currentTextEdit();
    if (!textEdit) {
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;
    const QString searchText = mpFindLineEdit->text();

    if (searchText.isEmpty()) {
        textEdit->setExtraSelections(extraSelections);
        return;
    }

    const QTextCursor currentCursor = textEdit->textCursor();

    // Laying out a mark costs about as much as drawing it, and a big note can hold tens of
    // thousands of matches, so only those on screen are marked
    const QRect viewportRect = textEdit->viewport()->rect();
    QTextCursor firstShown = textEdit->cursorForPosition(viewportRect.topLeft());
    firstShown.movePosition(QTextCursor::StartOfLine);
    QTextCursor lastShown = textEdit->cursorForPosition(viewportRect.bottomRight());
    lastShown.movePosition(QTextCursor::EndOfLine);

    // The matching QTextDocument::find() does, so the marks agree with where Next and Previous land,
    // but bounded by the screen where find() would run on to the next match however far away that is
    const int firstShownPosition = firstShown.position();
    const int lastShownPosition = lastShown.position();
    for (QTextBlock block = firstShown.block(); block.isValid() && block.position() <= lastShownPosition; block = block.next()) {
        QString text = block.text();
        text.replace(QChar::Nbsp, QLatin1Char(' '));
        for (qsizetype at = text.indexOf(searchText, 0, Qt::CaseInsensitive); at >= 0; at = text.indexOf(searchText, at + searchText.size(), Qt::CaseInsensitive)) {
            const int start = block.position() + at;
            const int end = start + searchText.size();
            if (start > lastShownPosition) {
                break;
            }
            if (end <= firstShownPosition) {
                continue;
            }

            QTextEdit::ExtraSelection selection;
            selection.cursor = QTextCursor(block);
            selection.cursor.setPosition(start);
            selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
            if (start == currentCursor.selectionStart() && end == currentCursor.selectionEnd()) {
                selection.format.setBackground(mCurrentMatchInk);
            } else {
                selection.format.setBackground(mOtherMatchInk);
            }

            extraSelections.append(selection);
        }
    }

    textEdit->setExtraSelections(extraSelections);
}

// Queued, as what is on screen changes while the edit is still part way through laying itself out
void dlgNotepad::scheduleRehighlight()
{
    if (mRehighlightPending) {
        return;
    }
    mRehighlightPending = true;
    QMetaObject::invokeMethod(
            this,
            [this]() {
                mRehighlightPending = false;
                if (mpFindBar && mpFindBar->isVisible()) {
                    highlightAllMatches();
                }
            },
            Qt::QueuedConnection);
}

void dlgNotepad::clearSearchHighlights()
{
    auto* textEdit = currentTextEdit();
    if (textEdit) {
        textEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    }
}
