// Fixture for CI/test-check-event-names.lua: every line ending in "// BAD" must
// be reported by CI/check-event-names.lua, and no other line in this file.
// Never compiled.

inline const auto csFixtureMeltdownEvent{qsl("sysFixtureMeltdownEvent")};

void raisesBadNames(Host* pHost, bool good)
{
    TEvent noPrefix{};
    noPrefix.mArgumentList.append(QLatin1String("fixtureStarted")); // BAD
    noPrefix.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    TEvent eventSuffix{};
    eventSuffix.mArgumentList.append(qsl("sysFixtureLoadEvent")); // BAD

    TEvent notAVerb{};
    notAVerb.mArgumentList << QLatin1String("sysFixtureMessage") << QString::number(1); // BAD

    TEvent toggled{};
    toggled.mArgumentList.append(qsl("sysFixtureToggled")); // BAD

    TEvent triggered{};
    triggered.mArgumentList.append(qsl("sysFixtureTriggered")); // BAD

    // The name is chosen in a branch, so each branch's append is read
    TEvent branched{};
    if (good) {
        branched.mArgumentList << QLatin1String("sysFixtureFinished");
    } else {
        branched.mArgumentList << QLatin1String("sysFixtureFailure"); // BAD
    }

    TEvent ternary{};
    ternary.mArgumentList.append(good ? qsl("sysFixtureOpened") : qsl("sysFixtureCloseEvent")); // BAD

    TEvent fromConstant{};
    fromConstant.mArgumentList << csFixtureMeltdownEvent; // BAD

    raiseProtocolEvent("sysFixtureProtocolChange", "GMCP"); // BAD

    // A particle at the end: the word before it has to be the verb form
    TEvent phrasal{};
    phrasal.mArgumentList.append(qsl("sysFixtureLookUp")); // BAD

    // On the legacy list, so it passes
    TEvent kept{};
    kept.mArgumentList.append(qsl("sysFixtureKeptEvent"));

    // Follows the standard, and is on the legacy list anyway
    TEvent tidy{};
    tidy.mArgumentList.append(qsl("sysFixtureTidied"));

    // Follows the standard, but the script editor's list does not offer it
    TEvent unlisted{};
    unlisted.mArgumentList.append(qsl("sysFixtureUnlisted")); // BAD
}
