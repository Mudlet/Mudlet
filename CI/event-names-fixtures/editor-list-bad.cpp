// Script editor's list for bad.cpp and bad.lua: a line ending in "// BAD" must be
// reported. The names on BAD lines of bad.cpp are left out, as those lines are
// reported anyway. Never compiled.

QStringList systemEvents()
{
    static const QStringList names{qsl("sysFixtureFinished"),
                                   qsl("sysFixtureKeptEvent"),
                                   qsl("sysFixtureNotRaised"), // BAD
                                   qsl("sysFixtureTidied"),
                                   qsl("sysFixtureWalked")};
    return names;
}
