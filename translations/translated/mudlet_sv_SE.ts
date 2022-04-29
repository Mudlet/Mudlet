<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="sv-SE" sourcelanguage="en">
  <context>
    <name>Discord</name>
    <message>
      <location filename="../src/discord.cpp" line="151"/>
      <source>via Mudlet</source>
      <translation>via Mudlet</translation>
    </message>
  </context>
  <context>
    <name>Feed</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="275"/>
      <source>Too many redirects.</source>
      <translation>För många omdirigeringar.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="284"/>
      <source>No data received from server</source>
      <translation>Ingen data mottagen från servern</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="295"/>
      <source>Could not verify download integrity.</source>
      <translation>Kunde inte verifiera nedladdningens integritet.</translation>
    </message>
  </context>
  <context>
    <name>GLWidget</name>
    <message>
      <location filename="../src/glwidget.cpp" line="288"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>Inga rum på kartan - ladda en annan karta eller börja kartläggningen från början.</translation>
    </message>
    <message>
      <location filename="../src/glwidget.cpp" line="293"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Du har inte en karta ännu - ladda en, eller börja kartlägga från början.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/glwidget.cpp" line="290"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>Host</name>
    <message>
      <location filename="../src/Host.cpp" line="483"/>
      <source>Text to send to the game</source>
      <translation>Text att skicka till spelet</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="657"/>
      <source>Failed to open xml file &quot;%1&quot; inside module %2 to update it. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when the xml file inside the module zip cannot be updated for some reason.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="669"/>
      <source>Failed to save &quot;%1&quot; to module &quot;%2&quot;. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when a module is saved as package but cannot be done for some reason.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="960"/>
      <source>[  OK  ]  - %1 Thanks a lot for using the Public Test Build!</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ]  - %1 Tack så mycket för att du använder Public Test Build!</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="961"/>
      <source>[  OK  ]  - %1 Help us make Mudlet better by reporting any problems.</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[ OK ] - %1 Hjälp oss att förbättra Mudlet genom att rapportera eventuella problem.</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1760"/>
      <source>Unpacking module:
&quot;%1&quot;
please wait...</source>
      <translation>Packar upp modulen:
&quot;%1&quot;
Vänligen vänta...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1762"/>
      <source>Unpacking package:
&quot;%1&quot;
please wait...</source>
      <translation>Packar upp paket:
&quot;%1&quot;
Vänligen vänta...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1766"/>
      <source>Unpacking</source>
      <translation>Packar upp</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2409"/>
      <source>Playing %1</source>
      <translation>Spelar %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2411"/>
      <location filename="../src/Host.cpp" line="2417"/>
      <source>%1 at %2:%3</source>
      <comment>%1 is the game name and %2:%3 is game server address like: mudlet.org:23</comment>
      <translation>%1 på %2:%3</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2804"/>
      <location filename="../src/Host.cpp" line="3844"/>
      <source>Map - %1</source>
      <translation>Karta - %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3860"/>
      <source>Pre-Map loading(3) report</source>
      <translation>Ladda ner förkarta(3) rapporten</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3870"/>
      <source>Loading map(3) at %1 report</source>
      <translation>Laddar karta (3) vid %1 -rapport</translation>
    </message>
  </context>
  <context>
    <name>KeyUnit</name>
    <message>
      <location filename="../src/KeyUnit.cpp" line="332"/>
      <source>%1undefined key (code: 0x%2)</source>
      <comment>%1 is a string describing the modifier keys (e.g. &quot;shift&quot; or &quot;control&quot;) used with the key, whose &apos;code&apos; number, in %2 is not one that we have a name for. This is probably one of those extra keys around the edge of the keyboard that some people have.</comment>
      <translation>%1undefinierad nyckel (kod: 0x%2)</translation>
    </message>
  </context>
  <context>
    <name>MapInfoContributorManager</name>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="116"/>
      <source>Area:%1%2 ID:%1%3 x:%1%4%1&lt;‑&gt;%1%5 y:%1%6%1&lt;‑&gt;%1%7 z:%1%8%1&lt;‑&gt;%1%9
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and non-breaking hyphens which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. %2 is the (text) name of the area, %3 is the number for it, %4 to %9 are pairs (min &lt;-&gt; max) of extremes for each of x,y and z coordinates</comment>
      <translation>Område:%1%2 ID:%1%3 x:%1%4%1&lt;-&gt;%1%5 y:%1%6%1&lt;-&gt;%1%7 z:%1%8%1&lt;-&gt;%1%9
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="140"/>
      <source>Room Name: %1
</source>
      <translation>Rumsnamn: %1
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="153"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when NO rooms are selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the current player&apos;s room.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="170"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when ONE room is selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the selected Room.</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mapInfoContributorManager.cpp" line="188"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="142"/>
      <source>! %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="144"/>
      <source>! %1 is away (%2)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="146"/>
      <source>! %1 is back</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="153"/>
      <source>! invited %1 to %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="156"/>
      <source>! %2 invited to %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="163"/>
      <source>! You have joined %1 as %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="165"/>
      <source>! %1 has joined %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="172"/>
      <source>! %1 kicked %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="180"/>
      <source>! %1 mode is %2 %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="182"/>
      <source>! %1 sets mode %2 %3 %4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="199"/>
      <source>[MOTD] %1%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="211"/>
      <source>! %1 has %2 users: %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="213"/>
      <source>! %1 has %2 users</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="220"/>
      <source>! %1 has changed nick to %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="234"/>
      <source>! %1 replied in %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="237"/>
      <location filename="../src/ircmessageformatter.cpp" line="286"/>
      <source>! %1 time is %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="240"/>
      <location filename="../src/ircmessageformatter.cpp" line="283"/>
      <source>! %1 version is %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="256"/>
      <source>[%1%2] %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="264"/>
      <source>&amp;lt;%1%2&amp;gt; [%3] %4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="278"/>
      <source>[INFO] %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="305"/>
      <location filename="../src/ircmessageformatter.cpp" line="331"/>
      <source>[ERROR] %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="315"/>
      <source>[Channel URL] %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="324"/>
      <source>[%1] %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="338"/>
      <source>! %1 has left %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="340"/>
      <source>! %1 has left %2 (%3)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="349"/>
      <source>! %1 replied in %2 seconds</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="363"/>
      <source>* %1 %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="369"/>
      <source>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="378"/>
      <source>! %1 has quit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="380"/>
      <source>! %1 has quit (%2)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="388"/>
      <source>! no topic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="397"/>
      <source>[TOPIC] %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="401"/>
      <source>! %2 cleared topic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="404"/>
      <source>! %2 changed topic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="410"/>
      <source>? %2 %3 %4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="417"/>
      <source>[WHOIS] %1 is %2@%3 (%4)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="418"/>
      <source>[WHOIS] %1 is connected via %2 (%3)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="419"/>
      <source>[WHOIS] %1 is connected since %2 (idle %3)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="421"/>
      <source>[WHOIS] %1 is away: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="424"/>
      <source>[WHOIS] %1 is logged in as %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="427"/>
      <source>[WHOIS] %1 is connected from %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="430"/>
      <source>[WHOIS] %1 is using a secure connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="433"/>
      <source>[WHOIS] %1 is on %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="442"/>
      <source>[WHOWAS] %1 was %2@%3 (%4)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="443"/>
      <source>[WHOWAS] %1 was connected via %2 (%3)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="445"/>
      <source>[WHOWAS] %1 was logged in as %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="453"/>
      <source>[WHO] %1 (%2)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="455"/>
      <source> - away</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="458"/>
      <source> - server operator</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="466"/>
      <source>%1s</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="473"/>
      <source>%1 days</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="477"/>
      <source>%1 hours</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="481"/>
      <source>%1 mins</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="483"/>
      <source>%1 secs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="55"/>
      <source>Start element not found!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="67"/>
      <source>line %1: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="149"/>
      <source>Expected %1 while parsing</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/jsonparser.cpp" line="145"/>
      <source>%1 @ line %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="82"/>
      <source>No data found!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="89"/>
      <source>Expected object in keymap
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="129"/>
      <source>Invalid keysequence used %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/texteditorkeymap.cpp" line="371"/>
      <source>Error parsing %1: %2 </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/textgrammar.cpp" line="306"/>
      <source>Error reading file %1:%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="435"/>
      <source>%1 ranges</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="441"/>
      <source>Line %1, Column %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="444"/>
      <source>, Offset %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="448"/>
      <source> | %1 chars selected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="452"/>
      <source> | scope: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="462"/>
      <source> (%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="399"/>
      <source>Error parsing theme %1:%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="404"/>
      <source>Error theme not found %1.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>RoomIdLineEditDelegate</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="137"/>
      <location filename="../src/dlgRoomExits.cpp" line="212"/>
      <source>Entered number is invalid. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="138"/>
      <location filename="../src/dlgRoomExits.cpp" line="142"/>
      <location filename="../src/dlgRoomExits.cpp" line="213"/>
      <location filename="../src/dlgRoomExits.cpp" line="217"/>
      <source>Set the number of the room that this special exit goes to.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="146"/>
      <location filename="../src/dlgRoomExits.cpp" line="221"/>
      <source>A positive roomID of the room that this special exit leads to is expected here. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>T2DMap</name>
    <message>
      <location filename="../src/T2DMap.cpp" line="3140"/>
      <source>Change the properties of this custom line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3369"/>
      <location filename="../src/T2DMap.cpp" line="4998"/>
      <source>Solid line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3370"/>
      <location filename="../src/T2DMap.cpp" line="4999"/>
      <source>Dot line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3371"/>
      <location filename="../src/T2DMap.cpp" line="5000"/>
      <source>Dash line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3372"/>
      <location filename="../src/T2DMap.cpp" line="5001"/>
      <source>Dash-dot line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3373"/>
      <location filename="../src/T2DMap.cpp" line="5002"/>
      <source>Dash-dot-dot line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3670"/>
      <source>x coordinate (was %1):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3671"/>
      <source>y coordinate (was %1):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3672"/>
      <source>z coordinate (was %1):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3864"/>
      <source>Delete color</source>
      <comment>Deletes an environment colour</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3883"/>
      <source>Define new color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4199"/>
      <source>%1 {count:%2}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1395"/>
      <location filename="../src/T2DMap.cpp" line="1516"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="142"/>
      <source>ID</source>
      <comment>Room ID in the mapper widget</comment>
      <translation>ID</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="142"/>
      <source>Name</source>
      <comment>Room name in the mapper widget</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="603"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1268"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Du har inte en karta ännu - ladda en, eller börja kartlägga från början.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/T2DMap.cpp" line="1265"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>Du har laddat en karta (%n rum), men Mudlet vet inte var du befinner dig just nu.</numerusform>
        <numerusform>Du har laddat en karta (%n rum), men Mudlet vet inte var du befinner dig just nu.</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1263"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>Inga rum på kartan - ladda en annan karta eller börja kartläggningen från början.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2412"/>
      <source>render time: %1S mO: (%2,%3,%4)</source>
      <comment>This is debug information that is not expected to be seen in release versions, %1 is a decimal time period and %2-%4 are the x,y and z coordinates at the center of the view (but y will be negative compared to previous room related ones as it represents the real coordinate system for this widget which has y increasing in a downward direction!)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2752"/>
      <source>Drag to select multiple rooms or labels, release to finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2856"/>
      <source>Undo</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2857"/>
      <source>Undo last point</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2864"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2866"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2867"/>
      <source>Change the properties of this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2870"/>
      <source>Finish</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2871"/>
      <source>Finish drawing this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2937"/>
      <source>Create new map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2940"/>
      <source>Load map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2962"/>
      <source>Move</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3034"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3001"/>
      <source>Increase map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3008"/>
      <source>Decrease map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3016"/>
      <source>Lock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Lås</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3017"/>
      <source>Lock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3021"/>
      <source>Unlock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3022"/>
      <source>Unlock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2994"/>
      <source>Set one or more symbols or letters to mark special rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2976"/>
      <source>Replace an exit line with a custom line</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2980"/>
      <source>Custom exit lines are not shown and are not editable in grid mode</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2956"/>
      <source>Create new room here</source>
      <comment>Menu option to create a new room in the mapper</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="144"/>
      <source>Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection.  Click on the relevant header to sort by that method.  Note that the name column will only show if at least one of the rooms has a name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2968"/>
      <source>Set exits...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2974"/>
      <source>Create exit line...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2987"/>
      <source>Set color...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2993"/>
      <source>Set symbol...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3000"/>
      <source>Spread...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3007"/>
      <source>Shrink...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3028"/>
      <source>Set weight...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3040"/>
      <source>Move to position...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3041"/>
      <source>Move selected room or group of rooms to the given coordinates in this area</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3047"/>
      <source>Move to area...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3052"/>
      <source>Create label...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3053"/>
      <source>Create label to show text or an image</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3059"/>
      <source>Set player location</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3060"/>
      <source>Set the player&apos;s current location to here</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3069"/>
      <source>Switch to editing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3070"/>
      <source>Switch to viewing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3077"/>
      <source>Move</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3078"/>
      <source>Move label</source>
      <comment>2D Mapper context menu item (label) tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3080"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3081"/>
      <source>Delete label</source>
      <comment>2D Mapper context menu (label) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3095"/>
      <source>Add point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3102"/>
      <source>Divide segment by adding a new point mid-way along</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3105"/>
      <source>Select a point first, then add a new point mid-way along the segment towards room</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3109"/>
      <source>Remove point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3116"/>
      <source>Merge pair of segments by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3120"/>
      <source>Remove last segment by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3125"/>
      <source>use &quot;delete line&quot; to remove the only segment ending in an editable point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action &quot;delete line&quot; should match the translation for that action)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3131"/>
      <source>Select a point first, then remove it</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3135"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3139"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3143"/>
      <source>Delete line</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3144"/>
      <source>Delete all of this custom line</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3663"/>
      <source>Move the selection, centered on
the highlighted room (%1) to:</source>
      <comment>Use linefeeds as necessary to format text into a reasonable rectangle of text, %1 is a room number</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3687"/>
      <source>OK</source>
      <comment>dialog (room(s) move) button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3692"/>
      <source>Cancel</source>
      <comment>dialog (room(s) move) button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3891"/>
      <source>OK</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3896"/>
      <source>Cancel</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3954"/>
      <source>Spread out rooms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3955"/>
      <source>Increase the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4015"/>
      <source>Shrink in rooms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4016"/>
      <source>Decrease the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4144"/>
      <location filename="../src/T2DMap.cpp" line="4158"/>
      <location filename="../src/T2DMap.cpp" line="4208"/>
      <source>Enter room weight</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4145"/>
      <source>Enter new roomweight
(= travel time), minimum
(and default) is 1:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4159"/>
      <source>Enter new roomweight
(= travel time) for all
selected rooms, minimum
(and default) is 1 and
the only current value
used is:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4197"/>
      <source>%1 {count:%2, default}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4205"/>
      <source>1 {count 0, default}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4209"/>
      <source>Choose an existing
