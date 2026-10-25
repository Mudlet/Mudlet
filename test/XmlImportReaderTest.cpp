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

// XmlImportReader has to answer every call XMLimport makes exactly as QXmlStreamReader does,
// whether it read the document from a pugixml tree or handed it to QXmlStreamReader. Each
// input is walked twice with the same calls - once forced through QXmlStreamReader - and the
// two transcripts of everything the reader said must match.

#include "GroupedTest.h"
#include "XmlImportCorpus.h"
#include "XmlImportReader.h"

#include <QBuffer>
#include <QRandomGenerator>
#include <QtTest/QtTest>

namespace {

const QStringList scmContainers{qsl("MudletPackage"),
                                qsl("HostPackage"),
                                qsl("Host"),
                                qsl("TriggerPackage"),
                                qsl("TimerPackage"),
                                qsl("AliasPackage"),
                                qsl("ActionPackage"),
                                qsl("ScriptPackage"),
                                qsl("KeyPackage"),
                                qsl("VariablePackage"),
                                qsl("HelpPackage"),
                                qsl("TriggerGroup"),
                                qsl("Trigger"),
                                qsl("TimerGroup"),
                                qsl("Timer"),
                                qsl("AliasGroup"),
                                qsl("Alias"),
                                qsl("ActionGroup"),
                                qsl("Action"),
                                qsl("ScriptGroup"),
                                qsl("Script"),
                                qsl("KeyGroup"),
                                qsl("Key"),
                                qsl("regexCodeList"),
                                qsl("regexCodePropertyList"),
                                qsl("eventHandlerList"),
                                qsl("mInstalledPackages"),
                                qsl("HiddenVariables"),
                                qsl("Variable"),
                                qsl("VariableGroup"),
                                qsl("mInstalledModules"),
                                qsl("stopwatches"),
                                qsl("map"),
                                qsl("areas"),
                                qsl("rooms"),
                                qsl("room"),
                                qsl("environments"),
                                qsl("features")};

enum class Step { Descend, Text, SkipChildren, IncludeChildren, SkipElement };

// Salt 0 walks like XMLimport does; the others vary the calls with the element's name and depth
Step stepFor(QStringView name, int depth, quint32 salt)
{
    if (salt == 0) {
        return scmContainers.contains(name) ? Step::Descend : Step::Text;
    }
    quint32 hash = 2166136261u ^ salt ^ static_cast<quint32>(depth * 31);
    for (const QChar c : name) {
        hash = (hash ^ c.unicode()) * 16777619u;
    }
    switch (hash % 8) {
    case 4:
        return Step::Text;
    case 5:
        return Step::SkipChildren;
    case 6:
        return Step::IncludeChildren;
    case 7:
        return Step::SkipElement;
    default:
        return Step::Descend;
    }
}

// Control characters spelled out, so a report shows where a carriage return went
QString visible(QStringView text)
{
    QString result;
    for (const QChar c : text) {
        if (c.unicode() < 0x20 || c.unicode() == 0x7F || c.unicode() == 0xFEFF) {
            result += qsl("\\x%1").arg(static_cast<uint>(c.unicode()), 2, 16, QChar(u'0'));
        } else {
            result += c;
        }
    }
    return result;
}

QString describe(const XmlImportReader& reader)
{
    QString line = QString::number(reader.tokenType());
    if (reader.isStartElement() || reader.isEndElement()) {
        line += u' ' + reader.name().toString();
    }
    if (reader.isCharacters()) {
        line += qsl(" chars[%1] cdata=%2").arg(visible(reader.text())).arg(reader.isCDATA());
    }
    if (reader.tokenType() == QXmlStreamReader::Comment) {
        line += qsl(" comment[%1]").arg(visible(reader.text()));
    }
    if (reader.isStartElement()) {
        const auto attributes = reader.attributes();
        const auto all = attributes.all();
        for (const auto& [name, value] : all) {
            line += qsl(" %1=[%2]").arg(name, visible(value));
        }
        const QString first = all.isEmpty() ? qsl("isActive") : all.constFirst().first;
        line += qsl(" has(%1)=%2 null=%3 empty=%4 missing=%5/%6 isEmpty=%7")
                        .arg(first)
                        .arg(attributes.hasAttribute(first))
                        .arg(attributes.value(first).isNull())
                        .arg(attributes.value(first).isEmpty())
                        .arg(attributes.hasAttribute(qsl("noSuchAttribute")))
                        .arg(attributes.value(QLatin1StringView("noSuchAttribute")).isNull())
                        .arg(attributes.isEmpty());
        line += qsl(" @%1:%2:%3").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.characterOffset());
    }
    line += qsl(" end=%1 error=%2[%3]").arg(reader.atEnd()).arg(reader.error()).arg(reader.errorString());
    if (reader.error() == QXmlStreamReader::UnexpectedElementError) {
        line += qsl(" @%1:%2:%3").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.characterOffset());
    }
    return line;
}

