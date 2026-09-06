/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2022 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "TSpellChecker.h"

#include "Host.h"
#include "MudletApp.h"
#include "utils.h"

#include <QCollator>
#include <QDebug>
#include <QFile>
#include <QMultiMap>
#include <QSaveFile>
#include <QTextBoundaryFinder>
#include <QTextStream>
#include <QTimer>

#if defined(Q_OS_WINDOWS)
#include <QFileInfo>
#include <QRegularExpression>
#include <QScopedArrayPointer>
#include <windows.h>
#endif

#include <hunspell/hunspell.h>

#include <algorithm>

Hunhandle* TSpellChecker::smpHunspell_sharedDictionary = nullptr;
QSet<QString> TSpellChecker::smWordSet_shared;

#if defined(Q_OS_WINDOWS)
// credit to Qt Creator (https://github.com/qt-creator/qt-creator/blob/50d93a656789d6e776ecca4adc2e5b487bac0dbc/src/libs/utils/fileutils.cpp)
static QString getShortPathName(const QString& name)
{
    if (name.isEmpty()) {
        return name;
    }

    // Determine length, then convert.
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(name.utf16()); // MinGW
    const DWORD length = GetShortPathNameW(nameC, NULL, 0);
    if (length == 0) {
        return name;
    }
    QScopedArrayPointer<TCHAR> buffer(new TCHAR[length]);
    GetShortPathNameW(nameC, buffer.data(), length);
    const QString rc = QString::fromWCharArray(buffer.data(), length - 1);

    return rc;
}

// 'strip' non-ASCII characters from the path by copying it to a location without them
// this is only an issue for the Win32 API; macOS and Linux don't have such issues
static void sanitizeUtf8Path(QString& originalLocation, const QString& fileName)
{
    static auto findNonAscii = QRegularExpression(qsl("([^ -~])"));

    auto nonAscii = findNonAscii.match(originalLocation);
    if (!nonAscii.hasMatch()) {
        return;
    }

    const auto shortPath = getShortPathName(originalLocation);
    // short path name might not always work: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getshortpathnamew#remarks
    if (shortPath != originalLocation) {
        originalLocation = shortPath;
        return;
    }

    const QString pureANSIpath = qsl("C:\\Windows\\Temp\\mudlet_%1").arg(fileName);
    if (!QFileInfo::exists(pureANSIpath)) {
        if (!QFile::copy(originalLocation, pureANSIpath)) {
            qWarning() << "sanitizeUtf8Path() ERROR: couldn't copy" << originalLocation << "to location without ASCII characters";
        } else {
            originalLocation = pureANSIpath;
        }
    }
}
#endif

TSpellChecker::TSpellChecker(Host* pHost)
: mpHost(pHost)
{
}

TSpellChecker::~TSpellChecker()
{
    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
        mpHunspell_system = nullptr;
    }
    if (mpHunspell_profile) {
        Hunspell_destroy(mpHunspell_profile);
        mpHunspell_profile = nullptr;
        // Need to commit any changes to personal dictionary
        qDebug() << "TSpellChecker::~TSpellChecker() INFO - Saving profile's own Hunspell dictionary...";
        saveDictionary(MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("profile")), mWordSet_profile);
    }
}

void TSpellChecker::setSystemDictionary(const QString& newDict)
{
    if (newDict.isEmpty() || mSystemDictionary == newDict) {
        return;
    }

    mSystemDictionary = newDict;

    if (mpHunspell_system) {
        Hunspell_destroy(mpHunspell_system);
        mpHunspell_system = nullptr;
        mHunspellCodecName_system.clear();
    }

    // A dictionary picked in the preferences leaves the handle cold, and only
    // another profile being opened would emit signal_profileLoaded to warm it
    // again - so read the new one here instead of in front of the next word
    // typed. During a profile load the handle is warmed once at the end, after
    // the profile's own choice of dictionary has been read from its XML.
    if (!mpHost->mIsProfileLoadingSequence) {
        QTimer::singleShot(0, mpHost, [this]() {
            warmDictionaries();
        });
    }
}

