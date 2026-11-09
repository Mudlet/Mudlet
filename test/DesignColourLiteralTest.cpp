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

/*
 * Guards that a designed surface takes its colours from uiDesign::themeTokens()
 * and its font sizes from uiDesign::typeSize(): a written colour is wrong in the
 * other theme, and nothing shows it until somebody opens the window in it.
 *
 * Run with: ctest -R DesignColourLiteralTest -V
 */

#include <QtTest/QtTest>

#include <cctype>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>

#include <algorithm>

// QStringLiteral rather than qsl(): this test links no Mudlet code, so utils.h
// is not on its include path.

class DesignColourLiteralTest : public QObject
{
    Q_OBJECT

    struct Hit
    {
        QString file;
        int line = 0;
        QString text;
        QString reason;
    };

    static QString srcDir() { return QStringLiteral(MUDLET_SRC_DIR); }

    // Only adopted surfaces: an unadopted window still writes colours out. The
    // .ui files are listed because Designer writes colours no C++ rule would see.
    static QStringList scannedFiles()
    {
        QStringList files{QStringLiteral("uiDesign.cpp"),
                          QStringLiteral("uiDesign.h"),
                          QStringLiteral("dlgTriggerEditor.cpp"),
                          QStringLiteral("dlgTriggerEditor.h"),
                          QStringLiteral("dlgProfilePreferences.cpp"),
                          QStringLiteral("dlgProfilePreferences.h"),
                          QStringLiteral("EditorTreeDelegate.cpp"),
                          QStringLiteral("EditorTreeDelegate.h"),
                          QStringLiteral("EditorTreeRowDelegate.cpp"),
                          QStringLiteral("EditorTreeRowDelegate.h"),
                          QStringLiteral("EditorTreeRowMetrics.cpp"),
                          QStringLiteral("EditorTreeRowMetrics.h"),
                          QStringLiteral("VariableTreeDelegate.cpp"),
                          QStringLiteral("VariableTreeDelegate.h"),
                          QStringLiteral("SearchResultDelegate.cpp"),
                          QStringLiteral("SearchResultDelegate.h"),
                          QStringLiteral("SidebarItemDelegate.cpp"),
                          QStringLiteral("SidebarItemDelegate.h"),
                          QStringLiteral("TTabBar.cpp"),
                          QStringLiteral("TTabBar.h"),
                          QStringLiteral("GripSplitter.cpp"),
                          QStringLiteral("GripSplitter.h"),
                          QStringLiteral("ChipRow.cpp"),
                          QStringLiteral("ChipRow.h"),
                          QStringLiteral("FlowLayout.cpp"),
                          QStringLiteral("FlowLayout.h"),
                          QStringLiteral("PairGridLayout.cpp"),
                          QStringLiteral("PairGridLayout.h"),
                          QStringLiteral("AlphaColourWell.cpp"),
                          QStringLiteral("AlphaColourWell.h"),
                          QStringLiteral("SidebarToggle.cpp"),
                          QStringLiteral("SidebarToggle.h"),
                          QStringLiteral("ToolBarOverflow.cpp"),
                          QStringLiteral("ToolBarOverflow.h"),
                          QStringLiteral("EditorPlaceholderButton.cpp"),
                          QStringLiteral("EditorPlaceholderButton.h"),
                          QStringLiteral("EditorLineMarginDelegate.cpp"),
                          QStringLiteral("EditorLineMarginDelegate.h"),
                          QStringLiteral("dlgAboutDialog.cpp"),
                          QStringLiteral("dlgAboutDialog.h"),
                          QStringLiteral("AboutLinkButton.cpp"),
                          QStringLiteral("AboutLinkButton.h"),
                          QStringLiteral("AboutSupporterBanner.cpp"),
                          QStringLiteral("AboutSupporterBanner.h"),
                          QStringLiteral("dlgConnectionProfiles.cpp"),
                          QStringLiteral("dlgConnectionProfiles.h"),
                          QStringLiteral("dlgNotepad.cpp"),
                          QStringLiteral("dlgNotepad.h"),
                          QStringLiteral("dlgPackageManager.cpp"),
                          QStringLiteral("dlgPackageManager.h"),
                          QStringLiteral("PackageItemDelegate.cpp"),
                          QStringLiteral("PackageItemDelegate.h"),
                          QStringLiteral("dlgPackageExporter.cpp"),
                          QStringLiteral("dlgPackageExporter.h"),
                          QStringLiteral("ExporterTreeDelegate.cpp"),
                          QStringLiteral("ExporterTreeDelegate.h"),
                          QStringLiteral("dlgColorTrigger.cpp"),
                          QStringLiteral("dlgSystemMessageArea.cpp"),
                          QStringLiteral("TDetachedWindow.cpp"),
                          QStringLiteral("mudlet.cpp"),
                          QStringLiteral("ui/trigger_editor.ui"),
                          QStringLiteral("ui/triggers_main_area.ui"),
                          QStringLiteral("ui/trigger_pattern_edit.ui"),
                          QStringLiteral("ui/color_trigger.ui"),
                          QStringLiteral("ui/connection_profiles.ui"),
                          QStringLiteral("ui/system_message_area.ui"),
                          QStringLiteral("ui/aliases_main_area.ui"),
                          QStringLiteral("ui/timers_main_area.ui"),
                          QStringLiteral("ui/scripts_main_area.ui"),
                          QStringLiteral("ui/keybindings_main_area.ui"),
                          QStringLiteral("ui/actions_main_area.ui"),
                          QStringLiteral("ui/vars_main_area.ui"),
                          QStringLiteral("ui/notes_editor.ui"),
                          QStringLiteral("ui/dlgPackageExporter.ui"),
                          QStringLiteral("ui/about_dialog.ui")};
        // Taken by pattern: the settings dialog may not stay one .ui file
        const QDir uiDir(srcDir() + QStringLiteral("/ui"));
        for (const QString& page : uiDir.entryList({QStringLiteral("profile_preferences*.ui")}, QDir::Files, QDir::Name)) {
            files.append(QStringLiteral("ui/%1").arg(page));
        }
        return files;
    }

