// Fixture for CI/test-check-event-names.lua: CI/check-event-names.lua must find
// nothing to report here. Never compiled.

inline const auto csFixtureSettledEvent{qsl("sysFixtureSettled")};

void raisesGoodNames(Host* pHost, bool viewOnly, const QString& scriptChosenName)
{
    TEvent loaded{};
    loaded.mArgumentList.append(QLatin1String("sysFixtureLoaded"));
    loaded.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    // Arguments are not names, camelCase or not they are never read as one here
    loaded.mArgumentList.append(viewOnly ? qsl("viewing") : qsl("editing"));
    loaded.mArgumentList << qsl("main") << qsl("GMCP");
    loaded.mArgumentList.append(qsl("playerName"));

    TEvent streamed{};
    streamed.mArgumentList << qsl("sysFixtureMoving") << qsl("roomName") << QString::number(2);

    // A name chosen in a branch: a later append is read for a sys name or a legacy one
    TEvent chosen{};
    if (viewOnly) {
        chosen.mArgumentList.append(qsl("sysFixtureViewed"));
    } else {
        chosen.mArgumentList.append(qsl("fixtureChosenEvent"));
    }

    TEvent irregular{};
    irregular.mArgumentList.append(qsl("sysFixtureSet"));
    TEvent irregularToo{};
    irregularToo.mArgumentList.append(qsl("sysFixtureDone"));

    TEvent fromConstant{};
    fromConstant.mArgumentList << csFixtureSettledEvent;

    raiseProtocolEvent("sysFixtureProtocolEnabled", "GMCP");
    raiseSpeechEventOn(pHost, qsl("sysFixtureHandedOver"), pHost->getName());

    // A setting key, not an event name
    raiseSettingChangedEvent(qsl("fixtureSettingKey"));

    // Protocol events keep their protocol's names
    TEvent protocol{};
    protocol.mArgumentList.append(qsl("channel102Message"));

    // A name a script chose is not Mudlet's to name
    TEvent scripted{};
    scripted.mArgumentList.append(scriptChosenName);

    // Names in comments are not raised: sysFixtureCommentEvent
    /* nor in block comments: mArgumentList.append(qsl("sysFixtureBlockEvent")); */

    // On the legacy list, so it passes
    TEvent kept{};
    kept.mArgumentList.append(qsl("sysFixtureKeptEvent"));
}
