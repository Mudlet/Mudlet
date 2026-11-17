// Script editor's list for good.cpp and good.lua. Never compiled.

QStringList systemEvents()
{
    static const QStringList names{qsl("channel102Message"),
                                   qsl("sysFixtureDone"),
                                   qsl("sysFixtureHandedOver"),
                                   qsl("sysFixtureKeptEvent"),
                                   qsl("sysFixtureLoaded"),
                                   qsl("sysFixtureMoving"),
                                   qsl("sysFixtureProtocolEnabled"),
                                   qsl("sysFixtureSaving"),
                                   qsl("sysFixtureSet"),
                                   qsl("sysFixtureSettled"),
                                   qsl("sysFixtureSwept")};
    return names;
}
