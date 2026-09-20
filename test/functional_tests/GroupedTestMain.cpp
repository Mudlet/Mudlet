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

// main() for every grouped functional test binary - see GroupedTest.h for the
// shape and why it exists.

#include "GroupedTest.h"

#include <QLatin1Char>
#include <QMap>
#include <QString>
#include <QStringList>

#include <cstdio>

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

namespace {

// Function-local, so it is built on first use: the registrars are file-scope
// objects in other translation units, and nothing orders their construction
// against a file-scope container here.
QMap<QString, GroupedTest::EntryFunction>& registry()
{
    static QMap<QString, GroupedTest::EntryFunction> cases;
    return cases;
}

// A Qt Resource Collection in a static library only reaches the executable if
// something on the executable's side names it, and a binary that does not runs
// quietly on without its resources, bundled fonts included. Doing it here means
// a grouped test cannot forget to. src/main.cpp registers them in this same
// spot, before the QApplication exists.
void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

} // namespace

void GroupedTest::registerCase(const char* name, EntryFunction entry)
{
    registry().insert(QString::fromUtf8(name), entry);
}

int main(int argc, char* argv[])
{
    initializeQRCResources();

    // Reporting the names and then exiting non-zero rather than succeeding: an
    // add_test() that forgot to pass one would otherwise be a green ctest case
    // that ran no test at all.
    if (argc < 2 || !registry().contains(QString::fromUtf8(argv[1]))) {
        fprintf(stderr,
                "%s runs one test class per invocation, named as its first argument.\n"
                "usage: %s <test class> [Qt Test arguments]\n"
                "this binary holds: %s\n",
                argv[0],
                argv[0],
                qUtf8Printable(QStringList(registry().keys()).join(QLatin1Char(' '))));
        return 2;
    }

    // qExec skips argv[0] and reads every later argument that is not an option
    // as a test function to run, so the class name has to take argv[0]'s place
    // rather than stay in front of the arguments meant for it.
    return registry().value(QString::fromUtf8(argv[1]))(argc - 1, argv + 1);
}