void TSpellChecker::warmDictionaries()
{
    // The Lua spellCheckWord() and spellSuggestWord() do not consult this flag,
    // so the lazy getter still serves a script in a profile that has spell
    // check off:
    if (mpHost->mEnableSpellCheck) {
        systemHandle();
    }

    // Deduplicating profile.dic and regenerating its .aff is startup work, so
    // it is done here rather than left for whatever first asks for the handle.
    applyUserDictionaryOptions();
}

Hunhandle* TSpellChecker::systemHandle()
{
    if (!mpHunspell_system) {
        if (mSystemDictionary.isEmpty()) {
            // A profile whose XML names no dictionary never calls
            // Host::setSpellDic(), so the platform's starting one has to be
            // picked up here:
            mSystemDictionary = mpHost->getSpellDic();
        }
        if (!mSystemDictionary.isEmpty()) {
            loadSystemDictionary();
        }
    }
    return mpHunspell_system;
}

const QByteArray& TSpellChecker::systemCodecName()
{
    systemHandle();
    return mHunspellCodecName_system;
}

void TSpellChecker::loadSystemDictionary()
{
    // Everywhere but macOS getMudletPath() probes for "<name>.aff" to settle
    // which directory wins, so it has to get the same name the files are then
    // loaded by.
    const QString path = MudletApp::getMudletPath(enums::hunspellDictionaryPath, mSystemDictionary);
    QString spell_aff = qsl("%1%2.aff").arg(path, mSystemDictionary);
    QString spell_dic = qsl("%1%2.dic").arg(path, mSystemDictionary);

#if defined(Q_OS_WINDOWS)
    // strip non-ASCII characters from the path because hunspell can't handle them
    // when compiled with MinGW 7.3.0
    sanitizeUtf8Path(spell_aff, qsl("%1.aff").arg(mSystemDictionary));
    sanitizeUtf8Path(spell_dic, qsl("%1.dic").arg(mSystemDictionary));
#endif

    mpHunspell_system = Hunspell_create(spell_aff.toUtf8().constData(), spell_dic.toUtf8().constData());
    if (mpHunspell_system) {
        mHunspellCodecName_system = QByteArray(Hunspell_get_dic_encoding(mpHunspell_system));
        qDebug().noquote().nospace() << "TSpellChecker::loadSystemDictionary() INFO - System Hunspell dictionary \"" << mSystemDictionary << "\" loaded for profile, it uses a \""
                                     << Hunspell_get_dic_encoding(mpHunspell_system) << "\" encoding...";
    }
}

// NOTE: Host::setUserDictionaryOptions() forces the user dictionary on, so the
// first branch here is unreachable today.
void TSpellChecker::applyUserDictionaryOptions()
{
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    if (!enableUserDictionary) {
        if (mpHunspell_profile) {
            Hunspell_destroy(mpHunspell_profile);
            mpHunspell_profile = nullptr;
            // Need to commit any changes to personal dictionary
            qDebug() << "TSpellChecker::applyUserDictionaryOptions() INFO - Saving profile's own Hunspell dictionary...";
            saveDictionary(MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("profile")), mWordSet_profile);
        }
        return;
    }

    if (useSharedDictionary) {
        // Called for the opening it does, the handle is fetched again when one
        // is wanted:
        sharedDictionary();
        return;
    }

    profileHandle();
}

Hunhandle* TSpellChecker::profileHandle()
{
    if (!mpHunspell_profile) {
        qDebug() << "TSpellChecker::profileHandle() INFO - Preparing profile's own Hunspell dictionary...";
        mpHunspell_profile = prepareProfileDictionary(mpHost->getName(), mWordSet_profile);
    }
    return mpHunspell_profile;
}

Hunhandle* TSpellChecker::userHandle()
{
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    if (!enableUserDictionary) {
        return nullptr;
    }

    if (useSharedDictionary) {
        // Read back rather than cached, so a shared dictionary closed on the
        // way out of the application cannot be handed out again as a stale
        // pointer - sharedDictionary() re-opens a closed one from disk:
        return sharedDictionary();
    }

    return profileHandle();
}

bool TSpellChecker::usingSharedDictionary() const
{
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    return useSharedDictionary;
}

QSet<QString> TSpellChecker::wordSet()
{
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    if (!enableUserDictionary) {
        return QSet<QString>();
    }

    // Asking for the handle is what reads the dictionary file in, so without
    // this a profile that has not spell-checked anything yet reports that it
    // knows no words at all:
    userHandle();

    if (!useSharedDictionary) {
        return mWordSet_profile;
    }
    return sharedWordSet();
}