QString textResult(const QString& text)
{
    return qsl("  text%1[%2]").arg(text.isNull() ? qsl("(null)") : QString(), visible(text));
}

QStringList walk(const QByteArray& xml, bool throughStreamReader, quint32 salt, bool* readFromTree = nullptr)
{
    XmlImportReader::setAlwaysUseStreamReaderForTesting(throughStreamReader);
    QBuffer buffer;
    buffer.setData(xml);
    buffer.open(QIODevice::ReadOnly);
    XmlImportReader reader;
    reader.setDevice(&buffer);

    QStringList transcript;
    int depth = 0;
    const qsizetype stepLimit = 2 * xml.size() + 100;
    for (qsizetype step = 0; step < stepLimit && !reader.atEnd(); ++step) {
        reader.readNext();
        transcript << describe(reader);
        if (reader.isEndElement()) {
            --depth;
        } else if (reader.isStartElement()) {
            switch (stepFor(reader.name().toString(), depth, salt)) {
            case Step::Descend:
                ++depth;
                break;
            case Step::Text:
                transcript << textResult(reader.readElementText());
                transcript << describe(reader);
                break;
            case Step::SkipChildren:
                transcript << textResult(reader.readElementText(QXmlStreamReader::SkipChildElements));
                transcript << describe(reader);
                break;
            case Step::IncludeChildren:
                transcript << textResult(reader.readElementText(QXmlStreamReader::IncludeChildElements));
                transcript << describe(reader);
                break;
            case Step::SkipElement:
                reader.skipCurrentElement();
                transcript << describe(reader);
                break;
            }
        }
    }
    // Past the end, where QXmlStreamReader raises an error of its own
    reader.readNext();
    transcript << describe(reader);
    if (readFromTree) {
        *readFromTree = reader.readingParsedTree();
    }
    XmlImportReader::setAlwaysUseStreamReaderForTesting(false);
    return transcript;
}

// Empty when they match, else where they first part
QString firstDifference(const QStringList& tree, const QStringList& stream)
{
    const qsizetype common = std::min(tree.size(), stream.size());
    for (qsizetype i = 0; i <= common; ++i) {
        if (i == common && tree.size() == stream.size()) {
            return {};
        }
        if (i == common || tree.at(i) != stream.at(i)) {
            QString report = qsl("line %1 of %2/%3:\n").arg(i).arg(tree.size()).arg(stream.size());
            for (qsizetype j = std::max<qsizetype>(0, i - 3); j < i; ++j) {
                report += qsl("    both:   %1\n").arg(tree.at(j));
            }
            report += qsl("    tree:   %1\n").arg(i < tree.size() ? tree.at(i) : qsl("<none>"));
            report += qsl("    stream: %1").arg(i < stream.size() ? stream.at(i) : qsl("<none>"));
            return report;
        }
    }
    return {};
}

