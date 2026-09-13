#ifndef MUDLET_GROUPEDTEST_H
#define MUDLET_GROUPEDTEST_H

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

#include <QtTest/QtTest>

#include "utils.h"

// Every functional test executable statically links mudlet_core, which costs a
// build tree ~250MB and a link step each. Tests that can share one binary use
// MUDLET_GROUPED_TEST_MAIN() in place of QTEST_MAIN(): instead of defining
// main() it hands GroupedTestMain.cpp's main() a function that runs this one
// class, and that main() runs exactly the class named on the command line.
//
// So a grouped ctest case is still its own process, with its own QApplication
// and untouched static state - only the link is shared. Qt has the same idea
// behind QTEST_BATCH_TESTS, but the official Qt binaries are built with
// QT_FEATURE_batch_test_support off, so the registry has to be ours.
namespace GroupedTest {

using EntryFunction = int (*)(int, char**);

void registerCase(const char* name, EntryFunction entry);

// Registration has to have happened by the time main() runs, so it rides on the
// constructor of the file-scope object the macro below declares.
struct Registrar
{
    Registrar(const char* name, EntryFunction entry) { registerCase(name, entry); }
};

} // namespace GroupedTest

// The body is QTEST_MAIN's, less its Qt-selftest coverage hook, so a grouped run
// sets up what QTEST_MAIN sets up in the same order. QTEST_MAIN_SETUP() and
// QTEST_SET_MAIN_SOURCE_PATH come from qtest.h; the first picks QApplication
// over QCoreApplication from the Qt libraries this translation unit sees and the
// second reads __FILE__, so both have to expand in the test's own file rather
// than in main().
//
// The application name is the one thing sharing a binary would otherwise change:
// QStandardPaths::AppConfigLocation is built from it, and Mudlet keeps profile
// encryption keys and passwords under there, so leaving it at the executable's
// name would give a whole group one store instead of a test each.
#define MUDLET_GROUPED_TEST_MAIN(TestObject)                                                                                                                                                           \
    static int run##TestObject(int argc, char* argv[])                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<TestObject>();                                                                                                                             \
        QTEST_MAIN_SETUP()                                                                                                                                                                             \
        app.setApplicationName(qsl(#TestObject));                                                                                                                                                      \
        TestObject tc;                                                                                                                                                                                 \
        QTEST_SET_MAIN_SOURCE_PATH                                                                                                                                                                     \
        return QTest::qExec(&tc, argc, argv);                                                                                                                                                          \
    }                                                                                                                                                                                                  \
    static const GroupedTest::Registrar registrar##TestObject(#TestObject, &run##TestObject);

#endif // MUDLET_GROUPEDTEST_H