    // Trailing, it exempts its line; on a line of its own, the lines under it up
    // to the next blank one, so a block such as an ANSI table is marked once.
    static constexpr char scmMarker[] = "theme-fixed:";

    // Taken off QColor at run time: a hand-kept list misses names nobody thought
    // of. "transparent" is left out, as every sheet clearing a background says it.
    static const QSet<QString>& namedColours()
    {
        static const QSet<QString> names = [] {
            QSet<QString> gathered;
            for (const QString& name : QColor::colorNames()) {
                if (name.compare(QStringLiteral("transparent"), Qt::CaseInsensitive) != 0) {
                    gathered.insert(name.toLower());
                }
            }
            return gathered;
        }();
        return names;
    }

    // Longest first, so that "darkgreen" is read whole rather than as "green"
    static const QRegularExpression& cssColourName()
    {
        static const QRegularExpression pattern = [] {
            QStringList names(namedColours().cbegin(), namedColours().cend());
            std::sort(names.begin(), names.end(), [](const QString& one, const QString& other) {
                return one.size() != other.size() ? one.size() > other.size() : one < other;
            });
            return QRegularExpression(QStringLiteral("\\b(?:%1)\\b").arg(names.join(QLatin1Char('|'))), QRegularExpression::CaseInsensitiveOption);
        }();
        return pattern;
    }

    static QString qtColourNames() { return QStringLiteral("white|black|red|green|blue|cyan|magenta|yellow|gray|darkGray|lightGray|darkRed|darkGreen|darkBlue|darkCyan|darkMagenta|darkYellow"); }

    static const QRegularExpression& qtColourName()
    {
        static const QRegularExpression pattern(QStringLiteral("Qt::(%1)\\b").arg(qtColourNames()));
        return pattern;
    }

    // Any other run of digits after a "#" is an issue number, an object name or a CSS id
    static QString hexColourIn(const QString& text)
    {
        for (qsizetype start = text.indexOf(QLatin1Char('#')); start >= 0; start = text.indexOf(QLatin1Char('#'), start + 1)) {
            qsizetype end = start + 1;
            while (end < text.size() && std::isxdigit(static_cast<unsigned char>(text.at(end).toLatin1()))) {
                ++end;
            }
            const qsizetype digits = end - start - 1;
            if (digits == 3 || digits == 4 || digits == 6 || digits == 8) {
                return text.mid(start, end - start);
            }
        }
        return QString();
    }