QPair<bool, QString> TSpellChecker::addWord(const QString& word)
{
    const QString errMsg = qsl("the word \"%1\" already seems to be in the user dictionary");
    QPair<bool, QString> result{};
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    if (!enableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (useSharedDictionary) {
        if (!sharedDictionary()) {
            return qMakePair(false, qsl("the shared dictionary could not be opened, so \"%1\" was not added").arg(word));
        }
        if (addWordToShared(word)) {
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }
        return result;
    }

    auto* handle = userHandle();
    if (!handle) {
        return qMakePair(false, qsl("the profile dictionary could not be opened, so \"%1\" was not added").arg(word));
    }

    // The return value from this function is unclear - it does not seem to
    // indicate anything useful
    Hunspell_add(handle, word.toUtf8().constData());
    if (!mWordSet_profile.contains(word)) {
        mWordSet_profile.insert(word);
        qDebug().noquote().nospace() << "TSpellChecker::addWord(\"" << word << "\") INFO - word added to profile mWordSet.";
        result.first = true;
    } else {
        result.second = errMsg.arg(word);
    }

    return result;
}

QPair<bool, QString> TSpellChecker::removeWord(const QString& word)
{
    const QString errMsg = qsl("the word \"%1\" does not seem to be in the user dictionary");
    QPair<bool, QString> result{};
    bool enableUserDictionary = false;
    bool useSharedDictionary = false;
    mpHost->getUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    if (!enableUserDictionary) {
        return qMakePair(false, QLatin1String("a user dictionary is not enable for this profile"));
    }

    if (useSharedDictionary) {
        if (!sharedDictionary()) {
            return qMakePair(false, qsl("the shared dictionary could not be opened, so \"%1\" was not removed").arg(word));
        }
        if (removeWordFromShared(word)) {
            result.first = true;
        } else {
            result.second = errMsg.arg(word);
        }
        return result;
    }

    auto* handle = userHandle();
    if (!handle) {
        return qMakePair(false, qsl("the profile dictionary could not be opened, so \"%1\" was not removed").arg(word));
    }

    // The return value from this function is unclear - it does not seem to
    // indicate anything useful
    Hunspell_remove(handle, word.toUtf8().constData());
    if (mWordSet_profile.remove(word)) {
        qDebug().noquote().nospace() << "TSpellChecker::removeWord(\"" << word << "\") INFO - word removed from profile mWordSet.";
        result.first = true;
    } else {
        result.second = errMsg.arg(word);
    }

    return result;
}

// This will load up the shared spelling dictionary for profiles that want it
// - and handles the absence of files for the first run from an older Mudlet
// version - it processes any changes made by the user in the ".dic" file and
// regenerates (deduplicates and sorts) it and (rebuilds the "TRY" line) in
// the ".aff" file:
/*static*/ Hunhandle* TSpellChecker::sharedDictionary()
{
    if (smpHunspell_sharedDictionary) {
        return smpHunspell_sharedDictionary;
    }

    QString dictionaryPath(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("mudlet.dic")));
    QString affixPath(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("mudlet.aff")));
    int oldWordCount = 0;
    QStringList wordList;
    QHash<QString, unsigned int> graphemeCounts;

    if (!scanDictionaryFile(dictionaryPath, oldWordCount, graphemeCounts, wordList)) {
        return nullptr;
    }

    if (!overwriteDictionaryFile(dictionaryPath, wordList)) {
        return nullptr;
    }

    const int wordCount = wordList.count();
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Considered an extra " << wordCount - oldWordCount << " words.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Considered " << oldWordCount - wordCount << " fewer words.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words in dictionary.";
    }

    if (!overwriteAffixFile(affixPath, graphemeCounts)) {
        return nullptr;
    }

    smWordSet_shared = QSet<QString>(wordList.begin(), wordList.end());

#if defined(Q_OS_WINDOWS)
    sanitizeUtf8Path(dictionaryPath, qsl("mudlet.dic"));
    sanitizeUtf8Path(affixPath, qsl("mudlet.aff"));
#endif
    smpHunspell_sharedDictionary = Hunspell_create(affixPath.toUtf8().constData(), dictionaryPath.toUtf8().constData());
    return smpHunspell_sharedDictionary;
}