roomweight (= travel
time) from the list
(sorted by most commonly
used first) or enter a
new (positive) integer
value for all selected
rooms:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4259"/>
      <source>Load Mudlet map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4261"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) or the ;;s as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5145"/>
      <location filename="../src/T2DMap.cpp" line="5178"/>
      <source>Left-click to add point, right-click to undo/change/finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5188"/>
      <source>Left-click and drag a square for the size and position of your label</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TAlias</name>
    <message>
      <location filename="../src/TAlias.cpp" line="132"/>
      <location filename="../src/TAlias.cpp" line="200"/>
      <source>[Alias Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TAlias.cpp" line="269"/>
      <source>Error: in &quot;Pattern:&quot;, faulty regular expression, reason: &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TArea</name>
    <message>
      <location filename="../src/TArea.cpp" line="376"/>
      <source>roomID=%1 does not exist, can not set properties of a non-existent room!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TArea.cpp" line="765"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TCommandLine</name>
    <message>
      <location filename="../src/TCommandLine.cpp" line="678"/>
      <source>Add to user dictionary</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="680"/>
      <source>Remove from user dictionary</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="684"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which we have bundled with Mudlet; the entries about this line are the ones that the user has personally added.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="694"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which is provided as part of the OS; the entries about this line are the ones that the user has personally added.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="762"/>
      <source>no suggestions (system)</source>
      <comment>used when the command spelling checker using the selected system dictionary has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="789"/>
      <source>no suggestions (shared)</source>
      <comment>used when the command spelling checker using the dictionary shared between profile has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="793"/>
      <source>no suggestions (profile)</source>
      <comment>used when the command spelling checker using the profile&apos;s own dictionary has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TConsole</name>
    <message>
      <location filename="../src/TConsole.cpp" line="82"/>
      <source>Debug Console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="355"/>
      <source>&lt;i&gt;N:&lt;/i&gt; is the latency of the game server and network (aka ping, in seconds),&lt;br&gt;&lt;i&gt;S:&lt;/i&gt; is the system processing time - how long your triggers took to process the last line(s).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="779"/>
      <source>Save profile?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="779"/>
      <source>Do you want to save the profile %1?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="792"/>
      <source>Couldn&apos;t save profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="792"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1709"/>
      <source>System Message: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="324"/>
      <source>Show Time Stamps.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="335"/>
      <source>Record a replay.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="345"/>
      <source>Start logging game output to log file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="387"/>
      <source>Emergency Stop. Stops all timers and triggers.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="402"/>
      <source>Search buffer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="409"/>
      <source>Earlier search result.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="419"/>
      <source>Later search result.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="877"/>
      <source>Replay recording has started. File: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="880"/>
      <source>Replay recording has been stopped. File: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1817"/>
      <location filename="../src/TConsole.cpp" line="1856"/>
      <source>No search results, sorry!</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TEasyButtonBar</name>
    <message>
      <location filename="../src/TEasyButtonBar.cpp" line="70"/>
      <source>Easybutton Bar - %1 - %2</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TLuaInterpreter</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="11426"/>
      <source>Playing %1</source>
      <translation>Spelar %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13895"/>
      <location filename="../src/TLuaInterpreter.cpp" line="13919"/>
      <source>ERROR</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13896"/>
      <location filename="../src/TLuaInterpreter.cpp" line="13910"/>
      <source>object</source>
      <comment>object is the Mudlet alias/trigger/script, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13896"/>
      <location filename="../src/TLuaInterpreter.cpp" line="13910"/>
      <source>function</source>
      <comment>function is the Lua function, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15418"/>
      <source>Some functions may not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14807"/>
      <source>No error message available from Lua</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14809"/>
      <source>Lua error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14811"/>
      <source>[ ERROR ] - Cannot find Lua module %1.%2%3%4</source>
      <comment>%1 is the name of the module;%2 will be a line-feed inserted to put the next argument on a new line;%3 is the error message from the lua sub-system;%4 can be an additional message about the expected effect (but may be blank).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14825"/>
      <source>[  OK  ]  - Lua module %1 loaded.</source>
      <comment>%1 is the name (may specify which variant) of the module.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15405"/>
      <source>Probably will not be able to access Mudlet Lua code.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15423"/>
      <source>Database support will not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15430"/>
      <source>utf8.* Lua functions won&apos;t be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15436"/>
      <source>yajl.* Lua functions won&apos;t be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15621"/>
      <source>No error message available from Lua.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15623"/>
      <source>Lua error: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15625"/>
      <source>[ ERROR ] - Cannot load code formatter, indenting functionality won&apos;t be available.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15702"/>
      <source>%1 (doesn&apos;t exist)</source>
      <comment>This file doesn&apos;t exist</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15707"/>
      <source>%1 (isn&apos;t a file or symlink to a file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15716"/>
      <source>%1 (isn&apos;t a readable file or symlink to a readable file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15734"/>
      <source>%1 (couldn&apos;t read file)</source>
      <comment>This file could not be read for some reason (for example, no permission)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15740"/>
      <source>[  OK  ]  - Mudlet-lua API &amp; Geyser Layout manager loaded.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15747"/>
      <source>[ ERROR ] - Couldn&apos;t find, load and successfully run LuaGlobal.lua - your Mudlet is broken!
Tried these locations:
%1</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMainConsole</name>
    <message>
      <location filename="../src/TMainConsole.cpp" line="185"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="226"/>
      <source>Logging has started. Log file is %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="236"/>
      <source>Logging has been stopped. Log file is %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="266"/>
      <source>Mudlet MUD Client version: %1%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="268"/>
      <source>Mudlet, log from %1 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="304"/>
      <location filename="../src/TMainConsole.cpp" line="326"/>
      <source>&apos;Log session starting at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="334"/>
      <source>&apos;Log session ending at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="330"/>
      <source>Stop logging game output to log file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="346"/>
      <source>Start logging game output to log file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="701"/>
      <source>Pre-Map loading(2) report</source>
      <translation>Ladda ner förkarta(2) rapporten</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="712"/>
      <source>Loading map(2) at %1 report</source>
      <translation>Laddar karta (2) vid %1 -rapport</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1126"/>
      <source>User window - %1 - %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1174"/>
      <source>N:%1 S:%2</source>
      <comment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1181"/>
      <source>&lt;no GA&gt; S:%1</source>
      <comment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1279"/>
      <source>Pre-Map loading(1) report</source>
      <translation>Ladda ner förkarta(1) rapporten</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1297"/>
      <source>Loading map(1) at %1 report</source>
      <translation>Laddar karta (1) vid %1 -rapport</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1299"/>
      <source>Loading map(1) &quot;%1&quot; at %2 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1343"/>
      <source>Pre-Map importing(1) report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1366"/>
      <source>[ ERROR ]  - Map file not found, path and name used was:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1372"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; was not found).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1381"/>
      <source>[ INFO ]  - Map file located and opened, now parsing it...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1388"/>
      <source>Importing map(1) &quot;%1&quot; at %2 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1391"/>
      <source>[ INFO ]  - Map file located but it could not opened, please check permissions on:&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1394"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; could not be opened for reading).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1418"/>
      <source>[ INFO ]  - Map reload request received from system...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1423"/>
      <source>[  OK  ]  - ... System Map reload request completed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1425"/>
      <source>[ WARN ]  - ... System Map reload request failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1452"/>
      <source>+--------------------------------------------------------------+
|                      system statistics                       |
+--------------------------------------------------------------+
</source>
      <comment>Header for the system&apos;s statistics information displayed in the console, it is 64 &apos;narrow&apos; characters wide</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1461"/>
      <source>GMCP events:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1465"/>
      <source>ATCP events:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1469"/>
      <source>Channel102 events:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1473"/>
      <source>MSSP events:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1478"/>
      <source>MSDP events:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1489"/>
      <source>Trigger Report:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1493"/>
      <source>Timer Report:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1497"/>
      <source>Alias Report:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1501"/>
      <source>Keybinding Report:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1505"/>
      <source>Script Report:</source>
      <comment>Heading for the system&apos;s statistics information displayed in the console</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMap</name>
    <message>
      <location filename="../src/TMap.cpp" line="214"/>
      <source>RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="225"/>
      <source>AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="639"/>
      <source>[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="642"/>
      <source>[ INFO ] - Converting old style label id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="647"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="650"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label with id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="679"/>
      <source>[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="686"/>
      <source>[  OK  ]  - Map loaded successfully (%1s).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1157"/>
      <source>n</source>
      <comment>This translation converts the direction that DIR_NORTH codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1158"/>
      <source>ne</source>
      <comment>This translation converts the direction that DIR_NORTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1159"/>
      <source>e</source>
      <comment>This translation converts the direction that DIR_EAST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1160"/>
      <source>se</source>
      <comment>This translation converts the direction that DIR_SOUTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1161"/>
      <source>s</source>
      <comment>This translation converts the direction that DIR_SOUTH codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1162"/>
      <source>sw</source>
      <comment>This translation converts the direction that DIR_SOUTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1163"/>
      <source>w</source>
      <comment>This translation converts the direction that DIR_WEST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1164"/>
      <source>nw</source>
      <comment>This translation converts the direction that DIR_NORTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1165"/>
      <source>up</source>
      <comment>This translation converts the direction that DIR_UP codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1166"/>
      <source>down</source>
      <comment>This translation converts the direction that DIR_DOWN codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1167"/>
      <source>in</source>
      <comment>This translation converts the direction that DIR_IN codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1168"/>
      <source>out</source>
      <comment>This translation converts the direction that DIR_OUT codes for to a direction string that the game server will accept!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="191"/>
      <source>[MAP ERROR:]%1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="48"/>
      <source>Default Area</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="49"/>
      <source>Unnamed Area</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="609"/>
      <source>[ INFO ]  - Map audit starting...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1615"/>
      <source>[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!
There is so much data that it DOES NOT have that you could be
better off starting again...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1678"/>
      <source>[ ALERT ] - Failed to load a Mudlet JSON Map file, reason:
%1; the file is:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1683"/>
      <source>[ INFO ]  - Ignoring this map file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1853"/>
      <source>[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an
area) not found, adding reserved -1 id.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1947"/>
      <source>[ INFO ]  - Successfully read the map file (%1s), checking some
consistency details...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1962"/>
      <source>No map found. Would you like to download the map or start your own?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1963"/>
      <source>Download the map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1964"/>
      <source>Start my own</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2414"/>
      <source>Map issues</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2421"/>
      <source>Area issues</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2427"/>
      <source>Area id: %1 &quot;%2&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2429"/>
      <source>Area id: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2438"/>
      <source>Room issues</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2445"/>
      <source>Room id: %1 &quot;%2&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2447"/>
      <source>Room id: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2457"/>
      <source>End of report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2463"/>
      <source>[ ALERT ] - At least one thing was detected during that last map operation
that it is recommended that you review the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2471"/>
      <source>[ INFO ]  - The equivalent to the above information about that last map
operation has been saved for review as the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2527"/>
      <source>[ ERROR ] - Unable to use or create directory to store map.
Please check that you have permissions/access to:
&quot;%1&quot;
and there is enough space. The download operation has failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2553"/>
      <source>[ INFO ]  - Map download initiated, please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2564"/>
      <source>Map download</source>
      <comment>This is a title of a progress window.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2627"/>
      <source>Map import</source>
      <comment>This is a title of a progress dialog.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2937"/>
      <location filename="../src/TMap.cpp" line="3428"/>
      <source>Exporting JSON map data from %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2952"/>
      <source>Map JSON export</source>
      <comment>This is a title of a progress window.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3087"/>
      <source>Exporting JSON map file from %1 - writing data to file:
%2 ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3113"/>
      <source>import or export already in progress</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3121"/>
      <source>could not open file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3131"/>
      <source>could not parse file, reason: &quot;%1&quot; at offset %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3140"/>
      <source>empty Json file, no map data detected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3155"/>
      <source>invalid format version &quot;%1&quot; detected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3161"/>
      <source>no format version detected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3167"/>
      <source>no areas detected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3192"/>
      <source>Map JSON import</source>
      <comment>This is a title of a progress window.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3288"/>
      <source>aborted by user</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3177"/>
      <location filename="../src/TMap.cpp" line="3438"/>
      <source>Importing JSON map data to %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1191"/>
      <source>[ ERROR ] - The format version &quot;%1&quot; you are trying to save the map with is too new
for this version of Mudlet. Supported are only formats up to version %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1207"/>
      <source>[ ALERT ] - Saving map in format version &quot;%1&quot; that is different than &quot;%2&quot; which
it was loaded as. This may be an issue if you want to share the resulting
map with others relying on the original format.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1217"/>
      <source>[ WARN ]  - Saving map in format version &quot;%1&quot; different from the
recommended map version %2 for this version of Mudlet.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1555"/>
      <location filename="../src/TMap.cpp" line="2006"/>
      <source>[ ERROR ] - Unable to open map file for reading: &quot;%1&quot;!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1577"/>
      <source>[ ALERT ] - File does not seem to be a Mudlet Map file. The part that indicates
its format version seems to be &quot;%1&quot; and that doesn&apos;t make sense. The file is:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1592"/>
      <source>[ ALERT ] - Map file is too new. Its format version &quot;%1&quot; is higher than this version of
Mudlet can handle (%2)! The file is:
&quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1599"/>
      <source>[ INFO ]  - You will need to update your Mudlet to read the map file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1608"/>
      <source>[ ALERT ] - Map file is really old. Its format version &quot;%1&quot; is so ancient that
this version of Mudlet may not gain enough information from
it but it will try! The file is: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1622"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File:
&quot;%2&quot;,
please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1625"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2022"/>
      <source>[ INFO ]  - Checking map file &quot;%1&quot;, format version &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2563"/>
      <location filename="../src/TMap.cpp" line="2946"/>
      <location filename="../src/TMap.cpp" line="3186"/>
      <source>Abort</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2561"/>
      <source>Downloading map file for use in %1...</source>
      <comment>%1 is the name of the current Mudlet profile</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1584"/>
      <source>[ INFO ]  - Ignoring this unlikely map file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2595"/>
      <source>loadMap: unable to perform request, a map is already being downloaded or
imported at user request.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2626"/>
      <source>Importing XML map file for use in %1...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2654"/>
      <source>loadMap: failure to import XML map file, further information may be available
in main console!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2689"/>
      <source>[ ALERT ] - Map download was canceled, on user&apos;s request.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2708"/>
      <source>[ ERROR ] - Map download encountered an error:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2739"/>
      <source>[ ALERT ] - Map download failed, error reported was:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2747"/>
      <source>[ ALERT ] - Map download failed, unable to open destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2752"/>
      <source>[ ALERT ] - Map download failed, unable to write destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2765"/>
      <location filename="../src/TMap.cpp" line="2782"/>
      <source>[ INFO ]  - ... map downloaded and stored, now parsing it...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2773"/>
      <location filename="../src/TMap.cpp" line="2808"/>
      <source>[ ERROR ] - Map download problem, failure in parsing destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2813"/>
      <source>[ ERROR ] - Map download problem, unable to read destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TRoom</name>
    <message>
      <location filename="../src/TRoom.cpp" line="86"/>
      <location filename="../src/TRoom.cpp" line="976"/>
      <source>North</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="87"/>
      <source>North-east</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="88"/>
      <source>North-west</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="89"/>
      <location filename="../src/TRoom.cpp" line="1018"/>
      <source>South</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="90"/>
      <source>South-east</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="91"/>
      <source>South-west</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="92"/>
      <location filename="../src/TRoom.cpp" line="1060"/>
      <source>East</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="93"/>
      <location filename="../src/TRoom.cpp" line="1074"/>
      <source>West</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="94"/>
      <location filename="../src/TRoom.cpp" line="1088"/>
      <source>Up</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="95"/>
      <location filename="../src/TRoom.cpp" line="1102"/>
      <source>Down</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="96"/>
      <location filename="../src/TRoom.cpp" line="1116"/>
      <source>In</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="97"/>
      <location filename="../src/TRoom.cpp" line="1130"/>
      <source>Out</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="98"/>
      <source>Other</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="99"/>
      <source>Unknown</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="310"/>
      <source>No area created!  Requested area ID=%1. Note: Area IDs must be &gt; 0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="332"/>
      <source>Warning: When setting the Area for Room (Id: %1) it did not have a current area!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="990"/>
      <source>Northeast</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1004"/>
      <source>Northwest</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1032"/>
      <source>Southeast</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1046"/>
      <source>Southwest</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1151"/>
      <source>[ WARN ]  - In room id:%1 removing invalid (special) exit to %2 {with no name!}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1155"/>
      <source>[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1164"/>
      <source>[ INFO ]  - In room with id: %1 correcting special exit &quot;%2&quot; that
was to room with an exit to invalid room: %3 to now go
to: %4.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1174"/>
      <source>[ INFO ]  - Room needed correcting of special exit &quot;%1&quot; that was to room with an exit to invalid room: %2 to now go to: %3.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1199"/>
      <source>[ WARN ]  - Room with id: %1 has a special exit &quot;%2&quot; with an
exit to: %3 but that room does not exist.  The exit will
be removed (but the destination room id will be stored in
the room user data under a key:
&quot;%4&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1212"/>
      <source>[ WARN ]  - Room has a special exit &quot;%1&quot; with an exit to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1251"/>
      <source>[ INFO ]  - In room with id: %1 special exit &quot;%2&quot;
that was to room with an invalid room: %3 that does not exist.
The exit will be removed (the bad destination room id will be stored in the
room user data under a key:
&quot;%4&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1264"/>
      <source>[ INFO ]  - Room had special exit &quot;%1&quot; that was to room with an invalid room: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1297"/>
      <source>%1 {none}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1300"/>
      <source>%1 (open)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1303"/>
      <source>%1 (closed)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1306"/>
      <source>%1 (locked)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1309"/>
      <source>%1 {invalid}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1313"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1320"/>
      <source>[ INFO ]  - Room had one or more surplus door items that were removed:%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1336"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1343"/>
      <source>[ INFO ]  - Room had one or more surplus weight items that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1359"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus exit lock items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1366"/>
      <source>[ INFO ]  - Room had one or more surplus exit lock items that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1445"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that
were removed: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1451"/>
      <source>[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1474"/>
      <source>[ INFO ]  - In room with id: %1 correcting exit &quot;%2&quot; that was to room with
an exit to invalid room: %3 to now go to: %4.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1483"/>
      <source>[ INFO ]  - Correcting exit &quot;%1&quot; that was to invalid room id: %2 to now go to: %3.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1494"/>
      <source>[ WARN ]  - Room with id: %1 has an exit &quot;%2&quot; to: %3 but that room
does not exist.  The exit will be removed (but the destination room
Id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1507"/>
      <source>[ WARN ]  - Room has an exit &quot;%1&quot; to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1551"/>
      <source>[ ALERT ] - Room with id: %1 has an exit &quot;%2&quot; to: %3 but also
has a stub exit!  As a real exit precludes a stub, the latter will
be removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1561"/>
      <source>[ ALERT ] - Room has an exit &quot;%1&quot; to: %2 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1617"/>
      <source>[ INFO ]  - In room with id: %1 exit &quot;%2&quot; that was to room with an invalid
room: %3 that does not exist.  The exit will be removed (the bad destination
room id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1628"/>
      <source>[ INFO ]  - Room exit &quot;%1&quot; that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1644"/>
      <source>
It was locked, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1648"/>
      <source>  It was locked, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1656"/>
      <source>
It had a weight, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1660"/>
      <source>  It had a weight, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1671"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but
it has not been possible to salvage this, it has been lost!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1676"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but it has not been possible to salvage this, it has been lost!</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TRoomDB</name>
    <message>
      <location filename="../src/TRoomDB.cpp" line="504"/>
      <source>Area with ID %1 already exists!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="523"/>
      <source>An Unnamed Area is (no longer) permitted!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="527"/>
      <source>An area called %1 already exists!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="626"/>
      <source>[ WARN ]  - Problem with data structure associated with room id: %1 - that
room&apos;s data has been lost so the id is now being deleted.  This
suggests serious problems with the currently running version of
Mudlet - is your system running out of memory?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="634"/>
      <source>[ WARN ]  - Problem with data structure associated with this room.  The room&apos;s data has been lost so the id is now being deleted.  This suggests serious problems with the currently running version of Mudlet - is your system running out of memory?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="692"/>
      <source>[ ALERT ] - Area with id: %1 expected but not found, will be created.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="695"/>
      <source>[ ALERT ] - Area with this id expected but not found, will be created.</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="740"/>
      <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="775"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1)
in map, now working out what new id numbers to use...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="787"/>
      <source>[ INFO ]  - The renumbered area ids will be:
Old ==&gt; New</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="805"/>
      <source>[ INFO ]  - The area with this bad id was renumbered to: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="806"/>
      <source>[ INFO ]  - This area was renumbered from the bad id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="843"/>
      <location filename="../src/TRoomDB.cpp" line="846"/>
      <source>[ INFO ]  - Area id numbering is satisfactory.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="854"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map, now working
out what new id numbers to use.</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="724"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that are supposed
 to be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="731"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="780"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1) in map!  Look for further messages related to this for each affected area ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="859"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!  Look for further messages related to this for each affected room ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="866"/>
      <source>[ INFO ]  - The renumbered rooms will be:
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="882"/>
      <source>[ INFO ]  - This room with the bad id was renumbered to: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="883"/>
      <source>[ INFO ]  - This room was renumbered from the bad id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="919"/>
      <location filename="../src/TRoomDB.cpp" line="922"/>
      <source>[ INFO ]  - Room id numbering is satisfactory.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="946"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="951"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="968"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="973"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1060"/>
      <source>[ INFO ]  - This room claims to be in area id: %1, but that did not have a record of it.  The area has been updated to include this room.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1066"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 rooms missing from those it
should be recording as possessing, they were:
%3
they have been added.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1074"/>
      <source>[ INFO ]  - In this area there were %1 rooms missing from those it should be recorded as possessing.  They are: %2.  They have been added.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1099"/>
      <source>[ INFO ]  - This room was claimed by area id: %1, but it does not belong there.  The area has been updated to not include this room.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1105"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 extra rooms compared to those it
should be recording as possessing, they were:
%3
they have been removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1113"/>
      <source>[ INFO ]  - In this area there were %1 extra rooms that it should not be recorded as possessing.  They were: %2.  They have been removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1212"/>
      <source>It has been detected that &quot;_###&quot; form suffixes have already been used, for simplicity in the renaming algorithm these will have been removed and possibly changed as Mudlet sorts this matter out, if a number assigned in this way &lt;b&gt;is&lt;/b&gt; important to you, you can change it back, provided you rename the area that has been allocated the suffix that was wanted first...!&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1216"/>
      <source>[  OK  ]  - The changes made are:
(ID) &quot;old name&quot; ==&gt; &quot;new name&quot;
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1223"/>
      <source>&lt;nothing&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1226"/>
      <source>[ INFO ]  - Area name changed to prevent duplicates or unnamed ones; old name: &quot;%1&quot;, new name: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1235"/>
      <source>[ ALERT ] - Empty and duplicate area names detected in Map file!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1236"/>
      <source>[ INFO ]  - Mudlet had previously allowed the map to have more than one area
with the same or no name. To resolve these cases, an area without a name
here (or created in the future) will automatically be assigned the name &quot;%1&quot;.
Duplicated area names will cause all but the first encountered one to gain a
&quot;_###&quot; style suffix.
%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1244"/>
      <source>[ ALERT ] - Duplicate area names detected in the Map file!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1245"/>
      <source>[ INFO ]  - Due to some situations not being checked in the past, Mudlet had
allowed the user to have more than one area with the same name.
These make some things confusing and are now disallowed.
  Duplicated area names will cause all but the first encountered one
to gain a &quot;_###&quot; style suffix where each &quot;###&quot; is an increasing
number; you may wish to change these, perhaps by replacing them with
a &quot;(sub-area name)&quot; but it is entirely up to you how you do this,
other then you will not be able to set one area&apos;s name to that of
another that exists at the time.
  If there were more than one area without a name then all but the
first will also gain a suffix in this manner.
%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1260"/>
      <source>[ ALERT ] - An empty area name was detected in the Map file!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1263"/>
      <source>[  OK  ]  - Due to some situations not being checked in the past, Mudlet had
allowed the map to have an area with no name. This can make some
things confusing and is now disallowed.
  To resolve this case, the area without a name here (or one created
in the future) will automatically be assigned the name &quot;%1&quot;.
  If this happens more then once the duplication of area names will
cause all but the first encountered one to gain a &quot;_###&quot; style
suffix where each &quot;###&quot; is an increasing number; you may wish to
change these, perhaps by adding more meaningful area names but it is
entirely up to you what is used, other then you will not be able to
set one area&apos;s name to that of another that exists at the time.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1287"/>
      <source>[ INFO ]  - Default (reset) area name (for rooms that have not been assigned to an
area) not found, adding &quot;%1&quot; against the reserved -1 id.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TTextEdit</name>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1416"/>
      <source>Copy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1427"/>
      <source>Copy HTML</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1431"/>
      <source>Copy as image</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1434"/>
      <source>Select All</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1439"/>
      <source>Search on %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1457"/>
      <source>Analyse characters</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1466"/>
      <source>Hover on this item to display the Unicode codepoints in the selection &lt;i&gt;(only the first line!)&lt;/i&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1475"/>
      <source>restore Main menu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1477"/>
      <source>Use this to restore the Main menu to get access to controls.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1479"/>
      <source>restore Main Toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1481"/>
      <source>Use this to restore the Main Toolbar to get access to controls.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1489"/>
      <source>Clear console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1492"/>
      <source>*** starting new session ***
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1577"/>
      <source>Mudlet, debug console extract</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1579"/>
      <source>Mudlet, %1 mini-console extract from %2 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1581"/>
      <source>Mudlet, %1 user window extract from %2 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1583"/>
      <source>Mudlet, main console extract from %1 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2050"/>
      <source>{tab}</source>
      <comment>Unicode U+0009 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2051"/>
      <source>{line-feed}</source>
      <comment>Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2052"/>
      <source>{carriage-return}</source>
      <comment>Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2053"/>
      <source>{space}</source>
      <comment>Unicode U+0020 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2054"/>
      <source>{non-breaking space}</source>
      <comment>Unicode U+00A0 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2055"/>
      <source>{soft hyphen}</source>
      <comment>Unicode U+00AD codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2056"/>
      <source>{combining grapheme joiner}</source>
      <comment>Unicode U+034F codepoint (badly named apparently - see Wikipedia!)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2057"/>
      <source>{ogham space mark}</source>
      <comment>Unicode U+1680 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2058"/>
      <source>{&apos;n&apos; quad}</source>
      <comment>Unicode U+2000 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2059"/>
      <source>{&apos;m&apos; quad}</source>
      <comment>Unicode U+2001 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2060"/>
      <source>{&apos;n&apos; space}</source>
      <comment>Unicode U+2002 codepoint - En (&apos;n&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2061"/>
      <source>{&apos;m&apos; space}</source>
      <comment>Unicode U+2003 codepoint - Em (&apos;m&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2062"/>
      <source>{3-per-em space}</source>
      <comment>Unicode U+2004 codepoint - three-per-em (&apos;m&apos;) wide (thick) space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2063"/>
      <source>{4-per-em space}</source>
      <comment>Unicode U+2005 codepoint - four-per-em (&apos;m&apos;) wide (Middle) space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2064"/>
      <source>{6-per-em space}</source>
      <comment>Unicode U+2006 codepoint - six-per-em (&apos;m&apos;) wide (Sometimes the same as a Thin) space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2065"/>
      <source>{digit space}</source>
      <comment>Unicode U+2007 codepoint - figure (digit) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2066"/>
      <source>{punctuation wide space}</source>
      <comment>Unicode U+2008 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2067"/>
      <source>{5-per-em space}</source>
      <comment>Unicode U+2009 codepoint - five-per-em (&apos;m&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2068"/>
      <source>{hair width space}</source>
      <comment>Unicode U+200A codepoint - thinnest space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2069"/>
      <source>{zero width space}</source>
      <comment>Unicode U+200B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2070"/>
      <source>{Zero width non-joiner}</source>
      <comment>Unicode U+200C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2071"/>
      <source>{zero width joiner}</source>
      <comment>Unicode U+200D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2072"/>
      <source>{left-to-right mark}</source>
      <comment>Unicode U+200E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2073"/>
      <source>{right-to-left mark}</source>
      <comment>Unicode U+200F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2074"/>
      <source>{line separator}</source>
      <comment>Unicode 0x2028 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2075"/>
      <source>{paragraph separator}</source>
      <comment>Unicode U+2029 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2076"/>
      <source>{Left-to-right embedding}</source>
      <comment>Unicode U+202A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2077"/>
      <source>{right-to-left embedding}</source>
      <comment>Unicode U+202B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2078"/>
      <source>{pop directional formatting}</source>
      <comment>Unicode U+202C codepoint - pop (undo last) directional formatting.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2079"/>
      <source>{Left-to-right override}</source>
      <comment>Unicode U+202D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2080"/>
      <source>{right-to-left override}</source>
      <comment>Unicode U+202E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2081"/>
      <source>{narrow width no-break space}</source>
      <comment>Unicode U+202F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2082"/>
      <source>{medium width mathematical space}</source>
      <comment>Unicode U+205F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2083"/>
      <source>{zero width non-breaking space}</source>
      <comment>Unicode U+2060 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2084"/>
      <source>{function application}</source>
      <comment>Unicode U+2061 codepoint - function application (whatever that means!)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2085"/>
      <source>{invisible times}</source>
      <comment>Unicode U+2062 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2086"/>
      <source>{invisible separator}</source>
      <comment>Unicode U+2063 codepoint - invisible separator or comma.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2087"/>
      <source>{invisible plus}</source>
      <comment>Unicode U+2064 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2088"/>
      <source>{left-to-right isolate}</source>
      <comment>Unicode U+2066 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2089"/>
      <source>{right-to-left isolate}</source>
      <comment>Unicode U+2067 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2090"/>
      <source>{first strong isolate}</source>
      <comment>Unicode U+2068 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2091"/>
      <source>{pop directional isolate}</source>
      <comment>Unicode U+2069 codepoint - pop (undo last) directional isolate.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2092"/>
      <source>{inhibit symmetrical swapping}</source>
      <comment>Unicode U+206A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2093"/>
      <source>{activate symmetrical swapping}</source>
      <comment>Unicode U+206B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2094"/>
      <source>{inhibit arabic form-shaping}</source>
      <comment>Unicode U+206C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2095"/>
      <source>{activate arabic form-shaping}</source>
      <comment>Unicode U+206D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2096"/>
      <source>{national digit shapes}</source>
      <comment>Unicode U+206E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2097"/>
      <source>{nominal Digit shapes}</source>
      <comment>Unicode U+206F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2098"/>
      <source>{ideographic space}</source>
      <comment>Unicode U+3000 codepoint - ideographic (CJK Wide) space</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2099"/>
      <source>{variation selector 1}</source>
      <comment>Unicode U+FE00 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2100"/>
      <source>{variation selector 2}</source>
      <comment>Unicode U+FE01 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2101"/>
      <source>{variation selector 3}</source>
      <comment>Unicode U+FE02 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2102"/>
      <source>{variation selector 4}</source>
      <comment>Unicode U+FE03 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2103"/>
      <source>{variation selector 5}</source>
      <comment>Unicode U+FE04 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2104"/>
      <source>{variation selector 6}</source>
      <comment>Unicode U+FE05 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2105"/>
      <source>{variation selector 7}</source>
      <comment>Unicode U+FE06 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2106"/>
      <source>{variation selector 8}</source>
      <comment>Unicode U+FE07 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2107"/>
      <source>{variation selector 9}</source>
      <comment>Unicode U+FE08 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2108"/>
      <source>{variation selector 10}</source>
      <comment>Unicode U+FE09 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2109"/>
      <source>{variation selector 11}</source>
      <comment>Unicode U+FE0A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2110"/>
      <source>{variation selector 12}</source>
      <comment>Unicode U+FE0B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2111"/>
      <source>{variation selector 13}</source>
      <comment>Unicode U+FE0C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2112"/>
      <source>{variation selector 14}</source>
      <comment>Unicode U+FE0D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2113"/>
      <source>{variation selector 15}</source>
      <comment>Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black &amp; white) rendition.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2114"/>
      <source>{variation selector 16}</source>
      <comment>Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured &apos;Emoji&apos; rendition.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2115"/>
      <source>{zero width no-break space}</source>
      <comment>Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2121"/>
      <source>{interlinear annotation anchor}</source>
      <comment>Unicode U+FFF9 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2122"/>
      <source>{interlinear annotation separator}</source>
      <comment>Unicode U+FFFA codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2123"/>
      <source>{interlinear annotation terminator}</source>
      <comment>Unicode U+FFFB codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2124"/>
      <source>{object replacement character}</source>
      <comment>Unicode U+FFFC codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2136"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFD0 to U+FDEF - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2138"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFFx - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2147"/>
      <source>{FitzPatrick modifier 1 or 2}</source>
      <comment>Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2148"/>
      <source>{FitzPatrick modifier 3}</source>
      <comment>Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2149"/>
      <source>{FitzPatrick modifier 4}</source>
      <comment>Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2150"/>
      <source>{FitzPatrick modifier 5}</source>
      <comment>Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2151"/>
      <source>{FitzPatrick modifier 6}</source>
      <comment>Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2155"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2426"/>
      <location filename="../src/TTextEdit.cpp" line="2488"/>
      <source>Index (UTF-16)</source>
      <comment>1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2429"/>
      <location filename="../src/TTextEdit.cpp" line="2490"/>
      <source>U+&lt;i&gt;####&lt;/i&gt; Unicode Code-point &lt;i&gt;(High:Low Surrogates)&lt;/i&gt;</source>
      <comment>2nd Row heading for Text analyser output, table item is the unicode code point (will be between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2433"/>
      <location filename="../src/TTextEdit.cpp" line="2494"/>
      <source>Visual</source>
      <comment>3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a &apos;{&apos;...&apos;}&apos; wrapped letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2437"/>
      <location filename="../src/TTextEdit.cpp" line="2498"/>
      <source>Index (UTF-8)</source>
      <comment>4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2441"/>
      <location filename="../src/TTextEdit.cpp" line="2502"/>
      <source>Byte</source>
      <comment>5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2445"/>
      <location filename="../src/TTextEdit.cpp" line="2506"/>
      <source>Lua character or code</source>
      <comment>6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TToolBar</name>
    <message>
      <location filename="../src/TToolBar.cpp" line="74"/>
      <source>Toolbar - %1 - %2</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TTrigger</name>
    <message>
      <location filename="../src/TTrigger.cpp" line="195"/>
      <source>Error: This trigger has no patterns defined, yet. Add some to activate it.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="226"/>
      <source>Error: in item %1, perl regex &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="247"/>
      <source>Error: in item %1, lua function &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="267"/>
      <source>Error: in item %1, no colors to match were set - at least &lt;i&gt;one&lt;/i&gt; of the foreground or background must not be &lt;i&gt;ignored&lt;/i&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="325"/>
      <location filename="../src/TTrigger.cpp" line="409"/>
      <source>[Trigger Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="1199"/>
      <source>Trigger name=%1 expired.
</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TTrigger.cpp" line="1204"/>
      <source>Trigger name=%1 will fire %n more time(s).
</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>UpdateDialog</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="20"/>
      <source>%APPNAME% update</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="50"/>
      <source>Loading update information …</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="87"/>
      <source>A new version of %APPNAME% is available!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="113"/>
      <source>%APPNAME% %UPDATE_VERSION% is available (you have %CURRENT_VERSION%).
Would you like to update now?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="151"/>
      <source>Changelog for %APPNAME%</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="161"/>
      <source>You are using version %CURRENT_VERSION%.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="192"/>
      <source>There are currently no updates available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="208"/>
      <source>You are using %APPNAME% %CURRENT_VERSION%.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="321"/>
      <source>Automatically download future updates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="368"/>
      <source>Cancel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="388"/>
      <source>Install update now</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="395"/>
      <source>OK</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="405"/>
      <source>Remind me later</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="410"/>
      <source>Skip this version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.cpp" line="555"/>
      <source>Could not open downloaded file %1</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>Updater</name>
    <message>
      <location filename="../src/updater.cpp" line="46"/>
      <location filename="../src/updater.cpp" line="195"/>
      <location filename="../src/updater.cpp" line="261"/>
      <source>Update</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="359"/>
      <source>Restart to apply update</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>VarUnit</name>
    <message>
      <location filename="../src/VarUnit.cpp" line="88"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>XMLimport</name>
    <message>
      <location filename="../src/XMLimport.cpp" line="166"/>
      <source>[ ALERT ] - Sorry, the file being read:
&quot;%1&quot;
reports it has a version (%2) it must have come from a later Mudlet version,
and this one cannot read it, you need a newer Mudlet!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="354"/>
      <source>Parsing area data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="358"/>
      <source>Parsing room data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="362"/>
      <source>Parsing environment data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="370"/>
      <source>Assigning rooms to their areas...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="583"/>
      <source>Parsing room data [count: %1]...</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>about_dialog</name>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="41"/>
      <source>About Mudlet</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="122"/>
      <source>Mudlet</source>
      <translation>Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="185"/>
      <source>Supporters</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="214"/>
      <source>License</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="249"/>
      <source>Third Party</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>actions_main_area</name>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="115"/>
      <source>Button Bar Properties</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="127"/>
      <source>Number of columns/rows (depending on orientation):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="150"/>
      <source>Orientation Horizontal</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="155"/>
      <source>Orientation Vertical</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="170"/>
      <source>Dock Area Top</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="175"/>
      <source>Dock Area Left</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="180"/>
      <source>Dock Area Right</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="185"/>
      <source>Floating Toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="208"/>
      <source>Button Properties</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="214"/>
      <source>Button Rotation:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="231"/>
      <source>no rotation</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="236"/>
      <source>90° rotation to the left</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="241"/>
      <source>90° rotation to the right</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="249"/>
      <source>Push down button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="256"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="269"/>
      <location filename="../src/ui/actions_main_area.ui" line="289"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="276"/>
      <source>Command (up):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="72"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your button, menu or toolbar. This will be displayed in the buttons tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="266"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game if it is pressed. (Optional)&lt;/p&gt;&lt;p&gt;If this is a &lt;i&gt;push-down&lt;/i&gt; button then this is sent only when the button goes from the &lt;i&gt;up&lt;/i&gt; to &lt;i&gt;down&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="286"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game when this button goes from the &lt;i&gt;down&lt;/i&gt; to &lt;i&gt;up&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="308"/>
      <source>Stylesheet:</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>aliases_main_area</name>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="35"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="57"/>
      <source>choose a unique name for your alias; it will show in the tree and is needed for scripting.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="76"/>
      <source>Pattern:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="103"/>
      <source>enter a perl regex pattern for your alias; alias are triggers on your input</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="110"/>
      <source>Type:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="124"/>
      <source>Regex</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="129"/>
      <source>Plain</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="149"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="171"/>
      <source>&lt;p&gt;Type in one or more commands you want the alias to send directly to the game if the keys entered match the pattern. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="174"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>cTelnet</name>
    <message>
      <location filename="../src/ctelnet.cpp" line="572"/>
      <source>[ INFO ]  - The IP address of %1 has been found. It is: %2
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="581"/>
      <source>[ ERROR ] - Host name lookup Failure!
Connection cannot be established.
The server name is not correct, not working properly,
or your nameservers are not working properly.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="412"/>
      <source>[ ERROR ] - TCP/IP socket ERROR:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="439"/>
      <source>[ INFO ]  - A secure connection has been established successfully.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="441"/>
      <source>[ INFO ]  - A connection has been established successfully.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="477"/>
      <source>[ INFO ]  - Connection time: %1
    </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="520"/>
      <source>Secure connections aren&apos;t supported by this game on this port - try turning the option off.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="565"/>
      <source>[ INFO ]  - Trying secure connection to %1: %2 ...
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1652"/>
      <location filename="../src/ctelnet.cpp" line="2014"/>
      <source>[ INFO ]  - The server wants to upgrade the GUI to new version &apos;%1&apos;.
Uninstalling old version &apos;%2&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1663"/>
      <location filename="../src/ctelnet.cpp" line="2025"/>
      <source>[  OK  ]  - Package is already installed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1672"/>
      <location filename="../src/ctelnet.cpp" line="2034"/>
      <source>downloading game GUI from server</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1672"/>
      <location filename="../src/ctelnet.cpp" line="2034"/>
      <source>Cancel</source>
      <comment>Cancel download of GUI package from Server</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1661"/>
      <location filename="../src/ctelnet.cpp" line="2023"/>
      <source>[ INFO ]  - Server offers downloadable GUI (url=&apos;%1&apos;) (package=&apos;%2&apos;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="381"/>
      <source>[ INFO ]  - Looking up the IP address of server: %1:%2 ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="479"/>
      <source>hh:mm:ss.zzz</source>
      <comment>This is the format to be used to show the profile connection time, it follows the rules of the &quot;QDateTime::toString(...)&quot; function and may need modification for some locales, e.g. France, Spain.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="510"/>
      <location filename="../src/ctelnet.cpp" line="522"/>
      <source>[ ALERT ] - Socket got disconnected.
Reason: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="574"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 ...
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="576"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 via proxy...
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="623"/>
      <source>[ ERROR ] - Internal error, no codec found for current setting of {&quot;%1&quot;}
so Mudlet cannot send data in that format to the Game Server. Please
check to see if there is an alternative that the MUD and Mudlet can
use. Mudlet will attempt to send the data using the ASCII encoding
but will be limited to only unaccented characters of basic English.
Note: this warning will only be issued once, until the encoding is
changed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2310"/>
      <source>ERROR</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2322"/>
      <source>LUA</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2333"/>
      <source>WARN</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2344"/>
      <source>ALERT</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2355"/>
      <source>INFO</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2366"/>
      <source>OK</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2559"/>
      <source>[ INFO ]  - Loading replay file:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2583"/>
      <source>Cannot replay file &quot;%1&quot;, error message was: &quot;replay file seems to be corrupt&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2585"/>
      <source>[ WARN ]  - The replay has been aborted as the file seems to be corrupt.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2594"/>
      <source>Cannot perform replay, another one may already be in progress. Try again when it has finished.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2596"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress.
Try again when it has finished.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2604"/>
      <source>Cannot read file &quot;%1&quot;, error message was: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2607"/>
      <source>[ ERROR ] - Cannot read file &quot;%1&quot;,
error message was: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2646"/>
      <source>[  OK  ]  - The replay has ended.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>color_trigger</name>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="17"/>
      <source>ANSI 256 Color chooser</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="39"/>
      <source>&lt;small&gt;Choose:&lt;ul&gt;&lt;li&gt;one of the basic 16 colors below&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;more&lt;/i&gt; button to get access to other colors in the 256-color set, then follow the instructions to select a color from that part of the 256 colors supported; if such a color is already in use then that part will already be showing&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;Default&lt;/i&gt; or &lt;i&gt;Ignore&lt;/i&gt; buttons at the bottom for a pair of other special cases&lt;/li&gt;
&lt;li&gt;click &lt;i&gt;Cancel&lt;/i&gt; to close this dialog without making any changes&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</source>
      <comment>Ensure that &quot;Default&quot;, &quot;Ignore&quot; and &quot;Cancel&quot; in this instruction are the same as used for the controls elsewhere on this dialog.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="58"/>
      <source>Basic ANSI Colors [0-15] - click a button to select that color number directly:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="240"/>
      <source>ANSI 6R x 6G x 6B Colors [16-231] - adjust red, green, blue and click button to select matching color number:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="252"/>
      <source>Red (0-5)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="262"/>
      <source>Green (0-5)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="272"/>
      <source>Blue (0-5)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="282"/>
      <source>16 + 36 x R + 6 x G + B =</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="343"/>
      <source>[16]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="353"/>
      <source>Set to RGB value</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="363"/>
      <source>ANSI 24 Grays scale [232-255] - adjust gray and click button to select matching color number:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="375"/>
      <source>Gray (0-23)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="382"/>
      <source>232 + Gr =</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="414"/>
      <source>[232]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="424"/>
      <source>Set to Grayscale value</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>composer</name>
    <message>
      <location filename="../src/ui/composer.ui" line="14"/>
      <source>News and Message Composer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="86"/>
      <source>Cancel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="99"/>
      <source>&lt;p&gt;Save (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="102"/>
      <source>Save</source>
      <translation>Spara</translation>
    </message>
  </context>
  <context>
    <name>connection_profiles</name>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="20"/>
      <source>Select a profile to connect with</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="101"/>
      <source>profiles list</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="363"/>
      <source>Remove</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="382"/>
      <source>Copy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="407"/>
      <source>New</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="462"/>
      <source>welcome message</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="478"/>
      <source>Required</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="484"/>
      <source>Profile name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="500"/>
      <source>Profile name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="503"/>
      <source>A unique name for the profile but which is limited to a subset of ascii characters only.</source>
      <comment>Using lower case letters for &apos;ASCII&apos; may make speech synthesisers say &apos;askey&apos; which is quicker than &apos;Aay Ess Cee Eye Eye&apos;!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="513"/>
      <source>Server address:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="535"/>
      <source>Game server URL</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="538"/>
      <source>The Internet host name or IP address</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="545"/>
      <source>Port:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="573"/>
      <source>Game server port</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="576"/>
      <source>The port that is used together with the server name to make the connection to the game server. If not specified a default of 23 for &quot;Telnet&quot; connections is used. Secure connections may require a different port number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="595"/>
      <source>Connect via a secure protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="598"/>
      <source>Make Mudlet use a secure SSL/TLS protocol instead of an unencrypted one</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="604"/>
      <source>Secure:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="611"/>
      <source>Profile history:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="634"/>
      <source>load newest profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="639"/>
      <source>load oldest profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="662"/>
      <source>Optional</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="678"/>
      <source>Character name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="688"/>
      <source>The characters name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="691"/>
      <source>Character name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="694"/>
      <source>If provided will be sent, along with password to identify the user in the game.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="713"/>
      <source>Auto-open profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="716"/>
      <source>Automatically start this profile when Mudlet is run</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="729"/>
      <source>Auto-reconnect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="732"/>
      <source>Automatically reconnect this profile if it should become disconnected for any reason other than the user disconnecting from the game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="748"/>
      <source>Password</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="751"/>
      <source>If provided will be sent, along with the character name to identify the user in the game.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="764"/>
      <source>Enable Discord integration (not supported by game)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="770"/>
      <source>Allow this profile to use Mudlet&apos;s Discord &quot;Rich Presence&quot;  features</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="876"/>
      <location filename="../src/ui/connection_profiles.ui" line="879"/>
      <source>Game description or your notes</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="668"/>
      <source>Password:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="745"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="710"/>
      <source>With this enabled, Mudlet will automatically start and connect on this profile when it is launched</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="722"/>
      <source>Open profile on Mudlet start</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="738"/>
      <source>Reconnect automatically</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="767"/>
      <source>Discord integration</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="805"/>
      <source>Informational</source>
      <translation>Information</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="832"/>
      <source>Website:</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>custom_line_properties</name>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="27"/>
      <source>Custom Line Properties [*]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="46"/>
      <source>Line Settings:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="84"/>
      <source>Color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="58"/>
      <source>Style:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="43"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="122"/>
      <source>Ends with an arrow:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="138"/>
      <source>Exit Details:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="153"/>
      <source>Origin:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="226"/>
      <source>Destination:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="188"/>
      <source>    Direction/Command:</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>custom_lines</name>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="14"/>
      <source>Custom Line selection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="44"/>
      <source>Choose line format, color and arrow option and then select the exit to start drawing</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="63"/>
      <source>Line Settings:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="97"/>
      <source>Ends with an arrow:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="110"/>
      <source>Style:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="126"/>
      <source>Color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="47"/>
      <source>&lt;p&gt;Selecting an exit immediately proceeds to drawing the first line segment from the centre point of the room.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="60"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head BEFORE then choosing the exit to draw the line for...&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="187"/>
      <source>&lt;p&gt;Select a normal exit to commence drawing a line for it, buttons are shown depressed if they already have such a custom line and disabled if there is not exit in that direction.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="190"/>
      <source>Normal Exits:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="212"/>
      <source>NW</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="235"/>
      <source>N</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="258"/>
      <source>NE</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="297"/>
      <source>UP</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="336"/>
      <source>W</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="346"/>
      <source>E</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="356"/>
      <source>IN</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="382"/>
      <source>OUT</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="392"/>
      <source>SW</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="402"/>
      <source>S</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="412"/>
      <source>SE</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="422"/>
      <source>DOWN</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="444"/>
      <source>&lt;p&gt;Select a special exit to commence drawing a line for it, the first column is checked if the exit already has such a custom line.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="503"/>
      <source>&lt;p&gt;Indicates if there is already a custom line for this special exit, will be replaced if the exit is selected.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="514"/>
      <source>&lt;p&gt;The room this special exit leads to.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="525"/>
      <source>&lt;p&gt;The command or LUA script that goes to the given room.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="561"/>
      <source>&lt;p&gt;To remove a custom line: cancel this dialog, select the line and right-click to obtain a &amp;quot;delete&amp;quot; option.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="447"/>
      <source>Special Exits:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="499"/>
      <source>Has
custom line?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="511"/>
      <source> Destination </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="522"/>
      <source> Command</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="567"/>
      <source>Cancel</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>delete_profile_confirmation</name>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="14"/>
      <source>Confirm permanent profile deletion</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="26"/>
      <source>Are you sure that you&apos;d like to delete this profile? Everything (aliases, triggers, backups, etc) will be gone.

If you are, please type in the profile name as a confirmation:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="54"/>
      <source>Delete</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="61"/>
      <source>Cancel</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dialog</name>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="39"/>
      <source>Status</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="44"/>
      <source>Symbol
(Set Font)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="50"/>
      <source>Symbol
(All Fonts)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="56"/>
      <source>Codepoints</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="61"/>
      <source>Usage
Count</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="67"/>
      <source>Rooms</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>directions</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15471"/>
      <source>north</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15473"/>
      <source>n</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15475"/>
      <source>east</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15477"/>
      <source>e</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15479"/>
      <source>south</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15481"/>
      <source>s</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15483"/>
      <source>west</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15485"/>
      <source>w</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15487"/>
      <source>northeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15489"/>
      <source>ne</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15491"/>
      <source>southeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15493"/>
      <source>se</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15495"/>
      <source>southwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15497"/>
      <source>sw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15499"/>
      <source>northwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15501"/>
      <source>nw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15503"/>
      <source>in</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15505"/>
      <source>i</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15507"/>
      <source>out</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15509"/>
      <source>o</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15511"/>
      <source>up</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15513"/>
      <source>u</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15515"/>
      <source>down</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="15517"/>
      <source>d</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgAboutDialog</name>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="146"/>
      <source>&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Homepage&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://www.mudlet.org/&quot;&gt;www.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Forums&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://forums.mudlet.org/&quot;&gt;forums.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Documentation&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://wiki.mudlet.org/w/Main_Page&quot;&gt;wiki.mudlet.org/w/Main_Page&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#7289DA;&quot;&gt;&lt;b&gt;Discord&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://www.mudlet.org/chat&quot;&gt;discord.gg&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Source code&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet&quot;&gt;github.com/Mudlet/Mudlet&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Features/bugs&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/issues&quot;&gt;github.com/Mudlet/Mudlet/issues&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="155"/>
      <source>Original author, original project lead, Mudlet core coding, retired.</source>
      <comment>about:Heiko</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="158"/>
      <source>GUI design and initial feature planning. He is responsible for the project homepage and the user manual. Maintainer of the Windows, macOS, Ubuntu and generic Linux installers. Maintains the Mudlet wiki, Lua API, and handles project management, public relations &amp;amp; user help. With the project from the very beginning and is an official spokesman of the project. Since the retirement of Heiko, he has become the head of the Mudlet project.</source>
      <comment>about:Vadi</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="165"/>
      <source>After joining in 2013, he has been poking various bits of the C++ code and GUI with a pointy stick; subsequently trying to patch over some of the holes made/found. Most recently he has been working on I18n and L10n for Mudlet 4.0.0 so if you are playing Mudlet in a language other than American English you will be seeing the results of him getting fed up with the spelling differences between what was being used and the British English his brain wanted to see.</source>
      <comment>about:SlySven</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="172"/>
      <source>Former maintainer of the early Windows and Apple OSX packages. He also administers our server and helps the project in many ways.</source>
      <comment>about:demonnic</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="176"/>
      <source>Contributed many improvements to Mudlet&apos;s db: interface, event system, and has been around the project for a very long while assisting users.</source>
      <comment>about:keneanung</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="180"/>
      <source>Does a ton of work in making Mudlet, the website and the wiki accessible to you regardless of the language you speak - and promoting our genre!</source>
      <comment>about:Leris</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="184"/>
      <source>Contributions to the Travis integration, CMake and Visual C++ build, a lot of code quality and memory management improvements.</source>
      <comment>about:ahmedcharles</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="188"/>
      <source>Developed a shared module system that allows script packages to be shared among profiles, a UI for viewing Lua variables, improvements in the mapper and all around.</source>
      <comment>about:Chris7</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="192"/>
      <source>Developed the first version of our Mac OSX installer. He is the former maintainer of the Mac version of Mudlet.</source>
      <comment>about:Ben Carlsen</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="196"/>
      <source>Joined in December 2009 though he&apos;s been around much longer. Contributed to the Lua API and is the former maintainer of the Lua API.</source>
      <comment>about:Ben Smith</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="200"/>
      <source>Joined in December 2009. He has contributed to the Lua API, submitted small bugfix patches and has helped with release management of 1.0.5.</source>
      <comment>about:Blaine von Roeder</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="204"/>
      <source>Developed the original cmake build script and he has committed a number of patches.</source>
      <comment>about:Bruno Bigras</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="207"/>
      <source>Contributed to the Lua API.</source>
      <comment>about:Carter Dewey</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="210"/>
      <source>Developed the Vyzor GUI Manager for Mudlet.</source>
      <comment>about:Oneymus</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="213"/>
      <source>Worked wonders in rejuvenating our Website in 2017 but who prefers a little anonymity - if you are a &lt;i&gt;SpamBot&lt;/i&gt; you will not get onto our Fora now. They have also made some useful C++ core code contributions and we look forward to future reviews on and work in that area.</source>
      <comment>about:TheFae</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="218"/>
      <source>Joining us 2017 they have given us some useful C++ and Lua contributions.</source>
      <comment>about:Dicene</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="221"/>
      <source>Contributed the Geyser layout manager for Mudlet in March 2010. It is written in Lua and aims at simplifying user GUI scripting.</source>
      <comment>about:James Younquist</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="225"/>
      <source>Helped develop and debug the Lua API.</source>
      <comment>about:John Dahlström</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="228"/>
      <source>Contributed several improvements and new features for Geyser.</source>
      <comment>about:Beliaar</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="231"/>
      <source>The original author of our Windows installer.</source>
      <comment>about:Leigh Stillard</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="234"/>
      <source>Worked on the manual, forum help and helps with GUI design and documentation.</source>
      <comment>about:Maksym Grinenko</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="237"/>
      <source>Developed a database Lua API that allows for far easier use of databases and one of the original OSX installers.</source>
      <comment>about:Stephen Hansen</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="240"/>
      <source>Designed our beautiful logo, our splash screen, the about dialog, our website, several icons and badges. Visit his homepage at &lt;a href=&quot;http://thorwil.wordpress.com/&quot;&gt;thorwil.wordpress.com&lt;/a&gt;.</source>
      <comment>about:Thorsten Wilms</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="249"/>
      <source>&lt;p&gt;Others too, have make their mark on different aspects of the Mudlet project and if they have not been mentioned here it is by no means intentional! For past contributors you may see them mentioned in the &lt;b&gt;&lt;a href=&quot;https://launchpad.net/~mudlet-makers/+members#active&quot;&gt;Mudlet Makers&lt;/a&gt;&lt;/b&gt; list (on our former bug-tracking site), or for on-going contributors they may well be included in the &lt;b&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/graphs/contributors&quot;&gt;Contributors&lt;/a&gt;&lt;/b&gt; list on GitHub.&lt;/p&gt;
&lt;br&gt;
&lt;p&gt;Many icons are taken from the &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;&lt;u&gt;KDE4 oxygen icon theme&lt;/u&gt;&lt;/b&gt;&lt;/span&gt; at &lt;a href=&quot;https://web.archive.org/web/20130921230632/http://www.oxygen-icons.org/&quot;&gt;www.oxygen-icons.org &lt;sup&gt;{wayback machine archive}&lt;/sup&gt;&lt;/a&gt; or &lt;a href=&quot;http://www.kde.org&quot;&gt;www.kde.org&lt;/a&gt;.  Most of the rest are from Thorsten Wilms, or from Stephen Lyons combining bits of Thorsten&apos;s work with the other sources.&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Brett Duzevich&lt;/b&gt;&lt;/span&gt; and &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Ronny Ho&lt;/b&gt;&lt;/span&gt;. They have contributed many good ideas and thus helped improve the scripting framework substantially.&lt;/p&gt;
&lt;p&gt;Thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Tomas Mecir&lt;/b&gt;&lt;/span&gt; (&lt;span style=&quot;color:#0000ff;&quot;&gt;kmuddy@kmuddy.com&lt;/span&gt;) who brought us all together and inspired us with his KMuddy project. Mudlet is using some of the telnet code he wrote for his KMuddy project (&lt;a href=&quot;https://cgit.kde.org/kmuddy.git/&quot;&gt;cgit.kde.org/kmuddy.git/&lt;/a&gt;).&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Nick Gammon&lt;/b&gt;&lt;/span&gt; (&lt;a href=&quot;http://www.gammon.com.au/mushclient/mushclient.htm&quot;&gt;www.gammon.com.au/mushclient/mushclient.htm&lt;/a&gt;) for giving us some valued pieces of advice.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="294"/>
      <source>&lt;p&gt;Mudlet was originally written by Heiko Köhn, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet is released under the GPL license version 2, which is reproduced below:&lt;/p&gt;</source>
      <comment>For non-english language versions please append a translation of the following to explain why the GPL is NOT reproduced in the relevant language: &apos;but only the English form is considered the official version of the license, so the following is reproduced in that language:&apos; to replace &apos;which is reproduced below:&apos;...</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="590"/>
      <source>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; is built upon the shoulders of other projects in the FOSS world; as well as using many GPL components we also make use of some third-party software with other licenses:&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="832"/>
      <source>&lt;h2&gt;&lt;u&gt;Communi IRC Library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008-2020 The Communi Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="835"/>
      <source>&lt;p&gt;Parts of &lt;tt&gt;irctextformat.cpp&lt;/t&gt; code come from Konversation and are copyrighted to:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="841"/>
      <source>&lt;h2&gt;&lt;u&gt;Lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="844"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaFileSystem&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2003-2020, Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="847"/>
      <source>&lt;h2&gt;&lt;u&gt;Lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="851"/>
      <source>&lt;h2&gt;&lt;u&gt;Luautf8 - A UTF-8 support module for Lua.&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2018 Xavier Wang&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="854"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaSql-Sqlite3 - Database connectivity for the Lua programming language (Sqlite3 component).&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2003-2019, The Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="857"/>
      <source>&lt;h2&gt;&lt;u&gt;Lrexlib-pcre -  Regular expression library binding (PCRE flavour).&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © Reuben Thomas 2000-2020&lt;br&gt;Copyright © Shmuel Zeigerman 2004-2020 &lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="868"/>
      <source>&lt;h2&gt;&lt;u&gt;Edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="886"/>
      <source>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [broken link]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archived version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="893"/>
      <source>&lt;h2&gt;&lt;u&gt;Dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="914"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim Peretokin&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="926"/>
      <source>&lt;h2&gt;&lt;u&gt;singleshot_connect.h - part of KDToolBox&lt;/u&gt;&lt;br&gt;Github: &lt;a href=&quot;https://github.com/KDAB/KDToolBox&quot;&gt;KDToolBox&lt;/a&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2020-2021 Klarälvdalens Datakonsult AB, a KDAB Group company, &amp;lt;info@kdab.comF&amp;gt;.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="930"/>
      <source>&lt;h2&gt;&lt;u&gt;utf8_filenames.lua - modifies standard Lua functions so that they work with UTF-8 filenames on Windows&lt;/u&gt;&lt;br&gt;&lt;a href=&quot;https://gist.github.com/Egor-Skriptunoff/2458547aa3b9210a8b5f686ac08ecbf0&quot;&gt;Github GIST&lt;/a&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2019 Egor-Skriptunoff&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="862"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="871"/>
      <source>The &lt;b&gt;edbee-lib&lt;/b&gt; widget itself incorporates other components with licences that must be noted as well, they are:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="873"/>
      <source>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="878"/>
      <source>&lt;h2&gt;&lt;u&gt;Oniguruma LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="882"/>
      <source>&lt;h2&gt;&lt;u&gt;Ruby BSDL&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1993-2013 Yukihiro Matsumoto.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="896"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="906"/>
      <source>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="920"/>
      <source>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="923"/>
      <source>&lt;h2&gt;&lt;u&gt;QtKeyChain - Platform-independent Qt API for storing passwords securely&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1090"/>
      <source>
                          These formidable folks will be fondly remembered forever&lt;br&gt;for their generous financial support on &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s patreon&lt;/a&gt;:
                          </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1101"/>
      <source>Technical information:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1102"/>
      <source>Version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1103"/>
      <source>OS</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1104"/>
      <source>CPU</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgColorTrigger</name>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="52"/>
      <source>More colors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="54"/>
      <source>Click to access all 256 ANSI colors.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="64"/>
      <source>Default</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="59"/>
      <source>Click to make the color trigger ignore the text&apos;s background color - however choosing this for both foreground and background is an error.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="60"/>
      <source>Click to make the color trigger ignore the text&apos;s foreground color - however choosing this for both foreground and background is an error.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="66"/>
      <source>Click to make the color trigger when the text&apos;s background color has not been modified from its normal value.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="67"/>
      <source>Click to make the color trigger when the text&apos;s foreground color has not been modified from its normal value.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="71"/>
      <source>Click a color to make the trigger fire only when the text&apos;s background color matches the color number indicated.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="72"/>
      <source>Click a color to make the trigger fire only when the text&apos;s foreground color matches the color number indicated.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="77"/>
      <source>Black</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="78"/>
      <source>Red</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="79"/>
      <source>Green</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="80"/>
      <source>Yellow</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="81"/>
      <source>Blue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="82"/>
      <source>Magenta</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="83"/>
      <source>Cyan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="84"/>
      <source>White (Light gray)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="86"/>
      <source>Light black (Dark gray)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="87"/>
      <source>Light red</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="88"/>
      <source>Light green</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="89"/>
      <source>Light yellow</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="90"/>
      <source>Light blue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="91"/>
      <source>Light magenta</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="92"/>
      <source>Light cyan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="93"/>
      <source>Light white</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="218"/>
      <source>%1 [%2]</source>
      <comment>Color Trigger dialog button in basic 16-color set, the first value is the name of the color, the second is the ANSI color number - for most languages modification is not likely to be needed - this text is used in two places</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="375"/>
      <source>All color options are showing.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgConnectionProfiles</name>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="99"/>
      <source>Connect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="204"/>
      <source>Characters password. Note that the password is not encrypted in storage</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="254"/>
      <source>Game name: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1024"/>
      <source>This profile is currently loaded - close it before changing the connection parameters.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1733"/>
      <source>The %1 character is not permitted. Use one of the following:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1756"/>
      <source>You have to enter a number. Other characters are not permitted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1745"/>
      <source>This profile name is already in use.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="512"/>
      <source>Could not rename your profile data on the computer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="111"/>
      <source>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Welcome to Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Click on one of the games on the list to play.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;To play a game not in the list, click on %1 &lt;span style=&quot; color:#555753;&quot;&gt;New&lt;/span&gt;, fill in the &lt;i&gt;Profile Name&lt;/i&gt;, &lt;i&gt;Server address&lt;/i&gt;, and &lt;i&gt;Port&lt;/i&gt; fields in the &lt;i&gt;Required &lt;/i&gt; area.&lt;/p&gt;&lt;p&gt;After that, click %2 &lt;span style=&quot; color:#555753;&quot;&gt;Connect&lt;/span&gt; to play.&lt;/p&gt;&lt;p&gt;Have fun!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;The Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</source>
      <comment>Welcome message. Both %1 and %2 may be replaced by icons when this text is used.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="101"/>
      <source>Offline</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="124"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1444"/>
      <source>Copy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="126"/>
      <source>Copy settings only</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="134"/>
      <source>copy profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="135"/>
      <source>copy the entire profile to new one that will require a different new name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="139"/>
      <source>copy profile settings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="140"/>
      <source>copy the settings and some other parts of the profile to a new one that will require a different new name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="202"/>
      <source>Characters password, stored securely in the computer&apos;s credential manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="251"/>
      <source>Click to load but not connect the selected profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="252"/>
      <source>Click to load and connect the selected profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="253"/>
      <source>Need to have a valid profile name, game server address and port before this button can be enabled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="255"/>
      <source>Button to select a mud game to play, double-click it to connect and start playing it.</source>
      <comment>Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="520"/>
      <source>Could not create the new profile folder on your computer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="556"/>
      <source>new profile name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="669"/>
      <source>Deleting &apos;%1&apos;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1071"/>
      <source>Discord integration not available on this platform</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1077"/>
      <source>Discord integration not supported by game</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1080"/>
      <source>Check to enable Discord integration</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1318"/>
      <source>Reset icon</source>
      <comment>Reset the custom picture for this profile in the connection dialog and show the default one instead</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1321"/>
      <source>Set custom icon</source>
      <comment>Set a custom picture to show for the profile in the connection dialog</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1325"/>
      <source>Set custom color</source>
      <comment>Set a custom color to show for the profile in the connection dialog</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1338"/>
      <source>Select custom image for profile (should be 120x30)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1338"/>
      <source>Images (%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1426"/>
      <source>Copying...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1766"/>
      <source>Port number must be above zero and below 65535.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1786"/>
      <source>Mudlet can not load support for secure connections.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1808"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1818"/>
      <source>Please enter the URL or IP address of the Game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1829"/>
      <source>SSL connections require the URL of the Game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1846"/>
      <source>Load profile without connecting.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1862"/>
      <source>Please set a valid profile name, game server address and the game port before loading.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1867"/>
      <source>Please set a valid profile name, game server address and the game port before connecting.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1921"/>
      <source>Click to hide the password; it will also hide if another profile is selected.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1925"/>
      <source>Click to reveal the password for this profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1774"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1777"/>
      <source>Mudlet is not configured for secure connections.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgIRC</name>
    <message>
      <location filename="../src/dlgIRC.cpp" line="104"/>
      <source>%1 closed their client.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="116"/>
      <source>Mudlet IRC Client - %1 - %2 on %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="130"/>
      <source>$ Starting Mudlet IRC Client...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="131"/>
      <source>$ Host: %1:%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="132"/>
      <source>$ Nick: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="133"/>
      <source>$ Auto-Join Channels: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="134"/>
      <source>$ This client supports Auto-Completion using the Tab key.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="135"/>
      <source>$ Type &lt;b&gt;/help&lt;/b&gt; for commands or &lt;b&gt;/help [command]&lt;/b&gt; for command syntax.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="194"/>
      <source>Restarting IRC Client</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="371"/>
      <source>[Error] MSGLIMIT requires &lt;limit&gt; to be a whole number greater than zero!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="401"/>
      <source>[HELP] Available Commands: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="403"/>
      <source>[HELP] Syntax: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="411"/>
      <source>! Connected to %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="412"/>
      <source>! Joining %1...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="417"/>
      <source>! Connecting %1...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="422"/>
      <source>! Disconnected from %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="488"/>
      <source>[ERROR] Syntax: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="490"/>
      <source>[ERROR] Unknown command: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="636"/>
      <source>! The Nickname %1 is reserved. Automatically changing Nickname to: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="647"/>
      <source>Your nick has changed.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgMapLabel</name>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="33"/>
      <source>Create label</source>
      <comment>Create label dialog title</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="75"/>
      <source>Foreground color</source>
      <comment>2D mapper create label color dialog title</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="95"/>
      <source>Background color</source>
      <comment>2D mapper create label color dialog title</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="114"/>
      <source>Label font</source>
      <comment>2D mapper create label font dialog title</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="131"/>
      <source>Select image</source>
      <comment>2D Mapper create label file dialog title</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgMapper</name>
    <message>
      <location filename="../src/dlgMapper.cpp" line="377"/>
      <source>None</source>
      <comment>Don&apos;t show the map overlay, &apos;none&apos; meaning no map overlay styled are enabled</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgModuleManager</name>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="49"/>
      <source>Module Manager - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="67"/>
      <source>Module Name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="67"/>
      <source>Priority</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="67"/>
      <source>Sync</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="67"/>
      <source>Module Location</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="107"/>
      <source>Checking this box will cause the module to be saved and &lt;i&gt;resynchronised&lt;/i&gt; across all sessions that share it when the &lt;i&gt;Save Profile&lt;/i&gt; button is clicked in the Editor or if it is saved at the end of the session.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="137"/>
      <source>Load Mudlet Module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="144"/>
      <source>Load Mudlet Module:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="144"/>
      <source>Cannot read file %1:
%2.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgPackageExporter</name>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="29"/>
      <source>Package name here</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="36"/>
      <source>or</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="82"/>
      <source>Check items to export</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="109"/>
      <source>(optional) add icon, description, and more</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="156"/>
      <source>Author</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="175"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="268"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="305"/>
      <source>optional</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="213"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="235"/>
      <source>Icon size of 512x512 recommended</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="245"/>
      <source>512x512 recommended</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="288"/>
      <source>optional. Markdown supported, and you can add images with drag and drop</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="182"/>
      <source>Icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="66"/>
      <location filename="../src/dlgPackageExporter.cpp" line="1464"/>
      <source>Select what to export</source>
      <comment>This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="216"/>
      <source>Add icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="255"/>
      <source>Short description</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="275"/>
      <source>Description</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="295"/>
      <source>Version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="312"/>
      <source>Does this package make use of other packages? List them here as requirements</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="315"/>
      <source>Required packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="404"/>
      <source>Include assets (images, sounds, fonts)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="414"/>
      <source>Drag and drop files and folders, or use the browse button below</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="462"/>
      <source>Select files to include in package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="372"/>
      <source>Does this package make use of other packages? List them here as requirements. Press &apos;Delete&apos; to remove a package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="20"/>
      <source>Package Exporter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="512"/>
      <source>Select export location</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="63"/>
      <source>Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="64"/>
      <source>Aliases</source>
      <translation>Aliasnamn</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="65"/>
      <source>Timers</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="66"/>
      <source>Scripts</source>
      <translation>Skript</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="67"/>
      <source>Keys</source>
      <translation>Nycklar</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="68"/>
      <source>Buttons</source>
      <translation>Knappar</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="90"/>
      <source>Export</source>
      <comment>Text for button to perform the package export on the items the user has selected.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="109"/>
      <source>update installed package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="110"/>
      <source>add dependencies</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="170"/>
      <source>Failed to open file &quot;%1&quot; to place into package. Error message was: &quot;%2&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but the code cannot open it.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="180"/>
      <source>Failed to add file &quot;%1&quot; to package. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but cannot be done for some reason.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="326"/>
      <location filename="../src/dlgPackageExporter.cpp" line="328"/>
      <source>Export to %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1031"/>
      <source>Failed to zip up the package. Error is: &quot;%1&quot;.</source>
      <comment>This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageExporter.cpp" line="1459"/>
      <source>Select what to export (%n item(s))</source>
      <comment>This is the text shown at the top of a groupbox when there is %n (one or more) items to export in the Package exporter dialogue; the initial (and when there is no items selected) is a separate text.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="347"/>
      <source>Open Icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="347"/>
      <source>Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="472"/>
      <source>Please enter the package name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="510"/>
      <location filename="../src/dlgPackageExporter.cpp" line="586"/>
      <source>Exporting package...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="530"/>
      <source>Failed to export. Could not open the folder &quot;%1&quot; for writing. Do you have the necessary permissions and free disk-space to write to that folder?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="855"/>
      <source>%1 doesn&apos;t seem to exist anymore - can you double-check it?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="785"/>
      <source>Failed to export. Could not write Mudlet items to the file &quot;%1&quot;.</source>
      <comment>This error message is shown when all the Mudlet items cannot be written to the &apos;packageName&apos;.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="956"/>
      <source>Failed to add directory &quot;%1&quot; to package. Error is: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="999"/>
      <source>Required file &quot;%1&quot; was not found in the staging area. This area contains the Mudlet items chosen for the package, which you selected to be included in the package file. This suggests there may be a problem with that directory: &quot;%2&quot; - Do you have the necessary permissions and free disk-space?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="572"/>
      <source>Package &quot;%1&quot; exported to: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="883"/>
      <source>Failed to open package file. Error is: &quot;%1&quot;.</source>
      <comment>This zipError message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer&apos;s file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1028"/>
      <source>Export cancelled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1440"/>
      <source>Why not &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;upload&lt;/a&gt; your package for other Mudlet users?</source>
      <comment>Only the text outside of the &apos;a&apos; (HTML anchor) tags PLUS the verb &apos;upload&apos; in between them in the source text, (associated with uploading the resulting package to the Mudlet forums) should be translated.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1089"/>
      <source>Where do you want to save the package?</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgPackageManager</name>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="51"/>
      <source>Package Manager (experimental) - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="105"/>
      <source>Import Mudlet Package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="112"/>
      <source>Import Mudlet Package:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="112"/>
      <source>Cannot read file %1:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="176"/>
      <source>Author</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="176"/>
      <source>Version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="176"/>
      <source>Created</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="176"/>
      <source>Dependencies</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageManager.cpp" line="244"/>
      <source>Remove %n package(s)</source>
      <comment>Message on button in package manager to remove one or more (%n is the count of) selected package(s).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="249"/>
      <source>Remove package</source>
      <comment>Message on button in package manager initially and when there is no packages to remove.</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgProfilePreferences</name>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="149"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {1 of 2}).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="141"/>
      <source>Location which will be used to store log files - matching logs will be appended to.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="142"/>
      <source>Select a directory where logs will be saved.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="143"/>
      <source>Reset the directory so that logs are saved to the profile&apos;s &lt;i&gt;log&lt;/i&gt; directory.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="147"/>
      <source>Set a custom name for your log. (New logs are appended if a log file of the same name already exists).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="168"/>
      <source>Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="195"/>
      <source>This will bring up a display showing all the symbols used in the current map and whether they can be drawn using just the specified font, any other font, or not at all.  It also shows the sequence of Unicode &lt;i&gt;code-points&lt;/i&gt; that make up that symbol, so that they can be identified even if they cannot be displayed; also, up to the first thirty two rooms that are using that symbol are listed, which may help to identify any unexpected or odd cases.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="201"/>
      <source>Select the only or the primary font used (depending on &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; setting) to produce the 2D mapper room symbols.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="203"/>
      <source>Using a single font is likely to produce a more consistent style but may cause the &lt;i&gt;font replacement character&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; to show if the font does not have a needed glyph (a font&apos;s individual character/symbol) to represent the grapheme (what is to be represented).  Clearing this checkbox will allow the best alternative glyph from another font to be used to draw that grapheme.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="216"/>
      <source>&lt;p&gt;Some East Asian MUDs may use glyphs (characters) that Unicode classifies as being of &lt;i&gt;Ambiguous&lt;/i&gt; width when drawn in a font with a so-called &lt;i&gt;fixed&lt;/i&gt; pitch; in fact such text is &lt;i&gt;duo-spaced&lt;/i&gt; when not using a proportional font. These symbols can be drawn using either a half or the whole space of a full character. By default Mudlet tries to chose the right width automatically but you can override the setting for each profile.&lt;/p&gt;&lt;p&gt;This control has three settings:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;narrow&lt;/i&gt;&apos; = Draw ambiguous width characters in a single &apos;space&apos;.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;wide&lt;/i&gt;&apos; = Draw ambiguous width characters two &apos;spaces&apos; wide.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use &apos;wide&apos; setting for MUD Server encodings of &lt;b&gt;Big5&lt;/b&gt;, &lt;b&gt;GBK&lt;/b&gt; or &lt;b&gt;GBK18030&lt;/b&gt; and &apos;narrow&apos; for all others.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This is a temporary arrangement and will probably change when Mudlet gains full support for languages other than English.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="299"/>
      <source>%1 (%2% done)</source>
      <comment>%1 is the (not-translated so users of the language can read it!) language name, %2 is percentage done.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="355"/>
      <source>Migrated all passwords to secure storage.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="362"/>
      <source>Migrated %1...</source>
      <comment>This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="366"/>
      <source>Migrated all passwords to profile storage.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="688"/>
      <source>From the dictionary file &lt;tt&gt;%1.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="822"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="824"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="825"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="828"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="829"/>
      <source>Named file (concatenate logs in one file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="862"/>
      <source>Other profiles to Map to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="902"/>
      <source>%1 {Default, recommended}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="913"/>
      <source>%1 {Upgraded, experimental/testing, NOT recommended}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="915"/>
      <source>%1 {Downgraded, for sharing with older version users, NOT recommended}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="924"/>
      <source>2D Map Room Symbol scaling factor:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="944"/>
      <source>Show &quot;%1&quot; in the map area selection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="985"/>
      <source>%1 (*Error, report to Mudlet Makers*)</source>
      <comment>The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1108"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="3981"/>
      <source>Profile preferences - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1420"/>
      <source>Profile preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1685"/>
      <source>Pick color</source>
      <comment>Generic pick color dialog title</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2159"/>
      <source>Load Mudlet map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2176"/>
      <source>Loading map - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2188"/>
      <source>Loaded map from %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2190"/>
      <source>Could not load map from %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2213"/>
      <source>Save Mudlet map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2235"/>
      <source>Saving map - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2252"/>
      <source>Saved map to %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2254"/>
      <source>Could not save map to %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2284"/>
      <source>Migrating passwords to secure storage...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2291"/>
      <source>Migrating passwords to profiles...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2323"/>
      <source>[ ERROR ] - Unable to use or create directory to store map for other profile &quot;%1&quot;.
Please check that you have permissions/access to:
&quot;%2&quot;
and there is enough space. The copying operation has failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2330"/>
      <source>Creating a destination directory failed...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2414"/>
      <source>Backing up current map - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2424"/>
      <source>Could not backup the map - saving it failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2449"/>
      <source>Could not copy the map - failed to work out which map file we just saved the map as!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2461"/>
      <source>Copying over map to %1 - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2467"/>
      <source>Could not copy the map to %1 - unable to copy the new map file over.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2471"/>
      <source>Map copied successfully to other profile %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2483"/>
      <source>Map copied, now signalling other profiles to reload it.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2519"/>
      <source>Where should Mudlet save log files?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2928"/>
      <source>%1 selected - press to change</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2931"/>
      <source>Press to pick destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3172"/>
      <source>Could not update themes: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3175"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3294"/>
      <source>{missing, possibly recently deleted trigger item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3297"/>
      <source>{missing, possibly recently deleted alias item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3300"/>
      <source>{missing, possibly recently deleted script item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3303"/>
      <source>{missing, possibly recently deleted timer item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3306"/>
      <source>{missing, possibly recently deleted key item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3309"/>
      <source>{missing, possibly recently deleted button item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3433"/>
      <source>The room symbol will appear like this if only symbols (glyphs) from the specific font are used.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3438"/>
      <source>The room symbol will appear like this if symbols (glyphs) from any font can be used.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3478"/>
      <source>How many rooms in the whole map have this symbol.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3496"/>
      <source>The rooms with this symbol, up to a maximum of thirty-two, if there are more than this, it is indicated but they are not shown.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3504"/>
      <source>The symbol can be made entirely from glyphs in the specified font.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3522"/>
      <source>The symbol cannot be drawn using any of the fonts in the system, either an invalid string was entered as the symbol for the indicated rooms or the map was created on a different systems with a different set of fonts available to use. You may be able to correct this by installing an additional font using whatever method is appropriate for this system or by editing the map to use a different symbol. It may be possible to do the latter via a lua script using the &lt;i&gt;getRoomChar&lt;/i&gt; and &lt;i&gt;setRoomChar&lt;/i&gt; functions.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4113"/>
      <source>Set outer color of player room mark.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4114"/>
      <source>Set inner color of player room mark.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="144"/>
      <source>&lt;p&gt;This option sets the format of the log name.&lt;/p&gt;&lt;p&gt;If &lt;i&gt;Named file&lt;/i&gt; is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="189"/>
      <source>&lt;p&gt;A timer with a short interval will quickly fill up the &lt;i&gt;Central Debug Console&lt;/i&gt; windows with messages that it ran correctly on &lt;i&gt;each&lt;/i&gt; occasion it is called.  This (per profile) control adjusts a threshold that will hide those messages in just that window for those timers which run &lt;b&gt;correctly&lt;/b&gt; when the timer&apos;s interval is less than this setting.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Any timer script that has errors will still have its error messages reported whatever the setting.&lt;/u&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="208"/>
      <source>&lt;p&gt;If &lt;b&gt;not&lt;/b&gt; checked Mudlet will only react to the first matching keybinding (combination of key and modifiers) even if more than one of them is set to be active. This means that a temporary keybinding (not visible in the Editor) created by a script or package may be used in preference to a permanent one that is shown and is set to be active. If checked then all matching keybindings will be run.&lt;/p&gt;&lt;p&gt;&lt;i&gt;It is recommended to not enable this option if you need to maintain compatibility with scripts or packages for Mudlet versions prior to &lt;b&gt;3.9.0&lt;/b&gt;.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="228"/>
      <source>&lt;p&gt;Enable a context (right click) menu action on any console/user window that, when the mouse cursor is hovered over it, will display the UTF-16 and UTF-8 items that make up each Unicode codepoint on the &lt;b&gt;first&lt;/b&gt; line of any selection.&lt;/p&gt;&lt;p&gt;This utility feature is intended to help the user identify any grapheme (visual equivalent to a &lt;i&gt;character&lt;/i&gt;) that a Game server may send even if it is composed of multiple bytes as any non-ASCII character will be in the Lua sub-system which uses the UTF-8 encoding system.&lt;p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="235"/>
      <source>&lt;p&gt;Some Desktop Environments tell Qt applications like Mudlet whether they should shown icons on menus, others, however do not. This control allows the user to override the setting, if needed, as follows:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;off&lt;/i&gt;&apos; = Prevent menus from being drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;on&lt;/i&gt;&apos; = Allow menus to be drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use the setting that the system provides.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This setting is only processed when individual menus are created and changes may not propagate everywhere until Mudlet is restarted.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="655"/>
      <source>Mudlet dictionaries:</source>
      <comment>On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="657"/>
      <source>System dictionaries:</source>
      <comment>On *nix systems where we find the system ones we use them.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="690"/>
      <source>%1 - not recognised</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="691"/>
      <source>&lt;p&gt;Mudlet does not recognise the code &quot;%1&quot;, please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!&lt;/p&gt;&lt;p&gt;The file &lt;tt&gt;%2.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file) is still usable.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="716"/>
      <source>No Hunspell dictionary files found, spell-checking will not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2151"/>
      <source>Any map file (*.dat *.json *.xml)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2152"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="2208"/>
      <source>Mudlet binary map (*.dat)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2153"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="2209"/>
      <source>Mudlet JSON map (*.json)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2154"/>
      <source>Mudlet XML map (*.xml)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2155"/>
      <source>Any file (*)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3466"/>
      <source>&lt;p&gt;These are the sequence of hexadecimal numbers that are used by the Unicode consortium to identify the graphemes needed to create the symbol.  These numbers can be utilised to determine precisely what is to be drawn even if some fonts have glyphs that are the same for different codepoints or combination of codepoints.&lt;/p&gt;&lt;p&gt;Character entry utilities such as &lt;i&gt;charmap.exe&lt;/i&gt; on &lt;i&gt;Windows&lt;/i&gt; or &lt;i&gt;gucharmap&lt;/i&gt; on many Unix type operating systems will also use these numbers which cover everything from U+0020 {Space} to U+10FFFD the last usable number in the &lt;i&gt;Private Use Plane 16&lt;/i&gt; via most of the written marks that humanity has ever made.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3490"/>
      <source>more - not shown...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3513"/>
      <source>&lt;p&gt;The symbol cannot be made entirely from glyphs in the specified font, but, using other fonts in the system, it can. Either un-check the &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; option or try and choose another font that does have the needed glyphs.&lt;/p&gt;&lt;p&gt;&lt;i&gt;You need not close this table to try another font, changing it on the main preferences dialogue will update this table after a slight delay.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3614"/>
      <source>Large icon</source>
      <comment>Discord Rich Presence large icon</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3615"/>
      <source>Detail</source>
      <comment>Discord Rich Presence detail</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3617"/>
      <source>Small icon</source>
      <comment>Discord Rich Presence small icon</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3618"/>
      <source>State</source>
      <comment>Discord Rich Presence state</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3620"/>
      <source>Party size</source>
      <comment>Discord Rich Presence party size</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3621"/>
      <source>Party max</source>
      <comment>Discord Rich Presence maximum party size</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3622"/>
      <source>Time</source>
      <comment>Discord Rich Presence time until or time elapsed</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3657"/>
      <source>Map symbol usage - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3734"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3735"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3736"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3737"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3740"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3741"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3742"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3743"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4199"/>
      <source>Deleting map - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4211"/>
      <source>Deleted map.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgRoomExits</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1053"/>
      <location filename="../src/dlgRoomExits.cpp" line="1060"/>
      <location filename="../src/dlgRoomExits.cpp" line="1069"/>
      <location filename="../src/dlgRoomExits.cpp" line="1076"/>
      <source>&lt;b&gt;Room&lt;/b&gt; Weight of destination: %1.</source>
      <comment>Bold HTML tags are used to emphasis that the value is destination room&apos;s weight whether overridden by a non-zero exit weight here or not.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="258"/>
      <source>(roomID)</source>
      <comment>Placeholder, if no roomID is set for an exit.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="259"/>
      <source>(command or Lua script)</source>
      <comment>Placeholder, if a special exit has no name/script set.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="447"/>
      <location filename="../src/dlgRoomExits.cpp" line="451"/>
      <location filename="../src/dlgRoomExits.cpp" line="988"/>
      <source>Set the number of the room that this special exit goes to.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="453"/>
      <location filename="../src/dlgRoomExits.cpp" line="1554"/>
      <source>Prevent a route being created via this exit, equivalent to an infinite exit weight.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="458"/>
      <location filename="../src/dlgRoomExits.cpp" line="1560"/>
      <source>Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="461"/>
      <location filename="../src/dlgRoomExits.cpp" line="1569"/>
      <source>No door symbol is drawn on 2D Map for this exit (only functional choice currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="463"/>
      <location filename="../src/dlgRoomExits.cpp" line="1570"/>
      <source>Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="465"/>
      <location filename="../src/dlgRoomExits.cpp" line="1571"/>
      <source>Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="467"/>
      <location filename="../src/dlgRoomExits.cpp" line="1572"/>
      <source>Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="983"/>
      <source>Entered number is invalid. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="984"/>
      <source>Set the number of the room that this special exit leads to.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="992"/>
      <source>A positive roomID of the room that this special exit leads to is expected here. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="997"/>
      <source>No command or Lua script entered, if left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="999"/>
      <source>(Lua scripts for those profiles using the &lt;tt&gt;mudlet-mapper&lt;/tt&gt; package need to be prefixed with &quot;script:&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1051"/>
      <source>Exit to &quot;%1&quot; in area: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1067"/>
      <source>Exit to unnamed room in area: &quot;%1&quot;, is valid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1075"/>
      <source>Exit to unnamed room is valid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1168"/>
      <source>Entered number is invalid, set the number of the room northwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1169"/>
      <location filename="../src/dlgRoomExits.cpp" line="1277"/>
      <location filename="../src/dlgRoomExits.cpp" line="1498"/>
      <source>Set the number of the room northwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1177"/>
      <source>Entered number is invalid, set the number of the room north of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1178"/>
      <location filename="../src/dlgRoomExits.cpp" line="1285"/>
      <location filename="../src/dlgRoomExits.cpp" line="1500"/>
      <source>Set the number of the room north of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1186"/>
      <source>Entered number is invalid, set the number of the room northeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1187"/>
      <location filename="../src/dlgRoomExits.cpp" line="1293"/>
      <location filename="../src/dlgRoomExits.cpp" line="1502"/>
      <source>Set the number of the room northeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1195"/>
      <source>Entered number is invalid, set the number of the room up from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1196"/>
      <location filename="../src/dlgRoomExits.cpp" line="1301"/>
      <location filename="../src/dlgRoomExits.cpp" line="1504"/>
      <source>Set the number of the room up from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1204"/>
      <source>Entered number is invalid, set the number of the room west of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1205"/>
      <location filename="../src/dlgRoomExits.cpp" line="1309"/>
      <location filename="../src/dlgRoomExits.cpp" line="1506"/>
      <source>Set the number of the room west of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1213"/>
      <source>Entered number is invalid, set the number of the room east of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1214"/>
      <location filename="../src/dlgRoomExits.cpp" line="1317"/>
      <location filename="../src/dlgRoomExits.cpp" line="1508"/>
      <source>Set the number of the room east of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1222"/>
      <source>Entered number is invalid, set the number of the room down from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1223"/>
      <location filename="../src/dlgRoomExits.cpp" line="1325"/>
      <location filename="../src/dlgRoomExits.cpp" line="1510"/>
      <source>Set the number of the room down from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1231"/>
      <source>Entered number is invalid, set the number of the room southwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1232"/>
      <location filename="../src/dlgRoomExits.cpp" line="1333"/>
      <location filename="../src/dlgRoomExits.cpp" line="1512"/>
      <source>Set the number of the room southwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1240"/>
      <source>Entered number is invalid, set the number of the room south of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1241"/>
      <location filename="../src/dlgRoomExits.cpp" line="1341"/>
      <location filename="../src/dlgRoomExits.cpp" line="1514"/>
      <source>Set the number of the room south of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1249"/>
      <source>Entered number is invalid, set the number of the room southeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1250"/>
      <location filename="../src/dlgRoomExits.cpp" line="1349"/>
      <location filename="../src/dlgRoomExits.cpp" line="1516"/>
      <source>Set the number of the room southeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1258"/>
      <source>Entered number is invalid, set the number of the room in from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1259"/>
      <location filename="../src/dlgRoomExits.cpp" line="1357"/>
      <location filename="../src/dlgRoomExits.cpp" line="1518"/>
      <source>Set the number of the room in from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1267"/>
      <source>Entered number is invalid, set the number of the room out from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1268"/>
      <location filename="../src/dlgRoomExits.cpp" line="1365"/>
      <location filename="../src/dlgRoomExits.cpp" line="1520"/>
      <source>Set the number of the room out from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1058"/>
      <source>Exit to &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1140"/>
      <location filename="../src/dlgRoomExits.cpp" line="1456"/>
      <source>Clear the stub exit for this exit to enter an exit roomID.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1488"/>
      <source>Exits for room: &quot;%1&quot; [*]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1490"/>
      <source>Exits for room Id: %1 [*]</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgRoomSymbol</name>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="80"/>
      <source>The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</source>
      <comment>This is for when applying a new room symbol to one or more rooms and some have the SAME symbol (others may have none) at present, %n is the total number of rooms involved and is at least two. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="91"/>
      <source>The symbol is &quot;%1&quot; in the selected room,
delete this to clear the symbol or replace
it with a new symbol for this room:</source>
      <comment>This is for when applying a new room symbol to one room. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="100"/>
      <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
      <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="134"/>
      <source>%1 {count:%2}</source>
      <comment>Everything after the first parameter (the &apos;%1&apos;) will be removed by processing it as a QRegularExpression programmatically, ensure the translated text has ` {` immediately after the &apos;%1&apos;, and &apos;}&apos; as the very last character, so that the right portion can be extracted if the user clicks on this item when it is shown in the QComboBox it is put in.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="205"/>
      <source>Pick color</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgTriggerEditor</name>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="211"/>
      <source>-- Enter your lua code here
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="291"/>
      <source>*** starting new session ***
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="379"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5840"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8376"/>
      <source>Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="380"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="381"/>
      <source>Show Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="409"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5864"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8388"/>
      <source>Buttons</source>
      <translation>Knappar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="410"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="411"/>
      <source>Show Buttons</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="384"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8378"/>
      <source>Aliases</source>
      <translation>Aliasnamn</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="385"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="386"/>
      <source>Show Aliases</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="394"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8382"/>
      <source>Timers</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="395"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="396"/>
      <source>Show Timers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="389"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5852"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8380"/>
      <source>Scripts</source>
      <translation>Skript</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="390"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="391"/>
      <source>Show Scripts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="399"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8384"/>
      <source>Keys</source>
      <translation>Nycklar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="400"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="401"/>
      <source>Show Keybindings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="404"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="6252"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8386"/>
      <source>Variables</source>
      <translation>Variabler</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="405"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="406"/>
      <source>Show Variables</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="431"/>
      <source>Activate</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="432"/>
      <source>Toggle Active or Non-Active Mode for Triggers, Scripts etc.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="442"/>
      <source>Add Item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="443"/>
      <source>Add new Trigger, Script, Alias or Filter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="446"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="448"/>
      <source>Delete Item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="447"/>
      <source>Delete Trigger, Script, Alias or Filter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="454"/>
      <source>Add Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="455"/>
      <source>Add new Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="458"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8366"/>
      <source>Save Item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8367"/>
      <source>Ctrl+S</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="462"/>
      <source>Saves the selected trigger, script, alias, etc, causing new changes to take effect - does not save to disk though...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="465"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8831"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8837"/>
      <source>Copy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="469"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="470"/>
      <source>Copy the trigger/script/alias/etc</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="479"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8832"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8838"/>
      <source>Paste</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="483"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="484"/>
      <source>Paste triggers/scripts/aliases/etc from the clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="498"/>
      <source>Import</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="502"/>
      <source>Export</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="506"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8368"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8673"/>
      <source>Save Profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8369"/>
      <source>Ctrl+Shift+S</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="518"/>
      <source>Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also &quot;synchronizes&quot; modules that are so marked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="521"/>
      <source>Save Profile As</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="420"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8392"/>
      <source>Statistics</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="69"/>
      <source>&lt;p&gt;Alias react on user input. To add a new alias:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define an input &lt;strong&gt;pattern&lt;/strong&gt; either literally or with a Perl regular expression.&lt;/li&gt;&lt;li&gt;Define a &apos;substitution&apos; &lt;strong&gt;command&lt;/strong&gt; to send to the game in clear text &lt;strong&gt;instead of the alias pattern&lt;/strong&gt;, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the alias.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;That&apos;s it! If you&apos;d like to be able to create aliases from the input line, there are a &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=22609&apos;&gt;couple&lt;/a&gt; of &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=16462&apos;&gt;packages&lt;/a&gt; that can help you.&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Aliases&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="77"/>
      <source>&lt;p&gt;Triggers react on game output. To add a new trigger:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;pattern&lt;/strong&gt; that you want to trigger on.&lt;/li&gt;&lt;li&gt;Select the appropriate pattern &lt;strong&gt;type&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the trigger.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;That&apos;s it! If you&apos;d like to be able to create triggers from the input line, there are a &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=22609&apos;&gt;couple&lt;/a&gt; of &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=16462&apos;&gt;packages&lt;/a&gt; that can help you.&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Triggers&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="142"/>
      <source>%1 - Editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="425"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8394"/>
      <source>Debug</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="459"/>
      <source>&lt;p&gt;Saves the selected item. (Ctrl+S)&lt;/p&gt;&lt;p&gt;Saving causes any changes to the item to take effect. It will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="508"/>
      <source>&lt;p&gt;Saves your profile. (Ctrl+Shift+S)&lt;/p&gt;&lt;p&gt;Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.&lt;/p&gt;&lt;p&gt;It also makes a backup of your profile, you can load an older version of it when connecting.&lt;/p&gt;&lt;p&gt;Should there be any modules that are marked to be &quot;&lt;i&gt;synced&lt;/i&gt;&quot; this will also cause them to be saved and reloaded into other profiles if they too are active.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="594"/>
      <source>Restore Actions toolbar</source>
      <extracomment>This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="597"/>
      <source>Restore Items toolbar</source>
      <extracomment>This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="684"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="687"/>
      <source>Search Options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="691"/>
      <source>Case sensitive</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="742"/>
      <source>Type</source>
      <comment>Heading for the first column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="743"/>
      <source>Name</source>
      <comment>Heading for the second column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="744"/>
      <source>Where</source>
      <comment>Heading for the third column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="745"/>
      <source>What</source>
      <comment>Heading for the fourth column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="792"/>
      <source>start of line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="825"/>
      <source>Text to find (trigger pattern)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2801"/>
      <source>Trying to activate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2803"/>
      <source>Trying to deactivate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2807"/>
      <source>&lt;b&gt;Unable to activate a filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2944"/>
      <source>Trying to activate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2946"/>
      <source>Trying to deactivate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2950"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an offset timer or timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2988"/>
      <source>Trying to activate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2990"/>
      <source>Trying to deactivate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2994"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3092"/>
      <source>Trying to activate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3094"/>
      <source>Trying to deactivate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3098"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a script group or script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3163"/>
      <source>Trying to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3165"/>
      <source>Trying to deactivate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3169"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3283"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4115"/>
      <source>New trigger group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3285"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4115"/>
      <source>New trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3384"/>
      <source>New timer group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3386"/>
      <source>New timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3474"/>
      <source>Table name...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3481"/>
      <source>Variable name...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3490"/>
      <source>New table name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3490"/>
      <source>New variable name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3540"/>
      <source>New key group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3542"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4815"/>
      <source>New key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3624"/>
      <source>New alias group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3626"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4223"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4293"/>
      <source>New alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3715"/>
      <source>New menu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3717"/>
      <source>New button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3748"/>
      <source>New toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3801"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4542"/>
      <source>New script group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3803"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4542"/>
      <source>New script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4237"/>
      <source>Alias &lt;em&gt;%1&lt;/em&gt; has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn&apos;t good as it&apos;ll call itself forever.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4780"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5562"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4915"/>
      <source>match on the prompt line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4918"/>
      <source>match on the prompt line (disabled)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4919"/>
      <source>A Go-Ahead (GA) signal from the game is required to make this feature work</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4969"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5079"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8572"/>
      <source>Foreground color ignored</source>
      <comment>Color trigger ignored foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4973"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5083"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8575"/>
      <source>Default foreground color</source>
      <comment>Color trigger default foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4977"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5087"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8578"/>
      <source>Foreground color [ANSI %1]</source>
      <comment>Color trigger ANSI foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4984"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5094"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8635"/>
      <source>Background color ignored</source>
      <comment>Color trigger ignored background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4988"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5098"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8638"/>
      <source>Default background color</source>
      <comment>Color trigger default background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4992"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5102"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8641"/>
      <source>Background color [ANSI %1]</source>
      <comment>Color trigger ANSI background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5113"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5115"/>
      <source>fault</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5168"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5172"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8461"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8487"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8981"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8982"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5621"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8435"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5660"/>
      <source>Menu properties</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5670"/>
      <source>Button properties</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5678"/>
      <source>Command (down);</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5858"/>
      <source>Aliases - Input Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5870"/>
      <source>Key Bindings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7606"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7610"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7630"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7634"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7654"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7658"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7678"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7682"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7702"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7706"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7726"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7731"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7751"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7755"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7774"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7778"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7801"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7820"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7824"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7843"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7847"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7866"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7871"/>
      <source>Export Package:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7606"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7610"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7630"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7634"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7654"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7658"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7678"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7682"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7702"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7706"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7726"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7731"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7751"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7755"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7774"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7778"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7801"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7820"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7824"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7843"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7847"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7866"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7871"/>
      <source>You have to choose an item for export first. Please select a tree item and then click on export again.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7615"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7639"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7663"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7687"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7711"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7736"/>
      <source>Package %1 saved</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7760"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7783"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7829"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7852"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7876"/>
      <source>Copied %1 to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Mudlet packages (*.xml)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Export Item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>export package:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>Cannot write file %1:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8169"/>
      <source>Import Mudlet Package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8263"/>
      <source>Couldn&apos;t save profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8263"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8270"/>
      <source>Backup Profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8270"/>
      <source>trigger files (*.trigger *.xml)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8395"/>
      <source>Ctrl+0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8422"/>
      <source>Images (*.png *.xpm *.jpg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8422"/>
      <source>Select Icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8431"/>
      <source>Command (down):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8452"/>
      <source>Select foreground color to apply to matches</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8478"/>
      <source>Select background color to apply to matches</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8496"/>
      <source>Choose sound file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8500"/>
      <source>Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;All files(*.*)</source>
      <comment>This the list of file extensions that are considered for sounds from triggers, the terms inside of the &apos;(&apos;...&apos;)&apos; and the &quot;;;&quot; are used programmatically and should not be changed.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8553"/>
      <source>Select foreground trigger color for item %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8617"/>
      <source>Select background trigger color for item %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8666"/>
      <source>Saving…</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8827"/>
      <source>Format All</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8830"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8836"/>
      <source>Cut</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8834"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8840"/>
      <source>Select All</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8996"/>
      <source>Sound file to play when the trigger fires.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="790"/>
      <source>substring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="86"/>
      <source>&lt;p&gt;Scripts organize code and can react to events. To add a new script:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Enter a script in the box below. You can for example define &lt;strong&gt;functions&lt;/strong&gt; to be called by other triggers, aliases, etc.&lt;/li&gt;&lt;li&gt;If you write lua &lt;strong&gt;commands&lt;/strong&gt; without defining a function, they will be run on Mudlet startup and each time you open the script for editing.&lt;/li&gt;&lt;li&gt;If needed, you can register a list of &lt;strong&gt;events&lt;/strong&gt; with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the script.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Scripts are run automatically when viewed, even if they are deactivated.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Events can also be added to a script from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;nameOfTheMudletEvent&amp;quot;, &amp;quot;nameOfYourFunctionToBeCalled&amp;quot;)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="97"/>
      <source>&lt;p&gt;Timers react after a timespan once or regularly. To add a new timer:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define the &lt;strong&gt;timespan&lt;/strong&gt; after which the timer should react in a this format: hours : minutes : seconds.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game when the time has passed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the timer.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Timers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;hello!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will greet you exactly 3 seconds after it was made.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="108"/>
      <source>&lt;p&gt;Buttons react on mouse clicks. To add a new button:&lt;ol&gt;&lt;li&gt;Add a new group to define a new &lt;strong&gt;button bar&lt;/strong&gt; in case you don&apos;t have any.&lt;/li&gt;&lt;li&gt;Add new groups as &lt;strong&gt;menus&lt;/strong&gt; to a button bar or sub-menus to menus.&lt;li&gt;&lt;li&gt;Add new items as &lt;strong&gt;buttons&lt;/strong&gt; to a button bar or menu or sub-menu.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the toolbar, menu or button. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If a button is made a &lt;strong&gt;click-down&lt;/strong&gt; button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua &apos;getButtonState()&apos; function reports whether the button is up or down.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="118"/>
      <source>&lt;p&gt;Keys react on keyboard presses. To add a new key binding:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Click on &lt;strong&gt;&apos;grab key&apos;&lt;/strong&gt; and then press your key combination, e.g. including modifier keys like Control, Shift, etc.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the new key binding.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Keys can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;my jump key&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;jump&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Pressing F8 will make you jump.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="128"/>
      <source>&lt;p&gt;Variables store information. To make a new variable:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above. To add a table instead click &apos;Add Group&apos;.&lt;/li&gt;&lt;li&gt;Select type of variable value (can be a string, integer, boolean)&lt;/li&gt;&lt;li&gt;Enter the value you want to store in this variable.&lt;/li&gt;&lt;li&gt;If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.&lt;/li&gt;&lt;li&gt;To remove a variable manually, set it to &apos;nil&apos; or click on the &apos;Delete&apos; icon above.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables created here won&apos;t be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables and tables can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will create a string called &apos;foo&apos; with &apos;bar&apos; as its value.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="201"/>
      <source>-- add your Lua code here</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="381"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8377"/>
      <source>Ctrl+1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="386"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8379"/>
      <source>Ctrl+2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="391"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8381"/>
      <source>Ctrl+3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="396"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8383"/>
      <source>Ctrl+4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="401"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8385"/>
      <source>Ctrl+5</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="406"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8387"/>
      <source>Ctrl+6</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="411"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8389"/>
      <source>Ctrl+7</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="415"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8390"/>
      <source>Errors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="416"/>
      <source>Show/Hide the errors console in the bottom right of this editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="417"/>
      <source>Show/Hide errors console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="417"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8391"/>
      <source>Ctrl+8</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="421"/>
      <source>Generate a statistics summary display on the main profile console.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="422"/>
      <source>Generate statistics</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="422"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8393"/>
      <source>Ctrl+9</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="426"/>
      <source>Show/Hide the separate Central Debug Console - when being displayed the system will be slower.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="427"/>
      <source>Show/Hide Debug Console (Ctrl+0) -&gt; system will be &lt;b&gt;&lt;i&gt;slower&lt;/i&gt;&lt;/b&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="545"/>
      <source>Editor Toolbar - %1 - Actions</source>
      <comment>This is the toolbar that is initially placed at the top of the editor.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="581"/>
      <source>Editor Toolbar - %1 - Items</source>
      <comment>This is the toolbar that is initially placed at the left side of the editor.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="693"/>
      <source>Match case precisely</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="697"/>
      <source>Include variables</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="699"/>
      <source>Search variables (slower)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="791"/>
      <source>perl regex</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="793"/>
      <source>exact match</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="794"/>
      <source>lua function</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="795"/>
      <source>line spacer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="796"/>
      <source>color trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="797"/>
      <source>prompt</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1923"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1935"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1963"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1995"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2025"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2037"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2064"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2099"/>
      <source>Trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1443"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1486"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1558"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1630"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1752"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1836"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1923"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2025"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2131"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2220"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2306"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2430"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2504"/>
      <source>Name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1498"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1503"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1570"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1575"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1642"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1647"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1851"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1935"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1940"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2037"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2042"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2141"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2146"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2318"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2323"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2442"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2447"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2516"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2521"/>
      <source>Command</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1963"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1968"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2064"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2069"/>
      <source>Pattern {%1}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1528"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1533"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1600"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1605"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1722"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1727"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1811"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1893"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1898"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1995"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2000"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2099"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2104"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2188"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2193"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2274"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2279"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2398"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2403"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2472"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2546"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2551"/>
      <source>Lua code (%1:%2)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1836"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1863"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1893"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2131"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2141"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2158"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2188"/>
      <source>Alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1863"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1868"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2158"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2163"/>
      <source>Pattern</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1752"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1774"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2220"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2242"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2274"/>
      <source>Script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1774"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1779"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2242"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2247"/>
      <source>Event Handler</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1630"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1642"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1661"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1722"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2306"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2318"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2337"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2398"/>
      <source>Button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1642"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1647"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2318"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2323"/>
      <source>Command {Down}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1661"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1666"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2337"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2342"/>
      <source>Command {Up}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1690"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2366"/>
      <source>Action</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1690"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1695"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2366"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2371"/>
      <source>Stylesheet {L: %1 C: %2}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1558"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1570"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1600"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2430"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2442"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2472"/>
      <source>Timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1486"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1498"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1528"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2504"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2516"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2546"/>
      <source>Key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1443"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1457"/>
      <source>Variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1457"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1463"/>
      <source>Value</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgTriggerPatternEdit</name>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="51"/>
      <source>Text to find (anywhere in the game output)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="54"/>
      <source>Text to find (as a regular expression pattern)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="57"/>
      <source>Text to find (from beginning of the line)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="60"/>
      <source>Exact line to match</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="63"/>
      <source>Lua code to run (return true to match)</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgVarsMainArea</name>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="51"/>
      <location filename="../src/dlgVarsMainArea.cpp" line="78"/>
      <source>Auto-Type</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="52"/>
      <source>key (string)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="53"/>
      <source>index (integer number)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="54"/>
      <source>table (use &quot;Add Group&quot; to create)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="55"/>
      <source>function (cannot create from GUI)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="79"/>
      <source>string</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="80"/>
      <source>number</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="81"/>
      <source>boolean</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="82"/>
      <source>table</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="83"/>
      <source>function</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>edbee::TextEditorComponent</name>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="573"/>
      <source>Cut</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="574"/>
      <source>Copy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="575"/>
      <source>Paste</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="577"/>
      <source>Select All</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>irc</name>
    <message>
      <location filename="../src/ui/irc.ui" line="25"/>
      <source>Mudlet IRC Client</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>keybindings_main_area</name>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your key or key group. This will be displayed in the key tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="40"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="50"/>
      <source>&lt;p&gt;Type in one or more commands you want the key to send directly to the game when pressed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="53"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="60"/>
      <source>Key Binding:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="77"/>
      <source>Grab New Key</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>lacking_mapper_script</name>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="23"/>
      <source>No mapping script found</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="35"/>
      <source>&lt;p&gt;It seems that you don&apos;t have any &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;mapping scripts&lt;/a&gt; installed yet - the mapper needs you to have one for your game, so it can track where you are and autowalk you. You can either make one yourself, or import an existing one that someone else made.&lt;/p&gt;&lt;p&gt;Would you like to see if any are available?&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="86"/>
      <source>Close</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="93"/>
      <source>Find some scripts</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <location filename="../src/main.cpp" line="197"/>
      <source>Profile to open automatically</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="197"/>
      <source>profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="200"/>
      <source>Display help and exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="203"/>
      <source>Display version and exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="206"/>
      <source>Don&apos;t show the splash screen when starting</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="209"/>
      <source>Mirror output of all consoles to STDOUT</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="219"/>
      <source>Usage: %1 [OPTION...]
       -h, --help           displays this message.
       -v, --version        displays version information.
       -q, --quiet          no splash screen on startup.
       --profile=&lt;profile&gt;  additional profile to open

There are other inherited options that arise from the Qt Libraries which are
less likely to be useful for normal use of this application:
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="276"/>
      <source>%1 %2%3 (with debug symbols, without optimisations)
</source>
      <comment>%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="282"/>
      <source>Qt libraries %1 (compilation) %2 (runtime)
</source>
      <comment>%1 and %2 are version numbers</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="285"/>
      <source>Copyright © 2008-2022  Mudlet developers
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="286"/>
      <source>Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="287"/>
      <source>This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="267"/>
      <source>Report bugs to: https://github.com/Mudlet/Mudlet/issues
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="268"/>
      <source>Project home page: http://www.mudlet.org/
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="311"/>
      <source>Version: %1</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>main_window</name>
    <message>
      <location filename="../src/ui/main_window.ui" line="95"/>
      <source>Toolbox</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="110"/>
      <source>Options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="117"/>
      <source>Help</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="128"/>
      <source>About</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="136"/>
      <source>Games</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="153"/>
      <source>Play</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="156"/>
      <source>&lt;p&gt;Configure connection details of, and make a connection to, game servers.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="164"/>
      <source>&lt;p&gt;Disconnect from the current game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="172"/>
      <source>&lt;p&gt;Disconnect and then reconnect to the current game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="180"/>
      <source>&lt;p&gt;Configure setting for the Mudlet application globally and for the current profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="188"/>
      <source>&lt;p&gt;Opens the Editor for the different types of things that can be scripted by the user.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="193"/>
      <source>Show errors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="196"/>
      <source>&lt;p&gt;Show errors from scripts that you have running&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="225"/>
      <source>IRC</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="236"/>
      <source>&lt;p&gt;Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="244"/>
      <source>&lt;p&gt;Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="252"/>
      <source>&lt;p&gt;Opens the (on-line) Mudlet Forum in your system web-browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="273"/>
      <source>&lt;p&gt;Show or hide the game map.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="281"/>
      <source>&lt;p&gt;Install and remove collections of Mudlet lua items (packages).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="289"/>
      <source>&lt;p&gt;Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="294"/>
      <source>Package exporter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="297"/>
      <source>&lt;p&gt;Gather and bundle up collections of Mudlet Lua items and other reasources into a module.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="328"/>
      <source>&lt;p&gt;Hide / show the search area and buttons at the bottom of the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="333"/>
      <source>Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="336"/>
      <source>&lt;p&gt;Open a link to Discord.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="344"/>
      <source>Discord help channel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="347"/>
      <source>&lt;p&gt;Open a link to the Mudlet server on Discord.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="352"/>
      <location filename="../src/ui/main_window.ui" line="355"/>
      <source>Report an issue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="358"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="363"/>
      <source>Close profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="161"/>
      <source>Disconnect</source>
      <translation>Koppla bort</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="169"/>
      <source>Reconnect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="177"/>
      <source>Preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="185"/>
      <source>Script editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="201"/>
      <source>Notepad</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="209"/>
      <source>API Reference</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="249"/>
      <source>Online forum</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="217"/>
      <source>About Mudlet</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="204"/>
      <source>&lt;p&gt;Opens a free form text editor window for this profile that is saved between sessions.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="212"/>
      <source>&lt;p&gt;Opens the Mudlet manual in your web browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="220"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="228"/>
      <location filename="../src/ui/main_window.ui" line="265"/>
      <source>&lt;p&gt;Opens a built-in IRC chat.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="233"/>
      <source>Video tutorials</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="241"/>
      <source>Load replay</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="257"/>
      <source>Check for updates...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="262"/>
      <source>Live help chat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="270"/>
      <source>Show map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="278"/>
      <source>Package manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="286"/>
      <source>Module manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="311"/>
      <source>MultiView</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="314"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="325"/>
      <source>Compact input line</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>map_label</name>
    <message>
      <location filename="../src/ui/map_label.ui" line="20"/>
      <source>Map label</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="38"/>
      <source>Type:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="49"/>
      <source>Text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="54"/>
      <source>Image</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="62"/>
      <source>Image:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="79"/>
      <location filename="../src/ui/map_label.ui" line="127"/>
      <source>...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="86"/>
      <source>Stretch image</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="93"/>
      <source>Label text:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="103"/>
      <source>My Label</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="110"/>
      <source>Font:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="134"/>
      <source>Foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="151"/>
      <source>Background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="169"/>
      <source>Background</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="174"/>
      <source>Foreground</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="182"/>
      <source>Position:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="192"/>
      <source>If deselected the label will have the same size when you zoom in and out in the mapper. If it is selected the label will scale when you zoom the mapper.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="195"/>
      <source>Scale with zoom</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="243"/>
      <source>Save</source>
      <translation>Spara</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="250"/>
      <source>Cancel</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>mapper</name>
    <message>
      <location filename="../src/ui/mapper.ui" line="60"/>
      <source>^</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="452"/>
      <source>Area:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="521"/>
      <source>Rooms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="550"/>
      <source>Exits</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="579"/>
      <source>Round</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="586"/>
      <source>Info</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="602"/>
      <source>IDs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="618"/>
      <source>Names</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="771"/>
      <source>top + 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="745"/>
      <source>bottom + 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="758"/>
      <source>bottom -1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="784"/>
      <source>top - 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="732"/>
      <source>1 level</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="427"/>
      <source>3D</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="680"/>
      <source>default</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="693"/>
      <source>top view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="706"/>
      <source>side view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="719"/>
      <source>all levels</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>module_manager</name>
    <message>
      <location filename="../src/ui/module_manager.ui" line="79"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Modules are a way to utilize a common package across many sessions - unlike packages, which are installed per-profile.&lt;/p&gt;
&lt;p&gt;Modules are loaded in ascending priority (1 will get loaded before 2 and so on), modules with the same priority will be loaded in alphabetical order.&lt;/p&gt;
&lt;p&gt;Modules with negative priority will be loaded before script packages.&lt;/p&gt;
&lt;p&gt;The &lt;b&gt;&lt;i&gt;Sync&lt;/i&gt;&lt;/b&gt; option, if it is enabled, will, when the module in &lt;b&gt;this profile&lt;/b&gt; is saved &lt;b&gt;to disk&lt;/b&gt;, cause it to be then reloaded into all profiles which also are using the same file that contains the module. To make several profiles use the same module, install it in each profile through this module manager (which should be opened when the particular profile is the one currently in the foreground).&lt;/p&gt;&lt;p&gt;
&lt;p&gt;For each save operation, modules are backed up to a directory, &lt;i&gt;moduleBackups&lt;/i&gt;, within your Mudlet profile directory.&lt;/p&gt;
&lt;/body&gt;&lt;/html&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="128"/>
      <source>Uninstall</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="141"/>
      <source>Install</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="148"/>
      <source>Module Help</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>mudlet</name>
    <message>
      <location filename="../src/mudlet.cpp" line="776"/>
      <source>Afrikaans</source>
      <extracomment>In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a &apos;-&apos; Chinese is an exception!</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="777"/>
      <source>Afrikaans (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="778"/>
      <source>Aragonese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="779"/>
      <source>Aragonese (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="780"/>
      <source>Arabic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="781"/>
      <source>Arabic (United Arab Emirates)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="782"/>
      <source>Arabic (Bahrain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="783"/>
      <source>Arabic (Algeria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="785"/>
      <source>Arabic (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="786"/>
      <source>Arabic (Iraq)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="787"/>
      <source>Arabic (Jordan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="788"/>
      <source>Arabic (Kuwait)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="789"/>
      <source>Arabic (Lebanon)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="790"/>
      <source>Arabic (Libya)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="791"/>
      <source>Arabic (Morocco)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="792"/>
      <source>Arabic (Oman)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="793"/>
      <source>Arabic (Qatar)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="794"/>
      <source>Arabic (Saudi Arabia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="795"/>
      <source>Arabic (Sudan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="796"/>
      <source>Arabic (Syria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="797"/>
      <source>Arabic (Tunisia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="798"/>
      <source>Arabic (Yemen)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="799"/>
      <source>Belarusian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="800"/>
      <source>Belarusian (Belarus)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="801"/>
      <source>Belarusian (Russia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="802"/>
      <source>Bulgarian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="803"/>
      <source>Bulgarian (Bulgaria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="804"/>
      <source>Bangla</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="805"/>
      <source>Bangla (Bangladesh)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="806"/>
      <source>Bangla (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="807"/>
      <source>Tibetan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="808"/>
      <source>Tibetan (China)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="809"/>
      <source>Tibetan (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="810"/>
      <source>Breton</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="811"/>
      <source>Breton (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="812"/>
      <source>Bosnian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="813"/>
      <source>Bosnian (Bosnia/Herzegovina)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="814"/>
      <source>Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="815"/>
      <source>Catalan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="816"/>
      <source>Catalan (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="817"/>
      <source>Catalan (Spain - Valencian)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="818"/>
      <source>Central Kurdish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="819"/>
      <source>Central Kurdish (Iraq)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="820"/>
      <source>Czech</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="821"/>
      <source>Czech (Czechia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="822"/>
      <source>Danish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="823"/>
      <source>Danish (Denmark)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="824"/>
      <source>German</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="825"/>
      <source>German (Austria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="826"/>
      <source>German (Austria, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="827"/>
      <source>German (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="828"/>
      <source>German (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="829"/>
      <source>German (Switzerland, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="830"/>
      <source>German (Germany/Belgium/Luxemburg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="831"/>
      <source>German (Germany/Belgium/Luxemburg, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="832"/>
      <source>German (Liechtenstein)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="833"/>
      <source>German (Luxembourg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="836"/>
      <source>Greek</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="837"/>
      <source>Greek (Greece)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="838"/>
      <source>English</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="839"/>
      <source>English (Antigua/Barbuda)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="840"/>
      <source>English (Australia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="842"/>
      <source>English (Bahamas)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="843"/>
      <source>English (Botswana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="844"/>
      <source>English (Belize)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="784"/>
      <source>Arabic (Egypt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="364"/>
      <location filename="../src/mudlet.cpp" line="366"/>
      <location filename="../src/mudlet.cpp" line="689"/>
      <source>Close profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="431"/>
      <source>Mudlet chat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="432"/>
      <source>Open a link to the Mudlet server on Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="474"/>
      <source>Packages (exp.)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="481"/>
      <source>Package Manager (experimental)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="483"/>
      <source>Packages (exp.)</source>
      <comment>exp. stands for experimental; shortened so it doesn&apos;t make buttons huge in the main interface</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="524"/>
      <source>Report issue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="536"/>
      <location filename="../src/mudlet.cpp" line="3648"/>
      <source>Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="678"/>
      <source>Script editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="679"/>
      <source>Show Map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="680"/>
      <source>Compact input line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="681"/>
      <source>Preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="683"/>
      <source>Package manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="684"/>
      <source>Module manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="686"/>
      <source>Play</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="834"/>
      <source>Dzongkha</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="835"/>
      <source>Dzongkha (Bhutan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="841"/>
      <source>English (Australia, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="845"/>
      <source>English (Canada)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="846"/>
      <source>English (Canada, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="847"/>
      <source>English (Denmark)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="848"/>
      <source>English (United Kingdom)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="849"/>
      <source>English (United Kingdom, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="850"/>
      <source>English (United Kingdom - &apos;ise&apos; not &apos;ize&apos;)</source>
      <comment>This dictionary prefers the British &apos;ise&apos; form over the American &apos;ize&apos; one.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="851"/>
      <source>English (Ghana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="852"/>
      <source>English (Hong Kong SAR China)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="853"/>
      <source>English (Ireland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="854"/>
      <source>English (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="855"/>
      <source>English (Jamaica)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="856"/>
      <source>English (Namibia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="857"/>
      <source>English (Nigeria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="858"/>
      <source>English (New Zealand)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="859"/>
      <source>English (Philippines)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="860"/>
      <source>English (Singapore)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="861"/>
      <source>English (Trinidad/Tobago)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="862"/>
      <source>English (United States)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="863"/>
      <source>English (United States, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="864"/>
      <source>English (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="865"/>
      <source>English (Zimbabwe)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="866"/>
      <source>Spanish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="867"/>
      <source>Spanish (Argentina)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="868"/>
      <source>Spanish (Bolivia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="869"/>
      <source>Spanish (Chile)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="870"/>
      <source>Spanish (Colombia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="871"/>
      <source>Spanish (Costa Rica)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="872"/>
      <source>Spanish (Cuba)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="873"/>
      <source>Spanish (Dominican Republic)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="874"/>
      <source>Spanish (Ecuador)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="875"/>
      <source>Spanish (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="876"/>
      <source>Spanish (Guatemala)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="877"/>
      <source>Spanish (Honduras)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="878"/>
      <source>Spanish (Mexico)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="879"/>
      <source>Spanish (Nicaragua)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="880"/>
      <source>Spanish (Panama)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="881"/>
      <source>Spanish (Peru)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="882"/>
      <source>Spanish (Puerto Rico)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="883"/>
      <source>Spanish (Paraguay)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="884"/>
      <source>Spanish (El Savador)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="885"/>
      <source>Spanish (United States)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="886"/>
      <source>Spanish (Uruguay)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="887"/>
      <source>Spanish (Venezuela)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="888"/>
      <source>Estonian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="889"/>
      <source>Estonian (Estonia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="890"/>
      <source>Basque</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="891"/>
      <source>Basque (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="892"/>
      <source>Basque (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="893"/>
      <location filename="../src/mudlet.cpp" line="894"/>
      <source>Finnish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="895"/>
      <source>French</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="896"/>
      <source>French (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="897"/>
      <source>French (Catalan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="898"/>
      <source>French (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="899"/>
      <source>French (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="900"/>
      <source>French (Luxemburg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="901"/>
      <source>French (Monaco)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="902"/>
      <source>Gaelic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="903"/>
      <source>Gaelic (United Kingdom {Scots})</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="904"/>
      <source>Galician</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="905"/>
      <source>Galician (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="906"/>
      <location filename="../src/mudlet.cpp" line="911"/>
      <source>Guarani</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="907"/>
      <location filename="../src/mudlet.cpp" line="912"/>
      <source>Guarani (Paraguay)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="908"/>
      <source>Gujarati</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="909"/>
      <source>Gujarati (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="913"/>
      <source>Hebrew</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="914"/>
      <source>Hebrew (Israel)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="915"/>
      <source>Hindi</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="916"/>
      <source>Hindi (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="917"/>
      <source>Croatian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="918"/>
      <source>Croatian (Croatia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="919"/>
      <source>Hungarian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="920"/>
      <source>Hungarian (Hungary)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="921"/>
      <source>Armenian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="922"/>
      <source>Armenian (Armenia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="923"/>
      <source>Indonesian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="924"/>
      <source>Indonesian (Indonesia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="925"/>
      <source>Interlingue</source>
      <comment>formerly known as Occidental, and not to be mistaken for Interlingua</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="926"/>
      <source>Icelandic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="927"/>
      <source>Icelandic (Iceland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="928"/>
      <source>Italian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="929"/>
      <source>Italian (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="930"/>
      <source>Italian (Italy)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="931"/>
      <source>Kazakh</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="932"/>
      <source>Kazakh (Kazakhstan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="933"/>
      <source>Kurmanji</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="934"/>
      <source>Kurmanji {Latin-alphabet Kurdish}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="935"/>
      <source>Korean</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="936"/>
      <source>Korean (South Korea)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="937"/>
      <source>Kurdish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="938"/>
      <source>Kurdish (Syria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="939"/>
      <source>Kurdish (Turkey)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="940"/>
      <source>Lao</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="941"/>
      <source>Lao (Laos)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="942"/>
      <source>Lithuanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="943"/>
      <source>Lithuanian (Lithuania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="944"/>
      <source>Latvian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="945"/>
      <source>Latvian (Latvia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="946"/>
      <source>Malayalam</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="947"/>
      <source>Malayalam (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="948"/>
      <source>Norwegian Bokmål</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="949"/>
      <source>Norwegian Bokmål (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="950"/>
      <source>Nepali</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="951"/>
      <source>Nepali (Nepal)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="952"/>
      <source>Dutch</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="953"/>
      <source>Dutch (Netherlands Antilles)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="954"/>
      <source>Dutch (Aruba)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="955"/>
      <source>Dutch (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="956"/>
      <source>Dutch (Netherlands)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="957"/>
      <source>Dutch (Suriname)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="958"/>
      <source>Norwegian Nynorsk</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="959"/>
      <source>Norwegian Nynorsk (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="960"/>
      <source>Occitan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="961"/>
      <source>Occitan (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="962"/>
      <source>Polish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="963"/>
      <source>Polish (Poland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="964"/>
      <source>Portuguese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="965"/>
      <source>Portuguese (Brazil)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="966"/>
      <source>Portuguese (Portugal)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="967"/>
      <source>Romanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="968"/>
      <source>Romanian (Romania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="969"/>
      <source>Russian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="970"/>
      <source>Russian (Russia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="971"/>
      <source>Northern Sami</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="972"/>
      <source>Northern Sami (Finland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="973"/>
      <source>Northern Sami (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="974"/>
      <source>Northern Sami (Sweden)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="975"/>
      <source>Shtokavian</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="976"/>
      <source>Shtokavian (former state of Yugoslavia)</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="977"/>
      <source>Sinhala</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="978"/>
      <source>Sinhala (Sri Lanka)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="979"/>
      <source>Slovak</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="980"/>
      <source>Slovak (Slovakia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="981"/>
      <source>Slovenian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="982"/>
      <source>Slovenian (Slovenia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="983"/>
      <source>Somali</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="984"/>
      <source>Somali (Somalia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="985"/>
      <source>Albanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="986"/>
      <source>Albanian (Albania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="987"/>
      <source>Serbian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="988"/>
      <source>Serbian (Montenegro)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="989"/>
      <source>Serbian (Serbia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="990"/>
      <source>Serbian (Serbia - Latin-alphabet)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="991"/>
      <source>Serbian (former state of Yugoslavia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="992"/>
      <source>Swati</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="993"/>
      <source>Swati (Swaziland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="994"/>
      <source>Swati (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="995"/>
      <source>Swedish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="996"/>
      <source>Swedish (Sweden)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="997"/>
      <source>Swedish (Finland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="998"/>
      <source>Swahili</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="999"/>
      <source>Swahili (Kenya)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1000"/>
      <source>Swahili (Tanzania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1013"/>
      <source>Turkish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1001"/>
      <source>Telugu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="512"/>
      <source>Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1002"/>
      <source>Telugu (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1003"/>
      <source>Thai</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1004"/>
      <source>Thai (Thailand)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1005"/>
      <source>Tigrinya</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1006"/>
      <source>Tigrinya (Eritrea)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1007"/>
      <source>Tigrinya (Ethiopia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1008"/>
      <source>Turkmen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1009"/>
      <source>Turkmen (Turkmenistan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1010"/>
      <source>Tswana</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1011"/>
      <source>Tswana (Botswana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1012"/>
      <source>Tswana (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1015"/>
      <source>Tsonga</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1016"/>
      <source>Tsonga (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1017"/>
      <source>Ukrainian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1018"/>
      <source>Ukrainian (Ukraine)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1019"/>
      <source>Uzbek</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1020"/>
      <source>Uzbek (Uzbekistan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1021"/>
      <source>Venda</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1022"/>
      <source>Vietnamese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1023"/>
      <source>Vietnamese (Vietnam)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1026"/>
      <source>Walloon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1027"/>
      <source>Xhosa</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1028"/>
      <source>Yiddish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1029"/>
      <source>Chinese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1030"/>
      <source>Chinese (China - simplified)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1031"/>
      <source>Chinese (Taiwan - traditional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1032"/>
      <source>Zulu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3058"/>
      <source>Cannot load a replay as one is already in progress in this or another profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3077"/>
      <source>Replay each step with a shorter time interval between steps.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3083"/>
      <source>Replay each step with a longer time interval between steps.</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3682"/>
      <source>Review the update(s) available...</source>
      <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4569"/>
      <source>Hide tray icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4574"/>
      <source>Quit Mudlet</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="254"/>
      <source>hh:mm:ss</source>
      <comment>Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&amp;) for the gory details...!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="311"/>
      <source>Main Toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="349"/>
      <location filename="../src/mudlet.cpp" line="356"/>
      <location filename="../src/mudlet.cpp" line="358"/>
      <source>Connect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="361"/>
      <location filename="../src/mudlet.cpp" line="687"/>
      <source>Disconnect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="426"/>
      <source>Open Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="438"/>
      <source>Open IRC</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="374"/>
      <source>Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="375"/>
      <source>Show and edit triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="382"/>
      <source>Aliases</source>
      <translation>Aliasnamn</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="383"/>
      <source>Show and edit aliases</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="388"/>
      <source>Timers</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="389"/>
      <source>Show and edit timers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="394"/>
      <source>Buttons</source>
      <translation>Knappar</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="395"/>
      <source>Show and edit easy buttons</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="400"/>
      <source>Scripts</source>
      <translation>Skript</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="401"/>
      <source>Show and edit scripts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="406"/>
      <source>Keys</source>
      <translation>Nycklar</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="407"/>
      <source>Show and edit keys</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="412"/>
      <source>Variables</source>
      <translation>Variabler</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="413"/>
      <source>Show and edit Lua variables</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="446"/>
      <source>Map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="447"/>
      <source>Show/hide the map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="452"/>
      <source>Manual</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="453"/>
      <source>Browse reference material and documentation</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="458"/>
      <source>Settings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="459"/>
      <source>See and edit profile preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="467"/>
      <location filename="../src/mudlet.cpp" line="682"/>
      <source>Notepad</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="468"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="486"/>
      <source>Module Manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="490"/>
      <source>Package Exporter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="500"/>
      <source>Replay</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="505"/>
      <location filename="../src/mudlet.cpp" line="688"/>
      <source>Reconnect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="506"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="511"/>
      <location filename="../src/mudlet.cpp" line="685"/>
      <source>MultiView</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="535"/>
      <location filename="../src/mudlet.cpp" line="3665"/>
      <source>About</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1014"/>
      <source>Turkish (Turkey)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1024"/>
      <source>Vietnamese (DauCu variant - old-style diacritics)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1025"/>
      <source>Vietnamese (DauMoi variant - new-style diacritics)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1034"/>
      <source>ASCII (Basic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1035"/>
      <source>UTF-8 (Recommended)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1040"/>
      <source>ISO 8859-1 (Western European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1041"/>
      <source>ISO 8859-2 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1042"/>
      <source>ISO 8859-3 (South European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1043"/>
      <source>ISO 8859-4 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1044"/>
      <source>ISO 8859-5 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1045"/>
      <source>ISO 8859-6 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1046"/>
      <source>ISO 8859-7 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1047"/>
      <source>ISO 8859-8 (Hebrew Visual)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1048"/>
      <source>ISO 8859-9 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1049"/>
      <source>ISO 8859-10 (Nordic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1050"/>
      <source>ISO 8859-11 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1051"/>
      <source>ISO 8859-13 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1052"/>
      <source>ISO 8859-14 (Celtic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1053"/>
      <source>ISO 8859-15 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1054"/>
      <source>ISO 8859-16 (Romanian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1055"/>
      <location filename="../src/mudlet.cpp" line="1056"/>
      <source>CP437 (OEM Font)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1057"/>
      <location filename="../src/mudlet.cpp" line="1058"/>
      <source>CP667 (Mazovia)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1059"/>
      <location filename="../src/mudlet.cpp" line="1060"/>
      <source>CP737 (DOS Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1061"/>
      <source>CP850 (Western Europe)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1062"/>
      <source>CP866 (Cyrillic/Russian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1063"/>
      <location filename="../src/mudlet.cpp" line="1064"/>
      <source>CP869 (DOS Greek 2)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1065"/>
      <source>CP1161 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1066"/>
      <source>KOI8-R (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1067"/>
      <source>KOI8-U (Cyrillic/Ukrainian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1068"/>
      <source>MACINTOSH</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1069"/>
      <source>WINDOWS-1250 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1070"/>
      <source>WINDOWS-1251 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1071"/>
      <source>WINDOWS-1252 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1072"/>
      <source>WINDOWS-1253 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1073"/>
      <source>WINDOWS-1254 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1074"/>
      <source>WINDOWS-1255 (Hebrew)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1075"/>
      <source>WINDOWS-1256 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1076"/>
      <source>WINDOWS-1257 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1077"/>
      <source>WINDOWS-1258 (Vietnamese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1699"/>
      <location filename="../src/mudlet.cpp" line="3154"/>
      <source>Load a Mudlet replay.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2759"/>
      <source>Central Debug Console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="547"/>
      <location filename="../src/mudlet.cpp" line="548"/>
      <source>Toggle Full Screen View</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="528"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1036"/>
      <source>GBK (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1037"/>
      <source>GB18030 (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1038"/>
      <source>Big5-ETen (Taiwan)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1039"/>
      <source>Big5-HKSCS (Hong Kong)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1662"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Disabled until a profile is loaded.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2528"/>
      <source>%1 - notes</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2612"/>
      <source>Select Replay</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2614"/>
      <source>*.dat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2917"/>
      <source>[  OK  ]  - Profile &quot;%1&quot; loaded in offline mode.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2998"/>
      <source>[ INFO ]  - Compact input line set. Press %1 to show bottom-right buttons again.</source>
      <comment>Here %1 will be replaced with the keyboard shortcut, default is ALT+L.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3075"/>
      <source>Faster</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3081"/>
      <source>Slower</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3093"/>
      <location filename="../src/mudlet.cpp" line="3162"/>
      <location filename="../src/mudlet.cpp" line="3175"/>
      <source>Speed: X%1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3100"/>
      <location filename="../src/mudlet.cpp" line="3117"/>
      <source>Time: %1</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3659"/>
      <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
      <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3677"/>
      <source>Review %n update(s)...</source>
      <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3707"/>
      <source>Update installed - restart to apply</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3743"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress,
try again when it has finished.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>package_manager</name>
    <message>
      <location filename="../src/ui/package_manager.ui" line="122"/>
      <source>Details</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="203"/>
      <source>Install new package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="219"/>
      <source>Remove package</source>
      <comment>Message on button in package manager initially and when there is no packages to remove.</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>package_manager_unpack</name>
    <message>
      <location filename="../src/ui/package_manager_unpack.ui" line="24"/>
      <source>unpacking please wait ...</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>profile_preferences</name>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="20"/>
      <source>Profile preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="55"/>
      <source>General</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="61"/>
      <source>Icon sizes</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="67"/>
      <source>Icon size toolbars:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="90"/>
      <source>Icon size in tree views:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="110"/>
      <source>Show menu bar:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="121"/>
      <location filename="../src/ui/profile_preferences.ui" line="150"/>
      <source>Never</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="126"/>
      <location filename="../src/ui/profile_preferences.ui" line="155"/>
      <source>Until a profile is loaded</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="131"/>
      <location filename="../src/ui/profile_preferences.ui" line="160"/>
      <source>Always</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="139"/>
      <source>Show main toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="282"/>
      <source>Allow server to install script packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="307"/>
      <source>Game protocols</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="364"/>
      <location filename="../src/ui/profile_preferences.ui" line="3606"/>
      <source>Please reconnect to your game for the change to take effect</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="380"/>
      <source>Log options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="389"/>
      <source>Save log files in HTML format instead of plain text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="396"/>
      <source>Add timestamps at the beginning of log lines</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="403"/>
      <source>Save log files in:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="420"/>
      <source>Browse...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="427"/>
      <source>Reset</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="434"/>
      <source>Log format:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="447"/>
      <source>Log name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="467"/>
      <source>.txt</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="500"/>
      <source>Input line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="509"/>
      <source>Input</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="515"/>
      <source>use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="518"/>
      <source>Strict UNIX line endings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="535"/>
      <source>Show the text you sent</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="525"/>
      <source>Auto clear the input line after you sent text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="201"/>
      <source>&lt;p&gt;If you are playing a non-English game and seeing � instead of text, or special letters like &lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt; aren&apos;t showing right - try changing the encoding to UTF-8 or to one suggested by your game.&lt;/p&gt;&lt;p&gt;For some encodings on some Operating Systems Mudlet itself has to provide the codec needed; if that is the case for this Mudlet then there will be a &lt;tt&gt;m &lt;/tt&gt; prefixed applied to those encoding names (so if they have errors the blame can be applied correctly!)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="343"/>
      <source>&lt;p&gt;Enables MSP - provides Mud Sound Protocol messages during game play for supported games&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="386"/>
      <source>&lt;p&gt;When checked will cause the date-stamp named log file to be HTML (file extension &apos;.html&apos;) which can convey color, font and other formatting information rather than a plain text (file extension &apos;.txt&apos;) format.  If changed while logging is already in progress it is necessary to stop and restart logging for this setting to take effect in a new log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="555"/>
      <source>React to all keybindings on the same key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="565"/>
      <source>Command separator:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="575"/>
      <source>Enter text to separate commands with or leave blank to disable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="582"/>
      <source>Command line minimum height in pixels:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="704"/>
      <source>Main display</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="716"/>
      <source>Font</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="722"/>
      <source>Font:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="745"/>
      <source>Size:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="780"/>
      <source>Use anti aliasing on fonts. Smoothes fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry. </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="783"/>
      <source>Enable anti-aliasing</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="799"/>
      <source>Display Border</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="820"/>
      <source>Top border height:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="864"/>
      <source>Left border width:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="908"/>
      <source>Bottom border height:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="952"/>
      <source>Right border width:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="993"/>
      <source>Word wrapping</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1017"/>
      <source>Wrap lines at:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1037"/>
      <location filename="../src/ui/profile_preferences.ui" line="1085"/>
      <source>characters</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1065"/>
      <source>Indent wrapped lines by:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1098"/>
      <source>Double-click</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1104"/>
      <source>Stop selecting a word on these characters:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1114"/>
      <source>&apos;&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1127"/>
      <source>Display options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1136"/>
      <source>Fix unnecessary linebreaks on GA servers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1146"/>
      <source>Show Spaces/Tabs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1156"/>
      <source>Use Mudlet on a netbook with a small screen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1166"/>
      <source>Show Line/Paragraphs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1176"/>
      <source>Echo Lua errors to the main console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1183"/>
      <source>Make &apos;Ambiguous&apos; E. Asian width characters wide</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1249"/>
      <source>Editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1255"/>
      <source>Theme</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1323"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1382"/>
      <source>Color view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1388"/>
      <source>Select your color preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1394"/>
      <source>Foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1417"/>
      <source>Background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1437"/>
      <source>Command line foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1454"/>
      <source>Command line background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1471"/>
      <source>Command foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1491"/>
      <source>Command background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="313"/>
      <source>&lt;p&gt;Enables GMCP - note that if you have MSDP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="333"/>
      <source>&lt;p&gt;Enables MSDP - note that if you have GMCP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="171"/>
      <source>Language &amp;&amp; data encoding</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="177"/>
      <source>Interface language:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="191"/>
      <source>Server data encoding:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="213"/>
      <source>Please restart Mudlet to apply the new language</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="223"/>
      <source>Miscellaneous</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="316"/>
      <source>Enable GMCP  (Generic Mud Communication Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="336"/>
      <source>Enable MSDP  (Mud Server Data Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="323"/>
      <source>&lt;p&gt;Enables MSSP - provides Mud Server Status Protocol information upon connection for supported games&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="326"/>
      <source>Enable MSSP  (Mud Server Status Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="532"/>
      <source>&lt;p&gt;Echo the text you send in the display box.&lt;/p&gt;&lt;p&gt;&lt;i&gt;This can be disabled by the game server if it negotiates to use the telnet ECHO option&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="552"/>
      <source>&lt;p&gt;Check all Key-bindings against key-presses.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Versions of Mudlet prior to &lt;b&gt;3.9.0&lt;/b&gt; would stop checking after the first matching combination of&lt;/i&gt; KeyCode &lt;i&gt;and&lt;/i&gt; KeyModifier &lt;i&gt;was found and then send the command and/or run the script of that Key-binding only.  This&lt;/i&gt; per-profile &lt;i&gt;option tells Mudlet to check and run the remaining matches; but, to retain compatibility with previous versions, defaults to the &lt;b&gt;un&lt;/b&gt;-checked state.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="608"/>
      <source>Spell checking</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="614"/>
      <source>&lt;p&gt;This option controls spell-checking on the command line in the main console for this profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="617"/>
      <source>System/Mudlet dictionary:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="624"/>
      <source>&lt;p&gt;Select one dictionary which will be available on the command line and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="652"/>
      <source>User dictionary:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="658"/>
      <source>&lt;p&gt;A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="661"/>
      <source>Profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="668"/>
      <source>&lt;p&gt;A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="671"/>
      <source>Shared</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="760"/>
      <source>The selected font doesn&apos;t work with Mudlet, please pick another</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="817"/>
      <location filename="../src/ui/profile_preferences.ui" line="833"/>
      <source>&lt;p&gt;Extra space to have before text on top - can be set to negative to move text up beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="861"/>
      <location filename="../src/ui/profile_preferences.ui" line="877"/>
      <source>&lt;p&gt;Extra space to have before text on the left - can be set to negative to move text left beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="905"/>
      <location filename="../src/ui/profile_preferences.ui" line="921"/>
      <source>&lt;p&gt;Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="949"/>
      <location filename="../src/ui/profile_preferences.ui" line="965"/>
      <source>&lt;p&gt;Extra space to have before text on the right - can be set to negative to move text right beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1111"/>
      <source>&lt;p&gt;Enter the characters you&apos;d like double-clicking to stop selecting text on here. If you don&apos;t enter any, double-clicking on a word will only stop at a space, and will include characters like a double or a single quote. For example, double-clicking on the word &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; in the following will select &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;quot;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;quot;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;If you set the characters in the field to &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;quot;! &lt;/span&gt;which will mean it should stop selecting on &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; &amp;quot; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; ! then double-clicking on &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; will just select &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &amp;quot;&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;!&amp;quot;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1117"/>
      <source>(characters to ignore in selection, for example &apos; or &quot;)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1133"/>
      <source>&lt;p&gt;Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1143"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1163"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show  line and paragraphs ends with visible marks as well as whitespace.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1173"/>
      <source>&lt;p&gt;Prints Lua errors to the main console in addition to the error tab in the editor.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1193"/>
      <source>Enable text analyzer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1944"/>
      <source>Delete map:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1957"/>
      <source>Press to delete map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1964"/>
      <source>Enable &apos;Delete map&apos; button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2143"/>
      <source>Use large area exit arrows in 2D view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2394"/>
      <source>Map info background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2836"/>
      <source>Server password: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2853"/>
      <source>&lt;p&gt;TLS/SSL is usually on port 6697. IRC networks often use a &lt;b&gt;+&lt;/b&gt; when listing secure ports offered.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2856"/>
      <source>Use a secure connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3474"/>
      <source>Shortcuts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3489"/>
      <source>Main window shortcuts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3501"/>
      <source>To disable shortcut input &apos;Esc&apos; key.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3520"/>
      <source>Reset to defaults</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3776"/>
      <source>h:mm:ss.zzz</source>
      <comment>Used to set a time interval only</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1333"/>
      <source>Autocomplete</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="251"/>
      <source>Notify on new data</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="248"/>
      <source>&lt;p&gt;Show a toolbar notification if Mudlet is minimized and new data arrives.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="258"/>
      <source>Auto save on exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="275"/>
      <source>Allow server to download and play media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="272"/>
      <source>&lt;p&gt;This also needs GMCP to be enabled in the next group below.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="230"/>
      <source>System setting</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="235"/>
      <source>Light</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="240"/>
      <source>Dark</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="265"/>
      <source>Appearance</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="294"/>
      <source>Set dark theme in &lt;a href=&quot;dark-code-editor&quot;&gt;code editor&lt;/a&gt; as well?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="346"/>
      <source>Enable MSP  (Mud Sound Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="542"/>
      <source>Highlights your input line text when scrolling through your history for easy cancellation</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="545"/>
      <source>Highlight history</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="772"/>
      <source>This font is not monospace, which may not be ideal for playing some text games:
you can use it but there could be issues with aligning columns of text</source>
      <comment>Note that this text is split into two lines so that the message is not too wide in English, please do the same for other locales where the text is the same or longer</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1153"/>
      <source>&lt;p&gt;Select this option for better compatibility if you are using a netbook, or some other computer model that has a small screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1200"/>
      <source>Display control characters as:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1214"/>
      <source>nothing</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1219"/>
      <source>Unicode Control Pictures</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1224"/>
      <source>CP437 (OEM Font)- like</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1339"/>
      <source>Autocomplete Lua functions in code editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1349"/>
      <source>Advanced</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1355"/>
      <source>Shows bidirection Unicode characters which can be used to change the meaning of source code while remaining invisible to the eye</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1358"/>
      <source>Show invisible Unicode control characters</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1401"/>
      <source>&lt;p&gt;The foreground color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1424"/>
      <source>&lt;p&gt;The background color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1444"/>
      <source>&lt;p&gt;The foreground color used for the main input area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1461"/>
      <source>&lt;p&gt;The background color used for the main input area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1478"/>
      <source>&lt;p&gt;The foreground color used for text sent to the game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1498"/>
      <source>&lt;p&gt;The background color used for text sent to the game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1524"/>
      <source>These preferences set how you want a particular color to be represented visually in the main display:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1534"/>
      <location filename="../src/ui/profile_preferences.ui" line="2418"/>
      <source>Black:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1541"/>
      <source>ANSI Color Number 0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1554"/>
      <location filename="../src/ui/profile_preferences.ui" line="2438"/>
      <source>Light black:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1561"/>
      <source>ANSI Color Number 8</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1574"/>
      <location filename="../src/ui/profile_preferences.ui" line="2458"/>
      <source>Red:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1581"/>
      <source>ANSI Color Number 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1594"/>
      <location filename="../src/ui/profile_preferences.ui" line="2478"/>
      <source>Light red:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1601"/>
      <source>ANSI Color Number 9</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1614"/>
      <location filename="../src/ui/profile_preferences.ui" line="2498"/>
      <source>Green:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1621"/>
      <source>ANSI Color Number 2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1634"/>
      <location filename="../src/ui/profile_preferences.ui" line="2518"/>
      <source>Light green:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1641"/>
      <source>ANSI Color Number 10</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1654"/>
      <location filename="../src/ui/profile_preferences.ui" line="2538"/>
      <source>Yellow:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1661"/>
      <source>ANSI Color Number 3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1674"/>
      <location filename="../src/ui/profile_preferences.ui" line="2558"/>
      <source>Light yellow:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1681"/>
      <source>ANSI Color Number 11</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1694"/>
      <location filename="../src/ui/profile_preferences.ui" line="2578"/>
      <source>Blue:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1701"/>
      <source>ANSI Color Number 4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1714"/>
      <location filename="../src/ui/profile_preferences.ui" line="2598"/>
      <source>Light blue:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1721"/>
      <source>ANSI Color Number 12</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1734"/>
      <location filename="../src/ui/profile_preferences.ui" line="2618"/>
      <source>Magenta:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1741"/>
      <source>ANSI Color Number 5</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1754"/>
      <location filename="../src/ui/profile_preferences.ui" line="2638"/>
      <source>Light magenta:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1761"/>
      <source>ANSI Color Number 13</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1774"/>
      <location filename="../src/ui/profile_preferences.ui" line="2658"/>
      <source>Cyan:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1781"/>
      <source>ANSI Color Number 6</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1794"/>
      <location filename="../src/ui/profile_preferences.ui" line="2678"/>
      <source>Light cyan:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1801"/>
      <source>ANSI Color Number 14</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1814"/>
      <location filename="../src/ui/profile_preferences.ui" line="2698"/>
      <source>White:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1821"/>
      <source>ANSI Color Number 7</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1834"/>
      <location filename="../src/ui/profile_preferences.ui" line="2718"/>
      <source>Light white:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1841"/>
      <source>ANSI Color Number 15</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1854"/>
      <location filename="../src/ui/profile_preferences.ui" line="2738"/>
      <source>Reset all colors to default</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1861"/>
      <source>&lt;p&gt;If this option is checked the Mud Server may send codes to change the above 16 colors or to reset them to their defaults by using standard ANSI &lt;tt&gt;OSC&lt;/tt&gt; Escape codes.&lt;/p&gt;&lt;p&gt;Specifically &lt;tt&gt;&amp;lt;OSC&amp;gt;Pirrggbb&amp;lt;ST&amp;gt;&lt;/tt&gt; will set the color with index &lt;i&gt;i&lt;/i&gt; to have the color with the given &lt;i&gt;rr&lt;/i&gt; red, &lt;i&gt;gg&lt;/i&gt; green and &lt;i&gt;bb&lt;/i&gt;  blue components where i is a single hex-digit (&apos;0&apos; to &apos;9&apos; or &apos;a&apos; to &apos;f&apos; or &apos;A&apos; to &apos;F&apos; to give a number between 0 an d15) and rr, gg and bb are two digit hex-digits numbers (between 0 to 255); &amp;lt;OSC&amp;gt; is &lt;i&gt;Operating System Command&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;[&lt;/tt&gt; and &amp;lt;ST&amp;gt; is the &lt;i&gt;String Terminator&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Conversely &lt;tt&gt;&amp;lt;OSC&amp;gt;R&amp;lt;ST&amp;gt;&lt;/tt&gt; will reset the colors to the defaults like the button to the right does.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1864"/>
      <source>Server allowed to redefine these colors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1888"/>
      <source>Mapper</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1894"/>
      <source>Map files</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1900"/>
      <source>Save your current map:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1910"/>
      <source>Press to choose location and save</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1927"/>
      <source>Load another map file in:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1937"/>
      <source>Press to choose file and load</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1917"/>
      <source>&lt;p&gt;Mudlet now does some sanity checking and repairing to clean up issues that may have arisen in previous version due to faulty code or badly documented commands. However if significant problems are found the report can be quite extensive, in particular for larger maps.&lt;/p&gt;&lt;p&gt;Unless this option is set, Mudlet will reduce the amount of on-screen messages by hiding many texts and showing a suggestion to review the report file instead.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2036"/>
      <source>&lt;p&gt;Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2078"/>
      <source>Download latest map provided by your game:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2107"/>
      <source>&lt;p&gt;This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer.&lt;/p&gt;&lt;p&gt;3D map view always has anti-aliasing enabled.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2117"/>
      <source>&lt;p&gt;The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2192"/>
      <source>2D map player room marker style:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2202"/>
      <source>Outer ring color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2212"/>
      <source>Inner ring color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2226"/>
      <source>Original</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2231"/>
      <source>Red ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2236"/>
      <source>Blue/Yellow ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2241"/>
      <source>Custom ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2249"/>
      <source>&lt;p&gt;Percentage ratio (&lt;i&gt;the default is 120%&lt;/i&gt;) of the marker symbol to the space available for the room.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2255"/>
      <location filename="../src/ui/profile_preferences.ui" line="2283"/>
      <source>%</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2258"/>
      <source>Outer diameter: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2277"/>
      <source>&lt;p&gt;Percentage ratio of the inner diameter of the marker symbol to the outer one (&lt;i&gt;the default is 70%&lt;/i&gt;).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2286"/>
      <source>Inner diameter: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2130"/>
      <source>&lt;p&gt;This enables borders around room. Color can be set in Mapper colors tab&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2133"/>
      <source>Show room borders</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2374"/>
      <source>Room border color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2762"/>
      <source>Chat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3425"/>
      <source>Username for logging into the proxy if required</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3441"/>
      <source>Password for logging into the proxy if required</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3550"/>
      <source>Special options needed for some older game drivers (needs client restart to take effect)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3588"/>
      <source>Force CHARSET negotiation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3693"/>
      <source>the computer&apos;s password manager (secure)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3698"/>
      <source>plaintext with the profile (portable)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3786"/>
      <source>&lt;p&gt;If checked this will cause all problem Unicode codepoints to be reported in the debug output as they occur; if cleared then each different one will only be reported once and summarized in as a table when the console in which they occurred is finally destroyed (when the profile is closed).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3672"/>
      <source>&lt;p&gt;Some MUDs use a flawed interpretation of the ANSI Set Graphics Rendition (&lt;b&gt;SGR&lt;/b&gt;) code sequences for 16M color mode which only uses semi-colons and not colons to separate parameter elements i.e. instead of using a code in the form: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38:2:&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Unused&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance Color Space (0=CIELUV; 1=CIELAB)&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;where the &lt;i&gt;Color Space Id&lt;/i&gt; is expected to be an empty string to specify the usual (default) case and all of the &lt;i&gt;Parameter Elements&lt;/i&gt; (the &quot;2&quot; and the values in the &lt;tt&gt;&amp;lt;...&amp;gt;&lt;/tt&gt;s) may, technically, be omitted; they use: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;or: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;It is not possible to reliably detect the difference between these two so checking this option causes Mudlet to expect the last one with the additional (but empty!) parameter.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3675"/>
      <source>Expect Color Space Id in SGR...(3|4)8;2;...m codes</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3682"/>
      <source>Store character login passwords in:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3047"/>
      <source>TLS/SSL secure connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3148"/>
      <source>Accept self-signed certificates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3158"/>
      <source>Accept expired certificates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3065"/>
      <source>Certificate</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3125"/>
      <source>Serial:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3074"/>
      <source>Issuer:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3091"/>
      <source>Issued to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3108"/>
      <source>Expires:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3168"/>
      <source>Accept all certificate errors       (unsecure)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1920"/>
      <source>Report map issues on screen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1978"/>
      <source>Copy map to other profile(s):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1994"/>
      <source>Press to pick destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2007"/>
      <source>Copy to destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2059"/>
      <source>An action above happened</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2014"/>
      <source>Map format version:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2042"/>
      <location filename="../src/ui/profile_preferences.ui" line="2046"/>
      <source># {default version}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2069"/>
      <source>Map download</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2091"/>
      <source>Download</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2101"/>
      <source>Map view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2153"/>
      <source>2D Map Room Symbol Font</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2110"/>
      <source>Use high quality graphics in 2D view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="184"/>
      <source>&lt;p&gt;Can you help translate Mudlet?&lt;/p&gt;&lt;p&gt;If so, please visit: &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1991"/>
      <source>&lt;p&gt;Select profiles that you want to copy map to, then press the Copy button to the right&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2004"/>
      <source>&lt;p&gt;Copy map into the selected profiles on the left&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2075"/>
      <location filename="../src/ui/profile_preferences.ui" line="2088"/>
      <source>&lt;p&gt;On games that provide maps for download, you can press this button to get the latest map. Note that this will &lt;span style=&quot; font-weight:600;&quot;&gt;overwrite&lt;/span&gt; any changes you&apos;ve done to your map, and will use the new map only&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2120"/>
      <source>Show the default area in map area selection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2163"/>
      <source>Only use symbols (glyphs) from chosen font</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2170"/>
      <source>Show symbol usage...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2319"/>
      <source>Mapper colors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2325"/>
      <source>Select your color preferences for the map display</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2331"/>
      <source>Link color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2354"/>
      <source>Background color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3544"/>
      <source>Special Options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3556"/>
      <source>Force compression off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3574"/>
      <source>Force telnet GA signal interpretation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3563"/>
      <source>This option adds a line line break &lt;LF&gt; or &quot;
&quot; to your command input on empty commands. This option will rarely be necessary.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3567"/>
      <source>Force new line on empty commands</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3581"/>
      <source>Force MXP negotiation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2866"/>
      <source>Discord privacy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2899"/>
      <source>Don&apos;t hide small icon or tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2904"/>
      <source>Hide small icon tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2909"/>
      <source>Hide small icon and tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2931"/>
      <source>Hide timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2954"/>
      <location filename="../src/ui/profile_preferences.ui" line="2967"/>
      <location filename="../src/ui/profile_preferences.ui" line="2983"/>
      <location filename="../src/ui/profile_preferences.ui" line="2999"/>
      <source>&lt;p&gt;Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). Leave empty to show it for any Discord account you log in to.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2960"/>
      <source>Restrict to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2873"/>
      <source>Don&apos;t hide large icon or tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2878"/>
      <source>Hide large icon tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2883"/>
      <source>Hide large icon and tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2938"/>
      <source>&lt;p&gt;Allow Lua to set Discord status&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2941"/>
      <source>Enable Lua API</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2973"/>
      <source>specific Discord username</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2917"/>
      <source>Hide state</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2924"/>
      <source>Hide party details</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2891"/>
      <source>Hide detail</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2768"/>
      <source>IRC client options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2784"/>
      <source>irc.example.net</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2791"/>
      <source>Port:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2829"/>
      <source>#channel1 #channel2 #etc...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2815"/>
      <source>MudletUser123</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2774"/>
      <source>Server address:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2822"/>
      <source>Auto-join channels: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2808"/>
      <source>Nickname:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2801"/>
      <source>6667</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3638"/>
      <source>Search Engine</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3622"/>
      <source>Mudlet updates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3628"/>
      <source>Disable automatic updates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3650"/>
      <source>Other Special options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3656"/>
      <source>Show icons on menus</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3041"/>
      <source>Connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3380"/>
      <source>Connect to the game via proxy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3395"/>
      <source>Address</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3415"/>
      <source>port</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3431"/>
      <source>username (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2846"/>
      <location filename="../src/ui/profile_preferences.ui" line="3447"/>
      <source>password (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3742"/>
      <source>Show debug messages for timers not smaller than:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3739"/>
      <source>&lt;p&gt;Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the &lt;i&gt;Central Debug Console&lt;/i&gt; useless if there is a very small interval timer running.&lt;/p&gt;</source>
      <comment>The term in &apos;...&apos; refer to a Mudlet specific thing and ought to match the corresponding translation elsewhere.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3789"/>
      <source>Report all Codepoint problems immediately</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3796"/>
      <source>Additional text wait time:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3806"/>
      <source>&lt;p&gt;&lt;i&gt;Go-Ahead&lt;/i&gt; (&lt;tt&gt;GA&lt;/tt&gt;) and &lt;i&gt;End-of-record&lt;/i&gt; (&lt;tt&gt;EOR&lt;/tt&gt;) signalling tells Mudlet when the game server is done sending text. On games that do not provide &lt;tt&gt;GA&lt;/tt&gt; or &lt;tt&gt;EOR&lt;/tt&gt;, this option controls how long Mudlet will wait for more text to arrive. Greater values will help reduce the risk that Mudlet will split a large piece of text (with unintended line-breaks in the middle) which can stop some triggers from working. Lesser values increases the risk of text getting broken up, but may make the game feel more responsive.&lt;/p&gt;&lt;p&gt;&lt;i&gt;The default value, which was what Mudlet used before this control was added, is 0.300 Seconds.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3809"/>
      <source> seconds</source>
      <extracomment>For most locales a space should be included so that the text is separated from the number!</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3879"/>
      <source>Save</source>
      <translation>Spara</translation>
    </message>
  </context>
  <context>
    <name>room_exits</name>
    <message>
      <location filename="../src/ui/room_exits.ui" line="37"/>
      <source>General exits:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="93"/>
      <source>NW exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="103"/>
      <location filename="../src/ui/room_exits.ui" line="240"/>
      <location filename="../src/ui/room_exits.ui" line="377"/>
      <location filename="../src/ui/room_exits.ui" line="514"/>
      <location filename="../src/ui/room_exits.ui" line="654"/>
      <location filename="../src/ui/room_exits.ui" line="858"/>
      <location filename="../src/ui/room_exits.ui" line="992"/>
      <location filename="../src/ui/room_exits.ui" line="1147"/>
      <location filename="../src/ui/room_exits.ui" line="1284"/>
      <location filename="../src/ui/room_exits.ui" line="1421"/>
      <location filename="../src/ui/room_exits.ui" line="1558"/>
      <location filename="../src/ui/room_exits.ui" line="1831"/>
      <source>&lt;p&gt;Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="230"/>
      <source>N exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="367"/>
      <source>NE exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="504"/>
      <source>Up exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="644"/>
      <source>W exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="752"/>
      <source>ID:</source>
      <translation>ID:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="775"/>
      <source>Weight:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="848"/>
      <source>E exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="982"/>
      <source>Down exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1131"/>
      <source>SW exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1274"/>
      <source>S exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1411"/>
      <source>SE exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1548"/>
      <source>In exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1821"/>
      <source>Out exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1653"/>
      <source>Key:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1671"/>
      <source>No route</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1686"/>
      <source>Stub Exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="67"/>
      <location filename="../src/ui/room_exits.ui" line="204"/>
      <location filename="../src/ui/room_exits.ui" line="341"/>
      <location filename="../src/ui/room_exits.ui" line="478"/>
      <location filename="../src/ui/room_exits.ui" line="615"/>
      <location filename="../src/ui/room_exits.ui" line="822"/>
      <location filename="../src/ui/room_exits.ui" line="956"/>
      <location filename="../src/ui/room_exits.ui" line="1099"/>
      <location filename="../src/ui/room_exits.ui" line="1248"/>
      <location filename="../src/ui/room_exits.ui" line="1385"/>
      <location filename="../src/ui/room_exits.ui" line="1522"/>
      <location filename="../src/ui/room_exits.ui" line="1795"/>
      <location filename="../src/ui/room_exits.ui" line="1979"/>
      <source>&lt;p&gt;Prevent a route being created via this exit, equivalent to an infinite exit weight.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="83"/>
      <location filename="../src/ui/room_exits.ui" line="220"/>
      <location filename="../src/ui/room_exits.ui" line="357"/>
      <location filename="../src/ui/room_exits.ui" line="494"/>
      <location filename="../src/ui/room_exits.ui" line="631"/>
      <location filename="../src/ui/room_exits.ui" line="838"/>
      <location filename="../src/ui/room_exits.ui" line="972"/>
      <location filename="../src/ui/room_exits.ui" line="1121"/>
      <location filename="../src/ui/room_exits.ui" line="1264"/>
      <location filename="../src/ui/room_exits.ui" line="1401"/>
      <location filename="../src/ui/room_exits.ui" line="1538"/>
      <location filename="../src/ui/room_exits.ui" line="1811"/>
      <source>&lt;p&gt;Create an exit in this direction with unknown destination, mutually exclusive with an actual exit roomID.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="116"/>
      <location filename="../src/ui/room_exits.ui" line="253"/>
      <location filename="../src/ui/room_exits.ui" line="390"/>
      <location filename="../src/ui/room_exits.ui" line="527"/>
      <location filename="../src/ui/room_exits.ui" line="667"/>
      <location filename="../src/ui/room_exits.ui" line="868"/>
      <location filename="../src/ui/room_exits.ui" line="1005"/>
      <location filename="../src/ui/room_exits.ui" line="1160"/>
      <location filename="../src/ui/room_exits.ui" line="1297"/>
      <location filename="../src/ui/room_exits.ui" line="1434"/>
      <location filename="../src/ui/room_exits.ui" line="1571"/>
      <location filename="../src/ui/room_exits.ui" line="1844"/>
      <location filename="../src/ui/room_exits.ui" line="1997"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="135"/>
      <location filename="../src/ui/room_exits.ui" line="272"/>
      <location filename="../src/ui/room_exits.ui" line="409"/>
      <location filename="../src/ui/room_exits.ui" line="686"/>
      <location filename="../src/ui/room_exits.ui" line="887"/>
      <location filename="../src/ui/room_exits.ui" line="1179"/>
      <location filename="../src/ui/room_exits.ui" line="1316"/>
      <location filename="../src/ui/room_exits.ui" line="1453"/>
      <source>&lt;p&gt;Green (Open) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="151"/>
      <location filename="../src/ui/room_exits.ui" line="288"/>
      <location filename="../src/ui/room_exits.ui" line="425"/>
      <location filename="../src/ui/room_exits.ui" line="702"/>
      <location filename="../src/ui/room_exits.ui" line="903"/>
      <location filename="../src/ui/room_exits.ui" line="1195"/>
      <location filename="../src/ui/room_exits.ui" line="1332"/>
      <location filename="../src/ui/room_exits.ui" line="1469"/>
      <source>&lt;p&gt;Orange (Closed) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="167"/>
      <location filename="../src/ui/room_exits.ui" line="304"/>
      <location filename="../src/ui/room_exits.ui" line="441"/>
      <location filename="../src/ui/room_exits.ui" line="718"/>
      <location filename="../src/ui/room_exits.ui" line="919"/>
      <location filename="../src/ui/room_exits.ui" line="1211"/>
      <location filename="../src/ui/room_exits.ui" line="1348"/>
      <location filename="../src/ui/room_exits.ui" line="1485"/>
      <source>&lt;p&gt;Red (Locked) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="546"/>
      <location filename="../src/ui/room_exits.ui" line="1024"/>
      <location filename="../src/ui/room_exits.ui" line="1590"/>
      <location filename="../src/ui/room_exits.ui" line="1863"/>
      <source>&lt;p&gt;A symbol is drawn with a green (Open) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="562"/>
      <location filename="../src/ui/room_exits.ui" line="1040"/>
      <location filename="../src/ui/room_exits.ui" line="1606"/>
      <location filename="../src/ui/room_exits.ui" line="1879"/>
      <source>&lt;p&gt;A symbol is drawn with an orange (Closed) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="578"/>
      <location filename="../src/ui/room_exits.ui" line="1056"/>
      <location filename="../src/ui/room_exits.ui" line="1622"/>
      <location filename="../src/ui/room_exits.ui" line="1895"/>
      <source>&lt;p&gt;A symbol is drawn with a red (Locked) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="765"/>
      <source>&lt;p&gt;This is the Room ID Number for this room - it cannot be changed here!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="788"/>
      <source>&lt;p&gt;This is the default weight for this room, which will be used for any exit &lt;i&gt;that leads to &lt;u&gt;this room&lt;/u&gt;&lt;/i&gt; which does not have its own value set - this value cannot be changed here.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1698"/>
      <source>Exit RoomID number</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1725"/>
      <source>No door</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1740"/>
      <source>Open door</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1752"/>
      <source>Closed door</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1764"/>
      <source>Locked door</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1923"/>
      <source>&lt;p&gt;Click on an item to edit/change it. To delete a Special Exit, ether: select it and press the keyboard Delete key; or set its Exit roomID to less than one; or clear the name/command entry.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1966"/>
      <source>Exit
Status</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1970"/>
      <source>&lt;p&gt;Indicates whether the exit is invalid, leads to another room in this area or leads to a room in another area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2006"/>
      <source>&lt;p&gt;Green (Open) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2015"/>
      <source>&lt;p&gt;Orange (Closed) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2024"/>
      <source>&lt;p&gt;Red (Locked) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2033"/>
      <source>&lt;p&gt;(Lua scripts for those profiles using the &lt;tt&gt;mudlet-mapper&lt;/tt&gt; package need to be prefixed with &quot;script:&quot;).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2101"/>
      <source>&lt;p&gt;Use this button to save any changes, will also remove any invalid Special exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2117"/>
      <source>&lt;p&gt;Use this button to close the dialogue without changing anything.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1961"/>
      <source>&lt;p&gt;Set the number of the room that this exit leads to, if set to zero the exit will be removed on saving the exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2050"/>
      <source>&lt;p&gt;Add an empty item to Special exits to be edited as required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2069"/>
      <source>&lt;p&gt;Press this button to deactivate the selection of a Special exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2104"/>
      <source>&amp;Save</source>
      <translation>&amp;Spara</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1710"/>
      <source>Exit Weight (0=No override)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2120"/>
      <source>&amp;Cancel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1914"/>
      <source>Special exits:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1957"/>
      <source>Exit
Room ID</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1975"/>
      <source>No
Route</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1984"/>
      <source>Exit
Weight</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1988"/>
      <source>&lt;p&gt;Set to a positive integer value to override the default (Room) Weight for using this Exit route, a zero value assigns the default.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1993"/>
      <source>Door
None</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2002"/>
      <source>Door
Open</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2011"/>
      <source>Door
Closed</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2020"/>
      <source>Door
Locked</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2029"/>
      <source>Command
or LUA script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2053"/>
      <source>&amp;Add special exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2072"/>
      <source>&amp;End S. Exits editing</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>room_symbol</name>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="14"/>
      <location filename="../src/ui/room_symbol.ui" line="112"/>
      <source>Room symbol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="141"/>
      <source>Color of to use for the room symbol(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="147"/>
      <source>Symbol color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="154"/>
      <source>Reset</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>scripts_main_area</name>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.&lt;/p&gt;&lt;p&gt;If a function within the script is to be used to handle events entered in the list below &lt;b&gt;&lt;u&gt;it must have the same name as is entered here.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="40"/>
      <source>Registered Event Handlers:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="88"/>
      <source>&lt;p&gt;Remove (selected) event handler from list.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="91"/>
      <source>-</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="98"/>
      <source>Add User Event Handler:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="133"/>
      <source>&lt;p&gt;Add entered event handler name to list.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="136"/>
      <source>+</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>set_room_area</name>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="14"/>
      <source>Move rooms to another area</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="20"/>
      <source>Which area would you like to move the room(s) to?</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>source_editor_area</name>
    <message>
      <location filename="../src/ui/source_editor_area.ui" line="26"/>
      <source>Form</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>timers_main_area</name>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="29"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="64"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="74"/>
      <source>&lt;p&gt;Type in one or more commands you want the timer to send directly to the game when the time has elapsed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="134"/>
      <source>&lt;p&gt;milliseconds&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="169"/>
      <source>Time:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="39"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="77"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="89"/>
      <source>&lt;p&gt;hours&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="104"/>
      <source>&lt;p&gt;minutes&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="119"/>
      <source>&lt;p&gt;seconds&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="184"/>
      <source>&lt;p&gt;The &lt;b&gt;hour&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="392"/>
      <source>&lt;p&gt;The &lt;b&gt;millisecond&lt;/b&gt; part of the interval that the timer will go off at (1000 milliseconds = 1 second).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="260"/>
      <source>&lt;p&gt;The &lt;b&gt;minute&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="326"/>
      <source>&lt;p&gt;The &lt;b&gt;second&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>trigger_editor</name>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="152"/>
      <location filename="../src/ui/trigger_editor.ui" line="200"/>
      <location filename="../src/ui/trigger_editor.ui" line="245"/>
      <location filename="../src/ui/trigger_editor.ui" line="290"/>
      <location filename="../src/ui/trigger_editor.ui" line="335"/>
      <location filename="../src/ui/trigger_editor.ui" line="380"/>
      <location filename="../src/ui/trigger_editor.ui" line="428"/>
      <location filename="../src/ui/trigger_editor.ui" line="590"/>
      <source>1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="436"/>
      <source>Show normally hidden variables</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="476"/>
      <source>&lt;p&gt;Enter text here to search through your code.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="510"/>
      <source>&lt;p&gt;Toggles the display of the search results area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>trigger_main_area</name>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="65"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="85"/>
      <source>&lt;p&gt;Use this control to show or hide the extra controls for the trigger; this can be used to allow more space to show the trigger &lt;i&gt;items&lt;/i&gt; on smaller screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="111"/>
      <source>Command:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="124"/>
      <source>&lt;p&gt;Type in one or more commands you want the trigger to send directly to the game if it fires. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="127"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="201"/>
      <source>&lt;p&gt;The trigger will only fire if &lt;u&gt;all&lt;/u&gt; conditions on the list have been met within the specified line delta, and captures will be saved in &lt;tt&gt;multimatches&lt;/tt&gt; instead of &lt;tt&gt;matches&lt;/tt&gt;.&lt;/p&gt;
&lt;p&gt;If this option is &lt;b&gt;not&lt;/b&gt; set the trigger will fire if &lt;u&gt;any&lt;/u&gt; condition on the list have been met.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="208"/>
      <source>AND / Multi-line (delta)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="260"/>
      <source>&lt;p&gt;When checked, only the filtered content (=capture groups) will be passed on to child triggers, not the initial line (see manual on filters).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="266"/>
      <source>only pass matches</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="302"/>
      <source>Do not pass whole line to children.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="324"/>
      <source>&lt;p&gt;Choose this option if you want to include all possible matches of the pattern in the line.&lt;/p&gt;&lt;p&gt;Without this option, the pattern matching will stop after the first successful match.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="415"/>
      <source>&lt;p&gt;Keep firing the script for this many more lines, after the trigger or chain has matched.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="421"/>
      <source>fire length (extra lines)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="479"/>
      <source>&lt;p&gt;Play a sound file if the trigger fires.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="524"/>
      <source>&lt;p&gt;Use this to open a file selection dialogue to find a sound file to play when the trigger fires.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Cancelling from the file dialogue will not make any changes; to clear the file use the clear button to the right of the file name display.&lt;/i&gt;&lt;/p&gt;</source>
      <comment>This is the button used to select a sound file to be played when a trigger fires.</comment>
      <extracomment>Please ensure the text used here is duplicated within the tooltip for the QLineEdit that displays the file name selected.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="528"/>
      <source>Choose file...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="559"/>
      <source>no file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="620"/>
      <source>&lt;p&gt;Enable this to highlight the matching text by changing the fore and background colors to the ones selected here.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="663"/>
      <source>Background</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="679"/>
      <location filename="../src/ui/triggers_main_area.ui" line="692"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="653"/>
      <source>Foreground</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="482"/>
      <source>play sound</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="78"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="247"/>
      <source>&lt;p&gt;Within how many lines must all conditions be true to fire the trigger?&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="330"/>
      <source>match all</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="360"/>
      <source>Match all occurrences of the pattern in the line.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="463"/>
      <source>&lt;p&gt;How many more lines, after the one that fired the trigger, should be passed to the trigger&apos;s children?&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="550"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <comment>This is the tooltip for the QLineEdit that shows - but does not permit changing - the sound file used for a trigger.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="572"/>
      <source>&lt;p&gt;Click to remove the sound file set for this trigger.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="626"/>
      <source>highlight</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>trigger_pattern_edit</name>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="124"/>
      <source>Foreground color ignored</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="148"/>
      <source>Background color ignored</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="175"/>
      <source>match on the prompt line</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>vars_main_area</name>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="109"/>
      <source>Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="175"/>
      <source>⏴ Key type:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="125"/>
      <source>&lt;p&gt;Set the &lt;i&gt;global variable&lt;/i&gt; or the &lt;i&gt;table entry&lt;/i&gt; name here. The name has to start with a letter, but can contain a mix of letters and numbers.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="51"/>
      <location filename="../src/ui/vars_main_area.ui" line="145"/>
      <source>Auto-Type</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="128"/>
      <source>Variable name ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="138"/>
      <source>&lt;p&gt;Tables can store values either in a list, and/or a hashmap.&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;list&lt;/b&gt;, &lt;i&gt;unique indexed keys&lt;/i&gt; represent values - so you can have values at &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;map&lt;/b&gt; {a.k.a. an &lt;i&gt;associative array}&lt;/i&gt;, &lt;i&gt;unique keys&lt;/i&gt; represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).&lt;/p&gt;&lt;p&gt;This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.&lt;/p&gt;&lt;p&gt;In other cases it displays other entities (&lt;span style=&quot; font-style:italic;&quot;&gt;functions&lt;/span&gt;) which cannot be modified from here.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="150"/>
      <source>key (string)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="155"/>
      <source>index (integer)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="71"/>
      <location filename="../src/ui/vars_main_area.ui" line="160"/>
      <source>table</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="165"/>
      <source>function
(cannot create
from GUI)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="185"/>
      <source>&lt;p&gt;If checked this item (and its children, if applicable) does not show in area to the left unless &lt;b&gt;Show normally hidden variables&lt;/b&gt; is checked.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="188"/>
      <source>hidden variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="84"/>
      <source>⏷ Value type:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="56"/>
      <source>string</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="61"/>
      <source>number</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="66"/>
      <source>boolean</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="76"/>
      <source>function</source>
      <translation type="unfinished"/>
    </message>
  </context>
</TS>