    // A stylesheet value is never translated, so a tr() literal reading "Black"
    // names a thing rather than a colour; a declaration inside one still counts.
    struct Literal
    {
        QString text;
        bool translated = false;
    };

    static QList<Literal> stringLiteralsIn(const QString& line)
    {
        static const QRegularExpression translating(QStringLiteral("\\b(?:tr|translate|QT_TR_NOOP|QT_TRANSLATE_NOOP)\\s*\\(\\s*$"));

        QList<Literal> literals;
        bool open = false;
        bool translated = false;
        qsizetype from = 0;
        for (qsizetype at = 0; at < line.size(); ++at) {
            const QChar character = line.at(at);
            if (character == QLatin1Char('\\') && open) {
                ++at;
                continue;
            }
            if (character != QLatin1Char('"')) {
                continue;
            }
            if (open) {
                literals.append({line.mid(from, at - from), translated});
                open = false;
            } else {
                open = true;
                from = at + 1;
                translated = translating.match(line.left(at)).hasMatch();
            }
        }
        return literals;
    }

    // A digit has to follow the paren, so the template uiDesign::rgba() fills
    // in, "rgba(%1, %2, %3, %4)", is not caught
    static const QRegularExpression& channelsInString()
    {
        static const QRegularExpression pattern(QStringLiteral("\\brgba?\\s*\\(\\s*\\d"));
        return pattern;
    }

    // How a name reaches a sheet through .arg(), with the declaration elsewhere
    static bool wholeLiteralIsAColourName(const QString& literal) { return namedColours().contains(literal.trimmed().toLower()); }

    // A colour name inside a longer literal counts only in a colour or background
    // declaration: in a sentence "black" is as likely to be a word.
    static QString colourInStrings(const QList<Literal>& literals)
    {
        for (const Literal& literal : literals) {
            if (const QString hex = hexColourIn(literal.text); !hex.isEmpty()) {
                return QStringLiteral("the colour %1 written into a string").arg(hex);
            }
            if (channelsInString().match(literal.text).hasMatch()) {
                return QStringLiteral("a colour written channel by channel into a string");
            }
            if (!literal.translated && wholeLiteralIsAColourName(literal.text)) {
                return QStringLiteral("the colour name %1 written as a string of its own").arg(literal.text.trimmed());
            }
            if (!literal.text.contains(QStringLiteral("color:")) && !literal.text.contains(QStringLiteral("background"))) {
                continue;
            }
            if (const QRegularExpressionMatch match = cssColourName().match(literal.text); match.hasMatch()) {
                return QStringLiteral("the colour name %1 written into a stylesheet").arg(match.captured(0));
            }
        }
        return QString();
    }

    // A digit after the colon is a hand-picked size: the design's steps reach a
    // sheet from uiDesign::typeSize() through .arg(), as "font-size: %1pt".
    static const QRegularExpression& writtenFontSize()
    {
        static const QRegularExpression pattern(QStringLiteral("font-size\\s*:\\s*[0-9]"));
        return pattern;
    }

    // "%" not followed by a digit, so that the "%1" of a template is not read
    // as the tail of a percentage
    static const QRegularExpression& percentageFontSize()
    {
        static const QRegularExpression pattern(QStringLiteral("font-size\\s*:\\s*[^;}]*%(?![0-9])"));
        return pattern;
    }

    static QString sizeInStrings(const QList<Literal>& literals)
    {
        for (const Literal& literal : literals) {
            if (literal.translated) {
                continue;
            }
            if (percentageFontSize().match(literal.text).hasMatch()) {
                return QStringLiteral("a font-size in per cent, which Qt's stylesheet parser drops without applying it");
            }
            if (writtenFontSize().match(literal.text).hasMatch()) {
                return QStringLiteral("a font-size written as a number rather than taken from uiDesign::typeSize()");
            }
        }
        return QString();
    }

