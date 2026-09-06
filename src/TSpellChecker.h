#ifndef MUDLET_TSPELLCHECKER_H
#define MUDLET_TSPELLCHECKER_H

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

#include <QByteArray>
#include <QHash>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

// hunspell.h spells the handle "typedef struct Hunhandle Hunhandle;", so the
// tag can be declared here and the library header kept out of every file that
// only passes a handle along.
struct Hunhandle;
class Host;

// A profile's spell checking: the dictionary chosen from the system, the
// profile's own user dictionary, and the shared one all profiles can write to.
// One instance per Host, so a profile with no view can still be spell-checked
// (#8681) - which is why nothing here may reach for a widget.
class TSpellChecker
{
public:
    explicit TSpellChecker(Host* pHost);
    ~TSpellChecker();

    void setSystemDictionary(const QString&);
    // Reading a dictionary costs tens of milliseconds, so the handle is only
    // built when something asks for it. This is what has the event loop pay
    // that once the profile has loaded rather than in front of the first word
    // typed.
    void warmSystemDictionary();
    Hunhandle* systemHandle();
    // The user dictionaries are always UTF-8, but the one chosen from the
    // system's may not be.
    const QByteArray& systemCodecName();
    // The per-profile or the shared handle, or nullptr, depending on the two
    // options held by Host.
    Hunhandle* userHandle();
    QSet<QString> wordSet() const;
    QPair<bool, QString> addWord(const QString&);
    QPair<bool, QString> removeWord(const QString&);
    bool usingSharedDictionary() const;
    void applyUserDictionaryOptions();

    // The dictionary shared by every profile that opts into it, opened once and
    // saved and closed when the application goes down.
    static Hunhandle* sharedDictionary();
    static QSet<QString> sharedWordSet();
    static bool addWordToShared(const QString&);
    static bool removeWordFromShared(const QString&);
    static void closeSharedDictionary();

private:
    void loadSystemDictionary();
    // Both of these revise the contents of the .aff file and handle a .dic file
    // that has been updated externally/manually (to add or remove words) - the
    // first also puts the contents of the .dic file into the supplied second
    // argument before returning the handle to the dictionary loaded:
    static Hunhandle* prepareProfileDictionary(const QString&, QSet<QString>&);
    // This will save and replace the .dic file with just the words in the
    // supplied second argument and update the .aff file as appropriate. It is
    // to be used at the end of a session to store away the user's changes:
    static bool saveDictionary(const QString&, QSet<QString>&);
    static int getDictionaryWordCount(const QString& dictionaryPath);
    static bool overwriteAffixFile(const QString& affixPath, const QHash<QString, unsigned int>&);
    static bool overwriteDictionaryFile(const QString& dictionaryPath, const QStringList&);
    static bool scanDictionaryFile(const QString& dictionaryPath, int&, QHash<QString, unsigned int>&, QStringList&);
    static int scanWordList(QStringList&, QHash<QString, unsigned int>&);

    Host* mpHost = nullptr;

    // Names the dictionary mpHunspell_system is built for. The build is put off
    // until the load has finished, so the profile load never reads the whole
    // dictionary. Host's mSpellDic is the profile's setting; this is only ever
    // what has been requested from it.
    QString mSystemDictionary;

    // Three handles, one for the dictionary the user chooses from the system
    // ones, one for the shared one and the third for a per profile one - the
    // last pair are built by the user and/or lua functions:
    Hunhandle* mpHunspell_system = nullptr;
    Hunhandle* mpHunspell_shared = nullptr;
    Hunhandle* mpHunspell_profile = nullptr;
    QByteArray mHunspellCodecName_system;
    // To update the profile dictionary we actually have to track all the words
    // in it so we load the contents into this on startup and adjust it as we
    // go. Then, at the end of a session we put the revised contents back into
    // the user's ".dic" file and regenerate the needed pair of lines for the
    // ".aff" file - this member is for the per profile option only as the
    // shared one is held by the statics below:
    QSet<QString> mWordSet_profile;

    static Hunhandle* smpHunspell_sharedDictionary;
    // The collection of words in what smpHunspell_sharedDictionary points to:
    static QSet<QString> smWordSet_shared;
};

#endif // MUDLET_TSPELLCHECKER_H
