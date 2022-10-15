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
        <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
        <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
        <translation type="vanished">
            <numerusform>{Unused} Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
            <numerusform>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mapInfoContributorManager.cpp" line="200"/>
        <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</source>
        <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
        <translation>
            <numerusform>{Unused} Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</numerusform>
            <numerusform>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>T2DMap</name>
    <message numerus="yes">
        <location filename="../../src/T2DMap.cpp" line="1202"/>
        <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
        <translation>
            <numerusform>You have a map loaded (1 room), but Mudlet does not know where you are at the moment.</numerusform>
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
            <numerusform>[ INFO ]  - The missing area is now called:
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
            <numerusform>[ ALERT ] - an area detected as missing in map: adding it in.
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
            <numerusform>[ ALERT ] - an area detected as missing in map: adding it in.
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
        <source>Trigger name=%1 will fire %n more time(s).
</source>
        <translation type="vanished">
            <numerusform>Trigger name=%1 will fire 1 more time.
</numerusform>
            <numerusform>Trigger name=%1 will fire %n more times.
</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TTrigger.cpp" line="1169"/>
        <source>Trigger name=%1 will fire %n more time(s).</source>
        <translation>
            <numerusform>Trigger name=%1 will fire 1 more time.</numerusform>
            <numerusform>Trigger name=%1 will fire %n more times.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgPackageExporter</name>
    <message numerus="yes">
        <location filename="../../src/dlgPackageExporter.cpp" line="1455"/>
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
        <location filename="../../src/dlgPackageManager.cpp" line="240"/>
        <source>Remove %n package(s)</source>
        <comment>Message on button in package manager to remove one or more (%n is the count of) selected package(s).</comment>
        <translation>
            <numerusform>Remove %n package</numerusform>
            <numerusform>Remove %n packages</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgRoomSymbol</name>
    <message numerus="yes">
        <location filename="../../src/dlgRoomSymbol.cpp" line="80"/>
        <source>The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</source>
        <comment>This is for when applying a new room symbol to one or more rooms and some have the SAME symbol (others may have none) at present, %n is the total number of rooms involved and is at least two. Use line feeds to format text into a reasonable rectangle.</comment>
        <translation>
            <numerusform>(Unused) The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</numerusform>
            <numerusform>The only used symbol is &quot;%1&quot; in one or
more of the selected %n rooms, delete
this to clear it from those rooms or replace
with a new symbol to use for all the rooms:</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/dlgRoomSymbol.cpp" line="100"/>
        <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
        <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
        <translation>
            <numerusform>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for the selected room:</numerusform>
            <numerusform>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected rooms:</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>mudlet</name>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3674"/>
        <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
        <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
        <translation>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;An update is now available!&lt;/i&gt;&lt;p&gt;</numerusform>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n updates are now available!&lt;/i&gt;&lt;p&gt;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3692"/>
        <source>Review %n update(s)...</source>
        <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
        <translatorcomment>Could do with the insertion of &quot;the&quot; as a second word!</translatorcomment>
        <translation>
            <numerusform>Review the update...</numerusform>
            <numerusform>Review the %n updates...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3697"/>
        <source>Review the update(s) available...</source>
        <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
        <translation>
            <numerusform>Review the update available...</numerusform>
            <numerusform>Review the updates available...</numerusform>
        </translation>
    </message>
</context>
</TS>
