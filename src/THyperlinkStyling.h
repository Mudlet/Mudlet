#ifndef MUDLET_THYPERLINKSTYING_H
#define MUDLET_THYPERLINKSTYING_H

/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoeshnHeiko@googlemail.com  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2024 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QString>

// Enhanced OSC 8 hyperlink styling support with CSS link states
namespace Mudlet {

struct HyperlinkStyling {
    // Base styling properties
    QColor foregroundColor;
    QColor backgroundColor;
    bool hasForegroundColor = false;
    bool hasBackgroundColor = false;
    bool isBold = false;
    bool isItalic = false;
    bool isUnderlined = false; // OSC 8 hyperlinks default to no underline (unlike other Mudlet hyperlinks)
    bool isStrikeOut = false;
    bool isOverlined = false;
    bool hasCustomStyling = false; // Tracks if any custom styling was provided
    bool hasBaseCustomStyling = false; // Tracks if base (non-pseudo-class) styling was provided

    // Extended text decoration support
    enum UnderlineStyle {
        UnderlineNone,
        UnderlineSolid,     // Standard underline
        UnderlineWavy,      // Squiggly/wavy underline
        UnderlineDotted,    // Dotted underline
        UnderlineDashed     // Dashed underline
    };
    UnderlineStyle underlineStyle = UnderlineSolid;
    QColor underlineColor;
    QColor overlineColor;
    QColor strikeoutColor;
    bool hasUnderlineColor = false;
    bool hasOverlineColor = false;
    bool hasStrikeoutColor = false;

    // CSS Link State Support with Accessibility
    enum LinkState {
        StateDefault,       // Default/unvisited (:link)
        StateVisited,       // Visited link (:visited)
        StateHover,         // Mouse hover (:hover)
        StateActive,        // Mouse down/active (:active)
        StateFocus,         // Keyboard focus (:focus)
        StateFocusVisible,  // Visible keyboard focus (:focus-visible)
        StateSelected,      // Selected state (from selection object)
        StateDisabled       // Disabled state (from selection object)
    };

    // State-specific styling containers
    struct StateStyle {
        QColor foregroundColor;
        QColor backgroundColor;
        QColor underlineColor;
        QColor overlineColor;
        QColor strikeoutColor;
        bool hasForegroundColor = false;
        bool hasBackgroundColor = false;
        bool hasUnderlineColor = false;
        bool hasOverlineColor = false;
        bool hasStrikeoutColor = false;
        bool isBold = false;
        bool isItalic = false;
        bool isUnderlined = false;
        bool isStrikeOut = false;
        bool isOverlined = false;
        UnderlineStyle underlineStyle = UnderlineSolid;
        bool hasCustomStyling = false;
    };

    // State-specific styles
    StateStyle linkStyle;           // :link (unvisited)
    StateStyle visitedStyle;        // :visited
    StateStyle hoverStyle;          // :hover
    StateStyle activeStyle;         // :active
    StateStyle focusStyle;          // :focus
    StateStyle focusVisibleStyle;   // :focus-visible
    StateStyle anyLinkStyle;        // :any-link (applies to both :link and :visited)
    StateStyle selectedStyle;       // :selected (from selection object)
    StateStyle disabledStyle;       // :disabled (from selection object)

    // State tracking
    LinkState currentState = StateDefault;

    // Methods to get effective styling for current state
    StateStyle getEffectiveStyle() const;

    // Selection control: toggleable, stateful links (radio/checkbox behavior)
    // JSON: {"group": "string", "value": "string", "toggle": bool, "selected": bool, "exclusive": bool, "disabled": bool}
    // When exclusive=true: radio button behavior (only one selected per group)
    // When exclusive=false: checkbox behavior (multiple selections per group)
    struct SelectionSettings {
        QString group;              // Group identifier for related selections
        QString value;              // Unique value within the group
        bool toggle = true;         // Allow deselecting when already selected
        bool selected = false;      // Initial selection state
        bool exclusive = true;      // Radio (true) vs checkbox (false) mode
        bool disabled = false;      // Cannot be clicked when disabled
        bool hasSelectionSettings = false;
    };

    SelectionSettings selection;

    // Visibility control: conceal (hide after delay/expire) or reveal (show after delay/expire)
    // JSON: {"action": "conceal"|"reveal"|["reveal","conceal"], "delay": ms, "wholeline": bool, "expire": {...}}
    // expire object: {"input": bool, "prompt": bool, "output": bool, "outputDelay": ms}
    // When action is ["reveal","conceal"]: starts hidden, reveals on trigger, then conceals on click
    struct VisibilitySettings {
        // Maximum allowed delay value (24 hours in milliseconds)
        static constexpr quint32 MaxDelayMs = 86400000;
        // Default output delay for batch detection (500ms)
        static constexpr quint32 DefaultOutputDelayMs = 500;

        enum class Action {
            None,
            Conceal,
            Reveal,
            RevealThenConceal  // Combined: reveal first, then conceal after click
        };

        Action action = Action::None;
        quint32 delayMs = 0;
        bool deletesEntireLine = false;
        bool isConcealed = false;
        bool hasVisibilitySettings = false;

        // Expire triggers - when visibility action should occur
        bool expireOnInput = false;    // User types/submits something
        bool expireOnPrompt = false;   // GA/EOR telnet signal received
        bool expireOnOutput = false;   // New output after idle gap
        quint32 outputDelayMs = DefaultOutputDelayMs;  // Idle gap for output trigger
    };

    VisibilitySettings visibility;

    bool isSpoiler = false;
};

} // namespace Mudlet

#endif // MUDLET_THYPERLINKSTYING_H