/*static*/ void TSpellChecker::closeSharedDictionary()
{
    if (!smpHunspell_sharedDictionary) {
        return;
    }

    saveDictionary(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("mudlet")), smWordSet_shared);
    Hunspell_destroy(smpHunspell_sharedDictionary);
    smpHunspell_sharedDictionary = nullptr;
    smWordSet_shared.clear();
}

/*static*/ bool TSpellChecker::addWordToShared(const QString& word)
{
    auto* handle = sharedDictionary();
    bool isAdded = false;
    Hunspell_add(handle, word.toUtf8().constData());
    if (!smWordSet_shared.contains(word)) {
        smWordSet_shared.insert(word);
        qDebug().noquote().nospace() << "TSpellChecker::addWordToShared(\"" << word << "\") INFO - word added to shared mWordSet.";
        isAdded = true;
    }
    return isAdded;
}

/*static*/ bool TSpellChecker::removeWordFromShared(const QString& word)
{
    auto* handle = sharedDictionary();
    bool isRemoved = false;
    Hunspell_remove(handle, word.toUtf8().constData());
    if (smWordSet_shared.remove(word)) {
        qDebug().noquote().nospace() << "TSpellChecker::removeWordFromShared(\"" << word << "\") INFO - word removed from shared mWordSet.";
        isRemoved = true;
    }
    return isRemoved;
}

/*static*/ QSet<QString> TSpellChecker::sharedWordSet()
{
    QSet<QString> wordSet;
    wordSet = smWordSet_shared;
    // Ensure we make a deep copy of it so the caller is not affected by
    // other profiles' edits.
    wordSet.detach();
    return wordSet;
}

// This will load up the spelling dictionary for the profile - and handles the
// absence of files for the first run in a new profile or from an older
// Mudlet version - it processes any changes made by the user in the ".dic" file
// and regenerates (deduplicates and sorts) it and rebuilds (the "TRY" line in)
// the ".aff" file:
/*static*/ Hunhandle* TSpellChecker::prepareProfileDictionary(const QString& hostName, QSet<QString>& wordSet)
{
    QString dictionaryPath(MudletApp::getMudletPath(enums::profileDataItemPath, hostName, qsl("profile.dic")));
    QString affixPath(MudletApp::getMudletPath(enums::profileDataItemPath, hostName, qsl("profile.aff")));

    int oldWordCount = 0;
    QStringList wordList;
    QHash<QString, unsigned int> graphemeCounts;

    if (!scanDictionaryFile(dictionaryPath, oldWordCount, graphemeCounts, wordList)) {
        return nullptr;
    }

    if (!overwriteDictionaryFile(dictionaryPath, wordList)) {
        return nullptr;
    }

    const int wordCount = wordList.count();
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Considered an extra " << wordCount - oldWordCount << " words.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Considered " << oldWordCount - wordCount << " fewer words.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words in dictionary.";
    }

    if (!overwriteAffixFile(affixPath, graphemeCounts)) {
        return nullptr;
    }

    // The affix file carries only the SET line, which forces the Hunspell API to
    // UTF-8, and the TRY line, which lets it search for completions: real affix
    // rules would confuse the add/remove code, and the language the game uses -
    // and so the locale whose affixes would suit - is not knowable here. The
    // word list is kept because Hunspell cannot give it back.

    wordSet = QSet<QString>(wordList.begin(), wordList.end());

#if defined(Q_OS_WINDOWS)
    sanitizeUtf8Path(dictionaryPath, qsl("profile.dic"));
    sanitizeUtf8Path(affixPath, qsl("profile.aff"));
#endif
    return Hunspell_create(affixPath.toUtf8().constData(), dictionaryPath.toUtf8().constData());
}