const QList<QByteArray> scmSnippets{"&amp;",
                                    "&foo;",
                                    "&#0;",
                                    "&#x41;",
                                    "&#X41;",
                                    "&#13;",
                                    "&#xD800;",
                                    "&#xFFFE;",
                                    "&#128;",
                                    "&#x1F600;",
                                    "]]>",
                                    "<![CDATA[x]]>",
                                    "<![CDATA[]]>",
                                    "<!--c-->",
                                    "<!--a--b-->",
                                    "<!---->",
                                    "<?pi x?>",
                                    "<a/>",
                                    "</a>",
                                    "<b x=\"1\" x=\"2\"/>",
                                    "<b x=\"1\"y=\"2\"/>",
                                    "<b x='1'/>",
                                    "<p:b/>",
                                    " xmlns=\"u\"",
                                    "<b>t<c/>u</b>",
                                    "\xEF\xBB\xBF",
                                    "\r\n",
                                    "\r",
                                    "\t",
                                    "\xC2\x85",
                                    "\xEF\xBF\xBE",
                                    "\xF0\x9F\x8E\x89",
                                    "\xED\xA0\x80",
                                    "\xC3\xA9",
                                    "<!DOCTYPE x>",
                                    "<?xml version=\"1.0\"?>",
                                    "<name>q</name>",
                                    "<name/>",
                                    "<script> </script>",
                                    "a < b",
                                    "a > b",
                                    "x=\"y\"",
                                    " ",
                                    "\x01",
                                    "\x7F",
                                    "\xFF"};

const QByteArray scmBytes("<>&\"']-/=; \t\r\n\x01\x7f\x80\xc3\xff#x:?!");

// One change somewhere in the document, described well enough to make it again
QByteArray mutate(const QByteArray& original, QRandomGenerator& random, QString& description)
{
    QByteArray xml = original;
    const qsizetype at = random.bounded(static_cast<int>(xml.size() + 1));
    switch (random.bounded(4)) {
    case 0:
        xml.truncate(at);
        description = qsl("truncated at %1").arg(at);
        break;
    case 1:
        if (at < xml.size()) {
            const char replacement = scmBytes.at(random.bounded(static_cast<int>(scmBytes.size())));
            xml[at] = replacement;
            description = qsl("byte %1 replaced by 0x%2").arg(at).arg(static_cast<uchar>(replacement), 2, 16, QChar(u'0'));
            break;
        }
        [[fallthrough]];
    case 2: {
        const QByteArray& snippet = scmSnippets.at(random.bounded(static_cast<int>(scmSnippets.size())));
        xml.insert(at, snippet);
        description = qsl("\"%1\" inserted at %2").arg(QString::fromLatin1(snippet.toPercentEncoding()), QString::number(at));
        break;
    }
    default: {
        const qsizetype length = std::min<qsizetype>(random.bounded(64) + 1, xml.size() - at);
        xml.remove(at, length);
        description = qsl("%1 bytes removed at %2").arg(length).arg(at);
    }
    }
    return xml;
}

} // namespace

class XmlImportReaderTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mUnpackDir;
    QList<XmlImportCorpus::Entry> mCorpus;

    void compareBothWays(const QByteArray& xml, const QString& what, int* fromTree = nullptr)
    {
        for (const quint32 salt : {0u, 1u, 2u}) {
            bool readFromTree = false;
            const QStringList tree = walk(xml, false, salt, &readFromTree);
            const QStringList stream = walk(xml, true, salt);
            const QString difference = firstDifference(tree, stream);
            QVERIFY2(difference.isEmpty(), qPrintable(qsl("%1 (salt %2) read differently from the tree, at %3").arg(what, QString::number(salt), difference)));
            if (fromTree && readFromTree && salt == 0) {
                ++*fromTree;
            }
        }
    }

