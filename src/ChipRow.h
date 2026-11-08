#ifndef MUDLET_CHIPROW_H
#define MUDLET_CHIPROW_H

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
#include <QFrame>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>

class QCompleter;
class QLabel;
class QLineEdit;
class QStandardItemModel;
class QTimer;
class QToolButton;

namespace uiDesign {

class FlowLayout;
class PlaceholderButton;
class SuggestionDelegate;
struct ThemeTokens;

struct Suggestion
{
    QString name;
    QString note;
    // Said when the pointer rests on the word of a chip holding this name
    QString origin;
    // The program's own name rather than one the user made, so a chip holding it reads in the chrome tone
    bool system = false;
};

// pOn is the widget the chip lies on, not the chip: see Chip::remeasure()
QFont chipFont(const QWidget* pOn);

class Chip : public QFrame
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(Chip)
    Chip(const QString& name, QWidget* pParent);

    void setName(const QString& name);
    [[nodiscard]] QString name() const { return mName; }
    // Each %1 is the chip's name
    void setWords(const QString& accessibleName, const QString& removeTooltip);
    void setRemoveGlyph(const QIcon& glyph);
    void setOrigin(const QString& origin, const bool system);
    // Public: a chip is measured off its row's font, so the row calls this when that font changes
    void remeasure();

signals:
    void editRequested();
    void removeRequested();

protected:
    void changeEvent(QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;

private:
    void applyPadding();

    QLabel* mpLabel = nullptr;
    QToolButton* mpRemove = nullptr;
    QString mName;
    QString mAccessibleNameWords;
    QString mRemoveTooltipWords;
};

class ChipRow : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ChipRow)
    explicit ChipRow(QWidget* pParent = nullptr);

    [[nodiscard]] QStringList items() const;
    // Emits nothing and shuts any open field: this is the row being filled, not edited
    void setItems(const QStringList& items);
    [[nodiscard]] int count() const;
    [[nodiscard]] QWidget* chipAt(const int index) const;
    // setItems() drops empty and repeated names, so an index into what was handed over is not one here
    [[nodiscard]] int indexOf(const QString& name) const;
    void focusItem(const int index);
    void beginAdd();

    void setAddWords(const QString& button, const QString& tooltip, const QString& fieldPrompt);
    // What a chip is called to a screen reader and on its cross, each %1 its name; a chip is an event until set
    void setChipWords(const QString& accessibleName, const QString& removeTooltip);

    // Names already on a chip are left out of the offer, since adding one again would be refused.
    // Every chip takes its tone and the words on its tooltip from the suggestion of its name.
    void setSuggestions(const QList<Suggestion>& suggestions);
    // Said on a chip whose name no suggestion carries
    void setUnlistedOrigin(const QString& origin);
    // The popup is a window of its own, so the form's stylesheet never reaches it
    void restyleSuggestions(const ThemeTokens& tokens);

    // Chips grow to the field's height where the form's sheet makes it the taller.
    // Public so a label leading the row can be set level with the first line.
    [[nodiscard]] int lineHeight() const;

    // Appended to the form's sheet, so a theme change re-mixes it with everything else
    [[nodiscard]] static QString styleSheetFor(const ThemeTokens& tokens);
    // Not in the sheet: a stylesheet can only point at a picture on disk, and these are tinted at runtime
    void restyleGlyphs(const ThemeTokens& tokens);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] bool hasHeightForWidth() const override;
    [[nodiscard]] int heightForWidth(int width) const override;

signals:
    void itemsChanged();
    void duplicateRefused(const QString& name);
    // Before the field offers anything, so suggestions that take a while to gather are gathered only then
    void fieldOpening();

protected:
    void changeEvent(QEvent* pEvent) override;
    void resizeEvent(QResizeEvent* pEvent) override;
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    Chip* makeChip(const QString& name);
    void markOrigin(Chip* pChip) const;
    void remeasure();
    // Re-adds everything in reading order: FlowLayout cannot insert into the middle
    void rebuild();
    void openField(const int index);
    void closeField(const bool keepNote = false);
    void commitField(const bool stillTyping);
    void removeAt(const int index);
    void showNote(const QString& name);
    void hideNote();
    void makeCompleter();
    void refreshSuggestionModel();
    void measureSuggestions();
    void inkSuggestionNotes();
    [[nodiscard]] static QString cleaned(const QString& name);

    FlowLayout* mpFlow = nullptr;
    QList<Chip*> mChips;
    PlaceholderButton* mpAdd = nullptr;
    QLineEdit* mpField = nullptr;
    QLabel* mpNote = nullptr;
    QTimer* mpNoteTimer = nullptr;
    QCompleter* mpCompleter = nullptr;
    QStandardItemModel* mpSuggestionModel = nullptr;
    SuggestionDelegate* mpSuggestionDelegate = nullptr;
    QList<Suggestion> mSuggestions;
    QString mUnlistedOrigin;
    QString mChipAccessibleName;
    QString mChipRemoveTooltip;
    // Kept for a restyle that comes before there is a popup to put them on
    QString mSuggestionSheet;
    QColor mSuggestionNote;
    QColor mSuggestionNoteChosen;
    QIcon mRemoveGlyph;
    // -1 while the open field adds a name rather than renaming a chip
    int mEditingIndex = -1;
    bool mFieldOpen = false;
    // Losing focus commits and committing moves the focus, so this bars a re-entrant commit
    bool mCommitting = false;
};

} // namespace uiDesign

#endif // MUDLET_CHIPROW_H
