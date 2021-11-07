<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US" sourcelanguage="en">
<context>
    <name>GLWidget</name>
    <message numerus="yes">
        <location filename="../../src/glwidget.cpp" line="290"/>
        <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
        <translation>
            <numerusform>You have a map loaded (one room), but Mudlet does not know where you are at the moment.</numerusform>
            <numerusform>You have a map loaded (%n rooms), but Mudlet does not know where you are at the moment.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>MapInfoContributorManager</name>
    <message numerus="yes">
        <location filename="../../src/mapInfoContributorManager.cpp" line="188"/>
        <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
        <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
        <translation type="unfinished">
            <numerusform>{Unused} Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
            <numerusform>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>T2DMap</name>
    <message numerus="yes">
        <location filename="../../src/T2DMap.cpp" line="1252"/>
        <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
        <translation>
            <numerusform>You have a map loaded with single room, but Mudlet does not know where you are at the moment.</numerusform>
            <numerusform>You have a map loaded (%n rooms), but Mudlet does not know where you are at the moment.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
        <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
        <translatorcomment>The singular case is never used in English as %n is always at least 2.</translatorcomment>
        <translation type="vanished">
            <numerusform>{Unused} Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
            <numerusform>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>Enter the symbol to use
for this/these %n room(s):</source>
        <comment>this is for when applying a new room symbol to one or more rooms and none have a symbol at present; use line feeds to format text into a reasonable rectangle, %n is the number of rooms involved</comment>
        <translatorcomment>No need to use %n in the singular case - even though Linguist doesn&apos;t like it.</translatorcomment>
        <translation type="vanished">
            <numerusform>Enter the symbol to use
for this room:</numerusform>
            <numerusform>Enter the symbol to use
for these %n rooms:</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</source>
        <comment>This is for when applying a new room symbol to one or more rooms and some have the SAME symbol (others may have none) at present, %n is the total number of rooms involved and is at least two. Use line feeds to format text into a reasonable rectangle.</comment>
        <translatorcomment>The singular case is never used in English as %n is always at least 2.</translatorcomment>
        <translation type="vanished">
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
        <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
        <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
        <translation type="vanished">
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
    <name>TRoomDB</name>
    <message numerus="yes">
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that are supposed
 to be in this/these area(s)...</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translatorcomment>The singular case does not need to include the %n count parameter.</translatorcomment>
        <translation type="vanished">
            <numerusform>[ ALERT ] - an area was detected as missing in map: adding it in.
 Look for further messsages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messsages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translatorcomment>The source text should assume multiple rooms in the first line an area with just one room is not that likely - in this case then the source text will be a duplicate of another one in the same file.</translatorcomment>
        <translation type="vanished">
            <numerusform>[ ALERT ] - an area was detected as missing in map: adding it in.
 Look for further messsages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messsages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
        <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
        <translatorcomment>Perhaps should put the %n back in the source code</translatorcomment>
        <translation type="vanished">
            <numerusform>[ INFO ]  - The missing area is now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
            <numerusform>[ INFO ]  - The missing %n areas are now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="724"/>
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that are supposed
 to be in this/these area(s)...</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n area detected as missing in map: adding it in.
 Look for further messages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="730"/>
        <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n area detected as missing in map: adding it in.
 Look for further messages related to the rooms that are supposed
 to be in this area...</numerusform>
            <numerusform>[ ALERT ] - %n areas detected as missing in map: adding them in.
 Look for further messages related to the rooms that are supposed
 to be in these areas...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="738"/>
        <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
        <comment>The last line is a headline explaining what follows is a list of lines, one for each area, in the form of the ID number followed by the new name of the area</comment>
        <translation type="unfinished">
            <numerusform>[ INFO ]  - The missing area is now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
            <numerusform>[ INFO ]  - The missing %n areas are now called:
(ID) ==&gt; &quot;name&quot;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="773"/>
        <source>[ ALERT ] - %n bad area id(s) found in map! (less than +1 and not the reserved -1)
Now working out what new id numbers to use...</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n bad area id found in map! (less than +1 and not the reserved -1)
Now working out what new id number to use...</numerusform>
            <numerusform>[ ALERT ] - %n bad area ids found in map! (less than +1 and not the reserved -1)