private slots:
    void initTestCase()
    {
        QVERIFY(mUnpackDir.isValid());
        mCorpus = XmlImportCorpus::load(qsl(MUDLET_SRC_DIR), mUnpackDir);
        QVERIFY2(mCorpus.size() > 40, qPrintable(qsl("only %1 corpus files were found under %2").arg(mCorpus.size()).arg(qsl(MUDLET_SRC_DIR))));
    }

    // Every file in the tree reads the same - and every well-formed one is read from pugixml's
    // tree, so the comparison is not of QXmlStreamReader with itself
    void test_theCorpusReadsTheSameFromTheTree()
    {
        for (const auto& entry : std::as_const(mCorpus)) {
            compareBothWays(entry.bytes, entry.name);
            if (QTest::currentTestFailed()) {
                return;
            }
            QXmlStreamReader plain(entry.bytes);
            while (!plain.atEnd()) {
                plain.readNext();
            }
            bool readFromTree = false;
            walk(entry.bytes, false, 0, &readFromTree);
            QVERIFY2(readFromTree == !plain.hasError(),
                     qPrintable(qsl("%1 %2 the tree, and QXmlStreamReader says \"%3\"").arg(entry.name, readFromTree ? qsl("was read from") : qsl("was not read from"), plain.errorString())));
        }
    }

    void test_edgeCasesReadTheSame_data()
    {
        QTest::addColumn<QByteArray>("xml");
        QTest::addColumn<bool>("fromTree");
        const QByteArray header("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n");
        QTest::newRow("plain") << header + "<a x=\"1\" y='2'><b>t&amp;u&lt;&gt;&quot;&apos;</b><c/>\n</a>\n" << true;
        QTest::newRow("no declaration") << QByteArray("<a><b>x</b></a>") << true;
        QTest::newRow("bom") << QByteArray("\xEF\xBB\xBF<a>\n\xC3\xA9\xF0\x9F\x8E\x89<b/></a>") << true;
        QTest::newRow("crlf") << QByteArray("<a>\r\n<b>1\r\n2\r3</b>\r\n<c x=\"1\r\n2\tz\"/>\r\n</a>") << true;
        QTest::newRow("character references") << header + "<a b=\"&#10;&#9;&#13;&#x41;\">&#13;&#10;&#x1F600;</a>" << true;
        QTest::newRow("whitespace text") << header + "<a>  <b>   </b>\t<c>\n</c></a>" << true;
        QTest::newRow("cdata") << header + "<a> <![CDATA[x<&>]]> <![CDATA[]]><![CDATA[\r\nb]]></a>" << true;
        QTest::newRow("comments") << QByteArray("<!--p--><a>p<!--c-->q<!----></a><!--e-->\n") << true;
        QTest::newRow("element inside text") << header + "<a><name>x<b/>y</name><c/></a>" << true;
        QTest::newRow("empty and missing attributes") << header + "<a e=\"\" f=''><b/></a>" << true;
        QTest::newRow("self-closing root") << QByteArray("<a/>") << true;
        QTest::newRow("deep") << QByteArray("<a><b><c><d><e>x</e></d></c></b></a>") << true;
        QTest::newRow("undeclared entity") << header + "<a>x&foo;y</a>" << false;
        QTest::newRow("duplicate attribute") << header + "<a x=\"1\" x=\"2\"/>" << false;
        QTest::newRow("control character") << header + QByteArray("<a>q\x01q</a>") << false;
        QTest::newRow("control reference") << header + "<a>&#1;</a>" << false;
        QTest::newRow("cdata end in text") << header + "<a>x]]>y</a>" << false;
        QTest::newRow("two roots") << header + "<a/><b/>" << false;
        QTest::newRow("trailing text") << header + "<a>x</a>tail" << false;
        QTest::newRow("text before root") << header + "junk<a/>" << false;
        QTest::newRow("namespace prefix") << header + "<p:a/>" << false;
        QTest::newRow("namespace declaration") << header + "<a xmlns=\"u\"/>" << false;
        QTest::newRow("less-than in attribute") << header + "<a b=\"<\"/>" << false;
        QTest::newRow("bare ampersand") << header + "<a>a & b</a>" << false;
        QTest::newRow("truncated text") << header + "<a><b>hello wor" << false;
        QTest::newRow("truncated tag") << header + "<a><b>x</b><Alias isActive=\"y" << false;
        QTest::newRow("invalid utf-8") << header + QByteArray("<a>q\xff q</a>") << false;
        QTest::newRow("xml 1.1") << QByteArray("<?xml version=\"1.1\"?><a/>") << false;
        QTest::newRow("latin-1") << QByteArray("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a>\xe9</a>") << false;
        QTest::newRow("double dash in comment") << header + "<a><!-- x -- y --></a>" << false;
        QTest::newRow("processing instruction") << header + "<a>x<?pi y?>z</a>" << false;
        QTest::newRow("internal subset") << QByteArray("<?xml version=\"1.0\"?>\n<!DOCTYPE a [<!ENTITY foo \"bar\">]>\n<a>&foo;</a>") << false;
        QTest::newRow("attributes run together") << header + "<a b=\"1\"c=\"2\"/>" << false;
        QTest::newRow("mismatched tags") << header + "<a></b>" << false;
        QTest::newRow("empty") << QByteArray() << false;
        QTest::newRow("plain text") << QByteArray("hello") << false;
        QTest::newRow("c1 control") << QByteArray("<a>\xC2\x85</a>") << false;
        QTest::newRow("noncharacter") << QByteArray("<a>\xEF\xBF\xBE</a>") << false;
    }

    void test_edgeCasesReadTheSame()
    {
        QFETCH(QByteArray, xml);
        QFETCH(bool, fromTree);
        compareBothWays(xml, qsl("the case"));
        bool readFromTree = false;
        walk(xml, false, 0, &readFromTree);
        QCOMPARE(readFromTree, fromTree);
    }

    // What breaks a file the way a crash or a hand edit does: cut off, a byte changed, a
    // snippet put in or a span taken out, from a seed that replays it
    void test_damagedFilesReadTheSame()
    {
        bool seedGiven = false;
        const quint32 firstSeed = qEnvironmentVariableIntValue("MUDLET_XML_IMPORT_FUZZ_SEED", &seedGiven);
        const int mutantsPerFile = seedGiven ? 200 : 25;
        int fromTree = 0;
        int mutants = 0;
        int wellFormed = 0;
        quint32 fileSeed = seedGiven ? firstSeed : 12144u;
        for (const auto& entry : std::as_const(mCorpus)) {
            ++fileSeed;
            if (entry.bytes.size() > 600 * 1024) {
                continue;
            }
            QRandomGenerator random(fileSeed);
            for (int i = 0; i < mutantsPerFile; ++i) {
                QString description;
                const QByteArray xml = mutate(entry.bytes, random, description);
                ++mutants;
                compareBothWays(xml, qsl("%1 with %2 (seed %3, mutant %4)").arg(entry.name, description, QString::number(fileSeed), QString::number(i)), &fromTree);
                if (QTest::currentTestFailed()) {
                    return;
                }
                QXmlStreamReader plain(xml);
                while (!plain.atEnd()) {
                    plain.readNext();
                }
                wellFormed += !plain.hasError();
            }
        }
        qInfo().nospace() << mutants << " damaged files, " << wellFormed << " of them still well-formed, " << fromTree << " read from the tree";
        // Many changes land in a script's text and leave the file well-formed, which is the part that tests the tree
        QVERIFY2(fromTree * 6 > mutants, qPrintable(qsl("only %1 of %2 damaged files were read from the tree").arg(fromTree).arg(mutants)));
    }
};

MUDLET_GROUPED_TEST_GUILESS_MAIN(XmlImportReaderTest)

#include "XmlImportReaderTest.moc"