// This commits any changes noted in the wordSet into the ".dic" file and
// regenerates the ".aff" file.
/*static*/ bool TSpellChecker::saveDictionary(const QString& pathFileBaseName, QSet<QString>& wordSet)
{
    const QString dictionaryPath(qsl("%1.dic").arg(pathFileBaseName));
    const QString affixPath(qsl("%1.aff").arg(pathFileBaseName));
    QHash<QString, unsigned int> graphemeCounts;

    // The file will have previously been created, so it being missing now is
    // not expected and aborts the save:
    const int oldWordCount = getDictionaryWordCount(dictionaryPath);
    if (oldWordCount == -1) {
        return false;
    }

    QStringList wordList{wordSet.begin(), wordSet.end()};

    // This also sorts wordList as a wanted side-effect:
    const int wordCount = scanWordList(wordList, graphemeCounts);
    if (wordCount > oldWordCount) {
        qDebug().nospace().noquote() << "  Saved an extra " << wordCount - oldWordCount << " words in dictionary.";
    } else if (wordCount < oldWordCount) {
        qDebug().nospace().noquote() << "  Saved " << oldWordCount - wordCount << " fewer words in dictionary.";
    } else {
        qDebug().nospace().noquote() << "  No change in the number of words saved in dictionary.";
    }

    if (!overwriteDictionaryFile(dictionaryPath, wordList)) {
        return false;
    }

    if (!overwriteAffixFile(affixPath, graphemeCounts)) {
        return false;
    }

    return true;
}

// Returns false on significant failure (where the caller will have to bail out)
/*static*/ bool TSpellChecker::scanDictionaryFile(const QString& dictionaryPath, int& oldWC, QHash<QString, unsigned int>& gc, QStringList& wl)
{
    QFile dict(dictionaryPath);
    if (!dict.exists()) {
        return true;
    }

    if (!dict.open(QFile::ReadOnly | QFile::Text)) {
        qWarning().nospace().noquote() << "TSpellChecker::scanDictionaryFile(...) ERROR - failed to open dictionary file (for reading): \"" << dict.fileName() << "\" reason: " << dict.errorString();
        return false;
    }

    QTextStream ds(&dict);
    QString dictionaryLine;
    ds.readLineInto(&dictionaryLine);

    bool isOk = false;
    oldWC = dictionaryLine.toInt(&isOk);
    do {
        ds.readLineInto(&dictionaryLine);
        if (!dictionaryLine.isEmpty()) {
            wl << dictionaryLine;
            QTextBoundaryFinder graphemeFinder(QTextBoundaryFinder::Grapheme, dictionaryLine);
            int startPos = 0;
            int endPos = graphemeFinder.toNextBoundary();
            do {
                if (endPos > 0) {
                    const QString grapheme(dictionaryLine.mid(startPos, endPos - startPos));
                    if (gc.contains(grapheme)) {
                        ++gc[grapheme];
                    } else {
                        gc[grapheme] = 1;
                    }
                    startPos = endPos;
                    endPos = graphemeFinder.toNextBoundary();
                }
            } while (endPos > 0);
        }
    } while (!ds.atEnd() && ds.status() == QTextStream::Ok);

    if (ds.status() != QTextStream::Ok) {
        qWarning().nospace().noquote() << "TSpellChecker::scanDictionaryFile(\"" << dict.fileName() << "\") ERROR - failed to completely read dictionary file, status: " << ds.status();
        return false;
    }

    dict.close();

    qDebug().nospace().noquote() << "Loaded custom dictionary \"" << dict.fileName() << "\" with " << wl.count() << " words.";
    if (oldWC != wl.count()) {
        qDebug().nospace().noquote() << "Previously, there were " << oldWC << " words recorded instead.";
    }
    if (wl.count() > 1) {
        // This will use the system default locale - it might be better to use
        // the Mudlet one...
        QCollator sorter;
        sorter.setCaseSensitivity(Qt::CaseSensitive);
        std::sort(wl.begin(), wl.end(), sorter);
        const int dupCount = wl.removeDuplicates();
        if (dupCount) {
            qDebug().nospace().noquote() << "  Removed " << dupCount << " duplicates.";
        }
    }

    return true;
}

// Returns false on significant failure (where the caller will have to bail out)
/*static*/ bool TSpellChecker::overwriteDictionaryFile(const QString& dictionaryPath, const QStringList& wl)
{
    QSaveFile dict(dictionaryPath);
    if (!dict.open(QFile::WriteOnly | QFile::Text)) {
        qWarning().nospace().noquote() << "TSpellChecker::overwriteDictionaryFile(...) ERROR - failed to open dictionary file (for writing): \"" << dict.fileName()
                                       << "\" reason: " << dict.errorString();
        return false;
    }

    QTextStream ds(&dict);
    ds << qMax(0, wl.count());
    if (!wl.isEmpty()) {
        ds << QChar(QChar::LineFeed);
        ds << wl.join(QChar::LineFeed).toUtf8();
    }
    ds.flush();
    dict.commit();
    if (dict.error() != QFile::NoError) {
        qWarning().nospace().noquote() << "TSpellChecker::overwriteDictionaryFile(...) ERROR - failed to completely write dictionary file: \"" << dict.fileName()
                                       << "\" status: " << dict.errorString();
        return false;
    }

    return true;
}