    // White and black pass only in blend() or fill(), as what a colour is mixed
    // towards or a mask cleared with, so that Qt::white as an ink is still caught.
    static QString exemptEndpoints(const QString& line)
    {
        QString probe = line;
        probe.replace(QStringLiteral("Qt::transparent"), QStringLiteral("themeFixedEndpoint"));
        if (probe.contains(QStringLiteral("blend(")) || probe.contains(QStringLiteral("fill("))) {
            probe.replace(QStringLiteral("Qt::white"), QStringLiteral("themeFixedEndpoint"));
            probe.replace(QStringLiteral("Qt::black"), QStringLiteral("themeFixedEndpoint"));
        }
        return probe;
    }

    static QString colourInCode(const QString& line)
    {
        const QString probe = exemptEndpoints(line);

        static const QRegularExpression fromQtName(QStringLiteral("QColor\\s*\\(\\s*Qt::"));
        if (fromQtName.match(probe).hasMatch()) {
            return QStringLiteral("a QColor built from a Qt colour name");
        }
        // Call and name are matched separately: they are routinely nested calls
        // apart, as in setForeground(0, QBrush(Qt::gray))
        static const QRegularExpression painter(QStringLiteral("\\b(?:QColor|QBrush|QPen|setPen|setColor|setForeground|setBackground)\\s*\\("));
        if (const QRegularExpressionMatch named = qtColourName().match(probe); named.hasMatch() && painter.match(probe).hasMatch()) {
            return QStringLiteral("Qt::%1 handed to something that paints with it").arg(named.captured(1));
        }
        // An assigned name reaches a painter on a later line the rule above never sees
        static const QRegularExpression assigned(QStringLiteral("(?<![=!<>])=\\s*Qt::(%1)\\b").arg(qtColourNames()));
        if (const QRegularExpressionMatch named = assigned.match(probe); named.hasMatch()) {
            return QStringLiteral("Qt::%1 assigned outright").arg(named.captured(1));
        }
        static const QRegularExpression fromString(QStringLiteral("QColor\\s*\\(\\s*(?:qsl|QStringLiteral|QLatin1String)?\\s*\\(?\\s*\""));
        if (fromString.match(probe).hasMatch()) {
            return QStringLiteral("a QColor built from a written colour");
        }
        // fromHslF() and fromHsvF() are deliberately not here: a semantic state hue
        // is made that way, with the lightness taken off the page it is drawn on
        static const QRegularExpression fromChannels(QStringLiteral("QColor::fromRgb\\s*\\(|QColor\\s*\\(\\s*\\d+\\s*,\\s*\\d+\\s*,\\s*\\d+"));
        if (fromChannels.match(probe).hasMatch()) {
            return QStringLiteral("a QColor built from written channel values");
        }
        // QColor(0xff8800): neither the "#" reading nor the channel rule sees it
        static const QRegularExpression fromPacked(QStringLiteral("QColor\\s*\\(\\s*(0x[0-9a-fA-F]{6,8})\\s*\\)"));
        if (const QRegularExpressionMatch packed = fromPacked.match(probe); packed.hasMatch()) {
            return QStringLiteral("a QColor built from the written %1").arg(packed.captured(1));
        }
        static const QRegularExpression constant(QStringLiteral("QColorConstants::(\\w+)"));
        if (const QRegularExpressionMatch named = constant.match(probe); named.hasMatch() && named.captured(1) != QStringLiteral("Transparent")) {
            return QStringLiteral("QColorConstants::%1").arg(named.captured(1));
        }
        return QString();
    }

    // A raw string literal cannot carry a comment inside it, so it is marked on
    // the line it opens on and one marker covers all of it.
    static int anchorOf(const QStringList& lines, const int line)
    {
        int anchor = line;
        bool insideRawString = false;
        for (int at = 1; at <= line; ++at) {
            const QString& text = lines.at(at - 1);
            if (insideRawString) {
                if (text.contains(QStringLiteral(")\""))) {
                    insideRawString = false;
                }
                continue;
            }
            const qsizetype opened = text.indexOf(QStringLiteral("R\"("));
            if (opened >= 0 && text.indexOf(QStringLiteral(")\""), opened + 3) < 0) {
                insideRawString = true;
                anchor = at;
            }
        }
        return insideRawString ? anchor : line;
    }

