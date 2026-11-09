/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2024, 2026 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016 by Owen Davison - odavison@cs.dal.ca               *
 *   Copyright (C) 2016-2020 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
 *   Copyright (C) 2023-2025 by Lecker Kebap - Leris@mudlet.org            *
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


#include "dlgTriggerEditor.h"

#include "EventNames.h"
#include "Host.h"
#include "LuaInterface.h"
#include "MudletApp.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "VarUnit.h"
#include "XMLimport.h"
#include "XMLexport.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgColorTrigger.h"
#include "dlgKeysMainArea.h"
#include "dlgPackageExporter.h"
#include "dlgProfilePreferences.h"
#include "dlgScriptsMainArea.h"
#include "dlgTriggerPatternEdit.h"
#include "SidebarItemDelegate.h"
#include "SingleLineTextEdit.h"
#include "TrailingWhitespaceMarker.h"
#include "ChipRow.h"
#include "FlowLayout.h"
#include "EditorAddItemCommand.h"
#include "EditorDeleteItemCommand.h"
#include "EditorItemXMLHelpers.h"
#include "EditorLineMarginDelegate.h"
#include "EditorModifyPropertyCommand.h"
#include "EditorMoveItemCommand.h"
#include "EditorPlaceholderButton.h"
#include "SidebarToggle.h"
#include "EditorToggleActiveCommand.h"
#include "EditorTreeDelegate.h"
#include "SearchResultDelegate.h"
#include "VariableTreeDelegate.h"
#include "mudlet.h"
#include "ToolBarOverflow.h"
#include "uiDesign.h"
#include "utils.h"
#include "widgetutils.h"
#include "edbee/models/textdocumentscopes.h"
#include "edbee/util/regexp.h"

#include <QApplication>
#include <QCheckBox>
#include <QAbstractButton>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QTime>
#include <QTimer>
#include <QShowEvent>
#include <QRegularExpression>
#include <QToolButton>
#include <QToolBar>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <array>
#include <chrono>
#include <sstream>
#include <utility>
#include <pugixml.hpp>
#include <QVBoxLayout>


static void setToolBarButtonLabelsShown(QToolBar* pToolBar, const QList<QAction*>& actions, const bool shown);
static void holdToolBarButtonWidth(QToolButton* pButton);

// Forward declaration for per-property undo helper (defined later in this file)
static void pushKeyPropertyCommand(EditorUndoStack* undoStack, Host* host, int keyID, const QString& keyName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML);

// Forward declaration for the tree item lookup helper (defined later in this file)
QTreeWidgetItem* findItemByID(QTreeWidgetItem* parent, int itemID);
QSet<int> collectExpandedItemIDs(QTreeWidgetItem* parent);
void restoreExpansionState(QTreeWidgetItem* parent, const QSet<int>& expandedIDs);

using namespace std::chrono_literals;

// Used as a QObject::property so that we can keep track of the color for the
// trigger colorizer buttons loaded from a trigger even if the user disables
// and then reenables the colorizer function (and we "grey out" the color while
// it is disabled):
static const char* cButtonBaseColor = "baseColor";
// The sound file field shows only the file name; the full path saving and loading need is kept here
static const char* cSoundFilePath = "editorSoundFilePath";

// Separates the XML packages of individually copied items on the clipboard so
// that pasting can import and place each one
static const QString cMultiItemPasteSeparator = qsl("\n<!--MUDLET_MULTI_ITEM_SEPARATOR-->\n");

// Every wording an action's button can carry, so its width is held to the widest and the buttons after it stay put
static constexpr char scmProp_editorToolBarWordings[] = "editorToolBarWordings";

// The message label's text before link ink was baked into its anchors, so a theme change can re-ink it
static constexpr char scmProp_editorRichText[] = "editorRichText";

// Set once on the frame carrying the form, the code heading and the code pane, so all three line up
static constexpr int scmEditorColumnPaddingHorizontal = 14;
// One number for all three columns' first rows, so they cannot drift apart when one column's margins change
static constexpr int scmEditorColumnTopInset = 12;
static constexpr int scmEditorColumnSpacing = 12;

static constexpr int scmEditorSidebarPadding = uiDesign::scmSidebarPadding;
static constexpr int scmEditorSidebarVerticalPadding = scmEditorColumnTopInset;
static constexpr int scmEditorSidebarMaximumWidth = 180;
static constexpr int scmEditorSidebarRailWidth = uiDesign::scmSidebarRailWidth;
static constexpr int scmEditorSidebarRailPadding = uiDesign::scmSidebarRailPadding;
static constexpr int scmEditorSidebarSeparatorInset = uiDesign::scmSidebarSeparatorInset;
static constexpr int scmEditorSidebarRowHeight = uiDesign::scmSidebarRowHeight;
// "Icon size toolbars" is a step from 1 to 4 and defaults to 3, which this makes the 18px glyph size
static constexpr int scmEditorIconSizeStep = 6;

// Hysteresis: without it a drag across the breakpoint both fits and does not fit, and the bar flickers
static constexpr int scmEditorToolBarRestoreMargin = 16;
// A floor for the toolbar's label breakpoint, not a width the column is held to
static constexpr int scmEditorContentColumnWidth = 640;

static constexpr int scmEditorIdChipPaddingVertical = 3;
// The .ui files' 3px reads as the word touching the drawn pill
static constexpr int scmEditorIdChipPaddingHorizontal = 9;
static constexpr int scmEditorIdChipGap = 4;

static constexpr int scmEditorFormHeadRowSpacing = 10;
static constexpr int scmEditorFormGridSpacingHorizontal = 14;
static constexpr int scmEditorFormGridSpacingVertical = 12;
// The name label sits in the head row, the lead labels in the grid; this evens their gaps so the fields align
static constexpr int scmEditorFormNameLabelExtra = scmEditorFormGridSpacingHorizontal - scmEditorFormHeadRowSpacing;

// A segment's height is set outright: a stylesheet min-height sizes the contents and the style adds margins on top
static constexpr int scmEditorSegmentPaddingVertical = 5;
static constexpr int scmEditorSegmentPaddingHorizontal = 12;

static constexpr qreal scmEditorHoveredBorderWeight = 0.35;

// The chosen row's accent bar is cut out of this gutter, not added to it, so the row's contents never shift
static constexpr int scmEditorTreeRowGutter = 4;

static uiDesign::SidebarMetrics editorSidebarMetrics(const int expandedWidth)
{
    return {.expandedWidth = expandedWidth,
            .railWidth = scmEditorSidebarRailWidth,
            .padding = scmEditorSidebarPadding,
            .railPadding = scmEditorSidebarRailPadding,
            .verticalPadding = scmEditorSidebarVerticalPadding,
            .separatorInset = scmEditorSidebarSeparatorInset};
}

static constexpr int scmEditorSearchResultIndent = 14;

static constexpr int scmEditorCodeHeaderGlyphSize = 13;
static constexpr int scmEditorCodeHeaderInset = 4;

static constexpr int scmEditorColorWellHeight = 22;
static constexpr int scmEditorColorWellWidth = 40;
static constexpr int scmEditorRowControlGap = 8;
static constexpr int scmEditorOptionRowGap = 24;
// Wide enough for the toolbar's labels and the trigger options strip on one line; restoreWindowGeometry()
// fits it to a smaller desktop
static constexpr int scmEditorDefaultWidth = 1300;
static constexpr int scmEditorDefaultHeight = 690;
static constexpr int scmEditorOptionsSpinBoxSlack = 2;
static constexpr int scmEditorSoundFileWidth = 150;
static constexpr int scmEditorSoundFileTextInset = 2 * (uiDesign::scmInputPaddingHorizontal + uiDesign::scmInputBorderWidth);
// The hidden spinBox_lineMargin must allow the same maximum, or it clamps what was typed and echoes it back,
// eating keystrokes
static constexpr int scmEditorMatchWithinLinesMax = 999;
static constexpr int scmEditorBannerCloseSize = 12;

// Must stay the sum of trigger_pattern_edit.ui's top and bottom margins
static constexpr int scmEditorPatternRowMargins = 8;
static constexpr int scmEditorPatternRowMinimumHeight = uiDesign::scmInputHeight + scmEditorPatternRowMargins;
static constexpr int scmEditorPatternRowPadding = scmEditorPatternRowMargins + 2 * (uiDesign::scmInputBorderWidth + uiDesign::scmInputPaddingVertical + SingleLineTextEdit::scmDocumentMargin);
static constexpr qreal scmEditorPatternHoverStrength = 0.07;
static constexpr int scmEditorPatternTypeChrome = 60;
static constexpr int scmEditorPatternNumberDigits = 2;
static constexpr int scmEditorPatternDeleteButtonSize = 22;
static constexpr int scmEditorPatternDeleteGlyphSize = 13;
static constexpr int scmEditorPatternDropIndicatorHeight = 2;
static constexpr int scmEditorPatternDragThreshold = 4;
static constexpr int scmEditorPatternRowLimit = 50;
static constexpr int scmEditorPatternSwatchSize = 12;
static constexpr qreal scmEditorPatternSwatchRadius = 3.0;
static constexpr qreal scmEditorPatternSwatchSaturation = 0.55;
static constexpr qreal scmEditorPatternSwatchLightnessOnDark = 0.58;
static constexpr qreal scmEditorPatternSwatchLightnessOnLight = 0.45;
static constexpr qreal scmEditorPatternGripDotDiameter = 2.0;
static constexpr qreal scmEditorPatternGripPitch = 3.0;
static constexpr int scmEditorPatternGripColumns = 2;
static constexpr int scmEditorPatternGripRows = 3;
// Wider than the dots: this is the hit target they are centred in
static constexpr int scmEditorPatternGripWidth = 12;
// Shared with the button's own painted frame, which has to inset by the same
static constexpr int scmEditorAddPatternMarginTop = 4;
static constexpr int scmEditorAddPatternMarginBottom = 2;
// Not Ctrl+Shift+Up/Down, which already jumps to the first and the last pattern
static constexpr auto scmEditorPatternMoveUpKeys = Qt::CTRL | Qt::ALT | Qt::Key_Up;
static constexpr auto scmEditorPatternMoveDownKeys = Qt::CTRL | Qt::ALT | Qt::Key_Down;

// The view role holds cmUnknownView on rows that run a one-off action rather than switch the view
static constexpr int scmRole_editorSidebarAction = Qt::UserRole;
static constexpr int scmRole_editorSidebarView = Qt::UserRole + 1;
// The Folders view shows items of every type, so its row names no one view
static constexpr int scmRole_editorSidebarFolders = Qt::UserRole + 2;

// One per mPatternList entry, in that order. A negative hue is a type with no colour of its own, mixed from
// the page at neutralWeight instead.
struct EditorPatternSwatch
{
    qreal hue;
    qreal neutralWeight;
};

static constexpr EditorPatternSwatch scmEditorPatternSwatches[]{
        {-1.0, 0.72}, // substring
        {0.58, 0.0},  // perl regex
        {0.02, 0.0},  // start of line
        {0.34, 0.0},  // exact match
        {0.50, 0.0},  // lua function
        {0.83, 0.0},  // line spacer
        {-1.0, 0.40}, // color trigger
        {0.13, 0.0},  // prompt
};

static QIcon editorPatternSwatchIcon(const QColor& fill, const QColor& outline, const qreal ratio)
{
    QPixmap swatch(qRound(scmEditorPatternSwatchSize * ratio), qRound(scmEditorPatternSwatchSize * ratio));
    swatch.setDevicePixelRatio(ratio);
    swatch.fill(Qt::transparent);

    QPainter painter(&swatch);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Inset half a pen, or the stroke loses half its width off the pixmap's edge
    const QRectF body(0.5, 0.5, scmEditorPatternSwatchSize - 1.0, scmEditorPatternSwatchSize - 1.0);
    painter.setPen(QPen(outline, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(body, scmEditorPatternSwatchRadius, scmEditorPatternSwatchRadius);
    painter.end();
    return QIcon(swatch);
}

// Painted rather than U+22EE twice, which not every interface font carries
static QPixmap editorPatternGripGlyph(const QColor& color, const qreal ratio)
{
    const qreal width = (scmEditorPatternGripColumns - 1) * scmEditorPatternGripPitch + scmEditorPatternGripDotDiameter;
    const qreal height = (scmEditorPatternGripRows - 1) * scmEditorPatternGripPitch + scmEditorPatternGripDotDiameter;
    QPixmap grip(qRound(width * ratio), qRound(height * ratio));
    grip.setDevicePixelRatio(ratio);
    grip.fill(Qt::transparent);

    QPainter painter(&grip);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    for (int column = 0; column < scmEditorPatternGripColumns; ++column) {
        for (int row = 0; row < scmEditorPatternGripRows; ++row) {
            painter.drawEllipse(QRectF(column * scmEditorPatternGripPitch, row * scmEditorPatternGripPitch, scmEditorPatternGripDotDiameter, scmEditorPatternGripDotDiameter));
        }
    }
    painter.end();
    return grip;
}

// Track whether the shared auto-complete provider has been initialized
bool dlgTriggerEditor::smAutoCompleteInitialized = false;

dlgTriggerEditor::dlgTriggerEditor(Host* pH, std::unique_ptr<TConsoleModel> errorConsoleModel)
: mpHost(pH)
, mSearchOptions(pH->mSearchOptions)
{
    // init generated dialog
    setupUi(this);

    // clang-format off
    introAddItem.insert(EditorViewType::cmAliasView, {
        //: Headline for the Alias intro
        tr("Alias react on user input."), {
        //: Name of a selectable option for the Alias intro
        {qsl("alias1"), tr("How to add a new alias now"),
        //: Help contents of a selectable option for the Alias intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define an input <strong>pattern</strong> either literally or with a Perl regular expression.</li>"
               "<li>Define a 'substitution' <strong>command</strong> to send to the game in clear text <strong>instead of the alias pattern</strong>, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the alias.</li></ol>")},
        //: Name of a selectable option for the Alias intro
        {qsl("alias2"), tr("How to add a new alias from the input line"),
            qsl("%1%2%3%4").arg(
                //: Help contents of a selectable option for the Alias intro
                qsl("<p>%1</p>").arg(tr("There are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you.")),
                //: Part of the Alias intro - This introductory text will be followed by a Lua code example for a trigger.
                qsl("<p>%1</p>").arg(tr("Alias can also be defined from the input line in the main profile window like this:")),
                qsl("<p><code>%1</code></p>").arg(qsl("lua permAlias(&quot;%1&quot;, &quot;&quot;, &quot;%2&quot;, function() send(&quot;%3&quot;) echo(&quot;%4&quot;) end)").arg(
                    //: Part of the Alias intro, code example for an alias - This is the name of the alias which reacts on the player typing "hi" by saying "Greetings, traveller!" in game.
                    tr("My greetings"),
                    //: Part of the Alias intro, code example for an alias - This is the text input from the player which will be reacted on by saying "Greetings, traveller!" in game.
                    tr("hi"),
                    //: Part of the Alias intro, code example for an alias - This is the command that Mudlet will send to the game after the player typed "hi".
                    tr("say Greetings, traveller!"),
                    //: Part of the Alias intro, code example for an alias - This is the confirmation text shown to the player after they typed "hi" and we said "Greetings, traveller!" in game.
                    tr("We said hi!"))),
                //: Part of the Alias intro - This is the conclusion after the code example for an alias which reacts on the player typing "hi" by saying "Greetings, traveller!" in game.
                qsl("<p>%1</p>").arg("You can now greet by typing 'hi'"))},
        {qsl("alias3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/Uz6EDvZYNvE"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Aliases'>Introduction to Aliases</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmTriggerView, {
        //: Headline for the Trigger intro
        tr("Triggers react on game output."), {
        //: Name of a selectable option for the Trigger intro
        {qsl("trigger1"), tr("How to add a new trigger now"),
        //: Help contents of a selectable option for the Trigger intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define a <strong>pattern</strong> that you want to trigger on.</li>"
               "<li>Select the appropriate pattern <strong>type</strong>.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..</li>"
               "<li><strong>Activate</strong> the trigger.</li></ol>")},
        //: Name of a selectable option for the Trigger intro
        {qsl("trigger2"), tr("How to add a new trigger from the input line"),
            qsl("%1%2%3%4").arg(
                //: Help contents of a selectable option for the Trigger intro
                qsl("<p>%1</p>").arg(tr("There are a <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=22609'>couple</a> of <a href='https://forums.mudlet.org/viewtopic.php?f=6&t=16462'>packages</a> that can help you.")),
                //: Part of the Trigger intro - This introductory text will be followed by a Lua code example for a trigger.
                qsl("<p>%1</p>").arg(tr("Triggers can also be defined from the input line in the main profile window like this:")),
                qsl("<p><code>%1</code></p>").arg(qsl("lua permSubstringTrigger(&quot;%1&quot;, &quot;&quot;, &quot;%2&quot;, function() send(&quot;%3&quot;) end)").arg(
                    //: Part of the Trigger intro, code example for a trigger - This is the name of the trigger which reacts on "You are thirsty" with "drink water".
                    tr("My drink trigger"),
                    //: Part of the Trigger intro, code example for a trigger - This is the text from game which will be triggered on, and reacted to with "drink water".
                    tr("You are thirsty."),
                    //: Part of the Trigger intro, code example for a trigger - This is the command sent to game after we triggered on text "You are thirsty." from game.
                    tr("drink water"))),
                //: Part of the Trigger intro - This is the conclusion after the code example for a trigger which reacts on "You are thirsty" with "drink water".
                qsl("<p>%1</p>").arg("This will keep you refreshed."))},
        {qsl("trigger3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/jYjop54-Y3I"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Triggers'>Introduction to Triggers</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmScriptView, {
        //: Headline for the Script intro
        tr("Scripts organize code and can react to events."), {
        //: Name of a selectable option for the Script intro
        {qsl("script1"), tr("How to add a new script now"),
        //: Help contents of a selectable option for the Script intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Enter a script in the box below. You can for example define <strong>functions</strong> to be called by other triggers, aliases, etc.</li>"
               "<li>If you write lua <strong>commands</strong> without defining a function, they will be run on Mudlet startup and each time you open the script for editing.</li>"
               "<li><strong>Activate</strong> the script.</li></ol>"
               "<p><strong>Note:</strong> Scripts are run automatically when viewed, even if they are deactivated.</p>")},
        //: Name of a selectable option for the Script intro
        {qsl("script2"), tr("How to have a script react to events"),
        //: Help contents of a selectable option for the Script intro
            tr("<p>You can register a list of <strong>events</strong> with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.</p>"
               "<p><strong>Note:</strong> Events can also be added to a script from the command line in the main profile window like this:</p>"
               "<p><code>lua registerAnonymousEventHandler(&quot;nameOfTheMudletEvent&quot;, &quot;nameOfYourFunctionToBeCalled&quot;)</code></p>")},
        {qsl("script3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/10mJUh4Hq-A"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Scripts'>Introduction to Scripts</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmTimerView, {
        //: Headline for the Timer intro
        tr("Timers react after a timespan once or regularly."), {
        //: Name of a selectable option for the Timer intro
        {qsl("timer1"), tr("How to add a new timer now"),
        //: Help contents of a selectable option for the Timer intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Define the <strong>timespan</strong> after which the timer should react in a this format: hours : minutes : seconds.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game when the time has passed, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the timer.</li></ol>"
               "<p><strong>Note:</strong> If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.</p>")},
        //: Name of a selectable option for the Timer intro
        {qsl("timer2"), tr("How to add a new timer from the input line"),
        //: Help contents of a selectable option for the Timer intro
            tr("<p>Timers can also be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua tempTimer(3, function() echo(&quot;hello!\n&quot;) end)</code></p>"
               "<p>This will greet you exactly 3 seconds after it was made.</p>")},
        {qsl("timer3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Timers'>Introduction to Timers</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmActionView, {
        //: Headline for the Button intro
        tr("Buttons react on mouse clicks."), {
        //: Name of a selectable option for the Button intro
        {qsl("button1"), tr("How to add a new button now"),
        //: Help contents of a selectable option for the Button intro
            tr("<ol><li>Add a new group to create a <strong>button bar</strong>.</li>"
               "<li>Add groups as <strong>menus</strong> or sub-menus.</li>"
               "<li>Add items as <strong>buttons</strong> to a bar or menu.</li>"
               "<li>Define a <strong>command</strong> or script to execute when pressed.</li>"
               "<li><strong>Activate</strong> the item. </li></ol>"
               "<p><strong>Note:</strong> Deactivated items are hidden, including all items they contain.</p>"
               "<p><strong>Click-down buttons:</strong> Can define separate commands for press/release. Use getButtonState() to check state.</p>")},
//        {qsl("button2"), tr("How to add a new button from the input line"),
//            tr("")},
        {qsl("button3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Buttons'>Introduction to Buttons</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmKeysView, {
        //: Headline for the Keys intro
        tr("Keys react on keyboard presses."), {
        //: Name of a selectable option for the Keys intro
        {qsl("key1"), tr("How to add a new keybinding now"),
        //: Help contents of a selectable option for the Keys intro
            tr("<ol><li>Click on the 'Add Item' icon above.</li>"
               "<li>Click on <strong>'grab key'</strong> and then press your key combination, e.g. including modifier keys like Control, Shift, etc.</li>"
               "<li>Define a clear text <strong>command</strong> that you want to send to the game if the button is pressed, or write a script for more complicated needs.</li>"
               "<li><strong>Activate</strong> the new key binding.</li></ol>")},
        //: Name of a selectable option for the Keys intro
        {qsl("key2"), tr("How to add a new keybinding from the input line"),
        //: Help contents of a selectable option for the Keys intro
            tr("<p>Keys can be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua permKey(&quot;my jump key&quot;, &quot;&quot;, mudlet.key.F8, [[send(&quot;jump&quot;]]) end)</code></p>"
               "<p>Pressing F8 will make you jump.</p>")},
        {qsl("key3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3%4</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p><li>").arg(tr("Watch a <a href='%1'>video demonstration</a> of the basic functionality.")
                    .arg(qsl("https://youtu.be/ZYRPZ-8fJWA"))),
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Keybindings'>Introduction to Keybindings</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});

    introAddItem.insert(EditorViewType::cmVarsView, {
        //: Headline for the Variable intro
        tr("Variables store information."), {
        //: Name of a selectable option for the Variable intro
        {qsl("variable1"), tr("How to add a new variable now"),
        //: Help contents of a selectable option for the Variable intro
            tr("<ol><li>Click on the 'Add Item' icon above. To add a table instead click 'Add Group'.</li>"
               "<li>Select type of variable value (can be a string, integer, boolean)</li>"
               "<li>Enter the value you want to store in this variable.</li>"
               "<li>If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.</li>"
               "<li>To remove a variable manually, set it to 'nil' or click on the 'Delete' icon above.</li></ol>"
               "<p><strong>Note:</strong> Variables created here won't be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.</p>")},
        //: Name of a selectable option for the Variable intro
        {qsl("variable2"), tr("How to add a new variable from the input line"),
        //: Help contents of a selectable option for the Variable intro
            tr("<p>Variables and tables can also be defined from the input line in the main profile window like this:</p>"
               "<p><code>lua foo = &quot;bar&quot;</code></p>"
               "<p>This will create a string called 'foo' with 'bar' as its value.</p>")},
        {qsl("variable3"), tr("Where to find more information"),
            qsl("<ul>%1%2%3</ul>").arg( // reduce clutter for translators
                qsl("<li><p>%1</p></li>").arg(tr("Read the <a href='http://wiki.mudlet.org/w/Manual:Introduction#Variables'>Introduction to Variables</a> for a detailed overview.")),
                qsl("<li><p>%1</p>").arg(tr("Do you maybe have any other suggestions, questions or doubts?")),
                qsl("<p>%1</p></li>").arg(tr("Join our community on <a href='https://www.mudlet.org/chat'>Discord</a> or in <a href='https://forums.mudlet.org/'>Mudlet forums</a> - See you there!")))}}});
    // clang-format on

    // Descriptions for screen readers, clarify to translators that the context of "activated" is current status and not confirmation of toggle.
    //: Item is currently on, short enough to be spoken
    descActive = tr("activated");
    //: Item is currently off, short enough to be spoken
    descInactive = tr("deactivated");
    //: Folder is currently turned on
    descActiveFolder = tr("activated folder");
    //: Folder is currently turned off
    descInactiveFolder = tr("deactivated folder");
    //: Item is currently inactive because of errors, short enough to be spoken
    descError = tr("deactivated due to error");
    //: Item is currently turned on individually, but is member of an inactive group
    descInactiveParent = tr("%1 in a deactivated group");
    //: A trigger that unlocks other triggers is currently turned on, short enough to be spoken
    descActiveFilterChain = tr("activated filter chain");
    //: A trigger that unlocks other triggers is currently turned off, short enough to be spoken
    descInactiveFilterChain = tr("deactivated filter chain");
    //: A timer that starts after another timer is currently turned on
    descActiveOffsetTimer = tr("activated offset timer");
    //: A timer that starts after another timer is currently turned off
    descInactiveOffsetTimer = tr("deactivated offset timer");
    //: Said after an item that is switched on while the emergency stop keeps everything from running. %1 is how the item is described otherwise, such as "activated" or "activated folder"
    descHeldByStop = tr("%1, held by the emergency stop");
    //: Accessible description for a newly created folder, shown after the folder name
    descNewFolder = tr("new folder");
    //: Accessible description for a newly created item, shown after the item name
    descNewItem = tr("new item");
    //: Accessible description indicating an item belongs to a package, shown after the item name. Keep short, as it's appended to other descriptions like "activated, package item"
    descPackageItem = tr("package item");
    //: Accessible description for a key binding whose key one of Mudlet's own shortcuts or an add-on command gets first, shown after the item name. Keep short, as it's appended to other descriptions like "activated, will not fire, key in use"
    descKeyTaken = tr("will not fire, key in use");

    const QString hostName{mpHost->getName()};
    setWindowTitle(tr("%1 - Editor").arg(hostName));
    setWindowIcon(QIcon(qsl(":/icons/mudlet_editor.png")));
    auto statusBar = new QStatusBar(this);
    statusBar->setObjectName(qsl("editorStatusBar"));
    statusBar->setSizeGripEnabled(true);
    setStatusBar(statusBar);
    statusBar->show();

    // A plain widget, which QStatusBar hides while a temporary message shows
    mpLabel_statusCounts = new QLabel(statusBar);
    mpLabel_statusCounts->setObjectName(qsl("editorStatusCounts"));
    statusBar->addWidget(mpLabel_statusCounts);
    // Permanent, which is the only way QStatusBar keeps a widget at the trailing edge and showing during messages
    mpLabel_statusAutosave = new QLabel(statusBar);
    mpLabel_statusAutosave->setObjectName(qsl("editorStatusAutosave"));
    statusBar->addPermanentWidget(mpLabel_statusAutosave);

    mpTimer_statusCounts = new QTimer(this);
    mpTimer_statusCounts->setSingleShot(true);
    mpTimer_statusCounts->setInterval(200ms);
    connect(mpTimer_statusCounts, &QTimer::timeout, this, &dlgTriggerEditor::updateEditorItemCounts);

    if (QLayout* pColumnLayout = frame_right->layout()) {
        pColumnLayout->setContentsMargins(scmEditorColumnPaddingHorizontal, scmEditorColumnTopInset, scmEditorColumnPaddingHorizontal, 0);
    }

    mpNonCodeWidgets = new QWidget(this);
    // A change in the column's height can mean a different split - see applyFormPaneSeamPolicy()
    mpNonCodeWidgets->installEventFilter(this);
    auto* layoutColumn = new QVBoxLayout(mpNonCodeWidgets);
    // A margin is part of the column's size hint, so a form held to its contents keeps the gap above the code heading
    layoutColumn->setContentsMargins(0, 0, 0, scmEditorColumnSpacing);
    layoutColumn->setSpacing(scmEditorColumnSpacing);
    splitter_right->addWidget(mpNonCodeWidgets);

    // system message area
    mpSystemMessageArea = new dlgSystemMessageArea(this);
    mpSystemMessageArea->setObjectName(qsl("mpSystemMessageArea"));
    // Ignored across, or the wrapped label's width guess widens the column; Maximum down, so the
    // height-for-width in dlgSystemMessageArea::sizeHint() is also the most it gets
    mpSystemMessageArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
    // The .ui's default margin would push the notice below the other two columns' first rows
    if (QLayout* pMessageAreaLayout = mpSystemMessageArea->layout()) {
        pMessageAreaLayout->setContentsMargins(0, 0, 0, 0);
    }
    // set the stretch factor of the message area to 0 and everything else to 1,
    // so our errors box doesn't stretch to produce a grey area
    layoutColumn->addWidget(mpSystemMessageArea, 0);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);
    connect(mudlet::self(), &mudlet::signal_appearanceChanged, this, &dlgTriggerEditor::slot_refreshBannerLinkColors);

    // main areas
    mpTriggersMainArea = new dlgTriggersMainArea(this);
    layoutColumn->addWidget(mpTriggersMainArea, 1);
    connect(mpTriggersMainArea->pushButtonFgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetFgColor);
    connect(mpTriggersMainArea->pushButtonBgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorizeTriggerSetBgColor);
    connect(mpTriggersMainArea->checkBox_triggerColorizer, &QCheckBox::clicked, this, &dlgTriggerEditor::slot_toggleGroupBoxColorizeTrigger);
    connect(mpTriggersMainArea->toolButton_clearSoundFile, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_clearSoundFile);

    // Fixed-field forms take no stretch, so spare room goes to the trailing stretch and they stay at the top;
    // the trigger and button forms grow their pattern list and stylesheet editor into it
    mpTimersMainArea = new dlgTimersMainArea(this);
    layoutColumn->addWidget(mpTimersMainArea, 0);

    mpAliasMainArea = new dlgAliasMainArea(this);
    layoutColumn->addWidget(mpAliasMainArea, 0);

    mpActionsMainArea = new dlgActionMainArea(this);
    layoutColumn->addWidget(mpActionsMainArea, 1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::checkStateChanged, this, &dlgTriggerEditor::slot_toggleIsPushDownButton);
#else
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::stateChanged, this, &dlgTriggerEditor::slot_toggleIsPushDownButton);
#endif

    mpKeysMainArea = new dlgKeysMainArea(this);
    layoutColumn->addWidget(mpKeysMainArea, 0);

    mpVarsMainArea = new dlgVarsMainArea(this);
    layoutColumn->addWidget(mpVarsMainArea, 0);

    mpScriptsMainArea = new dlgScriptsMainArea(this);
    layoutColumn->addWidget(mpScriptsMainArea, 0);

    layoutColumn->addStretch(0);

    // source editor area
    mpSourceEditorArea = new dlgSourceEditorArea(this);
    // Also a minimum, so a drag of the heading stops at the seam's floor rather than the widgets' own minimum
    mpSourceEditorArea->setMinimumHeight(scmEditorSourcePaneFloor);
    splitter_right->addWidget(mpSourceEditorArea);

    // And the edbee widget
    mpSourceEditorEdbee = mpSourceEditorArea->edbeeEditorWidget;
    // Before any text: the margin measures its width once per line count, the room for the dot included
    mpEditorLineMargin = new uiDesign::EditorLineMarginDelegate();
    mpSourceEditorEdbee->textMarginComponent()->giveDelegate(mpEditorLineMargin);
    mpSourceEditorEdbee->setAutoScrollMargin(20);
    mpSourceEditorEdbee->setPlaceholderText(tr("-- add your Lua code here"));
    mpSourceEditorEdbeeDocument = mpSourceEditorEdbee->textDocument();

    connect(mpSourceEditorEdbee->controller(), &edbee::TextEditorController::updateStatusTextSignal, this, &dlgTriggerEditor::slot_updateCaretPosition);
    // The caret readout hides and returns with the code pane
    mpSourceEditorArea->installEventFilter(this);
    mpSourceEditorEdbee->controller()->setAutoScrollToCaret(edbee::TextEditorController::AutoScrollWhenFocus);

    // Update the editor preferences
    connect(mudlet::self(), &mudlet::signal_editorTextOptionsChanged, this, &dlgTriggerEditor::slot_changeEditorTextOptions);

    mudlet::loadEdbeeTheme(mpHost->getEditorTheme(), mpHost->getEditorThemeFile());

    // edbee editor find area
    mpSourceEditorFindArea = new dlgSourceEditorFindArea(mpSourceEditorEdbee);
    mpSourceEditorEdbee->horizontalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorEdbee->verticalScrollBar()->installEventFilter(mpSourceEditorFindArea);
    // reposition the find area when the editor itself resizes, e.g. when the
    // system message area above it appears or disappears
    mpSourceEditorEdbee->installEventFilter(mpSourceEditorFindArea);
    mpSourceEditorFindArea->hide();

    connect(mpSourceEditorFindArea->lineEdit_findText, &QLineEdit::textChanged, this, &dlgTriggerEditor::slot_sourceFindTextChanges);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorMovementNecessary, this, &dlgTriggerEditor::slot_sourceFindMove);
    connect(mpSourceEditorFindArea->pushButton_findPrevious, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceFindPrevious);
    connect(mpSourceEditorFindArea->pushButton_findNext, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceFindNext);
    connect(mpSourceEditorFindArea->pushButton_replace, &QPushButton::clicked, this, &dlgTriggerEditor::slot_sourceReplace);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindPrevious, this, &dlgTriggerEditor::slot_sourceFindPrevious);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorFindNext, this, &dlgTriggerEditor::slot_sourceFindNext);
    connect(mpSourceEditorFindArea, &dlgSourceEditorFindArea::signal_sourceEditorReplace, this, &dlgTriggerEditor::slot_sourceReplace);
    connect(mpSourceEditorFindArea->pushButton_close, &QPushButton::clicked, this, &dlgTriggerEditor::slot_closeSourceFind);

    auto openSourceFindAction = new QAction(this);
    openSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    openSourceFindAction->setShortcut(QKeySequence(QKeySequence::Find));
    mpSourceEditorArea->addAction(openSourceFindAction);
    connect(openSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_openSourceFind);

    QAction* closeSourceFindAction = new QAction(this);
    closeSourceFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    closeSourceFindAction->setShortcut(QKeySequence(QKeySequence::Cancel));
    mpSourceEditorArea->addAction(closeSourceFindAction);
    connect(closeSourceFindAction, &QAction::triggered, this, &dlgTriggerEditor::slot_closeSourceFind);

    QAction* sourceFindNextAction = new QAction(this);
    sourceFindNextAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindNextAction->setShortcut(QKeySequence(QKeySequence::FindNext));
    mpSourceEditorArea->addAction(sourceFindNextAction);
    connect(sourceFindNextAction, &QAction::triggered, this, &dlgTriggerEditor::slot_sourceFindNext);

    QAction* sourceFindPreviousAction = new QAction(this);
    sourceFindPreviousAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    sourceFindPreviousAction->setShortcut(QKeySequence(QKeySequence::FindPrevious));
    mpSourceEditorArea->addAction(sourceFindPreviousAction);
    connect(sourceFindPreviousAction, &QAction::triggered, this, &dlgTriggerEditor::slot_sourceFindPrevious);
    mpUndoStack = new EditorUndoStack(this);
    mpUndoStack->setUndoLimit(50);

    // These route to either text editor or item operations based on focus
    mpUndoAction = new QAction(QIcon::fromTheme(qsl("edit-undo"), QIcon(qsl(":/icons/edit-undo.png"))), tr("Undo"), this);
    mpUndoAction->setShortcut(QKeySequence(QKeySequence::Undo)); // Ctrl+Z
    mpUndoAction->setShortcutContext(Qt::WindowShortcut);
    mpUndoAction->setEnabled(false);
    /* In this and the next addAction(...) call we want to use the
     * QMainWindow::addAction(...) method and NOT the
     * dlgTriggerEditor::addAction(...) - without specifying this the derived
     * method is used. Calling the second one causes a bogus "new Toolbar"
     * containing a "new Menu" to be created each time the profile is opened
     * - which persist with a new pair added to the pile each time.*/
    QMainWindow::addAction(mpUndoAction);
    connect(mpUndoAction, &QAction::triggered, this, &dlgTriggerEditor::slot_smartUndo);

    mpRedoAction = new QAction(QIcon::fromTheme(qsl("edit-redo"), QIcon(qsl(":/icons/edit-redo.png"))), tr("Redo"), this);
    mpRedoAction->setShortcut(QKeySequence(QKeySequence::Redo)); // Ctrl+Y or Ctrl+Shift+Z
    mpRedoAction->setShortcutContext(Qt::WindowShortcut);
    mpRedoAction->setEnabled(false);
    QMainWindow::addAction(mpRedoAction);
    connect(mpRedoAction, &QAction::triggered, this, &dlgTriggerEditor::slot_smartRedo);

    connect(mpUndoStack, &QUndoStack::canUndoChanged, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpUndoStack, &QUndoStack::canRedoChanged, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpUndoStack, &QUndoStack::indexChanged, this, &dlgTriggerEditor::updateEditorActivateAction);

    connect(mpUndoStack, &QUndoStack::undoTextChanged, this, [this](const QString& text) {
        QString shortcut = mpUndoAction->shortcut().toString(QKeySequence::NativeText);
        if (!text.isEmpty()) {
            //: Tooltip for undo action. %1 is the action being undone (e.g., "Activate trigger \"foo\""), %2 is the keyboard shortcut
            QString undoText = tr("Undo: %1 (%2)").arg(text, shortcut);
            mpUndoAction->setToolTip(utils::richText(undoText));
            mpUndoAction->setStatusTip(undoText);
        } else {
            //: Tooltip for undo action when no specific action. %1 is the keyboard shortcut
            QString undoText = tr("Undo (%1)").arg(shortcut);
            mpUndoAction->setToolTip(utils::richText(undoText));
            mpUndoAction->setStatusTip(undoText);
        }
    });
    connect(mpUndoStack, &QUndoStack::redoTextChanged, this, [this](const QString& text) {
        QString shortcut = mpRedoAction->shortcut().toString(QKeySequence::NativeText);
        if (!text.isEmpty()) {
            //: Tooltip for redo action. %1 is the action being redone (e.g., "Activate trigger \"foo\""), %2 is the keyboard shortcut
            QString redoText = tr("Redo: %1 (%2)").arg(text, shortcut);
            mpRedoAction->setToolTip(utils::richText(redoText));
            mpRedoAction->setStatusTip(redoText);
        } else {
            //: Tooltip for redo action when no specific action. %1 is the keyboard shortcut
            QString redoText = tr("Redo (%1)").arg(shortcut);
            mpRedoAction->setToolTip(utils::richText(redoText));
            mpRedoAction->setStatusTip(redoText);
        }
    });

    // Store guarded pointer to text editor's undo stack for safe signal connections
    mpTextUndoStack = mpSourceEditorEdbee->controller()->textDocument()->textUndoStack();

    connect(mpTextUndoStack, &edbee::TextUndoStack::undoExecuted, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpTextUndoStack, &edbee::TextUndoStack::redoExecuted, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpTextUndoStack, &edbee::TextUndoStack::changeAdded, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);

    slot_updateUndoRedoButtonStates();

    connect(mpUndoStack, &EditorUndoStack::itemsChanged, this, &dlgTriggerEditor::slot_itemsChanged);

    ensureLuaAutoCompleteProvider();

    mpSourceEditorEdbee->textEditorComponent()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpSourceEditorEdbee->textEditorComponent(), &QWidget::customContextMenuRequested, this, &dlgTriggerEditor::slot_editorContextMenu);

    // option areas
    if (!errorConsoleModel) {
        errorConsoleModel = newErrorConsoleModel(mpHost);
    }
    mpErrorConsole = new TConsole(mpHost, QString(), TConsole::ErrorConsole, this, std::move(errorConsoleModel));
    mpErrorConsole->setMinimumHeight(100);
    mpErrorConsole->setFont(mpHost->getEditorFont());
    mpErrorConsole->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    splitter_right->addWidget(mpErrorConsole);

    // Spare height goes to the code alone, the one pane with no natural height; sharing it out stretched
    // the form and pushed the code far down the window
    splitter_right->setStretchFactor(0, 0); // mpNonCodeWidgets
    splitter_right->setCollapsible(0, false);
    splitter_right->setStretchFactor(1, 1); // mpSourceEditorArea
    splitter_right->setCollapsible(1, false);
    splitter_right->setStretchFactor(2, 0); // mpErrorConsole
    splitter_right->setCollapsible(2, false);

    mpErrorConsole->hide();

    // After every pane is added, so the splitter handle between the first two exists
    setupEditorCodeHeader();

    buildTriggerOptionsStrip();

    // These row builders run before buildEditorFormHeadRows(), so the rows they fill are carried down with
    // the rest of the grid
    buildScriptEventRow();

    buildTimerIntervalRow();
    buildKeyBindingRow();
    buildVariableTypeRows();
    buildActionRows();

    buildEditorFormHeadRows();

    connect(splitter_right, &QSplitter::splitterMoved, this, &dlgTriggerEditor::slot_rightSplitterMoved);
    splitter_right->installEventFilter(this);
    // additional settings
    treeWidget_triggers->setColumnCount(1);
    treeWidget_triggers->setTreeType(TreeType::Trigger);
    treeWidget_triggers->setRootIsDecorated(false);
    treeWidget_triggers->setHost(mpHost);
    treeWidget_triggers->header()->hide();
    //: Accessible name of the editor's tree of triggers, which screen readers announce when it gets the focus
    treeWidget_triggers->setAccessibleName(tr("Triggers"));
    treeWidget_triggers->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_aliases->hide();
    treeWidget_aliases->setHost(mpHost);
    treeWidget_aliases->setTreeType(TreeType::Alias);
    treeWidget_aliases->setColumnCount(1);
    treeWidget_aliases->header()->hide();
    //: Accessible name of the editor's tree of aliases, which screen readers announce when it gets the focus
    treeWidget_aliases->setAccessibleName(tr("Aliases"));
    treeWidget_aliases->setRootIsDecorated(false);
    treeWidget_aliases->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_actions->hide();
    treeWidget_actions->setHost(mpHost);
    treeWidget_actions->setTreeType(TreeType::Action);
    treeWidget_actions->setColumnCount(1);
    treeWidget_actions->header()->hide();
    //: Accessible name of the editor's tree of buttons, which screen readers announce when it gets the focus
    treeWidget_actions->setAccessibleName(tr("Buttons"));
    treeWidget_actions->setRootIsDecorated(false);
    treeWidget_actions->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_timers->hide();
    treeWidget_timers->setHost(mpHost);
    treeWidget_timers->setTreeType(TreeType::Timer);
    treeWidget_timers->setColumnCount(1);
    treeWidget_timers->header()->hide();
    //: Accessible name of the editor's tree of timers, which screen readers announce when it gets the focus
    treeWidget_timers->setAccessibleName(tr("Timers"));
    treeWidget_timers->setRootIsDecorated(false);
    treeWidget_timers->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_variables->hide();
    treeWidget_variables->setHost(mpHost);
    treeWidget_variables->setTreeType(TreeType::Var);
    treeWidget_variables->setColumnCount(2);
    treeWidget_variables->hideColumn(1);
    treeWidget_variables->header()->hide();
    //: Accessible name of the editor's tree of variables, which screen readers announce when it gets the focus
    treeWidget_variables->setAccessibleName(tr("Variables"));
    treeWidget_variables->setRootIsDecorated(false);
    treeWidget_variables->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_keys->hide();
    treeWidget_keys->setHost(mpHost);
    treeWidget_keys->setTreeType(TreeType::Key);
    treeWidget_keys->setColumnCount(1);
    treeWidget_keys->header()->hide();
    //: Accessible name of the editor's tree of keys, which screen readers announce when it gets the focus
    treeWidget_keys->setAccessibleName(tr("Keys"));
    treeWidget_keys->setRootIsDecorated(false);
    treeWidget_keys->setContextMenuPolicy(Qt::ActionsContextMenu);

    treeWidget_scripts->hide();
    treeWidget_scripts->setHost(mpHost);
    treeWidget_scripts->setTreeType(TreeType::Script);
    treeWidget_scripts->setColumnCount(1);
    treeWidget_scripts->header()->hide();
    //: Accessible name of the editor's tree of scripts, which screen readers announce when it gets the focus
    treeWidget_scripts->setAccessibleName(tr("Scripts"));
    treeWidget_scripts->setRootIsDecorated(false);
    treeWidget_scripts->setContextMenuPolicy(Qt::ActionsContextMenu);

    // A TTreeWidget for the row option EditorTreeDelegate measures against; built from the type trees, so no drag
    mpFolderTree = new TTreeWidget(frame_left);
    mpFolderTree->setObjectName(qsl("treeWidget_folders"));
    //: Accessible name of the editor's Folders view, which lists every item type grouped by folder
    mpFolderTree->setAccessibleName(tr("Folders"));
    mpFolderTree->setSelectionMode(QAbstractItemView::SingleSelection);
    mpFolderTree->setDragDropMode(QAbstractItemView::NoDragDrop);
    mpFolderTree->setDragEnabled(false);
    mpFolderTree->setAcceptDrops(false);
    mpFolderTree->viewport()->setAcceptDrops(false);
    mpFolderTree->setColumnCount(2);
    // The item type is drawn after the name by the row's delegate
    mpFolderTree->hideColumn(1);
    mpFolderTree->header()->hide();
    mpFolderTree->setRootIsDecorated(false);
    mpFolderTree->setUniformRowHeights(true);
    mpFolderTree->setContextMenuPolicy(Qt::ActionsContextMenu);
    mpFolderTree->hide();
    connect(mpFolderTree, &QTreeWidget::currentItemChanged, this, &dlgTriggerEditor::slot_folderTreeItemChanged);
    connect(mpFolderTree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* pItem) {
        if (!pItem) {
            return;
        }
        if (pItem->data(0, FolderPartsRole).toStringList().size() == 1) {
            slot_toggleItemOrGroupActiveFlag();
            return;
        }
        if (mFolderGroupKey.isEmpty()) {
            showFolderGroup(pItem);
        }
    });
    // A merged folder stays current under the form of one of its folders, so choosing it changes no current item
    connect(mpFolderTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* pItem) {
        if (pItem && mFolderGroupKey.isEmpty() && pItem->data(0, FolderPartsRole).toStringList().size() != 1) {
            showFolderGroup(pItem);
        }
    });

    mpFolderTreeRefreshTimer = new QTimer(this);
    mpFolderTreeRefreshTimer->setSingleShot(true);
    connect(mpFolderTreeRefreshTimer, &QTimer::timeout, this, [this]() {
        if (mFolderViewActive) {
            populateFolderTree();
        }
    });
    for (const auto view :
         {EditorViewType::cmTriggerView, EditorViewType::cmAliasView, EditorViewType::cmScriptView, EditorViewType::cmTimerView, EditorViewType::cmKeysView, EditorViewType::cmActionView}) {
        QTreeWidget* pTree = treeForView(view);
        const QAbstractItemModel* pModel = pTree->model();
        connect(pModel, &QAbstractItemModel::rowsInserted, this, &dlgTriggerEditor::scheduleFolderTreeRefresh);
        connect(pModel, &QAbstractItemModel::rowsRemoved, this, &dlgTriggerEditor::scheduleFolderTreeRefresh);
        connect(pModel, &QAbstractItemModel::rowsMoved, this, &dlgTriggerEditor::scheduleFolderTreeRefresh);
        // Every save and every Lua toggle sets an item's icon again: a rebuild here would lose a pending click
        connect(pModel, &QAbstractItemModel::dataChanged, this, [this, view](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            refreshFolderTreeRows(view, topLeft, bottomRight);
        });
        connect(pModel, &QAbstractItemModel::layoutChanged, this, &dlgTriggerEditor::scheduleFolderTreeRefresh);
        connect(pModel, &QAbstractItemModel::modelReset, this, &dlgTriggerEditor::scheduleFolderTreeRefresh);
        // Activate is worded for the chosen row, and a save, a dot click or a script can switch that row under it
        connect(pTree, &QTreeWidget::currentItemChanged, this, &dlgTriggerEditor::updateEditorActivateAction);
        connect(pModel, &QAbstractItemModel::dataChanged, this, [this, pTree](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            const QModelIndex current = pTree->currentIndex();
            if (current.isValid() && current.parent() == topLeft.parent() && current.row() >= topLeft.row() && current.row() <= bottomRight.row()) {
                updateEditorActivateAction();
            }
        });
        // Adding, deleting, pasting, undoing and searching all pick their item in the type tree, hidden or not
        connect(pTree, &QTreeWidget::currentItemChanged, this, [this, view](QTreeWidgetItem* pCurrent) {
            if (mFolderViewActive && pCurrent && pCurrent->parent()) {
                selectFolderTreeNode(mFolderTreeNodes.value(folderTreeItemKey(view, pCurrent->data(0, IdRole).toInt())));
            }
        });
    }

    QAction* viewTriggerAction = new QAction(QIcon(qsl(":/icons/tools-wizard.png")), tr("Triggers"), this);
    viewTriggerAction->setStatusTip(tr("Show Triggers"));
    viewTriggerAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Triggers"), QKeySequence(Qt::CTRL | Qt::Key_1).toString(QKeySequence::NativeText)));
    viewTriggerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(viewTriggerAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showTriggers();
    });

    QAction* viewAliasAction = new QAction(QIcon(qsl(":/icons/system-users.png")), tr("Aliases"), this);
    viewAliasAction->setStatusTip(tr("Show Aliases"));
    viewAliasAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Aliases"), QKeySequence(Qt::CTRL | Qt::Key_2).toString(QKeySequence::NativeText)));
    viewAliasAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    connect(viewAliasAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showAliases();
    });

    QAction* viewScriptsAction = new QAction(QIcon(qsl(":/icons/document-properties.png")), tr("Scripts"), this);
    viewScriptsAction->setStatusTip(tr("Show Scripts"));
    viewScriptsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Scripts"), QKeySequence(Qt::CTRL | Qt::Key_3).toString(QKeySequence::NativeText)));
    viewScriptsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    connect(viewScriptsAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showScripts();
    });

    QAction* showTimersAction = new QAction(QIcon(qsl(":/icons/chronometer.png")), tr("Timers"), this);
    showTimersAction->setStatusTip(tr("Show Timers"));
    showTimersAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Timers"), QKeySequence(Qt::CTRL | Qt::Key_4).toString(QKeySequence::NativeText)));
    showTimersAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    connect(showTimersAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showTimers();
    });

    QAction* viewKeysAction = new QAction(QIcon(qsl(":/icons/preferences-desktop-keyboard.png")), tr("Keys"), this);
    viewKeysAction->setStatusTip(tr("Show Keybindings"));
    viewKeysAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Keybindings"), QKeySequence(Qt::CTRL | Qt::Key_5).toString(QKeySequence::NativeText)));
    viewKeysAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));
    connect(viewKeysAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showKeys();
    });

    QAction* viewVarsAction = new QAction(QIcon(qsl(":/icons/variables.png")), tr("Variables"), this);
    viewVarsAction->setStatusTip(tr("Show Variables"));
    viewVarsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Variables"), QKeySequence(Qt::CTRL | Qt::Key_6).toString(QKeySequence::NativeText)));
    viewVarsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_6));
    connect(viewVarsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showVariables);

    QAction* viewActionAction = new QAction(QIcon(qsl(":/icons/bookmarks.png")), tr("Buttons"), this);
    viewActionAction->setStatusTip(tr("Show Buttons"));
    viewActionAction->setToolTip(qsl("%1 (%2)").arg(tr("Show Buttons"), QKeySequence(Qt::CTRL | Qt::Key_7).toString(QKeySequence::NativeText)));
    viewActionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_7));
    connect(viewActionAction, &QAction::triggered, this, [this]() {
        leaveFolderView();
        slot_showActions();
    });

    //: Row of the editor's sidebar that shows every item type together, grouped by folder
    mpViewFoldersAction = new QAction(tr("Folders"), this);
    //: Status tip for the editor's Folders view button
    mpViewFoldersAction->setStatusTip(tr("Show triggers, aliases, scripts, timers, keys and buttons together, grouped by folder"));
    mpViewFoldersAction->setVisible(mpHost->folderViewInEditor());
    connect(mpViewFoldersAction, &QAction::triggered, this, &dlgTriggerEditor::slot_showFolders);


    QAction* viewErrorsAction = new QAction(QIcon(qsl(":/icons/errors.png")), tr("Errors"), this);
    viewErrorsAction->setStatusTip(tr("Show/Hide the errors console in the bottom right of this editor."));
    viewErrorsAction->setToolTip(qsl("%1 (%2)").arg(tr("Show/Hide errors console"), QKeySequence(Qt::CTRL | Qt::Key_8).toString(QKeySequence::NativeText)));
    viewErrorsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_8));
    connect(viewErrorsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewErrorsAction);

    QAction* viewStatsAction = new QAction(QIcon(qsl(":/icons/view-statistics.png")), tr("Statistics"), this);
    viewStatsAction->setStatusTip(tr("Generate a statistics summary display on the main profile console."));
    viewStatsAction->setToolTip(qsl("%1 (%2)").arg(tr("Generate statistics"), QKeySequence(Qt::CTRL | Qt::Key_9).toString(QKeySequence::NativeText)));
    viewStatsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_9));
    connect(viewStatsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_viewStatsAction);

    QAction* showDebugAreaAction = new QAction(QIcon(qsl(":/icons/tools-report-bug.png")), tr("Debug"), this);
    showDebugAreaAction->setStatusTip(tr("Show/Hide the separate Central Debug Console - when being displayed the system will be slower."));
    //: %1 is a keyboard shortcut, e.g. 'Ctrl+0' on Windows/Linux or '⌘0' on macOS
    showDebugAreaAction->setToolTip(
            utils::richText(tr("Show/Hide Debug Console (%1) -> system will be <b><i>slower</i></b>.").arg(QKeySequence(Qt::CTRL | Qt::Key_0).toString(QKeySequence::NativeText))));
    showDebugAreaAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(showDebugAreaAction, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleCentralDebugConsole);

    // Wording, tips and enabled state follow the chosen row: see updateEditorActivateAction()
    mpAction_toggleActive = new QAction(this);
    updateEditorActivateAction();
    connect(mpAction_toggleActive, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_triggers, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_aliases, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_timers, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_scripts, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_actions, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
    connect(treeWidget_keys, &QTreeWidget::itemActivated, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);


    // Text and tooltip of this and the three below follow the view: see updateEditorItemActionToolTips()
    mAddItem = new QAction(QIcon(qsl(":/icons/document-new.png")), QString(), this);
    mAddItem->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mAddItem->setShortcut(QKeySequence(QKeySequence::New));
    frame_left->addAction(mAddItem);
    connect(mAddItem, &QAction::triggered, this, &dlgTriggerEditor::slot_addNewItem);

    mDeleteItem = new QAction(QIcon::fromTheme(qsl(":/icons/edit-delete"), QIcon(qsl(":/icons/edit-delete.png"))), QString(), this);
    mDeleteItem->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mDeleteItem->setShortcut(QKeySequence(QKeySequence::Delete));
    frame_left->addAction(mDeleteItem);
    connect(mDeleteItem, &QAction::triggered, this, &dlgTriggerEditor::slot_deleteItemOrGroup);

    mAddGroup = new QAction(QIcon(qsl(":/icons/folder-new.png")), QString(), this);
    mAddGroup->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mAddGroup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    frame_left->addAction(mAddGroup);
    connect(mAddGroup, &QAction::triggered, this, &dlgTriggerEditor::slot_addNewGroup);

    // A grab leaves focus on the binding field, outside frame_left, and the next key is often added straight after
    mpKeysMainArea->lineEdit_key_binding->addAction(mAddItem);
    mpKeysMainArea->lineEdit_key_binding->addAction(mAddGroup);

    // 'Save Item' does not see to be translated as it is only ever used programmatically and not visible to the player
    mSaveItem = new QAction(QIcon(qsl(":/icons/document-save-as.png")), qsl("Save Item"), this);
    mSaveItem->setShortcut(QKeySequence(QKeySequence::Save));
    connect(mSaveItem, &QAction::triggered, this, &dlgTriggerEditor::slot_saveEdits);

    QAction* copyAction = new QAction(tr("Copy"), this);
    copyAction->setShortcut(QKeySequence(QKeySequence::Copy));
    // only take effect if the treeview is selected, otherwise it hijacks the shortcut from edbee
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    copyAction->setToolTip(utils::richText(tr("Copy the trigger/script/alias/etc")));
    copyAction->setStatusTip(tr("Copy the trigger/script/alias/etc"));
    treeWidget_triggers->addAction(copyAction);
    treeWidget_aliases->addAction(copyAction);
    treeWidget_timers->addAction(copyAction);
    treeWidget_scripts->addAction(copyAction);
    treeWidget_actions->addAction(copyAction);
    treeWidget_keys->addAction(copyAction);
    // A hidden widget's shortcuts do not fire, and the trees hide behind search results, so the panel carries the pair too
    frame_left->addAction(copyAction);
    mpFolderTree->addAction(copyAction);
    connect(copyAction, &QAction::triggered, this, &dlgTriggerEditor::slot_copyXml);

    QAction* pasteAction = new QAction(tr("Paste"), this);
    pasteAction->setShortcut(QKeySequence(QKeySequence::Paste));
    // only take effect if the treeview is selected, otherwise it hijacks the shortcut from edbee
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    pasteAction->setToolTip(tr("Paste triggers/scripts/aliases/etc from the clipboard"));
    pasteAction->setStatusTip(tr("Paste triggers/scripts/aliases/etc from the clipboard"));
    treeWidget_triggers->addAction(pasteAction);
    treeWidget_aliases->addAction(pasteAction);
    treeWidget_timers->addAction(pasteAction);
    treeWidget_scripts->addAction(pasteAction);
    treeWidget_actions->addAction(pasteAction);
    treeWidget_keys->addAction(pasteAction);
    frame_left->addAction(pasteAction);
    mpFolderTree->addAction(pasteAction);
    connect(pasteAction, &QAction::triggered, this, &dlgTriggerEditor::slot_pasteXml);

    // Add delete action to all tree widgets for right-click context menu
    treeWidget_triggers->addAction(mDeleteItem);
    treeWidget_aliases->addAction(mDeleteItem);
    treeWidget_timers->addAction(mDeleteItem);
    treeWidget_scripts->addAction(mDeleteItem);
    treeWidget_actions->addAction(mDeleteItem);
    treeWidget_keys->addAction(mDeleteItem);
    treeWidget_variables->addAction(mDeleteItem);
    mpFolderTree->addAction(mDeleteItem);

    // Add separators and additional actions to context menu
    QAction* separator1 = new QAction(this);
    separator1->setSeparator(true);
    QAction* separator2 = new QAction(this);
    separator2->setSeparator(true);

    // Add context menu actions to all tree widgets
    QList<QTreeWidget*> treeWidgets = {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables, mpFolderTree};

    for (QTreeWidget* widget : treeWidgets) {
        widget->addAction(mAddItem);
        widget->addAction(mAddGroup);
        widget->addAction(separator1);
        // Copy, Paste, Delete are already added above
        widget->addAction(separator2);
    }

    // Variables are left out: there is nothing there to switch
    const QList<QTreeWidget*> itemTreeWidgets = {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, mpFolderTree};
    for (QTreeWidget* widget : itemTreeWidgets) {
        widget->addAction(mpAction_toggleActive);
    }

    QAction* separator3 = new QAction(this);
    separator3->setSeparator(true);
    for (QTreeWidget* pTree : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys}) {
        pTree->addAction(separator3);
    }
    // Drag and drop is the only other way to rearrange the trees, and a screen reader user cannot drag
    auto addMoveAction = [this](const QString& text, const QKeySequence& shortcut, const KeyboardMove move) {
        auto* pAction = new QAction(text, this);
        pAction->setShortcut(shortcut);
        // Scoped to the tree, so Alt+arrows still reach the text being edited
        pAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        for (QTreeWidget* pTree : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys}) {
            pTree->addAction(pAction);
        }
        connect(pAction, &QAction::triggered, this, [this, move]() {
            moveCurrentItem(move);
        });
    };
    //: Context menu entry in the editor's item tree, moving the selected item one place up within its group
    addMoveAction(tr("Move Up"), QKeySequence(Qt::ALT | Qt::Key_Up), KeyboardMove::Up);
    //: Context menu entry in the editor's item tree, moving the selected item one place down within its group
    addMoveAction(tr("Move Down"), QKeySequence(Qt::ALT | Qt::Key_Down), KeyboardMove::Down);
    //: Context menu entry in the editor's item tree, moving the selected item out of its group to sit just after it
    addMoveAction(tr("Move Out of Group"), QKeySequence(Qt::ALT | Qt::Key_Left), KeyboardMove::OutOfGroup);
    //: Context menu entry in the editor's item tree, moving the selected item to the end of the group just above it
    addMoveAction(tr("Move Into Group Above"), QKeySequence(Qt::ALT | Qt::Key_Right), KeyboardMove::IntoGroupAbove);

    QAction* importAction = new QAction(QIcon(qsl(":/icons/import.png")), tr("Import"), this);
    importAction->setEnabled(true);
    connect(importAction, &QAction::triggered, this, &dlgTriggerEditor::slot_import);

    mpExportAction = new QAction(QIcon(qsl(":/icons/export.png")), tr("Export"), this);
    mpExportAction->setEnabled(true);
    connect(mpExportAction, &QAction::triggered, this, &dlgTriggerEditor::slot_export);

    mpCreateModuleAction = new QAction(QIcon(qsl(":/icons/package-exporter.png")), tr("Create Module"), this);
    mpCreateModuleAction->setEnabled(true);
    // Led by the button's text, which the tooltip stands in for once the bar drops its labels
    mpCreateModuleAction->setToolTip(qsl("<p>%1</p>%2").arg(mpCreateModuleAction->text(), tr("<p>Create a module from selected items</p>")));
    connect(mpCreateModuleAction, &QAction::triggered, this, &dlgTriggerEditor::slot_createModule);

    mProfileSaveAction = new QAction(QIcon(qsl(":/icons/document-save-all.png")), tr("Save Profile"), this);
    mProfileSaveAction->setToolTip(qsl("<p>%1 (%2)</p>%3")
                                           .arg(mProfileSaveAction->text(),
                                                QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S).toString(QKeySequence::NativeText),
                                                tr("<p>Saves your entire profile (triggers, aliases, scripts, timers, buttons and "
                                                   "keys, but not the map or script-specific settings) to your computer disk, so "
                                                   "in case of a computer or program crash, all changes you have done will be "
                                                   "retained.</p>"
                                                   "<p>It also makes a backup of your profile, you can load an older version of it "
                                                   "when connecting.</p>"
                                                   "<p>Should there be any modules that are marked to be \"<i>synced</i>\" this will "
                                                   "also cause them to be saved and reloaded into other profiles if they too are "
                                                   "active.</p>")));
    mProfileSaveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    //: Status tip for saving profile
    mProfileSaveAction->setStatusTip(tr("Save profile (triggers, aliases, scripts, timers, buttons, keys - not the map) and synchronize modules."));

    mProfileSaveAsAction = new QAction(QIcon(qsl(":/icons/utilities-file-archiver.png")), tr("Save Profile As"), this);

    if (mpHost->mLoadedOk) {
        connect(mProfileSaveAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAction);
        connect(mProfileSaveAsAction, &QAction::triggered, this, &dlgTriggerEditor::slot_profileSaveAsAction);
    } else {
        mProfileSaveAction->setDisabled(true);
        mProfileSaveAsAction->setDisabled(true);
        auto disabledSaving = tr("Something went wrong loading your Mudlet profile and it could not be loaded. "
                                 "Try loading an older version in 'Connect - Options - Profile history'");
        mProfileSaveAction->setToolTip(disabledSaving);
        mProfileSaveAsAction->setToolTip(disabledSaving);
    }

    auto* nextSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this);
    QObject::connect(nextSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_nextSection);

    QShortcut* previousSectionShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), this);
    connect(previousSectionShortcut, &QShortcut::activated, this, &dlgTriggerEditor::slot_previousSection);

    mpShortcutActivateMainWindow = new QShortcut(this);
    connect(mpShortcutActivateMainWindow, &QShortcut::activated, this, &dlgTriggerEditor::slot_activateMainWindow);
    slot_updateActivateMainWindowShortcut();
    connect(mudlet::self(), &mudlet::signal_shortcutsChanged, this, &dlgTriggerEditor::slot_updateActivateMainWindowShortcut);

    toolBar = new QToolBar();

    connect(mudlet::self(), &mudlet::signal_setToolBarIconSize, this, &dlgTriggerEditor::slot_setToolBarIconSize);
    connect(mudlet::self(), &mudlet::signal_setTreeIconSize, this, &dlgTriggerEditor::slot_setTreeWidgetIconSize);
    connect(mudlet::self(), &mudlet::signal_colouredIconsChanged, this, &dlgTriggerEditor::slot_colouredIconsChanged);
    slot_setToolBarIconSize(mudlet::self()->mToolbarIconSize);
    slot_setTreeWidgetIconSize(mudlet::self()->mEditorTreeWidgetIconSize);

    toolBar->setMovable(true);
    toolBar->setObjectName(qsl("editorActionsToolbar"));
    //: This is the toolbar that is initially placed at the top of the editor.
    toolBar->setWindowTitle(tr("Editor Toolbar - %1 - Actions").arg(hostName));

    toolBar->addAction(mAddItem);
    toolBar->addAction(mAddGroup);
    toolBar->addAction(mSaveItem);
    toolBar->addAction(mpAction_toggleActive);
    toolBar->addAction(mDeleteItem);
    // Add smart undo/redo toolbar buttons (route based on focus)
    toolBar->addAction(mpUndoAction);
    toolBar->addAction(mpRedoAction);

    // QToolBar has no alignment, so an expanding blank separates the item half from the profile half
    auto* pToolBarSpacer = new QWidget(toolBar);
    pToolBarSpacer->setObjectName(qsl("editorToolbarSpacer"));
    pToolBarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(pToolBarSpacer);

    toolBar->addAction(importAction);
    toolBar->addAction(mpExportAction);
    toolBar->addAction(mpCreateModuleAction);
    toolBar->addAction(mProfileSaveAction);

    mEditorActionGlyphs = {{mAddItem, qsl(":/icons/editor-add.svg"), QString(), uiDesign::scmIdentity_add},
                           {mAddGroup, qsl(":/icons/editor-add-group.svg"), QString(), uiDesign::scmIdentity_addGroup},
                           {mSaveItem, qsl(":/icons/editor-save-item.svg")},
                           {mpAction_toggleActive, qsl(":/icons/editor-activate.svg"), QString(), uiDesign::scmIdentity_activate},
                           {mDeleteItem, qsl(":/icons/editor-delete.svg"), QString(), uiDesign::scmIdentity_delete},
                           {mpUndoAction, qsl(":/icons/editor-undo.svg")},
                           {mpRedoAction, qsl(":/icons/editor-redo.svg")},
                           {importAction, qsl(":/icons/editor-import.svg"), QString(), uiDesign::scmIdentity_transfer},
                           {mpExportAction, qsl(":/icons/editor-export.svg"), QString(), uiDesign::scmIdentity_transfer},
                           {mpCreateModuleAction, qsl(":/icons/editor-module.svg"), QString(), uiDesign::scmIdentity_packages},
                           {mProfileSaveAction, qsl(":/icons/editor-save-profile.svg")},
                           // Its old picture was a brown archive box, Save Profile's a grey disk
                           {mProfileSaveAsAction, qsl(":/icons/editor-save-profile.svg"), QString(), uiDesign::scmIdentity_packages},
                           // Menu-only, and reached from the trees rather than the toolbar
                           {copyAction, qsl(":/icons/editor-copy.svg")},
                           {pasteAction, qsl(":/icons/editor-paste.svg")}};

    if (auto* pButton_saveProfile = qobject_cast<QToolButton*>(toolBar->widgetForAction(mProfileSaveAction))) {
        auto* pMenu_saveProfile = new QMenu(pButton_saveProfile);
        pMenu_saveProfile->addAction(mProfileSaveAsAction);
        pButton_saveProfile->setMenu(pMenu_saveProfile);
        pButton_saveProfile->setPopupMode(QToolButton::MenuButtonPopup);
        // Named so the stylesheet's split-button padding reaches this button and no other
        pButton_saveProfile->setObjectName(qsl("editorSaveProfileButton"));
    }

    // In the order the groups give up their labels: profile actions first, item actions last
    mEditorToolBarGroups.append(EditorToolBarGroup{{importAction, mpExportAction, mpCreateModuleAction, mProfileSaveAction}, true});
    mEditorToolBarGroups.append(EditorToolBarGroup{{mAddItem, mAddGroup, mSaveItem, mpAction_toggleActive, mDeleteItem}, true});
    // The fit runs on the bar's own resize, the one event that reports its room changing
    toolBar->installEventFilter(this);

    applyEditorToolbarButtonStyles();

    connect(checkBox_displayAllVariables, &QAbstractButton::toggled, this, &dlgTriggerEditor::slot_toggleHiddenVariables);

    connect(mpVarsMainArea->checkBox_variable_hidden, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_hideVariable);

    buildEditorSidebar();
    addEditorSidebarRow(viewTriggerAction, EditorViewType::cmTriggerView, qsl(":/icons/editor-triggers.svg"));
    addEditorSidebarRow(viewAliasAction, EditorViewType::cmAliasView, qsl(":/icons/editor-aliases.svg"));
    addEditorSidebarRow(viewScriptsAction, EditorViewType::cmScriptView, qsl(":/icons/editor-scripts.svg"));
    addEditorSidebarRow(showTimersAction, EditorViewType::cmTimerView, qsl(":/icons/editor-timers.svg"));
    addEditorSidebarRow(viewKeysAction, EditorViewType::cmKeysView, qsl(":/icons/editor-keys.svg"));
    addEditorSidebarRow(viewActionAction, EditorViewType::cmActionView, qsl(":/icons/editor-buttons.svg"));
    addEditorSidebarRow(viewVarsAction, EditorViewType::cmVarsView, qsl(":/icons/editor-variables.svg"));
    mpSidebarItem_folders = addEditorSidebarRow(mpViewFoldersAction, EditorViewType::cmUnknownView, qsl(":/icons/editor-folder.svg"));
    mpSidebarItem_folders->setData(scmRole_editorSidebarFolders, true);
    mpSidebarItem_folders->setHidden(!mpViewFoldersAction->isVisible());
    addEditorSidebarSeparator();
    addEditorSidebarRow(viewErrorsAction, EditorViewType::cmUnknownView, qsl(":/icons/editor-errors.svg"));
    addEditorSidebarRow(viewStatsAction, EditorViewType::cmUnknownView, qsl(":/icons/editor-statistics.svg"));
    addEditorSidebarRow(showDebugAreaAction, EditorViewType::cmUnknownView, qsl(":/icons/editor-debug.svg"));

    // A sidebar row cannot hold an action, so the window does, or the Ctrl+1 to Ctrl+0 shortcuts are unreachable
    addActions(mEditorViewActions);

    // Taken rather than reparented, or the main window would keep a central widget that has gone elsewhere
    auto* pShell = new QWidget(this);
    pShell->setObjectName(qsl("editorShell"));
    auto* pShellLayout = new QHBoxLayout(pShell);
    pShellLayout->setContentsMargins(0, 0, 0, 0);
    pShellLayout->setSpacing(0);
    pShellLayout->addWidget(mpWidget_editorSidebarPane);
    if (QWidget* pEditorBody = QMainWindow::takeCentralWidget()) {
        pShellLayout->addWidget(pEditorBody, 1);
        // Taking it made it a window of its own, which hid it
        pEditorBody->show();
    }
    QMainWindow::setCentralWidget(pShell);

    // A child of the shell, as it rides the seam between sidebar and body; not of the toolbar, which can be moved or floated
    mpToggle_editorSidebar = new uiDesign::SidebarToggle(mpWidget_editorSidebarPane, pShell);
    connect(mpToggle_editorSidebar, &QAbstractButton::clicked, this, [this]() {
        setEditorSidebarLabelsShown(!mEditorSidebarLabelsShown);
    });
    updateEditorSidebarToggle();

    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar);

    // (Top) "Actions" toolbar:
    //: This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.
    mpAction_restoreEditorActionsToolbar = new QAction(tr("Restore Actions toolbar"), this);

    connect(mpAction_restoreEditorActionsToolbar, &QAction::triggered, this, &dlgTriggerEditor::slot_restoreEditorActionsToolbar);
    connect(toolBar, &QToolBar::visibilityChanged, this, &dlgTriggerEditor::slot_visibilityChangedEditorActionsToolbar);
    connect(toolBar, &QToolBar::topLevelChanged, this, &dlgTriggerEditor::slot_floatingChangedEditorActionsToolbar);

    treeWidget_triggers->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_aliases->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_timers->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_scripts->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_actions->addAction(mpAction_restoreEditorActionsToolbar);
    treeWidget_keys->addAction(mpAction_restoreEditorActionsToolbar);
    mpFolderTree->addAction(mpAction_restoreEditorActionsToolbar);

    // Only needed once the toolbar is hidden, and it starts visible
    mpAction_restoreEditorActionsToolbar->setVisible(false);

    setupEditorPanel();
    applyEditorShellStyle();

    // Adding, deleting and activating all reach the tree through its model, the one place to track the counts from
    for (QTreeWidget* pTreeWidget : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys}) {
        connect(pTreeWidget->model(), &QAbstractItemModel::rowsInserted, this, &dlgTriggerEditor::scheduleEditorItemCountUpdate);
        connect(pTreeWidget->model(), &QAbstractItemModel::rowsRemoved, this, &dlgTriggerEditor::scheduleEditorItemCountUpdate);
        connect(pTreeWidget->model(), &QAbstractItemModel::dataChanged, this, &dlgTriggerEditor::scheduleEditorItemCountUpdate);
    }

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setThemeName(mpHost->getEditorTheme());
    config->setFont(mpHost->getEditorFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    // Every document handed to edbee brings its own config, so this is repeated per document
    config->setShowCaretOffset(false);
    config->endChanges();

    connect(comboBox_searchTerms, qOverload<int>(&QComboBox::activated), this, &dlgTriggerEditor::slot_searchMudletItems);

    // The selection slots reload the form unconditionally, so a clicked() on the already loaded row would discard
    // unsaved edits; itemSelectionChanged() has loaded a newly clicked row before clicked() arrives
    auto connectSelectionOnClick = [this](QTreeWidget* pTreeWidget, QTreeWidgetItem* const& pCurrentItem, void (dlgTriggerEditor::*pSelectionSlot)(QTreeWidgetItem*)) {
        connect(pTreeWidget, &QTreeWidget::itemClicked, this, [this, &pCurrentItem, pSelectionSlot](QTreeWidgetItem* pItem) {
            if (pItem != pCurrentItem) {
                (this->*pSelectionSlot)(pItem);
            }
        });
    };
    connectSelectionOnClick(treeWidget_triggers, mpCurrentTriggerItem, &dlgTriggerEditor::slot_triggerSelected);
    connect(treeWidget_triggers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_triggers, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_triggers, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_triggers, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connectSelectionOnClick(treeWidget_keys, mpCurrentKeyItem, &dlgTriggerEditor::slot_keySelected);
    connect(treeWidget_keys, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_keys, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_keys, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_keys, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connectSelectionOnClick(treeWidget_timers, mpCurrentTimerItem, &dlgTriggerEditor::slot_timerSelected);
    connect(treeWidget_timers, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_timers, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_timers, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_timers, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connectSelectionOnClick(treeWidget_scripts, mpCurrentScriptItem, &dlgTriggerEditor::slot_scriptsSelected);
    connect(treeWidget_scripts, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_scripts, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_scripts, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_scripts, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connectSelectionOnClick(treeWidget_aliases, mpCurrentAliasItem, &dlgTriggerEditor::slot_aliasSelected);
    connect(treeWidget_aliases, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_aliases, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_aliases, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_aliases, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connectSelectionOnClick(treeWidget_actions, mpCurrentActionItem, &dlgTriggerEditor::slot_actionSelected);
    connect(treeWidget_actions, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_actions, &TTreeWidget::itemMoved, this, &dlgTriggerEditor::slot_itemMoved);
    connect(treeWidget_actions, &TTreeWidget::batchMoveStarted, this, &dlgTriggerEditor::slot_batchMoveStarted);
    connect(treeWidget_actions, &TTreeWidget::batchMoveEnded, this, &dlgTriggerEditor::slot_batchMoveEnded);
    connect(treeWidget_variables, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_variableSelected);
    connect(treeWidget_variables, &QTreeWidget::itemChanged, this, &dlgTriggerEditor::slot_variableChanged);
    connect(treeWidget_variables, &QTreeWidget::itemSelectionChanged, this, &dlgTriggerEditor::slot_treeSelectionChanged);
    connect(treeWidget_searchResults, &QTreeWidget::itemClicked, this, &dlgTriggerEditor::slot_itemSelectedInSearchResults);

    // triggers
    connect(mpTriggersMainArea->lineEdit_trigger_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpTriggersMainArea->lineEdit_trigger_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for triggers (creates individual undo entries)
    connect(mpTriggersMainArea->lineEdit_trigger_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_TriggerName);
    connect(mpTriggersMainArea->lineEdit_trigger_command, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_TriggerCommand);
    connect(mpTriggersMainArea->spinBox_stayOpen, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_saveProperty_TriggerStayOpen);
    connect(mpTriggersMainArea->spinBox_lineMargin, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_saveProperty_TriggerLineMargin);
    connect(mpTriggersMainArea->checkBox_filterTrigger, &QCheckBox::toggled, this, &dlgTriggerEditor::slot_saveProperty_TriggerFilterTrigger);
    connect(mpTriggersMainArea->checkBox_perlSlashGOption, &QCheckBox::toggled, this, &dlgTriggerEditor::slot_saveProperty_TriggerPerlSlashG);
    connect(mpTriggersMainArea->checkBox_soundTrigger, &QCheckBox::toggled, this, &dlgTriggerEditor::slot_saveProperty_TriggerSoundEnabled);
    connect(mpTriggersMainArea->checkBox_triggerColorizer, &QCheckBox::toggled, this, &dlgTriggerEditor::slot_saveProperty_TriggerColorizer);

    // aliases
    connect(mpAliasMainArea->lineEdit_alias_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpAliasMainArea->lineEdit_alias_pattern, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpAliasMainArea->lineEdit_alias_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for aliases
    connect(mpAliasMainArea->lineEdit_alias_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_AliasName);
    connect(mpAliasMainArea->lineEdit_alias_pattern, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_AliasPattern);
    connect(mpAliasMainArea->lineEdit_alias_command, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_AliasCommand);

    // scripts
    connect(mpScriptsMainArea->lineEdit_script_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for scripts
    connect(mpScriptsMainArea->lineEdit_script_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_ScriptName);

    // timers
    connect(mpTimersMainArea->lineEdit_timer_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpTimersMainArea->lineEdit_timer_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for timers
    connect(mpTimersMainArea->lineEdit_timer_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_TimerName);
    connect(mpTimersMainArea->lineEdit_timer_command, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_TimerCommand);
    connect(mpTimersMainArea->timeEdit_timer_hours, &QTimeEdit::timeChanged, this, &dlgTriggerEditor::slot_saveProperty_TimerTime);
    connect(mpTimersMainArea->timeEdit_timer_minutes, &QTimeEdit::timeChanged, this, &dlgTriggerEditor::slot_saveProperty_TimerTime);
    connect(mpTimersMainArea->timeEdit_timer_seconds, &QTimeEdit::timeChanged, this, &dlgTriggerEditor::slot_saveProperty_TimerTime);
    connect(mpTimersMainArea->timeEdit_timer_msecs, &QTimeEdit::timeChanged, this, &dlgTriggerEditor::slot_saveProperty_TimerTime);

    // keys
    connect(mpKeysMainArea->lineEdit_key_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpKeysMainArea->lineEdit_key_command, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for keys
    connect(mpKeysMainArea->lineEdit_key_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_KeyName);
    connect(mpKeysMainArea->lineEdit_key_command, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_KeyCommand);

    // buttons
    connect(mpActionsMainArea->lineEdit_action_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpActionsMainArea->lineEdit_action_name, &QLineEdit::textEdited, this, &dlgTriggerEditor::slot_itemEdited);

    // Per-property immediate saves for actions (buttons)
    connect(mpActionsMainArea->lineEdit_action_name, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_ActionName);
    connect(mpActionsMainArea->lineEdit_action_button_command_down, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_ActionCommandDown);
    connect(mpActionsMainArea->lineEdit_action_button_command_up, &QLineEdit::editingFinished, this, &dlgTriggerEditor::slot_saveProperty_ActionCommandUp);
    connect(mpActionsMainArea->checkBox_action_button_isPushDown, &QCheckBox::toggled, this, &dlgTriggerEditor::slot_saveProperty_ActionIsPushDown);
    connect(mpActionsMainArea->spinBox_action_bar_columns, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_saveProperty_ActionBarColumns);
    connect(mpActionsMainArea->spinBox_action_bar_offsetToFirstButton, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_saveProperty_ActionBarFillerOffset);
    connect(mpActionsMainArea->comboBox_action_bar_orientation, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_saveProperty_ActionBarOrientation);
    connect(mpActionsMainArea->comboBox_action_bar_location, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_saveProperty_ActionBarLocation);
    connect(mpActionsMainArea->comboBox_action_button_rotation, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_saveProperty_ActionButtonRotation);
    connect(mpActionsMainArea->plainTextEdit_action_css, &QPlainTextEdit::textChanged, this, &dlgTriggerEditor::slot_saveProperty_ActionCSS);

    // No handler needed: an emptied field puts the results away - see the textChanged() below
    comboBox_searchTerms->lineEdit()->setClearButtonEnabled(true);
    auto pLineEdit_searchTerm = comboBox_searchTerms->lineEdit();
    //: Placeholder in the editor's search field, which searches the profile's own triggers, aliases, scripts and the rest
    pLineEdit_searchTerm->setPlaceholderText(tr("Search items..."));

    mpAction_searchOptions = new QAction(tr("Search Options"), this);
    mpAction_searchOptions->setObjectName(qsl("mpAction_searchOptions"));

    QMenu* pMenu_searchOptions = new QMenu(tr("Search Options"), this);
    pMenu_searchOptions->setObjectName(qsl("pMenu_searchOptions"));
    pMenu_searchOptions->setToolTipsVisible(true);

    mpAction_searchCaseSensitive = new QAction(tr("Case sensitive"), this);
    mpAction_searchCaseSensitive->setObjectName(qsl("mpAction_searchCaseSensitive"));
    mpAction_searchCaseSensitive->setToolTip(utils::richText(tr("Match case precisely")));
    mpAction_searchCaseSensitive->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchCaseSensitive);

    mpAction_searchIncludeVariables = new QAction(tr("Include variables"), this);
    mpAction_searchIncludeVariables->setObjectName(qsl("mpAction_searchIncludeVariables"));
    mpAction_searchIncludeVariables->setToolTip(utils::richText(tr("Search variables (slower)")));
    mpAction_searchIncludeVariables->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchIncludeVariables);

    mpAction_searchWholeWord = new QAction(tr("Whole word"), this);
    mpAction_searchWholeWord->setObjectName(qsl("mpAction_searchWholeWord"));
    mpAction_searchWholeWord->setToolTip(utils::richText(tr("Only match whole words")));
    mpAction_searchWholeWord->setCheckable(true);
    pMenu_searchOptions->insertAction(nullptr, mpAction_searchWholeWord);

    // This will set the icon and the Search Options menu items - and needs to
    // be done BEFORE the menu items are connect()ed:
    setSearchOptions(mSearchOptions);

    connect(mpAction_searchCaseSensitive, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchCaseSensitivity);
    connect(mpAction_searchIncludeVariables, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchIncludeVariables);
    connect(mpAction_searchWholeWord, &QAction::triggered, this, &dlgTriggerEditor::slot_toggleSearchWholeWord);


    mpAction_searchOptions->setMenu(pMenu_searchOptions);
    styleSearchOptionsMenu();

    pLineEdit_searchTerm->addAction(mpAction_searchOptions, QLineEdit::LeadingPosition);

    // The stylesheet gives the combo box's own drop-down no width, so this is the mouse's way into the history
    mpAction_searchHistory = pLineEdit_searchTerm->addAction(QIcon(), QLineEdit::TrailingPosition);
    mpAction_searchHistory->setObjectName(qsl("mpAction_searchHistory"));
    //: Tooltip on the small chevron at the right of the editor's search field, which reopens what was searched for before
    mpAction_searchHistory->setToolTip(utils::richText(tr("Recent searches")));
    connect(mpAction_searchHistory, &QAction::triggered, this, [this]() {
        comboBox_searchTerms->showPopup();
    });
    connect(comboBox_searchTerms->model(), &QAbstractItemModel::rowsInserted, this, &dlgTriggerEditor::updateSearchHistoryAction);
    connect(comboBox_searchTerms->model(), &QAbstractItemModel::rowsRemoved, this, &dlgTriggerEditor::updateSearchHistoryAction);
    updateSearchHistoryAction();

    mpTriggersMainArea->hide();
    mpTimersMainArea->hide();
    mpScriptsMainArea->hide();
    mpAliasMainArea->hide();
    mpActionsMainArea->hide();
    mpKeysMainArea->hide();
    mpVarsMainArea->hide();

    mpSourceEditorArea->hide();

    clearEditorNotification();

    treeWidget_triggers->show();
    treeWidget_aliases->hide();
    treeWidget_actions->hide();
    treeWidget_timers->hide();
    treeWidget_scripts->hide();
    treeWidget_keys->hide();
    treeWidget_variables->hide();

    // Qt only passes a hidden widget's layout invalidation up while it is still visible, so the containers keep
    // the ~1600px minimum of all seven areas stacked; with no event loop before readSettings(), the editor
    // would open at screen height
    const QList<QWidget*> justHidden{mpTriggersMainArea,
                                     mpTimersMainArea,
                                     mpScriptsMainArea,
                                     mpAliasMainArea,
                                     mpActionsMainArea,
                                     mpKeysMainArea,
                                     mpVarsMainArea,
                                     mpSourceEditorArea,
                                     treeWidget_aliases,
                                     treeWidget_actions,
                                     treeWidget_timers,
                                     treeWidget_scripts,
                                     treeWidget_keys,
                                     treeWidget_variables};
    for (const QWidget* pWidget : justHidden) {
        for (QWidget* pContainer = pWidget->parentWidget(); pContainer; pContainer = pContainer->parentWidget()) {
            pContainer->updateGeometry();
            if (pContainer == this) {
                break;
            }
        }
    }

    readSettings();

    // After readSettings(): on macOS this creates the native window at its current size, and cocoa drops a resize
    // of a native window that is created but not yet shown
    setUnifiedTitleAndToolBarOnMac(true);

    // A disclosure, not a preference: it opens closed whatever readSettings() restored
    slot_showAllTriggerControls(false);

    comboBox_searchTerms->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QFrame* searchContainer = new QFrame();
    mpWidget_searchResultsPane = searchContainer;
    QVBoxLayout* searchLayout = new QVBoxLayout(searchContainer);
    searchLayout->addWidget(treeWidget_searchResults);

    verticalLayout_frame_left->removeWidget(widget_searchTerm);
    verticalLayout_frame_left->insertWidget(0, widget_searchTerm);
    // On the row's own layout, since the pane behind it is painted to the top of the window
    if (QLayout* pSearchRowLayout = widget_searchTerm->layout()) {
        const QMargins searchRowMargins = pSearchRowLayout->contentsMargins();
        pSearchRowLayout->setContentsMargins(searchRowMargins.left(), scmEditorColumnTopInset, searchRowMargins.right(), searchRowMargins.bottom());
    }

    // Outside the trees pane, so it stays reachable while search results show, since it narrows them too. A layout
    // rather than a widget, so the row takes no height while the switch is hidden.
    verticalLayout_frame_left->removeWidget(checkBox_displayAllVariables);
    auto* pHiddenVariablesRow = new QHBoxLayout();
    if (QLayout* pSearchRowLayout = widget_searchTerm->layout()) {
        const QMargins searchRowMargins = pSearchRowLayout->contentsMargins();
        pHiddenVariablesRow->setContentsMargins(searchRowMargins.left(), 0, searchRowMargins.right(), 0);
    }
    pHiddenVariablesRow->addWidget(checkBox_displayAllVariables);
    mpLabel_hiddenVariablesCount = new QLabel(this);
    mpLabel_hiddenVariablesCount->setObjectName(qsl("editorHiddenVariablesCount"));
    mpLabel_hiddenVariablesCount->setFont(uiDesign::chipFont(this));
    mpLabel_hiddenVariablesCount->hide();
    pHiddenVariablesRow->addStretch(1);
    pHiddenVariablesRow->addWidget(mpLabel_hiddenVariablesCount);
    verticalLayout_frame_left->insertLayout(1, pHiddenVariablesRow);

    searchSplitter = new QSplitter(Qt::Vertical);

    QFrame* itemContainer = new QFrame();
    mpWidget_itemTreesPane = itemContainer;
    QVBoxLayout* itemLayout = new QVBoxLayout(itemContainer);

    itemLayout->addWidget(treeWidget_triggers);
    itemLayout->addWidget(treeWidget_aliases);
    itemLayout->addWidget(treeWidget_actions);
    itemLayout->addWidget(treeWidget_timers);
    itemLayout->addWidget(treeWidget_scripts);
    itemLayout->addWidget(treeWidget_keys);
    itemLayout->addWidget(treeWidget_variables);
    itemLayout->addWidget(mpFolderTree);

    // No stretch factors: one pane is always hidden, and a splitter gives all its room to the visible one
    searchSplitter->addWidget(itemContainer);
    searchSplitter->addWidget(searchContainer);

    verticalLayout_frame_left->addWidget(searchSplitter);
    verticalLayout_frame_left->setStretchFactor(searchSplitter, 1);

    setSearchResultsShown(false);

    // Escape gives the panel back to the trees; filtered on the line edit, not the combo box, so an open history
    // popup takes the key first and closes without clearing the search
    comboBox_searchTerms->lineEdit()->installEventFilter(this);
    connect(comboBox_searchTerms->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            slot_clearSearchResults();
        }
    });

    // The options strip rewraps on the form's resize, the one event that reports its width
    mpTriggersMainArea->installEventFilter(this);

    mpScrollArea = mpTriggersMainArea->scrollArea;
    // QAbstractScrollArea fills its viewport with QPalette::Base, which drew the pattern rows as a field-coloured box
    mpScrollArea->setObjectName(qsl("editorPatternScroll"));
    mpScrollArea->setFrameShape(QFrame::NoFrame);
    mpScrollArea->viewport()->setAutoFillBackground(false);
    mpWidget_triggerItems = new QWidget;
    mpWidget_triggerItems->setObjectName(qsl("editorPatternList"));
    auto lay1 = new QVBoxLayout(mpWidget_triggerItems);
    lay1->setContentsMargins(0, 0, 0, 0);
    lay1->setSpacing(0);
    mpScrollArea->setWidget(mpWidget_triggerItems);
    // After setWidget(), which turns autofill on and hands the widget the viewport's Base role; the stylesheet rule
    // from applyEditorShellStyle() can lose the race to the first paint or be skipped by its early return
    mpWidget_triggerItems->setAutoFillBackground(false);

    lay1->addStretch();

    mPatternList << tr("substring") << tr("perl regex") << tr("start of line") << tr("exact match") << tr("lua function") << tr("line spacer") << tr("color trigger") << tr("prompt");

    restylePatternTypeIcons();

    setupAddPatternButton();

    showPatternItems(1);
    setupPatternNavigationShortcuts();
    updatePatternTabOrder();

    connect(mpHost, &Host::signal_editorThemeChanged, this, &dlgTriggerEditor::slot_editorThemeChanged);
    connect(mpHost, &Host::signal_editorCleanResetRequested, this, &dlgTriggerEditor::doCleanReset);
    connect(mpHost, &Host::signal_profileStyleSheetChanged, this, &dlgTriggerEditor::setStyleSheet);
    connect(mpHost, &Host::signal_editorFontChanged, this, &dlgTriggerEditor::setEditorFont);
    // Patterns are game text, read in the display font rather than the code font, and every row measurement comes off its metrics
    connect(mpHost, &Host::signal_consoleFontChanged, this, [this]() {
        for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
            applyPatternWidgetStyle(patternEdit);
        }
    });
    connect(mpHost, &Host::signal_editorSearchOptionsChanged, this, &dlgTriggerEditor::setSearchOptions);
    connect(mpHost, &Host::signal_editorShowBidiChanged, this, &dlgTriggerEditor::setEditorShowBidi);
    connect(mpHost, &Host::signal_showIdsInEditorChanged, this, &dlgTriggerEditor::showIDLabels);
    connect(mpHost, &Host::signal_folderViewInEditorChanged, this, &dlgTriggerEditor::setFolderViewAvailable);
    connect(mpHost, &Host::signal_triggerToggled, this, &dlgTriggerEditor::refreshTriggerIcon);
    connect(mpHost, &Host::signal_aliasToggled, this, &dlgTriggerEditor::refreshAliasIcon);
    connect(mpHost, &Host::signal_timerToggled, this, &dlgTriggerEditor::refreshTimerIcon);
    connect(mpHost, &Host::signal_keyToggled, this, &dlgTriggerEditor::refreshKeyIcon);
    connect(mpHost, &Host::signal_scriptToggled, this, &dlgTriggerEditor::refreshScriptIcon);
    // The repaint above waits until the window is shown; the wording of Activate must not
    connect(mpHost, &Host::signal_triggerToggled, this, &dlgTriggerEditor::updateEditorActivateAction);
    connect(mpHost, &Host::signal_aliasToggled, this, &dlgTriggerEditor::updateEditorActivateAction);
    connect(mpHost, &Host::signal_timerToggled, this, &dlgTriggerEditor::updateEditorActivateAction);
    connect(mpHost, &Host::signal_keyToggled, this, &dlgTriggerEditor::updateEditorActivateAction);
    connect(mpHost, &Host::signal_scriptToggled, this, &dlgTriggerEditor::updateEditorActivateAction);
    connect(mpHost, &Host::signal_scriptCodeChanged, this, &dlgTriggerEditor::writeScript);
    connect(mpHost, &Host::signal_itemsChangedByScript, this, [this]() {
        mNeedUpdateData = true;
    });
    // The dots read the stop as they paint; what a screen reader is told is written into the rows, so those are rebuilt
    connect(mpHost, &Host::signal_emergencyStopChanged, this, [this]() {
        mNeedUpdateData = true;
        for (QTreeWidget* pTree : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_keys, mpFolderTree}) {
            pTree->viewport()->update();
        }
    });
    connect(mpHost, &Host::signal_errorConsolePrint, this, [this](const QString& text, const QColor& fgColor, const QColor& bgColor) {
        mpErrorConsole->print(text, fgColor, bgColor);
    });
    // fire this now as the theme has already been set and we need the syntax highlighter to pick it up
    mpHost->editorThemeChanged();

    const int triggerWidgetItemMinHeight = qRound(mTriggerPatternEdit.at(0)->minimumSizeHint().height() * 1.5);
    mpScrollArea->setMinimumHeight(triggerWidgetItemMinHeight);

    widget_searchTerm->updateGeometry();

    showIDLabels(mpHost->showIdsInEditor());
    if (mAutosaveInterval > 0) {
        startTimer(mAutosaveInterval * 1min);
    }
    // Built after the profile load's reset, so it starts as that reset leaves an editor that was never shown
    runScheduledCleanReset();
}

// Shared by every edbee widget, the preferences preview included
void dlgTriggerEditor::ensureLuaAutoCompleteProvider()
{
    if (smAutoCompleteInitialized) {
        return;
    }
    auto* provider = new edbee::StringTextAutoCompleteProvider();

    // Add lua functions and reserved lua terms to an AutoComplete provider
    for (const QString& key : mudlet::smLuaFunctionNames.keys()) {
        provider->add(key, 3, mudlet::smLuaFunctionNames.value(key).toString());
    }

    // Lua reserved keywords (highest priority for basic syntax)
    provider->add(qsl("and"), 14);
    provider->add(qsl("break"), 14);
    provider->add(qsl("else"), 14);
    provider->add(qsl("elseif"), 14);
    provider->add(qsl("end"), 14);
    provider->add(qsl("false"), 14);
    provider->add(qsl("for"), 14);
    provider->add(qsl("function"), 14);
    provider->add(qsl("goto"), 14);
    provider->add(qsl("local"), 14);
    provider->add(qsl("nil"), 14);
    provider->add(qsl("not"), 14);
    provider->add(qsl("repeat"), 14);
    provider->add(qsl("return"), 14);
    provider->add(qsl("then"), 14);
    provider->add(qsl("true"), 14);
    provider->add(qsl("until"), 14);
    provider->add(qsl("while"), 14);

    // Standard Lua library functions (priority 4 - between Mudlet functions and keywords)
    // String library
    provider->add(qsl("string.byte"), 4, qsl("string.byte(s [, i [, j]])"));
    provider->add(qsl("string.char"), 4, qsl("string.char(...)"));
    provider->add(qsl("string.dump"), 4, qsl("string.dump(function)"));
    provider->add(qsl("string.find"), 4, qsl("string.find(s, pattern [, init [, plain]])"));
    provider->add(qsl("string.format"), 4, qsl("string.format(formatstring, ...)"));
    provider->add(qsl("string.gmatch"), 4, qsl("string.gmatch(s, pattern)"));
    provider->add(qsl("string.gsub"), 4, qsl("string.gsub(s, pattern, repl [, n])"));
    provider->add(qsl("string.len"), 4, qsl("string.len(s)"));
    provider->add(qsl("string.lower"), 4, qsl("string.lower(s)"));
    provider->add(qsl("string.match"), 4, qsl("string.match(s, pattern [, init])"));
    provider->add(qsl("string.rep"), 4, qsl("string.rep(s, n)"));
    provider->add(qsl("string.reverse"), 4, qsl("string.reverse(s)"));
    provider->add(qsl("string.sub"), 4, qsl("string.sub(s, i [, j])"));
    provider->add(qsl("string.upper"), 4, qsl("string.upper(s)"));

    // Table library
    provider->add(qsl("table.concat"), 4, qsl("table.concat(list [, sep [, i [, j]]])"));
    provider->add(qsl("table.insert"), 4, qsl("table.insert(list, [pos,] value)"));
    provider->add(qsl("table.pack"), 4, qsl("table.pack(...)"));
    provider->add(qsl("table.remove"), 4, qsl("table.remove(list [, pos])"));
    provider->add(qsl("table.sort"), 4, qsl("table.sort(list [, comp])"));
    provider->add(qsl("table.unpack"), 4, qsl("table.unpack(list [, i [, j]])"));

    // Math library
    provider->add(qsl("math.abs"), 4, qsl("math.abs(x)"));
    provider->add(qsl("math.acos"), 4, qsl("math.acos(x)"));
    provider->add(qsl("math.asin"), 4, qsl("math.asin(x)"));
    provider->add(qsl("math.atan"), 4, qsl("math.atan(x)"));
    provider->add(qsl("math.atan2"), 4, qsl("math.atan2(y, x)"));
    provider->add(qsl("math.ceil"), 4, qsl("math.ceil(x)"));
    provider->add(qsl("math.cos"), 4, qsl("math.cos(x)"));
    provider->add(qsl("math.cosh"), 4, qsl("math.cosh(x)"));
    provider->add(qsl("math.deg"), 4, qsl("math.deg(x)"));
    provider->add(qsl("math.exp"), 4, qsl("math.exp(x)"));
    provider->add(qsl("math.floor"), 4, qsl("math.floor(x)"));
    provider->add(qsl("math.fmod"), 4, qsl("math.fmod(x, y)"));
    provider->add(qsl("math.frexp"), 4, qsl("math.frexp(x)"));
    provider->add(qsl("math.huge"), 4, qsl("math.huge"));
    provider->add(qsl("math.ldexp"), 4, qsl("math.ldexp(m, e)"));
    provider->add(qsl("math.log"), 4, qsl("math.log(x [, base])"));
    provider->add(qsl("math.log10"), 4, qsl("math.log10(x)"));
    provider->add(qsl("math.max"), 4, qsl("math.max(x, ...)"));
    provider->add(qsl("math.min"), 4, qsl("math.min(x, ...)"));
    provider->add(qsl("math.modf"), 4, qsl("math.modf(x)"));
    provider->add(qsl("math.pi"), 4, qsl("math.pi"));
    provider->add(qsl("math.pow"), 4, qsl("math.pow(x, y)"));
    provider->add(qsl("math.rad"), 4, qsl("math.rad(x)"));
    provider->add(qsl("math.random"), 4, qsl("math.random([m [, n]])"));
    provider->add(qsl("math.randomseed"), 4, qsl("math.randomseed(x)"));
    provider->add(qsl("math.sin"), 4, qsl("math.sin(x)"));
    provider->add(qsl("math.sinh"), 4, qsl("math.sinh(x)"));
    provider->add(qsl("math.sqrt"), 4, qsl("math.sqrt(x)"));
    provider->add(qsl("math.tan"), 4, qsl("math.tan(x)"));
    provider->add(qsl("math.tanh"), 4, qsl("math.tanh(x)"));

    // IO library
    provider->add(qsl("io.close"), 4, qsl("io.close([file])"));
    provider->add(qsl("io.flush"), 4, qsl("io.flush()"));
    provider->add(qsl("io.input"), 4, qsl("io.input([file])"));
    provider->add(qsl("io.lines"), 4, qsl("io.lines([filename, ...])"));
    provider->add(qsl("io.open"), 4, qsl("io.open(filename [, mode])"));
    provider->add(qsl("io.output"), 4, qsl("io.output([file])"));
    provider->add(qsl("io.popen"), 4, qsl("io.popen(prog [, mode])"));
    provider->add(qsl("io.read"), 4, qsl("io.read(...)"));
    provider->add(qsl("io.tmpfile"), 4, qsl("io.tmpfile()"));
    provider->add(qsl("io.type"), 4, qsl("io.type(obj)"));
    provider->add(qsl("io.write"), 4, qsl("io.write(...)"));

    // OS library
    provider->add(qsl("os.clock"), 4, qsl("os.clock()"));
    provider->add(qsl("os.date"), 4, qsl("os.date([format [, time]])"));
    provider->add(qsl("os.difftime"), 4, qsl("os.difftime(t2, t1)"));
    provider->add(qsl("os.execute"), 4, qsl("os.execute([command])"));
    provider->add(qsl("os.exit"), 4, qsl("os.exit([code [, close]])"));
    provider->add(qsl("os.getenv"), 4, qsl("os.getenv(varname)"));
    provider->add(qsl("os.remove"), 4, qsl("os.remove(filename)"));
    provider->add(qsl("os.rename"), 4, qsl("os.rename(oldname, newname)"));
    provider->add(qsl("os.setlocale"), 4, qsl("os.setlocale(locale [, category])"));
    provider->add(qsl("os.time"), 4, qsl("os.time([table])"));
    provider->add(qsl("os.tmpname"), 4, qsl("os.tmpname()"));

    // Coroutine library
    provider->add(qsl("coroutine.create"), 4, qsl("coroutine.create(f)"));
    provider->add(qsl("coroutine.resume"), 4, qsl("coroutine.resume(co [, val1, ...])"));
    provider->add(qsl("coroutine.running"), 4, qsl("coroutine.running()"));
    provider->add(qsl("coroutine.status"), 4, qsl("coroutine.status(co)"));
    provider->add(qsl("coroutine.wrap"), 4, qsl("coroutine.wrap(f)"));
    provider->add(qsl("coroutine.yield"), 4, qsl("coroutine.yield(...)"));

    // Debug library
    provider->add(qsl("debug.debug"), 4, qsl("debug.debug()"));
    provider->add(qsl("debug.gethook"), 4, qsl("debug.gethook([thread])"));
    provider->add(qsl("debug.getinfo"), 4, qsl("debug.getinfo([thread,] f [, what])"));
    provider->add(qsl("debug.getlocal"), 4, qsl("debug.getlocal([thread,] f, local)"));
    provider->add(qsl("debug.getmetatable"), 4, qsl("debug.getmetatable(value)"));
    provider->add(qsl("debug.getregistry"), 4, qsl("debug.getregistry()"));
    provider->add(qsl("debug.getupvalue"), 4, qsl("debug.getupvalue(f, up)"));
    provider->add(qsl("debug.getuservalue"), 4, qsl("debug.getuservalue(u)"));
    provider->add(qsl("debug.sethook"), 4, qsl("debug.sethook([thread,] hook, mask [, count])"));
    provider->add(qsl("debug.setlocal"), 4, qsl("debug.setlocal([thread,] level, local, value)"));
    provider->add(qsl("debug.setmetatable"), 4, qsl("debug.setmetatable(value, table)"));
    provider->add(qsl("debug.setupvalue"), 4, qsl("debug.setupvalue(f, up, value)"));
    provider->add(qsl("debug.setuservalue"), 4, qsl("debug.setuservalue(udata, value)"));
    provider->add(qsl("debug.traceback"), 4, qsl("debug.traceback([thread,] [message [, level]])"));
    provider->add(qsl("debug.upvalueid"), 4, qsl("debug.upvalueid(f, n)"));
    provider->add(qsl("debug.upvaluejoin"), 4, qsl("debug.upvaluejoin(f1, n1, f2, n2)"));

    // Package library
    provider->add(qsl("package.config"), 4, qsl("package.config"));
    provider->add(qsl("package.cpath"), 4, qsl("package.cpath"));
    provider->add(qsl("package.loaded"), 4, qsl("package.loaded"));
    provider->add(qsl("package.loadlib"), 4, qsl("package.loadlib(libname, funcname)"));
    provider->add(qsl("package.path"), 4, qsl("package.path"));
    provider->add(qsl("package.preload"), 4, qsl("package.preload"));
    provider->add(qsl("package.searchers"), 4, qsl("package.searchers"));
    provider->add(qsl("package.searchpath"), 4, qsl("package.searchpath(name, path [, sep [, rep]])"));

    // Mudlet framework namespaced functions (priority 4 - same as Lua stdlib)
    // Geyser UI Framework
    provider->add(qsl("Geyser.Container:new"), 4, qsl("Geyser.Container:new(cons, container)"));
    provider->add(qsl("Geyser.Window:new"), 4, qsl("Geyser.Window:new(cons, container)"));
    provider->add(qsl("Geyser.Label:new"), 4, qsl("Geyser.Label:new(cons, container)"));
    provider->add(qsl("Geyser.MiniConsole:new"), 4, qsl("Geyser.MiniConsole:new(cons, container)"));
    provider->add(qsl("Geyser.Button:new"), 4, qsl("Geyser.Button:new(cons, container)"));
    provider->add(qsl("Geyser.Gauge:new"), 4, qsl("Geyser.Gauge:new(cons, container)"));
    provider->add(qsl("Geyser.Mapper:new"), 4, qsl("Geyser.Mapper:new(cons, container)"));
    provider->add(qsl("Geyser.UserWindow:new"), 4, qsl("Geyser.UserWindow:new(cons)"));
    provider->add(qsl("Geyser.CommandLine:new"), 4, qsl("Geyser.CommandLine:new(cons, container)"));
    provider->add(qsl("Geyser.HBox:new"), 4, qsl("Geyser.HBox:new(cons, container)"));
    provider->add(qsl("Geyser.VBox:new"), 4, qsl("Geyser.VBox:new(cons, container)"));
    provider->add(qsl("Geyser.ScrollBox:new"), 4, qsl("Geyser.ScrollBox:new(cons, container)"));
    provider->add(qsl("Geyser.ScrollBox:new2"), 4, qsl("Geyser.ScrollBox:new2()"));
    provider->add(qsl("Geyser.StyleSheet:new"), 4, qsl("Geyser.StyleSheet:new(stylesheet, parent, target)"));

    // Geyser namespace functions
    provider->add(qsl("Geyser.Color.parse"), 4, qsl("Geyser.Color.parse(color)"));
    provider->add(qsl("Geyser.Color.hex"), 4, qsl("Geyser.Color.hex(color)"));
    provider->add(qsl("Geyser.Color.hexa"), 4, qsl("Geyser.Color.hexa(color)"));
    provider->add(qsl("Geyser.Color.hhex"), 4, qsl("Geyser.Color.hhex(color)"));
    provider->add(qsl("Geyser.Color.hhexa"), 4, qsl("Geyser.Color.hhexa(color)"));
    provider->add(qsl("Geyser.Color.hdec"), 4, qsl("Geyser.Color.hdec(color)"));
    provider->add(qsl("Geyser.Color.hdeca"), 4, qsl("Geyser.Color.hdeca(color)"));

    // Adjustable Container Framework
    provider->add(qsl("Adjustable.Container:new"), 4, qsl("Adjustable.Container:new(cons, container)"));

    // Database Framework
    provider->add(qsl("db.create"), 4, qsl("db.create(db_name, schema)"));
    provider->add(qsl("db.query"), 4, qsl("db.query(db_name, query, ...)"));
    provider->add(qsl("db.insert"), 4, qsl("db.insert(db_name, sheet_name, values)"));
    provider->add(qsl("db.update"), 4, qsl("db.update(db_name, sheet_name, values, query)"));
    provider->add(qsl("db.delete"), 4, qsl("db.delete(db_name, sheet_name, query)"));
    provider->add(qsl("db.fetch"), 4, qsl("db.fetch(db_name, query, ...)"));
    provider->add(qsl("db.aggregate"), 4, qsl("db.aggregate(db_name, query, ...)"));

    // DateTime utilities
    provider->add(qsl("datetime.parse"), 4, qsl("datetime.parse(format, date_string)"));

    // Transfer ownership to Edbee - deleted automatically at app shutdown
    edbee::Edbee::instance()->autoCompleteProviderList()->giveProvider(provider);
    smAutoCompleteInitialized = true;
}

// Made before the editor, to collect what its error console would have shown
std::unique_ptr<TConsoleModel> dlgTriggerEditor::newErrorConsoleModel(Host* pHost)
{
    auto model = std::make_unique<TConsoleModel>(pHost);
    model->mConsoleName = qsl("errors_%1").arg(pHost->getName());
    model->setWrapAt(100);
    model->mShowTimeStamps = true;
    model->print(qsl("%1\n").arg(tr("*** starting new session ***")));
    return model;
}

dlgTriggerEditor::~dlgTriggerEditor()
{
    // ~QWidget closes the editor once this destructor is done, and whichever
    // of the item fields has the keyboard focus then emits editingFinished()
    // into one of the slot_saveProperty_...() slots when this object is no
    // longer a valid receiver (#9574)
    widgetutils::disconnectChildSignals(this);
    // The undo stacks are not in this widget's child tree - the edbee one hangs
    // off a parentless CharTextDocument - so disconnect them by hand:
    if (mpTextUndoStack) {
        disconnect(mpTextUndoStack, nullptr, this, nullptr);
    }
    if (mpUndoStack) {
        disconnect(mpUndoStack, nullptr, this, nullptr);
    }
}

void dlgTriggerEditor::slot_clickedMessageBox(const QString& URL)
{
    if (URL.startsWith(qsl("mudlet-error-line:"))) {
        jumpToErrorLine(URL.mid(qsl("mudlet-error-line:").size()).toInt());
    } else if (URL.startsWith("http")) {
        QDesktopServices::openUrl(URL);
    } else { // internal links used by expanding info text navigation
        showIntro(URL);
    }
}

void dlgTriggerEditor::slot_editorThemeChanged()
{
    for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
        applyPatternWidgetStyle(patternEdit);
    }
}

void dlgTriggerEditor::slot_smartUndo()
{
    // Stack-based undo: prioritize text editor changes, then fall back to item operations
    // This provides intuitive behavior - most recent change undoes first, regardless of focus

    // A merged folder in the Folders view hides the item an undo would change
    if (!mFolderGroupKey.isEmpty()) {
        return;
    }
    bool canUndoText = mpTextUndoStack && mpTextUndoStack->canUndo();
    bool canUndoItems = mpUndoStack && mpUndoStack->canUndo();

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "dlgTriggerEditor::slot_smartUndo() - canUndoText:" << canUndoText << "canUndoItems:" << canUndoItems;
#endif

    if (canUndoText) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "dlgTriggerEditor::slot_smartUndo() - Performing text undo via edbee";
#endif
        mpSourceEditorEdbee->controller()->undo();
    } else if (canUndoItems) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "dlgTriggerEditor::slot_smartUndo() - Performing item undo";
#endif
        // Loop to skip commands invalidated by Lua API changes
        const int maxAttempts = 100; // Safety limit to prevent infinite loops
        int attempts = 0;
        while (mpUndoStack->canUndo() && attempts < maxAttempts) {
            mpUndoStack->undo();

            if (mpUndoStack->wasLastCommandValid()) {
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "dlgTriggerEditor::slot_smartUndo() - Valid command undone after" << (attempts + 1) << "attempts";
#endif
                break;
            }

            // Command was invalid (Lua changed the item), silently skip and try next
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "dlgTriggerEditor::slot_smartUndo() - Invalid command, trying next (attempt" << (attempts + 1) << ")";
#endif
            attempts++;
        }
    }

    slot_updateUndoRedoButtonStates();
}

void dlgTriggerEditor::slot_smartRedo()
{
    // Stack-based redo: prioritize text editor changes, then fall back to item operations
    // This provides intuitive behavior - most recently undone change redoes first, regardless of focus

    if (!mFolderGroupKey.isEmpty()) {
        return;
    }
    bool canRedoText = mpTextUndoStack && mpTextUndoStack->canRedo();
    bool canRedoItems = mpUndoStack && mpUndoStack->canRedo();

#if defined(DEBUG_UNDO_REDO)
    qDebug() << "dlgTriggerEditor::slot_smartRedo() - canRedoText:" << canRedoText << "canRedoItems:" << canRedoItems;
#endif

    if (canRedoText) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "dlgTriggerEditor::slot_smartRedo() - Performing text redo via edbee";
#endif
        mpSourceEditorEdbee->controller()->redo();
    } else if (canRedoItems) {
#if defined(DEBUG_UNDO_REDO)
        qDebug() << "dlgTriggerEditor::slot_smartRedo() - Performing item redo";
#endif
        // Loop to skip commands invalidated by Lua API changes
        const int maxAttempts = 100; // Safety limit to prevent infinite loops
        int attempts = 0;
        while (mpUndoStack->canRedo() && attempts < maxAttempts) {
            mpUndoStack->redo();

            if (mpUndoStack->wasLastCommandValid()) {
#if defined(DEBUG_UNDO_REDO)
                qDebug() << "dlgTriggerEditor::slot_smartRedo() - Valid command redone after" << (attempts + 1) << "attempts";
#endif
                break;
            }

            // Command was invalid (Lua changed the item), silently skip and try next
#if defined(DEBUG_UNDO_REDO)
            qDebug() << "dlgTriggerEditor::slot_smartRedo() - Invalid command, trying next (attempt" << (attempts + 1) << ")";
#endif
            attempts++;
        }
    }

    slot_updateUndoRedoButtonStates();
}

void dlgTriggerEditor::slot_updateUndoRedoButtonStates()
{
    // Early exit during shutdown - guards against accessing destroyed objects
    if (!mpSourceEditorEdbee || !mpUndoAction || !mpRedoAction || !mpTextUndoStack) {
        return;
    }

    bool canUndoText = mpTextUndoStack->canUndo();
    bool canUndoItems = mpUndoStack && mpUndoStack->canUndo();

    bool canRedoText = mpTextUndoStack->canRedo();
    bool canRedoItems = mpUndoStack && mpUndoStack->canRedo();

    // Disable undo/redo in variables view since variables can be modified via Lua API
    bool inVariablesView = (mCurrentView == EditorViewType::cmVarsView);

    if (inVariablesView || !mFolderGroupKey.isEmpty()) {
        // In variables view, disable buttons and clear tooltips
        mpUndoAction->setEnabled(false);
        mpRedoAction->setEnabled(false);
        mpUndoAction->setToolTip(QString());
        mpUndoAction->setStatusTip(QString());
        mpRedoAction->setToolTip(QString());
        mpRedoAction->setStatusTip(QString());
    } else {
        // In other views, enable/disable based on undo/redo availability
        mpUndoAction->setEnabled(canUndoText || canUndoItems);
        mpRedoAction->setEnabled(canRedoText || canRedoItems);

        // Restore normal tooltips with keyboard shortcuts
        QString undoShortcut = mpUndoAction->shortcut().toString(QKeySequence::NativeText);
        QString redoShortcut = mpRedoAction->shortcut().toString(QKeySequence::NativeText);

        // Get undo/redo text from stack if available
        QString undoText;
        QString redoText;

        if (mpUndoStack) {
            QString stackUndoText = mpUndoStack->undoText();
            if (!stackUndoText.isEmpty()) {
                undoText = tr("Undo: %1 (%2)").arg(stackUndoText, undoShortcut);
            }

            QString stackRedoText = mpUndoStack->redoText();
            if (!stackRedoText.isEmpty()) {
                redoText = tr("Redo: %1 (%2)").arg(stackRedoText, redoShortcut);
            }
        }

        // Fall back to generic tooltips if no specific action text
        if (undoText.isEmpty()) {
            undoText = tr("Undo (%1)").arg(undoShortcut);
        }
        if (redoText.isEmpty()) {
            redoText = tr("Redo (%1)").arg(redoShortcut);
        }

        mpUndoAction->setToolTip(utils::richText(undoText));
        mpUndoAction->setStatusTip(undoText);
        mpRedoAction->setToolTip(utils::richText(redoText));
        mpRedoAction->setStatusTip(redoText);
    }
}

void dlgTriggerEditor::applyPatternWidgetStyle(dlgTriggerPatternEdit* patternWidget)
{
    if (!patternWidget || !mpHost) {
        return;
    }

    QPalette referencePalette;

    if (mpTriggersMainArea && mpTriggersMainArea->lineEdit_trigger_name) {
        referencePalette = mpTriggersMainArea->lineEdit_trigger_name->palette();
    } else {
        referencePalette = patternWidget->singleLineTextEdit_pattern->palette();
    }

    patternWidget->singleLineTextEdit_pattern->setTheme(mpHost->getEditorTheme());
    patternWidget->applyThemePalette(referencePalette);
    // After the palette pass: a pattern field takes the window's field colours whatever the syntax theme says
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    patternWidget->singleLineTextEdit_pattern->setFieldColors(tokens.field, tokens.text);

    // Game text, so set in the display font, where a space is as wide as a letter
    const QFont patternFont = mpHost->getDisplayFont();
    patternWidget->singleLineTextEdit_pattern->setFont(patternFont);
    patternWidget->label_patternNumber->setFont(patternFont);

    const QFontMetrics patternMetrics(patternFont);
    patternWidget->label_patternNumber->setFixedWidth(patternMetrics.horizontalAdvance(QString(scmEditorPatternNumberDigits, QLatin1Char('0'))));
    // The .ui's heights are for interface text; the row holds a line of the profile's display font
    const int rowHeight = std::max(scmEditorPatternRowMinimumHeight, patternMetrics.height() + scmEditorPatternRowPadding);
    patternWidget->setFixedHeight(rowHeight);
    const QMargins rowMargins = patternWidget->layout()->contentsMargins();
    const int contentHeight = rowHeight - rowMargins.top() - rowMargins.bottom();
    for (QWidget* pControl : {static_cast<QWidget*>(patternWidget->singleLineTextEdit_pattern),
                              static_cast<QWidget*>(patternWidget->comboBox_patternType),
                              static_cast<QWidget*>(patternWidget->spinBox_lineSpacer),
                              static_cast<QWidget*>(patternWidget->pushButton_fgColor),
                              static_cast<QWidget*>(patternWidget->pushButton_bgColor),
                              static_cast<QWidget*>(patternWidget->label_prompt)}) {
        pControl->setMaximumHeight(contentHeight);
    }

    patternWidget->comboBox_patternType->setFixedWidth(patternTypeColumnWidth(patternWidget->comboBox_patternType->font()));

    if (mPatternDeleteIcon.isNull()) {
        mPatternDeleteIcon = patternDeleteIcon();
    }
    if (!mPatternHoverTint.isValid()) {
        mPatternHoverTint = patternHoverTint();
    }
    patternWidget->setDeleteGlyph(mPatternDeleteIcon);
    patternWidget->setHoverTint(mPatternHoverTint);
}

// The longest translated type name, so every row's pattern starts at the same place
int dlgTriggerEditor::patternTypeColumnWidth(const QFont& typeFont) const
{
    if (mPatternTypeColumnWidth > 0) {
        return mPatternTypeColumnWidth;
    }
    const QFontMetrics typeMetrics(typeFont);
    int typeWidth = 0;
    for (const QString& typeName : std::as_const(mPatternList)) {
        typeWidth = std::max(typeWidth, typeMetrics.horizontalAdvance(typeName));
    }
    mPatternTypeColumnWidth = typeWidth + scmEditorPatternTypeChrome;
    return mPatternTypeColumnWidth;
}

QIcon dlgTriggerEditor::patternDeleteIcon() const
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QPixmap deleteSource = uiDesign::glyphPixmap(qsl(":/icons/editor-delete.svg"));
    QIcon deleteGlyph(uiDesign::tintedGlyph(deleteSource, tokens.mutedText));
    deleteGlyph.addPixmap(uiDesign::tintedGlyph(deleteSource, tokens.text), QIcon::Active);
    return deleteGlyph;
}

void dlgTriggerEditor::restylePatternTypeIcons()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const qreal ratio = devicePixelRatioF();
    const QColor outline = uiDesign::blend(tokens.page, tokens.text, 0.35);
    const qreal lightness = tokens.darkPage ? scmEditorPatternSwatchLightnessOnDark : scmEditorPatternSwatchLightnessOnLight;

    mPatternIcons.clear();
    mPatternIcons.reserve(static_cast<int>(std::size(scmEditorPatternSwatches)));
    for (const auto& recipe : scmEditorPatternSwatches) {
        const QColor fill = (recipe.hue < 0.0) ? uiDesign::blend(tokens.page, tokens.text, recipe.neutralWeight) : QColor::fromHslF(recipe.hue, scmEditorPatternSwatchSaturation, lightness);
        mPatternIcons.append(editorPatternSwatchIcon(fill, outline, ratio));
    }

    mPatternGripGlyph = editorPatternGripGlyph(tokens.mutedText, ratio);

    for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
        if (!patternEdit) {
            continue;
        }
        applyPatternTypeIcons(patternEdit->comboBox_patternType);
        applyPatternGripGlyph(patternEdit->label_dragHandle);
    }
}

// Sized explicitly, or the platform style's default icon size scales the swatches up
void dlgTriggerEditor::applyPatternTypeIcons(QComboBox* pBox) const
{
    if (!pBox) {
        return;
    }
    pBox->setIconSize(QSize(scmEditorPatternSwatchSize, scmEditorPatternSwatchSize));
    for (int i = 0; i < mPatternIcons.size() && i < pBox->count(); ++i) {
        pBox->setItemIcon(i, mPatternIcons.at(i));
    }
}

void dlgTriggerEditor::applyPatternGripGlyph(QLabel* pHandle) const
{
    if (!pHandle) {
        return;
    }
    pHandle->setText(QString());
    pHandle->setPixmap(mPatternGripGlyph);
    pHandle->setFixedWidth(scmEditorPatternGripWidth);
}

void dlgTriggerEditor::createPatternItem(int index)
{
    // Parentless until the layout adopts it inside a lift: under a stylesheet setParent() gives a new widget the
    // application font rather than its parent's (see StyleSheetFontLift)
    auto* pItem = new dlgTriggerPatternEdit(nullptr);
    QComboBox* pBox = pItem->comboBox_patternType;
    pBox->addItems(mPatternList);
    pBox->setItemData(0, QVariant(index));
    applyPatternTypeIcons(pBox);
    // Before the first drop-down: the translucent window a rounded list needs is made at the popup's first show
    uiDesign::letPopupsTakeTheFieldsCorner(pBox);
    applyPatternGripGlyph(pItem->label_dragHandle);
    connect(pBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &dlgTriggerEditor::slot_setupPatternControls);
    connect(pItem->pushButton_fgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorTriggerFg);
    connect(pItem->pushButton_bgColor, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_colorTriggerBg);
    connect(pItem->singleLineTextEdit_pattern, &QPlainTextEdit::textChanged, this, &dlgTriggerEditor::slot_changedPattern);
    connect(pItem->singleLineTextEdit_pattern, &QPlainTextEdit::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    connect(pItem->singleLineTextEdit_pattern, &SingleLineTextEdit::signal_pastedExtraLines, this, &dlgTriggerEditor::slot_pastedExtraPatternLines);
    connect(pItem->spinBox_lineSpacer, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_lineSpacerChanged);
    connect(pItem->spinBox_lineSpacer, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::slot_itemEdited);

    connect(pItem->toolButton_deletePattern, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_deletePatternRow);

    auto* pLayout = static_cast<QVBoxLayout*>(mpWidget_triggerItems->layout());
    const int insertIndex = mpButton_addPattern ? pLayout->indexOf(mpButton_addPattern) : pLayout->count() - 1;
    {
        const uiDesign::StyleSheetFontLift lift(mpWidget_triggerItems);
        pLayout->insertWidget(insertIndex, pItem);
    }
    // Polishing resolves a class-font widget (a label, or a button on macOS) against that font past its parent,
    // so resolve again after it
    pItem->ensurePolished();
    uiDesign::resolveFontsUnderStyleSheets(pItem);

    mTriggerPatternEdit.push_back(pItem);
    pItem->mRow = index;
    pItem->pushButton_fgColor->hide();
    pItem->pushButton_bgColor->hide();
    pItem->label_prompt->hide();
    pItem->spinBox_lineSpacer->hide();
    pItem->label_patternNumber->setText(QString::number(index + 1));
    pItem->label_patternNumber->show();

    //: Tooltip on the grip at the left of one of a trigger's pattern rows. %1 and %2 are the keyboard shortcuts that do the same thing without the mouse.
    pItem->label_dragHandle->setToolTip(
            utils::richText(tr("Drag to reorder this pattern, or move it with %1 and %2")
                                    .arg(QKeySequence(scmEditorPatternMoveUpKeys).toString(QKeySequence::NativeText), QKeySequence(scmEditorPatternMoveDownKeys).toString(QKeySequence::NativeText))));
    //: Name of a pattern row's grip as a screen reader speaks it. %1 is the number of the row it belongs to.
    pItem->label_dragHandle->setAccessibleName(tr("Reorder pattern %1").arg(index + 1));
    //: Tooltip on the button that takes one of a trigger's patterns away
    pItem->toolButton_deletePattern->setToolTip(utils::richText(tr("Delete this pattern")));
    //: Name of a pattern row's delete button as a screen reader speaks it. %1 is the number of the row it belongs to.
    pItem->toolButton_deletePattern->setAccessibleName(tr("Delete pattern %1").arg(index + 1));
    // Its glyph only shows under the mouse, so the keyboard needs its own way in
    pItem->toolButton_deletePattern->setFocusPolicy(Qt::TabFocus);
    pItem->toolButton_deletePattern->setFixedSize(scmEditorPatternDeleteButtonSize, scmEditorPatternDeleteButtonSize);
    pItem->toolButton_deletePattern->setIconSize(QSize(scmEditorPatternDeleteGlyphSize, scmEditorPatternDeleteGlyphSize));

    lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = false;

    pItem->singleLineTextEdit_pattern->installEventFilter(this);
    // What a drag from the grip means depends on where the other rows are, which only the editor knows
    pItem->label_dragHandle->installEventFilter(this);
    applyPatternWidgetStyle(pItem);
    pItem->spinBox_lineSpacer->installEventFilter(this);
}

void dlgTriggerEditor::showPatternItems(int count)
{
    count = qBound(0, count, scmEditorPatternRowLimit);
    while (mTriggerPatternEdit.size() < count) {
        createPatternItem(mTriggerPatternEdit.size());
    }

    for (int i = 0; i < mTriggerPatternEdit.size(); ++i) {
        auto* pItem = mTriggerPatternEdit[i];
        if (!pItem) {
            continue;
        }

        if (i < count) {
            pItem->show();
        } else {
            auto* edit = pItem->singleLineTextEdit_pattern;
            edit->blockSignals(true);
            edit->clear();
            edit->blockSignals(false);
            lineEditShouldMarkSpaces[edit] = false;

            auto* combo = pItem->comboBox_patternType;
            combo->blockSignals(true);
            combo->setCurrentIndex(REGEX_SUBSTRING);
            combo->blockSignals(false);

            pItem->pushButton_fgColor->hide();
            pItem->pushButton_bgColor->hide();
            pItem->label_prompt->hide();
            pItem->spinBox_lineSpacer->hide();
            pItem->hide();
        }
    }

    mVisiblePatternCount = count;
    updatePatternPlaceholders();
    updateAddPatternButton();
    updatePatternTabOrder();
}

void dlgTriggerEditor::updatePatternPlaceholders()
{
    for (int i = 0; i < mVisiblePatternCount; ++i) {
        auto* patternItem = mTriggerPatternEdit.value(i, nullptr);
        if (!patternItem) {
            continue;
        }

        auto* edit = patternItem->singleLineTextEdit_pattern;
        if (!edit) {
            continue;
        }

        if (!edit->isVisible() || !edit->toPlainText().isEmpty()) {
            edit->setPlaceholderText(QString());
            continue;
        }

        const QString placeholder = patternPlaceholderText(patternItem->comboBox_patternType->currentIndex());
        edit->setPlaceholderText(placeholder);
    }
}

QString dlgTriggerEditor::patternPlaceholderText(const int patternType) const
{
    switch (patternType) {
    case REGEX_SUBSTRING:
        return tr("Text to find (anywhere in the game output)");
    case REGEX_PERL:
        return tr("Text to find (as a regular expression pattern)");
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
        return tr("Text to find (from beginning of the line)");
    case REGEX_EXACT_MATCH:
        return tr("Exact line to match");
    case REGEX_LUA_CODE:
        return tr("Lua code to run (return true to match)");
    default:
        return QString();
    }
}

void dlgTriggerEditor::setupPatternNavigationShortcuts()
{
    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->deleteLater();
        mFirstPatternShortcut = nullptr;
    }

    if (mLastPatternShortcut) {
        mLastPatternShortcut->deleteLater();
        mLastPatternShortcut = nullptr;
    }

    for (auto* shortcut : std::as_const(mPatternNavigationShortcuts)) {
        if (shortcut) {
            shortcut->deleteLater();
        }
    }
    mPatternNavigationShortcuts.clear();

    if (!mpTriggersMainArea) {
        return;
    }

    mFirstPatternShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up), mpTriggersMainArea);
    mFirstPatternShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(mFirstPatternShortcut, &QShortcut::activated, this, [this]() {
        if (mVisiblePatternCount < 1) {
            return;
        }
        focusPatternItem(0, Qt::ShortcutFocusReason);
    });


    mLastPatternShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down), mpTriggersMainArea);
    mLastPatternShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(mLastPatternShortcut, &QShortcut::activated, this, [this]() {
        if (mVisiblePatternCount < 1) {
            return;
        }
        focusPatternItem(mVisiblePatternCount - 1, Qt::ShortcutFocusReason);
    });

    // In mPatternNavigationShortcuts, so changeView() enables them with the trigger view only
    auto addReorderShortcut = [this](const QKeySequence& keys, const int offset) {
        auto* pShortcut = new QShortcut(keys, mpTriggersMainArea);
        pShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(pShortcut, &QShortcut::activated, this, [this, offset]() {
            movePatternRowByKeyboard(offset);
        });
        mPatternNavigationShortcuts.append(pShortcut);
    };
    addReorderShortcut(QKeySequence(scmEditorPatternMoveUpKeys), -1);
    addReorderShortcut(QKeySequence(scmEditorPatternMoveDownKeys), 1);

    const bool enableShortcuts = mCurrentView == EditorViewType::cmTriggerView;
    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->setEnabled(enableShortcuts);
    }

    if (mLastPatternShortcut) {
        mLastPatternShortcut->setEnabled(enableShortcuts);
    }

    for (auto* pShortcut : std::as_const(mPatternNavigationShortcuts)) {
        pShortcut->setEnabled(enableShortcuts);
    }
}

int dlgTriggerEditor::focusedPatternRow() const
{
    for (const QWidget* pFocused = QApplication::focusWidget(); pFocused; pFocused = pFocused->parentWidget()) {
        if (const auto* pRow = qobject_cast<const dlgTriggerPatternEdit*>(pFocused)) {
            return pRow->mRow;
        }
    }
    return -1;
}

void dlgTriggerEditor::movePatternRowByKeyboard(const int offset)
{
    const int from = focusedPatternRow();
    if (from < 0) {
        return;
    }

    const int to = from + offset;
    if (to < 0 || to > lastVisiblePatternRow()) {
        return;
    }

    movePatternRowContent(from, to);
    // The row widgets never reorder, so the focus follows the content for a second press to carry it further
    focusPatternItem(to, Qt::ShortcutFocusReason);
}

void dlgTriggerEditor::setupAddPatternButton()
{
    mpButton_addPattern = new uiDesign::PlaceholderButton(mpWidget_triggerItems);
    mpButton_addPattern->setObjectName(qsl("editorAddPattern"));
    mpButton_addPattern->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    //: Button under a trigger's patterns that gives it one more
    mpButton_addPattern->setText(tr("Add pattern"));
    mpButton_addPattern->setCursor(Qt::PointingHandCursor);
    // The button paints its own dashed frame; auto-raise would have the style draw a second one under the mouse
    mpButton_addPattern->setAutoRaise(false);
    mpButton_addPattern->setFrameMargins(QMargins(0, scmEditorAddPatternMarginTop, 0, scmEditorAddPatternMarginBottom));
    mpButton_addPattern->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(mpButton_addPattern, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_addPattern);

    auto* pLayout = static_cast<QVBoxLayout*>(mpWidget_triggerItems->layout());
    // Above the trailing stretch; createPatternItem() inserts each new row above this button
    pLayout->insertWidget(pLayout->count() - 1, mpButton_addPattern, 0, Qt::AlignLeft);

    // Out of the layout: it is laid over whichever row a drag would land on, which no layout has a slot for
    mpFrame_patternDropIndicator = new QFrame(mpWidget_triggerItems);
    mpFrame_patternDropIndicator->setObjectName(qsl("editorPatternDropIndicator"));
    mpFrame_patternDropIndicator->setFixedHeight(scmEditorPatternDropIndicatorHeight);
    mpFrame_patternDropIndicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    mpFrame_patternDropIndicator->hide();

    // Created after the shell was styled, so it asks for its own glyph
    restyleAddPatternIcon();
    updateAddPatternButton();
}

void dlgTriggerEditor::restyleAddPatternIcon()
{
    if (!mpButton_addPattern) {
        return;
    }

    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QPixmap addSource = uiDesign::glyphPixmap(qsl(":/icons/editor-add.svg"));
    QIcon addGlyph(uiDesign::tintedGlyph(addSource, tokens.mutedText));
    addGlyph.addPixmap(uiDesign::tintedGlyph(addSource, tokens.accentText), QIcon::Active);
    mpButton_addPattern->setIcon(addGlyph);
    const int addGlyphSize = qRound(mpButton_addPattern->fontMetrics().height() * 0.9);
    mpButton_addPattern->setIconSize(QSize(addGlyphSize, addGlyphSize));

    // Control contrast rather than a card hairline: the frame is the only sign one more pattern goes there
    const QColor restingFrame = uiDesign::blend(tokens.card, tokens.text, 0.45);
    const QColor disabledFrame = uiDesign::blend(tokens.card, tokens.text, 0.22);
    mpButton_addPattern->setFrameColors(restingFrame, tokens.accent, disabledFrame);
}

void dlgTriggerEditor::updateAddPatternButton()
{
    if (!mpButton_addPattern) {
        return;
    }

    const bool roomLeft = mVisiblePatternCount < scmEditorPatternRowLimit;
    mpButton_addPattern->setEnabled(roomLeft);
    if (roomLeft) {
        //: Tooltip on the button that gives a trigger one more pattern
        mpButton_addPattern->setToolTip(utils::richText(tr("Add another pattern to this trigger")));
    } else {
        //: Tooltip on that button once the trigger holds as many patterns as it can. %n is that number.
        mpButton_addPattern->setToolTip(utils::richText(tr("A trigger can hold %n pattern(s) at most", nullptr, scmEditorPatternRowLimit)));
    }
}

void dlgTriggerEditor::slot_addPattern()
{
    if (mVisiblePatternCount >= scmEditorPatternRowLimit) {
        return;
    }

    showPatternItems(mVisiblePatternCount + 1);
    // Before focusing: the scroll to the new row is measured against the pane's new height
    fitFormPaneToItsContents();
    focusPatternItem(mVisiblePatternCount - 1);
}

void dlgTriggerEditor::slot_deletePatternRow()
{
    auto* pButton = qobject_cast<QToolButton*>(sender());
    if (!pButton) {
        return;
    }

    auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(pButton->parentWidget());
    if (!patternItem) {
        return;
    }

    deletePatternRow(patternItem->mRow);
}

dlgTriggerEditor::PatternRowContent dlgTriggerEditor::patternRowContent(const dlgTriggerPatternEdit* patternItem) const
{
    PatternRowContent content;
    if (!patternItem) {
        return content;
    }

    content.type = patternItem->comboBox_patternType->currentIndex();
    content.pattern = patternItem->singleLineTextEdit_pattern->toPlainText();
    content.lineSpacerValue = patternItem->spinBox_lineSpacer->value();
    content.foregroundText = patternItem->pushButton_fgColor->text();
    content.foregroundStyleSheet = patternItem->pushButton_fgColor->styleSheet();
    content.backgroundText = patternItem->pushButton_bgColor->text();
    content.backgroundStyleSheet = patternItem->pushButton_bgColor->styleSheet();
    const auto marked = lineEditShouldMarkSpaces.find(patternItem->singleLineTextEdit_pattern);
    content.markSpaces = marked != lineEditShouldMarkSpaces.cend() && marked->second;
    return content;
}

void dlgTriggerEditor::setPatternRowContent(dlgTriggerPatternEdit* patternItem, const PatternRowContent& content)
{
    if (!patternItem) {
        return;
    }

    // Quiet through setupPatternControls() too: this row is one step of a shift that is still under way
    const bool patternWasBlocked = patternItem->singleLineTextEdit_pattern->blockSignals(true);
    {
        const QSignalBlocker typeBlocker(patternItem->comboBox_patternType);
        const QSignalBlocker spacerBlocker(patternItem->spinBox_lineSpacer);
        // Text before type: a type change writes a colour pattern's text when the row looks empty
        patternItem->singleLineTextEdit_pattern->setPlainText(content.pattern);
        patternItem->comboBox_patternType->setCurrentIndex(content.type);
        patternItem->spinBox_lineSpacer->setValue(content.lineSpacerValue);
    }
    // Carried with the row: the colour buttons are only worked out from the pattern text when a trigger loads
    patternItem->pushButton_fgColor->setStyleSheet(content.foregroundStyleSheet);
    patternItem->pushButton_fgColor->setText(content.foregroundText);
    patternItem->pushButton_bgColor->setStyleSheet(content.backgroundStyleSheet);
    patternItem->pushButton_bgColor->setText(content.backgroundText);

    setupPatternControls(content.type, patternItem);
    lineEditShouldMarkSpaces[patternItem->singleLineTextEdit_pattern] = content.markSpaces;
    patternItem->singleLineTextEdit_pattern->blockSignals(patternWasBlocked);
}

// Not bounded by the pool's size, or content could land in a hidden row
int dlgTriggerEditor::lastVisiblePatternRow() const
{
    return std::min(mVisiblePatternCount, static_cast<int>(mTriggerPatternEdit.size())) - 1;
}

void dlgTriggerEditor::movePatternRowContent(const int from, const int to)
{
    const int lastRow = lastVisiblePatternRow();
    if (from == to || from < 0 || to < 0 || from > lastRow || to > lastRow) {
        return;
    }

    const PatternRowContent moved = patternRowContent(mTriggerPatternEdit.at(from));
    mPatternBulkEdit = true;
    if (from < to) {
        for (int i = from; i < to; ++i) {
            setPatternRowContent(mTriggerPatternEdit.at(i), patternRowContent(mTriggerPatternEdit.at(i + 1)));
        }
    } else {
        for (int i = from; i > to; --i) {
            setPatternRowContent(mTriggerPatternEdit.at(i), patternRowContent(mTriggerPatternEdit.at(i - 1)));
        }
    }
    setPatternRowContent(mTriggerPatternEdit.at(to), moved);
    mPatternBulkEdit = false;

    slot_itemEdited();
    compactPatternRows();
    updatePatternTabOrder();
}

// The row widgets are a pool that is never reordered, so a delete shifts content up and empties the last row
void dlgTriggerEditor::deletePatternRow(const int row)
{
    const int lastRow = lastVisiblePatternRow();
    if (row < 0 || row > lastRow) {
        return;
    }

    mPatternBulkEdit = true;
    for (int i = row; i < lastRow; ++i) {
        setPatternRowContent(mTriggerPatternEdit.at(i), patternRowContent(mTriggerPatternEdit.at(i + 1)));
    }
    setPatternRowContent(mTriggerPatternEdit.at(lastRow), PatternRowContent());
    mPatternBulkEdit = false;

    slot_itemEdited();
    compactPatternRows();
    updatePatternTabOrder();
}

int dlgTriggerEditor::patternRowAt(const QPoint& itemsPos) const
{
    int lastVisible = -1;
    for (int i = 0; i < mVisiblePatternCount && i < mTriggerPatternEdit.size(); ++i) {
        const auto* patternItem = mTriggerPatternEdit.at(i);
        if (!patternItem || patternItem->isHidden()) {
            continue;
        }

        lastVisible = i;
        if (itemsPos.y() <= patternItem->geometry().bottom()) {
            return i;
        }
    }

    return lastVisible;
}

void dlgTriggerEditor::showPatternDropIndicator(const int targetRow)
{
    if (!mpFrame_patternDropIndicator) {
        return;
    }

    auto* patternItem = mTriggerPatternEdit.value(targetRow, nullptr);
    if (!patternItem || targetRow == mPatternDragSourceRow) {
        mpFrame_patternDropIndicator->hide();
        return;
    }

    const QRect rowRect = patternItem->geometry();
    const int indicatorTop = targetRow < mPatternDragSourceRow ? rowRect.top() : rowRect.bottom() - scmEditorPatternDropIndicatorHeight;
    mpFrame_patternDropIndicator->setGeometry(rowRect.left(), indicatorTop, rowRect.width(), scmEditorPatternDropIndicatorHeight);
    mpFrame_patternDropIndicator->raise();
    mpFrame_patternDropIndicator->show();
}

void dlgTriggerEditor::endPatternRowDrag(const bool dropped)
{
    if (mpFrame_patternDropIndicator) {
        mpFrame_patternDropIndicator->hide();
    }

    const int fromRow = mPatternDragSourceRow;
    const int toRow = mPatternDragTargetRow;
    mPatternDragSourceRow = -1;
    mPatternDragTargetRow = -1;
    mPatternDragActive = false;

    if (dropped) {
        movePatternRowContent(fromRow, toRow);
    }
}

// Tracked by hand rather than with QDrag: the rows never reorder, so there is no widget to carry, only content
bool dlgTriggerEditor::handlePatternHandleEvent(QObject* watched, QEvent* event)
{
    auto* pHandle = qobject_cast<QLabel*>(watched);
    if (!pHandle || pHandle->objectName() != qsl("label_dragHandle")) {
        return false;
    }

    auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(pHandle->parentWidget());
    if (!patternItem || !mpWidget_triggerItems) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto* pMouseEvent = static_cast<QMouseEvent*>(event);
        if (pMouseEvent->button() != Qt::LeftButton) {
            return false;
        }
        mPatternDragSourceRow = patternItem->mRow;
        mPatternDragTargetRow = patternItem->mRow;
        mPatternDragPressPos = pMouseEvent->globalPosition().toPoint();
        mPatternDragActive = false;
        return true;
    }
    case QEvent::MouseMove: {
        if (mPatternDragSourceRow < 0) {
            return false;
        }
        auto* pMouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint globalPos = pMouseEvent->globalPosition().toPoint();
        if (!mPatternDragActive) {
            if ((globalPos - mPatternDragPressPos).manhattanLength() < scmEditorPatternDragThreshold) {
                return true;
            }
            mPatternDragActive = true;
            pHandle->setCursor(Qt::ClosedHandCursor);
        }
        mPatternDragTargetRow = patternRowAt(mpWidget_triggerItems->mapFromGlobal(globalPos));
        showPatternDropIndicator(mPatternDragTargetRow);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        if (mPatternDragSourceRow < 0) {
            return false;
        }
        pHandle->setCursor(Qt::OpenHandCursor);
        endPatternRowDrag(mPatternDragActive);
        return true;
    }
    default:
        break;
    }

    return false;
}

void dlgTriggerEditor::slot_hideVariable(bool status)
{
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = treeWidget_variables->variableForRow(vu, mpCurrentVarItem);
    if (var) {
        if (status) {
            vu->addHidden(var, 1);
        } else {
            vu->removeHidden(var);
        }
        // The row stays in the tree until the variables are reread, so its hidden mark is updated here
        refreshVariableRow(mpCurrentVarItem);
        updateHiddenVariablesCount();
    }
}

void dlgTriggerEditor::slot_toggleHiddenVariables(bool state)
{
    if (showHiddenVars != state) {
        showHiddenVars = state;
        repopulateVars();
    }
}

void dlgTriggerEditor::slot_viewStatsAction()
{
    mpHost->mainConsoleView()->showStatistics();
    mudlet::self()->raise();
    mudlet::self()->activateWindow();
    mudlet::self()->raise();
}

void dlgTriggerEditor::slot_viewErrorsAction()
{
    mpErrorConsole->setVisible(!mpErrorConsole->isVisible());
}


void dlgTriggerEditor::slot_setToolBarIconSize(const int s)
{
    if (s <= 0) {
        return;
    }

    applyEditorToolbarButtonStyles();

    // One scale for toolbar and sidebar, since the two bars sit against each other
    mEditorIconSize = s * scmEditorIconSizeStep;
    toolBar->setIconSize(QSize(mEditorIconSize, mEditorIconSize));
    // This runs from the constructor as well, before there is a sidebar to size
    if (mpListWidget_editorSidebar) {
        mpListWidget_editorSidebar->setIconSize(QSize(mEditorIconSize, mEditorIconSize));
        // A wider glyph widens the rows, which moves the label breakpoint
        invalidateEditorSidebarWidths();
        updateEditorSidebarMode();
    }

    // Wider buttons move the toolbar's label breakpoint too
    fitEditorToolBarToItsLength();
}

void dlgTriggerEditor::slot_setTreeWidgetIconSize(const int s)
{
    if (s <= 0) {
        return;
    }

    const QSize newSize(s * 8, s * 8);
    treeWidget_triggers->setIconSize(newSize);
    treeWidget_aliases->setIconSize(newSize);
    treeWidget_timers->setIconSize(newSize);
    treeWidget_scripts->setIconSize(newSize);
    treeWidget_keys->setIconSize(newSize);
    treeWidget_actions->setIconSize(newSize);
    treeWidget_variables->setIconSize(newSize);
    mpFolderTree->setIconSize(newSize);
}

void dlgTriggerEditor::slot_colouredIconsChanged()
{
    restyleEditorIcons();
    const bool coloured = mudlet::self() && mudlet::self()->colouredIcons();
    for (uiDesign::EditorTreeDelegate* pDelegate : std::as_const(mEditorTreeDelegates)) {
        pDelegate->setColouredMarks(coloured);
        pDelegate->restyle();
    }
    for (QTreeWidget* pTree : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables, mpFolderTree}) {
        if (pTree) {
            pTree->viewport()->update();
        }
    }
}

void dlgTriggerEditor::closeEvent(QCloseEvent* event)
{
    endKeyGrab();
    emit editorClosing();
    writeSettings();
    event->accept();
}

void dlgTriggerEditor::hideEvent(QHideEvent* event)
{
    // The editor is hidden rather than closed on several paths, profile teardown among them
    endKeyGrab();
    QMainWindow::hideEvent(event);
}

static bool windowPlacementReachable(const QPoint& topLeft, const QSize& windowSize)
{
    constexpr int titleBarHeight = 30;
    constexpr int minimumGrabWidth = 120;

    const QRect grabStrip(topLeft, QSize(windowSize.width(), titleBarHeight));
    for (const QScreen* pScreen : QGuiApplication::screens()) {
        const QRect reachable = pScreen->availableGeometry().intersected(grabStrip);
        if (reachable.width() >= qMin(minimumGrabWidth, windowSize.width()) && reachable.height() >= titleBarHeight / 2) {
            return true;
        }
    }
    return false;
}

QSize dlgTriggerEditor::defaultEditorSize() const
{
    return QSize(scmEditorDefaultWidth, scmEditorDefaultHeight);
}

void dlgTriggerEditor::repositionOnProfileScreen()
{
    mRepositioningEditorWindow = true;
    if (TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr) {
        widgetutils::positionDialogOnActiveProfileScreen(this, nullptr, pConsole);
    } else {
        widgetutils::centerDialogOnScreen(this, QGuiApplication::primaryScreen());
    }
    mRepositioningEditorWindow = false;
}

bool dlgTriggerEditor::onSameScreenAsProfile() const
{
    const TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr;
    if (!pConsole) {
        return true;
    }

    const QScreen* pProfileScreen = QApplication::screenAt(pConsole->mapToGlobal(pConsole->rect().center()));
    if (!pProfileScreen) {
        pProfileScreen = pConsole->screen();
    }
    const QScreen* pEditorScreen = QApplication::screenAt(mapToGlobal(rect().center()));
    if (!pProfileScreen || !pEditorScreen) {
        return true;
    }
    return pProfileScreen == pEditorScreen;
}

void dlgTriggerEditor::restoreWindowGeometry()
{
    QSettings& settings = *MudletApp::getQSettings();

    const TMainConsole* pConsole = mpHost ? mpHost->mainConsoleView() : nullptr;
    const QScreen* pScreen = pConsole ? pConsole->screen() : nullptr;
    if (!pScreen) {
        pScreen = QGuiApplication::primaryScreen();
    }
    const QRect availableArea = pScreen ? pScreen->availableGeometry() : QRect(0, 0, 1024, 768);

    const QSize storedSize = settings.value(qsl("script_editor_size")).toSize();
    QSize targetSize = (storedSize.isValid() && !storedSize.isEmpty()) ? storedSize : defaultEditorSize();
    // A size stored on a larger or detached screen would open off the bottom of this desktop
    targetSize = targetSize.boundedTo(availableArea.size()).expandedTo(minimumSizeHint().boundedTo(availableArea.size()));
    resize(targetSize);

    if (settings.contains(qsl("script_editor_pos"))) {
        const QPoint storedPos = settings.value(qsl("script_editor_pos")).toPoint();
        if (windowPlacementReachable(storedPos, targetSize)) {
            mRepositioningEditorWindow = true;
            move(storedPos);
            mRepositioningEditorWindow = false;
            mEditorPlacementChosen = true;
            return;
        }
    }

    repositionOnProfileScreen();
}

void dlgTriggerEditor::readSettings()
{
    QSettings& settings = *MudletApp::getQSettings();

    restoreWindowGeometry();

    mAutosaveInterval = settings.value("autosaveIntervalMinutes", 2).toInt();

    mEditorSidebarLabelsShown = settings.value(qsl("editorSidebarLabelsShown"), true).toBool();
    updateEditorSidebarMode();
}

void dlgTriggerEditor::writeSettings()
{
    QSettings& settings = *MudletApp::getQSettings();
    // Every profile builds an editor as it loads and script_editor_pos/size are one pair for the whole
    // application, so an editor never shown must not overwrite the placement chosen in another profile
    if (mHasBeenShown) {
        // The normal geometry, or a maximized window would reopen at screen size for good
        const QRect normalArea = normalGeometry();
        const QRect storedArea = ((isMaximized() || isFullScreen()) && normalArea.isValid()) ? normalArea : QRect(pos(), size());
        settings.setValue(qsl("script_editor_pos"), storedArea.topLeft());
        settings.setValue(qsl("script_editor_size"), storedArea.size());
    }
    settings.setValue("autosaveIntervalMinutes", mAutosaveInterval);
    // Only the toggle changes this; a window too narrow for the labels folds them without touching it
    settings.setValue(qsl("editorSidebarLabelsShown"), mEditorSidebarLabelsShown);

    // A dragged split lasts only for the session, so the per-view split keys are obsolete
    for (const QString& retired : {qsl("mTriggerEditorSplitterState"),
                                   qsl("mAliasEditorSplitterState"),
                                   qsl("mScriptEditorSplitterState"),
                                   qsl("mActionEditorSplitterState"),
                                   qsl("mKeyEditorSplitterState"),
                                   qsl("mTimerEditorSplitterState"),
                                   qsl("mVarEditorSplitterState")}) {
        settings.remove(retired);
    }

    // Obsolete: the options strip always opens closed
    settings.remove(qsl("showAllTriggerControls"));
}

void dlgTriggerEditor::slot_itemSelectedInSearchResults(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    // Showing the item's view rebuilds stale trees, which would free the items found below
    if (mNeedUpdateData) {
        saveOpenChanges();
        rebuildStaleTrees();
    }

    // For changing views from one type to another (e.g. script->triggers), we have to show
    // the new view first before changing the TreeWidgetItem. Because we save changes to
    // the current item when it is left, if we change the TreeWidgetItem and then swap
    // views the contents of the previous item will be overwritten.
    QList<QTreeWidgetItem*> foundItemsList;
    switch (static_cast<EditorViewType>(pItem->data(0, ItemRole).toInt())) {
    case EditorViewType::cmTriggerView: { // DONE
        // These searches are to be case sensitive and recursive and find an
        // exact match - we are trying to find the "Name" of the item and then,
        // in case of duplicates we do a match on exact ID number
        foundItemsList = treeWidget_triggers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        // This was inside the loop but it is a constant value for the duration
        // of this method!
        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showTriggers();
                slot_triggerSelected(treeWidgetItem);
                treeWidget_triggers->setCurrentItem(treeWidgetItem, 0);
                treeWidget_triggers->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpTriggersMainArea->lineEdit_trigger_name->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern: {
                    // Deferred: ensureWidgetVisible() reads the pattern list's geometry, which the
                    // layout for the trigger just selected has not settled yet
                    const int row = pItem->data(0, PatternOrLineRole).toInt();
                    const int position = pItem->data(0, PositionRole).toInt();
                    QTimer::singleShot(0ms, this, [this, idSearch, row, position]() {
                        // Another item can be selected before this runs, and its rows are not the result's
                        if (mCurrentView != EditorViewType::cmTriggerView || !mpCurrentTriggerItem || mpCurrentTriggerItem->data(0, IdRole).toInt() != idSearch || row >= mVisiblePatternCount) {
                            return;
                        }
                        dlgTriggerPatternEdit* pTriggerPattern = mTriggerPatternEdit.value(row, nullptr);
                        if (!pTriggerPattern) {
                            return;
                        }
                        mpScrollArea->ensureWidgetVisible(pTriggerPattern);
                        auto* pEdit = pTriggerPattern->singleLineTextEdit_pattern;
                        if (!pEdit->isVisible()) {
                            // Colour, prompt and line spacer patterns have no text to put the caret in
                            focusPatternItem(row, Qt::OtherFocusReason);
                            return;
                        }
                        pEdit->setFocus(Qt::OtherFocusReason);
                        QTextCursor cursor = pEdit->textCursor();
                        cursor.setPosition(position);
                        pEdit->setTextCursor(cursor);
                    });
                    break;
                }
                case SearchResultIsCommand:
                    mpTriggersMainArea->lineEdit_trigger_command->setFocus(Qt::OtherFocusReason);
                    mpTriggersMainArea->lineEdit_trigger_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TRIGGER type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmAliasView: {
        foundItemsList = treeWidget_aliases->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showAliases();
                slot_aliasSelected(treeWidgetItem);
                treeWidget_aliases->setCurrentItem(treeWidgetItem, 0);
                treeWidget_aliases->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            auto controller = mpSourceEditorEdbee->controller();
                            controller->moveCaretTo(line, column, false);
                            controller->setAutoScrollToCaret(edbee::TextEditorController::AutoScrollWhenFocus);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpAliasMainArea->lineEdit_alias_name->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsPattern:
                    mpAliasMainArea->lineEdit_alias_pattern->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_pattern->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpAliasMainArea->lineEdit_alias_command->setFocus(Qt::OtherFocusReason);
                    mpAliasMainArea->lineEdit_alias_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a ALIAS type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
        break;
    }

    case EditorViewType::cmScriptView: {
        foundItemsList = treeWidget_scripts->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showScripts();
                slot_scriptsSelected(treeWidgetItem);
                treeWidget_scripts->setCurrentItem(treeWidgetItem, 0);
                treeWidget_scripts->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpScriptsMainArea->lineEdit_script_name->setFocus(Qt::OtherFocusReason);
                    mpScriptsMainArea->lineEdit_script_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsEventHandler: {
                    // By name, not by the searched index: the chip row skips empty and repeated names, so chip
                    // positions are not list positions
                    TScript* pScript = mpHost->getScriptUnit()->getScript(idSearch);
                    const QString handler = pScript ? pScript->getEventHandlerList().value(pItem->data(0, PatternOrLineRole).toInt()) : QString();
                    mpChipRow_scriptEvents->focusItem(mpChipRow_scriptEvents->indexOf(handler));
                    break;
                }
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a SCRIPT type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }

                return;
            }
        }
        break;
    }

    case EditorViewType::cmActionView: {
        foundItemsList = treeWidget_actions->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetitem : std::as_const(foundItemsList)) {
            if (treeWidgetitem->data(0, IdRole).toInt() == idSearch) {
                slot_showActions();
                slot_actionSelected(treeWidgetitem);
                treeWidget_actions->setCurrentItem(treeWidgetitem, 0);
                treeWidget_actions->scrollToItem(treeWidgetitem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpActionsMainArea->lineEdit_action_name->setFocus(Qt::OtherFocusReason);
                    mpActionsMainArea->lineEdit_action_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpActionsMainArea->lineEdit_action_button_command_down->setFocus(Qt::OtherFocusReason);
                    mpActionsMainArea->lineEdit_action_button_command_down->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsExtraCommand:
                    if (mpActionsMainArea->checkBox_action_button_isPushDown->isChecked()) {
                        mpActionsMainArea->lineEdit_action_button_command_up->setFocus(Qt::OtherFocusReason);
                        mpActionsMainArea->lineEdit_action_button_command_up->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    }
                    break;
                case SearchResultsIsCss: {
                    mpActionsMainArea->plainTextEdit_action_css->setFocus(Qt::OtherFocusReason);
                    QTextCursor cssCursor(mpActionsMainArea->plainTextEdit_action_css->textCursor());
                    cssCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
                    if (pItem->data(0, PatternOrLineRole).toInt()) {
                        // Are we not on the first line - so move down that many lines?
                        cssCursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, pItem->data(0, PatternOrLineRole).toInt());
                    }
                    if (pItem->data(0, PositionRole).toInt()) {
                        // Are we not on the first character - if so move right that many QChars...
                        cssCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, pItem->data(0, PositionRole).toInt());
                    }
                    mpActionsMainArea->plainTextEdit_action_css->setTextCursor(cssCursor);
                } // End case SearchResultsIsCss
                break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a BUTTON type item but handler for element of type:" << treeWidgetitem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End or switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmActionView

    case EditorViewType::cmTimerView: {
        foundItemsList = treeWidget_timers->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        const int idSearch = pItem->data(0, IdRole).toInt();

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            if (treeWidgetItem->data(0, IdRole).toInt() == idSearch) {
                slot_showTimers();
                slot_timerSelected(treeWidgetItem);
                treeWidget_timers->setCurrentItem(treeWidgetItem, 0);
                treeWidget_timers->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpTimersMainArea->lineEdit_timer_name->setFocus(Qt::OtherFocusReason);
                    mpTimersMainArea->lineEdit_timer_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpTimersMainArea->lineEdit_timer_command->setFocus(Qt::OtherFocusReason);
                    mpTimersMainArea->lineEdit_timer_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a TIMER type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if()
        } // End of for()
        break;
    } // End of case EditorViewType::cmTimerView

    case EditorViewType::cmKeysView: {
        foundItemsList = treeWidget_keys->findItems(pItem->data(0, NameRole).toString(), Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive, 0);

        for (auto treeWidgetItem : std::as_const(foundItemsList)) {
            const int idTree = treeWidgetItem->data(0, IdRole).toInt();
            const int idSearch = pItem->data(0, IdRole).toInt();
            if (idTree == idSearch) {
                slot_showKeys();
                slot_keySelected(treeWidgetItem);
                treeWidget_keys->setCurrentItem(treeWidgetItem, 0);
                treeWidget_keys->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsScript: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                case SearchResultIsName:
                    mpKeysMainArea->lineEdit_key_name->setFocus(Qt::OtherFocusReason);
                    mpKeysMainArea->lineEdit_key_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsCommand:
                    mpKeysMainArea->lineEdit_key_command->setFocus(Qt::OtherFocusReason);
                    mpKeysMainArea->lineEdit_key_command->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a KEY type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                } // End of switch()
                return;
            } // End of if
        } // End of for
        break;
    } // End of case EditorViewType::cmKeysView

    case EditorViewType::cmVarsView: {
        LuaInterface* lI = mpHost->getLuaInterface();
        VarUnit* vu = lI->getVarUnit();
        const QStringList varShort = pItem->data(0, IdRole).toStringList();
        // A stale-tree rebuild empties the variables tree without refilling it,
        // leaving mpVarBaseItem freed
        if (!treeWidget_variables->topLevelItemCount()) {
            repopulateVars();
        }
        QList<QTreeWidgetItem*> list;
        recurseVariablesDown(mpVarBaseItem, list);
        QListIterator<QTreeWidgetItem*> it(list);
        while (it.hasNext()) {
            QTreeWidgetItem* treeWidgetItem = it.next();
            TVar* var = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (vu->shortVarName(var) == varShort) {
                show_vars();
                treeWidget_variables->setCurrentItem(treeWidgetItem, 0);
                treeWidget_variables->scrollToItem(treeWidgetItem);

                highlightSearchMatches();

                switch (pItem->data(0, TypeRole).toInt()) {
                case SearchResultIsName:
                    mpVarsMainArea->lineEdit_var_name->setFocus(Qt::OtherFocusReason);
                    mpVarsMainArea->lineEdit_var_name->setCursorPosition(pItem->data(0, PositionRole).toInt());
                    break;
                case SearchResultIsValue: {
                    // Defer moveCaretTo so it fires after restoreEditorState's QTimer::singleShot(0)
                    // callback, ensuring the search result position takes precedence over the
                    // previously-saved editor state.
                    const auto line = static_cast<size_t>(pItem->data(0, PatternOrLineRole).toInt());
                    const auto column = static_cast<size_t>(pItem->data(0, PositionRole).toInt());
                    mpSourceEditorEdbee->setFocus();
                    QTimer::singleShot(0ms, this, [this, line, column]() {
                        if (mpSourceEditorEdbee) {
                            mpSourceEditorEdbee->controller()->moveCaretTo(line, column, false);
                        }
                    });
                    break;
                }
                default:
                    qDebug() << "dlgTriggerEditor::slot_item_selected_list(...) Called for a VAR type item but handler for element of type:" << treeWidgetItem->data(0, TypeRole).toInt()
                             << "not yet done/applicable...!";
                }
                return;
            }
        }
    } // End of case static_cast<int>(EditorViewType::cmVarsView)
    break;
    default: {
    } // No-op
    } // End of switch()
}

// QTreeWidgetItem::setData() reallocates the item's role list for each role it adds, and
// setAllSearchData() adds seven to each of the many rows a broad search makes
class dlgTriggerEditor::SearchResultItem : public QTreeWidgetItem
{
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    QVariant data(const int column, const int role) const override
    {
        if (column == 0 && role >= IdRole && role <= IndexRole) {
            return mRoleData[role - IdRole];
        }
        return QTreeWidgetItem::data(column, role);
    }

    void setData(const int column, const int role, const QVariant& value) override
    {
        if (column == 0 && role >= IdRole && role <= IndexRole) {
            mRoleData[role - IdRole] = value;
            emitDataChanged();
            return;
        }
        QTreeWidgetItem::setData(column, role, value);
    }

private:
    std::array<QVariant, IndexRole - IdRole + 1> mRoleData;
};

// A tab is drawn to the next tab stop but measured as one glyph when the match marker is sized, and a line break
// escapes the row. Returns the match's start in the flattened line.
static int flattenSnippet(QString& snippet, const int matchStart)
{
    // One QChar for one, so nothing before the match moves
    snippet.replace(QChar(QChar::LineFeed), QChar(QChar::Space));
    snippet.replace(QChar(QChar::CarriageReturn), QChar(QChar::Space));
    const int widened = snippet.left(std::max(0, matchStart)).count(QChar::Tabulation);
    snippet.replace(QChar(QChar::Tabulation), QString(QChar::Space).repeated(2));
    return matchStart + widened;
}

// The heading also carries its first match's navigation data, so clicking it goes where that match goes
void dlgTriggerEditor::addSearchResult(QTreeWidgetItem*& pParent, const SearchResultRow& row)
{
    const bool variable = (row.view == EditorViewType::cmVarsView);
    QString snippet = row.snippet;
    const int snippetStart = flattenSnippet(snippet, row.matchStart);

    if (!pParent) {
        pParent = new SearchResultItem(QStringList{row.title});
        if (variable) {
            setAllSearchData(pParent, row.name, row.variableId, row.what, row.matchStart, row.subInstance);
        } else {
            setAllSearchData(pParent, row.view, row.name, row.id, row.what, row.matchStart, row.instance, row.subInstance);
        }
        pParent->setData(0, uiDesign::SearchResultDelegate::RowKindRole, static_cast<int>(uiDesign::SearchResultDelegate::ItemRow));
        pParent->setData(0, uiDesign::SearchResultDelegate::TitleRole, row.title);
        pParent->setData(0, uiDesign::SearchResultDelegate::TypeLabelRole, row.typeLabel);
        //: Read out for a search result heading: %1 is the kind of thing it is (Trigger, Alias...) and %2 its name
        pParent->setData(0, Qt::AccessibleTextRole, tr("%1 %2").arg(row.typeLabel, row.title));
        // The delegate elides the name to the panel's width
        pParent->setToolTip(0, row.title);
        mPendingSearchResults.append(pParent);
    }

    auto* pItem = new SearchResultItem(QStringList{qsl("%1 %2").arg(row.where, snippet)});
    if (variable) {
        setAllSearchData(pItem, row.name, row.variableId, row.what, row.matchStart, row.subInstance);
    } else {
        setAllSearchData(pItem, row.view, row.name, row.id, row.what, row.matchStart, row.instance, row.subInstance);
    }
    pItem->setData(0, uiDesign::SearchResultDelegate::RowKindRole, static_cast<int>(uiDesign::SearchResultDelegate::MatchRow));
    pItem->setData(0, uiDesign::SearchResultDelegate::WhereRole, row.where);
    pItem->setData(0, uiDesign::SearchResultDelegate::SnippetRole, snippet);
    pItem->setData(0, uiDesign::SearchResultDelegate::MatchStartRole, snippetStart);
    pItem->setData(0, uiDesign::SearchResultDelegate::MatchLengthRole, mSearchTerm.length());
    //: Read out for one match inside a search result: %1 is where in the item it is ("Pattern 2") and %2 the line it is on
    pItem->setData(0, Qt::AccessibleTextRole, tr("%1: %2").arg(row.where, snippet));
    pItem->setToolTip(0, snippet);
    pParent->addChild(pItem);
}

void dlgTriggerEditor::slot_searchMudletItems(const int index)
{
    if (index < 0) {
        return;
    }
    const QString s{comboBox_searchTerms->itemText(index)};
    if (s.isEmpty()) {
        return;
    }

    mSearchTerm = s;
    treeWidget_searchResults->clear();
    treeWidget_searchResults->setUpdatesEnabled(false);

    searchTriggers(s);
    searchAliases(s);
    searchScripts(s);
    searchActions(s);
    searchTimers(s);
    searchKeys(s);

    if (mSearchOptions & enums::EditorSearchOptionIncludeVariables) {
        searchVariables(s);
    }

    // With sorting on, addTopLevelItems() inserts the rows one at a time, each in front of the last
    const bool sortingEnabled = treeWidget_searchResults->isSortingEnabled();
    treeWidget_searchResults->setSortingEnabled(false);
    treeWidget_searchResults->addTopLevelItems(std::exchange(mPendingSearchResults, {}));
    treeWidget_searchResults->setSortingEnabled(sortingEnabled);
    // Expansion is held by the view, so it can only be set once a row is in the tree. By row
    // rather than by item, as finding an item's row is a linear search of the top-level rows
    const QAbstractItemModel* model = treeWidget_searchResults->model();
    for (int row = 0, total = treeWidget_searchResults->topLevelItemCount(); row < total; ++row) {
        if (treeWidget_searchResults->topLevelItem(row)->childCount()) {
            treeWidget_searchResults->expand(model->index(row, 0));
        }
    }

    if (!treeWidget_searchResults->topLevelItemCount()) {
        //: Shown in place of search results when a search found nothing: %1 is what was typed into the search field, in the quotation marks the locale uses
        auto* pNotice = new QTreeWidgetItem(QStringList{tr("No matches for %1").arg(QLocale().quoteString(s))});
        pNotice->setData(0, uiDesign::SearchResultDelegate::RowKindRole, static_cast<int>(uiDesign::SearchResultDelegate::NoticeRow));
        pNotice->setData(0, uiDesign::SearchResultDelegate::TitleRole, pNotice->text(0));
        pNotice->setData(0, Qt::AccessibleTextRole, pNotice->text(0));
        pNotice->setFlags(Qt::ItemIsEnabled);
        treeWidget_searchResults->addTopLevelItem(pNotice);
    }

    mpSourceEditorEdbee->controller()->textSearcher()->setSearchTerm(s);
    mpSourceEditorEdbee->controller()->textSearcher()->setCaseSensitive(mSearchOptions & enums::EditorSearchOptionCaseSensitive);

    treeWidget_searchResults->setUpdatesEnabled(true);
    setSearchResultsShown(true);

    mpSourceEditorEdbee->controller()->update();
}

void dlgTriggerEditor::setSearchResultsShown(const bool shown)
{
    if (!mpWidget_searchResultsPane || !mpWidget_itemTreesPane) {
        return;
    }
    if (mpWidget_searchResultsPane->isVisibleTo(searchSplitter) == shown) {
        return;
    }

    mpWidget_searchResultsPane->setVisible(shown);
    mpWidget_itemTreesPane->setVisible(!shown);

    if (shown) {
        return;
    }
    // scrollToItem() does nothing on a hidden tree, so catch up on what was chosen from the results meanwhile
    for (QTreeWidget* pTreeWidget : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables, mpFolderTree}) {
        if (pTreeWidget->isVisibleTo(mpWidget_itemTreesPane) && pTreeWidget->currentItem()) {
            pTreeWidget->scrollToItem(pTreeWidget->currentItem());
        }
    }
}

// setFocus() on a hidden tree does nothing, which would leave the panel's shortcuts nowhere to fire from
void dlgTriggerEditor::focusPanelTree(QWidget* pTreeWidget)
{
    if (mpWidget_itemTreesPane && !mpWidget_itemTreesPane->isVisibleTo(searchSplitter)) {
        treeWidget_searchResults->setFocus();
        return;
    }
    pTreeWidget->setFocus();
}

void dlgTriggerEditor::searchVariables(const QString& text)
{
    if (mCurrentView != EditorViewType::cmVarsView) {
        // repopulateVars can take some time should there be a large number
        // of variables or big tables... 8-(
        repopulateVars();
    }

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* base = vu->getBase();
    QListIterator<TVar*> itBaseVarChildren(base->getChildren(false));
    while (itBaseVarChildren.hasNext()) {
        TVar* var = itBaseVarChildren.next();
        // We do not search for hidden variables - probably because we would
        // have to unhide all of them to show the hidden ones found by
        // searching
        if (!showHiddenVars && vu->isHidden(var)) {
            continue;
        }

        //recurse down this variable
        QList<TVar*> list;
        recursiveSearchVariables(var, list, false);
        QListIterator<TVar*> itVarDecendent(list);
        while (itVarDecendent.hasNext()) {
            TVar* varDecendent = itVarDecendent.next();
            if (!showHiddenVars && vu->isHidden(varDecendent)) {
                continue;
            }

            QTreeWidgetItem* parent = nullptr;
            const QString name = varDecendent->getName();
            const QString value = varDecendent->getValue();
            const QStringList idStringList = vu->shortVarName(varDecendent);
            QString idString;
            // Take the first element - to comply with lua requirement it
            // must begin with not a digit and not contain any spaces so is
            // a string - and it is used "unquoted" as is to be the base
            // of a lua table
            if (idStringList.size() > 1) {
                QStringList midStrings = idStringList;
                idString = midStrings.takeFirst();
                QStringListIterator itSubString(midStrings);
                while (itSubString.hasNext()) {
                    const QString intermediate = itSubString.next();
                    bool isOk = false;
                    const int numberValue = intermediate.toInt(&isOk);
                    if (isOk && QString::number(numberValue) == intermediate) {
                        // This seems to be an integer
                        idString.append(qsl("[%1]").arg(intermediate));
                    } else {
                        idString.append(qsl("[\"%1\"]").arg(intermediate));
                    }
                }
            } else if (!idStringList.empty()) {
                idString = idStringList.at(0);
            }

            SearchResultRow row;
            row.view = EditorViewType::cmVarsView;
            row.typeLabel = tr("Variable");
            // Titled by the expression that reaches it, not the bare key its parent table knows it by
            row.title = idString;
            row.name = name;
            row.variableId = vu->shortVarName(varDecendent);

            int startPos = 0;
            if ((startPos = findSearchMatch(name, text)) != -1) {
                // We do not (yet) worry about multiple search results in the "name"
                row.where = tr("Name");
                row.snippet = name;
                row.matchStart = startPos;
                row.what = SearchResultIsName;
                addSearchResult(parent, row);
            }

            // The additional first test is needed to exclude the case when
            // the search term matches on the word "function" which will
            // appear in EVERY "value" for a lua function in the variable
            // tree widget...
            if (value != QLatin1String("function") && (startPos = findSearchMatch(value, text)) != -1) {
                // We do not (yet) worry about multiple search results in the "value"
                row.where = tr("Value");
                row.snippet = value;
                row.matchStart = startPos;
                row.what = SearchResultIsValue;
                addSearchResult(parent, row);
            }
        }
    }
}

void dlgTriggerEditor::searchKeys(const QString& text)
{
    std::list<TKey*> const nodes = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : nodes) {
        searchSingleKey(key, text);
        recursiveSearchKeys(key, text);
    }
}

void dlgTriggerEditor::searchSingleTimer(TTimer* timer, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = timer->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmTimerView;
    row.typeLabel = tr("Timer");
    row.title = name;
    row.name = name;
    row.id = timer->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(timer->getCommand(), text)) != -1) {
        row.where = tr("Command");
        row.snippet = timer->getCommand();
        row.matchStart = startPos;
        row.what = SearchResultIsCommand;
        addSearchResult(parent, row);
    }

    emitScriptSearchMatches(timer->getScript(), text, name, timer->getID(), tr("Timer"), EditorViewType::cmTimerView, parent);
}

void dlgTriggerEditor::searchSingleKey(TKey* key, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = key->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmKeysView;
    row.typeLabel = tr("Key");
    row.title = name;
    row.name = name;
    row.id = key->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(key->getCommand(), text)) != -1) {
        row.where = tr("Command");
        row.snippet = key->getCommand();
        row.matchStart = startPos;
        row.what = SearchResultIsCommand;
        addSearchResult(parent, row);
    }

    emitScriptSearchMatches(key->getScript(), text, name, key->getID(), tr("Key"), EditorViewType::cmKeysView, parent);
}

void dlgTriggerEditor::highlightSearchMatches()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->markAll(controller->borderedTextRanges());
    controller->update();
}

void dlgTriggerEditor::emitScriptSearchMatches(
        const QString& scriptText, const QString& searchText, const QString& name, int objectId, const QString& parentLabel, EditorViewType viewType, QTreeWidgetItem*& parent)
{
    // Most scripts hold no match, and splitting each one into lines costs more than one look at it whole
    if (!containsSearchMatch(scriptText, searchText)) {
        return;
    }
    const QStringList textList = scriptText.split(qsl("\n"));
    const int total = textList.count();

    SearchResultRow row;
    row.view = viewType;
    row.typeLabel = parentLabel;
    row.title = name;
    row.name = name;
    row.id = objectId;
    row.what = SearchResultIsScript;

    for (int index = 0; index < total; ++index) {
        if (textList.at(index).isEmpty() || !containsSearchMatch(textList.at(index), searchText)) {
            continue;
        }

        int instance = 0;
        int startPos = 0;
        while ((startPos = findSearchMatch(textList.at(index), searchText, startPos)) != -1) {
            //: Where in an item a search match is: %1 is the line number in its Lua code and %2 the column on that line
            row.where = tr("Lua %1:%2").arg(index + 1).arg(startPos + 1);
            row.snippet = textList.at(index);
            row.matchStart = startPos;
            row.instance = index;
            row.subInstance = instance++;
            addSearchResult(parent, row);
            ++startPos;
        }
    }
}

void dlgTriggerEditor::searchTimers(const QString& text)
{
    std::list<TTimer*> const nodes = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : nodes) {
        searchSingleTimer(timer, text);
        recursiveSearchTimers(timer, text);
    }
}

void dlgTriggerEditor::searchSingleAction(TAction* action, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = action->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmActionView;
    row.typeLabel = tr("Button");
    row.title = name;
    row.name = name;
    row.id = action->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(action->getCommandButtonDown(), text)) != -1) {
        //: Where in a push-down button a search match is: the command it sends when pushed down
        row.where = action->isPushDownButton() ? tr("Command (down)") : tr("Command");
        row.snippet = action->getCommandButtonDown();
        row.matchStart = startPos;
        row.what = SearchResultIsCommand;
        addSearchResult(parent, row);
    }

    if (action->isPushDownButton()) {
        if ((startPos = findSearchMatch(action->getCommandButtonUp(), text)) != -1) {
            //: Where in a push-down button a search match is: the command it sends when let back up
            row.where = tr("Command (up)");
            row.snippet = action->getCommandButtonUp();
            row.matchStart = startPos;
            row.what = SearchResultIsExtraCommand;
            addSearchResult(parent, row);
        }
    }

    QStringList textList = action->css.split("\n");
    int total = textList.count();
    for (int index = 0; index < total; ++index) {
        if (textList.at(index).isEmpty() || !containsSearchMatch(textList.at(index), text)) {
            continue;
        }

        int instance = 0;
        startPos = 0;
        while ((startPos = findSearchMatch(textList.at(index), text, startPos)) != -1) {
            //: Where in a button a search match is: %1 is the line number in its stylesheet and %2 the column on that line
            row.where = tr("Style %1:%2").arg(index + 1).arg(startPos + 1);
            row.snippet = textList.at(index);
            row.matchStart = startPos;
            row.what = SearchResultsIsCss;
            row.instance = index;
            row.subInstance = instance++;
            addSearchResult(parent, row);
            ++startPos;
        }
    }

    emitScriptSearchMatches(action->getScript(), text, name, action->getID(), tr("Button"), EditorViewType::cmActionView, parent);
}

void dlgTriggerEditor::searchActions(const QString& text)
{
    std::list<TAction*> const nodes = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : nodes) {
        searchSingleAction(action, text);
        recursiveSearchActions(action, text);
    }
}

void dlgTriggerEditor::searchSingleScript(TScript* script, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = script->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmScriptView;
    row.typeLabel = tr("Script");
    row.title = name;
    row.name = name;
    row.id = script->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    QStringList textList = script->getEventHandlerList();
    int total = textList.count();
    for (int index = 0; index < total; ++index) {
        if (textList.at(index).isEmpty() || !containsSearchMatch(textList.at(index), text)) {
            continue;
        }

        int instance = 0;
        startPos = 0;
        while ((startPos = findSearchMatch(textList.at(index), text, startPos)) != -1) {
            //: Where in a script a search match is: one of the events it is registered for
            row.where = tr("Event");
            row.snippet = textList.at(index);
            row.matchStart = startPos;
            row.what = SearchResultIsEventHandler;
            row.instance = index;
            row.subInstance = instance++;
            addSearchResult(parent, row);
            ++startPos;
        }
    }

    emitScriptSearchMatches(script->getScript(), text, name, script->getID(), tr("Script"), EditorViewType::cmScriptView, parent);
}

void dlgTriggerEditor::searchScripts(const QString& text)
{
    std::list<TScript*> const nodes = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : nodes) {
        searchSingleScript(script, text);
        recursiveSearchScripts(script, text);
    }
}

void dlgTriggerEditor::searchSingleAlias(TAlias* alias, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = alias->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmAliasView;
    row.typeLabel = tr("Alias");
    row.title = name;
    row.name = name;
    row.id = alias->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(alias->getCommand(), text)) != -1) {
        row.where = tr("Command");
        row.snippet = alias->getCommand();
        row.matchStart = startPos;
        row.what = SearchResultIsCommand;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(alias->getRegexCode(), text)) != -1) {
        row.where = tr("Pattern");
        row.snippet = alias->getRegexCode();
        row.matchStart = startPos;
        row.what = SearchResultIsPattern;
        addSearchResult(parent, row);
    }

    emitScriptSearchMatches(alias->getScript(), text, name, alias->getID(), tr("Alias"), EditorViewType::cmAliasView, parent);
}

void dlgTriggerEditor::searchAliases(const QString& text)
{
    std::list<TAlias*> const nodes = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : nodes) {
        searchSingleAlias(alias, text);
        recursiveSearchAlias(alias, text);
    }
}

void dlgTriggerEditor::searchSingleTrigger(TTrigger* trigger, const QString& text)
{
    QTreeWidgetItem* parent = nullptr;
    const QString name = trigger->getName();
    int startPos = 0;

    SearchResultRow row;
    row.view = EditorViewType::cmTriggerView;
    row.typeLabel = tr("Trigger");
    row.title = name;
    row.name = name;
    row.id = trigger->getID();

    if ((startPos = findSearchMatch(name, text)) != -1) {
        row.where = tr("Name");
        row.snippet = name;
        row.matchStart = startPos;
        row.what = SearchResultIsName;
        addSearchResult(parent, row);
    }

    if ((startPos = findSearchMatch(trigger->getCommand(), text)) != -1) {
        row.where = tr("Command");
        row.snippet = trigger->getCommand();
        row.matchStart = startPos;
        row.what = SearchResultIsCommand;
        addSearchResult(parent, row);
    }

    QStringList textList = trigger->getPatternsList();
    int total = textList.count();
    for (int index = 0; index < total; ++index) {
        if (textList.at(index).isEmpty() || !containsSearchMatch(textList.at(index), text)) {
            continue;
        }

        int instance = 0;
        startPos = 0;
        while ((startPos = findSearchMatch(textList.at(index), text, startPos)) != -1) {
            //: Where in a trigger a search match is: %1 is which of its patterns
            row.where = tr("Pattern %1").arg(index + 1);
            row.snippet = textList.at(index);
            row.matchStart = startPos;
            row.what = SearchResultIsPattern;
            row.instance = index;
            row.subInstance = instance++;
            addSearchResult(parent, row);
            ++startPos;
        }
    }

    emitScriptSearchMatches(trigger->getScript(), text, name, trigger->getID(), tr("Trigger"), EditorViewType::cmTriggerView, parent);
}

void dlgTriggerEditor::searchTriggers(const QString& text)
{
    std::list<TTrigger*> const nodes = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : nodes) {
        searchSingleTrigger(trigger, text);
        recursiveSearchTriggers(trigger, text);
    }
}

void dlgTriggerEditor::recursiveSearchTriggers(TTrigger* pTriggerParent, const QString& text)
{
    std::list<Tree<TTrigger>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* triggerNode : *childrenList) {
        auto* trigger = static_cast<TTrigger*>(triggerNode);
        searchSingleTrigger(trigger, text);
        if (trigger->hasChildren()) {
            recursiveSearchTriggers(trigger, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchAlias(TAlias* pTriggerParent, const QString& text)
{
    std::list<Tree<TAlias>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* aliasNode : *childrenList) {
        auto* alias = static_cast<TAlias*>(aliasNode);
        searchSingleAlias(alias, text);
        if (alias->hasChildren()) {
            recursiveSearchAlias(alias, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchScripts(TScript* pTriggerParent, const QString& text)
{
    std::list<Tree<TScript>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* scriptNode : *childrenList) {
        auto* script = static_cast<TScript*>(scriptNode);
        searchSingleScript(script, text);
        if (script->hasChildren()) {
            recursiveSearchScripts(script, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchActions(TAction* pTriggerParent, const QString& text)
{
    std::list<Tree<TAction>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* actionNode : *childrenList) {
        auto* action = static_cast<TAction*>(actionNode);
        searchSingleAction(action, text);
        if (action->hasChildren()) {
            recursiveSearchActions(action, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchTimers(TTimer* pTriggerParent, const QString& text)
{
    std::list<Tree<TTimer>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* timerNode : *childrenList) {
        auto* timer = static_cast<TTimer*>(timerNode);
        searchSingleTimer(timer, text);
        if (timer->hasChildren()) {
            recursiveSearchTimers(timer, text);
        }
    }
}

void dlgTriggerEditor::recursiveSearchKeys(TKey* pTriggerParent, const QString& text)
{
    std::list<Tree<TKey>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* keyNode : *childrenList) {
        auto* key = static_cast<TKey*>(keyNode);
        searchSingleKey(key, text);
        if (key->hasChildren()) {
            recursiveSearchKeys(key, text);
        }
    }
}


// Undo restores an item from its own snapshot only when no ancestor was deleted along with it
static bool hasSelectedAncestor(const QTreeWidgetItem* pItem, const QSet<QTreeWidgetItem*>& selectedItemsSet, const QTreeWidgetItem* pBaseItem)
{
    for (QTreeWidgetItem* pParent = pItem->parent(); pParent && pParent != pBaseItem; pParent = pParent->parent()) {
        if (selectedItemsSet.contains(pParent)) {
            return true;
        }
    }
    return false;
}

void dlgTriggerEditor::delete_alias()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_aliases->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    const QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};

    QStringList itemNames;
    QList<TAlias*> aliasesToDelete;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            aliasesToDelete << pT;
        }
    }

    if (aliasesToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture an alias and all its descendants
    std::function<void(TAlias*, int, int, bool)> captureAliasAndChildren = [&](TAlias* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("AliasSnapshot");
            XMLexport exporter(pT);
            exporter.writeAlias(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureAliasAndChildren(static_cast<TAlias*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    // Helper lambda to calculate position in data model (not tree widget)
    auto calculatePosition = [](TAlias* item) -> int {
        if (!item) {
            return 0;
        }
        TAlias* parent = item->getParent();
        if (!parent) {
            return 0;
        }
        auto* childrenList = parent->getChildrenList();
        if (!childrenList) {
            return 0;
        }
        int position = 0;
        for (auto* child : *childrenList) {
            if (child == item) {
                return position;
            }
            position++;
        }
        return 0;
    };

    // Capture each selected alias and all its descendants
    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            int parentID = -1;
            int positionInParent = 0;

            TAlias* parent = pT->getParent();
            if (parent) {
                parentID = parent->getID();
                positionInParent = calculatePosition(pT);
            } else {
                parentID = -1;
                auto rootList = mpHost->getAliasUnit()->getAliasRootNodeList();
                int pos = 0;
                for (auto* rootItem : rootList) {
                    if (rootItem == pT) {
                        positionInParent = pos;
                        break;
                    }
                    pos++;
                }
            }

            captureAliasAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpAliasBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_aliases->indexFromItem(a);
        QModelIndex indexB = treeWidget_aliases->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    // Emptied silently so each removed row doesn't re-lay out the rest of the selection and reload
    // the form, and so the setCurrentItem() below is a real change the view gets told about
    QSignalBlocker selectionBlocker(treeWidget_aliases->selectionModel());
    treeWidget_aliases->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TAlias* pT = mpHost->getAliasUnit()->getAlias(itemId);

        if (pT) {
            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_aliases->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpAliasBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmAliasView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmAliasView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    // Detaching an item nulls treeWidget() on its whole subtree, which is how a
    // newSelection that sat inside another removed subtree is caught here:
    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpAliasBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentAliasItem = newSelection;
        treeWidget_aliases->setCurrentItem(newSelection);
        slot_aliasSelected(newSelection);
    } else {
        mpCurrentAliasItem = nullptr;
        clearAliasForm();
    }

    // Has to stay after the selection handling: the slots it fires still read
    // the detached items.
    qDeleteAll(removedItems);
}

void dlgTriggerEditor::delete_action()
{
    QList<QTreeWidgetItem*> initiallySelectedItems = treeWidget_actions->selectedItems();
    if (initiallySelectedItems.isEmpty()) {
        return;
    }

    // Capture each selected action and all its descendants
    // and put them into here:
    QList<QTreeWidgetItem*> selectedItems;
    for (const auto& item : std::as_const(initiallySelectedItems)) {
        treeWidget_actions->getAllChildren(item, selectedItems);
    }

    // Remove any duplicates by converting to a set and back to a list:
    QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};
    selectedItems = QList<QTreeWidgetItem*>{selectedItemsSet.cbegin(), selectedItemsSet.cend()};

    QStringList itemNames;
    QList<TAction*> actionsToDelete;
    for (const QTreeWidgetItem* pItem : std::as_const(initiallySelectedItems)) {
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            actionsToDelete << pT;
        }
    }

    if (actionsToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture an action and all its descendants
    std::function<void(TAction*, int, int, bool)> captureActionAndChildren = [&](TAction* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("ActionSnapshot");
            XMLexport exporter(pT);
            exporter.writeAction(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureActionAndChildren(static_cast<TAction*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            // Determine parent ID and position
            int parentID = -1;
            int positionInParent = 0;

            QTreeWidgetItem* pParentItem = pItem->parent();
            if (pParentItem && pParentItem != mpActionBaseItem) {
                parentID = pParentItem->data(0, Qt::UserRole).toInt();
                positionInParent = pParentItem->indexOfChild(pItem);
            } else {
                parentID = -1;
                positionInParent = mpActionBaseItem->indexOfChild(pItem);
            }

            // Recursively capture this action and all its children
            captureActionAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpActionBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_actions->indexFromItem(a);
        QModelIndex indexB = treeWidget_actions->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_actions->selectionModel());
    treeWidget_actions->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TAction* pT = mpHost->getActionUnit()->getAction(itemId);

        if (pT) {
            if (pT->isActive()) {
                pT->deactivate();
            }
            pT->setDataChanged();

            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_actions->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpActionBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmActionView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmActionView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpActionBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentActionItem = newSelection;
        treeWidget_actions->setCurrentItem(newSelection);
        slot_actionSelected(newSelection);
    } else {
        mpCurrentActionItem = nullptr;
        clearActionForm();
    }

    qDeleteAll(removedItems);

    mpHost->getActionUnit()->updateAllToolbars();
}

void dlgTriggerEditor::delete_variable()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_variables->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QStringList itemNames;
    QList<TVar*> varsToDelete;
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TVar* var = treeWidget_variables->variableForRow(vu, pItem);
        if (var) {
            itemNames << var->getName();
            varsToDelete << var;
        }
    }

    if (varsToDelete.isEmpty()) {
        return;
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_variables->indexFromItem(a);
        QModelIndex indexB = treeWidget_variables->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_variables->selectionModel());
    treeWidget_variables->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        TVar* var = treeWidget_variables->variableForRow(vu, pItem);

        if (var) {
            lI->deleteVar(var);
            TVar* parent = var->getParent();
            if (parent) {
                parent->removeChild(var);
            }

            if (pParentItem && !newSelection) {
                newSelection = pParentItem;
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
                // Deleting the TVar below frees its descendants too and nothing
                // unregisters those, so drop the whole detached subtree from the
                // lookup maps now: a later pass over a selected descendant would
                // otherwise resolve a freed TVar, as would a recycled item address:
                QList<QTreeWidgetItem*> pendingPurge{pItem};
                while (!pendingPurge.isEmpty()) {
                    QTreeWidgetItem* pEntry = pendingPurge.takeLast();
                    treeWidget_variables->forgetRow(pEntry);
                    for (int i = 0; i < pEntry->childCount(); ++i) {
                        pendingPurge.append(pEntry->child(i));
                    }
                }
            }
            delete var;
        }
    }
    selectionBlocker.unblock();

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpVarBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentVarItem = newSelection;
        treeWidget_variables->setCurrentItem(newSelection);
        slot_variableSelected(newSelection);
    } else {
        mpCurrentVarItem = nullptr;
        clearVarForm();
    }

    qDeleteAll(removedItems);
}

void dlgTriggerEditor::delete_script()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_scripts->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    const QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};

    QStringList itemNames;
    QList<TScript*> scriptsToDelete;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            scriptsToDelete << pT;
        }
    }

    if (scriptsToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture a script and all its descendants
    std::function<void(TScript*, int, int, bool)> captureScriptAndChildren = [&](TScript* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("ScriptSnapshot");
            XMLexport exporter(pT);
            exporter.writeScript(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureScriptAndChildren(static_cast<TScript*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    // Capture each selected script and all its descendants
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            // Determine parent ID and position
            int parentID = -1;
            int positionInParent = 0;

            QTreeWidgetItem* pParentItem = pItem->parent();
            if (pParentItem && pParentItem != mpScriptsBaseItem) {
                parentID = pParentItem->data(0, Qt::UserRole).toInt();
                positionInParent = pParentItem->indexOfChild(pItem);
            } else {
                parentID = -1;
                positionInParent = mpScriptsBaseItem->indexOfChild(pItem);
            }

            // Recursively capture this script and all its children
            captureScriptAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpScriptsBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_scripts->indexFromItem(a);
        QModelIndex indexB = treeWidget_scripts->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_scripts->selectionModel());
    treeWidget_scripts->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TScript* pT = mpHost->getScriptUnit()->getScript(itemId);

        if (pT) {
            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_scripts->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpScriptsBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmScriptView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmScriptView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpScriptsBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentScriptItem = newSelection;
        treeWidget_scripts->setCurrentItem(newSelection);
        slot_scriptsSelected(newSelection);
    } else {
        mpCurrentScriptItem = nullptr;
        clearScriptForm();
    }

    qDeleteAll(removedItems);
}

void dlgTriggerEditor::delete_key()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_keys->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    const QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};

    QStringList itemNames;
    QList<TKey*> keysToDelete;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            keysToDelete << pT;
        }
    }

    if (keysToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture a key and all its descendants
    std::function<void(TKey*, int, int, bool)> captureKeyAndChildren = [&](TKey* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("KeySnapshot");
            XMLexport exporter(pT);
            exporter.writeKey(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureKeyAndChildren(static_cast<TKey*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    // Capture each selected key and all its descendants
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            // Determine parent ID and position
            int parentID = -1;
            int positionInParent = 0;

            QTreeWidgetItem* pParentItem = pItem->parent();
            if (pParentItem && pParentItem != mpKeyBaseItem) {
                parentID = pParentItem->data(0, Qt::UserRole).toInt();
                positionInParent = pParentItem->indexOfChild(pItem);
            } else {
                parentID = -1;
                positionInParent = mpKeyBaseItem->indexOfChild(pItem);
            }

            // Recursively capture this key and all its children
            captureKeyAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpKeyBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_keys->indexFromItem(a);
        QModelIndex indexB = treeWidget_keys->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_keys->selectionModel());
    treeWidget_keys->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(itemId);

        if (pT) {
            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_keys->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpKeyBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmKeysView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmKeysView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpKeyBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentKeyItem = newSelection;
        treeWidget_keys->setCurrentItem(newSelection);
        slot_keySelected(newSelection);
    } else {
        mpCurrentKeyItem = nullptr;
        clearKeyForm();
    }

    qDeleteAll(removedItems);
}

void dlgTriggerEditor::delete_trigger()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    const QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};

    QStringList itemNames;
    QList<TTrigger*> triggersToDelete;

    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            triggersToDelete << pT;
        }
    }

    if (triggersToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture a trigger and all its descendants
    std::function<void(TTrigger*, int, int, bool)> captureTriggerAndChildren = [&](TTrigger* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("TriggerSnapshot");
            XMLexport exporter(pT);
            exporter.writeTrigger(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureTriggerAndChildren(static_cast<TTrigger*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    // Capture each selected trigger and all its descendants
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            // Determine parent ID and position
            int parentID = -1;
            int positionInParent = 0;

            QTreeWidgetItem* pParentItem = pItem->parent();
            if (pParentItem) {
                if (pParentItem == mpTriggerBaseItem) {
                    parentID = -1;
                    positionInParent = mpTriggerBaseItem->indexOfChild(pItem);
                } else {
                    parentID = pParentItem->data(0, Qt::UserRole).toInt();
                    positionInParent = pParentItem->indexOfChild(pItem);
                }
            } else {
                parentID = -1;
                positionInParent = treeWidget_triggers->indexOfTopLevelItem(pItem);
            }

            // Recursively capture this trigger and all its children
            captureTriggerAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpTriggerBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_triggers->indexFromItem(a);
        QModelIndex indexB = treeWidget_triggers->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_triggers->selectionModel());
    treeWidget_triggers->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(itemId);

        if (pT) {
            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_triggers->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpTriggerBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmTriggerView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmTriggerView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpTriggerBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentTriggerItem = newSelection;
        treeWidget_triggers->setCurrentItem(newSelection);
        slot_triggerSelected(newSelection);
    } else {
        mpCurrentTriggerItem = nullptr;
        clearTriggerForm();
    }

    qDeleteAll(removedItems);
}

void dlgTriggerEditor::delete_timer()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_timers->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    const QSet<QTreeWidgetItem*> selectedItemsSet{selectedItems.cbegin(), selectedItems.cend()};

    QStringList itemNames;
    QList<TTimer*> timersToDelete;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            itemNames << pT->getName();
            timersToDelete << pT;
        }
    }

    if (timersToDelete.isEmpty()) {
        return;
    }

    // Capture state of all items BEFORE deletion for undo
    QList<EditorDeleteItemCommand::DeletedItemInfo> deletedItems;

    // Recursive lambda to capture a timer and all its descendants
    std::function<void(TTimer*, int, int, bool)> captureTimerAndChildren = [&](TTimer* pT, int parentID, int positionInParent, bool withSnapshot) {
        if (!pT) {
            return;
        }

        EditorDeleteItemCommand::DeletedItemInfo info;
        info.itemID = pT->getID();
        info.itemName = pT->getName();
        info.parentID = parentID;
        info.positionInParent = positionInParent;

        if (withSnapshot) {
            pugi::xml_document doc;
            auto root = doc.append_child("TimerSnapshot");
            XMLexport exporter(pT);
            exporter.writeTimer(pT, root);
            std::ostringstream oss;
            doc.save(oss);
            info.xmlSnapshot = QString::fromStdString(oss.str());
        }

        deletedItems.append(info);

        if (pT->mpMyChildrenList) {
            int i = 0;
            for (auto* pChild : *pT->mpMyChildrenList) {
                captureTimerAndChildren(static_cast<TTimer*>(pChild), pT->getID(), i, false);
                ++i;
            }
        }
    };

    // Capture each selected timer and all its descendants
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
        if (pT) {
            // Determine parent ID and position
            int parentID = -1;
            int positionInParent = 0;

            QTreeWidgetItem* pParentItem = pItem->parent();
            if (pParentItem && pParentItem != mpTimerBaseItem) {
                parentID = pParentItem->data(0, Qt::UserRole).toInt();
                positionInParent = pParentItem->indexOfChild(pItem);
            } else {
                parentID = -1;
                positionInParent = mpTimerBaseItem->indexOfChild(pItem);
            }

            // Recursively capture this timer and all its children
            captureTimerAndChildren(pT, parentID, positionInParent, !hasSelectedAncestor(pItem, selectedItemsSet, mpTimerBaseItem));
        }
    }

    // Sort items by their position in tree (top to bottom) to delete correctly
    std::sort(selectedItems.begin(), selectedItems.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        QModelIndex indexA = treeWidget_timers->indexFromItem(a);
        QModelIndex indexB = treeWidget_timers->indexFromItem(b);
        return indexA.row() < indexB.row();
    });

    // Delete in reverse order to maintain valid indices
    std::reverse(selectedItems.begin(), selectedItems.end());

    QTreeWidgetItem* newSelection = nullptr;
    QList<QTreeWidgetItem*> removedItems;
    QSignalBlocker selectionBlocker(treeWidget_timers->selectionModel());
    treeWidget_timers->selectionModel()->clear();
    for (QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        QTreeWidgetItem* pParentItem = pItem->parent();
        const int itemId = pItem->data(0, Qt::UserRole).toInt();
        TTimer* pT = mpHost->getTimerUnit()->getTimer(itemId);

        if (pT) {
            if (!newSelection) {
                // Try to select sibling above, then parent, then base item
                int itemIndex = pParentItem ? pParentItem->indexOfChild(pItem) : treeWidget_timers->indexOfTopLevelItem(pItem);
                if (itemIndex > 0 && pParentItem) {
                    // Select sibling above
                    newSelection = pParentItem->child(itemIndex - 1);
                } else if (pParentItem) {
                    // No sibling above, select parent
                    newSelection = pParentItem;
                } else {
                    // Top-level item with no sibling above, select base item
                    newSelection = mpTimerBaseItem;
                }
            }
            if (pParentItem) {
                pParentItem->removeChild(pItem);
                removedItems.append(pItem);
            }
            clearEditorState(EditorViewType::cmTimerView, itemId);
            delete pT;
        }
    }
    selectionBlocker.unblock();

    if (!deletedItems.isEmpty()) {
        auto* qtCmd = new EditorDeleteItemCommand(EditorViewType::cmTimerView, deletedItems, mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }

    if (newSelection && !newSelection->treeWidget()) {
        newSelection = mpTimerBaseItem;
    }

    // Set new selection
    if (newSelection) {
        mpCurrentTimerItem = newSelection;
        treeWidget_timers->setCurrentItem(newSelection);
        slot_timerSelected(newSelection);
    } else {
        mpCurrentTimerItem = nullptr;
        clearTimerForm();
    }

    qDeleteAll(removedItems);
}


void dlgTriggerEditor::activeToggle_trigger()
{
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();
    pT->setIsActive(!oldState);
    bool newState = pT->isActive();

    if (pT->isFilterChain()) {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActiveFilterChain;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFilterChain;
        }
    } else if (pT->isFolder()) {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActiveFolder;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActive;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

    if (pItem->childCount() > 0) {
        children_icon_triggers(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmTriggerView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::slot_itemMoved(int itemID, int oldParentID, int newParentID, int oldPosition, int newPosition)
{
    if (!mpUndoStack) {
        return;
    }

    // Determine which view this move belongs to
    EditorViewType viewType;
    QString itemName;

    // Check which tree widget has focus or which view is active
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView: {
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(itemID);
        if (pT) {
            viewType = EditorViewType::cmTriggerView;
            itemName = pT->getName();
        } else {
            return;
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        TAlias* pA = mpHost->getAliasUnit()->getAlias(itemID);
        if (pA) {
            viewType = EditorViewType::cmAliasView;
            itemName = pA->getName();
        } else {
            return;
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        TTimer* pT = mpHost->getTimerUnit()->getTimer(itemID);
        if (pT) {
            viewType = EditorViewType::cmTimerView;
            itemName = pT->getName();
            // Becoming or ceasing to be an offset timer changes the interval sentence; deferred because the tree
            // announces the move before the timer unit has carried it out
            if (mpCurrentTimerItem && mpCurrentTimerItem->data(0, Qt::UserRole).toInt() == itemID) {
                QTimer::singleShot(0ms, this, &dlgTriggerEditor::refreshShownTimerIntervalSentence);
            }
        } else {
            return;
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        TScript* pS = mpHost->getScriptUnit()->getScript(itemID);
        if (pS) {
            viewType = EditorViewType::cmScriptView;
            itemName = pS->getName();
        } else {
            return;
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        TKey* pK = mpHost->getKeyUnit()->getKey(itemID);
        if (pK) {
            viewType = EditorViewType::cmKeysView;
            itemName = pK->getName();
        } else {
            return;
        }
        break;
    }
    case EditorViewType::cmActionView: {
        TAction* pA = mpHost->getActionUnit()->getAction(itemID);
        if (pA) {
            viewType = EditorViewType::cmActionView;
            itemName = pA->getName();
        } else {
            return;
        }
        break;
    }
    default:
        return;
    }

    // Push move command to undo system
    auto* qtCmd = new EditorMoveItemCommand(viewType, itemID, oldParentID, newParentID, oldPosition, newPosition, itemName, mpHost);
    mpUndoStack->pushCommand(qtCmd);
}

void dlgTriggerEditor::slot_batchMoveStarted()
{
    if (!mpUndoStack) {
        return;
    }

    mpUndoStack->beginMacro(tr("move items"));
}

void dlgTriggerEditor::slot_batchMoveEnded()
{
    if (!mpUndoStack) {
        return;
    }

    mpUndoStack->endMacro();
}

void dlgTriggerEditor::moveCurrentItem(const KeyboardMove move)
{
    const EditorViewType view = resolveCurrentView();
    if (view == EditorViewType::cmVarsView || view == EditorViewType::cmUnknownView) {
        return;
    }

    QTreeWidget* pTree = treeForView(view);
    QTreeWidgetItem* pItem = pTree ? pTree->currentItem() : nullptr;
    QTreeWidgetItem* pParentItem = pItem ? pItem->parent() : nullptr;
    if (!pItem) {
        return;
    }
    // A row with no parent is the tree's heading; a module's own folder is pinned in place, as it is for drag and drop
    if (!pParentItem || !(pItem->flags() & Qt::ItemIsDragEnabled)) {
        //: Screen reader announcement when an editor move shortcut is used on the tree's heading or on a module's own folder; %1 is the row's name
        mudlet::self()->announce(tr("%1 cannot be moved").arg(pItem->text(0)), QString(), true);
        return;
    }

    const int itemID = pItem->data(0, Qt::UserRole).toInt();
    const int oldParentID = pParentItem->data(0, Qt::UserRole).toInt();
    const int indexInTree = pParentItem->indexOfChild(pItem);
    // Positions are counted in the unit, which also holds temporary items the tree never shows
    const QList<int> siblingIDs = unitChildIDs(view, oldParentID);
    const int oldPosition = siblingIDs.indexOf(itemID);
    QTreeWidgetItem* pAbove = pParentItem->child(indexInTree - 1);
    QTreeWidgetItem* pBelow = pParentItem->child(indexInTree + 1);
    // The item named in the announcement: the one it passes, or the group it leaves or joins
    QTreeWidgetItem* pLandmark = nullptr;
    int newParentID = oldParentID;
    int newPosition = -1;
    // Filled in once saving has settled both names
    QString outcome;

    switch (move) {
    case KeyboardMove::Up:
        if (!pAbove) {
            //: Screen reader announcement when the editor's Move Up shortcut is used on an item already first in its group; %1 is the item's name
            mudlet::self()->announce(tr("%1 is already at the top of its group").arg(pItem->text(0)), QString(), true);
            return;
        }
        pLandmark = pAbove;
        newPosition = siblingIDs.indexOf(pAbove->data(0, Qt::UserRole).toInt());
        //: Screen reader announcement after an item in the editor's tree is moved up; %1 is its name, %2 the item it now sits above
        outcome = tr("Moved %1 above %2");
        break;
    case KeyboardMove::Down:
        if (!pBelow) {
            //: Screen reader announcement when the editor's Move Down shortcut is used on an item already last in its group; %1 is the item's name
            mudlet::self()->announce(tr("%1 is already at the bottom of its group").arg(pItem->text(0)), QString(), true);
            return;
        }
        pLandmark = pBelow;
        // Taking the item out shifts the one below up a place, so its old position is the one just after it
        newPosition = siblingIDs.indexOf(pBelow->data(0, Qt::UserRole).toInt());
        //: Screen reader announcement after an item in the editor's tree is moved down; %1 is its name, %2 the item it now sits below
        outcome = tr("Moved %1 below %2");
        break;
    case KeyboardMove::OutOfGroup: {
        QTreeWidgetItem* pGrandParentItem = pParentItem->parent();
        if (!pGrandParentItem) {
            //: Screen reader announcement when the editor's Move Out of Group shortcut is used on an item in no group; %1 is the item's name
            mudlet::self()->announce(tr("%1 is not in a group").arg(pItem->text(0)), QString(), true);
            return;
        }
        pLandmark = pParentItem;
        newParentID = pGrandParentItem->data(0, Qt::UserRole).toInt();
        const int groupPosition = unitChildIDs(view, newParentID).indexOf(oldParentID);
        newPosition = groupPosition < 0 ? -1 : groupPosition + 1;
        //: Screen reader announcement after an item in the editor's tree is moved out of a group; %1 is its name, %2 the group it left
        outcome = tr("Moved %1 out of %2");
        break;
    }
    case KeyboardMove::IntoGroupAbove:
        // A trigger already holding others is a filter chain, a group as much as a folder is; elsewhere
        // nesting under a plain item changes what the item does, such as making a timer an offset timer
        if (!pAbove || !(folderItem(view, pAbove->data(0, Qt::UserRole).toInt()) || (view == EditorViewType::cmTriggerView && pAbove->childCount() > 0))) {
            //: Screen reader announcement when the editor's Move Into Group Above shortcut is used on an item with no group just above it; %1 is the item's name
            mudlet::self()->announce(tr("There is no group just above %1").arg(pItem->text(0)), QString(), true);
            return;
        }
        pLandmark = pAbove;
        newParentID = pAbove->data(0, Qt::UserRole).toInt();
        newPosition = unitChildIDs(view, newParentID).size();
        //: Screen reader announcement after an item in the editor's tree is moved into a group; %1 is its name, %2 the group it is now in
        outcome = tr("Moved %1 into %2");
        break;
    }

    if (oldPosition < 0 || newPosition < 0) {
        return;
    }

    // The form is reloaded from the unit once the tree is rebuilt, which would drop edits the item refused
    if (!saveOpenChanges()) {
        //: Screen reader announcement when an editor move shortcut is refused because the item's changes could not be saved; %1 is the item's name
        mudlet::self()->announce(tr("%1 was not moved, as its changes could not be saved").arg(pItem->text(0)), QString(), true);
        return;
    }
    // Read after saving, which may have renamed either row
    const QString itemName = pItem->text(0);
    outcome = outcome.arg(itemName, pLandmark->text(0));

    reParentInUnit(view, itemID, oldParentID, newParentID, newPosition);
    if (mpUndoStack) {
        mpUndoStack->pushCommand(new EditorMoveItemCommand(view, itemID, oldParentID, newParentID, oldPosition, newPosition, itemName, mpHost));
    }
    slot_itemsChanged(view, {itemID});
    mudlet::self()->announce(outcome, QString(), true);
}

QList<int> dlgTriggerEditor::unitChildIDs(const EditorViewType view, const int parentID) const
{
    QList<int> ids;
    switch (view) {
    case EditorViewType::cmTriggerView:
        if (!parentID) {
            for (const TTrigger* pNode : mpHost->getTriggerUnit()->getTriggerRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TTrigger* pParent = mpHost->getTriggerUnit()->getTrigger(parentID)) {
            for (const Tree<TTrigger>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (!parentID) {
            for (const TAlias* pNode : mpHost->getAliasUnit()->getAliasRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TAlias* pParent = mpHost->getAliasUnit()->getAlias(parentID)) {
            for (const Tree<TAlias>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (!parentID) {
            for (const TTimer* pNode : mpHost->getTimerUnit()->getTimerRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TTimer* pParent = mpHost->getTimerUnit()->getTimer(parentID)) {
            for (const Tree<TTimer>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (!parentID) {
            for (const TScript* pNode : mpHost->getScriptUnit()->getScriptRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TScript* pParent = mpHost->getScriptUnit()->getScript(parentID)) {
            for (const Tree<TScript>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (!parentID) {
            for (const TKey* pNode : mpHost->getKeyUnit()->getKeyRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TKey* pParent = mpHost->getKeyUnit()->getKey(parentID)) {
            for (const Tree<TKey>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (!parentID) {
            for (const TAction* pNode : mpHost->getActionUnit()->getActionRootNodeList()) {
                ids.append(pNode->getID());
            }
        } else if (const TAction* pParent = mpHost->getActionUnit()->getAction(parentID)) {
            for (const Tree<TAction>* pNode : *pParent->getChildrenList()) {
                ids.append(pNode->getID());
            }
        }
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
    return ids;
}

void dlgTriggerEditor::reParentInUnit(const EditorViewType view, const int itemID, const int oldParentID, const int newParentID, const int position)
{
    switch (view) {
    case EditorViewType::cmTriggerView:
        mpHost->getTriggerUnit()->reParentTrigger(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        break;
    case EditorViewType::cmAliasView:
        mpHost->getAliasUnit()->reParentAlias(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        break;
    case EditorViewType::cmTimerView:
        mpHost->getTimerUnit()->reParentTimer(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        break;
    case EditorViewType::cmScriptView:
        mpHost->getScriptUnit()->reParentScript(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        break;
    case EditorViewType::cmKeysView:
        mpHost->getKeyUnit()->reParentKey(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        break;
    case EditorViewType::cmActionView:
        mpHost->getActionUnit()->reParentAction(itemID, oldParentID, newParentID, TreeItemInsertMode::AtPosition, position);
        mpHost->getActionUnit()->updateAllToolbars();
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
}

void dlgTriggerEditor::children_icon_triggers(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_triggers(pItem);
        }
        if (pT->state()) {
            if (pT->isFilterChain()) {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFilterChain;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFilterChain;
                }
            } else if (pT->isFolder()) {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}


void dlgTriggerEditor::activeToggle_timer()
{
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();

    if (!pT->isOffsetTimer()) {
        pT->setIsActive(!pT->shouldBeActive());
    } else {
        pT->setShouldBeActive(!pT->shouldBeActive());
    }

    // Capture new state after toggle
    bool newState = pT->shouldBeActive();

    if (pT->isFolder()) {
        // disable or enable all timers in the respective branch
        // irrespective of the user defined state.
        if (pT->shouldBeActive()) {
            pT->enableTimer(pT->getID());
        } else {
            pT->disableTimer(pT->getID());
        }

        if (pT->shouldBeActive()) {
            itemDescription = descActiveFolder;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->isOffsetTimer()) {
            // state of offset timers is managed by the trigger engine
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                // Its parent firing arms it, so it must not count down from being switched on
                pT->stop();
                itemDescription = descActiveOffsetTimer;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                pT->disableTimer(pT->getID());
                itemDescription = descInactiveOffsetTimer;
            }
        } else {
            if (pT->shouldBeActive()) {
                pT->enableTimer(pT->getID());
                itemDescription = descActive;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                pT->disableTimer(pT->getID());
                itemDescription = descInactive;
            }
        }
    }

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

    if (pItem->childCount() > 0) {
        children_icon_timer(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmTimerView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::children_icon_timer(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TTimer* pT = mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        const bool itemActive = (pT->isActive() || pT->shouldBeActive());

        if (pItem->childCount() > 0) {
            children_icon_timer(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (itemActive && !pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                if (pT->isOffsetTimer()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveOffsetTimer;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveOffsetTimer;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    }
                } else {
                    if (itemActive) {
                        itemDescription = descActive;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                    }
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::activeToggle_alias()
{
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();
    pT->setIsActive(!pT->shouldBeActive());
    // Capture new state after toggle
    bool newState = pT->isActive();

    if (pT->isFolder()) {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActiveFolder;
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActive;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

    if (pItem->childCount() > 0) {
        children_icon_alias(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmAliasView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::children_icon_alias(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TAlias* pT = mpHost->getAliasUnit()->getAlias(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_alias(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}


void dlgTriggerEditor::activeToggle_script()
{
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();
    pT->setIsActive(!pT->shouldBeActive());
    // Capture new state after toggle
    bool newState = pT->isActive();

    if (pT->isFolder()) {
        if (pT->isActive()) {
            itemDescription = descActiveFolder;
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->isActive()) {
            itemDescription = descActive;
            if (!pT->ancestorsActive()) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    if (pItem->childCount() > 0) {
        children_icon_script(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmScriptView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::children_icon_script(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TScript* pT = mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_script(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->isActive()) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (!pT->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_action()
{
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();
    pT->setIsActive(!pT->shouldBeActive());
    pT->setDataChanged();
    // Capture new state after toggle
    bool newState = pT->isActive();

    if (mpHost->mainConsoleView()) {
        mpHost->mainConsoleView()->setActionToolBarVisible(pT, pT->isActive());
    }

    const bool itemActive = pT->isActive();
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (itemActive && !pT->ancestorsActive()) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        if (itemActive) {
            itemDescription = descActive;
            if (!pT->ancestorsActive()) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }
    pItem->setText(0, pT->getName());
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

    mpHost->getActionUnit()->updateAllToolbars();
    if (pItem->childCount() > 0) {
        children_icon_action(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmActionView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::children_icon_action(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TAction* pT = mpHost->getActionUnit()->getAction(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        const bool itemActive = pT->isActive();
        if (pItem->childCount() > 0) {
            children_icon_action(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                // A folder with a package name is a module's or package's master folder, with no parent to be held inactive by
                if (pT->mPackageName.isEmpty() && itemActive && !pT->ancestorsActive()) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                if (pT->isActive()) {
                    itemDescription = descActive;
                    if (!pT->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}


void dlgTriggerEditor::activeToggle_key()
{
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (!pItem) {
        return;
    }
    QString itemDescription;

    TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
    if (!pT) {
        return;
    }

    // Capture old state for undo
    bool oldState = pT->shouldBeActive();
    pT->setIsActive(!pT->shouldBeActive());
    // Capture new state after toggle
    bool newState = pT->isActive();

    if (pT->isFolder()) {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActiveFolder;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActive;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        pItem->setText(0, pT->getName());
    } else {
        itemDescription = descError;
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

    if (!pT->state()) {
        pT->setIsActive(false);
        showActivationFailure(pT->getName(), pT->getError());
        itemDescription = descError;
    }

    if (pItem->childCount() > 0) {
        children_icon_key(pItem);
    }

    if (mpUndoStack && oldState != newState) {
        auto* qtCmd = new EditorToggleActiveCommand(EditorViewType::cmKeysView, pT->getID(), oldState, newState, pT->getName(), mpHost);
        mpUndoStack->pushCommand(qtCmd);
    }
}

void dlgTriggerEditor::children_icon_key(QTreeWidgetItem* pWidgetItemParent)
{
    for (int i = 0; i < pWidgetItemParent->childCount(); i++) {
        QTreeWidgetItem* pItem = pWidgetItemParent->child(i);
        TKey* pT = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        if (!pT) {
            return;
        }

        QString itemDescription;
        if (pItem->childCount() > 0) {
            children_icon_key(pItem);
        }
        if (pT->state()) {
            if (pT->isFolder()) {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }

                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}


void dlgTriggerEditor::addTrigger(bool isFolder)
{
    saveTrigger();

    QString name = isFolder ? tr("New trigger group") : tr("New trigger");
    QStringList nameList{name};
    const QStringList patterns;
    QList<int> const patternKinds;
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_triggers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTrigger* pNewTrigger = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TTrigger* pParentTrigger = mpHost->getTriggerUnit()->getTrigger(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewTrigger = new TTrigger(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewTrigger = new TTrigger(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewTrigger) {
        // Fallback to insert a new root item
        pNewTrigger = new TTrigger(name, patterns, patternKinds, false, mpHost);
        pNewItem = new QTreeWidgetItem(mpTriggerBaseItem, nameList);
        treeWidget_triggers->insertTopLevelItem(0, pNewItem);
    }


    if (!pNewTrigger) {
        return;
    }

    // Initialize logic object properties
    pNewTrigger->setName(name);
    pNewTrigger->setRegexCodeList(patterns, patternKinds, false);
    pNewTrigger->setScript(script);
    pNewTrigger->setIsFolder(isFolder);
    pNewTrigger->setIsActive(false);
    pNewTrigger->setShouldBeActive(true);
    pNewTrigger->setIsMultiline(false);
    pNewTrigger->mStayOpen = 0;
    pNewTrigger->setConditionLineDelta(0);
    pNewTrigger->registerTrigger();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewTrigger->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    // Block property-save slots so the widget changes below don't fire write-backs
    // into the previously selected trigger (mpCurrentTriggerItem still points at
    // it). slot_triggerSelected() clears the flag once the new item is loaded.
    mBlockPropertySave = true;
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
    mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(false);
    clearDocument(mpSourceEditorEdbee); // New Trigger
    mpTriggersMainArea->lineEdit_trigger_command->clear();
    mpTriggersMainArea->checkBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->spinBox_stayOpen->setValue(0);
    mpTriggersMainArea->spinBox_lineMargin->setValue(-1);
    mpTriggersMainArea->pushButtonFgColor->setChecked(false);
    mpTriggersMainArea->pushButtonBgColor->setChecked(false);
    mpTriggersMainArea->checkBox_triggerColorizer->setChecked(false);

    // Finalize selection
    mpCurrentTriggerItem = pNewItem;
    treeWidget_triggers->setCurrentItem(pNewItem);
    slot_triggerSelected(treeWidget_triggers->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpTriggerBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_triggers->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmTriggerView, pNewTrigger->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}


void dlgTriggerEditor::addTimer(bool isFolder)
{
    saveTimer();

    QString name = isFolder ? tr("New timer group") : tr("New timer");
    QStringList nameList = {name};
    const QString command = "";
    const QTime time;
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_timers->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TTimer* pNewTimer = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TTimer* pParentTrigger = mpHost->getTimerUnit()->getTimer(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewTimer = new TTimer(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewTimer = new TTimer(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewTimer) {
        // Fallback to insert a new root item
        pNewTimer = new TTimer(name, time, mpHost);
        pNewItem = new QTreeWidgetItem(mpTimerBaseItem, nameList);
        treeWidget_timers->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewTimer) {
        return;
    }

    // Initialize logic object properties
    pNewTimer->setName(name);
    pNewTimer->setCommand(command);
    pNewTimer->setScript(script);
    pNewTimer->setIsFolder(isFolder);
    pNewTimer->setIsActive(false);
    mpHost->getTimerUnit()->registerTimer(pNewTimer);

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewTimer->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    //FIXME
    //mpOptionsAreaTriggers->lineEdit_trigger_name->clear();
    mpTimersMainArea->lineEdit_timer_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Timer

    // Finalize selection
    mpCurrentTimerItem = pNewItem;
    treeWidget_timers->setCurrentItem(pNewItem);
    slot_timerSelected(treeWidget_timers->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpTimerBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_timers->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmTimerView, pNewTimer->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}

void dlgTriggerEditor::addVar(bool isFolder)
{
    saveVar();
    mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(0);
    // the variable left behind may have been one whose key type is not the
    // user's to choose, which leaves the combobox disabled
    mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
    if (isFolder) {
        // in lieu of readonly
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(true);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        clearDocument(mpSourceEditorEdbee, QLatin1String("NewTable"));
    } else {
        // in lieu of readonly
        mpSourceEditorEdbee->setEnabled(true);
        mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
    }

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    QStringList nameList = {QString(isFolder ? tr("table_variable") : tr("variable_name"))};
    mpVarsMainArea->lineEdit_var_name->setText(nameList[0]);
    QTreeWidgetItem* pParentItem = nullptr;
    QTreeWidgetItem* pNewItem;
    QTreeWidgetItem* cItem = treeWidget_variables->currentItem();
    if (cItem) {
        TVar* cVar = treeWidget_variables->variableForRow(vu, cItem);
        if (cVar && cVar->getValueType() == LUA_TTABLE) {
            pParentItem = cItem;
        } else {
            pParentItem = cItem->parent();
        }
    }

    auto newVar = std::make_unique<TVar>();
    if (pParentItem) {
        //we're nested under something, or going to be.  This HAS to be a table
        TVar* parent = treeWidget_variables->variableForRow(vu, pParentItem);
        if (parent && parent->getValueType() == LUA_TTABLE) {
            //create it under the parent
            pNewItem = new QTreeWidgetItem(pParentItem, nameList);
            newVar->setParent(parent);
        } else {
            pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameList);
            newVar->setParent(vu->getBase());
        }
    } else {
        pNewItem = new QTreeWidgetItem(mpVarBaseItem, nameList);
        newVar->setParent(vu->getBase());
    }

    if (isFolder) {
        newVar->setValue(QString(), LUA_TTABLE);
    } else {
        newVar->setValueType(LUA_TNONE);
    }
    treeWidget_variables->setNewVariableForRow(vu, pNewItem, std::move(newVar));
    pNewItem->setFlags(pNewItem->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
    refreshVariableRow(pNewItem);

    // Finalize selection
    mpCurrentVarItem = pNewItem;
    treeWidget_variables->setCurrentItem(pNewItem);
    slot_variableSelected(treeWidget_variables->currentItem());
    saveVar();
}

void dlgTriggerEditor::addKey(bool isFolder)
{
    saveKey();

    QString name = isFolder ? tr("New key group") : tr("New key");
    QStringList nameList = {name};
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_keys->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TKey* pNewKey = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TKey* pParentTrigger = mpHost->getKeyUnit()->getKey(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewKey = new TKey(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewKey = new TKey(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewKey) {
        // Fallback to insert a new root item
        pNewKey = new TKey(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpKeyBaseItem, nameList);
        treeWidget_keys->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewKey) {
        return;
    }

    // Initialize logic object properties
    pNewKey->setName(name);
    pNewKey->setKeyCode(Qt::Key_unknown);
    pNewKey->setKeyModifiers(Qt::NoModifier);
    pNewKey->setScript(script);
    pNewKey->setIsFolder(isFolder);
    pNewKey->setIsActive(false);
    pNewKey->setShouldBeActive(true);
    pNewKey->registerKey();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewKey->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    // The binding stands empty behind its placeholder; words in it would read as a key
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->clear();
    clearDocument(mpSourceEditorEdbee); // New Key

    // Finalize selection
    mpCurrentKeyItem = pNewItem;
    treeWidget_keys->setCurrentItem(pNewItem);
    slot_keySelected(treeWidget_keys->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpKeyBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_keys->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmKeysView, pNewKey->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}


void dlgTriggerEditor::addAlias(bool isFolder)
{
    saveAlias();

    QString name = isFolder ? tr("New alias group") : tr("New alias");
    QStringList nameList = {name};
    const QString regex = "";
    const QString command = "";
    const QString script = "";

    QTreeWidgetItem* pParentItem = treeWidget_aliases->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TAlias* pNewAlias = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TAlias* pParentTrigger = mpHost->getAliasUnit()->getAlias(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewAlias = new TAlias(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewAlias = new TAlias(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewAlias) {
        //insert a new root item
        pNewAlias = new TAlias(name, mpHost);
        pNewAlias->setRegexCode(regex); // Empty regex will always succeed to compile
        pNewItem = new QTreeWidgetItem(mpAliasBaseItem, nameList);
        treeWidget_aliases->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewAlias) {
        return;
    }

    // Initialize logic object properties
    pNewAlias->setName(name);
    pNewAlias->setCommand(command);
    pNewAlias->setRegexCode(regex); // Empty regex will always succeed to compile
    pNewAlias->setScript(script);
    pNewAlias->setIsFolder(isFolder);
    pNewAlias->setIsActive(false);
    pNewAlias->setShouldBeActive(true);
    pNewAlias->registerAlias();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewAlias->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->label_idNumber->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // New Alias
    mpAliasMainArea->lineEdit_alias_name->setText(name);
    mpAliasMainArea->label_idNumber->setText(QString::number(pNewAlias->getID()));

    // Finalize selection
    mpCurrentAliasItem = pNewItem;
    treeWidget_aliases->setCurrentItem(pNewItem);
    slot_aliasSelected(treeWidget_aliases->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpAliasBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_aliases->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmAliasView, pNewAlias->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}

void dlgTriggerEditor::addAction(bool isFolder)
{
    saveAction();

    QString name = isFolder ? tr("New menu") : tr("New button");
    QStringList nameList = {name};

    QTreeWidgetItem* pParentItem = treeWidget_actions->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    QPointer<TAction> pNewAction = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TAction* pParentAction = mpHost->getActionUnit()->getAction(parentID);

        if (pParentAction) {
            // insert new items as siblings unless the parent is a folder
            if (pParentAction->isFolder()) {
                pNewAction = new TAction(pParentAction, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentAction->getParent() && pParentItem->parent()) {
                pNewAction = new TAction(pParentAction->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }
    // Otherwise: insert a new root item
    // CHECKME: doesn't this HAVE to be a toolbar - surely buttons MUST be in a container?
    if (!pNewAction) {
        name = isFolder ? tr("New toolbar") : tr("New button");
        pNewAction = new TAction(name, mpHost);
        pNewAction->setCommandButtonUp(QString());
        QStringList nl;
        nl << name;
        pNewItem = new QTreeWidgetItem(mpActionBaseItem, nl);
        treeWidget_actions->insertTopLevelItem(0, pNewItem);
    }

    // Initialize logic object properties
    pNewAction->setName(name);
    pNewAction->setCommandButtonUp(QString());
    pNewAction->setCommandButtonDown(QString());
    pNewAction->setIsPushDownButton(false);
    pNewAction->mLocation = 1;
    pNewAction->mOrientation = 1;
    pNewAction->setScript(QString());
    pNewAction->setIsFolder(isFolder);
    pNewAction->setIsActive(false);
    pNewAction->registerAction();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewAction->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    // Block property-save slots so the widget changes below don't fire write-backs
    // into the previously selected action (mpCurrentActionItem still points at it).
    // Unchecking checkBox_action_button_isPushDown otherwise calls
    // slot_saveProperty_ActionIsPushDown on the old action. slot_actionSelected()
    // clears the flag once the new item is loaded.
    mBlockPropertySave = true;
    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(false);
    clearDocument(mpSourceEditorEdbee); // New Action

    // This prevents reloading a Floating toolbar when an empty action is added.
    // After the action is saved it may trigger the rebuild.
    pNewAction->setDataSaved();

    mpHost->getActionUnit()->updateAllToolbars();

    // Finalize selection
    mpCurrentActionItem = pNewItem;
    treeWidget_actions->setCurrentItem(pNewItem);
    slot_actionSelected(treeWidget_actions->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpActionBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_actions->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmActionView, pNewAction->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}


void dlgTriggerEditor::addScript(bool isFolder)
{
    saveScript();

    QString name = isFolder ? tr("New script group") : tr("New script");
    QStringList nameList = {name};
    const QString script;

    QTreeWidgetItem* pParentItem = treeWidget_scripts->currentItem();
    QTreeWidgetItem* pNewItem = nullptr;
    TScript* pNewScript = nullptr;

    if (pParentItem) {
        const int parentID = pParentItem->data(0, Qt::UserRole).toInt();
        TScript* pParentTrigger = mpHost->getScriptUnit()->getScript(parentID);

        if (pParentTrigger) {
            // insert new items as siblings unless the parent is a folder
            if (pParentTrigger->isFolder()) {
                pNewScript = new TScript(pParentTrigger, mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem, nameList);
                pParentItem->insertChild(0, pNewItem);
            } else if (pParentTrigger->getParent() && pParentItem->parent()) {
                pNewScript = new TScript(pParentTrigger->getParent(), mpHost);
                pNewItem = new QTreeWidgetItem(pParentItem->parent(), nameList);
                pParentItem->parent()->insertChild(0, pNewItem);
            }
        }
    }

    if (!pNewScript) {
        // Fallback to insert a new root item
        pNewScript = new TScript(name, mpHost);
        pNewItem = new QTreeWidgetItem(mpScriptsBaseItem, nameList);
        treeWidget_scripts->insertTopLevelItem(0, pNewItem);
    }

    if (!pNewScript) {
        return;
    }

    // Initialize logic object properties
    pNewScript->setName(name);
    pNewScript->setScript(script);
    pNewScript->setIsFolder(isFolder);
    pNewScript->setIsActive(false);
    pNewScript->setShouldBeActive(true);
    pNewScript->registerScript();

    // Initialize tree item properties
    pNewItem->setData(0, Qt::UserRole, pNewScript->getID());
    pNewItem->setData(0, Qt::AccessibleDescriptionRole, isFolder ? descNewFolder : descNewItem);

    // Expand parent if applicable
    if (pParentItem) {
        pParentItem->setExpanded(true);
    }

    // Reset UI
    mpScriptsMainArea->lineEdit_script_name->clear();
    mpScriptsMainArea->label_idNumber->clear();
    mpChipRow_scriptEvents->setItems({});
    clearDocument(mpSourceEditorEdbee, script);

    // Finalize selection
    mpCurrentScriptItem = pNewItem;
    treeWidget_scripts->setCurrentItem(pNewItem);
    slot_scriptsSelected(treeWidget_scripts->currentItem());

    QTreeWidgetItem* actualParent = pNewItem->parent();
    int parentID = (actualParent && actualParent != mpScriptsBaseItem) ? actualParent->data(0, Qt::UserRole).toInt() : -1;

    int positionInParent = 0;
    if (actualParent) {
        positionInParent = actualParent->indexOfChild(pNewItem);
    } else {
        positionInParent = treeWidget_scripts->indexOfTopLevelItem(pNewItem);
    }

    auto* qtCmd = new EditorAddItemCommand(EditorViewType::cmScriptView, pNewScript->getID(), parentID, positionInParent, isFolder, name, mpHost);
    mpUndoStack->pushCommand(qtCmd);

    // Later edits can merge into this command, see EditorAddItemCommand::mergeWith()
}

void dlgTriggerEditor::selectTriggerByID(int id)
{
    slot_showTriggers();
    QTreeWidgetItemIterator it(treeWidget_triggers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_triggerSelected((*it));
            treeWidget_triggers->clearSelection();
            treeWidget_triggers->setCurrentItem((*it), 0);
            treeWidget_triggers->scrollToItem((*it));
            mpCurrentTriggerItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectTimerByID(int id)
{
    slot_showTimers();
    QTreeWidgetItemIterator it(treeWidget_timers);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_timerSelected((*it));
            treeWidget_timers->clearSelection();
            treeWidget_timers->setCurrentItem((*it), 0);
            treeWidget_timers->scrollToItem((*it));
            mpCurrentTimerItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectAliasByID(int id)
{
    slot_showAliases();
    QTreeWidgetItemIterator it(treeWidget_aliases);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_aliasSelected((*it));
            treeWidget_aliases->clearSelection();
            treeWidget_aliases->setCurrentItem((*it), 0);
            treeWidget_aliases->scrollToItem((*it));
            mpCurrentAliasItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectScriptByID(int id)
{
    slot_showScripts();
    QTreeWidgetItemIterator it(treeWidget_scripts);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_scriptsSelected((*it));
            treeWidget_scripts->clearSelection();
            treeWidget_scripts->setCurrentItem((*it), 0);
            treeWidget_scripts->scrollToItem((*it));
            mpCurrentScriptItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectActionByID(int id)
{
    slot_showActions();
    QTreeWidgetItemIterator it(treeWidget_actions);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_actionSelected((*it));
            treeWidget_actions->clearSelection();
            treeWidget_actions->setCurrentItem((*it), 0);
            treeWidget_actions->scrollToItem((*it));
            mpCurrentActionItem = (*it);
            return;
        }
        ++it;
    }
}

void dlgTriggerEditor::selectKeyByID(int id)
{
    slot_showKeys();
    QTreeWidgetItemIterator it(treeWidget_keys);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == id) {
            slot_keySelected((*it));
            treeWidget_keys->clearSelection();
            treeWidget_keys->setCurrentItem((*it), 0);
            treeWidget_keys->scrollToItem((*it));
            mpCurrentKeyItem = (*it);
            return;
        }
        ++it;
    }
}

TTrigger* dlgTriggerEditor::getTriggerFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int triggerID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getTriggerUnit()->getTrigger(triggerID);
}

TAlias* dlgTriggerEditor::getAliasFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int aliasID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getAliasUnit()->getAlias(aliasID);
}

TScript* dlgTriggerEditor::getScriptFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int scriptID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getScriptUnit()->getScript(scriptID);
}

TTimer* dlgTriggerEditor::getTimerFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int timerID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getTimerUnit()->getTimer(timerID);
}

TKey* dlgTriggerEditor::getKeyFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int keyID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getKeyUnit()->getKey(keyID);
}

TAction* dlgTriggerEditor::getActionFromTreeItem(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) {
        return nullptr;
    }

    const int actionID = item->data(0, Qt::UserRole).toInt();
    return mpHost->getActionUnit()->getAction(actionID);
}

void dlgTriggerEditor::slot_itemEdited()
{
    QString packageName;
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView: {
        if (auto trigger = getTriggerFromTreeItem(mpCurrentTriggerItem)) {
            packageName = trigger->packageName(trigger);
        }
        break;
    }
    case EditorViewType::cmAliasView: {
        if (auto alias = getAliasFromTreeItem(mpCurrentAliasItem)) {
            packageName = alias->packageName(alias);
        }
        break;
    }
    case EditorViewType::cmTimerView: {
        if (auto timer = getTimerFromTreeItem(mpCurrentTimerItem)) {
            packageName = timer->packageName(timer);
        }
        break;
    }
    case EditorViewType::cmScriptView: {
        if (auto script = getScriptFromTreeItem(mpCurrentScriptItem)) {
            packageName = script->packageName(script);
        }
        break;
    }
    case EditorViewType::cmActionView: {
        if (auto action = getActionFromTreeItem(mpCurrentActionItem)) {
            packageName = action->packageName(action);
        }
        break;
    }
    case EditorViewType::cmKeysView: {
        if (auto key = getKeyFromTreeItem(mpCurrentKeyItem)) {
            packageName = key->packageName(key);
        }
        break;
    }
    case EditorViewType::cmUnknownView:
        [[fallthrough]];
    case EditorViewType::cmVarsView:
        break;
    }

    showPackageWarning(packageName);
}

void dlgTriggerEditor::saveTrigger()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }

    // Additional safety check: ensure the item's parent is still valid
    // and that the item is still part of the tree widget
    if (!pItem->parent() || pItem->treeWidget() != treeWidget_triggers) {
        return;
    }

    mpTriggersMainArea->trimName();
    const QString name = mpTriggersMainArea->lineEdit_trigger_name->text();
    const QString command = mpTriggersMainArea->lineEdit_trigger_command->text();
    QStringList patterns;
    QList<int> patternKinds;
    int validItems = 0;
    for (const auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
        QString pattern = patternEdit->singleLineTextEdit_pattern->toPlainText();

        // Spaces in the pattern may be marked with middle dots, convert them back
        unmarkQString(&pattern);

        const int patternType = patternEdit->comboBox_patternType->currentIndex();
        if (pattern.isEmpty() && patternType != REGEX_PROMPT && patternType != REGEX_LINE_SPACER) {
            continue;
        }

        ++validItems;
        switch (patternType) {
        case 0:
            patternKinds << REGEX_SUBSTRING;
            break;
        case 1:
            patternKinds << REGEX_PERL;
            break;
        case 2:
            patternKinds << REGEX_BEGIN_OF_LINE_SUBSTRING;
            break;
        case 3:
            patternKinds << REGEX_EXACT_MATCH;
            break;
        case 4:
            patternKinds << REGEX_LUA_CODE;
            break;
        case 5:
            patternKinds << REGEX_LINE_SPACER;
            pattern = patternEdit->spinBox_lineSpacer->text();
            break;
        case 6:
            patternKinds << REGEX_COLOR_PATTERN;
            break;
        case 7:
            patternKinds << REGEX_PROMPT;
            break;
        }
        patterns << pattern;
    }
    const bool isMultiline = (mpTriggersMainArea->spinBox_lineMargin->value() > -1) && (validItems > 1);
    const QString script = mpSourceEditorEdbeeDocument->text();

    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (pT) {
        // Capture OLD state before modifications (for undo)
        QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

        pT->setName(name);
        pT->setCommand(command);
        pT->setRegexCodeList(patterns, patternKinds);

        pT->setScript(script);
        pT->setIsMultiline(isMultiline);
        pT->mPerlSlashGOption = mpTriggersMainArea->checkBox_perlSlashGOption->isChecked();
        pT->mFilterTrigger = mpTriggersMainArea->checkBox_filterTrigger->isChecked();
        if (mpTriggersMainArea->spinBox_lineMargin->value() >= 0) {
            pT->setConditionLineDelta(mpTriggersMainArea->spinBox_lineMargin->value());
        }
        pT->mStayOpen = mpTriggersMainArea->spinBox_stayOpen->value();
        pT->mSoundTrigger = mpTriggersMainArea->checkBox_soundTrigger->isChecked();
        pT->setSound(triggerSoundFilePath());

        QColor fgColor(QColorConstants::Transparent);
        QColor bgColor(QColorConstants::Transparent);
        if (!mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString().isEmpty()) {
            fgColor = QColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerFgColor(fgColor);
        if (!mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString().isEmpty()) {
            bgColor = QColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString());
        }
        pT->setColorizerBgColor(bgColor);
        pT->setIsColorizerTrigger(mpTriggersMainArea->checkBox_triggerColorizer->isChecked());
        QString itemDescription;
        if (pT->isFilterChain()) {
            if (pT->shownActive(mpHost->mEmergencyStop)) {
                itemDescription = descActiveFilterChain;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFilterChain;
            }
        } else if (pT->isFolder()) {
            if (pT->shownActive(mpHost->mEmergencyStop)) {
                itemDescription = descActiveFolder;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveFolder;
            }
        } else {
            if (pT->shownActive(mpHost->mEmergencyStop)) {
                itemDescription = descActive;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
            }
        }
        if (pT->state()) {
            clearEditorNotification();

            if (pT->checkIfNew()) {
                if (pT->isFolder()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                    }
                } else {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActive;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                    }
                }
                pItem->setText(0, name);

                // The emergency stop's resume switches it on; doing so here would let it run during the stop
                if (pT->shouldBeActive() && !mpHost->mEmergencyStop) {
                    pT->setIsActive(true);
                }
                pT->unmarkAsNew();
            } else {
                pItem->setText(0, name);
            }
        } else {
            pItem->setText(0, name);
            itemDescription = descError;
            pT->setIsActive(false);
            reportCompileError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

        // Capture NEW state after modifications (for redo)
        QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

        // Only push undo command if something actually changed
        if (oldStateXML != newStateXML) {
            auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmTriggerView, triggerID, name, oldStateXML, newStateXML, mpHost);
            mpUndoStack->pushCommand(qtCmd);

            // Clear edbee undo stack after save to make Save a commit point
            if (mpTextUndoStack) {
                mpTextUndoStack->clear();
            }
        }
        mpUndoStack->finishAddingItem(EditorViewType::cmTriggerView, triggerID);
    }
}

void dlgTriggerEditor::saveTimer()
{
    QTreeWidgetItem* pItem = mpCurrentTimerItem;
    if (!pItem) {
        return;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_timers) {
        return;
    }

    mpTimersMainArea->trimName();
    const QString name = mpTimersMainArea->lineEdit_timer_name->text();
    const QString script = mpSourceEditorEdbeeDocument->text();


    const int timerID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (pT) {
        // Capture OLD state before modifications (for undo)
        QString oldStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);

        pT->setName(name);
        const QString command = mpTimersMainArea->lineEdit_timer_command->text();
        const int hours = mpTimersMainArea->timeEdit_timer_hours->time().hour();
        const int minutes = mpTimersMainArea->timeEdit_timer_minutes->time().minute();
        const int secs = mpTimersMainArea->timeEdit_timer_seconds->time().second();
        const int msecs = mpTimersMainArea->timeEdit_timer_msecs->time().msec();
        const QTime time(hours, minutes, secs, msecs);
        // setTime() stops the QTimer, so even an unchanged time would end the countdown
        if (pT->getTime() != time) {
            pT->setTime(time);
        }
        pT->setCommand(command);
        pT->setName(name);
        pT->setScript(script);

        pT->validateTime();
        QString itemDescription;
        if (pT->isFolder()) {
            if (!pT->mPackageName.isEmpty()) {
                if (pT->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (pT->shouldBeActive()) {
                    itemDescription = descActiveFolder;
                    if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            }
        } else if (pT->isOffsetTimer()) {
            if (pT->shouldBeActive()) {
                itemDescription = descActiveOffsetTimer;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactiveOffsetTimer;
            }
        } else {
            if (pT->shouldBeActive()) {
                itemDescription = descActive;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
                // The emergency stop's resume starts it; doing so here would let it run during the stop
                if (!mpHost->mEmergencyStop) {
                    pT->stayActive();
                }
            } else {
                itemDescription = descInactive;
            }
        }

        if (pT->state()) {
            clearEditorNotification();

            // don't activate new timers by default - might be annoying
            pItem->setText(0, name);

        } else {
            itemDescription = descError;
            pItem->setText(0, name);
            reportCompileError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

        // Capture NEW state after modifications (for redo)
        QString newStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);

        // Only push undo command if something actually changed
        if (oldStateXML != newStateXML) {
            auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmTimerView, timerID, name, oldStateXML, newStateXML, mpHost);
            mpUndoStack->pushCommand(qtCmd);

            // Clear edbee undo stack after save to make Save a commit point
            if (mpTextUndoStack) {
                mpTextUndoStack->clear();
            }
        }
        mpUndoStack->finishAddingItem(EditorViewType::cmTimerView, timerID);
    }
}

bool dlgTriggerEditor::saveAlias()
{
    QTreeWidgetItem* pItem = mpCurrentAliasItem;
    if (!pItem) {
        return true;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_aliases) {
        return true;
    }

    mpAliasMainArea->trimName();
    QString name = mpAliasMainArea->lineEdit_alias_name->text();
    QString regex = mpAliasMainArea->lineEdit_alias_pattern->text();
    unmarkQString(&regex);

    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(triggerID);
    name = aliasNameForPattern(pT, name, regex);
    const QString substitution = mpAliasMainArea->lineEdit_alias_command->text();
    //check if sub will trigger regex, ignore if there's nothing in regex - could be an alias group
    if (aliasSubstitutionLoops(regex, substitution)) {
        //we have a loop
        showAliasLoopWarning(pItem, name);
        return false;
    }

    const QString script = mpSourceEditorEdbeeDocument->text();

    if (pT) {
        // Capture OLD state before modifications (for undo)
        QString oldStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);

        pT->setName(name);
        if (mpAliasMainArea->lineEdit_alias_name->text() != name) {
            mpAliasMainArea->lineEdit_alias_name->setText(name);
        }
        pT->setCommand(substitution);
        pT->setRegexCode(regex); // This could generate an error state if regex does not compile
        pT->setScript(script);

        QString itemDescription;
        computeAliasDescription(pT, itemDescription);

        if (pT->state()) {
            clearEditorNotification();

            if (pT->checkIfNew()) {
                if (pT->isFolder()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                    }
                } else {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActive;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                    }
                }
                pItem->setText(0, name);

                // The emergency stop's resume switches it on; doing so here would let it run during the stop
                if (pT->shouldBeActive() && !mpHost->mEmergencyStop) {
                    pT->setIsActive(true);
                }
                pT->unmarkAsNew();
            } else {
                pItem->setText(0, name);
            }
        } else {
            itemDescription = descError;
            pItem->setText(0, name);
            reportCompileError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

        // Capture NEW state after modifications (for redo)
        QString newStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);

        // Only push undo command if something actually changed
        if (oldStateXML != newStateXML) {
            auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmAliasView, triggerID, name, oldStateXML, newStateXML, mpHost);
            mpUndoStack->pushCommand(qtCmd);

            // Clear edbee undo stack after save to make Save a commit point
            if (mpTextUndoStack) {
                mpTextUndoStack->clear();
            }
        }
        mpUndoStack->finishAddingItem(EditorViewType::cmAliasView, triggerID);
    }
    return true;
}

// An unnamed alias, or one still being added whose name is its old pattern, is
// named after its pattern; otherwise it keeps the typed name
QString dlgTriggerEditor::aliasNameForPattern(const TAlias* pT, const QString& typedName, const QString& pattern) const
{
    const QString trimmedPattern = pattern.trimmed();
    const bool nameIsOldPattern = pT && typedName == pT->getRegexCode().trimmed() && mpUndoStack->addingItem(EditorViewType::cmAliasView, pT->getID());
    if (nameIsOldPattern && trimmedPattern.isEmpty()) {
        return pT->isFolder() ? tr("New alias group") : tr("New alias");
    }
    if (!trimmedPattern.isEmpty() && (typedName.isEmpty() || typedName == tr("New alias") || nameIsOldPattern)) {
        return trimmedPattern;
    }
    return typedName;
}

// Returns true when the substitution would match its own pattern, which makes
// the alias call itself forever. An invalid pattern can't loop - the faulty-regex
// check surfaces that instead.
bool dlgTriggerEditor::aliasSubstitutionLoops(const QString& regex, const QString& substitution) const
{
    if (regex.isEmpty()) {
        return false;
    }
    const QRegularExpression rx(regex);
    if (!rx.isValid()) {
        return false;
    }
    return rx.match(substitution).capturedStart() != -1;
}

// The delegate paints a row's state off its unit, so only the description is set here.
// True when it changed, which is also when the row repaints: setData() with an equal value emits nothing.
static bool setRowDescription(QTreeWidgetItem* pItem, const QString& description)
{
    if (pItem->data(0, Qt::AccessibleDescriptionRole).toString() == description) {
        return false;
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, description);
    return true;
}

void dlgTriggerEditor::computeAliasDescription(TAlias* pT, QString& itemDescription) const
{
    if (pT->isFolder()) {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActiveFolder;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFolder;
        }
    } else {
        if (pT->shownActive(mpHost->mEmergencyStop)) {
            itemDescription = descActive;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }
}

// Restores an alias tree item to its non-error appearance and clears the editor notice.
void dlgTriggerEditor::setAliasNormalState(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification, bool respectNewState)
{
    if (touchNotification) {
        clearEditorNotification();
    }
    if (respectNewState && pT->checkIfNew()) {
        // A freshly added alias keeps its "unsaved" cue until an explicit Save
        // activates it - don't recompute it to an active/inactive state here.
        pItem->setData(0, Qt::AccessibleDescriptionRole, pT->isFolder() ? descNewFolder : descNewItem);
        return;
    }
    QString itemDescription;
    computeAliasDescription(pT, itemDescription);
    if (setRowDescription(pItem, itemDescription)) {
        ++mAliasIconPaintCount;
    }
}

// Flags an alias tree item as broken and shows the given error message.
void dlgTriggerEditor::showAliasError(QTreeWidgetItem* pItem, const QString& name, const QString& error, bool touchNotification)
{
    pItem->setText(0, name);
    if (setRowDescription(pItem, descError)) {
        ++mAliasIconPaintCount;
    }
    if (touchNotification && pItem == mpCurrentAliasItem) {
        reportCompileError(error);
    }
}

void dlgTriggerEditor::showAliasLoopWarning(QTreeWidgetItem* pItem, const QString& name)
{
    showAliasError(pItem,
                   name,
                   tr("Alias <em>%1</em> has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn't good as it'll call itself forever.").arg(name.toHtmlEscaped()));
}

void dlgTriggerEditor::applyAliasState(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification, bool respectNewState)
{
    if (pT->state()) {
        setAliasNormalState(pItem, pT, touchNotification, respectNewState);
    } else {
        showAliasError(pItem, pT->getName(), pT->getError(), touchNotification);
    }
}

// Repaints aliasID's tree item (and, if any, its descendants) from its
// current active state - used when the state changed via the Lua
// enableAlias()/disableAlias() API rather than the GUI toggle. Only touches
// the shared editor-wide notification banner when aliasID is the item
// currently open in the detail pane *and* the Aliases view is the one on
// screen - mpCurrentAliasItem persists across tab switches, so without the
// view check a background state change could clobber whatever diagnostic the
// user is actually looking at in a different view.
void dlgTriggerEditor::refreshAliasIcon(int aliasID)
{
    // See refreshTriggerIcon().
    mPendingAliasIconRefresh.insert(aliasID);
    if (isVisible() && !mAliasIconRefreshQueued) {
        mAliasIconRefreshQueued = true;
        QTimer::singleShot(0ms, this, &dlgTriggerEditor::flushPendingAliasIconRefresh);
    }
}

void dlgTriggerEditor::flushPendingAliasIconRefresh()
{
    mAliasIconRefreshQueued = false;
    if (mPendingAliasIconRefresh.isEmpty()) {
        return;
    }
    if (!mpAliasBaseItem || !isVisible()) {
        // See flushPendingTriggerIconRefresh() - leave the IDs queued rather
        // than dropping the update.
        return;
    }
    ++mAliasIconFlushCount;
    int remaining = mPendingAliasIconRefresh.size();
    refreshAliasIconsIn(mpAliasBaseItem, false, false, remaining);
    mPendingAliasIconRefresh.clear();
}

// See refreshTriggerIconsIn() for the shape of this walk, including what
// ancestorTouchNotification carries forward.
void dlgTriggerEditor::refreshAliasIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining)
{
    for (int i = 0, n = pParent->childCount(); i < n; ++i) {
        QTreeWidgetItem* pItem = pParent->child(i);
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const bool pending = mPendingAliasIconRefresh.contains(id);
        if (pending) {
            --remaining;
        }
        const bool dirty = ancestorDirty || pending;
        const bool touchNotification = pending ? ((pItem == mpCurrentAliasItem) && (mCurrentView == EditorViewType::cmAliasView) && mFolderGroupKey.isEmpty()) : ancestorTouchNotification;
        if (dirty) {
            if (TAlias* pT = mpHost->getAliasUnit()->getAlias(id)) {
                paintAliasItem(pItem, pT, touchNotification);
            }
        }
        if (pItem->childCount() > 0 && (dirty || remaining > 0)) {
            refreshAliasIconsIn(pItem, dirty, dirty ? touchNotification : false, remaining);
        }
        if (!ancestorDirty && remaining <= 0) {
            return;
        }
    }
}

void dlgTriggerEditor::paintAliasItem(QTreeWidgetItem* pItem, TAlias* pT, bool touchNotification)
{
    // A profile's aliases stay TAlias::mIsNew until explicitly saved in the
    // editor, so respecting that here would mark every Lua-toggled alias
    // "unsaved" instead of reporting its actual state.
    // setAliasNormalState()/showAliasError() bump mAliasIconPaintCount themselves, only when the row changes
    applyAliasState(pItem, pT, touchNotification, false);
}

void dlgTriggerEditor::saveAction()
{
    QTreeWidgetItem* pItem = mpCurrentActionItem;
    if (!pItem) {
        return;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_actions) {
        return;
    }

    mpActionsMainArea->trimName();
    const QString name = mpActionsMainArea->lineEdit_action_name->text();
    const QString icon = mpActionsMainArea->lineEdit_action_icon->text();
    const QString commandDown = mpActionsMainArea->lineEdit_action_button_command_down->text();
    const QString commandUp = mpActionsMainArea->lineEdit_action_button_command_up->text();
    const QString script = mpSourceEditorEdbeeDocument->text();
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    const int rotation = qMax(0, mpActionsMainArea->comboBox_action_button_rotation->currentIndex());
    const int columns = mpActionsMainArea->spinBox_action_bar_columns->value();
    const int offset = mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->value();
    const bool isChecked = mpActionsMainArea->checkBox_action_button_isPushDown->isChecked();
    // bottom location is no longer supported i.e. location = 1 = 0 = location top
    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    int location = qMax(0, mpActionsMainArea->comboBox_action_bar_location->currentIndex());
    if (location > 0) {
        // The comboBox has indexes of 0 to 4 but we don't use 1 so jump over it:
        ++location;
    }

    // currentIndex() can return -1 if no setting was previously made - need to fixup:
    const int orientation = qMax(0, mpActionsMainArea->comboBox_action_bar_orientation->currentIndex());

    // This is an unnecessary level of indentation but has been retained to
    // reduce the noise in a git commit/diff caused by the removal of a
    // redundant "if( pITem )" - can be removed next time the file is modified
    const int actionID = pItem->data(0, Qt::UserRole).toInt();
    TAction* pA = mpHost->getActionUnit()->getAction(actionID);
    if (pA) {
        // Capture OLD state before modifications (for undo)
        QString oldStateXML = exportActionToXML(pA, SnapshotScope::ItemOnly);

        // Check if data has been changed before it gets updated.
        bool actionDataChanged = false;
        if (pA->mLocation != location || pA->mOrientation != orientation || pA->css != mpActionsMainArea->plainTextEdit_action_css->toPlainText()) {
            actionDataChanged = true;
        }

        // Do not change anything for a module master folder - it won't "take"
        if (pA->mPackageName.isEmpty()) {
            pA->setName(name);
            pA->setIcon(icon);
            pA->setScript(script);
            pA->setCommandButtonDown(commandDown);
            pA->setCommandButtonUp(commandUp);
            pA->setIsPushDownButton(isChecked);
            pA->mLocation = location;
            pA->mOrientation = orientation;
            pA->setIsActive(pA->shouldBeActive());
            pA->setButtonRotation(rotation);
            pA->setButtonColumns(columns);
            pA->setButtonFillerOffset(offset);
            pA->mUseCustomLayout = false;
            pA->css = mpActionsMainArea->plainTextEdit_action_css->toPlainText();
        }

        QString itemDescription;
        const bool itemActive = pA->isActive();
        if (pA->isFolder()) {
            itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
            // A folder carrying a package name is a module's master folder,
            // which has no parent to be held inactive by
            if (pA->mPackageName.isEmpty() && itemActive && !pA->ancestorsActive()) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            // Is a button
            if (itemActive) {
                itemDescription = descActive;
                if (!pA->ancestorsActive()) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
            }
        }

        if (pA->state()) {
            clearEditorNotification();

            pItem->setText(0, name);
        } else {
            itemDescription = descError;
            pItem->setText(0, name);
            reportCompileError(pA->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

        // If not active, don't bother raising the TToolBar for this save.
        if (!pA->shouldBeActive()) {
            pA->setDataSaved();
        }

        if (actionDataChanged) {
            pA->setDataChanged();
        }

        if (auto* pConsole = mpHost->mainConsoleView()) {
            // if the action has a TToolBar instance with a script error, hide that toolbar.
            if (!pA->state()) {
                pConsole->setActionToolBarVisible(pA, false);
            }

            // if the action location is changed, make sure the old toolbar instance is hidden.
            if (pA->mLocation == 4) {
                pConsole->hideActionEasyButtonBar(pA);
            } else {
                pConsole->setActionToolBarVisible(pA, false);
            }
        }

        // Capture NEW state after modifications (for redo)
        QString newStateXML = exportActionToXML(pA, SnapshotScope::ItemOnly);

        // Only push undo command if something actually changed
        if (oldStateXML != newStateXML) {
            auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmActionView, actionID, name, oldStateXML, newStateXML, mpHost);
            mpUndoStack->pushCommand(qtCmd);

            // Clear edbee undo stack after save to make Save a commit point
            if (mpTextUndoStack) {
                mpTextUndoStack->clear();
            }
        }
        mpUndoStack->finishAddingItem(EditorViewType::cmActionView, actionID);
    }

    mpHost->getActionUnit()->updateAllToolbars();
    mudlet::self()->processEventLoopHack();
}

void dlgTriggerEditor::writeScript(int id)
{
    QTreeWidgetItem* pItem = mpCurrentScriptItem;
    if (!pItem) {
        return;
    }
    if (mCurrentView == EditorViewType::cmUnknownView || mCurrentView != EditorViewType::cmScriptView) {
        return;
    }
    const int scriptID = pItem->data(0, Qt::UserRole).toInt();
    if (scriptID != id) {
        return;
    }

    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    const QString scriptCode = pT->getScript();

    disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setText(scriptCode);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    // The heading, the margin and the row describe the code shown, which this has just replaced
    paintScriptItem(pItem, pT, false);
    if (pT->state()) {
        clearCompileState();
    } else {
        reportCompileError(pT->getError());
    }
}

void dlgTriggerEditor::saveScript()
{
    QTreeWidgetItem* pItem = mpCurrentScriptItem;
    if (!pItem) {
        return;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_scripts) {
        return;
    }

    mpScriptsMainArea->trimName();
    const QString name = mpScriptsMainArea->lineEdit_script_name->text();
    const QString script = mpSourceEditorEdbeeDocument->text();
    const QStringList handlerList = mpChipRow_scriptEvents->items();

    const int scriptID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    // Capture OLD state before modifications (for undo)
    QString oldStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);

    pT->setName(name);
    pT->setEventHandlerList(handlerList);
    pT->setScript(script);

    pT->compileAll();
    mpHost->getTriggerUnit()->doCleanup();
    QString itemDescription;
    const bool itemActive = pT->isActive();
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (itemActive && !pT->ancestorsActive()) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        if (itemActive) {
            itemDescription = descActive;
            if (!pT->ancestorsActive()) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactive;
        }
    }

    if (pT->state()) {
        clearEditorNotification();

        if (pT->checkIfNew()) {
            if (pT->isFolder()) {
                itemDescription = descActiveFolder;
                if (!pT->ancestorsActive()) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                if (pT->shouldBeActive()) {
                    itemDescription = descActive;
                    if (!pT->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
            pItem->setText(0, name);

            if (pT->shouldBeActive()) {
                pT->setIsActive(true);
            }
            pT->unmarkAsNew();
        } else {
            pItem->setText(0, name);
        }

    } else {
        itemDescription = descError;
        pItem->setText(0, name);
        reportCompileError(pT->getError());
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);

    // Capture NEW state after modifications (for redo)
    QString newStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);

    // Only push undo command if something actually changed
    if (oldStateXML != newStateXML) {
        auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmScriptView, scriptID, name, oldStateXML, newStateXML, mpHost);
        mpUndoStack->pushCommand(qtCmd);

        // Clear edbee undo stack after save to make Save a commit point
        if (mpTextUndoStack) {
            mpTextUndoStack->clear();
        }
    }
    mpUndoStack->finishAddingItem(EditorViewType::cmScriptView, scriptID);

    // If pT's own body uninstalled its package during the compile above, the delete
    // was deferred (see TScript::compileScript / ScriptUnit::uninstall). We are now
    // done with pT, so flush it before returning to the event loop - otherwise the
    // 0ms save uninstallPackage() queued would serialize the "uninstalled" script
    // back into the profile:
    mpHost->getScriptUnit()->doCleanup();
}

void dlgTriggerEditor::clearEditorNotification()
{
    mpSystemMessageArea->hide();
    mCurrentBannerKey.clear();
}

void dlgTriggerEditor::updatePackageItemAccessibility(QTreeWidgetItem* pItem, const QString& currentDescription)
{
    // Append package description to existing accessible description
    // Screen readers will announce: "Item Name, [current description], package item"
    QString newDescription;
    if (currentDescription.isEmpty()) {
        newDescription = descPackageItem;
    } else {
        // Combine descriptions: e.g., "activated, package item"
        newDescription = currentDescription + qsl(", ") + descPackageItem;
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, newDescription);
}

// Qt matches Mudlet's own shortcuts and add-on menu shortcuts before the command line sees the
// key, so a binding on one of their keys never fires. It is still accepted, only warned about.
// Empty when the binding will fire. The strings keep the KeyUnit context they were translated in.
QString dlgTriggerEditor::takenKeyWarning(Host* pHost, const TKey* pKey)
{
    auto* pMudlet = mudlet::self();
    if (!pKey || !pHost || !pMudlet || pKey->isFolder() || pKey->getKeyCode() == Qt::Key_unknown) {
        return {};
    }
    // A keypad or group-switch binding cannot be written as a key sequence, so
    // no shortcut can be the one holding it
    constexpr Qt::KeyboardModifiers sequenceModifiers = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    if (pKey->getKeyModifiers() & ~sequenceModifiers) {
        return {};
    }

    const QKeySequence sequence(QKeyCombination(pKey->getKeyModifiers(), pKey->getKeyCode()));
    const QString keyText = sequence.toString(QKeySequence::NativeText);
    // Not either/or: addCommand() refuses a key Mudlet holds, but the preferences can move a
    // Mudlet shortcut onto a command's key
    QStringList warnings;
    if (const QString action = pMudlet->ownShortcutUsingKey(pKey->getKeyCode(), pKey->getKeyModifiers()); !action.isEmpty()) {
        // "while that is available": a greyed-out menu item doesn't get the key, so the binding fires then
        //: Warning shown in the editor when a key binding is given a key one of Mudlet's own shortcuts already uses. %1 is a key such as "Alt+M", %2 the name of the Mudlet action holding it, as the Shortcuts tab of the preferences shows it.
        warnings.append(QCoreApplication::translate("KeyUnit",
                                                    "%1 is already used by Mudlet for \"%2\", which will get the key first, so this key binding will not fire while that is available. "
                                                    "Mudlet's own shortcuts can be changed in the preferences, under Shortcuts.")
                                .arg(keyText, action));
    }
    if (const QStringList holders = pMudlet->addonCommandsUsingShortcut(sequence, pHost); !holders.isEmpty()) {
        //: Warning shown in the editor when a key binding is given a key an add-on command already holds. %1 is a key such as "Alt+F9", %2 a comma separated list of the commands holding it.
        warnings.append(QCoreApplication::translate("KeyUnit", "%1 is already used by %2, which will get the key first, so this key binding will not fire.").arg(keyText, holders.join(qsl(", "))));
    }
    return warnings.join(QChar::Space);
}

// Also kept in the item's accessible description, heard on landing on it, so announcing is optional
void dlgTriggerEditor::showKeyTakenWarning(QTreeWidgetItem* pItem, const QString& warning, const bool announce)
{
    if (!warning.isEmpty()) {
        showWarning(warning, announce);
    }
    if (!pItem) {
        return;
    }
    // Selecting an item runs this again, so the old mark comes off first
    const QString suffix = qsl(", ") + descKeyTaken;
    QString description = pItem->data(0, Qt::AccessibleDescriptionRole).toString();
    description.remove(suffix);
    if (description == descKeyTaken) {
        description.clear();
    }
    if (!warning.isEmpty()) {
        description = description.isEmpty() ? descKeyTaken : description + suffix;
    }
    pItem->setData(0, Qt::AccessibleDescriptionRole, description);
}

// Only after an explicit save: saveKey() also runs on leaving the keys view, where the warning
// would stay over the next view and keep its intro from showing
void dlgTriggerEditor::rewarnAboutCurrentKey()
{
    if (!mpCurrentKeyItem) {
        return;
    }
    const TKey* pT = mpHost->getKeyUnit()->getKey(mpCurrentKeyItem->data(0, Qt::UserRole).toInt());
    // A failed compile's error is what the banner should keep showing
    if (pT && pT->state()) {
        showKeyTakenWarning(mpCurrentKeyItem, takenKeyWarning(mpHost, pT), false);
    }
}

int dlgTriggerEditor::canRecast(QTreeWidgetItem* pItem, int newNameType, int newValueType)
{
    //basic checks, return 1 if we can recast, 2 if no need to recast, 0 if we can't recast
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = treeWidget_variables->variableForRow(vu, pItem);
    if (!var) {
        return 2;
    }
    const int currentNameType = var->getKeyType();
    const int currentValueType = var->getValueType();
    //most anything is ok to do.  We just want to enforce these rules:
    //you cannot change the type of a table that has children
    //rule removed to see if anything bad happens:
    //you cannot change anything to a table that isn't a table already
    if (currentValueType == LUA_TFUNCTION || currentNameType == LUA_TTABLE) {
        return 0; //no recasting functions or table keys
    }

    if (newValueType == LUA_TTABLE && currentValueType != LUA_TTABLE) {
        //trying to change a table to something else
        if (!var->getChildren(false).empty()) {
            return 0;
        }
        //no children, we can do this without bad things happening
        return 1;
    }

    if (currentNameType == newNameType && currentValueType == newValueType) {
        return 2;
    }
    return 1;
}

// A rename the Variables view asked for and did not get: nothing else on screen
// would show that anything was refused, and the row has meanwhile been put back
// to the name the variable still answers to.
void dlgTriggerEditor::showVariableRenameRefused(TVar* variable)
{
    //: Warning shown in the editor's Variables view when a rename could not be carried out. %1 is the name the variable keeps.
    showWarning(tr("\"%1\" was not renamed: another member of the same table already has that name, or this variable's key is a table or a function, which has no name to change.")
                        .arg(variable->getName().toHtmlEscaped()));
}

void dlgTriggerEditor::saveVar()
{
    // We can enter this function if:
    // we click on a variable without having one selected ( no parent )
    // we click on a variable from another variable
    // we click on a variable from having the top-most element selected ( parent but parent is not a variable/table )
    // we click on a variable from the same variable (such as a double click)
    // we add a new variable
    // we switch away from a variable (so we are saving the old variable)

    if (!mpCurrentVarItem) {
        return;
    }
    QTreeWidgetItem* pItem = mpCurrentVarItem;
    if (!pItem->parent()) {
        return;
    }
    auto* luaInterface = mpHost->getLuaInterface();
    auto* varUnit = luaInterface->getVarUnit();
    TVar* variable = treeWidget_variables->variableForRow(varUnit, pItem);
    bool newVar = false;
    if (!variable) {
        newVar = true;
        variable = treeWidget_variables->newVariableForRow(varUnit, pItem);
    }
    if (!variable) {
        return;
    }
    const QString newName = mpVarsMainArea->lineEdit_var_name->text();
    QString newValue = mpSourceEditorEdbeeDocument->text();
    if (newName.isEmpty()) {
        slot_variableSelected(pItem);
        return;
    }
    // Everything below reaches the variable by the name the tree gave it, and a
    // write through a name that does not reach it lands on a key of its own,
    // leaving a second variable beside the real one (#9903). Quietly, because
    // selecting the variable already said so. A new variable is exempt: its name
    // is the key about to be created, so there is nothing for it to find yet.
    if (!newVar && !luaInterface->writableByName(variable)) {
        return;
    }
    mChangingVar = true;
    // said at the very end: the tail of this function re-selects the row, which
    // clears whatever notification is on screen
    bool renameRefused = false;
    int uiNameType = mpVarsMainArea->comboBox_variable_key_type->itemData(mpVarsMainArea->comboBox_variable_key_type->currentIndex(), Qt::UserRole).toInt();
    int uiValueType = mpVarsMainArea->comboBox_variable_value_type->itemData(mpVarsMainArea->comboBox_variable_value_type->currentIndex(), Qt::UserRole).toInt();
    if ((uiNameType == LUA_TNUMBER || uiNameType == LUA_TSTRING) && newVar) {
        uiNameType = LUA_TNONE;
    }
    //check variable recasting
    const int varRecast = canRecast(pItem, uiNameType, uiValueType);
    if ((uiNameType == -1) || (variable && uiNameType != variable->getKeyType())) {
        bool nameNumberOk = false;
        newName.toDouble(&nameNumberOk);
        // the key type combobox shows a boolean key but does not offer it as a
        // choice, so a name that still reads as a boolean keeps its boolean key
        if (variable->getKeyType() == LUA_TBOOLEAN && (newName.toLower() == QLatin1String("true") || newName.toLower() == QLatin1String("false"))) {
            uiNameType = LUA_TBOOLEAN;
        } else if (nameNumberOk) {
            uiNameType = LUA_TNUMBER;
        } else {
            uiNameType = LUA_TSTRING;
        }
    }
    if ((uiValueType != LUA_TTABLE) && (uiValueType == -1)) {
        bool valueNumberOk = false;
        newValue.toDouble(&valueNumberOk);
        if (valueNumberOk) {
            uiValueType = LUA_TNUMBER;
        } else if (newValue.toLower() == "true" || newValue.toLower() == "false") {
            uiValueType = LUA_TBOOLEAN;
        } else {
            uiValueType = LUA_TSTRING;
        }
    }
    if (varRecast == 2) {
        //we sometimes get in here from new variables
        if (newVar) {
            //we're making this var
            variable = treeWidget_variables->newVariableForRow(varUnit, pItem);
            if (!variable) {
                variable = new TVar();
            }
            variable->setName(newName, uiNameType);
            variable->setValue(newValue, uiValueType);
            // Before createVar(): the Lua it runs can rebuild the tree, which would delete it
            treeWidget_variables->forgetNewVariableForRow(pItem);
            luaInterface->createVar(variable);
            varUnit->addVariable(variable);
            treeWidget_variables->setVariableForRow(varUnit, pItem, variable);
            // Attach to its real parent TVar (set in addVar) so the XML writer,
            // which iterates from the base, nests it inside the parent table
            // rather than at root level.
            TVar* parentVar = variable->getParent();
            if (!parentVar) {
                parentVar = varUnit->getBase();
            }
            parentVar->addChild(variable);
            pItem->setText(0, newName);
            mpCurrentVarItem = nullptr;
        } else if (variable) {
            if (newName == variable->getName() && (variable->getValueType() == LUA_TTABLE && newValue == variable->getValue())) {
                //no change made
            } else {
                //we're trying to rename it/recast it
                int change = 0;
                if (newName != variable->getName() || uiNameType != variable->getKeyType()) {
                    //let's make sure the nametype works
                    bool nameNumberOk = false;
                    newName.toDouble(&nameNumberOk);
                    if (variable->getKeyType() == LUA_TBOOLEAN && (newName.toLower() == QLatin1String("true") || newName.toLower() == QLatin1String("false"))) {
                        uiNameType = LUA_TBOOLEAN;
                    } else if (variable->getKeyType() == LUA_TNUMBER && nameNumberOk) {
                        uiNameType = LUA_TNUMBER;
                    } else {
                        uiNameType = LUA_TSTRING;
                    }
                    change = change | 0x1;
                }
                variable->setNewName(newName, uiNameType);
                if (variable->getValueType() != LUA_TTABLE && (newValue != variable->getValue() || uiValueType != variable->getValueType())) {
                    //let's check again
                    bool valueNumberOk = false;
                    newValue.toDouble(&valueNumberOk);
                    if (variable->getValueType() == LUA_TTABLE) {
                        //HEIKO: obvious logic error used to be valueType == LUA_TABLE
                        uiValueType = LUA_TTABLE;
                    } else if (uiValueType == LUA_TNUMBER && valueNumberOk) {
                        uiValueType = LUA_TNUMBER;
                    } else if (uiValueType == LUA_TBOOLEAN && (newValue.toLower() == "true" || newValue.toLower() == "false")) {
                        uiValueType = LUA_TBOOLEAN;
                    } else {
                        uiValueType = LUA_TSTRING; //nope, you don't agree, you lose your value
                    }
                    variable->setValue(newValue, uiValueType);
                    change = change | 0x2;
                }
                if (change) {
                    bool renamed = true;
                    if (change & 0x1 || newVar) {
                        renamed = luaInterface->renameVar(variable);
                    }
                    if ((variable->getValueType() != LUA_TTABLE && change & 0x2) || newVar) {
                        luaInterface->setValue(variable);
                    }
                    if (renamed) {
                        pItem->setText(0, newName);
                        mpCurrentVarItem = nullptr;
                    } else {
                        // the variable still answers to the name it had, so
                        // that is what the row has to go back to showing
                        pItem->setText(0, variable->getName());
                        renameRefused = true;
                    }
                } else {
                    // nothing was renamed, so there is a pending new name to
                    // drop rather than to write onto the variable
                    variable->abandonNewName();
                }
            }
        }
    } else if (varRecast == 1) { //recast it
        TVar* var = treeWidget_variables->variableForRow(varUnit, pItem);
        if (newVar) {
            //we're making this var
            var = treeWidget_variables->newVariableForRow(varUnit, pItem);
            var->setName(newName, uiNameType);
            var->setValue(newValue, uiValueType);
            treeWidget_variables->forgetNewVariableForRow(pItem);
            luaInterface->createVar(var);
            varUnit->addVariable(var);
            treeWidget_variables->setVariableForRow(varUnit, pItem, var);
            pItem->setText(0, newName);
            mpCurrentVarItem = nullptr;
        } else if (var) {
            //we're trying to rename it/recast it
            int change = 0;
            if (newName != var->getName() || uiNameType != var->getKeyType()) {
                //let's make sure the nametype works
                bool nameNumberOk = false;
                newName.toDouble(&nameNumberOk);
                if (uiNameType == LUA_TSTRING) {
                    //do nothing, we can always make key to string
                } else if (var->getKeyType() == LUA_TBOOLEAN && (newName.toLower() == QLatin1String("true") || newName.toLower() == QLatin1String("false"))) {
                    uiNameType = LUA_TBOOLEAN;
                } else if (var->getKeyType() == LUA_TNUMBER && nameNumberOk) {
                    uiNameType = LUA_TNUMBER;
                } else {
                    uiNameType = LUA_TSTRING;
                }
                var->setNewName(newName, uiNameType);
                change = change | 0x1;
            }
            // a table's contents are not shown in the value editor, so a table staying a table is never a value change
            if ((newValue != var->getValue() || uiValueType != var->getValueType()) && !(uiValueType == LUA_TTABLE && var->getValueType() == LUA_TTABLE)) {
                //let's check again
                bool valueNumberOk = false;
                newValue.toDouble(&valueNumberOk);
                if (uiValueType == LUA_TTABLE) {
                    newValue = "{}";
                } else if (uiValueType == LUA_TNUMBER && valueNumberOk) {
                    uiValueType = LUA_TNUMBER;
                } else if (uiValueType == LUA_TBOOLEAN && (newValue.toLower() == QLatin1String("true") || newValue.toLower() == QLatin1String("false"))) {
                    uiValueType = LUA_TBOOLEAN;
                } else {
                    uiValueType = LUA_TSTRING; //nope, you don't agree, you lose your value
                }
                var->setValue(newValue, uiValueType);
                change = change | 0x2;
            }
            if (change) {
                bool renamed = true;
                if (change & 0x1 || newVar) {
                    renamed = luaInterface->renameVar(var);
                }
                if (change & 0x2 || newVar) {
                    luaInterface->setValue(var);
                }
                if (renamed) {
                    pItem->setText(0, newName);
                    mpCurrentVarItem = nullptr;
                } else {
                    pItem->setText(0, var->getName());
                    renameRefused = true;
                }
            }
        }
    }
    //redo this here in case we changed type
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
    if (!varUnit->shouldSave(variable)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        const QString reason = varUnit->getUnsaveableReason(variable);
        pItem->setToolTip(0, reason.isEmpty() ? QString() : utils::richText(reason));
        pItem->setCheckState(0, Qt::Unchecked);
    } else if (varUnit->isSaved(variable)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    refreshVariableRow(pItem);
    moveVariableRowToSortedPlace(pItem, variable);
    mChangingVar = false;
    slot_variableSelected(pItem);
    if (renameRefused) {
        showVariableRenameRefused(variable);
    }
}

// A full rebuild sorts the rows, so a row named in place has to go where that
// sort would put it
void dlgTriggerEditor::moveVariableRowToSortedPlace(QTreeWidgetItem* pItem, TVar* pVariable)
{
    QTreeWidgetItem* pParentItem = pItem->parent();
    if (!pParentItem || !pVariable) {
        return;
    }
    VarUnit* varUnit = mpHost->getLuaInterface()->getVarUnit();
    auto siblingVariable = [&](const int index) -> TVar* {
        QTreeWidgetItem* pSiblingItem = pParentItem->child(index);
        TVar* pSibling = treeWidget_variables->variableForRow(varUnit, pSiblingItem);
        return pSibling ? pSibling : treeWidget_variables->newVariableForRow(varUnit, pSiblingItem);
    };
    const int from = pParentItem->indexOfChild(pItem);
    // Every row switch saves the row left behind, so one already in order stays put without
    // a walk over its siblings, and is not moved past a sibling that sorts alike ("1", "01")
    TVar* pPrevious = from > 0 ? siblingVariable(from - 1) : nullptr;
    TVar* pNext = from + 1 < pParentItem->childCount() ? siblingVariable(from + 1) : nullptr;
    if ((!pPrevious || !TVarLessThan(pVariable, pPrevious)) && (!pNext || !TVarLessThan(pNext, pVariable))) {
        return;
    }
    int to = pParentItem->childCount() - 1;
    for (int i = 0; i < pParentItem->childCount(); ++i) {
        if (i == from) {
            continue;
        }
        TVar* pSibling = siblingVariable(i);
        if (pSibling && TVarLessThan(pVariable, pSibling)) {
            to = i > from ? i - 1 : i;
            break;
        }
    }
    if (to == from) {
        return;
    }

    // The view forgets the expanded and selected state of every row taken out
    // of it, and moves the current row off one it loses
    QTreeWidgetItem* pCurrentItem = treeWidget_variables->currentItem();
    const QList<QTreeWidgetItem*> selectedItems = treeWidget_variables->selectedItems();
    QList<QTreeWidgetItem*> expandedItems;
    QList<QTreeWidgetItem*> pendingItems{pItem};
    while (!pendingItems.isEmpty()) {
        QTreeWidgetItem* pEntry = pendingItems.takeLast();
        if (pEntry->isExpanded()) {
            expandedItems.append(pEntry);
        }
        for (int i = 0; i < pEntry->childCount(); ++i) {
            pendingItems.append(pEntry->child(i));
        }
    }
    // Taking the row out and putting the selection back both emit
    // itemSelectionChanged, whose slot would refill the form in the middle of this save
    const QSignalBlocker blocker(treeWidget_variables);
    pParentItem->takeChild(from);
    pParentItem->insertChild(to, pItem);
    for (QTreeWidgetItem* pExpandedItem : std::as_const(expandedItems)) {
        pExpandedItem->setExpanded(true);
    }
    if (pCurrentItem && treeWidget_variables->currentItem() != pCurrentItem) {
        treeWidget_variables->setCurrentItem(pCurrentItem, 0, QItemSelectionModel::NoUpdate);
    }
    treeWidget_variables->clearSelection();
    for (QTreeWidgetItem* pSelectedItem : selectedItems) {
        pSelectedItem->setSelected(true);
    }
}

void dlgTriggerEditor::saveKey()
{
    QTreeWidgetItem* pItem = mpCurrentKeyItem;
    if (!pItem) {
        return;
    }

    // Ensure the item is still part of the tree widget
    if (pItem->treeWidget() != treeWidget_keys) {
        return;
    }

    // The toolbar's buttons take no focus, so Save with a grab armed would
    // otherwise leave it armed and read the emptied field below as the name
    endKeyGrab();

    mpKeysMainArea->trimName();
    QString name = mpKeysMainArea->lineEdit_key_name->text();
    if (name.isEmpty() || name == tr("New key")) {
        // An empty binding field shows its placeholder, so name the item by what the reader sees there
        name = mpKeysMainArea->lineEdit_key_binding->text();
        if (name.isEmpty()) {
            name = mpKeysMainArea->lineEdit_key_binding->placeholderText();
        }
        if (name.isEmpty()) {
            name = tr("New key");
        }
    }
    const QString command = mpKeysMainArea->lineEdit_key_command->text();
    const QString script = mpSourceEditorEdbeeDocument->text();


    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(triggerID);
    if (pT) {
        // Capture OLD state before modifications (for undo)
        QString oldStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);

        // A grab can already have named a new key after its binding
        const bool firstSave = pT->getName() == tr("New key") || (!pT->isFolder() && mpUndoStack->addingItem(EditorViewType::cmKeysView, triggerID));
        pItem->setText(0, name);
        pT->setName(name);
        if (mpKeysMainArea->lineEdit_key_name->text() != name) {
            mpKeysMainArea->lineEdit_key_name->setText(name);
        }
        pT->setCommand(command);
        pT->setScript(script);

        pT->validateKeyBinding();

        QString itemDescription;
        const bool itemActive = pT->shownActive(mpHost->mEmergencyStop);
        if (pT->isFolder()) {
            itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
            if (itemActive && !pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            if (itemActive) {
                itemDescription = descActive;
                if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                itemDescription = descInactive;
            }
        }

        if (pT->state()) {
            clearEditorNotification();
            if (firstSave) {
                if (pT->isFolder()) {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActiveFolder;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveFolder;
                    }
                } else {
                    if (pT->shouldBeActive()) {
                        itemDescription = descActive;
                        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                    }
                }
                pItem->setText(0, name);

                // The emergency stop's resume switches it on; doing so here would let it run during the stop
                if (pT->shouldBeActive() && !mpHost->mEmergencyStop) {
                    pT->setIsActive(true);
                }
            } else {
                pItem->setText(0, name);
            }
        } else {
            itemDescription = descError;
            pItem->setText(0, name);
            reportCompileError(pT->getError());
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));

        // Capture NEW state after modifications (for redo)
        QString newStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);

        // Only push undo command if something actually changed
        if (oldStateXML != newStateXML) {
            auto* qtCmd = new EditorModifyPropertyCommand(EditorViewType::cmKeysView, triggerID, name, oldStateXML, newStateXML, mpHost);
            mpUndoStack->pushCommand(qtCmd);

            // Clear edbee undo stack after save to make Save a commit point
            if (mpTextUndoStack) {
                mpTextUndoStack->clear();
            }
        }
        mpUndoStack->finishAddingItem(EditorViewType::cmKeysView, triggerID);
    }
}


void dlgTriggerEditor::setupPatternControls(const int type, dlgTriggerPatternEdit* pItem)
{
    // Display middle dots for potentially unwanted spaces in perl regex
    if (type == REGEX_PERL) {
        markQTextEdit(pItem->singleLineTextEdit_pattern);
        lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = true;
        pItem->singleLineTextEdit_pattern->blockSignals(true);
        pItem->singleLineTextEdit_pattern->rehighlight();
        pItem->singleLineTextEdit_pattern->blockSignals(false);
    } else {
        unmarkQTextEdit(pItem->singleLineTextEdit_pattern);
        lineEditShouldMarkSpaces[pItem->singleLineTextEdit_pattern] = false;
    }

    switch (type) {
    case REGEX_SUBSTRING:
    case REGEX_PERL:
    case REGEX_BEGIN_OF_LINE_SUBSTRING:
    case REGEX_EXACT_MATCH:
    case REGEX_LUA_CODE:
        pItem->singleLineTextEdit_pattern->setHighlightingEnabled(type == REGEX_PERL);
        pItem->singleLineTextEdit_pattern->show();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        break;
    case REGEX_LINE_SPACER:
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->show();
        break;
    case REGEX_COLOR_PATTERN:
        // CHECKME: Do we need to regenerate (hidden patter text) and button texts/colors?
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->show();
        pItem->pushButton_bgColor->show();
        pItem->label_prompt->hide();
        pItem->spinBox_lineSpacer->hide();
        break;
    case REGEX_PROMPT:
        pItem->singleLineTextEdit_pattern->hide();
        pItem->pushButton_fgColor->hide();
        pItem->pushButton_bgColor->hide();
        if (mpHost->mTelnet.mGA_Driver) {
            pItem->label_prompt->setText(tr("match on the prompt line"));
            pItem->label_prompt->setToolTip(QString());
            pItem->label_prompt->setEnabled(true);
        } else {
            pItem->label_prompt->setText(tr("match on the prompt line (disabled)"));
            pItem->label_prompt->setToolTip(utils::richText(tr("A Go-Ahead (GA) signal from the game is required to make this feature work")));
            pItem->label_prompt->setEnabled(false);
        }
        pItem->label_prompt->show();
        pItem->spinBox_lineSpacer->hide();
        break;
    }

    // All three walk every row, so a move or delete, which sets one row after
    // another, runs them itself once it has finished
    if (mPatternBulkEdit) {
        return;
    }

    checkForMoreThanOneTriggerItem();
    updatePatternTabOrder();
    updatePatternPlaceholders();
}

// Typing never adds or removes rows, so nothing appears or vanishes under the cursor
void dlgTriggerEditor::handlePatternChange()
{
    checkForMoreThanOneTriggerItem();
    updatePatternPlaceholders();
}

// One row always stays: an empty trigger is one waiting for its first pattern
void dlgTriggerEditor::compactPatternRows()
{
    checkForMoreThanOneTriggerItem();

    int lastActive = -1;
    for (int i = 0; i < mVisiblePatternCount && i < mTriggerPatternEdit.size(); ++i) {
        const auto* item = mTriggerPatternEdit.at(i);
        if (!item) {
            continue;
        }

        bool itemHasContent = !item->singleLineTextEdit_pattern->toPlainText().isEmpty();
        const int type = item->comboBox_patternType->currentIndex();
        if (type == REGEX_PROMPT) {
            itemHasContent = true;
        } else if (type == REGEX_LINE_SPACER) {
            itemHasContent = item->spinBox_lineSpacer->value() > 0 || item->spinBox_lineSpacer->isVisible();
        }

        if (itemHasContent) {
            lastActive = i;
        }
    }

    const int desiredCount = qMax(lastActive + 1, 1);
    if (desiredCount != mVisiblePatternCount) {
        showPatternItems(desiredCount);
        // Give the freed room back to the code pane now, not when the trigger is next chosen
        fitFormPaneToItsContents();
    } else {
        updatePatternPlaceholders();
    }
}

QWidget* dlgTriggerEditor::firstFocusablePatternWidget(const dlgTriggerPatternEdit* patternItem) const
{
    if (!patternItem) {
        return nullptr;
    }

    if (patternItem->singleLineTextEdit_pattern->isVisible()) {
        return patternItem->singleLineTextEdit_pattern;
    }
    if (patternItem->spinBox_lineSpacer->isVisible()) {
        return patternItem->spinBox_lineSpacer;
    }
    if (patternItem->pushButton_fgColor->isVisible()) {
        return patternItem->pushButton_fgColor;
    }
    if (patternItem->pushButton_bgColor->isVisible()) {
        return patternItem->pushButton_bgColor;
    }

    return patternItem->comboBox_patternType;
}

bool dlgTriggerEditor::focusPatternItem(const int row, const Qt::FocusReason reason)
{
    if (row < 0 || row >= mVisiblePatternCount || row >= mTriggerPatternEdit.size()) {
        return false;
    }

    auto* patternItem = mTriggerPatternEdit.value(row, nullptr);
    if (!patternItem || !patternItem->isVisible()) {
        return false;
    }

    QWidget* target = firstFocusablePatternWidget(patternItem);
    if (!target) {
        return false;
    }

    mpScrollArea->ensureWidgetVisible(patternItem);
    target->setFocus(reason);

    if (auto* edit = qobject_cast<SingleLineTextEdit*>(target)) {
        auto cursor = edit->textCursor();
        cursor.select(QTextCursor::Document);
        edit->setTextCursor(cursor);
    } else if (auto* spinBox = qobject_cast<QSpinBox*>(target)) {
        spinBox->selectAll();
    }

    return true;
}

bool dlgTriggerEditor::focusNextPatternItem(const dlgTriggerPatternEdit* currentItem)
{
    if (!currentItem) {
        return false;
    }

    int nextRow = currentItem->mRow + 1;
    while (nextRow < mVisiblePatternCount && nextRow < mTriggerPatternEdit.size()) {
        auto* nextItem = mTriggerPatternEdit.value(nextRow, nullptr);
        if (nextItem && nextItem->isVisible()) {
            return focusPatternItem(nextRow);
        }
        ++nextRow;
    }

    return false;
}


bool dlgTriggerEditor::focusPreviousPatternItem(const dlgTriggerPatternEdit* currentItem)
{
    if (!currentItem) {
        return false;
    }

    int previousRow = currentItem->mRow - 1;
    while (previousRow >= 0) {
        auto* previousItem = mTriggerPatternEdit.value(previousRow, nullptr);
        if (previousItem && previousItem->isVisible()) {
            return focusPatternItem(previousRow);
        }
        --previousRow;
    }

    return false;
}


void dlgTriggerEditor::updatePatternTabOrder()
{
    if (!mpTriggersMainArea) {
        return;
    }

    QWidget* previous = mpTriggersMainArea->lineEdit_trigger_name;
    auto addToChain = [&previous, this](QWidget* next) {
        if (!next || !previous) {
            if (next) {
                previous = next;
            }
            return;
        }

        if (!next->isVisibleTo(mpTriggersMainArea)) {
            return;
        }

        QWidget::setTabOrder(previous, next);
        previous = next;
    };

    // The ID pill is a label and takes no focus. spinBox_lineMargin is always
    // hidden: the two segments are how the mode it holds is reached.
    addToChain(mpTriggersMainArea->lineEdit_trigger_command);
    addToChain(mpTriggersMainArea->toolButton_toggleExtraControls);
    addToChain(mpRadioButton_matchAny);
    addToChain(mpRadioButton_matchAll);
    addToChain(mpSpinBox_matchWithinLines);
    addToChain(mpTriggersMainArea->checkBox_perlSlashGOption);
    addToChain(mpTriggersMainArea->spinBox_stayOpen);
    addToChain(mpTriggersMainArea->checkBox_filterTrigger);
    addToChain(mpTriggersMainArea->checkBox_soundTrigger);
    addToChain(mpTriggersMainArea->lineEdit_soundFile);
    addToChain(mpTriggersMainArea->toolButton_clearSoundFile);
    addToChain(mpTriggersMainArea->checkBox_triggerColorizer);
    addToChain(mpTriggersMainArea->pushButtonFgColor);
    addToChain(mpTriggersMainArea->pushButtonBgColor);

    for (int i = 0; i < mVisiblePatternCount && i < mTriggerPatternEdit.size(); ++i) {
        auto* item = mTriggerPatternEdit.value(i, nullptr);
        if (!item || !item->isVisible()) {
            continue;
        }

        QWidget* first = firstFocusablePatternWidget(item);
        addToChain(first);

        if (item->spinBox_lineSpacer->isVisible() && item->spinBox_lineSpacer != first) {
            addToChain(item->spinBox_lineSpacer);
        }
        if (item->pushButton_fgColor->isVisible()) {
            addToChain(item->pushButton_fgColor);
        }
        if (item->pushButton_bgColor->isVisible()) {
            addToChain(item->pushButton_bgColor);
        }
        if (item->comboBox_patternType->isVisible()) {
            addToChain(item->comboBox_patternType);
        }
        // Drawn only under the mouse, but the row shows it while it holds focus
        addToChain(item->toolButton_deletePattern);
    }
    // The grip is left out: Ctrl+Alt+Up and Ctrl+Alt+Down reorder from the keyboard
    addToChain(mpButton_addPattern);
    addToChain(mpSourceEditorEdbee);
}

void dlgTriggerEditor::slot_changedPattern()
{
    SingleLineTextEdit* textEdit = qobject_cast<SingleLineTextEdit*>(sender());

    if (textEdit && lineEditShouldMarkSpaces[textEdit]) {
        markQTextEdit(textEdit);
        textEdit->blockSignals(true);
        textEdit->rehighlight();
        textEdit->blockSignals(false);
    }

    handlePatternChange();
}

// Fills only empty text rows, so nothing is overwritten; Lua code is one chunk a split would break
void dlgTriggerEditor::slot_pastedExtraPatternLines(const QStringList& lines)
{
    auto* textEdit = qobject_cast<SingleLineTextEdit*>(sender());
    auto* patternItem = textEdit ? qobject_cast<dlgTriggerPatternEdit*>(textEdit->parentWidget()) : nullptr;
    if (!patternItem) {
        return;
    }
    const int sourceType = patternItem->comboBox_patternType->currentIndex();
    if (sourceType == REGEX_LUA_CODE) {
        return;
    }

    // Rows the paste opens take the pasted row's type; rows already shown keep the type the player gave them
    const int firstNewRow = mVisiblePatternCount;
    int row = patternItem->mRow;
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        dlgTriggerPatternEdit* target = nullptr;
        while (!target && ++row < scmEditorPatternRowLimit) {
            if (row >= mVisiblePatternCount) {
                showPatternItems(row + 1);
            }
            auto* candidate = mTriggerPatternEdit.value(row, nullptr);
            if (!candidate || !candidate->singleLineTextEdit_pattern->toPlainText().isEmpty()) {
                continue;
            }
            const int type = candidate->comboBox_patternType->currentIndex();
            if (type == REGEX_SUBSTRING || type == REGEX_PERL || type == REGEX_BEGIN_OF_LINE_SUBSTRING || type == REGEX_EXACT_MATCH) {
                target = candidate;
            }
        }
        if (!target) {
            break;
        }

        if (row >= firstNewRow && target->comboBox_patternType->currentIndex() == REGEX_SUBSTRING) {
            target->comboBox_patternType->setCurrentIndex(sourceType);
        }
        target->singleLineTextEdit_pattern->setPlainText(line);
    }
    if (mVisiblePatternCount > firstNewRow) {
        fitFormPaneToItsContents();
    }
}

void dlgTriggerEditor::slot_lineSpacerChanged(int)
{
    handlePatternChange();
}

// This can get called after the lineEdit contents has changed and it is now a
// color pattern - ought to update coloration if it has been edited by hand
// but need to source the colors
void dlgTriggerEditor::slot_setupPatternControls(int type)
{
    QComboBox* pBox = qobject_cast<QComboBox*>(sender());
    if (!pBox) {
        return;
    }

    const int row = pBox->itemData(0).toInt();
    if (row < 0 || row >= mTriggerPatternEdit.size()) {
        return;
    }

    // This is the collection of widgets that make up one of the patterns
    // in the dlgTriggerMainArea:
    dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[row];
    setupPatternControls(type, pPatternItem);
    if (type == REGEX_COLOR_PATTERN) {
        if (pPatternItem->singleLineTextEdit_pattern->toPlainText().isEmpty()) {
            // This COLOR trigger is a new one in that there is NO text
            // So set it to the default (ignore both) - which will generate an
            // error if saved without setting a color for at least one element:

            pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(TTrigger::scmIgnored, TTrigger::scmIgnored));
        }

        // Only process the text if it looks like it should:
        if ((pPatternItem->singleLineTextEdit_pattern->toPlainText().startsWith(QLatin1String("ANSI_COLORS_F{"))
             && pPatternItem->singleLineTextEdit_pattern->toPlainText().contains(QLatin1String("}_B{")) && pPatternItem->singleLineTextEdit_pattern->toPlainText().endsWith(QLatin1String("}")))) {
            // It looks as though there IS a valid color pattern string in the
            // lineEdit, so, in case it has been edited by hand, regenerate the
            // colors that are used:
            int textAnsiFg = TTrigger::scmIgnored;
            int textAnsiBg = TTrigger::scmIgnored;
            TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), textAnsiFg, textAnsiBg);

            if (textAnsiFg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                //: Color trigger ignored foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored"));
            } else if (textAnsiFg == TTrigger::scmDefault) {
                pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                //: Color trigger default foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Default foreground color"));
            } else {
                pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiFg, false)));
                //: Color trigger ANSI foreground color button, ensure all three instances have the same text
                pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]").arg(QString::number(textAnsiFg)));
            }

            if (textAnsiBg == TTrigger::scmIgnored) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                //: Color trigger ignored background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Background color ignored"));
            } else if (textAnsiBg == TTrigger::scmDefault) {
                pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                //: Color trigger default background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Default background color"));
            } else {
                pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(mpHost->getAnsiColor(textAnsiBg, true)));
                //: Color trigger ANSI background color button, ensure all three instances have the same text
                pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]").arg(QString::number(textAnsiBg)));
            }
        }
        // Commented-out debug code:
        // else {
        //     qDebug() << "dlgTriggerEditor::slot_setupPatternControls(...) ERROR: Pattern listed as item:"
        //              << row + 1
        //              << "is supposed to be a color pattern trigger but the stored text that contains the color codes:"
        //              << pPatternItem->singleLineTextEdit_pattern->toPlainText()
        //              << "does not fit the pattern!";
        // }

    } else {
        // Is NOT a REGEX_COLOR_PATTERN - if the text corresponds to the color
        // pattern text equivalent to ignore both fore and back ground then
        // clear the text - otherwise leave as is:
        if (pPatternItem->singleLineTextEdit_pattern->toPlainText().compare(QLatin1String("ANSI_COLORS_F{IGNORE}_B{IGNORE}")) == 0) {
            pPatternItem->singleLineTextEdit_pattern->clear();
        }
    }

    handlePatternChange();
}

void dlgTriggerEditor::slot_triggerSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentTriggerItem) {
        if (mpCurrentTriggerItem) {
            saveEditorState(EditorViewType::cmTriggerView, mpCurrentTriggerItem->data(0, Qt::UserRole).toInt());
        }
        saveTrigger();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpCurrentTriggerItem = pItem;
    mpTriggersMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
    clearDocument(mpSourceEditorEdbee); // Trigger Select
    mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(false);
    mpTriggersMainArea->checkBox_filterTrigger->setChecked(false);
    mpTriggersMainArea->checkBox_triggerColorizer->setChecked(false);
    mpTriggersMainArea->pushButtonFgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, QVariant());
    describeColourWell(mpTriggersMainArea->pushButtonFgColor);
    mpTriggersMainArea->pushButtonBgColor->setStyleSheet(QString());
    mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, QVariant());
    describeColourWell(mpTriggersMainArea->pushButtonBgColor);
    mpTriggersMainArea->spinBox_lineMargin->setValue(-1);

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(ID);
    if (pT) {
        const QStringList patternList = pT->getPatternsList();
        QList<int> const propertyList = pT->getRegexCodePropertyList();

        if (patternList.size() != propertyList.size()) {
            return;
        }

        // No trailing empty row: the Add pattern button asks for one more
        showPatternItems(qMax(static_cast<int>(patternList.size()), 1));
        for (int i = 0; i < patternList.size() && i < mTriggerPatternEdit.size(); i++) {
            if (i >= pT->mColorPatternList.size()) {
                break;
            }
            // Use operator[] so we have write access to the array/list member:
            dlgTriggerPatternEdit* pPatternItem = mTriggerPatternEdit[i];
            const int pType = propertyList.at(i);
            if (!pType) {
                // If the control is for the default (0) case nudge the setting
                // up and down so that it copies the colour icon for the
                // subString type across into the QLineEdit:
                pPatternItem->comboBox_patternType->setCurrentIndex(1);
                setupPatternControls(1, pPatternItem);
            }
            pPatternItem->comboBox_patternType->setCurrentIndex(pType);
            setupPatternControls(pType, pPatternItem);
            if (pType == REGEX_PROMPT) {
                pPatternItem->singleLineTextEdit_pattern->clear();

            } else if (pType == REGEX_COLOR_PATTERN) {
                pPatternItem->singleLineTextEdit_pattern->setPlainText(patternList.at(i));
                if (pT->mColorPatternList.at(i)) {
                    if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        //: Color trigger ignored foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color ignored"));
                    } else if (pT->mColorPatternList.at(i)->ansiFg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                        //: Color trigger default foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Default foreground color"));
                    } else {
                        pPatternItem->pushButton_fgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mFgColor));
                        //: Color trigger ANSI foreground color button, ensure all three instances have the same text
                        pPatternItem->pushButton_fgColor->setText(tr("Foreground color [ANSI %1]").arg(QString::number(pT->mColorPatternList.at(i)->ansiFg)));
                    }

                    if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmIgnored) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        //: Color trigger ignored background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Background color ignored"));
                    } else if (pT->mColorPatternList.at(i)->ansiBg == TTrigger::scmDefault) {
                        pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                        //: Color trigger default background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Default background color"));
                    } else {
                        pPatternItem->pushButton_bgColor->setStyleSheet(generateButtonStyleSheet(pT->mColorPatternList.at(i)->mBgColor));
                        //: Color trigger ANSI background color button, ensure all three instances have the same text
                        pPatternItem->pushButton_bgColor->setText(tr("Background color [ANSI %1]").arg(QString::number(pT->mColorPatternList.at(i)->ansiBg)));
                    }
                } else {
                    qWarning() << "dlgTriggerEditor::slot_triggerSelected(...) ERROR: TTrigger instance has an mColorPattern of size:" << pT->mColorPatternList.size() << "but array element:" << i
                               << "is a nullptr";
                    pPatternItem->pushButton_fgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                    pPatternItem->pushButton_bgColor->setStyleSheet(QString());
                    pPatternItem->pushButton_fgColor->setText(tr("fault"));
                }
            } else if (pType == REGEX_LINE_SPACER) {
                pPatternItem->spinBox_lineSpacer->setValue(patternList.at(i).toInt());
            } else {
                // Keep track of lineEdits that should have trailing spaces marked
                if (pType == REGEX_PERL) {
                    lineEditShouldMarkSpaces[pPatternItem->singleLineTextEdit_pattern] = true;
                }
                pPatternItem->singleLineTextEdit_pattern->setPlainText(patternList.at(i));
            }
        }

        // reset the rest of the patterns that don't have any data
        for (int i = patternList.size(); i < mVisiblePatternCount; i++) {
            auto* patternItem = mTriggerPatternEdit[i];
            patternItem->singleLineTextEdit_pattern->clear();
            patternItem->pushButton_fgColor->hide();
            patternItem->pushButton_bgColor->hide();
            patternItem->label_prompt->hide();
            patternItem->spinBox_lineSpacer->hide();
            patternItem->comboBox_patternType->setCurrentIndex(0);
        }
        // Otherwise a long trigger opens scrolled away from its first patterns
        mpScrollArea->verticalScrollBar()->setValue(0);
        const QString command = pT->getCommand();
        mpTriggersMainArea->lineEdit_trigger_name->setText(pItem->text(0));
        mpTriggersMainArea->label_idNumber->setText(QString::number(ID));
        mpTriggersMainArea->lineEdit_trigger_command->setText(command);
        mpTriggersMainArea->checkBox_perlSlashGOption->setChecked(pT->mPerlSlashGOption);
        mpTriggersMainArea->checkBox_filterTrigger->setChecked(pT->mFilterTrigger);
        if (pT->isMultiline()) {
            mpTriggersMainArea->spinBox_lineMargin->setValue(pT->getConditionLineDelta());
        } else {
            mpTriggersMainArea->spinBox_lineMargin->setValue(-1);
        }
        mpTriggersMainArea->spinBox_stayOpen->setValue(pT->mStayOpen);
        mpTriggersMainArea->checkBox_soundTrigger->setChecked(pT->mSoundTrigger);
        showTriggerSoundFile(pT->mSoundFile);
        mpTriggersMainArea->checkBox_triggerColorizer->setChecked(pT->isColorizerTrigger());

        const QColor fgColor(pT->getFgColor());
        const QColor bgColor(pT->getBgColor());
        const bool transparentFg = fgColor == QColorConstants::Transparent;
        const bool transparentBg = bgColor == QColorConstants::Transparent;
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(fgColor, pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, transparentFg ? qsl("transparent") : fgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(transparentFg ? tr("keep") : QString());
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(pT->getBgColor(), pT->isColorizerTrigger()));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, transparentBg ? qsl("transparent") : bgColor.name());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(transparentBg ? tr("keep") : QString());
        describeColourWell(mpTriggersMainArea->pushButtonFgColor);
        describeColourWell(mpTriggersMainArea->pushButtonBgColor);

        checkForMoreThanOneTriggerItem();

        clearDocument(mpSourceEditorEdbee, pT->getScript());
        restoreEditorState(EditorViewType::cmTriggerView, ID);

        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);
    } else {
        clearTriggerForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    // Unblock property saves now that item loading is complete
    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

void dlgTriggerEditor::slot_aliasSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearAliasForm();
        return;
    }

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentAliasItem) {
        if (mpCurrentAliasItem) {
            saveEditorState(EditorViewType::cmAliasView, mpCurrentAliasItem->data(0, Qt::UserRole).toInt());
        }
        saveAlias();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpCurrentAliasItem = pItem;
    mpAliasMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpAliasMainArea->lineEdit_alias_name->clear();
    mpAliasMainArea->label_idNumber->clear();
    mpAliasMainArea->lineEdit_alias_pattern->clear();
    mpAliasMainArea->lineEdit_alias_command->clear();
    clearDocument(mpSourceEditorEdbee); // Alias Select

    // mpAliasMainArea->lineEdit_alias_name->setText(pItem->text(0));
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(ID);
    if (pT) {
        const QString pattern = pT->getRegexCode();
        const QString command = pT->getCommand();
        const QString name = pT->getName();

        mpAliasMainArea->lineEdit_alias_pattern->setText(pattern);
        mpAliasMainArea->lineEdit_alias_command->setText(command);
        mpAliasMainArea->lineEdit_alias_name->setText(name);
        mpAliasMainArea->label_idNumber->setText(QString::number(ID));

        clearDocument(mpSourceEditorEdbee, pT->getScript());
        restoreEditorState(EditorViewType::cmAliasView, ID);

        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearAliasForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    // Unblock property saves now that item loading is complete
    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

void dlgTriggerEditor::slot_keySelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearKeyForm();
        return;
    }

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentKeyItem) {
        if (mpCurrentKeyItem) {
            saveEditorState(EditorViewType::cmKeysView, mpCurrentKeyItem->data(0, Qt::UserRole).toInt());
        }
        saveKey();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpCurrentKeyItem = pItem;
    mpKeysMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    mpKeysMainArea->lineEdit_key_command->clear();
    mpKeysMainArea->lineEdit_key_binding->clear();
    mpKeysMainArea->lineEdit_key_name->clear();
    mpKeysMainArea->label_idNumber->clear();
    clearDocument(mpSourceEditorEdbee); // Key Select

    mpKeysMainArea->lineEdit_key_binding->setText(pItem->text(0));
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(ID);
    if (pT) {
        const QString command = pT->getCommand();
        const QString name = pT->getName();
        mpKeysMainArea->lineEdit_key_command->setText(command);
        mpKeysMainArea->lineEdit_key_name->setText(name);
        mpKeysMainArea->label_idNumber->setText(QString::number(ID));
        showKeyBinding();

        clearDocument(mpSourceEditorEdbee, pT->getScript());
        restoreEditorState(EditorViewType::cmKeysView, ID);

        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);
        // A warning given while the editor was closed can be replaced when it opens, so a script-made binding
        // is warned about here too. Not announced, or arrowing through the keys would be talked over.
        showKeyTakenWarning(pItem, takenKeyWarning(mpHost, pT), false);
    } else {
        clearKeyForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    // Unblock property saves now that item loading is complete
    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesUp(QTreeWidgetItem* const pItem, QList<QTreeWidgetItem*>& list)
{
    QTreeWidgetItem* pParentItem = pItem->parent();
    if (pParentItem && pParentItem != mpVarBaseItem) {
        list.append(pParentItem);
        recurseVariablesUp(pParentItem, list);
    }
}

// This should not modify the contents of what pItem points at:
void dlgTriggerEditor::recurseVariablesDown(QTreeWidgetItem* const pItem, QList<QTreeWidgetItem*>& list)
{
    list.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseVariablesDown(pItem->child(i), list);
    }
}

// This WAS called recurseVariablesDown(TVar*, QList<TVar*>&, bool) but it is
// used for searching like the other resursiveSearchXxxxx(...) are
void dlgTriggerEditor::recursiveSearchVariables(TVar* var, QList<TVar*>& list, bool isSorted)
{
    list.append(var);
    QListIterator<TVar*> it(var->getChildren(isSorted));
    while (it.hasNext()) {
        recursiveSearchVariables(it.next(), list, isSorted);
    }
}

void dlgTriggerEditor::slot_variableChanged(QTreeWidgetItem* pItem)
{
    // This handles a small case where the radio button is clicked while the item is currently selected
    // which causes the variable to not save. In places where we populate the TreeWidgetItem, we have
    // to guard it with mChangingVar or else this will be called with every change such as the variable
    // name, etc.
    if (!pItem || mChangingVar) {
        return;
    }
    const int column = 0;
    const int state = pItem->checkState(column);
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = treeWidget_variables->variableForRow(vu, pItem);
    if (!var) {
        return;
    }
    if (state == Qt::Checked || state == Qt::PartiallyChecked) {
        if (vu->isSaved(var)) {
            return;
        }
        // A tick is not on its own a decision to save: Qt's tristate cascade
        // ticks children the user cannot tick themselves, and marks a parent
        // partially ticked from a tick on any child, so what may be saved is
        // asked again here rather than read off the check state (#9957).
        if (!vu->shouldSave(var)) {
            return;
        }
        vu->addSavedVar(var);
        QList<QTreeWidgetItem*> list;
        recurseVariablesUp(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked) && vu->shouldSave(v)) {
                vu->addSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked) && vu->shouldSave(v)) {
                vu->addSavedVar(v);
            }
        }
    } else {
        // we're not checked, dont save us
        if (!vu->isSaved(var)) {
            return;
        }
        vu->removeSavedVar(var);
        QList<QTreeWidgetItem*> list;
        recurseVariablesUp(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->removeSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list);
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked)) {
                vu->removeSavedVar(v);
            }
        }
    }
}

// VariableTreeDelegate makes the clicked row current before calling this. Qt's
// tristate cascade also ticks members VarUnit refuses to save (functions,
// references, oversized tables), so those are unticked again afterwards.
void dlgTriggerEditor::slot_toggleVariableKept()
{
    QTreeWidgetItem* pItem = treeWidget_variables->currentItem();
    if (!pItem || !(pItem->flags() & Qt::ItemIsUserCheckable)) {
        return;
    }
    if (pItem->checkState(0) == Qt::Checked) {
        pItem->setCheckState(0, Qt::Unchecked);
        return;
    }
    // A partly ticked table goes to ticked, as Qt does for a box that is not user-tristate
    pItem->setCheckState(0, Qt::Checked);
    QList<QTreeWidgetItem*> members;
    treeWidget_variables->getAllChildren(pItem, members);
    VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
    for (QTreeWidgetItem* pMember : members) {
        if (!treeWidget_variables->rowCanBeSaved(vu, pMember)) {
            pMember->setCheckState(0, Qt::Unchecked);
        }
    }
}

void dlgTriggerEditor::slot_variableSelected(QTreeWidgetItem* pItem)
{
    if (!pItem || treeWidget_variables->indexOfTopLevelItem(pItem) == 0) {
        // Null item or it is for the first row of the tree
        clearVarForm();
        return;
    }

    clearEditorNotification();

    // save the current variable before switching to the new one
    if (pItem != mpCurrentVarItem) {
        saveVar();
    }

    mChangingVar = true;
    const int column = treeWidget_variables->currentColumn();
    const int state = pItem->checkState(column);
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    TVar* var = treeWidget_variables->variableForRow(vu, pItem); // This does NOT modify pItem or what it points at
    QList<QTreeWidgetItem*> list;
    if (state == Qt::Checked || state == Qt::PartiallyChecked) {
        // What may be saved is asked again rather than read off the check state,
        // for the same reason slot_variableChanged() asks: a row can be left
        // ticked from before whatever now makes it unsaveable - a table grown
        // past the size limit, say - and clicking it would enrol it again.
        if (var && vu->shouldSave(var)) {
            vu->addSavedVar(var);
        }
        recurseVariablesUp(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked) && vu->shouldSave(v)) {
                vu->addSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Checked || treeWidgetItem->checkState(column) == Qt::PartiallyChecked) && vu->shouldSave(v)) {
                vu->addSavedVar(v);
            }
        }
    } else {
        if (var) {
            vu->removeSavedVar(var);
        }
        recurseVariablesUp(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Unchecked)) {
                vu->removeSavedVar(v);
            }
        }
        list.clear();
        recurseVariablesDown(pItem, list); // This does NOT modify pItem or what it points at
        for (auto& treeWidgetItem : list) {
            TVar* v = treeWidget_variables->variableForRow(vu, treeWidgetItem);
            if (v && (treeWidgetItem->checkState(column) == Qt::Unchecked)) {
                vu->removeSavedVar(v);
            }
        }
    }
    mpVarsMainArea->show();
    mpSourceEditorArea->show();

    mpCurrentVarItem = pItem; //remember what has been clicked to save it

    if (column) {
        mChangingVar = false;
        return;
    }

    if (!var) {
        mpVarsMainArea->checkBox_variable_hidden->setChecked(false);
        clearDocument(mpSourceEditorEdbee); // Var Select
        //check for temp item
        var = treeWidget_variables->newVariableForRow(vu, pItem);
        if (var && var->getValueType() == LUA_TTABLE) {
            mpVarsMainArea->comboBox_variable_value_type->setDisabled(true);
            // index 4 = "table"
            mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        } else {
            mpVarsMainArea->comboBox_variable_value_type->setDisabled(false);
            // index 0 = "Auto-type"
            mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(0);
        }
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(0);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
        mChangingVar = false;
        return;
    }

    const int varType = var->getValueType();
    const int keyType = var->getKeyType();

    switch (keyType) {
        //    case LUA_TNONE: // -1
        //    case LUA_TNIL: // 0
    case LUA_TBOOLEAN: // 1
        // index 5 = "key (boolean)". Without this the combobox would keep
        // whatever the previously selected variable put there and name a key
        // type this variable does not have (#9959).
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(5);
        // a boolean key can be renamed between true and false, but it cannot be
        // recast: saveVar() puts a name that still reads as a boolean back to a
        // boolean key whatever the combobox says
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(false);
        break;
        //    case LUA_TLIGHTUSERDATA: // 2
    case LUA_TNUMBER: // 3
        // index 2 = "index (integer number)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(2);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
        break;
    case LUA_TSTRING: // 4
        // index 1 = "key (string)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(1);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(true);
        break;
    case LUA_TTABLE: // 5
        // index 3 = "table (use \"Add Group\" to create"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(3);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(false);
        break;
    case LUA_TFUNCTION: // 6
        // index 4 = "function (cannot create from GUI)"
        mpVarsMainArea->comboBox_variable_key_type->setCurrentIndex(4);
        mpVarsMainArea->comboBox_variable_key_type->setEnabled(false);
        break;
        //    case LUA_TUSERDATA: // 7
        //    case LUA_TTHREAD: // 8
    }

    switch (varType) {
    case LUA_TNONE:
        [[fallthrough]];
    case LUA_TNIL:
        mpSourceEditorArea->hide();
        break;
    case LUA_TBOOLEAN:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        // index 3 = "boolean"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(3);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TNUMBER:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        // index 2 = "number"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(2);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TSTRING:
        mpSourceEditorArea->show();
        mpSourceEditorEdbee->setEnabled(true);
        // index 1 = "string"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(1);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(true);
        break;
    case LUA_TTABLE:
        mpSourceEditorArea->hide();
        mpSourceEditorEdbee->setEnabled(false);
        // Only allow the type to be changed away from a table if it is empty:
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(!(pItem->childCount() > 0));
        // index 4 = "table"
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(4);
        break;
    case LUA_TFUNCTION:
        mpSourceEditorArea->hide();
        mpSourceEditorEdbee->setEnabled(false);
        mpVarsMainArea->comboBox_variable_value_type->setCurrentIndex(5);
        mpVarsMainArea->comboBox_variable_value_type->setEnabled(false);
        break;
    case LUA_TLIGHTUSERDATA:
        [[fallthrough]];
    case LUA_TUSERDATA:
        [[fallthrough]];
    case LUA_TTHREAD: {
    } // No-op
    }

    mpVarsMainArea->checkBox_variable_hidden->setChecked(vu->isHidden(var));
    mpVarsMainArea->lineEdit_var_name->setText(var->getName());
    clearDocument(mpSourceEditorEdbee, lI->getValue(var));
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable);
    pItem->setToolTip(0, utils::richText(tr("Checked variables will be saved and loaded with your profile.")));
    pItem->setCheckState(0, Qt::Unchecked);
    if (!vu->shouldSave(var)) {
        pItem->setFlags(pItem->flags() & ~(Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable));
        const QString reason = vu->getUnsaveableReason(var);
        pItem->setToolTip(0, reason.isEmpty() ? QString() : utils::richText(reason));
    } else if (vu->isSaved(var)) {
        pItem->setCheckState(0, Qt::Checked);
    }
    refreshVariableRow(pItem);
    mChangingVar = false;
    // Said on selection rather than when the user tries to save: getValue() goes
    // by this same name, so for most of what is refused here the value box filled
    // in above is empty, and without this it reads as a real value.
    if (!lI->writableByName(var)) {
        //: Warning shown in the editor's Variables view for a variable it cannot write back to Lua. %1 is the name the variable is shown under.
        showWarning(tr("\"%1\" cannot be changed here: Mudlet cannot safely change it under the name it is shown with, so anything saved for it could go somewhere else. "
                       "Its value may show up blank for the same reason. A script can still change it.")
                            .arg(var->getName().toHtmlEscaped()));
    }
    fitFormPaneToItsContents();
}

// A label and its control are two grid items, and a grid row only collapses once everything on it is hidden
static void showEditorFormRow(const bool visible, const QList<QWidget*>& row)
{
    for (QWidget* pWidget : row) {
        if (pWidget) {
            pWidget->setVisible(visible);
        }
    }
}

void dlgTriggerEditor::slot_actionSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearActionForm();
        return;
    }

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentActionItem) {
        if (mpCurrentActionItem) {
            saveEditorState(EditorViewType::cmActionView, mpCurrentActionItem->data(0, Qt::UserRole).toInt());
        }
        saveAction();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpActionsMainArea->show();
    mpSourceEditorArea->show();

    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Action Select

    mpActionsMainArea->lineEdit_action_icon->clear();
    mpActionsMainArea->lineEdit_action_name->clear();
    mpActionsMainArea->label_idNumber->clear();
    mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(false);
    mpActionsMainArea->lineEdit_action_button_command_down->clear();
    mpActionsMainArea->lineEdit_action_button_command_up->clear();
    mpActionsMainArea->spinBox_action_bar_columns->clear();
    mpActionsMainArea->plainTextEdit_action_css->clear();
    mpActionsMainArea->comboBox_action_bar_location->setCurrentIndex(0);
    mpActionsMainArea->comboBox_action_bar_orientation->setCurrentIndex(0);
    mpActionsMainArea->comboBox_action_button_rotation->setCurrentIndex(0);
    mpActionsMainArea->spinBox_action_bar_columns->setValue(1);
    mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->setMaximum(0);
    mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->setEnabled(false);
    mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->setValue(0);

    mpCurrentActionItem = pItem; //remember what has been clicked to save it
    // ID will be 0 for the root of the treewidget and it is not appropriate
    // to show any right hand side details - pT will also be nullptr!
    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(ID);
    if (pT) {
        mpActionsMainArea->lineEdit_action_name->setText(pT->getName());
        mpActionsMainArea->label_idNumber->setText(QString::number(ID));
        mpActionsMainArea->checkBox_action_button_isPushDown->setChecked(pT->isPushDownButton());
        mpActionsMainArea->lineEdit_action_icon->setText(pT->getIcon());
        mpActionsMainArea->lineEdit_action_button_command_down->setText(pT->getCommandButtonDown());
        mpActionsMainArea->lineEdit_action_button_command_up->setText(pT->getCommandButtonUp());
        mpActionsMainArea->comboBox_action_button_rotation->setCurrentIndex(pT->getButtonRotation());

        clearDocument(mpSourceEditorEdbee, pT->getScript());
        restoreEditorState(EditorViewType::cmActionView, ID);

        // location = 1 = location = bottom is no longer supported
        int location = pT->mLocation;
        if (location > 0) {
            location--;
        }
        mpActionsMainArea->comboBox_action_bar_location->setCurrentIndex(location);
        mpActionsMainArea->comboBox_action_bar_orientation->setCurrentIndex(pT->mOrientation);
        mpActionsMainArea->comboBox_action_button_rotation->setCurrentIndex(pT->getButtonRotation());
        mpActionsMainArea->spinBox_action_bar_columns->setValue(pT->getButtonColumns());
        mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->setValue(pT->getButtonFillerOffset());
        mpActionsMainArea->plainTextEdit_action_css->setPlainText(pT->css);
        // A folder carrying a package name is a module's master folder:
        // Tree<T>::mModuleName is NEVER used, Tree<T>::mPackageName is for both
        const bool moduleMasterFolder = pT->isFolder() && !pT->mPackageName.isEmpty();
        const bool toolbar = pT->isFolder() && !moduleMasterFolder && (!pT->getParent() || !pT->getParent()->mPackageName.isEmpty());
        const bool menu = pT->isFolder() && !moduleMasterFolder && !toolbar;
        const bool button = !pT->isFolder();

        if (mpWidget_actionHeadRow) {
            mpWidget_actionHeadRow->setVisible(!moduleMasterFolder);
        }
        mpSourceEditorArea->setVisible(!moduleMasterFolder);

        showEditorFormRow(menu || button, {mpActionsMainArea->label_action_button_rotation, mpWidget_actionRotationRow});
        // Only for a button: a menu is drawn like one but is never pressed down
        mpActionsMainArea->checkBox_action_button_isPushDown->setVisible(button);
        showEditorFormRow(button, {mpActionsMainArea->label_action_button_command_down, mpActionsMainArea->lineEdit_action_button_command_down});
        showEditorFormRow(button && pT->isPushDownButton(), {mpActionsMainArea->label_action_button_command_up, mpActionsMainArea->lineEdit_action_button_command_up});

        showEditorFormRow(toolbar, {mpActionsMainArea->label_action_bar_location, mpActionsMainArea->comboBox_action_bar_location});
        showEditorFormRow(toolbar, {mpActionsMainArea->label_action_bar_orientation, mpActionsMainArea->comboBox_action_bar_orientation});
        showEditorFormRow(toolbar, {mpActionsMainArea->label_action_bar_columns, mpActionsMainArea->spinBox_action_bar_columns});
        // No first-button offset unless the bar is laid out in lines, the same
        // test dlgActionMainArea::slot_setMaximumValueForOffset() applies
        showEditorFormRow(toolbar && mpActionsMainArea->spinBox_action_bar_columns->value() > 0,
                          {mpActionsMainArea->label_action_bar_offsetToFirstButton, mpActionsMainArea->spinBox_action_bar_offsetToFirstButton});

        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);
    } else {
        // On root of treewidget_actions: - show help message instead
        clearActionForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

void dlgTriggerEditor::slot_treeSelectionChanged()
{
    auto* sender = qobject_cast<TTreeWidget*>(QObject::sender());
    if (sender) {
        QTreeWidgetItem* item = sender->currentItem();
        if (!item) {
            QList<QTreeWidgetItem*> items = sender->selectedItems();
            if (items.empty()) {
                return;
            }
            item = items.first();
        }

        if (item) {
            if (sender == treeWidget_scripts) {
                slot_scriptsSelected(item);
            } else if (sender == treeWidget_keys) {
                slot_keySelected(item);
            } else if (sender == treeWidget_timers) {
                slot_timerSelected(item);
            } else if (sender == treeWidget_aliases) {
                slot_aliasSelected(item);
            } else if (sender == treeWidget_actions) {
                slot_actionSelected(item);
            } else if (sender == treeWidget_variables) {
                slot_variableSelected(item);
            } else if (sender == treeWidget_triggers) {
                slot_triggerSelected(item);
            }
        }
    }
}


static QList<uiDesign::Suggestion> eventSuggestions(const QList<eventNames::Entry>& known)
{
    QList<uiDesign::Suggestion> suggestions;
    suggestions.reserve(known.size());
    for (const eventNames::Entry& entry : known) {
        QString note;
        QString origin;
        switch (entry.source) {
        case eventNames::Source::Mudlet:
            //: Word beside an event name the script editor offers, saying Mudlet itself raises it
            note = dlgTriggerEditor::tr("Mudlet");
            //: Tooltip on one event in a script's row of events, saying Mudlet itself raises it
            origin = dlgTriggerEditor::tr("Raised by Mudlet");
            break;
        case eventNames::Source::Game:
            //: Word beside an event name the script editor offers, saying the game sent it
            note = dlgTriggerEditor::tr("game");
            //: Tooltip on one event in a script's row of events, saying the game sends it
            origin = dlgTriggerEditor::tr("Sent by the game");
            break;
        case eventNames::Source::Script:
            //: Word beside an event name the script editor offers, saying a script in this profile raises or listens for it
            note = dlgTriggerEditor::tr("script");
            //: Tooltip on one event in a script's row of events, saying a script in this profile raises or listens for it
            origin = dlgTriggerEditor::tr("Raised or listened for by a script in this profile");
            break;
        }
        suggestions.append({entry.name, note, origin, entry.source == eventNames::Source::Mudlet});
    }
    return suggestions;
}

void dlgTriggerEditor::slot_scriptsSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearScriptForm();
        return;
    }

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(ID);

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentScriptItem) {
        if (mpCurrentScriptItem) {
            saveEditorState(EditorViewType::cmScriptView, mpCurrentScriptItem->data(0, Qt::UserRole).toInt());
        }
        saveScript();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpCurrentScriptItem = pItem;
    mpScriptsMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Script Select
    mpScriptsMainArea->lineEdit_script_name->clear();
    mpScriptsMainArea->label_idNumber->clear();
    mpChipRow_scriptEvents->setItems({});

    if (pT) {
        const QString name = pT->getName();
        mpChipRow_scriptEvents->setItems(pT->getEventHandlerList());
        // Only what the chips need to show where their names come from; the field gathers the rest as it opens
        mpChipRow_scriptEvents->setSuggestions(eventSuggestions(Host::eventNameEntries(pT->getEventHandlerList())));
        const QString script = pT->getScript();
        clearDocument(mpSourceEditorEdbee, script);
        restoreEditorState(EditorViewType::cmScriptView, ID);

        mpScriptsMainArea->lineEdit_script_name->setText(name);
        mpScriptsMainArea->label_idNumber->setText(QString::number(ID));
        // A body that failed as the profile loaded is still failing: only a compile that succeeds clears it
        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);

    } else {
        // No details to show - as will be the case if the top item (ID = 0) is
        // selected - so show the help message:
        clearScriptForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

void dlgTriggerEditor::slot_timerSelected(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        // No details to show - so show the help message:
        clearTimerForm();
        return;
    }

    // Only save previous item if switching to a different item
    if (pItem != mpCurrentTimerItem) {
        if (mpCurrentTimerItem) {
            saveEditorState(EditorViewType::cmTimerView, mpCurrentTimerItem->data(0, Qt::UserRole).toInt());
        }
        saveTimer();
    }

    // Disable updates during document loading to prevent visual flicker
    mpSourceEditorEdbee->setUpdatesEnabled(false);

    // Block property saves while loading the new item to prevent spurious undo entries
    mBlockPropertySave = true;

    mpCurrentTimerItem = pItem;
    mpTimersMainArea->show();
    mpSourceEditorArea->show();
    clearEditorNotification();
    clearDocument(mpSourceEditorEdbee); // Timer Select

    mpTimersMainArea->lineEdit_timer_command->clear();
    mpTimersMainArea->timeEdit_timer_hours->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_minutes->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_seconds->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->timeEdit_timer_msecs->setTime(QTime(0, 0, 0, 0));
    mpTimersMainArea->label_idNumber->clear();
    // mpTimersMainArea->lineEdit_timer_name->setText(pItem->text(0));

    const int ID = pItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(ID);
    if (pT) {
        const QString command = pT->getCommand();
        const QString name = pT->getName();
        mpTimersMainArea->lineEdit_timer_command->setText(command);
        mpTimersMainArea->lineEdit_timer_name->setText(name);
        mpTimersMainArea->label_idNumber->setText(QString::number(ID));
        showTimerIntervalSentence(pT->isOffsetTimer());
        const QTime time = pT->getTime();
        mpTimersMainArea->timeEdit_timer_hours->setTime(QTime(time.hour(), 0, 0, 0));
        mpTimersMainArea->timeEdit_timer_minutes->setTime(QTime(0, time.minute(), 0, 0));
        mpTimersMainArea->timeEdit_timer_seconds->setTime(QTime(0, 0, time.second(), 0));
        mpTimersMainArea->timeEdit_timer_msecs->setTime(QTime(0, 0, 0, time.msec()));

        clearDocument(mpSourceEditorEdbee, pT->getScript());
        restoreEditorState(EditorViewType::cmTimerView, ID);

        if (!pT->state()) {
            reportCompileError(pT->getError());
        }
        showPackageWarning(pT->packageName(pT), pItem);
    } else {
        clearTimerForm();
        // Re-enable updates since restoreEditorState won't be called
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    }

    mBlockPropertySave = false;
    fitFormPaneToItsContents();
}

void dlgTriggerEditor::fillout_form()
{
    mCurrentView = EditorViewType::cmUnknownView;
    mpCurrentTriggerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentKeyItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentVarItem = nullptr;

    mNeedUpdateData = false;
    mCleanResetDeferred = false;
    // The trees are painted afresh below, which covers every toggle still queued
    mPendingTriggerIconRefresh.clear();
    mPendingAliasIconRefresh.clear();
    mPendingTimerIconRefresh.clear();
    mPendingScriptIconRefresh.clear();
    mPendingKeyIconRefresh.clear();
    mpTriggerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Triggers")));
    treeWidget_triggers->insertTopLevelItem(0, mpTriggerBaseItem);
    populateTriggers();
    mpTriggerBaseItem->setExpanded(true);
    treeWidget_triggers->setCurrentItem(mpTriggerBaseItem);

    mpTimerBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Timers")));
    treeWidget_timers->insertTopLevelItem(0, mpTimerBaseItem);
    populateTimers();
    mpTimerBaseItem->setExpanded(true);
    treeWidget_timers->setCurrentItem(mpTimerBaseItem);

    mpScriptsBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Scripts")));
    treeWidget_scripts->insertTopLevelItem(0, mpScriptsBaseItem);
    populateScripts();
    mpScriptsBaseItem->setExpanded(true);
    treeWidget_scripts->setCurrentItem(mpScriptsBaseItem);

    mpAliasBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Aliases - Input Triggers")));
    treeWidget_aliases->insertTopLevelItem(0, mpAliasBaseItem);
    populateAliases();
    mpAliasBaseItem->setExpanded(true);
    treeWidget_aliases->setCurrentItem(mpAliasBaseItem);

    mpActionBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Buttons")));
    treeWidget_actions->insertTopLevelItem(0, mpActionBaseItem);
    populateActions();
    mpActionBaseItem->setExpanded(true);
    treeWidget_actions->setCurrentItem(mpActionBaseItem);

    mpKeyBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Key Bindings")));
    treeWidget_keys->insertTopLevelItem(0, mpKeyBaseItem);
    populateKeys();
    mpKeyBaseItem->setExpanded(true);
    treeWidget_keys->setCurrentItem(mpKeyBaseItem);

    // The headings carry the sidebar's palette-tinted glyphs, so they are not set where the rows are made
    restyleEditorTreeHeadingIcons();
    for (const auto& [pTree, expandedIds] : std::as_const(mExpandedIdsBeforeRefill).asKeyValueRange()) {
        if (expandedIds.isEmpty()) {
            continue;
        }
        const bool animated = pTree->isAnimated();
        pTree->setAnimated(false);
        restoreExpansionState(pTree->topLevelItem(0), expandedIds);
        pTree->setAnimated(animated);
    }
    mExpandedIdsBeforeRefill.clear();

    // Clear undo stack after initial profile loading (only on first call)
    // Only user actions after this point should be undo-able
    if (mpUndoStack && !mInitialLoadDone) {
        mpUndoStack->clear();

        mInitialLoadDone = true;
    }
}

void dlgTriggerEditor::populateKeys()
{
    std::list<TKey*> const baseNodeList_key = mpHost->getKeyUnit()->getKeyRootNodeList();
    for (auto key : baseNodeList_key) {
        if (key->isTemporary()) {
            continue;
        }

        const QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpKeyBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(key->getID()));
        mpKeyBaseItem->addChild(pItem);
        if (key->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();
            computeKeyDescription(key, itemDescription);
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::computeKeyDescription(TKey* pT, QString& itemDescription) const
{
    const bool itemActive = pT->shownActive(mpHost->mEmergencyStop);
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (itemActive && !pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else if (itemActive) {
        itemDescription = descActive;
        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        itemDescription = descInactive;
    }
}

// Repaints keyID's tree item (and, if any, its descendants) from its current
// active state - used when the state changed via the Lua
// enableKey()/disableKey() API rather than the GUI toggle. Only touches the
// shared editor-wide notification banner when keyID is the item currently
// open in the detail pane *and* the Keys view is the one on screen -
// mpCurrentKeyItem persists across tab switches, so without the view check a
// background state change could clobber a diagnostic in a different view.
void dlgTriggerEditor::refreshKeyIcon(int keyID)
{
    // See refreshTriggerIcon().
    mPendingKeyIconRefresh.insert(keyID);
    if (isVisible() && !mKeyIconRefreshQueued) {
        mKeyIconRefreshQueued = true;
        QTimer::singleShot(0ms, this, &dlgTriggerEditor::flushPendingKeyIconRefresh);
    }
}

void dlgTriggerEditor::flushPendingKeyIconRefresh()
{
    mKeyIconRefreshQueued = false;
    if (mPendingKeyIconRefresh.isEmpty()) {
        return;
    }
    if (!mpKeyBaseItem || !isVisible()) {
        // See flushPendingTriggerIconRefresh() - leave the IDs queued rather
        // than dropping the update.
        return;
    }
    ++mKeyIconFlushCount;
    int remaining = mPendingKeyIconRefresh.size();
    refreshKeyIconsIn(mpKeyBaseItem, false, false, remaining);
    mPendingKeyIconRefresh.clear();
}

// See refreshTriggerIconsIn() for the shape of this walk, including what
// ancestorTouchNotification carries forward.
void dlgTriggerEditor::refreshKeyIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining)
{
    for (int i = 0, n = pParent->childCount(); i < n; ++i) {
        QTreeWidgetItem* pItem = pParent->child(i);
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const bool pending = mPendingKeyIconRefresh.contains(id);
        if (pending) {
            --remaining;
        }
        const bool dirty = ancestorDirty || pending;
        const bool touchNotification = pending ? ((pItem == mpCurrentKeyItem) && (mCurrentView == EditorViewType::cmKeysView) && mFolderGroupKey.isEmpty()) : ancestorTouchNotification;
        if (dirty) {
            if (TKey* pT = mpHost->getKeyUnit()->getKey(id)) {
                paintKeyItem(pItem, pT, touchNotification);
            }
        }
        if (pItem->childCount() > 0 && (dirty || remaining > 0)) {
            refreshKeyIconsIn(pItem, dirty, dirty ? touchNotification : false, remaining);
        }
        if (!ancestorDirty && remaining <= 0) {
            return;
        }
    }
}

void dlgTriggerEditor::paintKeyItem(QTreeWidgetItem* pItem, TKey* pT, bool touchNotification)
{
    QString itemDescription;
    if (pT->state()) {
        if (touchNotification) {
            clearEditorNotification();
        }
        computeKeyDescription(pT, itemDescription);
    } else {
        itemDescription = descError;
        // The heading describes the open item's own code; a child painted along with its folder keeps its error to its row
        if (touchNotification && pItem == mpCurrentKeyItem) {
            reportCompileError(pT->getError());
        }
    }
    if (setRowDescription(pItem, itemDescription)) {
        ++mKeyIconPaintCount;
    }
}

void dlgTriggerEditor::populateActions()
{
    std::list<TAction*> const baseNodeList_action = mpHost->getActionUnit()->getActionRootNodeList();
    for (auto action : baseNodeList_action) {
        if (action->isTemporary()) {
            continue;
        }

        const QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpActionBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(action->getID()));
        mpActionBaseItem->addChild(pItem);
        if (action->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();
            const bool itemActive = action->isActive();
            if (action->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (itemActive && !action->ancestorsActive()) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                if (itemActive) {
                    itemDescription = descActive;
                    if (!action->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::populateAliases()
{
    std::list<TAlias*> const baseNodeList_alias = mpHost->getAliasUnit()->getAliasRootNodeList();
    for (auto alias : baseNodeList_alias) {
        if (alias->isTemporary()) {
            continue;
        }

        const QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpAliasBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(alias->getID()));
        mpAliasBaseItem->addChild(pItem);
        if (alias->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        const bool itemActive = alias->shownActive(mpHost->mEmergencyStop);
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
                if (itemActive && !alias->ancestorsShownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descInactiveParent.arg(itemDescription);
                }
            } else {
                if (alias->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!alias->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::populateScripts()
{
    std::list<TScript*> const baseNodeList_scripts = mpHost->getScriptUnit()->getScriptRootNodeList();
    for (auto script : baseNodeList_scripts) {
        const QString s = script->getName();

        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpScriptsBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(script->getID()));
        mpScriptsBaseItem->addChild(pItem);
        if (script->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();
            computeScriptDescription(script, itemDescription);
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::computeScriptDescription(TScript* pT, QString& itemDescription) const
{
    const bool itemActive = pT->isActive();
    if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        if (pT->mPackageName.isEmpty() && itemActive && !pT->ancestorsActive()) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else if (itemActive) {
        itemDescription = descActive;
        if (!pT->ancestorsActive()) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        itemDescription = descInactive;
    }
}

// Repaints scriptID's tree item (and, if any, its descendants) from its
// current isActive() state - used when the state changed via the Lua
// enableScript()/disableScript() API rather than the GUI toggle. Only
// touches the shared editor-wide notification banner when scriptID is the
// item currently open in the detail pane *and* the Scripts view is the one
// on screen - mpCurrentScriptItem persists across tab switches, so without
// the view check a background state change could clobber a diagnostic in a
// different view.
void dlgTriggerEditor::refreshScriptIcon(int scriptID)
{
    // See refreshTriggerIcon().
    mPendingScriptIconRefresh.insert(scriptID);
    if (isVisible() && !mScriptIconRefreshQueued) {
        mScriptIconRefreshQueued = true;
        QTimer::singleShot(0ms, this, &dlgTriggerEditor::flushPendingScriptIconRefresh);
    }
}

void dlgTriggerEditor::flushPendingScriptIconRefresh()
{
    mScriptIconRefreshQueued = false;
    if (mPendingScriptIconRefresh.isEmpty()) {
        return;
    }
    if (!mpScriptsBaseItem || !isVisible()) {
        // See flushPendingTriggerIconRefresh() - leave the IDs queued rather
        // than dropping the update.
        return;
    }
    ++mScriptIconFlushCount;
    int remaining = mPendingScriptIconRefresh.size();
    refreshScriptIconsIn(mpScriptsBaseItem, false, false, remaining);
    mPendingScriptIconRefresh.clear();
}

// See refreshTriggerIconsIn() for the shape of this walk, including what
// ancestorTouchNotification carries forward.
void dlgTriggerEditor::refreshScriptIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining)
{
    for (int i = 0, n = pParent->childCount(); i < n; ++i) {
        QTreeWidgetItem* pItem = pParent->child(i);
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const bool pending = mPendingScriptIconRefresh.contains(id);
        if (pending) {
            --remaining;
        }
        const bool dirty = ancestorDirty || pending;
        const bool touchNotification = pending ? ((pItem == mpCurrentScriptItem) && (mCurrentView == EditorViewType::cmScriptView) && mFolderGroupKey.isEmpty()) : ancestorTouchNotification;
        if (dirty) {
            if (TScript* pT = mpHost->getScriptUnit()->getScript(id)) {
                paintScriptItem(pItem, pT, touchNotification);
            }
        }
        if (pItem->childCount() > 0 && (dirty || remaining > 0)) {
            refreshScriptIconsIn(pItem, dirty, dirty ? touchNotification : false, remaining);
        }
        if (!ancestorDirty && remaining <= 0) {
            return;
        }
    }
}

void dlgTriggerEditor::paintScriptItem(QTreeWidgetItem* pItem, TScript* pT, bool touchNotification)
{
    QString itemDescription;
    if (pT->state()) {
        if (touchNotification) {
            clearEditorNotification();
        }
        computeScriptDescription(pT, itemDescription);
    } else {
        itemDescription = descError;
        // The heading describes the open item's own code; a child painted along with its folder keeps its error to its row
        if (touchNotification && pItem == mpCurrentScriptItem) {
            reportCompileError(pT->getError());
        }
    }
    if (setRowDescription(pItem, itemDescription)) {
        ++mScriptIconPaintCount;
    }
}

void dlgTriggerEditor::populateTimers()
{
    std::list<TTimer*> const baseNodeList_timers = mpHost->getTimerUnit()->getTimerRootNodeList();
    for (auto timer : baseNodeList_timers) {
        if (timer->isTemporary()) {
            continue;
        }
        const QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTimerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(timer->getID()));
        mpTimerBaseItem->addChild(pItem);
        if (timer->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();
            computeTimerDescription(timer, itemDescription);
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::computeTimerDescription(TTimer* pT, QString& itemDescription) const
{
    if (pT->isFolder()) {
        itemDescription = (pT->shownActive(mpHost->mEmergencyStop) ? descActiveFolder : descInactiveFolder);
        if (pT->mPackageName.isEmpty() && pT->shouldBeActive() && !pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else if (pT->isOffsetTimer()) {
        if (pT->shouldBeActive()) {
            itemDescription = descActiveOffsetTimer;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveOffsetTimer;
        }
    } else if (pT->shouldBeActive()) {
        itemDescription = descActive;
        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        itemDescription = descInactive;
    }
}

// Repaints timerID's tree item (and, if any, its descendants) from its
// current active state - used when the state changed via the Lua
// enableTimer()/disableTimer() API rather than the GUI toggle. Only touches
// the shared editor-wide notification banner when timerID is the item
// currently open in the detail pane *and* the Timers view is the one on
// screen - mpCurrentTimerItem persists across tab switches, so without the
// view check a background state change could clobber a diagnostic in a
// different view.
void dlgTriggerEditor::refreshTimerIcon(int timerID)
{
    // See refreshTriggerIcon().
    mPendingTimerIconRefresh.insert(timerID);
    if (isVisible() && !mTimerIconRefreshQueued) {
        mTimerIconRefreshQueued = true;
        QTimer::singleShot(0ms, this, &dlgTriggerEditor::flushPendingTimerIconRefresh);
    }
}

void dlgTriggerEditor::flushPendingTimerIconRefresh()
{
    mTimerIconRefreshQueued = false;
    if (mPendingTimerIconRefresh.isEmpty()) {
        return;
    }
    if (!mpTimerBaseItem || !isVisible()) {
        // See flushPendingTriggerIconRefresh() - leave the IDs queued rather
        // than dropping the update.
        return;
    }
    ++mTimerIconFlushCount;
    int remaining = mPendingTimerIconRefresh.size();
    refreshTimerIconsIn(mpTimerBaseItem, false, false, remaining);
    mPendingTimerIconRefresh.clear();
}

// See refreshTriggerIconsIn() for the shape of this walk, including what
// ancestorTouchNotification carries forward.
void dlgTriggerEditor::refreshTimerIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining)
{
    for (int i = 0, n = pParent->childCount(); i < n; ++i) {
        QTreeWidgetItem* pItem = pParent->child(i);
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const bool pending = mPendingTimerIconRefresh.contains(id);
        if (pending) {
            --remaining;
        }
        const bool dirty = ancestorDirty || pending;
        const bool touchNotification = pending ? ((pItem == mpCurrentTimerItem) && (mCurrentView == EditorViewType::cmTimerView) && mFolderGroupKey.isEmpty()) : ancestorTouchNotification;
        if (dirty) {
            if (TTimer* pT = mpHost->getTimerUnit()->getTimer(id)) {
                paintTimerItem(pItem, pT, touchNotification);
            }
        }
        if (pItem->childCount() > 0 && (dirty || remaining > 0)) {
            refreshTimerIconsIn(pItem, dirty, dirty ? touchNotification : false, remaining);
        }
        if (!ancestorDirty && remaining <= 0) {
            return;
        }
    }
}

void dlgTriggerEditor::paintTimerItem(QTreeWidgetItem* pItem, TTimer* pT, bool touchNotification)
{
    QString itemDescription;
    if (pT->state()) {
        if (touchNotification) {
            clearEditorNotification();
        }
        computeTimerDescription(pT, itemDescription);
    } else {
        itemDescription = descError;
        // The heading describes the open item's own code; a child painted along with its folder keeps its error to its row
        if (touchNotification && pItem == mpCurrentTimerItem) {
            reportCompileError(pT->getError());
        }
    }
    if (setRowDescription(pItem, itemDescription)) {
        ++mTimerIconPaintCount;
    }
}

void dlgTriggerEditor::populateTriggers()
{
    std::list<TTrigger*> const baseNodeList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
    for (auto trigger : baseNodeList) {
        if (trigger->isTemporary()) {
            continue;
        }
        const QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(mpTriggerBaseItem, sList);
        pItem->setData(0, Qt::UserRole, QVariant(trigger->getID()));
        mpTriggerBaseItem->addChild(pItem);
        if (trigger->mModuleMasterFolder) {
            pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
        }
        QString itemDescription;
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();
            computeTriggerDescription(trigger, itemDescription);
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::computeTriggerDescription(TTrigger* pT, QString& itemDescription) const
{
    const bool itemActive = pT->shownActive(mpHost->mEmergencyStop);
    if (pT->isFilterChain()) {
        if (itemActive) {
            itemDescription = descActiveFilterChain;
            if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
                itemDescription = descInactiveParent.arg(itemDescription);
            }
        } else {
            itemDescription = descInactiveFilterChain;
        }
    } else if (pT->isFolder()) {
        itemDescription = (itemActive ? descActiveFolder : descInactiveFolder);
        // A folder carrying a package name is a package's master folder, which
        // has no parent to be held inactive by
        if (pT->mPackageName.isEmpty() && itemActive && !pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else if (itemActive) {
        itemDescription = descActive;
        if (!pT->ancestorsShownActive(mpHost->mEmergencyStop)) {
            itemDescription = descInactiveParent.arg(itemDescription);
        }
    } else {
        itemDescription = descInactive;
    }
}

// Repaints triggerID's tree item (and, if any, its descendants) from its
// current active state - used when the state changed via the Lua
// enableTrigger()/disableTrigger() API rather than the GUI toggle, which
// otherwise leaves the tree icon stale until the next full repopulation.
// Only touches the shared editor-wide notification banner when triggerID is
// the item currently open in the detail pane *and* the Triggers view is the
// one on screen - mpCurrentTriggerItem persists across tab switches, so
// without the view check a background state change could clobber whatever
// diagnostic the user is actually looking at in a different view.
void dlgTriggerEditor::refreshTriggerIcon(int triggerID)
{
    // One tree walk per turn, not a findItemByID() per call; showEvent() flushes what
    // arrives while closed. The units skip top-level temporary items, which the trees leave out.
    mPendingTriggerIconRefresh.insert(triggerID);
    if (isVisible() && !mTriggerIconRefreshQueued) {
        mTriggerIconRefreshQueued = true;
        QTimer::singleShot(0ms, this, &dlgTriggerEditor::flushPendingTriggerIconRefresh);
    }
}

void dlgTriggerEditor::flushPendingTriggerIconRefresh()
{
    mTriggerIconRefreshQueued = false;
    if (mPendingTriggerIconRefresh.isEmpty()) {
        return;
    }
    if (!mpTriggerBaseItem || !isVisible()) {
        // Editor closed before this flush ran - leave the IDs queued for
        // showEvent() rather than silently dropping the update.
        return;
    }
    int remaining = mPendingTriggerIconRefresh.size();
    refreshTriggerIconsIn(mpTriggerBaseItem, false, false, remaining);
    mPendingTriggerIconRefresh.clear();
}

// One pass over the tree: repaints every pending item and, since a folder's
// state changes its descendants' greyed-out look, everything under one. Stops
// as soon as every pending ID has been seen.
//
// ancestorTouchNotification carries the notification-eligibility of whichever
// pending item most recently rooted the current dirty subtree - matching the
// pre-coalescing behaviour where one refreshTriggerIcon(id) call decided
// touchNotification once, from id's own selection state, and applied it to
// that id's whole subtree. A descendant swept in only because an ancestor
// folder toggled must not touch the notification banner on its own account,
// even if that descendant happens to be the selected item.
void dlgTriggerEditor::refreshTriggerIconsIn(QTreeWidgetItem* pParent, bool ancestorDirty, bool ancestorTouchNotification, int& remaining)
{
    for (int i = 0, n = pParent->childCount(); i < n; ++i) {
        QTreeWidgetItem* pItem = pParent->child(i);
        const int id = pItem->data(0, Qt::UserRole).toInt();
        const bool pending = mPendingTriggerIconRefresh.contains(id);
        if (pending) {
            --remaining;
        }
        const bool dirty = ancestorDirty || pending;
        const bool touchNotification = pending ? ((pItem == mpCurrentTriggerItem) && (mCurrentView == EditorViewType::cmTriggerView) && mFolderGroupKey.isEmpty()) : ancestorTouchNotification;
        if (dirty) {
            if (TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(id)) {
                paintTriggerItem(pItem, pT, touchNotification);
            }
        }
        if (pItem->childCount() > 0 && (dirty || remaining > 0)) {
            refreshTriggerIconsIn(pItem, dirty, dirty ? touchNotification : false, remaining);
        }
        if (!ancestorDirty && remaining <= 0) {
            return;
        }
    }
}

void dlgTriggerEditor::paintTriggerItem(QTreeWidgetItem* pItem, TTrigger* pT, bool touchNotification)
{
    QString itemDescription;
    if (pT->state()) {
        if (touchNotification) {
            clearEditorNotification();
        }
        computeTriggerDescription(pT, itemDescription);
    } else {
        itemDescription = descError;
        // The heading describes the open item's own code; a child painted along with its folder keeps its error to its row
        if (touchNotification && pItem == mpCurrentTriggerItem) {
            reportCompileError(pT->getError());
        }
    }
    setRowDescription(pItem, itemDescription);
}

void dlgTriggerEditor::repopulateVars()
{
    treeWidget_variables->setUpdatesEnabled(false);
    mpVarBaseItem = new QTreeWidgetItem(QStringList(tr("Variables")));
    mpVarBaseItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    treeWidget_variables->clear();
    treeWidget_variables->clearVariableRows();
    mpCurrentVarItem = nullptr;
    treeWidget_variables->insertTopLevelItem(0, mpVarBaseItem);
    // The heading row is made afresh on every read, so its glyph is set again
    restyleEditorTreeHeadingIcons();
    mpVarBaseItem->setExpanded(true);
    LuaInterface* lI = mpHost->getLuaInterface();
    lI->getVars(false);
    VarUnit* vu = lI->getVarUnit();
    treeWidget_variables->buildVariableRows(vu, mpVarBaseItem, vu->getBase(), showHiddenVars);
    mpVarBaseItem->setExpanded(true);
    treeWidget_variables->setUpdatesEnabled(true);
    treeWidget_variables->setCurrentItem(mpVarBaseItem);
    updateHiddenVariablesCount();
}

// Nothing re-derives a row's drawn data once set, so every path that writes a
// variable back must come through here; painting never reads Lua's tree.
void dlgTriggerEditor::refreshVariableRow(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }
    VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
    TVar* var = treeWidget_variables->variableForRow(vu, pItem);
    if (!var) {
        // ...or the variable the editor has just made, which Lua does not hold yet
        var = treeWidget_variables->newVariableForRow(vu, pItem);
    }
    if (!var) {
        return;
    }
    uiDesign::VariableTreeDelegate::setVariableRowData(pItem, var, vu->isHidden(var));
}

// Counted off Lua's tree, not the rows, which are what is missing while the switch
// is off; only globals, since the switch adds top-level rows.
void dlgTriggerEditor::updateHiddenVariablesCount()
{
    if (!mpLabel_hiddenVariablesCount) {
        return;
    }
    int hiddenCount = 0;
    VarUnit* vu = mpHost->getLuaInterface()->getVarUnit();
    if (TVar* pBase = vu->getBase()) {
        const QList<TVar*> globals = pBase->getChildren(false);
        for (TVar* pGlobal : globals) {
            if (vu->isHidden(pGlobal)) {
                ++hiddenCount;
            }
        }
    }
    mpLabel_hiddenVariablesCount->setVisible(hiddenCount > 0);
    if (hiddenCount > 0) {
        //: Quiet reading beside the "Show hidden variables" switch over the editor's Variables tree, saying how many variables are hidden right now. %1 is that count.
        mpLabel_hiddenVariablesCount->setText(tr("%1 hidden").arg(hiddenCount));
        //: Accessible name of the "Show hidden variables" switch over the editor's Variables tree, said with how many variables are hidden right now. %1 is that count.
        checkBox_displayAllVariables->setAccessibleName(tr("Show hidden variables (%1)").arg(hiddenCount));
        return;
    }
    mpLabel_hiddenVariablesCount->clear();
    checkBox_displayAllVariables->setAccessibleName(checkBox_displayAllVariables->text());
}

void dlgTriggerEditor::expand_child_triggers(TTrigger* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TTrigger>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* triggerNode : *childrenList) {
        auto* trigger = static_cast<TTrigger*>(triggerNode);
        const QString s = trigger->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, trigger->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (trigger->hasChildren()) {
            expand_child_triggers(trigger, pItem);
        }
        if (trigger->state()) {
            clearEditorNotification();

            if (trigger->isFilterChain()) {
                if (trigger->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFilterChain;
                    if (!trigger->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFilterChain;
                }
            } else if (trigger->isFolder()) {
                if (trigger->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!trigger->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (trigger->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!trigger->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
            //pItem->setDisabled(!trigger->ancestorsActive());
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::expand_child_key(TKey* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TKey>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* keyNode : *childrenList) {
        auto* key = static_cast<TKey*>(keyNode);
        const QString s = key->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, key->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (key->hasChildren()) {
            expand_child_key(key, pItem);
        }
        if (key->state()) {
            clearEditorNotification();

            if (key->isFolder()) {
                if (key->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!key->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (key->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!key->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}


void dlgTriggerEditor::expand_child_scripts(TScript* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TScript>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* scriptNode : *childrenList) {
        auto* script = static_cast<TScript*>(scriptNode);
        const QString s = script->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, script->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (script->hasChildren()) {
            expand_child_scripts(script, pItem);
        }
        if (script->state()) {
            clearEditorNotification();

            if (script->isFolder()) {
                if (script->isActive()) {
                    itemDescription = descActiveFolder;
                    if (!script->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (script->isActive()) {
                    itemDescription = descActive;
                    if (!script->ancestorsActive()) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_alias(TAlias* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TAlias>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* aliasNode : *childrenList) {
        auto* alias = static_cast<TAlias*>(aliasNode);
        const QString s = alias->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, alias->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (alias->hasChildren()) {
            expand_child_alias(alias, pItem);
        }
        if (alias->state()) {
            clearEditorNotification();

            if (alias->isFolder()) {
                if (alias->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActiveFolder;
                    if (!alias->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (alias->shownActive(mpHost->mEmergencyStop)) {
                    itemDescription = descActive;
                    if (!alias->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

void dlgTriggerEditor::expand_child_action(TAction* pTriggerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TAction>*>* childrenList = pTriggerParent->getChildrenList();
    for (auto* actionNode : *childrenList) {
        auto* action = static_cast<TAction*>(actionNode);
        const QString s = action->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, action->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (action->hasChildren()) {
            expand_child_action(action, pItem);
        }
        if (action->state()) {
            clearEditorNotification();

            if (!action->getParent()->mPackageName.isEmpty()) {
                // Must have a parent (or would not be IN this method) and the
                // parent has a package name - this is a toolbar
                if (action->isActive()) {
                    itemDescription = descActiveFolder;
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else if (action->isFolder()) {
                // Is a folder and is not a toolbar - this is a menu
                if (action->isActive()) {
                    itemDescription = descActiveFolder;
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                // Is a button
                if (action->isActive()) {
                    itemDescription = descActive;
                } else {
                    itemDescription = descInactive;
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, itemDescription);
    }
}

void dlgTriggerEditor::expand_child_timers(TTimer* pTimerParent, QTreeWidgetItem* pWidgetItemParent)
{
    std::list<Tree<TTimer>*>* childrenList = pTimerParent->getChildrenList();
    for (auto* timerNode : *childrenList) {
        auto* timer = static_cast<TTimer*>(timerNode);
        const QString s = timer->getName();
        QStringList sList;
        sList << s;
        auto pItem = new QTreeWidgetItem(pWidgetItemParent, sList);
        pItem->setData(0, Qt::UserRole, timer->getID());

        pWidgetItemParent->insertChild(0, pItem);
        QString itemDescription;
        if (timer->hasChildren()) {
            expand_child_timers(timer, pItem);
        }
        if (timer->state()) {
            clearEditorNotification();

            if (timer->isFolder()) {
                if (timer->shouldBeActive()) {
                    if (timer->ancestorsShownActive(mpHost->mEmergencyStop)) {
                        itemDescription = descActiveFolder;
                    } else {
                        itemDescription = descInactiveParent.arg(itemDescription);
                    }
                } else {
                    itemDescription = descInactiveFolder;
                }
            } else {
                if (timer->isOffsetTimer()) {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActiveOffsetTimer;
                        if (!timer->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactiveOffsetTimer;
                    }
                } else {
                    if (timer->shouldBeActive()) {
                        itemDescription = descActive;
                        if (!timer->ancestorsShownActive(mpHost->mEmergencyStop)) {
                            itemDescription = descInactiveParent.arg(itemDescription);
                        }
                    } else {
                        itemDescription = descInactive;
                    }
                }
            }
        } else {
            itemDescription = descError;
        }
        pItem->setData(0, Qt::AccessibleDescriptionRole, spokenRunState(itemDescription));
    }
}

bool dlgTriggerEditor::saveOpenChanges()
{
    // The item behind a merged folder was saved when the folder was chosen, and its errors must stay hidden
    if (!mFolderGroupKey.isEmpty()) {
        return true;
    }
    beginSaveErrorCapture();

    bool saved = true;
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (mpCurrentTriggerItem) {
            saveEditorState(EditorViewType::cmTriggerView, mpCurrentTriggerItem->data(0, Qt::UserRole).toInt());
        }
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        if (mpCurrentTimerItem) {
            saveEditorState(EditorViewType::cmTimerView, mpCurrentTimerItem->data(0, Qt::UserRole).toInt());
        }
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        if (mpCurrentAliasItem) {
            saveEditorState(EditorViewType::cmAliasView, mpCurrentAliasItem->data(0, Qt::UserRole).toInt());
        }
        saved = saveAlias();
        break;
    case EditorViewType::cmScriptView:
        if (mpCurrentScriptItem) {
            saveEditorState(EditorViewType::cmScriptView, mpCurrentScriptItem->data(0, Qt::UserRole).toInt());
        }
        saveScript();
        break;
    case EditorViewType::cmActionView:
        if (mpCurrentActionItem) {
            saveEditorState(EditorViewType::cmActionView, mpCurrentActionItem->data(0, Qt::UserRole).toInt());
        }
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        if (mpCurrentKeyItem) {
            saveEditorState(EditorViewType::cmKeysView, mpCurrentKeyItem->data(0, Qt::UserRole).toInt());
        }
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    case EditorViewType::cmUnknownView:
        break;
    }

    endSaveErrorCapture();
    return saved;
}

// Helper function to determine the current view from which tree widget is visible with selection.
// This is used as a fallback when mCurrentView is cmUnknownView due to initialization timing issues.
EditorViewType dlgTriggerEditor::determineViewFromVisibleTree()
{
    if (treeWidget_triggers->isVisible() && treeWidget_triggers->currentItem()) {
        return EditorViewType::cmTriggerView;
    }
    if (treeWidget_aliases->isVisible() && treeWidget_aliases->currentItem()) {
        return EditorViewType::cmAliasView;
    }
    if (treeWidget_timers->isVisible() && treeWidget_timers->currentItem()) {
        return EditorViewType::cmTimerView;
    }
    if (treeWidget_scripts->isVisible() && treeWidget_scripts->currentItem()) {
        return EditorViewType::cmScriptView;
    }
    if (treeWidget_actions->isVisible() && treeWidget_actions->currentItem()) {
        return EditorViewType::cmActionView;
    }
    if (treeWidget_keys->isVisible() && treeWidget_keys->currentItem()) {
        return EditorViewType::cmKeysView;
    }
    if (treeWidget_variables->isVisible() && treeWidget_variables->currentItem()) {
        return EditorViewType::cmVarsView;
    }
    return EditorViewType::cmUnknownView;
}

EditorViewType dlgTriggerEditor::resolveCurrentView()
{
    if (mCurrentView != EditorViewType::cmUnknownView) {
        return mCurrentView;
    }
    EditorViewType resolved = determineViewFromVisibleTree();
    if (resolved != EditorViewType::cmUnknownView) {
        mCurrentView = resolved;
    }
    return mCurrentView;
}

void dlgTriggerEditor::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)

    if (isActiveWindow()) {
        autoSave();
    }
}

void dlgTriggerEditor::autoSave()
{
    // A save turned away or failed keeps the last time that did get written
    if (mpHost->saveProfileReportingFailure(qsl("autosave")) && mpLabel_statusAutosave) {
        //: Editor status bar, trailing edge. %1 is a time of day in the player's own format.
        mpLabel_statusAutosave->setText(tr("Autosaved %1").arg(QLocale::system().toString(QTime::currentTime(), QLocale::ShortFormat)));
    }
}

// A refilled tree starts with every folder closed, so fillout_form() opens again the ones recorded here
void dlgTriggerEditor::clearTypeTrees()
{
    for (QTreeWidget* pTree : {treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys}) {
        // An already empty tree must not wipe what an earlier clear recorded and no refill has used yet
        if (QTreeWidgetItem* pRoot = pTree->topLevelItem(0)) {
            mExpandedIdsBeforeRefill.insert(pTree, collectExpandedItemIDs(pRoot));
        }
        pTree->clear();
    }
}

void dlgTriggerEditor::rebuildStaleTrees()
{
    // fillout_form() forgets the view, and the Folders view hides every type tree resolveCurrentView() could recover it from
    const EditorViewType folderViewType = mFolderViewActive ? mCurrentView : EditorViewType::cmUnknownView;
    clearTypeTrees();
    treeWidget_variables->clear();
    // The clear deleted the Variables heading, and fillout_form() restyles every heading still pointed at
    mpVarBaseItem = nullptr;
    if (mCleanResetDeferred) {
        buildDeferredTrees(false);
    } else {
        fillout_form();
    }
    mNeedUpdateData = false;
    if (folderViewType != EditorViewType::cmUnknownView) {
        mCurrentView = folderViewType;
    }
    resolveCurrentView();
}

void dlgTriggerEditor::enterEvent(TEnterEvent* event)
{
    Q_UNUSED(event)
    if (mNeedUpdateData) {
        saveOpenChanges();
        rebuildStaleTrees();
    }
}

void dlgTriggerEditor::focusInEvent(QFocusEvent* pE)
{
    Q_UNUSED(pE)
    if (mNeedUpdateData) {
        saveOpenChanges();
        rebuildStaleTrees();
    }

    if (mCurrentView == EditorViewType::cmUnknownView) {
        mpCurrentTriggerItem = nullptr;
        mpCurrentAliasItem = nullptr;
        mpCurrentKeyItem = nullptr;
        mpCurrentActionItem = nullptr;
        mpCurrentScriptItem = nullptr;
        mpCurrentTimerItem = nullptr;
        return;
    }

    if (mpCurrentTriggerItem) {
        mpCurrentTriggerItem->setSelected(true);
    }
    if (mpCurrentTimerItem) {
        mpCurrentTimerItem->setSelected(true);
    }
    if (mpCurrentAliasItem) {
        mpCurrentAliasItem->setSelected(true);
    }
    if (mpCurrentScriptItem) {
        mpCurrentScriptItem->setSelected(true);
    }
    if (mpCurrentActionItem) {
        mpCurrentActionItem->setSelected(true);
    }
    if (mpCurrentKeyItem) {
        mpCurrentKeyItem->setSelected(true);
    }
}

void dlgTriggerEditor::focusOutEvent(QFocusEvent* pE)
{
    Q_UNUSED(pE)

    saveOpenChanges();
}

void dlgTriggerEditor::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    // A splitter never shown reports the minimum size hint of a layout never run,
    // a few hundred pixels short, so the sidebar breakpoint is measured again here
    if (!mHasBeenShown) {
        mHasBeenShown = true;
        invalidateEditorSidebarWidths();
        updateEditorSidebarMode();
        // The resize that gave the bar its length may already have gone past
        fitEditorToolBarToItsLength();

        // mudlet::slot_showEditorDialog() loads the first item before the window is
        // shown, when the splitter has no height. Asked again on the next turn in
        // case the show has not laid the panes out yet.
        fitFormPaneToItsContents();
        QTimer::singleShot(0ms, this, [this]() {
            fitFormPaneToItsContents();
        });
    }

    if (!windowPlacementReachable(pos(), size()) || (!mEditorPlacementChosen && !onSameScreenAsProfile())) {
        repositionOnProfileScreen();
    }

    if (mCleanResetDeferred) {
        buildDeferredTrees(true);
    }

    // Lua toggles that arrived while this editor was hidden are queued, but
    // nothing flushes them until now.
    flushPendingTriggerIconRefresh();
    flushPendingAliasIconRefresh();
    flushPendingTimerIconRefresh();
    flushPendingScriptIconRefresh();
    flushPendingKeyIconRefresh();
}

void dlgTriggerEditor::moveEvent(QMoveEvent* event)
{
    QMainWindow::moveEvent(event);

    if (isVisible() && !mRepositioningEditorWindow) {
        mEditorPlacementChosen = true;
    }
}

void dlgTriggerEditor::changeView(EditorViewType view)
{
    // Before the save reads the form: hiding the keys form ends a grab through the
    // field's FocusOut, but only after the save has read the field the grab emptied
    endKeyGrab();

    saveOpenChanges();

    if (mNeedUpdateData) {
        rebuildStaleTrees();
    }

    // in lieu of readonly
    mpSourceEditorEdbee->setEnabled(true);

    if (mCurrentView != view) {
        // Clear the current item pointer for the old view so that when we return,
        // the item will be properly reloaded (not considered "same")
        switch (mCurrentView) {
        case EditorViewType::cmTriggerView:
            mpCurrentTriggerItem = nullptr;
            break;
        case EditorViewType::cmTimerView:
            mpCurrentTimerItem = nullptr;
            break;
        case EditorViewType::cmAliasView:
            mpCurrentAliasItem = nullptr;
            break;
        case EditorViewType::cmScriptView:
            mpCurrentScriptItem = nullptr;
            break;
        case EditorViewType::cmActionView:
            mpCurrentActionItem = nullptr;
            break;
        case EditorViewType::cmKeysView:
            mpCurrentKeyItem = nullptr;
            break;
        case EditorViewType::cmVarsView:
        case EditorViewType::cmUnknownView:
            break;
        }
        // Disable updates during view change to prevent visual flicker
        // (selection handler's restoreEditorState will re-enable)
        mpSourceEditorEdbee->setUpdatesEnabled(false);
        clearDocument(mpSourceEditorEdbee); // Change View
    }
    mCurrentView = view;

    const bool bannerUndoToastShowing = mpBannerUndoTimer && mpBannerUndoTimer->isActive();
    cancelBannerUndoTimer();

    // A banner (or the dismissal undo toast) belongs to the view it was shown
    // in, so hide it on a view change - otherwise it lingers over the new view
    // when that view's own banner is suppressed. showIntro() will put up the
    // right banner for the new view if one is allowed. Errors and warnings
    // (which clear mCurrentBannerKey) are not hidden by this block, though the
    // pre-existing permanently-hidden check below still can hide them. Using
    // clearEditorNotification() rather than hideSystemMessageArea() as the
    // latter would also discard the current script's unacknowledged loading
    // error.
    if (bannerUndoToastShowing || !mCurrentBannerKey.isEmpty()) {
        clearEditorNotification();
    }

    if (bannerPermanentlyHidden(mCurrentView)) {
        hideSystemMessageArea();
    }

    if (view == EditorViewType::cmVarsView) {
        leaveFolderView();
    }
    hideFolderGroup();
    const bool showTypeTree = !mFolderViewActive;
    mpFolderTree->setVisible(mFolderViewActive);

    mpActionsMainArea->setVisible(view == EditorViewType::cmActionView);
    treeWidget_actions->setVisible(showTypeTree && view == EditorViewType::cmActionView);

    mpAliasMainArea->setVisible(view == EditorViewType::cmAliasView);
    treeWidget_aliases->setVisible(showTypeTree && view == EditorViewType::cmAliasView);

    mpKeysMainArea->setVisible(view == EditorViewType::cmKeysView);
    treeWidget_keys->setVisible(showTypeTree && view == EditorViewType::cmKeysView);

    mpScriptsMainArea->setVisible(view == EditorViewType::cmScriptView);
    treeWidget_scripts->setVisible(showTypeTree && view == EditorViewType::cmScriptView);

    mpTimersMainArea->setVisible(view == EditorViewType::cmTimerView);
    treeWidget_timers->setVisible(showTypeTree && view == EditorViewType::cmTimerView);

    mpTriggersMainArea->setVisible(view == EditorViewType::cmTriggerView);
    treeWidget_triggers->setVisible(showTypeTree && view == EditorViewType::cmTriggerView);

    const bool enablePatternShortcuts = view == EditorViewType::cmTriggerView;

    if (mFirstPatternShortcut) {
        mFirstPatternShortcut->setEnabled(enablePatternShortcuts);
    }
    for (auto* shortcut : std::as_const(mPatternNavigationShortcuts)) {
        if (shortcut) {
            shortcut->setEnabled(enablePatternShortcuts);
        }
    }
    if (mLastPatternShortcut) {
        mLastPatternShortcut->setEnabled(enablePatternShortcuts);
    }

    mpVarsMainArea->setVisible(view == EditorViewType::cmVarsView);
    treeWidget_variables->setVisible(view == EditorViewType::cmVarsView);
    checkBox_displayAllVariables->setVisible(view == EditorViewType::cmVarsView);
    // Only hidden here: inside the Variables view updateHiddenVariablesCount() decides
    if (mpLabel_hiddenVariablesCount && view != EditorViewType::cmVarsView) {
        mpLabel_hiddenVariablesCount->hide();
    }

    if (mpLabel_editorCodeHeaderTitle) {
        mpLabel_editorCodeHeaderTitle->setText(codeHeaderTitleFor(view));
    }

    mpExportAction->setEnabled(view != EditorViewType::cmVarsView && view != EditorViewType::cmUnknownView);

    // texts are duplicated here so that translators can work with the full string
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        mAddItem->setText(tr("Add Trigger"));
        mAddItem->setStatusTip(tr("Add new trigger"));
        mAddGroup->setText(tr("Add Trigger Group"));
        mAddGroup->setStatusTip(tr("Add new group of triggers"));
        mDeleteItem->setText(tr("Delete Trigger"));
        mDeleteItem->setStatusTip(tr("Delete the selected trigger"));
        mSaveItem->setText(tr("Save Trigger"));
        //: Status tip for saving trigger changes
        mSaveItem->setStatusTip(tr("Apply trigger changes (does not save to disk)."));
        break;
    case EditorViewType::cmTimerView:
        mAddItem->setText(tr("Add Timer"));
        mAddItem->setStatusTip(tr("Add new timer"));
        mAddGroup->setText(tr("Add Timer Group"));
        mAddGroup->setStatusTip(tr("Add new group of timers"));
        mDeleteItem->setText(tr("Delete Timer"));
        mDeleteItem->setStatusTip(tr("Delete the selected timer"));
        mSaveItem->setText(tr("Save Timer"));
        //: Status tip for saving timer changes
        mSaveItem->setStatusTip(tr("Apply timer changes (does not save to disk)."));
        break;
    case EditorViewType::cmAliasView:
        mAddItem->setText(tr("Add Alias"));
        mAddItem->setStatusTip(tr("Add new alias"));
        mAddGroup->setText(tr("Add Alias Group"));
        mAddGroup->setStatusTip(tr("Add new group of aliases"));
        mDeleteItem->setText(tr("Delete Alias"));
        mDeleteItem->setStatusTip(tr("Delete the selected alias"));
        mSaveItem->setText(tr("Save Alias"));
        //: Status tip for saving alias changes
        mSaveItem->setStatusTip(tr("Apply alias changes (does not save to disk)."));
        break;
    case EditorViewType::cmScriptView:
        mAddItem->setText(tr("Add Script"));
        mAddItem->setStatusTip(tr("Add new script"));
        mAddGroup->setText(tr("Add Script Group"));
        mAddGroup->setStatusTip(tr("Add new group of scripts"));
        mDeleteItem->setText(tr("Delete Script"));
        mDeleteItem->setStatusTip(tr("Delete the selected script"));
        mSaveItem->setText(tr("Save Script"));
        //: Status tip for saving script changes
        mSaveItem->setStatusTip(tr("Apply script changes (does not save to disk)."));
        break;
    case EditorViewType::cmActionView:
        mAddItem->setText(tr("Add Button"));
        mAddItem->setStatusTip(tr("Add new button"));
        mAddGroup->setText(tr("Add Toolbar or Menu"));
        mAddGroup->setStatusTip(tr("Add a Toolbar (top level) or Menu (lower levels) to contain menus or buttons"));
        mDeleteItem->setText(tr("Delete Button, Menu or Toolbar"));
        mDeleteItem->setStatusTip(tr("Delete the selected button, menu or toolbar"));
        mSaveItem->setText(tr("Save item"));
        //: Status tip for saving button changes
        mSaveItem->setStatusTip(tr("Apply button/menu/toolbar changes (does not save to disk)."));
        break;
    case EditorViewType::cmKeysView:
        mAddItem->setText(tr("Add Key"));
        mAddItem->setStatusTip(tr("Add new key"));
        mAddGroup->setText(tr("Add Key Group"));
        mAddGroup->setStatusTip(tr("Add new group of keys"));
        mDeleteItem->setText(tr("Delete Key"));
        mDeleteItem->setStatusTip(tr("Delete the selected key"));
        mSaveItem->setText(tr("Save Key"));
        //: Status tip for saving key changes
        mSaveItem->setStatusTip(tr("Apply key changes (does not save to disk)."));
        break;
    case EditorViewType::cmVarsView:
        mAddItem->setText(tr("Add Variable"));
        mAddItem->setStatusTip(tr("Add new variable"));
        mAddGroup->setText(tr("Add Lua table"));
        mAddGroup->setStatusTip(tr("Add new Lua table"));
        mDeleteItem->setText(tr("Delete Variable"));
        mDeleteItem->setStatusTip(tr("Delete the selected variable"));
        mSaveItem->setText(tr("Save Variable"));
        //: Status tip for saving variable changes
        mSaveItem->setStatusTip(tr("Apply variable changes (does not save to disk)."));
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::changeView() undefined view";
    }

    applyFormPaneSeamPolicy();

    // Four buttons were just renamed, which changes their tooltips and the bar's length
    updateEditorItemActionToolTips();
    updateEditorActivateAction();
    fitEditorToolBarToItsLength();

    // Update undo/redo button states when changing views
    slot_updateUndoRedoButtonStates();

    // Every way into a view (sidebar, shortcut, search result, deep link) comes through here
    syncEditorSidebarSelection();

    updateEditorItemCounts();

    // If we disabled updates during view change, ensure they get re-enabled
    // (selection handlers will also re-enable via restoreEditorState, but this
    // is a fallback in case no item is selected in the new view)
    if (mpSourceEditorEdbee && !mpSourceEditorEdbee->updatesEnabled()) {
        QTimer::singleShot(0ms, this, [this]() {
            if (mpSourceEditorEdbee) {
                mpSourceEditorEdbee->setUpdatesEnabled(true);
            }
        });
    }
}

// Only the pattern list and the stylesheet editor use extra room; the other forms are fixed sets of fields
static bool formPaneResizes(const EditorViewType view)
{
    switch (view) {
    case EditorViewType::cmAliasView:
    case EditorViewType::cmTimerView:
    case EditorViewType::cmKeysView:
    case EditorViewType::cmScriptView:
    case EditorViewType::cmVarsView:
        return false;
    case EditorViewType::cmTriggerView:
    case EditorViewType::cmActionView:
    case EditorViewType::cmUnknownView:
        break;
    }
    // No view is no form to hold to a height, so the column is left alone
    return true;
}

// The cap goes on the column, not only into the sizes: QSplitter shares a window
// resize out inside each pane's maximum.
void dlgTriggerEditor::applyFormPaneSeamPolicy()
{
    if (!splitter_right || !mpNonCodeWidgets || mHoldingFormPaneToItsContents) {
        return;
    }
    // Measuring delivers pending layout requests and the new cap posts another,
    // which reaches the event filter that called this
    mHoldingFormPaneToItsContents = true;
    holdFormPaneToItsContents();
    mHoldingFormPaneToItsContents = false;
}

// Not sizeHint(): the notice's wrapping label hints at a narrow width, so the hint overstates its height
int dlgTriggerEditor::formColumnHeightForItsWidth() const
{
    QLayout* pLayout = mpNonCodeWidgets ? mpNonCodeWidgets->layout() : nullptr;
    if (!pLayout) {
        return 0;
    }
    if (pLayout->hasHeightForWidth() && mpNonCodeWidgets->width() > 0) {
        const int wanted = pLayout->heightForWidth(mpNonCodeWidgets->width());
        if (wanted > 0) {
            return wanted;
        }
    }
    return mpNonCodeWidgets->sizeHint().height();
}

int dlgTriggerEditor::noticeRoomInFormColumn() const
{
    if (!mpSystemMessageArea || mpSystemMessageArea->isHidden() || !mpNonCodeWidgets) {
        return 0;
    }
    const int height = mpSystemMessageArea->heightForWidth(mpNonCodeWidgets->width());
    return (height > 0 ? height : mpSystemMessageArea->sizeHint().height()) + scmEditorColumnSpacing;
}

void dlgTriggerEditor::holdFormPaneToItsContents()
{
    // Handle 1 sits over mpSourceEditorArea and carries the code pane's heading
    const bool resizes = formPaneResizes(mCurrentView);
    splitter_right->setHandleResizes(1, resizes);

    const int noticeRoom = noticeRoomInFormColumn();
    const int noticeChange = noticeRoom - mNoticeRoomInFormColumn;
    mNoticeRoomInFormColumn = noticeRoom;

    if (resizes) {
        mpNonCodeWidgets->setMaximumHeight(QWIDGETSIZE_MAX);
        // A notice is the one change to these columns without an item change; moving
        // the seam by its cost, not re-measuring, leaves a dragged height where it was
        moveSeamByNoticeChange(noticeChange);
        return;
    }

    // A row shown or hidden posts its layout request, so deliver it before measuring
    uiDesign::invalidateLayoutsUpTo(currentFormArea(), mpNonCodeWidgets);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
    if (QLayout* pLayout = mpNonCodeWidgets->layout()) {
        pLayout->activate();
    }
    if (mpSourceEditorArea->isHidden()) {
        // Uncapped: QSplitter centres a lone child shorter than itself. isHidden(),
        // not isVisible(), so a pass while the editor is off screen reads the same.
        mpNonCodeWidgets->setMaximumHeight(QWIDGETSIZE_MAX);
        return;
    }
    const int wanted = formColumnHeightForItsWidth();
    if (wanted <= 0) {
        // Everything hidden: a moment between two views, not a height to hold to
        return;
    }

    QList<int> sizes = splitter_right->sizes();
    const int paneTotal = sizes.size() < 2 ? 0 : sizes.at(0) + sizes.at(1);
    const int paneHeight = paneTotal <= 0 ? wanted : std::clamp(wanted, 0, std::max(0, paneTotal - scmEditorSourcePaneFloor));
    mpNonCodeWidgets->setMaximumHeight(paneHeight);
    if (paneTotal <= 0 || sizes.at(0) == paneHeight) {
        return;
    }
    sizes[0] = paneHeight;
    sizes[1] = paneTotal - paneHeight;
    splitter_right->setSizes(sizes);
}

void dlgTriggerEditor::moveSeamByNoticeChange(const int noticeChange)
{
    if (noticeChange == 0 || mpSourceEditorArea->isHidden()) {
        return;
    }
    QList<int> sizes = splitter_right->sizes();
    if (sizes.size() < 2) {
        return;
    }
    const int paneTotal = sizes.at(0) + sizes.at(1);
    if (paneTotal <= 0) {
        return;
    }
    const int paneHeight = std::clamp(sizes.at(0) + noticeChange, 0, std::max(0, paneTotal - scmEditorSourcePaneFloor));
    if (paneHeight == sizes.at(0)) {
        return;
    }
    sizes[0] = paneHeight;
    sizes[1] = paneTotal - paneHeight;
    splitter_right->setSizes(sizes);
}

// A row hidden deeper than the column invalidates only its own parent's layout
QWidget* dlgTriggerEditor::currentFormArea() const
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        // The deepest row that hides; the pattern list under it is measured by the column
        return mpWidget_triggerOptionsRow;
    case EditorViewType::cmTimerView:
        return mpTimersMainArea;
    case EditorViewType::cmAliasView:
        return mpAliasMainArea;
    case EditorViewType::cmScriptView:
        return mpScriptsMainArea;
    case EditorViewType::cmActionView:
        return mpActionsMainArea;
    case EditorViewType::cmKeysView:
        return mpKeysMainArea;
    case EditorViewType::cmVarsView:
        return mpVarsMainArea;
    case EditorViewType::cmUnknownView:
        break;
    }
    return mpNonCodeWidgets;
}

int dlgTriggerEditor::formPaneHeightForItsContents(const int paneTotal) const
{
    // A change inside a widget only invalidates its immediate parent's layout, so
    // without this the form is measured as it was before the change
    uiDesign::invalidateLayoutsUpTo(mpWidget_triggerOptionsRow, mpNonCodeWidgets);
    uiDesign::invalidateLayoutsUpTo(mpScrollArea, mpNonCodeWidgets);
    int wanted = formColumnHeightForItsWidth();
    // Qt caps a scroll area's size hint at a couple of dozen lines of text, so
    // what the pattern rows want beyond that is asked for on top
    if (mpScrollArea && mpWidget_triggerItems && mpTriggersMainArea->isVisible()) {
        wanted += std::max(0, mpWidget_triggerItems->sizeHint().height() - mpScrollArea->sizeHint().height());
    }
    return std::clamp(wanted, 0, std::max(0, paneTotal - scmEditorSourcePaneFloor));
}

void dlgTriggerEditor::fitFormPaneToItsContents()
{
    if (!splitter_right || !mpNonCodeWidgets || !mpNonCodeWidgets->isVisible()) {
        return;
    }
    if (!formPaneResizes(mCurrentView)) {
        applyFormPaneSeamPolicy();
        return;
    }
    QList<int> sizes = splitter_right->sizes();
    if (sizes.size() < 2) {
        return;
    }
    const int paneTotal = sizes.at(0) + sizes.at(1);
    if (paneTotal <= 0) {
        return;
    }
    // Until the posted LayoutRequest arrives, the pattern scroll area answers with
    // the previous item's height. Only that event type is flushed, so nothing else
    // the selection queued runs in the middle of this.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
    if (QLayout* pLayout = mpNonCodeWidgets->layout()) {
        pLayout->activate();
    }

    int wanted = 0;
    const auto dragged = mDraggedFormPaneHeights.constFind(mCurrentView);
    if (dragged != mDraggedFormPaneHeights.constEnd()) {
        // Stored net of any notice (see slot_rightSplitterMoved()), so the one up now is added back
        wanted = std::clamp(dragged.value() + noticeRoomInFormColumn(), 0, std::max(0, paneTotal - scmEditorSourcePaneFloor));
    } else {
        wanted = formPaneHeightForItsContents(paneTotal);
    }
    if (wanted == sizes.at(0)) {
        return;
    }
    sizes[0] = wanted;
    sizes[1] = paneTotal - wanted;
    splitter_right->setSizes(sizes);
}

// Deferred: a resize arrives while the right hand splitter is handing its panes their geometry, and the
// refit moves the seam between those same panes. One per turn, as a window drag sends a resize per frame.
void dlgTriggerEditor::queueFormPaneRefit()
{
    if (mFormPaneRefitQueued) {
        return;
    }
    mFormPaneRefitQueued = true;
    QTimer::singleShot(0ms, this, [this]() {
        mFormPaneRefitQueued = false;
        fitFormPaneToItsContents();
    });
}

// A form shown for no item can be typed into but never saved, so the help stands in for it
static bool hasItemToShow(const QTreeWidget* pTree)
{
    const QTreeWidgetItem* pRoot = pTree->topLevelItem(0);
    const QTreeWidgetItem* pCurrent = pTree->currentItem();
    return pRoot && pRoot->childCount() && pCurrent && pCurrent != pRoot;
}

void dlgTriggerEditor::slot_showTimers()
{
    changeView(EditorViewType::cmTimerView);
    if (!hasItemToShow(treeWidget_timers)) {
        clearTimerForm();
    } else {
        mpTimersMainArea->show();
        mpSourceEditorArea->show();
        slot_timerSelected(treeWidget_timers->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_timers);
}

void dlgTriggerEditor::showCurrentTriggerItem()
{
    // The full reset leaves the editor on the triggers view, so there is nothing more to show
    if (mCleanResetDeferred) {
        buildDeferredTrees(true);
        return;
    }
    if (mCurrentView != EditorViewType::cmUnknownView) {
        return;
    }

    changeView(EditorViewType::cmTriggerView);
    if (!hasItemToShow(treeWidget_triggers)) {
        clearTriggerForm();
    } else {
        mpTriggersMainArea->show();
        mpSourceEditorArea->show();
        slot_triggerSelected(treeWidget_triggers->currentItem());
    }
}

void dlgTriggerEditor::slot_showTriggers()
{
    changeView(EditorViewType::cmTriggerView);
    if (!hasItemToShow(treeWidget_triggers)) {
        clearTriggerForm();
    } else {
        mpTriggersMainArea->show();
        mpSourceEditorArea->show();
        slot_triggerSelected(treeWidget_triggers->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_triggers);
}

void dlgTriggerEditor::slot_showScripts()
{
    changeView(EditorViewType::cmScriptView);
    if (!hasItemToShow(treeWidget_scripts)) {
        clearScriptForm();
    } else {
        mpScriptsMainArea->show();
        mpSourceEditorArea->show();
        slot_scriptsSelected(treeWidget_scripts->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_scripts);
}

void dlgTriggerEditor::slot_showKeys()
{
    changeView(EditorViewType::cmKeysView);
    if (!hasItemToShow(treeWidget_keys)) {
        clearKeyForm();
    } else {
        mpKeysMainArea->show();
        mpSourceEditorArea->show();
        slot_keySelected(treeWidget_keys->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_keys);
}

void dlgTriggerEditor::slot_showVariables()
{
    changeView(EditorViewType::cmVarsView);
    repopulateVars();
    mpCurrentVarItem = nullptr;
    checkBox_displayAllVariables->show();
    checkBox_displayAllVariables->setChecked(showHiddenVars);
    if (!hasItemToShow(treeWidget_variables)) {
        clearVarForm();
    } else {
        mpVarsMainArea->show();
        mpSourceEditorArea->show();
        slot_variableSelected(treeWidget_variables->currentItem());
    }
    fitFormPaneToItsContents();
    focusPanelTree(treeWidget_variables);
}

void dlgTriggerEditor::show_vars()
{
    //no repopulation of variables
    changeView(EditorViewType::cmVarsView);
    mpCurrentVarItem = nullptr;
    mpSourceEditorArea->show();
    checkBox_displayAllVariables->show();
    checkBox_displayAllVariables->setChecked(showHiddenVars);
    QTreeWidgetItem* pI = treeWidget_variables->topLevelItem(0);
    if (pI) {
        if (pI->childCount() > 0) {
            mpVarsMainArea->show();
            slot_variableSelected(treeWidget_variables->currentItem());
        } else {
            clearVarForm();
        }
    }
    treeWidget_variables->show();
}


void dlgTriggerEditor::slot_showAliases()
{
    changeView(EditorViewType::cmAliasView);
    if (!hasItemToShow(treeWidget_aliases)) {
        clearAliasForm();
    } else {
        mpAliasMainArea->show();
        mpSourceEditorArea->show();
        slot_aliasSelected(treeWidget_aliases->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_aliases);
}

// Chunk names as TTrigger::compileScript() and its siblings build them. A repaired item keeps its last error, so
// it counts only while broken; a cleared tree's current item is deleted before fillout_form() forgets it.
bool dlgTriggerEditor::currentScriptChunk(int& itemId, QString& chunkName, QString& error)
{
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (auto* pT = treeWidget_triggers->topLevelItemCount() ? getTriggerFromTreeItem(mpCurrentTriggerItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Trigger: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmAliasView:
        if (auto* pT = treeWidget_aliases->topLevelItemCount() ? getAliasFromTreeItem(mpCurrentAliasItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Alias: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmTimerView:
        if (auto* pT = treeWidget_timers->topLevelItemCount() ? getTimerFromTreeItem(mpCurrentTimerItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Timer: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmScriptView:
        if (auto* pT = treeWidget_scripts->topLevelItemCount() ? getScriptFromTreeItem(mpCurrentScriptItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Script: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmActionView:
        if (auto* pT = treeWidget_actions->topLevelItemCount() ? getActionFromTreeItem(mpCurrentActionItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Button: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmKeysView:
        if (auto* pT = treeWidget_keys->topLevelItemCount() ? getKeyFromTreeItem(mpCurrentKeyItem) : nullptr) {
            itemId = pT->getID();
            chunkName = qsl("Key: %1").arg(pT->getName());
            error = pT->state() ? QString() : pT->getError();
            return true;
        }
        break;
    case EditorViewType::cmUnknownView:
        [[fallthrough]];
    case EditorViewType::cmVarsView:
        break;
    }
    return false;
}

// Turns the [string "Trigger: name"]:12: location of a Lua error in the open
// item's own script into a link that jumps to that line. Another item's error -
// a descendant's, even one of the same name - or a location in another chunk,
// such as a script a running one called, stays as text.
QString dlgTriggerEditor::linkErrorLineToScript(const QString& error)
{
    mErrorLineView = EditorViewType::cmUnknownView;
    mErrorLineItemId = -1;

    int itemId = -1;
    QString chunkName;
    QString ownError;
    // Toggling a broken item on wraps its error in an explanation that names the item first
    const qsizetype ownErrorStart = currentScriptChunk(itemId, chunkName, ownError) && !ownError.isEmpty() ? error.indexOf(ownError) : -1;
    if (ownErrorStart < 0) {
        return error;
    }

    // TLuaInterpreter::compile() HTML-escapes the message, so the quotes arrive as entities
    static const QRegularExpression location(qsl(R"(\[string &quot;(.*?)&quot;\]:(\d+):)"));
    const QRegularExpressionMatch match = location.match(error, ownErrorStart);
    if (!match.hasMatch() || match.capturedEnd() > ownErrorStart + ownError.size()) {
        return error;
    }

    QString label = match.captured(1);
    label.replace(qsl("&quot;"), qsl("\"")).replace(qsl("&lt;"), qsl("<")).replace(qsl("&gt;"), qsl(">")).replace(qsl("&amp;"), qsl("&"));
    // Lua cuts a long chunk name, or one with a line break, short and appends "...";
    // the cut is by bytes, so a split character arrives as a replacement character
    bool sameChunk = label == chunkName;
    if (!sameChunk && label.endsWith(qsl("..."))) {
        QString stemText = label.chopped(3);
        while (stemText.endsWith(QChar::ReplacementCharacter)) {
            stemText.chop(1);
        }
        const QByteArray stem = stemText.toUtf8();
        sameChunk = !stem.isEmpty() && chunkName.toUtf8().startsWith(stem);
    }
    if (!sameChunk) {
        return error;
    }

    mErrorLineView = mCurrentView;
    mErrorLineItemId = itemId;
    QString linked = error;
    linked.replace(
            match.capturedStart(), match.capturedLength(), qsl("<a href='mudlet-error-line:%1' style='color: inherit; text-decoration: underline;'>%2</a>").arg(match.captured(2), match.captured(0)));
    return linked;
}

// The code heading already shows a script error itself, so the banner says what to do about it and links its line
void dlgTriggerEditor::showActivationFailure(const QString& name, const QString& error)
{
    const QRegularExpressionMatch match = csmCompileErrorLineRegex.match(QTextDocumentFragment::fromHtml(error).toPlainText().simplified());
    if (!match.hasMatch()) {
        //: Editor banner when an item could not be switched on. %1 is the item's name, %2 the reason the item gave, such as a pattern that is not valid.
        showError(tr("<p>\"%1\" could not be switched on: %2</p><p>Fix that, then switch it on again.</p>").arg(name.toHtmlEscaped(), error));
        return;
    }

    const int line = match.captured(1).toInt();
    //: Link in the editor banner that jumps to the script line with the error. %1 is the line number.
    const QString lineLink = qsl("<a href='mudlet-error-line:%1' style='color: inherit; text-decoration: underline;'>%2</a>").arg(QString::number(line), tr("line %1").arg(line));
    //: Editor banner when an item whose script has an error was switched on. %1 is the item's name, %2 a link reading "line 12", %3 what the Lua compiler said about that line (in English).
    showError(tr("<p>\"%1\" could not be switched on: %2: %3</p><p>Fix that, then switch it on again.</p>").arg(name.toHtmlEscaped(), lineLink, match.captured(2).toHtmlEscaped()));
    // showError() forgets the item any earlier link was for; this one is for the open item's own script
    int itemId = -1;
    QString chunkName;
    QString ownError;
    if (currentScriptChunk(itemId, chunkName, ownError)) {
        mErrorLineView = mCurrentView;
        mErrorLineItemId = itemId;
    }
}

// The tree's dot shows an item the emergency stop holds back as not running; this says the same to a screen reader.
// Only an item that would run without the stop is held: one in a switched-off group already says so.
QString dlgTriggerEditor::spokenRunState(const QString& description) const
{
    if (!mpHost || !mpHost->mEmergencyStop) {
        return description;
    }
    if (description == descActive || description == descActiveFolder || description == descActiveFilterChain || description == descActiveOffsetTimer) {
        return descHeldByStop.arg(description);
    }
    return description;
}

void dlgTriggerEditor::jumpToErrorLine(const int line)
{
    int itemId = -1;
    QString chunkName;
    QString ownError;
    // The banner can outlive a switch to another item, whose script the line is not in
    if (line < 1 || !mpSourceEditorEdbee || !mpSourceEditorEdbeeDocument || mCurrentView != mErrorLineView || !currentScriptChunk(itemId, chunkName, ownError) || itemId != mErrorLineItemId) {
        return;
    }

    // An unclosed block is reported on the "end" line the item classes append after the script
    const auto lineCount = static_cast<int>(mpSourceEditorEdbeeDocument->lineCount());
    const int targetLine = std::min(line, std::max(lineCount, 1)) - 1;
    mpSourceEditorEdbee->setFocus();
    mpSourceEditorEdbee->controller()->moveCaretTo(static_cast<size_t>(targetLine), static_cast<size_t>(0), false);
}

// A compile error of the item in the editor goes to reportCompileError(), not here
void dlgTriggerEditor::showError(const QString& text)
{
    // A still-running undo-toast expiry timer would hide this message when it
    // fires, so cancel it - the toast's content is gone from the screen anyway
    cancelBannerUndoTimer();
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->show();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    setSystemMessage(linkErrorLineToScript(text));
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();

    // Reconnect close button to normal hide behavior (not banner dismiss or package warning)
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_packageWarningDismissed);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);

    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(text);
    }
}

void dlgTriggerEditor::reportCompileError(const QString& error)
{
    if (mEditorSaveErrorCaptureOpen) {
        mEditorSaveErrorCaptured = error;
    } else {
        mEditorCompileMessage = error;
        updateEditorCodeHeading();
    }

    // A screen reader does not come across the heading's note on its own
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(error);
    }
}

void dlgTriggerEditor::showWarning(const QString& text, bool announce)
{
    // A still-running undo-toast expiry timer would hide this message when it
    // fires, so cancel it - the toast's content is gone from the screen anyway
    cancelBannerUndoTimer();
    mpSystemMessageArea->notificationAreaIconLabelInformation->hide();
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->show();
    setSystemMessage(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();

    // Reconnect close button to normal hide behavior (not banner dismiss or package warning)
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_packageWarningDismissed);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);

    if (!mpHost->mIsProfileLoadingSequence && announce) {
        mudlet::self()->announce(text);
    }
}

// Said once per package: the message is about the package, not the item that was clicked
void dlgTriggerEditor::showPackageWarning(const QString& packageName, QTreeWidgetItem* pItem)
{
    if (packageName.isEmpty()) {
        return;
    }

    // The screen-reader description belongs to the item, so it is set even when no banner is raised
    if (pItem) {
        updatePackageItemAccessibility(pItem, pItem->data(0, Qt::AccessibleDescriptionRole).toString());
    }

    if (mPackageWarningsDismissed || mWarnedPackages.contains(packageName)) {
        return;
    }
    mWarnedPackages.insert(packageName);

    //: Warning banner shown in the editor when the item being looked at came from a package
    showWarning(tr("This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications."), false);

    // showWarning() wired the plain hide, which would not remember this dismissal
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_packageWarningDismissed);

    if (!mPackageWarningAnnounced) {
        mPackageWarningAnnounced = true;
        //: First-time educational message for screen reader users about package items
        mudlet::self()->announce(tr("Package item. Copy before editing to preserve changes."));
    }
}

void dlgTriggerEditor::slot_packageWarningDismissed()
{
    mPackageWarningsDismissed = true;
    hideSystemMessageArea();
}

void dlgTriggerEditor::showInfo(const QString& text)
{
    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    setSystemMessage(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey.clear();
    if (!mpHost->mIsProfileLoadingSequence) {
        mudlet::self()->announce(text);
    }
}

static QString themedBannerLinkColor()
{
    return uiDesign::themeTokens().accentText.name();
}

// QLabel bakes an anchor's colour in at setText() and leaves 'color: inherit' at
// Qt's own blue, so the placeholder is inked here; the uninked message is kept so
// a theme change can re-ink a banner already on screen.
static QString inkedSystemMessage(const QString& content)
{
    return QString(content).replace(qsl("color: inherit"), qsl("color: %1").arg(themedBannerLinkColor()));
}

// text() holds the inked copy, so the message as given is kept for re-inking
void dlgTriggerEditor::setSystemMessage(const QString& content)
{
    auto* pMessageBox = mpSystemMessageArea->notificationAreaMessageBox;
    pMessageBox->setProperty(scmProp_editorRichText, content);
    pMessageBox->setText(inkedSystemMessage(content));
}

QString dlgTriggerEditor::systemMessage() const
{
    return mpSystemMessageArea ? mpSystemMessageArea->notificationAreaMessageBox->property(scmProp_editorRichText).toString() : QString();
}

void dlgTriggerEditor::showIntro(const QString& desiredOption)
{
    if (!introAddItem.contains(mCurrentView)) {
        qWarning() << "ERROR: dlgTriggerEditor::showIntro() undefined view";
        return;
    }

    static const auto bannerKey = qsl("intro");
    bool includeBasePreference = true;
    if (mCurrentView == EditorViewType::cmTriggerView) {
        // The trigger intro banner predates the global suppression toggle, so keep
        // honouring only its explicit "hide permanently" preference to ensure it
        // still shows up for profiles that never opted out directly.
        includeBasePreference = false;
    }

    if (bannerPermanentlyHidden(mCurrentView, bannerKey, includeBasePreference)) {
        return;
    }

    introTextParts introAddCurrentItem = introAddItem.value(mCurrentView);
    QString introTextOptions;
    for (const auto& [name, headline, contents] : std::as_const(introAddCurrentItem.options)) {
        introTextOptions.append((name != desiredOption) ? qsl("<li><a href='%1' style='color: inherit; text-decoration: underline;'>%2</a></li>").arg(name, headline)
                                                        : qsl("<li><strong>%1</strong>%2</li>").arg(headline, contents));
    }

    QString content = qsl("<p>%1</p><ul>%2</ul>").arg(introAddCurrentItem.summary, introTextOptions);

    showHideableBanner(content, bannerKey);
}

void dlgTriggerEditor::showHideableBanner(const QString& content, const QString& bannerKey)
{
    if (!mpSystemMessageArea) {
        return;
    }

    const QString settingsKey = bannerSettingsKey(mCurrentView, bannerKey);
    const QString baseKey = bannerSettingsKey(mCurrentView, QString());
    if (settingsKey.isEmpty()) {
        return;
    }

    if (mTemporarilyHiddenBanners.contains(settingsKey) || (!bannerKey.isEmpty() && mTemporarilyHiddenBanners.contains(baseKey))) {
        return;
    }

    bool includeBasePreference = true;
    if (mCurrentView == EditorViewType::cmTriggerView && bannerKey == qsl("intro")) {
        // Match the behaviour in showIntro(): ignore the view-wide suppression
        // switch so the legacy trigger intro reappears unless it was hidden via
        // its own banner controls.
        includeBasePreference = false;
    }

    if (bannerPermanentlyHidden(mCurrentView, bannerKey, includeBasePreference)) {
        return;
    }

    if (mpSystemMessageArea->isVisible() && mCurrentBannerKey != bannerKey && !systemMessage().isEmpty()) {
        return;
    }

    if (mpSystemMessageArea->isVisible() && mCurrentBannerKey == bannerKey && systemMessage() == content) {
        return;
    }

    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
    disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_packageWarningDismissed);
    connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);

    disconnect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, nullptr, nullptr);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);

    showInfo(content);
    mCurrentBannerKey = bannerKey;
}

QString dlgTriggerEditor::bannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const
{
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    if (legacyKey.isEmpty()) {
        return legacyKey;
    }

    const QString prefix = profileSettingsPrefix();
    if (prefix.isEmpty()) {
        return legacyKey;
    }

    return qsl("%1/%2").arg(prefix, legacyKey);
}

QString dlgTriggerEditor::legacyBannerSettingsKey(EditorViewType viewType, const QString& bannerKey) const
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<EditorViewType>();
    const char* enumName = metaEnum.valueToKey(static_cast<int>(viewType));

    if (!enumName) {
        return QString();
    }

    QString key = QString::fromLatin1(enumName).toLower();
    if (!bannerKey.isEmpty()) {
        key += qsl("/%1").arg(bannerKey);
    }

    return key;
}

QString dlgTriggerEditor::profileSettingsPrefix() const
{
    if (!mpHost) {
        return QString();
    }

    const QString profileName = mpHost->getName();
    if (profileName.isEmpty()) {
        return QString();
    }

    const QString sanitized = MudletApp::sanitizeForPath(profileName);
    if (sanitized.isEmpty()) {
        return QString();
    }

    return qsl("profiles/%1").arg(sanitized);
}

void dlgTriggerEditor::slot_showActions()
{
    changeView(EditorViewType::cmActionView);
    if (!hasItemToShow(treeWidget_actions)) {
        clearActionForm();
    } else {
        mpActionsMainArea->show();
        mpSourceEditorArea->show();
        slot_actionSelected(treeWidget_actions->currentItem());
    }
    fitFormPaneToItsContents();
    focusItemTree(treeWidget_actions);
}

void dlgTriggerEditor::slot_saveEdits()
{
    // The item behind a merged folder was saved when the folder was chosen, and its errors must stay hidden
    if (!mFolderGroupKey.isEmpty()) {
        return;
    }
    beginSaveErrorCapture();

    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        rewarnAboutCurrentKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    default:
        qWarning() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view, not sure what to save";
    }

    endSaveErrorCapture();

    // There was a mpHost->serialize() call here, but that code was
    // "short-circuited" and returned without doing anything;
}

void dlgTriggerEditor::slot_addNewItem()
{
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        addTrigger(false); //add normal trigger
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        addTimer(false); //add normal timer
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        addAlias(false); //add normal alias
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        addScript(false); //add normal script
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        addAction(false); //add normal action
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        addKey(false); //add normal key
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        addVar(false); //add variable
        mpVarsMainArea->lineEdit_var_name->setFocus();
        // variables start without a default name
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_addNewGroup()
{
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        addTrigger(true); //add trigger group
        mpTriggersMainArea->lineEdit_trigger_name->setFocus();
        mpTriggersMainArea->lineEdit_trigger_name->selectAll();
        break;
    case EditorViewType::cmTimerView:
        addTimer(true); //add timer group
        mpTimersMainArea->lineEdit_timer_name->setFocus();
        mpTimersMainArea->lineEdit_timer_name->selectAll();
        break;
    case EditorViewType::cmAliasView:
        addAlias(true); //add alias group
        mpAliasMainArea->lineEdit_alias_name->setFocus();
        mpAliasMainArea->lineEdit_alias_name->selectAll();
        break;
    case EditorViewType::cmScriptView:
        addScript(true); //add script group
        mpScriptsMainArea->lineEdit_script_name->setFocus();
        mpScriptsMainArea->lineEdit_script_name->selectAll();
        break;
    case EditorViewType::cmActionView:
        addAction(true); //add action group
        mpActionsMainArea->lineEdit_action_name->setFocus();
        mpActionsMainArea->lineEdit_action_name->selectAll();
        break;
    case EditorViewType::cmKeysView:
        addKey(true); //add keys group
        mpKeysMainArea->lineEdit_key_name->setFocus();
        mpKeysMainArea->lineEdit_key_name->selectAll();
        break;
    case EditorViewType::cmVarsView:
        addVar(true); // add lua table
        mpVarsMainArea->lineEdit_var_name->setFocus();
        // variables start without a default name
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
}

void dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag()
{
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        activeToggle_trigger();
        break;
    case EditorViewType::cmTimerView:
        activeToggle_timer();
        break;
    case EditorViewType::cmAliasView:
        activeToggle_alias();
        break;
    case EditorViewType::cmScriptView:
        activeToggle_script();
        break;
    case EditorViewType::cmActionView:
        activeToggle_action();
        break;
    case EditorViewType::cmKeysView:
        activeToggle_key();
        break;

    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_saveEdits() undefined view";
    }
    updateEditorActivateAction();
}

void dlgTriggerEditor::slot_sourceFindMove()
{
    int x = mpSourceEditorEdbee->width() - mpSourceEditorFindArea->width();
    int y = mpSourceEditorEdbee->height() - mpSourceEditorFindArea->height();
    if (mpSourceEditorEdbee->verticalScrollBar()->isVisible()) {
        x = x - mpSourceEditorEdbee->verticalScrollBar()->width();
    }
    if (mpSourceEditorEdbee->horizontalScrollBar()->isVisible()) {
        y = y - mpSourceEditorEdbee->horizontalScrollBar()->height();
    }
    mpSourceEditorFindArea->move(x, y);
    mpSourceEditorFindArea->update();
}

void dlgTriggerEditor::slot_openSourceFind()
{
    slot_sourceFindMove();
    mpSourceEditorFindArea->show();
    mpSourceEditorFindArea->lineEdit_findText->setFocus();
    mpSourceEditorFindArea->lineEdit_findText->selectAll();
}

void dlgTriggerEditor::slot_closeSourceFind()
{
    auto controller = mpSourceEditorEdbee->controller();
    controller->borderedTextRanges()->clear();
    controller->textSelection()->range(0).clearSelection();
    controller->update();
    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbee->setFocus();
}

void dlgTriggerEditor::slot_sourceReplace()
{
    auto controller = mpSourceEditorEdbee->controller();
    const QString findText = mpSourceEditorFindArea->lineEdit_findText->text();
    const QString replaceText = mpSourceEditorFindArea->lineEdit_replaceText->text();
    // the same matching as the finds below, which fold case the Unicode way (ß matches ss)
    edbee::RegExp term(findText, false, edbee::RegExp::SyntaxFixedString);
    for (size_t i = 0; i < controller->textSelection()->rangeCount(); i++) {
        auto& range = controller->textSelection()->range(i);
        // anchor() is the end of a selection made right to left, min() is always its start
        const QString selected = mpSourceEditorEdbee->textDocument()->text().mid(range.min(), range.length());
        // finding replaces the whole selection, so `range` and the count are stale after one
        if (selected == replaceText) {
            slot_sourceFindNext();
            return;
        }
        if (!range.hasSelection()) {
            slot_sourceFindPrevious();
            return;
        }
        if (term.indexIn(selected) != 0 || term.len(0) != static_cast<size_t>(selected.length())) {
            slot_sourceFindNext();
            return;
        }
        mpSourceEditorEdbee->textDocument()->replace(range.min(), range.length(), replaceText);
        range.setLength(replaceText.length());
    }
}

void dlgTriggerEditor::slot_sourceFindPrevious()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findPrev(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_sourceFindMove();
}

void dlgTriggerEditor::slot_sourceFindNext()
{
    auto controller = mpSourceEditorEdbee->controller();
    auto searcher = controller->textSearcher();
    searcher->setSearchTerm(mpSourceEditorFindArea->lineEdit_findText->text());
    searcher->setCaseSensitive(false);
    searcher->findNext(mpSourceEditorEdbee);
    controller->scrollCaretVisible();
    controller->update();
    slot_sourceFindMove();
}

void dlgTriggerEditor::slot_sourceFindTextChanges()
{
    auto text = mpSourceEditorFindArea->lineEdit_findText->text();
    auto controller = mpSourceEditorEdbee->controller();
    // the marks left by a longer term no longer match what the box holds
    controller->borderedTextRanges()->clear();
    if (text.length() <= 2) {
        controller->update();
        return;
    }

    auto searcher = controller->textSearcher();
    controller->textSelection()->range(0).clearSelection();
    searcher->setSearchTerm(text);
    searcher->markAll(controller->borderedTextRanges());
    controller->update();
}

void dlgTriggerEditor::slot_deleteItemOrGroup()
{
    const EditorViewType view = resolveCurrentView();
    if (QTreeWidget* pTree = treeForView(view)) {
        for (const QTreeWidgetItem* pItem : pTree->selectedItems()) {
            const QString moduleName = moduleOfMasterFolder(view, pItem->data(0, Qt::UserRole).toInt());
            if (!moduleName.isEmpty()) {
                offerToUninstallModule(moduleName);
                return;
            }
        }
    }

    switch (view) {
    case EditorViewType::cmTriggerView:
        delete_trigger();
        break;
    case EditorViewType::cmTimerView:
        delete_timer();
        break;
    case EditorViewType::cmAliasView:
        delete_alias();
        break;
    case EditorViewType::cmScriptView:
        delete_script();
        break;
    case EditorViewType::cmActionView:
        delete_action();
        break;
    case EditorViewType::cmKeysView:
        delete_key();
        break;
    case EditorViewType::cmVarsView:
        delete_variable();
        break;
    default:
        qDebug() << "ERROR: dlgTriggerEditor::slot_deleteItemOrGroup() undefined view";
    }
}

void dlgTriggerEditor::slot_saveSelectedItem()
{
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        rewarnAboutCurrentKey();
        break;
    case EditorViewType::cmVarsView:
        saveVar();
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_saveSelectedItem() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
    }
}


void dlgTriggerEditor::slot_toggleCentralDebugConsole()
{
    mudlet::self()->attachDebugArea(mpHost->getName());

    mudlet::smpDebugArea->setVisible(!TDebug::smDebugMode);
    TDebug::smDebugMode = !TDebug::smDebugMode;
    mudlet::smpDebugArea->setWindowTitle(tr("Central Debug Console"));
    if (TDebug::smDebugMode) {
        // Posted to run after the console's own deferred fit of its wrap to the window that showing it
        // queued: printed now, a first opening's lines wrap at the initial 100 columns and are cut off
        QTimer::singleShot(0ms, this, []() {
            if (!TDebug::smDebugMode) {
                return;
            }
            // If this is the first time the window is shown we want any previously
            // enqueued messages to be painted onto the central debug console:
            TDebug::flushMessageQueue();
            // Every time, not just the first: the filters may have been narrowed since it was last closed:
            TDebug::announceFilters();
        });
    }
    mudlet::self()->refreshTabBar();
}

void dlgTriggerEditor::slot_nextSection()
{
    // Hidden widgets still take focus, and typing there would edit the item a merged folder hides
    if (!mFolderGroupKey.isEmpty()) {
        mpFolderTree->setFocus();
        return;
    }
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_triggers)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_triggers)->hasFocus()) {
            mpTriggersMainArea->lineEdit_trigger_name->setFocus();
            return;
        }
        if (mpTriggersMainArea->hasFocus()) {
            mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
            return;
        }
        for (auto child : mpTriggersMainArea->scrollArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        for (auto child : mpTriggersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_timers)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_timers)->hasFocus()) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        }
        for (auto child : mpTimersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_aliases)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_aliases)->hasFocus()) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        }
        for (auto child : mpAliasMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_scripts)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_scripts)->hasFocus()) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        }
        for (auto child : mpScriptsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_actions)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_actions)->hasFocus()) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        }
        for (auto child : mpActionsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            sectionTree(treeWidget_keys)->setFocus();
            return;
        }
        if (sectionTree(treeWidget_keys)->hasFocus()) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        }
        for (auto child : mpKeysMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (qsl("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            treeWidget_variables->setFocus();
            return;
        }
        if (treeWidget_variables->hasFocus()) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        }
        for (auto child : mpVarsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpSourceEditorEdbee->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmUnknownView:
        return;
    }
}

void dlgTriggerEditor::slot_previousSection()
{
    if (!mFolderGroupKey.isEmpty()) {
        mpFolderTree->setFocus();
        return;
    }
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mTriggerPatternEdit[0]->singleLineTextEdit_pattern->setFocus();
            return;
        }
        if (sectionTree(treeWidget_triggers)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpTriggersMainArea->scrollArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                mpTriggersMainArea->lineEdit_trigger_name->setFocus();
                return;
            }
        }
        for (auto child : mpTriggersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_triggers)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmTimerView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpTimersMainArea->lineEdit_timer_name->setFocus();
            return;
        }
        if (sectionTree(treeWidget_timers)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpTimersMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_timers)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmAliasView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpAliasMainArea->lineEdit_alias_name->setFocus();
            return;
        }
        if (sectionTree(treeWidget_aliases)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpAliasMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_aliases)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmScriptView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpScriptsMainArea->lineEdit_script_name->setFocus();
            return;
        }
        if (sectionTree(treeWidget_scripts)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpScriptsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_scripts)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmActionView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpActionsMainArea->lineEdit_action_name->setFocus();
            return;
        }
        if (sectionTree(treeWidget_actions)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpActionsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_actions)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmKeysView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpKeysMainArea->lineEdit_key_name->setFocus();
            return;
        }
        if (sectionTree(treeWidget_keys)->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpKeysMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                sectionTree(treeWidget_keys)->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmVarsView:
        if (QString("edbee::TextEditorComponent").compare(QApplication::focusWidget()->metaObject()->className()) == 0) {
            mpVarsMainArea->lineEdit_var_name->setFocus();
            return;
        }
        if (treeWidget_variables->hasFocus()) {
            mpSourceEditorEdbee->setFocus();
            return;
        }
        for (auto child : mpVarsMainArea->findChildren<QWidget*>()) {
            if (child->hasFocus()) {
                treeWidget_variables->setFocus();
                return;
            }
        }
        break;
    case EditorViewType::cmUnknownView:
        return;
    }
}

// The same key that opens the editor brings the main window back from it
void dlgTriggerEditor::slot_updateActivateMainWindowShortcut()
{
    const QString key = qsl("Script editor");
    QKeySequence sequence;
    if (!mpHost) {
        return;
    }
    if (auto it = mpHost->profileShortcuts.find(key); it != mpHost->profileShortcuts.end()) {
        sequence = *it->second;
    } else if (auto* pManager = mudlet::self()->shortcutsManager()) {
        if (auto* pSequence = pManager->getSequence(key)) {
            sequence = *pSequence;
        }
    }
    mpShortcutActivateMainWindow->setKey(sequence);
}

void dlgTriggerEditor::slot_activateMainWindow()
{
    mudlet::self()->activateWindow();
    mpHost->mainConsoleView()->setFocus();
}

void dlgTriggerEditor::exportTrigger(const QString& fileName)
{
    QString name;
    TTrigger* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_triggers->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportTrigger(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportTimer(const QString& fileName)
{
    QString name;
    TTimer* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_timers->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getTimerUnit()->getTimer(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportTimer(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportAlias(const QString& fileName)
{
    QString name;
    TAlias* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_aliases->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getAliasUnit()->getAlias(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportAlias(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportAction(const QString& fileName)
{
    QString name;
    TAction* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_actions->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getActionUnit()->getAction(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportAction(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportScript(const QString& fileName)
{
    QString name;
    TScript* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_scripts->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getScriptUnit()->getScript(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }
    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportScript(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportKey(const QString& fileName)
{
    QString name;
    TKey* pT = nullptr;
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        pT = mpHost->getKeyUnit()->getKey(triggerID);
        if (pT) {
            name = pT->getName();
        } else {
            QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
            return;
        }

    } else {
        QMessageBox::warning(this, tr("Export Package:"), tr("You have to choose an item for export first. Please select a tree item and then click on export again."));
        return;
    }
    XMLexport writer(pT);
    if (writer.exportKey(fileName)) {
        statusBar()->showMessage(tr("Package %1 saved").arg(name.toHtmlEscaped()), 2000);
    }
}

void dlgTriggerEditor::exportTriggerToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_triggers->selectedItems();
    QStringList triggerNames;
    QList<TTrigger*> triggersToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int triggerID = pItem->data(0, Qt::UserRole).toInt();
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
        if (pT) {
            triggerNames << pT->getName();
            triggersToExport << pT;
        }
    }

    if (triggersToExport.isEmpty()) {
        return;
    }

    if (triggersToExport.size() == 1) {
        // Single item - use existing method
        XMLexport writer(triggersToExport.first());
        writer.exportToClipboard(triggersToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(triggerNames.first().toHtmlEscaped()), 2000);
    } else {
        // Multiple items - export them individually and let user paste multiple times
        exportMultipleTriggersToClipboard(triggersToExport);
        statusBar()->showMessage(tr("Copied %1 triggers to clipboard").arg(triggersToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleTriggersToClipboard(const QList<TTrigger*>& triggers)
{
    if (triggers.isEmpty()) {
        return;
    }

    // Store multiple XML packages separated by a special delimiter
    // This allows the paste function to split and import each item individually
    QStringList xmlPackages;

    for (TTrigger* trigger : triggers) {
        XMLexport writer(trigger);

        // Get the XML for this trigger by temporarily using the clipboard
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(trigger);
        QString triggerXml = QApplication::clipboard()->text();
        QApplication::clipboard()->setText(originalClipboard);

        xmlPackages << triggerXml;
    }

    // Combine all XML packages with a special separator that paste can recognize
    QString combinedXml = xmlPackages.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportTimerToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_timers->selectedItems();
    QStringList timerNames;
    QList<TTimer*> timersToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int timerID = pItem->data(0, Qt::UserRole).toInt();
        TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
        if (pT) {
            timerNames << pT->getName();
            timersToExport << pT;
        }
    }

    if (timersToExport.isEmpty()) {
        return;
    }

    if (timersToExport.size() == 1) {
        XMLexport writer(timersToExport.first());
        writer.exportToClipboard(timersToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(timerNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleTimersToClipboard(timersToExport);
        statusBar()->showMessage(tr("Copied %1 timers to clipboard").arg(timersToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleTimersToClipboard(const QList<TTimer*>& timers)
{
    if (timers.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TTimer* timer : timers) {
        XMLexport writer(timer);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(timer);
        QString timerXml = QApplication::clipboard()->text();
        xmlParts << timerXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportAliasToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_aliases->selectedItems();
    QStringList aliasNames;
    QList<TAlias*> aliasesToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int aliasID = pItem->data(0, Qt::UserRole).toInt();
        TAlias* pT = mpHost->getAliasUnit()->getAlias(aliasID);
        if (pT) {
            aliasNames << pT->getName();
            aliasesToExport << pT;
        }
    }

    if (aliasesToExport.isEmpty()) {
        return;
    }

    if (aliasesToExport.size() == 1) {
        XMLexport writer(aliasesToExport.first());
        writer.exportToClipboard(aliasesToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(aliasNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleAliasesToClipboard(aliasesToExport);
        statusBar()->showMessage(tr("Copied %1 aliases to clipboard").arg(aliasesToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleAliasesToClipboard(const QList<TAlias*>& aliases)
{
    if (aliases.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TAlias* alias : aliases) {
        XMLexport writer(alias);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(alias);
        QString aliasXml = QApplication::clipboard()->text();
        xmlParts << aliasXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportActionToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_actions->selectedItems();
    QStringList actionNames;
    QList<TAction*> actionsToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int actionID = pItem->data(0, Qt::UserRole).toInt();
        TAction* pT = mpHost->getActionUnit()->getAction(actionID);
        if (pT) {
            actionNames << pT->getName();
            actionsToExport << pT;
        }
    }

    if (actionsToExport.isEmpty()) {
        return;
    }

    if (actionsToExport.size() == 1) {
        XMLexport writer(actionsToExport.first());
        writer.exportToClipboard(actionsToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(actionNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleActionsToClipboard(actionsToExport);
        statusBar()->showMessage(tr("Copied %1 actions to clipboard").arg(actionsToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleActionsToClipboard(const QList<TAction*>& actions)
{
    if (actions.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TAction* action : actions) {
        XMLexport writer(action);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(action);
        QString actionXml = QApplication::clipboard()->text();
        xmlParts << actionXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportScriptToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_scripts->selectedItems();
    QStringList scriptNames;
    QList<TScript*> scriptsToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int scriptID = pItem->data(0, Qt::UserRole).toInt();
        TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
        if (pT) {
            scriptNames << pT->getName();
            scriptsToExport << pT;
        }
    }

    if (scriptsToExport.isEmpty()) {
        return;
    }

    if (scriptsToExport.size() == 1) {
        XMLexport writer(scriptsToExport.first());
        writer.exportToClipboard(scriptsToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(scriptNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleScriptsToClipboard(scriptsToExport);
        statusBar()->showMessage(tr("Copied %1 scripts to clipboard").arg(scriptsToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleScriptsToClipboard(const QList<TScript*>& scripts)
{
    if (scripts.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TScript* script : scripts) {
        XMLexport writer(script);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(script);
        QString scriptXml = QApplication::clipboard()->text();
        xmlParts << scriptXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::exportKeyToClipboard()
{
    QList<QTreeWidgetItem*> selectedItems = treeWidget_keys->selectedItems();
    QStringList keyNames;
    QList<TKey*> keysToExport;

    for (const QTreeWidgetItem* pItem : std::as_const(selectedItems)) {
        const int keyID = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(keyID);
        if (pT) {
            keyNames << pT->getName();
            keysToExport << pT;
        }
    }

    if (keysToExport.isEmpty()) {
        return;
    }

    if (keysToExport.size() == 1) {
        XMLexport writer(keysToExport.first());
        writer.exportToClipboard(keysToExport.first());
        statusBar()->showMessage(tr("Copied %1 to clipboard").arg(keyNames.first().toHtmlEscaped()), 2000);
    } else {
        exportMultipleKeysToClipboard(keysToExport);
        statusBar()->showMessage(tr("Copied %1 keys to clipboard").arg(keysToExport.size()), 2000);
    }
}

void dlgTriggerEditor::exportMultipleKeysToClipboard(const QList<TKey*>& keys)
{
    if (keys.isEmpty()) {
        return;
    }

    QStringList xmlParts;

    for (TKey* key : keys) {
        XMLexport writer(key);
        QString originalClipboard = QApplication::clipboard()->text();
        writer.exportToClipboard(key);
        QString keyXml = QApplication::clipboard()->text();
        xmlParts << keyXml;
        QApplication::clipboard()->setText(originalClipboard);
    }

    QString combinedXml = xmlParts.join(cMultiItemPasteSeparator);
    QApplication::clipboard()->setText(combinedXml);
}

void dlgTriggerEditor::slot_export()
{
    const EditorViewType currentView = resolveCurrentView();
    if (currentView == EditorViewType::cmUnknownView || currentView == EditorViewType::cmVarsView) {
        return;
    }

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Item"), lastDir, tr("Mudlet packages (*.xml)"));
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
        fileName.append(qsl(".xml"));
    }


    QFile checkWriteability(fileName);
    if (!checkWriteability.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("export package:"), tr("Cannot write file %1:\n%2.").arg(fileName.toHtmlEscaped(), checkWriteability.errorString()));
        return;
    }
    // Should close the checkWriteability that we have confirmed can be opened:
    checkWriteability.close();

    switch (currentView) {
    case EditorViewType::cmTriggerView:
        exportTrigger(fileName);
        break;
    case EditorViewType::cmTimerView:
        exportTimer(fileName);
        break;
    case EditorViewType::cmAliasView:
        exportAlias(fileName);
        break;
    case EditorViewType::cmScriptView:
        exportScript(fileName);
        break;
    case EditorViewType::cmActionView:
        exportAction(fileName);
        break;
    case EditorViewType::cmKeysView:
        exportKey(fileName);
        break;
    case EditorViewType::cmVarsView:
        [[fallthrough]];
    case EditorViewType::cmUnknownView:
        // These two have already been handled so this place in the code should
        // indeed be:
        Q_UNREACHABLE();
    }
}

void dlgTriggerEditor::slot_createModule()
{
    const EditorViewType currentView = resolveCurrentView();
    if (currentView == EditorViewType::cmUnknownView || currentView == EditorViewType::cmVarsView) {
        return;
    }

    // Open the package exporter dialog with module creation mode
    auto* packageExporter = new dlgPackageExporter(this, mpHost);
    packageExporter->setAttribute(Qt::WA_DeleteOnClose);

    // Pre-select the current item for export
    switch (currentView) {
    case EditorViewType::cmTriggerView:
        if (mpCurrentTriggerItem) {
            packageExporter->preselectTrigger(mpCurrentTriggerItem);
        }
        break;
    case EditorViewType::cmTimerView:
        if (mpCurrentTimerItem) {
            packageExporter->preselectTimer(mpCurrentTimerItem);
        }
        break;
    case EditorViewType::cmAliasView:
        if (mpCurrentAliasItem) {
            packageExporter->preselectAlias(mpCurrentAliasItem);
        }
        break;
    case EditorViewType::cmScriptView:
        if (mpCurrentScriptItem) {
            packageExporter->preselectScript(mpCurrentScriptItem);
        }
        break;
    case EditorViewType::cmActionView:
        if (mpCurrentActionItem) {
            packageExporter->preselectAction(mpCurrentActionItem);
        }
        break;
    case EditorViewType::cmKeysView:
        if (mpCurrentKeyItem) {
            packageExporter->preselectKey(mpCurrentKeyItem);
        }
        break;
    default:
        break;
    }

    // Set module creation mode
    packageExporter->setModuleCreationMode(true);
    packageExporter->show();
}

// With no valid item selected the *ToClipboard() callees return without a word, so a stray
// Ctrl+C raises no modal box; the Export action keeps its warning, as it was asked for a file
void dlgTriggerEditor::slot_copyXml()
{
    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        exportTriggerToClipboard();
        break;
    case EditorViewType::cmTimerView:
        exportTimerToClipboard();
        break;
    case EditorViewType::cmAliasView:
        exportAliasToClipboard();
        break;
    case EditorViewType::cmScriptView:
        exportScriptToClipboard();
        break;
    case EditorViewType::cmActionView:
        exportActionToClipboard();
        break;
    case EditorViewType::cmKeysView:
        exportKeyToClipboard();
        break;
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_copyXml() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_copyXml() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }
}

// FIXME: The switch cases in here need to handle EditorViewType::cmVarsView but how is not clear
// The import leaves pasted items at the bottom of the list; move them to just
// beneath the selected item, beside it even when it is a folder.
void dlgTriggerEditor::placePastedItems(EditorViewType itemType, const QList<int>& itemIDs)
{
    // The units' sibling lists also hold temporary items the tree does not show,
    // so a tree row is not a position in them - find the selected item itself
    const auto positionAfter = [](auto* pTarget, const auto& rootList) {
        const auto indexAfter = [pTarget](const auto& siblings) {
            int position = 1;
            for (const auto* pSibling : siblings) {
                if (pSibling == pTarget) {
                    return position;
                }
                ++position;
            }
            return -1;
        };
        if (!pTarget) {
            return -1;
        }
        return pTarget->getParent() ? indexAfter(*pTarget->getParent()->getChildrenList()) : indexAfter(rootList);
    };

    QTreeWidget* targetTree = nullptr;
    std::function<int(int)> positionAfterItem;
    std::function<void(int, int, TreeItemInsertMode, int)> reParentItem;

    switch (itemType) {
    case EditorViewType::cmTriggerView:
        targetTree = treeWidget_triggers;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getTriggerUnit();
            return positionAfter(pUnit->getTrigger(id), pUnit->getTriggerRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getTriggerUnit()->reParentTrigger(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmTimerView:
        targetTree = treeWidget_timers;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getTimerUnit();
            return positionAfter(pUnit->getTimer(id), pUnit->getTimerRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getTimerUnit()->reParentTimer(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmAliasView:
        targetTree = treeWidget_aliases;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getAliasUnit();
            return positionAfter(pUnit->getAlias(id), pUnit->getAliasRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getAliasUnit()->reParentAlias(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmScriptView:
        targetTree = treeWidget_scripts;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getScriptUnit();
            return positionAfter(pUnit->getScript(id), pUnit->getScriptRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getScriptUnit()->reParentScript(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmActionView:
        targetTree = treeWidget_actions;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getActionUnit();
            return positionAfter(pUnit->getAction(id), pUnit->getActionRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getActionUnit()->reParentAction(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmKeysView:
        targetTree = treeWidget_keys;
        positionAfterItem = [this, positionAfter](int id) {
            auto* pUnit = mpHost->getKeyUnit();
            return positionAfter(pUnit->getKey(id), pUnit->getKeyRootNodeList());
        };
        reParentItem = [this](int id, int parentId, TreeItemInsertMode mode, int position) {
            mpHost->getKeyUnit()->reParentKey(id, 0, parentId, mode, position);
        };
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        return;
    }

    QModelIndex targetIndex = targetTree->currentIndex();
    if (!targetIndex.isValid()) {
        QList<QTreeWidgetItem*> selectedItems = targetTree->selectedItems();
        if (!selectedItems.isEmpty()) {
            targetIndex = targetTree->indexFromItem(selectedItems.first());
        }
    }
    if (!targetIndex.isValid()) {
        return;
    }

    const int parentId = targetIndex.parent().data(Qt::UserRole).toInt();
    int position = positionAfterItem(targetIndex.data(Qt::UserRole).toInt());
    for (const int itemID : itemIDs) {
        if (position == -1) {
            reParentItem(itemID, parentId, TreeItemInsertMode::Append, 0);
        } else {
            reParentItem(itemID, parentId, TreeItemInsertMode::AtPosition, position++);
        }
    }
}

void dlgTriggerEditor::slot_pasteXml()
{
    XMLimport reader(mpHost);

    switch (resolveCurrentView()) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 1 not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 1 not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }

    // Check if clipboard contains multiple items (separated by our delimiter)
    QString clipboardText = QApplication::clipboard()->text();
    QStringList xmlPackages = clipboardText.split(cMultiItemPasteSeparator);

    EditorViewType importedItemType;
    int importedItemID;
    QList<int> importedIDs;

    if (xmlPackages.size() > 1) {
        EditorViewType firstImportType = EditorViewType::cmUnknownView;

        QString originalClipboard = QApplication::clipboard()->text();

        for (const QString& xmlItem : std::as_const(xmlPackages)) {
            QString xmlItemTrimmed = xmlItem.trimmed();
            if (xmlItemTrimmed.isEmpty()) {
                continue; // Skip empty items
            }

            // Temporarily set clipboard to single item
            QApplication::clipboard()->setText(xmlItemTrimmed);

            // Import this single item
            XMLimport itemReader(mpHost);
            auto [itemType, itemID] = itemReader.importFromClipboard();

            if (itemType != EditorViewType::cmUnknownView && itemID != 0) {
                importedIDs << itemID;
                if (firstImportType == EditorViewType::cmUnknownView) {
                    firstImportType = itemType;
                }
            }
        }

        // Restore original clipboard once at the end
        QApplication::clipboard()->setText(originalClipboard);

        if (!importedIDs.isEmpty()) {
            // For multiple items, we need to handle the reparenting here instead of later
            // since the later logic only handles one item at a time
            placePastedItems(firstImportType, importedIDs);

            // Use the first imported item's type and ID for the rest of the function
            importedItemType = firstImportType;
            importedItemID = importedIDs.first();

            statusBar()->showMessage(tr("Pasted %1 items successfully").arg(importedIDs.size()), 3000);
        } else {
            // No items were imported - don't create undo action
            return;
        }
    } else {
        // Single item - use original import method
        auto [itemType, itemID] = reader.importFromClipboard();
        importedItemType = itemType;
        importedItemID = itemID;

        // don't reset the view if what we pasted wasn't a Mudlet editor item
        if (importedItemType == EditorViewType::cmUnknownView && importedItemID == 0) {
            // No valid item was imported - don't create undo action
            return;
        }
    }

    if (mpUndoStack) {
        //: Undo/redo text for pasting items
        mpUndoStack->beginMacro(tr("paste"));
    }

    mCurrentView = static_cast<EditorViewType>(importedItemType);
    // multi-item pastes were already placed above; placing the first again would
    // link it into its parent's children twice
    if (xmlPackages.size() == 1) {
        placePastedItems(importedItemType, {importedItemID});
    }

    // Register undo commands for the pasted items
    if (mpUndoStack) {
        // Helper lambda to calculate position within parent's children
        auto calculatePositionInParent = [](auto* item) -> int {
            if (!item) {
                return 0;
            }
            auto* parent = item->getParent();
            if (!parent) {
                return 0;
            }
            auto* childrenList = parent->getChildrenList();
            if (!childrenList) {
                return 0;
            }
            int position = 0;
            for (auto* child : *childrenList) {
                if (child == item) {
                    return position;
                }
                position++;
            }
            return 0;
        };

        // Helper lambda to register an undo command for a single pasted item
        auto registerUndoCommand = [&](EditorViewType viewType, int itemID) {
            QString itemName;
            int parentID = -1;
            int positionInParent = 0;
            bool isFolder = false;

            switch (viewType) {
            case EditorViewType::cmTriggerView: {
                TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(itemID);
                if (pT) {
                    itemName = pT->getName();
                    isFolder = pT->isFolder();
                    auto* parent = pT->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pT);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getTriggerUnit()->getTriggerRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pT) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            case EditorViewType::cmTimerView: {
                TTimer* pT = mpHost->getTimerUnit()->getTimer(itemID);
                if (pT) {
                    itemName = pT->getName();
                    isFolder = pT->isFolder();
                    auto* parent = pT->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pT);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getTimerUnit()->getTimerRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pT) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            case EditorViewType::cmAliasView: {
                TAlias* pA = mpHost->getAliasUnit()->getAlias(itemID);
                if (pA) {
                    itemName = pA->getName();
                    isFolder = pA->isFolder();
                    auto* parent = pA->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pA);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getAliasUnit()->getAliasRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pA) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            case EditorViewType::cmScriptView: {
                TScript* pS = mpHost->getScriptUnit()->getScript(itemID);
                if (pS) {
                    itemName = pS->getName();
                    isFolder = pS->isFolder();
                    auto* parent = pS->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pS);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getScriptUnit()->getScriptRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pS) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            case EditorViewType::cmActionView: {
                TAction* pA = mpHost->getActionUnit()->getAction(itemID);
                if (pA) {
                    itemName = pA->getName();
                    isFolder = pA->isFolder();
                    auto* parent = pA->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pA);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getActionUnit()->getActionRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pA) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            case EditorViewType::cmKeysView: {
                TKey* pK = mpHost->getKeyUnit()->getKey(itemID);
                if (pK) {
                    itemName = pK->getName();
                    isFolder = pK->isFolder();
                    auto* parent = pK->getParent();
                    if (parent) {
                        parentID = parent->getID();
                        positionInParent = calculatePositionInParent(pK);
                    } else {
                        parentID = -1;
                        auto rootList = mpHost->getKeyUnit()->getKeyRootNodeList();
                        int pos = 0;
                        for (auto* rootItem : rootList) {
                            if (rootItem == pK) {
                                positionInParent = pos;
                                break;
                            }
                            pos++;
                        }
                    }
                }
                break;
            }
            default:
                return;
            }

            if (!itemName.isEmpty()) {
                auto* qtCmd = new EditorAddItemCommand(viewType, itemID, parentID, positionInParent, isFolder, itemName, mpHost);
                mpUndoStack->pushCommand(qtCmd);
            }
        };

        // Register undo commands for all imported items
        if (xmlPackages.size() > 1) {
            // Multiple items were pasted
            if (!importedIDs.isEmpty()) {
                for (const int itemID : std::as_const(importedIDs)) {
                    registerUndoCommand(importedItemType, itemID);
                }
            }
        } else {
            // Single item was pasted
            registerUndoCommand(importedItemType, importedItemID);
        }
    }

    // flag for re-rendering so the new item shows up in the right spot
    mNeedUpdateData = true;

    switch (importedItemType) {
    case EditorViewType::cmTriggerView: {
        // selecting the pasted item unfolds its ancestors, which would animate
        // and doesn't look nice - so turn off animation temporarily
        auto animated = treeWidget_triggers->isAnimated();
        treeWidget_triggers->setAnimated(false);
        selectTriggerByID(importedItemID);
        treeWidget_triggers->setAnimated(animated);

        // set the focus because hiding checkBox_displayAllVariables in changeView
        // changes the focus to the search box for some reason. This thus breaks
        // successive pastes because you'll now be pasting into the search box
        focusPanelTree(treeWidget_triggers);
        break;
    }
    case EditorViewType::cmTimerView: {
        auto animated = treeWidget_timers->isAnimated();
        treeWidget_timers->setAnimated(false);
        selectTimerByID(importedItemID);
        treeWidget_timers->setAnimated(animated);
        focusPanelTree(treeWidget_timers);
        break;
    }
    case EditorViewType::cmAliasView: {
        auto animated = treeWidget_aliases->isAnimated();
        treeWidget_aliases->setAnimated(false);
        selectAliasByID(importedItemID);
        treeWidget_aliases->setAnimated(animated);
        focusPanelTree(treeWidget_aliases);
        break;
    }
    case EditorViewType::cmScriptView: {
        auto animated = treeWidget_scripts->isAnimated();
        treeWidget_scripts->setAnimated(false);
        selectScriptByID(importedItemID);
        treeWidget_scripts->setAnimated(animated);
        focusPanelTree(treeWidget_scripts);
        break;
    }
    case EditorViewType::cmActionView: {
        auto animated = treeWidget_actions->isAnimated();
        treeWidget_actions->setAnimated(false);
        selectActionByID(importedItemID);
        treeWidget_actions->setAnimated(animated);
        focusPanelTree(treeWidget_actions);
        break;
    }
    case EditorViewType::cmKeysView: {
        auto animated = treeWidget_keys->isAnimated();
        treeWidget_keys->setAnimated(false);
        selectKeyByID(importedItemID);
        treeWidget_keys->setAnimated(animated);
        focusPanelTree(treeWidget_keys);
        break;
    }
    case EditorViewType::cmVarsView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 3 not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::slot_pasteXml() WARNING - switch(EditorViewType) number 3 not expected to be called for \"EditorViewType::cmUnknownView!\"";
        break;
    }

    if (mpUndoStack) {
        mpUndoStack->endMacro();
    }
}

void dlgTriggerEditor::slot_import()
{
    // The item behind a merged folder was saved when the folder was chosen, and its errors must stay hidden
    if (mFolderGroupKey.isEmpty()) {
        switch (mCurrentView) {
        case EditorViewType::cmTriggerView:
            saveTrigger();
            break;
        case EditorViewType::cmTimerView:
            saveTimer();
            break;
        case EditorViewType::cmAliasView:
            saveAlias();
            break;
        case EditorViewType::cmScriptView:
            saveScript();
            break;
        case EditorViewType::cmActionView:
            saveAction();
            break;
        case EditorViewType::cmKeysView:
            saveKey();
            break;
        case EditorViewType::cmVarsView:
            qWarning().nospace().noquote() << "dlgTriggerEditor::slot_import() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
            break;
        case EditorViewType::cmUnknownView:
            qWarning().nospace().noquote() << "dlgTriggerEditor::slot_import() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
        }
    }

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value(qsl("lastFileDialogLocation"), QDir::homePath()).toString();
    //: Trigger editor - import packages from file dialog (multi-select enabled)
    //: Trigger editor - file filter for supported package types (mpackage, zip, xml)
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
            qWarning() << "dlgTriggerEditor::slot_import() ERROR - failed to import" << baseName;
        }
    }

    clearTypeTrees();

    // Nullify current item pointers before saving to prevent use-after-free
    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

    slot_profileSaveAction();

    fillout_form();

    slot_showTriggers();

    if (!failedPackages.isEmpty()) {
        //: Trigger editor - status message shown when some packages failed to import. %1 is a comma-separated list of package names
        statusBar()->showMessage(tr("Failed to import: %1").arg(failedPackages.join(qsl(", "))), std::chrono::milliseconds(4s).count());
    }
}

void dlgTriggerEditor::doCleanReset()
{
    if (mCleanResetQueued) {
        return;
    }

    mCleanResetQueued = true;

    QTimer::singleShot(0ms, this, [=, this]() {
        mCleanResetQueued = false;

        runScheduledCleanReset();
    });
}

void dlgTriggerEditor::runScheduledCleanReset()
{
    if (!mpHost) {
        // The profile went away between doCleanReset() scheduling this and the timer firing,
        // which is the order a teardown destroys them in. There is nothing left to repopulate
        // from, and clearing the tree widgets below would re-enter the editor through
        // selectionChanged to read the theme and font off the Host that has just gone.
        return;
    }

    // Populating costs as much as the profile has items, so it waits for the editor to be seen;
    // changeView(), focus and enter events rebuild on mNeedUpdateData, showEvent() on the flag
    if (!mHasBeenShown) {
        mCleanResetDeferred = true;
        mNeedUpdateData = true;
        // What the build would have done to notices posted so far
        clearEditorNotification();
        return;
    }

    resetTrees();
}

void dlgTriggerEditor::resetTrees()
{
    // Clear all current item pointers BEFORE attempting to save or clear tree widgets
    // to prevent heap-use-after-free when the tree widgets are cleared
    mpCurrentTriggerItem = nullptr;
    mpCurrentTimerItem = nullptr;
    mpCurrentAliasItem = nullptr;
    mpCurrentScriptItem = nullptr;
    mpCurrentActionItem = nullptr;
    mpCurrentKeyItem = nullptr;

    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        saveTrigger();
        break;
    case EditorViewType::cmTimerView:
        saveTimer();
        break;
    case EditorViewType::cmAliasView:
        saveAlias();
        break;
    case EditorViewType::cmScriptView:
        saveScript();
        break;
    case EditorViewType::cmActionView:
        saveAction();
        break;
    case EditorViewType::cmKeysView:
        saveKey();
        break;
    case EditorViewType::cmVarsView:
        // FIXME: The switch in here need to handle (or at least treat correctly) the
        // EditorViewType:cmVarsView case but how is not clear:
        qWarning().nospace().noquote() << "dlgTriggerEditor::runScheduledCleanReset() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmVarsView!\"";
        break;
    case EditorViewType::cmUnknownView:
        qWarning().nospace().noquote() << "dlgTriggerEditor::runScheduledCleanReset() WARNING - switch(EditorViewType) not expected to be called for \"EditorViewType::cmUnknownView!\"";
    }

    clearTypeTrees();
    fillout_form();
    slot_showTriggers();
}

// A notice posted since the build was put off has to outlive it, as populating a tree clears the message
// area for every item it adds
void dlgTriggerEditor::buildDeferredTrees(const bool asAtLoad)
{
    const bool keepNotice = !mpSystemMessageArea->isHidden();
    const bool info = !mpSystemMessageArea->notificationAreaIconLabelInformation->isHidden();
    const bool error = !mpSystemMessageArea->notificationAreaIconLabelError->isHidden();
    const bool warning = !mpSystemMessageArea->notificationAreaIconLabelWarning->isHidden();
    const QString text = mpSystemMessageArea->notificationAreaMessageBox->text();
    const QString bannerKey = mCurrentBannerKey;

    if (asAtLoad) {
        resetTrees();
    } else {
        fillout_form();
    }

    if (!keepNotice) {
        return;
    }
    mpSystemMessageArea->notificationAreaIconLabelInformation->setVisible(info);
    mpSystemMessageArea->notificationAreaIconLabelError->setVisible(error);
    mpSystemMessageArea->notificationAreaIconLabelWarning->setVisible(warning);
    mpSystemMessageArea->notificationAreaMessageBox->setText(text);
    mpSystemMessageArea->show();
    mCurrentBannerKey = bannerKey;
    if (bannerKey.isEmpty()) {
        disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked);
        connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea, Qt::UniqueConnection);
    } else {
        disconnect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::hideSystemMessageArea);
        connect(mpSystemMessageArea->messageAreaCloseButton, &QAbstractButton::clicked, this, &dlgTriggerEditor::slot_bannerDismissClicked, Qt::UniqueConnection);
    }
}

void dlgTriggerEditor::slot_profileSaveAction()
{
    // saveProfile() calls qApp->processEvents() while a save is in progress,
    // which can re-enter here via a pending doCleanReset timer. The ongoing
    // save already covers current state, so skip the redundant call.
    if (mpHost->currentlySavingProfile()) {
        return;
    }

    slot_saveEdits();

    auto [ok, filename, error] = mpHost->saveProfile(QString(), QString(), true);

    if (ok) {
        // Most application-wide settings reach Mudlet.ini only at quit, so an
        // unclean exit would lose the ones this save is meant to keep
        mudlet::self()->writeSettings();
    } else if (!error.isEmpty()) {
        QMessageBox::critical(this, tr("Couldn't save profile"), tr("Sorry, couldn't save your profile - got the following error: %1").arg(error));
    }
}

void dlgTriggerEditor::slot_profileSaveAsAction()
{
    mSavingAs = true;

    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Backup Profile"), lastDir, tr("trigger files (*.trigger *.xml)"));

    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    // Must be case insensitive to work on MacOS platforms, possibly a cause of
    // https://bugs.launchpad.net/mudlet/+bug/1417234
    if (!fileName.endsWith(qsl(".xml"), Qt::CaseInsensitive) && !fileName.endsWith(qsl(".trigger"), Qt::CaseInsensitive)) {
        fileName.append(qsl(".xml"));
    }
    slot_saveEdits();

    auto [ok, filename, error] = mpHost->saveProfileAs(fileName);
    if (ok) {
        mudlet::self()->writeSettings();
    }
    mSavingAs = false;
    if (!ok && !error.isEmpty()) {
        //: %1 is the reason the profile could not be saved, such as an error from the operating system
        const QString message = tr("Sorry, couldn't save your profile - got the following error: %1").arg(error);
        QMessageBox::critical(this, tr("Couldn't save profile"), message);
    }
}

bool dlgTriggerEditor::eventFilter(QObject* watched, QEvent* event)
{
    // Before the key grab below, which swallows every event that is not a key while a key is grabbed
    if (watched == toolBar && event->type() == QEvent::Resize) {
        fitEditorToolBarToItsLength();
        return false;
    }

    // Cut to the label's own width, not the strip's: the caret reading beside it widens with every digit.
    // No resize loop: QLabel::setText() does nothing for unchanged text, and the label asks for no width.
    if (watched == mpLabel_editorCompileMessage && event->type() == QEvent::Resize) {
        elideCompileNote();
    }

    if (watched == mpWidget_editorCompileNote && event->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent*>(event)->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            jumpToCompileErrorLine();
            return true;
        default:
            break;
        }
    }

    // Watched with no grab armed too, since a click on it is what arms one. While armed this filter is also on
    // the application, and both passes name the field as `watched`.
    if (mpTriggersMainArea && watched == mpTriggersMainArea->lineEdit_soundFile && handleSoundFileFieldEvent(event)) {
        return true;
    }

    if (mpKeysMainArea && watched == mpKeysMainArea->lineEdit_key_binding && handleKeyBindingFieldEvent(event)) {
        return true;
    }

    if (mIsGrabKey) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Escape:
                this->event(event);
                return true;
            default:
                return false;
            }
        }
        return false;
    }

    if (handlePatternHandleEvent(watched, event)) {
        return true;
    }

    // No caret while the pane is away. On return edbee reports again rather than the old reading being put
    // back: the document under the pane has changed by then.
    if (watched == mpSourceEditorArea) {
        if (event->type() == QEvent::Hide) {
            QMainWindow::statusBar()->clearMessage();
        } else if (event->type() == QEvent::Show && mpSourceEditorEdbee) {
            mpSourceEditorEdbee->controller()->updateStatusText();
        }
    }

    // Any change to the column's height (a notice, an item chosen, a form row shown or hidden) arrives as this
    if (watched == mpNonCodeWidgets && event->type() == QEvent::LayoutRequest) {
        applyFormPaneSeamPolicy();
    }

    // The form's width decides how many lines its options strip wraps onto, so only a width change refits.
    // No loop: the refit moves the seam, which changes the form's height and never its width.
    if (watched == mpTriggersMainArea && event->type() == QEvent::Resize) {
        const auto* pResize = static_cast<QResizeEvent*>(event);
        if (pResize->size().width() != pResize->oldSize().width()) {
            queueFormPaneRefit();
        }
    }

    // QSplitter gives a change in its own height to the code pane alone (the stretch factors), which would keep a
    // long pattern list scrolling above a code pane with room to spare. No loop: the refit never changes that height.
    if (watched == splitter_right && event->type() == QEvent::Resize) {
        const auto* pResize = static_cast<QResizeEvent*>(event);
        if (pResize->size().height() != pResize->oldSize().height()) {
            queueFormPaneRefit();
        }
    }

    // Styling the rows drops the native focus rectangle, and a QSS rule cannot ask whether the widget a
    // subcontrol belongs to has the focus
    if (watched == mpListWidget_editorSidebar && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
        mpListWidget_editorSidebar->setProperty(uiDesign::scmProp_focused, event->type() == QEvent::FocusIn);
        mpListWidget_editorSidebar->style()->polish(mpListWidget_editorSidebar);
    }

    // While the history popup is open Escape is its; a style that lets it through would otherwise close the
    // list and clear the search in one keystroke
    if (event->type() == QEvent::KeyPress && comboBox_searchTerms && watched == comboBox_searchTerms->lineEdit() && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
        if (comboBox_searchTerms->view() && comboBox_searchTerms->view()->isVisible()) {
            return false;
        }
        // The field's textChanged() handler takes the results down
        comboBox_searchTerms->lineEdit()->clear();
        return true;
    }

    // A pattern field takes no line break, so Return in the last one is free to add the next pattern
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const bool plainReturn =
                (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && !(keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier));
        if (plainReturn) {
            if (auto* edit = qobject_cast<SingleLineTextEdit*>(watched)) {
                auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(edit->parentWidget());
                if (patternItem && patternItem->mRow == mVisiblePatternCount - 1 && mVisiblePatternCount < scmEditorPatternRowLimit) {
                    slot_addPattern();
                    return true;
                }
            }
        }
    }

    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        const Qt::KeyboardModifiers additionalModifiers = modifiers & (Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier | Qt::GroupSwitchModifier | Qt::KeypadModifier);
        if (modifiers.testFlag(Qt::ControlModifier) && additionalModifiers == Qt::NoModifier) {
            if (auto* edit = qobject_cast<SingleLineTextEdit*>(watched)) {
                auto* patternItem = qobject_cast<dlgTriggerPatternEdit*>(edit->parentWidget());
                if (keyEvent->key() == Qt::Key_Down) {
                    if (focusNextPatternItem(patternItem)) {
                        return true;
                    }
                } else if (keyEvent->key() == Qt::Key_Up) {
                    if (focusPreviousPatternItem(patternItem)) {
                        return true;
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

bool dlgTriggerEditor::event(QEvent* event)
{
    if (mIsGrabKey) {
        if (event->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);
            switch (ke->key()) {
            case Qt::Key_Escape:
                endKeyGrab();
                ke->accept();
                return true;

            case Qt::Key_Shift:
                [[fallthrough]];
            case Qt::Key_Control:
                [[fallthrough]];
            case Qt::Key_Meta:
                [[fallthrough]];
            case Qt::Key_Alt:
                [[fallthrough]];
            case Qt::Key_AltGr:
                break;

            default:
                keyGrabCallback(static_cast<Qt::Key>(ke->key()), static_cast<Qt::KeyboardModifiers>(ke->modifiers()));
                endKeyGrab();
                ke->accept();
                return true;
            }
        }
    }

    if (event->type() == QEvent::ApplicationFontChange) {
        // Handling this re-resolves the window's font; the lift carries it past the stylesheets
        const uiDesign::StyleSheetFontLift lift(this);
        return QMainWindow::event(event);
    }

    return QMainWindow::event(event);
}

void dlgTriggerEditor::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    if (mpSourceEditorArea->isVisible()) {
        slot_sourceFindMove();
    }
    updateEditorSidebarMode();
}

void dlgTriggerEditor::slot_keyGrab()
{
    if (mIsGrabKey) {
        return;
    }
    mIsGrabKey = true;
    suspendShortcuts();
    QCoreApplication::instance()->installEventFilter(this);
    slot_itemEdited();
    showKeyBindingListening();
}

// The view switchers are not on the toolbar, but a key grab must silence their shortcuts too
QList<QAction*> dlgTriggerEditor::toolbarActions() const
{
    return toolBar->actions() + mEditorViewActions;
}

// So a keystroke like Ctrl+S reaches the key grab rather than the action
void dlgTriggerEditor::suspendShortcuts()
{
    for (auto* action : toolbarActions()) {
        // A second arm finds every shortcut already empty, so the first grab's record stands
        if (const QList<QKeySequence> shortcuts = action->shortcuts(); !shortcuts.isEmpty()) {
            mSuspendedShortcuts.insert(action, shortcuts);
            action->setShortcut(QKeySequence());
        }
    }
    mpShortcutActivateMainWindow->setEnabled(false);
}

void dlgTriggerEditor::restoreShortcuts()
{
    for (auto* action : toolbarActions()) {
        if (const auto it = mSuspendedShortcuts.constFind(action); it != mSuspendedShortcuts.constEnd()) {
            action->setShortcuts(it.value());
        }
    }
    mSuspendedShortcuts.clear();
    mpShortcutActivateMainWindow->setEnabled(true);
}

QString dlgTriggerEditor::unboundKeyText() const
{
    //: Stands in the key binding field for a key that answers no keystroke yet, and names such a key
    return tr("No key chosen");
}

void dlgTriggerEditor::keyGrabCallback(const Qt::Key key, const Qt::KeyboardModifiers modifier)
{
    KeyUnit* pKeyUnit = mpHost->getKeyUnit();
    if (!pKeyUnit) {
        return;
    }
    const QString keyName = pKeyUnit->getKeyName(key, modifier);
    QLineEdit* pBindingField = mpKeysMainArea->lineEdit_key_binding;
    pBindingField->setText(keyName);
    QTreeWidgetItem* pItem = treeWidget_keys->currentItem();
    if (pItem) {
        const int keyID = pItem->data(0, Qt::UserRole).toInt();
        TKey* pT = mpHost->getKeyUnit()->getKey(keyID);
        if (pT) {
            if (pT->getKeyCode() == key && pT->getKeyModifiers() == modifier) {
                return;
            }

            QString oldStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);
            const QString typedName = mpKeysMainArea->lineEdit_key_name->text().trimmed();
            // From the key, not the field, which an armed grab has emptied. An unbound key was named after the field's
            // empty wording, or before the redesign after KeyUnit's.
            const bool unbound = pT->getKeyCode() == Qt::Key_unknown;
            const bool namedAfterBinding = unbound ? typedName == unboundKeyText() || typedName == pKeyUnit->getKeyName(Qt::Key_unknown, Qt::NoModifier)
                                                   : typedName == pKeyUnit->getKeyName(pT->getKeyCode(), pT->getKeyModifiers());
            // A saved key's scripts may find it by a name that happens to match its binding
            const bool nameIsOldBinding = namedAfterBinding && (unbound || mpUndoStack->addingItem(EditorViewType::cmKeysView, keyID));
            pT->setKeyCode(key);
            pT->setKeyModifiers(modifier);
            if (pItem == mpCurrentKeyItem && !pT->isFolder() && (typedName.isEmpty() || typedName == tr("New key") || nameIsOldBinding)) {
                pT->setName(keyName);
                pItem->setText(0, keyName);
                mpKeysMainArea->lineEdit_key_name->setText(keyName);
            }
            QString newStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);

            showKeyTakenWarning(pItem, takenKeyWarning(mpHost, pT), true);

            pushKeyPropertyCommand(mpUndoStack, mpHost, keyID, pT->getName(), qsl("keyBinding"), oldStateXML, newStateXML);
        }
    }
}

void dlgTriggerEditor::slot_toggleIsPushDownButton(const int state)
{
    showEditorFormRow(state == Qt::Checked, {mpActionsMainArea->label_action_button_command_up, mpActionsMainArea->lineEdit_action_button_command_up});
}

// Set the foreground color that will be applied to text that matches the trigger pattern(s)
void dlgTriggerEditor::slot_colorizeTriggerSetFgColor()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    if (!pItem->parent()) {
        return;
    }

    const QColor initialColor(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString());

    QColorDialog dialog(initialColor, this);
    dialog.setWindowTitle(tr("Select foreground color to apply to matches"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);

    bool keepColorClicked = false;
    auto* buttonBox = dialog.findChild<QDialogButtonBox*>();
    if (buttonBox) {
        //: Button in the color picker that preserves the existing text color on trigger matches
        auto* keepButton = buttonBox->addButton(tr("Keep color"), QDialogButtonBox::ActionRole);
        connect(keepButton, &QPushButton::clicked, &dialog, [&keepColorClicked, &dialog]() {
            keepColorClicked = true;
            dialog.accept();
        });
    }

    dialog.exec();

    if (keepColorClicked) {
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(QColorConstants::Transparent));
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(tr("keep"));
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, qsl("transparent"));
    } else if (dialog.selectedColor().isValid()) {
        const auto color = dialog.selectedColor();
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(color));
        mpTriggersMainArea->pushButtonFgColor->setText(QString());
        mpTriggersMainArea->pushButtonFgColor->setProperty(cButtonBaseColor, color.name());
    } else {
        return;
    }
    describeColourWell(mpTriggersMainArea->pushButtonFgColor);
    turnTriggerHighlightOn();
}

// Set the background color that will be applied to text that matches the trigger pattern(s)
void dlgTriggerEditor::slot_colorizeTriggerSetBgColor()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    if (!pItem->parent()) {
        return;
    }

    const QColor initialColor(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString());

    QColorDialog dialog(initialColor, this);
    dialog.setWindowTitle(tr("Select background color to apply to matches"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);

    bool keepColorClicked = false;
    auto* buttonBox = dialog.findChild<QDialogButtonBox*>();
    if (buttonBox) {
        //: Button in the color picker that preserves the existing text color on trigger matches
        auto* keepButton = buttonBox->addButton(tr("Keep color"), QDialogButtonBox::ActionRole);
        connect(keepButton, &QPushButton::clicked, &dialog, [&keepColorClicked, &dialog]() {
            keepColorClicked = true;
            dialog.accept();
        });
    }

    dialog.exec();

    if (keepColorClicked) {
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(QColorConstants::Transparent));
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(tr("keep"));
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, qsl("transparent"));
    } else if (dialog.selectedColor().isValid()) {
        const auto color = dialog.selectedColor();
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(color));
        mpTriggersMainArea->pushButtonBgColor->setText(QString());
        mpTriggersMainArea->pushButtonBgColor->setProperty(cButtonBaseColor, color.name());
    } else {
        return;
    }
    describeColourWell(mpTriggersMainArea->pushButtonBgColor);
    turnTriggerHighlightOn();
}

void dlgTriggerEditor::slot_soundTrigger()
{
    // Use the existing path/filename if it is not empty, otherwise start in last global user dir
    QSettings& settings = *MudletApp::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Choose sound file"),
            triggerSoundFilePath().isEmpty() ? lastDir : triggerSoundFilePath(),
            //: This the list of file extensions that are considered for sounds from triggers, the terms inside of the '('...')' and the ";;" are used programmatically and should not be changed.
            tr("Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;"
               "Advanced Audio Coding-stream(*.aac);;"
               "MPEG-2 Audio Layer 3(*.mp3);;"
               "MPEG-4 Audio(*.mp4a);;"
               "Ogg Vorbis(*.oga *.ogg);;"
               "PCM Audio(*.pcm);;"
               "Wave(*.wav);;"
               "Windows Media Audio(*.wma);;"
               "All files(*.*)"));
    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);
    acceptTriggerSoundFile(fileName);
}

// Two undo entries, in this order: the file, then the trigger playing it
void dlgTriggerEditor::acceptTriggerSoundFile(const QString& fileName)
{
    showTriggerSoundFile(fileName);
    // The field is never typed into, so nothing else tells the save path the file changed
    slot_saveProperty_TriggerSoundFile();
    // Choosing a file asks for it to be played; the switch's own save writes the second undo entry
    if (!mpTriggersMainArea->checkBox_soundTrigger->isChecked()) {
        mpTriggersMainArea->checkBox_soundTrigger->setChecked(true);
    }
    slot_itemEdited();
}

// Get the color from the user to use as that to look for as the foreground in
// a color trigger:
void dlgTriggerEditor::slot_colorTriggerFg()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto* pB = qobject_cast<QPushButton*>(sender());
    if (!pB) {
        return;
    }

    dlgTriggerPatternEdit* pPatternItem = qobject_cast<dlgTriggerPatternEdit*>(pB->parent());
    if (!pPatternItem) {
        return;
    }

    // This method parses the pattern text and extracts the ansi color values
    // from it - including the special values of DEFAULT (-2) and IGNORE (-1)
    // and assigns the values to the other arguments:
    TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, false, tr("Select foreground trigger color for item %1").arg(QString::number(pPatternItem->mRow + 1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // while we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();
    delete pD;

    const QColor color = pT->mColorTriggerFgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    // The dialogue should have changed pT->mColorTriggerFgAnsi
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerFgAnsi == TTrigger::scmIgnored) {
        //: Color trigger ignored foreground color button, ensure all three instances have the same text
        pB->setText(tr("Foreground color ignored"));
    } else if (pT->mColorTriggerFgAnsi == TTrigger::scmDefault) {
        //: Color trigger default foreground color button, ensure all three instances have the same text
        pB->setText(tr("Default foreground color"));
    } else {
        //: Color trigger ANSI foreground color button, ensure all three instances have the same text
        pB->setText(tr("Foreground color [ANSI %1]").arg(QString::number(pT->mColorTriggerFgAnsi)));
    }
}

// Get the color from the user to use as that to look for as the background in
// a color trigger:
void dlgTriggerEditor::slot_colorTriggerBg()
{
    QTreeWidgetItem* pItem = mpCurrentTriggerItem;
    if (!pItem) {
        return;
    }
    const int triggerID = pItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    auto* pB = qobject_cast<QPushButton*>(sender());
    if (!pB) {
        return;
    }

    dlgTriggerPatternEdit* pPatternItem = qobject_cast<dlgTriggerPatternEdit*>(pB->parent());
    if (!pPatternItem) {
        return;
    }

    // This method parses the pattern text and extracts the ansi color values
    // from it - including the special values of DEFAULT (-2) and IGNORE (-1)
    // and assigns the values to the other arguments:
    TTrigger::decodeColorPatternText(pPatternItem->singleLineTextEdit_pattern->toPlainText(), pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi);

    // The following method wants to know BOTH existing fore and backgrounds
    // it will select the appropriate as a result of the third argument and it
    // uses both to determine whether the result to return is valid considering
    // the other, non used (background in this method) part:
    auto pD = new dlgColorTrigger(this, pT, true, tr("Select background trigger color for item %1").arg(QString::number(pPatternItem->mRow + 1)));
    pD->setModal(true);
    // This sounds a bit iffy - prevent access to other application windows
    // while we get a colour setting:
    pD->setWindowModality(Qt::ApplicationModal);
    pD->exec();
    delete pD;

    const QColor color = pT->mColorTriggerBgColor;
    // The above will be an invalid colour if the colour has been reset/ignored
    QString styleSheet;
    if (color.isValid()) {
        styleSheet = generateButtonStyleSheet(color);
    }
    pB->setStyleSheet(styleSheet);

    pPatternItem->singleLineTextEdit_pattern->setPlainText(TTrigger::createColorPatternText(pT->mColorTriggerFgAnsi, pT->mColorTriggerBgAnsi));

    if (pT->mColorTriggerBgAnsi == TTrigger::scmIgnored) {
        //: Color trigger ignored background color button, ensure all three instances have the same text
        pB->setText(tr("Background color ignored"));
    } else if (pT->mColorTriggerBgAnsi == TTrigger::scmDefault) {
        //: Color trigger default background color button, ensure all three instances have the same text
        pB->setText(tr("Default background color"));
    } else {
        //: Color trigger ANSI background color button, ensure all three instances have the same text
        pB->setText(tr("Background color [ANSI %1]").arg(QString::number(pT->mColorTriggerBgAnsi)));
    }
}

void dlgTriggerEditor::slot_updateCaretPosition(const QString& statusText)
{
    // Loading a document into a hidden pane still moves the caret. isVisibleTo(), not isVisible(), so the
    // editor window not being shown yet does not hold the reading back.
    if (mpSourceEditorArea && !mpSourceEditorArea->isVisibleTo(this)) {
        return;
    }

    // edbee adds the scope and last command which is rather technical debugging information,
    // so strip it away by removing the first pipe and everything after it
    const QRegularExpressionMatch match = csmSimplifyCaretReportRegex.match(statusText, 0, QRegularExpression::PartialPreferFirstMatch);
    QString stripped;
    if (match.hasPartialMatch() || match.hasMatch()) {
        stripped = match.captured(1);
    } else {
        stripped = statusText;
    }

    // Not QStatusBar::showMessage(): an untimed message never comes down, and hideOrShow() only hides the
    // counts widget if it is already visible - the first report arrives before show, so they were painted over.
    if (mpLabel_editorCaretPosition) {
        mpLabel_editorCaretPosition->setText(stripped);
    }
}

void dlgTriggerEditor::slot_codeHeadingClicked(QWidget* pPiece)
{
    if (pPiece != mpWidget_editorCompileNote) {
        return;
    }
    jumpToCompileErrorLine();
}

void dlgTriggerEditor::jumpToCompileErrorLine()
{
    if (mEditorCompileErrorLine <= 0 || !mpSourceEditorEdbee) {
        return;
    }
    edbee::TextEditorController* pController = mpSourceEditorEdbee->controller();
    if (!pController || !pController->textDocument()) {
        return;
    }
    // Lua counts lines from one and edbee from zero; an unclosed block is reported against the wrapper's
    // trailing "end", one line past the item's last
    const size_t line = qMin(static_cast<size_t>(mEditorCompileErrorLine - 1), pController->textDocument()->lineCount() - 1);
    mpSourceEditorEdbee->setFocus();
    pController->moveCaretTo(line, static_cast<size_t>(0), false);
}

void dlgTriggerEditor::slot_profileSaveStarted()
{
    mProfileSaveAction->setDisabled(true);
    mProfileSaveAsAction->setDisabled(true);
    mProfileSaveAction->setText(tr("Saving…"));
}

void dlgTriggerEditor::slot_profileSaveFinished()
{
    mProfileSaveAction->setEnabled(true);
    mProfileSaveAsAction->setEnabled(true);
    mProfileSaveAction->setText(tr("Save Profile"));
}

void dlgTriggerEditor::slot_changeEditorTextOptions(QTextOption::Flags state)
{
    edbee::TextEditorConfig* config = mpSourceEditorEdbee->config();

    config->beginChanges();
    config->setShowWhitespaceMode((state & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(state & QTextOption::ShowLineAndParagraphSeparators);
    config->endChanges();
}

// clearDocument( edbee::TextEditorWidget* pEditorWidget)
//
// A temporary measure for dealing with the undo spanning over multiple documents bug,
// in place until we create a proper multi-document solution. This gets called whenever
// the editor needs to be "cleared", usually when a different alias/trigger/etc is
// made or selected.
void dlgTriggerEditor::clearDocument(edbee::TextEditorWidget* pEditorWidget, const QString& initialText)
{
    // Every item switch comes through here, so a failure reported against the item leaving does not follow
    // the one arriving
    clearCompileState();

    mpSourceEditorFindArea->hide();
    mpSourceEditorEdbeeDocument = new edbee::CharTextDocument();
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    // Buck.lua is a fake filename for edbee to figure out its lexer type with. Referencing the
    // lexer directly by name previously gave problems.
    // Don't apply Lua syntax highlighting for the Variables view since it displays plain data values, not code
    if (mCurrentView != EditorViewType::cmVarsView) {
        mpSourceEditorEdbeeDocument->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QLatin1String("Buck.lua")));
    }
    pEditorWidget->controller()->giveTextDocument(mpSourceEditorEdbeeDocument);

    // Update the text undo stack pointer since we have a new document
    // Disconnect from old undo stack if it exists
    if (mpTextUndoStack) {
        disconnect(mpTextUndoStack, nullptr, this, nullptr);
    }
    // Connect to the new document's undo stack
    mpTextUndoStack = mpSourceEditorEdbeeDocument->textUndoStack();
    connect(mpTextUndoStack, &edbee::TextUndoStack::undoExecuted, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpTextUndoStack, &edbee::TextUndoStack::redoExecuted, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);
    connect(mpTextUndoStack, &edbee::TextUndoStack::changeAdded, this, &dlgTriggerEditor::slot_updateUndoRedoButtonStates);

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    mudlet::ensureEdbeeTheme(mpHost->getEditorTheme(), mpHost->getEditorThemeFile());
    config->setThemeName(mpHost->getEditorTheme());
    config->setFont(mpHost->getEditorFont());
    config->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    config->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    config->setSmartTab(true);
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);
    config->setIndentSize(2);
    config->setCaretWidth(1);
    config->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    config->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    config->setAutocompleteMinimalCharacters(3);
    // Per document, not once: each document the pane is handed brings its own configuration
    config->setShowCaretOffset(false);
    config->endChanges();

    // If undo is not disabled when setting the initial text, the
    // setting of the text will be undoable.
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(false);
    disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setText(initialText);
    connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
    mpSourceEditorEdbeeDocument->setUndoCollectionEnabled(true);
}

void dlgTriggerEditor::saveEditorState(EditorViewType viewType, int itemId)
{
    if (!mpSourceEditorEdbee || !mpSourceEditorEdbeeDocument) {
        return;
    }

    EditorState state;

    if (auto* controller = mpSourceEditorEdbee->controller(); controller && controller->textSelection()) {
        const int caretOffset = controller->textSelection()->range(0).anchor();
        state.caretLine = mpSourceEditorEdbeeDocument->lineFromOffset(caretOffset);
        const int lineStart = mpSourceEditorEdbeeDocument->offsetFromLine(state.caretLine);
        state.caretColumn = caretOffset - lineStart;
    }

    if (auto* scrollBar = mpSourceEditorEdbee->verticalScrollBar()) {
        state.verticalScrollPos = scrollBar->value();
    }
    if (auto* scrollBar = mpSourceEditorEdbee->horizontalScrollBar()) {
        state.horizontalScrollPos = scrollBar->value();
    }

    mEditorStates[viewType][itemId] = state;
}

// Defers restoration using QTimer::singleShot to ensure the document and scroll bars
// are fully initialized after clearDocument(). Also re-enables widget updates that were
// disabled before document loading to prevent visual flicker.
void dlgTriggerEditor::restoreEditorState(EditorViewType viewType, int itemId)
{
    if (!mpSourceEditorEdbee || !mpSourceEditorEdbeeDocument) {
        if (mpSourceEditorEdbee) {
            mpSourceEditorEdbee->setUpdatesEnabled(true);
        }
        return;
    }

    const bool hasState = mEditorStates.contains(viewType) && mEditorStates[viewType].contains(itemId);
    const EditorState state = hasState ? mEditorStates[viewType][itemId] : EditorState{};

    QTimer::singleShot(0ms, this, [this, state, hasState]() {
        if (!mpSourceEditorEdbee) {
            return;
        }

        if (!mpSourceEditorEdbeeDocument) {
            mpSourceEditorEdbee->setUpdatesEnabled(true);
            return;
        }

        if (hasState) {
            auto* vScrollBar = mpSourceEditorEdbee->verticalScrollBar();
            auto* hScrollBar = mpSourceEditorEdbee->horizontalScrollBar();

            if (auto* controller = mpSourceEditorEdbee->controller()) {
                const auto lineCount = mpSourceEditorEdbeeDocument->lineCount();
                const auto maxLine = lineCount > 0 ? lineCount - 1 : 0;
                const auto line = std::min(static_cast<decltype(maxLine)>(state.caretLine), maxLine);
                const auto lineLength = mpSourceEditorEdbeeDocument->lineLength(line);
                const auto column = std::min(static_cast<decltype(lineLength)>(state.caretColumn), lineLength);

                controller->moveCaretTo(line, column, false);
            }

            if (vScrollBar) {
                vScrollBar->setValue(state.verticalScrollPos);
            }
            if (hScrollBar) {
                hScrollBar->setValue(state.horizontalScrollPos);
            }
        }

        // Re-enable updates now that scroll position is restored (or immediately if no state)
        mpSourceEditorEdbee->setUpdatesEnabled(true);
    });
}

void dlgTriggerEditor::clearEditorState(EditorViewType viewType, int itemId)
{
    if (mEditorStates.contains(viewType)) {
        mEditorStates[viewType].remove(itemId);
    }
}

void dlgTriggerEditor::setThemeAndOtherSettings(const QString& theme)
{
    auto localConfig = mpSourceEditorEdbee->config();
    localConfig->beginChanges();
    localConfig->setThemeName(theme);
    mpHost->editorThemeChanged();
    localConfig->setFont(mpHost->getEditorFont());
    localConfig->setShowWhitespaceMode((mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces) ? edbee::TextEditorConfig::ShowWhitespaces : edbee::TextEditorConfig::HideWhitespaces);
    localConfig->setUseLineSeparator(mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators);
    localConfig->setAutocompleteAutoShow(mpHost->mEditorAutoComplete);
    localConfig->setRenderBidiContolCharacters(mpHost->getEditorShowBidi());
    localConfig->setAutocompleteMinimalCharacters(3);
    // Per document, not once: each document the pane is handed brings its own configuration
    localConfig->setShowCaretOffset(false);
    localConfig->endChanges();
}

// The accent says that some option is in force, so a search quietly narrower than it looks still shows it
void dlgTriggerEditor::createSearchOptionIcon()
{
    // A restyle can call this before the search row is built
    if (!mpAction_searchOptions) {
        return;
    }

    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QPixmap source = uiDesign::glyphPixmap(qsl(":/icons/settings-search.svg"));
    const QIcon newIcon(uiDesign::tintedGlyph(source, (mSearchOptions == enums::EditorSearchOptionNone) ? tokens.mutedText : tokens.accentText));

    // Store the current setting icon - may need to copy it into the grandparent QComboBox items
    mIcon_searchOptions = newIcon;
    // Applied it to the QLineEdit for display purposes
    mpAction_searchOptions->setIcon(newIcon);

    QStringList activeOptions;
    if (mSearchOptions & enums::EditorSearchOptionCaseSensitive) {
        activeOptions << mpAction_searchCaseSensitive->text();
    }
    if (mSearchOptions & enums::EditorSearchOptionIncludeVariables) {
        activeOptions << mpAction_searchIncludeVariables->text();
    }
    if (mSearchOptions & enums::EditorSearchOptionWholeWord) {
        activeOptions << mpAction_searchWholeWord->text();
    }
    if (activeOptions.isEmpty()) {
        //: Tooltip on the magnifier at the left of the editor's search field while no search option is set
        mpAction_searchOptions->setToolTip(utils::richText(tr("Search options")));
    } else {
        //: Tooltip on that magnifier once options are set. %1 is the list of them, already in the reader's language.
        mpAction_searchOptions->setToolTip(utils::richText(tr("Search options: %1").arg(activeOptions.join(qsl(", ")))));
    }
}

void dlgTriggerEditor::updateSearchHistoryAction()
{
    if (!mpAction_searchHistory) {
        return;
    }

    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    QIcon chevron(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-chevron-down.svg")), tokens.mutedText));
    chevron.addPixmap(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-chevron-down.svg")), tokens.text), QIcon::Active);
    mpAction_searchHistory->setIcon(chevron);
    mpAction_searchHistory->setVisible(comboBox_searchTerms->count() > 0);
}

int dlgTriggerEditor::findSearchMatch(const QString& haystack, const QString& needle, int from) const
{
    if (needle.isEmpty()) {
        return -1;
    }

    if (mSearchOptions & enums::EditorSearchOptionWholeWord) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!(mSearchOptions & enums::EditorSearchOptionCaseSensitive)) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        if (needle != mWholeWordSearchNeedle || options != mWholeWordSearchRegex.patternOptions()) {
            mWholeWordSearchRegex = QRegularExpression(qsl("\\b%1\\b").arg(QRegularExpression::escape(needle)), options);
            mWholeWordSearchNeedle = needle;
        }
        const QRegularExpressionMatch match = mWholeWordSearchRegex.match(haystack, from);
        if (match.hasMatch()) {
            return match.capturedStart();
        }
        return -1;
    }

    return haystack.indexOf(needle, from, (mSearchOptions & enums::EditorSearchOptionCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

bool dlgTriggerEditor::containsSearchMatch(const QString& haystack, const QString& needle) const
{
    return findSearchMatch(haystack, needle) != -1;
}

void dlgTriggerEditor::slot_toggleSearchCaseSensitivity(const bool state)
{
    if ((mSearchOptions & enums::EditorSearchOptionCaseSensitive) != state) {
        mSearchOptions = (mSearchOptions & ~(enums::EditorSearchOptionCaseSensitive)) | (state ? enums::EditorSearchOptionCaseSensitive : enums::EditorSearchOptionNone);
        createSearchOptionIcon();
        mpHost->mSearchOptions = mSearchOptions;
    }
}

void dlgTriggerEditor::slot_toggleSearchIncludeVariables(const bool state)
{
    if ((mSearchOptions & enums::EditorSearchOptionIncludeVariables) != state) {
        mSearchOptions = (mSearchOptions & ~(enums::EditorSearchOptionIncludeVariables)) | (state ? enums::EditorSearchOptionIncludeVariables : enums::EditorSearchOptionNone);
        createSearchOptionIcon();
        mpHost->mSearchOptions = mSearchOptions;
    }
}

void dlgTriggerEditor::slot_toggleSearchWholeWord(const bool state)
{
    if ((mSearchOptions & enums::EditorSearchOptionWholeWord) != state) {
        mSearchOptions = (mSearchOptions & ~(enums::EditorSearchOptionWholeWord)) | (state ? enums::EditorSearchOptionWholeWord : enums::EditorSearchOptionNone);
        createSearchOptionIcon();
        mpHost->mSearchOptions = mSearchOptions;
    }
}

void dlgTriggerEditor::slot_clearSearchResults()
{
    // Want the clearing of the search results to show:
    treeWidget_searchResults->clear();
    treeWidget_searchResults->update();
    mSearchTerm.clear();
    setSearchResultsShown(false);

    // unhighlight all instances of the item that we've searched for.
    // edbee already remembers this from a setSearchTerm() call elsewhere
    auto controller = mpSourceEditorEdbee->controller();
    auto textRanges = controller->borderedTextRanges();
    textRanges->clear();
    controller->update();
}

// shows a custom right-click menu for the editor, including the indent action
void dlgTriggerEditor::slot_editorContextMenu()
{
    edbee::TextEditorWidget* editor = mpSourceEditorEdbee;
    if (!editor) {
        return;
    }

    edbee::TextEditorController* controller = mpSourceEditorEdbee->controller();

    auto menu = new QMenu();
    auto formatAction = new QAction(tr("Format All"), menu);
    // appropriate shortcuts are automatically supplied by edbee here
    if (qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        menu->addAction(controller->createAction("undo", tr("Undo"), QIcon(), menu));
        menu->addAction(controller->createAction("redo", tr("Redo"), QIcon(), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon(), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon(), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon(), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon(), menu));
    } else {
        menu->addAction(controller->createAction("undo", tr("Undo"), QIcon::fromTheme(qsl("edit-undo"), QIcon(qsl(":/icons/edit-undo.png"))), menu));
        menu->addAction(controller->createAction("redo", tr("Redo"), QIcon::fromTheme(qsl("edit-redo"), QIcon(qsl(":/icons/edit-redo.png"))), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("cut", tr("Cut"), QIcon::fromTheme(qsl("edit-cut"), QIcon(qsl(":/icons/edit-cut.png"))), menu));
        menu->addAction(controller->createAction("copy", tr("Copy"), QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))), menu));
        menu->addAction(controller->createAction("paste", tr("Paste"), QIcon::fromTheme(qsl("edit-paste"), QIcon(qsl(":/icons/edit-paste.png"))), menu));
        menu->addSeparator();
        menu->addAction(controller->createAction("sel_all", tr("Select All"), QIcon::fromTheme(qsl("edit-select-all"), QIcon(qsl(":/icons/edit-select-all.png"))), menu));
        formatAction->setIcon(QIcon::fromTheme(qsl("run-build-clean"), QIcon::fromTheme(qsl("run-build-clean"))));
    }

    connect(formatAction, &QAction::triggered, this, [=, this]() {
        auto formattedText = mpHost->mLuaInterpreter.formatLuaCode(mpSourceEditorEdbeeDocument->text());
        // workaround for crash if undo is used, see https://github.com/edbee/edbee-lib/issues/66
        controller->beginUndoGroup();
        disconnect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
        mpSourceEditorEdbeeDocument->setText(formattedText);
        connect(mpSourceEditorEdbeeDocument, &edbee::TextDocument::textChanged, this, &dlgTriggerEditor::slot_itemEdited);
        // don't coalesce the format text action - not that it matters for us since we we only change
        // the text once during the undo group
        controller->endUndoGroup(edbee::CoalesceId_None, false);
    });

    menu->addAction(formatAction);

    // Built on demand, outside the style pass, so it is styled here
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    menu->setStyleSheet(uiDesign::menuStyleSheet(tokens));
    uiDesign::letPopupsTakeTheFieldsCorner(menu);

    menu->exec(QCursor::pos());

    delete menu;
}

QString dlgTriggerEditor::generateButtonStyleSheet(const QColor& color, const bool isEnabled)
{
    if (color != QColorConstants::Transparent && color.isValid()) {
        // Read per call: a theme change restyles these wells, and the border tone has moved by then
        const QString hairline = uiDesign::themeTokens().border.name();
        if (isEnabled) {
            // theme-fixed: the fill is the colour the user picked, so the word on it is chosen against that fill rather than against a page
            return mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(color.lightness() > 127 ? QLatin1String("black") : QLatin1String("white"), color.name(), hairline);
        }

        const QColor disabledColor = QColor::fromHsl(color.hslHue(), color.hslSaturation() / 4, color.lightness());
        // theme-fixed: the same well, desaturated - the word still sits on the value rather than on a page
        return mudlet::self()->mTEXT_ON_BG_STYLESHEET.arg(QLatin1String("darkGray"), disabledColor.name(), hairline);
    }
    return QString();
}

// Retrieve the background-color or color setting from the previous method, the
// colors used can theoretically be:
// * any strings of those from http://www.w3.org/TR/SVG/types.html#ColorKeywords
// * #RGB (each of R, G, and B is a single hex digit) 3 Digits
// * #RRGGBB 6 Digits
// * #AARRGGBB (Since 5.2) 8 Digits
// * #RRRGGGBBB 9 Digits
// * #RRRRGGGGBBBB 12 Digits
// * "transparent"
QColor dlgTriggerEditor::parseButtonStyleSheetColors(const QString& styleSheetText, const bool isToGetForeground)
{
    if (styleSheetText.isEmpty()) {
        return QColor();
    }

    QRegularExpression hexColorRegex;
    QRegularExpression namedColorRegex;
    if (isToGetForeground) {
        hexColorRegex.setPattern(QLatin1String("(?:[{ ])color:\\s*(?:#)([[:xdigit:]]{3,12})\\s*;")); // Capture group 1 is a foreground color made of hex digits
        QRegularExpressionMatch match = hexColorRegex.match(styleSheetText);
        if (match.hasMatch()) {
            switch (match.capturedLength(1)) {
            case 3: // RGB
                [[fallthrough]];
            case 6: // RRGGBB
                [[fallthrough]];
            case 9: // RRRGGGBBB
                [[fallthrough]];
            case 12: // RRRRGGGGBBBB
                return QColor(match.captured(1).prepend(QLatin1Char('#')));

            default:
                // case 8: // AARRGGBB - Invalid here
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - Invalid hex string as foreground color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a foreground color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                if (QColor::isValidColor(match.captured(1))) {
#else
                if (QColor::isValidColorName(match.captured(1))) {
#endif
                    return QColor(match.captured(1));
                }
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \""
                                             << match.captured(1) << "\" found as name of foreground color!";
                return QColor();
            }
            qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                         << ") ERROR - No string as name of foreground color found!";
            return QColor();
        }
    } else {
        hexColorRegex.setPattern(QLatin1String("(?:[{ ])background-color:\\s*(?:#)([[:xdigit:]]{3,12})\\s*;")); // Capture group 1 is a background color made of hex digits
        QRegularExpressionMatch match = hexColorRegex.match(styleSheetText);
        if (match.hasMatch()) {
            switch (match.capturedLength(1)) {
            case 3: // RGB
                [[fallthrough]];
            case 6: // RRGGBB
                [[fallthrough]];
            case 9: // RRRGGGBBB
                [[fallthrough]];
            case 12: // RRRRGGGGBBBB
                return QColor(match.captured(1).prepend(QLatin1Char('#')));

            default:
                // case 8: // AARRGGBB - Invalid here
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                             << ") ERROR - Invalid hex string as background color!";
                return QColor();
            }
        } else {
            namedColorRegex.setPattern(QLatin1String("(?:[{ ])background-color:\\s*(\\w{3,})\\s*;")); // Capture group 1 is a word for a background color
            match = namedColorRegex.match(styleSheetText);
            if (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                if (QColor::isValidColor(match.captured(1))) {
#else
                if (QColor::isValidColorName(match.captured(1))) {
#endif
                    return QColor(match.captured(1));
                }
                qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground << ") ERROR - Invalid string \""
                                             << match.captured(1) << "\" found as name of background color!";
                return QColor();
            }
            qDebug().noquote().nospace() << "dlgTriggerEditor::parseButtonStyleSheetColors(\"" << styleSheetText << "\", " << isToGetForeground
                                         << ") ERROR - No string as name of background color found!";
            return QColor();
        }
    }
}

void dlgTriggerEditor::slot_toggleGroupBoxColorizeTrigger(const bool state)
{
    if (mpTriggersMainArea->checkBox_triggerColorizer->isChecked() != state) {
        mpTriggersMainArea->checkBox_triggerColorizer->setChecked(state);
    }

    if (state) {
        // Enabled so make buttons have full colour:
        const QString fgColor = mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString();
        const QString bgColor = mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString();
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(fgColor, true));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(bgColor, true));
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonFgColor->setText(fgColor == QLatin1String("transparent") ? tr("keep") : QString());
        //: Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button
        mpTriggersMainArea->pushButtonBgColor->setText(bgColor == QLatin1String("transparent") ? tr("keep") : QString());
    } else {
        // Disabled so make buttons greyed out a bit:
        mpTriggersMainArea->pushButtonFgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonFgColor->property(cButtonBaseColor).toString(), false));
        mpTriggersMainArea->pushButtonBgColor->setStyleSheet(generateButtonStyleSheet(mpTriggersMainArea->pushButtonBgColor->property(cButtonBaseColor).toString(), false));
        mpTriggersMainArea->pushButtonFgColor->setText(QString());
        mpTriggersMainArea->pushButtonBgColor->setText(QString());
    }
}

void dlgTriggerEditor::slot_clearSoundFile()
{
    showTriggerSoundFile(QString());
    slot_saveProperty_TriggerSoundFile();
}

// The switch's own toggled() writes the one undo entry, exactly as a click on it would
void dlgTriggerEditor::turnTriggerHighlightOn()
{
    if (mpTriggersMainArea->checkBox_triggerColorizer->isChecked()) {
        return;
    }
    mpTriggersMainArea->checkBox_triggerColorizer->setChecked(true);
    slot_toggleGroupBoxColorizeTrigger(true);
    slot_itemEdited();
}

// The font is set here rather than in the sheet: the pill radius is half the height these metrics give, a size the
// sheet set is only measurable after it applies, and a larger radius clamps into an ellipse. Returns that height.
static int styleEditorIdChip(QFrame* pFrameId, QLabel* pIdLabel, QLabel* pIdNumber)
{
    const QFont chipFont = uiDesign::chipFont(pFrameId);

    for (QLabel* pLabel : {pIdLabel, pIdNumber}) {
        pLabel->setFont(chipFont);
        // The .ui files grey the pair out; the chip is quiet enough, and a disabled label cannot be selected from
        pLabel->setEnabled(true);
    }

    pFrameId->setProperty("editorIdChip", true);
    if (QLayout* pChipLayout = pFrameId->layout()) {
        pChipLayout->setContentsMargins(scmEditorIdChipPaddingHorizontal, scmEditorIdChipPaddingVertical, scmEditorIdChipPaddingHorizontal, scmEditorIdChipPaddingVertical);
        pChipLayout->setSpacing(scmEditorIdChipGap);
    }

    const int chipHeight = QFontMetrics(chipFont).height() + 2 * (scmEditorIdChipPaddingVertical + uiDesign::scmInputBorderWidth);
    pFrameId->setFixedHeight(chipHeight);
    return chipHeight;
}

// A QGroupBox drawn as nothing: a screen reader announces a group box as a grouping, and a plain QWidget not at all
static QGroupBox* makeEditorOptionRow(QWidget* pParent, const QString& objectName, const QString& accessibleName)
{
    auto* pRow = new QGroupBox(pParent);
    pRow->setObjectName(objectName);
    pRow->setTitle(QString());
    pRow->setAccessibleName(accessibleName);
    pRow->setProperty("editorOptionRow", true);
    auto* pFlow = new uiDesign::FlowLayout(pRow, scmEditorOptionRowGap, scmEditorRowControlGap);
    pFlow->setContentsMargins(0, 0, 0, 0);
    // So the row does not widen its column by everything it holds; the rest wraps
    pFlow->setWidthPolicy(uiDesign::FlowLayout::Width::WidestItem);
    return pRow;
}

// A control and its words or wells, held together so they wrap onto the next line as one
static QWidget* makeEditorOptionGroup(QWidget* pRow)
{
    auto* pGroup = new QWidget(pRow);
    pGroup->setProperty("editorPanelSurface", true);
    auto* pLayout = new QHBoxLayout(pGroup);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(scmEditorRowControlGap);
    return pGroup;
}

// FlowLayout tops each item on its line, so a 20px check box beside a 30px spin box would sit above its middle;
// at one height each group's own layout centres its contents
static void holdToOneControlHeight(QWidget* pItem)
{
    pItem->setFixedHeight(uiDesign::scmInputHeight);
}

// A screen reader falls back to the tooltip only where there is no description, and would read its rich-text tags out
static void describeEditorControl(QWidget* pControl, const QString& name, const QString& tip)
{
    pControl->setAccessibleName(name);
    pControl->setAccessibleDescription(tip);
    pControl->setToolTip(utils::richText(tip));
}

// The strip wraps rather than squeezing, so a narrow editor costs the options a line, not the patterns their width
void dlgTriggerEditor::buildTriggerOptionsStrip()
{
    auto* pForm = mpTriggersMainArea;
    auto* pGrid = qobject_cast<QGridLayout*>(pForm->layout());
    if (!pGrid) {
        return;
    }

    pForm->label_trigger_name->setProperty("editorRowLabel", true);
    pForm->label_trigger_command->setProperty("editorRowLabel", true);

    QToolButton* pToggle = pForm->toolButton_toggleExtraControls;
    pToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    //: Button on the head row of a trigger form that shows or hides the trigger's options
    pToggle->setText(tr("Options"));
    describeEditorControl(pToggle,
                          //: Accessible name of that button
                          tr("Options"),
                          //: Tooltip on that button
                          tr("Show or hide the trigger's options: how it matches, how long it keeps firing, its sound and its highlight."));
    const int toggleGlyphSize = qRound(pToggle->fontMetrics().height() * 0.9);
    pToggle->setIconSize(QSize(toggleGlyphSize, toggleGlyphSize));
    // Auto-raise would take away the frame the stylesheet draws the checked state with
    pToggle->setAutoRaise(false);
    pToggle->setFixedHeight(uiDesign::scmInputHeight);
    pToggle->setCheckable(true);
    connect(pToggle, &QAbstractButton::clicked, this, &dlgTriggerEditor::setTriggerOptionsShown);

    // Hidden, not removed: save and load read the mode from it (its first value means Any), and the segments are
    // a view of it. The .ui file leaves it at QSpinBox's default maximum of 99.
    pForm->spinBox_lineMargin->setMaximum(scmEditorMatchWithinLinesMax);
    pForm->spinBox_lineMargin->hide();

    //: Word leading the one row of a trigger form that holds everything about the trigger other than its name, its patterns and its script
    mpLabel_optionsRow = new QLabel(tr("Options"), pForm);
    // One control tall, so it sits level with the first line of a wrapped strip rather than half way down it
    mpLabel_optionsRow->setWordWrap(false);
    mpLabel_optionsRow->setFixedHeight(uiDesign::scmInputHeight);

    QGroupBox* pOptionsRow = makeEditorOptionRow(pForm, qsl("editorOptionsRow"), mpLabel_optionsRow->text());
    mpWidget_triggerOptionsRow = pOptionsRow;

    QWidget* pMatchGroup = makeEditorOptionGroup(pOptionsRow);
    auto* pMatchLayout = qobject_cast<QHBoxLayout*>(pMatchGroup->layout());

    // Radio buttons, not drawn labels: a screen reader says which of the two is chosen and how many there are.
    // The word goes grey with them, being half of what the choice reads as.
    mpWidget_matchModeRows = makeEditorOptionGroup(pMatchGroup);
    auto* pSegments = qobject_cast<QHBoxLayout*>(mpWidget_matchModeRows->layout());
    auto* pMatchWord = new QLabel(mpWidget_matchModeRows);
    pMatchWord->setProperty("editorRowLabel", true);
    pMatchWord->setWordWrap(false);
    //: Word before the Any / All segments on a trigger's options strip
    pMatchWord->setText(tr("Match"));
    pSegments->setSpacing(0);
    pSegments->addWidget(pMatchWord);
    pSegments->addSpacing(scmEditorRowControlGap);

    mpRadioButton_matchAny = new QRadioButton(mpWidget_matchModeRows);
    mpRadioButton_matchAny->setObjectName(qsl("editorMatchAny"));
    //: One of a trigger's two matching modes, on a segment of a two-part control led by the word "Match": the trigger fires as soon as any one of its patterns matches. One word, since it is read as "Match Any"; the full name a screen reader hears is set below.
    mpRadioButton_matchAny->setText(tr("Any"));
    mpRadioButton_matchAll = new QRadioButton(mpWidget_matchModeRows);
    mpRadioButton_matchAll->setObjectName(qsl("editorMatchAll"));
    //: The other matching mode, on the second segment of that control: the trigger fires only once every pattern has matched. One word, read as "Match All".
    mpRadioButton_matchAll->setText(tr("All"));
    describeEditorControl(mpRadioButton_matchAny,
                          //: Accessible name of the "Any" segment, said in full because a screen reader reaches it without the word "Match" beside it
                          tr("Any pattern"),
                          //: Tooltip on the "Any" segment of a trigger's matching mode
                          tr("The trigger fires when any one of its patterns matches."));
    describeEditorControl(mpRadioButton_matchAll,
                          //: Accessible name of the "All" segment, said in full for the same reason
                          tr("All patterns"),
                          //: Tooltip on the "All" segment of a trigger's matching mode
                          tr("Every pattern must match, within the lines set beside this, before the trigger fires. Captures go to multimatches."));
    mpRadioButton_matchAny->setProperty("editorSegment", true);
    mpRadioButton_matchAny->setProperty("editorSegmentSide", qsl("first"));
    mpRadioButton_matchAll->setProperty("editorSegment", true);
    mpRadioButton_matchAll->setProperty("editorSegmentSide", qsl("last"));
    for (QRadioButton* pSegment : {mpRadioButton_matchAny, mpRadioButton_matchAll}) {
        // A QSS min-height sizes the contents and the style adds its margins on top, so the height is set outright
        pSegment->setFixedHeight(uiDesign::scmInputHeight);
        pSegments->addWidget(pSegment);
    }
    pMatchLayout->addWidget(mpWidget_matchModeRows);

    // Dimmed rather than hidden in Any mode: a control that comes and goes with a click moves everything beside it
    mpWidget_matchWithinRow = makeEditorOptionGroup(pMatchGroup);
    auto* pWithinLayout = qobject_cast<QHBoxLayout*>(mpWidget_matchWithinRow->layout());
    mpSpinBox_matchWithinLines = new QSpinBox(mpWidget_matchWithinRow);
    mpSpinBox_matchWithinLines->setObjectName(qsl("editorMatchWithinLines"));
    mpSpinBox_matchWithinLines->setRange(0, scmEditorMatchWithinLinesMax);
    mpSpinBox_matchWithinLines->setAlignment(Qt::AlignCenter);
    // The .ui file gives spinBox_stayOpen a policy that fills its line; this one would stop at its hint, a size smaller
    mpSpinBox_matchWithinLines->setFixedHeight(uiDesign::scmInputHeight);
    describeEditorControl(mpSpinBox_matchWithinLines,
                          //: Accessible name of the box holding how many lines a trigger's patterns must all match within
                          tr("Lines all the patterns must match within"),
                          //: Tooltip on that box
                          tr("How many lines after the first match the other patterns may still match on. 0 means the same line."));
    //: Completes the matching mode chosen beside it - all patterns must match - by saying how many lines they have to match within. %1 is replaced by a number box, and the words around it may be reordered to put it wherever the language needs it.
    uiDesign::buildControlSentenceRow(pWithinLayout, tr("within %1 lines"), mpSpinBox_matchWithinLines);
    pMatchLayout->addWidget(mpWidget_matchWithinRow);
    pOptionsRow->layout()->addWidget(pMatchGroup);

    //: Trigger option, was called "match all": the script runs once for every place in the line the pattern matches, rather than once for the first. Kept short for the options strip; the full name a screen reader hears is set below.
    pForm->checkBox_perlSlashGOption->setText(tr("Every occurrence"));
    describeEditorControl(pForm->checkBox_perlSlashGOption,
                          //: Accessible name of that option, said in full
                          tr("Every occurrence in a line"),
                          //: Tooltip on that option
                          tr("Fire once for each place the pattern matches on a line, not only the first."));
    pOptionsRow->layout()->addWidget(pForm->checkBox_perlSlashGOption);

    QWidget* pStayOpenGroup = makeEditorOptionGroup(pOptionsRow);
    describeEditorControl(pForm->spinBox_stayOpen,
                          //: Accessible name of the box holding how many more lines a trigger keeps firing for
                          tr("Lines to keep firing for after a match"),
                          //: Tooltip on that box
                          tr("After a match, keep running this trigger's script on the next lines too."));
    //: %1 is replaced by a number box holding how many extra lines the trigger keeps firing for, past the one it matched on. The words around it may be reordered to put it wherever the language needs it.
    uiDesign::buildControlSentenceRow(qobject_cast<QHBoxLayout*>(pStayOpenGroup->layout()), tr("Keep firing %1 more lines"), pForm->spinBox_stayOpen);
    pOptionsRow->layout()->addWidget(pStayOpenGroup);

    fitEditorOptionsSpinBoxes();

    //: Trigger option, was called "only pass matches": the trigger's children see only the part of the line its pattern matched. Kept short for the options strip; the full name a screen reader hears is set below.
    pForm->checkBox_filterTrigger->setText(tr("Only pass matches"));
    describeEditorControl(pForm->checkBox_filterTrigger,
                          //: Accessible name of that option, said in full
                          tr("Only pass matches to children"),
                          //: Tooltip on that option
                          tr("Child triggers see only the text that matched, not the whole line."));
    pOptionsRow->layout()->addWidget(pForm->checkBox_filterTrigger);

    // The field stays live while the switch is off: choosing a file is how the switch is turned on
    QWidget* pSoundGroup = makeEditorOptionGroup(pOptionsRow);
    auto* pSoundLayout = qobject_cast<QHBoxLayout*>(pSoundGroup->layout());
    //: Trigger option: a sound file is played whenever the trigger fires. One word, since the field naming the file stands beside it; the full name a screen reader hears is set below.
    pForm->checkBox_soundTrigger->setText(tr("Sound"));
    describeEditorControl(pForm->checkBox_soundTrigger,
                          //: Accessible name of that option, said in full
                          tr("Play a sound"),
                          //: Tooltip on that option
                          tr("Play a sound file whenever the trigger fires."));
    QLineEdit* pSoundField = pForm->lineEdit_soundFile;
    pSoundField->setObjectName(qsl("editorSoundFile"));
    pSoundField->setReadOnly(true);
    pSoundField->setFocusPolicy(Qt::StrongFocus);
    pSoundField->setCursor(Qt::PointingHandCursor);
    pSoundField->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pSoundField->setFixedWidth(scmEditorSoundFileWidth);
    //: Stands in the sound file field for a trigger that plays no sound file yet
    pSoundField->setPlaceholderText(tr("No sound file"));
    describeEditorControl(pSoundField,
                          //: Accessible name of the field naming the sound file a trigger plays
                          tr("Sound file"),
                          //: Tooltip on that field, which opens the file chooser when it is clicked
                          tr("Choose the sound file to play. Choosing one turns the sound on."));
    pSoundField->installEventFilter(this);
    describeEditorControl(pForm->toolButton_clearSoundFile,
                          //: Accessible name of the cross that takes the sound file off a trigger
                          tr("Remove the sound file"),
                          //: Tooltip on that cross
                          tr("Forget the sound file."));
    pForm->toolButton_clearSoundFile->setAutoRaise(true);
    pForm->toolButton_clearSoundFile->setFocusPolicy(Qt::StrongFocus);
    pSoundLayout->addWidget(pForm->checkBox_soundTrigger);
    pSoundLayout->addWidget(pSoundField);
    pSoundLayout->addWidget(pForm->toolButton_clearSoundFile);
    pOptionsRow->layout()->addWidget(pSoundGroup);

    // The wells stay live too: choosing a colour turns the highlight on
    QWidget* pHighlightGroup = makeEditorOptionGroup(pOptionsRow);
    auto* pHighlightLayout = qobject_cast<QHBoxLayout*>(pHighlightGroup->layout());
    //: Trigger option: the text the trigger matched is redrawn in the colours beside this. One word, since the two wells stand beside it; the full name a screen reader hears is set below.
    pForm->checkBox_triggerColorizer->setText(tr("Highlight"));
    describeEditorControl(pForm->checkBox_triggerColorizer,
                          //: Accessible name of that option, said in full
                          tr("Highlight matches"),
                          //: Tooltip on that option
                          tr("Recolour the matched text where it appears in the console."));
    describeEditorControl(pForm->pushButtonFgColor,
                          //: Accessible name of the well holding the colour a trigger's matched text is drawn in
                          tr("Foreground colour"),
                          //: Tooltip on that well
                          tr("The ink the matched text is drawn in. Choose Keep in the picker to leave the game's own."));
    describeEditorControl(pForm->pushButtonBgColor,
                          //: Accessible name of the well holding the colour drawn behind a trigger's matched text
                          tr("Background colour"),
                          //: Tooltip on that well
                          tr("The colour behind the matched text. Choose Keep in the picker to leave the game's own."));
    for (QPushButton* pWell : {pForm->pushButtonFgColor, pForm->pushButtonBgColor}) {
        pWell->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        pWell->setFixedSize(scmEditorColorWellWidth, scmEditorColorWellHeight);
        pHighlightLayout->addWidget(pWell);
    }
    pHighlightLayout->insertWidget(0, pForm->checkBox_triggerColorizer);
    pOptionsRow->layout()->addWidget(pHighlightGroup);

    // checkForMoreThanOneTriggerItem() shows this while there is only one pattern
    mpLabel_matchModeHint = new QLabel(pOptionsRow);
    mpLabel_matchModeHint->setProperty("editorRowLabel", true);
    mpLabel_matchModeHint->setWordWrap(false);
    //: Caption on a trigger's options strip, shown while the trigger has only one pattern and the Any / All choice is therefore greyed out
    mpLabel_matchModeHint->setText(tr("Add a second pattern to combine them."));
    pOptionsRow->layout()->addWidget(mpLabel_matchModeHint);

    for (QWidget* pRow : {static_cast<QWidget*>(mpWidget_matchWithinRow), pStayOpenGroup}) {
        for (QLabel* pWord : pRow->findChildren<QLabel*>()) {
            pWord->setProperty("editorRowLabel", true);
        }
    }

    for (QWidget* pItem : {pMatchGroup,
                           static_cast<QWidget*>(pForm->checkBox_perlSlashGOption),
                           pStayOpenGroup,
                           static_cast<QWidget*>(pForm->checkBox_filterTrigger),
                           pSoundGroup,
                           pHighlightGroup,
                           static_cast<QWidget*>(mpLabel_matchModeHint)}) {
        holdToOneControlHeight(pItem);
    }

    // The .ui grid has the head row at 0 and the pattern list at 1; the strip goes between them
    pGrid->removeWidget(pForm->widget_left);
    pGrid->addWidget(mpLabel_optionsRow, 1, 0, Qt::AlignLeft | Qt::AlignTop);
    pGrid->addWidget(pOptionsRow, 1, 1);
    pGrid->addWidget(pForm->widget_left, 2, 0, 1, 2);
    pGrid->setRowStretch(0, 0);
    pGrid->setRowStretch(1, 0);
    pGrid->setRowStretch(2, 1);
    pGrid->setColumnStretch(0, 0);
    pGrid->setColumnStretch(1, 1);

    // Through spinBox_lineMargin, so slot_saveProperty_TriggerLineMargin() and its undo entry hear of the change
    connect(mpRadioButton_matchAll, &QAbstractButton::toggled, this, [this](const bool checked) {
        mpTriggersMainArea->spinBox_lineMargin->setValue(checked ? mpSpinBox_matchWithinLines->value() : -1);
    });
    connect(mpSpinBox_matchWithinLines, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        if (mpRadioButton_matchAll->isChecked()) {
            mpTriggersMainArea->spinBox_lineMargin->setValue(value);
        }
    });
    // ...and back, so loading a trigger or undoing an edit shows in the segments
    connect(mpTriggersMainArea->spinBox_lineMargin, qOverload<int>(&QSpinBox::valueChanged), this, &dlgTriggerEditor::reflectTriggerMatchMode);

    reflectTriggerMatchMode();
    showTriggerSoundFile(QString());
}

// Reuses the .ui file's own controls, so their object names, connections and translations survive
static QWidget* buildEditorFormHeadRow(QWidget* pForm, const QList<QPair<QWidget*, int>>& controls)
{
    auto* pHeadRow = new QWidget(pForm);
    pHeadRow->setObjectName(qsl("widget_top"));
    pHeadRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* pRowLayout = new QHBoxLayout(pHeadRow);
    pRowLayout->setContentsMargins(0, 0, 0, 0);
    pRowLayout->setSpacing(scmEditorFormHeadRowSpacing);
    for (const auto& [pControl, stretch] : controls) {
        // Still in the .ui grid, and adding a widget to a second layout warns once per widget
        uiDesign::detachFromLayout(pControl);
        pRowLayout->addWidget(pControl, stretch);
    }
    return pHeadRow;
}

void dlgTriggerEditor::buildEditorFormHeadRows()
{
    const QList<QPair<QWidget*, QList<QPair<QWidget*, int>>>> heads{
            {mpAliasMainArea,
             {{mpAliasMainArea->label_alias_name, 0},
              {mpAliasMainArea->lineEdit_alias_name, 1},
              {mpAliasMainArea->label_alias_command, 0},
              {mpAliasMainArea->lineEdit_alias_command, 1},
              {mpAliasMainArea->frameId, 0}}},
            {mpTimersMainArea,
             {{mpTimersMainArea->label_timer_name, 0},
              {mpTimersMainArea->lineEdit_timer_name, 1},
              {mpTimersMainArea->label_timer_command, 0},
              {mpTimersMainArea->lineEdit_timer_command, 1},
              {mpTimersMainArea->frameId, 0}}},
            {mpKeysMainArea,
             {{mpKeysMainArea->label_key_name, 0},
              {mpKeysMainArea->lineEdit_key_name, 1},
              {mpKeysMainArea->label_key_command, 0},
              {mpKeysMainArea->lineEdit_key_command, 1},
              {mpKeysMainArea->frameId, 0}}},
            {mpScriptsMainArea, {{mpScriptsMainArea->label_script_name, 0}, {mpScriptsMainArea->lineEdit_script_name, 1}, {mpScriptsMainArea->frameId, 0}}},
            {mpActionsMainArea, {{mpActionsMainArea->label_action_name, 0}, {mpActionsMainArea->lineEdit_action_name, 1}, {mpActionsMainArea->frameId, 0}}},
            {mpVarsMainArea, {{mpVarsMainArea->label_variable_name, 0}, {mpVarsMainArea->lineEdit_var_name, 1}}}};

    for (const auto& [pForm, controls] : heads) {
        QWidget* pHeadRow = buildEditorFormHeadRow(pForm, controls);
        if (pForm == mpActionsMainArea) {
            mpWidget_actionHeadRow = pHeadRow;
        }
        if (auto* pGrid = qobject_cast<QGridLayout*>(pForm->layout())) {
            uiDesign::insertGridRowAtTop(pGrid, pHeadRow);
        } else if (auto* pColumn = qobject_cast<QBoxLayout*>(pForm->layout())) {
            pColumn->insertWidget(0, pHeadRow);
        }
    }

    // The frame carrying the column already insets it, so the forms add no margin of their own
    const QList<QWidget*> laidOut{mpAliasMainArea,
                                  mpTimersMainArea,
                                  mpKeysMainArea,
                                  mpScriptsMainArea,
                                  mpActionsMainArea,
                                  mpVarsMainArea,
                                  // The variables form's grid is on this nested frame
                                  mpVarsMainArea->frame_vars_main_area};
    for (QWidget* pWidget : laidOut) {
        QLayout* pLayout = pWidget->layout();
        pLayout->setContentsMargins(0, 0, 0, 0);
        if (auto* pGrid = qobject_cast<QGridLayout*>(pLayout)) {
            pGrid->setHorizontalSpacing(scmEditorFormGridSpacingHorizontal);
            pGrid->setVerticalSpacing(scmEditorFormGridSpacingVertical);
        } else {
            pLayout->setSpacing(scmEditorFormGridSpacingVertical);
        }
    }

    for (QLabel* pLabel : editorFormRowLabels()) {
        pLabel->setProperty("editorRowLabel", true);
        // Left: the labels are widened to one width at style time, and a right-set word would sit far from the one above
        pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
}

// Not in the .ui file: a row that wraps has to report its own height, which no Designer control does
void dlgTriggerEditor::buildScriptEventRow()
{
    auto* pGrid = qobject_cast<QGridLayout*>(mpScriptsMainArea->layout());
    if (!pGrid) {
        return;
    }

    mpChipRow_scriptEvents = new uiDesign::ChipRow(mpScriptsMainArea);
    mpChipRow_scriptEvents->setObjectName(qsl("editorScriptEvents"));
    //: Tooltip on one event in a script's row of events, when nothing the editor knows of - Mudlet, the game or a script in this profile - raises or listens for it
    mpChipRow_scriptEvents->setUnlistedOrigin(tr("Not seen in this profile yet"));
    pGrid->addWidget(mpChipRow_scriptEvents, 1, 1, 1, std::max(1, pGrid->columnCount() - 1));

    // Top-aligned, level with the first line of chips; alignEditorFormLeadLabels() holds it to one chip's height
    QLabel* pLabel = mpScriptsMainArea->label_script_registered_event_handlers;
    pGrid->setAlignment(pLabel, Qt::AlignLeft | Qt::AlignTop);
    pLabel->setWordWrap(false);

    connect(mpChipRow_scriptEvents, &uiDesign::ChipRow::itemsChanged, this, &dlgTriggerEditor::slot_itemEdited);
    connect(mpChipRow_scriptEvents, &uiDesign::ChipRow::fieldOpening, this, [this]() {
        if (mpHost) {
            mpChipRow_scriptEvents->setSuggestions(eventSuggestions(mpHost->knownEventNames()));
        }
    });
}

void dlgTriggerEditor::buildTimerIntervalRow()
{
    auto* pGrid = qobject_cast<QGridLayout*>(mpTimersMainArea->layout());
    const int labelIndex = pGrid ? pGrid->indexOf(mpTimersMainArea->label_timer_time) : -1;
    if (labelIndex < 0) {
        return;
    }

    mpWidget_timerInterval = new QWidget(mpTimersMainArea);
    mpWidget_timerInterval->setObjectName(qsl("editorTimerInterval"));
    mpWidget_timerInterval->setProperty("editorPanelSurface", true);
    auto* pRow = new QHBoxLayout(mpWidget_timerInterval);
    pRow->setContentsMargins(0, 0, 0, 0);
    pRow->setSpacing(scmEditorRowControlGap);

    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    pGrid->getItemPosition(labelIndex, &row, &column, &rowSpan, &columnSpan);
    pGrid->addWidget(mpWidget_timerInterval, row, column + 1, 1, std::max(1, pGrid->columnCount() - column - 1));

    showTimerIntervalSentence(false);
}

void dlgTriggerEditor::showTimerIntervalSentence(const bool offsetTimer)
{
    if (!mpWidget_timerInterval || (mTimerIntervalRowBuilt && mTimerIntervalOffset == offsetTimer)) {
        return;
    }
    mTimerIntervalRowBuilt = true;
    mTimerIntervalOffset = offsetTimer;

    const QList<QWidget*> fields{mpTimersMainArea->timeEdit_timer_hours, mpTimersMainArea->timeEdit_timer_minutes, mpTimersMainArea->timeEdit_timer_seconds, mpTimersMainArea->timeEdit_timer_msecs};

    // A screen reader names a field by its own name, never by the words beside it
    //: Accessible name of the hours field of a timer's interval
    mpTimersMainArea->timeEdit_timer_hours->setAccessibleName(tr("Hours"));
    //: Accessible name of the minutes field of a timer's interval
    mpTimersMainArea->timeEdit_timer_minutes->setAccessibleName(tr("Minutes"));
    //: Accessible name of the seconds field of a timer's interval
    mpTimersMainArea->timeEdit_timer_seconds->setAccessibleName(tr("Seconds"));
    //: Accessible name of the milliseconds field of a timer's interval
    mpTimersMainArea->timeEdit_timer_msecs->setAccessibleName(tr("Milliseconds"));

    auto* pRow = qobject_cast<QHBoxLayout*>(mpWidget_timerInterval->layout());
    while (QLayoutItem* pItem = pRow->takeAt(0)) {
        QWidget* pWord = pItem->widget();
        delete pItem;
        if (pWord && !fields.contains(pWord)) {
            delete pWord;
        }
    }
    for (QWidget* pField : fields) {
        // The .ui grid still holds these the first time, and adding a widget to a second layout warns once per widget
        uiDesign::detachFromLayout(pField);
    }

    QString sentence;
    if (offsetTimer) {
        //: How long after the timer above it an offset timer fires. %1, %2, %3 and %4 are the hour, minute, second and millisecond fields in that order, and may be put wherever the language reads them.
        sentence = tr("Fires once, %1 h %2 min %3 s %4 ms after the timer above it fires");
    } else {
        //: How often a timer fires. %1, %2, %3 and %4 are the hour, minute, second and millisecond fields in that order, and may be put wherever the language reads them.
        sentence = tr("Fires every %1 h %2 min %3 s %4 ms");
    }
    uiDesign::buildControlSentenceRow(pRow, sentence, fields);
    pRow->addStretch(1);

    for (QLabel* pWord : mpWidget_timerInterval->findChildren<QLabel*>()) {
        pWord->setProperty("editorRowLabel", true);
        uiDesign::repolish(pWord);
    }
}

// Read from the timer, not from what was drawn: a move can change its parent without the form reloading
void dlgTriggerEditor::refreshShownTimerIntervalSentence()
{
    if (!mpCurrentTimerItem) {
        return;
    }
    if (TTimer* pT = mpHost->getTimerUnit()->getTimer(mpCurrentTimerItem->data(0, Qt::UserRole).toInt()); pT) {
        showTimerIntervalSentence(pT->isOffsetTimer());
    }
}

void dlgTriggerEditor::buildKeyBindingRow()
{
    auto* pGrid = qobject_cast<QGridLayout*>(mpKeysMainArea->layout());
    QLineEdit* pField = mpKeysMainArea->lineEdit_key_binding;
    const int labelIndex = pGrid ? pGrid->indexOf(mpKeysMainArea->label_key_binding) : -1;
    if (labelIndex < 0) {
        return;
    }

    pField->setFocusPolicy(Qt::StrongFocus);
    pField->setCursor(Qt::PointingHandCursor);
    //: Tooltip on the field holding the keystroke a key answers; the field listens for the keystroke itself rather than being typed into
    pField->setToolTip(utils::richText(tr("The keystroke this key answers. Click it, then press the keys you want.")));
    pField->installEventFilter(this);

    mpButton_keyClear = new QToolButton(mpKeysMainArea);
    mpButton_keyClear->setObjectName(qsl("editorKeyClear"));
    mpButton_keyClear->setAutoRaise(true);
    mpButton_keyClear->setFocusPolicy(Qt::NoFocus);
    mpButton_keyClear->setIconSize(QSize(scmEditorPatternDeleteGlyphSize, scmEditorPatternDeleteGlyphSize));
    //: Tooltip on the cross that takes the keystroke off a key, leaving it bound to nothing
    mpButton_keyClear->setToolTip(utils::richText(tr("Forget this key")));

    mpLabel_keyHint = new QLabel(mpKeysMainArea);
    mpLabel_keyHint->setObjectName(qsl("editorKeyHint"));
    mpLabel_keyHint->setProperty("editorRowLabel", true);

    auto* pRowWidget = new QWidget(mpKeysMainArea);
    pRowWidget->setObjectName(qsl("editorKeyBindingRow"));
    pRowWidget->setProperty("editorPanelSurface", true);
    mpWidget_keyBindingRow = pRowWidget;
    auto* pRow = new QHBoxLayout(pRowWidget);
    pRow->setContentsMargins(0, 0, 0, 0);
    pRow->setSpacing(scmEditorRowControlGap);
    uiDesign::detachFromLayout(pField);
    pRow->addWidget(pField, 1);
    pRow->addWidget(mpButton_keyClear);
    pRow->addWidget(mpLabel_keyHint);
    pRow->addStretch(1);

    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    pGrid->getItemPosition(labelIndex, &row, &column, &rowSpan, &columnSpan);
    pGrid->addWidget(pRowWidget, row, column + 1, 1, std::max(1, pGrid->columnCount() - column - 1));

    // Through the capture path, so forgetting is one undo entry like any other change to the key
    connect(mpButton_keyClear, &QAbstractButton::clicked, this, [this]() {
        endKeyGrab();
        keyGrabCallback(Qt::Key_unknown, Qt::NoModifier);
        slot_itemEdited();
        showKeyBinding();
    });
}

void dlgTriggerEditor::buildVariableTypeRows()
{
    auto* pGrid = qobject_cast<QGridLayout*>(mpVarsMainArea->frame_vars_main_area->layout());
    const int labelIndex = pGrid ? pGrid->indexOf(mpVarsMainArea->label_variable_key) : -1;
    if (labelIndex < 0) {
        return;
    }

    // Read before anything leaves the grid: detaching an item renumbers the ones after it
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    pGrid->getItemPosition(labelIndex, &row, &column, &rowSpan, &columnSpan);
    const int span = std::max(1, pGrid->columnCount() - column - 1);

    auto* pRowWidget = new QWidget(mpVarsMainArea->frame_vars_main_area);
    pRowWidget->setObjectName(qsl("editorVariableTypes"));
    pRowWidget->setProperty("editorPanelSurface", true);
    auto* pRow = new QHBoxLayout(pRowWidget);
    pRow->setContentsMargins(0, 0, 0, 0);
    pRow->setSpacing(scmEditorRowControlGap);
    for (QWidget* pControl : {static_cast<QWidget*>(mpVarsMainArea->comboBox_variable_key_type),
                              static_cast<QWidget*>(mpVarsMainArea->label_variable_value),
                              static_cast<QWidget*>(mpVarsMainArea->comboBox_variable_value_type)}) {
        // Still in the .ui grid, and adding a widget to a second layout warns once per widget
        uiDesign::detachFromLayout(pControl);
        pRow->addWidget(pControl);
    }
    pRow->addStretch(1);

    pGrid->addWidget(pRowWidget, row, column + 1, 1, span);
}

void dlgTriggerEditor::buildActionRows()
{
    auto* pGrid = qobject_cast<QGridLayout*>(mpActionsMainArea->layout());
    const int labelIndex = pGrid ? pGrid->indexOf(mpActionsMainArea->label_action_button_rotation) : -1;
    if (labelIndex < 0) {
        return;
    }

    // Read before anything leaves the grid: detaching an item renumbers the ones after it
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    pGrid->getItemPosition(labelIndex, &row, &column, &rowSpan, &columnSpan);
    const int span = std::max(1, pGrid->columnCount() - column - 1);

    auto* pRowWidget = new QWidget(mpActionsMainArea);
    pRowWidget->setObjectName(qsl("editorActionRotationRow"));
    pRowWidget->setProperty("editorPanelSurface", true);
    mpWidget_actionRotationRow = pRowWidget;
    auto* pRow = new QHBoxLayout(pRowWidget);
    pRow->setContentsMargins(0, 0, 0, 0);
    pRow->setSpacing(scmEditorRowControlGap);
    for (QWidget* pControl : {static_cast<QWidget*>(mpActionsMainArea->comboBox_action_button_rotation), static_cast<QWidget*>(mpActionsMainArea->checkBox_action_button_isPushDown)}) {
        // Still in the .ui grid, and adding a widget to a second layout warns once per widget
        uiDesign::detachFromLayout(pControl);
        pRow->addWidget(pControl);
    }
    pRow->addStretch(1);
    pGrid->addWidget(pRowWidget, row, column + 1, 1, span);

    // The stylesheet field grows with the column, so its label sits level with its first line. The row's stretch
    // is the .ui file's, and insertGridRowAtTop() carries it down with the row.
    pGrid->setAlignment(mpActionsMainArea->label_action_css, Qt::AlignLeft | Qt::AlignTop);
}

// A key group gets no binding row: TKey::match() never matches a folder
void dlgTriggerEditor::showKeyBinding()
{
    if (!mpLabel_keyHint) {
        return;
    }

    // A grab still waiting for a keystroke would contradict the row about to say what the key holds
    releaseKeyGrab();

    // Not getKeyFromTreeItem(), which answers null for a row with no parent
    TKey* pKey = mpCurrentKeyItem ? mpHost->getKeyUnit()->getKey(mpCurrentKeyItem->data(0, Qt::UserRole).toInt()) : nullptr;
    const bool bindable = pKey && !pKey->isFolder();
    QLineEdit* pField = mpKeysMainArea->lineEdit_key_binding;
    mpKeysMainArea->label_key_binding->setVisible(bindable);
    // The whole row too: a visible empty widget still takes a grid row, dead height the code pane is measured against
    if (mpWidget_keyBindingRow) {
        mpWidget_keyBindingRow->setVisible(bindable);
    }
    pField->setVisible(bindable);
    mpLabel_keyHint->setVisible(bindable);
    if (!bindable) {
        mpButton_keyClear->hide();
        return;
    }

    const bool bound = pKey->getKeyCode() != Qt::Key_unknown && pKey->getKeyCode() != Qt::Key(0);
    pField->setProperty("editorListening", false);
    uiDesign::repolish(pField);
    pField->setCursor(Qt::PointingHandCursor);
    pField->setText(bound ? mpHost->getKeyUnit()->getKeyName(pKey->getKeyCode(), pKey->getKeyModifiers()) : QString());
    pField->setPlaceholderText(unboundKeyText());
    if (bound) {
        //: Beside a key's keystroke, saying that clicking the field is how it is changed
        mpLabel_keyHint->setText(tr("Click to change"));
    } else {
        //: Beside the empty key binding field, saying that clicking it is how a keystroke is set
        mpLabel_keyHint->setText(tr("Click to set"));
    }
    mpButton_keyClear->setVisible(bound);
}

void dlgTriggerEditor::showKeyBindingListening()
{
    if (!mpLabel_keyHint) {
        return;
    }

    QLineEdit* pField = mpKeysMainArea->lineEdit_key_binding;
    pField->clear();
    //: Stands in the key binding field while the editor waits for the user to press the keys to bind
    pField->setPlaceholderText(tr("Press a key combination"));
    //: Beside the key binding field while it waits for a keystroke, saying how to leave the keystroke as it was
    mpLabel_keyHint->setText(tr("Escape keeps the current key"));
    pField->setProperty("editorListening", true);
    uiDesign::repolish(pField);
    pField->setCursor(Qt::ArrowCursor);
}

void dlgTriggerEditor::releaseKeyGrab()
{
    if (!mIsGrabKey) {
        return;
    }
    mIsGrabKey = false;
    restoreShortcuts();
    QCoreApplication::instance()->removeEventFilter(this);
}

void dlgTriggerEditor::endKeyGrab()
{
    if (!mIsGrabKey) {
        return;
    }
    releaseKeyGrab();
    showKeyBinding();
}

bool dlgTriggerEditor::handleKeyBindingFieldEvent(QEvent* pEvent)
{
    switch (pEvent->type()) {
    case QEvent::MouseButtonPress:
        if (!mIsGrabKey && static_cast<QMouseEvent*>(pEvent)->button() == Qt::LeftButton) {
            mpKeysMainArea->lineEdit_key_binding->setFocus(Qt::MouseFocusReason);
            slot_keyGrab();
            return true;
        }
        return false;

    case QEvent::KeyPress:
        // The field holds the keyboard while armed, so the key goes to event(), where a grab is read; a read-only
        // QLineEdit would eat the arrows and Ctrl+A on the way
        if (mIsGrabKey) {
            event(pEvent);
            return true;
        }
        switch (static_cast<QKeyEvent*>(pEvent)->key()) {
        case Qt::Key_Return:
            [[fallthrough]];
        case Qt::Key_Enter:
            [[fallthrough]];
        case Qt::Key_Space:
            slot_keyGrab();
            return true;
        default:
            return false;
        }

    case QEvent::FocusOut:
        // Leaving the field is Escape by another route: the keystroke stands
        endKeyGrab();
        return false;

    default:
        return false;
    }
}

QList<QLabel*> dlgTriggerEditor::editorFormRowLabels() const
{
    return {mpLabel_optionsRow,
            mpAliasMainArea->label_alias_name,
            mpTimersMainArea->label_timer_name,
            mpKeysMainArea->label_key_name,
            mpScriptsMainArea->label_script_name,
            mpActionsMainArea->label_action_name,
            mpVarsMainArea->label_variable_name,
            mpAliasMainArea->label_alias_command,
            mpTimersMainArea->label_timer_command,
            mpKeysMainArea->label_key_command,
            mpAliasMainArea->label_alias_pattern,
            mpTimersMainArea->label_timer_time,
            mpKeysMainArea->label_key_binding,
            mpScriptsMainArea->label_script_registered_event_handlers,
            mpVarsMainArea->label_variable_key,
            mpVarsMainArea->label_variable_value,
            mpActionsMainArea->label_action_button_rotation,
            mpActionsMainArea->label_action_button_command_down,
            mpActionsMainArea->label_action_button_command_up,
            mpActionsMainArea->label_action_icon,
            mpActionsMainArea->label_action_bar_location,
            mpActionsMainArea->label_action_bar_orientation,
            mpActionsMainArea->label_action_bar_columns,
            mpActionsMainArea->label_action_bar_offsetToFirstButton,
            mpActionsMainArea->label_action_css};
}

QList<QLabel*> dlgTriggerEditor::editorFormLeadLabels() const
{
    return {mpLabel_optionsRow,
            mpAliasMainArea->label_alias_pattern,
            mpTimersMainArea->label_timer_time,
            mpKeysMainArea->label_key_binding,
            mpScriptsMainArea->label_script_registered_event_handlers,
            mpVarsMainArea->label_variable_key,
            mpActionsMainArea->label_action_button_rotation,
            mpActionsMainArea->label_action_button_command_down,
            mpActionsMainArea->label_action_button_command_up,
            mpActionsMainArea->label_action_icon,
            mpActionsMainArea->label_action_bar_location,
            mpActionsMainArea->label_action_bar_orientation,
            mpActionsMainArea->label_action_bar_columns,
            mpActionsMainArea->label_action_bar_offsetToFirstButton,
            mpActionsMainArea->label_action_css};
}

// The name label sits in the head row, laid out with a different gap, so it is given the difference on top.
// Measured, not named in a stylesheet: a word's width depends on the font the window is running at.
void dlgTriggerEditor::alignEditorFormLeadLabels()
{
    if (!mpAliasMainArea) {
        return;
    }

    int leadWidth = 0;
    for (QLabel* pLabel : editorFormLeadLabels()) {
        leadWidth = std::max(leadWidth, pLabel->fontMetrics().horizontalAdvance(pLabel->text()));
    }

    int nameWidth = 0;
    const QList<QLabel*> nameLabels{mpTriggersMainArea->label_trigger_name,
                                    mpAliasMainArea->label_alias_name,
                                    mpTimersMainArea->label_timer_name,
                                    mpKeysMainArea->label_key_name,
                                    mpScriptsMainArea->label_script_name,
                                    mpActionsMainArea->label_action_name,
                                    mpVarsMainArea->label_variable_name};
    for (QLabel* pLabel : nameLabels) {
        nameWidth = std::max(nameWidth, pLabel->fontMetrics().horizontalAdvance(pLabel->text()));
    }
    leadWidth = std::max(leadWidth, nameWidth - scmEditorFormNameLabelExtra);

    for (QLabel* pLabel : editorFormLeadLabels()) {
        pLabel->setFixedWidth(leadWidth);
    }
    for (QLabel* pLabel : nameLabels) {
        pLabel->setFixedWidth(leadWidth + scmEditorFormNameLabelExtra);
        pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (mpChipRow_scriptEvents) {
        mpScriptsMainArea->label_script_registered_event_handlers->setFixedHeight(mpChipRow_scriptEvents->lineHeight());
    }
}

void dlgTriggerEditor::fitEditorOptionsSpinBoxes()
{
    if (!mpSpinBox_matchWithinLines || !mpTriggersMainArea) {
        return;
    }

    for (QSpinBox* pBox : {mpSpinBox_matchWithinLines, mpTriggersMainArea->spinBox_stayOpen}) {
        const QFontMetrics metrics = pBox->fontMetrics();
        // Zero is the widest digit in most faces but not in all
        QChar widest = QLatin1Char('0');
        for (char digit = '0'; digit <= '9'; ++digit) {
            if (metrics.horizontalAdvance(QLatin1Char(digit)) > metrics.horizontalAdvance(widest)) {
                widest = QLatin1Char(digit);
            }
        }
        const int digits = QString::number(pBox->maximum()).length();
        const int room = metrics.horizontalAdvance(QString(digits, widest)) + 2 * (uiDesign::scmInputPaddingHorizontal + uiDesign::scmInputBorderWidth) + uiDesign::scmInputStepperWidth
                         + scmEditorOptionsSpinBoxSlack;
        pBox->setFixedWidth(room);
    }
}

// Signals blocked both ways: the segments write back into spinBox_lineMargin
void dlgTriggerEditor::reflectTriggerMatchMode()
{
    if (!mpRadioButton_matchAny) {
        return;
    }

    const int lineDelta = mpTriggersMainArea->spinBox_lineMargin->value();
    const bool allMode = lineDelta >= 0;
    {
        const QSignalBlocker anyBlocker(mpRadioButton_matchAny);
        const QSignalBlocker allBlocker(mpRadioButton_matchAll);
        const QSignalBlocker withinBlocker(mpSpinBox_matchWithinLines);
        mpRadioButton_matchAny->setChecked(!allMode);
        mpRadioButton_matchAll->setChecked(allMode);
        mpSpinBox_matchWithinLines->setValue(std::max(0, lineDelta));
    }
    // Also off while the segments are: checkForMoreThanOneTriggerItem() disables them for a single pattern
    mpWidget_matchWithinRow->setEnabled(allMode && mpWidget_matchModeRows->isEnabled());
}

void dlgTriggerEditor::setTriggerOptionsShown(const bool shown)
{
    mShowAllTriggerControls = shown;
    slot_showAllTriggerControls(shown);
}

void dlgTriggerEditor::slot_showAllTriggerControls(const bool isShown)
{
    QToolButton* pToggle = mpTriggersMainArea->toolButton_toggleExtraControls;
    if (pToggle->isChecked() != isShown) {
        pToggle->setChecked(isShown);
    }

    // Unconditionally: isVisible() is also false while the whole form is hidden, as during construction, which
    // would skip the hide that keeps the strip away once the form shows
    showEditorFormRow(isShown, {mpLabel_optionsRow, mpWidget_triggerOptionsRow});

    updatePatternTabOrder();

    if (mCurrentView == EditorViewType::cmTriggerView) {
        fitFormPaneToItsContents();
    }
}

// The path, which save and load read, rides on a property: drawn in a field this size it would be the tail of a
// directory
void dlgTriggerEditor::showTriggerSoundFile(const QString& path)
{
    QLineEdit* pField = mpTriggersMainArea->lineEdit_soundFile;
    pField->setProperty(cSoundFilePath, path);
    const QString name = path.isEmpty() ? QString() : QFileInfo(path).fileName();
    pField->setText(pField->fontMetrics().elidedText(name, Qt::ElideMiddle, scmEditorSoundFileWidth - scmEditorSoundFileTextInset));
    //: Tooltip on the sound file field, which opens the file chooser when it is clicked
    const QString tip = tr("Choose the sound file to play. Choosing one turns the sound on.");
    pField->setToolTip(utils::richText(path.isEmpty() ? tip : qsl("%1<br>%2").arg(path.toHtmlEscaped(), tip)));
    // The file, not the sentence about choosing one, which the accessible name already says
    pField->setAccessibleDescription(path.isEmpty() ? pField->placeholderText() : name);
    mpTriggersMainArea->toolButton_clearSoundFile->setVisible(!path.isEmpty());
}

QString dlgTriggerEditor::triggerSoundFilePath() const
{
    return mpTriggersMainArea->lineEdit_soundFile->property(cSoundFilePath).toString();
}

bool dlgTriggerEditor::handleSoundFileFieldEvent(QEvent* pEvent)
{
    switch (pEvent->type()) {
    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent*>(pEvent)->button() == Qt::LeftButton) {
            mpTriggersMainArea->lineEdit_soundFile->setFocus(Qt::MouseFocusReason);
            slot_soundTrigger();
            return true;
        }
        return false;

    case QEvent::KeyPress:
        switch (static_cast<QKeyEvent*>(pEvent)->key()) {
        case Qt::Key_Return:
            [[fallthrough]];
        case Qt::Key_Enter:
            [[fallthrough]];
        case Qt::Key_Space:
            slot_soundTrigger();
            return true;
        default:
            return false;
        }

    default:
        return false;
    }
}

// A well otherwise says what it holds by its colour alone
void dlgTriggerEditor::describeColourWell(QPushButton* pWell)
{
    const QString colour = pWell->property(cButtonBaseColor).toString();
    if (colour.isEmpty() || colour == QLatin1String("transparent")) {
        //: Read out for a trigger's colour well that leaves the game's own colour alone
        pWell->setAccessibleDescription(tr("keeps the game's colour"));
        return;
    }
    pWell->setAccessibleDescription(colour);
}

void dlgTriggerEditor::slot_rightSplitterMoved(const int, const int)
{
    // splitterMoved() comes from a drag, never from setSizes(), so this is the one place a user's height is heard
    const QList<int> movedTo = splitter_right->sizes();
    if (mCurrentView != EditorViewType::cmUnknownView && !movedTo.isEmpty()) {
        // Net of any notice up during the drag: moveSeamByNoticeChange() gives a notice its room out of this pane,
        // so a height recorded with one inside would be read back as form room and the next notice taken off twice
        mDraggedFormPaneHeights.insert(mCurrentView, std::max(0, movedTo.constFirst() - mNoticeRoomInFormColumn));
    }
    if (mpSourceEditorFindArea->isVisible()) {
        slot_sourceFindMove();
    }
}

// Only for other classes to set the options - as they will not be carried from
// here to the parent Host instance, whereas the slots that change the
// individual options DO also notify that Host instance about the changes they
// make:
void dlgTriggerEditor::setSearchOptions(const enums::EditorSearchOptions optionsState)
{
    mSearchOptions = optionsState;
    mpAction_searchCaseSensitive->setChecked(optionsState & enums::EditorSearchOptionCaseSensitive);
    mpAction_searchIncludeVariables->setChecked(optionsState & enums::EditorSearchOptionIncludeVariables);
    mpAction_searchWholeWord->setChecked(optionsState & enums::EditorSearchOptionWholeWord);
    createSearchOptionIcon();
}

void dlgTriggerEditor::showOrHideRestoreEditorActionsToolbarAction()
{
    if ((!toolBar->isVisible()) || toolBar->isFloating()
        || (QMainWindow::toolBarArea(toolBar) & (Qt::ToolBarArea::LeftToolBarArea | Qt::ToolBarArea::RightToolBarArea | Qt::ToolBarArea::BottomToolBarArea))) {
        // If it is NOT visible
        // OR If the toolbar is floating
        // OR it is docked in an area other than the top one
        // then show the restore action
        mpAction_restoreEditorActionsToolbar->setVisible(true);
    } else {
        // Otherwise - i.e. it is visible AND docked AND docked to the original
        // area:
        mpAction_restoreEditorActionsToolbar->setVisible(false);
    }
}

void dlgTriggerEditor::slot_visibilityChangedEditorActionsToolbar()
{
    showOrHideRestoreEditorActionsToolbarAction();
}

// Gets triggered twice during the dragging of the toolbar from one docking
// area to another - as it briefly floats during the drag:
void dlgTriggerEditor::slot_floatingChangedEditorActionsToolbar()
{
    showOrHideRestoreEditorActionsToolbarAction();

    // A torn-off bar is sized to what it holds, so it starts with every name shown and that resize refits it;
    // sized to the pictures alone it could never find the room to ask for its names back
    if (toolBar->isFloating()) {
        for (EditorToolBarGroup& group : mEditorToolBarGroups) {
            group.labelsShown = true;
            setToolBarButtonLabelsShown(toolBar, group.actions, true);
        }
    }
}

// This also triggers the corresponding signal that is connected to the
// showOrHideRestoreEditorActionsToolbarAction() SLOT:
void dlgTriggerEditor::slot_restoreEditorActionsToolbar()
{
    if (!toolBar->isVisible()) {
        // Reshow it
        toolBar->show();
    }
    // Forces it to redock in the starting area:
    QMainWindow::addToolBar(Qt::TopToolBarArea, toolBar);
}

void dlgTriggerEditor::clearTriggerForm()
{
    // Forgotten first, so neither the clearing below nor a later save is taken as an edit to that trigger
    mpCurrentTriggerItem = nullptr;

    // Clear pattern fields
    for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
        patternEdit->singleLineTextEdit_pattern->clear();
        if (patternEdit->singleLineTextEdit_pattern->isHidden()) {
            patternEdit->singleLineTextEdit_pattern->show();
        }
        patternEdit->pushButton_fgColor->hide();
        patternEdit->pushButton_bgColor->hide();
        patternEdit->label_prompt->hide();
        patternEdit->spinBox_lineSpacer->hide();
        // Nudge the type up and down so that the appropriate (coloured) icon is copied across to the QLineEdit:
        patternEdit->comboBox_patternType->setCurrentIndex(1);
        patternEdit->comboBox_patternType->setCurrentIndex(0);
    }

    mpTriggersMainArea->lineEdit_trigger_name->clear();
    mpTriggersMainArea->label_idNumber->clear();
    clearDocument(mpSourceEditorEdbee);
    mpTriggersMainArea->lineEdit_trigger_command->clear();

    mpTriggersMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearTimerForm()
{
    mpTimersMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearAliasForm()
{
    mpAliasMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearScriptForm()
{
    mpScriptsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearActionForm()
{
    mpActionsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearKeyForm()
{
    mpKeysMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::clearVarForm()
{
    mpVarsMainArea->hide();
    mpSourceEditorArea->hide();
    if (mCurrentView != EditorViewType::cmUnknownView) {
        showIntro();
    }
}

void dlgTriggerEditor::setEditorShowBidi(const bool state)
{
    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setRenderBidiContolCharacters(state);
    config->endChanges();
    mpSourceEditorEdbee->controller()->update();
}

void dlgTriggerEditor::hideSystemMessageArea()
{
    mpSystemMessageArea->hide();
}

// Mirrors dlgProfilePreferences::buildSidebar(), so that the two windows read as one design
void dlgTriggerEditor::buildEditorSidebar()
{
    mpWidget_editorSidebarPane = new QWidget(this);
    mpWidget_editorSidebarPane->setObjectName(qsl("editorSidebarPane"));
    mpWidget_editorSidebarPane->setFixedWidth(scmEditorSidebarRailWidth);
    auto* pSidebarLayout = new QVBoxLayout(mpWidget_editorSidebarPane);
    pSidebarLayout->setContentsMargins(scmEditorSidebarPadding, scmEditorSidebarVerticalPadding, scmEditorSidebarPadding, scmEditorSidebarVerticalPadding);
    pSidebarLayout->setSpacing(4);

    mpListWidget_editorSidebar = new QListWidget(mpWidget_editorSidebarPane);
    mpListWidget_editorSidebar->setObjectName(qsl("editorSidebar"));
    mpListWidget_editorSidebar->setFrameShape(QFrame::NoFrame);
    mpListWidget_editorSidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpListWidget_editorSidebar->setIconSize(QSize(mEditorIconSize, mEditorIconSize));
    mpListWidget_editorSidebar->setItemDelegate(new uiDesign::SidebarItemDelegate(mpListWidget_editorSidebar));
    //: Accessible name of the list down the left of the editor that switches between triggers, aliases, scripts and the rest
    mpListWidget_editorSidebar->setAccessibleName(tr("Editor sections"));
    // Which of its rows carries the focus ring is a property eventFilter() puts on
    mpListWidget_editorSidebar->installEventFilter(this);
    pSidebarLayout->addWidget(mpListWidget_editorSidebar, 1);

    connect(mpListWidget_editorSidebar, &QListWidget::currentRowChanged, this, &dlgTriggerEditor::slot_editorSidebarRowChanged);
    // One-off rows run on a click or Return rather than on the current row, so arrowing past cannot toggle a
    // console. A style that activates on one click sends both; slot_editorSidebarItemActivated() drops the second.
    connect(mpListWidget_editorSidebar, &QListWidget::itemClicked, this, &dlgTriggerEditor::slot_editorSidebarItemActivated);
    connect(mpListWidget_editorSidebar, &QListWidget::itemActivated, this, &dlgTriggerEditor::slot_editorSidebarItemActivated);
    invalidateEditorSidebarWidths();
}

void dlgTriggerEditor::updateEditorSidebarToggle()
{
    if (!mpToggle_editorSidebar) {
        return;
    }
    const bool namesShowing = mEditorSidebarLabelsShown && mEditorSidebarNamesFit;
    mpToggle_editorSidebar->setPointingLeft(namesShowing);

    //: Tooltip and accessible name of the chevron on the line down the right of the script editor's sidebar while the section names are showing. Pressing it leaves the sidebar in place as a rail of icons - it closes nothing - so the wording is about the labels rather than the sidebar.
    const QString minimise = tr("Minimise the sidebar to icons");
    //: Tooltip and accessible name of that same chevron while the sidebar is a rail of icons. Pressing it puts the section names back beside the icons.
    const QString showLabels = tr("Show the sidebar's labels");
    const QString wording = namesShowing ? minimise : showLabels;
    // The chevron is drawn, so its text is only what a screen reader is given
    mpToggle_editorSidebar->setText(wording);
    mpToggle_editorSidebar->setAccessibleName(wording);
    // In a window too narrow for the names it is disabled with a reason, rather than doing nothing when pressed
    mpToggle_editorSidebar->setEnabled(mEditorSidebarNamesFit);
    //: Tooltip on that same chevron when the editor window is too narrow to fit the section names, which is why pressing it is not offered.
    mpToggle_editorSidebar->setToolTip(utils::richText(mEditorSidebarNamesFit ? wording : tr("The window is too narrow for the sidebar's labels")));
    // Every path that can move the seam comes through here
    mpToggle_editorSidebar->reposition();
}

// The one place the preference changes; the width-driven collapse in updateEditorSidebarMode() bypasses it
void dlgTriggerEditor::setEditorSidebarLabelsShown(const bool shown)
{
    mEditorSidebarLabelsShown = shown;
    updateEditorSidebarMode();
}

// No icon yet: its tint is unknown until applyEditorShellStyle() has read the theme
QListWidgetItem* dlgTriggerEditor::addEditorSidebarRow(QAction* pAction, const EditorViewType view, const QString& iconFile)
{
    auto* pItem = new QListWidgetItem(pAction->text(), mpListWidget_editorSidebar);
    pItem->setData(scmRole_editorSidebarAction, QVariant::fromValue(pAction));
    pItem->setData(scmRole_editorSidebarView, static_cast<int>(view));
    pItem->setSizeHint(QSize(0, scmEditorSidebarRowHeight));
    pItem->setToolTip(pAction->toolTip());
    mEditorSidebarGlyphs.append({pItem, iconFile});
    mEditorViewActions.append(pAction);
    invalidateEditorSidebarWidths();
    return pItem;
}

void dlgTriggerEditor::addEditorSidebarSeparator()
{
    auto* pItem = new QListWidgetItem(mpListWidget_editorSidebar);
    pItem->setFlags(Qt::NoItemFlags);
    pItem->setSizeHint(QSize(0, 17));
    auto* pLine = new QFrame(mpListWidget_editorSidebar);
    pLine->setObjectName(qsl("editorSidebarSeparator"));
    pLine->setFrameShape(QFrame::HLine);
    mpListWidget_editorSidebar->setItemWidget(pItem, pLine);
}

// The colour of the old picture of each view, shared by its sidebar row and its tree's heading row
static uiDesign::IdentityColour editorViewIdentity(const QString& glyphFile)
{
    static const QHash<QString, uiDesign::IdentityColour> identities{{qsl(":/icons/editor-triggers.svg"), uiDesign::scmIdentity_triggers},
                                                                     {qsl(":/icons/editor-aliases.svg"), uiDesign::scmIdentity_input},
                                                                     {qsl(":/icons/editor-buttons.svg"), uiDesign::scmIdentity_buttons},
                                                                     {qsl(":/icons/editor-variables.svg"), uiDesign::scmIdentity_variables},
                                                                     {qsl(":/icons/editor-folder.svg"), uiDesign::scmIdentity_folder},
                                                                     {qsl(":/icons/editor-errors.svg"), uiDesign::scmIdentity_errors},
                                                                     {qsl(":/icons/editor-statistics.svg"), uiDesign::scmIdentity_statistics},
                                                                     {qsl(":/icons/editor-debug.svg"), uiDesign::scmIdentity_debug}};
    return mudlet::self() && mudlet::self()->colouredIcons() ? identities.value(glyphFile) : uiDesign::IdentityColour();
}

void dlgTriggerEditor::restyleEditorSidebarIcons(const QColor& normal, const QColor& selected)
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    for (const auto& glyph : std::as_const(mEditorSidebarGlyphs)) {
        if (const uiDesign::IdentityColour identity = editorViewIdentity(glyph.second); identity.hue >= 0.0) {
            glyph.first->setIcon(uiDesign::identityIcon(glyph.second, glyph.second, identity, tokens, tokens.disabledText));
            continue;
        }
        const QPixmap source = uiDesign::glyphPixmap(glyph.second);
        QIcon icon(uiDesign::tintedGlyph(source, normal));
        // Otherwise the view makes one by washing the icon in the highlight colour
        icon.addPixmap(uiDesign::tintedGlyph(source, selected), QIcon::Selected);
        glyph.first->setIcon(icon);
    }
}

// A QIcon set on a tree item is never re-derived, so every path that makes a heading row calls this:
// fillout_form(), repopulateVars() for the variables heading, and restyleEditorIcons() on a theme change
void dlgTriggerEditor::restyleEditorTreeHeadingIcons()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QList<QPair<QTreeWidgetItem*, QString>> headings{{mpTriggerBaseItem, qsl(":/icons/editor-triggers.svg")},
                                                           {mpAliasBaseItem, qsl(":/icons/editor-aliases.svg")},
                                                           {mpScriptsBaseItem, qsl(":/icons/editor-scripts.svg")},
                                                           {mpTimerBaseItem, qsl(":/icons/editor-timers.svg")},
                                                           {mpKeyBaseItem, qsl(":/icons/editor-keys.svg")},
                                                           {mpActionBaseItem, qsl(":/icons/editor-buttons.svg")},
                                                           {mpVarBaseItem, qsl(":/icons/editor-variables.svg")}};
    for (const auto& heading : headings) {
        if (!heading.first) {
            continue;
        }
        if (const uiDesign::IdentityColour identity = editorViewIdentity(heading.second); identity.hue >= 0.0) {
            heading.first->setIcon(0, uiDesign::identityIcon(heading.second, heading.second, identity, tokens, tokens.disabledText));
            continue;
        }
        const QPixmap source = uiDesign::glyphPixmap(heading.second);
        QIcon icon(uiDesign::tintedGlyph(source, tokens.mutedText));
        icon.addPixmap(uiDesign::tintedGlyph(source, tokens.accentText), QIcon::Selected);
        heading.first->setIcon(0, icon);
    }
}

// The view also changes from deep links, search results and shortcuts, so the row follows the view - see changeView()
void dlgTriggerEditor::syncEditorSidebarSelection()
{
    if (!mpListWidget_editorSidebar || (mCurrentView == EditorViewType::cmUnknownView && !mFolderViewActive)) {
        return;
    }
    for (int row = 0, rows = mpListWidget_editorSidebar->count(); row < rows; ++row) {
        QListWidgetItem* pItem = mpListWidget_editorSidebar->item(row);
        // The Folders view keeps the type of the item it opened as the current view
        const bool foldersRow = pItem->data(scmRole_editorSidebarFolders).toBool();
        if (mFolderViewActive ? !foldersRow : (foldersRow || static_cast<EditorViewType>(pItem->data(scmRole_editorSidebarView).toInt()) != mCurrentView)) {
            continue;
        }
        // Blocked, or choosing the row would ask for the view it came from
        const QSignalBlocker blocker(mpListWidget_editorSidebar);
        mpListWidget_editorSidebar->setCurrentItem(pItem);
        return;
    }
}

void dlgTriggerEditor::slot_editorSidebarRowChanged(const int row)
{
    QListWidgetItem* pItem = mpListWidget_editorSidebar->item(row);
    if (!pItem) {
        return;
    }
    if (pItem->data(scmRole_editorSidebarFolders).toBool()) {
        if (!mFolderViewActive) {
            mpViewFoldersAction->trigger();
        }
        return;
    }
    const auto view = static_cast<EditorViewType>(pItem->data(scmRole_editorSidebarView).toInt());
    // cmUnknownView marks a one-off action row, which is not somewhere to go
    if (view == EditorViewType::cmUnknownView || (view == mCurrentView && !mFolderViewActive)) {
        return;
    }
    if (auto* pAction = qvariant_cast<QAction*>(pItem->data(scmRole_editorSidebarAction))) {
        pAction->trigger();
    }
}

// Errors, Statistics and Debug are actions, not views: they run here and the chosen row goes back to the view on show
void dlgTriggerEditor::slot_editorSidebarItemActivated(QListWidgetItem* pItem)
{
    if (!pItem || pItem->data(scmRole_editorSidebarFolders).toBool() || static_cast<EditorViewType>(pItem->data(scmRole_editorSidebarView).toInt()) != EditorViewType::cmUnknownView) {
        return;
    }
    // On a style that activates on one click, the click and the activation arrive in the same event-loop pass;
    // a Return press's activation comes on a pass of its own, after the guard is released
    if (mEditorSidebarActionInFlight) {
        return;
    }
    mEditorSidebarActionInFlight = true;
    QTimer::singleShot(0ms, this, [this]() {
        mEditorSidebarActionInFlight = false;
    });

    if (auto* pAction = qvariant_cast<QAction*>(pItem->data(scmRole_editorSidebarAction))) {
        pAction->trigger();
    }
    syncEditorSidebarSelection();
}

void dlgTriggerEditor::invalidateEditorSidebarWidths()
{
    mEditorSidebarWidthsKnown = false;
}

// Measured: the interface font, a translation's longer names and the row style all move it
dlgTriggerEditor::EditorSidebarWidths dlgTriggerEditor::editorSidebarWidths() const
{
    if (mEditorSidebarWidthsKnown) {
        return mEditorSidebarWidths;
    }

    EditorSidebarWidths widths;
    // Zero collapses nothing: right for a half-built window, and not an answer to cache
    if (!mpListWidget_editorSidebar) {
        return widths;
    }
    int widestRow = 0;
    for (int row = 0, rows = mpListWidget_editorSidebar->count(); row < rows; ++row) {
        widestRow = std::max(widestRow, uiDesign::sidebarRowWidth(mpListWidget_editorSidebar, mpListWidget_editorSidebar->item(row)->text()));
    }
    widths.expanded = std::clamp(2 * scmEditorSidebarPadding + widestRow, scmEditorSidebarRailWidth, scmEditorSidebarMaximumWidth);

    // Not the expanded width: held equal, the first pixel of an inward drag took the names away. Floored at the
    // content column, not capped: capped, the breakpoint fell at the window's own minimum width, which the strict
    // test below can never reach.
    const int bodyMinimum = splitter_main ? splitter_main->minimumSizeHint().width() : 0;
    widths.collapseBelow = widths.expanded + std::max(scmEditorContentColumnWidth, bodyMinimum);

    mEditorSidebarWidths = widths;
    mEditorSidebarWidthsKnown = true;
    return widths;
}

void dlgTriggerEditor::updateEditorSidebarMode()
{
    if (!mpWidget_editorSidebarPane) {
        return;
    }
    const EditorSidebarWidths widths = editorSidebarWidths();
    if (!widths.expanded) {
        return;
    }
    // The window's width, not the space left: the threshold is the expanded sidebar's, so collapsing cannot flip
    // the test and oscillate
    mEditorSidebarNamesFit = width() >= widths.collapseBelow;
    uiDesign::setSidebarCollapsed(
            mpWidget_editorSidebarPane, mpListWidget_editorSidebar, qsl("editorSidebarSeparator"), !(mEditorSidebarNamesFit && mEditorSidebarLabelsShown), editorSidebarMetrics(widths.expanded));
    updateEditorSidebarToggle();
}

void dlgTriggerEditor::styleSearchOptionsMenu()
{
    QMenu* pMenu_searchOptions = findChild<QMenu*>(qsl("pMenu_searchOptions"));
    if (!pMenu_searchOptions) {
        return;
    }
    pMenu_searchOptions->setStyleSheet(uiDesign::menuStyleSheet(uiDesign::themeTokens()));
    uiDesign::letPopupsTakeTheFieldsCorner(pMenu_searchOptions);
}

void dlgTriggerEditor::restyleEditorIcons()
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QColor quietColor = tokens.mutedText;
    const QColor accentText = tokens.accentText;

    const bool coloured = mudlet::self() && mudlet::self()->colouredIcons();
    for (const auto& glyph : std::as_const(mEditorActionGlyphs)) {
        if (!glyph.pAction) {
            continue;
        }
        const bool onTheBar = toolBar && toolBar->widgetForAction(glyph.pAction);
        const QColor disabledInk = onTheBar ? tokens.disabledBarText : tokens.disabledText;
        if (coloured && glyph.identity.hue >= 0.0) {
            glyph.pAction->setIcon(uiDesign::identityIcon(glyph.glyphOff, glyph.glyphOff, glyph.identity, tokens, disabledInk));
            continue;
        }
        const QPixmap source = uiDesign::glyphPixmap(glyph.glyphOff);
        QIcon icon(uiDesign::tintedGlyph(source, quietColor));
        // Active, not Selected: a tool button asks for Active under the pointer and never for Selected
        icon.addPixmap(uiDesign::tintedGlyph(source, accentText), QIcon::Active);
        // Left to Qt, a grey glyph's generated Disabled picture is all but the Normal one
        icon.addPixmap(uiDesign::tintedGlyph(source, disabledInk), QIcon::Disabled);
        glyph.pAction->setIcon(icon);
    }

    uiDesign::ToolBarOverflow::restyle(toolBar, uiDesign::themeTokens());

    restyleEditorSidebarIcons(quietColor, accentText);

    restyleEditorTreeHeadingIcons();

    if (mpVariableTreeDelegate) {
        mpVariableTreeDelegate->restyle();
    }

    if (mpTriggersMainArea) {
        QToolButton* pClearSoundFile = mpTriggersMainArea->toolButton_clearSoundFile;
        pClearSoundFile->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));
        pClearSoundFile->setIconSize(QSize(scmEditorPatternDeleteGlyphSize, scmEditorPatternDeleteGlyphSize));

        mpTriggersMainArea->toolButton_toggleExtraControls->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-options.svg"), tokens));
    }

    if (mpChipRow_scriptEvents) {
        mpChipRow_scriptEvents->restyleGlyphs(tokens);
        mpChipRow_scriptEvents->restyleSuggestions(tokens);
    }

    if (mpButton_keyClear) {
        mpButton_keyClear->setIcon(uiDesign::tintedIcon(qsl(":/icons/editor-clear.svg"), tokens));
    }

    if (mpLabel_editorCodeHeaderIcon) {
        const qreal glyphRatio = mpLabel_editorCodeHeaderIcon->devicePixelRatioF();
        QPixmap headerGlyph = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-scripts.svg")), quietColor)
                                      .scaled(QSize(scmEditorCodeHeaderGlyphSize, scmEditorCodeHeaderGlyphSize) * glyphRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        headerGlyph.setDevicePixelRatio(glyphRatio);
        mpLabel_editorCodeHeaderIcon->setPixmap(headerGlyph);
    }

    // The trees' own mark for a broken item, so the row, the heading and the line in the margin read as one
    if (mpLabel_editorCompileMark) {
        const qreal glyphRatio = mpLabel_editorCompileMark->devicePixelRatioF();
        QPixmap mark = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-errors.svg")), uiDesign::errorInk(tokens))
                               .scaled(QSize(scmEditorCodeHeaderGlyphSize, scmEditorCodeHeaderGlyphSize) * glyphRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        mark.setDevicePixelRatio(glyphRatio);
        mpLabel_editorCompileMark->setPixmap(mark);
    }

    createSearchOptionIcon();
    updateSearchHistoryAction();

    restyleAddPatternIcon();

    restylePatternTypeIcons();

    mPatternDeleteIcon = patternDeleteIcon();
    mPatternHoverTint = patternHoverTint();
    for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
        patternEdit->setDeleteGlyph(mPatternDeleteIcon);
        patternEdit->setHoverTint(mPatternHoverTint);
    }

    // The notice styles its own pictures from the tokens; only the dismiss cross is this window's
    if (QToolButton* pClose = mpSystemMessageArea ? mpSystemMessageArea->messageAreaCloseButton : nullptr) {
        const qreal glyphRatio = pClose->devicePixelRatioF();
        const auto crossInked = [glyphRatio](const QColor& colour) {
            QPixmap glyph = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-clear.svg")), colour)
                                    .scaled(QSize(scmEditorBannerCloseSize, scmEditorBannerCloseSize) * glyphRatio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            glyph.setDevicePixelRatio(glyphRatio);
            return glyph;
        };
        QIcon cross(crossInked(tokens.mutedText));
        cross.addPixmap(crossInked(tokens.text), QIcon::Active);
        pClose->setIcon(cross);
        pClose->setIconSize(QSize(scmEditorBannerCloseSize, scmEditorBannerCloseSize));
    }
}

void dlgTriggerEditor::setupEditorPanel()
{
    const QList<QPair<TTreeWidget*, TreeType>> itemTrees{{treeWidget_triggers, TreeType::Trigger},
                                                         {treeWidget_aliases, TreeType::Alias},
                                                         {treeWidget_timers, TreeType::Timer},
                                                         {treeWidget_scripts, TreeType::Script},
                                                         {treeWidget_actions, TreeType::Action},
                                                         {treeWidget_keys, TreeType::Key},
                                                         {mpFolderTree, TreeType::None}};
    for (const auto& itemTree : itemTrees) {
        auto* pDelegate = new uiDesign::EditorTreeDelegate(itemTree.first, itemTree.second, mpHost);
        pDelegate->setNewDescriptions(descNewItem, descNewFolder);
        connect(pDelegate, &uiDesign::EditorTreeDelegate::toggleRequested, this, &dlgTriggerEditor::slot_toggleItemOrGroupActiveFlag);
        itemTree.first->setItemDelegate(pDelegate);
        mEditorTreeDelegates.append(pDelegate);
        // Cut rather than scrolled: a panel this narrow would spend its bottom edge on a scrollbar
        itemTree.first->setTextElideMode(Qt::ElideRight);
        itemTree.first->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    mpVariableTreeDelegate = new uiDesign::VariableTreeDelegate(treeWidget_variables);
    connect(mpVariableTreeDelegate, &uiDesign::VariableTreeDelegate::keptToggleRequested, this, &dlgTriggerEditor::slot_toggleVariableKept);
    treeWidget_variables->setItemDelegate(mpVariableTreeDelegate);
    treeWidget_variables->setTextElideMode(Qt::ElideRight);
    treeWidget_variables->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // With allColumnsShowFocus, QTreeView::drawRow() asks the style for the focus rect with no widget, so the trees'
    // "outline: none" cannot reach it and a square hairline is drawn round the rounded pill; per item, the delegate
    // asks with the widget in hand. The .ui sets it on all but the results tree, listed so all nine are settled here.
    const QList<QTreeWidget*> focusMarkedTrees{
            treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables, mpFolderTree, treeWidget_searchResults};
    for (QTreeWidget* pTreeWidget : focusMarkedTrees) {
        pTreeWidget->setAllColumnsShowFocus(false);
    }

    treeWidget_searchResults->setObjectName(qsl("editorSearchResults"));
    //: Accessible name of the editor's list of search results, which screen readers announce when it gets the focus
    treeWidget_searchResults->setAccessibleName(tr("Search results"));
    treeWidget_searchResults->setColumnCount(1);
    treeWidget_searchResults->setHeaderHidden(true);
    treeWidget_searchResults->setRootIsDecorated(true);
    treeWidget_searchResults->setIndentation(scmEditorSearchResultIndent);
    treeWidget_searchResults->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // The .ui file asks for uniform heights, but a heading is taller than the matches under it
    treeWidget_searchResults->setUniformRowHeights(false);
    // The .ui file asks for sorting, which would undo the order matches are found and read in
    treeWidget_searchResults->setSortingEnabled(false);
    mpSearchResultDelegate = new uiDesign::SearchResultDelegate(treeWidget_searchResults);
    treeWidget_searchResults->setItemDelegate(mpSearchResultDelegate);

    // The shell stylesheet selects on this name
    widget_searchTerm->setObjectName(qsl("editorSearchRow"));

    // The shell stylesheet paints the whole item panel once, under this name
    frame_left->setObjectName(qsl("editorItemPane"));
}

// The strip takes no mouse, so the heading is also the splitter handle the two panes are resized by
void dlgTriggerEditor::setupEditorCodeHeader()
{
    mpWidget_editorCodeHeader = new QWidget(this);
    mpWidget_editorCodeHeader->setObjectName(qsl("editorCodeHeader"));
    auto* pHeaderLayout = new QHBoxLayout(mpWidget_editorCodeHeader);
    pHeaderLayout->setContentsMargins(scmEditorCodeHeaderInset, 0, scmEditorCodeHeaderInset, 0);
    pHeaderLayout->setSpacing(6);

    mpLabel_editorCodeHeaderIcon = new QLabel(mpWidget_editorCodeHeader);
    mpLabel_editorCodeHeaderIcon->setObjectName(qsl("editorCodeHeaderIcon"));
    mpLabel_editorCodeHeaderIcon->setFixedSize(scmEditorCodeHeaderGlyphSize, scmEditorCodeHeaderGlyphSize);
    pHeaderLayout->addWidget(mpLabel_editorCodeHeaderIcon);

    mpLabel_editorCodeHeaderTitle = new QLabel(codeHeaderTitleFor(mCurrentView), mpWidget_editorCodeHeader);
    mpLabel_editorCodeHeaderTitle->setObjectName(qsl("editorCodeHeaderTitle"));
    pHeaderLayout->addWidget(mpLabel_editorCodeHeaderTitle);

    mpWidget_editorCompileNote = new QWidget(mpWidget_editorCodeHeader);
    mpWidget_editorCompileNote->setObjectName(qsl("editorCompileNote"));
    // The strip takes no mouse, so Tab is how the keyboard reaches the note; its keys come through eventFilter()
    mpWidget_editorCompileNote->setFocusPolicy(Qt::TabFocus);
    mpWidget_editorCompileNote->installEventFilter(this);
    auto* pNoteLayout = new QHBoxLayout(mpWidget_editorCompileNote);
    pNoteLayout->setContentsMargins(0, 0, 0, 0);
    pNoteLayout->setSpacing(5);
    mpLabel_editorCompileMark = new QLabel(mpWidget_editorCompileNote);
    mpLabel_editorCompileMark->setObjectName(qsl("editorCompileMark"));
    mpLabel_editorCompileMark->setFixedSize(scmEditorCodeHeaderGlyphSize, scmEditorCodeHeaderGlyphSize);
    pNoteLayout->addWidget(mpLabel_editorCompileMark);
    mpLabel_editorCompileMessage = new QLabel(mpWidget_editorCompileNote);
    mpLabel_editorCompileMessage->setObjectName(qsl("editorCompileMessage"));
    // Plain, or a message Qt takes for rich text would lose the line break elideCompileNote() puts in
    mpLabel_editorCompileMessage->setTextFormat(Qt::PlainText);
    // Ignored: a compiler's sentence is as long as it likes, and elideCompileNote() fits it to the room left
    mpLabel_editorCompileMessage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    mpLabel_editorCompileMessage->installEventFilter(this);
    pNoteLayout->addWidget(mpLabel_editorCompileMessage, 1);
    pHeaderLayout->addWidget(mpWidget_editorCompileNote, 1);
    // Holds the middle while there is no note, keeping the caret reading at the trailing edge - see
    // updateEditorCodeHeading()
    pHeaderLayout->addStretch(0);

    mpLabel_editorCaretPosition = new QLabel(mpWidget_editorCodeHeader);
    mpLabel_editorCaretPosition->setObjectName(qsl("editorCodeCaret"));
    mpLabel_editorCaretPosition->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    pHeaderLayout->addWidget(mpLabel_editorCaretPosition);

    // Index 1: the handle over mpSourceEditorArea, the second of the three panes
    splitter_right->setHeaderHandle(1, mpWidget_editorCodeHeader);
    // The strip takes no mouse, so the handle carrying it hears the click on the note
    if (auto* pHandle = qobject_cast<uiDesign::GripSplitterHandle*>(mpWidget_editorCodeHeader->parentWidget())) {
        pHandle->setClickable(mpWidget_editorCompileNote);
        connect(pHandle, &uiDesign::GripSplitterHandle::clicked, this, &dlgTriggerEditor::slot_codeHeadingClicked);
    }
    updateEditorCodeHeading();
}

QString dlgTriggerEditor::codeHeaderTitleFor(const EditorViewType view) const
{
    if (view == EditorViewType::cmVarsView) {
        //: Heading over the editor's code pane in the variables view, where the pane holds the chosen variable's value rather than a script
        return tr("Value");
    }
    //: Heading over the editor's code pane, naming the language what is typed there is written in
    return tr("Lua script");
}

void dlgTriggerEditor::updateEditorCodeHeading()
{
    if (!mpWidget_editorCompileNote || !mpLabel_editorCompileMark || !mpLabel_editorCompileMessage) {
        return;
    }

    const bool compiled = mEditorCompileMessage.isEmpty();
    QString plainMessage;
    mEditorCompileNoteText.clear();
    mEditorCompileErrorLine = 0;
    if (!compiled) {
        // The compiler's message is rich text; the strip shows one plain line and the tooltip the whole
        plainMessage = QTextDocumentFragment::fromHtml(mEditorCompileMessage).toPlainText().simplified();
        const QRegularExpressionMatch match = csmCompileErrorLineRegex.match(plainMessage);
        if (match.hasMatch()) {
            mEditorCompileErrorLine = match.captured(1).toInt();
            //: Note on the heading over the editor's code pane after a save that did not compile. %1 is the line of the script the compiler stopped on, %2 what it said about that line.
            mEditorCompileNoteText = tr("Line %1: %2").arg(match.captured(1), match.captured(2));
        } else {
            mEditorCompileNoteText = plainMessage;
        }
    }

    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QColor noteInk = uiDesign::errorInk(tokens);
    // A transparent border at rest, so focus paints it instead of shifting the words by a pixel
    const QString noteRule = qsl("#editorCompileNote { background: transparent; border: 1px solid transparent; border-radius: %1px; }"
                                 "#editorCompileNote:focus { border-color: %2; }"
                                 "#editorCompileMessage { color: %3; }")
                                     .arg(QString::number(uiDesign::scmRadiusChip), tokens.accent.name(), noteInk.name());
    // setStyleSheet() re-polishes the subtree even for identical rules, and every item switch comes here
    if (mpWidget_editorCompileNote->styleSheet() != noteRule) {
        mpWidget_editorCompileNote->setStyleSheet(noteRule);
    }

    mpWidget_editorCompileNote->setVisible(!compiled);
    if (auto* pHeadingLayout = qobject_cast<QBoxLayout*>(mpWidget_editorCodeHeader ? mpWidget_editorCodeHeader->layout() : nullptr)) {
        const int noteIndex = pHeadingLayout->indexOf(mpWidget_editorCompileNote);
        pHeadingLayout->setStretch(noteIndex, compiled ? 0 : 1);
        pHeadingLayout->setStretch(noteIndex + 1, compiled ? 1 : 0);
        // Settle the new stretch now, so elideCompileNote() reads the label's new width
        pHeadingLayout->activate();
    }
    elideCompileNote();

    // The strip is transparent to the mouse so a drag anywhere on it resizes; the tooltip goes on the handle instead
    if (QWidget* pHandle = mpWidget_editorCodeHeader ? mpWidget_editorCodeHeader->parentWidget() : nullptr) {
        pHandle->setToolTip(plainMessage);
    }
    if (mpEditorLineMargin) {
        mpEditorLineMargin->setErrorLine(mEditorCompileErrorLine);
    }
}

// A second line before any cut: what the compiler said is what the reader came to the note for
void dlgTriggerEditor::elideCompileNote()
{
    if (!mpLabel_editorCompileMessage) {
        return;
    }
    const int room = mpLabel_editorCompileMessage->width();
    const QFontMetrics metrics = mpLabel_editorCompileMessage->fontMetrics();
    QString shown = mEditorCompileNoteText;
    if (room > 0 && metrics.horizontalAdvance(shown) > room) {
        QTextLayout layout(shown, mpLabel_editorCompileMessage->font());
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout();
        QTextLine first = layout.createLine();
        first.setLineWidth(room);
        layout.endLayout();
        const int split = first.textLength();
        shown = shown.left(split).trimmed() + QLatin1Char('\n') + metrics.elidedText(shown.mid(split).trimmed(), Qt::ElideRight, room);
    }
    const bool wasTwoLines = mpLabel_editorCompileMessage->text().contains(QLatin1Char('\n'));
    mpLabel_editorCompileMessage->setText(shown);
    // The heading is a splitter handle, and QSplitter only reads its thickness again on a LayoutRequest
    if (shown.contains(QLatin1Char('\n')) != wasTwoLines && mpWidget_editorCodeHeader && mpWidget_editorCodeHeader->parentWidget()) {
        mpWidget_editorCodeHeader->parentWidget()->updateGeometry();
    }
}

void dlgTriggerEditor::clearCompileState()
{
    mEditorCompileMessage.clear();
    updateEditorCodeHeading();
}

void dlgTriggerEditor::beginSaveErrorCapture()
{
    mEditorSaveErrorCaptureOpen = true;
    mEditorSaveErrorCaptured.clear();
}

void dlgTriggerEditor::endSaveErrorCapture()
{
    mEditorSaveErrorCaptureOpen = false;
    mEditorCompileMessage = mEditorSaveErrorCaptured;
    updateEditorCodeHeading();
}

// The layout's own hint, so separators, grip, spacing and sheet padding count the way Qt counts them for the
// overflow; taken from size hints, not geometry, so it holds whether or not Qt has folded the tail away
static int toolBarLengthWanted(const QToolBar* pToolBar)
{
    const QLayout* pLayout = pToolBar->layout();
    const QSize wanted = pLayout ? pLayout->sizeHint() : pToolBar->sizeHint();
    return pToolBar->orientation() == Qt::Horizontal ? wanted.width() : wanted.height();
}

static void setToolBarButtonLabelsShown(QToolBar* pToolBar, const QList<QAction*>& actions, const bool shown)
{
    for (QAction* pAction : actions) {
        if (auto* pButton = qobject_cast<QToolButton*>(pToolBar->widgetForAction(pAction))) {
            pButton->setToolButtonStyle(shown ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
            holdToolBarButtonWidth(pButton);
        }
    }
    // A button Qt folded away is hidden, and a hidden widget's updateGeometry() does not reach its layout, so the
    // layout is invalidated outright or the next hint read would be stale
    if (QLayout* pLayout = pToolBar->layout()) {
        pLayout->invalidate();
    }
}

// The delta from the shown wording to the widest, so the style's and the sheet's padding count as Qt counts them
static void holdToolBarButtonWidth(QToolButton* pButton)
{
    const QAction* pAction = pButton->defaultAction();
    const QStringList wordings = pAction ? pAction->property(scmProp_editorToolBarWordings).toStringList() : QStringList();
    if (wordings.isEmpty()) {
        return;
    }
    pButton->setMinimumWidth(0);
    if (pButton->toolButtonStyle() == Qt::ToolButtonIconOnly) {
        return;
    }
    const QFontMetrics metrics(pButton->font());
    int widest = 0;
    for (const QString& wording : wordings) {
        widest = std::max(widest, metrics.size(Qt::TextShowMnemonic, wording).width());
    }
    pButton->setMinimumWidth(pButton->sizeHint().width() - metrics.size(Qt::TextShowMnemonic, pButton->text()).width() + widest);
}

void dlgTriggerEditor::applyEditorToolbarButtonStyles()
{
    if (!toolBar) {
        return;
    }
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    for (QAction* pAction : {mpUndoAction, mpRedoAction}) {
        if (!pAction) {
            continue;
        }
        if (auto* pButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(pAction))) {
            pButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
    }
    // Setting the bar's style resets every button's, so a group that gave its names up is told again
    for (const EditorToolBarGroup& group : mEditorToolBarGroups) {
        setToolBarButtonLabelsShown(toolBar, group.actions, group.labelsShown);
    }
}

// Instead of Qt posting the tail into its overflow, the groups give their names up in mEditorToolBarGroups order
// and get them back in reverse, only with scmEditorToolBarRestoreMargin to spare, so a drag across the breakpoint
// settles instead of flickering
void dlgTriggerEditor::fitEditorToolBarToItsLength()
{
    if (!toolBar || mEditorToolBarGroups.isEmpty() || mEditorToolBarFitting) {
        return;
    }
    // Along the bar: docked at a side it runs downwards
    const bool horizontal = toolBar->orientation() == Qt::Horizontal;
    const int available = horizontal ? toolBar->width() : toolBar->height();
    // A bar not laid out yet has nothing to measure against
    if (available <= 1) {
        return;
    }

    mEditorToolBarFitting = true;
    // Every way a button's cost changes (font, style, icon size, sheet) ends in this fit
    for (QAction* pAction : toolBar->actions()) {
        if (auto* pButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(pAction))) {
            holdToolBarButtonWidth(pButton);
        }
    }
    bool namesGivenUp = false;
    for (EditorToolBarGroup& group : mEditorToolBarGroups) {
        if (toolBarLengthWanted(toolBar) <= available) {
            break;
        }
        if (!group.labelsShown) {
            continue;
        }
        group.labelsShown = false;
        setToolBarButtonLabelsShown(toolBar, group.actions, false);
        namesGivenUp = true;
    }

    // Not in a pass that gave names up: asking at once would flip the groups it just collapsed back and forth
    if (!namesGivenUp) {
        for (auto it = mEditorToolBarGroups.rbegin(), end = mEditorToolBarGroups.rend(); it != end; ++it) {
            if (it->labelsShown) {
                continue;
            }
            setToolBarButtonLabelsShown(toolBar, it->actions, true);
            if (toolBarLengthWanted(toolBar) + scmEditorToolBarRestoreMargin > available) {
                setToolBarButtonLabelsShown(toolBar, it->actions, false);
                break;
            }
            it->labelsShown = true;
        }
    }
    mEditorToolBarFitting = false;
}

// A button that gave its name up is read by its tooltip alone, so the tooltip leads with the view's wording
void dlgTriggerEditor::updateEditorItemActionToolTips()
{
    mAddItem->setToolTip(qsl("<p>%1 (%2)</p>").arg(mAddItem->text(), QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText)));
    mAddGroup->setToolTip(qsl("<p>%1 (%2)</p>").arg(mAddGroup->text(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N).toString(QKeySequence::NativeText)));
    mDeleteItem->setToolTip(qsl("<p>%1 (%2)</p>").arg(mDeleteItem->text(), QKeySequence(QKeySequence::Delete).toString(QKeySequence::NativeText)));
    mSaveItem->setToolTip(qsl("<p>%1 (%2)</p>%3")
                                  .arg(mSaveItem->text(),
                                       QKeySequence(QKeySequence::Save).toString(QKeySequence::NativeText),
                                       //: Follows a heading that is the button's own wording - "Save Trigger", "Save Alias" and so on
                                       tr("<p>Saving causes any changes to the item to take effect. It will not save to disk, "
                                          "so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)</p>")));
}

// Says what a click will do to the chosen row, so neither the bar nor the menu offers to switch on what is already on
void dlgTriggerEditor::updateEditorActivateAction()
{
    if (!mpAction_toggleActive) {
        return;
    }
    // Not resolveCurrentView(): this runs while trees are filled, and must not settle the view as a side effect
    const EditorViewType view = mCurrentView != EditorViewType::cmUnknownView ? mCurrentView : determineViewFromVisibleTree();
    QTreeWidget* pTree = view == EditorViewType::cmVarsView ? nullptr : treeForView(view);
    const auto* pDelegate = pTree ? qobject_cast<uiDesign::EditorTreeDelegate*>(pTree->itemDelegate()) : nullptr;
    const uiDesign::EditorTreeDelegate::ItemState state = pDelegate ? pDelegate->stateOf(pTree->currentIndex()) : uiDesign::EditorTreeDelegate::ItemState();
    // On but inside a group that is off still counts as on: the click switches the row's own state
    const bool switchedOn = state.known && state.dot != uiDesign::EditorTreeDelegate::DotState::Off;

    //: Script editor toolbar button and tree right-click menu: switches the chosen trigger, alias, script, timer, key or button on
    const QString activate = tr("Activate");
    //: Script editor toolbar button and tree right-click menu: switches the chosen trigger, alias, script, timer, key or button off
    const QString deactivate = tr("Deactivate");
    mpAction_toggleActive->setProperty(scmProp_editorToolBarWordings, QStringList{activate, deactivate});
    mpAction_toggleActive->setText(switchedOn ? deactivate : activate);
    const QString tip = switchedOn
                                //: Status tip and tooltip of the script editor's Deactivate button
                                ? tr("Switch the chosen item off. Nothing inside a group runs while the group is off.")
                                //: Status tip and tooltip of the script editor's Activate button
                                : tr("Switch the chosen item on.");
    mpAction_toggleActive->setStatusTip(tip);
    mpAction_toggleActive->setToolTip(qsl("<p>%1</p><p>%2</p>").arg(mpAction_toggleActive->text(), tip));
    mpAction_toggleActive->setEnabled(state.known && mFolderGroupKey.isEmpty());

    if (toolBar) {
        if (auto* pButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(mpAction_toggleActive))) {
            holdToolBarButtonWidth(pButton);
        }
    }
}

QString dlgTriggerEditor::patternRowStyleSheet() const
{
    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    // The add button's disabled ink; restyleAddPatternIcon() hands its frame the same colour
    const QColor quietestText = uiDesign::blend(tokens.card, tokens.text, 0.55);

    return qsl("#label_dragHandle { background: transparent; }"
               "#label_patternNumber { color: %2; background: transparent; }"
               // :disabled in the design's tone: the platform's is a shade off the page on a light theme
               "#label_prompt { color: %2; background: transparent; }"
               "#label_prompt:disabled { color: %8; }"
               "#toolButton_deletePattern { border: none; border-radius: 4px; background: transparent; }"
               "#toolButton_deletePattern:hover { background-color: %5; }"
               // No border: PlaceholderButton paints its dashed frame, inset by the same two margin constants
               "#editorAddPattern { color: %2; border: none; padding: 4px 10px;"
               " margin: %6px 0px %7px 0px; background: transparent; }"
               "#editorAddPattern:hover { color: %3; }"
               "#editorAddPattern:disabled { color: %1; }"
               "#editorPatternDropIndicator { background-color: %4; border: none; border-radius: 1px; }"
               // Named outright so a profile stylesheet cannot put the field colour back under the rows; not
               // "> QWidget", which would take the scroll bars too
               "#editorPatternScroll, #editorPatternScroll > #qt_scrollarea_viewport, #editorPatternList"
               " { background: transparent; border: none; }")
                   .arg(quietestText.name(),
                        tokens.mutedText.name(),
                        tokens.text.name(),
                        tokens.accent.name(),
                        uiDesign::rgba(tokens.text, 0.14),
                        QString::number(scmEditorAddPatternMarginTop),
                        QString::number(scmEditorAddPatternMarginBottom),
                        tokens.disabledText.name())
           // Styling the scroll area at all drops its native scroll bar, so it gets the trees' one
           + uiDesign::scrollBarStyleSheet(qsl("#editorPatternScroll"), tokens);
}

// For widgets a "color:" rule cannot reach: a view's rows (::item reaches the palette for some selectors only) and
// self-painting controls. Readers of the palette, accessibility tools among them, then see what is painted.
static void inkAsChrome(QWidget* pWidget, const QColor& chrome, const QColor& chosen)
{
    QPalette palette = pWidget->palette();
    palette.setColor(QPalette::Text, chrome);
    palette.setColor(QPalette::WindowText, chrome);
    palette.setColor(QPalette::ButtonText, chrome);
    palette.setColor(QPalette::HighlightedText, chosen);
    pWidget->setPalette(palette);
}

// Painted by the row, not a QSS property rule: re-polishing re-runs the whole sheet over the row's controls
QColor dlgTriggerEditor::patternHoverTint() const
{
    QColor tint = uiDesign::themeTokens().text;
    tint.setAlphaF(scmEditorPatternHoverStrength);
    return tint;
}

void dlgTriggerEditor::applyEditorShellStyle()
{
    if (!toolBar) {
        return;
    }

    const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
    const QColor fieldColor = tokens.field;
    const QColor textColor = tokens.text;
    const QColor accentColor = tokens.accent;

    // Everything below is mixed from these four, so an unchanged set has nothing to redo. One setProfileStyleSheet()
    // from Lua sends a StyleChange and a PaletteChange, which would otherwise restyle the editor twice.
    const EditorShellStyleInputs styleInputs{tokens.page.rgb(), fieldColor.rgb(), textColor.rgb(), accentColor.rgb()};
    if (mEditorShellStyleApplied && styleInputs == mEditorShellStyleInputs) {
        return;
    }
    mEditorShellStyleInputs = styleInputs;
    mEditorShellStyleApplied = true;

    const QColor pageColor = tokens.page;
    const QColor paneColor = tokens.pane;
    const QColor separatorColor = tokens.separator;
    const QColor borderColor = tokens.border;
    const QColor mutedText = tokens.mutedText;
    const QColor disabledText = tokens.disabledText;
    const QString hoverSoft = tokens.hoverSoft;
    const QString accentSoft = tokens.accentSoft;
    const QColor accentText = tokens.accentText;
    // Set per form: a QLineEdit rule on the window would also reach the find bar and the trees' editors
    const QString inputRules = uiDesign::inputStyleSheet(tokens);
    // Per form for the same reason: a QPushButton rule on the window would reach the trees' toolbars
    const QString choiceRules = uiDesign::choiceStyleSheet(tokens);
    const QString buttonRules = uiDesign::buttonStyleSheet(tokens);
    // Class by class: a stylesheet colour does not pass from a widget to its children the way CSS's does
    const QString chromeInkRules = qsl("QLabel, QAbstractButton, QGroupBox { color: %1; }").arg(mutedText.name());
    // The window has one font, so the last pill's height answers for all six.
    int idChipHeight = 0;
    const QList<std::tuple<QFrame*, QLabel*, QLabel*>> idChips{{mpTriggersMainArea->frameId, mpTriggersMainArea->label_idLabel, mpTriggersMainArea->label_idNumber},
                                                               {mpAliasMainArea->frameId, mpAliasMainArea->label_idLabel, mpAliasMainArea->label_idNumber},
                                                               {mpTimersMainArea->frameId, mpTimersMainArea->label_idLabel, mpTimersMainArea->label_idNumber},
                                                               {mpKeysMainArea->frameId, mpKeysMainArea->label_idLabel, mpKeysMainArea->label_idNumber},
                                                               {mpScriptsMainArea->frameId, mpScriptsMainArea->label_idLabel, mpScriptsMainArea->label_idNumber},
                                                               {mpActionsMainArea->frameId, mpActionsMainArea->label_idLabel, mpActionsMainArea->label_idNumber}};
    for (const auto& [pFrameId, pIdLabel, pIdNumber] : idChips) {
        idChipHeight = styleEditorIdChip(pFrameId, pIdLabel, pIdNumber);
    }

    const QString formRules = inputRules + choiceRules + buttonRules + chromeInkRules
                              + qsl("QLabel:disabled, QCheckBox:disabled, QRadioButton:disabled, QGroupBox:disabled, QPushButton:disabled, QToolButton:disabled { color: %1; }"
                                    "QLabel[editorRowLabel=\"true\"] { color: %2; background: transparent; }"
                                    "#frameId { border: 1px solid %3; border-radius: %4px; background: transparent; }"
                                    "#frameId QLabel { color: %2; background: transparent; }")
                                        .arg(disabledText.name(), mutedText.name(), borderColor.name(), QString::number(idChipHeight / 2));

    restyleEditorIcons();

    const QString toolBarSelector = qsl("QToolBar#editorActionsToolbar");
    const QString toolBarRules = uiDesign::toolBarStyleSheet(toolBarSelector, uiDesign::ToolBarSeam::Bottom, tokens);

    // The split button recipe, rounded to the bar's corner since the face under it is the flat one toolBarStyleSheet() drew
    const QString saveProfileSelector = toolBarSelector + qsl(" QToolButton#editorSaveProfileButton");
    const QString splitButtonRules = saveProfileSelector + qsl(" { padding-right: %1px; }").arg(QString::number(uiDesign::scmToolBarButtonPaddingHorizontal + uiDesign::scmInputDropDownWidth))
                                     + uiDesign::splitButtonMenuHalfStyleSheet(saveProfileSelector, uiDesign::scmToolBarButtonRadius, tokens);

    const QString statusBarRules = qsl("QStatusBar#editorStatusBar { background-color: %1; border-top: 1px solid %2; }"
                                       // Or the platform style draws a sunken frame around
                                       // every widget the bar holds
                                       "QStatusBar#editorStatusBar::item { border: none; }"
                                       "QStatusBar#editorStatusBar QLabel { color: %3; padding: 0px 6px; }")
                                           .arg(pageColor.name(), borderColor.name(), mutedText.name());

    const QString shellStyleSheet = toolBarRules + splitButtonRules + statusBarRules;
    // Save Profile's menu hangs off its button, so the bar's sheet is what reaches it
    toolBar->setStyleSheet(shellStyleSheet + uiDesign::menuStyleSheet(tokens));
    uiDesign::letPopupsTakeTheFieldsCorner(toolBar);
    if (QStatusBar* pStatusBar = QMainWindow::statusBar()) {
        pStatusBar->setStyleSheet(shellStyleSheet);
    }

    styleSearchOptionsMenu();

    // Painted, not left to QPalette::Window: themeTokens() steps the page down from Window where Window equals Base
    // (macOS light), and the transparent edit column would be a shade lighter than the panel. Named on the shell: a
    // bare QWidget rule would paint every widget, and the window's sheet is the profile's.
    if (QWidget* pShell = QMainWindow::centralWidget()) {
        pShell->setStyleSheet(qsl("#editorShell { background-color: %1; }").arg(pageColor.name()));
    }

    // The count is coloured by a rule, not the palette: the pane's own sheet is what it shows through
    frame_left->setStyleSheet(qsl("#editorItemPane { background-color: %1; }"
                                  "#editorHiddenVariablesCount { color: %2; background: transparent; }")
                                      .arg(paneColor.name(), mutedText.name())
                              + chromeInkRules);
    if (mpLabel_hiddenVariablesCount) {
        // The row's font, not the label's: a rule naming the label stops it inheriting from what it sits on
        mpLabel_hiddenVariablesCount->setFont(uiDesign::chipFont(frame_left));
    }

    // A handle with no heading carries each neighbour's tone up to its hairline, and a sheet's fill is invisible to it
    frame_left->setProperty(uiDesign::scmProp_paneTone, paneColor);
    for (QWidget* pPane : {static_cast<QWidget*>(frame_right), static_cast<QWidget*>(mpNonCodeWidgets), static_cast<QWidget*>(mpSourceEditorArea), static_cast<QWidget*>(mpErrorConsole)}) {
        if (pPane) {
            pPane->setProperty(uiDesign::scmProp_paneTone, pageColor);
        }
    }

    // No ink for an unselected row: EditorTreeDelegate draws it by whether the item runs. Shared with the package
    // manager through itemRowStyleSheet(), so the two lists cannot disagree about a chosen row.
    const QString treeRules = uiDesign::itemRowStyleSheet(qsl("QTreeWidget"), tokens, paneColor, scmEditorTreeRowGutter) + uiDesign::scrollBarStyleSheet(qsl("QTreeWidget"), tokens, paneColor);

    const QList<QTreeWidget*> panelTrees{treeWidget_triggers, treeWidget_aliases, treeWidget_timers, treeWidget_scripts, treeWidget_actions, treeWidget_keys, treeWidget_variables, mpFolderTree};
    for (QTreeWidget* pTreeWidget : panelTrees) {
        pTreeWidget->setStyleSheet(treeRules);
        inkAsChrome(pTreeWidget, mutedText, accentText);
    }

    for (uiDesign::EditorTreeDelegate* pDelegate : std::as_const(mEditorTreeDelegates)) {
        pDelegate->setColouredMarks(mudlet::self() && mudlet::self()->colouredIcons());
        pDelegate->restyle();
    }

    // The branch column is left to the style here: its arrow is what says a heading can be folded away
    treeWidget_searchResults->setStyleSheet(treeRules + qsl("QTreeWidget#editorSearchResults::item { padding: 0px 2px; }"));
    inkAsChrome(treeWidget_searchResults, mutedText, accentText);
    if (mpSearchResultDelegate) {
        mpSearchResultDelegate->restyle();
    }

    widget_searchTerm->setStyleSheet(qsl("#editorSearchRow { background: transparent; }"
                                         "#editorSearchRow QComboBox { background-color: %1; border: 1px solid %2; border-radius: %5px; min-height: 32px; padding: 0px 6px; color: %3; }"
                                         "#editorSearchRow QComboBox:focus { border: 1px solid %4; }"
                                         // No drop-down: it is a field to type into, and the history still opens
                                         // with the down arrow and the completer
                                         "#editorSearchRow QComboBox::drop-down { width: 0px; border: none; background: transparent; }"
                                         "#editorSearchRow QComboBox::down-arrow { width: 0px; height: 0px; image: none; }"
                                         // Or the line edit draws a second frame inside the combo's
                                         "#editorSearchRow QComboBox QLineEdit { background: transparent; border: none; }")
                                             .arg(fieldColor.name(), borderColor.name(), textColor.name(), accentColor.name(), QString::number(uiDesign::scmRadiusProminentInput)));

    if (mpWidget_editorCodeHeader) {
        // Transparent: the handle carrying the strip paints the bar
        mpWidget_editorCodeHeader->setStyleSheet(qsl("#editorCodeHeader { background: transparent; }"
                                                     "#editorCodeHeaderTitle { color: %1; font-size: %2pt; }"
                                                     "#editorCodeCaret { color: %1; }")
                                                         .arg(mutedText.name(), QString::number(uiDesign::typeSize(uiDesign::TypeStep::Caption))));
        updateEditorCodeHeading();
    }

    if (mpTriggersMainArea) {
        const QColor hoveredBorder = uiDesign::blend(borderColor, textColor, scmEditorHoveredBorderWeight);

        // Shared rules first: a segment rule and a shared rule tie on specificity once both add a state, and the
        // later wins - the shared :checked would give the sizeless indicator a hairline back and move the pair
        mpTriggersMainArea->setStyleSheet(
                formRules + patternRowStyleSheet()
                + qsl(
                          // Named outright so a profile stylesheet cannot paint a band across a group
                          "QWidget[editorPanelSurface=\"true\"] { background: transparent; border: none; }"
                          "QGroupBox[editorOptionRow=\"true\"] { border: none; margin: 0px; padding: 0px; background: transparent; }"
                          "QRadioButton[editorSegment=\"true\"] { color: %2; background-color: %7; border: %9px solid %1;"
                          " padding: %10px %11px; spacing: 0px; }"
                          // No border or fill either: an indicator rule with nothing to draw falls back to the platform style
                          "QRadioButton[editorSegment=\"true\"]::indicator { width: 0px; height: 0px; border: none; background: transparent; image: none; }"
                          "QRadioButton[editorSegment=\"true\"]:hover { border-color: %8; }"
                          "QRadioButton[editorSegment=\"true\"]:checked { color: %4; background-color: %5; border-color: %3; }"
                          "QRadioButton[editorSegment=\"true\"]:focus { border-color: %3; }"
                          "QRadioButton[editorSegment=\"true\"]:disabled { color: %12; }"
                          // Last, to outlive the states above. The middle hairline belongs to the chosen segment so the
                          // accent goes all round it; the other takes its width back as padding, so neither moves.
                          "QRadioButton[editorSegment=\"true\"][editorSegmentSide=\"first\"]"
                          " { border-top-left-radius: %6px; border-bottom-left-radius: %6px; }"
                          "QRadioButton[editorSegment=\"true\"][editorSegmentSide=\"last\"]"
                          " { border-top-right-radius: %6px; border-bottom-right-radius: %6px; }"
                          "QRadioButton[editorSegment=\"true\"][editorSegmentSide=\"first\"]:!checked"
                          " { border-right-width: 0px; padding-right: %13px; }"
                          "QRadioButton[editorSegment=\"true\"][editorSegmentSide=\"last\"]:!checked"
                          " { border-left-width: 0px; padding-left: %13px; }")
                          .arg(borderColor.name(), mutedText.name(), accentColor.name(), accentText.name(), accentSoft, QString::number(uiDesign::scmRadiusInput))
                          .arg(fieldColor.name(), hoveredBorder.name(), QString::number(uiDesign::scmInputBorderWidth), QString::number(scmEditorSegmentPaddingVertical))
                          .arg(QString::number(scmEditorSegmentPaddingHorizontal), disabledText.name(), QString::number(scmEditorSegmentPaddingHorizontal + uiDesign::scmInputBorderWidth))
                + uiDesign::disclosureButtonStyleSheet(qsl("#toolButton_toggleExtraControls"), tokens)
                + qsl( // The button that empties the sound file field, drawn as
                          // the picture alone: a frame round a glyph this small reads as a second control
                          "#toolButton_clearSoundFile { border: none; border-radius: %1px; background: transparent; padding: 2px; }"
                          "#toolButton_clearSoundFile:hover { background-color: %2; }")
                          .arg(QString::number(uiDesign::scmRadiusChip), hoverSoft));
    }

    if (mpScriptsMainArea) {
        mpScriptsMainArea->setStyleSheet(formRules + uiDesign::ChipRow::styleSheetFor(tokens));
    }

    if (mpKeysMainArea) {
        mpKeysMainArea->setStyleSheet(formRules
                                      + qsl("#lineEdit_key_binding[editorListening=\"true\"] { border: %3px solid %1; background-color: %2; }"
                                            "#editorKeyClear { border: none; border-radius: %4px; background: transparent; padding: 2px; }"
                                            "#editorKeyClear:hover { background-color: %5; }")
                                                .arg(accentColor.name(), accentSoft, QString::number(uiDesign::scmInputBorderWidth))
                                                .arg(QString::number(uiDesign::scmRadiusChip), hoverSoft));
    }

    for (QWidget* pMainArea : {static_cast<QWidget*>(mpTimersMainArea), static_cast<QWidget*>(mpAliasMainArea), static_cast<QWidget*>(mpActionsMainArea), static_cast<QWidget*>(mpVarsMainArea)}) {
        if (pMainArea) {
            pMainArea->setStyleSheet(formRules);
        }
    }

    // The segments are radio buttons and covered too; arrow keys still move between them once one is focused
    for (QWidget* pMainArea : {static_cast<QWidget*>(mpTriggersMainArea),
                               static_cast<QWidget*>(mpScriptsMainArea),
                               static_cast<QWidget*>(mpKeysMainArea),
                               static_cast<QWidget*>(mpTimersMainArea),
                               static_cast<QWidget*>(mpAliasMainArea),
                               static_cast<QWidget*>(mpActionsMainArea),
                               static_cast<QWidget*>(mpVarsMainArea)}) {
        uiDesign::keepClickFocusOffControls(pMainArea);
        // Pattern rows are made later, so createPatternItem() does the same for its box
        uiDesign::letPopupsTakeTheFieldsCorner(pMainArea);
    }

    // After the sheets: the Events label is held to the chips' line height, known only once the field rule has landed
    alignEditorFormLeadLabels();
    fitEditorOptionsSpinBoxes();

    // The notice styles itself in dlgSystemMessageArea::slot_applyAppearance(); a second sheet on its frame covers its
    // accent hairline

    if (!mpWidget_editorSidebarPane) {
        return;
    }
    const EditorSidebarWidths widths = editorSidebarWidths();
    mpWidget_editorSidebarPane->setStyleSheet(qsl("#editorSidebarPane { background-color: %1; border-right: 1px solid %2; }").arg(pageColor.name(), separatorColor.name())
                                              + uiDesign::sidebarStyleSheet(qsl("editorSidebar"), qsl("editorSidebarSeparator"), mutedText, editorSidebarMetrics(widths.expanded), tokens)
                                              + uiDesign::scrollBarStyleSheet(qsl("#editorSidebar"), tokens));

    // A different font is a different width for the names, so the breakpoint is measured again
    updateEditorSidebarMode();

    // After that: collapsing re-polishes the list, which puts back the palette the rules were applied to
    if (mpListWidget_editorSidebar) {
        inkAsChrome(mpListWidget_editorSidebar, mutedText, accentText);
    }
    // It paints itself, so its palette is the only place its actual ink can be read from
    if (mpToggle_editorSidebar) {
        inkAsChrome(mpToggle_editorSidebar, mutedText, accentText);
    }

    // The rules move the buttons' padding and the bar's spacing but send no resize, so the fit is taken here
    fitEditorToolBarToItsLength();
}

void dlgTriggerEditor::scheduleEditorItemCountUpdate()
{
    // Not restarted: pushing the wait back on every add is how a long import would never count at all
    if (mpTimer_statusCounts && !mpTimer_statusCounts->isActive()) {
        mpTimer_statusCounts->start();
    }
}

void dlgTriggerEditor::updateEditorItemCounts()
{
    if (!mpLabel_statusCounts) {
        return;
    }

    QTreeWidgetItem* pBaseItem = nullptr;
    bool (*wantedOn)(Host*, const int) = nullptr;
    QString (*countText)(const int, const int) = nullptr;
    switch (mCurrentView) {
    case EditorViewType::cmTriggerView:
        pBaseItem = mpTriggerBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TTrigger* pT = pHost->getTriggerUnit()->getTrigger(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many triggers there are in total, %1 how many of those are turned on.
            return tr("%n trigger(s) - %1 active", "", total).arg(active);
        };
        break;
    case EditorViewType::cmAliasView:
        pBaseItem = mpAliasBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TAlias* pT = pHost->getAliasUnit()->getAlias(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many aliases there are in total, %1 how many of those are turned on.
            return tr("%n alias(es) - %1 active", "", total).arg(active);
        };
        break;
    case EditorViewType::cmTimerView:
        pBaseItem = mpTimerBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TTimer* pT = pHost->getTimerUnit()->getTimer(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many timers there are in total, %1 how many of those are turned on.
            return tr("%n timer(s) - %1 active", "", total).arg(active);
        };
        break;
    case EditorViewType::cmScriptView:
        pBaseItem = mpScriptsBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TScript* pT = pHost->getScriptUnit()->getScript(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many scripts there are in total, %1 how many of those are turned on.
            return tr("%n script(s) - %1 active", "", total).arg(active);
        };
        break;
    case EditorViewType::cmActionView:
        pBaseItem = mpActionBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TAction* pT = pHost->getActionUnit()->getAction(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many buttons, menus and toolbars there are in total, %1 how many of those are turned on.
            return tr("%n button(s) - %1 active", "", total).arg(active);
        };
        break;
    case EditorViewType::cmKeysView:
        pBaseItem = mpKeyBaseItem;
        wantedOn = [](Host* pHost, const int id) {
            TKey* pT = pHost->getKeyUnit()->getKey(id);
            return pT && pT->shouldBeActive();
        };
        countText = [](const int total, const int active) {
            //: Editor status bar. %n is how many keybindings there are in total, %1 how many of those are turned on.
            return tr("%n key(s) - %1 active", "", total).arg(active);
        };
        break;
    default:
        break;
    }
    if (!pBaseItem || mpHost.isNull()) {
        mpLabel_statusCounts->clear();
        return;
    }

    int total = 0;
    int active = 0;
    QList<QTreeWidgetItem*> pending;
    for (int i = 0, last = pBaseItem->childCount(); i < last; ++i) {
        pending.append(pBaseItem->child(i));
    }
    while (!pending.isEmpty()) {
        QTreeWidgetItem* pItem = pending.takeLast();
        for (int i = 0, last = pItem->childCount(); i < last; ++i) {
            pending.append(pItem->child(i));
        }
        ++total;
        // What the user switched on, not what runs: a trigger in a closed filter chain is off through no choice of theirs
        if (wantedOn(mpHost, pItem->data(0, Qt::UserRole).toInt())) {
            ++active;
        }
    }

    mpLabel_statusCounts->setText(countText(total, active));
}

void dlgTriggerEditor::setFont(const QFont& font)
{
    const uiDesign::StyleSheetFontLift lift(this);
    QMainWindow::setFont(font);
}

// Polishing hands every widget its pre-sheet font back, resolved against the application font rather than its
// parent's, as WA_StyleSheet is still set while that happens
void dlgTriggerEditor::setStyleSheet(const QString& styleSheet)
{
    QMainWindow::setStyleSheet(styleSheet);
    uiDesign::resolveFontsUnderStyleSheets(this);
}

// In case the profile was reset while the editor was out of focus, checks for any script loading errors and displays them
void dlgTriggerEditor::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);

    // These move what a sidebar row measures; re-taken here, since the restyle below may skip as unchanged
    if (e->type() == QEvent::LanguageChange || e->type() == QEvent::StyleChange || e->type() == QEvent::FontChange) {
        invalidateEditorSidebarWidths();
        updateEditorSidebarMode();
        // The same three move what a toolbar button costs the bar it is on; Activate is worded per row, not once
        updateEditorActivateAction();
        fitEditorToolBarToItsLength();
        // ...and the widest pattern type name, which a row's type column is held to
        mPatternTypeColumnWidth = 0;
        alignEditorFormLeadLabels();
        fitEditorOptionsSpinBoxes();
    }

    // Result rows are measured off the font, and the restyle below runs only on a colour change
    if (e->type() == QEvent::FontChange && mpSearchResultDelegate) {
        mpSearchResultDelegate->restyle();
    }

    // the appearance can be switched between light and dark while the editor is open
    if ((e->type() == QEvent::StyleChange || e->type() == QEvent::PaletteChange) && mpTriggersMainArea) {
        applyEditorShellStyle();
    }
}

void dlgTriggerEditor::showIDLabels(const bool visible)
{
    mpAliasMainArea->frameId->setVisible(visible);
    mpActionsMainArea->frameId->setVisible(visible);
    mpKeysMainArea->frameId->setVisible(visible);
    mpScriptsMainArea->frameId->setVisible(visible);
    mpTimersMainArea->frameId->setVisible(visible);
    mpTriggersMainArea->frameId->setVisible(visible);
}

void dlgTriggerEditor::checkForMoreThanOneTriggerItem()
{
    int activeItems = 0;
    if (!mpWidget_triggerItems || !mpWidget_triggerItems->layout()) {
        return;
    }
    auto pLayout = mpWidget_triggerItems->layout();
    for (qsizetype i = 0, total = pLayout->count(); i < total; ++i) {
        auto pLayoutItem = pLayout->itemAt(i)->widget();
        if (pLayoutItem) {
            auto* psingleLineTextEdit_pattern = pLayoutItem->findChild<SingleLineTextEdit*>(qsl("singleLineTextEdit_pattern"));
            auto* pComboBox_type = pLayoutItem->findChild<QComboBox*>(qsl("comboBox_patternType"));
            if (pComboBox_type && (pComboBox_type->currentIndex() == REGEX_PROMPT || pComboBox_type->currentIndex() == REGEX_LINE_SPACER)) {
                // These automatically counts as an active item - though if there
                // isn't any GA signals the first won't work...
                ++activeItems;
            } else {
                if (psingleLineTextEdit_pattern && !psingleLineTextEdit_pattern->toPlainText().isEmpty()) {
                    ++activeItems;
                }
            }
        }
    }

    const bool canCombinePatterns = activeItems > 1;
    if (mpWidget_matchModeRows) {
        mpWidget_matchModeRows->setEnabled(canCombinePatterns);
        reflectTriggerMatchMode();
    }
    if (mpLabel_matchModeHint) {
        mpLabel_matchModeHint->setVisible(!canCombinePatterns);
    }
}

void dlgTriggerEditor::setEditorFont(const QFont& newFont)
{
    if (mpErrorConsole) {
        mpErrorConsole->setFont(newFont);
    }

    auto config = mpSourceEditorEdbee->config();
    config->beginChanges();
    config->setFont(newFont);
    config->endChanges();
}

void dlgTriggerEditor::slot_bannerDismissClicked()
{
    handleBannerDismiss();
}

// Helper function to find a tree item by its ID recursively
QTreeWidgetItem* findItemByID(QTreeWidgetItem* parent, int itemID)
{
    if (!parent) {
        return nullptr;
    }

    // Check if this item matches
    if (parent->data(0, Qt::UserRole).toInt() == itemID) {
        return parent;
    }

    // Recursively search children
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* found = findItemByID(parent->child(i), itemID);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

// Helper function to find a nearby item when a deleted item is not found
// Tries to select: 1) sibling above (at position-1), or 2) parent
QTreeWidgetItem* findNearbyItem(QTreeWidgetItem* rootItem, int parentID, int positionInParent)
{
    if (!rootItem) {
        return nullptr;
    }

    // Find the parent item
    QTreeWidgetItem* parentItem = findItemByID(rootItem, parentID);
    if (!parentItem) {
        return nullptr;
    }

    // Try to select sibling above (at position - 1)
    if (positionInParent > 0 && parentItem->childCount() >= positionInParent) {
        // Position is 0-indexed, so position-1 is the item that was above the deleted one
        QTreeWidgetItem* siblingAbove = parentItem->child(positionInParent - 1);
        if (siblingAbove) {
            return siblingAbove;
        }
    }

    // No sibling above, return the parent
    return parentItem;
}

// Helper function to collect IDs of all expanded items in a tree
QSet<int> collectExpandedItemIDs(QTreeWidgetItem* parent)
{
    QSet<int> expandedIDs;
    if (!parent) {
        return expandedIDs;
    }

    // If this item is expanded, record its ID
    if (parent->isExpanded()) {
        int itemID = parent->data(0, Qt::UserRole).toInt();
        if (itemID > 0) { // Valid ID
            expandedIDs.insert(itemID);
        }
    }

    // Recursively collect from children
    for (int i = 0; i < parent->childCount(); ++i) {
        expandedIDs.unite(collectExpandedItemIDs(parent->child(i)));
    }

    return expandedIDs;
}

// Helper function to restore expansion state based on saved IDs
void restoreExpansionState(QTreeWidgetItem* parent, const QSet<int>& expandedIDs)
{
    if (!parent) {
        return;
    }

    // Check if this item should be expanded
    int itemID = parent->data(0, Qt::UserRole).toInt();
    if (itemID > 0 && expandedIDs.contains(itemID)) {
        parent->setExpanded(true);
    }

    // Recursively restore for children
    for (int i = 0; i < parent->childCount(); ++i) {
        restoreExpansionState(parent->child(i), expandedIDs);
    }
}

void dlgTriggerEditor::slot_itemsChanged(EditorViewType viewType, QList<int> affectedItemIDs)
{
    if (mCurrentView != viewType) {
        switch (viewType) {
        case EditorViewType::cmTriggerView:
            slot_showTriggers();
            break;
        case EditorViewType::cmAliasView:
            slot_showAliases();
            break;
        case EditorViewType::cmTimerView:
            slot_showTimers();
            break;
        case EditorViewType::cmScriptView:
            slot_showScripts();
            break;
        case EditorViewType::cmKeysView:
            slot_showKeys();
            break;
        case EditorViewType::cmActionView:
            slot_showActions();
            break;
        default:
            break;
        }
    }

    switch (viewType) {
    case EditorViewType::cmTriggerView: {
        // Clear the current item pointer to avoid use-after-free
        mpCurrentTriggerItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpTriggerBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_triggerSelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_triggers->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpTriggerBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateTriggers();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        // Must be disabled before scrollToItem() which auto-expands parents
        bool wasAnimated = treeWidget_triggers->isAnimated();
        treeWidget_triggers->setAnimated(false);

        mpTriggerBaseItem->setExpanded(true);
        restoreExpansionState(mpTriggerBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpTriggerBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on the selection model to prevent premature selection change cascades
                // Note: Must block on selectionModel(), not the widget itself, as the signal originates from QItemSelectionModel
                QItemSelectionModel* selModel = treeWidget_triggers->selectionModel();
                selModel->blockSignals(true);
                treeWidget_triggers->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_triggers->scrollToItem(itemToSelect);
                slot_triggerSelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpTriggerBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpTriggerBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Triggers" item
                if (!nearbyItem) {
                    nearbyItem = mpTriggerBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_triggers->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_triggers->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_triggers->scrollToItem(nearbyItem);
                    slot_triggerSelected(nearbyItem);
                }
            }
        } else {
            for (auto* patternEdit : std::as_const(mTriggerPatternEdit)) {
                patternEdit->singleLineTextEdit_pattern->clear();
                if (patternEdit->singleLineTextEdit_pattern->isHidden()) {
                    patternEdit->singleLineTextEdit_pattern->show();
                }
                patternEdit->pushButton_fgColor->hide();
                patternEdit->pushButton_bgColor->hide();
                patternEdit->label_prompt->hide();
                patternEdit->spinBox_lineSpacer->hide();
                patternEdit->comboBox_patternType->setCurrentIndex(1);
                patternEdit->comboBox_patternType->setCurrentIndex(0);
            }

            mpTriggersMainArea->lineEdit_trigger_name->clear();
            mpTriggersMainArea->label_idNumber->clear();
            clearDocument(mpSourceEditorEdbee);
            mpTriggersMainArea->lineEdit_trigger_command->clear();
        }

        treeWidget_triggers->setAnimated(wasAnimated);
        break;
    }
    case EditorViewType::cmTimerView: {
        mpCurrentTimerItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpTimerBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_timerSelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_timers->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpTimerBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateTimers();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        bool wasAnimated = treeWidget_timers->isAnimated();
        treeWidget_timers->setAnimated(false);

        mpTimerBaseItem->setExpanded(true);
        restoreExpansionState(mpTimerBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpTimerBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on selection model to prevent premature selection change cascades
                QItemSelectionModel* selModel = treeWidget_timers->selectionModel();
                selModel->blockSignals(true);
                treeWidget_timers->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_timers->scrollToItem(itemToSelect);
                slot_timerSelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpTimerBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpTimerBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Timers" item
                if (!nearbyItem) {
                    nearbyItem = mpTimerBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_timers->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_timers->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_timers->scrollToItem(nearbyItem);
                    slot_timerSelected(nearbyItem);
                }
            }
        }

        treeWidget_timers->setAnimated(wasAnimated);
        break;
    }
    case EditorViewType::cmAliasView: {
        mpCurrentAliasItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpAliasBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_aliasSelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_aliases->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpAliasBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateAliases();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        bool wasAnimated = treeWidget_aliases->isAnimated();
        treeWidget_aliases->setAnimated(false);

        mpAliasBaseItem->setExpanded(true);
        restoreExpansionState(mpAliasBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpAliasBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on selection model to prevent premature selection change cascades
                QItemSelectionModel* selModel = treeWidget_aliases->selectionModel();
                selModel->blockSignals(true);
                treeWidget_aliases->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_aliases->scrollToItem(itemToSelect);
                slot_aliasSelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpAliasBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpAliasBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Aliases" item
                if (!nearbyItem) {
                    nearbyItem = mpAliasBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_aliases->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_aliases->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_aliases->scrollToItem(nearbyItem);
                    slot_aliasSelected(nearbyItem);
                }
            }
        }

        treeWidget_aliases->setAnimated(wasAnimated);
        break;
    }
    case EditorViewType::cmScriptView: {
        mpCurrentScriptItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpScriptsBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_scriptsSelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_scripts->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpScriptsBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateScripts();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        bool wasAnimated = treeWidget_scripts->isAnimated();
        treeWidget_scripts->setAnimated(false);

        mpScriptsBaseItem->setExpanded(true);
        restoreExpansionState(mpScriptsBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpScriptsBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on selection model to prevent premature selection change cascades
                QItemSelectionModel* selModel = treeWidget_scripts->selectionModel();
                selModel->blockSignals(true);
                treeWidget_scripts->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_scripts->scrollToItem(itemToSelect);
                slot_scriptsSelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpScriptsBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpScriptsBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Scripts" item
                if (!nearbyItem) {
                    nearbyItem = mpScriptsBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_scripts->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_scripts->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_scripts->scrollToItem(nearbyItem);
                    slot_scriptsSelected(nearbyItem);
                }
            }
        }

        treeWidget_scripts->setAnimated(wasAnimated);
        break;
    }
    case EditorViewType::cmActionView: {
        mpCurrentActionItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpActionBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_actionSelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_actions->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpActionBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateActions();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        bool wasAnimated = treeWidget_actions->isAnimated();
        treeWidget_actions->setAnimated(false);

        mpActionBaseItem->setExpanded(true);
        restoreExpansionState(mpActionBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpActionBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on selection model to prevent premature selection change cascades
                QItemSelectionModel* selModel = treeWidget_actions->selectionModel();
                selModel->blockSignals(true);
                treeWidget_actions->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_actions->scrollToItem(itemToSelect);
                slot_actionSelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpActionBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpActionBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Actions" item
                if (!nearbyItem) {
                    nearbyItem = mpActionBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_actions->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_actions->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_actions->scrollToItem(nearbyItem);
                    slot_actionSelected(nearbyItem);
                }
            }
        }

        treeWidget_actions->setAnimated(wasAnimated);
        break;
    }
    case EditorViewType::cmKeysView: {
        mpCurrentKeyItem = nullptr;

        QSet<int> expandedIDs = collectExpandedItemIDs(mpKeyBaseItem);

        // Block signals on the selection model to prevent it from emitting during tree deletion
        // This prevents slot_keySelected from being called with dangling pointers
        QItemSelectionModel* selModel = treeWidget_keys->selectionModel();
        selModel->blockSignals(true);

        QList<QTreeWidgetItem*> children = mpKeyBaseItem->takeChildren();
        qDeleteAll(children);

        selModel->blockSignals(false);

        populateKeys();

        // Temporarily disable animation for instant expansion (looks better for undo/redo)
        bool wasAnimated = treeWidget_keys->isAnimated();
        treeWidget_keys->setAnimated(false);

        mpKeyBaseItem->setExpanded(true);
        restoreExpansionState(mpKeyBaseItem, expandedIDs);

        if (!affectedItemIDs.isEmpty()) {
            QTreeWidgetItem* itemToSelect = findItemByID(mpKeyBaseItem, affectedItemIDs.first());
            if (itemToSelect) {
                // Block signals on selection model to prevent premature selection change cascades
                QItemSelectionModel* selModel = treeWidget_keys->selectionModel();
                selModel->blockSignals(true);
                treeWidget_keys->setCurrentItem(itemToSelect);
                selModel->blockSignals(false);
                treeWidget_keys->scrollToItem(itemToSelect);
                slot_keySelected(itemToSelect);
            } else {
                // Item not found (was deleted) - try to select a nearby item
                QTreeWidgetItem* nearbyItem = nullptr;

                // Query the undo stack for deleted item info
                if (mpUndoStack) {
                    const QUndoCommand* lastCmd = mpUndoStack->getLastExecutedCommand();
                    if (auto* deleteCmd = dynamic_cast<const EditorDeleteItemCommand*>(lastCmd)) {
                        const auto* deletedInfo = deleteCmd->getDeletedItemInfo(affectedItemIDs.first());
                        if (deletedInfo) {
                            nearbyItem = findNearbyItem(mpKeyBaseItem, deletedInfo->parentID, deletedInfo->positionInParent);
                        }
                    } else if (auto* addCmd = dynamic_cast<const EditorAddItemCommand*>(lastCmd)) {
                        // Item was deleted via Add undo
                        nearbyItem = findNearbyItem(mpKeyBaseItem, addCmd->getParentID(), addCmd->getPositionInParent());
                    }
                }

                // If no nearby item found, select the top-level "Keys" item
                if (!nearbyItem) {
                    nearbyItem = mpKeyBaseItem;
                }

                if (nearbyItem) {
                    QItemSelectionModel* selModel = treeWidget_keys->selectionModel();
                    selModel->blockSignals(true);
                    treeWidget_keys->setCurrentItem(nearbyItem);
                    selModel->blockSignals(false);
                    treeWidget_keys->scrollToItem(nearbyItem);
                    slot_keySelected(nearbyItem);
                }
            }
        }

        treeWidget_keys->setAnimated(wasAnimated);
        break;
    }
    default:
        break;
    }

    // The new row was made current with the selection model's signals blocked, so the view never told a screen reader
    if (auto* pTree = qobject_cast<TTreeWidget*>(treeForView(viewType))) {
        pTree->reportCurrentRow();
    }
}

void dlgTriggerEditor::handleBannerDismiss()
{
    // With no banner on display the close button was pressed on the "Banner
    // hidden" undo toast itself - just close it instead of treating it as
    // another banner dismissal (which would suppress the whole view's banners
    // and stash the toast text as restorable banner content)
    if (mCurrentBannerKey.isEmpty()) {
        cancelBannerUndoTimer();
        hideSystemMessageArea();
        return;
    }

    mLastDismissedBannerView = mCurrentView;
    // Uninked, so an undo after an appearance change inks the banner for the appearance it returns into
    mLastDismissedBannerContent = systemMessage();
    mLastDismissedBannerKey = mCurrentBannerKey;

    const QString settingsKey = bannerSettingsKey(mCurrentView, mCurrentBannerKey);
    if (!settingsKey.isEmpty()) {
        mTemporarilyHiddenBanners.insert(settingsKey);
    }

    hideSystemMessageArea();
    mCurrentBannerKey.clear();
    showBannerUndoToast();
}

void dlgTriggerEditor::cancelBannerUndoTimer()
{
    if (mpBannerUndoTimer) {
        mpBannerUndoTimer->stop();
        mpBannerUndoTimer->deleteLater();
        mpBannerUndoTimer = nullptr;
    }
}

void dlgTriggerEditor::showBannerUndoToast()
{
    cancelBannerUndoTimer();

    mCurrentBannerKey.clear();

    mpBannerUndoTimer = new QTimer(this);
    mpBannerUndoTimer->setSingleShot(true);
    mpBannerUndoTimer->setInterval(5s);

    //: Toast notification shown when user dismisses an editor tip banner. Allows them to undo or permanently hide the tips for this editor view type.
    const QString toastMessage = tr("Banner hidden. <a href='undo' style='color: inherit; text-decoration: underline;'>Undo</a> | <a href='hide-permanently' style='color: inherit; text-decoration: "
                                    "underline;'>Hide permanently</a>");

    mpSystemMessageArea->notificationAreaIconLabelError->hide();
    mpSystemMessageArea->notificationAreaIconLabelWarning->hide();
    mpSystemMessageArea->notificationAreaIconLabelInformation->show();
    setSystemMessage(toastMessage);
    mpSystemMessageArea->show();

    connect(mpBannerUndoTimer, &QTimer::timeout, this, &dlgTriggerEditor::hideSystemMessageArea);
    mpBannerUndoTimer->start();

    disconnect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, &dlgTriggerEditor::slot_clickedMessageBox);
    connect(mpSystemMessageArea->notificationAreaMessageBox, &QLabel::linkActivated, this, [this](const QString& link) {
        if (link == "undo") {
            undoBannerDismiss();
        } else if (link == "hide-permanently") {
            handlePermanentBannerDismiss();
        } else {
            slot_clickedMessageBox(link);
        }
    });
}

// QLabel bakes an anchor's colour into its document at setText(), so a message on screen is written again from
// its uninked words. The undo stash is uninked too and is inked when shown again.
void dlgTriggerEditor::slot_refreshBannerLinkColors()
{
    if (!mpSystemMessageArea) {
        return;
    }
    const QString content = systemMessage();
    if (!content.isEmpty()) {
        mpSystemMessageArea->notificationAreaMessageBox->setText(inkedSystemMessage(content));
    }
}

void dlgTriggerEditor::undoBannerDismiss()
{
    cancelBannerUndoTimer();

    const QString settingsKey = bannerSettingsKey(mLastDismissedBannerView, mLastDismissedBannerKey);
    if (!settingsKey.isEmpty()) {
        mTemporarilyHiddenBanners.remove(settingsKey);
    }

    setBannerPermanentlyHidden(mLastDismissedBannerView, mLastDismissedBannerKey, false);

    // Remove the undo toast before restoring the banner so the new content can
    // be shown immediately without being blocked by the active notification.
    if (mpSystemMessageArea) {
        mpSystemMessageArea->hide();
    }
    mCurrentBannerKey.clear();

    if (mLastDismissedBannerView == mCurrentView && !mLastDismissedBannerContent.isEmpty()) {
        showHideableBanner(mLastDismissedBannerContent, mLastDismissedBannerKey);
    }
}


void dlgTriggerEditor::handlePermanentBannerDismiss()
{
    setBannerPermanentlyHidden(mLastDismissedBannerView, mLastDismissedBannerKey, true);
    hideSystemMessageArea();
    mCurrentBannerKey.clear();
}

bool dlgTriggerEditor::bannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey, bool includeBasePreference)
{
    const QString key = bannerSettingsKey(viewType, bannerKey);
    const QString baseKey = bannerSettingsKey(viewType, QString());
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    const QString legacyBaseKey = legacyBannerSettingsKey(viewType, QString());
    if (key.isEmpty()) {
        return false;
    }

    QSettings* settings = MudletApp::getQSettings();
    if (!settings) {
        return false;
    }

    auto migrateLegacyKey = [settings](const QString& newKey, const QString& oldKey) {
        if (newKey.isEmpty() || oldKey.isEmpty() || newKey == oldKey) {
            return;
        }

        const QString oldPath = qsl("Editor/banner_permanently_hidden/%1").arg(oldKey);
        if (!settings->contains(oldPath)) {
            return;
        }

        settings->remove(oldPath);
    };

    migrateLegacyKey(key, legacyKey);
    migrateLegacyKey(baseKey, legacyBaseKey);

    if (includeBasePreference && !bannerKey.isEmpty() && !baseKey.isEmpty()) {
        if (settings->value(qsl("Editor/banner_permanently_hidden/%1").arg(baseKey), false).toBool()) {
            return true;
        }
    }

    return settings->value(qsl("Editor/banner_permanently_hidden/%1").arg(key), false).toBool();
}

void dlgTriggerEditor::setBannerPermanentlyHidden(EditorViewType viewType, const QString& bannerKey, bool hidden)
{
    const QString key = bannerSettingsKey(viewType, bannerKey);
    const QString legacyKey = legacyBannerSettingsKey(viewType, bannerKey);
    if (key.isEmpty()) {
        return;
    }

    QSettings* settings = MudletApp::getQSettings();
    settings->setValue(qsl("Editor/banner_permanently_hidden/%1").arg(key), hidden);

    if (!legacyKey.isEmpty() && legacyKey != key) {
        settings->remove(qsl("Editor/banner_permanently_hidden/%1").arg(legacyKey));
    }

    if (!hidden) {
        mTemporarilyHiddenBanners.remove(key);
    }
}

// Helper function for per-property trigger saves
// Creates an undo command for a single property change with time-based merging support
static void
pushTriggerPropertyCommand(EditorUndoStack* undoStack, Host* host, int triggerID, const QString& triggerName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return; // No change
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmTriggerView, triggerID, triggerName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("trigger:%1:%2").arg(QString::number(triggerID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_TriggerName()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    mpTriggersMainArea->trimName();
    const QString newName = mpTriggersMainArea->lineEdit_trigger_name->text();

    // Skip if no actual change
    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentTriggerItem->setText(0, newName);
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerCommand()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpTriggersMainArea->lineEdit_trigger_command->text();

    if (pT->getCommand() == newCommand) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommand(newCommand);
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("command"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerStayOpen()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const int newValue = mpTriggersMainArea->spinBox_stayOpen->value();

    if (pT->mStayOpen == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->mStayOpen = newValue;
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("stayOpen"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerLineMargin()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const int newValue = mpTriggersMainArea->spinBox_lineMargin->value();
    const bool newIsMultiline = newValue >= 0;

    // Check if anything actually changed
    if (pT->isMultiline() == newIsMultiline && (!newIsMultiline || pT->getConditionLineDelta() == newValue)) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    if (newValue >= 0) {
        pT->setConditionLineDelta(newValue);
        pT->setIsMultiline(true);
    } else {
        pT->setIsMultiline(false);
    }
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("lineMargin"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerFilterTrigger()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const bool newValue = mpTriggersMainArea->checkBox_filterTrigger->isChecked();

    if (pT->mFilterTrigger == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->mFilterTrigger = newValue;
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("filterTrigger"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerPerlSlashG()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const bool newValue = mpTriggersMainArea->checkBox_perlSlashGOption->isChecked();

    if (pT->mPerlSlashGOption == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->mPerlSlashGOption = newValue;
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("perlSlashG"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerSoundEnabled()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const bool newValue = mpTriggersMainArea->checkBox_soundTrigger->isChecked();

    if (pT->mSoundTrigger == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->mSoundTrigger = newValue;
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("soundEnabled"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerSoundFile()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const QString newValue = triggerSoundFilePath();

    if (pT->mSoundFile == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->setSound(newValue);
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("soundFile"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerColorizer()
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    const bool newValue = mpTriggersMainArea->checkBox_triggerColorizer->isChecked();

    if (pT->isColorizerTrigger() == newValue) {
        return;
    }

    QString oldStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);
    pT->setIsColorizerTrigger(newValue);
    QString newStateXML = exportTriggerToXML(pT, SnapshotScope::ItemOnly);

    pushTriggerPropertyCommand(mpUndoStack, mpHost, triggerID, pT->getName(), qsl("colorizer"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TriggerPattern(int patternIndex)
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    // This slot would need to capture all patterns and update them
    // For now, patterns are handled by saveTrigger()
    Q_UNUSED(patternIndex);
}

void dlgTriggerEditor::slot_saveProperty_TriggerPatternType(int patternIndex)
{
    if (mBlockPropertySave || !mpCurrentTriggerItem) {
        return;
    }

    const int triggerID = mpCurrentTriggerItem->data(0, Qt::UserRole).toInt();
    TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(triggerID);
    if (!pT) {
        return;
    }

    // This slot would need to capture all patterns and update them
    // For now, patterns are handled by saveTrigger()
    Q_UNUSED(patternIndex);
}

// =============================================================================
// Alias Per-Property Save Slots
// =============================================================================

// Helper function for per-property alias saves
static void pushAliasPropertyCommand(EditorUndoStack* undoStack, Host* host, int aliasID, const QString& aliasName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return;
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmAliasView, aliasID, aliasName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("alias:%1:%2").arg(QString::number(aliasID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_AliasName()
{
    if (mBlockPropertySave || !mpCurrentAliasItem) {
        return;
    }

    const int aliasID = mpCurrentAliasItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(aliasID);
    if (!pT) {
        return;
    }

    mpAliasMainArea->trimName();
    const QString newName = mpAliasMainArea->lineEdit_alias_name->text();

    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentAliasItem->setText(0, newName);
    QString newStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);

    pushAliasPropertyCommand(mpUndoStack, mpHost, aliasID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_AliasPattern()
{
    if (mBlockPropertySave || !mpCurrentAliasItem) {
        return;
    }

    const int aliasID = mpCurrentAliasItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(aliasID);
    if (!pT) {
        return;
    }

    QString newPattern = mpAliasMainArea->lineEdit_alias_pattern->text();
    unmarkQString(&newPattern);

    if (pT->getRegexCode() == newPattern) {
        return;
    }

    // Mirror the explicit-save guard: refuse a pattern that would make the alias loop.
    const QString substitution = mpAliasMainArea->lineEdit_alias_command->text();
    if (aliasSubstitutionLoops(newPattern, substitution)) {
        showAliasLoopWarning(mpCurrentAliasItem, pT->getName());
        return;
    }

    QString oldStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);
    const QString typedName = mpAliasMainArea->lineEdit_alias_name->text().trimmed();
    const QString name = aliasNameForPattern(pT, typedName, newPattern);
    pT->setRegexCode(newPattern);
    if (name != typedName) {
        pT->setName(name);
        mpCurrentAliasItem->setText(0, name);
        mpAliasMainArea->lineEdit_alias_name->setText(name);
    }
    QString newStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);

    pushAliasPropertyCommand(mpUndoStack, mpHost, aliasID, pT->getName(), qsl("pattern"), oldStateXML, newStateXML);

    // Surface a faulty regex the same way the explicit Save button does.
    applyAliasState(mpCurrentAliasItem, pT);
}

void dlgTriggerEditor::slot_saveProperty_AliasCommand()
{
    if (mBlockPropertySave || !mpCurrentAliasItem) {
        return;
    }

    const int aliasID = mpCurrentAliasItem->data(0, Qt::UserRole).toInt();
    TAlias* pT = mpHost->getAliasUnit()->getAlias(aliasID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpAliasMainArea->lineEdit_alias_command->text();

    if (pT->getCommand() == newCommand) {
        return;
    }

    // Mirror the explicit-save guard: a substitution must not match its own pattern.
    QString regex = mpAliasMainArea->lineEdit_alias_pattern->text();
    unmarkQString(&regex);
    if (aliasSubstitutionLoops(regex, newCommand)) {
        showAliasLoopWarning(mpCurrentAliasItem, pT->getName());
        return;
    }

    QString oldStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommand(newCommand);
    QString newStateXML = exportAliasToXML(pT, SnapshotScope::ItemOnly);

    pushAliasPropertyCommand(mpUndoStack, mpHost, aliasID, pT->getName(), qsl("command"), oldStateXML, newStateXML);

    // Refresh the item so a fixed alias loses any stale error flag.
    applyAliasState(mpCurrentAliasItem, pT);
}

// =============================================================================
// Timer Per-Property Save Slots
// =============================================================================

// Helper function for per-property timer saves
static void pushTimerPropertyCommand(EditorUndoStack* undoStack, Host* host, int timerID, const QString& timerName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return;
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmTimerView, timerID, timerName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("timer:%1:%2").arg(QString::number(timerID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_TimerName()
{
    if (mBlockPropertySave || !mpCurrentTimerItem) {
        return;
    }

    const int timerID = mpCurrentTimerItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (!pT) {
        return;
    }

    mpTimersMainArea->trimName();
    const QString newName = mpTimersMainArea->lineEdit_timer_name->text();

    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentTimerItem->setText(0, newName);
    QString newStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);

    pushTimerPropertyCommand(mpUndoStack, mpHost, timerID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TimerCommand()
{
    if (mBlockPropertySave || !mpCurrentTimerItem) {
        return;
    }

    const int timerID = mpCurrentTimerItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpTimersMainArea->lineEdit_timer_command->text();

    if (pT->getCommand() == newCommand) {
        return;
    }

    QString oldStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommand(newCommand);
    QString newStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);

    pushTimerPropertyCommand(mpUndoStack, mpHost, timerID, pT->getName(), qsl("command"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_TimerTime()
{
    if (mBlockPropertySave || !mpCurrentTimerItem) {
        return;
    }

    const int timerID = mpCurrentTimerItem->data(0, Qt::UserRole).toInt();
    TTimer* pT = mpHost->getTimerUnit()->getTimer(timerID);
    if (!pT) {
        return;
    }

    const QTime newTime(mpTimersMainArea->timeEdit_timer_hours->time().hour(),
                        mpTimersMainArea->timeEdit_timer_minutes->time().minute(),
                        mpTimersMainArea->timeEdit_timer_seconds->time().second(),
                        mpTimersMainArea->timeEdit_timer_msecs->time().msec());

    if (pT->getTime() == newTime) {
        return;
    }

    QString oldStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);
    pT->setTime(newTime);
    QString newStateXML = exportTimerToXML(pT, SnapshotScope::ItemOnly);

    pushTimerPropertyCommand(mpUndoStack, mpHost, timerID, pT->getName(), qsl("time"), oldStateXML, newStateXML);
}

// =============================================================================
// Script Per-Property Save Slots
// =============================================================================

// Helper function for per-property script saves
static void
pushScriptPropertyCommand(EditorUndoStack* undoStack, Host* host, int scriptID, const QString& scriptName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return;
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmScriptView, scriptID, scriptName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("script:%1:%2").arg(QString::number(scriptID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_ScriptName()
{
    if (mBlockPropertySave || !mpCurrentScriptItem) {
        return;
    }

    const int scriptID = mpCurrentScriptItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    mpScriptsMainArea->trimName();
    const QString newName = mpScriptsMainArea->lineEdit_script_name->text();

    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentScriptItem->setText(0, newName);
    QString newStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);

    pushScriptPropertyCommand(mpUndoStack, mpHost, scriptID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ScriptEventHandlers()
{
    if (mBlockPropertySave || !mpCurrentScriptItem) {
        return;
    }

    const int scriptID = mpCurrentScriptItem->data(0, Qt::UserRole).toInt();
    TScript* pT = mpHost->getScriptUnit()->getScript(scriptID);
    if (!pT) {
        return;
    }

    const QStringList newHandlers = mpChipRow_scriptEvents->items();

    if (pT->getEventHandlerList() == newHandlers) {
        return;
    }

    QString oldStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);
    pT->setEventHandlerList(newHandlers);
    QString newStateXML = exportScriptToXML(pT, SnapshotScope::ItemOnly);

    pushScriptPropertyCommand(mpUndoStack, mpHost, scriptID, pT->getName(), qsl("eventHandlers"), oldStateXML, newStateXML);
}

// =============================================================================
// Key Per-Property Save Slots
// =============================================================================

// Helper function for per-property key saves
static void pushKeyPropertyCommand(EditorUndoStack* undoStack, Host* host, int keyID, const QString& keyName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return;
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmKeysView, keyID, keyName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("key:%1:%2").arg(QString::number(keyID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_KeyName()
{
    if (mBlockPropertySave || !mpCurrentKeyItem) {
        return;
    }

    const int keyID = mpCurrentKeyItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(keyID);
    if (!pT) {
        return;
    }

    mpKeysMainArea->trimName();
    const QString newName = mpKeysMainArea->lineEdit_key_name->text();

    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentKeyItem->setText(0, newName);
    QString newStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);

    pushKeyPropertyCommand(mpUndoStack, mpHost, keyID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_KeyCommand()
{
    if (mBlockPropertySave || !mpCurrentKeyItem) {
        return;
    }

    const int keyID = mpCurrentKeyItem->data(0, Qt::UserRole).toInt();
    TKey* pT = mpHost->getKeyUnit()->getKey(keyID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpKeysMainArea->lineEdit_key_command->text();

    if (pT->getCommand() == newCommand) {
        return;
    }

    QString oldStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommand(newCommand);
    QString newStateXML = exportKeyToXML(pT, SnapshotScope::ItemOnly);

    pushKeyPropertyCommand(mpUndoStack, mpHost, keyID, pT->getName(), qsl("command"), oldStateXML, newStateXML);
}

// =============================================================================
// Action Per-Property Save Slots
// =============================================================================

// Helper function for per-property action saves
static void
pushActionPropertyCommand(EditorUndoStack* undoStack, Host* host, int actionID, const QString& actionName, const QString& propertyName, const QString& oldStateXML, const QString& newStateXML)
{
    if (oldStateXML == newStateXML) {
        return;
    }

    auto* cmd = new EditorModifyPropertyCommand(EditorViewType::cmActionView, actionID, actionName, oldStateXML, newStateXML, host);
    cmd->setPropertyId(qsl("action:%1:%2").arg(QString::number(actionID), propertyName));
    undoStack->pushCommand(cmd);
}

void dlgTriggerEditor::slot_saveProperty_ActionName()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    mpActionsMainArea->trimName();
    const QString newName = mpActionsMainArea->lineEdit_action_name->text();

    if (pT->getName() == newName) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setName(newName);
    mpCurrentActionItem->setText(0, newName);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, newName, qsl("name"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionCommandDown()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpActionsMainArea->lineEdit_action_button_command_down->text();

    if (pT->getCommandButtonDown() == newCommand) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommandButtonDown(newCommand);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("commandDown"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionCommandUp()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const QString newCommand = mpActionsMainArea->lineEdit_action_button_command_up->text();

    if (pT->getCommandButtonUp() == newCommand) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setCommandButtonUp(newCommand);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("commandUp"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionIsPushDown()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const bool newValue = mpActionsMainArea->checkBox_action_button_isPushDown->isChecked();

    if (pT->isPushDownButton() == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setIsPushDownButton(newValue);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("isPushDown"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionBarColumns()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const int newValue = mpActionsMainArea->spinBox_action_bar_columns->value();

    if (pT->getButtonColumns() == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setButtonColumns(newValue);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("buttonColumn"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionBarFillerOffset()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const int newValue = mpActionsMainArea->spinBox_action_bar_offsetToFirstButton->value();

    if (pT->getButtonFillerOffset() == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setButtonFillerOffset(newValue);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("buttonFillerOffset"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionBarOrientation()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    // 0 = horizontal, 1 = vertical
    const int newValue = mpActionsMainArea->comboBox_action_bar_orientation->currentIndex();

    if (pT->mOrientation == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->mOrientation = newValue;
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("orientation"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionBarLocation()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    // CHECKME: This may need the increment if it isn't zero!
    const int newValue = mpActionsMainArea->comboBox_action_bar_location->currentIndex();

    if (pT->mLocation == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->mLocation = newValue;
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("barLocation"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionButtonRotation()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const int newValue = mpActionsMainArea->comboBox_action_button_rotation->currentIndex();

    if (pT->getButtonRotation() == newValue) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->setButtonRotation(newValue);
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("buttonRotation"), oldStateXML, newStateXML);
}

void dlgTriggerEditor::slot_saveProperty_ActionCSS()
{
    if (mBlockPropertySave || !mpCurrentActionItem) {
        return;
    }

    const int actionID = mpCurrentActionItem->data(0, Qt::UserRole).toInt();
    TAction* pT = mpHost->getActionUnit()->getAction(actionID);
    if (!pT) {
        return;
    }

    const QString newCSS = mpActionsMainArea->plainTextEdit_action_css->toPlainText();

    if (pT->css == newCSS) {
        return;
    }

    QString oldStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);
    pT->css = newCSS;
    QString newStateXML = exportActionToXML(pT, SnapshotScope::ItemOnly);

    pushActionPropertyCommand(mpUndoStack, mpHost, actionID, pT->getName(), qsl("css"), oldStateXML, newStateXML);
}

QTreeWidget* dlgTriggerEditor::treeForView(const EditorViewType view) const
{
    switch (view) {
    case EditorViewType::cmTriggerView:
        return treeWidget_triggers;
    case EditorViewType::cmAliasView:
        return treeWidget_aliases;
    case EditorViewType::cmScriptView:
        return treeWidget_scripts;
    case EditorViewType::cmTimerView:
        return treeWidget_timers;
    case EditorViewType::cmKeysView:
        return treeWidget_keys;
    case EditorViewType::cmActionView:
        return treeWidget_actions;
    case EditorViewType::cmVarsView:
        return treeWidget_variables;
    case EditorViewType::cmUnknownView:
        break;
    }
    return nullptr;
}

void dlgTriggerEditor::selectItemInView(const EditorViewType view, QTreeWidgetItem* pItem)
{
    switch (view) {
    case EditorViewType::cmTriggerView:
        slot_triggerSelected(pItem);
        break;
    case EditorViewType::cmAliasView:
        slot_aliasSelected(pItem);
        break;
    case EditorViewType::cmScriptView:
        slot_scriptsSelected(pItem);
        break;
    case EditorViewType::cmTimerView:
        slot_timerSelected(pItem);
        break;
    case EditorViewType::cmKeysView:
        slot_keySelected(pItem);
        break;
    case EditorViewType::cmActionView:
        slot_actionSelected(pItem);
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
}

bool dlgTriggerEditor::folderItem(const EditorViewType view, const int id)
{
    switch (view) {
    case EditorViewType::cmTriggerView: {
        auto* pTrigger = mpHost->getTriggerUnit()->getTrigger(id);
        return pTrigger && pTrigger->isFolder();
    }
    case EditorViewType::cmAliasView: {
        auto* pAlias = mpHost->getAliasUnit()->getAlias(id);
        return pAlias && pAlias->isFolder();
    }
    case EditorViewType::cmScriptView: {
        auto* pScript = mpHost->getScriptUnit()->getScript(id);
        return pScript && pScript->isFolder();
    }
    case EditorViewType::cmTimerView: {
        auto* pTimer = mpHost->getTimerUnit()->getTimer(id);
        return pTimer && pTimer->isFolder();
    }
    case EditorViewType::cmKeysView: {
        auto* pKey = mpHost->getKeyUnit()->getKey(id);
        return pKey && pKey->isFolder();
    }
    case EditorViewType::cmActionView: {
        auto* pAction = mpHost->getActionUnit()->getAction(id);
        return pAction && pAction->isFolder();
    }
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
    return false;
}

// A module's folder is rebuilt from the module file on every load and is not written back to it
QString dlgTriggerEditor::moduleOfMasterFolder(const EditorViewType view, const int id) const
{
    switch (view) {
    case EditorViewType::cmTriggerView:
        if (auto* pTrigger = mpHost->getTriggerUnit()->getTrigger(id); pTrigger && pTrigger->mModuleMasterFolder) {
            return pTrigger->mPackageName;
        }
        break;
    case EditorViewType::cmAliasView:
        if (auto* pAlias = mpHost->getAliasUnit()->getAlias(id); pAlias && pAlias->mModuleMasterFolder) {
            return pAlias->mPackageName;
        }
        break;
    case EditorViewType::cmScriptView:
        if (auto* pScript = mpHost->getScriptUnit()->getScript(id); pScript && pScript->mModuleMasterFolder) {
            return pScript->mPackageName;
        }
        break;
    case EditorViewType::cmTimerView:
        if (auto* pTimer = mpHost->getTimerUnit()->getTimer(id); pTimer && pTimer->mModuleMasterFolder) {
            return pTimer->mPackageName;
        }
        break;
    case EditorViewType::cmKeysView:
        if (auto* pKey = mpHost->getKeyUnit()->getKey(id); pKey && pKey->mModuleMasterFolder) {
            return pKey->mPackageName;
        }
        break;
    case EditorViewType::cmActionView:
        if (auto* pAction = mpHost->getActionUnit()->getAction(id); pAction && pAction->mModuleMasterFolder) {
            return pAction->mPackageName;
        }
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
    return {};
}

void dlgTriggerEditor::offerToUninstallModule(const QString& moduleName)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    //: Title of the dialog shown when the user tries to delete the folder that holds a module's items
    box.setWindowTitle(tr("Delete module folder"));
    //: %1 is the name of the module whose folder the user tried to delete
    box.setText(tr("This folder belongs to the module \"%1\" and can't be deleted on its own. Would you like to uninstall the module?").arg(moduleName));
    //: Button that removes the module whose folder the user tried to delete, along with all its items
    QPushButton* pUninstallButton = box.addButton(tr("Uninstall"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != pUninstallButton) {
        return;
    }

    if (!mpHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI)) {
        QString message;
        if (mpHost->currentlySavingProfile()) {
            //: %1 is the name of the module the user asked to remove
            message = tr("\"%1\" could not be removed while the profile is being saved. Please try again in a moment.").arg(moduleName);
        } else {
            //: %1 is the name of the module the user asked to remove, which turned out not to be installed any more
            message = tr("\"%1\" is no longer installed, so there was nothing to remove.").arg(moduleName);
        }
        //: Title of the dialog that says why a module the user asked to remove was not removed
        QMessageBox::warning(this, tr("Removal failed"), message);
    }
}

QTreeWidgetItem* dlgTriggerEditor::findItemById(QTreeWidget* pTree, const int id)
{
    QTreeWidgetItem* pRoot = pTree ? pTree->topLevelItem(0) : nullptr;
    QTreeWidgetItem* pItem = findItemByID(pRoot, id);
    return pItem == pRoot ? nullptr : pItem;
}

// What EditorTreeDelegate reads a Folders view row's state from: the one item it stands for,
// or nothing for a group of merged folders
static void setFolderTreeRowItem(QTreeWidgetItem* pNode, const EditorViewType view, const int id)
{
    TreeType type = TreeType::None;
    switch (view) {
    case EditorViewType::cmTriggerView:
        type = TreeType::Trigger;
        break;
    case EditorViewType::cmAliasView:
        type = TreeType::Alias;
        break;
    case EditorViewType::cmScriptView:
        type = TreeType::Script;
        break;
    case EditorViewType::cmTimerView:
        type = TreeType::Timer;
        break;
    case EditorViewType::cmKeysView:
        type = TreeType::Key;
        break;
    case EditorViewType::cmActionView:
        type = TreeType::Action;
        break;
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
    const bool oneItem = type != TreeType::None;
    pNode->setData(0, Qt::UserRole, oneItem ? QVariant(id) : QVariant());
    pNode->setData(0, uiDesign::EditorTreeDelegate::scmRole_rowTreeType, oneItem ? QVariant(static_cast<int>(type)) : QVariant());
}

QString dlgTriggerEditor::folderTreeItemKey(const EditorViewType view, const int id)
{
    return qsl("%1:%2").arg(static_cast<int>(view)).arg(id);
}

QString dlgTriggerEditor::folderTreeTypeName(const EditorViewType view, const bool plural)
{
    switch (view) {
    case EditorViewType::cmTriggerView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Triggers");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Trigger");
    case EditorViewType::cmAliasView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Aliases");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Alias");
    case EditorViewType::cmScriptView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Scripts");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Script");
    case EditorViewType::cmTimerView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Timers");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Timer");
    case EditorViewType::cmKeysView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Keys");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Key");
    case EditorViewType::cmActionView:
        if (plural) {
            //: Item types listed beside a folder in the editor's Folders view, such as "Triggers, Aliases"
            return tr("Buttons");
        }
        //: Item type shown beside an entry in the editor's Folders view
        return tr("Button");
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        break;
    }
    return {};
}

void dlgTriggerEditor::addFolderTreeNode(QTreeWidgetItem* pSource, const EditorViewType view, QTreeWidgetItem* pParent, QTreeWidgetItem* pLooseItems, QHash<QString, int>& folderCounts)
{
    const int id = pSource->data(0, IdRole).toInt();
    const QString itemKey = folderTreeItemKey(view, id);
    QTreeWidgetItem* pNode = nullptr;
    if (folderItem(view, id)) {
        // The k-th same-named folder of one type merges with the k-th of each other type, never with its own siblings
        const QString baseKey = (pParent ? pParent->data(0, FolderKeyRole).toString() : QString()) + QChar(0x1F) + pSource->text(0);
        const QString countKey = baseKey + QChar(0x1E) + QString::number(static_cast<int>(view));
        const int occurrence = folderCounts.value(countKey);
        folderCounts.insert(countKey, occurrence + 1);
        const QString key = baseKey + QChar(0x1E) + QString::number(occurrence);
        pNode = mFolderTreeNodes.value(key);
        if (!pNode) {
            pNode = new QTreeWidgetItem();
            pNode->setText(0, pSource->text(0));
            pNode->setData(0, FolderKeyRole, key);
            mFolderTreeNodes.insert(key, pNode);
            if (pParent) {
                pParent->addChild(pNode);
            } else {
                mpFolderTree->addTopLevelItem(pNode);
            }
        }
        QStringList parts = pNode->data(0, FolderPartsRole).toStringList();
        parts.append(itemKey);
        pNode->setData(0, FolderPartsRole, parts);
        setFolderTreeRowItem(pNode, parts.size() == 1 ? view : EditorViewType::cmUnknownView, id);
        const QString typeName = folderTreeTypeName(view, true);
        QStringList typeNames = pNode->text(1).isEmpty() ? QStringList() : pNode->text(1).split(qsl(", "));
        if (!typeNames.contains(typeName)) {
            typeNames.append(typeName);
            pNode->setText(1, typeNames.join(qsl(", ")));
            //: Screen reader description of a folder in the editor's Folders view, %1 is a list of item types such as "Triggers, Aliases"
            pNode->setData(0, Qt::AccessibleDescriptionRole, tr("Folder of %1").arg(pNode->text(1)));
        }
    } else {
        pNode = new QTreeWidgetItem();
        pNode->setText(0, pSource->text(0));
        pNode->setText(1, folderTreeTypeName(view, false));
        pNode->setData(0, FolderKeyRole, itemKey);
        pNode->setData(0, FolderPartsRole, QStringList{itemKey});
        pNode->setData(0, Qt::AccessibleDescriptionRole, pSource->data(0, Qt::AccessibleDescriptionRole));
        setFolderTreeRowItem(pNode, view, id);
        (pParent ? pParent : pLooseItems)->addChild(pNode);
    }
    mFolderTreeNodes.insert(itemKey, pNode);

    for (int i = 0, total = pSource->childCount(); i < total; ++i) {
        addFolderTreeNode(pSource->child(i), view, pNode, pLooseItems, folderCounts);
    }
}

void dlgTriggerEditor::populateFolderTree()
{
    QSet<QString> expandedKeys;
    for (QTreeWidgetItemIterator it(mpFolderTree); *it; ++it) {
        if ((*it)->isExpanded()) {
            expandedKeys.insert((*it)->data(0, FolderKeyRole).toString());
        }
    }
    QString previousKey;
    if (const QTreeWidgetItem* pCurrent = mpFolderTree->currentItem()) {
        previousKey = pCurrent->data(0, FolderKeyRole).toString();
        // Under one folder's form a merged folder stands for that folder, which a rebuild must open again
        if (mFolderGroupKey.isEmpty()) {
            const QString viewPrefix = qsl("%1:").arg(static_cast<int>(mCurrentView));
            for (const QString& part : pCurrent->data(0, FolderPartsRole).toStringList()) {
                if (part.startsWith(viewPrefix)) {
                    previousKey = part;
                }
            }
        }
    }
    const int scrollPosition = mpFolderTree->verticalScrollBar()->value();

    {
        const QSignalBlocker blocker(mpFolderTree);
        mpFolderTree->setUpdatesEnabled(false);
        mpFolderTree->clear();
        mFolderTreeNodes.clear();

        // Kept out of the tree until the end so it always comes last
        auto* pLooseItems = new QTreeWidgetItem();
        //: Group at the bottom of the editor's Folders view holding the items that are not inside any folder
        pLooseItems->setText(0, tr("Not in a folder"));
        pLooseItems->setData(0, FolderKeyRole, qsl("loose"));

        QHash<QString, int> folderCounts;
        for (const auto view :
             {EditorViewType::cmTriggerView, EditorViewType::cmAliasView, EditorViewType::cmScriptView, EditorViewType::cmTimerView, EditorViewType::cmKeysView, EditorViewType::cmActionView}) {
            QTreeWidgetItem* pRoot = treeForView(view)->topLevelItem(0);
            if (!pRoot) {
                continue;
            }
            for (int i = 0, total = pRoot->childCount(); i < total; ++i) {
                addFolderTreeNode(pRoot->child(i), view, nullptr, pLooseItems, folderCounts);
            }
        }
        if (pLooseItems->childCount()) {
            mpFolderTree->addTopLevelItem(pLooseItems);
            mFolderTreeNodes.insert(qsl("loose"), pLooseItems);
        } else {
            delete pLooseItems;
        }

        for (QTreeWidgetItemIterator it(mpFolderTree); *it; ++it) {
            if (expandedKeys.contains((*it)->data(0, FolderKeyRole).toString())) {
                (*it)->setExpanded(true);
            }
        }
        mFolderTreeStale = false;
        mpFolderTree->setUpdatesEnabled(true);
    }
    mpFolderTree->verticalScrollBar()->setValue(scrollPosition);
    syncFolderTreeSelection(previousKey);
}

void dlgTriggerEditor::refreshFolderTreeRows(const EditorViewType view, const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (!mFolderViewActive) {
        mFolderTreeStale = true;
        return;
    }
    if (mFolderTreeStale || !topLeft.parent().isValid()) {
        return;
    }
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        const QModelIndex index = topLeft.siblingAtRow(row);
        const QString itemKey = folderTreeItemKey(view, index.data(IdRole).toInt());
        QTreeWidgetItem* pNode = mFolderTreeNodes.value(itemKey);
        // A renamed folder may now merge with, or split from, another
        if (!pNode || pNode->text(0) != index.data(Qt::DisplayRole).toString()) {
            scheduleFolderTreeRefresh();
            return;
        }
        if (pNode->data(0, FolderKeyRole).toString() == itemKey) {
            pNode->setData(0, Qt::AccessibleDescriptionRole, index.data(Qt::AccessibleDescriptionRole));
        }
        // The delegate reads the item's state as it paints; a folder's row has no copied description to repaint it
        mpFolderTree->update(mpFolderTree->indexFromItem(pNode));
    }
}

void dlgTriggerEditor::scheduleFolderTreeRefresh()
{
    mFolderTreeStale = true;
    if (mFolderViewActive) {
        mpFolderTreeRefreshTimer->start();
    }
}

void dlgTriggerEditor::focusItemTree(QTreeWidget* pTypeTree)
{
    if (!mFolderViewActive) {
        focusPanelTree(pTypeTree);
        return;
    }
    syncFolderTreeSelection({});
    focusPanelTree(mpFolderTree);
}

QTreeWidget* dlgTriggerEditor::sectionTree(QTreeWidget* pTypeTree) const
{
    return mFolderViewActive ? mpFolderTree : pTypeTree;
}

void dlgTriggerEditor::showView(const EditorViewType view)
{
    switch (view) {
    case EditorViewType::cmAliasView:
        slot_showAliases();
        return;
    case EditorViewType::cmScriptView:
        slot_showScripts();
        return;
    case EditorViewType::cmTimerView:
        slot_showTimers();
        return;
    case EditorViewType::cmKeysView:
        slot_showKeys();
        return;
    case EditorViewType::cmActionView:
        slot_showActions();
        return;
    case EditorViewType::cmTriggerView:
    case EditorViewType::cmVarsView:
    case EditorViewType::cmUnknownView:
        slot_showTriggers();
        return;
    }
}

void dlgTriggerEditor::selectFolderTreeNode(QTreeWidgetItem* pNode)
{
    if (!pNode || pNode == mpFolderTree->currentItem()) {
        return;
    }
    {
        const QSignalBlocker blocker(mpFolderTree);
        mpFolderTree->setCurrentItem(pNode);
    }
    mpFolderTree->scrollToItem(pNode);
}

void dlgTriggerEditor::syncFolderTreeSelection(const QString& fallbackKey)
{
    collapseHiddenSelections();
    if (!mFolderGroupKey.isEmpty()) {
        QTreeWidgetItem* pGroupNode = mFolderTreeNodes.value(mFolderGroupKey);
        if (pGroupNode && pGroupNode->data(0, FolderPartsRole).toStringList().size() == 1) {
            selectFolderTreeNode(pGroupNode);
            slot_folderTreeItemChanged(pGroupNode);
            return;
        }
        if (pGroupNode) {
            selectFolderTreeNode(pGroupNode);
            showFolderGroup(pGroupNode);
            return;
        }
        showView(mCurrentView);
        return;
    }

    QTreeWidget* pTree = treeForView(mCurrentView);
    QTreeWidgetItem* pCurrent = pTree ? pTree->currentItem() : nullptr;
    if (pCurrent && pCurrent->parent()) {
        if (QTreeWidgetItem* pNode = mFolderTreeNodes.value(folderTreeItemKey(mCurrentView, pCurrent->data(0, IdRole).toInt()))) {
            selectFolderTreeNode(pNode);
            return;
        }
    }

    if (QTreeWidgetItem* pFallback = mFolderTreeNodes.value(fallbackKey)) {
        if (pFallback->data(0, FolderPartsRole).toStringList().size() > 1 && pFallback->data(0, FolderKeyRole).toString() != fallbackKey) {
            selectFolderTreeNode(pFallback);
            const QStringList viewAndId = fallbackKey.split(QLatin1Char(':'));
            openItemFromFolderTree(static_cast<EditorViewType>(viewAndId.constFirst().toInt()), viewAndId.constLast().toInt());
            return;
        }
        mpFolderTree->setCurrentItem(pFallback);
        return;
    }
    const QSignalBlocker blocker(mpFolderTree);
    mpFolderTree->setCurrentItem(nullptr);
}

void dlgTriggerEditor::slot_folderTreeItemChanged(QTreeWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }
    const QStringList parts = pItem->data(0, FolderPartsRole).toStringList();
    if (parts.size() != 1) {
        showFolderGroup(pItem);
        return;
    }
    const QStringList viewAndId = parts.constFirst().split(QLatin1Char(':'));
    openItemFromFolderTree(static_cast<EditorViewType>(viewAndId.constFirst().toInt()), viewAndId.constLast().toInt());
}

void dlgTriggerEditor::openItemFromFolderTree(const EditorViewType view, const int id)
{
    if (mNeedUpdateData) {
        saveOpenChanges();
        rebuildStaleTrees();
    }
    QTreeWidget* pTree = treeForView(view);
    QTreeWidgetItem* pItem = findItemById(pTree, id);
    if (!pItem) {
        scheduleFolderTreeRefresh();
        return;
    }

    changeView(view);
    {
        const QSignalBlocker blocker(pTree);
        // Without ClearAndSelect a held Ctrl or Shift would add to the hidden tree's selection, which Delete and Copy act on
        pTree->setCurrentItem(pItem, 0, QItemSelectionModel::ClearAndSelect);
    }
    pTree->scrollToItem(pItem);
    // Selecting fits the form pane to the item, which is what a view's own entry does too
    selectItemInView(view, pItem);
    updateEditorActivateAction();
}

void dlgTriggerEditor::showFolderGroup(QTreeWidgetItem* pNode)
{
    // Moving between merged folders leaves nothing new to save
    if (mFolderGroupKey.isEmpty()) {
        saveOpenChanges();
        clearEditorNotification();
    }
    mFolderGroupKey = pNode->data(0, FolderKeyRole).toString();

    mpTriggersMainArea->hide();
    mpAliasMainArea->hide();
    mpScriptsMainArea->hide();
    mpTimersMainArea->hide();
    mpKeysMainArea->hide();
    mpActionsMainArea->hide();
    mpVarsMainArea->hide();
    mpSourceEditorArea->hide();

    // The item the type tree still has selected is hidden, so nothing may act on it
    for (QAction* pAction : mpFolderTree->actions()) {
        if (pAction != mpAction_restoreEditorActionsToolbar) {
            pAction->setEnabled(false);
        }
    }
    mSaveItem->setEnabled(false);
    updateEditorActivateAction();
    mpExportAction->setEnabled(false);
    mpCreateModuleAction->setEnabled(false);
    slot_updateUndoRedoButtonStates();
}

void dlgTriggerEditor::hideFolderGroup()
{
    if (mFolderGroupKey.isEmpty()) {
        return;
    }
    mFolderGroupKey.clear();
    mpSourceEditorArea->show();
    for (QAction* pAction : mpFolderTree->actions()) {
        pAction->setEnabled(true);
    }
    mSaveItem->setEnabled(true);
    const bool itemView = mCurrentView != EditorViewType::cmVarsView && mCurrentView != EditorViewType::cmUnknownView;
    updateEditorActivateAction();
    mpExportAction->setEnabled(itemView);
    mpCreateModuleAction->setEnabled(true);
    slot_updateUndoRedoButtonStates();
}

void dlgTriggerEditor::leaveFolderView()
{
    if (!mFolderViewActive) {
        return;
    }
    mFolderViewActive = false;
    mpFolderTreeRefreshTimer->stop();
    mpFolderTree->hide();
    // On return the type trees decide what to highlight, not this stale row
    const QSignalBlocker blocker(mpFolderTree);
    mpFolderTree->setCurrentItem(nullptr);
}

void dlgTriggerEditor::slot_showFolders()
{
    if (!mpHost->folderViewInEditor()) {
        return;
    }
    if (mNeedUpdateData) {
        saveOpenChanges();
        rebuildStaleTrees();
    }
    mFolderViewActive = true;
    if (mFolderTreeStale) {
        populateFolderTree();
    }
    collapseHiddenSelections();
    showView(mCurrentView);
}

void dlgTriggerEditor::collapseHiddenSelections()
{
    // The Folders view shows one item at a time, so a selection made in a type tree, or by an add with Ctrl held, must not linger for Delete or Copy
    for (const auto view :
         {EditorViewType::cmTriggerView, EditorViewType::cmAliasView, EditorViewType::cmScriptView, EditorViewType::cmTimerView, EditorViewType::cmKeysView, EditorViewType::cmActionView}) {
        QTreeWidget* pTree = treeForView(view);
        if (pTree->selectedItems().size() > 1) {
            const QSignalBlocker blocker(pTree);
            pTree->setCurrentItem(pTree->currentItem(), 0, QItemSelectionModel::ClearAndSelect);
        }
    }
}

void dlgTriggerEditor::setFolderViewAvailable(const bool available)
{
    mpViewFoldersAction->setVisible(available);
    if (mpSidebarItem_folders) {
        mpSidebarItem_folders->setHidden(!available);
    }
    if (available || !mFolderViewActive) {
        return;
    }
    leaveFolderView();
    showView(mCurrentView);
}
