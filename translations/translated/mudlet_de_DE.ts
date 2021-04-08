<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de" sourcelanguage="en">
  <context>
    <name>Discord</name>
    <message>
      <location filename="../src/discord.cpp" line="149"/>
      <source>via Mudlet</source>
      <translation>via Mudlet</translation>
    </message>
  </context>
  <context>
    <name>Feed</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="275"/>
      <source>Too many redirects.</source>
      <translation>Zu viele Weiterleitungen.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="284"/>
      <source>No data received from server</source>
      <translation>Keine Daten vom Server empfangen</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="295"/>
      <source>Could not verify download integrity.</source>
      <translation>Die Integrität des Downloads konnte nicht überprüft werden.</translation>
    </message>
  </context>
  <context>
    <name>GLWidget</name>
    <message>
      <location filename="../src/glwidget.cpp" line="286"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Du hast noch keine Karte - lade eine oder beginne neu zu kartographieren.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/glwidget.cpp" line="285"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>Du hast eine Karte geladen (mit einem Raum), aber Mudlet weiß nicht, wo du gerade bist.</numerusform>
        <numerusform>Du hast eine Karte geladen (mit %n Räumen), aber Mudlet weiß nicht, wo du gerade bist.</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>Host</name>
    <message>
      <location filename="../src/Host.cpp" line="460"/>
      <source>Text to send to the game</source>
      <translation>Text, der zum Spiel gesendet werden soll</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="853"/>
      <source>[  OK  ]  - %1 Thanks a lot for using the Public Test Build!</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ]  - %1 Vielen Dank für die Nutzung der öffentlichen Testversion!</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="854"/>
      <source>[  OK  ]  - %1 Help us make Mudlet better by reporting any problems.</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ]  - %1 Hilf uns, Mudlet zu verbessern, indem Du Probleme meldest.</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1653"/>
      <source>Unpacking module:
&quot;%1&quot;
please wait...</source>
      <translation>Entpacke Modul:
&quot;%1&quot;
Bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1655"/>
      <source>Unpacking package:
&quot;%1&quot;
please wait...</source>
      <translation>Entpacke Paket:
&quot;%1&quot;
Bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1659"/>
      <source>Unpacking</source>
      <translation>Entpacke</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2300"/>
      <source>Playing %1</source>
      <translation>Spielt %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2302"/>
      <location filename="../src/Host.cpp" line="2308"/>
      <source>%1 at %2:%3</source>
      <comment>%1 is the game name and %2:%3 is game server address like: mudlet.org:23</comment>
      <translation>%1 bei %2:%3</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2688"/>
      <location filename="../src/Host.cpp" line="3620"/>
      <source>Map - %1</source>
      <translation>Karte - %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3636"/>
      <source>Pre-Map loading(3) report</source>
      <translation>Bericht bevor die Karte geladen wird(3)</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3646"/>
      <source>Loading map(3) at %1 report</source>
      <translation>Bericht der geladenen Karte(3) bei %1</translation>
    </message>
  </context>
  <context>
    <name>KeyUnit</name>
    <message>
      <location filename="../src/KeyUnit.cpp" line="333"/>
      <source>%1undefined key (code: 0x%2)</source>
      <comment>%1 is a string describing the modifier keys (e.g. &quot;shift&quot; or &quot;control&quot;) used with the key, whose &apos;code&apos; number, in %2 is not one that we have a name for. This is probably one of those extra keys around the edge of the keyboard that some people have.</comment>
      <translation>%1unbekannte Taste (Code: 0x%2)</translation>
    </message>
  </context>
  <context>
    <name>MapInfoContributorManager</name>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="116"/>
      <source>Area:%1%2 ID:%1%3 x:%1%4%1&lt;‑&gt;%1%5 y:%1%6%1&lt;‑&gt;%1%7 z:%1%8%1&lt;‑&gt;%1%9
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and non-breaking hyphens which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. %2 is the (text) name of the area, %3 is the number for it, %4 to %9 are pairs (min &lt;-&gt; max) of extremes for each of x,y and z coordinates</comment>
      <translation>Bereich:%1%2 ID:%1%3 x:%1%4%1&lt;‑&gt;%1%5 y:%1%6%1&lt;‑&gt;%1%7 z:%1%8%1&lt;‑&gt;%1%9
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="140"/>
      <source>Room Name: %1
</source>
      <translation>Raumname: %1
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="153"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when NO rooms are selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the current player&apos;s room.</comment>
      <translation>Raum%1ID:%1%2 Position%1auf%1der%1Karte: (%3,%4,%5) ‑%1Aktuelle Spielposition
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="170"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when ONE room is selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the selected Room.</comment>
      <translation>Raum%1ID:%1%2 Position%1auf%1der%1Karte: (%3,%4,%5) ‑%1Ausgewählter Raum
</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mapInfoContributorManager.cpp" line="188"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
      <translation>
        <numerusform>Raum%1ID:%1%2 Position%1auf%1der%1Karte: (%3,%4,%5) ‑%1Mittelpunkt der %n ausgewählten Räume</numerusform>
        <numerusform>Raum%1ID:%1%2 Position%1auf%1der%1Karte: (%3,%4,%5) ‑%1Mittelpunkt der %n ausgewählten Räume</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="142"/>
      <source>! %1</source>
      <translation>! %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="144"/>
      <source>! %1 is away (%2)</source>
      <translation>! %1 ist nicht da (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="146"/>
      <source>! %1 is back</source>
      <translation>! %1 ist zurück</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="153"/>
      <source>! invited %1 to %2</source>
      <translation>! Du hast %1 zu %2 eingeladen</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="156"/>
      <source>! %2 invited to %3</source>
      <translation>! %2 eingeladen nach %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="163"/>
      <source>! You have joined %1 as %2</source>
      <translation>! Du bist %1 als %2 beigetreten</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="165"/>
      <source>! %1 has joined %2</source>
      <translation>! %1 ist %2 beigetreten</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="172"/>
      <source>! %1 kicked %2</source>
      <translation>! %1 hat %2 gekickt</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="180"/>
      <source>! %1 mode is %2 %3</source>
      <translation>! %1 Modus ist %2 %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="182"/>
      <source>! %1 sets mode %2 %3 %4</source>
      <translation>! %1 setzt Modus %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="199"/>
      <source>[MOTD] %1%2</source>
      <translation>[MOTD] %1%2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="211"/>
      <source>! %1 has %2 users: %3</source>
      <translation>! %1 hat %2 Benutzer: %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="213"/>
      <source>! %1 has %2 users</source>
      <translation>! %1 hat %2 Benutzer</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="220"/>
      <source>! %1 has changed nick to %2</source>
      <translation>! %1 hat Nick auf %2 geändert</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="234"/>
      <source>! %1 replied in %2</source>
      <translation>! %1 antwortete in %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="237"/>
      <location filename="../src/ircmessageformatter.cpp" line="286"/>
      <source>! %1 time is %2</source>
      <translation>! %1 Zeit ist %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="240"/>
      <location filename="../src/ircmessageformatter.cpp" line="283"/>
      <source>! %1 version is %2</source>
      <translation>! %1 Version ist %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="256"/>
      <source>[%1%2] %3</source>
      <translation>[%1%2] %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="264"/>
      <source>&amp;lt;%1%2&amp;gt; [%3] %4</source>
      <translation>&amp;lt;%1%2&amp;gt; [%3] %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="278"/>
      <source>[INFO] %1</source>
      <translation>[INFO]  %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="305"/>
      <location filename="../src/ircmessageformatter.cpp" line="331"/>
      <source>[ERROR] %1</source>
      <translation>[FEHLER] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="315"/>
      <source>[Channel URL] %1</source>
      <translation>[Kanal URL] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="324"/>
      <source>[%1] %2</source>
      <translation>[%1] %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="338"/>
      <source>! %1 has left %2</source>
      <translation>! %1 hat %2 verlassen</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="340"/>
      <source>! %1 has left %2 (%3)</source>
      <translation>! %1 hat %2 verlassen (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="349"/>
      <source>! %1 replied in %2 seconds</source>
      <translation>! %1 antwortete in %2 Sekunden</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="363"/>
      <source>* %1 %2</source>
      <translation>* %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="369"/>
      <source>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</source>
      <translation>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="378"/>
      <source>! %1 has quit</source>
      <translation>! %1 hat beendet</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="380"/>
      <source>! %1 has quit (%2)</source>
      <translation>! %1 hat beendet (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="388"/>
      <source>! no topic</source>
      <translation>! kein Thema</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="397"/>
      <source>[TOPIC] %1</source>
      <translation>[THEMA] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="401"/>
      <source>! %2 cleared topic</source>
      <translation>! %2 löschte das Thema</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="404"/>
      <source>! %2 changed topic</source>
      <translation>! %2 änderte das Thema</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="410"/>
      <source>? %2 %3 %4</source>
      <translation>? %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="417"/>
      <source>[WHOIS] %1 is %2@%3 (%4)</source>
      <translation>[WHOIS] %1 ist %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="418"/>
      <source>[WHOIS] %1 is connected via %2 (%3)</source>
      <translation>[WHOIS] %1 ist verbunden via %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="419"/>
      <source>[WHOIS] %1 is connected since %2 (idle %3)</source>
      <translation>[WHOIS] %1 ist verbunden seit %2 (inaktiv %3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="421"/>
      <source>[WHOIS] %1 is away: %2</source>
      <translation>[WHOIS] %1 ist weg: %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="424"/>
      <source>[WHOIS] %1 is logged in as %2</source>
      <translation>[WHOIS] %1 ist angemeldet als %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="427"/>
      <source>[WHOIS] %1 is connected from %2</source>
      <translation>[WHOIS] %1 ist verbunden von %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="430"/>
      <source>[WHOIS] %1 is using a secure connection</source>
      <translation>[WHOIS] %1 benutzt eine sichere Verbindung</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="433"/>
      <source>[WHOIS] %1 is on %2</source>
      <translation>[WHOIS] %1 ist auf %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="442"/>
      <source>[WHOWAS] %1 was %2@%3 (%4)</source>
      <translation>[WHOWAS]%1 war %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="443"/>
      <source>[WHOWAS] %1 was connected via %2 (%3)</source>
      <translation>[WHOWAS] %1 war verbunden via %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="445"/>
      <source>[WHOWAS] %1 was logged in as %2</source>
      <translation>[WHOWAS] %1 war angemeldet als %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="453"/>
      <source>[WHO] %1 (%2)</source>
      <translation>[WHO] %1 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="455"/>
      <source> - away</source>
      <translation> - weg</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="458"/>
      <source> - server operator</source>
      <translation> - Serverbetreiber</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="466"/>
      <source>%1s</source>
      <translation>%1s</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="473"/>
      <source>%1 days</source>
      <translation>%1 Tage</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="477"/>
      <source>%1 hours</source>
      <translation>%1 Stunden</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="481"/>
      <source>%1 mins</source>
      <translation>%1 Minuten</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="483"/>
      <source>%1 secs</source>
      <translation>%1 Sekunden</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="55"/>
      <source>Start element not found!</source>
      <translation>Start-Element nicht gefunden!</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="67"/>
      <source>line %1: %2</source>
      <translation>Zeile %1: %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="149"/>
      <source>Expected %1 while parsing</source>
      <translation>%1 erwartet während der Analyse</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/jsonparser.cpp" line="145"/>
      <source>%1 @ line %2</source>
      <translation>%1 @ Zeile %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="82"/>
      <source>No data found!</source>
      <translation>Keine Daten gefunden!</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="89"/>
      <source>Expected object in keymap
</source>
      <translation>Erwartetes Objekt in 'keymap'
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="129"/>
      <source>Invalid keysequence used %1
</source>
      <translation>Ungültige Tastenfolge verwendet %1
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/texteditorkeymap.cpp" line="371"/>
      <source>Error parsing %1: %2 </source>
      <translation>Fehler beim Parsen von %1: %2 </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/textgrammar.cpp" line="306"/>
      <source>Error reading file %1:%2</source>
      <translation>Fehler beim Lesen der Datei %1:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="435"/>
      <source>%1 ranges</source>
      <translation>%1 Bereiche</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="441"/>
      <source>Line %1, Column %2</source>
      <translation>Zeile %1, Spalte %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="444"/>
      <source>, Offset %1</source>
      <translation>, Versatz %1</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="448"/>
      <source> | %1 chars selected</source>
      <translation> | %1 Zeichen ausgewählt</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="452"/>
      <source> | scope: </source>
      <translation> | Umfang: </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="462"/>
      <source> (%1)</source>
      <translation> (%1)</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="399"/>
      <source>Error parsing theme %1:%2</source>
      <translation>Fehler beim Analysieren des Themas %1:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="404"/>
      <source>Error theme not found %1.</source>
      <translation>Fehler: Thema nicht gefunden %1.</translation>
    </message>
  </context>
  <context>
    <name>T2DMap</name>
    <message>
      <location filename="../src/T2DMap.cpp" line="2789"/>
      <source>Create a new room here</source>
      <translation>Erstelle hier einen neuen Raum</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2962"/>
      <source>Change the properties of this custom line</source>
      <translation>Ändere die Eigenschaften dieser benutzerdefinierten Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3193"/>
      <location filename="../src/T2DMap.cpp" line="4808"/>
      <source>Solid line</source>
      <translation>Durchgezogene Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3194"/>
      <location filename="../src/T2DMap.cpp" line="4809"/>
      <source>Dot line</source>
      <translation>Gepunktete Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3195"/>
      <location filename="../src/T2DMap.cpp" line="4810"/>
      <source>Dash line</source>
      <translation>Gestrichelte Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3196"/>
      <location filename="../src/T2DMap.cpp" line="4811"/>
      <source>Dash-dot line</source>
      <translation>Strich-Punkt-Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3197"/>
      <location filename="../src/T2DMap.cpp" line="4812"/>
      <source>Dash-dot-dot line</source>
      <translation>Strich-Punkt-Punkt-Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3485"/>
      <source>x coordinate (was %1):</source>
      <translation>x-Koordinate (war %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3486"/>
      <source>y coordinate (was %1):</source>
      <translation>y-Koordinate (war %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3487"/>
      <source>z coordinate (was %1):</source>
      <translation>z-Koordinate (war %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3675"/>
      <source>Delete color</source>
      <comment>Deletes an environment colour</comment>
      <translation>Farbe löschen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3693"/>
      <source>Define new color</source>
      <translation>Neue Farbe definieren</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4007"/>
      <source>%1 {count:%2}</source>
      <translation>%1 {Anzahl: %2}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1378"/>
      <location filename="../src/T2DMap.cpp" line="1499"/>
      <location filename="../src/T2DMap.cpp" line="2310"/>
      <location filename="../src/T2DMap.cpp" line="2326"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>ohne Text</translation>
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
      <translation>Name</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="144"/>
      <source>&lt;p&gt;Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection.  Click on the relevant header to sort by that method.  Note that the name column will only show if at least one of the rooms has a name.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klick auf eine Zeile, um diese Raumnummer an- oder abzuwählen. Wenn der Raum einen Namen hat, wird dieser auch ausgewählt. Durch einen Klick auf die entsprechende Kopfzelle wird nach der Spalte sortiert. Hinweis: Die Spalte "Name" erscheint nur, wenn mindestens ein Raum einen Namen hat.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="602"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation>Mapper: Kein Weg von %1 nach %2 konnte mit bekannten Ausgängen gefunden werden.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1253"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Du hast noch keine Karte - lade eine oder beginne neu zu kartographieren.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/T2DMap.cpp" line="1252"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>Du hast eine Karte geladen (mit einem Raum), aber Mudlet weiß nicht, wo du gerade bist.</numerusform>
        <numerusform>Du hast eine Karte geladen (mit %n Räumen), aber Mudlet weiß nicht, wo du gerade bist.</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2244"/>
      <source>render time: %1S mO: (%2,%3,%4)</source>
      <comment>This is debug information that is not expected to be seen in release versions, %1 is a decimal time period and %2-%4 are the x,y and z coordinates at the center of the view (but y will be negative compared to previous room related ones as it represents the real coordinate system for this widget which has y increasing in a downward direction!)</comment>
      <translation>Berechnungszeit: %1S mO: (%2,%3,%4)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2316"/>
      <source>Text label or image label?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation>Text oder Bild?</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2317"/>
      <source>Text Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Textmarke</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2318"/>
      <source>Image Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Bildmarke</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2322"/>
      <source>Enter label text.</source>
      <comment>2D Mapper create label dialog title/text</comment>
      <translation>Text der Markierung eingeben.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2329"/>
      <source>Background color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Hintergrundfarbe</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2330"/>
      <source>Foreground color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Vordergrundfarbe</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2334"/>
      <source>Select image</source>
      <comment>2D Mapper create label file dialog title</comment>
      <translation>Bild auswählen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2341"/>
      <source>Draw label as background or on top of everything?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation>Markierung als Hintergrund zeichnen oder alles überlagern?</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2342"/>
      <source>Background</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Hintergrund</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2343"/>
      <source>Foreground</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Vordergrund</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2585"/>
      <source>Drag to select multiple rooms or labels, release to finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Ziehen zum Auswählen mehrerer Räume oder Marken, loslassen zum Beenden...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2689"/>
      <source>Undo</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Rückgängig machen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2690"/>
      <source>Undo last point</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Letzten Punkt rückgängig machen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2697"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)</comment>
      <translation>Eigenschaften</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2699"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)</comment>
      <translation>eigenschaften...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2700"/>
      <source>Change the properties of this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Ändere die Eigenschaften dieser Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2703"/>
      <source>Finish</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Fertigstellen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2704"/>
      <source>Finish drawing this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Zeichnen der Linie beenden</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2771"/>
      <source>Create new map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Neue Karte erstellen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2774"/>
      <source>Load map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Karte laden</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2788"/>
      <source>Create room</source>
      <comment>Menu option to create a new room in the mapper</comment>
      <translation>Raum erstellen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2795"/>
      <source>Move</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Verschieben</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2796"/>
      <source>Move room</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Raum verschieben</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2799"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Löschen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2800"/>
      <source>Delete room</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Raum löschen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2803"/>
      <source>Color</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Farbe</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2804"/>
      <source>Change room color</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Ändern der Raumfarbe</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2807"/>
      <source>Spread</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ausbreiten</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2808"/>
      <source>Increase map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Erhöhe den X-Y Kartenabstand der gewählten Gruppe von Räumen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2811"/>
      <source>Shrink</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Schrumpfen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2812"/>
      <source>Decrease map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Verringere den X-Y Kartenabstand der gewählten Gruppe von Räumen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2815"/>
      <source>Lock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Sperren</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2816"/>
      <source>Lock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Sperre den Raum für Schnellreisen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2819"/>
      <source>Unlock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Entsperren</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2820"/>
      <source>Unlock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Entsperre den Raum für Schnellreisen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2823"/>
      <source>Weight</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Gewicht</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2824"/>
      <source>Set room weight</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Setze Raumgewicht</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2827"/>
      <source>Exits</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ausgänge</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2828"/>
      <source>Set room exits</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Setze Raumausgänge</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2831"/>
      <source>Symbol</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Symbol</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2832"/>
      <source>Set one or more symbols or letters to mark special rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Markiere spezielle Räume mit einem oder mehreren Symbolen oder Buchstaben</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2835"/>
      <source>Move to</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Verschieben nach</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2836"/>
      <source>Move selected group to a given position</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Verschiebe die gewählte Gruppe an eine bestimmte Position</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2839"/>
      <source>Area</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Gebiet</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2840"/>
      <source>Set room&apos;s area number</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Gebietsnummer des Raumes festlegen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2843"/>
      <source>Custom exit line</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Benutzerdefinierte Ausgangslinie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2845"/>
      <source>Replace an exit line with a custom line</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Ersetze eine Ausgangslinie mit einer benutzerdefinierten Linie</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2849"/>
      <source>Custom exit lines are not shown and are not editable in grid mode</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation>Benutzerdefinierte Ausgangslinien werden im Rastermodus nicht angezeigt und sind in diesem nicht bearbeitbar</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2853"/>
      <source>Create Label</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Label erstellen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2854"/>
      <source>Create labels to show text or images</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Erstelle Marken und Text oder Bilder anzuzeigen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2873"/>
      <source>Set location</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Standort festlegen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2875"/>
      <source>Set player current location to here</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Setze die aktuelle Spielerposition hierher</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2879"/>
      <source>Can only set location when exactly one room is selected</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation>Kann die Position nur setzen, wenn genau ein Raum gewählt ist</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2886"/>
      <source>Switch to editing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Bearbeiten starten</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2887"/>
      <source>Switch to viewing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Bearbeiten beenden</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2897"/>
      <source>Move</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Verschieben</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2898"/>
      <source>Move label</source>
      <comment>2D Mapper context menu item (label) tooltip</comment>
      <translation>Verschiebe Marke</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2900"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Löschen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2901"/>
      <source>Delete label</source>
      <comment>2D Mapper context menu (label) item tooltip</comment>
      <translation>Lösche Marke</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2917"/>
      <source>Add point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Punkt hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2924"/>
      <source>Divide segment by adding a new point mid-way along</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state)</comment>
      <translation>Teile das Segment, indem ein neuer Punkt in der Mitte ergänzt wird</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2927"/>
      <source>Select a point first, then add a new point mid-way along the segment towards room</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)</comment>
      <translation>Wähle zuerst einen Punkt, dann ergänze einen neuen Punkt in der Mitte des Segments zum Raum</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2931"/>
      <source>Remove point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Punkt entfernen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2938"/>
      <source>Merge pair of segments by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)</comment>
      <translation>Vereinige zwei Segmente, indem dieser Punkt entfernt wird</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2942"/>
      <source>Remove last segment by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)</comment>
      <translation>Entferne das letzte Segment, indem dieser Punkt entfernt wird</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2947"/>
      <source>use &quot;delete line&quot; to remove the only segment ending in an editable point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action &quot;delete line&quot; should match the translation for that action)</comment>
      <translation>Benutze &quot;Linie löschen&quot;, um das einzige Segment zu entfernen, das an einem bearbeitbaren Punkt endet</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2953"/>
      <source>Select a point first, then remove it</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)</comment>
      <translation>Wähle zuerst einen Punkt, dann entferne ihn</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2957"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)</comment>
      <translation>Eigenschaften</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2961"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise</comment>
      <translation>eigenschaften...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2965"/>
      <source>Delete line</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Linie löschen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2966"/>
      <source>Delete all of this custom line</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip</comment>
      <translation>Lösche diese benutzerdefinierte Linie ganz</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3478"/>
      <source>Move the selection, centered on
the highlighted room (%1) to:</source>
      <comment>Use linefeeds as necessary to format text into a reasonable rectangle of text, %1 is a room number</comment>
      <translation>Die Auswahl, zentriert auf
den markierten Raum (%1), verschieben nach:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3502"/>
      <source>OK</source>
      <comment>dialog (room(s) move) button</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3507"/>
      <source>Cancel</source>
      <comment>dialog (room(s) move) button</comment>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3701"/>
      <source>OK</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3706"/>
      <source>Cancel</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3763"/>
      <source>Spread out rooms</source>
      <translation>Räume ausbreiten</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3764"/>
      <source>Increase the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>Erhöhe den Abstand der
gewählten Räume mittig 
um den markierten Raum
herum um ein Faktor von:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3823"/>
      <source>Shrink in rooms</source>
      <translation>Räume zusammenziehen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3824"/>
      <source>Decrease the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>Verringere den Abstand der 
gewählten Räume mittig 
um den markierten Raum 
herum um einen Faktor von:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3952"/>
      <location filename="../src/T2DMap.cpp" line="3966"/>
      <location filename="../src/T2DMap.cpp" line="4016"/>
      <source>Enter room weight</source>
      <translation>Raumgewicht eingeben</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3953"/>
      <source>Enter new roomweight
(= travel time), minimum
(and default) is 1:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Neues Raumgewicht
(= Reisezeit) eingeben,
mindestens 1 (Standard):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3967"/>
      <source>Enter new roomweight
(= travel time) for all
selected rooms, minimum
(and default) is 1 and
the only current value
used is:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Neues Raumgewicht
(= Reisezeit) eingeben
für alle gewählten Räume,
mindestens 1 (Standard)
und der einzige bisher
benutzte Wert ist:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4005"/>
      <source>%1 {count:%2, default}</source>
      <translation>%1 {Anzahl:%2, Standard}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4013"/>
      <source>1 {count 0, default}</source>
      <translation>1 {Anzahl 0, Standard}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4017"/>
      <source>Choose an existing
roomweight (= travel
time) from the list
(sorted by most commonly
used first) or enter a
new (positive) integer
value for all selected
rooms:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Vorhandenes Raumgewicht
(= Reisezeit) wählen für 
alle gewählten Räume
aus der Liste (sortiert
nach Häufigkeit) oder
eine neue (positive) 
Ganzzahl eingeben:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4066"/>
      <source>Load Mudlet map</source>
      <translation>Mudlet Karte laden</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4068"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) or the ;;s as they are used programmatically</comment>
      <translation>Mudlet Karte (*.dat);;Xml Kartendaten (*.xml);;Beliebige Datei (*)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4955"/>
      <location filename="../src/T2DMap.cpp" line="4989"/>
      <source>Left-click to add point, right-click to undo/change/finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Links klicken um einen Punkt hinzuzufügen, rechts klicken zum rückgängig machen/ändern/beenden...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5000"/>
      <source>Left-click and drag a square for the size and position of your label</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Links klicken und ein Rechteck aufziehen für die Größe und Position der Marke</translation>
    </message>
  </context>
  <context>
    <name>TAlias</name>
    <message>
      <location filename="../src/TAlias.cpp" line="132"/>
      <location filename="../src/TAlias.cpp" line="200"/>
      <source>[Alias Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation>[Alias-Fehler:] %1 Limit für erfasste Gruppen überschritten. Erfasse weniger Gruppen.
</translation>
    </message>
    <message>
      <location filename="../src/TAlias.cpp" line="269"/>
      <source>Error: in &quot;Pattern:&quot;, faulty regular expression, reason: &quot;%1&quot;.</source>
      <translation>Fehler: Fehlerhafter regulärer Ausdruck (Regex) bei &quot;Muster:&quot;, Grund: &quot;%1&quot;.</translation>
    </message>
  </context>
  <context>
    <name>TArea</name>
    <message>
      <location filename="../src/TArea.cpp" line="376"/>
      <source>roomID=%1 does not exist, can not set properties of a non-existent room!</source>
      <translation>Raum-ID=%1 ist nicht vorhanden, kann nicht die Eigenschaften von einem nicht existierenden Raum ändern!</translation>
    </message>
    <message>
      <location filename="../src/TArea.cpp" line="766"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>ohne Text</translation>
    </message>
  </context>
  <context>
    <name>TCommandLine</name>
    <message>
      <location filename="../src/TCommandLine.cpp" line="668"/>
      <source>Add to user dictionary</source>
      <translation>Dem Benutzerwörterbuch hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="670"/>
      <source>Remove from user dictionary</source>
      <translation>Aus Benutzerwörterbuch entfernen</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="674"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which we have bundled with Mudlet; the entries about this line are the ones that the user has personally added.</comment>
      <translation>▼ Mudlet ▼ │ Wörterbuchvorschläge │ ▲ Benutzer ▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="684"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which is provided as part of the OS; the entries about this line are the ones that the user has personally added.</comment>
      <translation>▼ System ▼ │ Wörterbuchvorschläge │ ▲ Benutzer ▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="752"/>
      <source>no suggestions (system)</source>
      <comment>used when the command spelling checker using the selected system dictionary has no words to suggest</comment>
      <translation>Keine Vorschläge (System)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="779"/>
      <source>no suggestions (shared)</source>
      <comment>used when the command spelling checker using the dictionary shared between profile has no words to suggest</comment>
      <translation>keine Vorschläge (gemeinsam)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="783"/>
      <source>no suggestions (profile)</source>
      <comment>used when the command spelling checker using the profile&apos;s own dictionary has no words to suggest</comment>
      <translation>keine Vorschläge (Profil)</translation>
    </message>
  </context>
  <context>
    <name>TConsole</name>
    <message>
      <location filename="../src/TConsole.cpp" line="113"/>
      <source>Debug Console</source>
      <translation>Debug-Konsole</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="787"/>
      <source>Save profile?</source>
      <translation>Profil speichern?</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="787"/>
      <source>Do you want to save the profile %1?</source>
      <translation>Möchtest du das Profil %1 speichern?</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="800"/>
      <source>Couldn&apos;t save profile</source>
      <translation>Konnte Profil nicht speichern</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="800"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation>Leider konnte das Profil nicht gespeichert werden - folgende Fehlermeldung erhalten: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1713"/>
      <source>System Message: %1</source>
      <translation>Systemnachricht: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="356"/>
      <source>Show Time Stamps.</source>
      <translation>Zeiten anzeigen.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="368"/>
      <source>Record a replay.</source>
      <translation>Wiederholung aufnehmen.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="379"/>
      <source>Start logging game output to log file.</source>
      <translation>Protokollierung der Ausgabe des Spiels als Log-Datei beginnen.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="390"/>
      <source>&lt;i&gt;N:&lt;/i&gt; is the latency of the game server and network (aka ping, in seconds), &lt;br&gt;&lt;i&gt;S:&lt;/i&gt; is the system processing time - how long your triggers took to process the last line(s).</source>
      <translation>&lt;i&gt;N:&lt;/i&gt; ist die Latenz von Spielserver und Netzwerk (aka Ping in Sekunden), &lt;br&gt;&lt;i&gt;S:&lt;/i&gt; ist die System-Verarbeitungszeit - wie lange die Trigger zum verarbeiten der letzten Zeile(n) brauchten.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="423"/>
      <source>Emergency Stop. Stops all timers and triggers.</source>
      <translation>Not-Aus. Stoppt alle Timer und Trigger.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="439"/>
      <source>Search buffer.</source>
      <translation>Suchpuffer.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="447"/>
      <source>Earlier search result.</source>
      <translation>Früheres Suchergebnis.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="458"/>
      <source>Later search result.</source>
      <translation>Späteres Suchergebnis.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="884"/>
      <source>Replay recording has started. File: %1</source>
      <translation>Die Aufzeichnung der Wiederholung wurde gestartet. Datei: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="887"/>
      <source>Replay recording has been stopped. File: %1</source>
      <translation>Die Aufzeichnung der Wiederholung wurde beendet. Datei: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1866"/>
      <location filename="../src/TConsole.cpp" line="1905"/>
      <source>No search results, sorry!</source>
      <translation>Leider keine Suchergebnisse!</translation>
    </message>
  </context>
  <context>
    <name>TEasyButtonBar</name>
    <message>
      <location filename="../src/TEasyButtonBar.cpp" line="70"/>
      <source>Easybutton Bar - %1 - %2</source>
      <translation>Easybutton Bar - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TLuaInterpreter</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="9979"/>
      <source>Playing %1</source>
      <translation>Spielt %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12398"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12419"/>
      <source>ERROR</source>
      <translation>FEHLER</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12399"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12410"/>
      <source>object</source>
      <comment>object is the Mudlet alias/trigger/script, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation>Objekt</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12399"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12410"/>
      <source>function</source>
      <comment>function is the Lua function, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation>Funktion</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13888"/>
      <source>Some functions may not be available.</source>
      <translation>Einige Funktionen sind möglicherweise nicht verfügbar.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13308"/>
      <source>No error message available from Lua</source>
      <translation>Keine Fehlermeldung von Lua verfügbar</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13310"/>
      <source>Lua error: %1</source>
      <translation>Lua-Fehler: %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13312"/>
      <source>[ ERROR ] - Cannot find Lua module %1.%2%3%4</source>
      <comment>%1 is the name of the module;%2 will be a line-feed inserted to put the next argument on a new line;%3 is the error message from the lua sub-system;%4 can be an additional message about the expected effect (but may be blank).</comment>
      <translation>[FEHLER] - Konnte Lua-Modul %1 nicht finden.%2%3%4</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13326"/>
      <source>[  OK  ]  - Lua module %1 loaded.</source>
      <comment>%1 is the name (may specify which variant) of the module.</comment>
      <translation>[  OK  ]  - Lua-Modul %1 geladen.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13875"/>
      <source>Probably will not be able to access Mudlet Lua code.</source>
      <translation>Wahrscheinlich wird es nicht möglich sein, auf Mudlet Lua-Code zuzugreifen.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13893"/>
      <source>Database support will not be available.</source>
      <translation>Datenbankunterstützung wird nicht verfügbar sein.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13900"/>
      <source>utf8.* Lua functions won&apos;t be available.</source>
      <translation>utf8.* Lua Funktionen werden nicht verfügbar sein.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13906"/>
      <source>yajl.* Lua functions won&apos;t be available.</source>
      <translation>yajl.* Lua Funktionen werden nicht verfügbar sein.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14087"/>
      <source>No error message available from Lua.</source>
      <translation>Keine Fehlermeldung von Lua verfügbar.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14089"/>
      <source>Lua error: %1.</source>
      <translation>Lua-Fehler: %1.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14091"/>
      <source>[ ERROR ] - Cannot load code formatter, indenting functionality won&apos;t be available.
</source>
      <translation>[FEHLER] - Code-Formatierer kann nicht geladen werden.
Die Einrückungungsfunktion wird nicht verfügbar sein.
</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14168"/>
      <source>%1 (doesn&apos;t exist)</source>
      <comment>This file doesn&apos;t exist</comment>
      <translation>%1 (nicht vorhanden)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14173"/>
      <source>%1 (isn&apos;t a file or symlink to a file)</source>
      <translation>%1 (ist keine Datei oder ein Symlink auf eine Datei)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14182"/>
      <source>%1 (isn&apos;t a readable file or symlink to a readable file)</source>
      <translation>%1 (ist keine lesbare Datei oder ein Symlink auf eine lesbare Datei)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14200"/>
      <source>%1 (couldn&apos;t read file)</source>
      <comment>This file could not be read for some reason (for example, no permission)</comment>
      <translation>%1 (konnte Datei nicht lesen)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14206"/>
      <source>[  OK  ]  - Mudlet-lua API &amp; Geyser Layout manager loaded.</source>
      <translation>[  OK  ]  - Mudlet-Lua-API &amp; Geyser Layoutmanager geladen.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14213"/>
      <source>[ ERROR ] - Couldn&apos;t find, load and successfully run LuaGlobal.lua - your Mudlet is broken!
Tried these locations:
%1</source>
      <translation>[FEHLER] - Konnte LuaGlobal.lua nicht finden, laden und erfolgreich ausführen - dein Mudlet ist kaputt! 
Diese Orte wurden ausprobiert:
%1</translation>
    </message>
  </context>
  <context>
    <name>TMainConsole</name>
    <message>
      <location filename="../src/TMainConsole.cpp" line="165"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).</comment>
      <translation>Protokolldatei</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="206"/>
      <source>Logging has started. Log file is %1
</source>
      <translation>Protokoll gestartet. Datei ist %1
</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="216"/>
      <source>Logging has been stopped. Log file is %1
</source>
      <translation>Protokoll beendet. Datei ist %1
</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="246"/>
      <source>Mudlet MUD Client version: %1%2</source>
      <translation>Mudlet MUD-Client Version: %1%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="248"/>
      <source>Mudlet, log from %1 profile</source>
      <translation>Mudlet, Log aus Profil %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="284"/>
      <location filename="../src/TMainConsole.cpp" line="306"/>
      <source>&apos;Log session starting at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation>&apos;Log Sitzung startet um &apos;hh:mm:ss&apos; am &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="311"/>
      <source>&lt;p&gt;Stop logging game output to log file.&lt;/p&gt;</source>
      <translation>&lt;bp&gt;Protokollierung der Ausgabe des Spiels als Log-Datei beenden.&lt;b/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="315"/>
      <source>&apos;Log session ending at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation>&apos;Protokoll der Sitzung endet um &apos;hh:mm:ss&apos; am &apos;dddd&apos;, &apos;d&apos;. &apos;MMMM&apos; &apos;yyyy&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="328"/>
      <source>&lt;p&gt;Start logging game output to log file.&lt;/p&gt;</source>
      <translation>&lt;bp&gt;Protokollierung der Ausgabe des Spiels als Log-Datei beginnen.&lt;b/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="642"/>
      <source>Pre-Map loading(2) report</source>
      <translation>Bericht bevor die Karte geladen wird(2)</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="652"/>
      <source>Loading map(2) at %1 report</source>
      <translation>Bericht der geladenen Karte(2) bei %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1052"/>
      <source>User window - %1 - %2</source>
      <translation>Benutzerfenster - %1 - %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1100"/>
      <source>N:%1 S:%2</source>
      <comment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</comment>
      <translation>N:%1 S:%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1107"/>
      <source>&lt;no GA&gt; S:%1</source>
      <comment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</comment>
      <translation>&lt;no GA&gt; S:%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1204"/>
      <source>Pre-Map loading(1) report</source>
      <translation>Bericht bevor die Karte lädt(1)</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1222"/>
      <source>Loading map(1) at %1 report</source>
      <translation>Bericht der geladenen Karte(1) bei %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1224"/>
      <source>Loading map(1) &quot;%1&quot; at %2 report</source>
      <translation>Lade Karte(1) &quot;%1&quot; bei %2 Bericht</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1268"/>
      <source>Pre-Map importing(1) report</source>
      <translation>Bericht bevor die Karte importiert wird(1)</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1291"/>
      <source>[ ERROR ]  - Map file not found, path and name used was:
%1.</source>
      <translation>[FEHLER] - Kartendatei nicht gefunden, verwendeter Pfad und Name waren:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1297"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; was not found).</source>
      <translation>loadMap: Falscher Wert für Argument #1 (benutzter Dateiname: 
&quot;%1&quot; wurde nicht gefunden).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1306"/>
      <source>[ INFO ]  - Map file located and opened, now parsing it...</source>
      <translation>[ INFO ]  - Kartendatei gefunden und geöffnet, wird jetzt verarbeitet...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1313"/>
      <source>Importing map(1) &quot;%1&quot; at %2 report</source>
      <translation>Importiere Karte(1) &quot;%1&quot; bei %2 Bericht</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1316"/>
      <source>[ INFO ]  - Map file located but it could not opened, please check permissions on:&quot;%1&quot;.</source>
      <translation>[ INFO ]  - Kartendatei gefunden, aber sie konnte nicht geöffnet werden. Prüfe bitte die Berechtigungen bei:&quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1319"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; could not be opened for reading).</source>
      <translation>loadMap: Falscher Wert für Argument #1 (benutzter Dateiname: 
&quot;%1&quot; konnte nicht gelesen werden).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1343"/>
      <source>[ INFO ]  - Map reload request received from system...</source>
      <translation>[ INFO ]  - Anforderung vom System empfangen, die Karte neu zu laden...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1348"/>
      <source>[  OK  ]  - ... System Map reload request completed.</source>
      <translation>[ OK ] - ... System-Anforderung, die Karte neu zu laden, wurde abgeschlossen.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1350"/>
      <source>[ WARN ]  - ... System Map reload request failed.</source>
      <translation>[ WARNUNG ] - ... System-Anforderung, die Karte neu zu laden, fehlgeschlagen.</translation>
    </message>
  </context>
  <context>
    <name>TMap</name>
    <message>
      <location filename="../src/TMap.cpp" line="210"/>
      <source>RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!</source>
      <translation>Raum-ID=%1 ist nicht vorhanden, kann nicht die Gebiets-ID=%2 bei einem nicht existierenden Raum setzen!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="221"/>
      <source>AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!</source>
      <translation>Gebiets-ID=%2 ist nicht vorhanden, kann nicht die Raum-ID=%1 auf ein nicht existierendes Gebiet setzen!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="469"/>
      <source>[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.</source>
      <translation>[ INFO ]  - UMWANDLUNG: herkömmliche Markierung, Gebiets-ID:%1 Markierungs-ID:%2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="472"/>
      <source>[ INFO ] - Converting old style label id: %1.</source>
      <translation>[ INFO ]  - Wandle herkömmliche Markierung ID: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="477"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.</source>
      <translation>[ WARNUNG ] - UMWANDLUNG: Kann herkömmliche Markierung nicht umwandeln im Gebiet mit ID: %1, Markierung-ID ist: %2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="480"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label with id: %1.</source>
      <translation>[ WARNUNG ] - UMWANDLUNG: Kann herkömmliche Markierung mit ID: %1 nicht umwandeln.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="509"/>
      <source>[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...</source>
      <translation>[  OK  ]  - Kartenprüfung abgeschlossen (%1s). Genieß dein Spiel...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="516"/>
      <source>[  OK  ]  - Map loaded successfully (%1s).</source>
      <translation>[  OK  ]  - Karte erfolgreich geladen (%1s).</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="987"/>
      <source>n</source>
      <comment>This translation converts the direction that DIR_NORTH codes for to a direction string that the game server will accept!</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="988"/>
      <source>ne</source>
      <comment>This translation converts the direction that DIR_NORTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation>no</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="989"/>
      <source>e</source>
      <comment>This translation converts the direction that DIR_EAST codes for to a direction string that the game server will accept!</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="990"/>
      <source>se</source>
      <comment>This translation converts the direction that DIR_SOUTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation>so</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="991"/>
      <source>s</source>
      <comment>This translation converts the direction that DIR_SOUTH codes for to a direction string that the game server will accept!</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="992"/>
      <source>sw</source>
      <comment>This translation converts the direction that DIR_SOUTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation>sw</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="993"/>
      <source>w</source>
      <comment>This translation converts the direction that DIR_WEST codes for to a direction string that the game server will accept!</comment>
      <translation>w</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="994"/>
      <source>nw</source>
      <comment>This translation converts the direction that DIR_NORTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation>nw</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="995"/>
      <source>up</source>
      <comment>This translation converts the direction that DIR_UP codes for to a direction string that the game server will accept!</comment>
      <translation>oben</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="996"/>
      <source>down</source>
      <comment>This translation converts the direction that DIR_DOWN codes for to a direction string that the game server will accept!</comment>
      <translation>unten</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="997"/>
      <source>in</source>
      <comment>This translation converts the direction that DIR_IN codes for to a direction string that the game server will accept!</comment>
      <translation>rein</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="998"/>
      <source>out</source>
      <comment>This translation converts the direction that DIR_OUT codes for to a direction string that the game server will accept!</comment>
      <translation>raus</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1037"/>
      <source>[ ALERT ] - Saving map in a format {%1} that is different than the one it was
loaded as {%2}. This may be an issue if you want to share the resulting
map with others relying on the original format.</source>
      <translation>[ ACHTUNG ] - Speichere Karte in einem anderem Format {%1} als dem, in dem sie geladen wurde {%2}. Dies kann ein Problem sein, wenn sie mit Anderen geteilt werden soll, die das ursprüngliche Format benötigen.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="187"/>
      <source>[MAP ERROR:]%1
</source>
      <translation>[KARTENFEHLER:]%1
</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="48"/>
      <source>Default Area</source>
      <translation>Standard-Region</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="49"/>
      <source>Unnamed Area</source>
      <translation>Unbekannte Region</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="439"/>
      <source>[ INFO ]  - Map audit starting...</source>
      <translation>[ INFO ]  - Kartenprüfung startet...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1021"/>
      <source>[ ERROR ] - The format {%1} you are trying to save the map with is too new
for this version of Mudlet. Supported are only formats up to version {%2}.</source>
      <translation>[FEHLER] - Du versuchst, die Karte in einen zu neuen Format {%1} zu speichern.
Diese Version von Mudlet unterstützt nur Formate bis zur Version {%2}.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1047"/>
      <source>[ WARN ]  - Saving map in a format {%1} different from the
recommended format {%2} for this version of Mudlet.</source>
      <translation>[ WARNUNG ]  - Speichere Karte in einem anderen Format {%1} als
dem für diese Version von Mudlet empfohlenem Format {%2}.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1398"/>
      <location filename="../src/TMap.cpp" line="1753"/>
      <source>[ ERROR ] - Unable to open (for reading) map file: &quot;%1&quot;!</source>
      <translation>[FEHLER] - Kartendatei kann nicht (zum Lesen) geöffnet werden: &quot;%1&quot;!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1420"/>
      <source>[ ERROR ] - Map file is too new, its file format (%1) is higher than this version of
Mudlet can handle (%2)!  The file is:
&quot;%3&quot;.</source>
      <translation>[FEHLER] - Kartendatei ist zu neu, das Dateiformat (%1) ist höher als diese Version von Mudlet verarbeiten kann (%2)! Die Datei ist:
&quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1427"/>
      <source>[ INFO ]  - You will need to upgrade your Mudlet or find a map file saved in an
older format.</source>
      <translation>[INFO]  - Bitte Mudlet aktualisieren oder eine in einem älteren Format gespeicherte Kartendatei finden.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1434"/>
      <source>[ ALERT ] - Map file is really old, its file format (%1) is so ancient that
this version of Mudlet may not gain enough information from
it but it will try!  The file is: &quot;%2&quot;.</source>
      <translation>[ ACHTUNG ] - Kartendatei ist wirklich alt, ihr Dateiformat (%1) ist so alt, dass
diese Version von Mudlet vielleicht nicht genügend Informationen daraus sammeln kann,
aber es wird es versuchen! Die Datei ist: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1441"/>
      <source>[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!
There is so much data that it DOES NOT have that you could be
better off starting again...</source>
      <translation>[INFO]  - Vielleicht sollte man DIESE Kartendatei dem Mudlet Museum spenden! Es gibt so viele Daten, die sie NICHT BESITZT, dass man vielleicht besser wieder von vorne beginnen könnte...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1450"/>
      <source>[ INFO ]  - Reading map (format version:%1) file:
&quot;%2&quot;,
please wait...</source>
      <translation>[ INFO ]  - Lese Karte (Formatversion: %1) aus Datei: 
&quot;%2&quot;,
bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1451"/>
      <source>[ INFO ]  - Reading map (format version:%1) file: &quot;%2&quot;.</source>
      <translation>[ INFO ]  - Lese Karte (Formatversion: %1) aus Datei: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1600"/>
      <source>[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an
area) not found, adding reserved -1 id.</source>
      <translation>[INFO]  - Standardgebiet (Reset) (für Räume, die nicht zu einem Gebiet zugewiesen wurden) wurde nicht gefunden, reservierte ID -1 ergänzt.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1694"/>
      <source>[ INFO ]  - Successfully read the map file (%1s), checking some
consistency details...</source>
      <translation>[ INFO ]  - Kartendatei (%1s) erfolgreich gelesen, prüfe einige Details der Konsistenz...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1709"/>
      <source>No map found. Would you like to download the map or start your own?</source>
      <translation>Keine Karte gefunden. Karte herunterladen oder eine eigene beginnen?</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1710"/>
      <source>Download the map</source>
      <translation>Karte herunterladen</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1711"/>
      <source>Start my own</source>
      <translation>Eigene beginnen</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1769"/>
      <source>[ INFO ]  - Checking map file: &quot;%1&quot;, format version:%2...</source>
      <translation>[ INFO ]  - Prüfe Kartendatei: &quot;%1&quot;, Formatversion: %2...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2157"/>
      <source>Map issues</source>
      <translation>Aspekte zur Karte</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2164"/>
      <source>Area issues</source>
      <translation>Aspekte zum Gebiet</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2170"/>
      <source>Area id: %1 &quot;%2&quot;</source>
      <translation>Gebiets-ID: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2172"/>
      <source>Area id: %1</source>
      <translation>Gebiets-ID: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2181"/>
      <source>Room issues</source>
      <translation>Aspekte zum Raum</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2188"/>
      <source>Room id: %1 &quot;%2&quot;</source>
      <translation>Raum-ID: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2190"/>
      <source>Room id: %1</source>
      <translation>Raum-ID: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2200"/>
      <source>End of report</source>
      <translation>Ende des Berichts</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2206"/>
      <source>[ ALERT ] - At least one thing was detected during that last map operation
that it is recommended that you review the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[ ACHTUNG ] - Mindestens eine Sache wurde während der letzten Kartenbearbeitung erkannt.
Es wird empfohlen, den jüngsten Bericht in
folgender Datei zu überprüfen:
&quot;%1&quot;
- dabei den (letzten) Bericht mit folgendem Titel suchen:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2214"/>
      <source>[ INFO ]  - The equivalent to the above information about that last map
operation has been saved for review as the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[ INFO ]  - Äquivalente Informationen wie oben zur letzten Kartenbearbeitung wurden zur Überprüfung als jüngster Bericht in folgender Datei gespeichert:
&quot;%1&quot;
- dabei den (letzten) Bericht mit folgendem Titel suchen:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2270"/>
      <source>[ ERROR ] - Unable to use or create directory to store map.
Please check that you have permissions/access to:
&quot;%1&quot;
and there is enough space. The download operation has failed.</source>
      <translation>[FEHLER] - Kann das Verzeichnis nicht nutzen oder erstellen, um die Karte zu speichern.
Prüfe bitte, dass du die Berechtigungen und Zugriff hast auf:
&quot;%1&quot;
und dass es genug Speicherplatz gibt. Der Download ist fehlgeschlagen.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2296"/>
      <source>[ INFO ]  - Map download initiated, please wait...</source>
      <translation>[INFO]  - Karte wird heruntergeladen, bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2307"/>
      <source>Map download</source>
      <comment>This is a title of a progress window.</comment>
      <translation>Karte herunterladen</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2370"/>
      <source>Map import</source>
      <comment>This is a title of a progress dialog.</comment>
      <translation>Karte importieren</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2670"/>
      <location filename="../src/TMap.cpp" line="3141"/>
      <source>Exporting JSON map data from %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>Exportiere JSON Kartendaten von %1
Gebiet %2 von %3   Raum %4 von %5   Markierung %6 von %7...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2685"/>
      <source>Map JSON export</source>
      <comment>This is a title of a progress window.</comment>
      <translation>JSON Kartenexport</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2820"/>
      <source>Exporting JSON map file from %1 - writing data to file:
%2 ...</source>
      <translation>Exportiere JSON Kartendaten von %1 - Schreibe Daten in Datei:
%2 ...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2907"/>
      <source>Map JSON import</source>
      <comment>This is a title of a progress window.</comment>
      <translation>JSON Kartenimport</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2892"/>
      <location filename="../src/TMap.cpp" line="3151"/>
      <source>Importing JSON map data to %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>Importiere JSON Kartendaten nach %1
Gebiet %2 von %3   Raum %4 von %5   Markierung %6 von %7...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2306"/>
      <location filename="../src/TMap.cpp" line="2679"/>
      <location filename="../src/TMap.cpp" line="2901"/>
      <source>Abort</source>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2304"/>
      <source>Downloading map file for use in %1...</source>
      <comment>%1 is the name of the current Mudlet profile</comment>
      <translation>Lade Kartendatei für den Gebrauch in %1...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2338"/>
      <source>loadMap: unable to perform request, a map is already being downloaded or
imported at user request.</source>
      <translation>loadMap: Anfrage kann nicht durchgeführt werden. Es wird bereits eine Karte heruntergeladen oder importiert auf Benutzerwunsch.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2369"/>
      <source>Importing XML map file for use in %1...</source>
      <translation>Importiere XML-Kartendatei für den Gebrauch in %1...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2397"/>
      <source>loadMap: failure to import XML map file, further information may be available
in main console!</source>
      <translation>loadMap: Fehler beim Import der XML-Kartendatei. Weitere Informationen sind möglicherweise in der Hauptkonsole verfügbar!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2432"/>
      <source>[ ALERT ] - Map download was canceled, on user&apos;s request.</source>
      <translation>[ ACHTUNG ] - Herunterladen der Karte wurde auf Benutzeranforderung abgebrochen.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2451"/>
      <source>[ ERROR ] - Map download encountered an error:
%1.</source>
      <translation>[FEHLER] - Herunterladen der Karte ist auf einen Fehler gestoßen: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2482"/>
      <source>[ ALERT ] - Map download failed, error reported was:
%1.</source>
      <translation>[ ACHTUNG ] - Herunterladen der Karte fehlgeschlagen. Fehlermeldung war: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2490"/>
      <source>[ ALERT ] - Map download failed, unable to open destination file:
%1.</source>
      <translation>[ ACHTUNG ] - Herunterladen der Karte fehlgeschlagen. Konnte Zieldatei nicht öffnen: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2495"/>
      <source>[ ALERT ] - Map download failed, unable to write destination file:
%1.</source>
      <translation>[ ACHTUNG ] - Herunterladen der Karte fehlgeschlagen. Konnte Zieldatei nicht schreiben: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2508"/>
      <location filename="../src/TMap.cpp" line="2525"/>
      <source>[ INFO ]  - ... map downloaded and stored, now parsing it...</source>
      <translation>[ INFO ]  - ... Karte heruntergeladen und gespeichert, wird jetzt verarbeitet...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2516"/>
      <location filename="../src/TMap.cpp" line="2551"/>
      <source>[ ERROR ] - Map download problem, failure in parsing destination file:
%1.</source>
      <translation>[FEHLER] - Problem beim Herunterladen der Karte. Fehler beim Verarbeiten der Zieldatei: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2556"/>
      <source>[ ERROR ] - Map download problem, unable to read destination file:
%1.</source>
      <translation>[FEHLER] - Problem beim Herunterladen der Karte. Konnte Zieldatei nicht lesen: %1.</translation>
    </message>
  </context>
  <context>
    <name>TRoom</name>
    <message>
      <location filename="../src/TRoom.cpp" line="86"/>
      <location filename="../src/TRoom.cpp" line="971"/>
      <source>North</source>
      <translation>Norden</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="87"/>
      <source>North-east</source>
      <translation>Nordosten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="88"/>
      <source>North-west</source>
      <translation>Nordwesten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="89"/>
      <location filename="../src/TRoom.cpp" line="1013"/>
      <source>South</source>
      <translation>Süden</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="90"/>
      <source>South-east</source>
      <translation>Südosten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="91"/>
      <source>South-west</source>
      <translation>Südwesten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="92"/>
      <location filename="../src/TRoom.cpp" line="1055"/>
      <source>East</source>
      <translation>Osten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="93"/>
      <location filename="../src/TRoom.cpp" line="1069"/>
      <source>West</source>
      <translation>Westen</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="94"/>
      <location filename="../src/TRoom.cpp" line="1083"/>
      <source>Up</source>
      <translation>Oben</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="95"/>
      <location filename="../src/TRoom.cpp" line="1097"/>
      <source>Down</source>
      <translation>Unten</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="96"/>
      <location filename="../src/TRoom.cpp" line="1111"/>
      <source>In</source>
      <translation>Rein</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="97"/>
      <location filename="../src/TRoom.cpp" line="1125"/>
      <source>Out</source>
      <translation>Raus</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="98"/>
      <source>Other</source>
      <translation>Anderes</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="100"/>
      <source>Unknown</source>
      <translation>Unbekannt</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="311"/>
      <source>No area created!  Requested area ID=%1. Note: Area IDs must be &gt; 0</source>
      <translation>Kein Gebiet erstellt! Angeforderte Gebiets-ID=%1. Hinweis: Gebiets-IDs müssen &gt; 0 sein</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="333"/>
      <source>Warning: When setting the Area for Room (Id: %1) it did not have a current area!</source>
      <translation>Warnung: Beim Einstellen des Gebiets für Raum (ID: %1) gab es kein aktuelles Gebiet!</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="985"/>
      <source>Northeast</source>
      <translation>Nordost</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="999"/>
      <source>Northwest</source>
      <translation>Nordwest</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1027"/>
      <source>Southeast</source>
      <translation>Südost</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1041"/>
      <source>Southwest</source>
      <translation>Südwest</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1146"/>
      <source>[ WARN ]  - In room id:%1 removing invalid (special) exit to %2 {with no name!}</source>
      <translation>[ WARNUNG ] - Entferne in Raum ID:%1 den ungültigen (besonderen) Ausgang zu %2 {ohne Name!}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1150"/>
      <source>[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.</source>
      <translation>[ WARNUNG ] - Raum hatte einen ungültigen (besonderen) Ausgang zu %1 {ohne Name!} - dieser wurde entfernt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1159"/>
      <source>[ INFO ]  - In room with id: %1 correcting special exit &quot;%2&quot; that
was to room with an exit to invalid room: %3 to now go
to: %4.</source>
      <translation>[ INFO ]  - Korrigiere im Raum mit ID: %1 den besonderen Ausgang &quot;%2&quot;, der zu einem Raum zeigte, mit einem Ausgang zu einem ungültigen Raum: %3, zeigt nun zu %4.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1169"/>
      <source>[ INFO ]  - Room needed correcting of special exit &quot;%1&quot; that was to room with an exit to invalid room: %2 to now go to: %3.</source>
      <translation>[ INFO ]  - Korrigiere im Raum den besonderen Ausgang &quot;%1&quot;, der zu einem Raum zeigte mit einem Ausgang zu einem ungültigen Raum: %2, zeigt nun zu %3.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1194"/>
      <source>[ WARN ]  - Room with id: %1 has a special exit &quot;%2&quot; with an
exit to: %3 but that room does not exist.  The exit will
be removed (but the destination room id will be stored in
the room user data under a key:
&quot;%4&quot;).</source>
      <translation>[ WARNUNG ]  - Der Raum mit der ID: %1 hat einen besonderen Ausgang &quot;%2&quot; in
Richtung: %3 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1207"/>
      <source>[ WARN ]  - Room has a special exit &quot;%1&quot; with an exit to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation>[ WARNUNG ]  - Der Raum hat einen besonderen Ausgang &quot;%1&quot; in
Richtung: %2 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%3&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1246"/>
      <source>[ INFO ]  - In room with id: %1 special exit &quot;%2&quot;
that was to room with an invalid room: %3 that does not exist.
The exit will be removed (the bad destination room id will be stored in the
room user data under a key:
&quot;%4&quot;).</source>
      <translation>[ WARNUNG ]  - Der Raum mit der ID: %1 hat einen besonderen Ausgang &quot;%2&quot; in
Richtung: %3 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1259"/>
      <source>[ INFO ]  - Room had special exit &quot;%1&quot; that was to room with an invalid room: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation>[ WARNUNG ]  - Der Raum hat einen besonderen Ausgang &quot;%1&quot; in
Richtung: %2 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%3&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1292"/>
      <source>%1 {none}</source>
      <translation>%1 {ohne}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1295"/>
      <source>%1 (open)</source>
      <translation>%1 (offen)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1298"/>
      <source>%1 (closed)</source>
      <translation>%1 (geschlossen)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1301"/>
      <source>%1 (locked)</source>
      <translation>%1 (versperrt)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1304"/>
      <source>%1 {invalid}</source>
      <translation>%1 {defekt}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1308"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:
%2.</source>
      <translation>[ INFO ] - Im Raum mit der ID: %1 wurden ein oder mehrere überzählige Tür-Elemente gefunden und entfernt:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1315"/>
      <source>[ INFO ]  - Room had one or more surplus door items that were removed:%1.</source>
      <translation>[ INFO ] - Im Raum wurden ein oder mehrere überzählige Tür-Elemente gefunden und entfernt:%1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1331"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:
%2.</source>
      <translation>[ INFO ] - Im Raum mit der ID: %1 wurden ein oder mehrere überzählige Gewicht-Elemente gefunden und entfernt:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1338"/>
      <source>[ INFO ]  - Room had one or more surplus weight items that were removed: %1.</source>
      <translation>[ INFO ] - Im Raum wurden ein oder mehrere überzählige Gewicht-Elemente gefunden und entfernt:%1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1354"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus exit lock items that were removed:
%2.</source>
      <translation>[ INFO ] - Im Raum mit der ID: %1 wurden ein oder mehrere überzählige Sperren gefunden und entfernt:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1361"/>
      <source>[ INFO ]  - Room had one or more surplus exit lock items that were removed: %1.</source>
      <translation>[ INFO ] - Im Raum wurden ein oder mehrere überzählige Sperren gefunden und entfernt:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1440"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that
were removed: %2.</source>
      <translation>[ INFO ] - Im Raum mit der ID: %1 wurden ein oder mehrere überzählige benutzerdefinierte Linienelemente gefunden und entfernt: %2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1446"/>
      <source>[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.</source>
      <translation>[ INFO ] - Im Raum wurden ein oder mehrere überzählige benutzerdefinierte Linienelemente gefunden und entfernt: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1469"/>
      <source>[ INFO ]  - In room with id: %1 correcting exit &quot;%2&quot; that was to room with
an exit to invalid room: %3 to now go to: %4.</source>
      <translation>[ INFO ]  - Korrigiere im Raum mit ID: %1 den Ausgang &quot;%2&quot;, der zu einem Raum zeigte, mit einem Ausgang zu einem ungültigen Raum: %3, zeigt nun zu %4.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1478"/>
      <source>[ INFO ]  - Correcting exit &quot;%1&quot; that was to invalid room id: %2 to now go to: %3.</source>
      <translation>[ INFO ]  - Korrigiere den Ausgang &quot;%1&quot;, der zu einem ungültigen Raum mit ID %2 zeigte, zeigt nun zu %3.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1489"/>
      <source>[ WARN ]  - Room with id: %1 has an exit &quot;%2&quot; to: %3 but that room
does not exist.  The exit will be removed (but the destination room
Id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation>[ WARNUNG ]  - Der Raum mit der ID: %1 hat einen Ausgang &quot;%2&quot; in
Richtung: %3 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;) 
und der Ausgang wird in eine Abzweigung umgewandelt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1502"/>
      <source>[ WARN ]  - Room has an exit &quot;%1&quot; to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation>[ WARNUNG ]  - Der Raum hat einen Ausgang &quot;%1&quot; in
Richtung: %2 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;) 
und der Ausgang wird in eine Abzweigung umgewandelt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1546"/>
      <source>[ ALERT ] - Room with id: %1 has an exit &quot;%2&quot; to: %3 but also
has a stub exit!  As a real exit precludes a stub, the latter will
be removed.</source>
      <translation>[ ACHTUNG ]  - Der Raum mit der ID: %1 hat einen Ausgang &quot;%2&quot; in
Richtung: %3 aber außerdem eine Abzweigung. Ein richtiger Ausgang geht vor,
also wird die Abzweigung entfernt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1556"/>
      <source>[ ALERT ] - Room has an exit &quot;%1&quot; to: %2 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.</source>
      <translation>[ ACHTUNG ]  - Der Raum hat einen Ausgang &quot;%1&quot; in
Richtung: %2 aber außerdem eine Abzweigung in dieselbe Richtung. 
Ein richtiger Ausgang geht vor, also wird die Abzweigung entfernt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1612"/>
      <source>[ INFO ]  - In room with id: %1 exit &quot;%2&quot; that was to room with an invalid
room: %3 that does not exist.  The exit will be removed (the bad destination
room id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation>[ INFO ]  - Der Raum mit der ID: %1 hat einen Ausgang &quot;%2&quot; in
Richtung: %3 aber dieser Raum existiert nicht. Der Ausgang wird
entfernt (aber die ID des Ziel-Raumes wird gespeichert in den 
Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;) 
und der Ausgang wird in eine Abzweigung umgewandelt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1623"/>
      <source>[ INFO ]  - Room exit &quot;%1&quot; that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation>[ INFO ]  - Der Raumausgang &quot;%1&quot; ging zu einem Raum mit einer ungültigen ID %2, die nicht existiert. Der Ausgang wird entfernt (aber die ID des Ziel-Raumes wird gespeichert in den Benutzerdaten des Raums unter dem Schlüssel: &quot;%4&quot;) und der Ausgang wird in eine Abzweigung umgewandelt.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1639"/>
      <source>
It was locked, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation>
Er war versperrt. Dies wird als Benutzerdaten 
gespeichert unter dem Schlüssel: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1643"/>
      <source>  It was locked, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation>  Er war versperrt. Dies wird als Benutzerdaten 
gespeichert unter dem Schlüssel: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1651"/>
      <source>
It had a weight, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation>
Er hatte ein Gewicht. Dies wird als Benutzerdaten 
gespeichert unter dem Schlüssel: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1655"/>
      <source>  It had a weight, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation>  Er hatte ein Gewicht. Dies wird als Benutzerdaten 
gespeichert unter dem Schlüssel: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1666"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but
it has not been possible to salvage this, it has been lost!</source>
      <translation>[ WARNUNG ]  - Mit dem ungültigen Ausgang war eine benutzerdefinierte 
Linie verbunden, die nicht wiederhergestellt werden konnte und verloren ist!</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1671"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but it has not been possible to salvage this, it has been lost!</source>
      <translation>[ WARNUNG ]  - Mit dem ungültigen Ausgang war eine benutzerdefinierte 
Linie verbunden, die nicht wiederhergestellt werden konnte und verloren ist!</translation>
    </message>
  </context>
  <context>
    <name>TRoomDB</name>
    <message>
      <location filename="../src/TRoomDB.cpp" line="504"/>
      <source>Area with ID %1 already exists!</source>
      <translation>Gebiet mit ID %1 existiert bereits!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="523"/>
      <source>An Unnamed Area is (no longer) permitted!</source>
      <translation>Ein unbenanntes Gebiet ist nicht (mehr) zulässig!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="527"/>
      <source>An area called %1 already exists!</source>
      <translation>Ein Gebiet namens %1 existiert bereits!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="626"/>
      <source>[ WARN ]  - Problem with data structure associated with room id: %1 - that
room&apos;s data has been lost so the id is now being deleted.  This
suggests serious problems with the currently running version of
Mudlet - is your system running out of memory?</source>
      <translation>[ WARNUNG ]  - Problematische Daten-Struktur bei Raum-ID: %1 - 
Die Daten dieses Raums sind verloren gegangen, also wird die ID 
nun gelöscht. Vermutlich gibt es ernsthafte Probleme mit der aktuell 
laufenden Mudlet Version - Hat dein System nicht mehr genügend 
Arbeitsspeicher frei?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="634"/>
      <source>[ WARN ]  - Problem with data structure associated with this room.  The room&apos;s data has been lost so the id is now being deleted.  This suggests serious problems with the currently running version of Mudlet - is your system running out of memory?</source>
      <translation>[ WARNUNG ]  - Problematische Daten-Struktur bei diesem Raum. Die Daten dieses Raums sind verloren gegangen, also wird die ID nun gelöscht. Vermutlich gibt es ernsthafte Probleme mit der aktuell laufenden Mudlet Version - Hat dein System nicht mehr genügend Arbeitsspeicher frei?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="692"/>
      <source>[ ALERT ] - Area with id: %1 expected but not found, will be created.</source>
      <translation>[ ACHTUNG ] - Gebiet mit ID: %1 erwartet, aber nicht gefunden, wird erstellt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="695"/>
      <source>[ ALERT ] - Area with this id expected but not found, will be created.</source>
      <translation>[ ACHTUNG ] - Gebiet mit dieser ID erwartet, aber nicht gefunden, wird erstellt.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="724"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that are supposed
 to be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation>
        <numerusform>[ ACHTUNG ] - Fehlendes Gebiet in der Karte entdeckt und ergänzt.
 Achte auf weitere Nachrichten zu Räumen, die in diesem Gebiet
 sein sollten...</numerusform>
        <numerusform>[ ACHTUNG ] - %n fehlende Gebiete in der Karte entdeckt und ergänzt.
 Achte auf weitere Nachrichten zu Räumen, die in diesen Gebieten
 sein sollten...</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="731"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation>
        <numerusform>[ ACHTUNG ] - Fehlendes Gebiet in der Karte entdeckt und ergänzt.
 Achte auf weitere Nachrichten zu Räumen, die in diesem Gebiet
 sein sollten...</numerusform>
        <numerusform>[ ACHTUNG ] - %n fehlende Gebiete in der Karte entdeckt und ergänzt.
 Achte auf weitere Nachrichten zu Räumen, die in diesen Gebieten
 sein sollten...</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="740"/>
      <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation>
        <numerusform>[ INFO ]  - Das fehlende Gebiet heißt nun:
(ID) ==&gt; &quot;Name&quot;</numerusform>
        <numerusform>[ INFO ] - Die fehlenden Gebiete heißen nun:
(ID) ==&gt; &quot;Name&quot;</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="775"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1)
in map, now working out what new id numbers to use...</source>
      <translation>[ ACHTUNG ] - Defekte Gebiets-IDs in der Karte gefunden (Anzahl: %1) 
(Defekte IDs sind kleiner als +1 und auch nicht die reservierte ID -1)
Bestimme nun neue IDs, die statt dessen genutzt werden können...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="780"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1) in map!  Look for further messsages related to this for each affected area ...</source>
      <translation>[ ACHTUNG ] - Defekte Gebiets-IDs in der Karte gefunden (Anzahl: %1) 
(Defekte IDs sind kleiner als +1 und auch nicht die reservierte ID -1)
Achte auf weitere Nachrichten zu jedem betroffenen Gebiet...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="787"/>
      <source>[ INFO ]  - The renumbered area ids will be:
Old ==&gt; New</source>
      <translation>[ INFO ]  - Die neuen Gebiets-IDs lauten:
Alt ==&gt; Neu</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="805"/>
      <source>[ INFO ]  - The area with this bad id was renumbered to: %1.</source>
      <translation>[ INFO ]  - Das Gebiet mit dieser defekten ID trägt nun: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="806"/>
      <source>[ INFO ]  - This area was renumbered from the bad id: %1.</source>
      <translation>[ INFO ]  - Das Gebiet mit der defekten ID: %1 wurde neu nummeriert.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="843"/>
      <location filename="../src/TRoomDB.cpp" line="846"/>
      <source>[ INFO ]  - Area id numbering is satisfactory.</source>
      <translation>[ INFO ]  - Nummerierung des Gebiets ist zufriedenstellend.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="854"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map, now working
out what new id numbers to use.</source>
      <translation>[ ACHTUNG ] - Defekte Raum-IDs in der Karte gefunden (Anzahl: %1) 
(Defekte IDs sind kleiner als +1)
Bestimme neue IDs, die statt dessen genutzt werden können.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="859"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!  Look for further messsages related to this for each affected room ...</source>
      <translation>[ ACHTUNG ] - Defekte Raum-IDs in der Karte gefunden (Anzahl: %1) 
(Defekte IDs sind kleiner als +1)
Achte auf weitere Nachrichten zu jedem betroffenen Raum...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="866"/>
      <source>[ INFO ]  - The renumbered rooms will be:
</source>
      <translation>[ INFO ]  - Die neu nummerierten Räume sind:
</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="882"/>
      <source>[ INFO ]  - This room with the bad id was renumbered to: %1.</source>
      <translation>[ INFO ]  - Dieser Raum mit der defekten ID trägt nun: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="883"/>
      <source>[ INFO ]  - This room was renumbered from the bad id: %1.</source>
      <translation>[ INFO ]  - Dieser Raum mit der defekten ID: %1 wurde neu nummeriert.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="919"/>
      <location filename="../src/TRoomDB.cpp" line="922"/>
      <source>[ INFO ]  - Room id numbering is satisfactory.</source>
      <translation>[ INFO ]  - Nummerierung der Raums ist zufriedenstellend.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="946"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ]  - Doppelte Abzweigungen im Raum ID: %1 gefunden. 
Dies ist eine Anomalie, aber wurde problemlos aufgeräumt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="951"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ]  - Doppelte Abzweigungen im Raum gefunden. 
Dies ist eine Anomalie, aber wurde problemlos aufgeräumt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="968"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ]  - Doppelte Sperren im Raum ID: %1 gefunden. 
Dies ist eine Anomalie, aber wurde problemlos aufgeräumt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="973"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ]  - Doppelte Sperren im Raum gefunden. Dies ist eine Anomalie, aber wurde problemlos aufgeräumt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1060"/>
      <source>[ INFO ]  - This room claims to be in area id: %1, but that did not have a record of it.  The area has been updated to include this room.</source>
      <translation>[ INFO ]  - Dieser Raum behauptet, zu Gebiets-ID: %1 zu gehören, aber dort war er unbekannt. Das Gebiet wird ihn zukünftig berücksichtigen.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1066"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 rooms missing from those it
should be recording as possessing, they were:
%3
they have been added.</source>
      <translation>[ INFO ]  - In Gebiets-ID: %1 fehlten %2 Räume, 
die eigentlich anwesend sein sollten, nämlich:
%3
Sie wurden dem Gebiet hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1074"/>
      <source>[ INFO ]  - In this area there were %1 rooms missing from those it should be recorded as possessing.  They are: %2.  They have been added.</source>
      <translation>[ INFO ]  - In diesem Gebiet fehlten %1 Räume, die eigentlich anwesend sein sollten, nämlich: %2. Sie wurden hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1099"/>
      <source>[ INFO ]  - This room was claimed by area id: %1, but it does not belong there.  The area has been updated to not include this room.</source>
      <translation>[ INFO ]  - Dieser Raum wurde von Gebiets-ID: %1 reklamiert, aber gehört nicht dorthin. Das Gebiet wird ihn zukünftig nicht mehr berücksichtigen.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1105"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 extra rooms compared to those it
should be recording as possessing, they were:
%3
they have been removed.</source>
      <translation>[ INFO ]  - Im Gebiets-ID: %1 gab es %2 Räume 
mehr als eigentlich anwesend sein sollten, nämlich:
%3
Sie wurden aus dem Gebiet entfernt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1113"/>
      <source>[ INFO ]  - In this area there were %1 extra rooms that it should not be recorded as possessing.  They were: %2.  They have been removed.</source>
      <translation>[ INFO ]  - In diesem Gebiet gab es %1 Räume mehr als eigentlich anwesend sein sollten, nämlich: %2. Sie wurden entfernt.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1212"/>
      <source>It has been detected that &quot;_###&quot; form suffixes have already been used, for simplicity in the renaming algorithm these will have been removed and possibly changed as Mudlet sorts this matter out, if a number assigned in this way &lt;b&gt;is&lt;/b&gt; important to you, you can change it back, provided you rename the area that has been allocated the suffix that was wanted first...!&lt;/p&gt;</source>
      <translation>Es wurde erkannt, dass Suffixe in der Form &quot;_###&quot; bereits verwendet wurden. Diese wurden zur Einfachheit der Umbenennung entfernt und eventuell geändert. Wenn eine zugewiesene Nummer auf diese Weise wichtig ist, kann sie zurück geändert werden. Dazu muss zuerst das Gebiet umbenannt werden, dem das gewünschte Suffix zugewiesen wurde.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1216"/>
      <source>[  OK  ]  - The changes made are:
(ID) &quot;old name&quot; ==&gt; &quot;new name&quot;
</source>
      <translation>[  OK  ]  - Die durchgeführten Änderungen sind:
(ID) &quot;alter Name&quot; ==&gt; &quot;neuer Name&quot;
</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1223"/>
      <source>&lt;nothing&gt;</source>
      <translation>&lt;nothing&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1226"/>
      <source>[ INFO ]  - Area name changed to prevent duplicates or unnamed ones; old name: &quot;%1&quot;, new name: &quot;%2&quot;.</source>
      <translation>[ INFO ]  - Gebietsnamen geändert, um Duplikate und Unbenannte zu vermeiden. Alter Name: &quot;%1&quot;, neuer Name: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1235"/>
      <source>[ ALERT ] - Empty and duplicate area names detected in Map file!</source>
      <translation>[ ACHTUNG ] - Leere und doppelte Gebietsnamen in der Kartendatei entdeckt!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1236"/>
      <source>[ INFO ]  - Due to some situations not being checked in the past,  Mudlet had
allowed the map to have more than one area with the same or no name.
These make some things confusing and are now disallowed.
  To resolve these cases, an area without a name here (or created in
the future) will automatically be assigned the name &quot;%1&quot;.
  Duplicated area names will cause all but the first encountered one
to gain a &quot;_###&quot; style suffix where each &quot;###&quot; is an increasing
number; you may wish to change these, perhaps by replacing them with
a &quot;(sub-area name)&quot; but it is entirely up to you how you do this,
other than you will not be able to set one area&apos;s name to that of
another that exists at the time.
  If there were more than one area without a name then all but the
first will also gain a suffix in this manner.
%2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1252"/>
      <source>[ ALERT ] - Duplicate area names detected in the Map file!</source>
      <translation>[ ACHTUNG ] - Doppelte Gebietsnamen in der Kartendatei entdeckt!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1253"/>
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
      <location filename="../src/TRoomDB.cpp" line="1268"/>
      <source>[ ALERT ] - An empty area name was detected in the Map file!</source>
      <translation>[ ACHTUNG ] - Ein unbenanntes Gebiet wurde in der Kartendatei festgestellt!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1271"/>
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
      <location filename="../src/TRoomDB.cpp" line="1295"/>
      <source>[ INFO ]  - Default (reset) area name (for rooms that have not been assigned to an
area) not found, adding &quot;%1&quot; against the reserved -1 id.</source>
      <translation>[ INFO ]  - Standardgebietsname (Reset) (für Räume, die nicht zu einem Gebiet zugewiesen wurden) wurde nicht gefunden. Ergänze &quot;%1&quot; gegen die reservierte ID -1.</translation>
    </message>
  </context>
  <context>
    <name>TTextEdit</name>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1298"/>
      <source>Copy</source>
      <translation>Kopieren</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1309"/>
      <source>Copy HTML</source>
      <translation>HTML kopieren</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1313"/>
      <source>Copy as image</source>
      <translation>Als Bild kopieren</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1316"/>
      <source>Select All</source>
      <translation>Alles auswählen</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1321"/>
      <source>Search on %1</source>
      <translation>Mit %1 suchen</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1339"/>
      <source>Analyse characters</source>
      <translation>Zeichen analysieren</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1348"/>
      <source>&lt;p&gt;Hover on this item to display the Unicode codepoints in the selection &lt;i&gt;(only the first line!)&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Auf dieses Element zeigen, um die Unicode Codepoints der Auswahl anzuzeigen &lt;i&gt;(nur die erste Zeile!)&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1357"/>
      <source>restore Main menu</source>
      <translation>Hauptmenü wiederherstellen</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1359"/>
      <source>Use this to restore the Main menu to get access to controls.</source>
      <translation>Verwende dieses, um das Hauptmenü wiederherzustellen und Zugriff auf die Steuerelemente zu erhalten.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1361"/>
      <source>restore Main Toolbar</source>
      <translation>Hauptsymbolleiste wiederherstellen</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1363"/>
      <source>Use this to restore the Main Toolbar to get access to controls.</source>
      <translation>Verwende dies, um die Hauptsymbolleiste wiederherzustellen und Zugriff auf die Steuerelemente zu erhalten.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1439"/>
      <source>Mudlet, debug console extract</source>
      <translation>Mudlet, Auszug der Debug-Konsole</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1441"/>
      <source>Mudlet, %1 mini-console extract from %2 profile</source>
      <translation>Mudlet, Auszug der %1 Minikonsole des %2 Profils</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1443"/>
      <source>Mudlet, %1 user window extract from %2 profile</source>
      <translation>Mudlet, Auszug des %1 Benutzerfensters des %2 Profils</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1445"/>
      <source>Mudlet, main console extract from %1 profile</source>
      <translation>Mudlet, Auszug des Hauptfensters des %1 Profils</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1913"/>
      <source>{tab}</source>
      <comment>Unicode U+0009 codepoint.</comment>
      <translation>{tab}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1914"/>
      <source>{line-feed}</source>
      <comment>Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation>{line-feed}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1915"/>
      <source>{carriage-return}</source>
      <comment>Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation>{carriage-return}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1916"/>
      <source>{space}</source>
      <comment>Unicode U+0020 codepoint.</comment>
      <translation>{space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1917"/>
      <source>{non-breaking space}</source>
      <comment>Unicode U+00A0 codepoint.</comment>
      <translation>{non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1918"/>
      <source>{soft hyphen}</source>
      <comment>Unicode U+00AD codepoint.</comment>
      <translation>{soft hyphen}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1919"/>
      <source>{combining grapheme joiner}</source>
      <comment>Unicode U+034F codepoint (badly named apparently - see Wikipedia!)</comment>
      <translation>{combining grapheme joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1920"/>
      <source>{ogham space mark}</source>
      <comment>Unicode U+1680 codepoint.</comment>
      <translation>{ogham space mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1921"/>
      <source>{&apos;n&apos; quad}</source>
      <comment>Unicode U+2000 codepoint.</comment>
      <translation>{&apos;n&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1922"/>
      <source>{&apos;m&apos; quad}</source>
      <comment>Unicode U+2001 codepoint.</comment>
      <translation>{&apos;m&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1923"/>
      <source>{&apos;n&apos; space}</source>
      <comment>Unicode U+2002 codepoint - En (&apos;n&apos;) wide space.</comment>
      <translation>{&apos;n&apos; space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1924"/>
      <source>{&apos;m&apos; space}</source>
      <comment>Unicode U+2003 codepoint - Em (&apos;m&apos;) wide space.</comment>
      <translation>{&apos;m&apos; space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1925"/>
      <source>{3-per-em space}</source>
      <comment>Unicode U+2004 codepoint - three-per-em (&apos;m&apos;) wide (thick) space.</comment>
      <translation>{3-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1926"/>
      <source>{4-per-em space}</source>
      <comment>Unicode U+2005 codepoint - four-per-em (&apos;m&apos;) wide (Middle) space.</comment>
      <translation>{4-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1927"/>
      <source>{6-per-em space}</source>
      <comment>Unicode U+2006 codepoint - six-per-em (&apos;m&apos;) wide (Sometimes the same as a Thin) space.</comment>
      <translation>{6-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1928"/>
      <source>{digit space}</source>
      <comment>Unicode U+2007 codepoint - figure (digit) wide space.</comment>
      <translation>{digit space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1929"/>
      <source>{punctuation wide space}</source>
      <comment>Unicode U+2008 codepoint.</comment>
      <translation>{punctuation wide space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1930"/>
      <source>{5-per-em space}</source>
      <comment>Unicode U+2009 codepoint - five-per-em (&apos;m&apos;) wide space.</comment>
      <translation>{5-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1931"/>
      <source>{hair width space}</source>
      <comment>Unicode U+200A codepoint - thinnest space.</comment>
      <translation>{hair width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1932"/>
      <source>{zero width space}</source>
      <comment>Unicode U+200B codepoint.</comment>
      <translation>{zero width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1933"/>
      <source>{Zero width non-joiner}</source>
      <comment>Unicode U+200C codepoint.</comment>
      <translation>{Zero width non-joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1934"/>
      <source>{zero width joiner}</source>
      <comment>Unicode U+200D codepoint.</comment>
      <translation>{zero width joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1935"/>
      <source>{left-to-right mark}</source>
      <comment>Unicode U+200E codepoint.</comment>
      <translation>{left-to-right mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1936"/>
      <source>{right-to-left mark}</source>
      <comment>Unicode U+200F codepoint.</comment>
      <translation>{right-to-left mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1937"/>
      <source>{line separator}</source>
      <comment>Unicode 0x2028 codepoint.</comment>
      <translation>{line separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1938"/>
      <source>{paragraph separator}</source>
      <comment>Unicode U+2029 codepoint.</comment>
      <translation>{paragraph separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1939"/>
      <source>{Left-to-right embedding}</source>
      <comment>Unicode U+202A codepoint.</comment>
      <translation>{Left-to-right embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1940"/>
      <source>{right-to-left embedding}</source>
      <comment>Unicode U+202B codepoint.</comment>
      <translation>{right-to-left embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1941"/>
      <source>{pop directional formatting}</source>
      <comment>Unicode U+202C codepoint - pop (undo last) directional formatting.</comment>
      <translation>{pop directional formatting}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1942"/>
      <source>{Left-to-right override}</source>
      <comment>Unicode U+202D codepoint.</comment>
      <translation>{Left-to-right override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1943"/>
      <source>{right-to-left override}</source>
      <comment>Unicode U+202E codepoint.</comment>
      <translation>{right-to-left override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1944"/>
      <source>{narrow width no-break space}</source>
      <comment>Unicode U+202F codepoint.</comment>
      <translation>{narrow width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1945"/>
      <source>{medium width mathematical space}</source>
      <comment>Unicode U+205F codepoint.</comment>
      <translation>{medium width mathematical space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1946"/>
      <source>{zero width non-breaking space}</source>
      <comment>Unicode U+2060 codepoint.</comment>
      <translation>{zero width non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1947"/>
      <source>{function application}</source>
      <comment>Unicode U+2061 codepoint - function application (whatever that means!)</comment>
      <translation>{function application}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1948"/>
      <source>{invisible times}</source>
      <comment>Unicode U+2062 codepoint.</comment>
      <translation>{invisible times}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1949"/>
      <source>{invisible separator}</source>
      <comment>Unicode U+2063 codepoint - invisible separator or comma.</comment>
      <translation>{invisible separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1950"/>
      <source>{invisible plus}</source>
      <comment>Unicode U+2064 codepoint.</comment>
      <translation>{invisible plus}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1951"/>
      <source>{left-to-right isolate}</source>
      <comment>Unicode U+2066 codepoint.</comment>
      <translation>{left-to-right isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1952"/>
      <source>{right-to-left isolate}</source>
      <comment>Unicode U+2067 codepoint.</comment>
      <translation>{right-to-left isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1953"/>
      <source>{first strong isolate}</source>
      <comment>Unicode U+2068 codepoint.</comment>
      <translation>{first strong isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1954"/>
      <source>{pop directional isolate}</source>
      <comment>Unicode U+2069 codepoint - pop (undo last) directional isolate.</comment>
      <translation>{pop directional isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1955"/>
      <source>{inhibit symmetrical swapping}</source>
      <comment>Unicode U+206A codepoint.</comment>
      <translation>{inhibit symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1956"/>
      <source>{activate symmetrical swapping}</source>
      <comment>Unicode U+206B codepoint.</comment>
      <translation>{activate symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1957"/>
      <source>{inhibit arabic form-shaping}</source>
      <comment>Unicode U+206C codepoint.</comment>
      <translation>{inhibit arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1958"/>
      <source>{activate arabic form-shaping}</source>
      <comment>Unicode U+206D codepoint.</comment>
      <translation>{activate arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1959"/>
      <source>{national digit shapes}</source>
      <comment>Unicode U+206E codepoint.</comment>
      <translation>{national digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1960"/>
      <source>{nominal Digit shapes}</source>
      <comment>Unicode U+206F codepoint.</comment>
      <translation>{nominal Digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1961"/>
      <source>{ideaographic space}</source>
      <comment>Unicode U+3000 codepoint - ideaographic (CJK Wide) space</comment>
      <translation>{ideaographic space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1962"/>
      <source>{variation selector 1}</source>
      <comment>Unicode U+FE00 codepoint.</comment>
      <translation>{variation selector 1}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1963"/>
      <source>{variation selector 2}</source>
      <comment>Unicode U+FE01 codepoint.</comment>
      <translation>{variation selector 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1964"/>
      <source>{variation selector 3}</source>
      <comment>Unicode U+FE02 codepoint.</comment>
      <translation>{variation selector 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1965"/>
      <source>{variation selector 4}</source>
      <comment>Unicode U+FE03 codepoint.</comment>
      <translation>{variation selector 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1966"/>
      <source>{variation selector 5}</source>
      <comment>Unicode U+FE04 codepoint.</comment>
      <translation>{variation selector 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1967"/>
      <source>{variation selector 6}</source>
      <comment>Unicode U+FE05 codepoint.</comment>
      <translation>{variation selector 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1968"/>
      <source>{variation selector 7}</source>
      <comment>Unicode U+FE06 codepoint.</comment>
      <translation>{variation selector 7}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1969"/>
      <source>{variation selector 8}</source>
      <comment>Unicode U+FE07 codepoint.</comment>
      <translation>{variation selector 8}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1970"/>
      <source>{variation selector 9}</source>
      <comment>Unicode U+FE08 codepoint.</comment>
      <translation>{variation selector 9}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1971"/>
      <source>{variation selector 10}</source>
      <comment>Unicode U+FE09 codepoint.</comment>
      <translation>{variation selector 10}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1972"/>
      <source>{variation selector 11}</source>
      <comment>Unicode U+FE0A codepoint.</comment>
      <translation>{variation selector 11}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1973"/>
      <source>{variation selector 12}</source>
      <comment>Unicode U+FE0B codepoint.</comment>
      <translation>{variation selector 12}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1974"/>
      <source>{variation selector 13}</source>
      <comment>Unicode U+FE0C codepoint.</comment>
      <translation>{variation selector 13}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1975"/>
      <source>{variation selector 14}</source>
      <comment>Unicode U+FE0D codepoint.</comment>
      <translation>{variation selector 14}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1976"/>
      <source>{variation selector 15}</source>
      <comment>Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black &amp; white) rendition.</comment>
      <translation>{variation selector 15}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1977"/>
      <source>{variation selector 16}</source>
      <comment>Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured &apos;Emoji&apos; rendition.</comment>
      <translation>{variation selector 16}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1978"/>
      <source>{zero width no-break space}</source>
      <comment>Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).</comment>
      <translation>{zero width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1984"/>
      <source>{interlinear annotation anchor}</source>
      <comment>Unicode U+FFF9 codepoint.</comment>
      <translation>{interlinear annotation anchor}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1985"/>
      <source>{interlinear annotation separator}</source>
      <comment>Unicode U+FFFA codepoint.</comment>
      <translation>{interlinear annotation separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1986"/>
      <source>{interlinear annotation terminator}</source>
      <comment>Unicode U+FFFB codepoint.</comment>
      <translation>{interlinear annotation terminator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1987"/>
      <source>{object replacement character}</source>
      <comment>Unicode U+FFFC codepoint.</comment>
      <translation>{object replacement character}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1999"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFD0 to U+FDEF - not a character.</comment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2001"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFFx - not a character.</comment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2010"/>
      <source>{FitzPatrick modifier 1 or 2}</source>
      <comment>Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.</comment>
      <translation>{FitzPatrick modifier 1 or 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2011"/>
      <source>{FitzPatrick modifier 3}</source>
      <comment>Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.</comment>
      <translation>{FitzPatrick modifier 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2012"/>
      <source>{FitzPatrick modifier 4}</source>
      <comment>Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.</comment>
      <translation>{FitzPatrick modifier 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2013"/>
      <source>{FitzPatrick modifier 5}</source>
      <comment>Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.</comment>
      <translation>{FitzPatrick modifier 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2014"/>
      <source>{FitzPatrick modifier 6}</source>
      <comment>Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.</comment>
      <translation>{FitzPatrick modifier 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2018"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.</comment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2289"/>
      <location filename="../src/TTextEdit.cpp" line="2351"/>
      <source>Index (UTF-16)</source>
      <comment>1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}</comment>
      <translation>Index (UTF-16)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2292"/>
      <location filename="../src/TTextEdit.cpp" line="2353"/>
      <source>U+&lt;i&gt;####&lt;/i&gt; Unicode Code-point &lt;i&gt;(High:Low Surrogates)&lt;/i&gt;</source>
      <comment>2nd Row heading for Text analyser output, table item is the unicode code point (will be between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}</comment>
      <translation>U+&lt;i&gt;####&lt;/i&gt; Unicode-Codepoint &lt;i&gt;(High:Low-Surrogates)&lt;/i&gt;</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2296"/>
      <location filename="../src/TTextEdit.cpp" line="2357"/>
      <source>Visual</source>
      <comment>3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a &apos;{&apos;...&apos;}&apos; wrapped letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}</comment>
      <translation>Ansicht</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2300"/>
      <location filename="../src/TTextEdit.cpp" line="2361"/>
      <source>Index (UTF-8)</source>
      <comment>4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation>Index (UTF-8)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2304"/>
      <location filename="../src/TTextEdit.cpp" line="2365"/>
      <source>Byte</source>
      <comment>5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation>Byte</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2308"/>
      <location filename="../src/TTextEdit.cpp" line="2369"/>
      <source>Lua character or code</source>
      <comment>6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2 times}</comment>
      <translation>Lua Zeichen oder Code</translation>
    </message>
  </context>
  <context>
    <name>TToolBar</name>
    <message>
      <location filename="../src/TToolBar.cpp" line="74"/>
      <source>Toolbar - %1 - %2</source>
      <translation>Symbolleiste - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TTrigger</name>
    <message>
      <location filename="../src/TTrigger.cpp" line="191"/>
      <source>Error: This trigger has no patterns defined, yet. Add some to activate it.</source>
      <translation>Fehler: Dieser Trigger hat noch keine Muster definiert. Füge einige hinzu, um ihn zu aktivieren.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="222"/>
      <source>Error: in item %1, perl regex &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>Fehler: Bei Element %1 konnte Perl-Regex &quot;%2&quot; nicht kompilieren, Grund: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="243"/>
      <source>Error: in item %1, lua function &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>Fehler: Bei Element %1 konnte Lua-Funktion &quot;%2&quot; nicht kompilieren, Grund: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="263"/>
      <source>Error: in item %1, no colors to match were set - at least &lt;i&gt;one&lt;/i&gt; of the foreground or background must not be &lt;i&gt;ignored&lt;/i&gt;.</source>
      <translation>Fehler bei Element %1: Keine zu suchenden Farben gewählt - mindestens &lt;i&gt;eine&lt;/i&gt; der Vordergrund- oder Hintergrundfarben darf nicht &lt;i&gt;ignoriert&lt;/i&gt; werden.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="319"/>
      <location filename="../src/TTrigger.cpp" line="389"/>
      <source>[Trigger Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation>[Trigger-Fehler:] %1 Limit für erfasste Gruppen überschritten. Erfasse weniger Gruppen.
</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="1148"/>
      <source>Trigger name=%1 expired.
</source>
      <translation>Trigger name=%1 abgelaufen.
</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TTrigger.cpp" line="1151"/>
      <source>Trigger name=%1 will fire %n more time(s).
</source>
      <translation>
        <numerusform>Trigger name=%1 wird noch %n mal auslösen.
</numerusform>
        <numerusform>Trigger name=%1 wird noch %n mal auslösen.
</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>UpdateDialog</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="20"/>
      <source>%APPNAME% update</source>
      <translation>%APPNAME% aktualisieren</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="50"/>
      <source>Loading update information …</source>
      <translation>Lade Informationen der Aktualisierung …</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="87"/>
      <source>A new version of %APPNAME% is available!</source>
      <translation>Eine neue Version von %APPNAME% ist verfügbar!</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="113"/>
      <source>%APPNAME% %UPDATE_VERSION% is available (you have %CURRENT_VERSION%).
Would you like to update now?</source>
      <translation>%APPNAME% %UPDATE_VERSION% ist verfügbar (Du hast %CURRENT_VERSION%).
Möchtest Du jetzt aktualisieren?</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="151"/>
      <source>Changelog for %APPNAME%</source>
      <translation>Änderungsprotokoll für %APPNAME%</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="161"/>
      <source>You are using version %CURRENT_VERSION%.</source>
      <translation>Du verwendest Version %CURRENT_VERSION%.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="192"/>
      <source>There are currently no updates available.</source>
      <translation>Derzeit sind keine Aktualisierungen verfügbar.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="208"/>
      <source>You are using %APPNAME% %CURRENT_VERSION%.</source>
      <translation>Du verwendest %APPNAME% %CURRENT_VERSION%.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="321"/>
      <source>Automatically download future updates</source>
      <translation>Zukünftige Aktualisierungen automatisch herunterladen</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="368"/>
      <source>Cancel</source>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="388"/>
      <source>Install update now</source>
      <translation>Aktualisierung jetzt installieren</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="395"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="405"/>
      <source>Remind me later</source>
      <translation>Erinnere mich später</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="410"/>
      <source>Skip this version</source>
      <translation>Überspringe diese Version</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.cpp" line="555"/>
      <source>Could not open downloaded file %1</source>
      <translation>Die heruntergeladene Datei %1 konnte nicht geöffnet werden</translation>
    </message>
  </context>
  <context>
    <name>Updater</name>
    <message>
      <location filename="../src/updater.cpp" line="46"/>
      <location filename="../src/updater.cpp" line="195"/>
      <location filename="../src/updater.cpp" line="261"/>
      <source>Update</source>
      <translation>Aktualisieren</translation>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="352"/>
      <source>Restart to apply update</source>
      <translation>Neu starten zum Aktualisieren</translation>
    </message>
  </context>
  <context>
    <name>XMLimport</name>
    <message>
      <location filename="../src/XMLimport.cpp" line="164"/>
      <source>[ ALERT ] - Sorry, the file being read:
&quot;%1&quot;
reports it has a version (%2) it must have come from a later Mudlet version,
and this one cannot read it, you need a newer Mudlet!</source>
      <translation>[ ACHTUNG ] - Entschuldigung, aber die gelesene Datei: 
&quot;%1&quot;
berichtet ihre Version als %2. Sie muss aus einer späteren Mudlet Version stammen,
diese Version kann diese Datei nicht lesen. Du benötigst ein aktuelleres Mudlet!</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="348"/>
      <source>Parsing area data...</source>
      <translation>Analysiere Gebietsdaten...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="352"/>
      <source>Parsing room data...</source>
      <translation>Analysiere Raumdaten...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="356"/>
      <source>Parsing environment data...</source>
      <translation>Analysiere Umgebungsdaten...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="364"/>
      <source>Assigning rooms to their areas...</source>
      <translation>Weise die Räume ihren Gebieten zu...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="519"/>
      <source>Parsing room data [count: %1]...</source>
      <translation>Analysiere Raumdaten [Anzahl: %1]...</translation>
    </message>
  </context>
  <context>
    <name>about_dialog</name>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="41"/>
      <source>About Mudlet</source>
      <translation>Über Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="122"/>
      <source>Mudlet</source>
      <translation>Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="185"/>
      <source>Supporters</source>
      <translation>Unterstützer</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="214"/>
      <source>License</source>
      <translation>Lizenz</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="249"/>
      <source>Third Party</source>
      <translation>Drittanbieter</translation>
    </message>
  </context>
  <context>
    <name>actions_main_area</name>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="115"/>
      <source>Button Bar Properties</source>
      <translation>Eigenschaften der Button-Leiste</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="127"/>
      <source>Number of columns/rows (depending on orientation):</source>
      <translation>Anzahl der Spalten/Zeilen (je nach Ausrichtung):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="150"/>
      <source>Orientation Horizontal</source>
      <translation>Horizontale Ausrichtung</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="155"/>
      <source>Orientation Vertical</source>
      <translation>Vertikale Ausrichtung</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="170"/>
      <source>Dock Area Top</source>
      <translation>Dock-Bereich oben</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="175"/>
      <source>Dock Area Left</source>
      <translation>Dock-Bereich links</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="180"/>
      <source>Dock Area Right</source>
      <translation>Dock-Bereich rechts</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="185"/>
      <source>Floating Toolbar</source>
      <translation>Schwebende Werkzeugleiste</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="208"/>
      <source>Button Properties</source>
      <translation>Button-Eigenschaften</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="214"/>
      <source>Button Rotation:</source>
      <translation>Drehung des Buttons:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="231"/>
      <source>no rotation</source>
      <translation>Keine Drehung</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="236"/>
      <source>90° rotation to the left</source>
      <translation>90° Drehung nach links</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="241"/>
      <source>90° rotation to the right</source>
      <translation>90° Drehung nach rechts</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="249"/>
      <source>Push down button</source>
      <translation>Einrastender Button</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="256"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="266"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game if it is pressed. (Optional)&lt;/p&gt;&lt;p&gt;If this is a &lt;i&gt;push-down&lt;/i&gt; button then this is sent only when the button goes from the &lt;i&gt;up&lt;/i&gt; to &lt;i&gt;down&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die der Button direkt an das Spiel senden soll, wenn er gedrückt wird. (Optional)&lt;/p&gt;&lt;p&gt;Falls dies ein &lt;i&gt;einrastender&lt;/i&gt; Button ist, dann wird dies nur gesendet, sobald der Button den Status von &lt;i&gt;oben&lt;/i&gt; nach &lt;i&gt;unten&lt;/i&gt; wechselt.&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="269"/>
      <location filename="../src/ui/actions_main_area.ui" line="289"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Text, der unverändert ans Spiel gesendet werden soll (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="286"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game when this button goes from the &lt;i&gt;down&lt;/i&gt; to &lt;i&gt;up&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die der Button direkt an das Spiel senden soll, wenn der Button vom &lt;i&gt;unten&lt;/i&gt; in den &lt;i&gt;oben&lt;/i&gt; Status wechselt.&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="276"/>
      <source>Command (up):</source>
      <translation>Befehl (nach oben):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="72"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your button, menu or toolbar. This will be displayed in the buttons tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen guten, bestenfalls einzigartigen Namen für deinen Button, das Menu oder die Werkzeugleiste. Dieser wird in der Liste angezeigt werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="308"/>
      <source>Stylesheet:</source>
      <translation>Stylesheet:</translation>
    </message>
  </context>
  <context>
    <name>aliases_main_area</name>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="35"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="57"/>
      <source>choose a unique name for your alias; it will show in the tree and is needed for scripting.</source>
      <translation>Wähle einen eindeutigen Namen für dein Alias. Er wird in der Liste angezeigt und für Skripte benötigt.</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="76"/>
      <source>Pattern:</source>
      <translation>Muster:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="103"/>
      <source>enter a perl regex pattern for your alias; alias are triggers on your input</source>
      <translation>Trage ein Perl Regex Muster für dein Alias ein. Aliase reagieren auf deine Eingaben.</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="110"/>
      <source>Type:</source>
      <translation>Art:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="124"/>
      <source>Regex</source>
      <translation>Regulärer Ausdruck</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="129"/>
      <source>Plain</source>
      <translation>Einfach</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="149"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="171"/>
      <source>&lt;p&gt;Type in one or more commands you want the alias to send directly to the game if the keys entered match the pattern. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die das Alias direkt an das Spiel senden soll, wenn der eingegebene Text dem Muster entspricht. (Optional)&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="174"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Text, der unverändert ans Spiel gesendet werden soll (optional)</translation>
    </message>
  </context>
  <context>
    <name>cTelnet</name>
    <message>
      <location filename="../src/ctelnet.cpp" line="609"/>
      <source>[ INFO ]  - The IP address of %1 has been found. It is: %2
</source>
      <translation>[ INFO ]  - Die IP-Adresse von %1 wurde gefunden. Es ist: %2
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="618"/>
      <source>[ ERROR ] - Host name lookup Failure!
Connection cannot be established.
The server name is not correct, not working properly,
or your nameservers are not working properly.</source>
      <translation>[FEHLER] - Nachschlagen des Servernamens fehlgeschlagen! Verbindung kann nicht hergestellt werden. Der Name des Servers ist nicht korrekt, funktioniert nicht richtig, oder die Nameserver arbeiten nicht richtig.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="449"/>
      <source>[ ERROR ] - TCP/IP socket ERROR:</source>
      <translation>[FEHLER] - TCP/IP-Socket-FEHLER:</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="476"/>
      <source>[ INFO ]  - A secure connection has been established successfully.</source>
      <translation>[ INFO ]  - Eine sichere Verbindung wurde erfolgreich hergestellt.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="478"/>
      <source>[ INFO ]  - A connection has been established successfully.</source>
      <translation>[ INFO ]  - Eine Verbindung wurde erfolgreich hergestellt.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="514"/>
      <source>[ INFO ]  - Connection time: %1
    </source>
      <translation>[ INFO ]  - Verbindungszeit: %1
    </translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="557"/>
      <source>Secure connections aren&apos;t supported by this game on this port - try turning the option off.</source>
      <translation>Sichere Verbindungen werden von diesem Spiel auf diesem Port nicht unterstützt - Versuche, die Option zu deaktivieren.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="602"/>
      <source>[ INFO ]  - Trying secure connection to %1: %2 ...
</source>
      <translation>[ INFO ]  - Versuche, eine sichere Verbindung aufzubauen zu %1: %2 ...
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1650"/>
      <location filename="../src/ctelnet.cpp" line="2017"/>
      <source>[ INFO ]  - The server wants to upgrade the GUI to new version &apos;%1&apos;.
Uninstalling old version &apos;%2&apos;.</source>
      <translation>[ INFO ]  - Der Server möchte die GUI auf die neue Version &apos;%1&apos; aktualisieren. 
Die alte Version &apos;%2&apos; wird deinstalliert.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1661"/>
      <location filename="../src/ctelnet.cpp" line="2028"/>
      <source>[  OK  ]  - Package is already installed.</source>
      <translation>[  OK  ]  - Paket  ist bereits installiert.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1670"/>
      <location filename="../src/ctelnet.cpp" line="2037"/>
      <source>downloading game GUI from server</source>
      <translation>Spiel-GUI wird vom Server geladen</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1670"/>
      <location filename="../src/ctelnet.cpp" line="2037"/>
      <source>Cancel</source>
      <comment>Cancel download of GUI package from Server</comment>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1659"/>
      <location filename="../src/ctelnet.cpp" line="2026"/>
      <source>[ INFO ]  - Server offers downloadable GUI (url=&apos;%1&apos;) (package=&apos;%2&apos;).</source>
      <translation>[ INFO ]  - Server bietet GUI zum Download an (URL=&apos;%1&apos;) (Paket=&apos;%2&apos;).</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="418"/>
      <source>[ INFO ]  - Looking up the IP address of server: %1:%2 ...</source>
      <translation>[ INFO ]  - Suche die IP-Adresse des Servers: %1:%2 ...</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="516"/>
      <source>hh:mm:ss.zzz</source>
      <comment>This is the format to be used to show the profile connection time, it follows the rules of the &quot;QDateTime::toString(...)&quot; function and may need modification for some locales, e.g. France, Spain.</comment>
      <translation>hh:mm:ss.zzz</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="547"/>
      <location filename="../src/ctelnet.cpp" line="559"/>
      <source>[ ALERT ] - Socket got disconnected.
Reason: </source>
      <translation>[ ACHTUNG ] - Verbindung wurde getrennt.
Grund: </translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="611"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 ...
</source>
      <translation>[ INFO ]  - Versuche, eine Verbindung aufzubauen zu %1:%2 ...
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="613"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 via proxy...
</source>
      <translation>[ INFO ]  - Versuche, eine Verbindung über Proxy aufzubauen zu %1:%2 ...
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="660"/>
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
      <location filename="../src/ctelnet.cpp" line="2320"/>
      <source>ERROR</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>FEHLER</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2332"/>
      <source>LUA</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>LUA</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2343"/>
      <source>WARN</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>WARNUNG</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2354"/>
      <source>ALERT</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>ACHTUNG</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2365"/>
      <source>INFO</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>INFO</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2376"/>
      <source>OK</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2559"/>
      <source>[ INFO ]  - Loading replay file:
&quot;%1&quot;.</source>
      <translation>[ INFO ]  - Lade Wiederholung aus Datei: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2578"/>
      <source>Cannot perform replay, another one may already be in progress. Try again when it has finished.</source>
      <translation>Kann Wiederholung nicht anzeigen. Eine andere könnte bereits laufen. Bitte wiederholen, nachdem diese endet.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2580"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress.
Try again when it has finished.</source>
      <translation>[ WARNUNG ]  - Kann Wiederholung nicht ausführen. Eine andere könnte 
bereits laufen. Bitte wiederholen, nachdem diese endet.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2588"/>
      <source>Cannot read file &quot;%1&quot;, error message was: &quot;%2&quot;.</source>
      <translation>Kann Datei &quot;%1&quot; nicht lesen, Fehlermeldung war: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2591"/>
      <source>[ ERROR ] - Cannot read file &quot;%1&quot;,
error message was: &quot;%2&quot;.</source>
      <translation>[FEHLER] - Kann Datei &quot;%1&quot; nicht lesen,
Fehlermeldung war: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2619"/>
      <source>[  OK  ]  - The replay has ended.</source>
      <translation>[  OK  ]  - Wiederholung beendet.</translation>
    </message>
  </context>
  <context>
    <name>color_trigger</name>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="17"/>
      <source>ANSI 256 Color chooser</source>
      <translation>ANSI-256-Farben-Auswahl</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="39"/>
      <source>&lt;small&gt;Choose:&lt;ul&gt;&lt;li&gt;one of the basic 16 colors below&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;more&lt;/i&gt; button to get access to other colors in the 256-color set, then follow the instructions to select a color from that part of the 256 colors supported; if such a color is already in use then that part will already be showing&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;Default&lt;/i&gt; or &lt;i&gt;Ignore&lt;/i&gt; buttons at the bottom for a pair of other special cases&lt;/li&gt;
&lt;li&gt;click &lt;i&gt;Cancel&lt;/i&gt; to close this dialog without making any changes&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</source>
      <comment>Ensure that &quot;Default&quot;, &quot;Ignore&quot; and &quot;Cancel&quot; in this instruction are the same as used for the controls elsewhere on this dialog.</comment>
      <translation>&lt;small&gt;Wähle:&lt;ul&gt;&lt;li&gt;eine der 16 Grundfarben unten&lt;/li&gt;
&lt;li&gt;Klicke auf &lt;i&gt;mehr&lt;/i&gt;, damit die restlichen Farben des 256-Farbspektrums angezeigt werden, dann folge den Anweisungen, um eine dieser 256 unterstützten Farben zu wählen; Falls eine solche Farbe schon benutzt wird, dann wird dieser Bereich bereits angezeigt&lt;/li&gt;
&lt;li&gt;Klicke unten auf die Schaltflächen &lt;i&gt;Standard&lt;/i&gt; oder &lt;i&gt;Ignorieren&lt;/i&gt; für zwei weitere besondere Möglichkeiten&lt;/li&gt;
&lt;li&gt;Klicke &lt;i&gt;Abbrechen&lt;/i&gt;, um dieses Fenster zu schließen, ohne irgendwelche Änderungen durchzuführen&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="58"/>
      <source>Basic ANSI Colors [0-15] - click a button to select that color number directly:</source>
      <translation>ANSI Basisfarben [0-15] - Klicke auf eine Schaltfläche, um diese Farbnummer direkt auszuwählen:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="240"/>
      <source>ANSI 6R x 6G x 6B Colors [16-231] - adjust red, green, blue and click button to select matching color number:</source>
      <translation>ANSI Farben 6R x 6G x 6B [16-231] - Stelle Rot, Grün, Blau ein und wähle so die passende Farbnummer:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="252"/>
      <source>Red (0-5)</source>
      <translation>Rot (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="262"/>
      <source>Green (0-5)</source>
      <translation>Grün (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="272"/>
      <source>Blue (0-5)</source>
      <translation>Blau (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="282"/>
      <source>16 + 36 x R + 6 x G + B =</source>
      <translation>16 + 36 x R + 6 x G + B =</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="343"/>
      <source>[16]</source>
      <translation>[16]</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="353"/>
      <source>Set to RGB value</source>
      <translation>Wähle den RGB-Wert</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="363"/>
      <source>ANSI 24 Grays scale [232-255] - adjust gray and click button to select matching color number:</source>
      <translation>ANSI 24 Graustufen [232-255] - Stelle Grau ein und wähle so die passende Farbnummer:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="375"/>
      <source>Gray (0-23)</source>
      <translation>Grau (0-23)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="382"/>
      <source>232 + Gr =</source>
      <translation>232 + Gr =</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="414"/>
      <source>[232]</source>
      <translation>[232]</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="424"/>
      <source>Set to Grayscale value</source>
      <translation>Wähle Graustufen-Wert</translation>
    </message>
  </context>
  <context>
    <name>composer</name>
    <message>
      <location filename="../src/ui/composer.ui" line="14"/>
      <source>News and Message Composer</source>
      <translation>Nachrichten und Mitteilungen erstellen</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="86"/>
      <source>Cancel</source>
      <translation>Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="99"/>
      <source>&lt;p&gt;Save (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</source>
      <translation>&lt;p&gt; Speichern (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="102"/>
      <source>Save</source>
      <translation>Speichern</translation>
    </message>
  </context>
  <context>
    <name>connection_profiles</name>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="20"/>
      <source>Select a profile to connect with</source>
      <translation>Mit welchem Profil möchtest du dich verbinden?</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="101"/>
      <source>profiles list</source>
      <translation>Liste der Profile</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="363"/>
      <source>Remove</source>
      <translation>Löschen</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="382"/>
      <source>Copy</source>
      <translation>Kopieren</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="407"/>
      <source>New</source>
      <translation>Neu</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="462"/>
      <source>welcome message</source>
      <translation>Willkommensnachricht</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="478"/>
      <source>Required</source>
      <translation>Erforderlich</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="484"/>
      <source>Profile name:</source>
      <translation>Profilname:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="500"/>
      <source>Profile name</source>
      <translation>Profilname</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="503"/>
      <source>A unique name for the profile but which is limited to a subset of ascii characters only.</source>
      <comment>Using lower case letters for &apos;ASCII&apos; may make speech synthesisers say &apos;askey&apos; which is quicker than &apos;Aay Ess Cee Eye Eye&apos;!</comment>
      <translation>Ein eindeutiger Name für das Profil, der jedoch nur aus Ascii-Zeichen bestehen darf.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="513"/>
      <source>Server address:</source>
      <translation>Serveradresse:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="535"/>
      <source>Game server URL</source>
      <translation>URL des Spielservers</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="538"/>
      <source>The Internet host name or IP address</source>
      <translation>Hostname oder IP-Adresse</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="545"/>
      <source>Port:</source>
      <translation>Port:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="573"/>
      <source>Game server port</source>
      <translation>Port des Spielservers</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="576"/>
      <source>The port that is used together with the server name to make the connection to the game server. If not specified a default of 23 for &quot;Telnet&quot; connections is used. Secure connections may require a different port number.</source>
      <translation>Der Port, der zusammen mit dem Servernamen verwendet wird, um die Verbindung zum Spielserver herzustellen. Wenn nicht angegeben, wird der Standard von 23 für &quot;Telnet&quot; Verbindungen verwendet. Für sichere Verbindungen kann eine andere Portnummer erforderlich sein.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="595"/>
      <source>Connect via a secure protocol</source>
      <translation>Verbinden über ein sicheres Protokoll</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="598"/>
      <source>Make Mudlet use a secure SSL/TLS protocol instead of an unencrypted one</source>
      <translation>Mudlet soll ein sicheres SSL/TLS-Protokoll anstelle eines unverschlüsselten Protokolls verwenden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="604"/>
      <source>Secure:</source>
      <translation>Sicher:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="611"/>
      <source>Profile history:</source>
      <translation>Profilverlauf:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="634"/>
      <source>load newest profile</source>
      <translation>das neueste Profil laden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="639"/>
      <source>load oldest profile</source>
      <translation>das älteste Profil laden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="662"/>
      <source>Optional</source>
      <translation>Optional</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="678"/>
      <source>Character name:</source>
      <translation>Charaktername:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="688"/>
      <source>The characters name</source>
      <translation>Der Name des Charakters</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="691"/>
      <source>Character name</source>
      <translation>Charaktername</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="694"/>
      <source>If provided will be sent, along with password to identify the user in the game.</source>
      <translation>Wird falls vorhanden zusammen mit dem Passwort zur Identifizierung des Benutzers im Spiel verschickt.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="713"/>
      <source>Auto-open profile</source>
      <translation>Profil automatisch öffnen</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="716"/>
      <source>Automatically start this profile when Mudlet is run</source>
      <translation>Dieses Profil automatisch starten, wenn Mudlet ausgeführt wird</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="729"/>
      <source>Auto-reconnect</source>
      <translation>Automatische Wiederverbindung</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="732"/>
      <source>Automatically reconnect this profile if it should become disconnected for any reason other than the user disconnecting from the game server.</source>
      <translation>Verbinde dieses Profil automatisch wieder zum Spiel, außer wenn du die Verbindung absichtlich getrennt hast.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="748"/>
      <source>Password</source>
      <translation>Passwort</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="751"/>
      <source>If provided will be sent, along with the character name to identify the user in the game.</source>
      <translation>Wird, falls vorhanden, zusammen mit dem Charakternamen zur Identifizierung des Benutzers im Spiel verschickt.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="764"/>
      <source>Enable Discord integration (not supported by game)</source>
      <translation>Discord Integration aktivieren (nicht unterstützt durch dieses Spiel)</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="770"/>
      <source>Allow this profile to use Mudlet&apos;s Discord &quot;Rich Presence&quot;  features</source>
      <translation>Diesem Profil erlauben, Mudlet&apos;s Discord &quot;Rich Presence&quot;  Funktionen zu verwenden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="876"/>
      <location filename="../src/ui/connection_profiles.ui" line="879"/>
      <source>Game description or your notes</source>
      <translation>Spielbeschreibung oder Notizen</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="668"/>
      <source>Password:</source>
      <translation>Passwort:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="745"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation>Das Passwort des Charakters. Beachte, dass das Kennwort nicht verschlüsselt gespeichert wird</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="710"/>
      <source>With this enabled, Mudlet will automatically start and connect on this profile when it is launched</source>
      <translation>Wenn dies aktiviert ist, wird sich Mudlet automatisch beim Start mit diesem Profil verbinden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="722"/>
      <source>Open profile on Mudlet start</source>
      <translation>Profil beim Start von Mudlet öffnen</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="738"/>
      <source>Reconnect automatically</source>
      <translation>Automatisch neu verbinden</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="767"/>
      <source>Discord integration</source>
      <translation>Discord-Integration</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="805"/>
      <source>Informational</source>
      <translation>Informationen</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="832"/>
      <source>Website:</source>
      <translation>Webseite:</translation>
    </message>
  </context>
  <context>
    <name>custom_line_properties</name>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="27"/>
      <source>Custom Line Properties [*]</source>
      <translation>Eigenschaften benutzerdefinierter Linien [*]</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="46"/>
      <source>Line Settings:</source>
      <translation>Einstellungen der Linie:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="84"/>
      <source>Color:</source>
      <translation>Farbe:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="58"/>
      <source>Style:</source>
      <translation>Design:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="43"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wählen Sie den Stil, die Farbe und ob die Linie mit einem Pfeil enden soll.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="122"/>
      <source>Ends with an arrow:</source>
      <translation>Endet mit einem Pfeil:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="138"/>
      <source>Exit Details:</source>
      <translation>Details des Ausgang:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="153"/>
      <source>Origin:</source>
      <translation>Start:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="226"/>
      <source>Destination:</source>
      <translation>Ziel:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="188"/>
      <source>    Direction/Command:</source>
      <translation>    Richtung/Befehl:</translation>
    </message>
  </context>
  <context>
    <name>custom_lines</name>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="14"/>
      <source>Custom Line selection</source>
      <translation>Benutzerdefinierte Linie wählen</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="44"/>
      <source>Choose line format, color and arrow option and then select the exit to start drawing</source>
      <translation>Wähle Farbe und Format der Linie, die Pfeilspitze und dann den Ausgang, um mit dem Zeichnen zu beginnen.</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="63"/>
      <source>Line Settings:</source>
      <translation>Einstellungen der Linie:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="97"/>
      <source>Ends with an arrow:</source>
      <translation>Endet mit einem Pfeil:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="110"/>
      <source>Style:</source>
      <translation>Design:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="126"/>
      <source>Color:</source>
      <translation>Farbe:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="47"/>
      <source>&lt;p&gt;Selecting an exit immediately proceeds to drawing the first line segment from the centre point of the room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Einen Ausgang auswählen führt unmittelbar dazu, dass das erste Liniensegment ab der Mitte des Raums gezeichnet wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="60"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head BEFORE then choosing the exit to draw the line for...&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle ZUERST Design und Farbe der Linie, und ob das Ende eine Pfeilspitze haben soll, dann wähle den Ausgang zu dem du die Linie zeichnen willst...&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="187"/>
      <source>&lt;p&gt;Select a normal exit to commence drawing a line for it, buttons are shown depressed if they already have such a custom line and disabled if there is not exit in that direction.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen normalen Ausgang, um das Zeichnen einer Linie dorthin zu beginnen. Die Buttons sehen gedrückt aus, wenn es sie schon so eine benutzerdefinierte Linie haben, und sind deaktiviert, wenn es keinen Ausgang in die Richtung gibt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="190"/>
      <source>Normal Exits:</source>
      <translation>Normale Ausgänge:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="212"/>
      <source>NW</source>
      <translation>NW</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="235"/>
      <source>N</source>
      <translation>N</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="258"/>
      <source>NE</source>
      <translation>NO</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="297"/>
      <source>UP</source>
      <translation>OBEN</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="336"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="346"/>
      <source>E</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="356"/>
      <source>IN</source>
      <translation>REIN</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="382"/>
      <source>OUT</source>
      <translation>RAUS</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="392"/>
      <source>SW</source>
      <translation>SW</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="402"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="412"/>
      <source>SE</source>
      <translation>SO</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="422"/>
      <source>DOWN</source>
      <translation>UNTEN</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="444"/>
      <source>&lt;p&gt;Select a special exit to commence drawing a line for it, the first column is checked if the exit already has such a custom line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen besonderen Ausgang, um das Zeichnen einer Linie dorthin zu beginnen. Die erste Spalte ist markiert, wenn der Ausgang schon so eine benutzerdefinierte Linie hat.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="503"/>
      <source>&lt;p&gt;Indicates if there is already a custom line for this special exit, will be replaced if the exit is selected.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Gibt an, ob es schon eine benutzerdefinierte für diesen besonderen Ausgang gibt. Dieser wird ersetzt, wenn der Ausgang gewählt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="514"/>
      <source>&lt;p&gt;The room this special exit leads to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der Raum, zu dem dieser spezielle Ausgang führt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="525"/>
      <source>&lt;p&gt;The command or LUA script that goes to the given room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der Befehl oder das LUA Skript, das in den gegebenen Raum führt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="561"/>
      <source>&lt;p&gt;To remove a custom line: cancel this dialog, select the line and right-click to obtain a &amp;quot;delete&amp;quot; option.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Um eine benutzerdefinierte Linie zu entfernen, breche diesen Dialog ab,
wähle die Linie und rechtsklicke für die Möglichkeit zu &quot;löschen&quot;.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="447"/>
      <source>Special Exits:</source>
      <translation>Besonderer Ausgang:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="499"/>
      <source>Has
custom line?</source>
      <translation>Hat benutzer-
definierte Linie?</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="511"/>
      <source> Destination </source>
      <translation> Ziel </translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="522"/>
      <source> Command</source>
      <translation> Befehl</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="567"/>
      <source>Cancel</source>
      <translation>Abbrechen</translation>
    </message>
  </context>
  <context>
    <name>delete_profile_confirmation</name>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="14"/>
      <source>Confirm permanent profile deletion</source>
      <translation>Bestätige die permanente Löschung des Profils</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="26"/>
      <source>Are you sure that you&apos;d like to delete this profile? Everything (aliases, triggers, backups, etc) will be gone.

If you are, please type in the profile name as a confirmation:</source>
      <translation>Bist du sicher, dass du dieses Profil löschen willst? Alles (Aliase, Trigger, Sicherungen, usw) wird verschwinden.

Falls du dir sicher bist, schreibe den Profilnamen als Bestätigung:</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="54"/>
      <source>Delete</source>
      <translation>Löschen</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="61"/>
      <source>Cancel</source>
      <translation>Abbrechen</translation>
    </message>
  </context>
  <context>
    <name>dialog</name>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="39"/>
      <source>Status</source>
      <translation>Status</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="44"/>
      <source>Symbol
(Set Font)</source>
      <translation>Symbol (in
ausgewählter
Schriftart)</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="50"/>
      <source>Symbol
(All Fonts)</source>
      <translation>Symbol (in
jeder
Schriftart)</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="56"/>
      <source>Codepoints</source>
      <translation>Codepoints</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="61"/>
      <source>Usage
Count</source>
      <translation>Anzahl der
Nutzungen</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="67"/>
      <source>Rooms</source>
      <translation>Räume</translation>
    </message>
  </context>
  <context>
    <name>directions</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13937"/>
      <source>north</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>norden</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13939"/>
      <source>n</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13941"/>
      <source>east</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>osten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13943"/>
      <source>e</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13945"/>
      <source>south</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>süden</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13947"/>
      <source>s</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13949"/>
      <source>west</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>westen</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13951"/>
      <source>w</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>w</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13953"/>
      <source>northeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>nordosten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13955"/>
      <source>ne</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>no</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13957"/>
      <source>southeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>südosten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13959"/>
      <source>se</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>so</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13961"/>
      <source>southwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>südwesten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13963"/>
      <source>sw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>sw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13965"/>
      <source>northwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>nordwesten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13967"/>
      <source>nw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>nw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13969"/>
      <source>in</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>rein</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13971"/>
      <source>i</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>rein</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13973"/>
      <source>out</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>raus</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13975"/>
      <source>o</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>raus</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13977"/>
      <source>up</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>oben</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13979"/>
      <source>u</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>ob</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13981"/>
      <source>down</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>unten</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13983"/>
      <source>d</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>u</translation>
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
      <translation>&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Homepage&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://www.mudlet.org/&quot;&gt;www.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Forum&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://forums.mudlet.org/&quot;&gt;forums.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Dokumentation&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://wiki.mudlet.org/w/Main_Page&quot;&gt;wiki.mudlet.org/w/Main_Page&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#7289DA;&quot;&gt;&lt;b&gt;Discord Chat&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://www.mudlet.org/chat&quot;&gt;discord.gg&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Quellcode&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet&quot;&gt;github.com/Mudlet/Mudlet&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Ideen/Fehler&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/issues&quot;&gt;github.com/Mudlet/Mudlet/issues&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="155"/>
      <source>Original author, original project lead, Mudlet core coding, retired.</source>
      <comment>about:Heiko</comment>
      <translation>Ursprünglicher Autor und Projektleiter, Mudlet Kernprogrammierung, im Ruhestand.</translation>
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
      <source>Worked wonders in rejuventating our Website in 2017 but who prefers a little anonymity - if you are a &lt;i&gt;SpamBot&lt;/i&gt; you will not get onto our Fora now. They have also made some useful C++ core code contributions and we look forward to future reviews on and work in that area.</source>
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
      <comment>For non-english language versions please append a translation of the following to explain why the GPL is NOT reproduced in the relevent language: &apos;but only the English form is considered the official version of the license, so the following is reproduced in that language:&apos; to replace &apos;which is reproduced below:&apos;...</comment>
      <translation>&lt;p&gt;Mudlet wurde ursprünglich geschrieben von Heiko Köhn, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet wird bereitgestellt unter GPL Lizenz Version 2, aber nur die englische Form dieser Lizenz wird als offizielle Form angesehen, daher wird das Folgende in dieser Sprache wiedergegeben:&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="590"/>
      <source>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; is built upon the shoulders of other projects in the FOSS world; as well as using many GPL components we also make use of some third-party software with other licenses:&lt;/p&gt;</source>
      <translation>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; wurde auf den Schultern von anderen Projekten der FOSS Welt aufgebaut; Einerseits werden viele GPL-Komponenten benutzt, andererseits auch einige Software von Dritten mit anderen Lizenzen:&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="832"/>
      <source>&lt;h2&gt;&lt;u&gt;Communi IRC Library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008-2020 The Communi Project&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Communi IRC Library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008-2020 The Communi Project&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="835"/>
      <source>&lt;p&gt;Parts of &lt;tt&gt;irctextformat.cpp&lt;/t&gt; code come from Konversation and are copyrighted to:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Teile des &lt;tt&gt;irctextformat.cpp&lt;/t&gt; Codes stammen aus Konversation und sind geschützt durch:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="841"/>
      <source>&lt;h2&gt;&lt;u&gt;lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="844"/>
      <source>&lt;h2&gt;&lt;u&gt;lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Autor: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="849"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Autor: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="854"/>
      <source>&lt;h2&gt;&lt;u&gt;edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="857"/>
      <source>The &lt;b&gt;edbee-lib&lt;/b&gt; widget itself incorporates other components with licences that must be noted as well, they are:</source>
      <translation>Das &lt;b&gt;edbee-lib&lt;/b&gt; Widget selbst beinhaltet andere Komponenten, deren Lizenzen ebenfalls vermerkt werden müssen, nämlich:</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="859"/>
      <source>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="864"/>
      <source>&lt;h2&gt;&lt;u&gt;Oniguruma LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Oniguruma LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="868"/>
      <source>&lt;h2&gt;&lt;u&gt;Ruby BSDL&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1993-2013 Yukihiro Matsumoto.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Ruby BSDL&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1993-2013 Yukihiro Matsumoto.&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="872"/>
      <source>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [broken link]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archived version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [Link defekt]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archivierte Version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="879"/>
      <source>&lt;h2&gt;&lt;u&gt;dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="882"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="892"/>
      <source>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="900"/>
      <source>&lt;h2&gt;&lt;u&gt;sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim Peretokin&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim Peretokin&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="906"/>
      <source>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="909"/>
      <source>&lt;h2&gt;&lt;u&gt;QtKeyChain - Platform-independent Qt API for storing passwords securely&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1057"/>
      <source>
                          These formidable folks will be fondly remembered forever&lt;br&gt;for their generous financial support on &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s patreon&lt;/a&gt;:
                          </source>
      <translation>
                          Diese formidablen Leute bleiben für immer in liebevoller Erinnerung&lt;br&gt;für ihre großzügige finanzielle Unterstützung an &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s Patreon&lt;/a&gt;:
                          </translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1068"/>
      <source>Technical information:</source>
      <translation>Technische Informationen:</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1069"/>
      <source>Version</source>
      <translation>Version</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1070"/>
      <source>OS</source>
      <translation>Betriebssystem</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1071"/>
      <source>CPU</source>
      <translation>Prozessor</translation>
    </message>
  </context>
  <context>
    <name>dlgColorTrigger</name>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="52"/>
      <source>More colors</source>
      <translation>Weitere Farben</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="54"/>
      <source>Click to access all 256 ANSI colors.</source>
      <translation>Klicke für Zugriff auf alle 256 ANSI-Farben.</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="59"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s background color - however chosing this for both this and the foreground is an error.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke, damit der Farbtrigger die Hintergrundfarbe des Textes ignoriert - allerdings wäre es ein Fehler, die sowohl für Vordergrund- und Hintergrundfarbe auszuwählen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="60"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s foreground color - however chosing this for both this and the background is an error.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke, damit der Farbtrigger die Vordergrundfarbe des Textes ignoriert - allerdings wäre es ein Fehler, die sowohl für Vordergrund- und Hintergrundfarbe auszuwählen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="64"/>
      <source>Default</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="66"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s background color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke hier, um den Trigger auszulösen, wenn die Hintergrundfarbe des Textes nicht vom Normalwert geändert wurde.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="67"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s foreground color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke hier, um den Trigger auszulösen, wenn die Vordergrundfarbe des Textes nicht vom Normalwert geändert wurde.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="71"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s background color matches the color number indicated.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle eine Farbe, um den Trigger nur dann auszulösen, wenn die Hintergrundfarbe des Textes mit der angegebenen Farbnummer übereinstimmt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="72"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s foreground color matches the color number indicated.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle eine Farbe, um den Trigger nur dann auszulösen, wenn die Vordergrundfarbe des Textes mit der angegebenen Farbnummer übereinstimmt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="77"/>
      <source>Black</source>
      <translation>Schwarz</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="78"/>
      <source>Red</source>
      <translation>Rot</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="79"/>
      <source>Green</source>
      <translation>Grün</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="80"/>
      <source>Yellow</source>
      <translation>Gelb</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="81"/>
      <source>Blue</source>
      <translation>Blau</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="82"/>
      <source>Magenta</source>
      <translation>Magenta</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="83"/>
      <source>Cyan</source>
      <translation>Cyan</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="84"/>
      <source>White (Light gray)</source>
      <translation>Weiß (Hellgrau)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="86"/>
      <source>Light black (Dark gray)</source>
      <translation>Hellschwarz (Dunkelgrau)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="87"/>
      <source>Light red</source>
      <translation>Helles Rot</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="88"/>
      <source>Light green</source>
      <translation>Helles Grün</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="89"/>
      <source>Light yellow</source>
      <translation>Helles Gelb</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="90"/>
      <source>Light blue</source>
      <translation>Helles Blau</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="91"/>
      <source>Light magenta</source>
      <translation>Helles Magenta</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="92"/>
      <source>Light cyan</source>
      <translation>Helles Cyan</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="93"/>
      <source>Light white</source>
      <translation>Helles Weiß</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="218"/>
      <source>%1 [%2]</source>
      <comment>Color Trigger dialog button in basic 16-color set, the first value is the name of the color, the second is the ANSI color number - for most languages modification is not likely to be needed - this text is used in two places</comment>
      <translation>%1 [%2]</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="405"/>
      <source>All color options are showing.</source>
      <translation>Alle Farboptionen werden angezeigt.</translation>
    </message>
  </context>
  <context>
    <name>dlgConnectionProfiles</name>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="110"/>
      <source>Connect</source>
      <translation>Verbinden</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="267"/>
      <source>Game name: %1</source>
      <translation>Name des Spiels: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1239"/>
      <source>This profile is currently loaded - close it before changing the connection parameters.</source>
      <translation>Dieses Profil ist momentan geladen - Beende es, bevor die Verbindungsparameter geändert werden.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2050"/>
      <source>The %1 character is not permitted. Use one of the following:</source>
      <translation>Das Zeichen %1 ist nicht erlaubt. Verwende eines der Folgenden:</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2080"/>
      <source>You have to enter a number. Other characters are not permitted.</source>
      <translation>Du musst eine Zahl eingeben. Andere Zeichen sind nicht erlaubt.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2066"/>
      <source>This profile name is already in use.</source>
      <translation>Dieser Profilname wird bereits verwendet.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="523"/>
      <source>Could not rename your profile data on the computer.</source>
      <translation>Konnte deine Profil-Daten auf dem Computer nicht umbenennen.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="122"/>
      <source>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Welcome to Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Click on one of the games on the list to play.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;To play a game not in the list, click on %1 &lt;span style=&quot; color:#555753;&quot;&gt;New&lt;/span&gt;, fill in the &lt;i&gt;Profile Name&lt;/i&gt;, &lt;i&gt;Server address&lt;/i&gt;, and &lt;i&gt;Port&lt;/i&gt; fields in the &lt;i&gt;Required &lt;/i&gt; area.&lt;/p&gt;&lt;p&gt;After that, click %2 &lt;span style=&quot; color:#555753;&quot;&gt;Connect&lt;/span&gt; to play.&lt;/p&gt;&lt;p&gt;Have fun!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;The Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</source>
      <comment>Welcome message. Both %1 and %2 may be replaced by icons when this text is used.</comment>
      <translation>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Willkommen bei Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Klicke zum Spielen auf eines der Spiele aus der Liste.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;Um ein Spiel zu spielen, das nicht aufgelistet ist, klicke auf %1 &lt;span style=&quot; color:#555753;&quot;&gt;Neu&lt;/span&gt;, fülle die Felder &lt;i&gt;Profilname&lt;/i&gt;, &lt;i&gt;Serveradresse&lt;/i&gt;, und &lt;i&gt;Port&lt;/i&gt; unter &lt;i&gt;Benötigt&lt;/i&gt; aus.&lt;/p&gt;&lt;p&gt;Klicke danach auf %2 &lt;span style=&quot; color:#555753;&quot;&gt;Verbinden&lt;/span&gt; zum Spielen.&lt;/p&gt;&lt;p&gt;Viel Spaß!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;Das Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="112"/>
      <source>Offline</source>
      <translation>Offline</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="135"/>
      <source>Copy</source>
      <translation>Kopieren</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="137"/>
      <source>Copy settings only</source>
      <translation>Nur Einstellungen kopieren</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="145"/>
      <source>copy profile</source>
      <translation>Profil kopieren</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="146"/>
      <source>copy the entire profile to new one that will require a different new name.</source>
      <translation>Kopiere das gesamte Profil in ein neues, das einen anderen neuen Namen benötigt.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="150"/>
      <source>copy profile settings</source>
      <translation>Profileinstellungen kopieren</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="151"/>
      <source>copy the settings and some other parts of the profile to a new one that will require a different new name.</source>
      <translation>Kopiere die Einstellungen und einige andere Teile des Profils in ein neues Profil, das einen anderen neuen Namen benötigt.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="215"/>
      <source>Characters password, stored securely in the computer&apos;s credential manager</source>
      <translation>Kennwort des Charakters, sicher im Anmeldeinformations-Manager des Computers gespeichert</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="217"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation>Das Passwort des Charakters. Beachte, dass das Kennwort nicht verschlüsselt gespeichert wird</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="264"/>
      <source>Click to load but not connect the selected profile.</source>
      <translation>Das ausgewählte Profil laden, aber nicht verbinden.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="265"/>
      <source>Click to load and connect the selected profile.</source>
      <translation>Das ausgewählte Profil laden und verbinden.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="266"/>
      <source>Need to have a valid profile name, game server address and port before this button can be enabled.</source>
      <translation>Du benötigst einen gültigen Profilnamen, eine Spielserveradresse und einen Port, bevor diese Schaltfläche aktiviert werden kann.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="268"/>
      <source>Button to select a mud game to play, double-click it to connect and start playing it.</source>
      <comment>Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option</comment>
      <translation>Schaltfläche, um ein Spiel zum Spielen auszuwählen. Klicke doppelt darauf, um eine Verbindung herzustellen und das Spiel zu beginnen.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="531"/>
      <source>Could not create the new profile folder on your computer.</source>
      <translation>Konnte den neuen Profil-Ordner auf dem Computer nicht erstellen.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="567"/>
      <source>new profile name</source>
      <translation>neuer Profilname</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="680"/>
      <source>Deleting &apos;%1&apos;</source>
      <translation>Lösche &apos;%1&apos;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1286"/>
      <source>Discord integration not available on this platform</source>
      <translation>Discord Integration ist auf dieser Plattform nicht verfügbar</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1292"/>
      <source>Discord integration not supported by game</source>
      <translation>Discord Integration wird durch dieses Spiel nicht unterstützt</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1295"/>
      <source>Check to enable Discord integration</source>
      <translation>Aktiviere die Discord Integration</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1651"/>
      <source>Reset icon</source>
      <comment>Reset the custom picture for this profile in the connection dialog and show the default one instead</comment>
      <translation>Bild zurücksetzen</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1653"/>
      <source>Set custom icon</source>
      <comment>Set a custom picture to show for the profile in the connection dialog</comment>
      <translation>Neues Bild wählen</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1663"/>
      <source>Select custom image for profile (should be 120x30)</source>
      <translation>Wählen ein neues Bild für das Profil (sollte 120x30 sein)</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1665"/>
      <source>Images (%1)</source>
      <translation>Bilder (%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2093"/>
      <source>Port number must be above zero and below 65535.</source>
      <translation>Die Portnummer muss größer als null sein und unter 65535.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2119"/>
      <source>Mudlet can not load support for secure connections.</source>
      <translation>Mudlet kann keine Unterstützung für sichere Verbindungen laden.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2143"/>
      <source>Please enter the URL or IP address of the Game server.</source>
      <translation>Bitte gib die URL oder IP-Adresse des Spielservers ein.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2156"/>
      <source>SSL connections require the URL of the Game server.</source>
      <translation>SSL-Verbindungen erfordern die URL des Spielservers.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2174"/>
      <source>&lt;p&gt;Load profile without connecting.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Profil laden ohne zu verbinden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2190"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before loading.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Bitte stelle vor dem Laden einen gültigen Profilnamen ein, sowie Adresse und Port des Spielservers.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2195"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before connecting.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Bitte stelle vor dem Verbinden einen gültigen Profilnamen ein, sowie Adresse und Port des Spielservers.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2248"/>
      <source>&lt;p&gt;Click to hide the password; it will also hide if another profile is selected.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke hier, um das Kennwort auszublenden. Es wird auch ausgeblendet, wenn ein anderes Profil ausgewählt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2252"/>
      <source>&lt;p&gt;Click to reveal the password for this profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke hier, um das Kennwort für dieses Profil einzublenden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2101"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2107"/>
      <source>Mudlet is not configured for secure connections.</source>
      <translation>Mudlet ist nicht für sichere Verbindungen konfiguriert.</translation>
    </message>
  </context>
  <context>
    <name>dlgIRC</name>
    <message>
      <location filename="../src/dlgIRC.cpp" line="119"/>
      <source>%1 closed their client.</source>
      <translation>%1 hat den Client geschlossen.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="131"/>
      <source>Mudlet IRC Client - %1 - %2 on %3</source>
      <translation>Mudlet IRC-Client - %1 - %2 auf %3</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="145"/>
      <source>$ Starting Mudlet IRC Client...</source>
      <translation>$ Starte Mudlet IRC Client...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="146"/>
      <source>$ Host: %1:%2</source>
      <translation>$ Server: %1:%2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="147"/>
      <source>$ Nick: %1</source>
      <translation>$ Spitzname: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="148"/>
      <source>$ Auto-Join Channels: %1</source>
      <translation>Automatisch betretene Kanäle: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="149"/>
      <source>$ This client supports Auto-Completion using the Tab key.</source>
      <translation>$ Dieser Client unterstützt Auto-Vervollständigung per Tab-Taste.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="150"/>
      <source>$ Type &lt;b&gt;/help&lt;/b&gt; for commands or &lt;b&gt;/help [command]&lt;/b&gt; for command syntax.</source>
      <translation>$ Tippe &lt;b&gt;/help&lt;/b&gt; für alle Befehle oder &lt;b&gt;/help [Befehl]&lt;/b&gt; für die Syntax eines Befehls.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="209"/>
      <source>Restarting IRC Client</source>
      <translation>Starte IRC Client neu</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="382"/>
      <source>[Error] MSGLIMIT requires &lt;limit&gt; to be a whole number greater than zero!</source>
      <translation>[Error] MSGLIMIT erfordert, dass &lt;limit&gt; eine ganze Zahl und größer als Null ist!</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="412"/>
      <source>[HELP] Available Commands: %1</source>
      <translation>[HILFE] Verfügbare Befehle: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="414"/>
      <source>[HELP] Syntax: %1</source>
      <translation>[HILFE] Syntax: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="422"/>
      <source>! Connected to %1.</source>
      <translation>! Verbunden mit %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="423"/>
      <source>! Joining %1...</source>
      <translation>! Betrete %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="428"/>
      <source>! Connecting %1...</source>
      <translation>! Verbinde mit %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="433"/>
      <source>! Disconnected from %1.</source>
      <translation>! Getrennt von %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="499"/>
      <source>[ERROR] Syntax: %1</source>
      <translation>[FEHLER] Syntax: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="501"/>
      <source>[ERROR] Unknown command: %1</source>
      <translation>[FEHLER] Unbekannter Befehl: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="647"/>
      <source>! The Nickname %1 is reserved. Automatically changing Nickname to: %2</source>
      <translation>! Der Spitzname %1 ist reserviert. Wechsle Spitznamen automatisch zu: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="658"/>
      <source>Your nick has changed.</source>
      <translation>Dein Spitzname hat sich geändert.</translation>
    </message>
  </context>
  <context>
    <name>dlgMapper</name>
    <message>
      <location filename="../src/dlgMapper.cpp" line="347"/>
      <source>None</source>
      <comment>Don&apos;t show the map overlay, &apos;none&apos; meaning no map overlay styled are enabled</comment>
      <translation>Ohne</translation>
    </message>
  </context>
  <context>
    <name>dlgModuleManager</name>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="48"/>
      <source>Module Manager - %1</source>
      <translation>Module - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Name</source>
      <translation>Modulname</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Priority</source>
      <translation>Priorität</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Sync</source>
      <translation>Synchronisieren</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Location</source>
      <translation>Ort des Moduls</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="105"/>
      <source>Checking this box will cause the module to be saved and &lt;i&gt;resynchronised&lt;/i&gt; across all sessions that share it when the &lt;i&gt;Save Profile&lt;/i&gt; button is clicked in the Editor or if it is saved at the end of the session.</source>
      <translation>Wenn diese Box aktiviert ist, wird das Modul gespeichert und über alle Sitzungen hinweg &lt;bi&gt;neu synchronisiert&lt;b/i&gt;, die es teilen, sobald die &lt;bi&gt;Profil speichern&lt;b/i&gt; Taste im Editor angeklickt wird, oder wenn es am Ende der Sitzung gespeichert wird.</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="111"/>
      <source>&lt;b&gt;Note:&lt;/b&gt; &lt;i&gt;.zip&lt;/i&gt; and &lt;i&gt;.mpackage&lt;/i&gt; modules are currently unable to be synced&lt;br&gt; only &lt;i&gt;.xml&lt;/i&gt; packages are able to be synchronized across profiles at the moment. </source>
      <translation>&lt;b&gt;Hinweis:&lt;/b&gt; &lt;i&gt;.zip&lt;/i&gt; und &lt;i&gt;.mpackage&lt;/i&gt; Module können derzeit nicht synchronisiert werden.&lt;br&gt;Nur &lt;i&gt;.xml&lt;/i&gt; Pakete können derzeit Profil-übergreifend synchronisiert werden. </translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="149"/>
      <source>Load Mudlet Module</source>
      <translation>Lade Mudlet Modul</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Load Mudlet Module:</source>
      <translation>Lade Mudlet Modul:</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>Kann Datei %1 nicht lesen:
%2.</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageExporter</name>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="20"/>
      <source>Package Exporter</source>
      <translation>Paket exportieren</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="28"/>
      <source>Package name here</source>
      <translation>Paketname hier</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="35"/>
      <source>or</source>
      <translation>oder</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="54"/>
      <location filename="../src/dlgPackageExporter.cpp" line="1254"/>
      <source>Select what to export</source>
      <translation>Wähle, was exportiert wird</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="64"/>
      <source>Check items to export</source>
      <translation>Wähle Elemente für den Export</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="93"/>
      <source>(optional) add icon, description, and more</source>
      <translation>(optional) Symbol, Beschreibung, usw. ergänzen</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="118"/>
      <source>Author</source>
      <translation>Autor</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="137"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="209"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="235"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="258"/>
      <source>optional</source>
      <translation>optional</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="144"/>
      <source>Icon</source>
      <translation>Symbol</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="162"/>
      <source>Add icon</source>
      <translation>Symbol hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="196"/>
      <source>Short description</source>
      <translation>Kurze Beschreibung</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="222"/>
      <source>Description</source>
      <translation>Beschreibung</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="248"/>
      <source>Version</source>
      <translation>Version</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="271"/>
      <source>Does this package make use of other packages? List them here as requirements</source>
      <translation>Verwendet dieses Paket andere Pakete? Liste sie hier als Voraussetzung auf</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="274"/>
      <source>Required packages</source>
      <translation>Benötigte Pakete</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="286"/>
      <source>Include assets (images, sounds, fonts)</source>
      <translation>Medien hinzufügen (Bilder, Töne, Schriftarten)</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="302"/>
      <source>Drag and drop files and folders, or use the browse button below</source>
      <translation>Füge hier Dateien und Ordner per Drag &amp; Drop ein, oder benutze die Schaltfläche unten</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="340"/>
      <source>Select files to include in package</source>
      <translation>Wähle Dateien, die in das Paket aufgenommen werden sollen</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="397"/>
      <source>+</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="410"/>
      <source>Does this package make use of other packages? List them here as requirements. Press &apos;Delete&apos; to remove a package</source>
      <translation>Verwendet dieses Paket andere Pakete? Liste sie hier als Voraussetzung auf. Drücke &apos;Löschen&apos;, um ein Paket zu entfernen</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="438"/>
      <source>Select export location</source>
      <translation>Wähle das Ziel des Exports</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="61"/>
      <source>Triggers</source>
      <translation>Trigger</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="62"/>
      <source>Aliases</source>
      <translation>Aliase</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="63"/>
      <source>Timers</source>
      <translation>Timers</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="64"/>
      <source>Scripts</source>
      <translation>Skripte</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="65"/>
      <source>Keys</source>
      <translation>Tasten</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="66"/>
      <source>Buttons</source>
      <translation>Buttons</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="84"/>
      <source>Export</source>
      <comment>Text for button to perform the package export on the items the user has selected.</comment>
      <translation>Exportieren</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="105"/>
      <source>update installed package</source>
      <translation>aktualisiere ein installiertes Paket</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="106"/>
      <source>add dependencies</source>
      <translation>wähle Abhängigkeit</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="151"/>
      <source>Failed to open file &quot;%1&quot; to place into package. Error message was: &quot;%2&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but the code cannot open it.</comment>
      <translation>Fehler beim Öffnen der Datei &quot;%1&quot;, die in das Paket eingefügt werden soll. Fehlernachricht: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="159"/>
      <source>Failed to add file &quot;%1&quot; to package &quot;%2&quot;. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but cannot be done for some reason.</comment>
      <translation>Fehler beim Hinzufügen der Datei &quot;%1&quot; in das Paket &quot;%2&quot;. Fehlernachricht: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="300"/>
      <source>Export to %1</source>
      <translation>Exportiere nach %1</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="304"/>
      <source>Export to %1/%2.mpackage</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="319"/>
      <source>Open Icon</source>
      <translation>Symbol öffnen</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="319"/>
      <source>Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</source>
      <translation>Bilddateien (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="429"/>
      <source>Please enter the package name.</source>
      <translation>Bitte einen Paketnamen eingeben.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="514"/>
      <source>Failed to export. Could not open the folder &quot;%1&quot; for writing in. - Do you have the necessary permissions and free disk-space to write to that folder?</source>
      <translation>Fehler beim Exportieren. Der Ordner &quot;%1&quot; konnte nicht zum Schreiben geöffnet werden. - Hast du die notwendigen Berechtigungen und freien Speicherplatz, um in diesen Ordner zu schreiben?</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="595"/>
      <source>Failed to export. Could not write Mudlet items to the file &quot;%1&quot;.</source>
      <comment>This error message is shown when all the Mudlet items cannot be written to the &apos;packageName&apos;.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened.</comment>
      <translation>Fehler beim Exportieren. Mudlet-Elemente konnten nicht in die Datei &quot;%1&quot; geschrieben werden.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="678"/>
      <source>Failed to open package file. Error is: &quot;%1&quot;.</source>
      <comment>This error message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer&apos;s file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.</comment>
      <translation>Fehler beim Öffnen der Paketdatei. Fehlermeldung: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="772"/>
      <source>Failed to add directory &quot;%1&quot; to package. Error is: &quot;%2&quot;.</source>
      <translation>Fehler beim Hinzufügen des Verzeichnisses &quot;%1&quot; zum Paket. Fehlermeldung: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="828"/>
      <source>Required file &quot;%1&quot; was not found in the staging area. This area contains the Mudlet items chosen for the package, which you selected to be included in the package file. This suggests there may be a problem with that directory: &quot;%2&quot; - Do you have the necessary permissions and free disk-space?</source>
      <translation>Die erforderliche Datei &quot;%1&quot; wurde im Bereitstellungsbereich nicht gefunden. Dieser Bereich enthält die Mudlet-Elemente, die du für die Aufnahme in das Paket ausgewählt hast. Dies deutet darauf hin, dass es ein Problem mit diesem Verzeichnis geben könnte: &quot;%2&quot; - Hast du die notwendigen Berechtigungen und freien Speicherplatz?</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="851"/>
      <source>Failed to write files into and then close the package. Error is: &quot;%1&quot;.</source>
      <comment>This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...</comment>
      <translation>Fehler beim Schreiben von Dateien und anschließenden Schließen des Pakets. Fehlermeldung: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="871"/>
      <source>Package &quot;%1&quot; exported to: %2</source>
      <translation>Paket &quot;%1&quot; exportiert nach: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1236"/>
      <source>Why not &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;upload&lt;/a&gt; your package for other Mudlet users?</source>
      <comment>Only the text outside of the &apos;a&apos; (HTML anchor) tags PLUS the verb &apos;upload&apos; in between them in the source text, (associated with uploading the resulting package to the Mudlet forums) should be translated.</comment>
      <translation>Warum nicht dein Paket für andere Mudlet-Benutzer &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;hochladen&lt;/a&gt;?</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageExporter.cpp" line="1256"/>
      <source>Select what to export (%1 items)</source>
      <comment>Package exporter selection</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="920"/>
      <source>Where do you want to save the package?</source>
      <translation>Wo möchtest du das Paket speichern?</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageManager</name>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="42"/>
      <source>Package Manager - %1</source>
      <translation>Pakete - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="54"/>
      <source>Import Mudlet Package</source>
      <translation>Importiere Mudlet Paket</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="61"/>
      <source>Import Mudlet Package:</source>
      <translation>Mudlet Paket importieren:</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="61"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>Kann Datei %1 nicht lesen:
%2.</translation>
    </message>
  </context>
  <context>
    <name>dlgProfilePreferences</name>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="142"/>
      <source>&lt;p&gt;Location which will be used to store log files - matching logs will be appended to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Standort, der zum Speichern von Log-Dateien verwendet wird - übereinstimmende Logs werden angehängt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="150"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {1 of 2}).</comment>
      <translation>Protokolldatei</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="196"/>
      <source>&lt;p&gt;This will bring up a display showing all the symbols used in the current map and whether they can be drawn using just the specifed font, any other font, or not at all.  It also shows the sequence of Unicode &lt;i&gt;code-points&lt;/i&gt; that make up that symbol, so that they can be identified even if they cannot be displayed; also, up to the first thirty two rooms that are using that symbol are listed, which may help to identify any unexpected or odd cases.&lt;p&gt;</source>
      <translation>&lt;p&gt;Hiermit werden alle Symbole angezeigt, die in der aktuellen Karte genutzt werden, und ob sie nur mit der angegebenen Schriftart dargestellt werden können, mit einer anderen Schriftart, oder überhaupt nicht. Es wird auch eine Sequenz von Unicode-&lt;i&gt;Code-Punkten&lt;/i&gt; angezeigt, aus denen das Symbol zusammengesetzt wird, damit es auch identifiziert werden kann, falls es nicht dargestellt werden kann. Außerdem werden bis zu 32 Räume aufgelistet, die dieses Symbol benutzen, wodurch man unerwartete oder seltsame Fälle aufdecken kann.&lt;p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="202"/>
      <source>&lt;p&gt;Select the only or the primary font used (depending on &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; setting) to produce the 2D mapper room symbols.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die einzige oder hauptsächlich benutzte Schriftart (je nach der Einstellung &lt;i&gt;Verwende nur Symbole (Glyphen) aus der gewählten Schriftart&lt;/i&gt;), um die Symbole der Räume auf der 2D-Karte erstellen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="236"/>
      <source>&lt;p&gt;Some Desktop Environments tell Qt applications like Mudlet whether they should shown icons on menus, others, however do not. This control allows the user to override the setting, if needed, as follows:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;off&lt;/i&gt;&apos; = Prevent menus from being drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;on&lt;/i&gt;&apos; = Allow menus to be drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use the setting that the system provides.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This setting is only processed when individual menus are created and changes may not propogate everywhere until Mudlet is restarted.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="293"/>
      <source>%1 (%2% done)</source>
      <comment>%1 is the (not-translated so users of the language can read it!) language name, %2 is percentage done.</comment>
      <translation>%1 (%2% fertig)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="346"/>
      <source>Migrated all passwords to secure storage.</source>
      <translation>Alle Kennwörter wurden in den sicheren Speicher migriert.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="353"/>
      <source>Migrated %1...</source>
      <comment>This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely</comment>
      <translation>%1 migriert...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="357"/>
      <source>Migrated all passwords to profile storage.</source>
      <translation>Alle Kennwörter wurden in den Profilspeicher migriert.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="738"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (z.B., 1970-01-01#00-00-00%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="740"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (z.B., 1970-01-01T00-00-00%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="741"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)</source>
      <translation>yyyy-MM-dd (Logs eines Tages aneinander hängen, z.B. 1970-01-01%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="744"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01%1)</source>
      <translation>yyyy-MM (Logs eines Monats aneinander hängen, z.B. 1970-01%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="745"/>
      <source>Named file (concatenate logs in one file)</source>
      <translation>Benannte Datei (Logs in einer Datei aneinander hängen)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="778"/>
      <source>Other profiles to Map to:</source>
      <translation>Andere Profile für die Karte:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="818"/>
      <source>%1 {Default, recommended}</source>
      <translation>%1 {Standard, empfohlen}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="829"/>
      <source>%1 {Upgraded, experimental/testing, NOT recommended}</source>
      <translation>%1 {Neuere Version, experimentell/testweise, NICHT empfohlen}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="831"/>
      <source>%1 {Downgraded, for sharing with older version users, NOT recommended}</source>
      <translation>%1 {Ältere Version, zum Austausch mit anderen Benutzern, NICHT empfohlen}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="840"/>
      <source>2D Map Room Symbol scaling factor:</source>
      <translation>Skalierungsfaktor des Symbols für Räume der 2D Karte:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="860"/>
      <source>Show &quot;%1&quot; in the map area selection</source>
      <translation>Zeige &quot;%1&quot; in der Auswahl der Kartengebiete</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="898"/>
      <source>%1 (*Error, report to Mudlet Makers*)</source>
      <comment>The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!</comment>
      <translation>%1 (*Fehler, bitte an Mudlet-Entwickler melden*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1012"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="3769"/>
      <source>Profile preferences - %1</source>
      <translation>Profil-Einstellungen - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1288"/>
      <source>Profile preferences</source>
      <translation>Profil-Einstellungen</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2006"/>
      <source>Load Mudlet map</source>
      <translation>Mudlet Karte laden</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2021"/>
      <source>Importing map - please wait...</source>
      <translation>Karte wird importiert, bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2025"/>
      <source>Imported map from %1.</source>
      <translation>Karte importiert von %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2027"/>
      <source>Could not import map from %1.</source>
      <translation>Karte konnte nicht importiert werden von %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2030"/>
      <source>Loading map - please wait...</source>
      <translation>Karte wird geladen, bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2035"/>
      <source>Loaded map from %1.</source>
      <translation>Karte geladen von %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2037"/>
      <source>Could not load map from %1.</source>
      <translation>Karte konnte nicht geladen werden von %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2054"/>
      <source>Save Mudlet map</source>
      <translation>Mudlet Karte speichern</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2054"/>
      <source>Mudlet map (*.dat)</source>
      <comment>Do not change the extension text (in braces) - it is needed programmatically!</comment>
      <translation>Mudlet Karte (*.dat)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2064"/>
      <source>Saving map - please wait...</source>
      <translation>Karte wird gespeichert, bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2074"/>
      <source>Saved map to %1.</source>
      <translation>Karte gespeichert nach %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2076"/>
      <source>Could not save map to %1.</source>
      <translation>Konnte die Karte nicht nach %1 speichern.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2104"/>
      <source>Migrating passwords to secure storage...</source>
      <translation>Alle Kennwörter werden in den sicheren Speicher migriert...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2111"/>
      <source>Migrating passwords to profiles...</source>
      <translation>Kennwörter werden in die Profile migriert...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2143"/>
      <source>[ ERROR ] - Unable to use or create directory to store map for other profile &quot;%1&quot;.
Please check that you have permissions/access to:
&quot;%2&quot;
and there is enough space. The copying operation has failed.</source>
      <translation>[FEHLER] - Kann das Verzeichnis nicht nutzen oder erstellen, um für das andere Profil
&quot;%1&quot; die Karte zu speichern.
Prüfe bitte, dass du die Berechtigungen und Zugriff hast auf:
&quot;%2&quot;
und dass es genug Speicherplatz gibt. Die Kopie ist fehlgeschlagen.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2150"/>
      <source>Creating a destination directory failed...</source>
      <translation>Erstellen eines Ziel-Verzeichnisses ist fehlgeschlagen...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2234"/>
      <source>Backing up current map - please wait...</source>
      <translation>Sichern der aktuellen Karte - bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2244"/>
      <source>Could not backup the map - saving it failed.</source>
      <translation>Karte konnte nicht gesichert werden - Speichern schlug fehl.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2269"/>
      <source>Could not copy the map - failed to work out which map file we just saved the map as!</source>
      <translation>Konnte Karte nicht kopieren - Wusste nicht mehr, als welche Datei die Karte gerade gespeichert wurde.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2281"/>
      <source>Copying over map to %1 - please wait...</source>
      <translation>Kopiere Karte herüber nach %1 - bitte warten...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2287"/>
      <source>Could not copy the map to %1 - unable to copy the new map file over.</source>
      <translation>Konnte Karte nicht kopieren nach %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2291"/>
      <source>Map copied successfully to other profile %1.</source>
      <translation>Karte erfolgreich in das andere Profil %1 kopiert.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2303"/>
      <source>Map copied, now signalling other profiles to reload it.</source>
      <translation>Karte kopiert. Signalisiere den anderen Profilen, sie neu zu laden.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2339"/>
      <source>Where should Mudlet save log files?</source>
      <translation>Wo soll Mudlet Protokolldateien speichern?</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2724"/>
      <source>%1 selected - press to change</source>
      <translation>%1 gewählt - drücken zum ändern</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2727"/>
      <source>Press to pick destination(s)</source>
      <translation>Drücken, um Ziel(e) zu wählen</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2968"/>
      <source>Could not update themes: %1</source>
      <translation>Designs konnten nicht aktualisiert werden: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2971"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>Aktualisiere Designs von colorsublime.github.io...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3088"/>
      <source>{missing, possibly recently deleted trigger item}</source>
      <translation>(Fehlender Trigger, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3091"/>
      <source>{missing, possibly recently deleted alias item}</source>
      <translation>(Fehlendes Alias, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3094"/>
      <source>{missing, possibly recently deleted script item}</source>
      <translation>(Fehlendes Skript, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3097"/>
      <source>{missing, possibly recently deleted timer item}</source>
      <translation>(Fehlender Timer, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3100"/>
      <source>{missing, possibly recently deleted key item}</source>
      <translation>(Fehlende Taste, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3103"/>
      <source>{missing, possibly recently deleted button item}</source>
      <translation>(Fehlender Button, vermutlich zwischenzeitlich gelöscht)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3901"/>
      <source>Set outer color of player room mark.</source>
      <translation>Wähle die äußere Farbe der Markierung des Spielerraums.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3902"/>
      <source>Set inner color of player room mark.</source>
      <translation>Wähle die innere Farbe der Markierung des Spielerraums.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="204"/>
      <source>&lt;p&gt;Using a single font is likely to produce a more consistent style but may cause the &lt;i&gt;font replacement character&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; to show if the font does not have a needed glyph (a font&apos;s individual character/symbol) to represent the grapheme (what is to be represented).  Clearing this checkbox will allow the best alternative glyph from another font to be used to draw that grapheme.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mit einer einzigen Schriftart ist es wahrscheinlicher, einen einheitlicheren Stil zu erzeugen, aber das &lt;i&gt;Ersetzungszeichen&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; könnte gezeigt werden, falls die Schriftart ein benötigte Glyphe nicht hat (ein einzelnes Zeichen oder Symbol der Schriftart), um das gewünschte Graphem darzustellen. Diese Markierung zu löschen führt dazu, dass die beste alternative Glyphe einer anderen Schriftart benutzt wird, um das Graphem zu zeichnen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="143"/>
      <source>&lt;p&gt;Select a directory where logs will be saved.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle ein Verzeichnis, in dem Protokolle gespeichert werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="144"/>
      <source>&lt;p&gt;Reset the directory so that logs are saved to the profile&apos;s &lt;i&gt;log&lt;/i&gt; directory.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Setze das Verzeichnis zurück, damit Protokolle in den &lt;i&gt;log&lt;/i&gt;-Ordner des Profils gespeichert werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="145"/>
      <source>&lt;p&gt;This option sets the format of the log name.&lt;/p&gt;&lt;p&gt;If &lt;i&gt;Named file&lt;/i&gt; is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)&lt;/p&gt;</source>
      <translation>&lt;p&gt;Diese Option bestimmt das Format des Namens der Protokolldatei.&lt;/p&gt;&lt;p&gt;Wenn &lt;i&gt;Benannte Datei&lt;/i&gt; ausgewählt wird, kannst du einen eigenen Namen definieren. (Die Protokolle werden angehängt, wenn bereits eine Datei mit dem gleichen Namen existiert.)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="148"/>
      <source>&lt;p&gt;Set a custom name for your log. (New logs are appended if a log file of the same name already exists).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Bestimme einen eigenen Namens für deine Protokolldatei. (Neue Protokolle werden angehängt, wenn bereits eine Datei mit dem gleichen Namen existiert.)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="169"/>
      <source>&lt;p&gt;Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Automatische Updates sind in Entwicklungsversionen deaktiviert, um zu verhindern, dass ein Update dein Mudlet überschreibt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="190"/>
      <source>&lt;p&gt;A timer with a short interval will quickly fill up the &lt;i&gt;Central Debug Console&lt;/i&gt; windows with messages that it ran correctly on &lt;i&gt;each&lt;/i&gt; occasion it is called.  This (per profile) control adjusts a threshold that will hide those messages in just that window for those timers which run &lt;b&gt;correctly&lt;/b&gt; when the timer&apos;s interval is less than this setting.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Any timer script that has errors will still have its error messages reported whatever the setting.&lt;/u&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein Timer mit kurzem Intervall wird schnell das Fenster für die &lt;i&gt;Zentrale Debug-Konsole&lt;/i&gt; füllen, weil er bei &lt;i&gt;jedem Aufruf&lt;/i&gt; melden wird, dass er richtig gelaufen ist. Die folgende (profilspezifische) Einstellung regelt eine Schwelle, unter der diese Meldungen an das Fenster unterdrückt werden, wenn ein Timer &lt;b&gt;richtig&lt;/b&gt; gelaufen ist, und sein Intervall kleiner ist als diese Einstellung.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Wenn ein Skript eines Timers einen Fehler meldet, werden diese weiter unabhängig von dieser Einstellung erscheinen.&lt;/u&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="209"/>
      <source>&lt;p&gt;If &lt;b&gt;not&lt;/b&gt; checked Mudlet will only react to the first matching keybinding (combination of key and modifiers) even if more than one of them is set to be active. This means that a temporary keybinding (not visible in the Editor) created by a script or package may be used in preference to a permanent one that is shown and is set to be active. If checked then all matching keybindings will be run.&lt;/p&gt;&lt;p&gt;&lt;i&gt;It is recommended to not enable this option if you need to maintain compatibility with scripts or packages for Mudlet versions prior to &lt;b&gt;3.9.0&lt;/b&gt;.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="217"/>
      <source>&lt;p&gt;Some East Asian MUDs may use glyphs (characters) that Unicode classifies as being of &lt;i&gt;Ambigous&lt;/i&gt; width when drawn in a font with a so-called &lt;i&gt;fixed&lt;/i&gt; pitch; in fact such text is &lt;i&gt;duo-spaced&lt;/i&gt; when not using a proportional font. These symbols can be drawn using either a half or the whole space of a full character. By default Mudlet tries to chose the right width automatically but you can override the setting for each profile.&lt;/p&gt;&lt;p&gt;This control has three settings:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;narrow&lt;/i&gt;&apos; = Draw ambiguous width characters in a single &apos;space&apos;.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;wide&lt;/i&gt;&apos; = Draw ambiguous width characters two &apos;spaces&apos; wide.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use &apos;wide&apos; setting for MUD Server encodings of &lt;b&gt;Big5&lt;/b&gt;, &lt;b&gt;GBK&lt;/b&gt; or &lt;b&gt;GBK18030&lt;/b&gt; and &apos;narrow&apos; for all others.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This is a temporary arrangement and will probably change when Mudlet gains full support for languages other than English.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="229"/>
      <source>&lt;p&gt;Enable a context (right click) menu action on any console/user window that, when the mouse cursor is hovered over it, will display the UTF-16 and UTF-8 items that make up each Unicode codepoint on the &lt;b&gt;first&lt;/b&gt; line of any selection.&lt;/p&gt;&lt;p&gt;This utility feature is intended to help the user identify any grapheme (visual equivalent to a &lt;i&gt;character&lt;/i&gt;) that a Game server may send even if it is composed of multiple bytes as any non-ASCII character will be in the Lua sub-system which uses the UTF-8 encoding system.&lt;p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="571"/>
      <source>Mudlet dictionaries:</source>
      <comment>On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones.</comment>
      <translation>Mudlet-Wörterbücher:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="573"/>
      <source>System dictionaries:</source>
      <comment>On *nix systems where we find the system ones we use them.</comment>
      <translation>System-Wörterbücher:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="604"/>
      <source>&lt;p&gt;From the dictionary file &lt;tt&gt;%1.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aus der Wörterbuchdatei &lt;tt&gt;%1.dic&lt;/tt&gt; (und der begleitenden Affix &lt;tt&gt;.aff&lt;/tt&gt; Datei).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="606"/>
      <source>%1 - not recognised</source>
      <translation>%1 - nicht erkannt</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="607"/>
      <source>&lt;p&gt;Mudlet does not recognise the code &quot;%1&quot;, please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!&lt;/p&gt;&lt;p&gt;The file &lt;tt&gt;%2.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file) is still usable.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet erkennt den Code &quot;%1&quot; nicht. Bitte melde ihn bei den Mudlet-Entwicklern, damit wir ihn in zukünftigen Mudlet-Versionen richtig beschreiben können!&lt;/p&gt;&lt;p&gt;Die Datei &lt;tt&gt;%2.dic&lt;/tt&gt; (und die zugehörige &lt;tt&gt;.aff&lt;/tt&gt; Affix-Datei) ist weiterhin verwendbar.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="632"/>
      <source>No Hunspell dictionary files found, spell-checking will not be available.</source>
      <translation>Es wurden keine Hunspell-Wörterbuchdateien gefunden. Rechtschreibprüfung ist nicht verfügbar.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2008"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>Mudlet Karte (*.dat);;Xml Kartendaten (*.xml);;Beliebige Datei (*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3228"/>
      <source>&lt;p&gt;The room symbol will appear like this if only symbols (glyphs) from the specfic font are used.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Raumsymbol erscheint wie folgt, wenn nur Symbole (Glyphen) aus der spezifischen Schriftart verwendet werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3234"/>
      <source>&lt;p&gt;The room symbol will appear like this if symbols (glyphs) from any font can be used.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Raumsymbol erscheint wie folgt, wenn Symbole (Glyphen) aus jeder Schriftart verwendet werdet dürfen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3263"/>
      <source>&lt;p&gt;These are the sequence of hexadecimal numbers that are used by the Unicode consortium to identify the graphemes needed to create the symbol.  These numbers can be utilised to determine precisely what is to be drawn even if some fonts have glyphs that are the same for different codepoints or combination of codepoints.&lt;/p&gt;&lt;p&gt;Character entry utilities such as &lt;i&gt;charmap.exe&lt;/i&gt; on &lt;i&gt;Windows&lt;/i&gt; or &lt;i&gt;gucharmap&lt;/i&gt; on many Unix type operating systems will also use these numbers which cover everything from U+0020 {Space} to U+10FFFD the last usable number in the &lt;i&gt;Private Use Plane 16&lt;/i&gt; via most of the written marks that humanity has ever made.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3276"/>
      <source>&lt;p&gt;How many rooms in the whole map have this symbol.</source>
      <translation>&lt;p&gt;Wie viele Räume in der gesamten Karte haben dieses Symbol.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3288"/>
      <source>more - not shown...</source>
      <translation>mehr - nicht angezeigt...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3295"/>
      <source>&lt;p&gt;The rooms with this symbol, up to a maximum of thirty-two, if there are more than this, it is indicated but they are not shown.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Räume mit diesem Symbol, maximal 32. Falls es mehr sind, wird es angezeigt, aber sie werden nicht gelistet.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3304"/>
      <source>&lt;p&gt;The symbol can be made entirely from glyphs in the specified font.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Symbol kann vollständig aus Glypthen der gewählten Schriftart erstellt werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3314"/>
      <source>&lt;p&gt;The symbol cannot be made entirely from glyphs in the specified font, but, using other fonts in the system, it can. Either un-check the &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; option or try and choose another font that does have the needed glyphs.&lt;/p&gt;&lt;p&gt;&lt;i&gt;You need not close this table to try another font, changing it on the main preferences dialogue will update this table after a slight delay.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Symbol kann nicht vollständig aus Glypthen der gewählten Schriftart erstellt werden, aber mit anderen Schriftarten geht es. Bitte deaktiviere entweder die Einstellung &lt;i&gt;Das Symbol kann vollständig aus Glypthen der gewählten Schriftart erstellt werden.&lt;/i&gt; oder versuche eine andere Schriftart mit den benötigten Glyphen.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Du brauchst diese Tabelle nicht zu schließen, um eine andere Schriftart auszuprobieren. Wenn du sie in den Einstellungen änderst, wird sich diese Tabelle kurz darauf aktualisieren.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3324"/>
      <source>&lt;p&gt;The symbol cannot be drawn using any of the fonts in the system, either an invalid string was entered as the symbol for the indicated rooms or the map was created on a different systems with a different set of fonts available to use. You may be able to correct this by installing an additional font using whatever method is appropriate for this system or by editing the map to use a different symbol. It may be possible to do the latter via a lua script using the &lt;i&gt;getRoomChar&lt;/i&gt; and &lt;i&gt;setRoomChar&lt;/i&gt; functions.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3413"/>
      <source>Large icon</source>
      <comment>Discord Rich Presence large icon</comment>
      <translation>Großes Symbol</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3414"/>
      <source>Detail</source>
      <comment>Discord Rich Presence detail</comment>
      <translation>Detail</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3416"/>
      <source>Small icon</source>
      <comment>Discord Rich Presence small icon</comment>
      <translation>Kleines Symbol</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3417"/>
      <source>State</source>
      <comment>Discord Rich Presence state</comment>
      <translation>Zustand</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3419"/>
      <source>Party size</source>
      <comment>Discord Rich Presence party size</comment>
      <translation>Gruppengröße</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3420"/>
      <source>Party max</source>
      <comment>Discord Rich Presence maximum party size</comment>
      <translation>Maximale Gruppengröße</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3421"/>
      <source>Time</source>
      <comment>Discord Rich Presence time until or time elapsed</comment>
      <translation>Zeit</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3456"/>
      <source>Map symbol usage - %1</source>
      <translation>Benutzung des Kartensymbols - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3533"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (z.B., 1970-01-01#00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3534"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (z.B., 1970-01-01T00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3535"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)</source>
      <translation>yyyy-MM-dd (Logs eines Tages aneinander hängen, z.B. 1970-01-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3536"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.html)</source>
      <translation>yyyy-MM (Logs eines Monats aneinander hängen, z.B. 1970-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3539"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (z.B., 1970-01-01#00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3540"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (z.B., 1970-01-01T00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3541"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)</source>
      <translation>yyyy-MM-dd (Logs eines Tages aneinander hängen, z.B. 1970-01-01.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3542"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)</source>
      <translation>yyyy-MM (Logs eines Monats aneinander hängen, z.B. 1970-01.txt)</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomExits</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="144"/>
      <location filename="../src/dlgRoomExits.cpp" line="149"/>
      <location filename="../src/dlgRoomExits.cpp" line="724"/>
      <location filename="../src/dlgRoomExits.cpp" line="729"/>
      <location filename="../src/dlgRoomExits.cpp" line="777"/>
      <location filename="../src/dlgRoomExits.cpp" line="782"/>
      <location filename="../src/dlgRoomExits.cpp" line="826"/>
      <location filename="../src/dlgRoomExits.cpp" line="831"/>
      <location filename="../src/dlgRoomExits.cpp" line="875"/>
      <location filename="../src/dlgRoomExits.cpp" line="880"/>
      <location filename="../src/dlgRoomExits.cpp" line="924"/>
      <location filename="../src/dlgRoomExits.cpp" line="929"/>
      <location filename="../src/dlgRoomExits.cpp" line="973"/>
      <location filename="../src/dlgRoomExits.cpp" line="978"/>
      <location filename="../src/dlgRoomExits.cpp" line="1022"/>
      <location filename="../src/dlgRoomExits.cpp" line="1027"/>
      <location filename="../src/dlgRoomExits.cpp" line="1071"/>
      <location filename="../src/dlgRoomExits.cpp" line="1076"/>
      <location filename="../src/dlgRoomExits.cpp" line="1120"/>
      <location filename="../src/dlgRoomExits.cpp" line="1125"/>
      <location filename="../src/dlgRoomExits.cpp" line="1169"/>
      <location filename="../src/dlgRoomExits.cpp" line="1174"/>
      <location filename="../src/dlgRoomExits.cpp" line="1218"/>
      <location filename="../src/dlgRoomExits.cpp" line="1223"/>
      <location filename="../src/dlgRoomExits.cpp" line="1267"/>
      <location filename="../src/dlgRoomExits.cpp" line="1272"/>
      <location filename="../src/dlgRoomExits.cpp" line="1741"/>
      <location filename="../src/dlgRoomExits.cpp" line="1746"/>
      <location filename="../src/dlgRoomExits.cpp" line="1849"/>
      <location filename="../src/dlgRoomExits.cpp" line="1854"/>
      <source>&lt;b&gt;Room&lt;/b&gt; Weight of destination: %1.</source>
      <comment>Bold HTML tags are used to emphasis that the value is destination room&apos;s weight whether overridden by a non-zero exit weight here or not.</comment>
      <translation>&lt;b&gt;Raum&lt;/b&gt; Gewicht des Ziels: %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="148"/>
      <location filename="../src/dlgRoomExits.cpp" line="728"/>
      <location filename="../src/dlgRoomExits.cpp" line="781"/>
      <location filename="../src/dlgRoomExits.cpp" line="830"/>
      <location filename="../src/dlgRoomExits.cpp" line="879"/>
      <location filename="../src/dlgRoomExits.cpp" line="928"/>
      <location filename="../src/dlgRoomExits.cpp" line="977"/>
      <location filename="../src/dlgRoomExits.cpp" line="1026"/>
      <location filename="../src/dlgRoomExits.cpp" line="1075"/>
      <location filename="../src/dlgRoomExits.cpp" line="1124"/>
      <location filename="../src/dlgRoomExits.cpp" line="1173"/>
      <location filename="../src/dlgRoomExits.cpp" line="1222"/>
      <location filename="../src/dlgRoomExits.cpp" line="1271"/>
      <location filename="../src/dlgRoomExits.cpp" line="1745"/>
      <location filename="../src/dlgRoomExits.cpp" line="1853"/>
      <source>Exit to unnamed room is valid</source>
      <translation>Ausgang zu unbenanntem Raum ist gültig</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="155"/>
      <source>Entered number is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums, zu dem dieser besondere Ausgang hinführt. Gültige Zahlen werden blau. Falls sie bleibt wie jetzt, wird dieser Ausgang gelöscht, sobald &apos;Speichern&apos; angeklickt wird.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="217"/>
      <source>Set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation>Wähle die ID des Raums, zu dem dieser besondere Ausgang hinführt. Gültige Zahlen werden blau. Falls sie bleibt wie jetzt, wird dieser Ausgang gelöscht, sobald &apos;Speichern&apos; angeklickt wird.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="220"/>
      <location filename="../src/dlgRoomExits.cpp" line="1872"/>
      <source>Prevent a route being created via this exit, equivalent to an infinite exit weight.</source>
      <translation>Verhindere die Erstellung von Routen durch diesen Ausgang. Das entspricht einem unendlichen Gewicht des Ausgangs.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="224"/>
      <location filename="../src/dlgRoomExits.cpp" line="1881"/>
      <source>Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.</source>
      <translation>Wähle einen positiven Wert, um die normale Gewichtung des Raums für diese Ausgangsrichtung zu setzen. Null setzt wieder den Standard.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="226"/>
      <location filename="../src/dlgRoomExits.cpp" line="1888"/>
      <source>No door symbol is drawn on 2D Map for this exit (only functional choice currently).</source>
      <translation>Für diesen Ausgang wird kein Symbol für die Tür auf der 2D Karte gezeichnet (derzeit einzige funktionale Auswahl).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="228"/>
      <location filename="../src/dlgRoomExits.cpp" line="1892"/>
      <source>Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Ein grünes Symbol für die (offene) Tür würde auf der benutzerdefinierten Linie für diesen Ausgang auf der 2D Karte gezeichnet (derzeit aber nicht).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="230"/>
      <location filename="../src/dlgRoomExits.cpp" line="1897"/>
      <source>Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Ein oranges Symbol für die (geschlossene) Tür würde auf der benutzerdefinierten Linie für diesen Ausgang auf der 2D Karte gezeichnet (derzeit aber nicht).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="232"/>
      <location filename="../src/dlgRoomExits.cpp" line="1900"/>
      <source>Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Ein rotes Symbol für die (versperrte) Tür würde auf der benutzerdefinierten Linie für diesen Ausgang auf der 2D Karte gezeichnet (derzeit aber nicht).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="92"/>
      <location filename="../src/dlgRoomExits.cpp" line="215"/>
      <source>(room ID)</source>
      <comment>Placeholder, if no room ID is set for an exit, yet. This string is used in 2 places, ensure they match!</comment>
      <translation>(Raum-ID)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="132"/>
      <location filename="../src/dlgRoomExits.cpp" line="233"/>
      <location filename="../src/dlgRoomExits.cpp" line="268"/>
      <location filename="../src/dlgRoomExits.cpp" line="2255"/>
      <location filename="../src/dlgRoomExits.cpp" line="2277"/>
      <source>(command or Lua script)</source>
      <comment>Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same</comment>
      <translation>(Befehl oder Lua-Skript)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="143"/>
      <location filename="../src/dlgRoomExits.cpp" line="723"/>
      <location filename="../src/dlgRoomExits.cpp" line="776"/>
      <location filename="../src/dlgRoomExits.cpp" line="825"/>
      <location filename="../src/dlgRoomExits.cpp" line="874"/>
      <location filename="../src/dlgRoomExits.cpp" line="923"/>
      <location filename="../src/dlgRoomExits.cpp" line="972"/>
      <location filename="../src/dlgRoomExits.cpp" line="1021"/>
      <location filename="../src/dlgRoomExits.cpp" line="1070"/>
      <location filename="../src/dlgRoomExits.cpp" line="1119"/>
      <location filename="../src/dlgRoomExits.cpp" line="1168"/>
      <location filename="../src/dlgRoomExits.cpp" line="1217"/>
      <location filename="../src/dlgRoomExits.cpp" line="1266"/>
      <location filename="../src/dlgRoomExits.cpp" line="1740"/>
      <location filename="../src/dlgRoomExits.cpp" line="1848"/>
      <source>Exit to &quot;%1&quot;.</source>
      <translation>Ausgang nach &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="737"/>
      <source>Entered number is invalid, set the number of the room northwest of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Nordwesten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="748"/>
      <location filename="../src/dlgRoomExits.cpp" line="1320"/>
      <source>Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Nordwesten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="788"/>
      <source>Entered number is invalid, set the number of the room north of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Norden von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="798"/>
      <location filename="../src/dlgRoomExits.cpp" line="1354"/>
      <source>Set the number of the room north of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Norden von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="837"/>
      <source>Entered number is invalid, set the number of the room northeast of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Nordosten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="847"/>
      <location filename="../src/dlgRoomExits.cpp" line="1385"/>
      <source>Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Nordosten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="886"/>
      <source>Entered number is invalid, set the number of the room up from this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Oben von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="896"/>
      <location filename="../src/dlgRoomExits.cpp" line="1416"/>
      <source>Set the number of the room up from this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Oben von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="935"/>
      <source>Entered number is invalid, set the number of the room west of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Westen von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="945"/>
      <location filename="../src/dlgRoomExits.cpp" line="1447"/>
      <source>Set the number of the room west of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Westen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="984"/>
      <source>Entered number is invalid, set the number of the room east of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Osten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="994"/>
      <location filename="../src/dlgRoomExits.cpp" line="1478"/>
      <source>Set the number of the room east of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Osten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1033"/>
      <source>Entered number is invalid, set the number of the room down from this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Unten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1043"/>
      <location filename="../src/dlgRoomExits.cpp" line="1509"/>
      <source>Set the number of the room down from this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Unten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1082"/>
      <source>Entered number is invalid, set the number of the room southwest of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Südwesten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1092"/>
      <location filename="../src/dlgRoomExits.cpp" line="1540"/>
      <source>Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Südwesten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1131"/>
      <source>Entered number is invalid, set the number of the room south of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Süden von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1141"/>
      <location filename="../src/dlgRoomExits.cpp" line="1571"/>
      <source>Set the number of the room south of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Süden von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1180"/>
      <source>Entered number is invalid, set the number of the room southeast of this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Südosten von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1190"/>
      <location filename="../src/dlgRoomExits.cpp" line="1602"/>
      <source>Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Südosten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1229"/>
      <source>Entered number is invalid, set the number of the room in from this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Innen von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1239"/>
      <location filename="../src/dlgRoomExits.cpp" line="1633"/>
      <source>Set the number of the room in from this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Innen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1278"/>
      <source>Entered number is invalid, set the number of the room out from this one, will turn blue for a valid number.</source>
      <translation>Die eingegebene Nummer ist ungültig. Wähle die ID des Raums in Richtung Außen von hier aus. Gültige Zahlen werden blau.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1288"/>
      <location filename="../src/dlgRoomExits.cpp" line="1664"/>
      <source>Set the number of the room out from this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung Außen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1312"/>
      <location filename="../src/dlgRoomExits.cpp" line="1346"/>
      <location filename="../src/dlgRoomExits.cpp" line="1377"/>
      <location filename="../src/dlgRoomExits.cpp" line="1408"/>
      <location filename="../src/dlgRoomExits.cpp" line="1439"/>
      <location filename="../src/dlgRoomExits.cpp" line="1470"/>
      <location filename="../src/dlgRoomExits.cpp" line="1501"/>
      <location filename="../src/dlgRoomExits.cpp" line="1532"/>
      <location filename="../src/dlgRoomExits.cpp" line="1563"/>
      <location filename="../src/dlgRoomExits.cpp" line="1594"/>
      <location filename="../src/dlgRoomExits.cpp" line="1625"/>
      <location filename="../src/dlgRoomExits.cpp" line="1656"/>
      <location filename="../src/dlgRoomExits.cpp" line="1769"/>
      <source>Clear the stub exit for this exit to enter an exit roomID.</source>
      <translation>Deaktiviere die Abzweigung, um einen Ausgang per ID einzugeben.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1691"/>
      <source>northwest</source>
      <translation>nordwesten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1692"/>
      <source>north</source>
      <translation>norden</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1693"/>
      <source>northeast</source>
      <translation>nordosten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1694"/>
      <source>up</source>
      <translation>oben</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1695"/>
      <source>west</source>
      <translation>westen</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1696"/>
      <source>east</source>
      <translation>osten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1697"/>
      <source>down</source>
      <translation>unten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1698"/>
      <source>southwest</source>
      <translation>südwesten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1699"/>
      <source>south</source>
      <translation>süden</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1700"/>
      <source>southeast</source>
      <translation>südosten</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1701"/>
      <source>in</source>
      <translation>rein</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1702"/>
      <source>out</source>
      <translation>raus</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1777"/>
      <source>Set the number of the room %1 of this one, will be blue for a valid number or red for invalid.</source>
      <translation>Wähle die ID des Raums in Richtung %1 von hier aus. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1800"/>
      <source>Exits for room: &quot;%1&quot; [*]</source>
      <translation>Ausgänge aus Raum: &quot;%1&quot; [*]</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1802"/>
      <source>Exits for room Id: %1 [*]</source>
      <translation>Ausgänge für Raum ID: %1 [*]</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1860"/>
      <source>Room Id is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number.</source>
      <translation>Raum ID ist ungültig. Wähle die ID des Raums, zu dem dieser besondere Ausgang führt. Gültige Zahlen werden blau, ungültige werden rot.</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomSymbol</name>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="75"/>
      <source>The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</source>
      <comment>This is for when applying a new room symbol to one or more rooms and some have the SAME symbol (others may have none) at present, %n is the total number of rooms involved and is at least two. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>
        <numerusform>Das einzige verwendete Symbol in 
einem oder mehreren der 
ausgewählten Räume ist &quot;%1&quot;. 
Lösche dieses, um es in allen 
ausgewählten Räumen zu löschen,
oder ersetze es mit einem neuen 
Symbol für alle Räume:</numerusform>
        <numerusform>Das einzige verwendete Symbol in 
einem oder mehreren der 
ausgewählten Räume ist &quot;%1&quot;. 
Lösche dieses, um es in allen 
ausgewählten Räumen zu löschen,
oder ersetze es mit einem neuen 
Symbol für alle Räume:</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="86"/>
      <source>The symbol is &quot;%1&quot; in the selected room,
delete this to clear the symbol or replace
it with a new symbol for this room:</source>
      <comment>This is for when applying a new room symbol to one room. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Das einzige verwendete Symbol im
ausgewählten Raum ist &quot;%1&quot;. 
Lösche dieses, um es im ausgewählten 
Raum zu löschen, oder ersetze es mit 
einem neuen Symbol für diesen Raum:</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="95"/>
      <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
      <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
      <translation>
        <numerusform>Wähle:
 • ein existierendes Symbol aus der Liste unten (sortiert nach Häufigkeit der Nutzung)
 • tippe ein oder mehrere Grapheme (&quot;sichtbare Zeichen&quot;) als neues Symbol
 • tippe ein Leerzeichen, um die bestehenden Symbole zu entfernen
für alle %n ausgewählten Räume:</numerusform>
        <numerusform>Wähle:
 • ein existierendes Symbol aus der Liste unten (sortiert nach Häufigkeit der Nutzung)
 • tippe ein oder mehrere Grapheme (&quot;sichtbare Zeichen&quot;) als neues Symbol
 • tippe ein Leerzeichen, um die bestehenden Symbole zu entfernen
für alle %n ausgewählten Räume:</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="129"/>
      <source>%1 {count:%2}</source>
      <comment>Everything after the first parameter (the &apos;%1&apos;) will be removed by processing it as a QRegularExpression programmatically, ensure the translated text has ` {` immediately after the &apos;%1&apos;, and &apos;}&apos; as the very last character, so that the right portion can be extracted if the user clicks on this item when it is shown in the QComboBox it is put in.</comment>
      <translation>%1 {Anzahl: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="200"/>
      <source>Pick color</source>
      <translation>Farbe wählen</translation>
    </message>
  </context>
  <context>
    <name>dlgTriggerEditor</name>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="251"/>
      <source>-- Enter your lua code here
</source>
      <translation>-- Gib deinen Lua-Code hier ein
</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="331"/>
      <source>*** starting new session ***
</source>
      <translation>*** Neue Sitzung wird gestartet ***
</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="419"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5838"/>
      <source>Triggers</source>
      <translation>Trigger</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="420"/>
      <source>Show Triggers</source>
      <translation>Trigger anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="423"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5862"/>
      <source>Buttons</source>
      <translation>Buttons</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="424"/>
      <source>Show Buttons</source>
      <translation>Buttons anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="428"/>
      <source>Aliases</source>
      <translation>Aliase</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="429"/>
      <source>Show Aliases</source>
      <translation>Aliase anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5844"/>
      <source>Timers</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="434"/>
      <source>Show Timers</source>
      <translation>Timer anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="437"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5850"/>
      <source>Scripts</source>
      <translation>Skripte</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="438"/>
      <source>Show Scripts</source>
      <translation>Skripte anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="441"/>
      <source>Keys</source>
      <translation>Tasten</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="442"/>
      <source>Show Keybindings</source>
      <translation>Verbundene Tasten anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="445"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="6250"/>
      <source>Variables</source>
      <translation>Variablen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="446"/>
      <source>Show Variables</source>
      <translation>Variablen anzeigen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="449"/>
      <source>Activate</source>
      <translation>Aktivieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="450"/>
      <source>Toggle Active or Non-Active Mode for Triggers, Scripts etc.</source>
      <translation>Wechsle aktiven und inaktiven Status des Triggers.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="460"/>
      <source>Add Item</source>
      <translation>Element hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="461"/>
      <source>Add new Trigger, Script, Alias or Filter</source>
      <translation>Ergänze neue Trigger, Skripte, Aliase oder Filter</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="464"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="466"/>
      <source>Delete Item</source>
      <translation>Element löschen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="465"/>
      <source>Delete Trigger, Script, Alias or Filter</source>
      <translation>Lösche Trigger, Skripte, Aliase oder Filter</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="472"/>
      <source>Add Group</source>
      <translation>Gruppe hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="473"/>
      <source>Add new Group</source>
      <translation>Neue Gruppe hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="476"/>
      <source>Save Item</source>
      <translation>Element speichern</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8316"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8341"/>
      <source>Ctrl+S</source>
      <translation>Ctrl+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="479"/>
      <source>Saves the selected item. (Ctrl+S)&lt;/p&gt;Saving causes any changes to the item to take effect.
It will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)</source>
      <translation>Speichert das gewählte Element. (Strg+S)&lt;/p&gt; Speichern bewirkt, dass alle Änderungen des Elements wirksam werden.
Es speichert nicht auf die Festplatte, also können Änderungen bei einem Absturz des Computers/Programms verloren gehen (&apos;Profil speichern&apos; rechts ist sicher.)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="481"/>
      <source>Saves the selected trigger, script, alias, etc, causing new changes to take effect - does not save to disk though...</source>
      <translation>Speichert die ausgewählten Trigger, Skripte, Aliase, usw, wodurch die neuen Änderungen wirksam werden - speichert aber nicht auf der Festplatte...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="484"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8807"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8813"/>
      <source>Copy</source>
      <translation>Kopieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="488"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="489"/>
      <source>Copy the trigger/script/alias/etc</source>
      <translation>Kopiere Trigger/Skript/Alias/etc</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="498"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8808"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8814"/>
      <source>Paste</source>
      <translation>Einfügen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="502"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="503"/>
      <source>Paste triggers/scripts/aliases/etc from the clipboard</source>
      <translation>Trigger/Skripte/Aliase/Etc aus der Zwischenablage einfügen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="517"/>
      <source>Import</source>
      <translation>Importieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="521"/>
      <source>Export</source>
      <translation>Exportieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="525"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8651"/>
      <source>Save Profile</source>
      <translation>Profil speichern</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="527"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8318"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8343"/>
      <source>Ctrl+Shift+S</source>
      <translation>Strg+Shift+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="530"/>
      <source>Saves your profile. (Ctrl+Shift+S)&lt;p&gt;Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.&lt;/p&gt;&lt;p&gt;It also makes a backup of your profile, you can load an older version of it when connecting.&lt;/p&gt;&lt;p&gt;Should there be any modules that are marked to be &quot;&lt;i&gt;synced&lt;/i&gt;&quot; this will also cause them to be saved and reloaded into other profiles if they too are active.</source>
      <translation>Speichert dein Profil. (Strg+Shift+S)&lt;p&gt;Speichert dein gesamtes Profil (Trigger, Aliase, Skripte, Timer, Buttons und Tasten, aber nicht die Karte oder Skript-spezifische Einstellungen) auf deinem Computerlaufwerk, so dass deine ganzen Änderungen bei einem Absturz des Programms oder Computers beibehalten werden.&lt;/p&gt;&lt;p&gt;Es wird auch ein Backup des Profils anlegen, so dass du beim Verbinden eine ältere Version laden kannst.&lt;/p&gt;&lt;p&gt;Sollten irgendwelche Module vorhanden und &quot;&lt;i&gt;zur Synchronisierung&lt;/i&gt;&quot; vorgemerkt sein, werden sie hierdurch auch gespeichern und in anderen Profilen neu geladen, falls diese auch aktiv sind.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="532"/>
      <source>Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also &quot;synchronizes&quot; modules that are so marked.</source>
      <translation>Speichert dein gesamtes Profil (Trigger, Aliase, Skripte, Timer, Buttons und Tasten, aber nicht die Karte oder skriptspezifische Einstellungen); synchronisiert auch Module, die dazu markiert sind.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="535"/>
      <source>Save Profile As</source>
      <translation>Profil speichern als</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="539"/>
      <source>Statistics</source>
      <translation>Statistiken</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="540"/>
      <source>Generates a statistics summary display on the main profile console.</source>
      <translation>Erstellt zusammenfassende Statistiken im Hauptfenster.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="544"/>
      <source>Shows/Hides the errors console in the bottom right of this editor.</source>
      <translation>Zeigt/Versteckt die Fehler-Konsole in der unteren rechten Seite des Editors.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="547"/>
      <source>Debug</source>
      <translation>Debug</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="548"/>
      <source>Activates Debug Messages -&gt; system will be &lt;b&gt;&lt;i&gt;slower&lt;/i&gt;&lt;/b&gt;.</source>
      <translation>Aktiviert Debug-Nachrichten -&gt; Das System wird &lt;b&gt;&lt;i&gt;langsamer&lt;/i&gt;&lt;/b&gt; sein.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="549"/>
      <source>Shows/Hides the separate Central Debug Console - when being displayed the system will be slower.</source>
      <translation>Zeigt/Versteckt die separate zentrale Debug-Konsole - während der Anzeige wird das System langsamer sein.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="572"/>
      <source>Editor Toolbar - %1 - Actions</source>
      <comment>This is the toolbar that is initally placed at the top of the editor.</comment>
      <translation>Editor-Symbolleiste - %1 - Aktionen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="608"/>
      <source>Editor Toolbar - %1 - Items</source>
      <comment>This is the toolbar that is initally placed at the left side of the editor.</comment>
      <translation>Editor-Symbolleiste - %1 - Elemente</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="672"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="675"/>
      <source>Search Options</source>
      <translation>Suchoptionen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="679"/>
      <source>Case sensitive</source>
      <translation>Groß-/Kleinschreibung beachten</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="732"/>
      <source>Type</source>
      <comment>Heading for the first column of the search results</comment>
      <translation>Art</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="733"/>
      <source>Name</source>
      <comment>Heading for the second column of the search results</comment>
      <translation>Name</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="734"/>
      <source>Where</source>
      <comment>Heading for the third column of the search results</comment>
      <translation>Wo</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="735"/>
      <source>What</source>
      <comment>Heading for the fourth column of the search results</comment>
      <translation>Was</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="782"/>
      <source>start of line</source>
      <translation>Zeilenanfang</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="815"/>
      <source>Text to find (trigger pattern)</source>
      <translation>Zu findender Text (Triggermuster)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2785"/>
      <source>Trying to activate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Triggergruppe, Filter oder Trigger, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; aktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2787"/>
      <source>Trying to deactivate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Triggergruppe, Filter oder Trigger, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; deaktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2791"/>
      <source>&lt;b&gt;Unable to activate a filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;b&gt;Triggergruppe, Filter oder Trigger, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, konnte nicht aktiviert werden. Grund: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Bitte erneut aktivieren, nachdem das Problem gelöst wurde.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2928"/>
      <source>Trying to activate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Timergruppe, Offset-Timer oder Timer, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; aktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2930"/>
      <source>Trying to deactivate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Timergruppe, Offset-Timer oder Timer, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; deaktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2934"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an offset timer or timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Offset-Timer oder Timer, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, konnte nicht aktiviert werden. Grund: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Bitte erneut aktivieren, nachdem das Problem gelöst wurde.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2972"/>
      <source>Trying to activate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Aliasgruppe, Alias, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; aktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2974"/>
      <source>Trying to deactivate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Aliasgruppe, Alias, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; deaktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2978"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Alias oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, konnte nicht aktiviert werden. Grund: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Bitte erneut aktivieren, nachdem das Problem gelöst wurde.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3076"/>
      <source>Trying to activate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Skriptgruppe, Skript, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; aktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3078"/>
      <source>Trying to deactivate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Skriptgruppe, Skript, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; deaktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3082"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a script group or script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Skriptgruppe oder Skript oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, konnte nicht aktiviert werden. Grund: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Bitte erneut aktivieren, nachdem das Problem gelöst wurde.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3147"/>
      <source>Trying to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Button, Menu oder Schaltleiste, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; aktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3149"/>
      <source>Trying to deactivate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Button, Menu oder Schaltleiste, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, &lt;em&gt;erfolgreich&lt;/em&gt; deaktiviert.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3153"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Button, Menu oder Schaltleiste, oder Teil eines Moduls &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;, das sie enthält, konnte nicht aktiviert werden. Grund: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Bitte erneut aktivieren, nachdem das Problem gelöst wurde.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3267"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4097"/>
      <source>New trigger group</source>
      <translation>Neue Trigger-Gruppe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3269"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4097"/>
      <source>New trigger</source>
      <translation>Neuer Trigger</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3368"/>
      <source>New timer group</source>
      <translation>Neue Timer-Gruppe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3370"/>
      <source>New timer</source>
      <translation>Neuer Timer</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3458"/>
      <source>Table name...</source>
      <translation>Tabellenname...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3465"/>
      <source>Variable name...</source>
      <translation>Name der Variable...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3522"/>
      <source>New key group</source>
      <translation>Neue Tasten-Gruppe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3524"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4797"/>
      <source>New key</source>
      <translation>Neue Taste</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3606"/>
      <source>New alias group</source>
      <translation>Neue Alias-Gruppe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3608"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4205"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4275"/>
      <source>New alias</source>
      <translation>Neuer Alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3697"/>
      <source>New menu</source>
      <translation>Neues Menü</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3699"/>
      <source>New button</source>
      <translation>Neuer Button</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3730"/>
      <source>New toolbar</source>
      <translation>Neue Symbolleiste</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3783"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4524"/>
      <source>New script group</source>
      <translation>Neue Skript-Gruppe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3785"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4524"/>
      <source>New script</source>
      <translation>Neues Skript</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4219"/>
      <source>Alias &lt;em&gt;%1&lt;/em&gt; has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn&apos;t good as it&apos;ll call itself forever.</source>
      <translation>Alias &lt;em&gt;%1&lt;/em&gt; weist eine Endlosschleife auf - Die Ersetzung stimmt mit dem eigenen Muster überein. Bitte korrigieren, da das Alias sich immer wieder selbst aufrufen wird.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4762"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation>Markierte Variablen werden mit deinem Profil gespeichert und geladen.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4897"/>
      <source>match on the prompt line</source>
      <translation>Promptzeile erkennen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4900"/>
      <source>match on the prompt line (disabled)</source>
      <translation>Promptzeile erkennen (deaktiviert)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4901"/>
      <source>A Go-Ahead (GA) signal from the game is required to make this feature work</source>
      <translation>Ein Go-Ahead (GA) Signal aus dem Spiel ist erforderlich, damit dieses Feature funktioniert</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4951"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5061"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8550"/>
      <source>Foreground color ignored</source>
      <comment>Color trigger ignored foreground color button, ensure all three instances have the same text</comment>
      <translation>Vordergrundfarbe ignoriert</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4955"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5065"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8553"/>
      <source>Default foreground color</source>
      <comment>Color trigger default foreground color button, ensure all three instances have the same text</comment>
      <translation>Standard-Vordergrundfarbe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4959"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5069"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8556"/>
      <source>Foreground color [ANSI %1]</source>
      <comment>Color trigger ANSI foreground color button, ensure all three instances have the same text</comment>
      <translation>Vordergrundfarbe [ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4966"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5076"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8613"/>
      <source>Background color ignored</source>
      <comment>Color trigger ignored background color button, ensure all three instances have the same text</comment>
      <translation>Hintergrundfarbe ignoriert</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4970"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5080"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8616"/>
      <source>Default background color</source>
      <comment>Color trigger default background color button, ensure all three instances have the same text</comment>
      <translation>Standard-Hintergrundfarbe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4974"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5084"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8619"/>
      <source>Background color [ANSI %1]</source>
      <comment>Color trigger ANSI background color button, ensure all three instances have the same text</comment>
      <translation>Hintergrundfarbe [ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5095"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5097"/>
      <source>fault</source>
      <translation>Fehler</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5150"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5154"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8439"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8465"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8957"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8958"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation>ohne</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5611"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8413"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5650"/>
      <source>Menu properties</source>
      <translation>Menü-Eigenschaften</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5660"/>
      <source>Button properties</source>
      <translation>Button-Eigenschaften</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5668"/>
      <source>Command (down);</source>
      <translation>Befehl (unten);</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5856"/>
      <source>Aliases - Input Triggers</source>
      <translation>Aliase</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5868"/>
      <source>Key Bindings</source>
      <translation>Tastenkürzel</translation>
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
      <translation>Paket exportieren:</translation>
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
      <translation>Sie müssen zuerst ein Element für den Export wählen. Bitte wählen Sie ein Element aus dem Baum und klicken Sie dann erneut auf Exportieren.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7615"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7639"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7663"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7687"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7711"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7736"/>
      <source>Package %1 saved</source>
      <translation>Paket %1 gespeichert</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7760"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7783"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7829"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7852"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7876"/>
      <source>Copied %1 to clipboard</source>
      <translation>In die Zwischenablage kopiert: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Export Triggers</source>
      <translation>Trigger exportieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Mudlet packages (*.xml)</source>
      <translation>Mudlet Pakete (*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>export package:</source>
      <translation>Paket exportieren:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>Cannot write file %1:
%2.</source>
      <translation>Kann Datei %1 nicht schreiben:
%2.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8169"/>
      <source>Import Mudlet Package</source>
      <translation>Mudlet Paket importieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8263"/>
      <source>Couldn&apos;t save profile</source>
      <translation>Konnte Profil nicht speichern</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8263"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation>Leider konnte das Profil nicht gespeichert werden - folgende Fehlermeldung erhalten: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8270"/>
      <source>Backup Profile</source>
      <translation>Profil sichern</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8270"/>
      <source>trigger files (*.trigger *.xml)</source>
      <translation>Trigger-Dateien (*.trigger *.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8400"/>
      <source>Seclect Icon</source>
      <translation>Bild wählen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8400"/>
      <source>Images (*.png *.xpm *.jpg)</source>
      <translation>Bilder (*.png *.xpm *.jpg)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8409"/>
      <source>Command (down):</source>
      <translation>Befehl (unten):</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8430"/>
      <source>Select foreground color to apply to matches</source>
      <translation>Wähle die Vordergrundfarbe, die Treffer erhalten sollen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8456"/>
      <source>Select background color to apply to matches</source>
      <translation>Wähle die Hintergrundfarbe, die Treffer erhalten sollen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8474"/>
      <source>Choose sound file</source>
      <translation>Audiodatei auswählen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8478"/>
      <source>Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;All files(*.*)</source>
      <comment>This the list of file extensions that are considered for sounds from triggers, the terms inside of the &apos;(&apos;...&apos;)&apos; and the &quot;;;&quot; are used programmatically and should not be changed.</comment>
      <translation>Audiodateien (*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-Stream (*.aac);;MPEG-2 Audio Layer 3 (*.mp3);;MPEG-4 Audio (*.mp4a);;Ogg Vorbis (*.oga *.ogg);;PCM Audio (*.pcm);;Wave (*.wav);;Windows Media Audio (*.wma);;Alle Dateien (*.*)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8531"/>
      <source>Select foreground trigger color for item %1</source>
      <translation>Wähle die Vordergrundfarbe, bei der Element %1 auslösen soll</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8595"/>
      <source>Select background trigger color for item %1</source>
      <translation>Wähle die Hintergrundfarbe, bei der Element %1 auslösen soll</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8644"/>
      <source>Saving…</source>
      <translation>Speichere…</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8803"/>
      <source>Format All</source>
      <translation>Alles formatieren</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8812"/>
      <source>Cut</source>
      <translation>Ausschneiden</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8810"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8816"/>
      <source>Select All</source>
      <translation>Alles auswählen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8972"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Audiodatei, die wiedergegeben werden soll, wenn der Trigger ausgelöst wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="780"/>
      <source>substring</source>
      <translation>Ausschnitt</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="103"/>
      <source>&lt;p&gt;Alias react on user input. To add a new alias:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define an input &lt;strong&gt;pattern&lt;/strong&gt; either literally or with a Perl regular expression.&lt;/li&gt;&lt;li&gt;Define a &apos;substitution&apos; &lt;strong&gt;command&lt;/strong&gt; to send to the game in clear text &lt;strong&gt;instead of the alias pattern&lt;/strong&gt;, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the alias.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Aliases can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permAlias(&amp;quot;my greets&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;^hi$&amp;quot;, [[send (&amp;quot;say Greetings, traveller!&amp;quot;) echo (&amp;quot;We said hi!&amp;quot;)]])&lt;/code&gt;&lt;/p&gt;&lt;p&gt;You can now greet by typing &apos;hi&apos;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aliase reagieren auf Benutzereingaben. So erstellst du ein neues Alias:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol.&lt;/li&gt;&lt;li&gt;Definiere ein &lt;strong&gt;Eingabemuster&lt;/strong&gt; entweder wortwörtlich oder mit einem regulären Ausdruck (Perl Regex)&lt;/li&gt;&lt;li&gt;Definiere einen &lt;strong&gt;Befehl&lt;/strong&gt;, der als reiner Text &lt;strong&gt;anstelle des Alias-Musters&lt;/strong&gt; an das Spiel gesendet werden soll, oder schreibe ein Skript für kompliziertere Anforderungen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Aktiviere&lt;/strong&gt; das Alias.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Aliase können auch von der Befehlszeile im Hauptfenster des Profils wie folgt definiert werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permAlias(&amp;quot;mein Gruß&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;^hi$&amp;quot;, [[send (&amp;quot;sag Grüße, Reisender!&amp;quot;) echo (&amp;quot;Wir haben gegrüßt!&amp;quot;)]])&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Du kannst jetzt grüßen, indem du &apos;hi&apos; eingibst.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="113"/>
      <source>&lt;p&gt;Triggers react on game output. To add a new trigger:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;pattern&lt;/strong&gt; that you want to trigger on.&lt;/li&gt;&lt;li&gt;Select the appropriate pattern &lt;strong&gt;type&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the trigger.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Triggers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permSubstringTrigger(&amp;quot;My drink trigger&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;You are thirsty.&amp;quot;, function() send(&amp;quot;drink water&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will keep you refreshed.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Trigger reagieren auf Spielausgaben. So erstellst du einen neuen Trigger:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol.&lt;/li&gt;&lt;li&gt;Definiere ein &lt;strong&gt;Muster&lt;/strong&gt; auf das du reagieren möchtest.&lt;/li&gt;&lt;li&gt;Wähle die passende &lt;strong&gt;Art&lt;/strong&gt; des Musters.&lt;/li&gt;&lt;li&gt;Definiere einen &lt;strong&gt;Befehl&lt;/strong&gt;, der an das Spiel gesendet werden soll, wenn der Trigger das Muster im Spieltext findet, oder schreibe ein Skript für kompliziertere Anforderungen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Aktiviere&lt;/strong&gt; den Trigger.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Trigger können auch von der Befehlszeile im Hauptfenster des Profils wie folgt definiert werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permSubstringTrigger(&amp;quot;mein Trinktrigger&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;Du bist durstig.&amp;quot;, function() send (&amp;quot;trinke wasser&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;So bleibst du erfrischt.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="124"/>
      <source>&lt;p&gt;Scripts organize code and can react to events. To add a new script:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Enter a script in the box below. You can for example define &lt;strong&gt;functions&lt;/strong&gt; to be called by other triggers, aliases, etc.&lt;/li&gt;&lt;li&gt;If you write lua &lt;strong&gt;commands&lt;/strong&gt; without defining a function, they will be run on Mudlet startup and each time you open the script for editing.&lt;/li&gt;&lt;li&gt;If needed, you can register a list of &lt;strong&gt;events&lt;/strong&gt; with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the script.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Scripts are run automatically when viewed, even if they are deactivated.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Events can also be added to a script from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;nameOfTheMudletEvent&amp;quot;, &amp;quot;nameOfYourFunctionToBeCalled&amp;quot;)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Skripte organisieren Code und können auf Ereignisse reagieren. So erstellst du ein neues Skript:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol.&lt;/li&gt;&lt;li&gt;Gebe ein Skript in die Box unten ein. Du kannst z.B. &lt;strong&gt;Funktionen&lt;/strong&gt; definieren, die von anderen Triggern, Aliasen, usw. aufgerufen werden.&lt;/li&gt;&lt;li&gt;Falls du &lt;strong&gt;Lua-Kommandos&lt;/strong&gt; schreibst, ohne eine Funktion zu definieren, werden sie beim Start von Mudlet ausgeführt, sowie jedes Mal, wenn du das Skript zur Bearbeitung öffnest.&lt;/li&gt;&lt;li&gt;Bei Bedarf kannst du eine Liste von &lt;strong&gt;Ereignissen&lt;/strong&gt; registrieren mit den + und - Symbolen. Wenn eines dieser Ereignisse stattfindet, wird die Funktion mit dem gleichen Namen wie das Skriptelement aufgerufen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Aktiviere&lt;/strong&gt; das Skript.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Skripte werden bei Ansicht automatisch ausgeführt, sogar wenn sie deaktiviert sind.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Ereignisse können einem Skript auch von der Befehlszeile im Hauptfenster des Profils wie folgt hinzugefügt werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;NameDesMudletEreignisses&amp;quot;, &amp;quot;NameDeinerAufzurufendenFunktion&amp;quot;)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="135"/>
      <source>&lt;p&gt;Timers react after a timespan once or regularly. To add a new timer:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define the &lt;strong&gt;timespan&lt;/strong&gt; after which the timer should react in a this format: hours : minutes : seconds.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game when the time has passed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the timer.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Timers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;hello!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will greet you exactly 3 seconds after it was made.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Timer reagieren nach einer Zeitspanne einmalig oder regelmäßig. So erstellst du einen neuen Timer:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol.&lt;/li&gt;&lt;li&gt;Definiere die &lt;strong&gt;Zeitspanne&lt;/strong&gt;, nach der der Timer reagieren soll in folgendem Format: Stunden : Minuten : Sekunden.&lt;/li&gt;&lt;li&gt;Definiere einen &lt;strong&gt;Befehl&lt;/strong&gt;, der als reiner Text an das Spiel gesendet werden soll, wenn die Zeit verstrichen ist, oder schreibe ein Skript für kompliziertere Anforderungen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activiere&lt;/strong&gt; den Timer.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Wenn du möchtest, dass der Timer nur einmalig und nicht regelmäßig reagiert, benutze statt dessen die Lua-Funktion tempTimer().&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Timer können auch von der Befehlszeile im Hauptfenster des Profils wie folgt definiert werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;Hallo!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Dadurch wirst du genau Sekunden nach der Erstellung begrüßt.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="146"/>
      <source>&lt;p&gt;Buttons react on mouse clicks. To add a new button:&lt;ol&gt;&lt;li&gt;Add a new group to define a new &lt;strong&gt;button bar&lt;/strong&gt; in case you don&apos;t have any.&lt;/li&gt;&lt;li&gt;Add new groups as &lt;strong&gt;menus&lt;/strong&gt; to a button bar or sub-menus to menus.&lt;li&gt;&lt;li&gt;Add new items as &lt;strong&gt;buttons&lt;/strong&gt; to a button bar or menu or sub-menu.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the toolbar, menu or button. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If a button is made a &lt;strong&gt;click-down&lt;/strong&gt; button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua &apos;getButtonState()&apos; function reports whether the button is up or down.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Buttons reagieren auf Mausklicks. So erstellst du einen neuen Button:&lt;ol&gt;&lt;li&gt;Erstelle eine neue Gruppe, um eine neue &lt;strong&gt;Button-Leiste&lt;/strong&gt; zu definieren, falls du noch keine hast.&lt;/li&gt;&lt;li&gt;Ergänze neue Gruppen als &lt;strong&gt;Menus&lt;/strong&gt; zu einer Button-Leiste oder Untermenus zu einem Menu.&lt;li&gt;&lt;li&gt;Ergänze neue Elemente als &lt;strong&gt;Buttons&lt;/strong&gt; zu einer Button-Leiste oder einem Menu oder Untermenu.&lt;/li&gt;&lt;li&gt;Definiere einen &lt;strong&gt;Befehl&lt;/strong&gt;, der als reiner Text an das Spiel gesendet werden soll, wenn der Button gedrückt wird, oder schreibe ein Skript für kompliziertere Anforderungen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activiere&lt;/strong&gt; die Leiste, das Menu oder den Button. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Deactivierte Elemente werden unsichtbar, und bei Leisten oder Menus werden auch alle darin enthaltenen Elemente unsichtbar.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Bei einem &lt;strong&gt;einrastenden Button&lt;/strong&gt; kannst du auch einen Befehl definieren, der als reiner Text an das Spiel gesendet werden soll, wenn der Button zum zweiten Mal gedrückt und somit gelöst wird, oder schreibe ein Skript, das dann ablaufen soll - in einem solchen Skript zeigt die Lua-Funktion &apos;getButtonState()&apos;, ob der Button gerade eingerastet oder gelöst ist.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="156"/>
      <source>&lt;p&gt;Keys react on keyboard presses. To add a new key binding:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Click on &lt;strong&gt;&apos;grab key&apos;&lt;/strong&gt; and then press your key combination, e.g. including modifier keys like Control, Shift, etc.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the new key binding.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Keys can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;my jump key&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;jump&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Pressing F8 will make you jump.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tasten reagieren auf die Tastatur. So erstellst du eine neue Taste:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol.&lt;/li&gt;&lt;li&gt;Klicke auf &lt;strong&gt;&apos;Taste erkennen&apos;&lt;/strong&gt; und drücke dann deine Tastenkombination, z.B. mit Hilfstasten wie Steuerung, Umschalttaste, etc.&lt;/li&gt;&lt;li&gt;Definiere einen &lt;strong&gt;Befehl&lt;/strong&gt;, der als reiner Text an das Spiel gesendet werden soll, wenn die Taste gedrückt wird, oder schreibe ein Skript für kompliziertere Anforderungen.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Aktiviere&lt;/strong&gt; die neue Tastenverbindung.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Tasten können auch von der Befehlszeile im Hauptfenster des Profils wie folgt definiert werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;Meine Sprungtaste&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;springe&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Durch Druck auf F8 wirst du springen.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="166"/>
      <source>&lt;p&gt;Variables store information. To make a new variable:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above. To add a table instead click &apos;Add Group&apos;.&lt;/li&gt;&lt;li&gt;Select type of variable value (can be a string, integer, boolean)&lt;/li&gt;&lt;li&gt;Enter the value you want to store in this variable.&lt;/li&gt;&lt;li&gt;If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.&lt;/li&gt;&lt;li&gt;To remove a variable manually, set it to &apos;nil&apos; or click on the &apos;Delete&apos; icon above.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables created here won&apos;t be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables and tables can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will create a string called &apos;foo&apos; with &apos;bar&apos; as its value.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Variablen speichern Informationen. So erstellst du eine neue Variable:&lt;ol&gt;&lt;li&gt;Klicke oben auf das &apos;Element hinzufügen&apos; Symbol. Klicke hingegen auf &apos;Gruppe hinzufügen&apos;, um eine Tabelle zu erstellen.&lt;/li&gt;&lt;li&gt;Wähle die Art des Werts der Variable (kann sein: String, Integer, Boolean)&lt;/li&gt;&lt;li&gt;Gib den Wert ein, den du in dieser Variable speichern möchtest&lt;/li&gt;&lt;li&gt;Wenn du die Variable in deiner nächsten Mudlet-Sitzung behalten möchtest, aktiviere das Kontrollkästchen auf der linken Seite in der Liste der Variablen.&lt;/li&gt;&lt;li&gt;Um eine Variable manuell zu entfernen, setze sie auf &apos;nil&apos; oder klicke oben auf das &apos;Löschen&apos; Symbol.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Hier erstellte Variablen werden nicht gespeichert, wenn Mudlet beendet wird, außer du aktivierst das Kontrollkästchen auf der linken Seite in der Liste der Variablen. Du könntest auch Skripte erstellen und darin die Variablen definieren.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Hinweis:&lt;/strong&gt; Variablen und Tabellen können auch von der Befehlszeile im Hauptfenster des Profils wie folgt definiert werden:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Dies erstellt eine String-Variable namens &apos;foo&apos; mit dem Wert &apos;bar&apos;.&lt;/p&gt;&lt;p&gt;Finde &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;weitere Informationen&lt;/a&gt; im Handbuch.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="241"/>
      <source>-- add your Lua code here</source>
      <translation>-- Gib hier deinen Lua-Code ein</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="543"/>
      <source>Errors</source>
      <translation>Fehler</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="682"/>
      <source>Match case precisely</source>
      <translation>Groß - / Kleinschreibung genau</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="686"/>
      <source>Include variables</source>
      <translation>Variablen einschließen</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="689"/>
      <source>Search variables (slower)</source>
      <translation>Variablen suchen (langsamer)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="781"/>
      <source>perl regex</source>
      <translation>Perl-Regex</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="783"/>
      <source>exact match</source>
      <translation>Genauer Treffer</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="784"/>
      <source>lua function</source>
      <translation>Lua-Funktion</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="785"/>
      <source>line spacer</source>
      <translation>Zeilenabstandshalter</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="786"/>
      <source>color trigger</source>
      <translation>Farbtrigger</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="787"/>
      <source>prompt</source>
      <translation>Prompt</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1914"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1926"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1954"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1986"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2016"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2028"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2055"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2090"/>
      <source>Trigger</source>
      <translation>Trigger</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1434"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1549"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1621"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1743"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1827"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1914"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2016"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2122"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2211"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2297"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2421"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2495"/>
      <source>Name</source>
      <translation>Name</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1489"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1494"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1561"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1566"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1633"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1638"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1837"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1842"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1926"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1931"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2028"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2033"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2132"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2137"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2309"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2314"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2438"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2507"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2512"/>
      <source>Command</source>
      <translation>Befehl</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1954"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1959"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2055"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2060"/>
      <source>Pattern {%1}</source>
      <translation>Muster {%1}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1519"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1524"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1591"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1596"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1713"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1718"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1802"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1884"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1889"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1986"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1991"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2090"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2095"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2179"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2184"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2265"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2270"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2389"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2394"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2463"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2468"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2537"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2542"/>
      <source>Lua code (%1:%2)</source>
      <translation>Lua-code (%1:%2)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1827"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1837"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1854"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1884"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2122"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2132"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2149"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2179"/>
      <source>Alias</source>
      <translation>Alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1854"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1859"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2149"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2154"/>
      <source>Pattern</source>
      <translation>Muster</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1743"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2211"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2233"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2265"/>
      <source>Script</source>
      <translation>Skript</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1770"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2233"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2238"/>
      <source>Event Handler</source>
      <translation>Ereignisbehandlung</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1621"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1633"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1652"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1713"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2297"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2309"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2328"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2389"/>
      <source>Button</source>
      <translation>Button</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1633"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1638"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2309"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2314"/>
      <source>Command {Down}</source>
      <translation>Befehl {Down}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1652"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1657"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2328"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2333"/>
      <source>Command {Up}</source>
      <translation>Befehl {Up}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1681"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2357"/>
      <source>Action</source>
      <translation>Aktion</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1681"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1686"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2357"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2362"/>
      <source>Stylesheet {L: %1 C: %2}</source>
      <translation>Stylesheet {L: %1 C: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1549"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1561"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1591"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2421"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2463"/>
      <source>Timer</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1489"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1519"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2495"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2507"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2537"/>
      <source>Key</source>
      <translation>Taste</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1434"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1448"/>
      <source>Variable</source>
      <translation>Variable</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1448"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1454"/>
      <source>Value</source>
      <translation>Wert</translation>
    </message>
  </context>
  <context>
    <name>dlgTriggerPatternEdit</name>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="52"/>
      <source>Text to find (anywhere in the game output)</source>
      <translation>Zu findender Text (irgendwo in der Ausgabe)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="55"/>
      <source>Text to find (as a regular expression pattern)</source>
      <translation>Zu findender Text (als Muster für reguläre Ausdrücke)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="58"/>
      <source>Text to find (from beginning of the line)</source>
      <translation>Zu findender Text (vom Zeilenanfang)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="61"/>
      <source>Exact line to match</source>
      <translation>Zu findende komplette Zeile</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="64"/>
      <source>Lua code to run (return true to match)</source>
      <translation>Lua-Code zur Ausführung (soll bei Treffer "true" zurückgeben)</translation>
    </message>
  </context>
  <context>
    <name>dlgVarsMainArea</name>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="51"/>
      <location filename="../src/dlgVarsMainArea.cpp" line="78"/>
      <source>Auto-Type</source>
      <translation>Automatisch</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="52"/>
      <source>key (string)</source>
      <translation>Schlüsselwert (String)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="53"/>
      <source>index (integer number)</source>
      <translation>Index (Zahl)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="54"/>
      <source>table (use &quot;Add Group&quot; to create)</source>
      <translation>Tabelle (benutze &quot;Gruppe hinzufügen&quot; zum Erstellen)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="55"/>
      <source>function (cannot create from GUI)</source>
      <translation>Funktion (nicht in GUI erstellbar)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="79"/>
      <source>string</source>
      <translation>String</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="80"/>
      <source>number</source>
      <translation>Zahl</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="81"/>
      <source>boolean</source>
      <translation>Boolean</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="82"/>
      <source>table</source>
      <translation>Tabelle</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="83"/>
      <source>function</source>
      <translation>Funktion</translation>
    </message>
  </context>
  <context>
    <name>edbee::TextEditorComponent</name>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="578"/>
      <source>Cut</source>
      <translation>Ausschneiden</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="579"/>
      <source>Copy</source>
      <translation>Kopieren</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="580"/>
      <source>Paste</source>
      <translation>Einfügen</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="582"/>
      <source>Select All</source>
      <translation>Alles auswählen</translation>
    </message>
  </context>
  <context>
    <name>irc</name>
    <message>
      <location filename="../src/ui/irc.ui" line="25"/>
      <source>Mudlet IRC Client</source>
      <translation>Mudlet IRC-Client</translation>
    </message>
  </context>
  <context>
    <name>keybindings_main_area</name>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your key or key group. This will be displayed in the key tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen guten, bestenfalls einzigartigen Namen für deine Taste oder die Gruppe. Dieser wird in der Liste angezeigt werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="40"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="50"/>
      <source>&lt;p&gt;Type in one or more commands you want the key to send directly to the game when pressed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die die Taste direkt an das Spiel senden soll, wenn sie gedrückt wird. (Optional)&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="53"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Text, der unverändert ans Spiel gesendet werden soll (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="60"/>
      <source>Key Binding:</source>
      <translation>Tastenkürzel:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="77"/>
      <source>Grab New Key</source>
      <translation>Taste erkennen</translation>
    </message>
  </context>
  <context>
    <name>lacking_mapper_script</name>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="23"/>
      <source>No mapping script found</source>
      <translation>Kein Skript zum Kartieren gefunden</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="35"/>
      <source>&lt;p&gt;It seems that you don&apos;t have any &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;mapping scripts&lt;/a&gt; installed yet - the mapper needs you to have one for your game, so it can track where you are and autowalk you. You can either make one yourself, or import an existing one that someone else made.&lt;/p&gt;&lt;p&gt;Would you like to see if any are available?&lt;/p&gt;</source>
      <translation>&lt;p&gt;Es scheint, dass noch keine &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;Kartenskripte&lt;/a&gt; installiert sind - Der Mapper braucht eins für dieses Spiel, um deine Bewegungen zu verfolgen und dich automatisch bewegen zu können. Du kannst entweder selbst eins erstellen oder ein vorhandenes importieren, das jemand anderes erstellt hat.&lt;/p&gt;&lt;p&gt;Würdest du gerne prüfen, ob welche zur Verfügung stehen?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="86"/>
      <source>Close</source>
      <translation>Schließen</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="93"/>
      <source>Find some scripts</source>
      <translation>Finde einige Skripte</translation>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <location filename="../src/main.cpp" line="169"/>
      <source>Profile to open automatically</source>
      <translation>Profil, das automatisch geöffnet werden soll</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="169"/>
      <source>profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="172"/>
      <location filename="../src/main.cpp" line="178"/>
      <source>Display help and exit</source>
      <translation>Hilfe anzeigen und beenden</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="175"/>
      <source>Display version and exit</source>
      <translation>Version anzeigen und beenden</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="188"/>
      <source>Usage: %1 [OPTION...]
       -h, --help           displays this message.
       -v, --version        displays version information.
       -q, --quiet          no splash screen on startup.
       --profile=&lt;profile&gt;  additional profile to open

There are other inherited options that arise from the Qt Libraries which are
less likely to be useful for normal use of this application:
</source>
      <translation>Verwendung: %1 [OPTION...]
       -h, --help           zeigt diese Meldung.
       -v, --version        zeigt Versionsinformationen an.
       -q, --quiet          kein Splashscreen beim Start.
       --profile=&lt;Profil>  Profil, das zusätzlich geöffnet werden soll.

Es gibt andere Optionen, die aus den Qt-Bibliotheken übernommen wurden, die 
wahrscheinlich weniger nützlich für die normale Verwendung dieser Anwendung sind:
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="245"/>
      <source>%1 %2%3 (with debug symbols, without optimisations)
</source>
      <comment>%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev</comment>
      <translation>%1 %2%3 (mit Debug-Symbolen, ohne Optimierungen)
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="251"/>
      <source>Qt libraries %1 (compilation) %2 (runtime)
</source>
      <comment>%1 and %2 are version numbers</comment>
      <translation>Qt-Bibliotheken %1 (Kompilierung) %2 (Laufzeit)
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="255"/>
      <source>Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html
</source>
      <translation>Lizenz GPLv2+: GNU GPL Version 2 oder höher - http://gnu.org/licenses/gpl.html
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="256"/>
      <source>This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
</source>
      <translation>Dies ist freie Software: Du darfst sie verändern und weiterverbreiten.
Es gibt KEINE GARANTIE soweit gesetzlich zulässig.
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="236"/>
      <source>Report bugs to: https://github.com/Mudlet/Mudlet/issues
</source>
      <translation>Berichte Fehler an: https://github.com/Mudlet/Mudlet/issues
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="237"/>
      <source>Project home page: http://www.mudlet.org/
</source>
      <translation>Webseite des Projekts: http://www.mudlet.org/
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="254"/>
      <source>Copyright © 2008-2021  Mudlet developers
</source>
      <translation>Copyright © 2008-2021 Mudlet-Entwickler
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="280"/>
      <source>Version: %1</source>
      <translation>Version: %1</translation>
    </message>
  </context>
  <context>
    <name>main_window</name>
    <message>
      <location filename="../src/ui/main_window.ui" line="95"/>
      <source>Toolbox</source>
      <translation>Werkzeuge</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="109"/>
      <source>Options</source>
      <translation>Optionen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="116"/>
      <source>Help</source>
      <translation>Hilfe</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="126"/>
      <source>About</source>
      <translation>Über</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="134"/>
      <source>Games</source>
      <translation>Spiele</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="149"/>
      <source>Play</source>
      <translation>Spielen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="152"/>
      <source>&lt;p&gt;Configure connection details of, and make a connection to, game servers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Konfiguriere die Details der Verbindungen und verbinde zu Spielservern.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="160"/>
      <source>&lt;p&gt;Disconnect from the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Trenne die Verbindung zum aktuellen Spielserver.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="168"/>
      <source>&lt;p&gt;Disconnect and then reconnect to the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Trenne und verbinde dann wieder zum aktuellen Spielserver.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="176"/>
      <source>&lt;p&gt;Configure setting for the Mudlet application globally and for the current profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Konfiguriere Einstellungen für die Mudlet Anwendung insgesamt und für das aktuelle Profil.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="184"/>
      <source>&lt;p&gt;Opens the Editor for the different types of things that can be scripted by the user.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffne den Editor für die verschiedenen Arten von Dingen, für die ein Benutzer ein Skript erstellen kann&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="213"/>
      <source>IRC</source>
      <translation>IRC</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="216"/>
      <location filename="../src/ui/main_window.ui" line="253"/>
      <source>&lt;p&gt;Opens a built-in IRC chat on the #mudlet channel on Freenode IRC servers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffne einen eingebauten IRC-Client auf dem #mudlet Kanal der Freenode IRC-Server.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="224"/>
      <source>&lt;p&gt;Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffne eine Online-Sammlung von &quot;Mudlet Lernvideos&quot; mit dem Webbrowser deines Systems.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="232"/>
      <source>&lt;p&gt;Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Lade eine zuvor gespeicherte Spielsitzung, mit der Mudlets Lua Systeme (offline!) getestet werden können.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="240"/>
      <source>&lt;p&gt;Opens the (on-line) Mudlet Forum in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffne das Mudlet Online-Forum mit dem Webbrowser deines Systems.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="261"/>
      <source>&lt;p&gt;Show or hide the game map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zeige oder verstecke die Karte des Spiels.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="269"/>
      <source>&lt;p&gt;Install and remove collections of Mudlet lua items (packages).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Installiere und entferne Sammlungen von Mudlet Lua-Elementen (Pakete).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="277"/>
      <source>&lt;p&gt;Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Installiere und entferne (gemeinsam nutzbare und synchronisierbare) Sammlungen von Mudlet Lua-Elementen (Module).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="285"/>
      <source>&lt;p&gt;Gather and bundle up collections of Mudlet Lua items and other reasources into a module.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Sammle und bündle einige Mudlet Lua-Elemente und anderen Ressourcen in ein Modul.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="316"/>
      <source>&lt;p&gt;Hide / show the search area and buttons at the bottom of the screen.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Suchbereich und Schaltflächen am unteren Bildschirmrand ein- und ausblenden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="321"/>
      <source>Discord help channel</source>
      <translation>Hilfskanal in Discord</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="326"/>
      <location filename="../src/ui/main_window.ui" line="329"/>
      <source>Report an issue</source>
      <translation>Problem melden</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="332"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation>Die öffentliche Testversion bringt Dir schneller neue Funktionen, und Du hilfst uns, Probleme darin schneller zu finden. Hast Du etwas Merkwürdiges entdeckt? Lass es uns so schnell wie möglich wissen!</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="157"/>
      <source>Disconnect</source>
      <translation>Trennen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="165"/>
      <source>Reconnect</source>
      <translation>Neu verbinden</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="173"/>
      <source>Preferences</source>
      <translation>Einstellungen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="181"/>
      <source>Script editor</source>
      <translation>Skript-Editor</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="189"/>
      <source>Notepad</source>
      <translation>Notizblock</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="197"/>
      <source>API Reference</source>
      <translation>API-Referenz</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="237"/>
      <source>Online forum</source>
      <translation>Online-Forum</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="205"/>
      <source>About Mudlet</source>
      <translation>Über Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="192"/>
      <source>&lt;p&gt;Opens a free form text editor window for this profile that is saved between sessions.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffne einen formlosen Texteditor für dieses Profil, der zwischen den Sitzungen gespeichert wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="200"/>
      <source>&lt;p&gt;Opens the Mudlet manual in your web browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Öffnet das Mudlet-Handbuch in deinem Webbrowser.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="208"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation>&lt;p&gt;Informiere dich über diese Version von Mudlet, die Menschen, die daran beteiligt waren, und die Lizenz, unter der du es teilen kannst.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="221"/>
      <source>Video tutorials</source>
      <translation>Videoanleitungen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="229"/>
      <source>Load replay</source>
      <translation>Wiederholung laden</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="245"/>
      <source>Check for updates...</source>
      <translation>Nach Updates suchen...</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="250"/>
      <source>Live help chat</source>
      <translation>Sofortiger Hilfe-Chat</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="258"/>
      <source>Show map</source>
      <translation>Karte anzeigen</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="266"/>
      <source>Package manager</source>
      <translation>Pakete</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="274"/>
      <source>Module manager</source>
      <translation>Module</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="282"/>
      <source>Package exporter (experimental)</source>
      <translation>Paket exportieren (experimentell)</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="299"/>
      <source>MultiView</source>
      <translation>Multi-Sicht</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="302"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation>&lt;p&gt;Teilt den Mudlet-Bildschirm auf, um mehrere Profile gleichzeitig anzuzeigen. Deaktiviert, wenn weniger als zwei geladen sind.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="307"/>
      <source>Compact input line</source>
      <translation>Kompakte Eingabezeile</translation>
    </message>
  </context>
  <context>
    <name>mapper</name>
    <message>
      <location filename="../src/ui/mapper.ui" line="60"/>
      <source>^</source>
      <translation>^</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="452"/>
      <source>Area:</source>
      <translation>Gebiet:</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="521"/>
      <source>Rooms</source>
      <translation>Räume</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="550"/>
      <source>Exits</source>
      <translation>Ausgänge</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="579"/>
      <source>Round</source>
      <translation>Rund</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="586"/>
      <source>Info</source>
      <translation>Info</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="602"/>
      <source>IDs</source>
      <translation>IDs</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="618"/>
      <source>Names</source>
      <translation>Namen</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="771"/>
      <source>top + 1</source>
      <translation>oben + 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="745"/>
      <source>bottom + 1</source>
      <translation>unten + 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="758"/>
      <source>bottom -1</source>
      <translation>unten - 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="784"/>
      <source>top - 1</source>
      <translation>oben - 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="732"/>
      <source>1 level</source>
      <translation>Ebene 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="427"/>
      <source>3D</source>
      <translation>3D</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="680"/>
      <source>default</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="693"/>
      <source>top view</source>
      <translation>von oben</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="706"/>
      <source>side view</source>
      <translation>von Seite</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="719"/>
      <source>all levels</source>
      <translation>alle Ebenen</translation>
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
&lt;i&gt;Note: &lt;b&gt;.zip&lt;/b&gt; and &lt;b&gt;.mpackage&lt;/b&gt; modules are currently unable to be synced, only &lt;b&gt;.xml&lt;/b&gt; packages are able to be synchronized across profiles at the moment.&lt;/i&gt;&lt;/p&gt;
&lt;p&gt;For each save operation, modules are backed up to a directory, &lt;i&gt;moduleBackups&lt;/i&gt;, within your Mudlet profile directory.&lt;/p&gt;
&lt;/body&gt;&lt;/html&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="129"/>
      <source>Uninstall</source>
      <translation>Deinstallieren</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="142"/>
      <source>Install</source>
      <translation>Installieren</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="149"/>
      <source>Module Help</source>
      <translation>Hilfe zum Modul</translation>
    </message>
  </context>
  <context>
    <name>mudlet</name>
    <message>
      <location filename="../src/mudlet.cpp" line="716"/>
      <source>Afrikaans</source>
      <extracomment>In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a &apos;-&apos; Chinese is an exception!</extracomment>
      <translation>Afrikaans</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="717"/>
      <source>Afrikaans (South Africa)</source>
      <translation>Afrikaans (Südafrika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="718"/>
      <source>Aragonese</source>
      <translation>Aragonese</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="719"/>
      <source>Aragonese (Spain)</source>
      <translation>Aragon (Spanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="720"/>
      <source>Arabic</source>
      <translation>Arabisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="721"/>
      <source>Arabic (United Arab Emirates)</source>
      <translation>Arabisch (Vereinigte Arabische Emirate)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="722"/>
      <source>Arabic (Bahrain)</source>
      <translation>Arabisch (Bahrain)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="723"/>
      <source>Arabic (Algeria)</source>
      <translation>Arabisch (Algerien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="725"/>
      <source>Arabic (India)</source>
      <translation>Arabisch (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="726"/>
      <source>Arabic (Iraq)</source>
      <translation>Arabisch (Irak)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="727"/>
      <source>Arabic (Jordan)</source>
      <translation>Arabisch (Jordanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="728"/>
      <source>Arabic (Kuwait)</source>
      <translation>Arabisch (Kuwait)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="729"/>
      <source>Arabic (Lebanon)</source>
      <translation>Arabisch (Libanon)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="730"/>
      <source>Arabic (Libya)</source>
      <translation>Arabisch (Libyen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="731"/>
      <source>Arabic (Morocco)</source>
      <translation>Arabisch (Marokko)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="732"/>
      <source>Arabic (Oman)</source>
      <translation>Arabisch (Oman)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="733"/>
      <source>Arabic (Qatar)</source>
      <translation>Arabisch (Katar)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="734"/>
      <source>Arabic (Saudi Arabia)</source>
      <translation>Arabisch (Saudi Arabien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="735"/>
      <source>Arabic (Sudan)</source>
      <translation>Arabisch (Sudan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="736"/>
      <source>Arabic (Syria)</source>
      <translation>Arabisch (Syrien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="737"/>
      <source>Arabic (Tunisia)</source>
      <translation>Arabisch (Tunesien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="738"/>
      <source>Arabic (Yemen)</source>
      <translation>Arabisch (Jemen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="739"/>
      <source>Belarusian</source>
      <translation>Weißrussisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="740"/>
      <source>Belarusian (Belarus)</source>
      <translation>Weißrussisch (Weißrussland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="741"/>
      <source>Belarusian (Russia)</source>
      <translation>Weißrussisch (Russland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="742"/>
      <source>Bulgarian</source>
      <translation>Bulgarisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="743"/>
      <source>Bulgarian (Bulgaria)</source>
      <translation>Bulgarisch (Bulgarien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="744"/>
      <source>Bangla</source>
      <translation>Bangla</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="745"/>
      <source>Bangla (Bangladesh)</source>
      <translation>Bangla (Bangladesch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="746"/>
      <source>Bangla (India)</source>
      <translation>Bangla (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="747"/>
      <source>Tibetan</source>
      <translation>Tibetisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="748"/>
      <source>Tibetan (China)</source>
      <translation>Tibetisch (China)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="749"/>
      <source>Tibetan (India)</source>
      <translation>Tibetisch (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="750"/>
      <source>Breton</source>
      <translation>Breton</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="751"/>
      <source>Breton (France)</source>
      <translation>Breton (Frankreich)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="752"/>
      <source>Bosnian</source>
      <translation>Bosnisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="753"/>
      <source>Bosnian (Bosnia/Herzegovina)</source>
      <translation>Bosnisch (Bosnien/Herzegowina)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="754"/>
      <source>Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)</source>
      <translation>Bosnisch (Bosnien/Herzegowina - kyrillisches Alphabet)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="755"/>
      <source>Catalan</source>
      <translation>Katalanisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="756"/>
      <source>Catalan (Spain)</source>
      <translation>Katalanisch (Spanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="757"/>
      <source>Catalan (Spain - Valencian)</source>
      <translation>Katalanisch (Spanien-Valencianisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="758"/>
      <source>Central Kurdish</source>
      <translation>Mittelkurdisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="759"/>
      <source>Central Kurdish (Iraq)</source>
      <translation>Mittelkurdisch (Irak)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="760"/>
      <source>Czech</source>
      <translation>Tschechisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="761"/>
      <source>Czech (Czechia)</source>
      <translation>Tschechisch (Tschechei)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="762"/>
      <source>Danish</source>
      <translation>Dänisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="763"/>
      <source>Danish (Denmark)</source>
      <translation>Dänisch (Dänemark)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="764"/>
      <source>German</source>
      <translation>Deutsch (German)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="765"/>
      <source>German (Austria)</source>
      <translation>Deutsch (Österreich)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="766"/>
      <source>German (Austria, revised by F M Baumann)</source>
      <translation>Deutsch (Österreich, überarbeitet von F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="767"/>
      <source>German (Belgium)</source>
      <translation>Deutsch (Belgien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="768"/>
      <source>German (Switzerland)</source>
      <translation>Deutsch (Schweiz)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="769"/>
      <source>German (Switzerland, revised by F M Baumann)</source>
      <translation>Deutsch (Schweiz, überarbeitet von F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="770"/>
      <source>German (Germany/Belgium/Luxemburg)</source>
      <translation>Deutsch (Deutschland/Belgien/Luxemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="771"/>
      <source>German (Germany/Belgium/Luxemburg, revised by F M Baumann)</source>
      <translation>Deutsch (Deutschland/Belgien/Luxemburg, überarbeitet von F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="772"/>
      <source>German (Liechtenstein)</source>
      <translation>Deutsch (Liechtenstein)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="773"/>
      <source>German (Luxembourg)</source>
      <translation>Deutsch (Luxemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="774"/>
      <source>Greek</source>
      <translation>Griechisch (Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="775"/>
      <source>Greek (Greece)</source>
      <translation>Griechisch (Griechenland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="776"/>
      <source>English</source>
      <translation>Englisch (American English)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="777"/>
      <source>English (Antigua/Barbuda)</source>
      <translation>Englisch (Antigua/Barbuda)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="778"/>
      <source>English (Australia)</source>
      <translation>Englisch (Australien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="780"/>
      <source>English (Bahamas)</source>
      <translation>Englisch (Bahamas)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="781"/>
      <source>English (Botswana)</source>
      <translation>Englisch (Botswana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="782"/>
      <source>English (Belize)</source>
      <translation>Englisch (Belize)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="724"/>
      <source>Arabic (Egypt)</source>
      <translation>Arabisch (Ägypten)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="478"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation>&lt;p&gt;Teilt den Mudlet-Bildschirm auf, um mehrere Profile gleichzeitig anzuzeigen. Deaktiviert, wenn weniger als zwei geladen sind.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="490"/>
      <source>Report issue</source>
      <translation>Problem melden</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="779"/>
      <source>English (Australia, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>Englisch (Australien, groß)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="783"/>
      <source>English (Canada)</source>
      <translation>Englisch (Kanada)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="784"/>
      <source>English (Canada, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>Englisch (Kanada, groß)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="785"/>
      <source>English (Denmark)</source>
      <translation>Englisch (Dänemark)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="786"/>
      <source>English (United Kingdom)</source>
      <translation>Englisch (Großbritannien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="787"/>
      <source>English (United Kingdom, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>Englisch (Großbritannien, groß)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="788"/>
      <source>English (United Kingdom - &apos;ise&apos; not &apos;ize&apos;)</source>
      <comment>This dictionary prefers the British &apos;ise&apos; form over the American &apos;ize&apos; one.</comment>
      <translation>Englisch (Großbritannien - &apos;ise&apos; nicht &apos;ize&apos;)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="789"/>
      <source>English (Ghana)</source>
      <translation>Englisch (Ghana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="790"/>
      <source>English (Hong Kong SAR China)</source>
      <translation>Englisch (Sonderverwaltungszone Hongkong)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="791"/>
      <source>English (Ireland)</source>
      <translation>Englisch (Irland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="792"/>
      <source>English (India)</source>
      <translation>Englisch (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="793"/>
      <source>English (Jamaica)</source>
      <translation>Englisch (Jamaika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="794"/>
      <source>English (Namibia)</source>
      <translation>Englisch (Namibia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="795"/>
      <source>English (Nigeria)</source>
      <translation>Englisch (Nigeria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="796"/>
      <source>English (New Zealand)</source>
      <translation>Englisch (Neuseeland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="797"/>
      <source>English (Philippines)</source>
      <translation>Englisch (Philippinen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="798"/>
      <source>English (Singapore)</source>
      <translation>Englisch (Singapur)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="799"/>
      <source>English (Trinidad/Tobago)</source>
      <translation>Englisch (Trinidad/Tobago)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="800"/>
      <source>English (United States)</source>
      <translation>Englisch (USA)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="801"/>
      <source>English (United States, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>Englisch (USA, groß)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="802"/>
      <source>English (South Africa)</source>
      <translation>Englisch (Südafrika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="803"/>
      <source>English (Zimbabwe)</source>
      <translation>Englisch (Simbabwe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="804"/>
      <source>Spanish</source>
      <translation>Spanisch (Spanish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="805"/>
      <source>Spanish (Argentina)</source>
      <translation>Spanisch (Argentinien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="806"/>
      <source>Spanish (Bolivia)</source>
      <translation>Spanisch (Bolivien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="807"/>
      <source>Spanish (Chile)</source>
      <translation>Spanisch (Chile)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="808"/>
      <source>Spanish (Colombia)</source>
      <translation>Spanisch (Kolumbien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="809"/>
      <source>Spanish (Costa Rica)</source>
      <translation>Spanisch (Costa Rica)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="810"/>
      <source>Spanish (Cuba)</source>
      <translation>Spanisch (Kuba)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="811"/>
      <source>Spanish (Dominican Republic)</source>
      <translation>Spanisch (Dominikanische Republik)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="812"/>
      <source>Spanish (Ecuador)</source>
      <translation>Spanisch (Ecuador)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="813"/>
      <source>Spanish (Spain)</source>
      <translation>Spanisch (Spanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="814"/>
      <source>Spanish (Guatemala)</source>
      <translation>Spanisch (Guatemala)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="815"/>
      <source>Spanish (Honduras)</source>
      <translation>Spanisch (Honduras)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="816"/>
      <source>Spanish (Mexico)</source>
      <translation>Spanisch (Mexiko)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="817"/>
      <source>Spanish (Nicaragua)</source>
      <translation>Spanisch (Nicaragua)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="818"/>
      <source>Spanish (Panama)</source>
      <translation>Spanisch (Panama)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="819"/>
      <source>Spanish (Peru)</source>
      <translation>Spanisch (Peru)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="820"/>
      <source>Spanish (Puerto Rico)</source>
      <translation>Spanisch (Puerto Rico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="821"/>
      <source>Spanish (Paraguay)</source>
      <translation>Spanisch (Paraguay)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="822"/>
      <source>Spanish (El Savador)</source>
      <translation>Spanisch (El Savador)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="823"/>
      <source>Spanish (United States)</source>
      <translation>Spanisch (USA)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="824"/>
      <source>Spanish (Uruguay)</source>
      <translation>Spanisch (Uruguay)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="825"/>
      <source>Spanish (Venezuela)</source>
      <translation>Spanisch (Venezuela)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="826"/>
      <source>Estonian</source>
      <translation>Estnisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="827"/>
      <source>Estonian (Estonia)</source>
      <translation>Estnisch (Estland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="828"/>
      <source>Basque</source>
      <translation>Baskisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="829"/>
      <source>Basque (Spain)</source>
      <translation>Baskisch (Spanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="830"/>
      <source>Basque (France)</source>
      <translation>Baskisch (Frankreich)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="831"/>
      <source>French</source>
      <translation>Französisch (French)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="832"/>
      <source>French (Belgium)</source>
      <translation>Französisch (Belgien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="833"/>
      <source>French (Catalan)</source>
      <translation>Französisch (Katalanisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="834"/>
      <source>French (Switzerland)</source>
      <translation>Französisch (Schweiz)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="835"/>
      <source>French (France)</source>
      <translation>Französisch (Frankreich)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="836"/>
      <source>French (Luxemburg)</source>
      <translation>Französisch (Luxemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="837"/>
      <source>French (Monaco)</source>
      <translation>Französisch (Monaco)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="838"/>
      <source>Gaelic</source>
      <translation>Gälisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="839"/>
      <source>Gaelic (United Kingdom {Scots})</source>
      <translation>Gälisch (Vereinigtes Königreich {Scots})</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="840"/>
      <source>Galician</source>
      <translation>Galizisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="841"/>
      <source>Galician (Spain)</source>
      <translation>Galizisch (Spanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="842"/>
      <source>Gujarati</source>
      <translation>Gujarati</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="843"/>
      <source>Gujarati (India)</source>
      <translation>Gujarati (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="844"/>
      <source>Hebrew</source>
      <translation>Hebräisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="845"/>
      <source>Hebrew (Israel)</source>
      <translation>Hebräisch (Israel)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="846"/>
      <source>Hindi</source>
      <translation>Hindi</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="847"/>
      <source>Hindi (India)</source>
      <translation>Hindi (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="848"/>
      <source>Croatian</source>
      <translation>Kroatisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="849"/>
      <source>Croatian (Croatia)</source>
      <translation>Kroatisch (Kroatien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="850"/>
      <source>Hungarian</source>
      <translation>Ungarisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="851"/>
      <source>Hungarian (Hungary)</source>
      <translation>Ungarisch (Ungarn)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="852"/>
      <source>Armenian</source>
      <translation>Armenisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="853"/>
      <source>Armenian (Armenia)</source>
      <translation>Armenisch (Armenien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="854"/>
      <source>Interlingue</source>
      <comment>formerly known as Occidental, and not to be mistaken for Interlingua</comment>
      <translation>Interlingue</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="855"/>
      <source>Icelandic</source>
      <translation>Isländisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="856"/>
      <source>Icelandic (Iceland)</source>
      <translation>Isländisch (Island)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="857"/>
      <source>Italian</source>
      <translation>Italienisch (Italian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="858"/>
      <source>Italian (Switzerland)</source>
      <translation>Italienisch (Schweiz)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="859"/>
      <source>Italian (Italy)</source>
      <translation>Italienisch (Italien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="860"/>
      <source>Kazakh</source>
      <translation>Kasachisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="861"/>
      <source>Kazakh (Kazakhstan)</source>
      <translation>Kasachisch (Kasachstan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="862"/>
      <source>Kurmanji</source>
      <translation>Kurmanji</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="863"/>
      <source>Kurmanji {Latin-alphabet Kurdish}</source>
      <translation>Kurmanji (Kurdisch mit lateinischem Alphabet)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="864"/>
      <source>Korean</source>
      <translation>Koreanisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="865"/>
      <source>Korean (South Korea)</source>
      <translation>Koreanisch (Südkorea)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="866"/>
      <source>Kurdish</source>
      <translation>Kurdisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="867"/>
      <source>Kurdish (Syria)</source>
      <translation>Kurdisch (Syrien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="868"/>
      <source>Kurdish (Turkey)</source>
      <translation>Kurdisch (Türkei)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="869"/>
      <source>Lao</source>
      <translation>Lao</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="870"/>
      <source>Lao (Laos)</source>
      <translation>Lao (Laos)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="871"/>
      <source>Lithuanian</source>
      <translation>Litauisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="872"/>
      <source>Lithuanian (Lithuania)</source>
      <translation>Litauisch (Litauen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="873"/>
      <source>Malayalam</source>
      <translation>Malayalam</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="874"/>
      <source>Malayalam (India)</source>
      <translation>Malayalam (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="875"/>
      <source>Norwegian Bokmål</source>
      <translation>Norwegisch Bokmål</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="876"/>
      <source>Norwegian Bokmål (Norway)</source>
      <translation>Norwegisch Bokmål (Norwegen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="877"/>
      <source>Nepali</source>
      <translation>Nepalisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="878"/>
      <source>Nepali (Nepal)</source>
      <translation>Nepalisch (Nepal)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="879"/>
      <source>Dutch</source>
      <translation>Niederländisch (Dutch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="880"/>
      <source>Dutch (Netherlands Antilles)</source>
      <translation>Niederländisch (Niederländische Antillen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="881"/>
      <source>Dutch (Aruba)</source>
      <translation>Niederländisch (Aruba)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="882"/>
      <source>Dutch (Belgium)</source>
      <translation>Niederländisch (Belgien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="883"/>
      <source>Dutch (Netherlands)</source>
      <translation>Niederländisch (Niederlande)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="884"/>
      <source>Dutch (Suriname)</source>
      <translation>Niederländisch (Surinam)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="885"/>
      <source>Norwegian Nynorsk</source>
      <translation>Norwegisch Nynorsk</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="886"/>
      <source>Norwegian Nynorsk (Norway)</source>
      <translation>Norwegisch Nynorsk (Norwegen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="887"/>
      <source>Occitan</source>
      <translation>Okzitanisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="888"/>
      <source>Occitan (France)</source>
      <translation>Okzitanisch (Frankreich)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="889"/>
      <source>Polish</source>
      <translation>Polnisch (Polish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="890"/>
      <source>Polish (Poland)</source>
      <translation>Polnisch (Polen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="891"/>
      <source>Portuguese</source>
      <translation>Portugiesisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="892"/>
      <source>Portuguese (Brazil)</source>
      <translation>Portugiesisch (Brasilien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="893"/>
      <source>Portuguese (Portugal)</source>
      <translation>Portugiesisch (Portugal)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="894"/>
      <source>Romanian</source>
      <translation>Rumänisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="895"/>
      <source>Romanian (Romania)</source>
      <translation>Rumänisch (Rumänien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="896"/>
      <source>Russian</source>
      <translation>Russisch (Russian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="897"/>
      <source>Russian (Russia)</source>
      <translation>Russisch (Russland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="898"/>
      <source>Northern Sami</source>
      <translation>Nord-Sami</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="899"/>
      <source>Northern Sami (Finland)</source>
      <translation>Nord-Sami (Finnland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="900"/>
      <source>Northern Sami (Norway)</source>
      <translation>Nord-Sami (Norwegen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="901"/>
      <source>Northern Sami (Sweden)</source>
      <translation>Nord-Sami (Schweden)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="902"/>
      <source>Shtokavian</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication</comment>
      <translation>Shtokavian</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="903"/>
      <source>Shtokavian (former state of Yugoslavia)</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication</comment>
      <translation>Shtokavian (ehemaliger Staat Jugoslawiens)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="904"/>
      <source>Sinhala</source>
      <translation>Sinhala</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="905"/>
      <source>Sinhala (Sri Lanka)</source>
      <translation>Sinhala (Sri Lanka)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="906"/>
      <source>Slovak</source>
      <translation>Slovakisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="907"/>
      <source>Slovak (Slovakia)</source>
      <translation>Slowakisch (Slowakei)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="908"/>
      <source>Slovenian</source>
      <translation>Slowenisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="909"/>
      <source>Slovenian (Slovenia)</source>
      <translation>Slowenisch (Slowenien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="910"/>
      <source>Somali</source>
      <translation>Somali</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="911"/>
      <source>Somali (Somalia)</source>
      <translation>Somali (Somalia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="912"/>
      <source>Albanian</source>
      <translation>Albanisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="913"/>
      <source>Albanian (Albania)</source>
      <translation>Albanisch (Albanien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="914"/>
      <source>Serbian</source>
      <translation>Serbisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="915"/>
      <source>Serbian (Montenegro)</source>
      <translation>Serbisch (Montenegro)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="916"/>
      <source>Serbian (Serbia)</source>
      <translation>Serbisch (Serbien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="917"/>
      <source>Serbian (Serbia - Latin-alphabet)</source>
      <translation>Serbisch (Serbien - lateinisches Alphabet)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="918"/>
      <source>Serbian (former state of Yugoslavia)</source>
      <translation>Serbisch (ehemaliger Staat Jugoslawien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="919"/>
      <source>Swati</source>
      <translation>Swati</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="920"/>
      <source>Swati (Swaziland)</source>
      <translation>Swati (Swasiland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="921"/>
      <source>Swati (South Africa)</source>
      <translation>Swati (Südafrika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="922"/>
      <source>Swedish</source>
      <translation>Schwedisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="923"/>
      <source>Swedish (Sweden)</source>
      <translation>Schwedisch (Schweden)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="924"/>
      <source>Swedish (Finland)</source>
      <translation>Schwedisch (Finnland)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="925"/>
      <source>Swahili</source>
      <translation>Swahili</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="926"/>
      <source>Swahili (Kenya)</source>
      <translation>Swahili (Kenia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="927"/>
      <source>Swahili (Tanzania)</source>
      <translation>Swahili (Tansania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="928"/>
      <source>Turkish</source>
      <translation>Türkisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="929"/>
      <source>Telugu</source>
      <translation>Telugu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="930"/>
      <source>Telugu (India)</source>
      <translation>Telugu (Indien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="931"/>
      <source>Thai</source>
      <translation>Thailändisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="932"/>
      <source>Thai (Thailand)</source>
      <translation>Thailändisch (Thailand)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="933"/>
      <source>Tigrinya</source>
      <translation>Tigrinya</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="934"/>
      <source>Tigrinya (Eritrea)</source>
      <translation>Tigrinya (Eritrea)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="935"/>
      <source>Tigrinya (Ethiopia)</source>
      <translation>Tigrinya (Äthiopien)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="936"/>
      <source>Turkmen</source>
      <translation>Turkmenisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="937"/>
      <source>Turkmen (Turkmenistan)</source>
      <translation>Turkmenisch (Turkmenistan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="938"/>
      <source>Tswana</source>
      <translation>Tswana</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="939"/>
      <source>Tswana (Botswana)</source>
      <translation>Tswana (Botswana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="940"/>
      <source>Tswana (South Africa)</source>
      <translation>Tswana (Südafrika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="941"/>
      <source>Tsonga</source>
      <translation>Tsonga</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="942"/>
      <source>Tsonga (South Africa)</source>
      <translation>Tsonga (Südafrika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="943"/>
      <source>Ukrainian</source>
      <translation>Ukrainisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="944"/>
      <source>Ukrainian (Ukraine)</source>
      <translation>Ukrainisch (Ukraine)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="945"/>
      <source>Uzbek</source>
      <translation>Usbekisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="946"/>
      <source>Uzbek (Uzbekistan)</source>
      <translation>Usbekisch (Usbekistan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="947"/>
      <source>Venda</source>
      <translation>Venda</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="948"/>
      <source>Vietnamese</source>
      <translation>Vietnamesich</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="949"/>
      <source>Vietnamese (Vietnam)</source>
      <translation>Vietnamesisch (Vietnam)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="950"/>
      <source>Vietnamese (DauCu varient - old-style diacritics)</source>
      <translation>Vietnamesisch (DauCu Variante - alte Diakritika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="951"/>
      <source>Vietnamese (DauMoi varient - new-style diacritics)</source>
      <translation>Vietnamesisch (DauMoi Variante - neue Diakritika)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="952"/>
      <source>Walloon</source>
      <translation>Wallonisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="953"/>
      <source>Xhosa</source>
      <translation>Xhosa</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="954"/>
      <source>Yiddish</source>
      <translation>Jiddisch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="955"/>
      <source>Chinese</source>
      <translation>Chinesisch (Chinese)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="956"/>
      <source>Chinese (China - simplified)</source>
      <translation>Chinesisch (China - vereinfacht)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="957"/>
      <source>Chinese (Taiwan - traditional)</source>
      <translation>Chinesisch (Taiwan - traditionell)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="958"/>
      <source>Zulu</source>
      <translation>Zulu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4326"/>
      <source>Hide tray icon</source>
      <translation>Tray-Symbol ausblenden</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4331"/>
      <source>Quit Mudlet</source>
      <translation>Mudlet beenden</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="235"/>
      <source>hh:mm:ss</source>
      <comment>Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&amp;) for the gory details...!</comment>
      <translation>hh:mm:ss</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="287"/>
      <source>Main Toolbar</source>
      <translation>Hauptsymbolleiste</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="328"/>
      <location filename="../src/mudlet.cpp" line="335"/>
      <location filename="../src/mudlet.cpp" line="337"/>
      <source>Connect</source>
      <translation>Verbinden</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="340"/>
      <source>Disconnect</source>
      <translation>Trennen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="399"/>
      <source>Open Discord</source>
      <translation>Discord öffnen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="404"/>
      <source>Open IRC</source>
      <translation>IRC öffnen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="456"/>
      <source>Package Exporter</source>
      <translation>Paket exportieren</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="347"/>
      <source>Triggers</source>
      <translation>Trigger</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="348"/>
      <source>Show and edit triggers</source>
      <translation>Trigger anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="355"/>
      <source>Aliases</source>
      <translation>Aliase</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="356"/>
      <source>Show and edit aliases</source>
      <translation>Aliase anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="361"/>
      <source>Timers</source>
      <translation>Timer</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="362"/>
      <source>Show and edit timers</source>
      <translation>Timer anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="367"/>
      <source>Buttons</source>
      <translation>Buttons</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="368"/>
      <source>Show and edit easy buttons</source>
      <translation>Einfache Buttons anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="373"/>
      <source>Scripts</source>
      <translation>Skripte</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="374"/>
      <source>Show and edit scripts</source>
      <translation>Skripte anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="379"/>
      <source>Keys</source>
      <translation>Tasten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="380"/>
      <source>Show and edit keys</source>
      <translation>Tasten anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="385"/>
      <source>Variables</source>
      <translation>Variablen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="386"/>
      <source>Show and edit Lua variables</source>
      <translation>Lua Variablen anzeigen und bearbeiten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="412"/>
      <source>Map</source>
      <translation>Karte</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="413"/>
      <source>Show/hide the map</source>
      <translation>Zeige/Verberge die Karte</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="418"/>
      <source>Manual</source>
      <translation>Handbuch</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="419"/>
      <source>Browse reference material and documentation</source>
      <translation>Durchsuche Referenzmaterial und Dokumentation</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="424"/>
      <source>Settings</source>
      <translation>Einstellungen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="425"/>
      <source>See and edit profile preferences</source>
      <translation>Zeige und bearbeite Profileinstellungen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="433"/>
      <source>Notepad</source>
      <translation>Notiz</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="434"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation>Öffne einen Texteditor, um deine Notizen zu speichern</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="440"/>
      <location filename="../src/mudlet.cpp" line="447"/>
      <location filename="../src/mudlet.cpp" line="449"/>
      <source>Package Manager</source>
      <translation>Pakete</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="452"/>
      <source>Module Manager</source>
      <translation>Module</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="466"/>
      <source>Replay</source>
      <translation>Wiederholung</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="471"/>
      <source>Reconnect</source>
      <translation>Neu verbinden</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="472"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation>Trennt dich vom Spiel und verbindet wieder</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="477"/>
      <source>MultiView</source>
      <translation>Multi-Sicht</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="501"/>
      <location filename="../src/mudlet.cpp" line="3449"/>
      <source>About</source>
      <translation>Über</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="502"/>
      <location filename="../src/mudlet.cpp" line="3432"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation>&lt;p&gt;Informiere dich über diese Version von Mudlet, die Menschen, die daran beteiligt waren, und die Lizenz, unter der du es teilen kannst.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="960"/>
      <source>ASCII (Basic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ASCII (Basis)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="961"/>
      <source>UTF-8 (Recommended)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>UTF-8 (empfohlen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="966"/>
      <source>ISO 8859-1 (Western European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-1 (Westeuropa/Western European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="967"/>
      <source>ISO 8859-2 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-2 (Mitteleuropa/Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="968"/>
      <source>ISO 8859-3 (South European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-3 (Südeuropa/South European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="969"/>
      <source>ISO 8859-4 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-4 (Baltisch/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="970"/>
      <source>ISO 8859-5 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-5 (Kyrillisch/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="971"/>
      <source>ISO 8859-6 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-6 (Arabisch/Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="972"/>
      <source>ISO 8859-7 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-7 (Griechisch/Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="973"/>
      <source>ISO 8859-8 (Hebrew Visual)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-8 (Hebräisch Visuell/Hebrew Visual)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="974"/>
      <source>ISO 8859-9 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-9 (Türkisch/Turkish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="975"/>
      <source>ISO 8859-10 (Nordic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-10 (Nordisch/Nordic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="976"/>
      <source>ISO 8859-11 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-11 (Latin/Thai)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="977"/>
      <source>ISO 8859-13 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-13 (Baltisch/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="978"/>
      <source>ISO 8859-14 (Celtic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-14 (Keltisch/Celtic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="979"/>
      <source>ISO 8859-15 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-15 (Westlich/Western)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="980"/>
      <source>ISO 8859-16 (Romanian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-16 (Rumänisch/Romanian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="981"/>
      <location filename="../src/mudlet.cpp" line="982"/>
      <source>CP437 (OEM Font)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP437 (OEM-Schriftart)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="983"/>
      <location filename="../src/mudlet.cpp" line="984"/>
      <source>CP667 (Mazovia)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP667 (Masowien/Mazovia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="985"/>
      <location filename="../src/mudlet.cpp" line="986"/>
      <source>CP737 (DOS Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP737 (DOS-Griechisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="987"/>
      <source>CP850 (Western Europe)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP850 (Westeuropa)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="988"/>
      <source>CP866 (Cyrillic/Russian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP866 (Kyrillisch/Russisch / Cyrillic/Russian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="989"/>
      <location filename="../src/mudlet.cpp" line="990"/>
      <source>CP869 (DOS Greek 2)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP869 (DOS-Griechisch 2)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="991"/>
      <source>CP1161 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP1161 (Lateinisch/Thailändisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="992"/>
      <source>KOI8-R (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>KOI8-R (Kyrillisch/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="993"/>
      <source>KOI8-U (Cyrillic/Ukrainian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>KOI8-U (Kyrillisch/Ukrainisch / Cyrillic/Ukrainian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="994"/>
      <source>MACINTOSH</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>MACINTOSH</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="995"/>
      <source>WINDOWS-1250 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1250 (Mitteleuropa/Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="996"/>
      <source>WINDOWS-1251 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1251 (Kyrillisch/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="997"/>
      <source>WINDOWS-1252 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1252 (Westlich/Western)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="998"/>
      <source>WINDOWS-1253 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1253 (Griechisch / Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="999"/>
      <source>WINDOWS-1254 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1254 (Türkisch/Turkish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1000"/>
      <source>WINDOWS-1255 (Hebrew)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1255 (Hebräisch/Hebrew)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1001"/>
      <source>WINDOWS-1256 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1256 (Arabisch/Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1002"/>
      <source>WINDOWS-1257 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1257 (Baltisch/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1003"/>
      <source>WINDOWS-1258 (Vietnamese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1258 (Vietnamesisch/Vietnamese)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2493"/>
      <source>Central Debug Console</source>
      <translation>Zentrale Debug-Konsole</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="513"/>
      <location filename="../src/mudlet.cpp" line="514"/>
      <source>Toggle Full Screen View</source>
      <translation>Vollbildanzeige umschalten</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="494"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation>Die öffentliche Testversion bringt Dir schneller neue Funktionen, und Du hilfst uns, Probleme darin schneller zu finden. Hast Du etwas Merkwürdiges entdeckt? Lass es uns so schnell wie möglich wissen!</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="962"/>
      <source>GBK (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>GBK (Chinesisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="963"/>
      <source>GB18030 (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>GB18030 (Chinesisch)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="964"/>
      <source>Big5-ETen (Taiwan)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>Big5-ETen (Taiwan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="965"/>
      <source>Big5-HKSCS (Hong Kong)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>Big5-HKSCS (Hongkong)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1560"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Disabled until a profile is loaded.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Lade eine Mudlet Wiederholung.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Deaktiviert, bis ein Profil geladen ist.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1596"/>
      <location filename="../src/mudlet.cpp" line="2864"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Lade eine Mudlet Wiederholung.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2319"/>
      <source>%1 - notes</source>
      <translation>%1 - Notizen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2372"/>
      <source>Select Replay</source>
      <translation>Wiederholung auswählen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2374"/>
      <source>*.dat</source>
      <translation>*.dat</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2631"/>
      <source>[  OK  ]  - Profile &quot;%1&quot; loaded in offline mode.</source>
      <translation>[  OK  ]  - Profil &quot;%1&quot; im Offline-Modus geladen.</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2766"/>
      <source>&lt;p&gt;Cannot load a replay as one is already in progress in this or another profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wiederholung kann nicht geladen werden, da schon eine in diesem oder einem anderen Profil läuft.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2783"/>
      <source>Faster</source>
      <translation>Schneller</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2786"/>
      <source>&lt;p&gt;Replay each step with a shorter time interval between steps.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jeden Schritt mit kürzerem zeitlichen Abstand abspielen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2790"/>
      <source>Slower</source>
      <translation>Langsamer</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2793"/>
      <source>&lt;p&gt;Replay each step with a longer time interval between steps.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jeden Schritt mit längerem zeitlichen Abstand abspielen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2803"/>
      <location filename="../src/mudlet.cpp" line="2872"/>
      <location filename="../src/mudlet.cpp" line="2885"/>
      <source>Speed: X%1</source>
      <translation>Geschwindigkeit: X%1</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2810"/>
      <location filename="../src/mudlet.cpp" line="2827"/>
      <source>Time: %1</source>
      <translation>Zeit: %1</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3443"/>
      <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
      <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
      <translation>
        <numerusform>&lt;p&gt;Über Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;Eine neue Version ist jetzt verfügbar!&lt;/i&gt;&lt;p&gt;</numerusform>
        <numerusform>&lt;p&gt;Über Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n neue Versionen sind jetzt verfügbar!&lt;/i&gt;&lt;p&gt;</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3461"/>
      <source>Review %n update(s)...</source>
      <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
      <translation>
        <numerusform>1 Update vorhanden...</numerusform>
        <numerusform>%n Updates vorhanden...</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3466"/>
      <source>&lt;p&gt;Review the update(s) available...&lt;/p&gt;</source>
      <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
      <translation>
        <numerusform>&lt;p&gt;Prüfe die neue Version...&lt;/p&gt;</numerusform>
        <numerusform>&lt;p&gt;Prüfe die neue Versionen...&lt;/p&gt;</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3491"/>
      <source>Update installed - restart to apply</source>
      <translation>Update installiert - Neustart zum Übernehmen</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3527"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress,
try again when it has finished.</source>
      <translation>[ WARNUNG ] - Kann Wiederholung nicht ausführen. Eine andere könnte bereits laufen. Bitte wiederholen, nachdem diese endet.</translation>
    </message>
  </context>
  <context>
    <name>package_manager</name>
    <message>
      <location filename="../src/ui/package_manager.ui" line="48"/>
      <source>Install</source>
      <translation>Installieren</translation>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="61"/>
      <source>Uninstall</source>
      <translation>Deinstallieren</translation>
    </message>
  </context>
  <context>
    <name>package_manager_unpack</name>
    <message>
      <location filename="../src/ui/package_manager_unpack.ui" line="24"/>
      <source>unpacking please wait ...</source>
      <translation>Entpacke, bitte warten...</translation>
    </message>
  </context>
  <context>
    <name>profile_preferences</name>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="20"/>
      <source>Profile preferences</source>
      <translation>Profil-Einstellungen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="55"/>
      <source>General</source>
      <translation>Allgemein</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="61"/>
      <source>Icon sizes</source>
      <translation>Symbolgröße</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="67"/>
      <source>Icon size toolbars:</source>
      <translation>Symbolgröße in Symbolleisten:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="90"/>
      <source>Icon size in tree views:</source>
      <translation>Symbolgröße in Baumansichten:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="110"/>
      <source>Show menu bar:</source>
      <translation>Menüleiste anzeigen:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="121"/>
      <location filename="../src/ui/profile_preferences.ui" line="150"/>
      <source>Never</source>
      <translation>Niemals</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="126"/>
      <location filename="../src/ui/profile_preferences.ui" line="155"/>
      <source>Until a profile is loaded</source>
      <translation>Bis ein Profil geladen wurde</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="131"/>
      <location filename="../src/ui/profile_preferences.ui" line="160"/>
      <source>Always</source>
      <translation>Immer</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="139"/>
      <source>Show main toolbar</source>
      <translation>Funktionsleiste anzeigen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="239"/>
      <source>Allow server to install script packages</source>
      <translation>Erlaube Servern Skriptpakete zu installieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="266"/>
      <source>Game protocols</source>
      <translation>Spiel-Protokolle</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="323"/>
      <location filename="../src/ui/profile_preferences.ui" line="3306"/>
      <source>Please reconnect to your game for the change to take effect</source>
      <translation>Bitte neu mit dem Spiel verbinden, damit die Änderungen aktiv werden.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="339"/>
      <source>Log options</source>
      <translation>Optionen für die Protokollierung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="348"/>
      <source>Save log files in HTML format instead of plain text</source>
      <translation>Speichere Protokolldateien im HTML-Format anstelle von normalem Text</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="355"/>
      <source>Add timestamps at the beginning of log lines</source>
      <translation>Ergänze Zeitstempel zu Beginn der protokollierten Zeilen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="362"/>
      <source>Save log files in:</source>
      <translation>Protokolldateien speichern unter:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="379"/>
      <source>Browse...</source>
      <translation>Durchsuchen...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="386"/>
      <source>Reset</source>
      <translation>Zurücksetzen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="393"/>
      <source>Log format:</source>
      <translation>Format des Protokolls:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="406"/>
      <source>Log name:</source>
      <translation>Name des Protokolls:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="426"/>
      <source>.txt</source>
      <translation>.txt</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="459"/>
      <source>Input line</source>
      <translation>Eingabezeile</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="468"/>
      <source>Input</source>
      <translation>Eingabe</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="474"/>
      <source>use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly</source>
      <translation>Benutze strenge UNIX-Zeilenenden bei Befehlen für alte UNIX-Server, die mit Windows-Zeilenenden nicht korrekt umgehen können</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="477"/>
      <source>Strict UNIX line endings</source>
      <translation>Strenge UNIX-Zeilenenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="494"/>
      <source>Show the text you sent</source>
      <translation>Zeige von dir gesendeten Text</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="484"/>
      <source>Auto clear the input line after you sent text</source>
      <translation>Leere die Eingabezeile nachdem du Text gesendet hast</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="201"/>
      <source>&lt;p&gt;If you are playing a non-English game and seeing � instead of text, or special letters like &lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt; aren&apos;t showing right - try changing the encoding to UTF-8 or to one suggested by your game.&lt;/p&gt;&lt;p&gt;For some encodings on some Operating Systems Mudlet itself has to provide the codec needed; if that is the case for this Mudlet then there will be a &lt;tt&gt;m &lt;/tt&gt; prefixed applied to those encoding names (so if they have errors the blame can be applied correctly!)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="302"/>
      <source>&lt;p&gt;Enables MSP - provides Mud Sound Protocol messages during game play for supported games&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aktiviert MSP - bietet während des Spiels Meldungen zum Mud Sound Protocol für unterstützte Spiele&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="345"/>
      <source>&lt;p&gt;When checked will cause the date-stamp named log file to be HTML (file extension &apos;.html&apos;) which can convey color, font and other formatting information rather than a plain text (file extension &apos;.txt&apos;) format.  If changed while logging is already in progress it is necessary to stop and restart logging for this setting to take effect in a new log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="514"/>
      <source>React to all keybindings on the same key</source>
      <translation>Reagiere auf alle Tasenkürzel derselben Taste</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="524"/>
      <source>Command separator:</source>
      <translation>Trennzeichen zwischen Befehlen:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="534"/>
      <source>Enter text to separate commands with or leave blank to disable</source>
      <translation>Text eingeben, an dem Befehle getrennt werden, oder leer lassen zum deaktivieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="541"/>
      <source>Command line minimum height in pixels:</source>
      <translation>Mindesthöhe der Befehlszeile in Pixel:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="663"/>
      <source>Main display</source>
      <translation>Hauptanzeige</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="675"/>
      <source>Font</source>
      <translation>Schriftart</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="681"/>
      <source>Font:</source>
      <translation>Schriftart:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="704"/>
      <source>Size:</source>
      <translation>Größe:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="726"/>
      <source>Use anti aliasing on fonts. Smoothes fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry. </source>
      <translation>Glätte Kanten der Schriften. Ergibt glattere Schriften, wenn du eine hohe Bildschirmauflösung und größere Schriften benutzt. Beachte, dass die Schrift bei kleinen Auflösungen verschwommen wird. </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="729"/>
      <source>Enable anti-aliasing</source>
      <translation>Kantenglättung aktivieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="758"/>
      <source>Display Border</source>
      <translation>Rand der Anzeige</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="779"/>
      <source>Top border height:</source>
      <translation>Höhe des Randes oben:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="823"/>
      <source>Left border width:</source>
      <translation>Breite des Randes links:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="867"/>
      <source>Bottom border height:</source>
      <translation>Höhe des Randes unten:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="911"/>
      <source>Right border width:</source>
      <translation>Breite des Randes rechts:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="952"/>
      <source>Word wrapping</source>
      <translation>Zeilenumbruch</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="976"/>
      <source>Wrap lines at:</source>
      <translation>Zeilenumbruch bei:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="996"/>
      <location filename="../src/ui/profile_preferences.ui" line="1044"/>
      <source>characters</source>
      <translation>Zeichen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1024"/>
      <source>Indent wrapped lines by:</source>
      <translation>Einzug bei umgebruchenen Zeilen:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1057"/>
      <source>Double-click</source>
      <translation>Doppelklick</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1063"/>
      <source>Stop selecting a word on these characters:</source>
      <translation>Stoppe die Auswahl eines Wortes bei diesen Zeichen:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1073"/>
      <source>&apos;&quot;</source>
      <translation>&apos;&quot;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1086"/>
      <source>Display options</source>
      <translation>Darstellungsoptionen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1095"/>
      <source>Fix unnecessary linebreaks on GA servers</source>
      <translation>Behebe unnötige Zeilenbrüche auf GA-Servern</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1105"/>
      <source>Show Spaces/Tabs</source>
      <translation>Zeige Leerzeichen/Tabs</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1115"/>
      <source>Use Mudlet on a netbook with a small screen</source>
      <translation>Benutze Mudlet auf einem Netbook mit kleinem Bildschirm</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1125"/>
      <source>Show Line/Paragraphs</source>
      <translation>Zeige Zeilen/Absätze</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1135"/>
      <source>Echo Lua errors to the main console</source>
      <translation>Zeige Lua Fehler in der Hauptanzeige</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1142"/>
      <source>Make &apos;Ambiguous&apos; E. Asian width characters wide</source>
      <translation>Verbreitere &apos;mehrdeutige&apos; ostasiatische Zeichen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1176"/>
      <source>Editor</source>
      <translation>Editor</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1182"/>
      <source>Theme</source>
      <translation>Design</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1252"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>Aktualisiere Designs von colorsublime.github.io...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1292"/>
      <source>Color view</source>
      <translation>Farbansicht</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1298"/>
      <source>Select your color preferences</source>
      <translation>Wähle deine Farb-Einstellungen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1304"/>
      <source>Foreground:</source>
      <translation>Vordergrund:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1327"/>
      <source>Background:</source>
      <translation>Hintergrund:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1347"/>
      <source>Command line foreground:</source>
      <translation>Vordergrund der Befehlszeile:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1364"/>
      <source>Command line background:</source>
      <translation>Hintergrund der Befehlszeile:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1381"/>
      <source>Command foreground:</source>
      <translation>Vordergrund des Befehls:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1401"/>
      <source>Command background:</source>
      <translation>Hintergrund des Befehls:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="272"/>
      <source>&lt;p&gt;Enables GMCP - note that if you have MSDP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aktiviere GMCP - Hinweis: Wenn auch MSDP aktiviert ist, werden manche Server eins von beidem bevorzugen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="292"/>
      <source>&lt;p&gt;Enables MSDP - note that if you have GMCP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aktiviere MSDP - Hinweis: Wenn auch GMCP aktiviert ist, werden manche Server eins von beidem bevorzugen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="171"/>
      <source>Language &amp;&amp; data encoding</source>
      <translation>Sprache &amp;&amp; Datenkodierung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="177"/>
      <source>Interface language:</source>
      <translation>Sprache der Benutzeroberfläche:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="191"/>
      <source>Server data encoding:</source>
      <translation>Server-Daten-Kodierung:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="213"/>
      <source>Please restart Mudlet to apply the new language</source>
      <translation>Bitte Mudlet neu starten, um die neue Sprache anzuwenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="223"/>
      <source>Miscellaneous</source>
      <translation>Sonstiges</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="275"/>
      <source>Enable GMCP  (Generic Mud Communication Protocol)</source>
      <translation>GMCP aktivieren (Generic Mud Communication Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="295"/>
      <source>Enable MSDP  (Mud Server Data Protocol)</source>
      <translation>MSDP aktivieren (Mud Server Data Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="282"/>
      <source>&lt;p&gt;Enables MSSP - provides Mud Server Status Protocol information upon connection for supported games&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aktiviert MSSP - bietet Informationen zum Mud-Server-Status-Protokoll bei Verbindung zu unterstützten Spielen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="285"/>
      <source>Enable MSSP  (Mud Server Status Protocol)</source>
      <translation>MSSP aktivieren (Mud Server Status Protokoll)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="491"/>
      <source>&lt;p&gt;Echo the text you send in the display box.&lt;/p&gt;&lt;p&gt;&lt;i&gt;This can be disabled by the game server if it negotiates to use the telnet ECHO option&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zeigt den Text, den du sendest, in der Anzeigebox an.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Dies kann vom Spielserver deaktiviert werden, wenn er die Telnet-ECHO-Option verwendet.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="511"/>
      <source>&lt;p&gt;Check all Key-bindings against key-presses.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Versions of Mudlet prior to &lt;b&gt;3.9.0&lt;/b&gt; would stop checking after the first matching combination of&lt;/i&gt; KeyCode &lt;i&gt;and&lt;/i&gt; KeyModifier &lt;i&gt;was found and then send the command and/or run the script of that Key-binding only.  This&lt;/i&gt; per-profile &lt;i&gt;option tells Mudlet to check and run the remaining matches; but, to retain compatibility with previous versions, defaults to the &lt;b&gt;un&lt;/b&gt;-checked state.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="567"/>
      <source>Spell checking</source>
      <translation>Rechtschreibprüfung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="573"/>
      <source>&lt;p&gt;This option controls spell-checking on the command line in the main console for this profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Diese Option steuert die Rechtschreibprüfung der Befehlszeile in der Hauptkonsole für dieses Profil.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="576"/>
      <source>System/Mudlet dictionary:</source>
      <translation>System-/Mudlet -Wörterbuch:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="583"/>
      <source>&lt;p&gt;Select one dictionary which will be available on the command line and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle ein Wörterbuch, das in der Befehlszeile und im Lua-Subsystem verfügbar sein wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="611"/>
      <source>User dictionary:</source>
      <translation>Benutzerwörterbuch:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="617"/>
      <source>&lt;p&gt;A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Benutzerwörterbuch wird spezifisch für dieses Profil genutzt. Dies wird in der Befehlszeile angezeigt (Wörter aus dem Wörterbuch werden blau unterstrichen) und ist im Lua-Subsystem verfügbar.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="620"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="627"/>
      <source>&lt;p&gt;A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Benutzerwörterbuch wird von allen Profilen gemeinsam genutzt (wenn sie diese Option ausgewählt haben). Dies wird in der Befehlszeile angezeigt (Wörter aus dem Wörterbuch werden blau unterstrichen) und ist im Lua-Subsystem verfügbar.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="630"/>
      <source>Shared</source>
      <translation>Gemeinsam genutzt</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="719"/>
      <source>The selected font doesn&apos;t work with Mudlet, please pick another</source>
      <translation>Die ausgewählte Schriftart funktioniert nicht mit Mudlet, bitte wähle eine andere</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="776"/>
      <location filename="../src/ui/profile_preferences.ui" line="792"/>
      <source>&lt;p&gt;Extra space to have before text on top - can be set to negative to move text up beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mehr Platz vor dem Text oben - kann negativ gesetzt werden, um Text oben vom Bildschirm verschwinden zu lassen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="820"/>
      <location filename="../src/ui/profile_preferences.ui" line="836"/>
      <source>&lt;p&gt;Extra space to have before text on the left - can be set to negative to move text left beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mehr Platz vor dem Text links - kann negativ gesetzt werden, um Text links vom Bildschirm verschwinden zu lassen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="864"/>
      <location filename="../src/ui/profile_preferences.ui" line="880"/>
      <source>&lt;p&gt;Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mehr Platz vor dem Text unten - kann negativ gesetzt werden, um Text unten vom Bildschirm verschwinden zu lassen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="908"/>
      <location filename="../src/ui/profile_preferences.ui" line="924"/>
      <source>&lt;p&gt;Extra space to have before text on the right - can be set to negative to move text right beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mehr Platz vor dem Text rechts - kann negativ gesetzt werden, um Text rechts vom Bildschirm verschwinden zu lassen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1070"/>
      <source>&lt;p&gt;Enter the characters you&apos;d like double-clicking to stop selecting text on here. If you don&apos;t enter any, double-clicking on a word will only stop at a space, and will include characters like a double or a single quote. For example, double-clicking on the word &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; in the following will select &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;quot;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;quot;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;If you set the characters in the field to &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;quot;! &lt;/span&gt;which will mean it should stop selecting on &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; &amp;quot; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; ! then double-clicking on &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; will just select &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &amp;quot;&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;!&amp;quot;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1076"/>
      <source>(characters to ignore in selection, for example &apos; or &quot;)</source>
      <translation>(Zeichen, die in der Auswahl ignoriert werden, bspw. &apos; oder &quot;)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1092"/>
      <source>&lt;p&gt;Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Einige Spiele (besonders alle IRE MUDs) leiden unter einem Fehler, wobei sie mit dem Client nicht richtig kommunizieren, wo eine Zeile umbrechen sollte. Aktiviere diese Einstellung, damit Text nicht immer an die vorherige Promptzeile angehängt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1102"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;Bei der Anzeige von Lua Inhalten im Hauptfenster des Editors werden Tabulatoren und Leerzeichen sichtbar markiert.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1112"/>
      <source>&lt;p&gt;Select this option for better compatability if you are using a netbook, or some other computer model that has a small screen.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle diese Option für bessere Kompatibilität für Netbooks oder andere Computermodelle mit kleinem Bildschirm.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1122"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show  line and paragraphs ends with visible marks as well as whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;Bei der Anzeige von Lua Inhalten im Hauptfenster des Editors werden Enden von Zeilen und Absätzen sichtbar markiert.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1132"/>
      <source>&lt;p&gt;Prints Lua errors to the main console in addition to the error tab in the editor.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zeigt Lua-Fehler auch im Hauptfenster, nicht nur im Fehlerregister des Editors.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1152"/>
      <source>Enable text analyzer</source>
      <translation>Textanalyse aktivieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3473"/>
      <source>h:mm:ss.zzz</source>
      <comment>Used to set a time interval only</comment>
      <translation>h:mm:ss.zzz</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1262"/>
      <source>Autocomplete</source>
      <translation>Autovervollständigung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="229"/>
      <source>Notify on new data</source>
      <translation>Benachrichtigung bei neuen Daten</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="232"/>
      <source>&lt;p&gt;Show a toolbar notification if Mudlet is minimized and new data arrives.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zeigt eine Benachrichtigung in der Symbolleiste an, wenn Mudlet minimiert ist und neue Daten eintreffen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="246"/>
      <source>Auto save on exit</source>
      <translation>Automatisch speichern beim Beenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="253"/>
      <source>Allow server to download and play media</source>
      <translation>Erlaube Servern das Herunterladen und Abspielen von Medien</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="256"/>
      <source>&lt;p&gt;This also needs GMCP to be enabled in the next group below.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dies erfordert auch die Aktivierung von GMCP in der nächsten Gruppe unten.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="305"/>
      <source>Enable MSP  (Mud Sound Protocol)</source>
      <translation>MSP aktivieren (Mud Sound Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="501"/>
      <source>Highlights your input line text when scrolling through your history for easy cancellation</source>
      <translation>Hebt den Text deiner Eingabezeile hervor während du durch den Verlauf scrollst, um leichter abzubrechen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="504"/>
      <source>Highlight history</source>
      <translation>Verlauf hervorheben</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="741"/>
      <source>This font is not monospace, which may not be ideal for playing some text games:
you can use it but there could be issues with aligning columns of text</source>
      <comment>Note that this text is split into two lines so that the message is not too wide in English, please do the same for other locales where the text is the same or longer</comment>
      <translation>Dies ist keine Monospace-Schriftart, was bei manchen Textspielen vielleicht nicht ideal ist: 
Du kannst sie verwenden, aber es könnte Probleme mit der Ausrichtung von Textspalten geben</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1268"/>
      <source>Autocomplete Lua functions in code editor</source>
      <translation>Vervollständige automatisch die Lua-Funktionen im Code-Editor</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1311"/>
      <source>&lt;p&gt;The foreground color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Vordergrundfarbe wird standardmäßig für die Hauptkonsole verwendet (es sei denn, dies wurde geändert durch einen Lua-Befehl oder vom Spielserver).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1334"/>
      <source>&lt;p&gt;The background color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Hintergrundfarbe wird standardmäßig für die Hauptkonsole verwendet (es sei denn, dies wurde geändert durch einen Lua-Befehl oder vom Spielserver).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1354"/>
      <source>&lt;p&gt;The foreground color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Vordergrundfarbe wird standardmäßig für den Eingabebereich verwendet.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1371"/>
      <source>&lt;p&gt;The background color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Hintergrundfarbe wird standardmäßig für den Eingabebereich verwendet.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1388"/>
      <source>&lt;p&gt;The foreground color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Vordergrundfarbe wird für Text verwendet, der an den Spielserver geschickt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1408"/>
      <source>&lt;p&gt;The background color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Die Hintergrundfarbe wird für Text verwendet, der an den Spielserver geschickt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1434"/>
      <source>These preferences set how you want a particular color to be represented visually in the main display:</source>
      <translation>Diese Einstellungen legen fest, wie bestimmte Farben in der Hauptanzeige visuell dargestellt werden sollen:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1444"/>
      <location filename="../src/ui/profile_preferences.ui" line="2262"/>
      <source>Black:</source>
      <translation>Schwarz:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1451"/>
      <source>ANSI Color Number 0</source>
      <translation>ANSI-Farbe Nummer 0</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1464"/>
      <location filename="../src/ui/profile_preferences.ui" line="2279"/>
      <source>Light black:</source>
      <translation>Helles Schwarz:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1471"/>
      <source>ANSI Color Number 8</source>
      <translation>ANSI-Farbe Nummer 8</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1484"/>
      <location filename="../src/ui/profile_preferences.ui" line="2296"/>
      <source>Red:</source>
      <translation>Rot:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1491"/>
      <source>ANSI Color Number 1</source>
      <translation>ANSI-Farbe Nummer 1</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1504"/>
      <location filename="../src/ui/profile_preferences.ui" line="2313"/>
      <source>Light red:</source>
      <translation>Helles Rot:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1511"/>
      <source>ANSI Color Number 9</source>
      <translation>ANSI-Farbe Nummer 9</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1524"/>
      <location filename="../src/ui/profile_preferences.ui" line="2330"/>
      <source>Green:</source>
      <translation>Grün:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1531"/>
      <source>ANSI Color Number 2</source>
      <translation>ANSI-Farbe Nummer 2</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1544"/>
      <location filename="../src/ui/profile_preferences.ui" line="2347"/>
      <source>Light green:</source>
      <translation>Helles Grün:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1551"/>
      <source>ANSI Color Number 10</source>
      <translation>ANSI-Farbe Nummer 10</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1564"/>
      <location filename="../src/ui/profile_preferences.ui" line="2364"/>
      <source>Yellow:</source>
      <translation>Gelb:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1571"/>
      <source>ANSI Color Number 3</source>
      <translation>ANSI-Farbe Nummer 3</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1584"/>
      <location filename="../src/ui/profile_preferences.ui" line="2381"/>
      <source>Light yellow:</source>
      <translation>Helles Gelb:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1591"/>
      <source>ANSI Color Number 11</source>
      <translation>ANSI-Farbe Nummer 11</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1604"/>
      <location filename="../src/ui/profile_preferences.ui" line="2398"/>
      <source>Blue:</source>
      <translation>Blau:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1611"/>
      <source>ANSI Color Number 4</source>
      <translation>ANSI-Farbe Nummer 4</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1624"/>
      <location filename="../src/ui/profile_preferences.ui" line="2415"/>
      <source>Light blue:</source>
      <translation>Helles Blau:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1631"/>
      <source>ANSI Color Number 12</source>
      <translation>ANSI-Farbe Nummer 12</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1644"/>
      <location filename="../src/ui/profile_preferences.ui" line="2432"/>
      <source>Magenta:</source>
      <translation>Magenta:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1651"/>
      <source>ANSI Color Number 5</source>
      <translation>ANSI-Farbe Nummer 5</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1664"/>
      <location filename="../src/ui/profile_preferences.ui" line="2449"/>
      <source>Light magenta:</source>
      <translation>Helles Magenta:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1671"/>
      <source>ANSI Color Number 13</source>
      <translation>ANSI-Farbe Nummer 13</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1684"/>
      <location filename="../src/ui/profile_preferences.ui" line="2466"/>
      <source>Cyan:</source>
      <translation>Türkis:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1691"/>
      <source>ANSI Color Number 6</source>
      <translation>ANSI-Farbe Nummer 6</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1704"/>
      <location filename="../src/ui/profile_preferences.ui" line="2483"/>
      <source>Light cyan:</source>
      <translation>Helles Türkis:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1711"/>
      <source>ANSI Color Number 14</source>
      <translation>ANSI-Farbe Nummer 14</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1724"/>
      <location filename="../src/ui/profile_preferences.ui" line="2500"/>
      <source>White:</source>
      <translation>Weiß:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1731"/>
      <source>ANSI Color Number 7</source>
      <translation>ANSI-Farbe Nummer 7</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1744"/>
      <location filename="../src/ui/profile_preferences.ui" line="2517"/>
      <source>Light white:</source>
      <translation>Helles Weiß:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1751"/>
      <source>ANSI Color Number 15</source>
      <translation>ANSI-Farbe Nummer 15</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1764"/>
      <location filename="../src/ui/profile_preferences.ui" line="2534"/>
      <source>Reset all colors to default</source>
      <translation>Alle Farben auf den Standard zurücksetzen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1771"/>
      <source>&lt;p&gt;If this option is checked the Mud Server may send codes to change the above 16 colors or to reset them to their defaults by using standard ANSI &lt;tt&gt;OSC&lt;/tt&gt; Escape codes.&lt;/p&gt;&lt;p&gt;Specifically &lt;tt&gt;&amp;lt;OSC&amp;gt;Pirrggbb&amp;lt;ST&amp;gt;&lt;/tt&gt; will set the color with index &lt;i&gt;i&lt;/i&gt; to have the color with the given &lt;i&gt;rr&lt;/i&gt; red, &lt;i&gt;gg&lt;/i&gt; green and &lt;i&gt;bb&lt;/i&gt;  blue components where i is a single hex-digit (&apos;0&apos; to &apos;9&apos; or &apos;a&apos; to &apos;f&apos; or &apos;A&apos; to &apos;F&apos; to give a number between 0 an d15) and rr, gg and bb are two digit hex-digits numbers (between 0 to 255); &amp;lt;OSC&amp;gt; is &lt;i&gt;Operating System Command&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;[&lt;/tt&gt; and &amp;lt;ST&amp;gt; is the &lt;i&gt;String Terminator&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Conversely &lt;tt&gt;&amp;lt;OSC&amp;gt;R&amp;lt;ST&amp;gt;&lt;/tt&gt; will reset the colors to the defaults like the button to the right does.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1774"/>
      <source>Server allowed to redefine these colors</source>
      <translation>Der Server darf diese Farben neu definieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1798"/>
      <source>Mapper</source>
      <translation>Mapper</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1804"/>
      <source>Map files</source>
      <translation>Kartendateien</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1810"/>
      <source>Save your current map:</source>
      <translation>Speichere deine aktuelle Karte:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1820"/>
      <source>Press to choose location and save</source>
      <translation>Wähle das Ziel und speichere</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1827"/>
      <source>Load another map file in:</source>
      <translation>Lade eine andere Kartendatei:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1837"/>
      <source>Press to choose file and load</source>
      <translation>Wähle eine Datei und lade</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1844"/>
      <source>&lt;p&gt;Mudlet now does some sanity checking and repairing to clean up issues that may have arisen in previous version due to faulty code or badly documented commands. However if significant problems are found the report can be quite extensive, in particular for larger maps.&lt;/p&gt;&lt;p&gt;Unless this option is set, Mudlet will reduce the amount of on-screen messages by hiding many texts and showing a suggestion to review the report file instead.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet führt nun Prüfungen und Reparaturen durch, um Probleme mit vorherigen Versionen zu beseitigen, die auf fehlerhaften Code oder schlecht dokumentierten Befehlen zurückgehen. Sollten schwerwiegende Probleme auftauchen, kann der Bericht sehr umfangreich ausfallen, besonders bei sehr großen Karten.&lt;/p&gt;&lt;p&gt;Solange diese Funktion deaktiviert ist, wird Mudlet die Anzahl der Meldungen überschaubar halten und weniger Texte auf dem Bildschirm ausgegeben, sondern auf die Datei mit dem vollständigen Bericht verwiesen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1928"/>
      <source>&lt;p&gt;Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ändere dies auf eine kleinere Version, wenn du deine Karte in einem Format speichern möchtest, die von älteren Mudlet Versionen gelesen werden kann. Dadurch werden zusätzliche Daten aus dem aktuellen Kartenformat verloren gehen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1958"/>
      <source>Download latest map provided by your game:</source>
      <translation>Lade die neueste Karte herunter, die dein Spiel bereitstellt:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1987"/>
      <source>&lt;p&gt;This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer.&lt;/p&gt;&lt;p&gt;3D map view always has anti-aliasing enabled.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dies ermöglicht Kantenglättung (Anti-Aliasing) der 2D-Kartenansicht, die so glatter und schöner aussieht. Deaktiviere dies auf sehr langsamen Computern.&lt;/p&gt;&lt;p&gt;Die 3D-Kartenansicht hat immer Kantenglättung aktiviert.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1997"/>
      <source>&lt;p&gt;The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area&lt;/p&gt;</source>
      <translation>&lt;p&gt;Das Standard-Gebiet (Gebiets-ID -1) wird von einigen Mapper-Skripten als temporärer &apos;Wartebereich&apos; für Räume verwendet, bevor diese ins richtige Gebiet platziert werden&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2049"/>
      <source>2D map player room marker style:</source>
      <translation>Stil der Markierung des Spielerraums auf der 2D-Karte:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2059"/>
      <source>Outer ring color</source>
      <translation>Farbe des Außenrings</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2069"/>
      <source>Inner ring color</source>
      <translation>Farbe des Innenrings</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2083"/>
      <source>Original</source>
      <translation>Original</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2088"/>
      <source>Red ring</source>
      <translation>Roter Ring</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2093"/>
      <source>Blue/Yellow ring</source>
      <translation>Blauer/Gelber Ring</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2098"/>
      <source>Custom ring</source>
      <translation>Benutzerdefinierter Ring</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2106"/>
      <source>&lt;p&gt;Percentage ratio (&lt;i&gt;the default is 120%&lt;/i&gt;) of the marker symbol to the space available for the room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Prozentuales Verhältnis (&lt;i&gt;der Standardwert ist 120%&lt;/i&gt;) des markierenden Symbols zum verfügbaren Platz des Raumes.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2112"/>
      <location filename="../src/ui/profile_preferences.ui" line="2140"/>
      <source>%</source>
      <translation>%</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2115"/>
      <source>Outer diameter: </source>
      <translation>Äußerer Durchmesser: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2134"/>
      <source>&lt;p&gt;Percentage ratio of the inner diameter of the marker symbol to the outer one (&lt;i&gt;the default is 70%&lt;/i&gt;).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Prozentuales Verhältnis des Innendurchmessers des markierenden Symbols zum äußeren Durchmesser (&lt;i&gt;der Standardwert ist 70%&lt;/i&gt;).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2143"/>
      <source>Inner diameter: </source>
      <translation>Innerer Durchmesser: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2159"/>
      <source>&lt;p&gt;This enables borders around room. Color can be set in Mapper colors tab&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dies zeigt Ränder am Raum. Die Farbe kann unter Mapper-Farben eingestellt werden&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2162"/>
      <source>Show room borders</source>
      <translation>Raumränder anzeigen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2238"/>
      <source>Room border color:</source>
      <translation>Farbe der Raumränder:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2558"/>
      <source>Chat</source>
      <translation>Chat</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3250"/>
      <source>Special options needed for some older game drivers (needs client restart to take effect)</source>
      <translation>Spezielle Optionen für einige ältere Spiel-Treiber (Aktivierung benötigt Neustart des Client)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3288"/>
      <source>Force CHARSET negotiation off</source>
      <translation>CHARSET Verhandlungen ausschalten</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3390"/>
      <source>the computer&apos;s password manager (secure)</source>
      <translation>im Kennwort-Manager des Computers (sicher)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3395"/>
      <source>plaintext with the profile (portable)</source>
      <translation>als Klartext mit dem Profil (protabel)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3486"/>
      <source>&lt;p&gt;If checked this will cause all problem Unicode codepoints to be reported in the debug output as they occur; if cleared then each different one will only be reported once and summarized in as a table when the console in which they occurred is finally destroyed (when the profile is closed).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3372"/>
      <source>&lt;p&gt;Some MUDs use a flawed interpretation of the ANSI Set Graphics Rendition (&lt;b&gt;SGR&lt;/b&gt;) code sequences for 16M color mode which only uses semi-colons and not colons to separate parameter elements i.e. instead of using a code in the form: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38:2:&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Unused&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance Color Space (0=CIELUV; 1=CIELAB)&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;where the &lt;i&gt;Color Space Id&lt;/i&gt; is expected to be an empty string to specify the usual (default) case and all of the &lt;i&gt;Parameter Elements&lt;/i&gt; (the &quot;2&quot; and the values in the &lt;tt&gt;&amp;lt;...&amp;gt;&lt;/tt&gt;s) may, technically, be omitted; they use: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;or: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;It is not possible to reliably detect the difference between these two so checking this option causes Mudlet to expect the last one with the additional (but empty!) parameter.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3375"/>
      <source>Expect Color Space Id in SGR...(3|4)8;2;...m codes</source>
      <translation>Erwarte Farbraum-ID in SGR... (3|4)8;2;... m-Codes</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3382"/>
      <source>Store character login passwords in:</source>
      <translation>Kennwörter für Charakteranmeldung speichern in:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3408"/>
      <source>TextLabel</source>
      <translation>Text</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2816"/>
      <source>TLS/SSL secure connection</source>
      <translation>Sichere TLS/SSL-Verbindung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3120"/>
      <source>Accept self-signed certificates</source>
      <translation>Akzeptiere selbstsignierte Zertifikate</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3130"/>
      <source>Accept expired certificates</source>
      <translation>Akzeptiere abgelaufene Zertifikate</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2834"/>
      <source>Certificate</source>
      <translation>Zertifikat</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2894"/>
      <source>Serial:</source>
      <translation>Seriennummer:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2843"/>
      <source>Issuer:</source>
      <translation>Aussteller:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2860"/>
      <source>Issued to:</source>
      <translation>Ausgestellt für:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2877"/>
      <source>Expires:</source>
      <translation>Läuft ab:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3140"/>
      <source>Accept all certificate errors       (unsecure)</source>
      <translation>Akzeptiere alle Zertifikatsfehler (unsicher)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1847"/>
      <source>Report map issues on screen</source>
      <translation>Berichte Kartenprobleme auf dem Bildschirm</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1861"/>
      <source>Copy map to other profile(s):</source>
      <translation>Kopiere Karte an andere(s) Profil(e):</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1877"/>
      <source>Press to pick destination(s)</source>
      <translation>Wähle Ziel(e)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1890"/>
      <source>Copy to destination(s)</source>
      <translation>Kopiere an Ziel(e)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1902"/>
      <source>An action above happened</source>
      <translation>Eine Aktion geschah oben.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1909"/>
      <source>Map format version:</source>
      <translation>Version des Kartenformats:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1934"/>
      <location filename="../src/ui/profile_preferences.ui" line="1938"/>
      <source># {default version}</source>
      <translation># {Standard-Version}</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1949"/>
      <source>Map download</source>
      <translation>Karte herunterladen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1971"/>
      <source>Download</source>
      <translation>Herunterladen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1981"/>
      <source>Map view</source>
      <translation>Kartenansicht</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2010"/>
      <source>2D Map Room Symbol Font</source>
      <translation>2D-Karte-Raum-Symbol-Schriftart</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1990"/>
      <source>Use high quality graphics in 2D view</source>
      <translation>Verwende hochwertige Grafiken in der 2D-Ansicht</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="184"/>
      <source>&lt;p&gt;Can you help translate Mudlet?&lt;/p&gt;&lt;p&gt;If so, please visit: &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kannst du helfen, Mudlet zu übersetzen?&lt;/p&gt;&lt;p&gt;Falls ja, besuche bitte: &lt;b&gt;https://https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1874"/>
      <source>&lt;p&gt;Select profiles that you want to copy map to, then press the Copy button to the right&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die Profile aus, in die die Karte kopiert werden soll, und drücke dann rechts auf Kopieren&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1887"/>
      <source>&lt;p&gt;Copy map into the selected profiles on the left&lt;/p&gt;</source>
      <translation>&lt;p&gt;Karte in die links ausgewählten Profile kopieren&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1955"/>
      <location filename="../src/ui/profile_preferences.ui" line="1968"/>
      <source>&lt;p&gt;On games that provide maps for download, you can press this button to get the latest map. Note that this will &lt;span style=&quot; font-weight:600;&quot;&gt;overwrite&lt;/span&gt; any changes you&apos;ve done to your map, and will use the new map only&lt;/p&gt;</source>
      <translation>&lt;p&gt;In Spielen, die Karten zum Download anbieten, kannst du diese Taste drücken, um die neueste Karte zu erhalten. Beachte, dass dies deine bisherigen Änderungen &lt;span style=&quot; font-weight:600;&quot;&gt;überschreiben&lt;/span&gt; wird, und nur noch die neue Karte verwendet&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2000"/>
      <source>Show the default area in map area selection</source>
      <translation>Das Standard-Gebiet in der Auswahl der Kartengebiete anzeigen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2020"/>
      <source>Only use symbols (glyphs) from chosen font</source>
      <translation>Verwende nur Symbole (Glyphen) aus der gewählten Schriftart</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2027"/>
      <source>Show symbol usage...</source>
      <translation>Zeige Verwendung der Symbole...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2189"/>
      <source>Mapper colors</source>
      <translation>Farben des Mappers</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2195"/>
      <source>Select your color preferences for the map display</source>
      <translation>Wähle deine Farbeinstellungen für die Kartenanzeige</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2201"/>
      <source>Link color</source>
      <translation>Farbe der Verbindungen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2221"/>
      <source>Background color:</source>
      <translation>Hintergrundfarbe:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3244"/>
      <source>Special Options</source>
      <translation>Spezielle Optionen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3256"/>
      <source>Force compression off</source>
      <translation>Komprimierung zwingend abschalten</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3274"/>
      <source>Force telnet GA signal interpretation off</source>
      <translation>Telnet GA Signalinterpretation zwingend abschalten</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3263"/>
      <source>This option adds a line line break &lt;LF&gt; or &quot;
&quot; to your command input on empty commands. This option will rarely be necessary.</source>
      <translation>Diese Option ergänzt einen Zeilenumbruch &lt;LF&gt; oder &quot;
&quot; zu deiner Befehlszeile bei leeren Befehlen. Diese Option wird nur selten notwendig sein.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3267"/>
      <source>Force new line on empty commands</source>
      <translation>Neue Zeile bei leeren Befehlen ergänzen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3281"/>
      <source>Force MXP negotiation off</source>
      <translation>MXP Verhandlungen ausschalten</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2635"/>
      <source>Discord privacy</source>
      <translation>Discord Privatsphäre</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2668"/>
      <source>Don&apos;t hide small icon or tooltip</source>
      <translation>Tooltipp und kleines Symbol einblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2673"/>
      <source>Hide small icon tooltip</source>
      <translation>Tooltip am kleinen Symbol ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2678"/>
      <source>Hide small icon and tooltip</source>
      <translation>Tooltipp und kleines Symbol ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2700"/>
      <source>Hide timer</source>
      <translation>Timer ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2723"/>
      <location filename="../src/ui/profile_preferences.ui" line="2736"/>
      <location filename="../src/ui/profile_preferences.ui" line="2752"/>
      <location filename="../src/ui/profile_preferences.ui" line="2768"/>
      <source>&lt;p&gt;Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). Leave empty to show it for any Discord account you log in to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet wird Informationen zur Rich Presence nur zeigen, während du diesen Discord Benutzernamen benutzt (hilfreich, falls du mehrere Discord Konten hast). Leer lassen, um sie für jedes eingeloggtes Discord Konto anzuzeigen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2729"/>
      <source>Restrict to:</source>
      <translation>Beschränken auf:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2642"/>
      <source>Don&apos;t hide large icon or tooltip</source>
      <translation>Tooltipp und großes Symbol einblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2647"/>
      <source>Hide large icon tooltip</source>
      <translation>Tooltip am großen Symbol ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2652"/>
      <source>Hide large icon and tooltip</source>
      <translation>Tooltipp und großes Symbol ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2707"/>
      <source>&lt;p&gt;Allow Lua to set Discord status&lt;/p&gt;</source>
      <translation>&lt;p&gt;Erlaube Lua, den Status des Discord zu setzen&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2710"/>
      <source>Enable Lua API</source>
      <translation>Lua API aktivieren</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2742"/>
      <source>specific Discord username</source>
      <translation>bestimmter Discord-Benutzername</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2686"/>
      <source>Hide state</source>
      <translation>Zustand ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2693"/>
      <source>Hide party details</source>
      <translation>Gruppendetails ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2660"/>
      <source>Hide detail</source>
      <translation>Detail ausblenden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2564"/>
      <source>IRC client options</source>
      <translation>IRC-Client-Optionen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2580"/>
      <source>irc.example.net</source>
      <translation>irc.beispiel.net</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2587"/>
      <source>Port:</source>
      <translation>Port:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2625"/>
      <source>#channel1 #channel2 #etc...</source>
      <translation>#kanal1 #kanal2 #usw...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2611"/>
      <source>MudletUser123</source>
      <translation>MudletNutzer123</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2570"/>
      <source>Server address:</source>
      <translation>Server-Adresse:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2618"/>
      <source>Auto-join channels: </source>
      <translation>Automatisch betretene Kanäle: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2604"/>
      <source>Nickname:</source>
      <translation>Spitzname:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2597"/>
      <source>6667</source>
      <translation>6667</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3338"/>
      <source>Search Engine</source>
      <translation>Suchmaschine</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3322"/>
      <source>Mudlet updates</source>
      <translation>Mudlet Aktualisierungen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3328"/>
      <source>Disable automatic updates</source>
      <translation>Deaktiviere automatische Aktualisierungen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3350"/>
      <source>Other Special options</source>
      <translation>Andere spezielle Optionen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3356"/>
      <source>Show icons on menus</source>
      <translation>Symbole in Menüs anzeigen</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2810"/>
      <source>Connection</source>
      <translation>Verbindung</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3150"/>
      <source>Connect to the game via proxy</source>
      <translation>Mit dem Spiel über Proxy verbinden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3165"/>
      <source>Address</source>
      <translation>Adresse</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3185"/>
      <source>port</source>
      <translation>port</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3195"/>
      <source>Username for logging into the proxy if requred</source>
      <translation>Benutzername für die Anmeldung am Proxy, falls nötig</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3201"/>
      <source>username (optional)</source>
      <translation>Benutzername (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3211"/>
      <source>Password for logging into the proxy if requred</source>
      <translation>Passwort für die Anmeldung am Proxy, falls nötig</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3217"/>
      <source>password (optional)</source>
      <translation>Passwort (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3436"/>
      <source>Show debug messages for timers not smaller than:</source>
      <translation>Debug-Meldungen für Timer anzeigen, die nicht kleiner sind als:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3442"/>
      <source>&lt;p&gt;Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the &lt;i&gt;Central Debug Console&lt;/i&gt; useless if there is a very small interval timer running.&lt;/p&gt;</source>
      <comment>The term in &apos;...&apos; refer to a Mudlet specific thing and ought to match the corresponding translation elsewhere.</comment>
      <translation>&lt;p&gt;Zeige &apos;LUA OK&apos;-Meldungen für Timer mit dem angegebenen Mindestintervall (h:mm:ss.zzz). Der Minimalwert (die Voreinstellung) zeigt alle derartigen Meldungen an, kann aber die &lt;i&gt;Zentrale Debug-Konsole&lt;/i&gt; nutzlos machen, wenn ein Timer mit sehr kleinem Intervall läuft.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3483"/>
      <source>Report all Codepoint problems immediately</source>
      <translation>Alle Codepoint-Probleme sofort melden</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3541"/>
      <source>Save</source>
      <translation>Speichern</translation>
    </message>
  </context>
  <context>
    <name>room_exits</name>
    <message>
      <location filename="../src/ui/room_exits.ui" line="37"/>
      <source>General exits:</source>
      <translation>Allgemeine Ausgänge:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="96"/>
      <source>NW exit...</source>
      <translation>NW-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="106"/>
      <location filename="../src/ui/room_exits.ui" line="246"/>
      <location filename="../src/ui/room_exits.ui" line="386"/>
      <location filename="../src/ui/room_exits.ui" line="526"/>
      <location filename="../src/ui/room_exits.ui" line="669"/>
      <location filename="../src/ui/room_exits.ui" line="876"/>
      <location filename="../src/ui/room_exits.ui" line="1013"/>
      <location filename="../src/ui/room_exits.ui" line="1171"/>
      <location filename="../src/ui/room_exits.ui" line="1311"/>
      <location filename="../src/ui/room_exits.ui" line="1451"/>
      <location filename="../src/ui/room_exits.ui" line="1591"/>
      <location filename="../src/ui/room_exits.ui" line="1731"/>
      <source>&lt;p&gt;Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen positiven Wert, um die normale Gewichtung des Raums für diese Ausgangsrichtung zu setzen. Null setzt wieder den Standard.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="236"/>
      <source>N exit...</source>
      <translation>N-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="376"/>
      <source>NE exit...</source>
      <translation>NO-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="516"/>
      <source>Up exit...</source>
      <translation>Oben-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="659"/>
      <source>W exit...</source>
      <translation>W-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="767"/>
      <source>ID:</source>
      <translation>ID:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="790"/>
      <source>Weight:</source>
      <translation>Gewicht:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="866"/>
      <source>E exit...</source>
      <translation>O-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1003"/>
      <source>Down exit...</source>
      <translation>Unten-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1155"/>
      <source>SW exit...</source>
      <translation>SW-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1301"/>
      <source>S exit...</source>
      <translation>S-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1441"/>
      <source>SE exit...</source>
      <translation>SO-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1581"/>
      <source>In exit...</source>
      <translation>Rein-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1721"/>
      <source>Out exit...</source>
      <translation>Raus-Ausgang...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1826"/>
      <source>Key:</source>
      <translation>Taste:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1844"/>
      <source>No route</source>
      <translation>Keine Route</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1859"/>
      <source>Stub Exit</source>
      <translation>Platzhalter</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="67"/>
      <location filename="../src/ui/room_exits.ui" line="207"/>
      <location filename="../src/ui/room_exits.ui" line="347"/>
      <location filename="../src/ui/room_exits.ui" line="487"/>
      <location filename="../src/ui/room_exits.ui" line="627"/>
      <location filename="../src/ui/room_exits.ui" line="837"/>
      <location filename="../src/ui/room_exits.ui" line="974"/>
      <location filename="../src/ui/room_exits.ui" line="1120"/>
      <location filename="../src/ui/room_exits.ui" line="1272"/>
      <location filename="../src/ui/room_exits.ui" line="1412"/>
      <location filename="../src/ui/room_exits.ui" line="1552"/>
      <location filename="../src/ui/room_exits.ui" line="1692"/>
      <location filename="../src/ui/room_exits.ui" line="2057"/>
      <source>&lt;p&gt;Prevent a route being created via this exit, equivalent to an infinite exit weight.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Verhindere die Erstellung von Routen durch diesen Ausgang. Das entspricht einem unendlichen Gewicht des Ausgangs.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="83"/>
      <location filename="../src/ui/room_exits.ui" line="223"/>
      <location filename="../src/ui/room_exits.ui" line="363"/>
      <location filename="../src/ui/room_exits.ui" line="503"/>
      <location filename="../src/ui/room_exits.ui" line="643"/>
      <location filename="../src/ui/room_exits.ui" line="853"/>
      <location filename="../src/ui/room_exits.ui" line="990"/>
      <location filename="../src/ui/room_exits.ui" line="1142"/>
      <location filename="../src/ui/room_exits.ui" line="1288"/>
      <location filename="../src/ui/room_exits.ui" line="1428"/>
      <location filename="../src/ui/room_exits.ui" line="1568"/>
      <location filename="../src/ui/room_exits.ui" line="1708"/>
      <source>&lt;p&gt;Create an exit in this direction with unknown destination, mutually exclusive with an actual exit roomID.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Erstelle in diese Richtung einen Abzweig mit unbekanntem Ziel. Kann nicht gleichzeitig einen Ausgang mit echter RaumID haben.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="93"/>
      <source>&lt;p&gt;Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Nordwesten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="119"/>
      <location filename="../src/ui/room_exits.ui" line="259"/>
      <location filename="../src/ui/room_exits.ui" line="399"/>
      <location filename="../src/ui/room_exits.ui" line="539"/>
      <location filename="../src/ui/room_exits.ui" line="682"/>
      <location filename="../src/ui/room_exits.ui" line="886"/>
      <location filename="../src/ui/room_exits.ui" line="1026"/>
      <location filename="../src/ui/room_exits.ui" line="1184"/>
      <location filename="../src/ui/room_exits.ui" line="1324"/>
      <location filename="../src/ui/room_exits.ui" line="1464"/>
      <location filename="../src/ui/room_exits.ui" line="1604"/>
      <location filename="../src/ui/room_exits.ui" line="1744"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Für diesen Ausgang wird kein Symbol für die Tür auf der 2D Karte gezeichnet.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="138"/>
      <location filename="../src/ui/room_exits.ui" line="278"/>
      <location filename="../src/ui/room_exits.ui" line="418"/>
      <location filename="../src/ui/room_exits.ui" line="701"/>
      <location filename="../src/ui/room_exits.ui" line="905"/>
      <location filename="../src/ui/room_exits.ui" line="1203"/>
      <location filename="../src/ui/room_exits.ui" line="1343"/>
      <location filename="../src/ui/room_exits.ui" line="1483"/>
      <source>&lt;p&gt;Green (Open) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein grünes Symbol für die (offene) Tür wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="154"/>
      <location filename="../src/ui/room_exits.ui" line="294"/>
      <location filename="../src/ui/room_exits.ui" line="434"/>
      <location filename="../src/ui/room_exits.ui" line="717"/>
      <location filename="../src/ui/room_exits.ui" line="921"/>
      <location filename="../src/ui/room_exits.ui" line="1219"/>
      <location filename="../src/ui/room_exits.ui" line="1359"/>
      <location filename="../src/ui/room_exits.ui" line="1499"/>
      <source>&lt;p&gt;Orange (Closed) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein oranges Symbol für die (geschlossene) Tür wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="170"/>
      <location filename="../src/ui/room_exits.ui" line="310"/>
      <location filename="../src/ui/room_exits.ui" line="450"/>
      <location filename="../src/ui/room_exits.ui" line="733"/>
      <location filename="../src/ui/room_exits.ui" line="937"/>
      <location filename="../src/ui/room_exits.ui" line="1235"/>
      <location filename="../src/ui/room_exits.ui" line="1375"/>
      <location filename="../src/ui/room_exits.ui" line="1515"/>
      <source>&lt;p&gt;Red (Locked) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein rotes Symbol für die (versperrte) Tür wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="233"/>
      <source>&lt;p&gt;Set the number of the room north of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Norden von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="373"/>
      <source>&lt;p&gt;Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Nordosten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="513"/>
      <source>&lt;p&gt;Set the number of the room up from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Oben von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="558"/>
      <location filename="../src/ui/room_exits.ui" line="1045"/>
      <location filename="../src/ui/room_exits.ui" line="1623"/>
      <location filename="../src/ui/room_exits.ui" line="1763"/>
      <source>&lt;p&gt;A symbol is drawn with a green (Open) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein Symbol mit grüner (offener) Füllung wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="574"/>
      <location filename="../src/ui/room_exits.ui" line="1061"/>
      <location filename="../src/ui/room_exits.ui" line="1639"/>
      <location filename="../src/ui/room_exits.ui" line="1779"/>
      <source>&lt;p&gt;A symbol is drawn with an orange (Closed) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein Symbol mit oranger (geschlossener) Füllung wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="590"/>
      <location filename="../src/ui/room_exits.ui" line="1077"/>
      <location filename="../src/ui/room_exits.ui" line="1655"/>
      <location filename="../src/ui/room_exits.ui" line="1795"/>
      <source>&lt;p&gt;A symbol is drawn with a red (Locked) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein Symbol mit roter (versperrter) Füllung wird auf der 2D Karte gezeichnet, passt entweder zu einem Platzhalter oder einem echten Ausgang.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="653"/>
      <source>&lt;p&gt;Set the number of the room west of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Westen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="780"/>
      <source>&lt;p&gt;This is the Room ID Number for this room - it cannot be changed here!</source>
      <translation>&lt;p&gt;Dies ist die Raum ID des aktuellen Raums - sie kann hier nicht geändert werden!</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="803"/>
      <source>&lt;p&gt;This is the default weight for this room, which will be used for any exit &lt;i&gt;that leads to &lt;u&gt;this room&lt;/u&gt;&lt;/i&gt; which does not have its own value set - this value cannot be changed here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dies ist die normale Gewichtung dieses Raums. Sie wird für jeden Ausgang benutzt, der &lt;i&gt;zu &lt;u&gt;diesem Raum&lt;/u&gt; hinführt&lt;/i&gt; und keinen eigenen Wert gesetzt hat - Dieser Wert kann hier nicht geändert werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="863"/>
      <source>&lt;p&gt;Set the number of the room east of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Osten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1000"/>
      <source>&lt;p&gt;Set the number of the room down from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Unten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1152"/>
      <source>&lt;p&gt;Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Südwesten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1298"/>
      <source>&lt;p&gt;Set the number of the room south of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Süden von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1438"/>
      <source>&lt;p&gt;Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Südosten von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1578"/>
      <source>&lt;p&gt;Set the number of the room in from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Innen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1718"/>
      <source>&lt;p&gt;Set the number of the room out from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Außen von hier aus. Gültige Zahlen werden blau, ungültige werden rot.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1871"/>
      <source>&lt;p&gt;Set the number of the room that&apos;s to the southwest here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums in Richtung Südwesten von hier aus.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1874"/>
      <source>Exit RoomID number</source>
      <translation>Ausgangs RaumID Nummer</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1901"/>
      <source>No door</source>
      <translation>Keine Tür</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1916"/>
      <source>Open door</source>
      <translation>Offene Tür</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1928"/>
      <source>Closed door</source>
      <translation>Geschlossene Tür</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1940"/>
      <source>Locked door</source>
      <translation>Versperrte Tür</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1975"/>
      <source>&lt;p&gt;Use this button to save any changes, will also remove any invalid Special exits.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Benutze diese Schaltfläche, um alle Änderungen zu speichern, und auch alle ungültigen besonderen Ausgänge zu entfernen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1991"/>
      <source>&lt;p&gt;Use this button to close the dialogue without changing anything.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mit dieser Schaltfläche schließt du den Dialog ohne etwas zu ändern.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2010"/>
      <source>&lt;p&gt;Click on an item to edit/change it, to DELETE a Special Exit set its Exit Room ID to zero.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke auf ein Element, um es zu bearbeiten. Um einen besonderen Ausgang zu LÖSCHEN, setze die Raum ID des Ausgangs auf Null.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2048"/>
      <source>&lt;p&gt;Set the number of the room that this exit leads to, if set to zero the exit will be removed on saving the exits.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle die ID des Raums zu dem dieser Ausgang hinführt. Bei Null wird dieser Ausgang beim Speichern entfernt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2075"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit (only functional choice currently).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Für diesen Ausgang wird kein Symbol für die Tür auf der 2D Karte gezeichnet (derzeit einzige funktionale Auswahl).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2084"/>
      <source>&lt;p&gt;Green (Open) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein grünes Symbol für die (offene) Tür würde auf der 2D Karte gezeichnet (derzeit aber nicht).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2093"/>
      <source>&lt;p&gt;Orange (Closed) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein oranges Symbol für die (geschlossene) Tür würde auf der 2D Karte gezeichnet (derzeit aber nicht).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2102"/>
      <source>&lt;p&gt;Red (Locked) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ein rotes Symbol für die (versperrte) Tür würde auf der 2D Karte gezeichnet (derzeit aber nicht).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2111"/>
      <source>&lt;p&gt;(Lua scripts need to be prefixed with &quot;script:&quot;).&lt;/p&gt;</source>
      <translation>&lt;p&gt;(Lua Skripte brauchen &quot;script:&quot; vorangestellt).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2128"/>
      <source>&lt;p&gt;Add an empty item to Special exits to be edited as required.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ergänze ein leeres Element zu den besonderen Ausgängen, damit es wie benötigt bearbeitet werden kann.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2147"/>
      <source>&lt;p&gt;Press this button to deactivate the selection of a Special exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Klicke diese Schaltfläche um die Auswahl des besonderen Ausgangs zu deaktivieren.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1978"/>
      <source>&amp;Save</source>
      <translation>&amp;Speichern</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1886"/>
      <source>Exit Weight (0=No override)</source>
      <translation>Gewichtung des Ausgangs (0=Nicht überschreiben)</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1994"/>
      <source>&amp;Cancel</source>
      <translation>&amp;Abbrechen</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2001"/>
      <source>Special exits:</source>
      <translation>Spezielle Ausgänge:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2044"/>
      <source>Exit
Room ID</source>
      <translation>Ausgang
Raum-ID</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2053"/>
      <source>No
Route</source>
      <translation>Keine
Route</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2062"/>
      <source>Exit
Weight</source>
      <translation>Ausgang
Gewicht</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2066"/>
      <source>&lt;p&gt;Set to a positive integer value to override the default (Room) Weight for using this Exit route, a zero value assigns the default.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen positiven Wert, um die normale Gewichtung des Raums für diese Ausgangsrichtung zu setzen. Null setzt wieder den Standard.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2071"/>
      <source>Door
None</source>
      <translation>Tür
Keine</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2080"/>
      <source>Door
Open</source>
      <translation>Tür
Offen</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2089"/>
      <source>Door
Closed</source>
      <translation>Tür
Geschlossen</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2098"/>
      <source>Door
Locked</source>
      <translation>Tür
Versperrt</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2107"/>
      <source>Command
or LUA script</source>
      <translation>Befehl oder 
LUA Skript</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2131"/>
      <source>&amp;Add special exit</source>
      <translation>Besonderen Ausgang hinzufügen</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2150"/>
      <source>&amp;End S. Exits editing</source>
      <translation>Besonderen Auswahl abwählen</translation>
    </message>
  </context>
  <context>
    <name>room_symbol</name>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="14"/>
      <location filename="../src/ui/room_symbol.ui" line="112"/>
      <source>Room symbol</source>
      <translation>Raumsymbol</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="141"/>
      <source>Color of to use for the room symbol(s)</source>
      <translation>Farbe für die Raumsymbole</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="147"/>
      <source>Symbol color</source>
      <translation>Symbolfarbe</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="154"/>
      <source>Reset</source>
      <translation>Zurücksetzen</translation>
    </message>
  </context>
  <context>
    <name>scripts_main_area</name>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="40"/>
      <source>Registered Event Handlers:</source>
      <translation>Reagiere auf Ereignisse:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.&lt;/p&gt;&lt;p&gt;If a function withn the script is to be used to handle events entered in the list below &lt;b&gt;&lt;u&gt;it must have the same name as is entered here.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen guten (bestenfalls einzigartigen, aber muss nicht sein) Namen für dein Skript oder die Gruppe. Dieser wird in der Liste angezeigt werden.&lt;/p&gt;&lt;p&gt;Falls eine Funktion innerhalb des Skriptes zur Behandlung von Ereignissen aus der Liste unten vorgesehen ist, dann &lt;b&gt;&lt;u&gt;muss sie den gleichen Namen haben, der hier eigegeben wird.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="88"/>
      <source>&lt;p&gt;Remove (selected) event handler from list.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Entferne das ausgewählte Ereignis aus der Liste.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="91"/>
      <source>-</source>
      <translation>-</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="98"/>
      <source>Add User Event Handler:</source>
      <translation>Ergänze Ereignisse:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="133"/>
      <source>&lt;p&gt;Add entered event handler name to list.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Füge das genannte Ereignis der Liste hinzu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="136"/>
      <source>+</source>
      <translation>+</translation>
    </message>
  </context>
  <context>
    <name>set_room_area</name>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="14"/>
      <source>Move rooms to another area</source>
      <translation>Räume in ein anderes Gebiet verschieben</translation>
    </message>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="20"/>
      <source>Which area would you like to move the room(s) to?</source>
      <translation>In welches Gebiet willst du die Räume verschieben?</translation>
    </message>
  </context>
  <context>
    <name>source_editor_area</name>
    <message>
      <location filename="../src/ui/source_editor_area.ui" line="26"/>
      <source>Form</source>
      <translation>Form</translation>
    </message>
  </context>
  <context>
    <name>timers_main_area</name>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="29"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="64"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="169"/>
      <source>Time:</source>
      <translation>Zeit:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="39"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen guten (bestenfalls einzigartigen, aber muss nicht sein) Namen für deinen Timer, Offset-Timer oder die Gruppe. Dieser wird in der Liste angezeigt werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="74"/>
      <source>&lt;p&gt;Type in one or more commands you want the timer to send directly to the game when the time has elapsed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die der Timer direkt an das Spiel senden soll, wenn die Zeit abgelaufen ist. (Optional)&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="77"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Text, der unverändert ans Spiel gesendet werden soll (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="89"/>
      <source>&lt;p&gt;hours&lt;/p&gt;</source>
      <translation>&lt;p&gt;Stunden&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="104"/>
      <source>&lt;p&gt;minutes&lt;/p&gt;</source>
      <translation>&lt;p&gt;Minuten&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="119"/>
      <source>&lt;p&gt;seconds&lt;/p&gt;</source>
      <translation>&lt;p&gt;Sekunden&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="134"/>
      <source>&lt;p&gt;miliseconds&lt;/p&gt;</source>
      <translation>&lt;p&gt;Millisekunden&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="184"/>
      <source>&lt;p&gt;The &lt;b&gt;hour&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der &lt;b&gt;Stunden&lt;/b&gt;-Teil des Intervalls, nach dem der Timer ausgelöst wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="392"/>
      <source>&lt;p&gt;The &lt;b&gt;millisecond&lt;/b&gt; part of the interval that the timer will go off at (1000 milliseconds = 1 second).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der &lt;b&gt;Millisekunden&lt;/b&gt;-Teil des Intervalls, nach dem der Timer ausgelöst wird. (1000 Millisekunden = 1 Sekunde).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="260"/>
      <source>&lt;p&gt;The &lt;b&gt;minute&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der &lt;b&gt;Minuten&lt;/b&gt;-Teil des Intervalls, nach dem der Timer ausgelöst wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="326"/>
      <source>&lt;p&gt;The &lt;b&gt;second&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der &lt;b&gt;Sekunden&lt;/b&gt;-Teil des Intervalls, nach dem der Timer ausgelöst wird.&lt;/p&gt;</translation>
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
      <translation>1</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="436"/>
      <source>Show normally hidden variables</source>
      <translation>Zeige normalerweise verborgene Variablen</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="476"/>
      <source>&lt;p&gt;Enter text here to search through your code.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Gib hier Text ein, um deinen Code zu durchsuchen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="510"/>
      <source>&lt;p&gt;Toggles the display of the search results area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Steuert die Anzeige des Bereichs für Suchergebnisse.&lt;/p&gt;</translation>
    </message>
  </context>
  <context>
    <name>trigger_main_area</name>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="65"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="85"/>
      <source>&lt;p&gt;Use this control to show or hide the extra controls for the trigger; this can be used to allow more space to show the trigger &lt;i&gt;items&lt;/i&gt; on smaller screen.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Benutze dieses Steuerelement zum Anzeigen oder Ausblenden der zusätzlichen Bedienelemente des Triggers. Damit bleibt auf kleineren Bildschirmen mehr Raum für die einzelnen Trigger-&lt;i&gt;Elemente&lt;/i&gt;.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="111"/>
      <source>Command:</source>
      <translation>Befehl:</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="127"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Text, der unverändert ans Spiel gesendet werden soll (optional)</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="201"/>
      <source>&lt;p&gt;The trigger will only fire if &lt;u&gt;all&lt;/u&gt; conditions on the list have been met within the specified line delta, and captures will be saved in &lt;tt&gt;multimatches&lt;/tt&gt; instead of &lt;tt&gt;matches&lt;/tt&gt;.&lt;/p&gt;
&lt;p&gt;If this option is &lt;b&gt;not&lt;/b&gt; set the trigger will fire if &lt;u&gt;any&lt;/u&gt; condition on the list have been met.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Der Trigger wird nur ausgelöst, wenn &lt;u&gt;alle&lt;/u&gt; Bedingungen der Liste innerhalb des angegebenen Zeilenabstands erfüllt sind. Erfasstes wird als &lt;tt&gt;multimatches&lt;/tt&gt; gespeichert werden, anstelle von &lt;tt&gt;matches&lt;/tt&gt;.&lt;/p&gt; 
&lt;p&gt;Wenn diese Option &lt;b&gt;nicht&lt;/b&gt; gesetzt ist, wird der Trigger ausgelöst, wenn &lt;u&gt;eine&lt;/u&gt; Bedingung der Liste erfüllt worden ist.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="208"/>
      <source>AND / Multi-line (delta)</source>
      <translation>UND / Mehrzeilig (Delta)</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="260"/>
      <source>&lt;p&gt;When checked, only the filtered content (=capture groups) will be passed on to child triggers, not the initial line (see manual on filters).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wenn aktiviert, wird nur gefilterter Inhalt (=erfasste Gruppen, capture groups) an die untergeordneten Trigger weitergeleitet, nicht die ursprüngliche Zeile (siehe Anleitung zu Filtern).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="266"/>
      <source>only pass matches</source>
      <translation>Nur Übereinstimmungen übergeben</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="302"/>
      <source>Do not pass whole line to children.</source>
      <translation>Nicht die ganze Zeile an Kinder übergeben.</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="324"/>
      <source>&lt;p&gt;Choose this option if you want to include all possible matches of the pattern in the line.&lt;/p&gt;&lt;p&gt;Without this option, the pattern matching will stop after the first successful match.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle diese Option, wenn du alle möglichen Treffer des Musters in der Zeile aufnehmen möchtest.&lt;/p&gt;&lt;p&gt;Ohne diese Option wird nach dem ersten erfolgreichen Treffer des Musters nicht weitergemacht.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="360"/>
      <source>Match all occurences of the pattern in the line.</source>
      <translation>Treffe alle Vorkommnisse des Musters in der Zeile.</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="415"/>
      <source>&lt;p&gt;Keep firing the script for this many more lines, after the trigger or chain has matched.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Löse das Skript noch für so viele weitere Zeilen aus, nachdem der Trigger oder die Kette übergestimmt haben.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="421"/>
      <source>fire length (extra lines)</source>
      <translation>Weitere Zeilen auslösen</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="479"/>
      <source>&lt;p&gt;Play a sound file if the trigger fires.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Eine Audiodatei wiedergeben, wenn der Trigger ausgelöst wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="524"/>
      <source>&lt;p&gt;Use this to open a file selection dialogue to find a sound file to play when the trigger fires.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Cancelling from the file dialogue will not make any changes; to clear the file use the clear button to the right of the file name display.&lt;/i&gt;&lt;/p&gt;</source>
      <comment>This is the button used to select a sound file to be played when a trigger fires.</comment>
      <extracomment>Please ensure the text used here is duplicated within the tooltip for the QLineEdit that displays the file name selected.</extracomment>
      <translation>&lt;p&gt;Wähle hier eine Audiodatei, die beim Auslösen abgespielt werden soll.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Um die Datei zu entfernen, klicke die Schaltfläche rechts neben dem Dateinamen.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="528"/>
      <source>Choose file...</source>
      <translation>Datei auswählen...</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="559"/>
      <source>no file</source>
      <translation>Keine Datei</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="620"/>
      <source>&lt;p&gt;Enable this to highlight the matching text by changing the fore and background colors to the ones selected here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aktiviere diese Option, um passenden Text hervorzuheben, indem Vorder- und Hintergrundfarben in die hier ausgewählten Farben geändert werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="663"/>
      <source>Background</source>
      <translation>Hintergrund</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="679"/>
      <location filename="../src/ui/triggers_main_area.ui" line="692"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation>ohne</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="653"/>
      <source>Foreground</source>
      <translation>Vordergrund</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="482"/>
      <source>play sound</source>
      <translation>Ton abspielen</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="78"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wähle einen guten (bestenfalls einzigartigen, aber muss nicht sein) Namen für deinen Trigger oder die Gruppe. Dieser wird in der Liste angezeigt werden.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="124"/>
      <source>&lt;p&gt;Type in one or more commands you want the trigger to send directly to the game if it fires. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tippe ein oder mehrere Befehle, die der Trigger direkt an das Spiel senden soll, wenn der eingegebene Text dem Muster entspricht. (Optional)&lt;/p&gt;&lt;p&gt;Um komplexere Befehle zu senden, die von Variablen innerhalb dieses Profils abhängen oder diese verändern könnten, sollte &lt;i&gt;stattdessen&lt;/i&gt; ein Lua Skript im unteren Bereich des Editors eingegeben werden. Alles hier eingegebene wird buchstäblich nur zum Spielserver gesendet.&lt;/p&gt;&lt;p&gt;Man darf auch hier etwas &lt;i&gt;und&lt;/i&gt; ein Lua Skript eingeben - dieser Text wird gesendet &lt;b&gt;bevor&lt;/b&gt; das Skript ausgeführt wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="247"/>
      <source>&lt;p&gt;Within how many lines must all conditions be true to fire the trigger?&lt;/p&gt;</source>
      <translation>&lt;p&gt;Innerhalb von wie vielen Zeilen müssen alle Bedingungen wahr sein, um den Trigger auszulösen?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="330"/>
      <source>match all</source>
      <translation>Treffe alle</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="463"/>
      <source>&lt;p&gt;How many more lines, after the one that fired the trigger, should be passed to the trigger&apos;s children?&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wie viele Zeilen nach der Auslösung des Triggers sollen zusätzlich an seine Kinder übergeben werden?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="550"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <comment>This is the tooltip for the QLineEdit that shows - but does not permit changing - the sound file used for a trigger.</comment>
      <translation>&lt;p&gt;Audiodatei, die wiedergegeben werden soll, wenn der Trigger ausgelöst wird.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="572"/>
      <source>&lt;p&gt;Click to remove the sound file set for this trigger.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Hier klicken, um die Audiodatei für diesen Trigger zu entfernen.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="626"/>
      <source>highlight</source>
      <translation>Hervorheben</translation>
    </message>
  </context>
  <context>
    <name>trigger_pattern_edit</name>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="124"/>
      <source>Foreground color ignored</source>
      <translation>Vordergrundfarbe ignoriert</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="148"/>
      <source>Background color ignored</source>
      <translation>Hintergrundfarbe ignoriert</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="175"/>
      <source>match on the prompt line</source>
      <translation>Promptzeile erkennen</translation>
    </message>
  </context>
  <context>
    <name>vars_main_area</name>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="109"/>
      <source>Name:</source>
      <translation>Name:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="175"/>
      <source>⏴ Key type:</source>
      <translation>⏴ Art des Schlüsselwertes:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="125"/>
      <source>&lt;p&gt;Set the &lt;i&gt;global variable&lt;/i&gt; or the &lt;i&gt;table entry&lt;/i&gt; name here. The name has to start with a letter, but can contain a mix of letters and numbers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Setze hier den Namen der &lt;i&gt;globalen Variable&lt;/i&gt; oder des &lt;i&gt;Tabelleneintrags&lt;/i&gt;. Der Name muss mit einem Buchstaben beginnen, aber kann Buchstaben und Zahlen enthalten.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="138"/>
      <source>&lt;p&gt;Tables can store values either in a list, and/or a hashmap.&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;list&lt;/b&gt;, &lt;i&gt;unique indexed keys&lt;/i&gt; represent values - so you can have values at &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;map&lt;/b&gt; {a.k.a. an &lt;i&gt;associative array}&lt;/i&gt;, &lt;i&gt;unique keys&lt;/i&gt; represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).&lt;/p&gt;&lt;p&gt;This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.&lt;/p&gt;&lt;p&gt;In other cases it displays other entities (&lt;span style=&quot; font-style:italic;&quot;&gt;functions&lt;/span&gt;) which cannot be modifed from here.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Tabellen können Werte entweder als Liste und/oder als Hashtabelle speichern.&lt;/p&gt;&lt;p&gt;In einer &lt;b&gt;Liste&lt;/b&gt;, repräsentieren &lt;i&gt;einzigartige nummerierte Schlüssel&lt;/i&gt; die Werte - also kannst du Werte haben bei &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In einer &lt;b&gt;Hashtabelle&lt;/b&gt; {bzw. einem &lt;i&gt;assoziativen Datenfeld&lt;/i&gt;} repräsentieren &lt;i&gt;einzigartige Schlüssel&lt;/i&gt; die Werte - du kannst also jeden Wert unter jeder gewünschten Bezeichnung eintragen (theoretisch sogar einer Funktion oder anderen Lua Objekten, aber diese GUI unterstützt nur Strings).&lt;/p&gt;&lt;p&gt;Hier wählst du für neu erstellte Tabellen(-gruppen), ob es eine nummerierte oder assoziative Tabelle sein soll.&lt;/p&gt;&lt;p&gt;Andernfalls werden andere Objekte (&lt;span style=&quot; font-style:italic;&quot;&gt;Funktionen&lt;/span&gt;) angezeigt, die von hier nicht verändert werden können.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="51"/>
      <location filename="../src/ui/vars_main_area.ui" line="145"/>
      <source>Auto-Type</source>
      <translation>Automatisch</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="128"/>
      <source>Variable name ...</source>
      <translation>Name der Variable ...</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="150"/>
      <source>key (string)</source>
      <translation>Schlüsselwert (String)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="155"/>
      <source>index (integer)</source>
      <translation>Index (Zahl)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="71"/>
      <location filename="../src/ui/vars_main_area.ui" line="160"/>
      <source>table</source>
      <translation>Tabelle</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="165"/>
      <source>function
(cannot create
from GUI)</source>
      <translation>Funktion
(nicht in GUI
erstellbar)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="185"/>
      <source>&lt;p&gt;If checked this item (and its children, if applicable) does not show in area to the left unless &lt;b&gt;Show normally hidden variables&lt;/b&gt; is checked.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Falls gewählt, wird dieses Objekt (und wo möglich seine Kinder) nicht in dem Bereich links angezeigt, außer &lt;b&gt;Zeige normalerweise verborgene Variablen&lt;/b&gt; ist ausgewählt.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="188"/>
      <source>hidden variable</source>
      <translation>Verborgene Variable</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="84"/>
      <source>⏷ Value type:</source>
      <translation>⏷ Art des Wertes:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="56"/>
      <source>string</source>
      <translation>String</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="61"/>
      <source>number</source>
      <translation>Zahl</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="66"/>
      <source>boolean</source>
      <translation>Boolean</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="76"/>
      <source>function</source>
      <translation>Funktion</translation>
    </message>
  </context>
</TS>