    static bool exempted(const QStringList& lines, const int line)
    {
        if (lines.at(line - 1).contains(QLatin1String(scmMarker))) {
            return true;
        }
        for (int at = anchorOf(lines, line); at >= 1; --at) {
            const QString& text = lines.at(at - 1);
            if (text.trimmed().isEmpty()) {
                return false;
            }
            if (text.contains(QLatin1String(scmMarker))) {
                return true;
            }
        }
        return false;
    }

    enum class Looking { Colours, Sizes };

    static void scanSource(const QString& file, const QStringList& lines, QList<Hit>& hits, const Looking looking)
    {
        bool insideRawString = false;
        for (int number = 1; number <= lines.size(); ++number) {
            const QString& raw = lines.at(number - 1);
            const bool wasInsideRawString = insideRawString;
            if (insideRawString) {
                insideRawString = !raw.contains(QStringLiteral(")\""));
            } else if (const qsizetype opened = raw.indexOf(QStringLiteral("R\"(")); opened >= 0 && raw.indexOf(QStringLiteral(")\""), opened + 3) < 0) {
                insideRawString = true;
            }

            // Inside a raw string every character is content, quotes included
            const QList<Literal> literals = wasInsideRawString ? QList<Literal>{{raw, false}} : stringLiteralsIn(raw);
            QString reason = looking == Looking::Sizes ? sizeInStrings(literals) : colourInStrings(literals);
            if (looking == Looking::Colours && reason.isEmpty() && !wasInsideRawString) {
                reason = colourInCode(raw);
            }
            if (reason.isEmpty() || exempted(lines, number)) {
                continue;
            }
            hits.append({file, number, raw.trimmed(), reason});
        }
    }

    static void scanDesignerFile(const QString& file, const QStringList& lines, QList<Hit>& hits, const Looking looking)
    {
        bool insideStyleSheet = false;
        for (int number = 1; number <= lines.size(); ++number) {
            const QString& raw = lines.at(number - 1);
            QString reason;
            // Designer writes a palette colour as channel elements under a <color>
            if (looking == Looking::Colours && raw.contains(QStringLiteral("<color"))) {
                for (qsizetype ahead = number; ahead < std::min<qsizetype>(number + 5, lines.size()); ++ahead) {
                    if (lines.at(ahead).contains(QStringLiteral("<red>"))) {
                        reason = QStringLiteral("a colour set in Designer");
                        break;
                    }
                }
            }
            if (raw.contains(QStringLiteral("name=\"styleSheet\""))) {
                insideStyleSheet = true;
            } else if (insideStyleSheet) {
                if (raw.contains(QStringLiteral("</property>"))) {
                    insideStyleSheet = false;
                } else if (reason.isEmpty()) {
                    reason = looking == Looking::Sizes ? sizeInStrings({{raw, false}}) : colourInStrings({{raw, false}});
                }
            }
            if (reason.isEmpty() || exempted(lines, number)) {
                continue;
            }
            hits.append({file, number, raw.trimmed(), reason});
        }
    }

    // Fails on an unreadable file: one renamed off the list is a surface that
    // stopped being checked.
    static bool gather(const Looking looking, QList<Hit>& hits, int& scannedLines, QString& failure)
    {
        for (const QString& file : scannedFiles()) {
            QFile source(QStringLiteral("%1/%2").arg(srcDir(), file));
            if (!source.open(QIODevice::ReadOnly | QIODevice::Text)) {
                failure = QStringLiteral("src/%1 is on the scan list and could not be read - it has been renamed or removed").arg(file);
                return false;
            }
            const QStringList lines = QString::fromUtf8(source.readAll()).split(QChar::LineFeed);
            scannedLines += lines.size();
            if (file.endsWith(QStringLiteral(".ui"))) {
                scanDesignerFile(QStringLiteral("src/%1").arg(file), lines, hits, looking);
            } else {
                scanSource(QStringLiteral("src/%1").arg(file), lines, hits, looking);
            }
        }
        return true;
    }