// Returns -1 on significant failure (where the caller will have to bail out)
/*static*/ int TSpellChecker::getDictionaryWordCount(const QString& dictionaryPath)
{
    QFile dict(dictionaryPath);
    if (!dict.open(QFile::ReadOnly | QFile::Text)) {
        qWarning().nospace().noquote() << "TSpellChecker::getDictionaryWordCount(...) ERROR - failed to open dictionary file (for reading): \"" << dict.fileName()
                                       << "\" reason: " << dict.errorString();
        return -1;
    }

    QTextStream ds(&dict);
    QString dictionaryLine;
    ds.readLineInto(&dictionaryLine);
    bool isOk = false;
    const int oldWordCount = dictionaryLine.toInt(&isOk);
    dict.close();
    if (isOk) {
        return oldWordCount;
    }

    return -1;
}

// Returns false on significant failure (where the caller will have to bail out)
/*static*/ bool TSpellChecker::overwriteAffixFile(const QString& affixPath, const QHash<QString, unsigned int>& gc)
{
    QMultiMap<unsigned int, QString> sortedGraphemeCounts;
    // Sort the graphemes into a descending order list:
    if (!gc.isEmpty()) {
        QHashIterator<QString, unsigned int> itGraphemeCount(gc);
        while (itGraphemeCount.hasNext()) {
            itGraphemeCount.next();
            sortedGraphemeCounts.insert(itGraphemeCount.value(), itGraphemeCount.key());
        }
    }

    // Generate TRY line:
    QString tryLine = qsl("TRY ");
    QMultiMapIterator<unsigned int, QString> itGrapheme(sortedGraphemeCounts);
    itGrapheme.toBack();
    while (itGrapheme.hasPrevious()) {
        itGrapheme.previous();
        tryLine.append(itGrapheme.value());
    }

    QStringList affixLines;
    affixLines << qsl("SET UTF-8");
    affixLines << tryLine;

    QSaveFile aff(affixPath);
    if (!aff.open(QFile::WriteOnly | QFile::Text)) {
        qWarning().nospace().noquote() << "TSpellChecker::overwriteAffixFile(...) ERROR - failed to open affix file (for writing): \"" << aff.fileName() << "\" reason: " << aff.errorString();
        return false;
    }

    QTextStream as(&aff);
    as << affixLines.join(QChar::LineFeed).toUtf8();
    as << QChar(QChar::LineFeed);
    as.flush();
    if (!aff.commit()) {
        qWarning().nospace().noquote() << "TSpellChecker::overwriteAffixFile(...) ERROR - failed to commit affix file: \"" << aff.fileName() << "\" reason: " << aff.errorString();
        return false;
    }

    return true;
}

/*static*/ int TSpellChecker::scanWordList(QStringList& wl, QHash<QString, unsigned int>& gc)
{
    const int wordCount = wl.count();
    if (wordCount > 1) {
        // This will use the system default locale - it might be better to use
        // the Mudlet one...
        QCollator sorter;
        sorter.setCaseSensitivity(Qt::CaseSensitive);
        std::sort(wl.begin(), wl.end(), sorter);
    }

    for (const auto& word : wl) {
        QTextBoundaryFinder graphemeFinder(QTextBoundaryFinder::Grapheme, word);
        int startPos = 0;
        int endPos = graphemeFinder.toNextBoundary();
        do {
            if (endPos > 0) {
                const QString grapheme(word.mid(startPos, endPos - startPos));
                if (gc.contains(grapheme)) {
                    ++gc[grapheme];
                } else {
                    gc[grapheme] = 1;
                }
                startPos = endPos;
                endPos = graphemeFinder.toNextBoundary();
            }
        } while (endPos > 0);
    }

    return wordCount;
}