Now working out what new id numbers to use...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="777"/>
        <source>[ ALERT ] - %n bad area id(s) found in map! (less than +1 and not the reserved -1)  Look for further messages related to this for each affected area ...</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n bad area id found in map! (less than +1 and not the reserved -1)  Look for further messages related to this for each affected area ...</numerusform>
            <numerusform>[ ALERT ] - %n bad area ids found in map! (less than +1 and not the reserved -1)  Look for further messages related to this for each affected area ...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="851"/>
        <source>[ ALERT ] - %n bad room id(s) found in map! (less than +1)
Now working out what new id numbers to use.</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n bad room id found in map! (less than +1)
Now working out what new id numbers to use.</numerusform>
            <numerusform>[ ALERT ] - %n bad room ids found in map! (less than +1)
Now working out what new id numbers to use.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="855"/>
        <source>[ ALERT ] - %n bad room id(s) found in map! (less than +1)  Look for further messages related to this for each affected room ...</source>
        <translation type="unfinished">
            <numerusform>[ ALERT ] - %n bad room id found in map! (less than +1)  Look for further messages related to this for each affected room ...</numerusform>
            <numerusform>[ ALERT ] - %n bad room ids found in map! (less than +1)  Look for further messages related to this for each affected room ...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="1061"/>
        <source>[ INFO ]  - In area with id: %1 there were %n room(s) missing from those it
should be recording as possessing. They are:
%2.
They have been added.</source>
        <translation type="unfinished">
            <numerusform>[ INFO ]  - In area with id: %1 there was %n room missing from those it
should be recording as possessing. It is:
%2.
It has been added.</numerusform>
            <numerusform>[ INFO ]  - In area with id: %1 there were %n rooms missing from those it
should be recording as possessing. They are:
%2.
They have been added.</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/TRoomDB.cpp" line="1068"/>
        <source>[ INFO ]  - In this area there were %n room(s) missing from those it should be recorded as possessing.  They are: %1.  They have been added.</source>
        <translation type="unfinished">
            <numerusform>[ INFO ]  - In this area there was %n room missing from those it should be recorded as possessing.  It is: %1.  It has been added.</numerusform>
            <numerusform>[ INFO ]  - In this area there were %n rooms missing from those it should be recorded as possessing.  They are: %1.  They have been added.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>TTrigger</name>
    <message numerus="yes">
        <location filename="../../src/TTrigger.cpp" line="1167"/>
        <source>Trigger name=%1 will fire %n more time(s).
</source>
        <translation>
            <numerusform>Trigger name=%1 will fire 1 more time.
</numerusform>
            <numerusform>Trigger name=%1 will fire %n more times.
</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgPackageExporter</name>
    <message numerus="yes">
        <location filename="../../src/dlgPackageExporter.cpp" line="1461"/>
        <source>Select what to export (%1 items)</source>
        <comment>Package exporter selection</comment>
        <translation type="unfinished">
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgPackageManager</name>
    <message numerus="yes">
        <location filename="../../src/dlgPackageManager.cpp" line="246"/>
        <source>Remove packages</source>
        <comment>Button in package manager to remove selected package(s)</comment>
        <translation type="unfinished">
            <numerusform></numerusform>
            <numerusform></numerusform>
        </translation>
    </message>
</context>
<context>
    <name>dlgRoomSymbol</name>
    <message numerus="yes">
        <location filename="../../src/dlgRoomSymbol.cpp" line="77"/>
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
        <location filename="../../src/dlgRoomSymbol.cpp" line="97"/>
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
        <location filename="../../src/mudlet.cpp" line="3517"/>
        <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
        <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
        <translation>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;An update is now available!&lt;/i&gt;&lt;p&gt;</numerusform>
            <numerusform>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n updates are now available!&lt;/i&gt;&lt;p&gt;</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3535"/>
        <source>Review %n update(s)...</source>
        <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
        <translatorcomment>Could do with the insertion of &quot;the&quot; as a second word!</translatorcomment>
        <translation>
            <numerusform>Review the update...</numerusform>
            <numerusform>Review the %n updates...</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../../src/mudlet.cpp" line="3540"/>
        <source>&lt;p&gt;Review the update(s) available...&lt;/p&gt;</source>
        <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
        <translatorcomment>As per the developer&apos;s comment it is not necessary to include the number of update in this text in English.</translatorcomment>
        <translation>
            <numerusform>&lt;p&gt;Review the update available...&lt;/p&gt;</numerusform>
            <numerusform>&lt;p&gt;Review the updates available...&lt;/p&gt;</numerusform>
        </translation>
    </message>
</context>
</TS>
