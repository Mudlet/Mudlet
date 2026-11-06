#ifndef MUDLET_UIDESIGN_H
#define MUDLET_UIDESIGN_H

/***************************************************************************
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

#include <QColor>
#include <QFont>
#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

class QAbstractButton;
class QAction;
class QBoxLayout;
class QGridLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QListWidget;
class QObject;
class QPainter;
class QPalette;
class QRect;
class QTabBar;
class QTimer;
class QWidget;
class TKeySequenceEdit;

namespace uiDesign {

// A property earns a constant here once a shared builder or delegate reads it; the rest stay literals, since a
// constant cannot be interpolated into a QStringLiteral.
inline constexpr char scmProp_searchKeywords[] = "searchKeywords";
// Named for the settings dialog but shared: it is the sidebar delegate's contract
inline constexpr char scmProp_rail[] = "settingsRail";
// A QSS ::item rule cannot ask whether its list holds the keyboard focus
inline constexpr char scmProp_focused[] = "settingsFocused";
// A stylesheet fills the editor's columns and the palette knows nothing of it, so a splitter handle reads each
// neighbour's tone from this
inline constexpr char scmProp_paneTone[] = "uiPaneTone";
inline constexpr char scmProp_settingsCard[] = "settingsCard";
inline constexpr char scmProp_aboutCard[] = "aboutCard";
inline constexpr char scmProp_settingsCardPlain[] = "settingsCardPlain";
inline constexpr char scmProp_aboutCardPlain[] = "aboutCardPlain";
// Opt-in: the editor's forms hold tool buttons that paint themselves and must not be drawn as buttons
inline constexpr char scmProp_menuButton[] = "uiMenuButton";
// Opt-in, and never on two buttons of one window: two lit buttons tell the reader nothing
inline constexpr char scmProp_primaryButton[] = "uiPrimaryButton";

// Every surface is mixed from the palette, so a theme change moves them all together; the depth model is
// "Three tones of depth" in docs/design-language.md.
struct ThemeTokens
{
    QColor text;
    QColor accent;
    // Off the palette rather than MudletApp::darkMode(), so a dark system theme
    // under "follow the system" gets the dark treatment too
    bool darkPage = false;
    QColor page;
    QColor card;
    QColor pane;
    QColor field;
    QColor border;
    QColor separator;
    QColor mutedText;
    QColor disabledText;
    // A toolbar button that cannot be used: dimmer than disabledText, which has to read on a card, so a bar's
    // unavailable buttons are told from its available ones at a glance
    QColor disabledBarText;
    QColor accentText;
    QColor marker;
    QColor hoverWash;
    QColor accentWash;
    QString hoverSoft;
    QString accentSoft;
};

ThemeTokens themeTokens();
// For a window whose colours a theme stylesheet decides: the palette a widget polished with that stylesheet ends up
// with, read off a fresh one, since a stylesheet freezes the palette of the widget it is set on
ThemeTokens themeTokens(const QPalette& themePalette);

// The deepest accent wash anything is drawn on: accentText is walked to read on it, so it holds on every lighter one
inline constexpr qreal scmAccentWashStrength = 0.24;

// Blue rather than green: "running" sits beside "broken" all over the editor, and a red-green colour-blind
// reader cannot tell those apart
inline constexpr qreal scmStateHue_ok = 0.58;
inline constexpr qreal scmStateHue_warning = 0.09;
// Green, apart from scmStateHue_ok: an update waiting is never read beside "broken", and the "!" and the word
// "Update" say it for a reader who cannot see the hue
inline constexpr qreal scmStateHue_update = 0.34;
// A shade off pure red rather than towards orange: at the saturation a red is
// mixed at, the orange side of it reads as tomato
inline constexpr qreal scmStateHue_error = 0.01;
QColor stateColor(const qreal hue, const bool darkPage);

// Walked off the separator, the darkest surface any error ink sits on; a caller on a lighter surface walks it on
// with readableOn() against that surface.
QColor errorInk(const ThemeTokens& tokens);

// Light enough that the words on the wash still read against the surface underneath
inline constexpr qreal scmSoftWashStrength = 0.14;

inline constexpr qreal scmHoverWashStrength = 0.07;

// A console is drawn in the profile's colours, not the theme's, so this mixes its command-line colours
inline constexpr qreal scmConsoleFootSeamStrength = 0.20;

// A scroll area's own bars answer only to a descendant selector, hence the prefix. An invalid surface means the page.
QString scrollBarStyleSheet(const QString& selectorPrefix, const ThemeTokens& tokens, const QColor& surface = QColor());

// The view's delegate must paint the accent bar with paintAccentBar(): a coloured border-left bends into an arc at
// the corner radius, so the border here is only a transparent gutter. rowGutter includes the bar's width.
QString itemRowStyleSheet(const QString& viewSelector, const ThemeTokens& tokens, const QColor& surface, const int rowGutter);

// Separate from choiceStyleSheet() only because a view's indicator is a sub-control of the view
QString itemCheckMarkStyleSheet(const QString& viewSelector, const ThemeTokens& tokens);

// Empty where the chevrons could not be cached: a rule aimed at a missing file draws nothing, leaving no handle to
// fold a group by.
QString treeBranchStyleSheet(const QString& viewSelector, const ThemeTokens& tokens, const QColor& surface);

inline constexpr int scmItemRowPaddingVertical = 2;
inline constexpr int scmItemRowPaddingTrailing = 4;

inline constexpr int scmSidebarRailWidth = 46;
inline constexpr int scmSidebarPadding = 12;
inline constexpr int scmSidebarRailPadding = 6;
inline constexpr int scmSidebarSeparatorInset = 12;
inline constexpr int scmSidebarRowHeight = 36;
inline constexpr int scmSidebarIconSize = 18;

struct SidebarMetrics
{
    int expandedWidth = 0;
    int railWidth = 0;
    int padding = 0;
    int railPadding = 0;
    int verticalPadding = 0;
    // Collapsed, both windows hold the divider 2px off the edges, so only the expanded inset is asked for
    int separatorInset = 0;
};

// Object names, not selectors: setSidebarCollapsed() finds the separators by the same name
QString sidebarStyleSheet(const QString& listName, const QString& separatorName, const QColor& itemColor, const SidebarMetrics& metrics, const ThemeTokens& tokens);

// Not a constant: a style's item margins differ per appearance (2px under the dark theme's Fusion, 4px under
// macOS), so a row wide enough in one elides in the other.
// Also independent of the list's sheet: callers measure before building it, since the bar stop is a fraction of this width
int sidebarRowWidth(const QListWidget* pList, const QString& name);

// False when nothing moved, so a caller can skip its own follow-up
bool setSidebarCollapsed(QWidget* pPane, QListWidget* pList, const QString& separatorName, const bool collapsed, const SidebarMetrics& metrics);

inline constexpr int scmRadiusChip = 4;
// A border, not a gap: whatever leaves room for the bar takes it out of its own padding
inline constexpr int scmAccentBarWidth = 3;
inline constexpr int scmRadiusInput = 5;
inline constexpr int scmRadiusPanel = 8;
inline constexpr int scmRadiusProminentInput = scmRadiusPanel;

// Set on a form rather than a window, so a code pane keeps the field it paints itself; a window with no form
// passes the container's selector as selectorPrefix.
QString inputStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix = QString());

// Exported for a read-only field, which is let down the same way without being disabled
QColor disabledFieldColour(const ThemeTokens& tokens);
QColor disabledBorderColour(const ThemeTokens& tokens);

// The editor's segmented control zeroes its indicator with property selectors, which outrank the type selectors
// here.
QString choiceStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix = QString());

// A styled check indicator has no size of its own to fall back on
inline constexpr int scmChoiceIndicatorSize = 13;
inline constexpr int scmChoiceGlyphSize = scmChoiceIndicatorSize - 4;
inline constexpr int scmChoiceDotDiameter = 6;

QString buttonStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix = QString());

// cornerRadius must be the button's own, or a lit half pokes square corners out of it. Empty when the chevron
// could not be cached.
QString splitButtonMenuHalfStyleSheet(const QString& buttonSelector, const int cornerRadius, const ThemeTokens& tokens);

enum class ToolBarSeam { Bottom, Top };

QString toolBarStyleSheet(const QString& toolBarSelector, const ToolBarSeam seam, const ThemeTokens& tokens);

inline constexpr int scmToolBarGripExtent = 11;
// Off the radius scale, but named: a split button's menu half on the bar must round to the same corner
inline constexpr int scmToolBarButtonRadius = 6;
inline constexpr int scmToolBarButtonPaddingVertical = 3;
inline constexpr int scmToolBarButtonPaddingHorizontal = 7;

QString disclosureButtonStyleSheet(const QString& buttonSelector, const ThemeTokens& tokens);

// Follow with letPopupsTakeTheFieldsCorner() on the same container; a menu built on demand gets both before exec().
QString menuStyleSheet(const ThemeTokens& tokens, const QString& selectorPrefix = QString());

inline constexpr int scmNoticeGlyphSize = 20;
inline constexpr int scmNoticePaddingHorizontal = 10;
inline constexpr int scmNoticePaddingVertical = 8;
inline constexpr int scmNoticeSpacing = 8;

enum class NoticeKind { Information, Warning, Error };

QString noticeStyleSheet(const QString& frameSelector, const ThemeTokens& tokens);

QPixmap noticeGlyph(const NoticeKind kind, const ThemeTokens& tokens, const qreal devicePixelRatio);

// Fixed to the glyph's square, or the label keeps the height the .ui file's 64px bitmap needed
void applyNoticeGlyph(QLabel* pLabel, const NoticeKind kind, const ThemeTokens& tokens);

// Call prepareTabStrip() on the bar too. A crowded strip's scroll buttons are left alone: their arrows belong to a
// QToolButton the bar makes, and a face without them is a blank square.
QString tabBarStyleSheet(const QString& tabWidgetSelector, const ThemeTokens& tokens);

// Safe before or after the bar holds tabs
void prepareTabStrip(QTabBar* pTabBar);

inline constexpr int scmTabPaneInset = 4;

// Public because TTabBar paints the profile strip's chips from these same numbers
inline constexpr int scmMenuItemPaddingVertical = 4;
inline constexpr int scmTabPaddingVertical = scmMenuItemPaddingVertical;
inline constexpr int scmTabPaddingHorizontal = 10;
inline constexpr int scmTabGap = 2;
inline constexpr int scmTabStripInset = scmTabPaneInset;
inline constexpr int scmTabCloseBoxSize = 14;
inline constexpr int scmTabCloseGlyphSize = 8;

// QAbstractButton reads SH_Button_FocusPolicy once, in its constructor, and the base style swaps with the
// appearance; both answer Qt::StrongFocus, so a click leaves the focus accent on the control. Call after setting
// the sheets; re-running is harmless.
void keepClickFocusOffControls(QWidget* pRoot);

// The platform reads WA_TranslucentBackground only when it first creates a popup's surface, so call this before a
// combo box or menu is first shown, including ones built later. Only where menuStyleSheet() reaches: a menu with
// nothing painting its surface turns see-through all over.
void letPopupsTakeTheFieldsCorner(QWidget* pRoot);

inline constexpr int scmInputContentHeight = 22;
inline constexpr int scmInputPaddingVertical = 2;
inline constexpr int scmInputPaddingHorizontal = 6;
inline constexpr int scmInputBorderWidth = 1;
inline constexpr int scmInputHeight = scmInputContentHeight + 2 * (scmInputPaddingVertical + scmInputBorderWidth);
inline constexpr int scmInputStepperWidth = 16;
// A window drawing its own split button pads its words clear of this itself: no selector can ask whether a button
// carries a menu.
inline constexpr int scmInputDropDownWidth = 18;

qreal contrastRatio(const QColor& first, const QColor& second);

inline constexpr qreal scmTextMinimumRatio = 4.5;
// WCAG's lower floor, for an unavailable word or a placeholder
inline constexpr qreal scmQuietMinimumRatio = 3.0;
// WCAG asks nothing of an inactive control; about where macOS draws one, at a quarter of the label's opacity
inline constexpr qreal scmDisabledBarMinimumRatio = 2.0;

// fallback when the lightness walk runs out before minimumRatio
QColor readableOn(const QColor& background, const QColor& wanted, const QColor& fallback, const qreal minimumRatio);

// QLayout::setAlignment() does not look in child layouts, and cards nest controls in them
bool alignInLayoutTree(QLayout* pLayout, const QWidget* pWidget, const Qt::Alignment alignment);

// QLayout::removeWidget() only looks at its own items, and the .ui file nests controls deeply.
// QLayout::addWidget() would find them, but warns once per widget, flooding the console.
bool removeFromLayoutTree(QLayout* pLayout, QWidget* pWidget);

void detachFromLayout(QWidget* pWidget);

void invalidateLayoutsUpTo(QWidget* pWidget, const QWidget* pTop);

// A profile's Lua stylesheet reaches every widget in the dialog it does not name, so the shell's
// scaffolding carries a property the shell stylesheet uses to keep it transparent.
void markAsShellSurface(QWidget* pWidget);

// QGridLayout cannot insert a row, so every item is re-added one row lower with its row properties.
// Columns are untouched, so a .ui file's column stretches still apply.
void insertGridRowAtTop(QGridLayout* pGrid, QWidget* pWidget);

// One translatable string with %1 where the control goes, so a translation can move it; a translation that lost
// its %1 gets the control after the words.
void buildControlSentenceRow(QBoxLayout* pRow, const QString& translatedSentence, QWidget* pControl);

// Unlike the single-control form, the controls keep their own accessible names, since one sentence cannot name
// several fields: set one on each before building the row.
void buildControlSentenceRow(QBoxLayout* pRow, const QString& translatedSentence, const QList<QWidget*>& controls);

// A row that navigates rather than sets something; the shell stylesheet draws its chevron from this property
void makeChevronRow(QAbstractButton* pButton);

void collectFocusableInLayoutOrder(const QLayout* pLayout, QList<QWidget*>& chain);

QString spotlightStyleSheet(const QColor& accent, const qreal strength);

QString foldForSearch(const QString& text);

// Not for combo boxes: they show one item, and callers want the whole list or nothing
QString visibleTextOf(const QWidget* pWidget);

void collectSearchText(const QWidget* pWidget, QStringList& parts);

// Includes synonyms, so a keyword match still highlights the control carrying it
QString highlightTextOf(const QWidget* pWidget);

// One ideograph is a word, one Latin letter is not: a lone letter matches most of the dialog
bool wordEnoughToSearch(const QStringList& needles);

inline constexpr qreal scmGripDotDiameter = 2.0;
inline constexpr qreal scmGripDotPitch = 3.0;
inline constexpr int scmGripDotsAcross = 2;
inline constexpr int scmGripDotsAlong = 3;

// Empty if the cache cannot be written. alongTheBar: a bar running across the window takes a tall grip, one down
// its side a wide one.
QString gripGlyphFile(const QColor& color, const bool alongTheBar);

// A stylesheet rule selecting on a property only takes effect on a re-polish
void repolish(QWidget* pWidget);

void setSearchMatch(QWidget* pWidget, const QVariant& matched);

// Qt propagates no font into or out of a widget carrying Qt::WA_StyleSheet. While this lives the flag is off on
// the widget, its parent and its same-window subtree, so Qt's own propagation crosses the sheets; Qt sets it again
// per widget after walking its subtree, and the destructor sets it on the rest.
class StyleSheetFontLift
{
public:
    explicit StyleSheetFontLift(QWidget* pWidget);
    ~StyleSheetFontLift();
    Q_DISABLE_COPY_MOVE(StyleSheetFontLift)

private:
    void lift(QWidget* pWidget);

    // QPointer, as a FontChange handler in the tree is free to rebuild widgets
    QList<QPointer<QWidget>> mLifted;
};

void resolveFontsUnderStyleSheets(QWidget* pRoot);

QColor blend(const QColor& from, const QColor& to, const qreal amount);

QString rgba(const QColor& color, const qreal alpha);
QString rgba(const QColor& colourWithAlpha);

// cornerRadius is the pill's own, so the bar takes its corner at both ends
void paintAccentBar(QPainter* pPainter, const QRect& row, const QColor& accent, const int cornerRadius = scmRadiusPanel);

// Fills through the alpha channel, keeping antialiased edges that recolouring pixels would harden
QPixmap tintedGlyph(const QPixmap& source, const QColor& color);

// Lucide authors at 2, which reads heavy at toolbar and editor sizes; the glyphs ship as SVG so the weight is set
// here.
inline constexpr qreal scmGlyphStrokeWidth = 1.5;

inline constexpr int scmGlyphRasterSize = 128;

// The only way a glyph file becomes a pixmap, so every glyph gets scmGlyphStrokeWidth. Cached, as parsing an SVG
// per use measurably slows the About dialog's construction.
// Raster files pass through untouched: the GitHub, Patreon and Discord marks are filled shapes with no stroke to set
QPixmap glyphPixmap(const QString& file);

QPixmap themedGlyphPixmap(const QString& glyphFile, const QColor& color, const int boxSize, const int glyphSize, const qreal devicePixelRatio);

// QIcon::Active is what a QToolButton asks for under the pointer, and QIcon::Selected is supplied so a view does not
// wash a chosen row's glyph in the highlight colour itself.
QIcon tintedIcon(const QString& glyphOff, const QString& glyphOn, const ThemeTokens& tokens);
QIcon tintedIcon(const QString& glyph, const ThemeTokens& tokens);

// The main colour of the picture a glyph replaced, measured from the old PNG, for a player who still finds an icon
// by it. A hue below 0 is no colour: the old picture was grey, or a colour already means something on
// that button (About's green arch against its update state).
struct IdentityColour
{
    qreal hue = -1.0;
    qreal saturation = 0.0;
    qreal lightness = 0.0;
};
inline constexpr IdentityColour scmIdentity_connect{0.097, 0.86, 0.56};
inline constexpr IdentityColour scmIdentity_triggers{0.065, 0.81, 0.53};
inline constexpr IdentityColour scmIdentity_buttons{0.127, 0.92, 0.54};
inline constexpr IdentityColour scmIdentity_variables{0.167, 1.00, 0.25};
inline constexpr IdentityColour scmIdentity_sound{0.574, 0.73, 0.41};
inline constexpr IdentityColour scmIdentity_discord{0.653, 0.86, 0.65};
inline constexpr IdentityColour scmIdentity_map{0.586, 0.54, 0.49};
inline constexpr IdentityColour scmIdentity_manual{0.139, 0.94, 0.60};
inline constexpr IdentityColour scmIdentity_packages{0.103, 0.38, 0.53};
inline constexpr IdentityColour scmIdentity_reconnect{0.125, 0.85, 0.50};
inline constexpr IdentityColour scmIdentity_add{0.333, 0.85, 0.36};
inline constexpr IdentityColour scmIdentity_addGroup{0.591, 0.46, 0.59};
inline constexpr IdentityColour scmIdentity_activate{0.129, 0.92, 0.45};
inline constexpr IdentityColour scmIdentity_delete{0.003, 0.79, 0.43};
inline constexpr IdentityColour scmIdentity_transfer{0.333, 0.77, 0.34};
inline constexpr IdentityColour scmIdentity_folder{0.619, 0.89, 0.70};
inline constexpr IdentityColour scmIdentity_errors{0.004, 0.74, 0.49};
inline constexpr IdentityColour scmIdentity_statistics{0.333, 0.74, 0.44};
inline constexpr IdentityColour scmIdentity_debug{0.000, 0.90, 0.39};
// Settings pages had no coloured pictures, so these two are chosen rather than measured; the others that are
// coloured open what a toolbar button opens, and take its colour
// The input line's, and the aliases' typed into it
inline constexpr IdentityColour scmIdentity_input{0.300, 0.58, 0.45};
// The access symbol's blue
inline constexpr IdentityColour scmIdentity_accessibility{0.580, 0.75, 0.50};
// The paint on the Appearance palette, a dot each, in the order of the ids in settings-appearance-dots.svg
inline constexpr IdentityColour scmIdentityPaintDots[]{scmIdentity_delete, scmIdentity_buttons, scmIdentity_input, scmIdentity_map};
// How much of the colour washes the glyph's own shapes under its line
inline constexpr qreal scmIdentityFillOnDark = 0.45;
inline constexpr qreal scmIdentityFillOnLight = 0.40;

// The line, walked to scmQuietMinimumRatio on the bar and on the menus a button opens
QColor identityInk(const IdentityColour& identity, const ThemeTokens& tokens);
QColor identityFill(const IdentityColour& identity, const ThemeTokens& tokens);

// The glyph's shapes filled in with no line, for a wash under it. A glyph whose lines leave a shape open beside
// <name>.svg ships a <name>-fill.svg holding the shape to fill instead. Empty for a raster file.
QPixmap glyphFillPixmap(const QString& file);
// The glyph's line in identityInk() over its wash in identityFill(), and its dots of paint
QPixmap identityGlyph(const QString& file, const IdentityColour& identity, const ThemeTokens& tokens);
// The colour in every state but unavailable: it is what the player looks for, and the surface under a hovered,
// chosen or switched-on button already shows which
QIcon identityIcon(const QString& glyphOff, const QString& glyphOn, const IdentityColour& identity, const ThemeTokens& tokens, const QColor& disabledInk);
// A glyph with dots of paint ships <name>-dots.svg, whose dot-N elements take scmIdentityPaintDots in turn; a grey
// glyph keeps them too, as the paint is what makes it a palette
void addPaintDots(QPixmap& glyph, const QString& file, const ThemeTokens& tokens);

struct ActionGlyph
{
    QPointer<QAction> pAction;
    QString glyphOff;
    // Left empty where both states are the same picture
    QString glyphOn;
    IdentityColour identity;
};

void restyleActionGlyphs(const QList<ActionGlyph>& glyphs, const ThemeTokens& tokens);
// For actions on a toolbar: unavailable, the glyph takes disabledBarText as the bar's words do. Coloured, a glyph with
// an identity colour keeps it at rest, under the pointer and switched on.
void restyleToolBarGlyphs(const QList<ActionGlyph>& glyphs, const ThemeTokens& tokens, const bool coloured = false);

struct CardMetrics
{
    const char* cardProperty = scmProp_settingsCard;
    // Null when every card of the window carries a title
    const char* plainProperty = nullptr;
    int padding = 0;
    // From measuredCardTitleHeight()
    int titleHeight = 0;
    bool flattenNestedGroupBoxes = false;
};

QString cardStyleSheet(const CardMetrics& metrics, const ThemeTokens& tokens);

// Fusion draws a group box's indicator from palette(window) darkened 40%, a 1.1:1 outline on a dark card, and a
// styled indicator loses its check mark, so the whole mark is drawn. Separate from cardStyleSheet() because
// measuredCardTitleHeight() measures under these rules alone.
QString cardIndicatorStyleSheet(const char* cardProperty, const ThemeTokens& tokens);

// Measured rather than added up: the room a style leaves round a title and its check indicator is the style's own
int measuredCardTitleHeight(QWidget* pParent, const QString& indicatorRules, const char* cardProperty = scmProp_settingsCard);

inline constexpr int scmCardTitleGap = 8;

// QLabel rich text reaches images only by URL, and a glyph tinted at runtime has no path, so it is inlined
QString inlineGlyph(const QPixmap& glyph);

// A QLabel bakes each anchor's colour at setText() from the application palette, so neither QPalette::Link on the
// widget nor a re-polish reaches it; a colour on the anchor does. Keep the raw text to re-ink it on an appearance
// change.
QString withLinkColour(const QString& richText, const QColor& colour);

QFont fixedPitchFont(const QFont& base, const qreal scale = 1.0);

// In points: Qt's stylesheet parser reads only pt and px for font-size and silently drops a percentage. Whole
// points, so a height measured off a step stays an integer.
enum class TypeStep { Caption, Body, Title, Display };
int typeSize(const TypeStep step);

// Covers the types connectApplyTriggers() listens to, so anything that can schedule an apply can be
// compared with its snapshot
QVariant controlValue(const QObject* pControl);

// Qt sets the modified flag on the first keystroke and slot_lineEditFinished() clears it; until then the
// field holds a half-typed word, which neither the apply nor the snapshot takes as a setting.
bool beingTypedInto(const QObject* pControl);

// Control values as last read from the settings, so an apply writes back only what the user changed.
// Both references are to members of the owning dialog, so they outlive this. Keys may dangle: they are
// only compared, never dereferenced, and a control created after the last snapshot reads as dirty.
class SettingsSnapshot
{
public:
    Q_DISABLE_COPY(SettingsSnapshot)
    SettingsSnapshot(const QWidget& owner, const QMap<QString, QKeySequence>& shortcuts);
    static bool carriesValue(const QObject* pControl);
    // Call once the controls hold the settings' values; later differences count as user edits
    void take();
    // For one control whose list was rebuilt while the dialog is showing
    void take(const QObject* pControl);
    bool dirty(const QObject* pControl) const;
    bool anyDirty(const QList<const QObject*>& controls) const;
    bool shortcutsDirty() const;
    bool shortcutDirty(const QString& key) const;
    bool pendingEdits(const QTimer* pApplyTimer, const QLineEdit* pSearchField) const;
    // Lets a second profile reuse the first one's editors rather than adding another row
    TKeySequenceEdit* editorFor(const QString& key) const;
    void addEditor(const QString& key, TKeySequenceEdit* pEditor);

private:
    const QWidget& mOwner;
    const QMap<QString, QKeySequence>& mCurrentShortcuts;
    QHash<const QObject*, QVariant> mValues;
    QMap<QString, QKeySequence> mShortcuts;
    QMap<QString, QPointer<TKeySequenceEdit>> mEditors;
};

} // namespace uiDesign

#endif // MUDLET_UIDESIGN_H