    // QtTest cuts a failure message off at a few hundred characters, so each
    // hit is printed on a line of its own
    static void report(const QList<Hit>& hits)
    {
        for (const Hit& hit : hits) {
            qWarning().noquote() << QStringLiteral("%1:%2: %3   [%4]").arg(hit.file, QString::number(hit.line), hit.text, hit.reason);
        }
    }

private slots:
    // One list rather than a failure per hit: stopping at the first would take a
    // run per colour to clear
    void test_everyColourOnADesignedSurfaceComesFromTheTokens()
    {
        const QStringList files = scannedFiles();
        QVERIFY2(files.size() > 20, "the list of scanned files is too short to be the design language's surfaces");

        QList<Hit> hits;
        int scannedLines = 0;
        QString failure;
        QVERIFY2(gather(Looking::Colours, hits, scannedLines, failure), qPrintable(failure));

        qInfo().noquote() << QStringLiteral("  %1 lines over %2 files").arg(QString::number(scannedLines), QString::number(files.size()));

        report(hits);
        QVERIFY2(hits.isEmpty(),
                 qPrintable(QStringLiteral("%1 colour(s) written out rather than taken from uiDesign::themeTokens() - listed above. Mix each from the tokens, or - if it is a value being shown "
                                           "rather than chrome - write \"// theme-fixed: <why>\" on the line.")
                                    .arg(QString::number(hits.size()))));
    }

    void test_everyFontSizeOnADesignedSurfaceComesFromTheScale()
    {
        QList<Hit> hits;
        int scannedLines = 0;
        QString failure;
        QVERIFY2(gather(Looking::Sizes, hits, scannedLines, failure), qPrintable(failure));

        report(hits);
        QVERIFY2(hits.isEmpty(),
                 qPrintable(QStringLiteral("%1 font-size(s) that are not a step of uiDesign::typeSize() - listed above. Qt's stylesheet parser reads pt and px and nothing else, so a percentage "
                                           "sets no size at all, and a written number is a size picked by hand rather than a step of the scale. Interpolate typeSize() into the rule as a pt "
                                           "value, or - if it is a picture of something outside this application - write \"// theme-fixed: <why>\" on the line.")
                                    .arg(QString::number(hits.size()))));
    }

    // The scans above pass on finding nothing, which a scanner that reads nothing does too
    void test_theScannerReportsAWrittenColour_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<bool>("sizes");
        QTest::newRow("hex in a string") << QStringLiteral(R"x(painter.setPen(QColor("#ff8800"));)x") << false;
        QTest::newRow("Qt name to a pen") << QStringLiteral("painter.setPen(Qt::red);") << false;
        QTest::newRow("Qt name assigned") << QStringLiteral("mInk = Qt::red;") << false;
        QTest::newRow("channels") << QStringLiteral("const QColor ink = QColor(12, 34, 56);") << false;
        QTest::newRow("packed") << QStringLiteral("const QColor ink = QColor(0xff8800);") << false;
        QTest::newRow("constant") << QStringLiteral("const QColor ink = QColorConstants::Red;") << false;
        QTest::newRow("size in a sheet") << QStringLiteral(R"x(sheet = QStringLiteral("QLabel { font-size: 12pt; }");)x") << true;
    }

    void test_theScannerReportsAWrittenColour()
    {
        QFETCH(QString, line);
        QFETCH(bool, sizes);
        QList<Hit> hits;
        scanSource(QStringLiteral("sample.cpp"), {line}, hits, sizes ? Looking::Sizes : Looking::Colours);
        QVERIFY2(!hits.isEmpty(), qPrintable(QStringLiteral("the scanner let \"%1\" through").arg(line)));
    }

    void test_theScannerReportsAColourSetInDesigner()
    {
        QList<Hit> hits;
        scanDesignerFile(QStringLiteral("sample.ui"),
                         {QStringLiteral("<property name=\"styleSheet\">"), QStringLiteral("<string>QLabel { color: #ff0000; }</string>"), QStringLiteral("</property>")},
                         hits,
                         Looking::Colours);
        QVERIFY2(!hits.isEmpty(), "the scanner let a colour in a Designer stylesheet through");
    }
};

QTEST_GUILESS_MAIN(DesignColourLiteralTest)

#include "DesignColourLiteralTest.moc"
