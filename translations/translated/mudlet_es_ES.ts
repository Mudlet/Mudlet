<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es-ES" sourcelanguage="en">
  <context>
    <name>Discord</name>
    <message>
      <location filename="../src/discord.cpp" line="149"/>
      <source>via Mudlet</source>
      <translation>vía Mudlet</translation>
    </message>
  </context>
  <context>
    <name>Feed</name>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="275"/>
      <source>Too many redirects.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="284"/>
      <source>No data received from server</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/feed.cpp" line="295"/>
      <source>Could not verify download integrity.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>GLWidget</name>
    <message>
      <location filename="../src/glwidget.cpp" line="286"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/glwidget.cpp" line="285"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>Host</name>
    <message>
      <location filename="../src/Host.cpp" line="459"/>
      <source>Text to send to the game</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="840"/>
      <source>[  OK  ]  - %1 Thanks a lot for using the Public Test Build!</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="841"/>
      <source>[  OK  ]  - %1 Help us make Mudlet better by reporting any problems.</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1640"/>
      <source>Unpacking module:
&quot;%1&quot;
please wait...</source>
      <translation>Desempacando módulo:
&quot;%1&quot;
Por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1642"/>
      <source>Unpacking package:
&quot;%1&quot;
please wait...</source>
      <translation>Desempacando paquete:
&quot;%1&quot;
Por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1646"/>
      <source>Unpacking</source>
      <translation>Descomprimir</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2250"/>
      <source>Playing %1</source>
      <translation>En juego %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2252"/>
      <location filename="../src/Host.cpp" line="2258"/>
      <source>%1 at %2:%3</source>
      <comment>%1 is the game name and %2:%3 is game server address like: mudlet.org:23</comment>
      <translation>%1 en %2:%3</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2638"/>
      <location filename="../src/Host.cpp" line="3570"/>
      <source>Map - %1</source>
      <translation>Mapa - %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3586"/>
      <source>Pre-Map loading(3) report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3596"/>
      <source>Loading map(3) at %1 report</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>KeyUnit</name>
    <message>
      <location filename="../src/KeyUnit.cpp" line="333"/>
      <source>%1undefined key (code: 0x%2)</source>
      <comment>%1 is a string describing the modifier keys (e.g. &quot;shift&quot; or &quot;control&quot;) used with the key, whose &apos;code&apos; number, in %2 is not one that we have a name for. This is probably one of those extra keys around the edge of the keyboard that some people have.</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>MapInfoContributorManager</name>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="116"/>
      <source>Area:%1%2 ID:%1%3 x:%1%4%1&lt;‑&gt;%1%5 y:%1%6%1&lt;‑&gt;%1%7 z:%1%8%1&lt;‑&gt;%1%9
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and non-breaking hyphens which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. %2 is the (text) name of the area, %3 is the number for it, %4 to %9 are pairs (min &lt;-&gt; max) of extremes for each of x,y and z coordinates</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="140"/>
      <source>Room Name: %1
</source>
      <translation>Nombre de Habitación: %1
</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="153"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when NO rooms are selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the current player&apos;s room.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="170"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when ONE room is selected, %3 is the room number of, and %4-%6 are the x,y and z coordinates for, the selected Room.</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mapInfoContributorManager.cpp" line="188"/>
      <source>Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms
</source>
      <comment>This text uses non-breaking spaces (as &apos;%1&apos;s, as Qt Creator cannot handlethem literally in raw strings) and a non-breaking hyphen which are used to prevent the line being split at some places it might otherwise be; when translating please consider at which points the text may be divided to fit onto more than one line. This text is for when TWO or MORE rooms are selected; %1 is the room number for which %2-%4 are the x,y and z coordinates of the room nearest the middle of the selection. This room has the yellow cross-hairs. %n is the count of rooms selected and will ALWAYS be greater than 1 in this situation. It is provided so that non-English translations can select required plural forms as needed.</comment>
      <translation type="unfinished"/>
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
      <translation>! %1 está lejos (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="146"/>
      <source>! %1 is back</source>
      <translation>! %1 ha vuelto</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="153"/>
      <source>! invited %1 to %2</source>
      <translation>! %1 invitado a %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="156"/>
      <source>! %2 invited to %3</source>
      <translation>! %2 invitado a %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="163"/>
      <source>! You have joined %1 as %2</source>
      <translation>! Te has unido a %1 como %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="165"/>
      <source>! %1 has joined %2</source>
      <translation>! %1 se ha unido a %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="172"/>
      <source>! %1 kicked %2</source>
      <translation>! %1 patadas %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="180"/>
      <source>! %1 mode is %2 %3</source>
      <translation>! El modo %1 es %2 %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="182"/>
      <source>! %1 sets mode %2 %3 %4</source>
      <translation>! %1 fija el modo %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="199"/>
      <source>[MOTD] %1%2</source>
      <translation>[MDD] %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="211"/>
      <source>! %1 has %2 users: %3</source>
      <translation>! %1 ha %2 usuarios: %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="213"/>
      <source>! %1 has %2 users</source>
      <translation>! %1 ha %2 usuarios</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="220"/>
      <source>! %1 has changed nick to %2</source>
      <translation>! %1 ha cambiado su nick a %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="234"/>
      <source>! %1 replied in %2</source>
      <translation>! %1 constestó en %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="237"/>
      <location filename="../src/ircmessageformatter.cpp" line="286"/>
      <source>! %1 time is %2</source>
      <translation>! tiempo de %1 es %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="240"/>
      <location filename="../src/ircmessageformatter.cpp" line="283"/>
      <source>! %1 version is %2</source>
      <translation>! versión %1 es %2</translation>
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
      <translation>[URL del canal] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="324"/>
      <source>[%1] %2</source>
      <translation>[%1] %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="338"/>
      <source>! %1 has left %2</source>
      <translation>! %1 ha dejado %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="340"/>
      <source>! %1 has left %2 (%3)</source>
      <translation>! %1 ha abandonado %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="349"/>
      <source>! %1 replied in %2 seconds</source>
      <translation>! %1 respondió en %2 segundos</translation>
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
      <translation>%1 se ha ido</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="380"/>
      <source>! %1 has quit (%2)</source>
      <translation>! %1 se ha desconectado (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="388"/>
      <source>! no topic</source>
      <translation>no hay tema</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="397"/>
      <source>[TOPIC] %1</source>
      <translation>[TOPIC] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="401"/>
      <source>! %2 cleared topic</source>
      <translation>! %2 ha borrado el asunto</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="404"/>
      <source>! %2 changed topic</source>
      <translation>! %2 a cambiado de tema</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="410"/>
      <source>? %2 %3 %4</source>
      <translation>? %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="417"/>
      <source>[WHOIS] %1 is %2@%3 (%4)</source>
      <translation>[WHOIS] %1 es %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="418"/>
      <source>[WHOIS] %1 is connected via %2 (%3)</source>
      <translation>[WHOIS] %1 está conectado desde %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="419"/>
      <source>[WHOIS] %1 is connected since %2 (idle %3)</source>
      <translation>[WHOIS] %1 está conectado desde %2 (inactivo %3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="421"/>
      <source>[WHOIS] %1 is away: %2</source>
      <translation>[WHOIS] %1 está ausente: %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="424"/>
      <source>[WHOIS] %1 is logged in as %2</source>
      <translation>[WHOIS] %1 está registrado como %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="427"/>
      <source>[WHOIS] %1 is connected from %2</source>
      <translation>[WHOIS] %1 está conectado de %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="430"/>
      <source>[WHOIS] %1 is using a secure connection</source>
      <translation>[WHOIS] %1 está utilizando una conexión segura</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="433"/>
      <source>[WHOIS] %1 is on %2</source>
      <translation>[WHOIS] %1 está en %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="442"/>
      <source>[WHOWAS] %1 was %2@%3 (%4)</source>
      <translation>[WHOWAS]%1 fue %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="443"/>
      <source>[WHOWAS] %1 was connected via %2 (%3)</source>
      <translation>[WHOWAS] %1 estaba conectado a través de %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="445"/>
      <source>[WHOWAS] %1 was logged in as %2</source>
      <translation>[WHOWAS] %1 estaba registrado como %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="453"/>
      <source>[WHO] %1 (%2)</source>
      <translation>[WHO] %1 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="455"/>
      <source> - away</source>
      <translation> - ausente</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="458"/>
      <source> - server operator</source>
      <translation> - operador del servidor</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="466"/>
      <source>%1s</source>
      <translation>%1s</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="473"/>
      <source>%1 days</source>
      <translation>%1 días</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="477"/>
      <source>%1 hours</source>
      <translation>%1 horas</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="481"/>
      <source>%1 mins</source>
      <translation>%1 mins</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="483"/>
      <source>%1 secs</source>
      <translation>%1 segs</translation>
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
      <translation> (%1)</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="399"/>
      <source>Error parsing theme %1:%2</source>
      <translation>Error analizar el tema %1:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="404"/>
      <source>Error theme not found %1.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>T2DMap</name>
    <message>
      <location filename="../src/T2DMap.cpp" line="2789"/>
      <source>Create a new room here</source>
      <translation>Crear una nueva habitación aquí</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2962"/>
      <source>Change the properties of this custom line</source>
      <translation>Cambiar las propiedades de esta línea personalizada</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3193"/>
      <location filename="../src/T2DMap.cpp" line="4808"/>
      <source>Solid line</source>
      <translation>Línea sólida</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3194"/>
      <location filename="../src/T2DMap.cpp" line="4809"/>
      <source>Dot line</source>
      <translation>Linea de puntos</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3195"/>
      <location filename="../src/T2DMap.cpp" line="4810"/>
      <source>Dash line</source>
      <translation>Línea de trazos</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3196"/>
      <location filename="../src/T2DMap.cpp" line="4811"/>
      <source>Dash-dot line</source>
      <translation>Línea de trazo-punto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3197"/>
      <location filename="../src/T2DMap.cpp" line="4812"/>
      <source>Dash-dot-dot line</source>
      <translation>Línea trazo-punto-punto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3485"/>
      <source>x coordinate (was %1):</source>
      <translation>coordenada x (era %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3486"/>
      <source>y coordinate (was %1):</source>
      <translation>coordenada y (era %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3487"/>
      <source>z coordinate (was %1):</source>
      <translation>coordenada z (era %1):</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3675"/>
      <source>Delete color</source>
      <comment>Deletes an environment colour</comment>
      <translation>Eliminar color</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3693"/>
      <source>Define new color</source>
      <translation>Definir nuevo color</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4007"/>
      <source>%1 {count:%2}</source>
      <translation>%1 {count:%2}</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1378"/>
      <location filename="../src/T2DMap.cpp" line="1499"/>
      <location filename="../src/T2DMap.cpp" line="2310"/>
      <location filename="../src/T2DMap.cpp" line="2326"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>sin texto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="142"/>
      <source>ID</source>
      <comment>Room ID in the mapper widget</comment>
      <translation>Numero de identificación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="142"/>
      <source>Name</source>
      <comment>Room name in the mapper widget</comment>
      <translation>Nombre</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="144"/>
      <source>&lt;p&gt;Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection.  Click on the relevant header to sort by that method.  Note that the name column will only show if at least one of the rooms has a name.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="602"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1253"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/T2DMap.cpp" line="1252"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2244"/>
      <source>render time: %1S mO: (%2,%3,%4)</source>
      <comment>This is debug information that is not expected to be seen in release versions, %1 is a decimal time period and %2-%4 are the x,y and z coordinates at the center of the view (but y will be negative compared to previous room related ones as it represents the real coordinate system for this widget which has y increasing in a downward direction!)</comment>
      <translation>tiempo de renderizado: %1S mO: (%2,%3,%4)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2316"/>
      <source>Text label or image label?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2317"/>
      <source>Text Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Etiqueta de texto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2318"/>
      <source>Image Label</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Etiqueta de imagén</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2322"/>
      <source>Enter label text.</source>
      <comment>2D Mapper create label dialog title/text</comment>
      <translation>Introducir texto de la etiqueta.</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2329"/>
      <source>Background color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Color del fondo</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2330"/>
      <source>Foreground color</source>
      <comment>2D Mapper create label color dialog title</comment>
      <translation>Color de primer plano</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2334"/>
      <source>Select image</source>
      <comment>2D Mapper create label file dialog title</comment>
      <translation>Seleccionar imagen</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2341"/>
      <source>Draw label as background or on top of everything?</source>
      <comment>2D Mapper create label dialog text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2342"/>
      <source>Background</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Fondo</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2343"/>
      <source>Foreground</source>
      <comment>2D Mapper create label dialog button</comment>
      <translation>Primer plano</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2585"/>
      <source>Drag to select multiple rooms or labels, release to finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation>Arrastre para seleccionar múltiples habitaciones o etiquetas, suelte para terminar...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2689"/>
      <source>Undo</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Deshacer</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2690"/>
      <source>Undo last point</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Deshacer el último punto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2697"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)</comment>
      <translation>Propiedades</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2699"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)</comment>
      <translation>propiedades...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2700"/>
      <source>Change the properties of this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Cambia las propiedades de esta línea</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2703"/>
      <source>Finish</source>
      <comment>2D Mapper context menu (drawing custom exit line) item</comment>
      <translation>Finalizar</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2704"/>
      <source>Finish drawing this line</source>
      <comment>2D Mapper context menu (drawing custom exit line) item tooltip</comment>
      <translation>Completar el dibujo de esta línea</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2771"/>
      <source>Create new map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Crear nuevo mapa</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2774"/>
      <source>Load map</source>
      <comment>2D Mapper context menu (no map found) item</comment>
      <translation>Cargar mapa</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2788"/>
      <source>Create room</source>
      <comment>Menu option to create a new room in the mapper</comment>
      <translation>Crear habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2795"/>
      <source>Move</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Mover</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2796"/>
      <source>Move room</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Mover habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2799"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Eliminar</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2800"/>
      <source>Delete room</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Eliminar habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2803"/>
      <source>Color</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Color</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2804"/>
      <source>Change room color</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Cambia el color de la habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2807"/>
      <source>Spread</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Esparcir</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2808"/>
      <source>Increase map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Incrementa el espaciado X-Y del mapa para el grupo seleccionado de habitaciones</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2811"/>
      <source>Shrink</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Encoger</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2812"/>
      <source>Decrease map X-Y spacing for the selected group of rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Disminuye el espaciado X-Y del mapa para el grupo seleccionado de habitaciones</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2815"/>
      <source>Lock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Bloquear</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2816"/>
      <source>Lock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Bloquear habitación para speed walk</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2819"/>
      <source>Unlock</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Desbloquear</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2820"/>
      <source>Unlock room for speed walks</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Desbloquear habitación para recorrido rápido</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2823"/>
      <source>Weight</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Peso</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2824"/>
      <source>Set room weight</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Definir el peso de la habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2827"/>
      <source>Exits</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Salidas</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2828"/>
      <source>Set room exits</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Establece las salidas de la habitación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2831"/>
      <source>Symbol</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Símbolo</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2832"/>
      <source>Set one or more symbols or letters to mark special rooms</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Elija uno o más símbolos o letras para marcar habitaciones especiales</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2835"/>
      <source>Move to</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Mover a</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2836"/>
      <source>Move selected group to a given position</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Mueve el grupo elegido a la posición indicada</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2839"/>
      <source>Area</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Área</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2840"/>
      <source>Set room&apos;s area number</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2843"/>
      <source>Custom exit line</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2845"/>
      <source>Replace an exit line with a custom line</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Sustituir una línea de salida por una línea personalizada</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2849"/>
      <source>Custom exit lines are not shown and are not editable in grid mode</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation>Las líneas de salida personalizadas no se muestran y no son editables en el modo de cuadrícula</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2853"/>
      <source>Create Label</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2854"/>
      <source>Create labels to show text or images</source>
      <comment>2D Mapper context menu (room) item tooltip</comment>
      <translation>Crear etiquetas para mostrar texto o imágenes</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2873"/>
      <source>Set location</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation>Establecer ubicación</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2875"/>
      <source>Set player current location to here</source>
      <comment>2D Mapper context menu (room) item tooltip (enabled state)</comment>
      <translation>Establecer aquí la ubicación actual del jugador</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2879"/>
      <source>Can only set location when exactly one room is selected</source>
      <comment>2D Mapper context menu (room) item tooltip (disabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2886"/>
      <source>Switch to editing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2887"/>
      <source>Switch to viewing mode</source>
      <comment>2D Mapper context menu (room) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2897"/>
      <source>Move</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Mover</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2898"/>
      <source>Move label</source>
      <comment>2D Mapper context menu item (label) tooltip</comment>
      <translation>Mover Etiqueta</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2900"/>
      <source>Delete</source>
      <comment>2D Mapper context menu (label) item</comment>
      <translation>Eliminar</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2901"/>
      <source>Delete label</source>
      <comment>2D Mapper context menu (label) item tooltip</comment>
      <translation>Eliminar etiqueta</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2917"/>
      <source>Add point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Añadir punto</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2924"/>
      <source>Divide segment by adding a new point mid-way along</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2927"/>
      <source>Select a point first, then add a new point mid-way along the segment towards room</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2931"/>
      <source>Remove point</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2938"/>
      <source>Merge pair of segments by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2942"/>
      <source>Remove last segment by removing this point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2947"/>
      <source>use &quot;delete line&quot; to remove the only segment ending in an editable point</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action &quot;delete line&quot; should match the translation for that action)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2953"/>
      <source>Select a point first, then remove it</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)</comment>
      <translation>Seleccione un punto primero, luego eliminelo</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2957"/>
      <source>Properties</source>
      <comment>2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)</comment>
      <translation>Propiedades</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2961"/>
      <source>properties...</source>
      <comment>2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise</comment>
      <translation>propiedades...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2965"/>
      <source>Delete line</source>
      <comment>2D Mapper context menu (custom line editing) item</comment>
      <translation>Borrar línea</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2966"/>
      <source>Delete all of this custom line</source>
      <comment>2D Mapper context menu (custom line editing) item tooltip</comment>
      <translation>Eliminar todo de esta línea personalizada</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3478"/>
      <source>Move the selection, centered on
the highlighted room (%1) to:</source>
      <comment>Use linefeeds as necessary to format text into a reasonable rectangle of text, %1 is a room number</comment>
      <translation type="unfinished"/>
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
      <translation>Cancelar</translation>
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
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3763"/>
      <source>Spread out rooms</source>
      <translation>Separar habitaciones</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3764"/>
      <source>Increase the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>Aumentar el espaciado de
las habitaciones seleccionadas,
centradas en la habitación
resaltada por un factor
de:</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3823"/>
      <source>Shrink in rooms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3824"/>
      <source>Decrease the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3952"/>
      <location filename="../src/T2DMap.cpp" line="3966"/>
      <location filename="../src/T2DMap.cpp" line="4016"/>
      <source>Enter room weight</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="3953"/>
      <source>Enter new roomweight
(= travel time), minimum
(and default) is 1:</source>
      <comment>Use line feeds to format text into a reasonable rectangle.</comment>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4005"/>
      <source>%1 {count:%2, default}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4013"/>
      <source>1 {count 0, default}</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4066"/>
      <source>Load Mudlet map</source>
      <translation>Cargar mapa de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4068"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) or the ;;s as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4955"/>
      <location filename="../src/T2DMap.cpp" line="4989"/>
      <source>Left-click to add point, right-click to undo/change/finish...</source>
      <comment>2D Mapper big, bottom of screen help message</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5000"/>
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
      <location filename="../src/TArea.cpp" line="766"/>
      <source>no text</source>
      <comment>Default text if a label is created in mapper with no text</comment>
      <translation>sin texto</translation>
    </message>
  </context>
  <context>
    <name>TCommandLine</name>
    <message>
      <location filename="../src/TCommandLine.cpp" line="661"/>
      <source>Add to user dictionary</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="663"/>
      <source>Remove from user dictionary</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="667"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which we have bundled with Mudlet; the entries about this line are the ones that the user has personally added.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="677"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <comment>This line is shown in the list of spelling suggestions on the profile&apos;s command-line context menu to clearly divide up where the suggestions for correct spellings are coming from.  The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which is provided as part of the OS; the entries about this line are the ones that the user has personally added.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="745"/>
      <source>no suggestions (system)</source>
      <comment>used when the command spelling checker using the selected system dictionary has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="772"/>
      <source>no suggestions (shared)</source>
      <comment>used when the command spelling checker using the dictionary shared between profile has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="776"/>
      <source>no suggestions (profile)</source>
      <comment>used when the command spelling checker using the profile&apos;s own dictionary has no words to suggest</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TConsole</name>
    <message>
      <location filename="../src/TConsole.cpp" line="113"/>
      <source>Debug Console</source>
      <translation>Consola de depuración</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="771"/>
      <source>Save profile?</source>
      <translation>Guardar Perfil?</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="771"/>
      <source>Do you want to save the profile %1?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="784"/>
      <source>Couldn&apos;t save profile</source>
      <translation>No fue posible guardar el perfil</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="784"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1697"/>
      <source>System Message: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="349"/>
      <source>Show Time Stamps.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="361"/>
      <source>Record a replay.</source>
      <translation>Graba una repetición.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="372"/>
      <source>Start logging game output to log file.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="383"/>
      <source>&lt;i&gt;N:&lt;/i&gt; is the latency of the game server and network (aka ping, in seconds), &lt;br&gt;&lt;i&gt;S:&lt;/i&gt; is the system processing time - how long your triggers took to process the last line(s).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="416"/>
      <source>Emergency Stop. Stops all timers and triggers.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="432"/>
      <source>Search buffer.</source>
      <translation>Búfer de búsqueda.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="440"/>
      <source>Earlier search result.</source>
      <translation>Resultado de la búsqueda anterior.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="451"/>
      <source>Later search result.</source>
      <translation>Resultado de la búsqueda posterior.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="868"/>
      <source>Replay recording has started. File: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="871"/>
      <source>Replay recording has been stopped. File: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1850"/>
      <location filename="../src/TConsole.cpp" line="1889"/>
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
      <location filename="../src/TLuaInterpreter.cpp" line="9976"/>
      <source>Playing %1</source>
      <translation>Reproduciendo %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12319"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12340"/>
      <source>ERROR</source>
      <translation>ERROR</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12320"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12331"/>
      <source>object</source>
      <comment>object is the Mudlet alias/trigger/script, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="12320"/>
      <location filename="../src/TLuaInterpreter.cpp" line="12331"/>
      <source>function</source>
      <comment>function is the Lua function, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</comment>
      <translation>función</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13804"/>
      <source>Some functions may not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13228"/>
      <source>No error message available from Lua</source>
      <translation>No hay mensaje de error disponible de Lua</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13230"/>
      <source>Lua error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13232"/>
      <source>[ ERROR ] - Cannot find Lua module %1.%2%3%4</source>
      <comment>%1 is the name of the module;%2 will be a line-feed inserted to put the next argument on a new line;%3 is the error message from the lua sub-system;%4 can be an additional message about the expected effect (but may be blank).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13246"/>
      <source>[  OK  ]  - Lua module %1 loaded.</source>
      <comment>%1 is the name (may specify which variant) of the module.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13791"/>
      <source>Probably will not be able to access Mudlet Lua code.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13809"/>
      <source>Database support will not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13816"/>
      <source>utf8.* Lua functions won&apos;t be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13822"/>
      <source>yajl.* Lua functions won&apos;t be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14003"/>
      <source>No error message available from Lua.</source>
      <translation>No hay mensaje de error disponible de Lua.</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14005"/>
      <source>Lua error: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14007"/>
      <source>[ ERROR ] - Cannot load code formatter, indenting functionality won&apos;t be available.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14084"/>
      <source>%1 (doesn&apos;t exist)</source>
      <comment>This file doesn&apos;t exist</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14089"/>
      <source>%1 (isn&apos;t a file or symlink to a file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14098"/>
      <source>%1 (isn&apos;t a readable file or symlink to a readable file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14116"/>
      <source>%1 (couldn&apos;t read file)</source>
      <comment>This file could not be read for some reason (for example, no permission)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14122"/>
      <source>[  OK  ]  - Mudlet-lua API &amp; Geyser Layout manager loaded.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="14129"/>
      <source>[ ERROR ] - Couldn&apos;t find, load and successfully run LuaGlobal.lua - your Mudlet is broken!
Tried these locations:
%1</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMainConsole</name>
    <message>
      <location filename="../src/TMainConsole.cpp" line="161"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {2 of 2}).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="202"/>
      <source>Logging has started. Log file is %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="212"/>
      <source>Logging has been stopped. Log file is %1
</source>
      <translation>Registro detenido. Archivo: %1
</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="242"/>
      <source>Mudlet MUD Client version: %1%2</source>
      <translation>Versión del cliente Mudlet: %1%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="244"/>
      <source>Mudlet, log from %1 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="280"/>
      <location filename="../src/TMainConsole.cpp" line="302"/>
      <source>&apos;Log session starting at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="307"/>
      <source>&lt;p&gt;Stop logging game output to log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="311"/>
      <source>&apos;Log session ending at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <comment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="324"/>
      <source>&lt;p&gt;Start logging game output to log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="637"/>
      <source>Pre-Map loading(2) report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="647"/>
      <source>Loading map(2) at %1 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1047"/>
      <source>User window - %1 - %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1095"/>
      <source>N:%1 S:%2</source>
      <comment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1102"/>
      <source>&lt;no GA&gt; S:%1</source>
      <comment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1199"/>
      <source>Pre-Map loading(1) report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1217"/>
      <source>Loading map(1) at %1 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1219"/>
      <source>Loading map(1) &quot;%1&quot; at %2 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1263"/>
      <source>Pre-Map importing(1) report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1286"/>
      <source>[ ERROR ]  - Map file not found, path and name used was:
%1.</source>
      <translation>[ ERROR ] - Archivo de mapa no encontrado, ruta de acceso y nombre utilizado fue:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1292"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; was not found).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1301"/>
      <source>[ INFO ]  - Map file located and opened, now parsing it...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1308"/>
      <source>Importing map(1) &quot;%1&quot; at %2 report</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1311"/>
      <source>[ INFO ]  - Map file located but it could not opened, please check permissions on:&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1314"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; could not be opened for reading).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1338"/>
      <source>[ INFO ]  - Map reload request received from system...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1343"/>
      <source>[  OK  ]  - ... System Map reload request completed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1345"/>
      <source>[ WARN ]  - ... System Map reload request failed.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMap</name>
    <message>
      <location filename="../src/TMap.cpp" line="210"/>
      <source>RoomID=%1 does not exist, can not set AreaID=%2 for non-existing room!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="221"/>
      <source>AreaID=%2 does not exist, can not set RoomID=%1 to non-existing area!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="469"/>
      <source>[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="472"/>
      <source>[ INFO ] - Converting old style label id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="477"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="480"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label with id: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="509"/>
      <source>[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="516"/>
      <source>[  OK  ]  - Map loaded successfully (%1s).</source>
      <translation>[ OK] - Mapa cargado correctamente (%1s).</translation>
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
      <translation>ne</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="989"/>
      <source>e</source>
      <comment>This translation converts the direction that DIR_EAST codes for to a direction string that the game server will accept!</comment>
      <translation>e</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="990"/>
      <source>se</source>
      <comment>This translation converts the direction that DIR_SOUTHEAST codes for to a direction string that the game server will accept!</comment>
      <translation>se</translation>
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
      <translation>so</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="993"/>
      <source>w</source>
      <comment>This translation converts the direction that DIR_WEST codes for to a direction string that the game server will accept!</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="994"/>
      <source>nw</source>
      <comment>This translation converts the direction that DIR_NORTHWEST codes for to a direction string that the game server will accept!</comment>
      <translation>no</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="995"/>
      <source>up</source>
      <comment>This translation converts the direction that DIR_UP codes for to a direction string that the game server will accept!</comment>
      <translation>arriba</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="996"/>
      <source>down</source>
      <comment>This translation converts the direction that DIR_DOWN codes for to a direction string that the game server will accept!</comment>
      <translation>abajo</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="997"/>
      <source>in</source>
      <comment>This translation converts the direction that DIR_IN codes for to a direction string that the game server will accept!</comment>
      <translation>adentro</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="998"/>
      <source>out</source>
      <comment>This translation converts the direction that DIR_OUT codes for to a direction string that the game server will accept!</comment>
      <translation>afuera</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1037"/>
      <source>[ ALERT ] - Saving map in a format {%1} that is different than the one it was
loaded as {%2}. This may be an issue if you want to share the resulting
map with others relying on the original format.</source>
      <translation>[ ALERTA ] - Guardando el mapa en un formato {%1} diferente al formato en el que fue cargado {%2}. Esto podría resultar un problema si quieres compartir el mapa resultante con otros usuarios que estén usando el formato original.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="187"/>
      <source>[MAP ERROR:]%1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="48"/>
      <source>Default Area</source>
      <translation>Área Predeterminada</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="49"/>
      <source>Unnamed Area</source>
      <translation>Área sin nombre</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="439"/>
      <source>[ INFO ]  - Map audit starting...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1021"/>
      <source>[ ERROR ] - The format {%1} you are trying to save the map with is too new
for this version of Mudlet. Supported are only formats up to version {%2}.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1047"/>
      <source>[ WARN ]  - Saving map in a format {%1} different from the
recommended format {%2} for this version of Mudlet.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1398"/>
      <location filename="../src/TMap.cpp" line="1753"/>
      <source>[ ERROR ] - Unable to open (for reading) map file: &quot;%1&quot;!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1420"/>
      <source>[ ERROR ] - Map file is too new, its file format (%1) is higher than this version of
Mudlet can handle (%2)!  The file is:
&quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1427"/>
      <source>[ INFO ]  - You will need to upgrade your Mudlet or find a map file saved in an
older format.</source>
      <translation>[ INFO ]  - Necesitarás actualizar Mudlet a una versión posterior o encontrar un archivo de mapa guardado en un formato anterior.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1434"/>
      <source>[ ALERT ] - Map file is really old, its file format (%1) is so ancient that
this version of Mudlet may not gain enough information from
it but it will try!  The file is: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1441"/>
      <source>[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!
There is so much data that it DOES NOT have that you could be
better off starting again...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1450"/>
      <source>[ INFO ]  - Reading map (format version:%1) file:
&quot;%2&quot;,
please wait...</source>
      <translation>[ INFO ]  - Leyendo mapa (versión del formato:%1) archivo:
&quot;%2&quot;,
por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1451"/>
      <source>[ INFO ]  - Reading map (format version:%1) file: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1600"/>
      <source>[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an
area) not found, adding reserved -1 id.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1694"/>
      <source>[ INFO ]  - Successfully read the map file (%1s), checking some
consistency details...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1709"/>
      <source>No map found. Would you like to download the map or start your own?</source>
      <translation>Mapa no encontrado. Deseas descargar el mapa o empezar uno propio?</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1710"/>
      <source>Download the map</source>
      <translation>Descargar el mapa</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1711"/>
      <source>Start my own</source>
      <translation>Iniciar mi propio</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1769"/>
      <source>[ INFO ]  - Checking map file: &quot;%1&quot;, format version:%2...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2157"/>
      <source>Map issues</source>
      <translation>Problemas de mapa</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2164"/>
      <source>Area issues</source>
      <translation>Problemas de área</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2170"/>
      <source>Area id: %1 &quot;%2&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2172"/>
      <source>Area id: %1</source>
      <translation>Id de área: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2181"/>
      <source>Room issues</source>
      <translation>Problemas de habitación</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2188"/>
      <source>Room id: %1 &quot;%2&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2190"/>
      <source>Room id: %1</source>
      <translation>Id de habitación: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2200"/>
      <source>End of report</source>
      <translation>Fin del reporte</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2206"/>
      <source>[ ALERT ] - At least one thing was detected during that last map operation
that it is recommended that you review the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2214"/>
      <source>[ INFO ]  - The equivalent to the above information about that last map
operation has been saved for review as the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2270"/>
      <source>[ ERROR ] - Unable to use or create directory to store map.
Please check that you have permissions/access to:
&quot;%1&quot;
and there is enough space. The download operation has failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2296"/>
      <source>[ INFO ]  - Map download initiated, please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2307"/>
      <source>Map download</source>
      <comment>This is a title of a progress window.</comment>
      <translation>Descarga de mapa</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2370"/>
      <source>Map import</source>
      <comment>This is a title of a progress dialog.</comment>
      <translation>Importar mapa</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2670"/>
      <location filename="../src/TMap.cpp" line="3141"/>
      <source>Exporting JSON map data from %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2685"/>
      <source>Map JSON export</source>
      <comment>This is a title of a progress window.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2820"/>
      <source>Exporting JSON map file from %1 - writing data to file:
%2 ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2907"/>
      <source>Map JSON import</source>
      <comment>This is a title of a progress window.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2892"/>
      <location filename="../src/TMap.cpp" line="3151"/>
      <source>Importing JSON map data to %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2306"/>
      <location filename="../src/TMap.cpp" line="2679"/>
      <location filename="../src/TMap.cpp" line="2901"/>
      <source>Abort</source>
      <translation>Abortar</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2304"/>
      <source>Downloading map file for use in %1...</source>
      <comment>%1 is the name of the current Mudlet profile</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2338"/>
      <source>loadMap: unable to perform request, a map is already being downloaded or
imported at user request.</source>
      <translation>loadMap: no se puede cargar el mapa, ya hay unos descargandose o siendo
importado a petición del usuario.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2369"/>
      <source>Importing XML map file for use in %1...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2397"/>
      <source>loadMap: failure to import XML map file, further information may be available
in main console!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2432"/>
      <source>[ ALERT ] - Map download was canceled, on user&apos;s request.</source>
      <translation>[ ALERTA ] - La descarga de mapa ha sido cancelada, a petición del usuario.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2451"/>
      <source>[ ERROR ] - Map download encountered an error:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2482"/>
      <source>[ ALERT ] - Map download failed, error reported was:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2490"/>
      <source>[ ALERT ] - Map download failed, unable to open destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2495"/>
      <source>[ ALERT ] - Map download failed, unable to write destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2508"/>
      <location filename="../src/TMap.cpp" line="2525"/>
      <source>[ INFO ]  - ... map downloaded and stored, now parsing it...</source>
      <translation>[ INFO ]  - ... mapa descargado y almacenado, analizándolo...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2516"/>
      <location filename="../src/TMap.cpp" line="2551"/>
      <source>[ ERROR ] - Map download problem, failure in parsing destination file:
%1.</source>
      <translation>[ ERROR ] -Problema de descarga de mapa, fallo en el análisis del archivo de destino:
%1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2556"/>
      <source>[ ERROR ] - Map download problem, unable to read destination file:
%1.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TRoom</name>
    <message>
      <location filename="../src/TRoom.cpp" line="86"/>
      <location filename="../src/TRoom.cpp" line="971"/>
      <source>North</source>
      <translation>Norte</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="87"/>
      <source>North-east</source>
      <translation>Noreste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="88"/>
      <source>North-west</source>
      <translation>Noroeste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="89"/>
      <location filename="../src/TRoom.cpp" line="1013"/>
      <source>South</source>
      <translation>Sur</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="90"/>
      <source>South-east</source>
      <translation>Sureste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="91"/>
      <source>South-west</source>
      <translation>Suroeste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="92"/>
      <location filename="../src/TRoom.cpp" line="1055"/>
      <source>East</source>
      <translation>Este</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="93"/>
      <location filename="../src/TRoom.cpp" line="1069"/>
      <source>West</source>
      <translation>Oeste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="94"/>
      <location filename="../src/TRoom.cpp" line="1083"/>
      <source>Up</source>
      <translation>Arriba</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="95"/>
      <location filename="../src/TRoom.cpp" line="1097"/>
      <source>Down</source>
      <translation>Down</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="96"/>
      <location filename="../src/TRoom.cpp" line="1111"/>
      <source>In</source>
      <translation>Adentro</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="97"/>
      <location filename="../src/TRoom.cpp" line="1125"/>
      <source>Out</source>
      <translation>Afuera</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="98"/>
      <source>Other</source>
      <translation>Otro</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="100"/>
      <source>Unknown</source>
      <translation>Desconocido</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="311"/>
      <source>No area created!  Requested area ID=%1. Note: Area IDs must be &gt; 0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="333"/>
      <source>Warning: When setting the Area for Room (Id: %1) it did not have a current area!</source>
      <translation>Advertencia: Al establecer el Área de Habitación (Id: %1) no tiene un área actual!</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="985"/>
      <source>Northeast</source>
      <translation>Noreste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="999"/>
      <source>Northwest</source>
      <translation>Noroeste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1027"/>
      <source>Southeast</source>
      <translation>Sureste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1041"/>
      <source>Southwest</source>
      <translation>Suroeste</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1146"/>
      <source>[ WARN ]  - In room id:%1 removing invalid (special) exit to %2 {with no name!}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1150"/>
      <source>[ WARN ]  - Room had an invalid (special) exit to %1 {with no name!} it was removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1159"/>
      <source>[ INFO ]  - In room with id: %1 correcting special exit &quot;%2&quot; that
was to room with an exit to invalid room: %3 to now go
to: %4.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1169"/>
      <source>[ INFO ]  - Room needed correcting of special exit &quot;%1&quot; that was to room with an exit to invalid room: %2 to now go to: %3.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1194"/>
      <source>[ WARN ]  - Room with id: %1 has a special exit &quot;%2&quot; with an
exit to: %3 but that room does not exist.  The exit will
be removed (but the destination room id will be stored in
the room user data under a key:
&quot;%4&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1207"/>
      <source>[ WARN ]  - Room has a special exit &quot;%1&quot; with an exit to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1246"/>
      <source>[ INFO ]  - In room with id: %1 special exit &quot;%2&quot;
that was to room with an invalid room: %3 that does not exist.
The exit will be removed (the bad destination room id will be stored in the
room user data under a key:
&quot;%4&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1259"/>
      <source>[ INFO ]  - Room had special exit &quot;%1&quot; that was to room with an invalid room: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%3&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1292"/>
      <source>%1 {none}</source>
      <translation>%1 {none}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1295"/>
      <source>%1 (open)</source>
      <translation>%1 (abierto)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1298"/>
      <source>%1 (closed)</source>
      <translation>%1 (cerrado)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1301"/>
      <source>%1 (locked)</source>
      <translation>%1 (bloqueado)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1304"/>
      <source>%1 {invalid}</source>
      <translation>%1 {invalid}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1308"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus door items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1315"/>
      <source>[ INFO ]  - Room had one or more surplus door items that were removed:%1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1331"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus weight items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1338"/>
      <source>[ INFO ]  - Room had one or more surplus weight items that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1354"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus exit lock items that were removed:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1361"/>
      <source>[ INFO ]  - Room had one or more surplus exit lock items that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1440"/>
      <source>[ INFO ]  - In room with id: %1 found one or more surplus custom line elements that
were removed: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1446"/>
      <source>[ INFO ]  - Room had one or more surplus custom line elements that were removed: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1469"/>
      <source>[ INFO ]  - In room with id: %1 correcting exit &quot;%2&quot; that was to room with
an exit to invalid room: %3 to now go to: %4.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1478"/>
      <source>[ INFO ]  - Correcting exit &quot;%1&quot; that was to invalid room id: %2 to now go to: %3.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1489"/>
      <source>[ WARN ]  - Room with id: %1 has an exit &quot;%2&quot; to: %3 but that room
does not exist.  The exit will be removed (but the destination room
Id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1502"/>
      <source>[ WARN ]  - Room has an exit &quot;%1&quot; to: %2 but that room does not exist.  The exit will be removed (but the destination room id will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1546"/>
      <source>[ ALERT ] - Room with id: %1 has an exit &quot;%2&quot; to: %3 but also
has a stub exit!  As a real exit precludes a stub, the latter will
be removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1556"/>
      <source>[ ALERT ] - Room has an exit &quot;%1&quot; to: %2 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1612"/>
      <source>[ INFO ]  - In room with id: %1 exit &quot;%2&quot; that was to room with an invalid
room: %3 that does not exist.  The exit will be removed (the bad destination
room id will be stored in the room user data under a key:
&quot;%4&quot;)
and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1623"/>
      <source>[ INFO ]  - Room exit &quot;%1&quot; that was to a room with an invalid id: %2 that does not exist.  The exit will be removed (the bad destination room id will be stored in the room user data under a key:&quot;%4&quot;) and the exit will be turned into a stub.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1639"/>
      <source>
It was locked, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1643"/>
      <source>  It was locked, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1651"/>
      <source>
It had a weight, this is recorded as user data with key:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1655"/>
      <source>  It had a weight, this is recorded as user data with key: &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1666"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but
it has not been possible to salvage this, it has been lost!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1671"/>
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
      <location filename="../src/TRoomDB.cpp" line="724"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that are supposed
 to be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="731"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
 Look for further messsages related to the rooms that is/are supposed to
 be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
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
      <location filename="../src/TRoomDB.cpp" line="780"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1) in map!  Look for further messsages related to this for each affected area ...</source>
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
    <message>
      <location filename="../src/TRoomDB.cpp" line="859"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!  Look for further messsages related to this for each affected room ...</source>
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
      <translation>&lt;nada&gt;</translation>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TTextEdit</name>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1297"/>
      <source>Copy</source>
      <translation>Copiar</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1308"/>
      <source>Copy HTML</source>
      <translation>Copiar Html</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1312"/>
      <source>Copy as image</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1315"/>
      <source>Select All</source>
      <translation>Seleccionar todo</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1320"/>
      <source>Search on %1</source>
      <translation>Buscar en %1</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1338"/>
      <source>Analyse characters</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1347"/>
      <source>&lt;p&gt;Hover on this item to display the Unicode codepoints in the selection &lt;i&gt;(only the first line!)&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1356"/>
      <source>restore Main menu</source>
      <translation>restaurar Menú principal</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1358"/>
      <source>Use this to restore the Main menu to get access to controls.</source>
      <translation>Utilice esta opción para restaurar el menú Principal para obtener acceso a los controles.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1360"/>
      <source>restore Main Toolbar</source>
      <translation>restaurar la Barra de herramientas Principal</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1362"/>
      <source>Use this to restore the Main Toolbar to get access to controls.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1438"/>
      <source>Mudlet, debug console extract</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1440"/>
      <source>Mudlet, %1 mini-console extract from %2 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1442"/>
      <source>Mudlet, %1 user window extract from %2 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1444"/>
      <source>Mudlet, main console extract from %1 profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1912"/>
      <source>{tab}</source>
      <comment>Unicode U+0009 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1913"/>
      <source>{line-feed}</source>
      <comment>Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1914"/>
      <source>{carriage-return}</source>
      <comment>Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1915"/>
      <source>{space}</source>
      <comment>Unicode U+0020 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1916"/>
      <source>{non-breaking space}</source>
      <comment>Unicode U+00A0 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1917"/>
      <source>{soft hyphen}</source>
      <comment>Unicode U+00AD codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1918"/>
      <source>{combining grapheme joiner}</source>
      <comment>Unicode U+034F codepoint (badly named apparently - see Wikipedia!)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1919"/>
      <source>{ogham space mark}</source>
      <comment>Unicode U+1680 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1920"/>
      <source>{&apos;n&apos; quad}</source>
      <comment>Unicode U+2000 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1921"/>
      <source>{&apos;m&apos; quad}</source>
      <comment>Unicode U+2001 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1922"/>
      <source>{&apos;n&apos; space}</source>
      <comment>Unicode U+2002 codepoint - En (&apos;n&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1923"/>
      <source>{&apos;m&apos; space}</source>
      <comment>Unicode U+2003 codepoint - Em (&apos;m&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1924"/>
      <source>{3-per-em space}</source>
      <comment>Unicode U+2004 codepoint - three-per-em (&apos;m&apos;) wide (thick) space.</comment>
      <translation>{3-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1925"/>
      <source>{4-per-em space}</source>
      <comment>Unicode U+2005 codepoint - four-per-em (&apos;m&apos;) wide (Middle) space.</comment>
      <translation>{4-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1926"/>
      <source>{6-per-em space}</source>
      <comment>Unicode U+2006 codepoint - six-per-em (&apos;m&apos;) wide (Sometimes the same as a Thin) space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1927"/>
      <source>{digit space}</source>
      <comment>Unicode U+2007 codepoint - figure (digit) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1928"/>
      <source>{punctuation wide space}</source>
      <comment>Unicode U+2008 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1929"/>
      <source>{5-per-em space}</source>
      <comment>Unicode U+2009 codepoint - five-per-em (&apos;m&apos;) wide space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1930"/>
      <source>{hair width space}</source>
      <comment>Unicode U+200A codepoint - thinnest space.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1931"/>
      <source>{zero width space}</source>
      <comment>Unicode U+200B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1932"/>
      <source>{Zero width non-joiner}</source>
      <comment>Unicode U+200C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1933"/>
      <source>{zero width joiner}</source>
      <comment>Unicode U+200D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1934"/>
      <source>{left-to-right mark}</source>
      <comment>Unicode U+200E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1935"/>
      <source>{right-to-left mark}</source>
      <comment>Unicode U+200F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1936"/>
      <source>{line separator}</source>
      <comment>Unicode 0x2028 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1937"/>
      <source>{paragraph separator}</source>
      <comment>Unicode U+2029 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1938"/>
      <source>{Left-to-right embedding}</source>
      <comment>Unicode U+202A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1939"/>
      <source>{right-to-left embedding}</source>
      <comment>Unicode U+202B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1940"/>
      <source>{pop directional formatting}</source>
      <comment>Unicode U+202C codepoint - pop (undo last) directional formatting.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1941"/>
      <source>{Left-to-right override}</source>
      <comment>Unicode U+202D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1942"/>
      <source>{right-to-left override}</source>
      <comment>Unicode U+202E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1943"/>
      <source>{narrow width no-break space}</source>
      <comment>Unicode U+202F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1944"/>
      <source>{medium width mathematical space}</source>
      <comment>Unicode U+205F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1945"/>
      <source>{zero width non-breaking space}</source>
      <comment>Unicode U+2060 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1946"/>
      <source>{function application}</source>
      <comment>Unicode U+2061 codepoint - function application (whatever that means!)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1947"/>
      <source>{invisible times}</source>
      <comment>Unicode U+2062 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1948"/>
      <source>{invisible separator}</source>
      <comment>Unicode U+2063 codepoint - invisible separator or comma.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1949"/>
      <source>{invisible plus}</source>
      <comment>Unicode U+2064 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1950"/>
      <source>{left-to-right isolate}</source>
      <comment>Unicode U+2066 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1951"/>
      <source>{right-to-left isolate}</source>
      <comment>Unicode U+2067 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1952"/>
      <source>{first strong isolate}</source>
      <comment>Unicode U+2068 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1953"/>
      <source>{pop directional isolate}</source>
      <comment>Unicode U+2069 codepoint - pop (undo last) directional isolate.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1954"/>
      <source>{inhibit symmetrical swapping}</source>
      <comment>Unicode U+206A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1955"/>
      <source>{activate symmetrical swapping}</source>
      <comment>Unicode U+206B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1956"/>
      <source>{inhibit arabic form-shaping}</source>
      <comment>Unicode U+206C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1957"/>
      <source>{activate arabic form-shaping}</source>
      <comment>Unicode U+206D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1958"/>
      <source>{national digit shapes}</source>
      <comment>Unicode U+206E codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1959"/>
      <source>{nominal Digit shapes}</source>
      <comment>Unicode U+206F codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1960"/>
      <source>{ideaographic space}</source>
      <comment>Unicode U+3000 codepoint - ideaographic (CJK Wide) space</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1961"/>
      <source>{variation selector 1}</source>
      <comment>Unicode U+FE00 codepoint.</comment>
      <translation>{variation selector 1}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1962"/>
      <source>{variation selector 2}</source>
      <comment>Unicode U+FE01 codepoint.</comment>
      <translation>{variation selector 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1963"/>
      <source>{variation selector 3}</source>
      <comment>Unicode U+FE02 codepoint.</comment>
      <translation>{variation selector 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1964"/>
      <source>{variation selector 4}</source>
      <comment>Unicode U+FE03 codepoint.</comment>
      <translation>{variation selector 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1965"/>
      <source>{variation selector 5}</source>
      <comment>Unicode U+FE04 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1966"/>
      <source>{variation selector 6}</source>
      <comment>Unicode U+FE05 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1967"/>
      <source>{variation selector 7}</source>
      <comment>Unicode U+FE06 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1968"/>
      <source>{variation selector 8}</source>
      <comment>Unicode U+FE07 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1969"/>
      <source>{variation selector 9}</source>
      <comment>Unicode U+FE08 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1970"/>
      <source>{variation selector 10}</source>
      <comment>Unicode U+FE09 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1971"/>
      <source>{variation selector 11}</source>
      <comment>Unicode U+FE0A codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1972"/>
      <source>{variation selector 12}</source>
      <comment>Unicode U+FE0B codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1973"/>
      <source>{variation selector 13}</source>
      <comment>Unicode U+FE0C codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1974"/>
      <source>{variation selector 14}</source>
      <comment>Unicode U+FE0D codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1975"/>
      <source>{variation selector 15}</source>
      <comment>Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black &amp; white) rendition.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1976"/>
      <source>{variation selector 16}</source>
      <comment>Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured &apos;Emoji&apos; rendition.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1977"/>
      <source>{zero width no-break space}</source>
      <comment>Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1983"/>
      <source>{interlinear annotation anchor}</source>
      <comment>Unicode U+FFF9 codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1984"/>
      <source>{interlinear annotation separator}</source>
      <comment>Unicode U+FFFA codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1985"/>
      <source>{interlinear annotation terminator}</source>
      <comment>Unicode U+FFFB codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1986"/>
      <source>{object replacement character}</source>
      <comment>Unicode U+FFFC codepoint.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="1998"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFD0 to U+FDEF - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2000"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint in range U+FFFx - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2009"/>
      <source>{FitzPatrick modifier 1 or 2}</source>
      <comment>Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.</comment>
      <translation>{FitzPatrick modifier 1 or 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2010"/>
      <source>{FitzPatrick modifier 3}</source>
      <comment>Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.</comment>
      <translation>{FitzPatrick modifier 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2011"/>
      <source>{FitzPatrick modifier 4}</source>
      <comment>Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.</comment>
      <translation>{FitzPatrick modifier 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2012"/>
      <source>{FitzPatrick modifier 5}</source>
      <comment>Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2013"/>
      <source>{FitzPatrick modifier 6}</source>
      <comment>Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2017"/>
      <source>{noncharacter}</source>
      <comment>Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2288"/>
      <location filename="../src/TTextEdit.cpp" line="2350"/>
      <source>Index (UTF-16)</source>
      <comment>1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2291"/>
      <location filename="../src/TTextEdit.cpp" line="2352"/>
      <source>U+&lt;i&gt;####&lt;/i&gt; Unicode Code-point &lt;i&gt;(High:Low Surrogates)&lt;/i&gt;</source>
      <comment>2nd Row heading for Text analyser output, table item is the unicode code point (will be between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2295"/>
      <location filename="../src/TTextEdit.cpp" line="2356"/>
      <source>Visual</source>
      <comment>3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a &apos;{&apos;...&apos;}&apos; wrapped letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2299"/>
      <location filename="../src/TTextEdit.cpp" line="2360"/>
      <source>Index (UTF-8)</source>
      <comment>4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2303"/>
      <location filename="../src/TTextEdit.cpp" line="2364"/>
      <source>Byte</source>
      <comment>5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2307"/>
      <location filename="../src/TTextEdit.cpp" line="2368"/>
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
      <location filename="../src/TTrigger.cpp" line="191"/>
      <source>Error: This trigger has no patterns defined, yet. Add some to activate it.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="222"/>
      <source>Error: in item %1, perl regex &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="243"/>
      <source>Error: in item %1, lua function &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="263"/>
      <source>Error: in item %1, no colors to match were set - at least &lt;i&gt;one&lt;/i&gt; of the foreground or background must not be &lt;i&gt;ignored&lt;/i&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="319"/>
      <location filename="../src/TTrigger.cpp" line="389"/>
      <source>[Trigger Error:] %1 capture group limit exceeded, capture less groups.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="1148"/>
      <source>Trigger name=%1 expired.
</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TTrigger.cpp" line="1151"/>
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
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="388"/>
      <source>Install update now</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../3rdparty/dblsqd/dblsqd/update_dialog.ui" line="395"/>
      <source>OK</source>
      <translation>OK</translation>
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
      <translation>Actualización</translation>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="352"/>
      <source>Restart to apply update</source>
      <translation>Reinicie para aplicar la actualización</translation>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="348"/>
      <source>Parsing area data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="352"/>
      <source>Parsing room data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="356"/>
      <source>Parsing environment data...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="364"/>
      <source>Assigning rooms to their areas...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="519"/>
      <source>Parsing room data [count: %1]...</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>about_dialog</name>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="41"/>
      <source>About Mudlet</source>
      <translation>Acerca De Mudlet</translation>
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
      <translation>Licencia</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="249"/>
      <source>Third Party</source>
      <translation>De terceros</translation>
    </message>
  </context>
  <context>
    <name>actions_main_area</name>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="115"/>
      <source>Button Bar Properties</source>
      <translation>Propiedades de la barra de botones</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="127"/>
      <source>Number of columns/rows (depending on orientation):</source>
      <translation>Número de columnas/filas (dependiendo de la orientación):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="150"/>
      <source>Orientation Horizontal</source>
      <translation>Orientación Horizontal</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="155"/>
      <source>Orientation Vertical</source>
      <translation>Orientación Vertical</translation>
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
      <translation>Barra De Herramientas Flotante</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="208"/>
      <source>Button Properties</source>
      <translation>Propiedades Del Botón</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="214"/>
      <source>Button Rotation:</source>
      <translation>Rotación de Botón:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="231"/>
      <source>no rotation</source>
      <translation>sin rotación</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="236"/>
      <source>90° rotation to the left</source>
      <translation>Rotación de 90° a la izquierda</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="241"/>
      <source>90° rotation to the right</source>
      <translation>Rotación de 90° a la derecha</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="249"/>
      <source>Push down button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="256"/>
      <source>Command:</source>
      <translation>Comando:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="266"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game if it is pressed. (Optional)&lt;/p&gt;&lt;p&gt;If this is a &lt;i&gt;push-down&lt;/i&gt; button then this is sent only when the button goes from the &lt;i&gt;up&lt;/i&gt; to &lt;i&gt;down&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="269"/>
      <location filename="../src/ui/actions_main_area.ui" line="289"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="286"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game when this button goes from the &lt;i&gt;down&lt;/i&gt; to &lt;i&gt;up&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
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
      <location filename="../src/ui/actions_main_area.ui" line="308"/>
      <source>Stylesheet:</source>
      <translation>Hoja de estilo:</translation>
    </message>
  </context>
  <context>
    <name>aliases_main_area</name>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="35"/>
      <source>Name:</source>
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="57"/>
      <source>choose a unique name for your alias; it will show in the tree and is needed for scripting.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="76"/>
      <source>Pattern:</source>
      <translation>Patrón:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="103"/>
      <source>enter a perl regex pattern for your alias; alias are triggers on your input</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="110"/>
      <source>Type:</source>
      <translation>Tipo:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="124"/>
      <source>Regex</source>
      <translation>Regex</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="129"/>
      <source>Plain</source>
      <translation>Plano</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="149"/>
      <source>Command:</source>
      <translation>Comando:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="171"/>
      <source>&lt;p&gt;Type in one or more commands you want the alias to send directly to the game if the keys entered match the pattern. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
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
      <location filename="../src/ctelnet.cpp" line="609"/>
      <source>[ INFO ]  - The IP address of %1 has been found. It is: %2
</source>
      <translation>[ INFO ]  - La dirección IP de %1 ha sido encontrada. Es: %2
</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="618"/>
      <source>[ ERROR ] - Host name lookup Failure!
Connection cannot be established.
The server name is not correct, not working properly,
or your nameservers are not working properly.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="449"/>
      <source>[ ERROR ] - TCP/IP socket ERROR:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="476"/>
      <source>[ INFO ]  - A secure connection has been established successfully.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="478"/>
      <source>[ INFO ]  - A connection has been established successfully.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="514"/>
      <source>[ INFO ]  - Connection time: %1
    </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="557"/>
      <source>Secure connections aren&apos;t supported by this game on this port - try turning the option off.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="602"/>
      <source>[ INFO ]  - Trying secure connection to %1: %2 ...
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1647"/>
      <location filename="../src/ctelnet.cpp" line="2014"/>
      <source>[ INFO ]  - The server wants to upgrade the GUI to new version &apos;%1&apos;.
Uninstalling old version &apos;%2&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1658"/>
      <location filename="../src/ctelnet.cpp" line="2025"/>
      <source>[  OK  ]  - Package is already installed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1667"/>
      <location filename="../src/ctelnet.cpp" line="2034"/>
      <source>downloading game GUI from server</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1667"/>
      <location filename="../src/ctelnet.cpp" line="2034"/>
      <source>Cancel</source>
      <comment>Cancel download of GUI package from Server</comment>
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1656"/>
      <location filename="../src/ctelnet.cpp" line="2023"/>
      <source>[ INFO ]  - Server offers downloadable GUI (url=&apos;%1&apos;) (package=&apos;%2&apos;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="418"/>
      <source>[ INFO ]  - Looking up the IP address of server: %1:%2 ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="516"/>
      <source>hh:mm:ss.zzz</source>
      <comment>This is the format to be used to show the profile connection time, it follows the rules of the &quot;QDateTime::toString(...)&quot; function and may need modification for some locales, e.g. France, Spain.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="547"/>
      <location filename="../src/ctelnet.cpp" line="559"/>
      <source>[ ALERT ] - Socket got disconnected.
Reason: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="611"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 ...
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="613"/>
      <source>[ INFO ]  - Trying to connect to %1:%2 via proxy...
</source>
      <translation type="unfinished"/>
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
      <location filename="../src/ctelnet.cpp" line="2316"/>
      <source>ERROR</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>ERROR</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2328"/>
      <source>LUA</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2339"/>
      <source>WARN</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2350"/>
      <source>ALERT</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>ALERTA</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2361"/>
      <source>INFO</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>INFO</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2372"/>
      <source>OK</source>
      <comment>Keep the capisalisation, the translated text at 7 letters max so it aligns nicely</comment>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2555"/>
      <source>[ INFO ]  - Loading replay file:
&quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2574"/>
      <source>Cannot perform replay, another one may already be in progress. Try again when it has finished.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2576"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress.
Try again when it has finished.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2584"/>
      <source>Cannot read file &quot;%1&quot;, error message was: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2587"/>
      <source>[ ERROR ] - Cannot read file &quot;%1&quot;,
error message was: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2615"/>
      <source>[  OK  ]  - The replay has ended.</source>
      <translation>[OK] - Ha terminado la repetición.</translation>
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
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="99"/>
      <source>&lt;p&gt;Save (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="102"/>
      <source>Save</source>
      <translation>Guardar</translation>
    </message>
  </context>
  <context>
    <name>connection_profiles</name>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="20"/>
      <source>Select a profile to connect with</source>
      <translation>Selecciona un perfil para conectarse</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="101"/>
      <source>profiles list</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="363"/>
      <source>Remove</source>
      <translation>Eliminar</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="382"/>
      <source>Copy</source>
      <translation>Copiar</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="407"/>
      <source>New</source>
      <translation>Nuevo</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="462"/>
      <source>welcome message</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="478"/>
      <source>Required</source>
      <translation>Requerido</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="484"/>
      <source>Profile name:</source>
      <translation>Nombre de perfil:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="500"/>
      <source>Profile name</source>
      <translation>Nombre de perfil</translation>
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
      <translation>Dirección del servidor:</translation>
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
      <translation>Puerto:</translation>
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
      <translation>Historial de perfil:</translation>
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
      <translation>Opcional</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="678"/>
      <source>Character name:</source>
      <translation>Nombre del personaje:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="688"/>
      <source>The characters name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="691"/>
      <source>Character name</source>
      <translation>Nombre del personaje</translation>
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
      <translation>Contraseña</translation>
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
      <translation>Contraseña:</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="745"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="710"/>
      <source>With this enabled, Mudlet will automatically start and connect on this profile when it is launched</source>
      <translation>Con esta opción habilitada, Mudlet se iniciará y conectará automáticamente a este perfil cuando se ponga en marcha</translation>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="832"/>
      <source>Website:</source>
      <translation>Sitio web:</translation>
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
      <translation>Ajustes de línea:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="84"/>
      <source>Color:</source>
      <translation>Color:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="58"/>
      <source>Style:</source>
      <translation>Estilo:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="43"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="122"/>
      <source>Ends with an arrow:</source>
      <translation>Termina con una flecha:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="138"/>
      <source>Exit Details:</source>
      <translation>Detalles de salida:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="153"/>
      <source>Origin:</source>
      <translation>Origen:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="226"/>
      <source>Destination:</source>
      <translation>Destino:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="188"/>
      <source>    Direction/Command:</source>
      <translation>    Dirección/Comando:</translation>
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
      <translation>Ajustes de línea:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="97"/>
      <source>Ends with an arrow:</source>
      <translation>Termina con una flecha:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="110"/>
      <source>Style:</source>
      <translation>Estilo:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="126"/>
      <source>Color:</source>
      <translation>Color:</translation>
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
      <translation>Salidas normales:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="212"/>
      <source>NW</source>
      <translation>NO</translation>
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
      <translation>AR</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="336"/>
      <source>W</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="346"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="356"/>
      <source>IN</source>
      <translation>EN</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="382"/>
      <source>OUT</source>
      <translation>FUERA</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="392"/>
      <source>SW</source>
      <translation>SO</translation>
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
      <translation>Salidas especiales:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="499"/>
      <source>Has
custom line?</source>
      <translation>Tiene línea personalizada?</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="511"/>
      <source> Destination </source>
      <translation> Destino </translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="522"/>
      <source> Command</source>
      <translation> Comando</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="567"/>
      <source>Cancel</source>
      <translation>Cancelar</translation>
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
      <translation>Eliminar</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="61"/>
      <source>Cancel</source>
      <translation>Cancelar</translation>
    </message>
  </context>
  <context>
    <name>dialog</name>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="39"/>
      <source>Status</source>
      <translation>Estado</translation>
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
      <translation>Habitaciones</translation>
    </message>
  </context>
  <context>
    <name>directions</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13853"/>
      <source>north</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>norte</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13855"/>
      <source>n</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13857"/>
      <source>east</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>este</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13859"/>
      <source>e</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>e</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13861"/>
      <source>south</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>sur</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13863"/>
      <source>s</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13865"/>
      <source>west</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>oeste</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13867"/>
      <source>w</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13869"/>
      <source>northeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>noreste</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13871"/>
      <source>ne</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>ne</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13873"/>
      <source>southeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>sureste</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13875"/>
      <source>se</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>se</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13877"/>
      <source>southwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>suroeste</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13879"/>
      <source>sw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>so</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13881"/>
      <source>northwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>noroeste</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13883"/>
      <source>nw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>no</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13885"/>
      <source>in</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>adentro</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13887"/>
      <source>i</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13889"/>
      <source>out</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>afuera</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13891"/>
      <source>o</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13893"/>
      <source>up</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>arriba</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13895"/>
      <source>u</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13897"/>
      <source>down</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>abajo</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="13899"/>
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
      <translation>&lt;p&gt;Mudlet ha sido programado por Heiko Köhn, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet ha sido publicado bajo la versión 2 de la licencia GPL, cuya única versión oficial es la redactada en inglés, la cual se reproduce a continuación:&lt;/p&gt;</translation>
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
      <source>&lt;h2&gt;&lt;u&gt;lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="844"/>
      <source>&lt;h2&gt;&lt;u&gt;lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="849"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="854"/>
      <source>&lt;h2&gt;&lt;u&gt;edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="857"/>
      <source>The &lt;b&gt;edbee-lib&lt;/b&gt; widget itself incorporates other components with licences that must be noted as well, they are:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="859"/>
      <source>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="864"/>
      <source>&lt;h2&gt;&lt;u&gt;Oniguruma LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="868"/>
      <source>&lt;h2&gt;&lt;u&gt;Ruby BSDL&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1993-2013 Yukihiro Matsumoto.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="872"/>
      <source>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [broken link]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archived version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="879"/>
      <source>&lt;h2&gt;&lt;u&gt;dblsqd&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="882"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="892"/>
      <source>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="900"/>
      <source>&lt;h2&gt;&lt;u&gt;sparkle-glue&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008 Remko Troncon&lt;br&gt;Copyright © 2017 Vadim Peretokin&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="906"/>
      <source>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1068"/>
      <source>Technical information:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1069"/>
      <source>Version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1070"/>
      <source>OS</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1071"/>
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
      <location filename="../src/dlgColorTrigger.cpp" line="59"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s background color - however chosing this for both this and the foreground is an error.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="60"/>
      <source>&lt;p&gt;Click to make the color trigger ignore the text&apos;s foreground color - however chosing this for both this and the background is an error.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="64"/>
      <source>Default</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="66"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s background color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="67"/>
      <source>&lt;p&gt;Click to make the color trigger when the text&apos;s foreground color has not been modified from its normal value.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="71"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s background color matches the color number indicated.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="72"/>
      <source>&lt;p&gt;Click a color to make the trigger fire only when the text&apos;s foreground color matches the color number indicated.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="77"/>
      <source>Black</source>
      <translation>Negro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="78"/>
      <source>Red</source>
      <translation>Rojo</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="79"/>
      <source>Green</source>
      <translation>Verde</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="80"/>
      <source>Yellow</source>
      <translation>Amarillo</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="81"/>
      <source>Blue</source>
      <translation>Azul</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="82"/>
      <source>Magenta</source>
      <translation>Magenta</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="83"/>
      <source>Cyan</source>
      <translation>Cian</translation>
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
      <translation>Rojo claro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="88"/>
      <source>Light green</source>
      <translation>Verde claro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="89"/>
      <source>Light yellow</source>
      <translation>Amarillo claro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="90"/>
      <source>Light blue</source>
      <translation>Azul celeste</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="91"/>
      <source>Light magenta</source>
      <translation>Magenta claro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="92"/>
      <source>Light cyan</source>
      <translation>Cian claro</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="93"/>
      <source>Light white</source>
      <translation>Blanco claro</translation>
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
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgConnectionProfiles</name>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="110"/>
      <source>Connect</source>
      <translation>Conectar</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="267"/>
      <source>Game name: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1239"/>
      <source>This profile is currently loaded - close it before changing the connection parameters.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2050"/>
      <source>The %1 character is not permitted. Use one of the following:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2080"/>
      <source>You have to enter a number. Other characters are not permitted.</source>
      <translation>Tienes que introducir un número. No están permitidos otros caracteres.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2066"/>
      <source>This profile name is already in use.</source>
      <translation>Este nombre de perfil ya está en uso.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="523"/>
      <source>Could not rename your profile data on the computer.</source>
      <translation>No se pudo renombrar la información de su perfil en el ordenador.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="122"/>
      <source>&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Welcome to Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;b&gt;Click on one of the games on the list to play.&lt;/b&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;To play a game not in the list, click on %1 &lt;span style=&quot; color:#555753;&quot;&gt;New&lt;/span&gt;, fill in the &lt;i&gt;Profile Name&lt;/i&gt;, &lt;i&gt;Server address&lt;/i&gt;, and &lt;i&gt;Port&lt;/i&gt; fields in the &lt;i&gt;Required &lt;/i&gt; area.&lt;/p&gt;&lt;p&gt;After that, click %2 &lt;span style=&quot; color:#555753;&quot;&gt;Connect&lt;/span&gt; to play.&lt;/p&gt;&lt;p&gt;Have fun!&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;The Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</source>
      <comment>Welcome message. Both %1 and %2 may be replaced by icons when this text is used.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="112"/>
      <source>Offline</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="135"/>
      <source>Copy</source>
      <translation>Copiar</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="137"/>
      <source>Copy settings only</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="145"/>
      <source>copy profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="146"/>
      <source>copy the entire profile to new one that will require a different new name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="150"/>
      <source>copy profile settings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="151"/>
      <source>copy the settings and some other parts of the profile to a new one that will require a different new name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="215"/>
      <source>Characters password, stored securely in the computer&apos;s credential manager</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="217"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="264"/>
      <source>Click to load but not connect the selected profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="265"/>
      <source>Click to load and connect the selected profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="266"/>
      <source>Need to have a valid profile name, game server address and port before this button can be enabled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="268"/>
      <source>Button to select a mud game to play, double-click it to connect and start playing it.</source>
      <comment>Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="531"/>
      <source>Could not create the new profile folder on your computer.</source>
      <translation>No se pudo crear el directorio de nuevo perfil en tu ordenador.</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="567"/>
      <source>new profile name</source>
      <translation>nombre del nuevo perfil</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="680"/>
      <source>Deleting &apos;%1&apos;</source>
      <translation>Borrando &apos;%1&apos;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1286"/>
      <source>Discord integration not available on this platform</source>
      <translation>Integración de discord no disponible en esta plataforma</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1292"/>
      <source>Discord integration not supported by game</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1295"/>
      <source>Check to enable Discord integration</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1651"/>
      <source>Reset icon</source>
      <comment>Reset the custom picture for this profile in the connection dialog and show the default one instead</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1653"/>
      <source>Set custom icon</source>
      <comment>Set a custom picture to show for the profile in the connection dialog</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1663"/>
      <source>Select custom image for profile (should be 120x30)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1665"/>
      <source>Images (%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2093"/>
      <source>Port number must be above zero and below 65535.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2119"/>
      <source>Mudlet can not load support for secure connections.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2143"/>
      <source>Please enter the URL or IP address of the Game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2156"/>
      <source>SSL connections require the URL of the Game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2174"/>
      <source>&lt;p&gt;Load profile without connecting.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2190"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before loading.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2195"/>
      <source>&lt;p&gt;Please set a valid profile name, game server address and the game port before connecting.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2248"/>
      <source>&lt;p&gt;Click to hide the password; it will also hide if another profile is selected.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2252"/>
      <source>&lt;p&gt;Click to reveal the password for this profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2101"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2107"/>
      <source>Mudlet is not configured for secure connections.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgIRC</name>
    <message>
      <location filename="../src/dlgIRC.cpp" line="119"/>
      <source>%1 closed their client.</source>
      <translation>%1 cerró su cliente.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="131"/>
      <source>Mudlet IRC Client - %1 - %2 on %3</source>
      <translation>Cliente IRC de Mudlet - %1 - %2 en %3</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="145"/>
      <source>$ Starting Mudlet IRC Client...</source>
      <translation>$ Iniciando Cliente IRC de Mudlet...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="146"/>
      <source>$ Host: %1:%2</source>
      <translation>$ Nombre de servidor: %1:%2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="147"/>
      <source>$ Nick: %1</source>
      <translation>$ Apodo: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="148"/>
      <source>$ Auto-Join Channels: %1</source>
      <translation>$ Auto-Unirse a Canales: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="149"/>
      <source>$ This client supports Auto-Completion using the Tab key.</source>
      <translation>$ Este cliente soporta auto-completar usando la tecla Tabulador.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="150"/>
      <source>$ Type &lt;b&gt;/help&lt;/b&gt; for commands or &lt;b&gt;/help [command]&lt;/b&gt; for command syntax.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="209"/>
      <source>Restarting IRC Client</source>
      <translation>Reiniciando cliente de IRC</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="382"/>
      <source>[Error] MSGLIMIT requires &lt;limit&gt; to be a whole number greater than zero!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="412"/>
      <source>[HELP] Available Commands: %1</source>
      <translation>[AYUDA] Comandos disponibles: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="414"/>
      <source>[HELP] Syntax: %1</source>
      <translation>[HELP] Sintaxis: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="422"/>
      <source>! Connected to %1.</source>
      <translation>! Conectado a %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="423"/>
      <source>! Joining %1...</source>
      <translation>! Conectando %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="428"/>
      <source>! Connecting %1...</source>
      <translation>! Conectando %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="433"/>
      <source>! Disconnected from %1.</source>
      <translation>! Desconectado de %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="499"/>
      <source>[ERROR] Syntax: %1</source>
      <translation>[ERROR] Sintaxis: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="501"/>
      <source>[ERROR] Unknown command: %1</source>
      <translation>[ERROR] Comando desconocido: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="647"/>
      <source>! The Nickname %1 is reserved. Automatically changing Nickname to: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="658"/>
      <source>Your nick has changed.</source>
      <translation>Tu nick ha cambiado.</translation>
    </message>
  </context>
  <context>
    <name>dlgMapper</name>
    <message>
      <location filename="../src/dlgMapper.cpp" line="347"/>
      <source>None</source>
      <comment>Don&apos;t show the map overlay, &apos;none&apos; meaning no map overlay styled are enabled</comment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgModuleManager</name>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="48"/>
      <source>Module Manager - %1</source>
      <translation>Gestor de Módulos - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Name</source>
      <translation>Nombre del Módulo</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Priority</source>
      <translation>Prioridad</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Sync</source>
      <translation>Sincronizar</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Location</source>
      <translation>Ubicación Del Módulo</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="105"/>
      <source>Checking this box will cause the module to be saved and &lt;i&gt;resynchronised&lt;/i&gt; across all sessions that share it when the &lt;i&gt;Save Profile&lt;/i&gt; button is clicked in the Editor or if it is saved at the end of the session.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="111"/>
      <source>&lt;b&gt;Note:&lt;/b&gt; &lt;i&gt;.zip&lt;/i&gt; and &lt;i&gt;.mpackage&lt;/i&gt; modules are currently unable to be synced&lt;br&gt; only &lt;i&gt;.xml&lt;/i&gt; packages are able to be synchronized across profiles at the moment. </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="149"/>
      <source>Load Mudlet Module</source>
      <translation>Cargar Módulo Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Load Mudlet Module:</source>
      <translation>Cargar Módulo Mudlet:</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="156"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>No es posible leer el archivo %1:
%2.</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageExporter</name>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="14"/>
      <source>Package Exporter</source>
      <translation>Exportador de paquetes</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="21"/>
      <source>Check items to export</source>
      <translation>Selecciona los elementos a exportar</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="57"/>
      <source>Triggers</source>
      <translation>Disparadores</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="58"/>
      <source>Aliases</source>
      <translation>Alias</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="59"/>
      <source>Timers</source>
      <translation>Temporizadores</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="60"/>
      <source>Scripts</source>
      <translation>Scripts</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="61"/>
      <source>Keys</source>
      <translation>Teclas</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="62"/>
      <source>Buttons</source>
      <translation>Botones</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="78"/>
      <source>Export</source>
      <comment>Text for button to perform the package export on the items the user has selected.</comment>
      <translation>Exportar</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="88"/>
      <source>What do you wish to call the package:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="155"/>
      <source>Failed to open file &quot;%1&quot; to place into package. Error message was: &quot;%2&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but the code cannot open it.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="163"/>
      <source>Failed to add file &quot;%1&quot; to package &quot;%2&quot;. Error message was: &quot;%3&quot;.</source>
      <comment>This error message will appear when a file is to be placed into the package but cannot be done for some reason.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="182"/>
      <source>Failed to export. Could not open the folder &quot;%1&quot; for writing in. - Do you have the necessary permissions and free disk-space to write to that folder?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="262"/>
      <source>Failed to export. Could not write Mudlet items to the file &quot;%1&quot;.</source>
      <comment>This error message is shown when all the Mudlet items cannot be written to the &apos;packageName&apos;.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="345"/>
      <source>Failed to open package file. Error is: &quot;%1&quot;.</source>
      <comment>This error message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer&apos;s file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="439"/>
      <source>Failed to add directory &quot;%1&quot; to package. Error is: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="495"/>
      <source>Required file &quot;%1&quot; was not found in the staging area. This area contains the Mudlet items chosen for the package, which you selected to be included in the package file. This suggests there may be a problem with that directory: &quot;%2&quot; - Do you have the necessary permissions and free disk-space?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="517"/>
      <source>Failed to write files into and then close the package. Error is: &quot;%1&quot;.</source>
      <comment>This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="537"/>
      <source>Package &quot;%1&quot; exported to: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="833"/>
      <source>Why not &lt;a href=&quot;https://forums.mudlet.org/viewforum.php?f=6&quot;&gt;upload&lt;/a&gt; your package for other Mudlet users?</source>
      <comment>Only the text outside of the &apos;a&apos; (HTML anchor) tags PLUS the verb &apos;upload&apos; in between them in the source text, (associated with uploading the resulting package to the Mudlet forums) should be translated.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="32"/>
      <source>Do you want to add additional files to the package e. g. images, sounds etc. ? The add files button will bring up the file explorer. Copy all files and folders that you want to add to the package in the temporary package directory that is shown in the file explorer. When you are done click on the export button below to start package creation.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="45"/>
      <source>Add Files</source>
      <translation>Agregar archivos</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="52"/>
      <source>Export location:</source>
      <translation>Ubicación de exportación:</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="88"/>
      <source>Package name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="99"/>
      <source>Where do you want to save the package?</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgPackageManager</name>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="42"/>
      <source>Package Manager - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="54"/>
      <source>Import Mudlet Package</source>
      <translation>Importar Paquete de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="61"/>
      <source>Import Mudlet Package:</source>
      <translation>Importar Paquete de Mudlet:</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="61"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>No es posible leer el archivo %1:
%2.</translation>
    </message>
  </context>
  <context>
    <name>dlgProfilePreferences</name>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="142"/>
      <source>&lt;p&gt;Location which will be used to store log files - matching logs will be appended to.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="150"/>
      <source>logfile</source>
      <comment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {1 of 2}).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="196"/>
      <source>&lt;p&gt;This will bring up a display showing all the symbols used in the current map and whether they can be drawn using just the specifed font, any other font, or not at all.  It also shows the sequence of Unicode &lt;i&gt;code-points&lt;/i&gt; that make up that symbol, so that they can be identified even if they cannot be displayed; also, up to the first thirty two rooms that are using that symbol are listed, which may help to identify any unexpected or odd cases.&lt;p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="202"/>
      <source>&lt;p&gt;Select the only or the primary font used (depending on &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; setting) to produce the 2D mapper room symbols.&lt;/p&gt;</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="346"/>
      <source>Migrated all passwords to secure storage.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="353"/>
      <source>Migrated %1...</source>
      <comment>This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="357"/>
      <source>Migrated all passwords to profile storage.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="738"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="740"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="741"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="744"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01%1)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="745"/>
      <source>Named file (concatenate logs in one file)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="778"/>
      <source>Other profiles to Map to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="818"/>
      <source>%1 {Default, recommended}</source>
      <translation>%1 {Default, recommended}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="829"/>
      <source>%1 {Upgraded, experimental/testing, NOT recommended}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="831"/>
      <source>%1 {Downgraded, for sharing with older version users, NOT recommended}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="840"/>
      <source>2D Map Room Symbol scaling factor:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="860"/>
      <source>Show &quot;%1&quot; in the map area selection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="898"/>
      <source>%1 (*Error, report to Mudlet Makers*)</source>
      <comment>The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1012"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="3769"/>
      <source>Profile preferences - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1288"/>
      <source>Profile preferences</source>
      <translation>Preferencias de perfil</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2006"/>
      <source>Load Mudlet map</source>
      <translation>Cargar mapa de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2021"/>
      <source>Importing map - please wait...</source>
      <translation>Importando mapa - por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2025"/>
      <source>Imported map from %1.</source>
      <translation>Mapa importado desde %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2027"/>
      <source>Could not import map from %1.</source>
      <translation>No se pudo importar el mapa desde %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2030"/>
      <source>Loading map - please wait...</source>
      <translation>Cargando mapa - por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2035"/>
      <source>Loaded map from %1.</source>
      <translation>Mapa cargado desde %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2037"/>
      <source>Could not load map from %1.</source>
      <translation>No se pudo cargar el mapa desde %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2054"/>
      <source>Save Mudlet map</source>
      <translation>Guardar mapa de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2054"/>
      <source>Mudlet map (*.dat)</source>
      <comment>Do not change the extension text (in braces) - it is needed programmatically!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2064"/>
      <source>Saving map - please wait...</source>
      <translation>Guardando mapa - por favor, espere...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2074"/>
      <source>Saved map to %1.</source>
      <translation>Mapa guardado en %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2076"/>
      <source>Could not save map to %1.</source>
      <translation>No se pudo guardar el mapa en %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2104"/>
      <source>Migrating passwords to secure storage...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2111"/>
      <source>Migrating passwords to profiles...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2143"/>
      <source>[ ERROR ] - Unable to use or create directory to store map for other profile &quot;%1&quot;.
Please check that you have permissions/access to:
&quot;%2&quot;
and there is enough space. The copying operation has failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2150"/>
      <source>Creating a destination directory failed...</source>
      <translation>La creación de un directorio de destino ha fallado...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2234"/>
      <source>Backing up current map - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2244"/>
      <source>Could not backup the map - saving it failed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2269"/>
      <source>Could not copy the map - failed to work out which map file we just saved the map as!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2281"/>
      <source>Copying over map to %1 - please wait...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2287"/>
      <source>Could not copy the map to %1 - unable to copy the new map file over.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2291"/>
      <source>Map copied successfully to other profile %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2303"/>
      <source>Map copied, now signalling other profiles to reload it.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2339"/>
      <source>Where should Mudlet save log files?</source>
      <translation>Donde debe Mudlet guardar los archivos de registro?</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2724"/>
      <source>%1 selected - press to change</source>
      <translation>%1 seleccionado- pulsa para cambiar</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2727"/>
      <source>Press to pick destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2968"/>
      <source>Could not update themes: %1</source>
      <translation>No se pudo actualizar temas: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2971"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3088"/>
      <source>{missing, possibly recently deleted trigger item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3091"/>
      <source>{missing, possibly recently deleted alias item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3094"/>
      <source>{missing, possibly recently deleted script item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3097"/>
      <source>{missing, possibly recently deleted timer item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3100"/>
      <source>{missing, possibly recently deleted key item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3103"/>
      <source>{missing, possibly recently deleted button item}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3901"/>
      <source>Set outer color of player room mark.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3902"/>
      <source>Set inner color of player room mark.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="204"/>
      <source>&lt;p&gt;Using a single font is likely to produce a more consistent style but may cause the &lt;i&gt;font replacement character&lt;/i&gt; &apos;&lt;b&gt;�&lt;/b&gt;&apos; to show if the font does not have a needed glyph (a font&apos;s individual character/symbol) to represent the grapheme (what is to be represented).  Clearing this checkbox will allow the best alternative glyph from another font to be used to draw that grapheme.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="143"/>
      <source>&lt;p&gt;Select a directory where logs will be saved.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="144"/>
      <source>&lt;p&gt;Reset the directory so that logs are saved to the profile&apos;s &lt;i&gt;log&lt;/i&gt; directory.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="145"/>
      <source>&lt;p&gt;This option sets the format of the log name.&lt;/p&gt;&lt;p&gt;If &lt;i&gt;Named file&lt;/i&gt; is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="148"/>
      <source>&lt;p&gt;Set a custom name for your log. (New logs are appended if a log file of the same name already exists).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="169"/>
      <source>&lt;p&gt;Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="190"/>
      <source>&lt;p&gt;A timer with a short interval will quickly fill up the &lt;i&gt;Central Debug Console&lt;/i&gt; windows with messages that it ran correctly on &lt;i&gt;each&lt;/i&gt; occasion it is called.  This (per profile) control adjusts a threshold that will hide those messages in just that window for those timers which run &lt;b&gt;correctly&lt;/b&gt; when the timer&apos;s interval is less than this setting.&lt;/p&gt;&lt;p&gt;&lt;u&gt;Any timer script that has errors will still have its error messages reported whatever the setting.&lt;/u&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="573"/>
      <source>System dictionaries:</source>
      <comment>On *nix systems where we find the system ones we use them.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="604"/>
      <source>&lt;p&gt;From the dictionary file &lt;tt&gt;%1.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="606"/>
      <source>%1 - not recognised</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="607"/>
      <source>&lt;p&gt;Mudlet does not recognise the code &quot;%1&quot;, please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!&lt;/p&gt;&lt;p&gt;The file &lt;tt&gt;%2.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file) is still usable.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="632"/>
      <source>No Hunspell dictionary files found, spell-checking will not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2008"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3228"/>
      <source>&lt;p&gt;The room symbol will appear like this if only symbols (glyphs) from the specfic font are used.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3234"/>
      <source>&lt;p&gt;The room symbol will appear like this if symbols (glyphs) from any font can be used.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3263"/>
      <source>&lt;p&gt;These are the sequence of hexadecimal numbers that are used by the Unicode consortium to identify the graphemes needed to create the symbol.  These numbers can be utilised to determine precisely what is to be drawn even if some fonts have glyphs that are the same for different codepoints or combination of codepoints.&lt;/p&gt;&lt;p&gt;Character entry utilities such as &lt;i&gt;charmap.exe&lt;/i&gt; on &lt;i&gt;Windows&lt;/i&gt; or &lt;i&gt;gucharmap&lt;/i&gt; on many Unix type operating systems will also use these numbers which cover everything from U+0020 {Space} to U+10FFFD the last usable number in the &lt;i&gt;Private Use Plane 16&lt;/i&gt; via most of the written marks that humanity has ever made.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3276"/>
      <source>&lt;p&gt;How many rooms in the whole map have this symbol.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3288"/>
      <source>more - not shown...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3295"/>
      <source>&lt;p&gt;The rooms with this symbol, up to a maximum of thirty-two, if there are more than this, it is indicated but they are not shown.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3304"/>
      <source>&lt;p&gt;The symbol can be made entirely from glyphs in the specified font.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3314"/>
      <source>&lt;p&gt;The symbol cannot be made entirely from glyphs in the specified font, but, using other fonts in the system, it can. Either un-check the &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; option or try and choose another font that does have the needed glyphs.&lt;/p&gt;&lt;p&gt;&lt;i&gt;You need not close this table to try another font, changing it on the main preferences dialogue will update this table after a slight delay.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
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
      <translation>Ícono Grande</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3414"/>
      <source>Detail</source>
      <comment>Discord Rich Presence detail</comment>
      <translation>Detalle</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3416"/>
      <source>Small icon</source>
      <comment>Discord Rich Presence small icon</comment>
      <translation>Ícono pequeño</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3417"/>
      <source>State</source>
      <comment>Discord Rich Presence state</comment>
      <translation>Estado</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3419"/>
      <source>Party size</source>
      <comment>Discord Rich Presence party size</comment>
      <translation>Tamaño del grupo</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3420"/>
      <source>Party max</source>
      <comment>Discord Rich Presence maximum party size</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3421"/>
      <source>Time</source>
      <comment>Discord Rich Presence time until or time elapsed</comment>
      <translation>Hora</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3456"/>
      <source>Map symbol usage - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3533"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)</source>
      <translation>aaa-MM-dd#HH-mm-ss (ej. 1970-01-01#00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3534"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)</source>
      <translation>aaa-MM-ddTHH-mm-ss (ej. 1970-01-01T00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3535"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3536"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.html)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3539"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3540"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3541"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3542"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="155"/>
      <source>Entered number is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="217"/>
      <source>Set the number of the room that this special exit leads to, will turn blue for a valid number; if left like this, this exit will be deleted when &amp;lt;i&amp;gt;save&amp;lt;/i&amp;gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="220"/>
      <location filename="../src/dlgRoomExits.cpp" line="1872"/>
      <source>Prevent a route being created via this exit, equivalent to an infinite exit weight.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="224"/>
      <location filename="../src/dlgRoomExits.cpp" line="1881"/>
      <source>Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="226"/>
      <location filename="../src/dlgRoomExits.cpp" line="1888"/>
      <source>No door symbol is drawn on 2D Map for this exit (only functional choice currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="228"/>
      <location filename="../src/dlgRoomExits.cpp" line="1892"/>
      <source>Green (Open) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="230"/>
      <location filename="../src/dlgRoomExits.cpp" line="1897"/>
      <source>Orange (Closed) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="232"/>
      <location filename="../src/dlgRoomExits.cpp" line="1900"/>
      <source>Red (Locked) door symbol would be drawn on a custom exit line for this exit on 2D Map (but not currently).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="92"/>
      <location filename="../src/dlgRoomExits.cpp" line="215"/>
      <source>(room ID)</source>
      <comment>Placeholder, if no room ID is set for an exit, yet. This string is used in 2 places, ensure they match!</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="132"/>
      <location filename="../src/dlgRoomExits.cpp" line="233"/>
      <location filename="../src/dlgRoomExits.cpp" line="268"/>
      <location filename="../src/dlgRoomExits.cpp" line="2255"/>
      <location filename="../src/dlgRoomExits.cpp" line="2277"/>
      <source>(command or Lua script)</source>
      <comment>Placeholder, if a special exit has no code given, yet. This string is also used programmatically - ensure all five instances are the same</comment>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="737"/>
      <source>Entered number is invalid, set the number of the room northwest of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="748"/>
      <location filename="../src/dlgRoomExits.cpp" line="1320"/>
      <source>Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="788"/>
      <source>Entered number is invalid, set the number of the room north of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="798"/>
      <location filename="../src/dlgRoomExits.cpp" line="1354"/>
      <source>Set the number of the room north of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="837"/>
      <source>Entered number is invalid, set the number of the room northeast of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="847"/>
      <location filename="../src/dlgRoomExits.cpp" line="1385"/>
      <source>Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="886"/>
      <source>Entered number is invalid, set the number of the room up from this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="896"/>
      <location filename="../src/dlgRoomExits.cpp" line="1416"/>
      <source>Set the number of the room up from this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="935"/>
      <source>Entered number is invalid, set the number of the room west of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="945"/>
      <location filename="../src/dlgRoomExits.cpp" line="1447"/>
      <source>Set the number of the room west of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="984"/>
      <source>Entered number is invalid, set the number of the room east of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="994"/>
      <location filename="../src/dlgRoomExits.cpp" line="1478"/>
      <source>Set the number of the room east of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1033"/>
      <source>Entered number is invalid, set the number of the room down from this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1043"/>
      <location filename="../src/dlgRoomExits.cpp" line="1509"/>
      <source>Set the number of the room down from this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1082"/>
      <source>Entered number is invalid, set the number of the room southwest of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1092"/>
      <location filename="../src/dlgRoomExits.cpp" line="1540"/>
      <source>Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1131"/>
      <source>Entered number is invalid, set the number of the room south of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1141"/>
      <location filename="../src/dlgRoomExits.cpp" line="1571"/>
      <source>Set the number of the room south of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1180"/>
      <source>Entered number is invalid, set the number of the room southeast of this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1190"/>
      <location filename="../src/dlgRoomExits.cpp" line="1602"/>
      <source>Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1229"/>
      <source>Entered number is invalid, set the number of the room in from this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1239"/>
      <location filename="../src/dlgRoomExits.cpp" line="1633"/>
      <source>Set the number of the room in from this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1278"/>
      <source>Entered number is invalid, set the number of the room out from this one, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1288"/>
      <location filename="../src/dlgRoomExits.cpp" line="1664"/>
      <source>Set the number of the room out from this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1691"/>
      <source>northwest</source>
      <translation>noroeste</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1692"/>
      <source>north</source>
      <translation>norte</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1693"/>
      <source>northeast</source>
      <translation>noreste</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1694"/>
      <source>up</source>
      <translation>arriba</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1695"/>
      <source>west</source>
      <translation>oeste</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1696"/>
      <source>east</source>
      <translation>este</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1697"/>
      <source>down</source>
      <translation>abajo</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1698"/>
      <source>southwest</source>
      <translation>suroeste</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1699"/>
      <source>south</source>
      <translation>sur</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1700"/>
      <source>southeast</source>
      <translation>sureste</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1701"/>
      <source>in</source>
      <translation>adentro</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1702"/>
      <source>out</source>
      <translation>afuera</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1777"/>
      <source>Set the number of the room %1 of this one, will be blue for a valid number or red for invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1800"/>
      <source>Exits for room: &quot;%1&quot; [*]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1802"/>
      <source>Exits for room Id: %1 [*]</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1860"/>
      <source>Room Id is invalid, set the number of the room that this special exit leads to, will turn blue for a valid number.</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="86"/>
      <source>The symbol is &quot;%1&quot; in the selected room,
delete this to clear the symbol or replace
it with a new symbol for this room:</source>
      <comment>This is for when applying a new room symbol to one room. Use line feeds to format text into a reasonable rectangle.</comment>
      <translation>El símbolo es &quot;%1&quot; en la sala seleccionada, borrar aquí para suprimir el símbolo o sustitúyalo por un nuevo símbolo para esta sala:</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomSymbol.cpp" line="95"/>
      <source>Choose:
 • an existing symbol from the list below (sorted by most commonly used first)
 • enter one or more graphemes (&quot;visible characters&quot;) as a new symbol
 • enter a space to clear any existing symbols
for all of the %n selected room(s):</source>
      <comment>Use line feeds to format text into a reasonable rectangle if needed, %n is the number of rooms involved.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="129"/>
      <source>%1 {count:%2}</source>
      <comment>Everything after the first parameter (the &apos;%1&apos;) will be removed by processing it as a QRegularExpression programmatically, ensure the translated text has ` {` immediately after the &apos;%1&apos;, and &apos;}&apos; as the very last character, so that the right portion can be extracted if the user clicks on this item when it is shown in the QComboBox it is put in.</comment>
      <translation>%1 {count:%2}</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomSymbol.cpp" line="200"/>
      <source>Pick color</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgTriggerEditor</name>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="251"/>
      <source>-- Enter your lua code here
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="331"/>
      <source>*** starting new session ***
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="419"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5838"/>
      <source>Triggers</source>
      <translation>Disparadores</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="420"/>
      <source>Show Triggers</source>
      <translation>Mostrar disparadores</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="423"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5862"/>
      <source>Buttons</source>
      <translation>Botones</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="424"/>
      <source>Show Buttons</source>
      <translation>Mostrar Botones</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="428"/>
      <source>Aliases</source>
      <translation>Alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="429"/>
      <source>Show Aliases</source>
      <translation>Mostrar alias</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5844"/>
      <source>Timers</source>
      <translation>Temporizadores</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="434"/>
      <source>Show Timers</source>
      <translation>Mostrar Temporizadores</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="437"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5850"/>
      <source>Scripts</source>
      <translation>Scripts</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="438"/>
      <source>Show Scripts</source>
      <translation>Mostrar Scripts</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="441"/>
      <source>Keys</source>
      <translation>Teclas</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="442"/>
      <source>Show Keybindings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="445"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="6250"/>
      <source>Variables</source>
      <translation>Variables</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="446"/>
      <source>Show Variables</source>
      <translation>Mostrar Variables</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="449"/>
      <source>Activate</source>
      <translation>Activar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="450"/>
      <source>Toggle Active or Non-Active Mode for Triggers, Scripts etc.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="460"/>
      <source>Add Item</source>
      <translation>Añadir un elemento</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="461"/>
      <source>Add new Trigger, Script, Alias or Filter</source>
      <translation>Agregar nuevo Disparador, Programa, Alias o Filtro</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="464"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="466"/>
      <source>Delete Item</source>
      <translation>Eliminar elemento</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="465"/>
      <source>Delete Trigger, Script, Alias or Filter</source>
      <translation>Eliminar Disparador, Programa, Alias o Filtro</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="472"/>
      <source>Add Group</source>
      <translation>Agregar Grupo</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="473"/>
      <source>Add new Group</source>
      <translation>Añadir nuevo grupo</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="476"/>
      <source>Save Item</source>
      <translation>Guardar elemento</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8376"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8401"/>
      <source>Ctrl+S</source>
      <translation>Ctrl+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="479"/>
      <source>Saves the selected item. (Ctrl+S)&lt;/p&gt;Saving causes any changes to the item to take effect.
It will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="481"/>
      <source>Saves the selected trigger, script, alias, etc, causing new changes to take effect - does not save to disk though...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="484"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8867"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8873"/>
      <source>Copy</source>
      <translation>Copiar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="488"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="489"/>
      <source>Copy the trigger/script/alias/etc</source>
      <translation>Copiar el disparador/programa/alias/etc</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="498"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8868"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8874"/>
      <source>Paste</source>
      <translation>Pegar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="502"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="503"/>
      <source>Paste triggers/scripts/aliases/etc from the clipboard</source>
      <translation>Pegar disparador/scripts/alias/etc desde el portapapeles</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="517"/>
      <source>Import</source>
      <translation>Importar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="521"/>
      <source>Export</source>
      <translation>Exportar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="525"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8711"/>
      <source>Save Profile</source>
      <translation>Guardar Perfil</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="527"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8378"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8403"/>
      <source>Ctrl+Shift+S</source>
      <translation>Ctrl+Shift+S</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="530"/>
      <source>Saves your profile. (Ctrl+Shift+S)&lt;p&gt;Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.&lt;/p&gt;&lt;p&gt;It also makes a backup of your profile, you can load an older version of it when connecting.&lt;/p&gt;&lt;p&gt;Should there be any modules that are marked to be &quot;&lt;i&gt;synced&lt;/i&gt;&quot; this will also cause them to be saved and reloaded into other profiles if they too are active.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="532"/>
      <source>Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings); also &quot;synchronizes&quot; modules that are so marked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="535"/>
      <source>Save Profile As</source>
      <translation>Guardar Perfil Como</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="539"/>
      <source>Statistics</source>
      <translation>Estadísticas</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="540"/>
      <source>Generates a statistics summary display on the main profile console.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="544"/>
      <source>Shows/Hides the errors console in the bottom right of this editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="547"/>
      <source>Debug</source>
      <translation>Depuración</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="548"/>
      <source>Activates Debug Messages -&gt; system will be &lt;b&gt;&lt;i&gt;slower&lt;/i&gt;&lt;/b&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="549"/>
      <source>Shows/Hides the separate Central Debug Console - when being displayed the system will be slower.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="572"/>
      <source>Editor Toolbar - %1 - Actions</source>
      <comment>This is the toolbar that is initally placed at the top of the editor.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="608"/>
      <source>Editor Toolbar - %1 - Items</source>
      <comment>This is the toolbar that is initally placed at the left side of the editor.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="672"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="675"/>
      <source>Search Options</source>
      <translation>Opciones de Búsqueda</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="679"/>
      <source>Case sensitive</source>
      <translation>Sensible a mayúsculas</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="732"/>
      <source>Type</source>
      <comment>Heading for the first column of the search results</comment>
      <translation>Tipo</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="733"/>
      <source>Name</source>
      <comment>Heading for the second column of the search results</comment>
      <translation>Nombre</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="734"/>
      <source>Where</source>
      <comment>Heading for the third column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="735"/>
      <source>What</source>
      <comment>Heading for the fourth column of the search results</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="782"/>
      <source>start of line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="815"/>
      <source>Text to find (trigger pattern)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2785"/>
      <source>Trying to activate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2787"/>
      <source>Trying to deactivate a trigger group, filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2791"/>
      <source>&lt;b&gt;Unable to activate a filter or trigger or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2928"/>
      <source>Trying to activate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2930"/>
      <source>Trying to deactivate a timer group, offset timer, timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2934"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an offset timer or timer or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2972"/>
      <source>Trying to activate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2974"/>
      <source>Trying to deactivate an alias group, alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2978"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate an alias or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3076"/>
      <source>Trying to activate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3078"/>
      <source>Trying to deactivate a script group, script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3082"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a script group or script or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3147"/>
      <source>Trying to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3149"/>
      <source>Trying to deactivate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them &lt;em&gt;succeeded&lt;/em&gt;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3153"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate a button/menu/toolbar or the part of a module &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot; that contains them; reason: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3267"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4097"/>
      <source>New trigger group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3269"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4097"/>
      <source>New trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3368"/>
      <source>New timer group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3370"/>
      <source>New timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3458"/>
      <source>Table name...</source>
      <translation>Nombre de la tabla...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3465"/>
      <source>Variable name...</source>
      <translation>Nombre de la variable...</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3522"/>
      <source>New key group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3524"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4797"/>
      <source>New key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3606"/>
      <source>New alias group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3608"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4205"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4275"/>
      <source>New alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3697"/>
      <source>New menu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3699"/>
      <source>New button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3730"/>
      <source>New toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3783"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4524"/>
      <source>New script group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="3785"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4524"/>
      <source>New script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4219"/>
      <source>Alias &lt;em&gt;%1&lt;/em&gt; has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn&apos;t good as it&apos;ll call itself forever.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4762"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4897"/>
      <source>match on the prompt line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4900"/>
      <source>match on the prompt line (disabled)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4901"/>
      <source>A Go-Ahead (GA) signal from the game is required to make this feature work</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4951"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5061"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8610"/>
      <source>Foreground color ignored</source>
      <comment>Color trigger ignored foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4955"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5065"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8613"/>
      <source>Default foreground color</source>
      <comment>Color trigger default foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4959"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5069"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8616"/>
      <source>Foreground color [ANSI %1]</source>
      <comment>Color trigger ANSI foreground color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4966"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5076"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8673"/>
      <source>Background color ignored</source>
      <comment>Color trigger ignored background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4970"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5080"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8676"/>
      <source>Default background color</source>
      <comment>Color trigger default background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4974"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5084"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8679"/>
      <source>Background color [ANSI %1]</source>
      <comment>Color trigger ANSI background color button, ensure all three instances have the same text</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5095"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5097"/>
      <source>fault</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5150"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5154"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8499"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8525"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="9017"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="9018"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5611"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8473"/>
      <source>Command:</source>
      <translation>Comando:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5650"/>
      <source>Menu properties</source>
      <translation>Propiedades del menú</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5660"/>
      <source>Button properties</source>
      <translation>Propiedades Del Botón</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5668"/>
      <source>Command (down);</source>
      <translation>Comando (abajo);</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5856"/>
      <source>Aliases - Input Triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5868"/>
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
      <translation>Exportar Paquete:</translation>
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
      <translation>Paquete %1 guardado</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7760"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7783"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7806"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7829"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7852"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7876"/>
      <source>Copied %1 to clipboard</source>
      <translation>Copiado %1 al portapapeles</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Export Triggers</source>
      <translation>Exportar Disparadores</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7886"/>
      <source>Mudlet packages (*.xml)</source>
      <translation>Paquetes Mudlet (*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>export package:</source>
      <translation>exportar paquete:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7900"/>
      <source>Cannot write file %1:
%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8171"/>
      <source>Import Mudlet Package</source>
      <translation>Importar Paquete de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8178"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8192"/>
      <source>Import Mudlet Package:</source>
      <translation>Importar Paquete de Mudlet:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8178"/>
      <source>Cannot read file %1:
%2.</source>
      <translation>No es posible leer el archivo %1:
%2.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8192"/>
      <source>Package %1 is already installed.</source>
      <translation>Paquete %1 ya está instalado.</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8323"/>
      <source>Couldn&apos;t save profile</source>
      <translation>No fue posible guardar el perfil</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8323"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8330"/>
      <source>Backup Profile</source>
      <translation>Respaldar Perfil</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8330"/>
      <source>trigger files (*.trigger *.xml)</source>
      <translation>archivos de disparador (*.trigger *.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8460"/>
      <source>Seclect Icon</source>
      <translation>Seleccionar Ícono</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8460"/>
      <source>Images (*.png *.xpm *.jpg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8469"/>
      <source>Command (down):</source>
      <translation>Comando (abajo):</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8490"/>
      <source>Select foreground color to apply to matches</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8516"/>
      <source>Select background color to apply to matches</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8534"/>
      <source>Choose sound file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8538"/>
      <source>Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;All files(*.*)</source>
      <comment>This the list of file extensions that are considered for sounds from triggers, the terms inside of the &apos;(&apos;...&apos;)&apos; and the &quot;;;&quot; are used programmatically and should not be changed.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8591"/>
      <source>Select foreground trigger color for item %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8655"/>
      <source>Select background trigger color for item %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8704"/>
      <source>Saving…</source>
      <translation>Guardando…</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8863"/>
      <source>Format All</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8866"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8872"/>
      <source>Cut</source>
      <translation>Cortar</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8870"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8876"/>
      <source>Select All</source>
      <translation>Seleccionar todo</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9032"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="780"/>
      <source>substring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="103"/>
      <source>&lt;p&gt;Alias react on user input. To add a new alias:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define an input &lt;strong&gt;pattern&lt;/strong&gt; either literally or with a Perl regular expression.&lt;/li&gt;&lt;li&gt;Define a &apos;substitution&apos; &lt;strong&gt;command&lt;/strong&gt; to send to the game in clear text &lt;strong&gt;instead of the alias pattern&lt;/strong&gt;, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the alias.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Aliases can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permAlias(&amp;quot;my greets&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;^hi$&amp;quot;, [[send (&amp;quot;say Greetings, traveller!&amp;quot;) echo (&amp;quot;We said hi!&amp;quot;)]])&lt;/code&gt;&lt;/p&gt;&lt;p&gt;You can now greet by typing &apos;hi&apos;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="113"/>
      <source>&lt;p&gt;Triggers react on game output. To add a new trigger:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;pattern&lt;/strong&gt; that you want to trigger on.&lt;/li&gt;&lt;li&gt;Select the appropriate pattern &lt;strong&gt;type&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the trigger.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Triggers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permSubstringTrigger(&amp;quot;My drink trigger&amp;quot;, &amp;quot;&amp;quot;, &amp;quot;You are thirsty.&amp;quot;, function() send(&amp;quot;drink water&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will keep you refreshed.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="124"/>
      <source>&lt;p&gt;Scripts organize code and can react to events. To add a new script:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Enter a script in the box below. You can for example define &lt;strong&gt;functions&lt;/strong&gt; to be called by other triggers, aliases, etc.&lt;/li&gt;&lt;li&gt;If you write lua &lt;strong&gt;commands&lt;/strong&gt; without defining a function, they will be run on Mudlet startup and each time you open the script for editing.&lt;/li&gt;&lt;li&gt;If needed, you can register a list of &lt;strong&gt;events&lt;/strong&gt; with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the script.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Scripts are run automatically when viewed, even if they are deactivated.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Events can also be added to a script from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;nameOfTheMudletEvent&amp;quot;, &amp;quot;nameOfYourFunctionToBeCalled&amp;quot;)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="135"/>
      <source>&lt;p&gt;Timers react after a timespan once or regularly. To add a new timer:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define the &lt;strong&gt;timespan&lt;/strong&gt; after which the timer should react in a this format: hours : minutes : seconds.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game when the time has passed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the timer.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Timers can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;hello!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will greet you exactly 3 seconds after it was made.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="146"/>
      <source>&lt;p&gt;Buttons react on mouse clicks. To add a new button:&lt;ol&gt;&lt;li&gt;Add a new group to define a new &lt;strong&gt;button bar&lt;/strong&gt; in case you don&apos;t have any.&lt;/li&gt;&lt;li&gt;Add new groups as &lt;strong&gt;menus&lt;/strong&gt; to a button bar or sub-menus to menus.&lt;li&gt;&lt;li&gt;Add new items as &lt;strong&gt;buttons&lt;/strong&gt; to a button bar or menu or sub-menu.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the toolbar, menu or button. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Deactivated items will be hidden and if they are toolbars or menus then all the items they contain will be also be hidden.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If a button is made a &lt;strong&gt;click-down&lt;/strong&gt; button then you may also define a clear text command that you want to send to the game when the button is pressed a second time to uncheck it or to write a script to run when it happens - within such a script the Lua &apos;getButtonState()&apos; function reports whether the button is up or down.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="156"/>
      <source>&lt;p&gt;Keys react on keyboard presses. To add a new key binding:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Click on &lt;strong&gt;&apos;grab key&apos;&lt;/strong&gt; and then press your key combination, e.g. including modifier keys like Control, Shift, etc.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the new key binding.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Keys can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;my jump key&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;jump&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Pressing F8 will make you jump.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="166"/>
      <source>&lt;p&gt;Variables store information. To make a new variable:&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above. To add a table instead click &apos;Add Group&apos;.&lt;/li&gt;&lt;li&gt;Select type of variable value (can be a string, integer, boolean)&lt;/li&gt;&lt;li&gt;Enter the value you want to store in this variable.&lt;/li&gt;&lt;li&gt;If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.&lt;/li&gt;&lt;li&gt;To remove a variable manually, set it to &apos;nil&apos; or click on the &apos;Delete&apos; icon above.&lt;/li&gt;&lt;/ol&gt;&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables created here won&apos;t be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables and tables can also be defined from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will create a string called &apos;foo&apos; with &apos;bar&apos; as its value.&lt;/p&gt;&lt;p&gt;Check the manual for &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Contents&apos;&gt;more information&lt;/a&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="241"/>
      <source>-- add your Lua code here</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="543"/>
      <source>Errors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="682"/>
      <source>Match case precisely</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="686"/>
      <source>Include variables</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="689"/>
      <source>Search variables (slower)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="781"/>
      <source>perl regex</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="783"/>
      <source>exact match</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="784"/>
      <source>lua function</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="785"/>
      <source>line spacer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="786"/>
      <source>color trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="787"/>
      <source>prompt</source>
      <translation type="unfinished"/>
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
      <translation>Disparador</translation>
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
      <translation>Nombre</translation>
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
      <translation>Comando</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1954"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1959"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2055"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2060"/>
      <source>Pattern {%1}</source>
      <translation>Patrón {%1}</translation>
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
      <translation>Código Lua (%1:%2)</translation>
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
      <translation>Patrón</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1743"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2211"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2233"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2265"/>
      <source>Script</source>
      <translation>Script</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1765"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1770"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2233"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2238"/>
      <source>Event Handler</source>
      <translation>Controlador De Eventos</translation>
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
      <translation>Botón</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1633"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1638"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2309"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2314"/>
      <source>Command {Down}</source>
      <translation>Comando {Abajo}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1652"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1657"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2328"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2333"/>
      <source>Command {Up}</source>
      <translation>Comando {Arriba}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1681"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2357"/>
      <source>Action</source>
      <translation>Acción</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1681"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1686"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2357"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2362"/>
      <source>Stylesheet {L: %1 C: %2}</source>
      <translation>Hoja De Estilos {L: %1 C: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1549"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1561"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1591"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2421"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2463"/>
      <source>Timer</source>
      <translation>Temporizador</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1489"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1519"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2495"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2507"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2537"/>
      <source>Key</source>
      <translation>Tecla</translation>
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
      <translation>Valor</translation>
    </message>
  </context>
  <context>
    <name>dlgTriggerPatternEdit</name>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="52"/>
      <source>Text to find (anywhere in the game output)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="55"/>
      <source>Text to find (as a regular expression pattern)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="58"/>
      <source>Text to find (from beginning of the line)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="61"/>
      <source>Exact line to match</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerPatternEdit.cpp" line="64"/>
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
      <translation>clave (string)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="53"/>
      <source>index (integer number)</source>
      <translation>índice (número entero)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="54"/>
      <source>table (use &quot;Add Group&quot; to create)</source>
      <translation>tabla (use &quot;Agregar Grupo&quot; para crear)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="55"/>
      <source>function (cannot create from GUI)</source>
      <translation>función (no se puede crear a partir de la interfaz gráfica)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="79"/>
      <source>string</source>
      <translation>cadena de texto</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="80"/>
      <source>number</source>
      <translation>número</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="81"/>
      <source>boolean</source>
      <translation>booleano</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="82"/>
      <source>table</source>
      <translation>tabla</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="83"/>
      <source>function</source>
      <translation>función</translation>
    </message>
  </context>
  <context>
    <name>edbee::TextEditorComponent</name>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="578"/>
      <source>Cut</source>
      <translation>Cortar</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="579"/>
      <source>Copy</source>
      <translation>Copiar</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="580"/>
      <source>Paste</source>
      <translation>Pegar</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="582"/>
      <source>Select All</source>
      <translation>Seleccionar todo</translation>
    </message>
  </context>
  <context>
    <name>irc</name>
    <message>
      <location filename="../src/ui/irc.ui" line="25"/>
      <source>Mudlet IRC Client</source>
      <translation>Cliente IRC de Mudlet</translation>
    </message>
  </context>
  <context>
    <name>keybindings_main_area</name>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your key or key group. This will be displayed in the key tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="40"/>
      <source>Command:</source>
      <translation>Comando:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="50"/>
      <source>&lt;p&gt;Type in one or more commands you want the key to send directly to the game when pressed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
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
      <translation>Combinaciones de teclas:</translation>
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
      <translation>Cerrar</translation>
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
      <location filename="../src/main.cpp" line="169"/>
      <source>Profile to open automatically</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="169"/>
      <source>profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="172"/>
      <location filename="../src/main.cpp" line="178"/>
      <source>Display help and exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="175"/>
      <source>Display version and exit</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="245"/>
      <source>%1 %2%3 (with debug symbols, without optimisations)
</source>
      <comment>%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="251"/>
      <source>Qt libraries %1 (compilation) %2 (runtime)
</source>
      <comment>%1 and %2 are version numbers</comment>
      <translation>Librerías Qt %1 (compilación) %2 (tiempo de ejecución)
</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="255"/>
      <source>Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="256"/>
      <source>This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="236"/>
      <source>Report bugs to: https://github.com/Mudlet/Mudlet/issues
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="237"/>
      <source>Project home page: http://www.mudlet.org/
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="254"/>
      <source>Copyright © 2008-2021  Mudlet developers
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="280"/>
      <source>Version: %1</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>main_window</name>
    <message>
      <location filename="../src/ui/main_window.ui" line="95"/>
      <source>Toolbox</source>
      <translation>Caja de herramientas</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="109"/>
      <source>Options</source>
      <translation>Opciones</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="116"/>
      <source>Help</source>
      <translation>Ayuda</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="126"/>
      <source>About</source>
      <translation>Acerca de</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="134"/>
      <source>Games</source>
      <translation>Juegos</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="149"/>
      <source>Play</source>
      <translation>Jugar</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="152"/>
      <source>&lt;p&gt;Configure connection details of, and make a connection to, game servers.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="160"/>
      <source>&lt;p&gt;Disconnect from the current game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="168"/>
      <source>&lt;p&gt;Disconnect and then reconnect to the current game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="176"/>
      <source>&lt;p&gt;Configure setting for the Mudlet application globally and for the current profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="184"/>
      <source>&lt;p&gt;Opens the Editor for the different types of things that can be scripted by the user.&lt;/p&gt;</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="224"/>
      <source>&lt;p&gt;Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="232"/>
      <source>&lt;p&gt;Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="240"/>
      <source>&lt;p&gt;Opens the (on-line) Mudlet Forum in your system web-browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="261"/>
      <source>&lt;p&gt;Show or hide the game map.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="269"/>
      <source>&lt;p&gt;Install and remove collections of Mudlet lua items (packages).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="277"/>
      <source>&lt;p&gt;Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="285"/>
      <source>&lt;p&gt;Gather and bundle up collections of Mudlet Lua items and other reasources into a module.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="316"/>
      <source>&lt;p&gt;Hide / show the search area and buttons at the bottom of the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="321"/>
      <source>Discord help channel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="326"/>
      <location filename="../src/ui/main_window.ui" line="329"/>
      <source>Report an issue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="332"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="157"/>
      <source>Disconnect</source>
      <translation>Desconectar</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="165"/>
      <source>Reconnect</source>
      <translation>Reconectar</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="173"/>
      <source>Preferences</source>
      <translation>Preferencias</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="181"/>
      <source>Script editor</source>
      <translation>Editor de scripts</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="189"/>
      <source>Notepad</source>
      <translation>Bloc de notas</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="197"/>
      <source>API Reference</source>
      <translation>Referencia de la API</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="237"/>
      <source>Online forum</source>
      <translation>Foro en línea</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="205"/>
      <source>About Mudlet</source>
      <translation>Acerca De Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="192"/>
      <source>&lt;p&gt;Opens a free form text editor window for this profile that is saved between sessions.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="200"/>
      <source>&lt;p&gt;Opens the Mudlet manual in your web browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="208"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="221"/>
      <source>Video tutorials</source>
      <translation>Videotutoriales</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="229"/>
      <source>Load replay</source>
      <translation>Cargar reproducción</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="245"/>
      <source>Check for updates...</source>
      <translation>Buscar actualizaciones...</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="250"/>
      <source>Live help chat</source>
      <translation>Chat de ayuda en vivo</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="258"/>
      <source>Show map</source>
      <translation>Mostrar mapa</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="266"/>
      <source>Package manager</source>
      <translation>Gestor de paquetes</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="274"/>
      <source>Module manager</source>
      <translation>Gestor de módulos</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="282"/>
      <source>Package exporter (experimental)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="299"/>
      <source>MultiView</source>
      <translation>Vista Múltiple</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="302"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="307"/>
      <source>Compact input line</source>
      <translation type="unfinished"/>
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
      <translation>Área:</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="521"/>
      <source>Rooms</source>
      <translation>Habitaciones</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="550"/>
      <source>Exits</source>
      <translation>Salidas</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="579"/>
      <source>Round</source>
      <translation>Redondo</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="586"/>
      <source>Info</source>
      <translation>Info</translation>
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
&lt;i&gt;Note: &lt;b&gt;.zip&lt;/b&gt; and &lt;b&gt;.mpackage&lt;/b&gt; modules are currently unable to be synced, only &lt;b&gt;.xml&lt;/b&gt; packages are able to be synchronized across profiles at the moment.&lt;/i&gt;&lt;/p&gt;
&lt;p&gt;For each save operation, modules are backed up to a directory, &lt;i&gt;moduleBackups&lt;/i&gt;, within your Mudlet profile directory.&lt;/p&gt;
&lt;/body&gt;&lt;/html&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="129"/>
      <source>Uninstall</source>
      <translation>Desinstalar</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="142"/>
      <source>Install</source>
      <translation>Instalar</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="149"/>
      <source>Module Help</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>mudlet</name>
    <message>
      <location filename="../src/mudlet.cpp" line="718"/>
      <source>Afrikaans</source>
      <extracomment>In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a &apos;-&apos; Chinese is an exception!</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="719"/>
      <source>Afrikaans (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="720"/>
      <source>Aragonese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="721"/>
      <source>Aragonese (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="722"/>
      <source>Arabic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="723"/>
      <source>Arabic (United Arab Emirates)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="724"/>
      <source>Arabic (Bahrain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="725"/>
      <source>Arabic (Algeria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="727"/>
      <source>Arabic (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="728"/>
      <source>Arabic (Iraq)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="729"/>
      <source>Arabic (Jordan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="730"/>
      <source>Arabic (Kuwait)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="731"/>
      <source>Arabic (Lebanon)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="732"/>
      <source>Arabic (Libya)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="733"/>
      <source>Arabic (Morocco)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="734"/>
      <source>Arabic (Oman)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="735"/>
      <source>Arabic (Qatar)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="736"/>
      <source>Arabic (Saudi Arabia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="737"/>
      <source>Arabic (Sudan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="738"/>
      <source>Arabic (Syria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="739"/>
      <source>Arabic (Tunisia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="740"/>
      <source>Arabic (Yemen)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="741"/>
      <source>Belarusian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="742"/>
      <source>Belarusian (Belarus)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="743"/>
      <source>Belarusian (Russia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="744"/>
      <source>Bulgarian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="745"/>
      <source>Bulgarian (Bulgaria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="746"/>
      <source>Bangla</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="747"/>
      <source>Bangla (Bangladesh)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="748"/>
      <source>Bangla (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="749"/>
      <source>Tibetan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="750"/>
      <source>Tibetan (China)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="751"/>
      <source>Tibetan (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="752"/>
      <source>Breton</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="753"/>
      <source>Breton (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="754"/>
      <source>Bosnian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="755"/>
      <source>Bosnian (Bosnia/Herzegovina)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="756"/>
      <source>Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="757"/>
      <source>Catalan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="758"/>
      <source>Catalan (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="759"/>
      <source>Catalan (Spain - Valencian)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="760"/>
      <source>Central Kurdish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="761"/>
      <source>Central Kurdish (Iraq)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="762"/>
      <source>Czech</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="763"/>
      <source>Czech (Czechia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="764"/>
      <source>Danish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="765"/>
      <source>Danish (Denmark)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="766"/>
      <source>German</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="767"/>
      <source>German (Austria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="768"/>
      <source>German (Austria, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="769"/>
      <source>German (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="770"/>
      <source>German (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="771"/>
      <source>German (Switzerland, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="772"/>
      <source>German (Germany/Belgium/Luxemburg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="773"/>
      <source>German (Germany/Belgium/Luxemburg, revised by F M Baumann)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="774"/>
      <source>German (Liechtenstein)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="775"/>
      <source>German (Luxembourg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="776"/>
      <source>Greek</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="777"/>
      <source>Greek (Greece)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="778"/>
      <source>English</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="779"/>
      <source>English (Antigua/Barbuda)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="780"/>
      <source>English (Australia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="782"/>
      <source>English (Bahamas)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="783"/>
      <source>English (Botswana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="784"/>
      <source>English (Belize)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="726"/>
      <source>Arabic (Egypt)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="480"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="492"/>
      <source>Report issue</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="781"/>
      <source>English (Australia, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="785"/>
      <source>English (Canada)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="786"/>
      <source>English (Canada, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="787"/>
      <source>English (Denmark)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="788"/>
      <source>English (United Kingdom)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="789"/>
      <source>English (United Kingdom, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="790"/>
      <source>English (United Kingdom - &apos;ise&apos; not &apos;ize&apos;)</source>
      <comment>This dictionary prefers the British &apos;ise&apos; form over the American &apos;ize&apos; one.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="791"/>
      <source>English (Ghana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="792"/>
      <source>English (Hong Kong SAR China)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="793"/>
      <source>English (Ireland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="794"/>
      <source>English (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="795"/>
      <source>English (Jamaica)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="796"/>
      <source>English (Namibia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="797"/>
      <source>English (Nigeria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="798"/>
      <source>English (New Zealand)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="799"/>
      <source>English (Philippines)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="800"/>
      <source>English (Singapore)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="801"/>
      <source>English (Trinidad/Tobago)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="802"/>
      <source>English (United States)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="803"/>
      <source>English (United States, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="804"/>
      <source>English (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="805"/>
      <source>English (Zimbabwe)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="806"/>
      <source>Spanish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="807"/>
      <source>Spanish (Argentina)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="808"/>
      <source>Spanish (Bolivia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="809"/>
      <source>Spanish (Chile)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="810"/>
      <source>Spanish (Colombia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="811"/>
      <source>Spanish (Costa Rica)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="812"/>
      <source>Spanish (Cuba)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="813"/>
      <source>Spanish (Dominican Republic)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="814"/>
      <source>Spanish (Ecuador)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="815"/>
      <source>Spanish (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="816"/>
      <source>Spanish (Guatemala)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="817"/>
      <source>Spanish (Honduras)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="818"/>
      <source>Spanish (Mexico)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="819"/>
      <source>Spanish (Nicaragua)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="820"/>
      <source>Spanish (Panama)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="821"/>
      <source>Spanish (Peru)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="822"/>
      <source>Spanish (Puerto Rico)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="823"/>
      <source>Spanish (Paraguay)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="824"/>
      <source>Spanish (El Savador)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="825"/>
      <source>Spanish (United States)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="826"/>
      <source>Spanish (Uruguay)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="827"/>
      <source>Spanish (Venezuela)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="828"/>
      <source>Estonian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="829"/>
      <source>Estonian (Estonia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="830"/>
      <source>Basque</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="831"/>
      <source>Basque (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="832"/>
      <source>Basque (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="833"/>
      <source>French</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="834"/>
      <source>French (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="835"/>
      <source>French (Catalan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="836"/>
      <source>French (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="837"/>
      <source>French (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="838"/>
      <source>French (Luxemburg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="839"/>
      <source>French (Monaco)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="840"/>
      <source>Gaelic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="841"/>
      <source>Gaelic (United Kingdom {Scots})</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="842"/>
      <source>Galician</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="843"/>
      <source>Galician (Spain)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="844"/>
      <source>Gujarati</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="845"/>
      <source>Gujarati (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="846"/>
      <source>Hebrew</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="847"/>
      <source>Hebrew (Israel)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="848"/>
      <source>Hindi</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="849"/>
      <source>Hindi (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="850"/>
      <source>Croatian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="851"/>
      <source>Croatian (Croatia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="852"/>
      <source>Hungarian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="853"/>
      <source>Hungarian (Hungary)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="854"/>
      <source>Armenian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="855"/>
      <source>Armenian (Armenia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="856"/>
      <source>Interlingue</source>
      <comment>formerly known as Occidental, and not to be mistaken for Interlingua</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="857"/>
      <source>Icelandic</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="858"/>
      <source>Icelandic (Iceland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="859"/>
      <source>Italian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="860"/>
      <source>Italian (Switzerland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="861"/>
      <source>Italian (Italy)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="862"/>
      <source>Kazakh</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="863"/>
      <source>Kazakh (Kazakhstan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="864"/>
      <source>Kurmanji</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="865"/>
      <source>Kurmanji {Latin-alphabet Kurdish}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="866"/>
      <source>Korean</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="867"/>
      <source>Korean (South Korea)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="868"/>
      <source>Kurdish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="869"/>
      <source>Kurdish (Syria)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="870"/>
      <source>Kurdish (Turkey)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="871"/>
      <source>Lao</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="872"/>
      <source>Lao (Laos)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="873"/>
      <source>Lithuanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="874"/>
      <source>Lithuanian (Lithuania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="875"/>
      <source>Malayalam</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="876"/>
      <source>Malayalam (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="877"/>
      <source>Norwegian Bokmål</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="878"/>
      <source>Norwegian Bokmål (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="879"/>
      <source>Nepali</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="880"/>
      <source>Nepali (Nepal)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="881"/>
      <source>Dutch</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="882"/>
      <source>Dutch (Netherlands Antilles)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="883"/>
      <source>Dutch (Aruba)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="884"/>
      <source>Dutch (Belgium)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="885"/>
      <source>Dutch (Netherlands)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="886"/>
      <source>Dutch (Suriname)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="887"/>
      <source>Norwegian Nynorsk</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="888"/>
      <source>Norwegian Nynorsk (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="889"/>
      <source>Occitan</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="890"/>
      <source>Occitan (France)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="891"/>
      <source>Polish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="892"/>
      <source>Polish (Poland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="893"/>
      <source>Portuguese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="894"/>
      <source>Portuguese (Brazil)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="895"/>
      <source>Portuguese (Portugal)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="896"/>
      <source>Romanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="897"/>
      <source>Romanian (Romania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="898"/>
      <source>Russian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="899"/>
      <source>Russian (Russia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="900"/>
      <source>Northern Sami</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="901"/>
      <source>Northern Sami (Finland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="902"/>
      <source>Northern Sami (Norway)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="903"/>
      <source>Northern Sami (Sweden)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="904"/>
      <source>Shtokavian</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="905"/>
      <source>Shtokavian (former state of Yugoslavia)</source>
      <comment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="906"/>
      <source>Sinhala</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="907"/>
      <source>Sinhala (Sri Lanka)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="908"/>
      <source>Slovak</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="909"/>
      <source>Slovak (Slovakia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="910"/>
      <source>Slovenian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="911"/>
      <source>Slovenian (Slovenia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="912"/>
      <source>Somali</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="913"/>
      <source>Somali (Somalia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="914"/>
      <source>Albanian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="915"/>
      <source>Albanian (Albania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="916"/>
      <source>Serbian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="917"/>
      <source>Serbian (Montenegro)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="918"/>
      <source>Serbian (Serbia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="919"/>
      <source>Serbian (Serbia - Latin-alphabet)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="920"/>
      <source>Serbian (former state of Yugoslavia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="921"/>
      <source>Swati</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="922"/>
      <source>Swati (Swaziland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="923"/>
      <source>Swati (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="924"/>
      <source>Swedish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="925"/>
      <source>Swedish (Sweden)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="926"/>
      <source>Swedish (Finland)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="927"/>
      <source>Swahili</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="928"/>
      <source>Swahili (Kenya)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="929"/>
      <source>Swahili (Tanzania)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="930"/>
      <source>Turkish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="931"/>
      <source>Telugu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="932"/>
      <source>Telugu (India)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="933"/>
      <source>Thai</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="934"/>
      <source>Thai (Thailand)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="935"/>
      <source>Tigrinya</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="936"/>
      <source>Tigrinya (Eritrea)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="937"/>
      <source>Tigrinya (Ethiopia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="938"/>
      <source>Turkmen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="939"/>
      <source>Turkmen (Turkmenistan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="940"/>
      <source>Tswana</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="941"/>
      <source>Tswana (Botswana)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="942"/>
      <source>Tswana (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="943"/>
      <source>Tsonga</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="944"/>
      <source>Tsonga (South Africa)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="945"/>
      <source>Ukrainian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="946"/>
      <source>Ukrainian (Ukraine)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="947"/>
      <source>Uzbek</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="948"/>
      <source>Uzbek (Uzbekistan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="949"/>
      <source>Venda</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="950"/>
      <source>Vietnamese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="951"/>
      <source>Vietnamese (Vietnam)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="952"/>
      <source>Vietnamese (DauCu varient - old-style diacritics)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="953"/>
      <source>Vietnamese (DauMoi varient - new-style diacritics)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="954"/>
      <source>Walloon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="955"/>
      <source>Xhosa</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="956"/>
      <source>Yiddish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="957"/>
      <source>Chinese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="958"/>
      <source>Chinese (China - simplified)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="959"/>
      <source>Chinese (Taiwan - traditional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="960"/>
      <source>Zulu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4322"/>
      <source>Hide tray icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4327"/>
      <source>Quit Mudlet</source>
      <translation type="unfinished"/>
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
      <translation>Barra de herramientas principal</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="330"/>
      <location filename="../src/mudlet.cpp" line="337"/>
      <location filename="../src/mudlet.cpp" line="339"/>
      <source>Connect</source>
      <translation>Conectar</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="342"/>
      <source>Disconnect</source>
      <translation>Desconectar</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="401"/>
      <source>Open Discord</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="406"/>
      <source>Open IRC</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="458"/>
      <source>Package Exporter</source>
      <translation>Exportador de paquetes</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="349"/>
      <source>Triggers</source>
      <translation>Disparadores</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="350"/>
      <source>Show and edit triggers</source>
      <translation>Mostrar y editar disparadores</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="357"/>
      <source>Aliases</source>
      <translation>Alias</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="358"/>
      <source>Show and edit aliases</source>
      <translation>Mostrar y editar alias</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="363"/>
      <source>Timers</source>
      <translation>Temporizadores</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="364"/>
      <source>Show and edit timers</source>
      <translation>Mostrar y editar temporizadores</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="369"/>
      <source>Buttons</source>
      <translation>Botones</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="370"/>
      <source>Show and edit easy buttons</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="375"/>
      <source>Scripts</source>
      <translation>Scripts</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="376"/>
      <source>Show and edit scripts</source>
      <translation>Mostrar y editar los scripts</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="381"/>
      <source>Keys</source>
      <translation>Teclas</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="382"/>
      <source>Show and edit keys</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="387"/>
      <source>Variables</source>
      <translation>Variables</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="388"/>
      <source>Show and edit Lua variables</source>
      <translation>Mostrar y editar variables de Lua</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="414"/>
      <source>Map</source>
      <translation>Mapa</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="415"/>
      <source>Show/hide the map</source>
      <translation>Mostrar/ocultar el mapa</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="420"/>
      <source>Manual</source>
      <translation>Manual</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="421"/>
      <source>Browse reference material and documentation</source>
      <translation>Examinar el material de referencia y documentación</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="426"/>
      <source>Settings</source>
      <translation>Configuración</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="427"/>
      <source>See and edit profile preferences</source>
      <translation>Ver y editar preferencias de perfil</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="435"/>
      <source>Notepad</source>
      <translation>Bloc de notas</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="436"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation>Abrir un bloc de notas en el que puede almacenar sus notas</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="442"/>
      <location filename="../src/mudlet.cpp" line="449"/>
      <location filename="../src/mudlet.cpp" line="451"/>
      <source>Package Manager</source>
      <translation>Gestor de paquetes</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="454"/>
      <source>Module Manager</source>
      <translation>Gestor de módulo</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="468"/>
      <source>Replay</source>
      <translation>Repetir</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="473"/>
      <source>Reconnect</source>
      <translation>Reconectar</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="474"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation>Te desconecta del juego y vuelve a conectarte a él</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="479"/>
      <source>MultiView</source>
      <translation>Vista Múltiple</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="503"/>
      <location filename="../src/mudlet.cpp" line="3445"/>
      <source>About</source>
      <translation>Acerca de...</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="504"/>
      <location filename="../src/mudlet.cpp" line="3428"/>
      <source>&lt;p&gt;Inform yourself about this version of Mudlet, the people who made it and the licence under which you can share it.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in 3 places - please ensure all have the same translation).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="962"/>
      <source>ASCII (Basic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="963"/>
      <source>UTF-8 (Recommended)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="968"/>
      <source>ISO 8859-1 (Western European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="969"/>
      <source>ISO 8859-2 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-2 (europa Central)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="970"/>
      <source>ISO 8859-3 (South European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-8859-3 (Europa del sur)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="971"/>
      <source>ISO 8859-4 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-4 (Báltico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="972"/>
      <source>ISO 8859-5 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-8859-5 (Cirílico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="973"/>
      <source>ISO 8859-6 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-8859-6 (Árabe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="974"/>
      <source>ISO 8859-7 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-8859-7 (Griego)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="975"/>
      <source>ISO 8859-8 (Hebrew Visual)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-8 (hebreo Visual)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="976"/>
      <source>ISO 8859-9 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-8859-9 (Turco)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="977"/>
      <source>ISO 8859-10 (Nordic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-10 (Nórdico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="978"/>
      <source>ISO 8859-11 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-11 (latín/tailandés)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="979"/>
      <source>ISO 8859-13 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-13 (idiomas bálticos y polaco)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="980"/>
      <source>ISO 8859-14 (Celtic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-14 (idiomas celtas [gaélico irlandés, escocés, galés])</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="981"/>
      <source>ISO 8859-15 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-15 (Europa occidental)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="982"/>
      <source>ISO 8859-16 (Romanian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO 8859-16 (idiomas centroeuropeos)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="983"/>
      <location filename="../src/mudlet.cpp" line="984"/>
      <source>CP437 (OEM Font)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="985"/>
      <location filename="../src/mudlet.cpp" line="986"/>
      <source>CP667 (Mazovia)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="987"/>
      <location filename="../src/mudlet.cpp" line="988"/>
      <source>CP737 (DOS Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="989"/>
      <source>CP850 (Western Europe)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP850 (Europa occidental)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="990"/>
      <source>CP866 (Cyrillic/Russian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>CP866 (cirílico/ruso)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="991"/>
      <location filename="../src/mudlet.cpp" line="992"/>
      <source>CP869 (DOS Greek 2)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="993"/>
      <source>CP1161 (Latin/Thai)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="994"/>
      <source>KOI8-R (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>ISO-889-5 (cirílico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="995"/>
      <source>KOI8-U (Cyrillic/Ukrainian)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>KOI8-U (cirílico/ucraniano)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="996"/>
      <source>MACINTOSH</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>MACINTOSH</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="997"/>
      <source>WINDOWS-1250 (Central European)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1250 (idiomas centroeuropeos)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="998"/>
      <source>WINDOWS-1251 (Cyrillic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1251 (cirílico)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="999"/>
      <source>WINDOWS-1252 (Western)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1252 (occidental)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1000"/>
      <source>WINDOWS-1253 (Greek)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1253 (griego)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1001"/>
      <source>WINDOWS-1254 (Turkish)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1254 (turco)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1002"/>
      <source>WINDOWS-1255 (Hebrew)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1255 (hebreo)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1003"/>
      <source>WINDOWS-1256 (Arabic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1256 (árabe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1004"/>
      <source>WINDOWS-1257 (Baltic)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1257 (idiomas bálticos)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1005"/>
      <source>WINDOWS-1258 (Vietnamese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation>WINDOWS-1258 (vietnamita)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2489"/>
      <source>Central Debug Console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="516"/>
      <location filename="../src/mudlet.cpp" line="517"/>
      <source>Toggle Full Screen View</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="496"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="964"/>
      <source>GBK (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="965"/>
      <source>GB18030 (Chinese)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="966"/>
      <source>Big5-ETen (Taiwan)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="967"/>
      <source>Big5-HKSCS (Hong Kong)</source>
      <comment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1550"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Disabled until a profile is loaded.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1586"/>
      <location filename="../src/mudlet.cpp" line="2860"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2315"/>
      <source>%1 - notes</source>
      <translation>%1 - notas</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2368"/>
      <source>Select Replay</source>
      <translation>Seleccionar repetición</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2370"/>
      <source>*.dat</source>
      <translation>*.dat</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2627"/>
      <source>[  OK  ]  - Profile &quot;%1&quot; loaded in offline mode.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2762"/>
      <source>&lt;p&gt;Cannot load a replay as one is already in progress in this or another profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;No se puede cargar una repetición, ya que otra se encuentra en curso en este u otro perfil.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2779"/>
      <source>Faster</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2782"/>
      <source>&lt;p&gt;Replay each step with a shorter time interval between steps.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Reproduce cada paso con un intervalo de tiempo más corto entre pasos.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2786"/>
      <source>Slower</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2789"/>
      <source>&lt;p&gt;Replay each step with a longer time interval between steps.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Reproduce cada paso con un intervalo de tiempo más largo entre pasos.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2799"/>
      <location filename="../src/mudlet.cpp" line="2868"/>
      <location filename="../src/mudlet.cpp" line="2881"/>
      <source>Speed: X%1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2806"/>
      <location filename="../src/mudlet.cpp" line="2823"/>
      <source>Time: %1</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3439"/>
      <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
      <comment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3457"/>
      <source>Review %n update(s)...</source>
      <comment>Review update(s) menu item, %n is the count of how many updates are available</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="3462"/>
      <source>&lt;p&gt;Review the update(s) available...&lt;/p&gt;</source>
      <comment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3487"/>
      <source>Update installed - restart to apply</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="3523"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress,
try again when it has finished.</source>
      <translation>[ATENCIÓN] - No se puede realizar la repetición, puede que esté otra en progreso, inténtelo de nuevo cuando haya terminado.</translation>
    </message>
  </context>
  <context>
    <name>package_manager</name>
    <message>
      <location filename="../src/ui/package_manager.ui" line="48"/>
      <source>Install</source>
      <translation>Instalar</translation>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="61"/>
      <source>Uninstall</source>
      <translation>Desinstalar</translation>
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
      <translation>Preferencias de perfil</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="55"/>
      <source>General</source>
      <translation>General</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="61"/>
      <source>Icon sizes</source>
      <translation>Tamaño de los iconos</translation>
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
      <translation>Nunca</translation>
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
      <translation>Siempre</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="139"/>
      <source>Show main toolbar</source>
      <translation>Mostrar barra de herramientas principal</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="239"/>
      <source>Allow server to install script packages</source>
      <translation>Permitir al servidor instalar paquetes de scripts</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="266"/>
      <source>Game protocols</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="323"/>
      <location filename="../src/ui/profile_preferences.ui" line="3306"/>
      <source>Please reconnect to your game for the change to take effect</source>
      <translation>Por favor, vuelve a conectarte a tu juego para que el cambio surta efecto</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="339"/>
      <source>Log options</source>
      <translation>Opciones del archivo de registro</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="348"/>
      <source>Save log files in HTML format instead of plain text</source>
      <translation>Guardar los archivos de registro en formato HTML en lugar de en texto sin formato</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="355"/>
      <source>Add timestamps at the beginning of log lines</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="362"/>
      <source>Save log files in:</source>
      <translation>Guardar archivos de registro en:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="379"/>
      <source>Browse...</source>
      <translation>Examinar...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="386"/>
      <source>Reset</source>
      <translation>Reestablecer</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="393"/>
      <source>Log format:</source>
      <translation>Formato del archivo de registro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="406"/>
      <source>Log name:</source>
      <translation>Nombre del archivo de registro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="426"/>
      <source>.txt</source>
      <translation>.txt</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="459"/>
      <source>Input line</source>
      <translation>Línea de entrada</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="468"/>
      <source>Input</source>
      <translation>Entrada</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="474"/>
      <source>use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="477"/>
      <source>Strict UNIX line endings</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="494"/>
      <source>Show the text you sent</source>
      <translation>Mostrar el texto enviado</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="484"/>
      <source>Auto clear the input line after you sent text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="201"/>
      <source>&lt;p&gt;If you are playing a non-English game and seeing � instead of text, or special letters like &lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt; aren&apos;t showing right - try changing the encoding to UTF-8 or to one suggested by your game.&lt;/p&gt;&lt;p&gt;For some encodings on some Operating Systems Mudlet itself has to provide the codec needed; if that is the case for this Mudlet then there will be a &lt;tt&gt;m &lt;/tt&gt; prefixed applied to those encoding names (so if they have errors the blame can be applied correctly!)&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="302"/>
      <source>&lt;p&gt;Enables MSP - provides Mud Sound Protocol messages during game play for supported games&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="345"/>
      <source>&lt;p&gt;When checked will cause the date-stamp named log file to be HTML (file extension &apos;.html&apos;) which can convey color, font and other formatting information rather than a plain text (file extension &apos;.txt&apos;) format.  If changed while logging is already in progress it is necessary to stop and restart logging for this setting to take effect in a new log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="514"/>
      <source>React to all keybindings on the same key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="524"/>
      <source>Command separator:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="534"/>
      <source>Enter text to separate commands with or leave blank to disable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="541"/>
      <source>Command line minimum height in pixels:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="663"/>
      <source>Main display</source>
      <translation>Pantalla principal</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="675"/>
      <source>Font</source>
      <translation>Fuente</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="681"/>
      <source>Font:</source>
      <translation>Fuente:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="704"/>
      <source>Size:</source>
      <translation>Tamaño:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="726"/>
      <source>Use anti aliasing on fonts. Smoothes fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry. </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="729"/>
      <source>Enable anti-aliasing</source>
      <translation>Habilitar anti-aliasing</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="758"/>
      <source>Display Border</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="779"/>
      <source>Top border height:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="823"/>
      <source>Left border width:</source>
      <translation>Anchura del borde izquierdo:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="867"/>
      <source>Bottom border height:</source>
      <translation>Altura del borde inferior:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="911"/>
      <source>Right border width:</source>
      <translation>Anchura del borde derecho:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="952"/>
      <source>Word wrapping</source>
      <translation>Salto automatico de linea</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="976"/>
      <source>Wrap lines at:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="996"/>
      <location filename="../src/ui/profile_preferences.ui" line="1044"/>
      <source>characters</source>
      <translation>caracteres</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1024"/>
      <source>Indent wrapped lines by:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1057"/>
      <source>Double-click</source>
      <translation>Doble clic</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1063"/>
      <source>Stop selecting a word on these characters:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1073"/>
      <source>&apos;&quot;</source>
      <translation>&apos;&quot;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1086"/>
      <source>Display options</source>
      <translation>Opciones de Visualización</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1095"/>
      <source>Fix unnecessary linebreaks on GA servers</source>
      <translation>Arreglar saltos de línea innecesarios en servidores con GA</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1105"/>
      <source>Show Spaces/Tabs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1115"/>
      <source>Use Mudlet on a netbook with a small screen</source>
      <translation>Usar Mudlet en un netbook con pantalla pequeña</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1125"/>
      <source>Show Line/Paragraphs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1135"/>
      <source>Echo Lua errors to the main console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1142"/>
      <source>Make &apos;Ambiguous&apos; E. Asian width characters wide</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1176"/>
      <source>Editor</source>
      <translation>Editor</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1182"/>
      <source>Theme</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1252"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1292"/>
      <source>Color view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1298"/>
      <source>Select your color preferences</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1304"/>
      <source>Foreground:</source>
      <translation>Primer plano:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1327"/>
      <source>Background:</source>
      <translation>Fondo:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1347"/>
      <source>Command line foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1364"/>
      <source>Command line background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1381"/>
      <source>Command foreground:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1401"/>
      <source>Command background:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="272"/>
      <source>&lt;p&gt;Enables GMCP - note that if you have MSDP enabled as well, some servers will prefer one over the other&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="292"/>
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
      <location filename="../src/ui/profile_preferences.ui" line="275"/>
      <source>Enable GMCP  (Generic Mud Communication Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="295"/>
      <source>Enable MSDP  (Mud Server Data Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="282"/>
      <source>&lt;p&gt;Enables MSSP - provides Mud Server Status Protocol information upon connection for supported games&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="285"/>
      <source>Enable MSSP  (Mud Server Status Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="491"/>
      <source>&lt;p&gt;Echo the text you send in the display box.&lt;/p&gt;&lt;p&gt;&lt;i&gt;This can be disabled by the game server if it negotiates to use the telnet ECHO option&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="511"/>
      <source>&lt;p&gt;Check all Key-bindings against key-presses.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Versions of Mudlet prior to &lt;b&gt;3.9.0&lt;/b&gt; would stop checking after the first matching combination of&lt;/i&gt; KeyCode &lt;i&gt;and&lt;/i&gt; KeyModifier &lt;i&gt;was found and then send the command and/or run the script of that Key-binding only.  This&lt;/i&gt; per-profile &lt;i&gt;option tells Mudlet to check and run the remaining matches; but, to retain compatibility with previous versions, defaults to the &lt;b&gt;un&lt;/b&gt;-checked state.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="567"/>
      <source>Spell checking</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="573"/>
      <source>&lt;p&gt;This option controls spell-checking on the command line in the main console for this profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="576"/>
      <source>System/Mudlet dictionary:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="583"/>
      <source>&lt;p&gt;Select one dictionary which will be available on the command line and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="611"/>
      <source>User dictionary:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="617"/>
      <source>&lt;p&gt;A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="620"/>
      <source>Profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="627"/>
      <source>&lt;p&gt;A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="630"/>
      <source>Shared</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="719"/>
      <source>The selected font doesn&apos;t work with Mudlet, please pick another</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="776"/>
      <location filename="../src/ui/profile_preferences.ui" line="792"/>
      <source>&lt;p&gt;Extra space to have before text on top - can be set to negative to move text up beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="820"/>
      <location filename="../src/ui/profile_preferences.ui" line="836"/>
      <source>&lt;p&gt;Extra space to have before text on the left - can be set to negative to move text left beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="864"/>
      <location filename="../src/ui/profile_preferences.ui" line="880"/>
      <source>&lt;p&gt;Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="908"/>
      <location filename="../src/ui/profile_preferences.ui" line="924"/>
      <source>&lt;p&gt;Extra space to have before text on the right - can be set to negative to move text right beyond the screen&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1070"/>
      <source>&lt;p&gt;Enter the characters you&apos;d like double-clicking to stop selecting text on here. If you don&apos;t enter any, double-clicking on a word will only stop at a space, and will include characters like a double or a single quote. For example, double-clicking on the word &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; in the following will select &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;quot;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;quot;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;If you set the characters in the field to &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;quot;! &lt;/span&gt;which will mean it should stop selecting on &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; &amp;quot; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; ! then double-clicking on &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; will just select &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &amp;quot;&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;!&amp;quot;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1076"/>
      <source>(characters to ignore in selection, for example &apos; or &quot;)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1092"/>
      <source>&lt;p&gt;Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1102"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1112"/>
      <source>&lt;p&gt;Select this option for better compatability if you are using a netbook, or some other computer model that has a small screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1122"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show  line and paragraphs ends with visible marks as well as whitespace.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1132"/>
      <source>&lt;p&gt;Prints Lua errors to the main console in addition to the error tab in the editor.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1152"/>
      <source>Enable text analyzer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3473"/>
      <source>h:mm:ss.zzz</source>
      <comment>Used to set a time interval only</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1262"/>
      <source>Autocomplete</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="229"/>
      <source>Notify on new data</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="232"/>
      <source>&lt;p&gt;Show a toolbar notification if Mudlet is minimized and new data arrives.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="246"/>
      <source>Auto save on exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="253"/>
      <source>Allow server to download and play media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="256"/>
      <source>&lt;p&gt;This also needs GMCP to be enabled in the next group below.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="305"/>
      <source>Enable MSP  (Mud Sound Protocol)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="501"/>
      <source>Highlights your input line text when scrolling through your history for easy cancellation</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="504"/>
      <source>Highlight history</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="741"/>
      <source>This font is not monospace, which may not be ideal for playing some text games:
you can use it but there could be issues with aligning columns of text</source>
      <comment>Note that this text is split into two lines so that the message is not too wide in English, please do the same for other locales where the text is the same or longer</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1268"/>
      <source>Autocomplete Lua functions in code editor</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1311"/>
      <source>&lt;p&gt;The foreground color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1334"/>
      <source>&lt;p&gt;The background color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1354"/>
      <source>&lt;p&gt;The foreground color used for the main input area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1371"/>
      <source>&lt;p&gt;The background color used for the main input area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1388"/>
      <source>&lt;p&gt;The foreground color used for text sent to the game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1408"/>
      <source>&lt;p&gt;The background color used for text sent to the game server.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1434"/>
      <source>These preferences set how you want a particular color to be represented visually in the main display:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1444"/>
      <location filename="../src/ui/profile_preferences.ui" line="2262"/>
      <source>Black:</source>
      <translation>Negro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1451"/>
      <source>ANSI Color Number 0</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1464"/>
      <location filename="../src/ui/profile_preferences.ui" line="2279"/>
      <source>Light black:</source>
      <translation>Negro claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1471"/>
      <source>ANSI Color Number 8</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1484"/>
      <location filename="../src/ui/profile_preferences.ui" line="2296"/>
      <source>Red:</source>
      <translation>Rojo:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1491"/>
      <source>ANSI Color Number 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1504"/>
      <location filename="../src/ui/profile_preferences.ui" line="2313"/>
      <source>Light red:</source>
      <translation>Rojo claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1511"/>
      <source>ANSI Color Number 9</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1524"/>
      <location filename="../src/ui/profile_preferences.ui" line="2330"/>
      <source>Green:</source>
      <translation>Verde:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1531"/>
      <source>ANSI Color Number 2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1544"/>
      <location filename="../src/ui/profile_preferences.ui" line="2347"/>
      <source>Light green:</source>
      <translation>Verde claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1551"/>
      <source>ANSI Color Number 10</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1564"/>
      <location filename="../src/ui/profile_preferences.ui" line="2364"/>
      <source>Yellow:</source>
      <translation>Amarillo:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1571"/>
      <source>ANSI Color Number 3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1584"/>
      <location filename="../src/ui/profile_preferences.ui" line="2381"/>
      <source>Light yellow:</source>
      <translation>Amarillo claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1591"/>
      <source>ANSI Color Number 11</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1604"/>
      <location filename="../src/ui/profile_preferences.ui" line="2398"/>
      <source>Blue:</source>
      <translation>Azul:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1611"/>
      <source>ANSI Color Number 4</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1624"/>
      <location filename="../src/ui/profile_preferences.ui" line="2415"/>
      <source>Light blue:</source>
      <translation>Azul celeste:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1631"/>
      <source>ANSI Color Number 12</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1664"/>
      <location filename="../src/ui/profile_preferences.ui" line="2449"/>
      <source>Light magenta:</source>
      <translation>Magenta claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1671"/>
      <source>ANSI Color Number 13</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1684"/>
      <location filename="../src/ui/profile_preferences.ui" line="2466"/>
      <source>Cyan:</source>
      <translation>Cian:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1691"/>
      <source>ANSI Color Number 6</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1704"/>
      <location filename="../src/ui/profile_preferences.ui" line="2483"/>
      <source>Light cyan:</source>
      <translation>Cian claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1711"/>
      <source>ANSI Color Number 14</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1724"/>
      <location filename="../src/ui/profile_preferences.ui" line="2500"/>
      <source>White:</source>
      <translation>Blanco:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1731"/>
      <source>ANSI Color Number 7</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1744"/>
      <location filename="../src/ui/profile_preferences.ui" line="2517"/>
      <source>Light white:</source>
      <translation>Blanco claro:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1751"/>
      <source>ANSI Color Number 15</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1764"/>
      <location filename="../src/ui/profile_preferences.ui" line="2534"/>
      <source>Reset all colors to default</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1771"/>
      <source>&lt;p&gt;If this option is checked the Mud Server may send codes to change the above 16 colors or to reset them to their defaults by using standard ANSI &lt;tt&gt;OSC&lt;/tt&gt; Escape codes.&lt;/p&gt;&lt;p&gt;Specifically &lt;tt&gt;&amp;lt;OSC&amp;gt;Pirrggbb&amp;lt;ST&amp;gt;&lt;/tt&gt; will set the color with index &lt;i&gt;i&lt;/i&gt; to have the color with the given &lt;i&gt;rr&lt;/i&gt; red, &lt;i&gt;gg&lt;/i&gt; green and &lt;i&gt;bb&lt;/i&gt;  blue components where i is a single hex-digit (&apos;0&apos; to &apos;9&apos; or &apos;a&apos; to &apos;f&apos; or &apos;A&apos; to &apos;F&apos; to give a number between 0 an d15) and rr, gg and bb are two digit hex-digits numbers (between 0 to 255); &amp;lt;OSC&amp;gt; is &lt;i&gt;Operating System Command&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;[&lt;/tt&gt; and &amp;lt;ST&amp;gt; is the &lt;i&gt;String Terminator&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Conversely &lt;tt&gt;&amp;lt;OSC&amp;gt;R&amp;lt;ST&amp;gt;&lt;/tt&gt; will reset the colors to the defaults like the button to the right does.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1774"/>
      <source>Server allowed to redefine these colors</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1798"/>
      <source>Mapper</source>
      <translation>Mapeador</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1804"/>
      <source>Map files</source>
      <translation>Archivos de mapa</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1810"/>
      <source>Save your current map:</source>
      <translation>Guardar el mapa actual:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1820"/>
      <source>Press to choose location and save</source>
      <translation>Pulsa para seleccionar la ubicación y guardar</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1827"/>
      <source>Load another map file in:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1837"/>
      <source>Press to choose file and load</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1844"/>
      <source>&lt;p&gt;Mudlet now does some sanity checking and repairing to clean up issues that may have arisen in previous version due to faulty code or badly documented commands. However if significant problems are found the report can be quite extensive, in particular for larger maps.&lt;/p&gt;&lt;p&gt;Unless this option is set, Mudlet will reduce the amount of on-screen messages by hiding many texts and showing a suggestion to review the report file instead.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1928"/>
      <source>&lt;p&gt;Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1958"/>
      <source>Download latest map provided by your game:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1987"/>
      <source>&lt;p&gt;This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer.&lt;/p&gt;&lt;p&gt;3D map view always has anti-aliasing enabled.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1997"/>
      <source>&lt;p&gt;The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2049"/>
      <source>2D map player room marker style:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2059"/>
      <source>Outer ring color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2069"/>
      <source>Inner ring color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2083"/>
      <source>Original</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2088"/>
      <source>Red ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2093"/>
      <source>Blue/Yellow ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2098"/>
      <source>Custom ring</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2106"/>
      <source>&lt;p&gt;Percentage ratio (&lt;i&gt;the default is 120%&lt;/i&gt;) of the marker symbol to the space available for the room.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2112"/>
      <location filename="../src/ui/profile_preferences.ui" line="2140"/>
      <source>%</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2115"/>
      <source>Outer diameter: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2134"/>
      <source>&lt;p&gt;Percentage ratio of the inner diameter of the marker symbol to the outer one (&lt;i&gt;the default is 70%&lt;/i&gt;).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2143"/>
      <source>Inner diameter: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2159"/>
      <source>&lt;p&gt;This enables borders around room. Color can be set in Mapper colors tab&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2162"/>
      <source>Show room borders</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2238"/>
      <source>Room border color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2558"/>
      <source>Chat</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3250"/>
      <source>Special options needed for some older game drivers (needs client restart to take effect)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3288"/>
      <source>Force CHARSET negotiation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3390"/>
      <source>the computer&apos;s password manager (secure)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3395"/>
      <source>plaintext with the profile (portable)</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3382"/>
      <source>Store character login passwords in:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3408"/>
      <source>TextLabel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2816"/>
      <source>TLS/SSL secure connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3120"/>
      <source>Accept self-signed certificates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3130"/>
      <source>Accept expired certificates</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2834"/>
      <source>Certificate</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2894"/>
      <source>Serial:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2843"/>
      <source>Issuer:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2860"/>
      <source>Issued to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2877"/>
      <source>Expires:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3140"/>
      <source>Accept all certificate errors       (unsecure)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1847"/>
      <source>Report map issues on screen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1861"/>
      <source>Copy map to other profile(s):</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1877"/>
      <source>Press to pick destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1890"/>
      <source>Copy to destination(s)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1902"/>
      <source>An action above happened</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1909"/>
      <source>Map format version:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1934"/>
      <location filename="../src/ui/profile_preferences.ui" line="1938"/>
      <source># {default version}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1949"/>
      <source>Map download</source>
      <translation>Descarga de mapa</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1971"/>
      <source>Download</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1981"/>
      <source>Map view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2010"/>
      <source>2D Map Room Symbol Font</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1990"/>
      <source>Use high quality graphics in 2D view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="184"/>
      <source>&lt;p&gt;Can you help translate Mudlet?&lt;/p&gt;&lt;p&gt;If so, please visit: &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1874"/>
      <source>&lt;p&gt;Select profiles that you want to copy map to, then press the Copy button to the right&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1887"/>
      <source>&lt;p&gt;Copy map into the selected profiles on the left&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1955"/>
      <location filename="../src/ui/profile_preferences.ui" line="1968"/>
      <source>&lt;p&gt;On games that provide maps for download, you can press this button to get the latest map. Note that this will &lt;span style=&quot; font-weight:600;&quot;&gt;overwrite&lt;/span&gt; any changes you&apos;ve done to your map, and will use the new map only&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2000"/>
      <source>Show the default area in map area selection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2020"/>
      <source>Only use symbols (glyphs) from chosen font</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2027"/>
      <source>Show symbol usage...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2189"/>
      <source>Mapper colors</source>
      <translation>Colores del mapeador</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2195"/>
      <source>Select your color preferences for the map display</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2201"/>
      <source>Link color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2221"/>
      <source>Background color:</source>
      <translation>Color del fondo:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3244"/>
      <source>Special Options</source>
      <translation>Opciones especiales</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3256"/>
      <source>Force compression off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3274"/>
      <source>Force telnet GA signal interpretation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3263"/>
      <source>This option adds a line line break &lt;LF&gt; or &quot;
&quot; to your command input on empty commands. This option will rarely be necessary.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3267"/>
      <source>Force new line on empty commands</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3281"/>
      <source>Force MXP negotiation off</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2635"/>
      <source>Discord privacy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2668"/>
      <source>Don&apos;t hide small icon or tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2673"/>
      <source>Hide small icon tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2678"/>
      <source>Hide small icon and tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2700"/>
      <source>Hide timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2723"/>
      <location filename="../src/ui/profile_preferences.ui" line="2736"/>
      <location filename="../src/ui/profile_preferences.ui" line="2752"/>
      <location filename="../src/ui/profile_preferences.ui" line="2768"/>
      <source>&lt;p&gt;Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). Leave empty to show it for any Discord account you log in to.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2729"/>
      <source>Restrict to:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2642"/>
      <source>Don&apos;t hide large icon or tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2647"/>
      <source>Hide large icon tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2652"/>
      <source>Hide large icon and tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2707"/>
      <source>&lt;p&gt;Allow Lua to set Discord status&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2710"/>
      <source>Enable Lua API</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2742"/>
      <source>specific Discord username</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2686"/>
      <source>Hide state</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2693"/>
      <source>Hide party details</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2660"/>
      <source>Hide detail</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2564"/>
      <source>IRC client options</source>
      <translation>Opciones del cliente de IRC</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2580"/>
      <source>irc.example.net</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2587"/>
      <source>Port:</source>
      <translation>Puerto:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2625"/>
      <source>#channel1 #channel2 #etc...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2611"/>
      <source>MudletUser123</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2570"/>
      <source>Server address:</source>
      <translation>Dirección del servidor:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2618"/>
      <source>Auto-join channels: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2604"/>
      <source>Nickname:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2597"/>
      <source>6667</source>
      <translation>6667</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3338"/>
      <source>Search Engine</source>
      <translation>Motor de búsqueda</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3322"/>
      <source>Mudlet updates</source>
      <translation>Actualizaciones de Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3328"/>
      <source>Disable automatic updates</source>
      <translation>Deshabilitar las actualizaciones automáticas</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3350"/>
      <source>Other Special options</source>
      <translation>Otras Opciones especiales</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3356"/>
      <source>Show icons on menus</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2810"/>
      <source>Connection</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3150"/>
      <source>Connect to the game via proxy</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3165"/>
      <source>Address</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3185"/>
      <source>port</source>
      <translation>puerto</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3195"/>
      <source>Username for logging into the proxy if requred</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3201"/>
      <source>username (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3211"/>
      <source>Password for logging into the proxy if requred</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3217"/>
      <source>password (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3436"/>
      <source>Show debug messages for timers not smaller than:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3442"/>
      <source>&lt;p&gt;Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the &lt;i&gt;Central Debug Console&lt;/i&gt; useless if there is a very small interval timer running.&lt;/p&gt;</source>
      <comment>The term in &apos;...&apos; refer to a Mudlet specific thing and ought to match the corresponding translation elsewhere.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3483"/>
      <source>Report all Codepoint problems immediately</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3541"/>
      <source>Save</source>
      <translation>Guardar</translation>
    </message>
  </context>
  <context>
    <name>room_exits</name>
    <message>
      <location filename="../src/ui/room_exits.ui" line="37"/>
      <source>General exits:</source>
      <translation>Salidas generales:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="96"/>
      <source>NW exit...</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="236"/>
      <source>N exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="376"/>
      <source>NE exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="516"/>
      <source>Up exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="659"/>
      <source>W exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="767"/>
      <source>ID:</source>
      <translation>ID:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="790"/>
      <source>Weight:</source>
      <translation>Peso:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="866"/>
      <source>E exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1003"/>
      <source>Down exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1155"/>
      <source>SW exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1301"/>
      <source>S exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1441"/>
      <source>SE exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1581"/>
      <source>In exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1721"/>
      <source>Out exit...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1826"/>
      <source>Key:</source>
      <translation>Tecla:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1844"/>
      <source>No route</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1859"/>
      <source>Stub Exit</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="93"/>
      <source>&lt;p&gt;Set the number of the room northwest of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
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
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="233"/>
      <source>&lt;p&gt;Set the number of the room north of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="373"/>
      <source>&lt;p&gt;Set the number of the room northeast of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="513"/>
      <source>&lt;p&gt;Set the number of the room up from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="558"/>
      <location filename="../src/ui/room_exits.ui" line="1045"/>
      <location filename="../src/ui/room_exits.ui" line="1623"/>
      <location filename="../src/ui/room_exits.ui" line="1763"/>
      <source>&lt;p&gt;A symbol is drawn with a green (Open) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="574"/>
      <location filename="../src/ui/room_exits.ui" line="1061"/>
      <location filename="../src/ui/room_exits.ui" line="1639"/>
      <location filename="../src/ui/room_exits.ui" line="1779"/>
      <source>&lt;p&gt;A symbol is drawn with an orange (Closed) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="590"/>
      <location filename="../src/ui/room_exits.ui" line="1077"/>
      <location filename="../src/ui/room_exits.ui" line="1655"/>
      <location filename="../src/ui/room_exits.ui" line="1795"/>
      <source>&lt;p&gt;A symbol is drawn with a red (Locked) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="653"/>
      <source>&lt;p&gt;Set the number of the room west of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="780"/>
      <source>&lt;p&gt;This is the Room ID Number for this room - it cannot be changed here!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="803"/>
      <source>&lt;p&gt;This is the default weight for this room, which will be used for any exit &lt;i&gt;that leads to &lt;u&gt;this room&lt;/u&gt;&lt;/i&gt; which does not have its own value set - this value cannot be changed here.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="863"/>
      <source>&lt;p&gt;Set the number of the room east of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1000"/>
      <source>&lt;p&gt;Set the number of the room down from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1152"/>
      <source>&lt;p&gt;Set the number of the room southwest of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1298"/>
      <source>&lt;p&gt;Set the number of the room south of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1438"/>
      <source>&lt;p&gt;Set the number of the room southeast of this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1578"/>
      <source>&lt;p&gt;Set the number of the room in from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1718"/>
      <source>&lt;p&gt;Set the number of the room out from this one, will be blue for a valid number or red for invalid.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1871"/>
      <source>&lt;p&gt;Set the number of the room that&apos;s to the southwest here.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1874"/>
      <source>Exit RoomID number</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1901"/>
      <source>No door</source>
      <translation>Ninguna puerta</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1916"/>
      <source>Open door</source>
      <translation>Puerta abierta</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1928"/>
      <source>Closed door</source>
      <translation>Puerta cerrada</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1940"/>
      <source>Locked door</source>
      <translation>Puerta asegurada</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1975"/>
      <source>&lt;p&gt;Use this button to save any changes, will also remove any invalid Special exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1991"/>
      <source>&lt;p&gt;Use this button to close the dialogue without changing anything.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2010"/>
      <source>&lt;p&gt;Click on an item to edit/change it, to DELETE a Special Exit set its Exit Room ID to zero.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2048"/>
      <source>&lt;p&gt;Set the number of the room that this exit leads to, if set to zero the exit will be removed on saving the exits.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2075"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit (only functional choice currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2084"/>
      <source>&lt;p&gt;Green (Open) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2093"/>
      <source>&lt;p&gt;Orange (Closed) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2102"/>
      <source>&lt;p&gt;Red (Locked) door symbol would be drawn on 2D Map (but not currently).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2111"/>
      <source>&lt;p&gt;(Lua scripts need to be prefixed with &quot;script:&quot;).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2128"/>
      <source>&lt;p&gt;Add an empty item to Special exits to be edited as required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2147"/>
      <source>&lt;p&gt;Press this button to deactivate the selection of a Special exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1978"/>
      <source>&amp;Save</source>
      <translation>&amp;Guardar</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1886"/>
      <source>Exit Weight (0=No override)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1994"/>
      <source>&amp;Cancel</source>
      <translation>&amp;Cancelar</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2001"/>
      <source>Special exits:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2044"/>
      <source>Exit
Room ID</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2053"/>
      <source>No
Route</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2062"/>
      <source>Exit
Weight</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2066"/>
      <source>&lt;p&gt;Set to a positive integer value to override the default (Room) Weight for using this Exit route, a zero value assigns the default.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2071"/>
      <source>Door
None</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2080"/>
      <source>Door
Open</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2089"/>
      <source>Door
Closed</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2098"/>
      <source>Door
Locked</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2107"/>
      <source>Command
or LUA script</source>
      <translation>Comando o script LUA</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2131"/>
      <source>&amp;Add special exit</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2150"/>
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
      <translation>Reestablecer</translation>
    </message>
  </context>
  <context>
    <name>scripts_main_area</name>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="40"/>
      <source>Registered Event Handlers:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.&lt;/p&gt;&lt;p&gt;If a function withn the script is to be used to handle events entered in the list below &lt;b&gt;&lt;u&gt;it must have the same name as is entered here.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</source>
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
      <translation>-</translation>
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
      <translation>+</translation>
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
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="64"/>
      <source>Command:</source>
      <translation>Comando:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="169"/>
      <source>Time:</source>
      <translation>Hora:</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="39"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="74"/>
      <source>&lt;p&gt;Type in one or more commands you want the timer to send directly to the game when the time has elapsed. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
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
      <location filename="../src/ui/timers_main_area.ui" line="134"/>
      <source>&lt;p&gt;miliseconds&lt;/p&gt;</source>
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
      <translation>1</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="436"/>
      <source>Show normally hidden variables</source>
      <translation>Mostrar variables normalmente ocultas</translation>
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
      <translation>Nombre:</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="85"/>
      <source>&lt;p&gt;Use this control to show or hide the extra controls for the trigger; this can be used to allow more space to show the trigger &lt;i&gt;items&lt;/i&gt; on smaller screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="111"/>
      <source>Command:</source>
      <translation>Comando:</translation>
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
      <location filename="../src/ui/triggers_main_area.ui" line="360"/>
      <source>Match all occurences of the pattern in the line.</source>
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
      <translation>Seleccionar archivo...</translation>
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
      <translation>Fondo</translation>
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
      <translation>Primer plano</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="482"/>
      <source>play sound</source>
      <translation>reproducir audio</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="78"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="124"/>
      <source>&lt;p&gt;Type in one or more commands you want the trigger to send directly to the game if it fires. (Optional)&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissable to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
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
      <translation>Nombre:</translation>
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
      <location filename="../src/ui/vars_main_area.ui" line="138"/>
      <source>&lt;p&gt;Tables can store values either in a list, and/or a hashmap.&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;list&lt;/b&gt;, &lt;i&gt;unique indexed keys&lt;/i&gt; represent values - so you can have values at &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;map&lt;/b&gt; {a.k.a. an &lt;i&gt;associative array}&lt;/i&gt;, &lt;i&gt;unique keys&lt;/i&gt; represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).&lt;/p&gt;&lt;p&gt;This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.&lt;/p&gt;&lt;p&gt;In other cases it displays other entities (&lt;span style=&quot; font-style:italic;&quot;&gt;functions&lt;/span&gt;) which cannot be modifed from here.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</source>
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
      <translation>Nombre de la variable ...</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="150"/>
      <source>key (string)</source>
      <translation>clave (string)</translation>
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
      <translation>tabla</translation>
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
      <translation>variable oculta</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="84"/>
      <source>⏷ Value type:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="56"/>
      <source>string</source>
      <translation>cadena de texto</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="61"/>
      <source>number</source>
      <translation>número</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="66"/>
      <source>boolean</source>
      <translation>booleano</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="76"/>
      <source>function</source>
      <translation>función</translation>
    </message>
  </context>
</TS>
