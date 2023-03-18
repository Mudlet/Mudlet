<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US" sourcelanguage="en">
<context>
    <name>GLWidget</name>
    <message numerus="yes">
        <location filename="../../src/glwidget.cpp" line="282"/>
        <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
        <translation>
            <numerusform>You have a map loaded (%n room), but Mudlet does not know where you are at the moment.</numerusform>
            <numerusform>You have a map loaded (%n rooms), but Mudlet does not know where you are at the moment.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>MapInfoContributorManager</name>
    <message numerus="yes">
        <location filename="../../src/mapInfoContributorManager.cpp" line="202"/>
        <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</source>
        <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
        <translation>
            <numerusform>{unused} Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</numerusform>
            <numerusform>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>T2DMap</name>
    <message numerus="yes">
        <location filename="../../src/T2DMap.cpp" line="1204"/>
        <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
        <translation>
            <numerusform>You have a map loaded (%n room), but Mudlet does not know where you are at the moment.</numerusform>
            <numerusform>You have a map loaded (%n rooms), but Mudlet does not know where you are at the moment.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>TRoomDB</name>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="731"/>
        <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translatorcomment>Perhaps should put the %n back in the source code</translatorcomment>
        <translation>
            <numerusform>[ INFO ]  - The missing %n area is now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
            <numerusform>[ INFO ]  - The missing %n areas are now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="715"/>
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that are supposed
 to be in this/these area(s)...</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translation>
            <numerusform>[ ALERT ] - %n area detected as missing in map: adding it in.
 Look for further messages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="722"/>
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translation>
            <numerusform>[ ALERT ] - %n area detected as missing in map: adding it in.
 Look for further messages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>TTrigger</name>
    <message numerus="yes">
        <location filename="../../src/TTrigger.cpp" line="1169"/>
        <source>Trigger name=%1 will fire %n more time(s).</source>
        <translation>
            <numerusform>Trigger name=%1 will fire %n more time.</numerusform>
            <numerusform>Trigger name=%1 will fire %n more times.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgPackageExporter</name>
    <message numerus="yes">
        <location filename="../../src/dlgPackageExporter.cpp" line="1459"/>
        <source>Select what to export (%n item(s))</source>
        <comment>This is the text shown at the top of a groupbox when there is %n (one or more) items to export in the Package exporter dialogue; the initial (and when there is no items selected) is a separate text.</comment>
        <translation>
            <numerusform>Select what to export (%n item)</numerusform>
            <numerusform>Select what to export (%n items)</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgPackageManager</name>
    <message numerus="yes">
        <location filename="../../src/dlgPackageManager.cpp" line="235"/>
        <source>Remove %n package(s)</source>
        <comment>Message on button in package manager to remove one or more (%n is the count of) selected package(s).</comment>
        <translation>
            <numerusform>Remove %n package</numerusform>
            <numerusform>Remove %n packages</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgRoomProperties</name>
    <message numerus="yes">
        <location filename="../../src/dlgRoomProperties.cpp" line="150"/>
        <source>Lock room(s), so it/they will never be used for speedwalking</source>
        <comment>This text will be shown at a checkbox, where you can set/unset a number of room&apos;s lock.</comment>
        <translation>
            <numerusform>Lock room, so it will never be used for speedwalking</numerusform>
            <numerusform>Lock rooms, so they will never be used for speedwalking</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/dlgRoomProperties.cpp" line="167"/>
        <source>Enter a new room weight to use as the travel time for all of the %n selected room(s). This will be used for calculating the best path. The minimum and default is 1.</source>
        <comment>%n is the total number of rooms involved.</comment>
        <translation>
            <numerusform>Enter a new room weight to use as the travel time for the %n selected room. This will be used for calculating the best path. The minimum and default is 1.</numerusform>
            <numerusform>Enter a new room weight to use as the travel time for all of the %n selected rooms. This will be used for calculating the best path. The minimum and default is 1.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/dlgRoomProperties.cpp" line="173"/>
        <source>To change the room weight for all of the %n selected room(s), please choose:
 • an existing room weight from the list below (sorted by most commonly used first)
 • enter a new positive integer value to use as a new weight. The default is 1.</source>
        <comment>This is for when applying a new room weight to one or more rooms and some have different weights at present. %n is the total number of rooms involved.</comment>
        <translation>
            <numerusform>To change the room weight for the %n selected room, please choose:
 • an existing room weight from the list below (sorted by most commonly used first)
 • enter a new positive integer value to use as a new weight. The default is 1.</numerusform>
            <numerusform>To change the room weight for all of the %n selected rooms, please choose:
 • an existing room weight from the list below (sorted by most commonly used first)
 • enter a new positive integer value to use as a new weight. The default is 1.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/dlgRoomProperties.cpp" line="195"/>
        <source>Type one or more graphemes (&quot;visible characters&quot;) to use as a symbol for all of the %n selected room(s), or enter a space to clear the symbol:</source>
        <comment>%n is the total number of rooms involved.</comment>
        <translation>
            <numerusform>Type one or more graphemes (&quot;visible characters&quot;) to use as a symbol for the %n selected room, or enter a space to clear the symbol:</numerusform>
            <numerusform>Type one or more graphemes (&quot;visible characters&quot;) to use as a symbol for all of the %n selected rooms, or enter a space to clear the symbol:</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/dlgRoomProperties.cpp" line="201"/>
        <source>To change the symbol for all of the %n selected room(s), please choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols</source>
        <comment>This is for when applying a new room symbol to one or more rooms and some have different symbols or no symbol at present. %n is the total number of rooms involved.</comment>
        <translation>
            <numerusform>To change the symbol for the %n selected rooms, please choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols</numerusform>
            <numerusform>To change the symbol for all of the %n selected rooms, please choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>mudlet</name>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3547"/>
        <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
        <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
        <translation>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;An update is now available!&lt;/i&gt;&lt;p&gt;</numerusform>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n updates are now available!&lt;/i&gt;&lt;p&gt;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3565"/>
        <source>Review %n update(s)...</source>
        <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
        <translatorcomment>Could do with the insertion of &quot;the&quot; as a second word!</translatorcomment>
        <translation>
            <numerusform>Review the update...</numerusform>
            <numerusform>Review the %n updates...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3570"/>
        <source>Review the update(s) available...</source>
        <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
        <translation>
            <numerusform>Review the update available...</numerusform>
            <numerusform>Review the updates available...</numerusform>
        </translation>
    </message>
</context>
</TS>
