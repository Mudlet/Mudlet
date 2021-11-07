<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="pl" sourcelanguage="en">
  <context>
    <name>Discord</name>
    <message>
      <location filename="../src/discord.cpp" line="150"/>
      <source>via Mudlet</source>
      <translation>przez Mudlet</translation>
    </message>
  </context>
  <context>
    <name>Feed</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="275"/>
      <source>Too many redirects.</source>
      <translation>Zbyt wiele przekierowań.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="284"/>
      <source>No data received from server</source>
      <translation>Nie otrzymano danych z serwera</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="295"/>
      <source>Could not verify download integrity.</source>
      <translation>Nie można zweryfikować integralności pobranych danych.</translation>
    </message>
  </context>
  <context>
    <name>GLWidget</name>
    <message>
      <location filename="../src/glwidget.cpp" line="288"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>Brak lokacji w mapie - załaduj inną lub zacznij mapowanie od początku.</translation>
    </message>
    <message>
      <location filename="../src/glwidget.cpp" line="293"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Nie masz jeszcze mapy - załaduj jedną lub rozpocznij mapowanie od zera, aby rozpocząć.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/glwidget.cpp" line="290"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>Masz załadowaną mapę (%n pokoi(je)), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>Host</name>
    <message>
      <location filename="../src/Host.cpp" line="469"/>
      <source>Text to send to the game</source>
      <translation>Tekst do wysłania do gry</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="873"/>
      <source>[  OK  ]  - %1 Thanks a lot for using the Public Test Build!</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ] - %1 Wielkie dzięki za korzystanie z kompilacji do testów publicznych!</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="874"/>
      <source>[  OK  ]  - %1 Help us make Mudlet better by reporting any problems.</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[ OK ] - %1 Pomóż nam ulepszyć Mudlet, zgłaszając wszelkie problemy.</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1674"/>
      <source>Unpacking module:
&quot;%1&quot;
please wait...</source>
      <translation>Rozpakowywanie modułu:&quot;%1&quot; proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1676"/>
      <source>Unpacking package:
&quot;%1&quot;
please wait...</source>
      <translation>Rozpakowywanie pakietu:&quot;%1&quot; proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1680"/>
      <source>Unpacking</source>
      <translation>Rozpakowywanie</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2322"/>
      <source>Playing %1</source>
      <translation>Gra w %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2324"/>
      <location filename="../src/Host.cpp" line="2330"/>
      <source>%1 at %2:%3</source>
      <comment>%1 is the game name and %2:%3 is game server address like: mudlet.org:23</comment>
      <translation>%1 na %2:%3</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2716"/>
      <location filename="../src/Host.cpp" line="3648"/>
      <source>Map - %1</source>
      <translation>Mapa - %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3664"/>
      <source>Pre-Map loading(3) report</source>
      <translation>Wstępne ładowanie mapy(3) raport</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3674"/>
      <source>Loading map(3) at %1 report</source>
      <translation>Ładowanie mapy (3) w raporcie %1</translation>
    </message>
  </context>
  <context>
    <name>KeyUnit</name>
    <message>
      <location filename="../src/KeyUnit.cpp" line="333"/>
      <source>%1undefined key (code: 0x%2)</source>
      <comment>%1 is a string describing the modifier keys (e.g. &quot;shift&quot; or &quot;control&quot;) used with the key, whose &apos;code&apos; number, in %2 is not one that we have a name for. This is probably one of those extra keys around the edge of the keyboard that some people have.</comment>
      <translation>%1 niezdefiniowany klucz (kod: 0x%2)</translation>
    </message>
  </context>
  <context>
    <name>MapInfoContributorManager</name>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="116"/>
      <source>Area:%1%2 ID:%1%3 x:%1%4%1&lt;‑&gt;%1%5 y:%1%6%1&lt;‑&gt;%1%7 z:%1%8%1&lt;‑&gt;%1%9
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and non-breaking hyphens which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. %2 is the (text) name of the area, %3 is the number for it, %4 to %9 are pairs (min &lt;-&gt; max) of extremes for each of x,y and z coordinates</comment>
      <translation>Obszar:%1%2 ID:%1%3 x:%1%4%1&lt;–&gt;%1%5 y:%1%6%1&lt;–&gt;%1%7 z:%1%8%1&lt;–&gt;%1%9
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="140"/>
      <source>Room Name: %1
</source>
      <translation>Nazwa pokoju: %1
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="153"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when NO rooms are selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the current player&apos;s room.</comment>
      <translation>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1aktualna lokacja gracza
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="170"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when ONE room is selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the selected Room.</comment>
      <translation>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1wybrany pokój
</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mapInfoContributorManager.cpp" line="188"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handle them literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
      <translation>
        <numerusform>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1wyśrodkuj %n zaznaczone pokoje
</numerusform>
        <numerusform>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1wyśrodkuj %n zaznaczone pokoje
</numerusform>
        <numerusform>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1wyśrodkuj %n zaznaczone pokoje
</numerusform>
        <numerusform>Pokój%1ID:%1%2 Pozycja%1na%1mapie: (%3,%4,%5) -%1wyśrodkuj %n zaznaczone pokoje
</numerusform>
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
      <translation>! %1 jest nieobecny (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="146"/>
      <source>! %1 is back</source>
      <translation>! %1 jest z powrotem</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="153"/>
      <source>! invited %1 to %2</source>
      <translation>! zaproszono %1 do %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="156"/>
      <source>! %2 invited to %3</source>
      <translation>! %2 zaprosił %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="163"/>
      <source>! You have joined %1 as %2</source>
      <translation>! Dołączyłeś do %1 jako %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="165"/>
      <source>! %1 has joined %2</source>
      <translation>! %1 dołączył do %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="172"/>
      <source>! %1 kicked %2</source>
      <translation>! %1 wyrzucił %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="180"/>
      <source>! %1 mode is %2 %3</source>
      <translation>! Tryb %1 to %2 %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="182"/>
      <source>! %1 sets mode %2 %3 %4</source>
      <translation>! %1 ustawia tryb %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="199"/>
      <source>[MOTD] %1%2</source>
      <translation>[MOTD] %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="211"/>
      <source>! %1 has %2 users: %3</source>
      <translation>! %1 ma %2 użytkowników: %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="213"/>
      <source>! %1 has %2 users</source>
      <translation>! %1 ma %2 użytkowników</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="220"/>
      <source>! %1 has changed nick to %2</source>
      <translation>! %1 zmienił pseudonim na %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="234"/>
      <source>! %1 replied in %2</source>
      <translation>! %1 odpowiedział w %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="237"/>
      <location filename="../src/ircmessageformatter.cpp" line="286"/>
      <source>! %1 time is %2</source>
      <translation>! czas %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="240"/>
      <location filename="../src/ircmessageformatter.cpp" line="283"/>
      <source>! %1 version is %2</source>
      <translation>! %1 jest w wersji %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="256"/>
      <source>[%1%2] %3</source>
      <translation>[%1%2] %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="264"/>
      <source>&amp;lt;%1%2&amp;gt; [%3] %4</source>
      <translation>&amp;&lt;%1%2&amp;&gt; [%3] %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="278"/>
      <source>[INFO] %1</source>
      <translation>[INFO] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="305"/>
      <location filename="../src/ircmessageformatter.cpp" line="331"/>
      <source>[ERROR] %1</source>
      <translation>[ERROR] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="315"/>
      <source>[Channel URL] %1</source>
      <translation>[Adres URL kanału] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="324"/>
      <source>[%1] %2</source>
      <translation>[%1] %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="338"/>
      <source>! %1 has left %2</source>
      <translation>! %1 opuścił(a) %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="340"/>
      <source>! %1 has left %2 (%3)</source>
      <translation>! %1 opuścił %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="349"/>
      <source>! %1 replied in %2 seconds</source>
      <translation>! %1 odpowiedział w %2 sekund</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="363"/>
      <source>* %1 %2</source>
      <translation>* %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="369"/>
      <source>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</source>
      <translation>&lt;b&gt;&amp;&lt;%1&amp;&gt;&lt;/b&gt; %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="378"/>
      <source>! %1 has quit</source>
      <translation>! %1 wyszedł</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="380"/>
      <source>! %1 has quit (%2)</source>
      <translation>! %1 wyszedł (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="388"/>
      <source>! no topic</source>
      <translation>! bez tematu</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="397"/>
      <source>[TOPIC] %1</source>
      <translation>[TOPIC] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="401"/>
      <source>! %2 cleared topic</source>
      <translation>! %2 wyczyścił temat</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="404"/>
      <source>! %2 changed topic</source>
      <translation>! %2 zmienił(a) temat</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="410"/>
      <source>? %2 %3 %4</source>
      <translation>? %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="417"/>
      <source>[WHOIS] %1 is %2@%3 (%4)</source>
      <translation>[WHOIS] %1 to %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="418"/>
      <source>[WHOIS] %1 is connected via %2 (%3)</source>
      <translation>[WHOIS] %1 łączy się przez %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="419"/>
      <source>[WHOIS] %1 is connected since %2 (idle %3)</source>
      <translation>[WHOIS] %1 jest połączony/a od %2 (idle %3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="421"/>
      <source>[WHOIS] %1 is away: %2</source>
      <translation>[WHOIS] %1 jest nieobecny: %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="424"/>
      <source>[WHOIS] %1 is logged in as %2</source>
      <translation>[WHOIS] %1 jest zalogowany jako %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="427"/>
      <source>[WHOIS] %1 is connected from %2</source>
      <translation>[WHOIS] %1 łączy się z %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="430"/>
      <source>[WHOIS] %1 is using a secure connection</source>
      <translation>[WHOIS] %1 używa bezpiecznego połączenia</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="433"/>
      <source>[WHOIS] %1 is on %2</source>
      <translation>[WHOIS] %1 jest w %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="442"/>
      <source>[WHOWAS] %1 was %2@%3 (%4)</source>
      <translation>[KTOBYŁ] %1 był %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="443"/>
      <source>[WHOWAS] %1 was connected via %2 (%3)</source>
      <translation>[WHOWAS] %1 był podłączony poprzez %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="445"/>
      <source>[WHOWAS] %1 was logged in as %2</source>
      <translation>[WHOWAS] %1 był zalogowany jako %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="453"/>
      <source>[WHO] %1 (%2)</source>
      <translation>[WHO] %1 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="455"/>
      <source> - away</source>
      <translation> - nieobecny</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="458"/>
      <source> - server operator</source>
      <translation> -operator serwera</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="466"/>
      <source>%1s</source>
      <translation>%1s</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="473"/>
      <source>%1 days</source>
      <translation>%1 dni</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="477"/>
      <source>%1 hours</source>
      <translation>%1 godzin</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="481"/>
      <source>%1 mins</source>
      <translation>%1 minut</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="483"/>
      <source>%1 secs</source>
      <translation>%1 sekund</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="55"/>
      <source>Start element not found!</source>
      <translation>Element początkowy nie został znaleziony!</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="67"/>
      <source>line %1: %2</source>
      <translation>linia %1: %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="149"/>
      <source>Expected %1 while parsing</source>
      <translation>Oczekiwano %1 podczas analizowania</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/jsonparser.cpp" line="145"/>
      <source>%1 @ line %2</source>
      <translation>%1 @ linia %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="82"/>
      <source>No data found!</source>
      <translation>Nie znaleziono żadnych danych!</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="89"/>
      <source>Expected object in keymap
</source>
      <translation>Oczekiwany obiekt w mapie klawiszy
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="129"/>
      <source>Invalid keysequence used %1
</source>
      <translation>Użyto niepoprawnej sekwencji klawiszy %1
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/texteditorkeymap.cpp" line="371"/>
      <source>Error parsing %1: %2 </source>
      <translation>Błąd podczas analizowania %1: %2 </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/textgrammar.cpp" line="306"/>
      <source>Error reading file %1:%2</source>
      <translation>Błąd wczytywania pliku %1:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="435"/>
      <source>%1 ranges</source>
      <translation>%1 zakresy</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="441"/>
      <source>Line %1, Column %2</source>
      <translation>Linia %1, Kolumna %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="444"/>
      <source>, Offset %1</source>
      <translation>, Przesunięcie %1</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="448"/>
      <source> | %1 chars selected</source>
      <translation> | %1 wybranych znaków</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="452"/>
      <source> | scope: </source>
      <translation> | zakres: </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="462"/>
      <source> (%1)</source>
      <translation> (%1)</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="399"/>
      <source>Error parsing theme %1:%2</source>
      <translation>Błąd przetwarzania motywu %1:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="404"/>
      <source>Error theme not found %1.</source>
      <translation>Nie znaleziono motywu błędu %1.</translation>
    </message>
  </context>
  <context>
    <name>T2DMap</name>
    <message>
      <location filename="../src/T2DMap.cpp" line="2989"/>
      <source>Change the properties of this custom line</source>
      <translation>Zmień właściwości tej linii</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3217"/>
      <location filename="../src/T2DMap.cpp" line="4822"/>
      <source>Solid line</source>
      <translation>Linia ciągła</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3218"/>
      <location filename="../src/T2DMap.cpp" line="4823"/>
      <source>Dot line</source>
      <translation>Linia kropkowana</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3219"/>
      <location filename="../src/T2DMap.cpp" line="4824"/>
      <source>Dash line</source>
      <translation>Linia kreskowa</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3220"/>
      <location filename="../src/T2DMap.cpp" line="4825"/>
      <source>Dash-dot line</source>
      <translation>Linia kreska-kropka</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3221"/>
      <location filename="../src/T2DMap.cpp" line="4826"/>
      <source>Dash-dot-dot line</source>
      <translation>Linia kreska-kropka-kropka</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3509"/>
      <source>x coordinate (was %1):</source>
      <translation>współrzędna x (była %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3510"/>
      <source>y coordinate (was %1):</source>
      <translation>współrzędna y (była %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3511"/>
      <source>z coordinate (was %1):</source>
      <translation>współrzędna z (była %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3699"/>
      <source>Delete color</source>
      <comment>Deletes an environment colour</comment>
      <translation>Usuń kolor</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3717"/>
      <source>Define new color</source>
      <translation>Zdefiniuj nowy kolor</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4030"/>
      <source>%1 {count:%2}</source>
      <translation>%1 {ilość: %2}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1382"/>
      <location filename="../src/T2DMap.cpp" line="1503"/>
      <location filename="../src/T2DMap.cpp" line="2318"/>
      <location filename="../src/T2DMap.cpp" line="2334"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>brak tekstu</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="140"/>
      <source>ID</source>
      <comment>Room ID in the mapper widget</comment>
      <translation>ID</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="140"/>
      <source>Name</source>
      <comment>Room name in the mapper widget</comment>
      <translation>Nazwa</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="142"/>
      <source>&lt;p&gt;Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection.  Click on the relevant header to sort by that method.  Note that the name column will only show if at least one of the rooms has a name.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij na linię, aby zaznaczyć lub odznaczyć numer pokoju (z podaną nazwą, jeśli pokoje są nazwane), aby dodać lub usunąć pokój z zaznaczenia.  Kliknij na odpowiedni nagłówek, aby posortować go według tej metody.  Zwróć uwagę, że kolumna z nazwą będzie wyświetlana tylko wtedy, gdy przynajmniej jeden z pokoi ma swoją nazwę.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="600"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation>Mapper: Nie może znaleźć ścieżki od %1 do %2 używając znanych wyjść.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1255"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>Nie masz jeszcze mapy - załaduj jedną lub rozpocznij mapowanie od zera, aby rozpocząć.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/T2DMap.cpp" line="1252"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>Masz załadowaną mapę (%n pokoi), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi(je)), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi(je)), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
        <numerusform>Masz załadowaną mapę (%n pokoi(je)), ale Mudlet nie wie, gdzie jesteś w tej chwili.</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1250"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>Brak lokacji w mapie - załaduj inną lub zacznij mapowanie od początku.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2252"/>
      <source>render time: %1S mO: (%2,%3,%4)</source>
      <comment>This is debug information that is not expected to be seen in release versions, %1 is a decimal time period and %2-%4 are the x,y and z coordinates at the center of the view (but y will be negative compared to previous room related ones as it represents the real coordinate system for this widget which has y increasing in a downward direction!)</comment>
      <translation>czas przetwarzania: %1S mO: (%2,%3,%4)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2324"/>
      <source>Text label or image label?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation>Etykieta tekstowa czy obraz?</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2325"/>
      <source>Text Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Tekst etykiety</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2326"/>
      <source>Image Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Etykieta obrazu</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2330"/>
      <source>Enter label text.</source>
      <comment>2D Mapper create label dialog title/text</comment>
      <translation>Wprowadź tekst etykiety.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2337"/>
      <source>Background color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Kolor tła</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2338"/>
      <source>Foreground color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Kolor pierwszego planu</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2342"/>
      <source>Select image</source>
      <comment>2D Mapper create label file dialog title</comment>
      <translation>Wybierz obraz</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2349"/>
      <source>Draw label as background or on top of everything?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation>Narysować etykietę jako tło czy na wierzchu?</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2350"/>
      <source>Background</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Tło</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2351"/>
      <source>Foreground</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Pierwszy plan</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2601"/>
      <source>Drag to select multiple rooms or labels, release to finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Przeciągnij, aby zaznaczyć kilka pokoi lub etykiet, puść przycisk aby zakończyć...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2705"/>
      <source>Undo</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Cofnij</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2706"/>
      <source>Undo last point</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Cofnij ostatnią edycję</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2713"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)</comment>
      <translation>Właściwości</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2715"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)</comment>
      <translation>właściwości...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2716"/>
      <source>Change the properties of this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Zmień właściwości tej linii</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2719"/>
      <source>Finish</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Zakończ</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2720"/>
      <source>Finish drawing this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Zakończ rysowanie tej linii</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2786"/>
      <source>Create new map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Utwórz nową mapę</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2789"/>
      <source>Load map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Wczytaj mapę</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2811"/>
      <source>Move</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Przenieś</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2883"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Skasuj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2850"/>
      <source>Increase map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Zwiększ odległość między zaznaczonymi lokacjami na osi X-Y</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2857"/>
      <source>Decrease map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Zmniejsz odległość między zaznaczonymi lokacjami na osi X-Y</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2865"/>
      <source>Lock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Zablokuj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2866"/>
      <source>Lock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Zablokuj lokację dla łazików</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2870"/>
      <source>Unlock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Odblokuj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2871"/>
      <source>Unlock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Odblokuj lokację dla łazików</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2843"/>
      <source>Set one or more symbols or letters to mark special rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Ustaw jeden lub więcej symboli bądź liter aby oznaczyć specjalne lokacje</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2825"/>
      <source>Replace an exit line with a custom line</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Zamień linię wyjścia na linię niestandardową</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2829"/>
      <source>Custom exit lines are not shown and are not editable in grid mode</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation>Niestandardowe linie wyjścia nie są widoczne i nie mogą być edytowane w trybie siatki</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2805"/>
      <source>Create new room here</source>
      <comment>Menu option to create a new room in the mapper</comment>
      <translation>Stwórz nową lokację w tym miejscu</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2817"/>
      <source>Set exits...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ustaw wyjścia...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2823"/>
      <source>Create exit line...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Utwórz linię wyjścia...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2836"/>
      <source>Set color...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ustaw kolor...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2842"/>
      <source>Set symbol...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ustaw symbol...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2849"/>
      <source>Spread...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Rozprosz...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2856"/>
      <source>Shrink...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ścieśnij...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2877"/>
      <source>Set weight...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ustaw wagę...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2889"/>
      <source>Move to position...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Przenieś na pozycję...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2890"/>
      <source>Move selected room or group of rooms to the given coordinates in this area</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Przesuń wybrany pokój lub grupę pokoi na podane współrzędne w tym obszarze</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2896"/>
      <source>Move to area...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Przenieś do obszaru...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2901"/>
      <source>Create label...</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Utwórz etykietę...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2902"/>
      <source>Create label to show text or an image</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Utwórz etykietę, aby wyświetlić tekst lub obraz</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2908"/>
      <source>Set player location</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Ustaw lokalizację gracza</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2909"/>
      <source>Set the player&apos;s current location to here</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Ustaw tutaj aktualną lokację gracza</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2918"/>
      <source>Switch to editing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Przełącz do trybu edycji</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2919"/>
      <source>Switch to viewing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Przełącz do trybu wyświetlania</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2926"/>
      <source>Move</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Przenieś</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2927"/>
      <source>Move label</source>
      <comment>2D Mapper context menu item (label) tooltip</comment>
      <translation>Przenieś etykietę</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2929"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Skasuj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2930"/>
      <source>Delete label</source>
      <comment>2D Mapper context menu (label) item tooltip</comment>
      <translation>Usuń etykietę</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2944"/>
      <source>Add point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Dodaj punkt</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2951"/>
      <source>Divide segment by adding a new point mid-way along</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state)</comment>
      <translation>Podziel segment przez dodanie nowego punktu w połowie drogi</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2954"/>
      <source>Select a point first, then add a new point mid-way along the segment towards room</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)</comment>
      <translation>Wybierz pierwszy punkt, następnie dodaj kolejny punkt w połowie drogi do lokacji</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2958"/>
      <source>Remove point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Usuń punkt</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2965"/>
      <source>Merge pair of segments by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)</comment>
      <translation>Złącz parę odcinków usuwając ten punkt</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2969"/>
      <source>Remove last segment by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)</comment>
      <translation>Usuń ostatni odcinek usuwając ten punkt</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2974"/>
      <source>use &quot;delete line&quot; to remove the only segment ending in an editable point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action &quot;delete line&quot; should match the translation for that action)</comment>
      <translation>użyj &quot;usuń linię&quot;, aby usunąć jedynie segment kończący się w punkcie edytowalnym</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2980"/>
      <source>Select a point first, then remove it</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)</comment>
      <translation>Najpierw wybierz punkt, potem go usuń</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2984"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)</comment>
      <translation>Właściwości</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2988"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise</comment>
      <translation>właściwości...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2992"/>
      <source>Delete line</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Usuń linię</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2993"/>
      <source>Delete all of this custom line</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip</comment>
      <translation>Usuń całą linię</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3502"/>
      <source>Move the selection, centered on
the highlighted room (%1) to:</source>
      <comment>Use linefeeds as necessary to format text into a reasonable rectangle of text, %1 is a room number</comment>
      <translation>Przesuń zaznaczenie, wyśrodkowane na
podświetlonej lokacji (%1), do:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3526"/>
      <source>OK</source>
      <comment>dialog (room(s) move) button</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3531"/>
      <source>Cancel</source>
      <comment>dialog (room(s) move) button</comment>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3725"/>
      <source>OK</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3730"/>
      <source>Cancel</source>
      <comment>dialog (room(s) change color) button</comment>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3787"/>
      <source>Spread out rooms</source>
      <translation>Rozłóż lokacje</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3788"/>
      <source>Increase the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>Zwiększ odległość między wybranymi lokacjami, z wyśrodkowaniem na podkreślonym pokoju, przez współczynnik:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3847"/>
      <source>Shrink in rooms</source>
      <translation>Zmniejsz odległości pomiędzy lokacjami</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3848"/>
      <source>Decrease the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>Zmniejsz odstępy pomiędzy wybranymi lokacjami, środkując na podświetlonej lokacji, o współczynnik:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3975"/>
      <location filename="../src/T2DMap.cpp" line="3989"/>
      <location filename="../src/T2DMap.cpp" line="4039"/>
      <source>Enter room weight</source>
      <translation>Podaj wagę lokacji (roomweight w API)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3976"/>
      <source>Enter new roomweight
(= travel time), minimum
(and default) is 1:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Podaj nową wagę lokacji (= czas podróży, roomweight w API), najmniejsza (oraz domyślna) wartość to 1:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3990"/>
      <source>Enter new roomweight
(= travel time) for all
selected rooms, minimum
(and default) is 1 and
the only current value
used is:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Podaj nową wagę (= czas podróży, roomweight w API) dla wszystkich zaznaczonych lokacji, najmniejsza (oraz domyślna) wartość to 1:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4028"/>
      <source>%1 {count:%2, default}</source>
      <translation>%1 {ilość:%2, domyślne}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4036"/>
      <source>1 {count 0, default}</source>
      <translation>1 {ilość 0, domyślne}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4040"/>
      <source>Choose an existing
roomweight (= travel
time) from the list
(sorted by most commonly
used first) or enter a
new (positive) integer
value for all selected
rooms:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Wybierz wagę lokacji (= czas podróży) z listy (posortowanej według częstotliwości użycia) lub wprowadź nową dodatnią liczbę jako wagę dla wszystkich zaznaczonych lokacji:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4089"/>
      <source>Load Mudlet map</source>
      <translation>Załaduj mapę Mudletową</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4091"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) or the ;;s as they are used programmatically</comment>
      <translation>Mapa Mudlet'a (*.dat);;Dane mapy w formacie xml (*.xml);;Dowolny plik (*)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4969"/>
      <location filename="../src/T2DMap.cpp" line="5003"/>
      <source>Left-click to add point, right-click to undo/change/finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Kliknij lewy przycisk aby dodać punkt, kliknij prawym przyciskiem myszy, aby anulować / zmienić / zakończyć ...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5014"/>
      <source>Left-click and drag a square for the size and position of your label</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Przytrzymaj lewy przycisk myszy i przeciągnij ustalając rozmiar i pozycję etykiety</translation>
    </message>
  </context>
  <context>
    <name>TAlias</name>
    <message>
      <location filename="../src/TAlias.cpp" line="132"/>
      <location filename="../src/TAlias.cpp" line="200"/>
      <source>[Alias Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation>[ Błąd aliasu:] Przekroczono limit %1 grup przechwytywania, przechwyć mniej grup.
</translation>
    </message>
    <message>
      <location filename="../src/TAlias.cpp" line="269"/>
      <source>Error: in &quot;Pattern:&quot;, faulty regular expression, reason: &quot;%1&quot;.</source>
      <translation>Błąd: w &quot;Wzór:&quot;, błędne wyrażenie regularne, powód: &quot;%1&quot;.</translation>
    </message>
  </context>
  <context>
    <name>TArea</name>
    <message>
      <location filename="../src/TArea.cpp" line="376"/>
      <source>roomID=%1 does not exist, can not set properties of a non-existent room!</source>
      <translation>roomID=%1 nie istnieje, nie można ustawić właściwości nieistniejącej lokacji!</translation>
    </message>
    <message>
      <location filename="../src/TArea.cpp" line="765"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>brak tekstu</translation>
    </message>
  </context>
  <context>
    <name>TCommandLine</name>
    <message>
      <location filename="../src/TCommandLine.cpp" line="678"/>
      <source>Add to user dictionary</source>
      <translation>Dodaj do słownika użytkownika</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="680"/>
      <source>Remove from user dictionary</source>
      <translation>Usuń ze słownika użytkownika</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="684"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which we have bundled with Mudlet; the entries about this line are the ones that the user has personally added.</comment>
      <translation>▼Mudlet ▼B │ propozycje słownikowe │ ▲ Użytkownik▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="694"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which is provided as part of the OS; the entries about this line are the ones that the user has personally added.</comment>
      <translation>▼System ▼│ propozycje słownikowe │ ▲ Użytkownik▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="762"/>
      <source>no suggestions (system)</source>
      <comment>used when the command spelling checker using the selected system dictionary has no words to suggest</comment>
      <translation>brak sugestii (system)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="789"/>
      <source>no suggestions (shared)</source>
      <comment>used when the command spelling checker using the dictionary shared between profile has no words to suggest</comment>
      <translation>brak sugestii (udostępnione)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="793"/>
      <source>no suggestions (profile)</source>
      <comment>used when the command spelling checker using the profile&apos;s own dictionary has no words to suggest</comment>
      <translation>brak sugestii (profil)</translation>
    </message>
  </context>
  <context>
    <name>TConsole</name>
    <message>
      <location filename="../src/TConsole.cpp" line="81"/>
      <source>Debug Console</source>
      <translation>Konsola debugowania</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="771"/>
      <source>Save profile?</source>
      <translation>Zapisać profil?</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="771"/>
      <source>Do you want to save the profile %1?</source>
      <translation>Czy chcesz zapisać profil %1?</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="784"/>
      <source>Couldn&apos;t save profile</source>
      <translation>Nie udało się zapisać profilu</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="784"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation>Przepraszamy, nie udało się zapisać profilu - otrzymano następujący błąd: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1705"/>
      <source>System Message: %1</source>
      <translation>Komunikat systemowy: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="324"/>
      <source>Show Time Stamps.</source>
      <translation>Pokaż znaczniki czasu.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="336"/>
      <source>Record a replay.</source>
      <translation>Nagraj powtórkę.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="347"/>
      <source>Start logging game output to log file.</source>
      <translation>Rozpocznij logowanie wyjścia gry do pliku.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="358"/>
      <source>&lt;i&gt;N:&lt;/i&gt; is the latency of the game server and network (aka ping, in seconds), &lt;br&gt;&lt;i&gt;S:&lt;/i&gt; is the system processing time - how long your triggers took to process the last line(s).</source>
      <translation>&lt;i&gt;N:&lt;/i&gt; jest opóźnieniem serwera i sieci gry (czyli ping, w sekundach), &lt;br&gt;&lt;i&gt;S:&lt;/i&gt; to czas przetwarzania systemu - jak długo twoje skrypty przetwarzały ostatnią linię tekstu.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="391"/>
      <source>Emergency Stop. Stops all timers and triggers.</source>
      <translation>Awaryjne zatrzymanie wszystkich liczników czasu oraz wyzwalaczy.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="407"/>
      <source>Search buffer.</source>
      <translation>Bufor wyszukiwania.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="415"/>
      <source>Earlier search result.</source>
      <translation>Poprzedni wynik wyszukiwania.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="426"/>
      <source>Later search result.</source>
      <translation>Następny wynik wyszukiwania.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="868"/>
      <source>Replay recording has started. File: %1</source>
      <translation>Rozpoczęto nagrywanie powtórki. Plik: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="871"/>
      <source>Replay recording has been stopped. File: %1</source>
      <translation>Nagrywanie powtórki zostało zatrzymane. Plik: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1813"/>
      <location filename="../src/TConsole.cpp" line="1852"/>
      <source>No search results, sorry!</source>
      <translation>Brak wyników wyszukiwania, przepraszam!</translation>
    </message>
  </context>
  <context>
    <name>TEasyButtonBar</name>
    <message>
      <location filename="../src/TEasyButtonBar.cpp" line="70"/>
      <source>Easybutton Bar - %1 - %2</source>
      <translation>Pasek ŁatwychPrzycisków - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TLuaInterpreter</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="10279"/>
      <source>Playing %1</source>
      <translation>Gra w %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12738"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12759"/>
      <source>ERROR</source>
      <translation>BŁĄD</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12739"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12750"/>
      <source>object</source>
      <comment>object is the Mudlet alias/trigger/script, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation>obiekt</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12739"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12750"/>
      <source>function</source>
      <comment>function is the Lua function, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation>funkcja</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14237"/>
      <source>Some functions may not be available.</source>
      <translation>Niektóre funkcje mogą być niedostępne.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13647"/>
      <source>No error message available from Lua</source>
      <translation>Brak komunikatu o błędzie z Lua</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13649"/>
      <source>Lua error: %1</source>
      <translation>Błąd Lua: %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13651"/>
      <source>[ ERROR ] - Cannot find Lua module %1.%2%3%4</source>
      <comment>%1 is the name of the module;%2 will be a line-feed inserted to put the next argument on a new line;%3 is the error message from the lua sub-system;%4 can be an additional message about the expected effect (but may be blank).</comment>
      <translation>[ BŁĄD ] - Nie można znaleźć modułu Lua %1.%2%3%4</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13665"/>
      <source>[  OK  ]  - Lua module %1 loaded.</source>
      <comment>%1 is the name (may specify which variant) of the module.</comment>
      <translation>[  OK  ] - Moduł Lua %1 załadowany.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14224"/>
      <source>Probably will not be able to access Mudlet Lua code.</source>
      <translation>Prawdopodobnie nie będzie w stanie uzyskać dostępu do kodu Mudlet Lua.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14242"/>
      <source>Database support will not be available.</source>
      <translation>Obsługa bazy danych nie będzie dostępna.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14249"/>
      <source>utf8.* Lua functions won&apos;t be available.</source>
      <translation>utf8.* funkcje Lua nie będą dostępne.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14255"/>
      <source>yajl.* Lua functions won&apos;t be available.</source>
      <translation>yajl.* funkcje Lua nie będą dostępne.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14440"/>
      <source>No error message available from Lua.</source>
      <translation>Brak komunikatu o błędzie z Lua.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14442"/>
      <source>Lua error: %1.</source>
      <translation>Błąd Lua: %1.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14444"/>
      <source>[ ERROR ] - Cannot load code formatter, indenting functionality won&apos;t be available.
</source>
      <translation>[ BŁĄD ] - Nie można załadować formatera kodu, funkcja wcięcia nie będzie dostępna.
</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14521"/>
      <source>%1 (doesn&apos;t exist)</source>
      <comment>This file doesn&apos;t exist</comment>
      <translation>%1 (nie istnieje)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14526"/>
      <source>%1 (isn&apos;t a file or symlink to a file)</source>
      <translation>%1 (nie ma pliku lub symlinku do pliku)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14535"/>
      <source>%1 (isn&apos;t a readable file or symlink to a readable file)</source>
      <translation>%1 (nie ma pliku do odczytu lub symlinku do pliku do odczytu)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14553"/>
      <source>%1 (couldn&apos;t read file)</source>
      <comment>This file could not be read for some reason (for example, no permission)</comment>
      <translation>%1 (niemożna odczytać plik)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14559"/>
      <source>[  OK  ]  - Mudlet-lua API &amp; Geyser Layout manager loaded.</source>
      <translation>[  OK  ] - Wczytano Mudlet-lua API i Geyser menedżer układu.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14566"/>
      <source>[ ERROR ] - Couldn&apos;t find, load and successfully run LuaGlobal.lua - your Mudlet is broken!
Tried these locations:
%1</source>
      <translation>[ BŁĄD ] - Nie można znaleźć, pobrać i uruchomić LuaGlobal.Lua - twój Mudlet jest uszkodzony!
próbowałem te miejsca:
%1</translation>
    </message>
  </context>
  <context>
    <name>TMainConsole</name>
    <message>
      <location filename="../src/TMainConsole.cpp" line="166"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).</comment>
      <translation>plik dziennika</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="207"/>
      <source>Logging has started. Log file is %1
</source>
      <translation>Rozpoczęcie rejestrowania. Plik dziennika to %1
</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="217"/>
      <source>Logging has been stopped. Log file is %1
</source>
      <translation>Przerwanie rejestrowania. Plik dziennika to %1
</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="247"/>
      <source>Mudlet MUD Client version: %1%2</source>
      <translation>Wersja klienta MUD Mudlet: %1%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="249"/>
      <source>Mudlet, log from %1 profile</source>
      <translation>Mudlet, zapis z profilu %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="285"/>
      <location filename="../src/TMainConsole.cpp" line="307"/>
      <source>&apos;Log session starting at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation>&apos;Loguj sesję zaczynającą się o &apos;hh:mm:ss&apos; na &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="312"/>
      <source>&lt;p&gt;Stop logging game output to log file.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zatrzymaj logowanie wyjścia gry do pliku logu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="316"/>
      <source>&apos;Log session ending at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation>&apos;Sesja dziennika kończy się o &apos;Ss&apos; na &apos;Dddd&apos;, &apos;D&apos; &apos;Mmmm&apos; &apos;Rrrr&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="329"/>
      <source>&lt;p&gt;Start logging game output to log file.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Rozpocznij logowanie wyjścia gry do pliku logu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="643"/>
      <source>Pre-Map loading(2) report</source>
      <translation>Wstępne ładowanie mapy(2) raport</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="654"/>
      <source>Loading map(2) at %1 report</source>
      <translation>Wczytywanie mapy(2) w raporcie %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1054"/>
      <source>User window - %1 - %2</source>
      <translation>Okno użytkownika - %1 - %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1102"/>
      <source>N:%1 S:%2</source>
      <comment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</comment>
      <translation>N:%1 S:%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1109"/>
      <source>&lt;no GA&gt; S:%1</source>
      <comment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</comment>
      <translation>&lt;no GA&gt; S:%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1206"/>
      <source>Pre-Map loading(1) report</source>
      <translation>Wstępne ładowanie mapy(1) raport</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1224"/>
      <source>Loading map(1) at %1 report</source>
      <translation>Ładowanie mapy (1) w raporcie %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1226"/>
      <source>Loading map(1) &quot;%1&quot; at %2 report</source>
      <translation>Ładowanie mapy (1) &quot;%1&quot; w raporcie %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1270"/>
      <source>Pre-Map importing(1) report</source>
      <translation>Raport importu mapy przed odwzorowaniem (1)</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1293"/>
      <source>[ ERROR ]  - Map file not found, path and name used was:
%1.</source>
      <translation>[ BŁĄD ] - Plik mapy nie odnaleziony, ścieżka i nazwa pliku: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1299"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; was not found).</source>
      <translation>loadMap: niepoprawna wartość argumentu #1 (plik o nazwie &quot;%1&quot; nie został znaleziony).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1308"/>
      <source>[ INFO ]  - Map file located and opened, now parsing it...</source>
      <translation>[ INFO ]  - Plik mapy odszukany i otwarty, przetwarzanie...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1315"/>
      <source>Importing map(1) &quot;%1&quot; at %2 report</source>
      <translation>Importowanie mapy(1) &quot;%1&quot; w raporcie %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1318"/>
      <source>[ INFO ]  - Map file located but it could not opened, please check permissions on:&quot;%1&quot;.</source>
      <translation>[ INFO ] - Plik mapy zlokalizowany, ale nie można go otworzyć, proszę sprawdzić uprawnienia dostępu do pliku:&quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1321"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; could not be opened for reading).</source>
      <translation>loadMap: niepoprawna wartość argumentu #1 (plik o nazwie &quot;%1&quot; nie mógł być otwarty do odczytu).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1345"/>
      <source>[ INFO ]  - Map reload request received from system...</source>
      <translation>[ INFO ]  - Żądanie przeładowania mapy otrzymane z systemu...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1350"/>
      <source>[  OK  ]  - ... System Map reload request completed.</source>
      <translation>[  OK  ]  - ... żądanie ponownego wczytania mapy zakończone.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1352"/>
      <source>[ WARN ]  - ... System Map reload request failed.</source>
      <translation>[ OSTRZEŻENIE ] - ... żądanie ponownego wczytania mapy zakończone niepowodzeniem.</translation>
    </message>
  </context>
  <context>
    <name>TMap</name>
    <message>
      <location filename="../src/TMap.cpp" line="210"/>
      <source>RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!</source>
      <translation>RoomID=%1 nie istnieje, nie można ustawić obszaru AreaID=%2 dla nieistniejącej lokacji!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="221"/>
      <source>AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!</source>
      <translation>AreaID=%2 nie istnieje, nie można przypisać lokacji RoomID=%1 do nieistniejącego obszaru!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="628"/>
      <source>[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.</source>
      <translation>[ INFO ] - KONWERSJA: etykieta starego typu, areaID:%1 labelID:%2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="631"/>
      <source>[ INFO ] - Converting old style label id: %1.</source>
      <translation>[ INFO ] - Konwersja etykiety starego typu, id: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="636"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.</source>
      <translation>[ OSTRZEŻENIE ] - KONWERSJA: Nie jest możliwe skonwertowanie etykiety starego typu w obszarze o id: %1, id etykiety: %2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="639"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label with id: %1.</source>
      <translation>[ OSTRZEŻENIE ] - KONWERSJA: Nie jest możliwe skonwertowanie etykiety starego typu, id etykiety: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="668"/>
      <source>[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...</source>
      <translation>[  OK  ]  - Audyt mapy zakończony (%1s). Miłej gry...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="675"/>
      <source>[  OK  ]  - Map loaded successfully (%1s).</source>
      <translation>[  OK  ]  - Mapa wczytana pomyślnie (%1s).</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1146"/>
      <source>n</source>
      <comment>This translation converts the direction that DIR_NORTH codes for to a direction string that the game server will accept!</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1147"/>
      <source>ne</source>
      <comment>This translation converts the direction that DIR_NORTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation>ne</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1148"/>
      <source>e</source>
      <comment>This translation converts the direction that DIR_EAST codes for to a direction string that the game server will accept!</comment>
      <translation>e</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1149"/>
      <source>se</source>
      <comment>This translation converts the direction that DIR_SOUTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation>se</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1150"/>
      <source>s</source>
      <comment>This translation converts the direction that DIR_SOUTH codes for to a direction string that the game server will accept!</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1151"/>
      <source>sw</source>
      <comment>This translation converts the direction that DIR_SOUTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation>sw</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1152"/>
      <source>w</source>
      <comment>This translation converts the direction that DIR_WEST codes for to a direction string that the game server will accept!</comment>
      <translation>w</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1153"/>
      <source>nw</source>
      <comment>This translation converts the direction that DIR_NORTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation>nw</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1154"/>
      <source>up</source>
      <comment>This translation converts the direction that DIR_UP codes for to a direction string that the game server will accept!</comment>
      <translation>u</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1155"/>
      <source>down</source>
      <comment>This translation converts the direction that DIR_DOWN codes for to a direction string that the game server will accept!</comment>
      <translation>d</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1156"/>
      <source>in</source>
      <comment>This translation converts the direction that DIR_IN codes for to a direction string that the game server will accept!</comment>
      <translation>do środka</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1157"/>
      <source>out</source>
      <comment>This translation converts the direction that DIR_OUT codes for to a direction string that the game server will accept!</comment>
      <translation>na zewnątrz</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="187"/>
      <source>[MAP ERROR:]%1
</source>
      <translation>[BŁĄD MAPY:]%1
</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="48"/>
      <source>Default Area</source>
      <translation>Domyślny obszar</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="49"/>
      <source>Unnamed Area</source>
      <translation>Nienazwany obszar</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="598"/>
      <source>[ INFO ]  - Map audit starting...</source>
      <translation>[ INFO ] - Rozpoczęcie audytu mapy...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1604"/>
      <source>[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!
There is so much data that it DOES NOT have that you could be
better off starting again...</source>
      <translation>[ INFO]-Może chcesz przekazać ten plik mapy do Mudlet Museum!
Jest tyle danych, ale lepiej zacząć od nowa ...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1667"/>
      <source>[ ALERT ] - Failed to load a Mudlet JSON Map file, reason:
%1; the file is:
&quot;%2&quot;.</source>
      <translation>[ALERT ] - Nie udało się załadować pliku mapy w formacie JSON, powód:
%1; plik to:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1672"/>
      <source>[ INFO ]  - Ignoring this map file.</source>
      <translation>[ INFO ] - Ignorowanie tego pliku mapy.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1842"/>
      <source>[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an
area) not found, adding reserved -1 id.</source>
      <translation>[INFO] - Domyślny (reset) obszar (dla pokoi, które nie zostały przypisane do żadnego obszaru) nie został znaleziony, dodanie zarezerwowanego identyfikatora -1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1936"/>
      <source>[ INFO ]  - Successfully read the map file (%1s), checking some
consistency details...</source>
      <translation>[ INFO] - Pomyślnie odczytany plik mapy (%1s), sprawdzanie niektórych szczegółów spójności...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1951"/>
      <source>No map found. Would you like to download the map or start your own?</source>
      <translation>Nie znaleziono mapy. Czy chcesz pobrać mapę czy zacząć własną?</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1952"/>
      <source>Download the map</source>
      <translation>Pobierz mapę</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1953"/>
      <source>Start my own</source>
      <translation>Zacznij własną</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2398"/>
      <source>Map issues</source>
      <translation>Problemy z mapą</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2405"/>
      <source>Area issues</source>
      <translation>Problemy z obszarem</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2411"/>
      <source>Area id: %1 &quot;%2&quot;</source>
      <translation>Identyfikator obszaru: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2413"/>
      <source>Area id: %1</source>
      <translation>Identyfikator obszaru: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2422"/>
      <source>Room issues</source>
      <translation>Problemy z pokojem</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2429"/>
      <source>Room id: %1 &quot;%2&quot;</source>
      <translation>ID pokoju: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2431"/>
      <source>Room id: %1</source>
      <translation>Identyfikator lokacji: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2441"/>
      <source>End of report</source>
      <translation>Koniec raportu</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2447"/>
      <source>[ ALERT ] - At least one thing was detected during that last map operation
that it is recommended that you review the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[ ALERT ] - Przynajmniej jedna rzecz została wykryta podczas ostatniej operacji na mapie, zaleca się przejrzenie ostatniego raportu w pliku:
&quot;%1&quot;
- poszukaj (ostatniego) raportu z tytułem:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2455"/>
      <source>[ INFO ]  - The equivalent to the above information about that last map
operation has been saved for review as the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[ INFO ] - Równowartość powyższej informacji o ostatniej operacji na mapie,
została zapisana do wiadomości jako ostatni raport w pliku:
&quot;%1&quot;
- poszukaj (ostatniego) raportu z tytułem:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2511"/>
      <source>[ ERROR ] - Unable to use or create directory to store map.
Please check that you have permissions/access to:
&quot;%1&quot;
and there is enough space. The download operation has failed.</source>
      <translation>[ BŁĄD ] - Nie można używać lub tworzyć katalogu do przechowywania mapy.
Proszę sprawdzić, czy masz uprawnienia/dostęp do:
&quot;%1&quot;
i jest wystarczająco dużo miejsca na dysku. Operacja pobierania nie powiodła się.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2537"/>
      <source>[ INFO ]  - Map download initiated, please wait...</source>
      <translation>[ INFO ] - Pobieranie mapy rozpoczęte, proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2548"/>
      <source>Map download</source>
      <comment>This is a title of a progress window.</comment>
      <translation>Pobieranie mapy</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2611"/>
      <source>Map import</source>
      <comment>This is a title of a progress dialog.</comment>
      <translation>Importowanie mapy</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2916"/>
      <location filename="../src/TMap.cpp" line="3407"/>
      <source>Exporting JSON map data from %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>Eksport mapy w formacie JSON z %1
Obszary: %2 z: %3   Lokacje: %4 z: %5   Etykiety: %6 z: %7...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2931"/>
      <source>Map JSON export</source>
      <comment>This is a title of a progress window.</comment>
      <translation>Eksport mapy do JSON</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3066"/>
      <source>Exporting JSON map file from %1 - writing data to file:
%2 ...</source>
      <translation>Eksportowanie mapy w formacie JSON z %1 - zapisywanie danych do pliku:
%2 ...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3092"/>
      <source>import or export already in progress</source>
      <translation>import lub eksport jest już w toku</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3100"/>
      <source>could not open file</source>
      <translation>nie można otworzyć pliku</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3110"/>
      <source>could not parse file, reason: &quot;%1&quot; at offset %2</source>
      <translation>nie można przetworzyć pliku, powód: &quot;%1&quot; na pozycji %2</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3119"/>
      <source>empty Json file, no map data detected</source>
      <translation>pusty plik JSON, nie wykryto danych mapy</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3134"/>
      <source>invalid format version &quot;%1&quot; detected</source>
      <translation>wykryto niepoprawną wersję &quot;%1&quot; formatu mapy</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3140"/>
      <source>no format version detected</source>
      <translation>nie wykryto wersji formatu mapy</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3146"/>
      <source>no areas detected</source>
      <translation>nie wykryto żadnych obszarów</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3171"/>
      <source>Map JSON import</source>
      <comment>This is a title of a progress window.</comment>
      <translation>Import mapy JSON</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3267"/>
      <source>aborted by user</source>
      <translation>przerwane przez użytkownika</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3156"/>
      <location filename="../src/TMap.cpp" line="3417"/>
      <source>Importing JSON map data to %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>Importowanie danych mapy JSON do %1
Obszary: %2 z: %3   Lokacji: %4 z: %5   Etykiety: %6 z: %7...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1180"/>
      <source>[ ERROR ] - The format version &quot;%1&quot; you are trying to save the map with is too new
for this version of Mudlet. Supported are only formats up to version %2.</source>
      <translation>[ ERROR ] - Wersja formatu mapy &quot;%1&quot; w którym próbujesz zapisać mapę jest zbyt nowa dla tej wersji Mudleta. Obsługiwane są tylko formaty do wersji %2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1196"/>
      <source>[ ALERT ] - Saving map in format version &quot;%1&quot; that is different than &quot;%2&quot; which
it was loaded as. This may be an issue if you want to share the resulting
map with others relying on the original format.</source>
      <translation>[ ALERT ] - Zapisujesz mapę w wersji formatu &quot;%1&quot;, która jest inna niż wersja formatu &quot;%2&quot; w której została załadowana mapa. To może być problemem jeśli chcesz udostępnić zapisany plik osobom które używają wersji formatu mapy w której została ona oryginalnie zapisana.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1206"/>
      <source>[ WARN ]  - Saving map in format version &quot;%1&quot; different from the
recommended map version %2 for this version of Mudlet.</source>
      <translation>[ UWAGA ] - Zapisywanie mapy w formacie &quot;%1&quot;, innym niż
zalecany format &quot;%2&quot; dla tej wersji Mudleta.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1544"/>
      <location filename="../src/TMap.cpp" line="1995"/>
      <source>[ ERROR ] - Unable to open map file for reading: &quot;%1&quot;!</source>
      <translation>[ ERROR ] - Nie można otworzyć pliku mapy do odczytu: &quot;%1&quot;!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1566"/>
      <source>[ ALERT ] - File does not seem to be a Mudlet Map file. The part that indicates
its format version seems to be &quot;%1&quot; and that doesn&apos;t make sense. The file is:
&quot;%2&quot;.</source>
      <translation>[ALERT ] - Plik nie wydaje się być plikiem mapy Mudleta. Odczytana z pliku wersja formatu to &quot;%1&quot; i to nie ma sensu. Plik to: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1581"/>
      <source>[ ALERT ] - Map file is too new. Its format version &quot;%1&quot; is higher than this version of
Mudlet can handle (%2)! The file is:
&quot;%3&quot;.</source>
      <translation>[ BŁĄD ] - Plik mapy jest zbyt nowy, jego wersja formatu to &quot;%1&quot; i jest wyższa niż ta wersja Mudleta może obsłużyć (%2)! Plik to:
&quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1588"/>
      <source>[ INFO ]  - You will need to update your Mudlet to read the map file.</source>
      <translation>[ INFO ] - Musisz zaktualizować Mudlet, aby odczytać ten plik mapy.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1597"/>
      <source>[ ALERT ] - Map file is really old. Its format version &quot;%1&quot; is so ancient that
this version of Mudlet may not gain enough information from
it but it will try! The file is: &quot;%2&quot;.</source>
      <translation>[ ALERT ] - Plik mapy jest naprawdę stary. Jego format w wersji &quot;%1&quot; jest tak stary, że
ta wersja Mudleta może nie uzyskać wystarczającej ilości informacji z
niego, ale będzie próbować! Plik to: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1611"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File:
&quot;%2&quot;,
please wait...</source>
      <translation>[ INFO ] - Wczytywanie mapy. Wersja formatu: %1. Plik:
&quot;%2&quot;,
proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1614"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File: &quot;%2&quot;.</source>
      <translation>[ INFO ] - Wczytywanie mapy. Wersja formatu: %1. Plik: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2011"/>
      <source>[ INFO ]  - Checking map file &quot;%1&quot;, format version &quot;%2&quot;.</source>
      <translation>[ INFO ] - Sprawdzanie pliku mapy &quot;%1&quot;, wersja formatu: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2547"/>
      <location filename="../src/TMap.cpp" line="2925"/>
      <location filename="../src/TMap.cpp" line="3165"/>
      <source>Abort</source>
      <translation>Przerwij</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2545"/>
      <source>Downloading map file for use in %1...</source>
      <comment>%1 is the name of the current Mudlet profile</comment>
      <translation>Pobieranie pliku mapy do użycia w %1...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1573"/>
      <source>[ INFO ]  - Ignoring this unlikely map file.</source>
      <translation>[ INFO ] - Ignorowanie tego pliku, nie przypomina on pliku mapy.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2579"/>
      <source>loadMap: unable to perform request, a map is already being downloaded or
imported at user request.</source>
      <translation>loadMap: nie można wykonać żądania, mapa jest już pobierana lub
importowana na żądanie użytkownika.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2610"/>
      <source>Importing XML map file for use in %1...</source>
      <translation>Importowanie pliku XML z mapą %1...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2638"/>
      <source>loadMap: failure to import XML map file, further information may be available
in main console!</source>
      <translation>loadMap: niepowodzenie importowania pliku mapy XML, dalsze informacje mogą być dostępne w głównej konsoli!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2673"/>
      <source>[ ALERT ] - Map download was canceled, on user&apos;s request.</source>
      <translation>[ ALERT]-Pobieranie mapy zostało anulowane, na żądanie użytkownika.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2692"/>
      <source>[ ERROR ] - Map download encountered an error:
%1.</source>
      <translation>[ ERROR]-Pobieranie mapy napotkało błąd:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2723"/>
      <source>[ ALERT ] - Map download failed, error reported was:
%1.</source>
      <translation>[ ALERT]-Pobieranie mapy nie powiodło się, zgłoszony błąd:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2731"/>
      <source>[ ALERT ] - Map download failed, unable to open destination file:
%1.</source>
      <translation>[ ALERT]-Pobieranie mapy nie powiodło się, nie można otworzyć pliku docelowego:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2736"/>
      <source>[ ALERT ] - Map download failed, unable to write destination file:
%1.</source>
      <translation>[ ALERT ] - Pobieranie mapy nie powiodło się, nie można zapisać pliku docelowego:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2749"/>
      <location filename="../src/TMap.cpp" line="2766"/>
      <source>[ INFO ]  - ... map downloaded and stored, now parsing it...</source>
      <translation>[ INFO] - ... mapa pobrana i zapisana, teraz analizuje ja ...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2757"/>
      <location filename="../src/TMap.cpp" line="2792"/>
      <source>[ ERROR ] - Map download problem, failure in parsing destination file:
%1.</source>
      <translation>[ BŁĄD ] - Problem z pobieraniem mapy, niepowodzenie w analizowaniu pliku docelowego: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2797"/>
      <source>[ ERROR ] - Map download problem, unable to read destination file:
%1.</source>
      <translation>[ BŁĄD ] - Problem z pobieraniem mapy, brak możliwości odczytania pliku docelowego: %1.</translation>
    </message>
  </context>
  <context>
    <name>TRoom</name>
    <message>
      <location filename="../src/TRoom.cpp" line="86"/>
      <location filename="../src/TRoom.cpp" line="971"/>
      <source>North</source>
      <translation>Północ</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="87"/>
      <source>North-east</source>
      <translation>Północny wschód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="88"/>
      <source>North-west</source>
      <translation>Północny zachód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="89"/>
      <location filename="../src/TRoom.cpp" line="1013"/>
      <source>South</source>
      <translation>Południe</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="90"/>
      <source>South-east</source>
      <translation>Południowy wschód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="91"/>
      <source>South-west</source>
      <translation>Południowy zachód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="92"/>
      <location filename="../src/TRoom.cpp" line="1055"/>
      <source>East</source>
      <translation>Wschód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="93"/>
      <location filename="../src/TRoom.cpp" line="1069"/>
      <source>West</source>
      <translation>Zachód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="94"/>
      <location filename="../src/TRoom.cpp" line="1083"/>
      <source>Up</source>
      <translation>W górę</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="95"/>
      <location filename="../src/TRoom.cpp" line="1097"/>
      <source>Down</source>
      <translation>W dół</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="96"/>
      <location filename="../src/TRoom.cpp" line="1111"/>
      <source>In</source>
      <translation>Do środka</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="97"/>
      <location filename="../src/TRoom.cpp" line="1125"/>
      <source>Out</source>
      <translation>Na zewnątrz</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="98"/>
      <source>Other</source>
      <translation>Inne</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="100"/>
      <source>Unknown</source>
      <translation>Nieznane</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="311"/>
      <source>No area created!  Requested area ID=%1. Note: Area IDs must be &gt; 0</source>
      <translation>Nie utworzono żadnego obszaru!  Żądany obszar ID=%1. Uwaga: Identyfikatory obszaru muszą być &gt; 0</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="333"/>
      <source>Warning: When setting the Area for Room (Id: %1) it did not have a current area!</source>
      <translation>Ostrzeżenie: Podczas ustawiania obszaru dla pokoju (Id: %1) nie ma bieżącego obszaru!</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="985"/>
      <source>Northeast</source>
      <translation>Północny wschód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="999"/>
      <source>Northwest</source>
      <translation>Północny zachód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1027"/>
      <source>Southeast</source>
      <translation>Południowy wschód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1041"/>
      <source>Southwest</source>
      <translation>Południowy zachód</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1146"/>
      <source>[ WARN ]  - In room id:%1 removing invalid (special) exit to %2 {with no name!}</source>
      <translation>[ OSTRZEŻENIE ] - W pokoju id:%1 usunięcie nieprawidłowego (specjalnego) wyjścia do %2 {bez nazwy!}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1150"/>
      <source>[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.</source>
      <translation>[ OSTRZEŻENIE ] - Pokój miał nieprawidłowe (specjalne) wyjście do %1 {bez nazwy!} został usunięty.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1159"/>
      <source>[ INFO ]  - In room with id: %1 correcting special exit &quot;%2&quot; that
was to room with an exit to invalid room: %3 to now go
to: %4.</source>
      <translation>[ INFO ] - W pokoju z id: %1 korygując wyjście specjalne &quot;%2&quot;
było w pokoju z wyjściem do nieprawidłowego pokoju: %3 teraz idź
do: %4.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1169"/>
      <source>[ INFO ]  - Room needed correcting of special exit &quot;%1&quot; that was to room with an exit to invalid room: %2 to now go to: %3.</source>
      <translation>[ INFO ] - Pokój wymaga korekty specjalnego wyjścia &quot;%1&quot; , który był w pokoju z wyjściem do nieprawidłowego pokoju: %2 , aby teraz przejść do: %3.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1194"/>
      <source>[ WARN ]  - Room with id: %1 has a special exit &quot;%2&quot; with an
exit to: %3 but that room does not exist.  The exit will
be removed (but the destination room id will be stored in
the room user data under a key:
&quot;%4&quot;).</source>
      <translation>[ OSTRZEŻENIE] - Pokój o identyfikatorze: %1 ma specjalne wyjście &quot;%2&quot; z
wyjściem do: %3 , ale ten pokój nie istnieje. Wyjście zostanie usunięte
(ale ID pokoju docelowego będzie przechowywane w
dane użytkownika pokoju pod kluczem:
&quot;%4&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1207"/>
      <source>[ WARN ]  - Room has a special exit &quot;%1&quot; with an exit to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation>[ OSTRZEŻENIE] - Pokój ma specjalne wyjście &quot;%1&quot; z wyjściem do: %2 , ale ten pokój nie istnieje. Wyjście zostanie usunięte (ale ID pokoju docelowego będzie przechowywane w danych użytkownika pokoju pod kluczem:&quot;%3&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1246"/>
      <source>[ INFO ]  - In room with id: %1 special exit &quot;%2&quot;
that was to room with an invalid room: %3 that does not exist.
The exit will be removed (the bad destination room id will be stored in the
room user data under a key:
&quot;%4&quot;).</source>
      <translation>[ INFO ] - W pokoju o ID: %1 specjalne wyjście &quot;%2&quot;
które było na pokój z nieprawidłowym pomieszczeniem: %3 który nie istnieje.
Wyjście zostanie usunięte (złe ID pokoju docelowego zostanie zapisane w danych użytkownika
pod kluczem:
&quot;%4&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1259"/>
      <source>[ INFO ]  - Room had special exit &quot;%1&quot; that was to room with an invalid room: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation>[ INFO]-Pokój miał specjalne wyjście &quot;%1&quot; , które miało miejsce w pokoju z niepoprawnym pokojem: %2 , który nie istnieje.  Wyjście zostanie usunięte (zły identyfikator pokoju docelowego będzie zapisany w danych użytkownika pokoju pod kluczem:&quot;%3&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1292"/>
      <source>%1 {none}</source>
      <translation>%1 {none}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1295"/>
      <source>%1 (open)</source>
      <translation>%1 (otwarte)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1298"/>
      <source>%1 (closed)</source>
      <translation>%1 (zamknięte)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1301"/>
      <source>%1 (locked)</source>
      <translation>%1 (zamknięte na klucz)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1304"/>
      <source>%1 {invalid}</source>
      <translation>%1 {nieważne}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1308"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:
%2.</source>
      <translation>[INFO] - W pokoju z id: %1 znaleziono jeden lub więcej nadwyżek drzwi, które zostały usunięte:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1315"/>
      <source>[ INFO ]  - Room had one or more surplus door items that were removed:%1.</source>
      <translation>[INFO] - Pokój miał jeden lub więcej nadwyżek elementów drzwi, które zostały usunięte:%1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1331"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:
%2.</source>
      <translation>[ INFO ] - W pokoju z id: %1 znaleziono jeden lub więcej elementów wagi nadmiarowo, które zostały usunięte:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1338"/>
      <source>[ INFO ]  - Room had one or more surplus weight items that were removed: %1.</source>
      <translation>[INFO] - Pokój miał jeden lub więcej nadwyżek wagi, które zostały usunięte: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1354"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus exit lock items that were removed:
%2.</source>
      <translation>[ INFO ] - W pokoju z id: %1 znaleziono jeden lub więcej nadwyżek elementów blokady wyjścia, które zostały usunięte:
%2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1361"/>
      <source>[ INFO ]  - Room had one or more surplus exit lock items that were removed: %1.</source>
      <translation>[ INFO ] - Pokój miał jeden lub więcej nadwyżek elementów blokady wyjścia, które zostały usunięte: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1440"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that
were removed: %2.</source>
      <translation>[ INFO ] - W pokoju z id: %1 znaleźć jeden lub więcej nadwyżek elementów linii niestandardowych, które usunięto: %2.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1446"/>
      <source>[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.</source>
      <translation>[ INFO ] - Pokój miał jeden lub więcej nadwyżek elementów linii niestandardowych, które zostały usunięte: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1469"/>
      <source>[ INFO ]  - In room with id: %1 correcting exit &quot;%2&quot; that was to room with
an exit to invalid room: %3 to now go to: %4.</source>
      <translation>[ INFO ] - W pokoju z id: %1 korygowanie wyjścia &quot;%2&quot; to było do pokoju z wyjściem do nieprawidłowego pokoju: %3 , aby przejść do: %4.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1478"/>
      <source>[ INFO ]  - Correcting exit &quot;%1&quot; that was to invalid room id: %2 to now go to: %3.</source>
      <translation>[ INFO ] - Korekta wyjścia &quot;%1&quot; , która była nieprawidłowym id pokoju: %2 teraz prowadzi do: %3.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1489"/>
      <source>[ WARN ]  - Room with id: %1 has an exit &quot;%2&quot; to: %3 but that room
does not exist.  The exit will be removed (but the destination room
Id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation>[ WARN ] - Pokój z identyfikatorem: %1 ma wyjście &quot;%2&quot; do: %3 ale ten pokój nie istnieje.  Wyjście zostanie usunięte (ale pomieszczenie docelowe
Identyfikator będzie przechowywany w danych użytkownika pokoju pod kluczem:
&quot;%4&quot;) a wyjście zostanie przekształcone w zalążek.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1502"/>
      <source>[ WARN ]  - Room has an exit &quot;%1&quot; to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation>[ WARN ] - Pokój ma wyjście &quot;%1&quot; do: %2 ale ten pokój nie istnieje.  Wyjście zostanie usunięte (ale identyfikator pokoju docelowego będzie przechowywany w danych użytkownika pokoju pod kluczem: &quot;%4&quot;), a wyjście zostanie przekształcone w zalążek.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1546"/>
      <source>[ ALERT ] - Room with id: %1 has an exit &quot;%2&quot; to: %3 but also
has a stub exit!  As a real exit precludes a stub, the latter will
be removed.</source>
      <translation>[ ALERT ] - Pokój z identyfikatorem: %1 ma wyjście &quot;%2&quot; do: %3 ale także ma zalażek wyjścia  (stub)!  Ponieważ prawdziwe wyjście wyklucza zalążek, ten ostatni zostanie usunięty.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1556"/>
      <source>[ ALERT ] - Room has an exit &quot;%1&quot; to: %2 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.</source>
      <translation>[ ALERT ] - Pokój ma wyjście &quot;%1&quot; do: %2 ale ma również zalążek wyjścia (stub) w tym samym kierunku!  Ponieważ prawdziwe wyjście wyklucza zalążek, ten ostatni zostanie usunięty.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1612"/>
      <source>[ INFO ]  - In room with id: %1 exit &quot;%2&quot; that was to room with an invalid
room: %3 that does not exist.  The exit will be removed (the bad destination
room id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation>[ INFO ] - W pokoju z id: %1 Wyjścia &quot;%2&quot; który był do pokoju z nieprawidłowym
pokojem: %3 który nie istnieje.  Wyjście zostanie usunięte (zły cel
id pokoju będą przechowywane w danych użytkownika pokoju pod kluczem:
&quot;%4&quot;)
a wyjście zostanie przekształcone w zalążek.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1623"/>
      <source>[ INFO ]  - Room exit &quot;%1&quot; that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation>[ INFO ] - Wyjście z pokoju &quot;%1&quot; który był do pokoju z nieprawidłowym id: %2 który nie istnieje.  Wyjście zostanie usunięte (zły identyfikator pokoju docelowego będzie przechowywany w danych użytkownika pokoju pod kluczem:&quot;%4&quot;), a wyjście zostanie przekształcone w zalążek.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1639"/>
      <source>
It was locked, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation>
Został zablokowany, jest to rejestrowane jako dane użytkownika z kluczem:
&quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1643"/>
      <source>  It was locked, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation>  Został zablokowany, jest to rejestrowane jako dane użytkownika z kluczem: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1651"/>
      <source>
It had a weight, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation>
Miał wagę, jest to rejestrowane jako dane użytkownika z kluczem:
&quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1655"/>
      <source>  It had a weight, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation>  Miał wagę, jest to rejestrowane jako dane użytkownika z kluczem: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1666"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but
it has not been possible to salvage this, it has been lost!</source>
      <translation>[ OSTRZEŻENIE ] - Istniała niestandardowa linia wyjścia powiązana z nieprawidłowym zakończeniem, ale
nie było możliwe jej odzyskanie, stracona!</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1671"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but it has not been possible to salvage this, it has been lost!</source>
      <translation>[ OSTRZEŻENIE ] - Istniała niestandardowa linia wyjścia powiązana z nieprawidłowym zakończeniem, ale nie było możliwe jej odzyskanie, stracona!</translation>
    </message>
  </context>
  <context>
    <name>TRoomDB</name>
    <message>
      <location filename="../src/TRoomDB.cpp" line="504"/>
      <source>Area with ID %1 already exists!</source>
      <translation>Obszar o identyfikatorze %1 już istnieje!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="523"/>
      <source>An Unnamed Area is (no longer) permitted!</source>
      <translation>Obszar bez nazwy jest nie jest już dozwolony!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="527"/>
      <source>An area called %1 already exists!</source>
      <translation>Obszar o nazwie %1 już istnieje!</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="626"/>
      <source>[ WARN ]  - Problem with data structure associated with room id: %1 - that
room&apos;s data has been lost so the id is now being deleted.  This
suggests serious problems with the currently running version of
Mudlet - is your system running out of memory?</source>
      <translation>[ OSTRZEŻENIE ] - Problem z danymi powiązanymi z lokacją o ID %1 - dane te zostały utracone więc ID lokacji zostanie skasowane. To sugeruje poważne problemy z aktualnie uruchomioną wersją Mudleta - czy zaczyna brakować pamięci w systemie?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="634"/>
      <source>[ WARN ]  - Problem with data structure associated with this room.  The room&apos;s data has been lost so the id is now being deleted.  This suggests serious problems with the currently running version of Mudlet - is your system running out of memory?</source>
      <translation>[ OSTRZEŻENIE ] - Problem ze strukturą danych związaną z tym pomieszczeniem. Dane zostały utracone, więc identyfikator jest teraz usuwany. Sugeruje to poważne problemy z obecnie uruchomioną wersją Mudlet - czy system nie ma pamięci?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="692"/>
      <source>[ ALERT ] - Area with id: %1 expected but not found, will be created.</source>
      <translation>[ UWAGA ] - Obszar z id: %1 oczekiwany, ale nie znaleziony, zostanie utworzony.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="695"/>
      <source>[ ALERT ] - Area with this id expected but not found, will be created.</source>
      <translation>[ UWAGA ] - Obszar o tym id oczekiwany, ale nie znaleziony, zostanie utworzony.</translation>
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
      <translation>[ UWAGA ] - Znaleziono błędne (mniej niż +1 i nie zastrzeżone -1) identyfikatory pokoju (liczba wystąpień: %1) na mapie, teraz ustalam, jakich nowych numerów identyfikatorów użyć...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="787"/>
      <source>[ INFO ]  - The renumbered area ids will be:
Old ==&gt; New</source>
      <translation>[ INFO ] - Zmienione numery ID obszaru będą następujące:
Stary ==&gt; Nowe</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="805"/>
      <source>[ INFO ]  - The area with this bad id was renumbered to: %1.</source>
      <translation>[ INFO ] - Obszar o takim błędnym ID został zmieniony na %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="806"/>
      <source>[ INFO ]  - This area was renumbered from the bad id: %1.</source>
      <translation>[ INFO ] - Ten obszar został przenumerowany ze złego identyfikatora: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="843"/>
      <location filename="../src/TRoomDB.cpp" line="846"/>
      <source>[ INFO ]  - Area id numbering is satisfactory.</source>
      <translation>[ INFO ] - Numerowanie identyfikatorów obszarów jest zadowalające.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="854"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map, now working
out what new id numbers to use.</source>
      <translation>[ UWAGA ] - Znaleziono złe, (mniej niż +1) identyfikatory pokoju (liczba wystąpień: %1) na mapie, teraz działam
jakich nowych numerów identyfikatorów użyć.</translation>
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
      <translation>[ ALERT ] - Znaleziono niepoprawny (mniejszy niż +1 i nie będący -1) identyfikatory obszaru (liczba wystąpień: %1) na mapie! Oczekuj dalszych wiadomości związanych z tym problem dla poszczególnych obszarów...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="859"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!  Look for further messages related to this for each affected room ...</source>
      <translation>[ ALERT ] - Niepoprawny (mniejszy niż +1) identyfikator lokacji (wystąpienia: %1) na mapie! Oczekuj dalszych komunikatów dla poszczególnych lokacji...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="866"/>
      <source>[ INFO ]  - The renumbered rooms will be:
</source>
      <translation>[ INFO ] - Zmienione numery pokoi będą następujące:
</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="882"/>
      <source>[ INFO ]  - This room with the bad id was renumbered to: %1.</source>
      <translation>[ INFO ] - Ten pokój ze złym identyfikatorem został przenumerowany na: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="883"/>
      <source>[ INFO ]  - This room was renumbered from the bad id: %1.</source>
      <translation>[ INFO ] - Ten pokój został przenumerowany od złego identyfikatora: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="919"/>
      <location filename="../src/TRoomDB.cpp" line="922"/>
      <source>[ INFO ]  - Room id numbering is satisfactory.</source>
      <translation>[ INFO ] - Numerowanie identyfikatorów pomieszczeń jest zadowalające.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="946"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ] - Zduplikowane identyfikatory wyjść znajdują się w pomieszczeniu ID: %1, jest to
anomalia, ale została łatwo oczyszczona.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="951"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ] - Zduplikowane identyfikatory wyjść znalezione w pokoju, jest to anomala, ale została łatwo oczyszczona.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="968"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ] - Zduplikowane identyfikatory blokady wyjścia znalezione w pokoju id: %1, to jest
anomala, ale została łatwo oczyszczona.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="973"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ INFO ] - Duplikowane identyfikatory blokady wyjścia znalezione w pokoju, jest to anomala, ale łatwa do usunięcia.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1060"/>
      <source>[ INFO ]  - This room claims to be in area id: %1, but that did not have a record of it.  The area has been updated to include this room.</source>
      <translation>[ INFO ] - Ten pokój twierdzi, że jest w obszarze id: %1, ale to nie było w rekordzie tego obszaru. Obszar został zaktualizowany, aby włączyć ten pokój.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1066"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 rooms missing from those it
should be recording as possessing, they were:
%3
they have been added.</source>
      <translation>[ INFO ] - W obszarze z id: %1 były %2 pokoje brakuje z tych jakie
powinny być zapisane jako posiadające, były:
%3
zostały one dodane.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1074"/>
      <source>[ INFO ]  - In this area there were %1 rooms missing from those it should be recorded as possessing.  They are: %2.  They have been added.</source>
      <translation>[ INFO ] - W tym obszarze %1 brakuje pomieszczeń z tych, które powinny być rejestrowane jako posiadające.  Są to: %2.  Zostały one dodane.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1099"/>
      <source>[ INFO ]  - This room was claimed by area id: %1, but it does not belong there.  The area has been updated to not include this room.</source>
      <translation>[ INFO ] - Ten pokój został zgłoszony przez ID obszaru: %1, ale nie należy do niego.  Obszar został zaktualizowany, aby nie uwzględnić tego pokoju.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1105"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 extra rooms compared to those it
should be recording as possessing, they were:
%3
they have been removed.</source>
      <translation>[ INFO ] - W obszarze z id: %1 były %2 dodatkowych pokoi w porównaniu z tymi, które
powinny być nagrywane jako posiadające, były:
%3
zostały one usunięte.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1113"/>
      <source>[ INFO ]  - In this area there were %1 extra rooms that it should not be recorded as possessing.  They were: %2.  They have been removed.</source>
      <translation>[ INFO ] - W tym obszarze nie było %1 dodatkowych pomieszczeń, których nie należy rejestrować jako posiadających.  Były: %2.  Zostały one usunięte.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1212"/>
      <source>It has been detected that &quot;_###&quot; form suffixes have already been used, for simplicity in the renaming algorithm these will have been removed and possibly changed as Mudlet sorts this matter out, if a number assigned in this way &lt;b&gt;is&lt;/b&gt; important to you, you can change it back, provided you rename the area that has been allocated the suffix that was wanted first...!&lt;/p&gt;</source>
      <translation>Wykryto, że &quot;_###&quot; przyrostki formularzy zostały już użyte, aby uprościć algorytm zmiany nazwy, zostaną one usunięte i ewentualnie zmienione, gdy Mudlet uporządkuje tę sprawę, jeśli liczba przypisana w ten sposób &lt;b&gt;jest dla Ciebie&lt;/b&gt; ważna, możesz ją zmienić z powrotem pod warunkiem zmiany nazwy obszaru, który został przydzielony w pierwszej kolejności, która była pożądana. .)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1216"/>
      <source>[  OK  ]  - The changes made are:
(ID) &quot;old name&quot; ==&gt; &quot;new name&quot;
</source>
      <translation>[ OK ] - Wprowadzone zmiany są następujące:
(ID) &quot;stara nazwa&quot; ==&gt; &quot;nowa nazwa&quot;
</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1223"/>
      <source>&lt;nothing&gt;</source>
      <translation>&lt;nic&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1226"/>
      <source>[ INFO ]  - Area name changed to prevent duplicates or unnamed ones; old name: &quot;%1&quot;, new name: &quot;%2&quot;.</source>
      <translation>[ INFO ] - Zmieniono nazwę obszaru, aby zapobiec duplikatom lub nienazwanym; stara nazwa: &quot;%1&quot;, nowa nazwa: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1235"/>
      <source>[ ALERT ] - Empty and duplicate area names detected in Map file!</source>
      <translation>[ UWAGA ] - W pliku mapy wykryto puste i zduplikowane nazwy obszarów!</translation>
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
      <translation>[ INFORMACJA ] - Ze względu na pewne sytuacje, które nie były sprawdzane w przeszłości, Mudlet pozwalał na posiadanie na mapie więcej niż jednego obszaru o tej samej lub bez nazwy.
Sprawiają one, że niektóre rzeczy są mylące, dwuznaczne i teraz są niedozwolone. Aby rozwiązać te przypadki, obszar bez nazwy tutaj (lub utworzony w
w przyszłości) zostanie automatycznie przypisany do nazwy &quot;%1&quot;.
  Duplikowane nazwy obszarów spowodują, że wszystkie oprócz pierwszego napotkanego
uzyskają &quot;_###&quot; przyrostek stylu, gdzie każdy &quot;###&quot; jest rosnącym
numerem; możesz chcieć je zmienić, być może zastępując je
a &quot;(nazwa podobszaru)&quot; , ale to zależy wyłącznie od Ciebie, jak to zrobisz,
inny niż nie będziesz mógł ustawić jednego obszaru&apos;s nazwy na
inny, który istnieje w tym czasie.
  Jeśli było więcej niż jeden obszar bez nazwy, to wszystkie oprócz
najpierw uzyskają w ten sposób przyrostek.
%2</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1252"/>
      <source>[ ALERT ] - Duplicate area names detected in the Map file!</source>
      <translation>[ UWAGA ] - W pliku mapy wykryto zduplikowane nazwy obszarów!</translation>
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
      <translation>[ INFO ] - Ze względu na pewne sytuacje, które nie były sprawdzane w przeszłości, Mudlet pozwalał użytkownikowi na posiadanie więcej niż jednego obszaru o tej samej nazwie.
Sprawiają one, że niektóre rzeczy są mylące i teraz są niedozwolone.
  Duplikowane nazwy obszarów spowodują, że wszystkie z wyjątkiem pierwszego napotkanego
uzyskają &quot;_###&quot; przyrostek stylu, gdzie każdy &quot;###&quot; jest rosnącym
numerem; możesz chcieć je zmienić, być może zastępując je
a &quot;(nazwa podobszaru)&quot; , ale to zależy wyłącznie od Ciebie, jak to zrobisz,
inny wtedy nie będziesz mógł ustawić jednego obszaru&apos;s nazwy na
inny, który istnieje w tym czasie.
  Jeśli było więcej niż jeden obszar bez nazwy, wtedy wszystkie oprócz
najpierw uzyskają przyrostek w ten sposób.
%1)</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1268"/>
      <source>[ ALERT ] - An empty area name was detected in the Map file!</source>
      <translation>[ ALERT ] - W pliku mapy wykryto pustą nazwę obszaru!</translation>
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
      <translation>[ OK ] - Ze względu na pewne sytuacje, które w przeszłości nie były sprawdzane, Mudlet pozwalał na posiadanie na mapie obszaru bez nazwy. Może to sprawić, że niektóre rzeczy będą mylące i jest to teraz niedozwolone.
  Aby rozwiązać ten przypadek, obszar bez nazwy tutaj (lub utworzony
w przyszłości) zostanie automatycznie przypisany do nazwy &quot;%1&quot;.
  Jeśli zdarzy się to więcej niż raz, to powielanie nazw obszarów spowoduje, że
wszystkie z wyjątkiem pierwszego napotkanego obszaru uzyskają &quot;_###&quot; styl
przyrostek, gdzie każdy &quot;###&quot; jest rosnącą liczbą; możesz chcieć
zmienić je, być może dodając bardziej znaczące nazwy obszarów, ale to
całkowicie zależy od Ciebie, co jest używane, inne wtedy nie będziesz mógł
ustawić jednego obszaru nazwę do innego, który istnieje w tym czasie.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1295"/>
      <source>[ INFO ]  - Default (reset) area name (for rooms that have not been assigned to an
area) not found, adding &quot;%1&quot; against the reserved -1 id.</source>
      <translation>[ INFO ] - Domyślna (resetowana) nazwa obszaru (dla pokoi, które nie zostały przypisane do
obszaru) nie znaleziono, dodając &quot;%1&quot; w stosunku do zarezerwowanego -1 id.</translation>
    </message>
  </context>
  <context>
    <name>TTextEdit</name>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1298"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1309"/>
      <source>Copy HTML</source>
      <translation>Skopiuj HTML</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1313"/>
      <source>Copy as image</source>
      <translation>Skopiuj jako obraz</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1316"/>
      <source>Select All</source>
      <translation>Zaznacz wszystko</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1321"/>
      <source>Search on %1</source>
      <translation>Szukaj na %1</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1339"/>
      <source>Analyse characters</source>
      <translation>Analizowanie znaków</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1348"/>
      <source>&lt;p&gt;Hover on this item to display the Unicode codepoints in the selection &lt;i&gt;(only the first line!)&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Przesuń kursor na tę pozycję, aby wyświetlić punkty kodowe Unicode w zaznaczeniu &lt;i&gt;(tylko pierwsza linia!)&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1357"/>
      <source>restore Main menu</source>
      <translation>przywrócić menu główne</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1359"/>
      <source>Use this to restore the Main menu to get access to controls.</source>
      <translation>Użyj tego, aby przywrócić Menu Główne w celu uzyskania dostępu do elementów sterujących.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1361"/>
      <source>restore Main Toolbar</source>
      <translation>przywróć główny pasek narzędzi</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1363"/>
      <source>Use this to restore the Main Toolbar to get access to controls.</source>
      <translation>Użyj tego, aby przywrócić Główny Pasek Narzędzi w celu uzyskania dostępu do kontroli.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1450"/>
      <source>Mudlet, debug console extract</source>
      <translation>Mudlet, wyciąg z konsoli debugowania</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1452"/>
      <source>Mudlet, %1 mini-console extract from %2 profile</source>
      <translation>Mudlet, %1 mini-konsola wyciąg z %2 proﬁlu</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1454"/>
      <source>Mudlet, %1 user window extract from %2 profile</source>
      <translation>Mudlet, %1 wyciąg z okna użytkownika z %2 proﬁlu</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1456"/>
      <source>Mudlet, main console extract from %1 profile</source>
      <translation>Mudlet, wyciąg z konsoli głównej %1 proﬁlu</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1924"/>
      <source>{tab}</source>
      <comment>Unicode U+0009 codepoint.</comment>
      <translation>{tab}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1925"/>
      <source>{line-feed}</source>
      <comment>Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation>{line-feed}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1926"/>
      <source>{carriage-return}</source>
      <comment>Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation>{carriage-return}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1927"/>
      <source>{space}</source>
      <comment>Unicode U+0020 codepoint.</comment>
      <translation>{space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1928"/>
      <source>{non-breaking space}</source>
      <comment>Unicode U+00A0 codepoint.</comment>
      <translation>{non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1929"/>
      <source>{soft hyphen}</source>
      <comment>Unicode U+00AD codepoint.</comment>
      <translation>{soft hyphen}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1930"/>
      <source>{combining grapheme joiner}</source>
      <comment>Unicode U+034F codepoint (badly named apparently - see Wikipedia!)</comment>
      <translation>{combining grapheme joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1931"/>
      <source>{ogham space mark}</source>
      <comment>Unicode U+1680 codepoint.</comment>
      <translation>{ogham space mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1932"/>
      <source>{&apos;n&apos; quad}</source>
      <comment>Unicode U+2000 codepoint.</comment>
      <translation>{&apos;n&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1933"/>
      <source>{&apos;m&apos; quad}</source>
      <comment>Unicode U+2001 codepoint.</comment>
      <translation>{&apos;m&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1934"/>
      <source>{&apos;n&apos; space}</source>
      <comment>Unicode U+2002 codepoint - En (&apos;n&apos;) wide space.</comment>
      <translation>{&apos;n&apos; spacja}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1935"/>
      <source>{&apos;m&apos; space}</source>
      <comment>Unicode U+2003 codepoint - Em (&apos;m&apos;) wide space.</comment>
      <translation>{&apos;m&apos; space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1936"/>
      <source>{3-per-em space}</source>
      <comment>Unicode U+2004 codepoint - three-per-em (&apos;m&apos;) wide (thick) space.</comment>
      <translation>{3-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1937"/>
      <source>{4-per-em space}</source>
      <comment>Unicode U+2005 codepoint - four-per-em (&apos;m&apos;) wide (Middle) space.</comment>
      <translation>{4-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1938"/>
      <source>{6-per-em space}</source>
      <comment>Unicode U+2006 codepoint - six-per-em (&apos;m&apos;) wide (Sometimes the same as a Thin) space.</comment>
      <translation>{6-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1939"/>
      <source>{digit space}</source>
      <comment>Unicode U+2007 codepoint - figure (digit) wide space.</comment>
      <translation>{digit space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1940"/>
      <source>{punctuation wide space}</source>
      <comment>Unicode U+2008 codepoint.</comment>
      <translation>{punctuation wide space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1941"/>
      <source>{5-per-em space}</source>
      <comment>Unicode U+2009 codepoint - five-per-em (&apos;m&apos;) wide space.</comment>
      <translation>{5-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1942"/>
      <source>{hair width space}</source>
      <comment>Unicode U+200A codepoint - thinnest space.</comment>
      <translation>{hair width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1943"/>
      <source>{zero width space}</source>
      <comment>Unicode U+200B codepoint.</comment>
      <translation>{zero width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1944"/>
      <source>{Zero width non-joiner}</source>
      <comment>Unicode U+200C codepoint.</comment>
      <translation>{Zero width non-joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1945"/>
      <source>{zero width joiner}</source>
      <comment>Unicode U+200D codepoint.</comment>
      <translation>{zero width joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1946"/>
      <source>{left-to-right mark}</source>
      <comment>Unicode U+200E codepoint.</comment>
      <translation>{left-to-right mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1947"/>
      <source>{right-to-left mark}</source>
      <comment>Unicode U+200F codepoint.</comment>
      <translation>{right-to-left mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1948"/>
      <source>{line separator}</source>
      <comment>Unicode 0x2028 codepoint.</comment>
      <translation>{line separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1949"/>
      <source>{paragraph separator}</source>
      <comment>Unicode U+2029 codepoint.</comment>
      <translation>{paragraph separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1950"/>
      <source>{Left-to-right embedding}</source>
      <comment>Unicode U+202A codepoint.</comment>
      <translation>{Left-to-right embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1951"/>
      <source>{right-to-left embedding}</source>
      <comment>Unicode U+202B codepoint.</comment>
      <translation>{right-to-left embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1952"/>
      <source>{pop directional formatting}</source>
      <comment>Unicode U+202C codepoint - pop (undo last) directional formatting.</comment>
      <translation>{pop directional formatting}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1953"/>
      <source>{Left-to-right override}</source>
      <comment>Unicode U+202D codepoint.</comment>
      <translation>{Left-to-right override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1954"/>
      <source>{right-to-left override}</source>
      <comment>Unicode U+202E codepoint.</comment>
      <translation>{right-to-left override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1955"/>
      <source>{narrow width no-break space}</source>
      <comment>Unicode U+202F codepoint.</comment>
      <translation>{narrow width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1956"/>
      <source>{medium width mathematical space}</source>
      <comment>Unicode U+205F codepoint.</comment>
      <translation>{medium width mathematical space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1957"/>
      <source>{zero width non-breaking space}</source>
      <comment>Unicode U+2060 codepoint.</comment>
      <translation>{zero width non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1958"/>
      <source>{function application}</source>
      <comment>Unicode U+2061 codepoint - function application (whatever that means!)</comment>
      <translation>{function application}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1959"/>
      <source>{invisible times}</source>
      <comment>Unicode U+2062 codepoint.</comment>
      <translation>{invisible times}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1960"/>
      <source>{invisible separator}</source>
      <comment>Unicode U+2063 codepoint - invisible separator or comma.</comment>
      <translation>{niewidoczny separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1961"/>
      <source>{invisible plus}</source>
      <comment>Unicode U+2064 codepoint.</comment>
      <translation>{niewidoczny plus}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1962"/>
      <source>{left-to-right isolate}</source>
      <comment>Unicode U+2066 codepoint.</comment>
      <translation>{lewo-do-prawej izolować}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1963"/>
      <source>{right-to-left isolate}</source>
      <comment>Unicode U+2067 codepoint.</comment>
      <translation>{prawo-do-lewej izolować}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1964"/>
      <source>{first strong isolate}</source>
      <comment>Unicode U+2068 codepoint.</comment>
      <translation>{first strong isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1965"/>
      <source>{pop directional isolate}</source>
      <comment>Unicode U+2069 codepoint - pop (undo last) directional isolate.</comment>
      <translation>{pop directional isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1966"/>
      <source>{inhibit symmetrical swapping}</source>
      <comment>Unicode U+206A codepoint.</comment>
      <translation>{inhibit symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1967"/>
      <source>{activate symmetrical swapping}</source>
      <comment>Unicode U+206B codepoint.</comment>
      <translation>{activate symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1968"/>
      <source>{inhibit arabic form-shaping}</source>
      <comment>Unicode U+206C codepoint.</comment>
      <translation>{inhibit arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1969"/>
      <source>{activate arabic form-shaping}</source>
      <comment>Unicode U+206D codepoint.</comment>
      <translation>{activate arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1970"/>
      <source>{national digit shapes}</source>
      <comment>Unicode U+206E codepoint.</comment>
      <translation>{national digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1971"/>
      <source>{nominal Digit shapes}</source>
      <comment>Unicode U+206F codepoint.</comment>
      <translation>{nominal Digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1972"/>
      <source>{ideographic space}</source>
      <comment>Unicode U+3000 codepoint - ideographic (CJK Wide) space</comment>
      <translation>{ideographic space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1973"/>
      <source>{variation selector 1}</source>
      <comment>Unicode U+FE00 codepoint.</comment>
      <translation>{variation selector 1}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1974"/>
      <source>{variation selector 2}</source>
      <comment>Unicode U+FE01 codepoint.</comment>
      <translation>{variation selector 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1975"/>
      <source>{variation selector 3}</source>
      <comment>Unicode U+FE02 codepoint.</comment>
      <translation>{variation selector 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1976"/>
      <source>{variation selector 4}</source>
      <comment>Unicode U+FE03 codepoint.</comment>
      <translation>{variation selector 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1977"/>
      <source>{variation selector 5}</source>
      <comment>Unicode U+FE04 codepoint.</comment>
      <translation>{variation selector 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1978"/>
      <source>{variation selector 6}</source>
      <comment>Unicode U+FE05 codepoint.</comment>
      <translation>{variation selector 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1979"/>
      <source>{variation selector 7}</source>
      <comment>Unicode U+FE06 codepoint.</comment>
      <translation>{variation selector 7}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1980"/>
      <source>{variation selector 8}</source>
      <comment>Unicode U+FE07 codepoint.</comment>
      <translation>{variation selector 8}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1981"/>
      <source>{variation selector 9}</source>
      <comment>Unicode U+FE08 codepoint.</comment>
      <translation>{variation selector 9}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1982"/>
      <source>{variation selector 10}</source>
      <comment>Unicode U+FE09 codepoint.</comment>
      <translation>{variation selector 10}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1983"/>
      <source>{variation selector 11}</source>
      <comment>Unicode U+FE0A codepoint.</comment>
      <translation>{variation selector 11}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1984"/>
      <source>{variation selector 12}</source>
      <comment>Unicode U+FE0B codepoint.</comment>
      <translation>{variation selector 12}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1985"/>
      <source>{variation selector 13}</source>
      <comment>Unicode U+FE0C codepoint.</comment>
      <translation>{variation selector 13}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1986"/>
      <source>{variation selector 14}</source>
      <comment>Unicode U+FE0D codepoint.</comment>
      <translation>{variation selector 14}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1987"/>
      <source>{variation selector 15}</source>
      <comment>Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black &amp; white) rendition.</comment>
      <translation>{variation selector 15}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1988"/>
      <source>{variation selector 16}</source>
      <comment>Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured &apos;Emoji&apos; rendition.</comment>
      <translation>{variation selector 16}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1989"/>
      <source>{zero width no-break space}</source>
      <comment>Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).</comment>
      <translation>{zero width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1995"/>
      <source>{interlinear annotation anchor}</source>
      <comment>Unicode U+FFF9 codepoint.</comment>
      <translation>{interlinear annotation anchor}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1996"/>
      <source>{interlinear annotation separator}</source>
      <comment>Unicode U+FFFA codepoint.</comment>
      <translation>{interlinear annotation separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1997"/>
      <source>{interlinear annotation terminator}</source>
      <comment>Unicode U+FFFB codepoint.</comment>
      <translation>{interlinear annotation terminator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1998"/>
      <source>{object replacement character}</source>
      <comment>Unicode U+FFFC codepoint.</comment>
      <translation>{object replacement character}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2010"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFD0 to U+FDEF - not a character.</comment>
      <translation>{noncharacter}-</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2012"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFFx - not a character.</comment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2021"/>
      <source>{FitzPatrick modifier 1 or 2}</source>
      <comment>Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.</comment>
      <translation>{FitzPatrick modifier 1 or 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2022"/>
      <source>{FitzPatrick modifier 3}</source>
      <comment>Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.</comment>
      <translation>{FitzPatrick modifier 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2023"/>
      <source>{FitzPatrick modifier 4}</source>
      <comment>Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.</comment>
      <translation>{FitzPatrick modifier 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2024"/>
      <source>{FitzPatrick modifier 5}</source>
      <comment>Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.</comment>
      <translation>{FitzPatrick modifier 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2025"/>
      <source>{FitzPatrick modifier 6}</source>
      <comment>Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.</comment>
      <translation>{FitzPatrick modifier 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2029"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.</comment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2300"/>
      <location filename="../src/TTextEdit.cpp" line="2362"/>
      <source>Index (UTF-16)</source>
      <comment>1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}</comment>
      <translation>Indeks (UTF-16)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2303"/>
      <location filename="../src/TTextEdit.cpp" line="2364"/>
      <source>U+&lt;i&gt;####&lt;/i&gt; Unicode Code-point &lt;i&gt;(High:Low Surrogates)&lt;/i&gt;</source>
      <comment>2nd Row heading for Text analyser output, table item is the unicode code point (will be between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}</comment>
      <translation>U+&lt;i&gt;####&lt;/i&gt; Punkt kodu Unicode &lt;i&gt;(Wysoki:Niskie surogatki)&lt;/i&gt;</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2307"/>
      <location filename="../src/TTextEdit.cpp" line="2368"/>
      <source>Visual</source>
      <comment>3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a &apos;{&apos;...&apos;}&apos; wrapped letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}</comment>
      <translation>Wizualne</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2311"/>
      <location filename="../src/TTextEdit.cpp" line="2372"/>
      <source>Index (UTF-8)</source>
      <comment>4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation>Indeks (UTF-8)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2315"/>
      <location filename="../src/TTextEdit.cpp" line="2376"/>
      <source>Byte</source>
      <comment>5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation>Bajtów</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2319"/>
      <location filename="../src/TTextEdit.cpp" line="2380"/>
      <source>Lua character or code</source>
      <comment>6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2 times}</comment>
      <translation>Lua znak lub kod</translation>
    </message>
  </context>
  <context>
    <name>TToolBar</name>
    <message>
      <location filename="../src/TToolBar.cpp" line="74"/>
      <source>Toolbar - %1 - %2</source>
      <translation>Pasek narzędzi - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TTrigger</name>
    <message>
      <location filename="../src/TTrigger.cpp" line="192"/>
      <source>Error: This trigger has no patterns defined, yet. Add some to activate it.</source>
      <translation>Błąd: Ten wyzwalacz nie ma ciągle zdefiniowanych wzorców. Dodaj jakiś, aby go aktywować.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="223"/>
      <source>Error: in item %1, perl regex &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>Błąd: w elemencie %1, perl regex &quot;%2&quot; nie udało się skompilować, powód: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="244"/>
      <source>Error: in item %1, lua function &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>Błąd: w elemencie %1, funkcja lua &quot;%2&quot; nie udało się skompilować, powód: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="264"/>
      <source>Error: in item %1, no colors to match were set - at least &lt;i&gt;one&lt;/i&gt; of the foreground or background must not be &lt;i&gt;ignored&lt;/i&gt;.</source>
      <translation>Błąd: w elemencie %1, nie ustawiono żadnych kolorów do dopasowania - co najmniej &lt;i&gt;jeden&lt;/i&gt; pierwszego planu lub tła nie może być &lt;i&gt;ignorowany&lt;/i&gt;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="318"/>
      <location filename="../src/TTrigger.cpp" line="396"/>
      <source>[Trigger Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation>[Błąd wyzwalacza:] Limit %1 grup przechwytywania przekroczony, przechwyć mniej grup.
</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="1156"/>
      <source>Trigger name=%1 expired.
</source>
      <translation>Wyzwalacz o nazwie=%1 Wygasł.
</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TTrigger.cpp" line="1161"/>
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
      <translation>Aktualizacja %APPNAME%</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="50"/>
      <source>Loading update information …</source>
      <translation>Pobieranie aktualnych informacji …</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="87"/>
      <source>A new version of %APPNAME% is available!</source>
      <translation>Nowa wersja %APPNAME% jest dostępna!</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="113"/>
      <source>%APPNAME% %UPDATE_VERSION% is available (you have %CURRENT_VERSION%).
Would you like to update now?</source>
      <translation>%APPNAME% %UPDATE_VERSION% jest dostępna (masz %CURRENT_VERSION%).
Czy chciałbyś teraz zaktualizować?</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="151"/>
      <source>Changelog for %APPNAME%</source>
      <translation>Lista zmian dla %APPNAME%</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="161"/>
      <source>You are using version %CURRENT_VERSION%.</source>
      <translation>Używasz wersji %CURRENT_VERSION%.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="192"/>
      <source>There are currently no updates available.</source>
      <translation>Obecnie nie ma dostępnych aktualizacji.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="208"/>
      <source>You are using %APPNAME% %CURRENT_VERSION%.</source>
      <translation>Używasz %APPNAME% %CURRENT_VERSION%.</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="321"/>
      <source>Automatically download future updates</source>
      <translation>Automatycznie pobieraj przyszłe aktualizacje</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="368"/>
      <source>Cancel</source>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="388"/>
      <source>Install update now</source>
      <translation>Zainstaluj aktualizację teraz</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="395"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="405"/>
      <source>Remind me later</source>
      <translation>Przypomnij mi później</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="410"/>
      <source>Skip this version</source>
      <translation>Pomiń tę wersję</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.cpp" line="555"/>
      <source>Could not open downloaded file %1</source>
      <translation>Nie można otworzyć pobranego pliku %1</translation>
    </message>
  </context>
  <context>
    <name>Updater</name>
    <message>
      <location filename="../src/updater.cpp" line="46"/>
      <location filename="../src/updater.cpp" line="195"/>
      <location filename="../src/updater.cpp" line="261"/>
      <source>Update</source>
      <translation>Aktualizuj</translation>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="359"/>
      <source>Restart to apply update</source>
      <translation>Uruchom ponownie, aby zainstalować aktualizację</translation>
    </message>
  </context>
  <context>
    <name>XMLimport</name>
    <message>
      <location filename="../src/XMLimport.cpp" line="165"/>
      <source>[ ALERT ] - Sorry, the file being read:
&quot;%1&quot;
reports it has a version (%2) it must have come from a later Mudlet version,
and this one cannot read it, you need a newer Mudlet!</source>
      <translation>[ ALERT ] - Niestety, plik jest odczytywany:
&quot;%1&quot;
raporty, że ma wersję (%2) musi pochodzić z późniejszej wersji Mudlet,
a ten nie może go odczytać, potrzebujesz nowszego Mudleta!</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="353"/>
      <source>Parsing area data...</source>
      <translation>Przetwarzanie danych obszarów...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="357"/>
      <source>Parsing room data...</source>
      <translation>Przetwarzanie danych lokacji...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="361"/>
      <source>Parsing environment data...</source>
      <translation>Analizowanie danych środowiska...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="369"/>
      <source>Assigning rooms to their areas...</source>
      <translation>Przypisywanie lokacji do ich obszarów...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="526"/>
      <source>Parsing room data [count: %1]...</source>
      <translation>Analizowanie danych pokoju [liczba: %1]...</translation>
    </message>
  </context>
  <context>
    <name>about_dialog</name>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="41"/>
      <source>About Mudlet</source>
      <translation>O Mudlecie</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="122"/>
      <source>Mudlet</source>
      <translation>Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="185"/>
      <source>Supporters</source>
      <translation>Wspierający</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="214"/>
      <source>License</source>
      <translation>Licencja</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="249"/>
      <source>Third Party</source>
      <translation>Zasoby zewnętrzne</translation>
    </message>
  </context>
  <context>
    <name>actions_main_area</name>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="115"/>
      <source>Button Bar Properties</source>
      <translation>Właściwości paska przycisków</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="127"/>
      <source>Number of columns/rows (depending on orientation):</source>
      <translation>Liczba kolumn/wierszy (w zależności od orientacji):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="150"/>
      <source>Orientation Horizontal</source>
      <translation>Orientacja pozioma</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="155"/>
      <source>Orientation Vertical</source>
      <translation>Orientacja pionowa</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="170"/>
      <source>Dock Area Top</source>
      <translation>Góra obszaru doku</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="175"/>
      <source>Dock Area Left</source>
      <translation>Obszar doku lewy</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="180"/>
      <source>Dock Area Right</source>
      <translation>Obszar doku prawy</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="185"/>
      <source>Floating Toolbar</source>
      <translation>Swobodny pasek narzędzi</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="208"/>
      <source>Button Properties</source>
      <translation>Właściwości przycisku</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="214"/>
      <source>Button Rotation:</source>
      <translation>Obrót przycisku:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="231"/>
      <source>no rotation</source>
      <translation>brak obrotu</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="236"/>
      <source>90° rotation to the left</source>
      <translation>Obrót o 90° w lewo</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="241"/>
      <source>90° rotation to the right</source>
      <translation>Obrót o 90° w prawo</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="249"/>
      <source>Push down button</source>
      <translation>Przełącznik wciskany</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="256"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="269"/>
      <location filename="../src/ui/actions_main_area.ui" line="289"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Tekst do wysłania do gry jak-jest (opcjonalne)</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="276"/>
      <source>Command (up):</source>
      <translation>Polecenie (góra):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="72"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your button, menu or toolbar. This will be displayed in the buttons tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz dobrą, idealnie unikalną nazwę przycisku, menu lub paska narzędzi. Zostanie to wyświetlone w drzewie przycisków.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="266"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game if it is pressed. (Optional)&lt;/p&gt;&lt;p&gt;If this is a &lt;i&gt;push-down&lt;/i&gt; button then this is sent only when the button goes from the &lt;i&gt;up&lt;/i&gt; to &lt;i&gt;down&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt; Wprowadź jedno lub więcej poleceń które zostaną wysłane bezpośrednio do gry po wciśnięciu przycisku. (Opcjonalne)&lt;/p&gt;&lt;p&gt;Jeśli jest to przełącznik włącz-wyłącz, to polecenia zostaną wysłane do gry gry zmieni się stan przycisku zmieni się z &lt;i&gt;wyłączonego&lt;/i&gt; na &lt;i&gt;włączony&lt;/i&gt;&lt;/p&gt;&lt;p&gt;Aby wysłać bardziej skomplikowane polecenia, które np. mogą być zależne od stanu zmiennych, należy &lt;i&gt;zamiast tego&lt;/i&gt; wpisać w pole poniżej skrypt Lua który to wykona. Tekst wpisany w to pole zostanie wysłany do serwera gry beż żadnych zmian.&lt;/p&gt;&lt;p&gt;Jest dopuszczalne równoczesne użycie obu pól - polecenia z tego pola zostaną wysłane &lt;b&gt;przed &lt;/b&gt; wykonaniem skryptu Lua.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="286"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game when this button goes from the &lt;i&gt;down&lt;/i&gt; to &lt;i&gt;up&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wpisz jedno lub więcej poleceń, które przycisk ma wysłać bezpośrednio do gry, gdy ten przycisk zmieni stan z &lt;i&gt;właczonego&lt;/i&gt; na &lt;i&gt;wyłączony&lt;/i&gt;.&lt;/p&gt;&lt;p&gt;Aby wysyłać bardziej złożone polecenia, które mogą zależeć od zmiennych w tym profilu lub muszą je zmodyfikować, należy wprowadzić skrypt Lua &lt;i&gt;zamiast&lt;/i&gt; wpisanych komend w obszarze edytora poniżej. Wszystko, co zostanie wpisane w to pole, jest wysyłane do gry bez zmian.&lt;/p&gt;&lt;p&gt;Można wykorzystać jednocześnie to pole &lt;i&gt;oraz&lt;/i&amp;gt skrypt Lua - zawartość tego pola zostanie wtedy wysłana &lt;b&gt;przed&lt;/b&gt; uruchomieniem skryptu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="308"/>
      <source>Stylesheet:</source>
      <translation>Arkusz stylów:</translation>
    </message>
  </context>
  <context>
    <name>aliases_main_area</name>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="35"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="57"/>
      <source>choose a unique name for your alias; it will show in the tree and is needed for scripting.</source>
      <translation>wybierz unikalną nazwę dla aliasu; zostanie ona pokazana w drzewku aliasów oraz jest używana przy tworzeniu skryptów.</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="76"/>
      <source>Pattern:</source>
      <translation>Wzorzec:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="103"/>
      <source>enter a perl regex pattern for your alias; alias are triggers on your input</source>
      <translation>wprowadź wzorzec perl regex dla swojego aliasu; alias są wyzwalaczami(triggerami) na danych wejściowych</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="110"/>
      <source>Type:</source>
      <translation>Typ:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="124"/>
      <source>Regex</source>
      <translation>Wyrażenie regularne (regex)</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="129"/>
      <source>Plain</source>
      <translation>Zwykły</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="149"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="171"/>
      <source>&lt;p&gt;Type in one or more commands you want the alias to send directly to the game if the keys entered match the pattern. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wpisz jedno lub więcej poleceń, które alias na wysłać bezpośrednio do gry, jeśli wprowadzone polecenie jest zgodne ze wzorcem. (Opcjonalne)&lt;/p&gt;&lt;p&gt;Aby wysyłać bardziej złożone polecenia, które mogą zależeć od zmiennych w tym profilu lub je modyfikują, należy &lt;i&gt;zamiast&lt;/i&gt; tego wprowadzić skrypt Lua w obszarze edytora poniżej. Wszystko, co jet wpisane w to pole jest wysyłane go gry bez zmian.&lt;/p&gt;&lt;p&gt;Wszystko, co zostanie wpisane w to pole, jest wysyłane do gry bez zmian.&lt;/p&gt;&lt;p&gt;Można wykorzystać jednocześnie to pole &lt;i&gt;oraz&lt;/i&amp;gt skrypt Lua - zawartość tego pola zostanie wtedy wysłana &lt;b&gt;przed&lt;/b&gt; uruchomieniem skryptu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="174"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>Tekst do wysłania do gry jak-jest (opcjonalne)</translation>
    </message>
  </context>
  <context>
    <name>cTelnet</name>
    <message>
      <location filename="../src/ctelnet.cpp" line="609"/>
      <source>[ INFO ]  - The IP address of %1 has been found. It is: %2
</source>
      <translation>[ INFO ] - Adres IP %1 został znaleziony. Jest to: %2
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="618"/>
      <source>[ ERROR ] - Host name lookup Failure!
Connection cannot be established.
The server name is not correct, not working properly,
or your nameservers are not working properly.</source>
      <translation>[ BŁĄD ] - Niepowodzenie wyszukiwania nazwy serwera!
Nie można nawiązać połączenia.
Nazwa serwera jest nieprawidłowa, serwer nie działa poprawnie lub serwery nazw (DNS) nie działają prawidłowo.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="449"/>
      <source>[ ERROR ] - TCP/IP socket ERROR:</source>
      <translation>[ BŁĄD ] - BŁĄD gniazda TCP/IP:</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="476"/>
      <source>[ INFO ]  - A secure connection has been established successfully.</source>
      <translation>[ INFO ] - Bezpieczne połączenie zostało pomyślnie nawiązane.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="478"/>
      <source>[ INFO ]  - A connection has been established successfully.</source>
      <translation>[ INFO ] - Połączenie zostało nawiązane pomyślnie.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="514"/>
      <source>[ INFO ]  - Connection time: %1
    </source>
      <translation>[ INFO ] - Czas połączenia: %1
    </translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="557"/>
      <source>Secure connections aren&apos;t supported by this game on this port - try turning the option off.</source>
      <translation>Bezpieczne połączenia nie są obsługiwane przez tę grę na tym porcie - spróbuj wyłączyć opcję.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="602"/>
      <source>[ INFO ]  - Trying secure connection to %1: %2 ...
</source>
      <translation>[ INFO ] - Próba bezpiecznego połączenia z %1: %2 ...
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1666"/>
      <location filename="../src/ctelnet.cpp" line="2028"/>
      <source>[ INFO ]  - The server wants to upgrade the GUI to new version &apos;%1&apos;.
Uninstalling old version &apos;%2&apos;.</source>
      <translation>[ INFO ] - Serwer chce uaktualnić GUI do nowej wersji &apos;%1&apos;.
Odinstalowywanie starej wersji &apos;%2&apos;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1677"/>
      <location filename="../src/ctelnet.cpp" line="2039"/>
      <source>[  OK  ]  - Package is already installed.</source>
      <translation>[  OK  ] - Pakiet jest już zainstalowany.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1686"/>
      <location filename="../src/ctelnet.cpp" line="2048"/>
      <source>downloading game GUI from server</source>
      <translation>pobieranie GUI gry z serwera</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1686"/>
      <location filename="../src/ctelnet.cpp" line="2048"/>
      <source>Cancel</source>
      <comment>Cancel download of GUI package from Server</comment>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1675"/>
      <location filename="../src/ctelnet.cpp" line="2037"/>
      <source>[ INFO ]  - Server offers downloadable GUI (url=&apos;%1&apos;) (package=&apos;%2&apos;).</source>
      <translation>[ INFO ] - Serwer oferuje GUI do pobrania (url=&apos;%1&apos;) (pakiet=&apos;%2&apos;).</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="418"/>
      <source>[ INFO ]  - Looking up the IP address of server: %1:%2 ...</source>
      <translation>[ INFO ] - Wyszukiwanie adresu IP serwera: %1:%2...</translation>
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
      <translation>[ UWAGA ] - Połączenie zostało odłączone. 
Powód odłączenia: </translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="611"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 ...
</source>
      <translation>[ INFO ] - Próba połączenia do %1:%2 ...
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="613"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 via proxy...
</source>
      <translation>[ INFO ] - Próba połączenia do %1:%2 przez serwer pośredniczący proxy...
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
      <translation>[BŁĄD] - Błąd wewnętrzny, nie znaleziono kodera-dekodera dla bieżącego ustawienia {&quot;%1&quot;}, więc Mudlet nie może wysłać danych w tym formacie na serwer gry. Proszę sprawdzić, czy istnieje alternatywa, której MUD i Mudlet mogą użyć. Mudlet podejmie próbę wysłania danych przy użyciu kodowania ASCII, ale będzie ograniczony tylko do podstawowych znaków bez akcentu języka angielskiego.
Uwaga: to ostrzeżenie zostanie wydane tylko raz, dopóki kodowanie nie zostanie zmienione.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2332"/>
      <source>ERROR</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>BŁĄD</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2344"/>
      <source>LUA</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>LUA</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2355"/>
      <source>WARN</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>OSTRZEŻENIE</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2366"/>
      <source>ALERT</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>UWAGA</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2377"/>
      <source>INFO</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>INFO</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2388"/>
      <source>OK</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2580"/>
      <source>[ INFO ]  - Loading replay file:
&quot;%1&quot;.</source>
      <translation>[ INFO ] - Ładowanie pliku powtórki:
&quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2599"/>
      <source>Cannot perform replay, another one may already be in progress. Try again when it has finished.</source>
      <translation>Nie można wykonać powtórki, inna może być już w toku. Spróbuj ponownie po zakończeniu.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2601"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress.
Try again when it has finished.</source>
      <translation>[ WARN ] - Nie można wykonać powtórki, inna może być już w toku.
Spróbuj ponownie po jej zakończeniu.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2609"/>
      <source>Cannot read file &quot;%1&quot;, error message was: &quot;%2&quot;.</source>
      <translation>Nie można odczytać pliku &quot;%1&quot;, komunikat o błędzie to: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2612"/>
      <source>[ ERROR ] - Cannot read file &quot;%1&quot;,
error message was: &quot;%2&quot;.</source>
      <translation>[ BŁĄD ] - Nie można odczytać pliku &quot;%1&quot;,
komunikat o błędzie to: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2640"/>
      <source>[  OK  ]  - The replay has ended.</source>
      <translation>[  OK  ] - Powtórka została zakończona.</translation>
    </message>
  </context>
  <context>
    <name>color_trigger</name>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="17"/>
      <source>ANSI 256 Color chooser</source>
      <translation>ANSI 256 Wybór kolorów</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="39"/>
      <source>&lt;small&gt;Choose:&lt;ul&gt;&lt;li&gt;one of the basic 16 colors below&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;more&lt;/i&gt; button to get access to other colors in the 256-color set, then follow the instructions to select a color from that part of the 256 colors supported; if such a color is already in use then that part will already be showing&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;Default&lt;/i&gt; or &lt;i&gt;Ignore&lt;/i&gt; buttons at the bottom for a pair of other special cases&lt;/li&gt;
&lt;li&gt;click &lt;i&gt;Cancel&lt;/i&gt; to close this dialog without making any changes&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</source>
      <comment>Ensure that &quot;Default&quot;, &quot;Ignore&quot; and &quot;Cancel&quot; in this instruction are the same as used for the controls elsewhere on this dialog.</comment>
      <translation>&lt;small&gt;Wybierz:&lt;ul&gt;&lt;li&gt;jeden z podstawowych 16 kolorów poniżej&lt;/li&gt;
&lt;li&gt;kliknij przycisk &lt;i&gt;Więcej&lt;/i&gt; , aby uzyskać dostęp do innych kolorów w zestawie 256 kolorów, a następnie postępuj zgodnie z instrukcjami, aby wybrać kolor z tej części obsługiwanych kolorów 256; jeśli taki kolor jest już w użyciu, to ta część będzie już&lt;/li&gt;
&lt;li&gt;kliknij przycisk &lt;i&gt;Domyślny&lt;/i&gt; Lub &lt;i&gt;Ignoruj&lt;/i&gt; przyciski na dole dla pary innych specjalnych przypadków&lt;/li&gt;
&lt;li&gt;Kliknij &lt;i&gt;Anuluj&lt;/i&gt; , aby zamknąć to okno dialogowe bez wprowadzania zmian&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="58"/>
      <source>Basic ANSI Colors [0-15] - click a button to select that color number directly:</source>
      <translation>Podstawowe kolory ANSI [0-15] - kliknij przycisk, aby wybrać ten numer koloru bezpośrednio:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="240"/>
      <source>ANSI 6R x 6G x 6B Colors [16-231] - adjust red, green, blue and click button to select matching color number:</source>
      <translation>ANSI 6R x 6G x 6B Kolory [16-231] - dostosuj czerwony R, zielony G, niebieski B i naciśnij przycisk, aby wybrać pasujący numer koloru:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="252"/>
      <source>Red (0-5)</source>
      <translation>Czerwony (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="262"/>
      <source>Green (0-5)</source>
      <translation>Zielony (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="272"/>
      <source>Blue (0-5)</source>
      <translation>Niebieski (0-5)</translation>
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
      <translation>Ustawianie wartości RGB</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="363"/>
      <source>ANSI 24 Grays scale [232-255] - adjust gray and click button to select matching color number:</source>
      <translation>Skala skali szarości ANSI 24 [232-255] - dostosuj szary i kliknij przycisk, aby wybrać pasujący numer kolorów:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="375"/>
      <source>Gray (0-23)</source>
      <translation>Szary (0-23)</translation>
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
      <translation>Ustaw na wartość skali szarości</translation>
    </message>
  </context>
  <context>
    <name>composer</name>
    <message>
      <location filename="../src/ui/composer.ui" line="14"/>
      <source>News and Message Composer</source>
      <translation>Nowości i kompozytor wiadomości</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="86"/>
      <source>Cancel</source>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="99"/>
      <source>&lt;p&gt;Save (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zapisz (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="102"/>
      <source>Save</source>
      <translation>Zapisz</translation>
    </message>
  </context>
  <context>
    <name>connection_profiles</name>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="20"/>
      <source>Select a profile to connect with</source>
      <translation>Wybierz profil do połączenia</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="101"/>
      <source>profiles list</source>
      <translation>lista profili</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="363"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="382"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="407"/>
      <source>New</source>
      <translation>Nowy</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="462"/>
      <source>welcome message</source>
      <translation>wiadomość powitalna</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="478"/>
      <source>Required</source>
      <translation>Wymagane</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="484"/>
      <source>Profile name:</source>
      <translation>Nazwa profilu:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="500"/>
      <source>Profile name</source>
      <translation>Nazwa profilu</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="503"/>
      <source>A unique name for the profile but which is limited to a subset of ascii characters only.</source>
      <comment>Using lower case letters for &apos;ASCII&apos; may make speech synthesisers say &apos;askey&apos; which is quicker than &apos;Aay Ess Cee Eye Eye&apos;!</comment>
      <translation>Unikalna nazwa profilu, która jest ograniczona tylko do podzbioru znaków ascii.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="513"/>
      <source>Server address:</source>
      <translation>Adres serwera:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="535"/>
      <source>Game server URL</source>
      <translation>Adres URL serwera gry</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="538"/>
      <source>The Internet host name or IP address</source>
      <translation>Wpisz nazwę hosta lub adres IP</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="545"/>
      <source>Port:</source>
      <translation>Port:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="573"/>
      <source>Game server port</source>
      <translation>Port serwera gry</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="576"/>
      <source>The port that is used together with the server name to make the connection to the game server. If not specified a default of 23 for &quot;Telnet&quot; connections is used. Secure connections may require a different port number.</source>
      <translation>Port, który jest używany wraz z nazwą serwera do nawiązania połączenia z serwerem gry. Jeśli nie podano, dla połączeń &quot;Telnet&quot; jest używana wartość domyślna 23. Bezpieczne połączenia mogą wymagać innego numeru portu.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="595"/>
      <source>Connect via a secure protocol</source>
      <translation>Połącz się za pomocą bezpiecznego protokołu</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="598"/>
      <source>Make Mudlet use a secure SSL/TLS protocol instead of an unencrypted one</source>
      <translation>Spraw, aby Mudlet używał bezpiecznego protokołu SSL/TLS zamiast niezaszyfrowanego</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="604"/>
      <source>Secure:</source>
      <translation>Zabezpieczenie:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="611"/>
      <source>Profile history:</source>
      <translation>Historia profilu:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="634"/>
      <source>load newest profile</source>
      <translation>wczytaj najnowszy profil</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="639"/>
      <source>load oldest profile</source>
      <translation>wczytaj najstarszy profil</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="662"/>
      <source>Optional</source>
      <translation>Opcjonalne</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="678"/>
      <source>Character name:</source>
      <translation>Nazwa postaci:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="688"/>
      <source>The characters name</source>
      <translation>Nazwa postaci</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="691"/>
      <source>Character name</source>
      <translation>Nazwa postaci</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="694"/>
      <source>If provided will be sent, along with password to identify the user in the game.</source>
      <translation>Jeśli zostanie podane, zostanie wysłane wraz z hasłem, aby zidentyfikować użytkownika w grze.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="713"/>
      <source>Auto-open profile</source>
      <translation>Automatyczne otwieranie profilu</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="716"/>
      <source>Automatically start this profile when Mudlet is run</source>
      <translation>Automatycznie uruchom ten profil po uruchomieniu Mudletu</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="729"/>
      <source>Auto-reconnect</source>
      <translation>Automatyczne łączenie ponowne</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="732"/>
      <source>Automatically reconnect this profile if it should become disconnected for any reason other than the user disconnecting from the game server.</source>
      <translation>Automatycznie połącz ten profil ponownie, gdyby został on odłączony z jakiegokolwiek innego powodu niż odłączenie się użytkownika z serwera gry.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="748"/>
      <source>Password</source>
      <translation>Hasło</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="751"/>
      <source>If provided will be sent, along with the character name to identify the user in the game.</source>
      <translation>Jeśli zostanie podane, zostanie wysłane wraz z imieniem postaci, aby zidentyfikować użytkownika w grze.</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="764"/>
      <source>Enable Discord integration (not supported by game)</source>
      <translation>Włącz integrację Discorda (nie jest obsługiwana przez grę)</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="770"/>
      <source>Allow this profile to use Mudlet&apos;s Discord &quot;Rich Presence&quot;  features</source>
      <translation>Pozwól temu profilowi używać funkcji &quot;Rich Presence&quot; Discorda</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="876"/>
      <location filename="../src/ui/connection_profiles.ui" line="879"/>
      <source>Game description or your notes</source>
      <translation>Opis gry na twojej notatce</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="668"/>
      <source>Password:</source>
      <translation>Hasło:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="745"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation>Hasło postaci. Uwaga, hasła nie są zapisywane zaszyfrowane</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="710"/>
      <source>With this enabled, Mudlet will automatically start and connect on this profile when it is launched</source>
      <translation>Mudlet po uruchomieniu będzie automatycznie łączył się z tym profilem</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="722"/>
      <source>Open profile on Mudlet start</source>
      <translation>Otwórz profil przy startowaniu Mudleta</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="738"/>
      <source>Reconnect automatically</source>
      <translation>Połącz się ponownie automatycznie</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="767"/>
      <source>Discord integration</source>
      <translation>Integracja z Discordem</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="805"/>
      <source>Informational</source>
      <translation>Informacyjne</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="832"/>
      <source>Website:</source>
      <translation>Strona WWW:</translation>
    </message>
  </context>
  <context>
    <name>custom_line_properties</name>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="27"/>
      <source>Custom Line Properties [*]</source>
      <translation>Właściwości wiersza niestandardowego [*]</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="46"/>
      <source>Line Settings:</source>
      <translation>Ustawienia linii:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="84"/>
      <source>Color:</source>
      <translation>Kolor:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="58"/>
      <source>Style:</source>
      <translation>Styl:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="43"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz styl, kolor i czy chcesz zakończyć linię nagłówkiem strzałki.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="122"/>
      <source>Ends with an arrow:</source>
      <translation>Zakończona strzałką:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="138"/>
      <source>Exit Details:</source>
      <translation>Szczegóły wyjścia:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="153"/>
      <source>Origin:</source>
      <translation>Początek:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="226"/>
      <source>Destination:</source>
      <translation>Koniec:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="188"/>
      <source>    Direction/Command:</source>
      <translation>    Kierunek/Polecenie:</translation>
    </message>
  </context>
  <context>
    <name>custom_lines</name>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="14"/>
      <source>Custom Line selection</source>
      <translation>Wybór niestandardowej linii</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="44"/>
      <source>Choose line format, color and arrow option and then select the exit to start drawing</source>
      <translation>Wybierz format linii, kolor i opcje strzałki, a następnie wybierz wyjście, aby rozpocząć rysowanie</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="63"/>
      <source>Line Settings:</source>
      <translation>Ustawienia linii:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="97"/>
      <source>Ends with an arrow:</source>
      <translation>Zakończona strzałką:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="110"/>
      <source>Style:</source>
      <translation>Styl:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="126"/>
      <source>Color:</source>
      <translation>Kolor:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="47"/>
      <source>&lt;p&gt;Selecting an exit immediately proceeds to drawing the first line segment from the centre point of the room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierając wyjście natychmiast wybiera odcinek pierwszej linii z punktu środkowego pokoju.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="60"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head BEFORE then choosing the exit to draw the line for...&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz Styl, Kolor i czy zakończyć linię za pomocą strzałki PRZED, a następnie wybierając wyjście, aby narysować linię dla...&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="187"/>
      <source>&lt;p&gt;Select a normal exit to commence drawing a line for it, buttons are shown depressed if they already have such a custom line and disabled if there is not exit in that direction.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz normalne wyjście, aby rozpocząć rysowanie linii dla niego, przyciski są wyświetlane wciśnięty, jeśli już mają taką linię niestandardową i wyłączone, jeśli nie ma wyjścia w tym kierunku.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="190"/>
      <source>Normal Exits:</source>
      <translation>Wyjścia zwyczajne:</translation>
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
      <translation>NE</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="297"/>
      <source>UP</source>
      <translation>W GÓRĘ</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="336"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="346"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="356"/>
      <source>IN</source>
      <translation>DO</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="382"/>
      <source>OUT</source>
      <translation>NA ZEWNĄTRZ</translation>
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
      <translation>SE</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="422"/>
      <source>DOWN</source>
      <translation>W DÓŁ</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="444"/>
      <source>&lt;p&gt;Select a special exit to commence drawing a line for it, the first column is checked if the exit already has such a custom line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz specjalne wyjście, aby rozpocząć rysowanie linii dla niego, pierwsza kolumna jest sprawdzana, jeśli wyjście ma już taki wiersz niestandardowy.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="503"/>
      <source>&lt;p&gt;Indicates if there is already a custom line for this special exit, will be replaced if the exit is selected.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wskazuje, czy istnieje już niestandardowa linia dla tego specjalnego wyjścia, zostanie zastąpiona, jeśli wyjście zostanie wybrane.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="514"/>
      <source>&lt;p&gt;The room this special exit leads to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Pokój do którego prowadzi to specjalne wyjście.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="525"/>
      <source>&lt;p&gt;The command or LUA script that goes to the given room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Komenda lub skrypt LUA, który przechodzi do danego pomieszczenia.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="561"/>
      <source>&lt;p&gt;To remove a custom line: cancel this dialog, select the line and right-click to obtain a &amp;quot;delete&amp;quot; option.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Aby usunąć niestandardową linię: anuluj to okno dialogowe, zaznacz linię i kliknij prawym przyciskiem myszy, aby uzyskać &amp;cytat; usuń&amp;cytat; opcje.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="447"/>
      <source>Special Exits:</source>
      <translation>Wyjścia specjalne:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="499"/>
      <source>Has
custom line?</source>
      <translation>Posiada niestandardową linię?</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="511"/>
      <source> Destination </source>
      <translation> Koniec </translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="522"/>
      <source> Command</source>
      <translation> Polecenie</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="567"/>
      <source>Cancel</source>
      <translation>Anuluj</translation>
    </message>
  </context>
  <context>
    <name>delete_profile_confirmation</name>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="14"/>
      <source>Confirm permanent profile deletion</source>
      <translation>Potwierdź usunięcie profilu</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="26"/>
      <source>Are you sure that you&apos;d like to delete this profile? Everything (aliases, triggers, backups, etc) will be gone.

If you are, please type in the profile name as a confirmation:</source>
      <translation>Czy na pewno chcesz usunąć ten profil? Wszystko (aliasy, wyzwalacze, kopie zapasowe itp.) zostanie usunięte.

Jeśli tak, wpisz nazwę profilu w celu potwierdzenia:</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="54"/>
      <source>Delete</source>
      <translation>Skasuj</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="61"/>
      <source>Cancel</source>
      <translation>Anuluj</translation>
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
      <translation>Symbol
(Ustaw czcionkę)</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="50"/>
      <source>Symbol
(All Fonts)</source>
      <translation>Symbol
(Wszystkie czcionki)</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="56"/>
      <source>Codepoints</source>
      <translation>Punkty Kodowania</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="61"/>
      <source>Usage
Count</source>
      <translation>Licznik użycia</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="67"/>
      <source>Rooms</source>
      <translation>Pokoje</translation>
    </message>
  </context>
  <context>
    <name>directions</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14290"/>
      <source>north</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>północ</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14292"/>
      <source>n</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14294"/>
      <source>east</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>wschód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14296"/>
      <source>e</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>e</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14298"/>
      <source>south</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>południe</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14300"/>
      <source>s</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14302"/>
      <source>west</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>zachód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14304"/>
      <source>w</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>w</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14306"/>
      <source>northeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>północny wschód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14308"/>
      <source>ne</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>ne</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14310"/>
      <source>southeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>południowy wschód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14312"/>
      <source>se</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>se</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14314"/>
      <source>southwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>południowy zachód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14316"/>
      <source>sw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>sw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14318"/>
      <source>northwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>północny zachód</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14320"/>
      <source>nw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>nw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14322"/>
      <source>in</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>do środka</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14324"/>
      <source>i</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>i</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14326"/>
      <source>out</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>na zewnątrz</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14328"/>
      <source>o</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14330"/>
      <source>up</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>u</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14332"/>
      <source>u</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>u</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14334"/>
      <source>down</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>d</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14336"/>
      <source>d</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>d</translation>
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
      <translation>&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Strona domowa&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://www.mudlet.org/&quot;&gt;www.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Forum&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://forums.mudlet.org/&quot;&gt;forums.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Dokumentacja&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://wiki.mudlet.org/w/Main_Page&quot;&gt;wiki.mudlet.org/w/Main_Page&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#7289DA;&quot;&gt;&lt;b&gt;Discord&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://www.mudlet.org/chat&quot;&gt;discord.gg&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Kod źródłowy&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet&quot;&gt;github.com/Mudlet/Mudlet&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Zgłaszanie błędów&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/issues&quot;&gt;github.com/Mudlet/Mudlet/issues&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="155"/>
      <source>Original author, original project lead, Mudlet core coding, retired.</source>
      <comment>about:Heiko</comment>
      <translation>Oryginalny autor, oryginalny kierownik projektu, kodowanie rdzenia Mudlet, w stanie spoczynku.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="158"/>
      <source>GUI design and initial feature planning. He is responsible for the project homepage and the user manual. Maintainer of the Windows, macOS, Ubuntu and generic Linux installers. Maintains the Mudlet wiki, Lua API, and handles project management, public relations &amp;amp; user help. With the project from the very beginning and is an official spokesman of the project. Since the retirement of Heiko, he has become the head of the Mudlet project.</source>
      <comment>about:Vadi</comment>
      <translation>Projektowanie gui i wstępne planowanie funkcji. Jest odpowiedzialny za stronę główną projektu i instrukcję obsługi. Opiekun windows, macOS, Ubuntu i ogólnych instalatorów Linuksa. Utrzymuje mudlet wiki, Lua API i obsługuje zarządzanie projektami, public relations &amp; pomocy użytkownika. Z projektem od samego początku i jest oficjalnym rzecznikiem projektu. Od czasu przejścia na emeryturę Heiko został szefem projektu Mudlet.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="165"/>
      <source>After joining in 2013, he has been poking various bits of the C++ code and GUI with a pointy stick; subsequently trying to patch over some of the holes made/found. Most recently he has been working on I18n and L10n for Mudlet 4.0.0 so if you are playing Mudlet in a language other than American English you will be seeing the results of him getting fed up with the spelling differences between what was being used and the British English his brain wanted to see.</source>
      <comment>about:SlySven</comment>
      <translation>Po dołączeniu w 2013 roku, został i grzebie różne bity kodu C ++ i GUI szpiczastym kijem; następnie próbuje załatać niektóre dziury wykonane lub znalezione. Ostatnio pracuje nad I18n i L10n dla Mudlet 4.0.0 więc jeśli grasz Mudlet w języku innym niż amerykański czy angielski będzie można zobaczyć jego wyniki oraz dość różnic ortograficznych między nimi, co było używane i brytyjski-angielski jego mózg chciał zobaczyć.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="172"/>
      <source>Former maintainer of the early Windows and Apple OSX packages. He also administers our server and helps the project in many ways.</source>
      <comment>about:demonnic</comment>
      <translation>Były opiekun wczesnych pakietów Windows i Apple OSX. Zarządza także naszym serwerem i pomaga projektowi na wiele sposobów.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="176"/>
      <source>Contributed many improvements to Mudlet&apos;s db: interface, event system, and has been around the project for a very long while assisting users.</source>
      <comment>about:keneanung</comment>
      <translation>Przyczynił się wiele ulepszeń do Mudlet&apos;s db: interfejs, system zdarzeń i jest wokół projektu od bardzo dawna, pomagając użytkownikom.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="180"/>
      <source>Does a ton of work in making Mudlet, the website and the wiki accessible to you regardless of the language you speak - and promoting our genre!</source>
      <comment>about:Leris</comment>
      <translation>Wykonuje mnóstwo przy tworzeniu Mudlet, strona internetowa i wiki dostępne dla ciebie niezależnie od języka, który mówisz - i promocja naszego rodzaju!</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="184"/>
      <source>Contributions to the Travis integration, CMake and Visual C++ build, a lot of code quality and memory management improvements.</source>
      <comment>about:ahmedcharles</comment>
      <translation>Wkład w integrację Travis, CMake i Visual C++ kompilacji, wiele ulepszeń jakości kodu i zarządzania pamięcią.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="188"/>
      <source>Developed a shared module system that allows script packages to be shared among profiles, a UI for viewing Lua variables, improvements in the mapper and all around.</source>
      <comment>about:Chris7</comment>
      <translation>Opracował wspólny system modułów, który umożliwia udostępnianie pakietów skryptów między profilami, interfejs użytkownika do przeglądania zmiennych Lua, ulepszenia w maperze i na całości.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="192"/>
      <source>Developed the first version of our Mac OSX installer. He is the former maintainer of the Mac version of Mudlet.</source>
      <comment>about:Ben Carlsen</comment>
      <translation>Opracował pierwszą wersję instalatora Mac OSX. Twórca i były opiekun wersji Mac.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="196"/>
      <source>Joined in December 2009 though he&apos;s been around much longer. Contributed to the Lua API and is the former maintainer of the Lua API.</source>
      <comment>about:Ben Smith</comment>
      <translation>Dołączył w grudniu 2009 r., choć współpracował już znacznie dłużej. Przyczynił się do rozwoju Lua API i jest byłym opiekunem LUA API.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="200"/>
      <source>Joined in December 2009. He has contributed to the Lua API, submitted small bugfix patches and has helped with release management of 1.0.5.</source>
      <comment>about:Blaine von Roeder</comment>
      <translation>Dołączył w grudniu 2009 roku. Przyczynił się do rozwoju API Lua, przesłał małe poprawki błędów i pomógł w zarządzaniu wydaniem 1.0.5.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="204"/>
      <source>Developed the original cmake build script and he has committed a number of patches.</source>
      <comment>about:Bruno Bigras</comment>
      <translation>Opracował oryginalny skrypt kompilacji cmake i popełnił szereg poprawek.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="207"/>
      <source>Contributed to the Lua API.</source>
      <comment>about:Carter Dewey</comment>
      <translation>Przyczynił się do rozwoju lua API.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="210"/>
      <source>Developed the Vyzor GUI Manager for Mudlet.</source>
      <comment>about:Oneymus</comment>
      <translation>Opracował Vyzor GUI Manager dla Mudlet.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="213"/>
      <source>Worked wonders in rejuvenating our Website in 2017 but who prefers a little anonymity - if you are a &lt;i&gt;SpamBot&lt;/i&gt; you will not get onto our Fora now. They have also made some useful C++ core code contributions and we look forward to future reviews on and work in that area.</source>
      <comment>about:TheFae</comment>
      <translation>Zdziałał cuda przy odświeżaniu naszej strony w 2017 roku. Lubi prywatność - jeżeli jesteś &lt;i&gt;SpamBotem&lt;/i&gt; nie dostaniesz się już na nasze fora. Wykonał także kilka przydatnych zmian z podstawowym kodzie C++ aplikacji, mamy nadzieję na jego przyszły wkład i udział w ocenianiu zmian w kodzie.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="218"/>
      <source>Joining us 2017 they have given us some useful C++ and Lua contributions.</source>
      <comment>about:Dicene</comment>
      <translation>Dołączając do nas 2017 dał nam przydatny wkład w C++ i Lua.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="221"/>
      <source>Contributed the Geyser layout manager for Mudlet in March 2010. It is written in Lua and aims at simplifying user GUI scripting.</source>
      <comment>about:James Younquist</comment>
      <translation>Przyczynił się do menedżera układów Geyser dla Mudlet w marcu 2010 r. Jest on napisany w Lua i ma na celu uproszczenie skryptu GUI użytkownika.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="225"/>
      <source>Helped develop and debug the Lua API.</source>
      <comment>about:John Dahlström</comment>
      <translation>Pomaga rozwinąć i debugować API Lua.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="228"/>
      <source>Contributed several improvements and new features for Geyser.</source>
      <comment>about:Beliaar</comment>
      <translation>Przyczynił się do kilku ulepszeń i nowych funkcji dla Geyser.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="231"/>
      <source>The original author of our Windows installer.</source>
      <comment>about:Leigh Stillard</comment>
      <translation>Oryginalny autor naszego instalatora Windows.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="234"/>
      <source>Worked on the manual, forum help and helps with GUI design and documentation.</source>
      <comment>about:Maksym Grinenko</comment>
      <translation>Pracował nad instrukcją, pomocą na forum i pomaga w projektowaniu GUI i dokumentacji.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="237"/>
      <source>Developed a database Lua API that allows for far easier use of databases and one of the original OSX installers.</source>
      <comment>about:Stephen Hansen</comment>
      <translation>Stworzył bazę danych Lua API, która pozwala na znacznie łatwiejsze korzystanie z baz danych i jednego z oryginalnych instalatorów OSX.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="240"/>
      <source>Designed our beautiful logo, our splash screen, the about dialog, our website, several icons and badges. Visit his homepage at &lt;a href=&quot;http://thorwil.wordpress.com/&quot;&gt;thorwil.wordpress.com&lt;/a&gt;.</source>
      <comment>about:Thorsten Wilms</comment>
      <translation>Zaprojektował nasze piękne logo, nasz splash screen, okno dialogowe, naszą stronę internetową, kilka ikon i odznaki. Odwiedź jego stronę internetową pod &lt;a href=&quot;http://thorwil.wordpress.com/&quot;&gt;thorwil.wordpress.com&lt;/a&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="249"/>
      <source>&lt;p&gt;Others too, have make their mark on different aspects of the Mudlet project and if they have not been mentioned here it is by no means intentional! For past contributors you may see them mentioned in the &lt;b&gt;&lt;a href=&quot;https://launchpad.net/~mudlet-makers/+members#active&quot;&gt;Mudlet Makers&lt;/a&gt;&lt;/b&gt; list (on our former bug-tracking site), or for on-going contributors they may well be included in the &lt;b&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/graphs/contributors&quot;&gt;Contributors&lt;/a&gt;&lt;/b&gt; list on GitHub.&lt;/p&gt;
&lt;br&gt;
&lt;p&gt;Many icons are taken from the &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;&lt;u&gt;KDE4 oxygen icon theme&lt;/u&gt;&lt;/b&gt;&lt;/span&gt; at &lt;a href=&quot;https://web.archive.org/web/20130921230632/http://www.oxygen-icons.org/&quot;&gt;www.oxygen-icons.org &lt;sup&gt;{wayback machine archive}&lt;/sup&gt;&lt;/a&gt; or &lt;a href=&quot;http://www.kde.org&quot;&gt;www.kde.org&lt;/a&gt;.  Most of the rest are from Thorsten Wilms, or from Stephen Lyons combining bits of Thorsten&apos;s work with the other sources.&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Brett Duzevich&lt;/b&gt;&lt;/span&gt; and &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Ronny Ho&lt;/b&gt;&lt;/span&gt;. They have contributed many good ideas and thus helped improve the scripting framework substantially.&lt;/p&gt;
&lt;p&gt;Thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Tomas Mecir&lt;/b&gt;&lt;/span&gt; (&lt;span style=&quot;color:#0000ff;&quot;&gt;kmuddy@kmuddy.com&lt;/span&gt;) who brought us all together and inspired us with his KMuddy project. Mudlet is using some of the telnet code he wrote for his KMuddy project (&lt;a href=&quot;https://cgit.kde.org/kmuddy.git/&quot;&gt;cgit.kde.org/kmuddy.git/&lt;/a&gt;).&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Nick Gammon&lt;/b&gt;&lt;/span&gt; (&lt;a href=&quot;http://www.gammon.com.au/mushclient/mushclient.htm&quot;&gt;www.gammon.com.au/mushclient/mushclient.htm&lt;/a&gt;) for giving us some valued pieces of advice.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Inni też odcisnęli swoje piętno na różnych aspektach projektu Mudlet i jeśli nie zostały tu wymienione, to w żadnym wypadku nie jest to celowe! Wcześniejsi współpracownicy mogą zobaczyć ich wspomnienia na liście &lt;b&gt;&lt;a href=&quot;https://launchpad.net/~mudlet-makers/+members#active&quot;&gt;Mudlet Makers&lt;/a&gt;&lt;/b&gt; (na naszej dawnej stronie śledzenia błędów), lub dla stałych współpracowników mogą oni zostać włączeni do listy &lt;b&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/graphs/contributors&quot;&gt;Uczestników&lt;/a&gt;&lt;/b&gt; na GitHubie.&lt;/p&gt;
&lt;br&gt;
&lt;p&gt;Wiele ikon zostało pobranych z &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;&lt;u&gt;KDE4 temat ikony tlenu&lt;/u&gt;&lt;/b&gt;&lt;/span&gt; na &lt;a href=&quot;https://web.archive.org/web/20130921230632/http://www.oxygen-icons.org/&quot;&gt;www.oxygen-icons.org &lt;sup&gt;{wayback machine archive}&lt;/sup&gt;&lt;/a&gt; lub &lt;a href=&quot;http://www.kde.org&quot;&gt;www.kde.org&lt;/a&gt;.  Większość pozostałych pochodzi od Thorstena Wilmsa, lub od Stephena Lyonsa łączącego bity Thorstena&apos;s pracy z innymi źródłami.&lt;/p&gt;
&lt;p&gt;Specjalne podziękowania dla &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Brett Duzewicz&lt;/b&gt;&lt;/span&gt; oraz &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Ronny Ho&lt;/b&gt;&lt;/span&gt;. Przyczyniły się one do powstania wielu dobrych pomysłów i w ten sposób przyczyniły się do znacznego ulepszenia ram skryptowych.&lt;/p&gt;
&lt;p&gt;Skromne podziękowania dla &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Michała W.&lt;/b&gt;&lt;/span&gt; alias &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Dogid&lt;/b&gt;&lt;/span&gt;. autora tego koślawego tłumaczenia na Polski. Ku chwale Arkadii!&lt;/p&gt;
&lt;p&gt;Dzięki &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Tomas Mecir&lt;/b&gt;&lt;/span&gt; (&lt;span style=&quot;color:#0000ff;&quot;&gt;kmuddy@kmuddy.com&lt;/span&gt;), który połączył nas wszystkich i zainspirował swoim projektem KMuddy. Mudlet wykorzystuje część kodu telnetowego, który napisał dla swojego projektu KMuddy (&lt;a href=&quot;https://cgit.kde.org/kmuddy.git/&quot;&gt;cgit.kde.org/kmuddy.git/&lt;/a&gt;).&lt;/p&gt;
&lt;p&gt;Specjalne podziękowania dla &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Nick Gammon&lt;/b&gt;&lt;/span&gt; (&lt;a href=&quot;http://www.gammon.com.au/mushclient/mushclient.htm&quot;&gt;www.gammon.com.au/mushclient/mushclient.htm&lt;/a&gt;) za udzielenie nam kilku cennych rad.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="294"/>
      <source>&lt;p&gt;Mudlet was originally written by Heiko Köhn, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet is released under the GPL license version 2, which is reproduced below:&lt;/p&gt;</source>
      <comment>For non-english language versions please append a translation of the following to explain why the GPL is NOT reproduced in the relevant language: &apos;but only the English form is considered the official version of the license, so the following is reproduced in that language:&apos; to replace &apos;which is reproduced below:&apos;...</comment>
      <translation>&lt;p&gt;Mudlet został pierwotnie napisany przez Heiko Köhna, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet został wydany na licencji GPL w wersji 2, która została powielona poniżej:&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="590"/>
      <source>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; is built upon the shoulders of other projects in the FOSS world; as well as using many GPL components we also make use of some third-party software with other licenses:&lt;/p&gt;</source>
      <translation>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; jest zbudowany przy wsparciu innych projektów ze świata FOSS; oprócz tego, że korzystamy z wielu komponentów GPL, korzystamy także z niektórych programów innych firm na innych licencjach:&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="832"/>
      <source>&lt;h2&gt;&lt;u&gt;Communi IRC Library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008-2020 The Communi Project&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Komunalna Biblioteka IRC&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Prawa autorskie © 2008-2020 The Communi Projec&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="835"/>
      <source>&lt;p&gt;Parts of &lt;tt&gt;irctextformat.cpp&lt;/t&gt; code come from Konversation and are copyrighted to:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Parts of &lt;tt&gt;irctextformat.cpp&lt;/t&gt; code come from Konversation and are copyrighted to:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="841"/>
      <source>&lt;h2&gt;&lt;u&gt;lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="844"/>
      <source>&lt;h2&gt;&lt;u&gt;lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="849"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="854"/>
      <source>&lt;h2&gt;&lt;u&gt;edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="857"/>
      <source>The &lt;b&gt;edbee-lib&lt;/b&gt; widget itself incorporates other components with licences that must be noted as well, they are:</source>
      <translation>Widżet &lt;b&gt;edbee-lib&lt;/b&gt; sam w sobie zawiera inne komponenty z licencjami, które muszą być odnotowane, są one również:</translation>
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
      <translation>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [broken link]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archived version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="879"/>
      <source>&lt;h2&gt;&lt;u&gt;dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Prawa autorskie © 2017 Philipp Medien&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="882"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Sparkle - aktualizacja macOS&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Prawa autorskie © 2006-2013 Andy Matuschak.&lt;br&gt;Prawa autorskie © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Prawa autorskie © 2011-2014 Kornel Lesiński.&lt;br&gt;Prawa autorskie © 2015-2017 Mayur Pawashe.&lt;br&gt;Prawa autorskie © 2014 C.W. Betts.&lt;br&gt;Prawa autorskie © 2014 Petroules Corporation.&lt;br&gt;Prawa autorskie © 2014 Big Nerd Ranch.&lt;br&gt;Wszelkie prawa zastrzeżone.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="892"/>
      <source>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h4&gt;bspatch.c i bsdiff.c, od bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Prawa autorskie © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c i sais.c, z sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Prawa autorskie © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Prawa autorskie © 2011 Mark Hamlin.&lt;br&gt;Wszelkie prawa zastrzeżone.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="900"/>
      <source>&lt;h2&gt;&lt;u&gt;sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim Peretokin&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim PereTokin&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="906"/>
      <source>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - Biblioteka RPC&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="909"/>
      <source>&lt;h2&gt;&lt;u&gt;QtKeyChain - Platform-independent Qt API for storing passwords securely&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;QtKeyChain - Niezależny od platformy interfejs API Qt do bezpiecznego przechowywania haseł&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Prawa autorskie © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1057"/>
      <source>
                          These formidable folks will be fondly remembered forever&lt;br&gt;for their generous financial support on &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s patreon&lt;/a&gt;:
                          </source>
      <translation>
                          Ci wspaniali goście zostaną zapamiętani na zawsze&lt;br&gt;za ich hojne wsparcie finansowe na &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s patreon&lt;/a&gt;:
                          </translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1068"/>
      <source>Technical information:</source>
      <translation>Informacje techniczne:</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1069"/>
      <source>Version</source>
      <translation>Wersja</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1070"/>
      <source>OS</source>
      <translation>System operacyjny</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1071"/>
      <source>CPU</source>
      <translation>Procesor</translation>
    </message>
  </context>
  <context>
    <name>dlgColorTrigger</name>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="52"/>
      <source>More colors</source>
      <translation>Więcej kolorów</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="54"/>
      <source>Click to access all 256 ANSI colors.</source>
      <translation>Kliknij, aby uzyskać dostęp do wszystkich 256 kolorów ANSI.</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="59"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s background color - however choosing this for both foreground and background is an error.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby wyzwalacz kolorów ignorował koloru tła tekstu. Wybranie tego zarówno dla tła jak i dla koloru samego tekstu jest błędem.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="60"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s foreground color - however choosing this for both foreground and background is an error.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby wyzwalacz kolorów ignorował koloru tekstu. Wybranie tego zarówno dla koloru tekstu jak i dla tła tekstu jest błędem.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="64"/>
      <source>Default</source>
      <translation>Domyślne</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="66"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s background color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby ustawić wyzwalacz koloru, gdy tekst kolor tła nie został zmodyfikowany z jego wartości normalnej.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="67"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s foreground color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby ustawić wyzwalacz koloru, gdy tekst kolor pierwszoplanowy nie został zmodyfikowany z wartości normalnej.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="71"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s background color matches the color number indicated.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij na kolor, aby wyzwalać tylko wtedy, gdy kolor tła odpowiada wskazanemu numerowi koloru.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="72"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s foreground color matches the color number indicated.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij na kolor, aby wyzwalać tylko wtedy, gdy kolor na pierwszym planie odpowiada wskazanemu numerowi koloru.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="77"/>
      <source>Black</source>
      <translation>Czarny</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="78"/>
      <source>Red</source>
      <translation>Czerwony</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="79"/>
      <source>Green</source>
      <translation>Zielony</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="80"/>
      <source>Yellow</source>
      <translation>Żółty</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="81"/>
      <source>Blue</source>
      <translation>Niebieski</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="82"/>
      <source>Magenta</source>
      <translation>Purpura</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="83"/>
      <source>Cyan</source>
      <translation>Cyjan</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="84"/>
      <source>White (Light gray)</source>
      <translation>Biały (jasnoszary)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="86"/>
      <source>Light black (Dark gray)</source>
      <translation>Jasny czarny (ciemnoszary)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="87"/>
      <source>Light red</source>
      <translation>Rozjaśniony czerwony</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="88"/>
      <source>Light green</source>
      <translation>Rozjaśniony zielony</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="89"/>
      <source>Light yellow</source>
      <translation>Rozjaśniony żółty</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="90"/>
      <source>Light blue</source>
      <translation>Rozjaśniony niebieski</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="91"/>
      <source>Light magenta</source>
      <translation>Rozjaśniona purpura</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="92"/>
      <source>Light cyan</source>
      <translation>Rozjaśniony cyjan</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="93"/>
      <source>Light white</source>
      <translation>Rozjaśniony biały</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="218"/>
      <source>%1 [%2]</source>
      <comment>Color Trigger dialog button in basic 16-color set, the first value is the name of the color, the second is the ANSI color number - for most languages modification is not likely to be needed - this text is used in two places</comment>
      <translation>%1 [%2] </translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="375"/>
      <source>All color options are showing.</source>
      <translation>Wyświetlane są wszystkie opcje kolorów.</translation>
    </message>
  </context>
  <context>
    <name>dlgConnectionProfiles</name>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="98"/>
      <source>Connect</source>
      <translation>Połącz</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="253"/>
      <source>Game name: %1</source>
      <translation>Nazwa gry: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1010"/>
      <source>This profile is currently loaded - close it before changing the connection parameters.</source>
      <translation>Ten profil jest aktualnie załadowany — zamknij go przed zmianą parametrów połączenia.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1700"/>
      <source>The %1 character is not permitted. Use one of the following:</source>
      <translation>Znak %1 jest niedozwolony. Użyj jednego z poniższych znaków:</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1723"/>
      <source>You have to enter a number. Other characters are not permitted.</source>
      <translation>Wprowadź numer, żadne znaki poza cyframi nie są dozwolone.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1712"/>
      <source>This profile name is already in use.</source>
      <translation>Nazwa profilu jest już w użyciu.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="509"/>
      <source>Could not rename your profile data on the computer.</source>
      <translation>Nie można zmienić nazwy danych twojego profilu na komputerze.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="110"/>
      <source>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Welcome to Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Click on one of the games on the list to play.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;To play a game not in the list, click on %1 &lt;span style=&quot; color:#555753;&quot;&gt;New&lt;/span&gt;, fill in the &lt;i&gt;Profile Name&lt;/i&gt;, &lt;i&gt;Server address&lt;/i&gt;, and &lt;i&gt;Port&lt;/i&gt; fields in the &lt;i&gt;Required &lt;/i&gt; area.&lt;/p&gt;&lt;p&gt;After that, click %2 &lt;span style=&quot; color:#555753;&quot;&gt;Connect&lt;/span&gt; to play.&lt;/p&gt;&lt;p&gt;Have fun!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;The Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</source>
      <comment>Welcome message. Both %1 and %2 may be replaced by icons when this text is used.</comment>
      <translation>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Witamy w Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Kliknij na jedną z gier na liście, aby zagrać.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;Aby zagrać w inną grę, która nie znajduje się na liście (tłumacz poleca Arkadia MUD), kliknij %1 &lt;span style=&quot; color:#555753;&quot;&gt;Nowy&lt;/span&gt;, wypełnij &lt;i&gt;Nazwa profilu&lt;/i&gt;, &lt;i&gt;Adres serwera&lt;/i&gt;I &lt;i&gt;Portu&lt;/i&gt; pól w &lt;i&gt;Wymagane &lt;/i&gt; Obszar.&lt;/p&gt;&lt;p&gt;Następnie kliknij przycisk %2 &lt;span style=&quot; color:#555753;&quot;&gt;Połączyć&lt;/span&gt; aby zagrać.&lt;/p&gt;&lt;p&gt;Baw się dobrze!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;Zespół Mudleta. &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="100"/>
      <source>Offline</source>
      <translation>Bez połączenia</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="123"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="125"/>
      <source>Copy settings only</source>
      <translation>Kopiuj tylko ustawienia</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="133"/>
      <source>copy profile</source>
      <translation>kopiuj profil</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="134"/>
      <source>copy the entire profile to new one that will require a different new name.</source>
      <translation>skopiuj cały profil do nowego, który będzie wymagał nowej, zmienionej nazwy.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="138"/>
      <source>copy profile settings</source>
      <translation>kopiuj ustawienia profilu</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="139"/>
      <source>copy the settings and some other parts of the profile to a new one that will require a different new name.</source>
      <translation>skopiuj ustawienia i inne części profilu na nowy, który będzie wymagał nowej, zmienionej nazwy.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="201"/>
      <source>Characters password, stored securely in the computer&apos;s credential manager</source>
      <translation>Hasło postaci przechowywane bezpiecznie w komputerowym menedżerze danych logowania</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="203"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation>Hasło postaci. Uwaga, hasła nie są zapisywane zaszyfrowane</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="250"/>
      <source>Click to load but not connect the selected profile.</source>
      <translation>Kliknij, aby załadować, ale nie połączyć wybranego profilu.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="251"/>
      <source>Click to load and connect the selected profile.</source>
      <translation>Kliknij, aby wczytać i połączyć wybrany profil.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="252"/>
      <source>Need to have a valid profile name, game server address and port before this button can be enabled.</source>
      <translation>Aby możliwe było włączenie tego przycisku, należy mieć poprawną nazwę profilu, adres serwera gry i port.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="254"/>
      <source>Button to select a mud game to play, double-click it to connect and start playing it.</source>
      <comment>Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option</comment>
      <translation>Przycisk wyboru gry, kliknij dwukrotnie by się połączyć i rozpocząć grę.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="517"/>
      <source>Could not create the new profile folder on your computer.</source>
      <translation>Nie można utworzyć nowego katalogu na profil postaci.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="553"/>
      <source>new profile name</source>
      <translation>nazwa nowego profilu</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="666"/>
      <source>Deleting &apos;%1&apos;</source>
      <translation>Kasowanie &apos;%1&apos;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1057"/>
      <source>Discord integration not available on this platform</source>
      <translation>Integracja z Discordem niedostępna na tej platformie</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1063"/>
      <source>Discord integration not supported by game</source>
      <translation>Integracja Discord nie jest obsługiwana przez grę</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1066"/>
      <source>Check to enable Discord integration</source>
      <translation>Zaznacz aby włączyć integrację z Discordem</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1303"/>
      <source>Reset icon</source>
      <comment>Reset the custom picture for this profile in the connection dialog and show the default one instead</comment>
      <translation>Zresetuj ikony</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1306"/>
      <source>Set custom icon</source>
      <comment>Set a custom picture to show for the profile in the connection dialog</comment>
      <translation>Ustawianie ikony niestandardowej</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1310"/>
      <source>Set custom color</source>
      <comment>Set a custom color to show for the profile in the connection dialog</comment>
      <translation>Ustaw własny kolor</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1323"/>
      <source>Select custom image for profile (should be 120x30)</source>
      <translation>Wybierz własny obraz profilu (powinien być 120x30)</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1323"/>
      <source>Images (%1)</source>
      <translation>Obrazy (%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1733"/>
      <source>Port number must be above zero and below 65535.</source>
      <translation>Numer portu musi być pomiędzy 0 a 65535.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1753"/>
      <source>Mudlet can not load support for secure connections.</source>
      <translation>Mudlet nie może załadować obsługi dla bezpiecznych połączeń.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1775"/>
      <source>Please enter the URL or IP address of the Game server.</source>
      <translation>Wprowadź adres URL lub adres IP serwera Gry.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1785"/>
      <source>SSL connections require the URL of the Game server.</source>
      <translation>Połączenia SSL wymagają adresu URL serwera Gry.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1802"/>
      <source>&lt;p&gt;Load profile without connecting.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wczytaj profil bez połączenia.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1818"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before loading.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Proszę ustawić prawidłową nazwę profilu, adres serwera gry i port gry przed załadowaniem&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1823"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before connecting.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Proszę ustawić prawidłową nazwę profilu, adres serwera gry i port gry przed połączeniem.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1876"/>
      <source>&lt;p&gt;Click to hide the password; it will also hide if another profile is selected.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby ukryć hasło; ukryje również po wybraniu innego profilu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1880"/>
      <source>&lt;p&gt;Click to reveal the password for this profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby wyświetlić hasło do tego profilu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1741"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1744"/>
      <source>Mudlet is not configured for secure connections.</source>
      <translation>Mudlet nie jest skonfigurowany do bezpiecznych połączeń.</translation>
    </message>
  </context>
  <context>
    <name>dlgIRC</name>
    <message>
      <location filename="../src/dlgIRC.cpp" line="123"/>
      <source>%1 closed their client.</source>
      <translation>%1 zamknął/ęła swojego klienta.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="135"/>
      <source>Mudlet IRC Client - %1 - %2 on %3</source>
      <translation>Mudlet klient IRC - %1 - %2 na %3</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="149"/>
      <source>$ Starting Mudlet IRC Client...</source>
      <translation>$ Uruchamianie klienta IRC Mudleta...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="150"/>
      <source>$ Host: %1:%2</source>
      <translation>$ Host: %1:%2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="151"/>
      <source>$ Nick: %1</source>
      <translation>$ Nick - Pseudonim: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="152"/>
      <source>$ Auto-Join Channels: %1</source>
      <translation>$ Auto-dołączenie kanałów: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="153"/>
      <source>$ This client supports Auto-Completion using the Tab key.</source>
      <translation>$ Ten klient obsługuje automatyczne uzupełnianie przy użyciu klawisza Tab.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="154"/>
      <source>$ Type &lt;b&gt;/help&lt;/b&gt; for commands or &lt;b&gt;/help [command]&lt;/b&gt; for command syntax.</source>
      <translation>$ Wpisz &lt;b&gt;/help&lt;/b&gt; aby uzyskać listę poleceń lub &lt;b&gt;/help [polecenie]&lt;/b&gt; aby zobaczyć pomoc dla konkretnego polecenia.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="213"/>
      <source>Restarting IRC Client</source>
      <translation>Restartowanie klienta IRC</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="388"/>
      <source>[Error] MSGLIMIT requires &lt;limit&gt; to be a whole number greater than zero!</source>
      <translation>[Error] MSGLIMIT wymaga &lt;limit&gt; aby być liczbą całkowitą większą niż zero!</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="418"/>
      <source>[HELP] Available Commands: %1</source>
      <translation>[POMOC] Dostępne polecenia: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="420"/>
      <source>[HELP] Syntax: %1</source>
      <translation>[HELP] składnia: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="428"/>
      <source>! Connected to %1.</source>
      <translation>! Podłączono do %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="429"/>
      <source>! Joining %1...</source>
      <translation>! Dołączenie %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="434"/>
      <source>! Connecting %1...</source>
      <translation>! Łączenie %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="439"/>
      <source>! Disconnected from %1.</source>
      <translation>! Odłączono od %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="505"/>
      <source>[ERROR] Syntax: %1</source>
      <translation>[BŁĄD] Składna: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="507"/>
      <source>[ERROR] Unknown command: %1</source>
      <translation>[BŁĄD] Nieznane polecenie: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="653"/>
      <source>! The Nickname %1 is reserved. Automatically changing Nickname to: %2</source>
      <translation>! Pseudonim %1 jest zarezerwowany. Automatyczna zmiana pseudonimu na: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="664"/>
      <source>Your nick has changed.</source>
      <translation>Twój pseudonim się zmienił.</translation>
    </message>
  </context>
  <context>
    <name>dlgMapper</name>
    <message>
      <location filename="../src/dlgMapper.cpp" line="347"/>
      <source>None</source>
      <comment>Don&apos;t show the map overlay, &apos;none&apos; meaning no map overlay styled are enabled</comment>
      <translation>Brak</translation>
    </message>
  </context>
  <context>
    <name>dlgModuleManager</name>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="48"/>
      <source>Module Manager - %1</source>
      <translation>Menedżer modułów - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Name</source>
      <translation>Nazwa modułu</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Priority</source>
      <translation>Priorytet</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Sync</source>
      <translation>Synchronizuj</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Location</source>
      <translation>Lokalizacja modułu</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="105"/>
      <source>Checking this box will cause the module to be saved and &lt;i&gt;resynchronised&lt;/i&gt; across all sessions that share it when the &lt;i&gt;Save Profile&lt;/i&gt; button is clicked in the Editor or if it is saved at the end of the session.</source>
      <translation>Zaznaczenie tego pola spowoduje, że moduł będzie zapisany i &lt;i&gt;synchronizowany&lt;/i&gt; we wszystkich sesjach, które go używają, gdy przycisk &lt;i&gt;Zapisz profil&lt;/i&gt; zostanie kliknięty w Edytorze lub gdy zostanie zapisany na końcu sesji.</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="111"/>
      <source>&lt;b&gt;Note:&lt;/b&gt; &lt;i&gt;.zip&lt;/i&gt; and &lt;i&gt;.mpackage&lt;/i&gt; modules are currently unable to be synced&lt;br&gt; only &lt;i&gt;.xml&lt;/i&gt; packages are able to be synchronized across profiles at the moment. </source>
      <translation>&lt;b&gt;Uwaga:&lt;/b&gt; moduły &lt;i&gt;.zip&lt;/i&gt; i &lt;i&gt;.mpackage&lt;/i&gt; obecnie nie mogą być synchronizowane&lt;br&gt; aktualnie tylko pakiety &lt;i&gt;.xml&lt;/i&gt; mogą być synchronizowane między profilami. </translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="149"/>
      <source>Load Mudlet Module</source>
      <translation>Załaduj moduł Mudleta</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Load Mudlet Module:</source>
      <translation>Załaduj moduł Mudleta:</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>Nie można odczytać pliku %1:
%2.</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageExporter</name>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="29"/>
      <source>Package name here</source>
      <translation>Podaj nazwę pakietu</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="36"/>
      <source>or</source>
      <translation>lub</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="66"/>
      <location filename="../src/dlgPackageExporter.cpp" line="1459"/>
      <source>Select what to export</source>
      <translation>Wybierz elementy do wyeksportowania</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="82"/>
      <source>Check items to export</source>
      <translation>Wybierz co zostanie wyeksportowane</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="109"/>
      <source>(optional) add icon, description, and more</source>
      <translation>(opcjonalne) dodaj ikonę, opis i więcej</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="156"/>
      <source>Author</source>
      <translation>Autor</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="175"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="268"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="305"/>
      <source>optional</source>
      <translation>opcjonalne</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="213"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="235"/>
      <source>Icon size of 512x512 recommended</source>
      <translation>Zalecana wielkość ikony to 512x512 pikseli</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="245"/>
      <source>512x512 recommended</source>
      <translation>zalecane 512x512 pikseli</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="288"/>
      <source>optional. Markdown supported, and you can add images with drag and drop</source>
      <translation>opcjonalne, można używać Markdown, można przeciągać na to pole obrazki</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="182"/>
      <source>Icon</source>
      <translation>Ikona</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="216"/>
      <source>Add icon</source>
      <translation>Dodaj ikonę</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="255"/>
      <source>Short description</source>
      <translation>Krótki opis</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="275"/>
      <source>Description</source>
      <translation>Opis</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="295"/>
      <source>Version</source>
      <translation>Wersja</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="312"/>
      <source>Does this package make use of other packages? List them here as requirements</source>
      <translation>Czy ten pakiet korzysta z innych pakietów? Wymień je tutaj jako wymagania</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="315"/>
      <source>Required packages</source>
      <translation>Wymagane pakiety</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="404"/>
      <source>Include assets (images, sounds, fonts)</source>
      <translation>Dodaj zasoby (obrazki, dźwięki, czcionki)</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="414"/>
      <source>Drag and drop files and folders, or use the browse button below</source>
      <translation>Przeciągnij pliki lub foldery, lub użyj przyciski Przeglądaj poniżej</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="462"/>
      <source>Select files to include in package</source>
      <translation>Wybierz pliki do załączenia w pakiecie</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="372"/>
      <source>Does this package make use of other packages? List them here as requirements. Press &apos;Delete&apos; to remove a package</source>
      <translation>Czy ten pakiet korzysta z innych pakietów? Wymień je tutaj jako wymagania. Naciśnij &apos;Usuń&apos;, aby usunąć pakiet</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="20"/>
      <source>Package Exporter</source>
      <translation>Eksporter pakietów</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="512"/>
      <source>Select export location</source>
      <translation>Wybierz miejsce zapisu</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="63"/>
      <source>Triggers</source>
      <translation>Wyzwalacze</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="64"/>
      <source>Aliases</source>
      <translation>Aliasy</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="65"/>
      <source>Timers</source>
      <translation>Liczniki czasu</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="66"/>
      <source>Scripts</source>
      <translation>Skrypty</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="67"/>
      <source>Keys</source>
      <translation>Klawisze</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="68"/>
      <source>Buttons</source>
      <translation>Przyciski</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="90"/>
      <source>Export</source>
      <comment>Text for button to perform the package export on the items the user has selected.</comment>
      <translation>Eksportuj</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="109"/>
      <source>update installed package</source>
      <translation>uaktualnij zainstalowane pakiety</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="110"/>
      <source>add dependencies</source>
      <translation>dodaj zależność</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="170"/>
      <source>Failed to open file &quot;%1&quot; to place into package. Error message was: &quot;%2&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but the code cannot open it.</comment>
      <translation>Nie udało się otworzyć pliku &quot;%1&quot; aby umieścić go w pakiecie. Komunikat o błędzie był: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="180"/>
      <source>Failed to add file &quot;%1&quot; to package. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but cannot be done for some reason.</comment>
      <translation>Nie udało się dodać pliku &quot;%1&quot; do pakietu. Komunikat błędu: &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="326"/>
      <location filename="../src/dlgPackageExporter.cpp" line="328"/>
      <source>Export to %1</source>
      <translation>Wyeksportuj do %1</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1031"/>
      <source>Failed to zip up the package. Error is: &quot;%1&quot;.</source>
      <comment>This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...</comment>
      <translation>Nie udało się spakować pakietu. Błąd: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="347"/>
      <source>Open Icon</source>
      <translation>Wybierz ikonę</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="347"/>
      <source>Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</source>
      <translation>Pliki obrazków (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="472"/>
      <source>Please enter the package name.</source>
      <translation>Wpisz nazwę pakietu.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="510"/>
      <location filename="../src/dlgPackageExporter.cpp" line="586"/>
      <source>Exporting package...</source>
      <translation>Eksportowanie pakietu...</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="530"/>
      <source>Failed to export. Could not open the folder &quot;%1&quot; for writing. Do you have the necessary permissions and free disk-space to write to that folder?</source>
      <translation>Eksport nie powiódł się. Nie można otworzyć folderu &quot;%1&quot; do zapisu. Czy masz niezbędne uprawnienia i wolną przestrzeń dyskową aby zapisać do tego folderu?</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="855"/>
      <source>%1 doesn&apos;t seem to exist anymore - can you double-check it?</source>
      <translation>%1 nie istnieje - upewnij się że plik istnieje zanim spróbujesz ponownie.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="785"/>
      <source>Failed to export. Could not write Mudlet items to the file &quot;%1&quot;.</source>
      <comment>This error message is shown when all the Mudlet items cannot be written to the &apos;packageName&apos;.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened.</comment>
      <translation>Nie udało się wyeksportować. Nie można zapisać elementów Mudleta do pliku &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="956"/>
      <source>Failed to add directory &quot;%1&quot; to package. Error is: &quot;%2&quot;.</source>
      <translation>Nie można dodać katalogu &quot;%1&quot; do pakietu. Błąd to: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="999"/>
      <source>Required file &quot;%1&quot; was not found in the staging area. This area contains the Mudlet items chosen for the package, which you selected to be included in the package file. This suggests there may be a problem with that directory: &quot;%2&quot; - Do you have the necessary permissions and free disk-space?</source>
      <translation>Wymagany plik &quot;%1&quot; nie znaleziono w miejscu postoju. Ten obszar zawiera elementy Mudlet wybrane dla pakietu, które zostały wybrane do włączenia do pliku pakietu. Sugeruje to, że może być problem z tym katalogiem: &quot;%2&quot; - Czy masz niezbędne uprawnienia i wolne miejsce na dysku?</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="572"/>
      <source>Package &quot;%1&quot; exported to: %2</source>
      <translation>Pakiet &quot;%1&quot; wyeksportowany do: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="883"/>
      <source>Failed to open package file. Error is: &quot;%1&quot;.</source>
      <comment>This zipError message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer&apos;s file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.</comment>
      <translation>Nie można otworzyć pliku pakietu. Błąd to: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1028"/>
      <source>Export cancelled.</source>
      <translation>Eksport anulowany.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1440"/>
      <source>Why not &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;upload&lt;/a&gt; your package for other Mudlet users?</source>
      <comment>Only the text outside of the &apos;a&apos; (HTML anchor) tags PLUS the verb &apos;upload&apos; in between them in the source text, (associated with uploading the resulting package to the Mudlet forums) should be translated.</comment>
      <translation>Dlaczego nie &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;Przesłać&lt;/a&gt; twójego pakietu dla innych użytkowników Mudlet?</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageExporter.cpp" line="1461"/>
      <source>Select what to export (%1 items)</source>
      <comment>Package exporter selection</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1089"/>
      <source>Where do you want to save the package?</source>
      <translation>Gdzie chcesz zapisać pakiet?</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageManager</name>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="50"/>
      <source>Package Manager (experimental) - %1</source>
      <translation>Menedżer pakietów (eksperymentalny) - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="104"/>
      <source>Import Mudlet Package</source>
      <translation>Importuj pakiet Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="111"/>
      <source>Import Mudlet Package:</source>
      <translation>Importuj pakiet Mudlet:</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="111"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>Nie można odczytać pliku %1:
%2.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="175"/>
      <source>Author</source>
      <translation>Autor</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="175"/>
      <source>Version</source>
      <translation>Wersja</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="175"/>
      <source>Created</source>
      <translation>Utworzono</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="175"/>
      <source>Dependencies</source>
      <translation>Zależności</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageManager.cpp" line="246"/>
      <source>Remove packages</source>
      <comment>Button in package manager to remove selected package(s)</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgProfilePreferences</name>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="142"/>
      <source>&lt;p&gt;Location which will be used to store log files - matching logs will be appended to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Lokalizacja, która będzie używana do przechowywania plików dziennika - pasujące dzienniki zostaną dołączone do.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="150"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {1 of 2}).</comment>
      <translation>plik dziennika</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="202"/>
      <source>&lt;p&gt;Select the only or the primary font used (depending on &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; setting) to produce the 2D mapper room symbols.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz jedyną lub używaną czcionkę podstawową (w zależności od &lt;i&gt;Używaj tylko symboli (glifów) z wybranej czcionki&lt;/i&gt; ), aby uzyskać symbole pokoju mapperów 2D.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="217"/>
      <source>&lt;p&gt;Some East Asian MUDs may use glyphs (characters) that Unicode classifies as being of &lt;i&gt;Ambiguous&lt;/i&gt; width when drawn in a font with a so-called &lt;i&gt;fixed&lt;/i&gt; pitch; in fact such text is &lt;i&gt;duo-spaced&lt;/i&gt; when not using a proportional font. These symbols can be drawn using either a half or the whole space of a full character. By default Mudlet tries to chose the right width automatically but you can override the setting for each profile.&lt;/p&gt;&lt;p&gt;This control has three settings:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;narrow&lt;/i&gt;&apos; = Draw ambiguous width characters in a single &apos;space&apos;.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;wide&lt;/i&gt;&apos; = Draw ambiguous width characters two &apos;spaces&apos; wide.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use &apos;wide&apos; setting for MUD Server encodings of &lt;b&gt;Big5&lt;/b&gt;, &lt;b&gt;GBK&lt;/b&gt; or &lt;b&gt;GBK18030&lt;/b&gt; and &apos;narrow&apos; for all others.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This is a temporary arrangement and will probably change when Mudlet gains full support for languages other than English.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Niektóre wschodnioazjatyckie MUD mogą używać glifów (znaków), które Unicode klasyfikuje jako znaki o &lt;i&gt;nieokreślonej szerokości&lt;/i&gt; przy wyświetlaniu tekstu czcionką o stałej szerokości. Tak naprawdę takie znaki mają &lt;i&gt;podwójną szerokość&lt;/i&gt;, gdy nie są wyświetlane czcionką o zmiennej szerokości. Symbole te można rysować za pomocą jednej lub dwóch normalnych szerokości znaku. Domyślnie Mudlet próbuje automatycznie wybrać właściwą szerokość, ale można nadpisać ustawienie dla każdego profilu.&lt;/p&gt;&lt;p&gt;Ta opcja ma trzy ustawienia:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Niezaznaczone&lt;/b&gt; &apos;&lt;i&gt;wąskie&lt;/i&gt;&apos; = Narysuj dwuznaczne znaki jedną szerokością znaku&lt;li&gt;&lt;b&gt;Zaznaczone&lt;/b&gt; &apos;&lt;i&gt;o szerokim&lt;/i&gt;&apos; = Narysuj dwuznaczne znaki dwoma szerokościami zwykłego znaku.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Częściowo sprawdzone&lt;/b&gt; &lt;i&gt;(domyślnie) &apos;auto&apos;&lt;/i&gt; = Użyj ustawienia &apos;szeroki&apos; dla kodowania serwera MUD &lt;b&gt;Big5&lt;/b&gt; &lt;b&gt;GBK&lt;/b&gt; lub &lt;b&gt;GBK18030&lt;/b&gt; i &apos;wąski&apos; dla wszystkich innych.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;Jest to rozwiązanie tymczasowe i prawdopodobnie zmieni się, gdy Mudlet uzyska pełne wsparcie dla języków innych niż angielski.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="298"/>
      <source>%1 (%2% done)</source>
      <comment>%1 is the (not-translated so users of the language can read it!) language name, %2 is percentage done.</comment>
      <translation>%1 (%2% zrobiono)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="351"/>
      <source>Migrated all passwords to secure storage.</source>
      <translation>Przeniesiono wszystkie hasła do zabezpieczonej pamięci.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="358"/>
      <source>Migrated %1...</source>
      <comment>This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely</comment>
      <translation>Przeniesiono %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="362"/>
      <source>Migrated all passwords to profile storage.</source>
      <translation>Przeniesiono wszystkie hasła do pamięci profilowej.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="750"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (np. 1970-01-01#00-00-00%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="752"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (np. 1970-01-01T00-00-00%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="753"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)</source>
      <translation>yyyy-MM-dd (połącz dzienne logi, np. 1970-01-01%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="756"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01%1)</source>
      <translation>yyyy-MM (połącz miesięczne logi, np. 1970-01%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="757"/>
      <source>Named file (concatenate logs in one file)</source>
      <translation>Nazwany plik (dołączaj wszystkie logi do jednego pliku)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="790"/>
      <source>Other profiles to Map to:</source>
      <translation>Inne profile do Mapy:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="830"/>
      <source>%1 {Default, recommended}</source>
      <translation>%1 {Default, recommended}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="841"/>
      <source>%1 {Upgraded, experimental/testing, NOT recommended}</source>
      <translation>%1 {Uaktualniony, eksperymentalny/testowy, NIE zalecany}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="843"/>
      <source>%1 {Downgraded, for sharing with older version users, NOT recommended}</source>
      <translation>%1 {starty poziom, dla użytkowników starej wersji, NIE zalecany}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="852"/>
      <source>2D Map Room Symbol scaling factor:</source>
      <translation>Współczynnik skalowania symbolu pokoju 2D:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="872"/>
      <source>Show &quot;%1&quot; in the map area selection</source>
      <translation>Pokaż &quot;%1&quot; w wyborze obszaru mapy</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="910"/>
      <source>%1 (*Error, report to Mudlet Makers*)</source>
      <comment>The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!</comment>
      <translation>%1 (*Błąd, zgłoś do twórców Mudletu*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1025"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="3799"/>
      <source>Profile preferences - %1</source>
      <translation>Preferencje profilu - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1302"/>
      <source>Profile preferences</source>
      <translation>Preferencje profilu</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2020"/>
      <source>Load Mudlet map</source>
      <translation>Załaduj mapę Mudletową</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2035"/>
      <source>Importing map - please wait...</source>
      <translation>Importowanie mapy - proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2039"/>
      <source>Imported map from %1.</source>
      <translation>Mapa zaimportowana z %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2041"/>
      <source>Could not import map from %1.</source>
      <translation>Importowanie mapy z %1 się nie powiodło.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2044"/>
      <source>Loading map - please wait...</source>
      <translation>Ładowanie mapy - proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2049"/>
      <source>Loaded map from %1.</source>
      <translation>Mapa załadowana z %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2051"/>
      <source>Could not load map from %1.</source>
      <translation>Ładowanie mapy z %1 się nie powiodło.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2068"/>
      <source>Save Mudlet map</source>
      <translation>Zapisz Mudletową mapę</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2068"/>
      <source>Mudlet map (*.dat)</source>
      <comment>Do not change the extension text (in braces) - it is needed programmatically!</comment>
      <translation>Mudletowa mapa (*.dat)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2078"/>
      <source>Saving map - please wait...</source>
      <translation>Zapisywanie mapy - proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2088"/>
      <source>Saved map to %1.</source>
      <translation>Mapa zapisana do %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2090"/>
      <source>Could not save map to %1.</source>
      <translation>Zapisanie mapy do %1 się nie powiodło.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2118"/>
      <source>Migrating passwords to secure storage...</source>
      <translation>Przenoszenie haseł do bezpiecznego przechowywania...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2125"/>
      <source>Migrating passwords to profiles...</source>
      <translation>Trwa migrowanie haseł do profili...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2157"/>
      <source>[ ERROR ] - Unable to use or create directory to store map for other profile &quot;%1&quot;.
Please check that you have permissions/access to:
&quot;%2&quot;
and there is enough space. The copying operation has failed.</source>
      <translation>[ BŁĄD ] - Nie można użyć lub utworzyć katalogu do przechowywania mapy dla innego profilu &quot;%1&quot;.
Sprawdź, czy masz uprawnienia/dostęp do:
&quot;%2&quot;
i jest wystarczająco dużo miejsca. Operacja kopiowania nie powiodła się.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2164"/>
      <source>Creating a destination directory failed...</source>
      <translation>Tworzenie katalogu docelowego nie powiodło się...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2248"/>
      <source>Backing up current map - please wait...</source>
      <translation>Tworzenie kopii zapasowej aktualnej mapy - proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2258"/>
      <source>Could not backup the map - saving it failed.</source>
      <translation>Nie udało się zrobić kopii zapasowej mapy - zapisanie jej nie powiodło się.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2283"/>
      <source>Could not copy the map - failed to work out which map file we just saved the map as!</source>
      <translation>Nie można skopiować mapy - nie udało się sprawdzić, który plik mapy właśnie zapisaliśmy jako mapę!</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2295"/>
      <source>Copying over map to %1 - please wait...</source>
      <translation>Kopiowanie mapy do %1 - proszę czekać...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2301"/>
      <source>Could not copy the map to %1 - unable to copy the new map file over.</source>
      <translation>Nie można skopiować mapy do %1 - nie można skopiować nowego pliku mapy.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2305"/>
      <source>Map copied successfully to other profile %1.</source>
      <translation>Mapa skopiowana pomyślnie do innego profilu %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2317"/>
      <source>Map copied, now signalling other profiles to reload it.</source>
      <translation>Mapa skopiowana, teraz daj znać innym profilom, aby ją przeładować.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2353"/>
      <source>Where should Mudlet save log files?</source>
      <translation>Gdzie Mudlet powinien zapisywać pliki dziennika?</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2745"/>
      <source>%1 selected - press to change</source>
      <translation>%1 zaznaczone - naciśnij, aby zmienić</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2748"/>
      <source>Press to pick destination(s)</source>
      <translation>Naciśnij by wybrać miejsce docelowe</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2989"/>
      <source>Could not update themes: %1</source>
      <translation>Nie można zaktualizować motywów: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2992"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>Aktualizowanie skórek z colorsublime.github.io...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3109"/>
      <source>{missing, possibly recently deleted trigger item}</source>
      <translation>{missing, possibly recently deleted trigger item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3112"/>
      <source>{missing, possibly recently deleted alias item}</source>
      <translation>{missing, possibly recently deleted alias item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3115"/>
      <source>{missing, possibly recently deleted script item}</source>
      <translation>{missing, possibly recently deleted script item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3118"/>
      <source>{missing, possibly recently deleted timer item}</source>
      <translation>{missing, possibly recently deleted timer item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3121"/>
      <source>{missing, possibly recently deleted key item}</source>
      <translation>{missing, possibly recently deleted key item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3124"/>
      <source>{missing, possibly recently deleted button item}</source>
      <translation>{missing, possibly recently deleted button item}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3249"/>
      <source>&lt;p&gt;The room symbol will appear like this if only symbols (glyphs) from the specific font are used.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol pomieszczenia będzie wyglądał w ten sposób, jeśli używane są tylko symbole (glify) z wybranej czcionki.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3931"/>
      <source>Set outer color of player room mark.</source>
      <translation>Ustaw zewnętrzny kolor znaku pokoju gracza.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3932"/>
      <source>Set inner color of player room mark.</source>
      <translation>Ustaw wewnętrzny kolor znaku pokoju gracza.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="204"/>
      <source>&lt;p&gt;Using a single font is likely to produce a more consistent style but may cause the &lt;i&gt;font replacement character&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; to show if the font does not have a needed glyph (a font&apos;s individual character/symbol) to represent the grapheme (what is to be represented).  Clearing this checkbox will allow the best alternative glyph from another font to be used to draw that grapheme.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Użycie jednej czcionki pozwala uzyskać bardziej spójny styl mapy, ale może spowodować, że &lt;i&gt;znak zastępczy&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; zostanie wyświetlony, jeśli czcionka nie ma potrzebnego glifu (pojedynczego znaku/symbolu czcionki) do narysowania grafemu który chcemy wyświetlić. Wyczyszczenie tego pola wyboru pozwoli na użycie najlepszego alternatywnego glifu z innej czcionki do narysowania tego grafemu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="143"/>
      <source>&lt;p&gt;Select a directory where logs will be saved.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz katalog, w którym będą zapisywane dzienniki.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="144"/>
      <source>&lt;p&gt;Reset the directory so that logs are saved to the profile&apos;s &lt;i&gt;log&lt;/i&gt; directory.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Resetuj katalog tak, aby logi były zapisywane w katalogu profilu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="145"/>
      <source>&lt;p&gt;This option sets the format of the log name.&lt;/p&gt;&lt;p&gt;If &lt;i&gt;Named file&lt;/i&gt; is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ta opcja ustawia format nazwy dzienników - logów.&lt;/p&gt;&lt;p&gt;Jeśli &lt;i&gt;Nazwany plik&lt;/i&gt; zostanie wybrany, możesz ustawić niestandardową nazwę. (Dzienniki są dołączane, jeśli plik dziennika o tej samej nazwie już istnieje).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="148"/>
      <source>&lt;p&gt;Set a custom name for your log. (New logs are appended if a log file of the same name already exists).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ustaw niestandardową nazwę dla swojego logu. (Nowe logi są dołączone, jeśli plik dziennika o tej samej nazwie już istnieje.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="169"/>
      <source>&lt;p&gt;Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Automatyczne aktualizacje są wyłączone w wersjach programistycznych, aby zapobiec nadpisaniu aktualizacji twojego Mudleta&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="190"/>
      <source>&lt;p&gt;A timer with a short interval will quickly fill up the &lt;i&gt;Central Debug Console&lt;/i&gt; windows with messages that it ran correctly on &lt;i&gt;each&lt;/i&gt; occasion it is called.  This (per profile) control adjusts a threshold that will hide those messages in just that window for those timers which run &lt;b&gt;correctly&lt;/b&gt; when the timer&apos;s interval is less than this setting.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Any timer script that has errors will still have its error messages reported whatever the setting.&lt;/u&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Timer z krótkim odstępem czasu szybko wypełni &lt;i&gt;Centralną konsolę debugowania&lt;/i&gt; okno z komunikatami, które działały poprawnie na &lt;i&gt;każdą&lt;/i&gt; okazję wywołania. Ten (na profil) formant dostosowuje próg, który ukryje te wiadomości w tym oknie dla tych timerów, które działają &lt;b&gt;Poprawnie&lt;/b&gt; gdy zegar jest mniejszy niż to ustawienie.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Każdy skrypt zegara, który ma błędy, nadal będzie miał swoje komunikaty o błędach zgłoszone niezależnie od tego ustawienia.&lt;/u&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="196"/>
      <source>&lt;p&gt;This will bring up a display showing all the symbols used in the current map and whether they can be drawn using just the specified font, any other font, or not at all.  It also shows the sequence of Unicode &lt;i&gt;code-points&lt;/i&gt; that make up that symbol, so that they can be identified even if they cannot be displayed; also, up to the first thirty two rooms that are using that symbol are listed, which may help to identify any unexpected or odd cases.&lt;p&gt;</source>
      <translation>&lt;p&gt;Spowoduje to wyświetlenie wyświetlania wszystkich symboli używanych na bieżącej mapie i tego, czy można je narysować przy użyciu tylko wybranej czcionki, dowolnej innej czcionki, czy w ogóle. Pokazuje również sekwencję Unicode które tworzy ten symbol, tak aby można ją było zidentyfikować, nawet jeśli nie może ona być wyświetlona. Wyświetla również do pierwszych trzydziestu dwóch pomieszczeń, które używają tego symbolu, co może pomóc w odnalezieniu wszelkich nieoczekiwanych lub dziwnych użyć.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="209"/>
      <source>&lt;p&gt;If &lt;b&gt;not&lt;/b&gt; checked Mudlet will only react to the first matching keybinding (combination of key and modifiers) even if more than one of them is set to be active. This means that a temporary keybinding (not visible in the Editor) created by a script or package may be used in preference to a permanent one that is shown and is set to be active. If checked then all matching keybindings will be run.&lt;/p&gt;&lt;p&gt;&lt;i&gt;It is recommended to not enable this option if you need to maintain compatibility with scripts or packages for Mudlet versions prior to &lt;b&gt;3.9.0&lt;/b&gt;.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jeśli &lt;b&gt;nie&lt;/b&gt; zaznaczone Mudlet zareaguje tylko na pierwsze pasujące skróty klawiszowe (kombinacja klawiszy i modyfikatorów), nawet jeśli więcej niż jeden z nich jest aktywny. Oznacza to, że tymczasowe powiązanie kluczy (niewidoczne w Edytorze) utworzone przez skrypt lub pakiet może być używane zamiast stałego, który jest wyświetlany i ustawiony jako aktywny. Jeśli zaznaczone, wszystkie pasujące klawisze będą uruchomione.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Zaleca się, aby nie włączać tej opcji, jeśli musisz zachować kompatybilność ze skryptami lub pakietami dla wersji Mudlet przed &lt;b&gt;3.0&lt;/b&gt;.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="229"/>
      <source>&lt;p&gt;Enable a context (right click) menu action on any console/user window that, when the mouse cursor is hovered over it, will display the UTF-16 and UTF-8 items that make up each Unicode codepoint on the &lt;b&gt;first&lt;/b&gt; line of any selection.&lt;/p&gt;&lt;p&gt;This utility feature is intended to help the user identify any grapheme (visual equivalent to a &lt;i&gt;character&lt;/i&gt;) that a Game server may send even if it is composed of multiple bytes as any non-ASCII character will be in the Lua sub-system which uses the UTF-8 encoding system.&lt;p&gt;</source>
      <translation>&lt;p&gt;Włączenie działania menu kontekstowego (po kliknięciu prawym przyciskiem myszy) w dowolnym oknie konsoli/użytkownika, które po umieszczaniu kursora myszy nad nim spowoduje wyświetlenie pozycji UTF-16 i UTF-8, które tworzą każdy punkt kodowy Unicode na &lt;b&gt;pierwszej linii&lt;/b&gt; dowolnego wyboru.&lt;/p&gt;&lt;p&gt;Ten program narzędziowy jest przeznaczony do pomocy użytkownikowi w identyfikacji dowolnego grapheme (wizualnego odpowiednik &lt;i&gt;znak&lt;/i&gt;) że serwer gry może wysłać nawet wtedy, gdy składa się on z wielu bajtów, ponieważ każdy znak inny niż ASCII będzie w podsystemie Lua, który używa systemu kodowania UTF-8.&lt;p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="236"/>
      <source>&lt;p&gt;Some Desktop Environments tell Qt applications like Mudlet whether they should shown icons on menus, others, however do not. This control allows the user to override the setting, if needed, as follows:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Unchecked&lt;/b&gt; &apos;&lt;i&gt;off&lt;/i&gt;&apos; = Prevent menus from being drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Checked&lt;/b&gt; &apos;&lt;i&gt;on&lt;/i&gt;&apos; = Allow menus to be drawn with icons.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Partly checked&lt;/b&gt; &lt;i&gt;(Default) &apos;auto&apos;&lt;/i&gt; = Use the setting that the system provides.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;This setting is only processed when individual menus are created and changes may not propagate everywhere until Mudlet is restarted.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Niektóre interfejsy graficzne systemu informują aplikacje Qt, takie jak Mudlet, czy powinny być wyświetlane ikony w menu, inne jednak nie. Ta kontrolka umożliwia użytkownikowi nadpisanie tego ustawienia, w razie potrzeby, w następujący sposób:&lt;ul&gt;&lt;li&gt;&lt;b&gt;Niezaznaczone&lt;/b&gt; &apos;&lt;i&gt;wył.&lt;/i&gt;&apos; = Zapobiegaj rysowaniu menu z ikonami.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Zaznaczone&lt;/b&gt; &apos;&lt;i&gt;wł.&lt;/i&gt;&apos; = Zezwalaj na rysowanie menu z ikonami.&lt;/li&gt;&lt;li&gt;&lt;b&gt;Częściowo sprawdzone&lt;/b&gt; &lt;i&gt;(Domyślnie) &apos;Automatycznie&apos;&lt;/i&gt; = Użyj ustawienia, które wskazuje system.&lt;/li&gt;&lt;/ul&gt;&lt;/p&gt;&lt;p&gt;&lt;i&gt;To ustawienie jest przetwarzane tylko wtedy, gdy tworzone są poszczególne menu, a zmiany mogą nie pojawić się wszędzie, dopóki mudlet nie zostanie ponownie uruchomiony.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="583"/>
      <source>Mudlet dictionaries:</source>
      <comment>On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones.</comment>
      <translation>Słowniki Mudlet:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="585"/>
      <source>System dictionaries:</source>
      <comment>On *nix systems where we find the system ones we use them.</comment>
      <translation>Słowniki systemowe:</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="616"/>
      <source>&lt;p&gt;From the dictionary file &lt;tt&gt;%1.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Z pliku słownika &lt;tt&gt;%1.dic&lt;/tt&gt; (i jego towarzysz afiks &lt;tt&gt;.aff&lt;/tt&gt; pliku).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="618"/>
      <source>%1 - not recognised</source>
      <translation>%1 - nie rozpoznano</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="619"/>
      <source>&lt;p&gt;Mudlet does not recognise the code &quot;%1&quot;, please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!&lt;/p&gt;&lt;p&gt;The file &lt;tt&gt;%2.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file) is still usable.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet nie rozpoznaje kodu &quot;%1&quot;, zgłoś to do deweloperów Mudlet, abyśmy mogli je prawidłowo opisać w kolejnych wersjach Mudleta!&lt;/p&gt;&lt;p&gt;Plik &lt;tt&gt;%2. plik i&lt;/tt&gt; (i jego towarzyszący dodatek &lt;tt&gt;.aff&lt;/tt&gt; ) jest nadal użyteczny.&lt;/p&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="644"/>
      <source>No Hunspell dictionary files found, spell-checking will not be available.</source>
      <translation>Nie znaleziono plików słownika Hunspell, sprawdzanie pisowni nie będzie dostępne.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2022"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>Mapa Mudlet'a (*.dat);;Dane mapy w formacie xml (*.xml);;Dowolny plik (*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3255"/>
      <source>&lt;p&gt;The room symbol will appear like this if symbols (glyphs) from any font can be used.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol pomieszczenia będzie wyglądał w ten sposób, jeśli można użyć symboli (glifów) z dowolnej czcionki.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3284"/>
      <source>&lt;p&gt;These are the sequence of hexadecimal numbers that are used by the Unicode consortium to identify the graphemes needed to create the symbol.  These numbers can be utilised to determine precisely what is to be drawn even if some fonts have glyphs that are the same for different codepoints or combination of codepoints.&lt;/p&gt;&lt;p&gt;Character entry utilities such as &lt;i&gt;charmap.exe&lt;/i&gt; on &lt;i&gt;Windows&lt;/i&gt; or &lt;i&gt;gucharmap&lt;/i&gt; on many Unix type operating systems will also use these numbers which cover everything from U+0020 {Space} to U+10FFFD the last usable number in the &lt;i&gt;Private Use Plane 16&lt;/i&gt; via most of the written marks that humanity has ever made.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Są to sekwencja numerów szesnastowych, które są używane przez konsorcjum Unicode do identyfikacji grafemów potrzebnych do utworzenia symbolu.  Liczby te mogą być wykorzystane do dokładnego określenia, co ma być rysowane, nawet jeśli niektóre czcionki mają glify, które są takie same dla różnych punktów kodowych lub kombinacji punktów kodowych.&lt;/p&gt;&lt;p&gt;Narzędzia wprowadzania znaków, takie jak &lt;i&gt;charmap.exe&lt;/i&gt; Na &lt;i&gt;Windows&lt;/i&gt; Lub &lt;i&gt;mapa gucharmap&lt;/i&gt; w wielu systemach operacyjnych typu Unix będą również korzystać z tych numerów, które obejmują wszystko od U+0020 {Space} do U+10FFFD ostatni numer użytkowy w &lt;i&gt;Planowane do użytku prywatnego 16&lt;/i&gt; przez większość pisemnych znaków, które ludzkość kiedykolwiek stworzyła.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3297"/>
      <source>&lt;p&gt;How many rooms in the whole map have this symbol.</source>
      <translation>&lt;p&gt;Jak wiele pokoi w całej mapie ma ten symbol.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3309"/>
      <source>more - not shown...</source>
      <translation>więcej - nie pokazano...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3316"/>
      <source>&lt;p&gt;The rooms with this symbol, up to a maximum of thirty-two, if there are more than this, it is indicated but they are not shown.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Pokoje z tym symbolem, maksymalnie do trzydziestu dwóch, jeśli jest ich więcej, jest to wskazane, ale nie są one wyświetlane.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3325"/>
      <source>&lt;p&gt;The symbol can be made entirely from glyphs in the specified font.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol może być wykonany w całości z glifów w określonej czcionce.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3335"/>
      <source>&lt;p&gt;The symbol cannot be made entirely from glyphs in the specified font, but, using other fonts in the system, it can. Either un-check the &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; option or try and choose another font that does have the needed glyphs.&lt;/p&gt;&lt;p&gt;&lt;i&gt;You need not close this table to try another font, changing it on the main preferences dialogue will update this table after a slight delay.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol nie może być wykonany w całości z glifów w określonej czcionce, ale przy użyciu innych czcionek w systemie, może. Albo odznacz kontrolkę &lt;i&gt;Używaj tylko symboli (glifów) z wybranej czcionki&lt;/i&gt; lub spróbuj wybrać inną czcionkę, która ma potrzebne glify.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Nie musisz zamykać tej tabeli, aby wypróbować inną czcionkę, zmieniając ją w głównym oknie preferencji, zaktualizuje tę tabelę po niewielkim opóźnieniu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3345"/>
      <source>&lt;p&gt;The symbol cannot be drawn using any of the fonts in the system, either an invalid string was entered as the symbol for the indicated rooms or the map was created on a different systems with a different set of fonts available to use. You may be able to correct this by installing an additional font using whatever method is appropriate for this system or by editing the map to use a different symbol. It may be possible to do the latter via a lua script using the &lt;i&gt;getRoomChar&lt;/i&gt; and &lt;i&gt;setRoomChar&lt;/i&gt; functions.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol nie może być rysowany przy użyciu żadnej z czcionek w systemie, jako symbol wskazanych pomieszczeń wprowadzono nieprawidłowy ciąg znaków lub mapa została utworzona w różnych systemach z innym zestawem czcionek dostępnych do użycia. Można to poprawić, instalując dodatkową czcionkę przy użyciu dowolnej metody odpowiedniej dla tego systemu lub edytując mapę w celu użycia innego symbolu. Może to być możliwe za pomocą skryptu lua za pomocą &lt;i&gt;getRoomChar&lt;/i&gt; &lt;i&gt;setRoomChar&lt;/i&gt; Funkcje.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3434"/>
      <source>Large icon</source>
      <comment>Discord Rich Presence large icon</comment>
      <translation>Duża ikona</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3435"/>
      <source>Detail</source>
      <comment>Discord Rich Presence detail</comment>
      <translation>Szczegóły</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3437"/>
      <source>Small icon</source>
      <comment>Discord Rich Presence small icon</comment>
      <translation>Mała ikona</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3438"/>
      <source>State</source>
      <comment>Discord Rich Presence state</comment>
      <translation>Stan</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3440"/>
      <source>Party size</source>
      <comment>Discord Rich Presence party size</comment>
      <translation>Rozmiar grupy</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3441"/>
      <source>Party max</source>
      <comment>Discord Rich Presence maximum party size</comment>
      <translation>Maksymalna wielkość grupy</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3442"/>
      <source>Time</source>
      <comment>Discord Rich Presence time until or time elapsed</comment>
      <translation>Czas</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3477"/>
      <source>Map symbol usage - %1</source>
      <translation>Użycie symbolu mapy - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3554"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (np. 1970-01-01#00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3555"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (np. 1970-01-01T00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3556"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)</source>
      <translation>yyyy-MM-dd (łączenie codziennych logowań, np. 1970-01-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3557"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.html)</source>
      <translation>yyyy-MM (łączenie miesięcznych logowań się, np. 1970-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3560"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (np. 1970-01-01#00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3561"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (np. 1970-01-01T00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3562"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)</source>
      <translation>yyyy-MM-dd (łączenie codziennych logowań, np. 1970-01-01.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3563"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)</source>
      <translation>yyyy-MM (łącz miesiąc loguje się, np. 1970-01.txt)</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomExits</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="188"/>
      <location filename="../src/dlgRoomExits.cpp" line="193"/>
      <location filename="../src/dlgRoomExits.cpp" line="811"/>
      <location filename="../src/dlgRoomExits.cpp" line="818"/>
      <location filename="../src/dlgRoomExits.cpp" line="827"/>
      <location filename="../src/dlgRoomExits.cpp" line="834"/>
      <location filename="../src/dlgRoomExits.cpp" line="1297"/>
      <location filename="../src/dlgRoomExits.cpp" line="1302"/>
      <source>&lt;b&gt;Room&lt;/b&gt; Weight of destination: %1.</source>
      <comment>Bold HTML tags are used to emphasis that the value is destination room&apos;s weight whether overridden by a non-zero exit weight here or not.</comment>
      <translation>&lt;b&gt;Pokój&lt;/b&gt; Waga miejsca przeznaczenia: %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="192"/>
      <location filename="../src/dlgRoomExits.cpp" line="1301"/>
      <source>Exit to unnamed room is valid</source>
      <translation>Wyjście do nienazwanego pokoju jest poprawne</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="199"/>
      <source>Entered number is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation>Wprowadzony numer jest nieprawidłowy, ustaw numer pokoju, do których prowadzi to specjalne wyjście, zmieni kolor na niebieski dla prawidłowego numeru; jeśli pozostanie tak, to wyjście zostanie usunięte, gdy &amp;lt;i&amp;gt;zapisz&amp;lt;/i&amp;gt; zostanie kliknięty.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="261"/>
      <source>Set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation>Ustaw numer pomieszczenia, do których prowadzi to specjalne wyjście, zmieni kolor na niebieski dla prawidłowego numeru; jeśli pozostanie tak, to wyjście zostanie usunięte, gdy &amp;lt;i&amp;gt;zapisz&amp;lt;/i&amp;gt; zostanie kliknięty.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="264"/>
      <location filename="../src/dlgRoomExits.cpp" line="1320"/>
      <source>Prevent a route being created via this exit, equivalent to an infinite exit weight.</source>
      <translation>Zapobiec tworzonej trasie za pośrednictwem tego wyjścia, co odpowiada nieskończonej masie wyjściowej.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="268"/>
      <location filename="../src/dlgRoomExits.cpp" line="1329"/>
      <source>Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.</source>
      <translation>Ustaw dodatnią wartość, aby nadpisać domyślną (Room) wagę dla tej trasy wyjścia, wartość zerowa przypisuje domyślną.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="270"/>
      <location filename="../src/dlgRoomExits.cpp" line="1336"/>
      <source>No door symbol is drawn on 2D Map for this exit (only functional choice currently).</source>
      <translation>Na mapie 2D nie jest rysowany symbol drzwi dla tego wyjścia (obecnie tylko wybór funkcjonalny).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="272"/>
      <location filename="../src/dlgRoomExits.cpp" line="1340"/>
      <source>Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Zielony (otwarty) symbol drzwi zostanie narysowany na niestandardowej linii wyjściowej dla tego wyjścia na mapie 2D (ale nie obecnie).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="274"/>
      <location filename="../src/dlgRoomExits.cpp" line="1345"/>
      <source>Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Pomarańczowy (zamknięty) symbol drzwi zostanie narysowany na niestandardowej linii wyjściowej dla tego wyjścia na mapie 2D (ale nie obecnie).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="276"/>
      <location filename="../src/dlgRoomExits.cpp" line="1348"/>
      <source>Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation>Czerwony (zablokowany) symbol drzwi zostanie narysowany na niestandardowej linii wyjściowej dla tego wyjścia na mapie 2D (ale nie obecnie).</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="809"/>
      <source>Exit to &quot;%1&quot; in area: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="825"/>
      <source>Exit to unnamed room in area: &quot;%1&quot;, is valid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="833"/>
      <source>Exit to unnamed room is valid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="926"/>
      <source>Entered number is invalid, set the number of the room northwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="927"/>
      <location filename="../src/dlgRoomExits.cpp" line="1035"/>
      <location filename="../src/dlgRoomExits.cpp" line="1256"/>
      <source>Set the number of the room northwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="935"/>
      <source>Entered number is invalid, set the number of the room north of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="936"/>
      <location filename="../src/dlgRoomExits.cpp" line="1043"/>
      <location filename="../src/dlgRoomExits.cpp" line="1258"/>
      <source>Set the number of the room north of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="944"/>
      <source>Entered number is invalid, set the number of the room northeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="945"/>
      <location filename="../src/dlgRoomExits.cpp" line="1051"/>
      <location filename="../src/dlgRoomExits.cpp" line="1260"/>
      <source>Set the number of the room northeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="953"/>
      <source>Entered number is invalid, set the number of the room up from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="954"/>
      <location filename="../src/dlgRoomExits.cpp" line="1059"/>
      <location filename="../src/dlgRoomExits.cpp" line="1262"/>
      <source>Set the number of the room up from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="962"/>
      <source>Entered number is invalid, set the number of the room west of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="963"/>
      <location filename="../src/dlgRoomExits.cpp" line="1067"/>
      <location filename="../src/dlgRoomExits.cpp" line="1264"/>
      <source>Set the number of the room west of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="971"/>
      <source>Entered number is invalid, set the number of the room east of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="972"/>
      <location filename="../src/dlgRoomExits.cpp" line="1075"/>
      <location filename="../src/dlgRoomExits.cpp" line="1266"/>
      <source>Set the number of the room east of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="980"/>
      <source>Entered number is invalid, set the number of the room down from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="981"/>
      <location filename="../src/dlgRoomExits.cpp" line="1083"/>
      <location filename="../src/dlgRoomExits.cpp" line="1268"/>
      <source>Set the number of the room down from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="989"/>
      <source>Entered number is invalid, set the number of the room southwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="990"/>
      <location filename="../src/dlgRoomExits.cpp" line="1091"/>
      <location filename="../src/dlgRoomExits.cpp" line="1270"/>
      <source>Set the number of the room southwest of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="998"/>
      <source>Entered number is invalid, set the number of the room south of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="999"/>
      <location filename="../src/dlgRoomExits.cpp" line="1099"/>
      <location filename="../src/dlgRoomExits.cpp" line="1272"/>
      <source>Set the number of the room south of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1007"/>
      <source>Entered number is invalid, set the number of the room southeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1008"/>
      <location filename="../src/dlgRoomExits.cpp" line="1107"/>
      <location filename="../src/dlgRoomExits.cpp" line="1274"/>
      <source>Set the number of the room southeast of this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1016"/>
      <source>Entered number is invalid, set the number of the room in from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1017"/>
      <location filename="../src/dlgRoomExits.cpp" line="1115"/>
      <location filename="../src/dlgRoomExits.cpp" line="1276"/>
      <source>Set the number of the room in from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1025"/>
      <source>Entered number is invalid, set the number of the room out from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1026"/>
      <location filename="../src/dlgRoomExits.cpp" line="1123"/>
      <location filename="../src/dlgRoomExits.cpp" line="1278"/>
      <source>Set the number of the room out from this one.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="135"/>
      <location filename="../src/dlgRoomExits.cpp" line="259"/>
      <source>(room ID)</source>
      <comment>Placeholder, if no room ID is set for an exit, yet. This string is used in 2 places, ensure they match!</comment>
      <translation>(ID pokoju)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="175"/>
      <location filename="../src/dlgRoomExits.cpp" line="277"/>
      <location filename="../src/dlgRoomExits.cpp" line="312"/>
      <location filename="../src/dlgRoomExits.cpp" line="1701"/>
      <location filename="../src/dlgRoomExits.cpp" line="1723"/>
      <source>(command or Lua script)</source>
      <comment>Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same</comment>
      <translation>(polecenie lub skrypt Lua)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="187"/>
      <location filename="../src/dlgRoomExits.cpp" line="816"/>
      <location filename="../src/dlgRoomExits.cpp" line="1296"/>
      <source>Exit to &quot;%1&quot;.</source>
      <translation>Wyjście do &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="898"/>
      <location filename="../src/dlgRoomExits.cpp" line="1214"/>
      <source>Clear the stub exit for this exit to enter an exit roomID.</source>
      <translation>Wyczyść wyjście z listy dla tego wyjścia, aby wejść do Id opuszczania pokoju.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1246"/>
      <source>Exits for room: &quot;%1&quot; [*]</source>
      <translation>Wyjścia dla pokoju: &quot;%1&quot;[*]</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1248"/>
      <source>Exits for room Id: %1 [*]</source>
      <translation>Wyjścia dla pokoju Id: %1 [*]</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1308"/>
      <source>Room Id is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number.</source>
      <translation>Id pokoju jest nieprawidłowy, ustaw liczbę pokoju, do którego prowadzi to specjalne wyjście, zmieni kolor na kolor niebieski dla prawidłowej liczby.</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomSymbol</name>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="77"/>
      <source>The only used symbol is &quot;%1&quot; in one or
more of the selected %n room(s), delete this to
clear it from all selected rooms or replace
with a new symbol to use for all the rooms:</source>
      <comment>This is for when applying a new room symbol to one or more rooms and some have the SAME symbol (others may have none) at present, %n is the total number of rooms involved and is at least two. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>
        <numerusform>Jedynym używanym symbolem jest &quot;%1&quot; w jednym lub
więcej wybranych pokoi %n, usuń to aby wyczyść go ze wszystkich wybranych pomieszczeń lub zastąp nowym symbolem, który ma być używany do wszystkich pomieszczeń:</numerusform>
        <numerusform>Jedynym używanym symbolem jest &quot;%1&quot; w jednym lub
więcej wybranych pokoi %n, usuń to aby wyczyść go ze wszystkich wybranych pomieszczeń lub zastąp nowym symbolem, który ma być używany do wszystkich pomieszczeń:</numerusform>
        <numerusform>Jedynym używanym symbolem jest &quot;%1&quot; w jednym lub
więcej wybranych pokoi %n, usuń to aby wyczyść go ze wszystkich wybranych pomieszczeń lub zastąp nowym symbolem, który ma być używany do wszystkich pomieszczeń:</numerusform>
        <numerusform>Jedynym używanym symbolem jest &quot;%1&quot; w jednym lub
więcej wybranych pokoi %n, usuń to aby wyczyść go ze wszystkich wybranych pomieszczeń lub zastąp nowym symbolem, który ma być używany do wszystkich pomieszczeń:</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="88"/>
      <source>The symbol is &quot;%1&quot; in the selected room,
delete this to clear the symbol or replace
it with a new symbol for this room:</source>
      <comment>This is for when applying a new room symbol to one room. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>Symbolem jest &quot;%1&quot; w wybranym pokoju,
usuń to, aby skasować symbol lub zamienić
to z nowym symbolem dla tego pokoju:</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="97"/>
      <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
      <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
      <translation>
        <numerusform>Wybierz:
 • istniejący symbol z poniższej listy (posortowany według najczęściej używanych)
 • wprowadź jeden lub więcej znaków (&quot;widoczne znaki&quot;) jako nowy symbol
 • wprowadź spację, aby wyczyścić istniejące symbole
dla wszystkich %n wybranych pokoi:</numerusform>
        <numerusform>Wybierz:
 • istniejący symbol z poniższej listy (posortowany według najczęściej używanych)
 • wprowadź jeden lub więcej znaków (&quot;widoczne znaki&quot;) jako nowy symbol
 • wprowadź spację, aby wyczyścić istniejące symbole
dla wszystkich %n wybranych pokoi:</numerusform>
        <numerusform>Wybierz:
 • istniejący symbol z poniższej listy (posortowany według najczęściej używanych)
 • wprowadź jeden lub więcej znaków (&quot;widoczne znaki&quot;) jako nowy symbol
 • wprowadź spację, aby wyczyścić istniejące symbole
dla wszystkich %n wybranych pokoi:</numerusform>
        <numerusform>Wybierz:
 • istniejący symbol z poniższej listy (posortowany według najczęściej używanych)
 • wprowadź jeden lub więcej znaków (&quot;widoczne znaki&quot;) jako nowy symbol
 • wprowadź spację, aby wyczyścić istniejące symbole
dla wszystkich %n wybranych pokoi:</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="131"/>
      <source>%1 {count:%2}</source>
      <comment>Everything after the first parameter (the &apos;%1&apos;) will be removed by processing it as a QRegularExpression programmatically, ensure the translated text has ` {` immediately after the &apos;%1&apos;, and &apos;}&apos; as the very last character, so that the right portion can be extracted if the user clicks on this item when it is shown in the QComboBox it is put in.</comment>
      <translation>%1 {ilość: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="202"/>
      <source>Pick color</source>
      <translation>Wybierz kolor</translation>
    </message>
  </context>
  <context>
    <name>dlgTriggerEditor</name>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="248"/>
      <source>-- Enter your lua code here
</source>
      <translation>-- Wprowadź tutaj swój kod lua
</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="328"/>
      <source>*** starting new session ***
</source>
      <translation>*** rozpoczęcie nowej sesji ***
</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="416"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8386"/>
      <source>Triggers</source>
      <translation>Wyzwalacze</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="417"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="418"/>
      <source>Show Triggers</source>
      <translation>Pokaż Wyzwalacze</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="446"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5870"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8398"/>
      <source>Buttons</source>
      <translation>Przyciski</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="447"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="448"/>
      <source>Show Buttons</source>
      <translation>Pokaż przyciski</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="421"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8388"/>
      <source>Aliases</source>
      <translation>Aliasy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="422"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="423"/>
      <source>Show Aliases</source>
      <translation>Pokaż Aliasy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="431"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5852"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8392"/>
      <source>Timers</source>
      <translation>Liczniki czasu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="432"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="433"/>
      <source>Show Timers</source>
      <translation>Pokaż Timery</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="426"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5858"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8390"/>
      <source>Scripts</source>
      <translation>Skrypty</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="427"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="428"/>
      <source>Show Scripts</source>
      <translation>Pokaż skrypty</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="436"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8394"/>
      <source>Keys</source>
      <translation>Klawisze</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="437"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="438"/>
      <source>Show Keybindings</source>
      <translation>Pokaż Kombinacji Klawiszy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="441"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="6258"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8396"/>
      <source>Variables</source>
      <translation>Zmienne</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="442"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="443"/>
      <source>Show Variables</source>
      <translation>Pokaż Zmienne</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="468"/>
      <source>Activate</source>
      <translation>Aktywuj</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="469"/>
      <source>Toggle Active or Non-Active Mode for Triggers, Scripts etc.</source>
      <translation>Przełącz tryb Aktywny lub Nieaktywny dla wyzwalaczy, skryptów itp.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="479"/>
      <source>Add Item</source>
      <translation>Dodaj element</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="480"/>
      <source>Add new Trigger, Script, Alias or Filter</source>
      <translation>Dodaj nowy wyzwalacz, skrypt, alias lub filtr</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="483"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="485"/>
      <source>Delete Item</source>
      <translation>Usuń element</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="484"/>
      <source>Delete Trigger, Script, Alias or Filter</source>
      <translation>Usuń wyzwalacz, skrypt, alias lub filtr</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="491"/>
      <source>Add Group</source>
      <translation>Dodaj grupę</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="492"/>
      <source>Add new Group</source>
      <translation>Dodawanie nowej grupy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="495"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8376"/>
      <source>Save Item</source>
      <translation>Zapisz element</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8377"/>
      <source>Ctrl+S</source>
      <translation>Ctrl+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="497"/>
      <source>Saves the selected item. (Ctrl+S)&lt;/p&gt;Saving causes any changes to the item to take effect.
It will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)</source>
      <translation>Zapisuje zaznaczony element. (Ctrl+S)&lt;/p&gt;Zapisywanie powoduje, że wszelkie zmiany elementu zostaną wprowadzone.
Nie zapisze się na dysku, więc zmiany zostaną utracone w przypadku awarii komputera / programu (ale Zapisz profil po prawej stronie będzie bezpieczny).</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="499"/>
      <source>Saves the selected trigger, script, alias, etc, causing new changes to take effect - does not save to disk though...</source>
      <translation>Zapisuje wybrany wyzwalacz, skrypt, alias itp., powodując wprowadzenie nowych zmian - nie zapisuje się jednak na dysku...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="502"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8841"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8847"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="506"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="507"/>
      <source>Copy the trigger/script/alias/etc</source>
      <translation>Kopiuj wyzwalacz/script/alias/itp</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="516"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8842"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8848"/>
      <source>Paste</source>
      <translation>Wklej</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="520"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="521"/>
      <source>Paste triggers/scripts/aliases/etc from the clipboard</source>
      <translation>Wklej wyzwalacze / skrypty / aliasy / etc ze schowka</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="535"/>
      <source>Import</source>
      <translation>Importuj</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="539"/>
      <source>Export</source>
      <translation>Eksportuj</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="543"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8378"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8683"/>
      <source>Save Profile</source>
      <translation>Zapisz profil</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8379"/>
      <source>Ctrl+Shift+S</source>
      <translation>Ctrl+Shift+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="547"/>
      <source>Saves your profile. (Ctrl+Shift+S)&lt;p&gt;Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.&lt;/p&gt;&lt;p&gt;It also makes a backup of your profile, you can load an older version of it when connecting.&lt;/p&gt;&lt;p&gt;Should there be any modules that are marked to be &quot;&lt;i&gt;synced&lt;/i&gt;&quot; this will also cause them to be saved and reloaded into other profiles if they too are active.</source>
      <translation>Zapisuje swój profil. (Ctrl+Shift+S)&lt;p&gt;Zapisuje cały profil (wyzwalacze, aliasy, skrypty, czasomierze, przyciski i klucze, ale nie mapę lub ustawienia specyficzne dla skryptu) na dysku komputera, więc w przypadku awarii komputera lub programu wszystkie wprowadzone zmiany zostaną zachowane.&lt;/p&gt;&lt;p&gt;Tworzy również kopię zapasową profilu, można załadować starszą wersję podczas łączenia.&lt;/p&gt;&lt;p&gt;Jeżeli istnieją jakieś moduły, które są oznaczone jako &quot;&lt;i&gt;Synchronizowane&lt;/i&gt;&quot; spowoduje to również ich zapisanie i ponowne załadowanie do innych profili, jeśli również są aktywne.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="549"/>
      <source>Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also &quot;synchronizes&quot; modules that are so marked.</source>
      <translation>Zapisuje cały profil (wyzwalacze, aliasy, skrypty, timery, przyciski i klawisze, ale nie mapę ani ustawienia własne skryptu); również &quot;synchronizuje&quot; moduły, które są tak oznaczone.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="552"/>
      <source>Save Profile As</source>
      <translation>Zapisz profil jako</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="457"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8402"/>
      <source>Statistics</source>
      <translation>Statystyki</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="104"/>
      <source>&lt;p&gt;Alias react on user input. To add a new alias:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define an input &lt;strong&gt;pattern&lt;/strong&gt; either literally or with a Perl regular expression.&lt;/li&gt;&lt;li&gt;Define a &apos;substitution&apos; &lt;strong&gt;command&lt;/strong&gt; to send to the game in clear text &lt;strong&gt;instead of the alias pattern&lt;/strong&gt;, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the alias.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;That&apos;s it! If you&apos;d like to be able to create aliases from the input line, there are a &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=22609&apos;&gt;couple&lt;/a&gt; of &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=16462&apos;&gt;packages&lt;/a&gt; that can help you.&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Aliases&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="112"/>
      <source>&lt;p&gt;Triggers react on game output. To add a new trigger:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;pattern&lt;/strong&gt; that you want to trigger on.&lt;/li&gt;&lt;li&gt;Select the appropriate pattern &lt;strong&gt;type&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the trigger.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;That&apos;s it! If you&apos;d like to be able to create triggers from the input line, there are a &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=22609&apos;&gt;couple&lt;/a&gt; of &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=16462&apos;&gt;packages&lt;/a&gt; that can help you.&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Triggers&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="177"/>
      <source>%1 - Editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="462"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8404"/>
      <source>Debug</source>
      <translation>Debugowanie</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="678"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="681"/>
      <source>Search Options</source>
      <translation>Opcje wyszukiwania</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="685"/>
      <source>Case sensitive</source>
      <translation>Uwzględniana wielkość liter</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="738"/>
      <source>Type</source>
      <comment>Heading for the first column of the search results</comment>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="739"/>
      <source>Name</source>
      <comment>Heading for the second column of the search results</comment>
      <translation>Nazwa</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="740"/>
      <source>Where</source>
      <comment>Heading for the third column of the search results</comment>
      <translation>Gdzie</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="741"/>
      <source>What</source>
      <comment>Heading for the fourth column of the search results</comment>
      <translation>Co</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="788"/>
      <source>start of line</source>
      <translation>początek linii</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="821"/>
      <source>Text to find (trigger pattern)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2791"/>
      <source>Trying to activate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba aktywowania grupy wyzwalacza, filtru lub wyzwalacza lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2793"/>
      <source>Trying to deactivate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba dezaktywacji grupy wyzwalacza, filtru lub wyzwalacza lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2797"/>
      <source>&lt;b&gt;Unable to activate a filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;b&gt;Nie można aktywować filtru lub wyzwalacza lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera; Powodu: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Należy ponownie aktywować to po naprawieniu problemu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2934"/>
      <source>Trying to activate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba aktywowania grupy czasomierza, czasomierza offsetowego, timera lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2936"/>
      <source>Trying to deactivate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba dezaktywacji grupy czasomierza, czasomierza offsetowego, timera lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2940"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an offset timer or timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Nie można aktywować czasomierza offsetowego lub timera lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera; Powodu: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Należy ponownie aktywować to po naprawieniu problemu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2978"/>
      <source>Trying to activate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba aktywowania grupy aliasów, aliasu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2980"/>
      <source>Trying to deactivate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba dezaktywacji grupy aliasów, aliasu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2984"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Nie można aktywować aliasu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera; Powodu: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Należy ponownie aktywować to po naprawieniu problemu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3082"/>
      <source>Trying to activate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba aktywowania grupy skryptów, skryptu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3084"/>
      <source>Trying to deactivate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba dezaktywacji grupy skryptów, skryptu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3088"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a script group or script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Nie można aktywować grupy skryptów lub skryptu lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera; Powodu: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Należy ponownie aktywować to po naprawieniu problemu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3153"/>
      <source>Trying to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba aktywowania przycisku/menu/paska narzędzi lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; który je zawiera &lt;em&gt;Zakończyła się pomyślnie&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3155"/>
      <source>Trying to deactivate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation>Próba zdezaktywowania przycisku/menu/paska narzędzi lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; , który je zawiera &lt;em&gt;, powiodła się&lt;/em&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3159"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;&lt;b&gt;Nie można aktywować przycisku/menu/paska narzędziowego lub części modułu &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; , który je zawiera; powód: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;Trzeba to ponownie uaktywnić po usunięciu problemu.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3273"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4105"/>
      <source>New trigger group</source>
      <translation>Nowa grupa wyzwalaczy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3275"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4105"/>
      <source>New trigger</source>
      <translation>Nowy wyzwalacz</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3374"/>
      <source>New timer group</source>
      <translation>Nowa grupa zegarów</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3376"/>
      <source>New timer</source>
      <translation>Nowy zegar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3464"/>
      <source>Table name...</source>
      <translation>Nazwa tabeli...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3471"/>
      <source>Variable name...</source>
      <translation>Nazwa zmiennej...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3480"/>
      <source>New table name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3480"/>
      <source>New variable name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3530"/>
      <source>New key group</source>
      <translation>Nowa grupa kluczy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3532"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4805"/>
      <source>New key</source>
      <translation>Nowy sekwencja klawiszy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3614"/>
      <source>New alias group</source>
      <translation>Nowa grupa aliasów</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3616"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4213"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4283"/>
      <source>New alias</source>
      <translation>Nowy alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3705"/>
      <source>New menu</source>
      <translation>Nowe menu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3707"/>
      <source>New button</source>
      <translation>Nowy Przycisk</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3738"/>
      <source>New toolbar</source>
      <translation>Nowy pasek narzędzi</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3791"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4532"/>
      <source>New script group</source>
      <translation>Nowa grupa skryptów</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3793"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4532"/>
      <source>New script</source>
      <translation>Nowy skrypt</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4227"/>
      <source>Alias &lt;em&gt;%1&lt;/em&gt; has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn&apos;t good as it&apos;ll call itself forever.</source>
      <translation>Alias &lt;em&gt;%1&lt;/em&gt; ma nieskończoną pętlę - podstawienie pasuje do własnego wzoru. Proszę go naprawić - ten alias jest&apos;t dobry, jak będzie&apos;uruchamiać się nie na zawsze.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4770"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation>Sprawdzone zmienne zostaną zapisane i załadowane wraz z Twoim profilem.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4905"/>
      <source>match on the prompt line</source>
      <translation>dopasować w wierszu monitu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4908"/>
      <source>match on the prompt line (disabled)</source>
      <translation>dopasowywanie w wierszu monitu (wyłączone)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4909"/>
      <source>A Go-Ahead (GA) signal from the game is required to make this feature work</source>
      <translation>Sygnał Go-Ahead (GA) z gry jest wymagany, aby ta funkcja działała</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4959"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5069"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8582"/>
      <source>Foreground color ignored</source>
      <comment>Color trigger ignored foreground color button, ensure all three instances have the same text</comment>
      <translation>Kolor pierwszego etapu ignorowany</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4963"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5073"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8585"/>
      <source>Default foreground color</source>
      <comment>Color trigger default foreground color button, ensure all three instances have the same text</comment>
      <translation>Domyślny kolor pierwszego planu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4967"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5077"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8588"/>
      <source>Foreground color [ANSI %1]</source>
      <comment>Color trigger ANSI foreground color button, ensure all three instances have the same text</comment>
      <translation>Kolor pierwszego planu [ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4974"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5084"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8645"/>
      <source>Background color ignored</source>
      <comment>Color trigger ignored background color button, ensure all three instances have the same text</comment>
      <translation>Kolor tła ignorowany</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4978"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5088"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8648"/>
      <source>Default background color</source>
      <comment>Color trigger default background color button, ensure all three instances have the same text</comment>
      <translation>Domyślny kolor tła</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4982"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5092"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8651"/>
      <source>Background color [ANSI %1]</source>
      <comment>Color trigger ANSI background color button, ensure all three instances have the same text</comment>
      <translation>Kolor tła [ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5103"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5105"/>
      <source>fault</source>
      <translation>usterka</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5158"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5162"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8471"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8497"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8991"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8992"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation>zachowaj</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5619"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8445"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5658"/>
      <source>Menu properties</source>
      <translation>Właściwości menu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5668"/>
      <source>Button properties</source>
      <translation>Właściwości przycisku</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5676"/>
      <source>Command (down);</source>
      <translation>Polecenie (dół);</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5864"/>
      <source>Aliases - Input Triggers</source>
      <translation>Aliasy — wyzwalacze wejściowe</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5876"/>
      <source>Key Bindings</source>
      <translation>Przypisania klawiszy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7616"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7620"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7640"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7644"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7664"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7668"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7688"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7692"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7712"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7716"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7736"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7741"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7761"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7784"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7788"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7807"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7811"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7830"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7834"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7853"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7857"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7876"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7881"/>
      <source>Export Package:</source>
      <translation>Eksportuj pakiet:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7616"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7620"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7640"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7644"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7664"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7668"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7688"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7692"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7712"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7716"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7736"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7741"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7761"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7784"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7788"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7807"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7811"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7830"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7834"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7853"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7857"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7876"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7881"/>
      <source>You have to choose an item for export first. Please select a tree item and then click on export again.</source>
      <translation>Najpierw musisz wybrać przedmiot do wyeksportowania. Wybierz element drzewa, a następnie kliknij ponownie na eksport.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7625"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7649"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7673"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7697"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7721"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7746"/>
      <source>Package %1 saved</source>
      <translation>Pakiet %1 zapisany</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7770"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7793"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7816"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7839"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7862"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Copied %1 to clipboard</source>
      <translation>Skopiowano %1 do schowka</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7896"/>
      <source>Mudlet packages (*.xml)</source>
      <translation>Pakiety mudlet (*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7896"/>
      <source>Export Item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7910"/>
      <source>export package:</source>
      <translation>pakiet eksportowy:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7910"/>
      <source>Cannot write file %1:
%2.</source>
      <translation>Nie można zapisać pliku %1:
%2.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8179"/>
      <source>Import Mudlet Package</source>
      <translation>Importuj pakiet Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8273"/>
      <source>Couldn&apos;t save profile</source>
      <translation>Nie udało się zapisać profilu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8273"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation>Przepraszamy, nie udało się zapisać profilu - otrzymano następujący błąd: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8280"/>
      <source>Backup Profile</source>
      <translation>Profil kopii zapasowej</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8280"/>
      <source>trigger files (*.trigger *.xml)</source>
      <translation>pliki wyzwalacza (*.trigger *.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8405"/>
      <source>Ctrl+0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8432"/>
      <source>Images (*.png *.xpm *.jpg)</source>
      <translation>Obrazy (*.png *.xpm *.jpg)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8432"/>
      <source>Select Icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8441"/>
      <source>Command (down):</source>
      <translation>Polecenie (dół):</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8462"/>
      <source>Select foreground color to apply to matches</source>
      <translation>Wybierz kolor pierwszego planu, aby zastosować je do dopasowań</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8488"/>
      <source>Select background color to apply to matches</source>
      <translation>Wybieranie koloru tła do dopasowania do dopasowania</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8506"/>
      <source>Choose sound file</source>
      <translation>Wybieranie pliku dźwiękowego</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8510"/>
      <source>Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;All files(*.*)</source>
      <comment>This the list of file extensions that are considered for sounds from triggers, the terms inside of the &apos;(&apos;...&apos;)&apos; and the &quot;;;&quot; are used programmatically and should not be changed.</comment>
      <translation>Pliki audio(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);; Zaawansowane kodowanie dźwięku-strumień(*.aac);; MPEG-2 Warstwa audio 3(*.mp3);; MPEG-4 Audio(*.mp4a);;O gg Vorbis(*.oga *.ogg);; PCM Audio(*.pcm);; Fala(*.wav);; Windows Media Audio(*.wma);; Wszystkie pliki(*.*)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8563"/>
      <source>Select foreground trigger color for item %1</source>
      <translation>Wybieranie koloru wyzwalacza pierwszego planu dla elementu %1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8627"/>
      <source>Select background trigger color for item %1</source>
      <translation>Wybieranie koloru wyzwalacza tła dla elementu %1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8676"/>
      <source>Saving…</source>
      <translation>Zapisywanie…</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8837"/>
      <source>Format All</source>
      <translation>Sformatuj wszystko</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8840"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8846"/>
      <source>Cut</source>
      <translation>Wytnij</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8844"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8850"/>
      <source>Select All</source>
      <translation>Zaznacz wszystko</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9006"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Plik dźwiękowy do odtwożenia po uruchomieniu wyzwalacza.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="786"/>
      <source>substring</source>
      <translation>wycinek tekstu</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="121"/>
      <source>&lt;p&gt;Scripts organize code and can react to events. To add a new script:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Enter a script in the box below. You can for example define &lt;strong&gt;functions&lt;/strong&gt; to be called by other triggers, aliases, etc.&lt;/li&gt;&lt;li&gt;If you write lua &lt;strong&gt;commands&lt;/strong&gt; without defining a function, they will be run on Mudlet startup and each time you open the script for editing.&lt;/li&gt;&lt;li&gt;If needed, you can register a list of &lt;strong&gt;events&lt;/strong&gt; with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the script.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Scripts are run automatically when viewed, even if they are deactivated.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Events can also be added to a script from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;nameOfTheMudletEvent&amp;quot;, &amp;quot;nameOfYourFunctionToBeCalled&amp;quot;)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="132"/>
      <source>&lt;p&gt;Timers react after a timespan once or regularly. To add a new timer:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define the &lt;strong&gt;timespan&lt;/strong&gt; after which the timer should react in a this format: hours : minutes : seconds.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game when the time has passed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the timer.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Timers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;hello!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will greet you exactly 3 seconds after it was made.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="143"/>
      <source>&lt;p&gt;Buttons react on mouse clicks. To add a new button:&lt;ol&gt;&lt;li&gt;Add a new group to define a new &lt;strong&gt;button bar&lt;/strong&gt; in case you don&apos;t have any.&lt;/li&gt;&lt;li&gt;Add new groups as &lt;strong&gt;menus&lt;/strong&gt; to a button bar or sub-menus to menus.&lt;li&gt;&lt;li&gt;Add new items as &lt;strong&gt;buttons&lt;/strong&gt; to a button bar or menu or sub-menu.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the toolbar, menu or button. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If a button is made a &lt;strong&gt;click-down&lt;/strong&gt; button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua &apos;getButtonState()&apos; function reports whether the button is up or down.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="153"/>
      <source>&lt;p&gt;Keys react on keyboard presses. To add a new key binding:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Click on &lt;strong&gt;&apos;grab key&apos;&lt;/strong&gt; and then press your key combination, e.g. including modifier keys like Control, Shift, etc.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the new key binding.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Keys can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;my jump key&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;jump&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Pressing F8 will make you jump.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="163"/>
      <source>&lt;p&gt;Variables store information. To make a new variable:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above. To add a table instead click &apos;Add Group&apos;.&lt;/li&gt;&lt;li&gt;Select type of variable value (can be a string, integer, boolean)&lt;/li&gt;&lt;li&gt;Enter the value you want to store in this variable.&lt;/li&gt;&lt;li&gt;If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.&lt;/li&gt;&lt;li&gt;To remove a variable manually, set it to &apos;nil&apos; or click on the &apos;Delete&apos; icon above.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables created here won&apos;t be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables and tables can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will create a string called &apos;foo&apos; with &apos;bar&apos; as its value.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="238"/>
      <source>-- add your Lua code here</source>
      <translation>-- dodaj tutaj swój kod Lua</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="418"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8387"/>
      <source>Ctrl+1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="423"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8389"/>
      <source>Ctrl+2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="428"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8391"/>
      <source>Ctrl+3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8393"/>
      <source>Ctrl+4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="438"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8395"/>
      <source>Ctrl+5</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="443"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8397"/>
      <source>Ctrl+6</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="448"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8399"/>
      <source>Ctrl+7</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="452"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8400"/>
      <source>Errors</source>
      <translation>Błędy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="453"/>
      <source>Show/Hide the errors console in the bottom right of this editor.</source>
      <translation>Pokazuje/ukrywa konsolę błędów w prawym dolnym rogu edytora.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="454"/>
      <source>Show/Hide errors console</source>
      <translation>Pokaż/ukryj konsolę błędów</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="454"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8401"/>
      <source>Ctrl+8</source>
      <translation>Ctrl+8</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="458"/>
      <source>Generate a statistics summary display on the main profile console.</source>
      <translation>Wygeneruj podsumowanie statystyk na głównej konsoli profilu.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="459"/>
      <source>Generate statistics</source>
      <translation>Generuj statystyki</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="459"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8403"/>
      <source>Ctrl+9</source>
      <translation>Ctrl+9</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="463"/>
      <source>Show/Hide the separate Central Debug Console - when being displayed the system will be slower.</source>
      <translation>Pokazuje/ukrywa oddzielną Centralną Konsolę Debugowania - gdy jest wyświetlana system będzie wolniejszy.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="464"/>
      <source>Show/Hide Debug Console (Ctrl+0) -&gt; system will be &lt;b&gt;&lt;i&gt;slower&lt;/i&gt;&lt;/b&gt;.</source>
      <translation>Aktywuje komunikaty debugowania -&gt; system będzie &lt;b&gt;&lt;i&gt;wolniejszy&lt;/i&gt;&lt;/b&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="576"/>
      <source>Editor Toolbar - %1 - Actions</source>
      <comment>This is the toolbar that is initially placed at the top of the editor.</comment>
      <translation>Pasek narzędzi edytora - %1 - Akcje</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="612"/>
      <source>Editor Toolbar - %1 - Items</source>
      <comment>This is the toolbar that is initially placed at the left side of the editor.</comment>
      <translation>Pasek narzędzi Edytora - %1 - Elementy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="688"/>
      <source>Match case precisely</source>
      <translation>Dokładnie dopasuj wielkość liter</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="692"/>
      <source>Include variables</source>
      <translation>Dodaj zmienne</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="695"/>
      <source>Search variables (slower)</source>
      <translation>Szukaj zmiennych (wolniejsze)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="787"/>
      <source>perl regex</source>
      <translation>perl regex</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="789"/>
      <source>exact match</source>
      <translation>dokładne dopasowanie</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="790"/>
      <source>lua function</source>
      <translation>funkcja lua</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="791"/>
      <source>line spacer</source>
      <translation>separator linii</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="792"/>
      <source>color trigger</source>
      <translation>wyzwalacz kolorowy</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="793"/>
      <source>prompt</source>
      <translation>zachęta</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1920"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1932"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1960"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1992"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2022"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2034"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2061"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2096"/>
      <source>Trigger</source>
      <translation>Wyzwalacz</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1440"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1483"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1555"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1627"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1749"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1833"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1920"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2022"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2128"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2217"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2303"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2427"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2501"/>
      <source>Name</source>
      <translation>Nazwa</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1495"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1500"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1567"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1572"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1639"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1644"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1843"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1848"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1932"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1937"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2034"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2039"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2138"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2143"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2315"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2320"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2439"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2444"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2513"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2518"/>
      <source>Command</source>
      <translation>Polecenie</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1960"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1965"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2061"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2066"/>
      <source>Pattern {%1}</source>
      <translation>Wzorzec {%1}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1525"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1530"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1597"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1602"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1719"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1724"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1803"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1808"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1890"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1895"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1992"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1997"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2096"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2101"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2185"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2190"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2271"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2276"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2395"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2400"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2469"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2474"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2543"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2548"/>
      <source>Lua code (%1:%2)</source>
      <translation>Kod Lua (%1:%2)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1833"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1843"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1860"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1890"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2128"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2138"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2155"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2185"/>
      <source>Alias</source>
      <translation>Alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1860"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1865"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2155"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2160"/>
      <source>Pattern</source>
      <translation>Wzorzec</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1749"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1771"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1803"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2217"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2239"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2271"/>
      <source>Script</source>
      <translation>Skrypt</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1771"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1776"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2239"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2244"/>
      <source>Event Handler</source>
      <translation>Obsługa zdarzeń</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1627"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1639"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1658"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1719"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2303"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2315"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2334"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2395"/>
      <source>Button</source>
      <translation>Przycisk</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1639"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1644"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2315"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2320"/>
      <source>Command {Down}</source>
      <translation>Komenda {Down}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1658"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1663"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2334"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2339"/>
      <source>Command {Up}</source>
      <translation>Komenda {Up}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1687"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2363"/>
      <source>Action</source>
      <translation>Akcja</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1687"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1692"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2363"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2368"/>
      <source>Stylesheet {L: %1 C: %2}</source>
      <translation>Arkusz stylów {L: %1 C: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1555"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1567"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1597"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2427"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2439"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2469"/>
      <source>Timer</source>
      <translation>Zegar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1483"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1495"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1525"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2501"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2513"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2543"/>
      <source>Key</source>
      <translation>Klawisz</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1440"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1454"/>
      <source>Variable</source>
      <translation>Zmienna</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1454"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1460"/>
      <source>Value</source>
      <translation>Wartość</translation>
    </message>
  </context>
  <context>
    <name>dlgTriggerPatternEdit</name>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="52"/>
      <source>Text to find (anywhere in the game output)</source>
      <translation>Tekst do znalezienia (gdziekolwiek w tekście z gry)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="55"/>
      <source>Text to find (as a regular expression pattern)</source>
      <translation>Tekst do znalezienia (jako wyrażenie regularne)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="58"/>
      <source>Text to find (from beginning of the line)</source>
      <translation>Tekst do znalezienia (od początku linii)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="61"/>
      <source>Exact line to match</source>
      <translation>Dokładny tekst linii do dopasowania</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="64"/>
      <source>Lua code to run (return true to match)</source>
      <translation>Kod Lua do uruchomienia (zwróć true, aby dopasować)</translation>
    </message>
  </context>
  <context>
    <name>dlgVarsMainArea</name>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="51"/>
      <location filename="../src/dlgVarsMainArea.cpp" line="78"/>
      <source>Auto-Type</source>
      <translation>Automatyczny Typ</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="52"/>
      <source>key (string)</source>
      <translation>klucz (ciąg znaków)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="53"/>
      <source>index (integer number)</source>
      <translation>indeks (liczba całkowita)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="54"/>
      <source>table (use &quot;Add Group&quot; to create)</source>
      <translation>tabela (użyj &quot;Dodaj grupę&quot;, aby utworzyć)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="55"/>
      <source>function (cannot create from GUI)</source>
      <translation>funkcja (nie można tworzyć z GUI)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="79"/>
      <source>string</source>
      <translation>ciąg znaków</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="80"/>
      <source>number</source>
      <translation>liczba</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="81"/>
      <source>boolean</source>
      <translation>wartość logiczna</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="82"/>
      <source>table</source>
      <translation>tabela</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="83"/>
      <source>function</source>
      <translation>funkcja</translation>
    </message>
  </context>
  <context>
    <name>edbee::TextEditorComponent</name>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="578"/>
      <source>Cut</source>
      <translation>Wytnij</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="579"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="580"/>
      <source>Paste</source>
      <translation>Wklej</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="582"/>
      <source>Select All</source>
      <translation>Zaznacz wszystko</translation>
    </message>
  </context>
  <context>
    <name>irc</name>
    <message>
      <location filename="../src/ui/irc.ui" line="25"/>
      <source>Mudlet IRC Client</source>
      <translation>Klient IRC Mudleta</translation>
    </message>
  </context>
  <context>
    <name>keybindings_main_area</name>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your key or key group. This will be displayed in the key tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz dobrą, idealnie unikalną, nazwę klucza lub grupy znaków. Będzie ona wyświetlana w drzewie kluczy.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="40"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
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
      <translation>Powiązanie klawiszy:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="77"/>
      <source>Grab New Key</source>
      <translation>Pobierz nowy klucz</translation>
    </message>
  </context>
  <context>
    <name>lacking_mapper_script</name>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="23"/>
      <source>No mapping script found</source>
      <translation>Nie znaleziono skryptu mapowania</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="35"/>
      <source>&lt;p&gt;It seems that you don&apos;t have any &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;mapping scripts&lt;/a&gt; installed yet - the mapper needs you to have one for your game, so it can track where you are and autowalk you. You can either make one yourself, or import an existing one that someone else made.&lt;/p&gt;&lt;p&gt;Would you like to see if any are available?&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wygląda na to, że nie masz jeszcze zainstalowanego żądnego &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;skryptu mapującego&lt;/a&gt; - mapa musi mieć jeden skrypt dla gry, aby móc śledzić gdzie jesteś i automatycznie Cię śledzić. Możesz stworzyć siebie albo zaimportować istniejący już inny skrypt.&lt;/p&gt;&lt;p&gt;Czy chciałbyś zobaczyć, czy ktoś jest dostępny?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="86"/>
      <source>Close</source>
      <translation>Zamknij</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="93"/>
      <source>Find some scripts</source>
      <translation>Znajdź skrypty</translation>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <location filename="../src/main.cpp" line="196"/>
      <source>Profile to open automatically</source>
      <translation>Profil otwierany automatycznie</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="196"/>
      <source>profile</source>
      <translation>proﬁl</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="199"/>
      <source>Display help and exit</source>
      <translation>Wyświetl pomoc i wyjdź</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="202"/>
      <source>Display version and exit</source>
      <translation>Wyświetl wersję i wyjdź</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="205"/>
      <source>Don&apos;t show the splash screen when starting</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="208"/>
      <source>Mirror output of all consoles to STDOUT</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="218"/>
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
      <location filename="../src/main.cpp" line="275"/>
      <source>%1 %2%3 (with debug symbols, without optimisations)
</source>
      <comment>%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev</comment>
      <translation>%1 %2%3 (z symbolami debugowania, bez optymalizacji)
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="281"/>
      <source>Qt libraries %1 (compilation) %2 (runtime)
</source>
      <comment>%1 and %2 are version numbers</comment>
      <translation>Biblioteki Qt %1 (kompilacja) %2 (czas wykonania)
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="285"/>
      <source>Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html
</source>
      <translation>Licencja GPLv2+: GNU GPL w wersji 2 lub nowszej - http://gnu.org/licenses/gpl.html
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="286"/>
      <source>This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
</source>
      <translation>To jest wolne oprogramowanie: możesz go zmieniać i rozpowszechniać.
NIE MA GWARANCJI, w zakresie dozwolonym przez prawo.
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="266"/>
      <source>Report bugs to: https://github.com/Mudlet/Mudlet/issues
</source>
      <translation>Zgłaszanie błędów do: https://github.com/Mudlet/Mudlet/issues
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="267"/>
      <source>Project home page: http://www.mudlet.org/
</source>
      <translation>Strona główna projektu: http://www.mudlet.org/
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="284"/>
      <source>Copyright © 2008-2021  Mudlet developers
</source>
      <translation>Prawa autorskie © 2008-2021  Deweloperzy Mudlet
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="310"/>
      <source>Version: %1</source>
      <translation>Wersja: %1</translation>
    </message>
  </context>
  <context>
    <name>main_window</name>
    <message>
      <location filename="../src/ui/main_window.ui" line="95"/>
      <source>Toolbox</source>
      <translation>Narzędzia</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="110"/>
      <source>Options</source>
      <translation>Opcje</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="117"/>
      <source>Help</source>
      <translation>Pomoc</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="128"/>
      <source>About</source>
      <translation>O programie</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="136"/>
      <source>Games</source>
      <translation>Gry</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="151"/>
      <source>Play</source>
      <translation>Graj</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="154"/>
      <source>&lt;p&gt;Configure connection details of, and make a connection to, game servers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Skonfiguruj szczegóły połączenia z serwerami gry.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="162"/>
      <source>&lt;p&gt;Disconnect from the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Odłącz się od aktualnego serwera gry.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="170"/>
      <source>&lt;p&gt;Disconnect and then reconnect to the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Rozłącz się, a następnie połącz ponownie z bieżącym serwerem gry.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="178"/>
      <source>&lt;p&gt;Configure setting for the Mudlet application globally and for the current profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Skonfiguruj ustawienia dla aplikacji Mudlet globalnie i dla bieżącego profilu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="186"/>
      <source>&lt;p&gt;Opens the Editor for the different types of things that can be scripted by the user.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Otwiera edytor dla różnych typów rzeczy, które mogą być skryptowane przez użytkownika.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="191"/>
      <source>Show errors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="194"/>
      <source>&lt;p&gt;Show errors from scripts that you have running&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="223"/>
      <source>IRC</source>
      <translation>IRC</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="234"/>
      <source>&lt;p&gt;Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Otwiera (on-line) kolekcję &quot;Ekrany edukacyjne Mudlet&quot; w przeglądarce internetowej systemu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="242"/>
      <source>&lt;p&gt;Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Załaduj wcześniej zapisaną sesję gry, która może być użyta do testowania systemów Mudlet lua (off-line!).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="250"/>
      <source>&lt;p&gt;Opens the (on-line) Mudlet Forum in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Otwiera (on-line) Forum Mudleta w przeglądarce internetowej systemu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="271"/>
      <source>&lt;p&gt;Show or hide the game map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Pokaż lub ukryj mapę.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="279"/>
      <source>&lt;p&gt;Install and remove collections of Mudlet lua items (packages).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Instalacja i usuwanie kolekcji elementów (pakietów) Mudlet lua.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="287"/>
      <source>&lt;p&gt;Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zainstaluj i usuń (share- &amp; sync-able) kolekcje elementów Mudlet lua (moduły).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="292"/>
      <source>Package exporter</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="295"/>
      <source>&lt;p&gt;Gather and bundle up collections of Mudlet Lua items and other reasources into a module.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zbierz i zapakuj kolekcje przedmiotów Mudlet Lua i innych zasobów do modułu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="326"/>
      <source>&lt;p&gt;Hide / show the search area and buttons at the bottom of the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="331"/>
      <source>Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="334"/>
      <source>&lt;p&gt;Open a link to Discord.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="342"/>
      <source>Discord help channel</source>
      <translation>Kanał pomocy Discord</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="345"/>
      <source>&lt;p&gt;Open a link to the Mudlet server on Discord.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="350"/>
      <location filename="../src/ui/main_window.ui" line="353"/>
      <source>Report an issue</source>
      <translation>Zgłaszanie problemu</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="356"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation>Publiczna kompilacja testowa szybciej zyskuje nowe funkcje, a Ty pomagasz nam szybciej znaleźć w nich problemy. Zauważył coś dziwnego? Daj nam znać jak najszybciej!</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="159"/>
      <source>Disconnect</source>
      <translation>Rozłącz się</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="167"/>
      <source>Reconnect</source>
      <translation>Połącz ponownie</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="175"/>
      <source>Preferences</source>
      <translation>Preferencje</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="183"/>
      <source>Script editor</source>
      <translation>Edytor skryptów</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="199"/>
      <source>Notepad</source>
      <translation>Notatnik</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="207"/>
      <source>API Reference</source>
      <translation>Odwołanie do interfejsu API</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="247"/>
      <source>Online forum</source>
      <translation>Forum online</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="215"/>
      <source>About Mudlet</source>
      <translation>O Mudlecie</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="202"/>
      <source>&lt;p&gt;Opens a free form text editor window for this profile that is saved between sessions.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Otwiera okno edytora tekstu w formie swobodnej dla tego profilu, który jest zapisywany między sesjami.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="210"/>
      <source>&lt;p&gt;Opens the Mudlet manual in your web browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Otwiera podręcznik Mudlet w przeglądarce internetowej.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="218"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation>&lt;p&gt;Poinformuj się o tej wersji Mudlet, o ludziach, którzy go stworzyli i o licencji, na której możesz się nim dzielić.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="226"/>
      <location filename="../src/ui/main_window.ui" line="263"/>
      <source>&lt;p&gt;Opens a built-in IRC chat.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="231"/>
      <source>Video tutorials</source>
      <translation>Samouczki wideo</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="239"/>
      <source>Load replay</source>
      <translation>Wczytaj powtórkę</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="255"/>
      <source>Check for updates...</source>
      <translation>Sprawdź dostępność aktualizacji...</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="260"/>
      <source>Live help chat</source>
      <translation>Czat pomocy na żywo</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="268"/>
      <source>Show map</source>
      <translation>Pokaż mapę</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="276"/>
      <source>Package manager</source>
      <translation>Menedżer pakietów</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="284"/>
      <source>Module manager</source>
      <translation>Menedżer modułów</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="309"/>
      <source>MultiView</source>
      <translation>MultiView</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="312"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="323"/>
      <source>Compact input line</source>
      <translation>Kompaktowa linia wprowadzania</translation>
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
      <translation>Obszar:</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="521"/>
      <source>Rooms</source>
      <translation>Pokoje</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="550"/>
      <source>Exits</source>
      <translation>Wyjścia</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="579"/>
      <source>Round</source>
      <translation>Okrągłe</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="586"/>
      <source>Info</source>
      <translation>Informacje</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="602"/>
      <source>IDs</source>
      <translation>Identyfikatory</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="618"/>
      <source>Names</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="771"/>
      <source>top + 1</source>
      <translation>góra + 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="745"/>
      <source>bottom + 1</source>
      <translation>dół + 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="758"/>
      <source>bottom -1</source>
      <translation>dół -1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="784"/>
      <source>top - 1</source>
      <translation>góra - 1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="732"/>
      <source>1 level</source>
      <translation>1 poziom</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="427"/>
      <source>3D</source>
      <translation>3D</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="680"/>
      <source>default</source>
      <translation>domyślny</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="693"/>
      <source>top view</source>
      <translation>widok z góry</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="706"/>
      <source>side view</source>
      <translation>widok z boku</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="719"/>
      <source>all levels</source>
      <translation>wszystkie poziomy</translation>
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
      <translation>Odinstaluj</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="142"/>
      <source>Install</source>
      <translation>Zainstaluj</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="149"/>
      <source>Module Help</source>
      <translation>Moduł pomocy</translation>
    </message>
  </context>
  <context>
    <name>mudlet</name>
    <message>
      <location filename="../src/mudlet.cpp" line="734"/>
      <source>Afrikaans</source>
      <extracomment>In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a &apos;-&apos; Chinese is an exception!</extracomment>
      <translation>Afrykański</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="735"/>
      <source>Afrikaans (South Africa)</source>
      <translation>Afrikański (Południowa Afryka)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="736"/>
      <source>Aragonese</source>
      <translation>Aragoński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="737"/>
      <source>Aragonese (Spain)</source>
      <translation>Aragoński (Hiszpania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="738"/>
      <source>Arabic</source>
      <translation>Arabski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="739"/>
      <source>Arabic (United Arab Emirates)</source>
      <translation>Arabski (Zjednoczone Emiraty Arabskie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="740"/>
      <source>Arabic (Bahrain)</source>
      <translation>Arabski (Bahrajn)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="741"/>
      <source>Arabic (Algeria)</source>
      <translation>Arabski (Algieria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="743"/>
      <source>Arabic (India)</source>
      <translation>Arabski (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="744"/>
      <source>Arabic (Iraq)</source>
      <translation>Arabski (Irak)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="745"/>
      <source>Arabic (Jordan)</source>
      <translation>Arabski (Jordania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="746"/>
      <source>Arabic (Kuwait)</source>
      <translation>Arabski (Kuwejt)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="747"/>
      <source>Arabic (Lebanon)</source>
      <translation>Arabski (Liban)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="748"/>
      <source>Arabic (Libya)</source>
      <translation>Arabski (Libia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="749"/>
      <source>Arabic (Morocco)</source>
      <translation>Arabski (Maroko)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="750"/>
      <source>Arabic (Oman)</source>
      <translation>Arabski (Oman)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="751"/>
      <source>Arabic (Qatar)</source>
      <translation>Arabski (Katar)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="752"/>
      <source>Arabic (Saudi Arabia)</source>
      <translation>Arabski (Arabia Saudyjska)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="753"/>
      <source>Arabic (Sudan)</source>
      <translation>Arabski (Sudan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="754"/>
      <source>Arabic (Syria)</source>
      <translation>Arabski (Syria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="755"/>
      <source>Arabic (Tunisia)</source>
      <translation>Arabski (Tunezja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="756"/>
      <source>Arabic (Yemen)</source>
      <translation>Arabski (Jemen)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="757"/>
      <source>Belarusian</source>
      <translation>Białoruski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="758"/>
      <source>Belarusian (Belarus)</source>
      <translation>Białoruski (Białoruś)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="759"/>
      <source>Belarusian (Russia)</source>
      <translation>Białoruski (Rosja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="760"/>
      <source>Bulgarian</source>
      <translation>Bułgarski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="761"/>
      <source>Bulgarian (Bulgaria)</source>
      <translation>Bułgarski (Bułgaria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="762"/>
      <source>Bangla</source>
      <translation>Bangla</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="763"/>
      <source>Bangla (Bangladesh)</source>
      <translation>Bangla (Bangladesz)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="764"/>
      <source>Bangla (India)</source>
      <translation>Bangla (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="765"/>
      <source>Tibetan</source>
      <translation>Tybetański</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="766"/>
      <source>Tibetan (China)</source>
      <translation>Tybetański (Chiny)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="767"/>
      <source>Tibetan (India)</source>
      <translation>Tybetański (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="768"/>
      <source>Breton</source>
      <translation>Bretoński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="769"/>
      <source>Breton (France)</source>
      <translation>Bretoński (Francja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="770"/>
      <source>Bosnian</source>
      <translation>Bośniacki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="771"/>
      <source>Bosnian (Bosnia/Herzegovina)</source>
      <translation>Bośniacki (Bosnia/Hercegowina)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="772"/>
      <source>Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)</source>
      <translation>Bośniacki (Bośnia/Hercegowina - cyrylica)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="773"/>
      <source>Catalan</source>
      <translation>Kataloński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="774"/>
      <source>Catalan (Spain)</source>
      <translation>Kataloński (Hiszpania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="775"/>
      <source>Catalan (Spain - Valencian)</source>
      <translation>Kataloński (Hiszpania - Walencja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="776"/>
      <source>Central Kurdish</source>
      <translation>Centralny Kurdyjski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="777"/>
      <source>Central Kurdish (Iraq)</source>
      <translation>Kurdyjski centralny (Irak)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="778"/>
      <source>Czech</source>
      <translation>Czeski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="779"/>
      <source>Czech (Czechia)</source>
      <translation>Czeski (Czechy)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="780"/>
      <source>Danish</source>
      <translation>Duński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="781"/>
      <source>Danish (Denmark)</source>
      <translation>Duński (Dania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="782"/>
      <source>German</source>
      <translation>Niemiecki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="783"/>
      <source>German (Austria)</source>
      <translation>Niemiecki (Austria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="784"/>
      <source>German (Austria, revised by F M Baumann)</source>
      <translation>Niemiecki (Austria, poprawiony przez F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="785"/>
      <source>German (Belgium)</source>
      <translation>Niemiecki (Belgia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="786"/>
      <source>German (Switzerland)</source>
      <translation>Niemiecki (Szwajcaria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="787"/>
      <source>German (Switzerland, revised by F M Baumann)</source>
      <translation>Niemiecki (Szwajcaria, poprawiony przez F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="788"/>
      <source>German (Germany/Belgium/Luxemburg)</source>
      <translation>Niemiecki (Niemcy/Belgia/Luksemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="789"/>
      <source>German (Germany/Belgium/Luxemburg, revised by F M Baumann)</source>
      <translation>Niemiecki (Niemcy/Belgia/Luksemburg, zmieniony przez F M Baumann)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="790"/>
      <source>German (Liechtenstein)</source>
      <translation>Niemiecki (Liechtenstein)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="791"/>
      <source>German (Luxembourg)</source>
      <translation>Niemiecki (Luksemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="794"/>
      <source>Greek</source>
      <translation>Grecki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="795"/>
      <source>Greek (Greece)</source>
      <translation>Grecki (Grecja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="796"/>
      <source>English</source>
      <translation>Angielski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="797"/>
      <source>English (Antigua/Barbuda)</source>
      <translation>Angielski (Antigua/Barbuda)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="798"/>
      <source>English (Australia)</source>
      <translation>Angielski (Australia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="800"/>
      <source>English (Bahamas)</source>
      <translation>Angielski (Bahamy)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="801"/>
      <source>English (Botswana)</source>
      <translation>Angielski (Botswana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="802"/>
      <source>English (Belize)</source>
      <translation>Angielski (Belize)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="742"/>
      <source>Arabic (Egypt)</source>
      <translation>Arabski (Egipt)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="409"/>
      <source>Mudlet chat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="410"/>
      <source>Open a link to the Mudlet server on Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="452"/>
      <source>Packages (exp.)</source>
      <translation>Pakiety (eksp.)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="459"/>
      <source>Package Manager (experimental)</source>
      <translation>Menedżer pakietów (eksperymentalny)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="461"/>
      <source>Packages (exp.)</source>
      <comment>exp. stands for experimental; shortened so it doesn&apos;t make buttons huge in the main interface</comment>
      <translation>Pakiety (eksp.)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="490"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="502"/>
      <source>Report issue</source>
      <translation>Zgłoś problem</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="792"/>
      <source>Dzongkha</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="793"/>
      <source>Dzongkha (Bhutan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="799"/>
      <source>English (Australia, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="803"/>
      <source>English (Canada)</source>
      <translation>Angielski (Kanada)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="804"/>
      <source>English (Canada, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="805"/>
      <source>English (Denmark)</source>
      <translation>Angielski (Dania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="806"/>
      <source>English (United Kingdom)</source>
      <translation>Angielski (Wielka Brytania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="807"/>
      <source>English (United Kingdom, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="808"/>
      <source>English (United Kingdom - &apos;ise&apos; not &apos;ize&apos;)</source>
      <comment>This dictionary prefers the British &apos;ise&apos; form over the American &apos;ize&apos; one.</comment>
      <translation>Angielski (Wielka Brytania - &apos;Ise&apos; Nie &apos;Ize&apos;)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="809"/>
      <source>English (Ghana)</source>
      <translation>Angielski (Ghana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="810"/>
      <source>English (Hong Kong SAR China)</source>
      <translation>Angielski (Hong Kong SAR China)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="811"/>
      <source>English (Ireland)</source>
      <translation>Angielski (Irlandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="812"/>
      <source>English (India)</source>
      <translation>Angielski (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="813"/>
      <source>English (Jamaica)</source>
      <translation>Angielski (Jamajka)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="814"/>
      <source>English (Namibia)</source>
      <translation>Angielski (Namibia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="815"/>
      <source>English (Nigeria)</source>
      <translation>Angielski (Nigeria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="816"/>
      <source>English (New Zealand)</source>
      <translation>Angielski (Nowa Zelandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="817"/>
      <source>English (Philippines)</source>
      <translation>Angielski (Filipiny)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="818"/>
      <source>English (Singapore)</source>
      <translation>Angielski (Singapur)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="819"/>
      <source>English (Trinidad/Tobago)</source>
      <translation>Angielski (Trynidad/Tobago)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="820"/>
      <source>English (United States)</source>
      <translation>Angielski (Stany Zjednoczone)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="821"/>
      <source>English (United States, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="822"/>
      <source>English (South Africa)</source>
      <translation>Angielski (Republika Południowej Afryki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="823"/>
      <source>English (Zimbabwe)</source>
      <translation>Angielski (Zimbabwe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="824"/>
      <source>Spanish</source>
      <translation>Hiszpański</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="825"/>
      <source>Spanish (Argentina)</source>
      <translation>Hiszpański (Argentyna)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="826"/>
      <source>Spanish (Bolivia)</source>
      <translation>Hiszpański (Boliwia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="827"/>
      <source>Spanish (Chile)</source>
      <translation>Hiszpański (Chile)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="828"/>
      <source>Spanish (Colombia)</source>
      <translation>Hiszpański (Kolumbia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="829"/>
      <source>Spanish (Costa Rica)</source>
      <translation>Hiszpański (Kostaryka)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="830"/>
      <source>Spanish (Cuba)</source>
      <translation>Hiszpański (Kuba)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="831"/>
      <source>Spanish (Dominican Republic)</source>
      <translation>Hiszpański (Dominikana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="832"/>
      <source>Spanish (Ecuador)</source>
      <translation>Hiszpański (Ekwador)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="833"/>
      <source>Spanish (Spain)</source>
      <translation>Hiszpański (Hiszpania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="834"/>
      <source>Spanish (Guatemala)</source>
      <translation>Hiszpański (Gwatemala)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="835"/>
      <source>Spanish (Honduras)</source>
      <translation>Hiszpański (Honduras)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="836"/>
      <source>Spanish (Mexico)</source>
      <translation>Hiszpański (Meksyk)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="837"/>
      <source>Spanish (Nicaragua)</source>
      <translation>Hiszpański (Nikaragua)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="838"/>
      <source>Spanish (Panama)</source>
      <translation>Hiszpański (Panama)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="839"/>
      <source>Spanish (Peru)</source>
      <translation>Hiszpański (Peru)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="840"/>
      <source>Spanish (Puerto Rico)</source>
      <translation>Hiszpański (Portoryko)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="841"/>
      <source>Spanish (Paraguay)</source>
      <translation>Hiszpański (Paragwaj)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="842"/>
      <source>Spanish (El Savador)</source>
      <translation>Hiszpański (Salwador)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="843"/>
      <source>Spanish (United States)</source>
      <translation>Hiszpański (Stany Zjednoczone)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="844"/>
      <source>Spanish (Uruguay)</source>
      <translation>Hiszpański (Urugwaj)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="845"/>
      <source>Spanish (Venezuela)</source>
      <translation>Hiszpański (Wenezuela)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="846"/>
      <source>Estonian</source>
      <translation>Estoński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="847"/>
      <source>Estonian (Estonia)</source>
      <translation>Estoński (Estonia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="848"/>
      <source>Basque</source>
      <translation>Baskijski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="849"/>
      <source>Basque (Spain)</source>
      <translation>Baskijski (Hiszpania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="850"/>
      <source>Basque (France)</source>
      <translation>Baskijski (Francja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="851"/>
      <location filename="../src/mudlet.cpp" line="852"/>
      <source>Finnish</source>
      <translation>Fiński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="853"/>
      <source>French</source>
      <translation>Francuski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="854"/>
      <source>French (Belgium)</source>
      <translation>Francuski (Belgia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="855"/>
      <source>French (Catalan)</source>
      <translation>Francuski (Kataloński)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="856"/>
      <source>French (Switzerland)</source>
      <translation>Francuski (Szwajcaria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="857"/>
      <source>French (France)</source>
      <translation>Francuski (Francja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="858"/>
      <source>French (Luxemburg)</source>
      <translation>Francuski (Luksemburg)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="859"/>
      <source>French (Monaco)</source>
      <translation>Francuski (Monako)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="860"/>
      <source>Gaelic</source>
      <translation>Celtycki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="861"/>
      <source>Gaelic (United Kingdom {Scots})</source>
      <translation>Celtycki (Wielka Brytania {Szkocja})</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="862"/>
      <source>Galician</source>
      <translation>Galicyjski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="863"/>
      <source>Galician (Spain)</source>
      <translation>Galicyjski (Hiszpania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="864"/>
      <location filename="../src/mudlet.cpp" line="869"/>
      <source>Guarani</source>
      <translation>Guarani</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="865"/>
      <location filename="../src/mudlet.cpp" line="870"/>
      <source>Guarani (Paraguay)</source>
      <translation>Guarani (Paragwaj)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="866"/>
      <source>Gujarati</source>
      <translation>Gujarati</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="867"/>
      <source>Gujarati (India)</source>
      <translation>Gujarati (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="871"/>
      <source>Hebrew</source>
      <translation>Hebrajski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="872"/>
      <source>Hebrew (Israel)</source>
      <translation>Hebrajski (Izrael)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="873"/>
      <source>Hindi</source>
      <translation>Hinduski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="874"/>
      <source>Hindi (India)</source>
      <translation>Hinduski (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="875"/>
      <source>Croatian</source>
      <translation>Chorwacki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="876"/>
      <source>Croatian (Croatia)</source>
      <translation>Chorwacki (Chorwacja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="877"/>
      <source>Hungarian</source>
      <translation>Węgierski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="878"/>
      <source>Hungarian (Hungary)</source>
      <translation>Węgierski (Węgry)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="879"/>
      <source>Armenian</source>
      <translation>Armeński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="880"/>
      <source>Armenian (Armenia)</source>
      <translation>Armeński (Armenia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="881"/>
      <source>Indonesian</source>
      <translation>Indonezyjski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="882"/>
      <source>Indonesian (Indonesia)</source>
      <translation>Indonezyjski (Indonezja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="883"/>
      <source>Interlingue</source>
      <comment>formerly known as Occidental, and not to be mistaken for Interlingua</comment>
      <translation>Interlingue</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="884"/>
      <source>Icelandic</source>
      <translation>Islandzki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="885"/>
      <source>Icelandic (Iceland)</source>
      <translation>Islandzki (Islandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="886"/>
      <source>Italian</source>
      <translation>Włoski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="887"/>
      <source>Italian (Switzerland)</source>
      <translation>Włoski (Szwajcaria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="888"/>
      <source>Italian (Italy)</source>
      <translation>Włoski (Włochy)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="889"/>
      <source>Kazakh</source>
      <translation>Kazachski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="890"/>
      <source>Kazakh (Kazakhstan)</source>
      <translation>Kazachski (Kazachstan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="891"/>
      <source>Kurmanji</source>
      <translation>Okręg wyborczy Kurmanji</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="892"/>
      <source>Kurmanji {Latin-alphabet Kurdish}</source>
      <translation>Okręg wyborczy Kurmanji {Latin-alphabet Kurdish}</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="893"/>
      <source>Korean</source>
      <translation>Koreański</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="894"/>
      <source>Korean (South Korea)</source>
      <translation>Koreański (Korea Południowa)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="895"/>
      <source>Kurdish</source>
      <translation>Kurdyjski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="896"/>
      <source>Kurdish (Syria)</source>
      <translation>Kurdyjski (Syria)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="897"/>
      <source>Kurdish (Turkey)</source>
      <translation>Kurdyjski (Turcja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="898"/>
      <source>Lao</source>
      <translation>Lao</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="899"/>
      <source>Lao (Laos)</source>
      <translation>Laoty (Laos)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="900"/>
      <source>Lithuanian</source>
      <translation>Litewski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="901"/>
      <source>Lithuanian (Lithuania)</source>
      <translation>Litewski (Litwa)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="902"/>
      <source>Latvian</source>
      <translation>Łotewski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="903"/>
      <source>Latvian (Latvia)</source>
      <translation>Łotewski (Łotwa)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="904"/>
      <source>Malayalam</source>
      <translation>Malezja</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="905"/>
      <source>Malayalam (India)</source>
      <translation>Malajalam (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="906"/>
      <source>Norwegian Bokmål</source>
      <translation>Norweski Bokmål</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="907"/>
      <source>Norwegian Bokmål (Norway)</source>
      <translation>Norwegian Bokmål (Norwegia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="908"/>
      <source>Nepali</source>
      <translation>Nepalski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="909"/>
      <source>Nepali (Nepal)</source>
      <translation>Nepal (Nepal)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="910"/>
      <source>Dutch</source>
      <translation>Holenderski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="911"/>
      <source>Dutch (Netherlands Antilles)</source>
      <translation>Holenderski (Antyle Holenderskie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="912"/>
      <source>Dutch (Aruba)</source>
      <translation>Holenderski (Aruba)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="913"/>
      <source>Dutch (Belgium)</source>
      <translation>Holenderski (Belgia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="914"/>
      <source>Dutch (Netherlands)</source>
      <translation>Holenderski (Holandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="915"/>
      <source>Dutch (Suriname)</source>
      <translation>Holenderski (Surinam)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="916"/>
      <source>Norwegian Nynorsk</source>
      <translation>Norweski Nynorsk</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="917"/>
      <source>Norwegian Nynorsk (Norway)</source>
      <translation>Norweski Nynorsk (Norwegia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="918"/>
      <source>Occitan</source>
      <translation>Prowansalski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="919"/>
      <source>Occitan (France)</source>
      <translation>Prowansalski (Francja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="920"/>
      <source>Polish</source>
      <translation>Polski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="921"/>
      <source>Polish (Poland)</source>
      <translation>Polski (Polska)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="922"/>
      <source>Portuguese</source>
      <translation>Portugalski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="923"/>
      <source>Portuguese (Brazil)</source>
      <translation>Portugalski (Brazylia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="924"/>
      <source>Portuguese (Portugal)</source>
      <translation>Portugalski (Portugalia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="925"/>
      <source>Romanian</source>
      <translation>Rumuński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="926"/>
      <source>Romanian (Romania)</source>
      <translation>Rumuński (Rumunia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="927"/>
      <source>Russian</source>
      <translation>Rosyjski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="928"/>
      <source>Russian (Russia)</source>
      <translation>Rosyjski (Rosja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="929"/>
      <source>Northern Sami</source>
      <translation>Norweski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="930"/>
      <source>Northern Sami (Finland)</source>
      <translation>Północne Sami (Finlandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="931"/>
      <source>Northern Sami (Norway)</source>
      <translation>Północne Sami (Norwegia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="932"/>
      <source>Northern Sami (Sweden)</source>
      <translation>Północne Sami (Szwecja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="933"/>
      <source>Shtokavian</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication</comment>
      <translation>Shtokavian</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="934"/>
      <source>Shtokavian (former state of Yugoslavia)</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication</comment>
      <translation>Shtokavian (były stan Jugosławii)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="935"/>
      <source>Sinhala</source>
      <translation>Syngaleski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="936"/>
      <source>Sinhala (Sri Lanka)</source>
      <translation>Syngaleski (Sri Lanka)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="937"/>
      <source>Slovak</source>
      <translation>Słowacki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="938"/>
      <source>Slovak (Slovakia)</source>
      <translation>Słowacki (Słowacja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="939"/>
      <source>Slovenian</source>
      <translation>Słoweński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="940"/>
      <source>Slovenian (Slovenia)</source>
      <translation>Słoweński (Słowenia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="941"/>
      <source>Somali</source>
      <translation>Somalijski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="942"/>
      <source>Somali (Somalia)</source>
      <translation>Somalijski (Somalia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="943"/>
      <source>Albanian</source>
      <translation>Albański</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="944"/>
      <source>Albanian (Albania)</source>
      <translation>Albański (Albania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="945"/>
      <source>Serbian</source>
      <translation>Serbski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="946"/>
      <source>Serbian (Montenegro)</source>
      <translation>Serbski (Czarnogóra)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="947"/>
      <source>Serbian (Serbia)</source>
      <translation>Serbski (Serbia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="948"/>
      <source>Serbian (Serbia - Latin-alphabet)</source>
      <translation>Serbski (Serbia - alfabet łaciński)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="949"/>
      <source>Serbian (former state of Yugoslavia)</source>
      <translation>Serbski (dawny stan Jugosławii)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="950"/>
      <source>Swati</source>
      <translation>Swati</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="951"/>
      <source>Swati (Swaziland)</source>
      <translation>Swati (Suazi)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="952"/>
      <source>Swati (South Africa)</source>
      <translation>Swati (Republika Południowej Afryki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="953"/>
      <source>Swedish</source>
      <translation>Szwedzki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="954"/>
      <source>Swedish (Sweden)</source>
      <translation>Szwedzki (Szwecja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="955"/>
      <source>Swedish (Finland)</source>
      <translation>Szwedzki (Finlandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="956"/>
      <source>Swahili</source>
      <translation>Suahili</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="957"/>
      <source>Swahili (Kenya)</source>
      <translation>Suahili (Kenia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="958"/>
      <source>Swahili (Tanzania)</source>
      <translation>Suahili (Tanzania)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="971"/>
      <source>Turkish</source>
      <translation>Turecki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="959"/>
      <source>Telugu</source>
      <translation>Telugu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="960"/>
      <source>Telugu (India)</source>
      <translation>Telugu (Indie)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="961"/>
      <source>Thai</source>
      <translation>Tajski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="962"/>
      <source>Thai (Thailand)</source>
      <translation>Tajski (Tajlandia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="963"/>
      <source>Tigrinya</source>
      <translation>Tigrinia</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="964"/>
      <source>Tigrinya (Eritrea)</source>
      <translation>Tigrinya (Erytrea)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="965"/>
      <source>Tigrinya (Ethiopia)</source>
      <translation>Tigrinya (Etiopia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="966"/>
      <source>Turkmen</source>
      <translation>Turkmeński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="967"/>
      <source>Turkmen (Turkmenistan)</source>
      <translation>Turkmenianie (Turkmenistan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="968"/>
      <source>Tswana</source>
      <translation>Tswana</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="969"/>
      <source>Tswana (Botswana)</source>
      <translation>Tswana (Botswana)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="970"/>
      <source>Tswana (South Africa)</source>
      <translation>Tswana (Republika Południowej Afryki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="973"/>
      <source>Tsonga</source>
      <translation>Tsonga</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="974"/>
      <source>Tsonga (South Africa)</source>
      <translation>Tsonga (Republika Południowej Afryki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="975"/>
      <source>Ukrainian</source>
      <translation>Ukraiński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="976"/>
      <source>Ukrainian (Ukraine)</source>
      <translation>Ukraiński (Ukraina)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="977"/>
      <source>Uzbek</source>
      <translation>Uzbecki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="978"/>
      <source>Uzbek (Uzbekistan)</source>
      <translation>Uzbecki (Uzbekistan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="979"/>
      <source>Venda</source>
      <translation>Venda</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="980"/>
      <source>Vietnamese</source>
      <translation>Wietnamski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="981"/>
      <source>Vietnamese (Vietnam)</source>
      <translation>Wietnamski (Wietnam)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="984"/>
      <source>Walloon</source>
      <translation>Waloński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="985"/>
      <source>Xhosa</source>
      <translation>Xhosa</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="986"/>
      <source>Yiddish</source>
      <translation>Jidysz</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="987"/>
      <source>Chinese</source>
      <translation>Chiński</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="988"/>
      <source>Chinese (China - simplified)</source>
      <translation>Chiński (Chiny - uproszczony)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="989"/>
      <source>Chinese (Taiwan - traditional)</source>
      <translation>Chiński (Tajwan - tradycyjny)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="990"/>
      <source>Zulu</source>
      <translation>Zulu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4460"/>
      <source>Hide tray icon</source>
      <translation>Ukryj ikonę z zasobnika</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4465"/>
      <source>Quit Mudlet</source>
      <translation>Zakończ Mudlet</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="238"/>
      <source>hh:mm:ss</source>
      <comment>Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&amp;) for the gory details...!</comment>
      <translation>hh:mm:ss</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="292"/>
      <source>Main Toolbar</source>
      <translation>Główny Pasek Narzędzi</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="333"/>
      <location filename="../src/mudlet.cpp" line="340"/>
      <location filename="../src/mudlet.cpp" line="342"/>
      <source>Connect</source>
      <translation>Połącz</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="345"/>
      <source>Disconnect</source>
      <translation>Odłącz</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="404"/>
      <source>Open Discord</source>
      <translation>Otwórz Discord</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="416"/>
      <source>Open IRC</source>
      <translation>Otwórz IRC</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="352"/>
      <source>Triggers</source>
      <translation>Wyzwalacze</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="353"/>
      <source>Show and edit triggers</source>
      <translation>Pokaż i edytuj wyzwalacze</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="360"/>
      <source>Aliases</source>
      <translation>Aliasy</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="361"/>
      <source>Show and edit aliases</source>
      <translation>Pokazuj i edytuj aliasy</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="366"/>
      <source>Timers</source>
      <translation>Liczniki czasu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="367"/>
      <source>Show and edit timers</source>
      <translation>Pokaż i edytuj timery</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="372"/>
      <source>Buttons</source>
      <translation>Przyciski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="373"/>
      <source>Show and edit easy buttons</source>
      <translation>Pokaż i edytuj przyciski</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="378"/>
      <source>Scripts</source>
      <translation>Skrypty</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="379"/>
      <source>Show and edit scripts</source>
      <translation>Pokazuj i edytuj skrypty</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="384"/>
      <source>Keys</source>
      <translation>Klawisze</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="385"/>
      <source>Show and edit keys</source>
      <translation>Pokaż i edytuj klawisze</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="390"/>
      <source>Variables</source>
      <translation>Zmienne</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="391"/>
      <source>Show and edit Lua variables</source>
      <translation>Pokaż i edytuj zmienne Lua</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="424"/>
      <source>Map</source>
      <translation>Mapa</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="425"/>
      <source>Show/hide the map</source>
      <translation>Pokaż/ukryj mapę</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="430"/>
      <source>Manual</source>
      <translation>Podręcznik</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="431"/>
      <source>Browse reference material and documentation</source>
      <translation>Przeglądaj materiały i dokumentację referencyjną</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="436"/>
      <source>Settings</source>
      <translation>Ustawienia</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="437"/>
      <source>See and edit profile preferences</source>
      <translation>Wyświetlanie i edytowanie preferencji profilu</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="445"/>
      <source>Notepad</source>
      <translation>Notatnik</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="446"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation>Otwórz notatnik, w którym można przechowywać notatki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="464"/>
      <source>Module Manager</source>
      <translation>Menedżer modułów</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="468"/>
      <source>Package Exporter</source>
      <translation>Eksporter pakietów</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="478"/>
      <source>Replay</source>
      <translation>Powtórka</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="483"/>
      <source>Reconnect</source>
      <translation>Podłącz ponownie</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="484"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation>Odłącza cię od gry i łączy się ponownie</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="489"/>
      <source>MultiView</source>
      <translation>Multiview</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="513"/>
      <location filename="../src/mudlet.cpp" line="3565"/>
      <source>About</source>
      <translation>O programie</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="514"/>
      <location filename="../src/mudlet.cpp" line="3548"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation>&lt;p&gt;Poinformuj się o tej wersji Mudlet, o ludziach, którzy go stworzyli i o licencji, na której możesz się nim dzielić.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="972"/>
      <source>Turkish (Turkey)</source>
      <translation>Turecki (Turcja)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="982"/>
      <source>Vietnamese (DauCu variant - old-style diacritics)</source>
      <translation>Wietnamski (wariant DauCu - znaki diakrytyczne starego stylu)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="983"/>
      <source>Vietnamese (DauMoi variant - new-style diacritics)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="992"/>
      <source>ASCII (Basic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ASCII (Basic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="993"/>
      <source>UTF-8 (Recommended)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>UTF-8 (zalecane)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="998"/>
      <source>ISO 8859-1 (Western European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-1 (Western European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="999"/>
      <source>ISO 8859-2 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-2 (Centralna Europa/Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1000"/>
      <source>ISO 8859-3 (South European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-3 (Południowoeuropejski/South European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1001"/>
      <source>ISO 8859-4 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-4 (kraje bałtyckie/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1002"/>
      <source>ISO 8859-5 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-5 (cyrylica/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1003"/>
      <source>ISO 8859-6 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-6 (arabski/Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1004"/>
      <source>ISO 8859-7 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-7 (grecki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1005"/>
      <source>ISO 8859-8 (Hebrew Visual)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-8 (hebrajski wizualny)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1006"/>
      <source>ISO 8859-9 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-9 (turecki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1007"/>
      <source>ISO 8859-10 (Nordic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-10 (kraje bałtyckie/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1008"/>
      <source>ISO 8859-11 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-11 (Latin/tajski)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1009"/>
      <source>ISO 8859-13 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-13 (kraje bałtyckie/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1010"/>
      <source>ISO 8859-14 (Celtic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-14 (celtycki)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1011"/>
      <source>ISO 8859-15 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-15 (Zachodni)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1012"/>
      <source>ISO 8859-16 (Romanian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-16 (rumuński/Romanian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1013"/>
      <location filename="../src/mudlet.cpp" line="1014"/>
      <source>CP437 (OEM Font)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP437 (OEM Font)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1015"/>
      <location filename="../src/mudlet.cpp" line="1016"/>
      <source>CP667 (Mazovia)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP667 (Mazovia)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1017"/>
      <location filename="../src/mudlet.cpp" line="1018"/>
      <source>CP737 (DOS Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP737 (DOS Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1019"/>
      <source>CP850 (Western Europe)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP850 (Europa Zachodnia/Western Europe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1020"/>
      <source>CP866 (Cyrillic/Russian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP866 (cyrylica/rosyjski/Cyrillic/Russian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1021"/>
      <location filename="../src/mudlet.cpp" line="1022"/>
      <source>CP869 (DOS Greek 2)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP869 (DOS Greek 2)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1023"/>
      <source>CP1161 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP1161 (Latin/Thai)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1024"/>
      <source>KOI8-R (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>KOI8-R (z cyrylicą/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1025"/>
      <source>KOI8-U (Cyrillic/Ukrainian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>KOI8-U (cyrylica/ukraiński/Cyrillic/Ukrainian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1026"/>
      <source>MACINTOSH</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>MACINTOSH</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1027"/>
      <source>WINDOWS-1250 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1250 (środkowoeuropejskie/Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1028"/>
      <source>WINDOWS-1251 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1251 (cyrylica/Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1029"/>
      <source>WINDOWS-1252 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1252 (zachodni/Western)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1030"/>
      <source>WINDOWS-1253 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1253 (grecki/Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1031"/>
      <source>WINDOWS-1254 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1254 (turecki/Turkish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1032"/>
      <source>WINDOWS-1255 (Hebrew)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1255 (hebrajski/Hebrew)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1033"/>
      <source>WINDOWS-1256 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1256 (arabski/Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1034"/>
      <source>WINDOWS-1257 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1257 (kraje bałtyckie/Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1035"/>
      <source>WINDOWS-1258 (Vietnamese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1258 (wietnamski/Vietnamese)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2586"/>
      <source>Central Debug Console</source>
      <translation>Centralna konsola debugowania</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="525"/>
      <location filename="../src/mudlet.cpp" line="526"/>
      <source>Toggle Full Screen View</source>
      <translation>Przełącz widok pełnoekranowy</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="506"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation>Publiczna kompilacja testowa szybciej zyskuje nowe funkcje, a Ty pomagasz nam szybciej znaleźć w nich problemy. Zauważył coś dziwnego? Daj nam znać jak najszybciej!</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="994"/>
      <source>GBK (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>GBK (Chinese)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="995"/>
      <source>GB18030 (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>GB18030 (Chinese)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="996"/>
      <source>Big5-ETen (Taiwan)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>Big5-ETen (Taiwan)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="997"/>
      <source>Big5-HKSCS (Hong Kong)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>Big5-HKSCS (Hong Kong)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1597"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Disabled until a profile is loaded.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Załaduj powtórkę Mudleta.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Wyłączone, dopóki profil nie zostanie załadowany.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1633"/>
      <location filename="../src/mudlet.cpp" line="2976"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Załaduj powtórkę Mudleta.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2355"/>
      <source>%1 - notes</source>
      <translation>%1 - notatki</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2439"/>
      <source>Select Replay</source>
      <translation>Wybierz powtórkę</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2441"/>
      <source>*.dat</source>
      <translation>*.dat</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2743"/>
      <source>[  OK  ]  - Profile &quot;%1&quot; loaded in offline mode.</source>
      <translation>[  OK  ]  - Profil &quot;%1&quot; uruchomiony w trybie offline.</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2878"/>
      <source>&lt;p&gt;Cannot load a replay as one is already in progress in this or another profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Nie można załadować powtórki, ponieważ w tym lub innym profilu jest już uruchomiona inna.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2895"/>
      <source>Faster</source>
      <translation>Szybciej</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2898"/>
      <source>&lt;p&gt;Replay each step with a shorter time interval between steps.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Powtórz każdy krok z krótszym czasem między kolejnymi krokami.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2902"/>
      <source>Slower</source>
      <translation>Wolniej</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2905"/>
      <source>&lt;p&gt;Replay each step with a longer time interval between steps.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2915"/>
      <location filename="../src/mudlet.cpp" line="2984"/>
      <location filename="../src/mudlet.cpp" line="2997"/>
      <source>Speed: X%1</source>
      <translation>Prędkość: X%1</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2922"/>
      <location filename="../src/mudlet.cpp" line="2939"/>
      <source>Time: %1</source>
      <translation>Czas: %1</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3559"/>
      <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
      <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3577"/>
      <source>Review %n update(s)...</source>
      <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3582"/>
      <source>&lt;p&gt;Review the update(s) available...&lt;/p&gt;</source>
      <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3607"/>
      <source>Update installed - restart to apply</source>
      <translation>Aktualizacja zainstalowana - uruchom ponownie, aby zastosować</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3643"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress,
try again when it has finished.</source>
      <translation>[ OSTRZEŻENIE ] - Nie można odtworzyć powtórki, inna może być już w toku. Spróbuj ponownie po jej zakończeniu.</translation>
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
      <source>Remove packages</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>package_manager_unpack</name>
    <message>
      <location filename="../src/ui/package_manager_unpack.ui" line="24"/>
      <source>unpacking please wait ...</source>
      <translation>rozpakowywanie proszę czekać...</translation>
    </message>
  </context>
  <context>
    <name>profile_preferences</name>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="20"/>
      <source>Profile preferences</source>
      <translation>Preferencje profilu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="55"/>
      <source>General</source>
      <translation>Ogólne</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="61"/>
      <source>Icon sizes</source>
      <translation>Rozmiary ikon</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="67"/>
      <source>Icon size toolbars:</source>
      <translation>Rozmiar ikon:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="90"/>
      <source>Icon size in tree views:</source>
      <translation>Rozmiar ikony w widokach drzewa:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="110"/>
      <source>Show menu bar:</source>
      <translation>Pokaż pasek menu:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="121"/>
      <location filename="../src/ui/profile_preferences.ui" line="150"/>
      <source>Never</source>
      <translation>Nigdy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="126"/>
      <location filename="../src/ui/profile_preferences.ui" line="155"/>
      <source>Until a profile is loaded</source>
      <translation>Dopóki profil nie zostanie załadowany</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="131"/>
      <location filename="../src/ui/profile_preferences.ui" line="160"/>
      <source>Always</source>
      <translation>Zawsze</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="139"/>
      <source>Show main toolbar</source>
      <translation>Pokaż główny pasek narzędzi</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="249"/>
      <source>Allow server to install script packages</source>
      <translation>Zezwalaj serwerowi na instalowanie pakietów skryptów</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="276"/>
      <source>Game protocols</source>
      <translation>Protokoły gier</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="333"/>
      <location filename="../src/ui/profile_preferences.ui" line="3326"/>
      <source>Please reconnect to your game for the change to take effect</source>
      <translation>Aby zmiana została włączona, ponownie połącz się z grą,</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="349"/>
      <source>Log options</source>
      <translation>Opcje zapisu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="358"/>
      <source>Save log files in HTML format instead of plain text</source>
      <translation>Zapisuj pliki dziennika w formacie HTML zamiast zwykłego tekstu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="365"/>
      <source>Add timestamps at the beginning of log lines</source>
      <translation>Dodawanie znaczników czasu na początku każdego wiersza</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="372"/>
      <source>Save log files in:</source>
      <translation>Zapisz pliki dziennika w:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="389"/>
      <source>Browse...</source>
      <translation>Przeglądaj...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="396"/>
      <source>Reset</source>
      <translation>Resetuj</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="403"/>
      <source>Log format:</source>
      <translation>Format dziennika:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="416"/>
      <source>Log name:</source>
      <translation>Nazwa dziennika:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="436"/>
      <source>.txt</source>
      <translation>.txt</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="469"/>
      <source>Input line</source>
      <translation>Linia wprowadzania</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="478"/>
      <source>Input</source>
      <translation>Wejście</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="484"/>
      <source>use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly</source>
      <translation>użyj ścisłych zakończeń linii UNIX na poleceniach dla starych serwerów UNIX, które nie mogą poprawnie obsługiwać zakończenia linii systemu Windows</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="487"/>
      <source>Strict UNIX line endings</source>
      <translation>Ścisłe zakończenia linii UNIX</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="504"/>
      <source>Show the text you sent</source>
      <translation>Pokaż tekst który wysłałeś</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="494"/>
      <source>Auto clear the input line after you sent text</source>
      <translation>Automatyczne czyszczenie wiersza wprowadzania po wysłaniu tekstu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="201"/>
      <source>&lt;p&gt;If you are playing a non-English game and seeing � instead of text, or special letters like &lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt; aren&apos;t showing right - try changing the encoding to UTF-8 or to one suggested by your game.&lt;/p&gt;&lt;p&gt;For some encodings on some Operating Systems Mudlet itself has to provide the codec needed; if that is the case for this Mudlet then there will be a &lt;tt&gt;m &lt;/tt&gt; prefixed applied to those encoding names (so if they have errors the blame can be applied correctly!)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="312"/>
      <source>&lt;p&gt;Enables MSP - provides Mud Sound Protocol messages during game play for supported games&lt;/p&gt;</source>
      <translation>&lt;p&gt;Włącza MSP - zapewnia komunikaty Mud Sound Protocol podczas gry dla obsługiwanych gier&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="355"/>
      <source>&lt;p&gt;When checked will cause the date-stamp named log file to be HTML (file extension &apos;.html&apos;) which can convey color, font and other formatting information rather than a plain text (file extension &apos;.txt&apos;) format.  If changed while logging is already in progress it is necessary to stop and restart logging for this setting to take effect in a new log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="524"/>
      <source>React to all keybindings on the same key</source>
      <translation>Reaguj na wszystkie wiązania klawiszy na tym samym klawiszu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="534"/>
      <source>Command separator:</source>
      <translation>Separator poleceń:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="544"/>
      <source>Enter text to separate commands with or leave blank to disable</source>
      <translation>Wprowadź tekst do oddzielania poleceń lub pozostaw puste aby wyłączyć oddzielanie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="551"/>
      <source>Command line minimum height in pixels:</source>
      <translation>Minimalna wysokość wiersza poleceń w pikselach:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="673"/>
      <source>Main display</source>
      <translation>Główny ekran</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="685"/>
      <source>Font</source>
      <translation>Czcionka</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="691"/>
      <source>Font:</source>
      <translation>Czcionka:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="714"/>
      <source>Size:</source>
      <translation>Wielkość:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="749"/>
      <source>Use anti aliasing on fonts. Smoothes fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry. </source>
      <translation>Użyj antyaliasowania czcionek. Wygładza czcionki, jeśli masz wysoką rozdzielczość ekranu i możesz używać większych czcionek. Należy zauważyć, że w przypadku niskich rozdzielczości i małych rozmiarów czcionek czcionka staje się rozmyta. </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="752"/>
      <source>Enable anti-aliasing</source>
      <translation>Włącz antyaliasing</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="768"/>
      <source>Display Border</source>
      <translation>Wyświetl obramowanie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="789"/>
      <source>Top border height:</source>
      <translation>Wysokość górnej krawędzi:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="833"/>
      <source>Left border width:</source>
      <translation>Szerokość lewej krawędzi:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="877"/>
      <source>Bottom border height:</source>
      <translation>Wysokość dolnej krawędzi:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="921"/>
      <source>Right border width:</source>
      <translation>Szerokość prawej krawędzi:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="962"/>
      <source>Word wrapping</source>
      <translation>Zawijanie wyrazów</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="986"/>
      <source>Wrap lines at:</source>
      <translation>Zawijanie linii w:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1006"/>
      <location filename="../src/ui/profile_preferences.ui" line="1054"/>
      <source>characters</source>
      <translation>znaków</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1034"/>
      <source>Indent wrapped lines by:</source>
      <translation>Wcięcie zawiniętych wierszy przez:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1067"/>
      <source>Double-click</source>
      <translation>Podwójne kliknięcie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1073"/>
      <source>Stop selecting a word on these characters:</source>
      <translation>Przestań wybierać słowo na tych znakach:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1083"/>
      <source>&apos;&quot;</source>
      <translation>&apos;&quot;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1096"/>
      <source>Display options</source>
      <translation>Opcje wyświetlania</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1105"/>
      <source>Fix unnecessary linebreaks on GA servers</source>
      <translation>Napraw niepotrzebne podziały linii na serwerach GA</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1115"/>
      <source>Show Spaces/Tabs</source>
      <translation>Pokaż spacje/tabulację</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1125"/>
      <source>Use Mudlet on a netbook with a small screen</source>
      <translation>Użyj Mudlet komputerach z małym ekranem</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1135"/>
      <source>Show Line/Paragraphs</source>
      <translation>Pokaż Wiersze/Akapity</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1145"/>
      <source>Echo Lua errors to the main console</source>
      <translation>Błędy Echo Lua na głównej konsoli</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1152"/>
      <source>Make &apos;Ambiguous&apos; E. Asian width characters wide</source>
      <translation>Zrób &apos;Niejednoznaczne&apos; E. Znaki szerokości azjatyckiej szerokie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1186"/>
      <source>Editor</source>
      <translation>Edytor</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1192"/>
      <source>Theme</source>
      <translation>Szablon</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1262"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>Aktualizowanie skórek z colorsublime.github.io...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1302"/>
      <source>Color view</source>
      <translation>Widok koloru</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1308"/>
      <source>Select your color preferences</source>
      <translation>Wybierz swoje ustawienia kolorów</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1314"/>
      <source>Foreground:</source>
      <translation>Pierwszy plan:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1337"/>
      <source>Background:</source>
      <translation>Tło:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1357"/>
      <source>Command line foreground:</source>
      <translation>Kolor linii wiersza poleceń:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1374"/>
      <source>Command line background:</source>
      <translation>Kolor tła linii wiersza poleceń:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1391"/>
      <source>Command foreground:</source>
      <translation>Kolor poleceń:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1411"/>
      <source>Command background:</source>
      <translation>Kolor tła poleceń:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="282"/>
      <source>&lt;p&gt;Enables GMCP - note that if you have MSDP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation>&lt;p&gt;Włącza GMCP - należy pamiętać, że jeśli masz włączone również MSDP, niektóre serwery będą preferować jeden nad drugim&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="302"/>
      <source>&lt;p&gt;Enables MSDP - note that if you have GMCP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation>&lt;p&gt;Włącza MSDP - zwróć uwagę, że jeśli masz również włączony GMCP, niektóre serwery będą preferować jeden nad drugim&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="171"/>
      <source>Language &amp;&amp; data encoding</source>
      <translation>Kodowanie języka i danych</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="177"/>
      <source>Interface language:</source>
      <translation>Wybór języka:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="191"/>
      <source>Server data encoding:</source>
      <translation>Kodowanie danych serwera:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="213"/>
      <source>Please restart Mudlet to apply the new language</source>
      <translation>Uruchom ponownie Mudlet, aby zastosować nowy język</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="223"/>
      <source>Miscellaneous</source>
      <translation>Pozostałe</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="285"/>
      <source>Enable GMCP  (Generic Mud Communication Protocol)</source>
      <translation>Włącz GMCP (Generic Mud Communication Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="305"/>
      <source>Enable MSDP  (Mud Server Data Protocol)</source>
      <translation>Włącz MSDP (Mud Server Data Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="292"/>
      <source>&lt;p&gt;Enables MSSP - provides Mud Server Status Protocol information upon connection for supported games&lt;/p&gt;</source>
      <translation>&lt;p&gt;Włącza MSSP - zapewnia informacje o protokole Mud Server Status Protocol po połączeniu z grami go obsługującymi&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="295"/>
      <source>Enable MSSP  (Mud Server Status Protocol)</source>
      <translation>Włącz MSSP (Mud Server Status Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="501"/>
      <source>&lt;p&gt;Echo the text you send in the display box.&lt;/p&gt;&lt;p&gt;&lt;i&gt;This can be disabled by the game server if it negotiates to use the telnet ECHO option&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Powtórz tekst wysłany w polu wyświetlania.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Może to być wyłączone przez serwer gry, jeśli negocjuje użycie opcji telnet ECHO&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="521"/>
      <source>&lt;p&gt;Check all Key-bindings against key-presses.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Versions of Mudlet prior to &lt;b&gt;3.9.0&lt;/b&gt; would stop checking after the first matching combination of&lt;/i&gt; KeyCode &lt;i&gt;and&lt;/i&gt; KeyModifier &lt;i&gt;was found and then send the command and/or run the script of that Key-binding only.  This&lt;/i&gt; per-profile &lt;i&gt;option tells Mudlet to check and run the remaining matches; but, to retain compatibility with previous versions, defaults to the &lt;b&gt;un&lt;/b&gt;-checked state.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Sprawdź wszystkie przypisania klawiszy podczas naciskania ich.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Wersje Mudleta przed &lt;b&gt;3.9.0&lt;/b&gt; przestają sprawdzać po znalezieniu pierwszej pasującej kombinacji &lt;/i&gt; KeyCode &lt;i&gt;oraz&lt;/i&gt; KeyModifier &lt;i&gt;, po czym wysyłają komendę i/lub uruchamiają skrypt LUA przypisany do znalezionej kombinacji. Ta opcja,&lt;/i&gt; ustawiana osobna dla każdego profilu, &lt;i&gt;sprawia że zostaną sprawdzone wszystkie pozostałe dopasowania klawiszy. Dla zachowania kompatybilności ze starszymi wersjami, ta opcja domyślnie jest &lt;b&gt;od&lt;/b&gt;znaczona.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="577"/>
      <source>Spell checking</source>
      <translation>Sprawdzanie pisowni</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="583"/>
      <source>&lt;p&gt;This option controls spell-checking on the command line in the main console for this profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ta opcja kontroluje sprawdzanie pisowni w wierszu polecenia w głównej konsoli dla tego profilu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="586"/>
      <source>System/Mudlet dictionary:</source>
      <translation>Słownik system/mudlet:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="593"/>
      <source>&lt;p&gt;Select one dictionary which will be available on the command line and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz jeden słownik, który będzie dostępny w wierszu polecenia i w podsystemie lua.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="621"/>
      <source>User dictionary:</source>
      <translation>Słownik użytkownika:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="627"/>
      <source>&lt;p&gt;A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dostępny będzie słownik użytkownika specyficzny dla tego profilu. Będzie to w wierszu polecenia (słowa, które są w nim pojawią się z przerywanym podkreśleniem koloru cyjanu) i w podsystemie lua.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="630"/>
      <source>Profile</source>
      <translation>Proﬁl</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="637"/>
      <source>&lt;p&gt;A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dostępny będzie słownik użytkownika współużytkowane przez wszystkie profile (których opcja jest zaznaczona). Będzie to w wierszu polecenia (słowa, które są w nim pojawią się z przerywanym podkreśleniem koloru cyjanu) i w podsystemie lua.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="640"/>
      <source>Shared</source>
      <translation>Współdzielone</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="729"/>
      <source>The selected font doesn&apos;t work with Mudlet, please pick another</source>
      <translation>Wybrana czcionka nie pracuje z Mudlete, proszę wybrać inną</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="786"/>
      <location filename="../src/ui/profile_preferences.ui" line="802"/>
      <source>&lt;p&gt;Extra space to have before text on top - can be set to negative to move text up beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dodatkowe miejsce, które można mieć przed tekstem na górze - można ustawić na ujemny, aby przenieść tekst poza ekran&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="830"/>
      <location filename="../src/ui/profile_preferences.ui" line="846"/>
      <source>&lt;p&gt;Extra space to have before text on the left - can be set to negative to move text left beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dodatkowe miejsce przed tekstem po lewej stronie - można ustawić na ujemny, aby przenieść tekst w lewo poza ekran&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="874"/>
      <location filename="../src/ui/profile_preferences.ui" line="890"/>
      <source>&lt;p&gt;Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dodatkowe miejsce, aby mieć przed tekstem na dole - można ustawić na ujemny, aby umożliwić tekst zejść poza ekran&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="918"/>
      <location filename="../src/ui/profile_preferences.ui" line="934"/>
      <source>&lt;p&gt;Extra space to have before text on the right - can be set to negative to move text right beyond the screen&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dodatkowe miejsce, aby mieć przed tekstem po prawej stronie - można ustawić na negatywne, aby przenieść tekst bezpośrednio poza ekran&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1080"/>
      <source>&lt;p&gt;Enter the characters you&apos;d like double-clicking to stop selecting text on here. If you don&apos;t enter any, double-clicking on a word will only stop at a space, and will include characters like a double or a single quote. For example, double-clicking on the word &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; in the following will select &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;quot;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;quot;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;If you set the characters in the field to &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;quot;! &lt;/span&gt;which will mean it should stop selecting on &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; &amp;quot; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; ! then double-clicking on &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; will just select &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &amp;quot;&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;!&amp;quot;&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wprowadź znaki, które&apos;d jak dwukrotne kliknięcie, aby zatrzymać zaznaczanie tekstu tutaj. Jeśli nie&apos;t wprowadź dowolny, dwukrotne kliknięcie wyrazu zatrzyma się tylko na spacji i będzie zawierać znaki takie jak podwójny lub pojedynczy cudzysłów. Na przykład dwukrotne kliknięcie wyrazu &lt;span style=&quot; font-style:italic;&quot;&gt;Cześć&lt;/span&gt; w poniższej opcji wybierze &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;notowania;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Cześć!&amp;notowania;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Mówisz: &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;notowania; Cześć!&amp;notowania;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Jeśli znaki w polu ustawisz na &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;oferta;! &lt;/span&gt;co oznacza, że powinien przestać wybierać &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;Lub&lt;/span&gt; &amp;notowania; &lt;span style=&quot; font-style:italic;&quot;&gt;Lub&lt;/span&gt; ! następnie dwukrotne kliknięcie na &lt;span style=&quot; font-style:italic;&quot;&gt;Cześć&lt;/span&gt; po prostu wybierze &lt;span style=&quot; font-style:italic;&quot;&gt;Cześć&lt;/span&gt;&lt;/p&gt;&lt;p&gt;Mówisz: &amp;notowania;&lt;span style=&quot; font-weight:600;&quot;&gt;Cześć&lt;/span&gt;!&amp;notowania;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1086"/>
      <source>(characters to ignore in selection, for example &apos; or &quot;)</source>
      <translation>(znaki do zignorowania przy wyborze, na przykład &apos; lub &quot;)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1102"/>
      <source>&lt;p&gt;Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Niektóre gry (zwłaszcza wszystkie IRE MUDs) cierpią z powodu błędu, w którym&apos;t prawidłowo komunikować się z klientem, gdzie powinna znajdować się nowa linia. Włącz to, aby naprawić tekst z coraz dołączone do poprzedniego wiersza monitu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1112"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;Podczas wyświetlania zawartości Lua w głównym obszarze edytora tekstu edytora pokaż tabulatory i spacje z widocznymi znacznikami zamiast odstępów.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1132"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show  line and paragraphs ends with visible marks as well as whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;Podczas wyświetlania zawartości Lua w głównym obszarze edytora tekstu Edytor pokaże wiersze i akapity kończe się widocznymi znacznikami, a także odstępami.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1142"/>
      <source>&lt;p&gt;Prints Lua errors to the main console in addition to the error tab in the editor.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wyświetla błędy Lua na głównej konsoli gry poza zakładką podglądu błędu w edytorze.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1162"/>
      <source>Enable text analyzer</source>
      <translation>Włącz analizator tekstu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2642"/>
      <source>&lt;p&gt;TLS/SSL is usually on port 6697. IRC networks often use a &lt;b&gt;+&lt;/b&gt; when listing secure ports offered.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2645"/>
      <source>Use a secure connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3496"/>
      <source>h:mm:ss.zzz</source>
      <comment>Used to set a time interval only</comment>
      <translation>h:mm:ss.zzz</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1272"/>
      <source>Autocomplete</source>
      <translation>Auto-uzupełnianie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="229"/>
      <source>Use a dark theme</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="232"/>
      <source>&lt;p&gt;Changes your Mudlet Style to a Dark Fusion Style.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="239"/>
      <source>Notify on new data</source>
      <translation>Powiadamiaj o nowych danych</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="242"/>
      <source>&lt;p&gt;Show a toolbar notification if Mudlet is minimized and new data arrives.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="256"/>
      <source>Auto save on exit</source>
      <translation>Automatycznie zapisuj przy wyjściu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="263"/>
      <source>Allow server to download and play media</source>
      <translation>Zezwalaj serwerowi na pobieranie i odtwarzanie multimediów</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="266"/>
      <source>&lt;p&gt;This also needs GMCP to be enabled in the next group below.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="315"/>
      <source>Enable MSP  (Mud Sound Protocol)</source>
      <translation>Włącz MSP (Mud Sound Protocol)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="511"/>
      <source>Highlights your input line text when scrolling through your history for easy cancellation</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="514"/>
      <source>Highlight history</source>
      <translation>Podświetl historię</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="741"/>
      <source>This font is not monospace, which may not be ideal for playing some text games:
you can use it but there could be issues with aligning columns of text</source>
      <comment>Note that this text is split into two lines so that the message is not too wide in English, please do the same for other locales where the text is the same or longer</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1122"/>
      <source>&lt;p&gt;Select this option for better compatibility if you are using a netbook, or some other computer model that has a small screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1278"/>
      <source>Autocomplete Lua functions in code editor</source>
      <translation>Autouzupełniaj funkcje Lua w edytorze kodu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1321"/>
      <source>&lt;p&gt;The foreground color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Domyślnie używany kolor znaków na głównej konsoli (o ile nie został zmieniony przez skrypt LUA lub serwer gry).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1344"/>
      <source>&lt;p&gt;The background color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Domyślnie używany kolor tła znaków na głównej konsoli (o ile nie został zmieniony przez skrypt LUA lub serwer gry).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1364"/>
      <source>&lt;p&gt;The foreground color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kolor pierwszego planu używany dla głównego obszaru wejściowego.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1381"/>
      <source>&lt;p&gt;The background color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kolor tła używany dla głównego obszaru wejściowego.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1398"/>
      <source>&lt;p&gt;The foreground color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kolor pierwszego planu używany dla tekstu wysyłanego do serwera gry.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1418"/>
      <source>&lt;p&gt;The background color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kolor tła używany do tekstu wysyłanego na serwer gry.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1444"/>
      <source>These preferences set how you want a particular color to be represented visually in the main display:</source>
      <translation>Te preferencje określają sposób, w jaki dany kolor ma być reprezentowany wizualnie na ekranie głównym:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1454"/>
      <location filename="../src/ui/profile_preferences.ui" line="2272"/>
      <source>Black:</source>
      <translation>Czarny:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1461"/>
      <source>ANSI Color Number 0</source>
      <translation>Numer koloru ANSI 0</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1474"/>
      <location filename="../src/ui/profile_preferences.ui" line="2289"/>
      <source>Light black:</source>
      <translation>Rozjaśniony czarny:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1481"/>
      <source>ANSI Color Number 8</source>
      <translation>Numer koloru ANSI 8</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1494"/>
      <location filename="../src/ui/profile_preferences.ui" line="2306"/>
      <source>Red:</source>
      <translation>Czerwony:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1501"/>
      <source>ANSI Color Number 1</source>
      <translation>Numer koloru ANSI 1</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1514"/>
      <location filename="../src/ui/profile_preferences.ui" line="2323"/>
      <source>Light red:</source>
      <translation>Rozjaśniony czerwony:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1521"/>
      <source>ANSI Color Number 9</source>
      <translation>Numer koloru ANSI 9</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1534"/>
      <location filename="../src/ui/profile_preferences.ui" line="2340"/>
      <source>Green:</source>
      <translation>Zielony:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1541"/>
      <source>ANSI Color Number 2</source>
      <translation>Numer koloru ANSI 2</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1554"/>
      <location filename="../src/ui/profile_preferences.ui" line="2357"/>
      <source>Light green:</source>
      <translation>Rozjaśniony zielony:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1561"/>
      <source>ANSI Color Number 10</source>
      <translation>Numer koloru ANSI 10</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1574"/>
      <location filename="../src/ui/profile_preferences.ui" line="2374"/>
      <source>Yellow:</source>
      <translation>Żółty:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1581"/>
      <source>ANSI Color Number 3</source>
      <translation>Numer koloru ANSI 3</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1594"/>
      <location filename="../src/ui/profile_preferences.ui" line="2391"/>
      <source>Light yellow:</source>
      <translation>Rozjaśniony żółty:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1601"/>
      <source>ANSI Color Number 11</source>
      <translation>Numer koloru ANSI 11</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1614"/>
      <location filename="../src/ui/profile_preferences.ui" line="2408"/>
      <source>Blue:</source>
      <translation>Niebieski:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1621"/>
      <source>ANSI Color Number 4</source>
      <translation>Numer koloru ANSI 4</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1634"/>
      <location filename="../src/ui/profile_preferences.ui" line="2425"/>
      <source>Light blue:</source>
      <translation>Rozjaśniony niebieski:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1641"/>
      <source>ANSI Color Number 12</source>
      <translation>Numer koloru ANSI 12</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1654"/>
      <location filename="../src/ui/profile_preferences.ui" line="2442"/>
      <source>Magenta:</source>
      <translation>Purpura:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1661"/>
      <source>ANSI Color Number 5</source>
      <translation>Numer koloru ANSI 5</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1674"/>
      <location filename="../src/ui/profile_preferences.ui" line="2459"/>
      <source>Light magenta:</source>
      <translation>Rozjaśniona purpura:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1681"/>
      <source>ANSI Color Number 13</source>
      <translation>Numer koloru ANSI 13</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1694"/>
      <location filename="../src/ui/profile_preferences.ui" line="2476"/>
      <source>Cyan:</source>
      <translation>Cyjan:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1701"/>
      <source>ANSI Color Number 6</source>
      <translation>Numer koloru ANSI 6</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1714"/>
      <location filename="../src/ui/profile_preferences.ui" line="2493"/>
      <source>Light cyan:</source>
      <translation>Rozjaśniony cyjan:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1721"/>
      <source>ANSI Color Number 14</source>
      <translation>Numer koloru ANSI 14</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1734"/>
      <location filename="../src/ui/profile_preferences.ui" line="2510"/>
      <source>White:</source>
      <translation>Biały:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1741"/>
      <source>ANSI Color Number 7</source>
      <translation>Numer koloru ANSI 7</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1754"/>
      <location filename="../src/ui/profile_preferences.ui" line="2527"/>
      <source>Light white:</source>
      <translation>Rozjaśniony biały:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1761"/>
      <source>ANSI Color Number 15</source>
      <translation>Numer koloru ANSI 15</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1774"/>
      <location filename="../src/ui/profile_preferences.ui" line="2544"/>
      <source>Reset all colors to default</source>
      <translation>Resetuj wszystkie kolory do domyślnych</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1781"/>
      <source>&lt;p&gt;If this option is checked the Mud Server may send codes to change the above 16 colors or to reset them to their defaults by using standard ANSI &lt;tt&gt;OSC&lt;/tt&gt; Escape codes.&lt;/p&gt;&lt;p&gt;Specifically &lt;tt&gt;&amp;lt;OSC&amp;gt;Pirrggbb&amp;lt;ST&amp;gt;&lt;/tt&gt; will set the color with index &lt;i&gt;i&lt;/i&gt; to have the color with the given &lt;i&gt;rr&lt;/i&gt; red, &lt;i&gt;gg&lt;/i&gt; green and &lt;i&gt;bb&lt;/i&gt;  blue components where i is a single hex-digit (&apos;0&apos; to &apos;9&apos; or &apos;a&apos; to &apos;f&apos; or &apos;A&apos; to &apos;F&apos; to give a number between 0 an d15) and rr, gg and bb are two digit hex-digits numbers (between 0 to 255); &amp;lt;OSC&amp;gt; is &lt;i&gt;Operating System Command&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;[&lt;/tt&gt; and &amp;lt;ST&amp;gt; is the &lt;i&gt;String Terminator&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Conversely &lt;tt&gt;&amp;lt;OSC&amp;gt;R&amp;lt;ST&amp;gt;&lt;/tt&gt; will reset the colors to the defaults like the button to the right does.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jeśli ta opcja jest zaznaczona, Mud Server może wysłać kody, aby zmienić powyższe 16 kolorów lub zresetować je do ustawień domyślnych przy użyciu standardowego ANSI &lt;tt&gt;Osc&lt;/tt&gt; Kody ucieczki.&lt;/p&gt;&lt;p&gt;Specjalnie &lt;tt&gt;&amp;lt;OSC&amp;gt; Pirrggbb ( Pirrggbb )&amp;lt; St&amp;gt;&lt;/tt&gt; ustawi kolor z indeksem &lt;i&gt;I&lt;/i&gt; aby mieć kolor z danym &lt;i&gt;Rr&lt;/i&gt; Czerwony &lt;i&gt;Gg&lt;/i&gt; zielony i &lt;i&gt;Bb&lt;/i&gt;  niebieskie komponenty, w których i jest pojedynczą cyfrą szesnastkową (&apos;0&apos; do &apos;9&apos; Lub &apos;A&apos; do &apos;F&apos; Lub &apos;A&apos; do &apos;F&apos; aby podać liczbę między 0 a d15) i rr, gg i bb są dwucyfrowymi cyframi sześciokątnych (od 0 do 255); &amp;lt;OSC&amp;gt; Jest &lt;i&gt;Polecenie systemu operacyjnego&lt;/i&gt; który jest zwykle kodowany jako ASCII &amp;lt; Esc&amp;gt; charakter, po którym następuje &lt;tt&gt;[&lt;/tt&gt; I &amp;lt; St&amp;gt; jest &lt;i&gt;Terminator strun&lt;/i&gt; który jest zwykle kodowany jako ASCII &amp;lt; Esc&amp;gt; charakter, po którym następuje &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Odwrotnie &lt;tt&gt;&amp;lt;OSC&amp;gt; R&amp;lt; St&amp;gt;&lt;/tt&gt; zresetuje kolory do ustawień domyślnych, takich jak przycisk po prawej stronie.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1784"/>
      <source>Server allowed to redefine these colors</source>
      <translation>Serwer może ponownie zdefiniować te kolory</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1808"/>
      <source>Mapper</source>
      <translation>Maper</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1814"/>
      <source>Map files</source>
      <translation>Pliki mapy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1820"/>
      <source>Save your current map:</source>
      <translation>Zapisz aktualną mapę:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1830"/>
      <source>Press to choose location and save</source>
      <translation>Naciśnij aby wybrać lokalizację i zapisać</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1837"/>
      <source>Load another map file in:</source>
      <translation>Wczytaj inny plik mapy w:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1847"/>
      <source>Press to choose file and load</source>
      <translation>Naciśnij, aby wybrać plik i załadować</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1854"/>
      <source>&lt;p&gt;Mudlet now does some sanity checking and repairing to clean up issues that may have arisen in previous version due to faulty code or badly documented commands. However if significant problems are found the report can be quite extensive, in particular for larger maps.&lt;/p&gt;&lt;p&gt;Unless this option is set, Mudlet will reduce the amount of on-screen messages by hiding many texts and showing a suggestion to review the report file instead.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet teraz sprawdza i naprawia problemy, które mogły powstać w poprzedniej wersji z powodu błędnego kodu lub źle udokumentowanych poleceń. Jednakże w przypadku stwierdzenia istotnych problemów sprawozdanie może być dość obszerne, w szczególności w przypadku większych map.&lt;/p&gt;&lt;p&gt;Jeśli ta opcja nie jest ustawiona, Mudlet zmniejszy ilość wiadomości na ekranie, ukrywając wiele tekstów i pokazując sugestię, aby zamiast tego przejrzeć plik zgłoszenia.&lt;/p&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1938"/>
      <source>&lt;p&gt;Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zmień to na niższą wersję, jeśli potrzebujesz zapisać swoją mapę w formacie, który może być odczytany przez starsze wersje Mudlet. W ten sposób stracisz dodatkowe dane dostępne w aktualnym formacie mapy&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1968"/>
      <source>Download latest map provided by your game:</source>
      <translation>Pobierz najnowszą mapę dostarczoną przez grę:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1997"/>
      <source>&lt;p&gt;This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer.&lt;/p&gt;&lt;p&gt;3D map view always has anti-aliasing enabled.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Umożliwia to antyaliasing (AA) dla widoku mapy 2D, dzięki czemu wygląda on gładko i ładniej. Wyłącz to, jeśli&apos;jesteś na bardzo wolnym komputerze.&lt;/p&gt;&lt;p&gt;Widok mapy 3D zawsze ma włączoną funkcję antyaliasingu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2007"/>
      <source>&lt;p&gt;The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area&lt;/p&gt;</source>
      <translation>&lt;p&gt;Domyślny obszar (identyfikator obszaru -1) jest używany przez niektóre skrypty mapujące jako tymczasowy &apos;obszar utrzymywania&apos; dla pomieszczeń, zanim&apos;zostaną umieszczone w odpowiednim obszarze&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2072"/>
      <source>2D map player room marker style:</source>
      <translation>Styl znacznika pokoju z mapą 2D:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2082"/>
      <source>Outer ring color</source>
      <translation>Kolor pierścienia zewnętrznego</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2092"/>
      <source>Inner ring color</source>
      <translation>Kolor pierścienia wewnętrznego</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2106"/>
      <source>Original</source>
      <translation>Oryginał</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2111"/>
      <source>Red ring</source>
      <translation>Czerwony pierścień</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2116"/>
      <source>Blue/Yellow ring</source>
      <translation>Niebieski/żółty pierścień</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2121"/>
      <source>Custom ring</source>
      <translation>Pierścień niestandardowy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2129"/>
      <source>&lt;p&gt;Percentage ratio (&lt;i&gt;the default is 120%&lt;/i&gt;) of the marker symbol to the space available for the room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Stosunek procentowy (&lt;i&gt;wartość domyślna to 120%&lt;/i&gt;) symbolu znacznika do przestrzeni dostępnej dla pomieszczenia.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2135"/>
      <location filename="../src/ui/profile_preferences.ui" line="2163"/>
      <source>%</source>
      <translation>%</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2138"/>
      <source>Outer diameter: </source>
      <translation>Średnica zewnętrzna: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2157"/>
      <source>&lt;p&gt;Percentage ratio of the inner diameter of the marker symbol to the outer one (&lt;i&gt;the default is 70%&lt;/i&gt;).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Stosunek procentowy średnicy wewnętrznej symbolu znacznika do zewnętrznej (&lt;i&gt;wartość domyślna to 70%&lt;/i&gt;).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2166"/>
      <source>Inner diameter: </source>
      <translation>Średnica wewnętrzna: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2020"/>
      <source>&lt;p&gt;This enables borders around room. Color can be set in Mapper colors tab&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2023"/>
      <source>Show room borders</source>
      <translation>Pokaż krawędzie pokoju</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2248"/>
      <source>Room border color:</source>
      <translation>Kolor krawędzi pomieszczenia:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2568"/>
      <source>Chat</source>
      <translation>Czat</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3215"/>
      <source>Username for logging into the proxy if required</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3231"/>
      <source>Password for logging into the proxy if required</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3270"/>
      <source>Special options needed for some older game drivers (needs client restart to take effect)</source>
      <translation>Specjalne opcje potrzebne dla niektórych starszych sterowników gier (wymaga ponownego uruchomienia klienta, aby wejść w życie)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3308"/>
      <source>Force CHARSET negotiation off</source>
      <translation>Wymuszenie negocjowania CHARSET wyłączone</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3413"/>
      <source>the computer&apos;s password manager (secure)</source>
      <translation>komputera menedżer haseł (bezpieczny)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3418"/>
      <source>plaintext with the profile (portable)</source>
      <translation>zwykły tekst z profilem (przenośny)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3506"/>
      <source>&lt;p&gt;If checked this will cause all problem Unicode codepoints to be reported in the debug output as they occur; if cleared then each different one will only be reported once and summarized in as a table when the console in which they occurred is finally destroyed (when the profile is closed).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3392"/>
      <source>&lt;p&gt;Some MUDs use a flawed interpretation of the ANSI Set Graphics Rendition (&lt;b&gt;SGR&lt;/b&gt;) code sequences for 16M color mode which only uses semi-colons and not colons to separate parameter elements i.e. instead of using a code in the form: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38:2:&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Unused&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance Color Space (0=CIELUV; 1=CIELAB)&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;where the &lt;i&gt;Color Space Id&lt;/i&gt; is expected to be an empty string to specify the usual (default) case and all of the &lt;i&gt;Parameter Elements&lt;/i&gt; (the &quot;2&quot; and the values in the &lt;tt&gt;&amp;lt;...&amp;gt;&lt;/tt&gt;s) may, technically, be omitted; they use: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;or: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;It is not possible to reliably detect the difference between these two so checking this option causes Mudlet to expect the last one with the additional (but empty!) parameter.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Niektóre identyfikatory MUD używają błędnej interpretacji wersji graficznej ZESTAWU ANSI (&lt;b&gt;Sgr&lt;/b&gt;) sekwencje kodów dla trybu kolorów 16M, który używa tylko średników średników, a nie dwukropków do oddzielania elementów parametrów, czyli zamiast używania kodu w formularzu: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38:2:&lt;/tt&gt;&amp;lt; Identyfikator przestrzeni kolorów&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Czerwony&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Zielony&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Niebieski&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Nieużywane&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Tolerancji&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt; Tolerancja przestrzeni kolorów (0= CIELUV; 1=CIELAB)&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;M&lt;/tt&gt;&lt;br&gt;w przypadku gdy &lt;i&gt;Identyfikator przestrzeni kolorów&lt;/i&gt; ma być pustym ciągiem, aby określić zwykłą (domyślną) sprawę i wszystkie &lt;i&gt;Elementy parametrów&lt;/i&gt; ( &quot;2&quot; oraz wartości w &lt;tt&gt;&amp;lt;...&amp;gt;&lt;/tt&gt;s) mogą, technicznie, zostać pominięte; używają: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt; Czerwony&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt; Zielony&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt; Niebieski&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;M&lt;/tt&gt;&lt;br&gt;Lub: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt; Identyfikator przestrzeni kolorów&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt; Czerwony&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt; Zielony&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt; Niebieski&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;M&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;Nie jest możliwe niezawodne wykrycie różnicy między tymi dwoma, więc sprawdzenie tej opcji powoduje, że Mudlet oczekuje ostatniego z dodatkowym (ale pustym!) parametrem.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3395"/>
      <source>Expect Color Space Id in SGR...(3|4)8;2;...m codes</source>
      <translation>Oczekiwany Id przestrzeni barw w SGR... (3 | 4) 8; 2;... m kody</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3402"/>
      <source>Store character login passwords in:</source>
      <translation>Przechowuj hasła do logowania postaci w:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2836"/>
      <source>TLS/SSL secure connection</source>
      <translation>Bezpieczne połączenie TLS/SSL</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2937"/>
      <source>Accept self-signed certificates</source>
      <translation>Akceptowanie certyfikatów z podpisem własnym</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2947"/>
      <source>Accept expired certificates</source>
      <translation>Akceptowanie wygasłych certyfikatów</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2854"/>
      <source>Certificate</source>
      <translation>Certyfikat</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2914"/>
      <source>Serial:</source>
      <translation>Numer seryjny:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2863"/>
      <source>Issuer:</source>
      <translation>Wydawca:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2880"/>
      <source>Issued to:</source>
      <translation>Wydane dla:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2897"/>
      <source>Expires:</source>
      <translation>Kończy się:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2957"/>
      <source>Accept all certificate errors       (unsecure)</source>
      <translation>Zaakceptuj wszystkie błędy certyfikatów (niebezpieczne)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1857"/>
      <source>Report map issues on screen</source>
      <translation>Zgłoś problemy na ekranie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1871"/>
      <source>Copy map to other profile(s):</source>
      <translation>Kopiuj mapę do innych profili:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1887"/>
      <source>Press to pick destination(s)</source>
      <translation>Naciśnij, aby wybrać miejsca docelowe</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1900"/>
      <source>Copy to destination(s)</source>
      <translation>Kopiowanie do miejsc docelowych</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1912"/>
      <source>An action above happened</source>
      <translation>Powyższa akcja miała miejsce</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1919"/>
      <source>Map format version:</source>
      <translation>Wersja formatu mapy:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1944"/>
      <location filename="../src/ui/profile_preferences.ui" line="1948"/>
      <source># {default version}</source>
      <translation># {domyślna wersja}</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1959"/>
      <source>Map download</source>
      <translation>Pobieranie mapy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1981"/>
      <source>Download</source>
      <translation>Pobierz</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1991"/>
      <source>Map view</source>
      <translation>Widok mapy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2033"/>
      <source>2D Map Room Symbol Font</source>
      <translation>Czcionka symbolu pokoju mapy 2D</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2000"/>
      <source>Use high quality graphics in 2D view</source>
      <translation>Użyj wysokiej jakości grafiki, w widoku 2D</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="184"/>
      <source>&lt;p&gt;Can you help translate Mudlet?&lt;/p&gt;&lt;p&gt;If so, please visit: &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Czy potrafisz pomóc w tłumaczeniu Mudleta?&lt;/p&gt;&lt;p&gt;Jeśli tak, odwiedź &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1884"/>
      <source>&lt;p&gt;Select profiles that you want to copy map to, then press the Copy button to the right&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz profile, do których chcesz skopiować mapę, a następnie naciśnij przycisk Kopiuj po prawej&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1897"/>
      <source>&lt;p&gt;Copy map into the selected profiles on the left&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kopiowanie mapy do wybranych profili po lewej stronie&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1965"/>
      <location filename="../src/ui/profile_preferences.ui" line="1978"/>
      <source>&lt;p&gt;On games that provide maps for download, you can press this button to get the latest map. Note that this will &lt;span style=&quot; font-weight:600;&quot;&gt;overwrite&lt;/span&gt; any changes you&apos;ve done to your map, and will use the new map only&lt;/p&gt;</source>
      <translation>&lt;p&gt;W grach, które zapewniają mapy do pobrania, możesz nacisnąć ten przycisk, aby pobrać najnowszą mapę. Zauważ, że &lt;span style=&quot; font-weight:600;&quot;&gt;nadpisze&lt;/span&gt; wszelkie zmiany, które&apos;zrobiłeś na mapie, i będzie używać tylko nowej mapy (stracisz starą)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2010"/>
      <source>Show the default area in map area selection</source>
      <translation>Pokaż domyślny obszar w zaznaczonym obszarze mapy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2043"/>
      <source>Only use symbols (glyphs) from chosen font</source>
      <translation>Używaj tylko symboli (glifów) z wybranej czcionki</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2050"/>
      <source>Show symbol usage...</source>
      <translation>Pokaż użyte symbole...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2199"/>
      <source>Mapper colors</source>
      <translation>Kolory mapera</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2205"/>
      <source>Select your color preferences for the map display</source>
      <translation>Wybieranie preferencji kolorów dla wyświetlania mapy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2211"/>
      <source>Link color</source>
      <translation>Kolor połączenia</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2231"/>
      <source>Background color:</source>
      <translation>Kolor tła:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3264"/>
      <source>Special Options</source>
      <translation>Opcje specjalne</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3276"/>
      <source>Force compression off</source>
      <translation>Wymuś brak kompresji</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3294"/>
      <source>Force telnet GA signal interpretation off</source>
      <translation>Wymuś wyłączenie interpretacji sygnału telnet GA</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3283"/>
      <source>This option adds a line line break &lt;LF&gt; or &quot;
&quot; to your command input on empty commands. This option will rarely be necessary.</source>
      <translation>Ta opcja dodaje znak nowej linii &lt;LF&gt; lub &quot;
&quot; do wpisanego polecenia w przypadku pustych poleceń. Ta opcja rzadko jest potrzebna.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3287"/>
      <source>Force new line on empty commands</source>
      <translation>Wymuś znak nowej linii przy pustych poleceniach</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3301"/>
      <source>Force MXP negotiation off</source>
      <translation>Wymuś wynegocjowanie MXP wyłączony</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2655"/>
      <source>Discord privacy</source>
      <translation>Ochrona prywatności Discorda</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2688"/>
      <source>Don&apos;t hide small icon or tooltip</source>
      <translation>Nie ukrywaj małej ikonki lub etykietki narzędzia</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2693"/>
      <source>Hide small icon tooltip</source>
      <translation>Ukryj małą ikonę podpowiedzi</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2698"/>
      <source>Hide small icon and tooltip</source>
      <translation>Ukryj małą ikonę i podpowiedź</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2720"/>
      <source>Hide timer</source>
      <translation>Ukryj zegar</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2743"/>
      <location filename="../src/ui/profile_preferences.ui" line="2756"/>
      <location filename="../src/ui/profile_preferences.ui" line="2772"/>
      <location filename="../src/ui/profile_preferences.ui" line="2788"/>
      <source>&lt;p&gt;Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). Leave empty to show it for any Discord account you log in to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudlet będzie wyświetlał informacje o obecności tylko podczas używania tej nazwy użytkownika Discord (przydatnej, jeśli masz wiele kont Discord). Pozostaw puste, aby wyświetlić go dla każdego konta Discord, na które się zalogujesz.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2749"/>
      <source>Restrict to:</source>
      <translation>Ogranicz do:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2662"/>
      <source>Don&apos;t hide large icon or tooltip</source>
      <translation>Nie ukrywa dużej ikony lub podpowiedzi</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2667"/>
      <source>Hide large icon tooltip</source>
      <translation>Ukryj dużą ikonę podpowiedzi</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2672"/>
      <source>Hide large icon and tooltip</source>
      <translation>Ukryj dużą ikonę i podpowiedź</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2727"/>
      <source>&lt;p&gt;Allow Lua to set Discord status&lt;/p&gt;</source>
      <translation>&lt;p&gt;Zezwól Lua na ustawienie statusu Discorda&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2730"/>
      <source>Enable Lua API</source>
      <translation>Włącz interfejs API Lua</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2762"/>
      <source>specific Discord username</source>
      <translation>nazwa użytkownika Discorda</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2706"/>
      <source>Hide state</source>
      <translation>Ukryj stan</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2713"/>
      <source>Hide party details</source>
      <translation>Ukryj szczegóły grupy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2680"/>
      <source>Hide detail</source>
      <translation>Ukryj szczegóły</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2574"/>
      <source>IRC client options</source>
      <translation>Opcje klienta IRC</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2590"/>
      <source>irc.example.net</source>
      <translation>irc.example.net</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2597"/>
      <source>Port:</source>
      <translation>Port:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2635"/>
      <source>#channel1 #channel2 #etc...</source>
      <translation>#channel1 #channel2 #etc...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2621"/>
      <source>MudletUser123</source>
      <translation>MudletUser123</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2580"/>
      <source>Server address:</source>
      <translation>Adres serwera:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2628"/>
      <source>Auto-join channels: </source>
      <translation>Automatycznie dołącz kanały: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2614"/>
      <source>Nickname:</source>
      <translation>Pseudonim:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2607"/>
      <source>6667</source>
      <translation>6667</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3358"/>
      <source>Search Engine</source>
      <translation>Mechanizm wyszukiwania</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3342"/>
      <source>Mudlet updates</source>
      <translation>Aktualizacje Mudleta</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3348"/>
      <source>Disable automatic updates</source>
      <translation>Wyłączanie aktualizacji automatycznych</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3370"/>
      <source>Other Special options</source>
      <translation>Inne opcje specjalne</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3376"/>
      <source>Show icons on menus</source>
      <translation>Pokazuj ikony w menu</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2830"/>
      <source>Connection</source>
      <translation>Połączenie</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3170"/>
      <source>Connect to the game via proxy</source>
      <translation>Połącz się z grą za pośrednictwem serwera proxy</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3185"/>
      <source>Address</source>
      <translation>Adres</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3205"/>
      <source>port</source>
      <translation>port</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3221"/>
      <source>username (optional)</source>
      <translation>nazwa użytkownika (opcjonalnie)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3237"/>
      <source>password (optional)</source>
      <translation>hasło (opcjonalnie)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3459"/>
      <source>Show debug messages for timers not smaller than:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3465"/>
      <source>&lt;p&gt;Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the &lt;i&gt;Central Debug Console&lt;/i&gt; useless if there is a very small interval timer running.&lt;/p&gt;</source>
      <comment>The term in &apos;...&apos; refer to a Mudlet specific thing and ought to match the corresponding translation elsewhere.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3509"/>
      <source>Report all Codepoint problems immediately</source>
      <translation>Natychmiast zgłoś wszystkie problemy z Codepoint</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3516"/>
      <source>Additional text wait time:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3526"/>
      <source>&lt;p&gt;&lt;i&gt;Go-Ahead&lt;/i&gt; (&lt;tt&gt;GA&lt;/tt&gt;) and &lt;i&gt;End-of-record&lt;/i&gt; (&lt;tt&gt;EOR&lt;/tt&gt;) signalling tells Mudlet when the game server is done sending text. On games that do not provide &lt;tt&gt;GA&lt;/tt&gt; or &lt;tt&gt;EOR&lt;/tt&gt;, this option controls how long Mudlet will wait for more text to arrive. Greater values will help reduce the risk that Mudlet will split a large piece of text (with unintended line-breaks in the middle) which can stop some triggers from working. Lesser values increases the risk of text getting broken up, but may make the game feel more responsive.&lt;/p&gt;&lt;p&gt;&lt;i&gt;The default value, which was what Mudlet used before this control was added, is 0.300 Seconds.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3529"/>
      <source> seconds</source>
      <extracomment>For most locales a space should be included so that the text is separated from the number!</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3599"/>
      <source>Save</source>
      <translation>Zapisz</translation>
    </message>
  </context>
  <context>
    <name>room_exits</name>
    <message>
      <location filename="../src/ui/room_exits.ui" line="37"/>
      <source>General exits:</source>
      <translation>Ogólne wyjścia:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="93"/>
      <source>NW exit...</source>
      <translation>NW wyjście...</translation>
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
      <translation>&lt;p&gt;Ustaw dodatnią wartość, aby zastąpić domyślną (pokoju) wagę dla tej trasy wyjścia, wartość zerowa przypisuje wartość domyślną.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="230"/>
      <source>N exit...</source>
      <translation>N wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="367"/>
      <source>NE exit...</source>
      <translation>NE wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="504"/>
      <source>Up exit...</source>
      <translation>Na górę...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="644"/>
      <source>W exit...</source>
      <translation>W wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="752"/>
      <source>ID:</source>
      <translation>ID:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="775"/>
      <source>Weight:</source>
      <translation>Waga:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="848"/>
      <source>E exit...</source>
      <translation>E wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="982"/>
      <source>Down exit...</source>
      <translation>W dół wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1131"/>
      <source>SW exit...</source>
      <translation>SW wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1274"/>
      <source>S exit...</source>
      <translation>S wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1411"/>
      <source>SE exit...</source>
      <translation>SE wyjście...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1548"/>
      <source>In exit...</source>
      <translation>Na wyjściu...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1821"/>
      <source>Out exit...</source>
      <translation>Na wyjściu...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1653"/>
      <source>Key:</source>
      <translation>Klawisz:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1671"/>
      <source>No route</source>
      <translation>Brak ścieżki</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1686"/>
      <source>Stub Exit</source>
      <translation>Wyjście odcinek</translation>
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
      <location filename="../src/ui/room_exits.ui" line="1970"/>
      <source>&lt;p&gt;Prevent a route being created via this exit, equivalent to an infinite exit weight.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Unikaj tworzenia trasy za pośrednictwem tego wyjścia, co odpowiada nieskończonej wadze wyjścia.&lt;/p&gt;</translation>
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
      <translation>&lt;p&gt;Utwórz wyjście w tym kierunku z nieznanym miejscem docelowym, wzajemnie wykluczające się z rzeczywistym exit roomID.&lt;/p&gt;</translation>
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
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Nie rysuje się symbolu drzwi na mapie 2D dla tego wyjścia.&lt;/p&gt;</translation>
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
      <translation>&lt;p&gt;Zielony (otwarty) symbol drzwi jest rysowany na mapie 2D, można ustawić na wycinku lub rzeczywistym wyjściu.&lt;/p&gt;</translation>
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
      <translation>&lt;p&gt;Pomarańczowy (zamknięty) symbol drzwi jest rysowany na mapie 2D, można ustawić na wycinku lub rzeczywistym wyjściu.&lt;/p&gt;</translation>
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
      <translation>&lt;p&gt;Czerwony (zablokowany) symbol drzwi jest rysowany na mapie 2D, można ustawić na wycinek lub prawdziwe wyjście.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="546"/>
      <location filename="../src/ui/room_exits.ui" line="1024"/>
      <location filename="../src/ui/room_exits.ui" line="1590"/>
      <location filename="../src/ui/room_exits.ui" line="1863"/>
      <source>&lt;p&gt;A symbol is drawn with a green (Open) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol jest rysowany zielonym (otwartym) kolorem wypełnienia na mapie 2D, można ustawić na wycinku lub rzeczywistym wyjściu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="562"/>
      <location filename="../src/ui/room_exits.ui" line="1040"/>
      <location filename="../src/ui/room_exits.ui" line="1606"/>
      <location filename="../src/ui/room_exits.ui" line="1879"/>
      <source>&lt;p&gt;A symbol is drawn with an orange (Closed) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol jest rysowany pomarańczowym (zamkniętym) kolorem wypełnienia na mapie 2D, można ustawić na wycinku lub rzeczywistym wyjściu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="578"/>
      <location filename="../src/ui/room_exits.ui" line="1056"/>
      <location filename="../src/ui/room_exits.ui" line="1622"/>
      <location filename="../src/ui/room_exits.ui" line="1895"/>
      <source>&lt;p&gt;A symbol is drawn with a red (Locked) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Symbol jest rysowany czerwonym (zablokowanym) kolorem wypełnienia na mapie 2D, można ustawić na wycinku lub rzeczywistym wyjściu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="765"/>
      <source>&lt;p&gt;This is the Room ID Number for this room - it cannot be changed here!</source>
      <translation>&lt;p&gt;Jest to numer ID pokoju dla tego pokoju - nie można go tu zmienić!</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="788"/>
      <source>&lt;p&gt;This is the default weight for this room, which will be used for any exit &lt;i&gt;that leads to &lt;u&gt;this room&lt;/u&gt;&lt;/i&gt; which does not have its own value set - this value cannot be changed here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jest to domyślna waga dla tego pokoju, która będzie używana dla każdego wyjścia &lt;i&gt;, który prowadzi do &lt;u&gt;tego pokoju&lt;/u&gt;&lt;/i&gt; który nie ma własnego zestawu wartości - ta wartość nie może zostać tutaj zmieniona.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1698"/>
      <source>Exit RoomID number</source>
      <translation>Wyjście numer RoomID</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1725"/>
      <source>No door</source>
      <translation>Brak drzwi</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1740"/>
      <source>Open door</source>
      <translation>Otwarte drzwi</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1752"/>
      <source>Closed door</source>
      <translation>Drzwi zamknięte</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1764"/>
      <source>Locked door</source>
      <translation>Zablokowane drzwi</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2092"/>
      <source>&lt;p&gt;Use this button to save any changes, will also remove any invalid Special exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2108"/>
      <source>&lt;p&gt;Use this button to close the dialogue without changing anything.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Użyj tego przycisku, aby zamknąć okno dialogowe bez zmiany czegokolwiek.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1923"/>
      <source>&lt;p&gt;Click on an item to edit/change it, to DELETE a Special Exit set its Exit Room ID to zero.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij element, aby go edytować/zmienić. Aby usunąć specjalne wyjście ustaw identyfikator lokacji na zero.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1961"/>
      <source>&lt;p&gt;Set the number of the room that this exit leads to, if set to zero the exit will be removed on saving the exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1988"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit (only functional choice currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1997"/>
      <source>&lt;p&gt;Green (Open) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2006"/>
      <source>&lt;p&gt;Orange (Closed) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2015"/>
      <source>&lt;p&gt;Red (Locked) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2024"/>
      <source>&lt;p&gt;(Lua scripts need to be prefixed with &quot;script:&quot;).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2041"/>
      <source>&lt;p&gt;Add an empty item to Special exits to be edited as required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2060"/>
      <source>&lt;p&gt;Press this button to deactivate the selection of a Special exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2095"/>
      <source>&amp;Save</source>
      <translation>Zapisz</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1710"/>
      <source>Exit Weight (0=No override)</source>
      <translation>Waga wyjściowa (0=Bez nadpisania)</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2111"/>
      <source>&amp;Cancel</source>
      <translation>Anuluj</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1914"/>
      <source>Special exits:</source>
      <translation>Wyjścia specjalne:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1957"/>
      <source>Exit
Room ID</source>
      <translation>Wyjście
ID pokoju</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1966"/>
      <source>No
Route</source>
      <translation>Brak
Trasy</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1975"/>
      <source>Exit
Weight</source>
      <translation>Waga
wyjścia</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1979"/>
      <source>&lt;p&gt;Set to a positive integer value to override the default (Room) Weight for using this Exit route, a zero value assigns the default.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ustaw dodatnią wartość całkowitą, aby zastąpić domyślną wagę (Pokój) dla korzystania z tej trasy zakończenia, wartość zero przypisuje wartość domyślną.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1984"/>
      <source>Door
None</source>
      <translation>Drzwi
Brak</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1993"/>
      <source>Door
Open</source>
      <translation>Drzwi
Otwórz</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2002"/>
      <source>Door
Closed</source>
      <translation>Drzwi
zamknięte</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2011"/>
      <source>Door
Locked</source>
      <translation>Drzwi
zablokowane</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2020"/>
      <source>Command
or LUA script</source>
      <translation>Polecenie lub skrypt LUA</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2044"/>
      <source>&amp;Add special exit</source>
      <translation>&amp;Dodaj specjalne wyjście</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2063"/>
      <source>&amp;End S. Exits editing</source>
      <translation>&amp;End S. Kończy edycję</translation>
    </message>
  </context>
  <context>
    <name>room_symbol</name>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="14"/>
      <location filename="../src/ui/room_symbol.ui" line="112"/>
      <source>Room symbol</source>
      <translation>Symbol lokacji</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="141"/>
      <source>Color of to use for the room symbol(s)</source>
      <translation>Kolor do użycia dla symbolu(-ów) lokacji</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="147"/>
      <source>Symbol color</source>
      <translation>Kolor symbolu</translation>
    </message>
    <message>
      <location filename="../src/ui/room_symbol.ui" line="154"/>
      <source>Reset</source>
      <translation>Resetuj</translation>
    </message>
  </context>
  <context>
    <name>scripts_main_area</name>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.&lt;/p&gt;&lt;p&gt;If a function within the script is to be used to handle events entered in the list below &lt;b&gt;&lt;u&gt;it must have the same name as is entered here.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="40"/>
      <source>Registered Event Handlers:</source>
      <translation>Zarejestrowane podmioty obsługujące zdarzenia:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="88"/>
      <source>&lt;p&gt;Remove (selected) event handler from list.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Usuń (wybraną) obsługę zdarzeń z listy.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="91"/>
      <source>-</source>
      <translation>-</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="98"/>
      <source>Add User Event Handler:</source>
      <translation>Dodaj Obsługę Zdarzeń użytkownika:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="133"/>
      <source>&lt;p&gt;Add entered event handler name to list.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Dodaj wprowadzoną nazwę obsługi zdarzeń do listy.&lt;/p&gt;</translation>
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
      <translation>Przenieś pokoje do innego obszaru</translation>
    </message>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="20"/>
      <source>Which area would you like to move the room(s) to?</source>
      <translation>Do którego obszaru chcesz przenieść pokój(-e)?</translation>
    </message>
  </context>
  <context>
    <name>source_editor_area</name>
    <message>
      <location filename="../src/ui/source_editor_area.ui" line="26"/>
      <source>Form</source>
      <translation>Forma</translation>
    </message>
  </context>
  <context>
    <name>timers_main_area</name>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="29"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="64"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
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
      <translation>Czas:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="39"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz dobrą (najlepiej, choć nie musi być, unikalną) nazwę dla timera, czasomierza lub grupy czasomierza. Zostanie to wyświetlone w drzewie zegarów.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="77"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="89"/>
      <source>&lt;p&gt;hours&lt;/p&gt;</source>
      <translation>&lt;p&gt;godzin&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="104"/>
      <source>&lt;p&gt;minutes&lt;/p&gt;</source>
      <translation>&lt;p&gt;minut&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="119"/>
      <source>&lt;p&gt;seconds&lt;/p&gt;</source>
      <translation>&lt;p&gt;sekund&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="184"/>
      <source>&lt;p&gt;The &lt;b&gt;hour&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;H &lt;b&gt;Godziny&lt;/b&gt; część interwału, w której wyłączy się czasomierz.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="392"/>
      <source>&lt;p&gt;The &lt;b&gt;millisecond&lt;/b&gt; part of the interval that the timer will go off at (1000 milliseconds = 1 second).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="260"/>
      <source>&lt;p&gt;The &lt;b&gt;minute&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;M &lt;b&gt;Minuyt&lt;/b&gt; część interwału, w której wyłączy się czasomierz.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="326"/>
      <source>&lt;p&gt;The &lt;b&gt;second&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;S &lt;b&gt;Sekundy&lt;/b&gt; część interwału, w której wyłączy się czasomierz.&lt;/p&gt;</translation>
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
      <translation>Pokaż zmienne zwykle ukryte</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="476"/>
      <source>&lt;p&gt;Enter text here to search through your code.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wpisz tutaj tekst, aby przeszukać kod.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="510"/>
      <source>&lt;p&gt;Toggles the display of the search results area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Przełącza wyświetlanie wyników wyszukiwania.&lt;/p&gt;</translation>
    </message>
  </context>
  <context>
    <name>trigger_main_area</name>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="65"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="85"/>
      <source>&lt;p&gt;Use this control to show or hide the extra controls for the trigger; this can be used to allow more space to show the trigger &lt;i&gt;items&lt;/i&gt; on smaller screen.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Użyj tego elementu sterującego, aby pokazać lub ukryć dodatkowe elementy sterujące dla wyzwalacza. Można to wykorzystać, aby umożliwić więcej miejsca na wyświetlenie wyzwalacza &lt;i&gt;pozycji&lt;/i&gt; na mniejszym ekranie.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="111"/>
      <source>Command:</source>
      <translation>Polecenie:</translation>
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
      <translation>&lt;p&gt;Wyzwalacz wystrzeli tylko wtedy, gdy &lt;u&gt;wszystkie&lt;/u&gt; warunki na liście zostały spełnione w obrębie określonej delty linii, a przechwyty zostaną zapisane w &lt;tt&gt;wielowątkowości&lt;/tt&gt; zamiast &lt;tt&gt;dopasowań&lt;/tt&gt;.&lt;/p&gt;
&lt;p&gt;Jeśli opcja ta jest &lt;b&gt;nie&lt;/b&gt; ustawi spust zostanie uruchomiony, jeśli &lt;u&gt;jakikolwiek&lt;/u&gt; warunek na liście został spełniony.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="208"/>
      <source>AND / Multi-line (delta)</source>
      <translation>AND / Multi-line (delta)</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="260"/>
      <source>&lt;p&gt;When checked, only the filtered content (=capture groups) will be passed on to child triggers, not the initial line (see manual on filters).&lt;/p&gt;</source>
      <translation>&lt;p&gt;Gdy jest zaznaczone, tylko przefiltrowana zawartość (=grupy przechwytujące) zostanie przekazana do pochodnych wyzwalaczy, a nie linia początkowa (patrz instrukcja dotycząca filtrów).&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="266"/>
      <source>only pass matches</source>
      <translation>tylko pasujące wyniki</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="302"/>
      <source>Do not pass whole line to children.</source>
      <translation>Nie należy przekazywać całej linii pochodnym-dzieciom.</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="324"/>
      <source>&lt;p&gt;Choose this option if you want to include all possible matches of the pattern in the line.&lt;/p&gt;&lt;p&gt;Without this option, the pattern matching will stop after the first successful match.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz tę opcję, jeśli chcesz włączyć wszystkie możliwe dopasowania wzoru do linii.&lt;/p&gt;&lt;p&gt;Bez tej opcji, dopasowanie wzorca zatrzyma się po pierwszym udanym dopasowaniu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="415"/>
      <source>&lt;p&gt;Keep firing the script for this many more lines, after the trigger or chain has matched.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Po dopasowaniu wyzwalacza lub łańcucha nadal dopasowuj dla wielu kolejnych linii.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="421"/>
      <source>fire length (extra lines)</source>
      <translation>długość wyzwalacza (dodatkowe linie)</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="479"/>
      <source>&lt;p&gt;Play a sound file if the trigger fires.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Odtwórz plik dźwiękowy, jeśli wyzwalacz zostanie uruchomiony.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="524"/>
      <source>&lt;p&gt;Use this to open a file selection dialogue to find a sound file to play when the trigger fires.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Cancelling from the file dialogue will not make any changes; to clear the file use the clear button to the right of the file name display.&lt;/i&gt;&lt;/p&gt;</source>
      <comment>This is the button used to select a sound file to be played when a trigger fires.</comment>
      <extracomment>Please ensure the text used here is duplicated within the tooltip for the QLineEdit that displays the file name selected.</extracomment>
      <translation>&lt;p&gt;Użyj tego, aby otworzyć okno wyboru pliku, aby znaleźć plik dźwiękowy do odtworzenia po uruchomieniu wyzwalacza.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Anulowanie z okna pliku nie spowoduje żadnych zmian;, aby wyczyścić plik, użyj przycisku wyczyść po prawej stronie wyświetlanej nazwy pliku.&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="528"/>
      <source>Choose file...</source>
      <translation>Wybierz plik...</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="559"/>
      <source>no file</source>
      <translation>brak pliku</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="620"/>
      <source>&lt;p&gt;Enable this to highlight the matching text by changing the fore and background colors to the ones selected here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Włącz to, aby wyróżnić pasujący tekst, zmieniając kolory liter i tła na te zaznaczone w tym miejscu.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="663"/>
      <source>Background</source>
      <translation>Tło</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="679"/>
      <location filename="../src/ui/triggers_main_area.ui" line="692"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation>zachowaj</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="653"/>
      <source>Foreground</source>
      <translation>Pierwszy plan</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="482"/>
      <source>play sound</source>
      <translation>odtwarzanie dźwięku</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="78"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Wybierz dobrą (najlepiej, choć nie musi być, unikalną) nazwę dla grupy wyzwalacza lub wyzwalacza. Zostanie to wyświetlone w drzewie wyzwalaczy.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="247"/>
      <source>&lt;p&gt;Within how many lines must all conditions be true to fire the trigger?&lt;/p&gt;</source>
      <translation>&lt;p&gt;W ilu liniach muszą być spełnione wszystkie warunki, aby wystrzeliwać wyzwalacz?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="330"/>
      <source>match all</source>
      <translation>dopasować wszystkie</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="360"/>
      <source>Match all occurrences of the pattern in the line.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="463"/>
      <source>&lt;p&gt;How many more lines, after the one that fired the trigger, should be passed to the trigger&apos;s children?&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ile kolejnych linii, po tej, która uruchomi wyzwalacz, powinno być przesłanych do pochodnych wyzwalacza?&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="550"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <comment>This is the tooltip for the QLineEdit that shows - but does not permit changing - the sound file used for a trigger.</comment>
      <translation>&lt;p&gt;Plik dźwiękowy do odtwożenia po uruchomieniu wyzwalacza.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="572"/>
      <source>&lt;p&gt;Click to remove the sound file set for this trigger.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Kliknij, aby usunąć plik dźwiękowy ustawiony dla tego wyzwalacza.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="626"/>
      <source>highlight</source>
      <translation>podświetlenie</translation>
    </message>
  </context>
  <context>
    <name>trigger_pattern_edit</name>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="124"/>
      <source>Foreground color ignored</source>
      <translation>Ignorowany kolor pierwszego planu</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="148"/>
      <source>Background color ignored</source>
      <translation>Kolor tła ignorowany</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="175"/>
      <source>match on the prompt line</source>
      <translation>dopasować w wierszu monitu</translation>
    </message>
  </context>
  <context>
    <name>vars_main_area</name>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="109"/>
      <source>Name:</source>
      <translation>Nazwa:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="175"/>
      <source>⏴ Key type:</source>
      <translation>⏴ Typ klucza:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="125"/>
      <source>&lt;p&gt;Set the &lt;i&gt;global variable&lt;/i&gt; or the &lt;i&gt;table entry&lt;/i&gt; name here. The name has to start with a letter, but can contain a mix of letters and numbers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Ustaw &lt;i&gt;zmienna globalna&lt;/i&gt; lub &lt;i&gt;wpis tabeli&lt;/i&gt; nazwę tutaj. Nazwa musi zaczynać się od litery, ale może zawierać kombinację liter i cyfr.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="51"/>
      <location filename="../src/ui/vars_main_area.ui" line="145"/>
      <source>Auto-Type</source>
      <translation>Automatyczny Typ</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="128"/>
      <source>Variable name ...</source>
      <translation>Nazwa zmiennej ...</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="138"/>
      <source>&lt;p&gt;Tables can store values either in a list, and/or a hashmap.&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;list&lt;/b&gt;, &lt;i&gt;unique indexed keys&lt;/i&gt; represent values - so you can have values at &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;map&lt;/b&gt; {a.k.a. an &lt;i&gt;associative array}&lt;/i&gt;, &lt;i&gt;unique keys&lt;/i&gt; represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).&lt;/p&gt;&lt;p&gt;This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.&lt;/p&gt;&lt;p&gt;In other cases it displays other entities (&lt;span style=&quot; font-style:italic;&quot;&gt;functions&lt;/span&gt;) which cannot be modified from here.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="150"/>
      <source>key (string)</source>
      <translation>klucz (ciąg znaków)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="155"/>
      <source>index (integer)</source>
      <translation>indeks (liczba całkowita)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="71"/>
      <location filename="../src/ui/vars_main_area.ui" line="160"/>
      <source>table</source>
      <translation>tabela</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="165"/>
      <source>function
(cannot create
from GUI)</source>
      <translation>funkcja
(nie można utworzyć
z GUI)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="185"/>
      <source>&lt;p&gt;If checked this item (and its children, if applicable) does not show in area to the left unless &lt;b&gt;Show normally hidden variables&lt;/b&gt; is checked.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Jeśli ten element jest zaznaczony (i jego elementy podrzędne, jeśli ma zastosowanie), nie jest wyświetlany w obszarze po lewej stronie, chyba że zaznaczono opcję &lt;b&gt;Pokaż normalnie ukryte zmienne&lt;/b&gt;.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="188"/>
      <source>hidden variable</source>
      <translation>ukryta zmienna</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="84"/>
      <source>⏷ Value type:</source>
      <translation>⏷ Typ wartości:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="56"/>
      <source>string</source>
      <translation>ciąg znaków</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="61"/>
      <source>number</source>
      <translation>liczba</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="66"/>
      <source>boolean</source>
      <translation>Wartość logiczna</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="76"/>
      <source>function</source>
      <translation>funkcja</translation>
    </message>
  </context>
</TS>
