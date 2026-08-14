<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-TW" sourcelanguage="en">
  <context>
    <name>CrashReporter</name>
    <message>
      <location filename="../src/crash_reporter/main.cpp" line="76"/>
      <source>Mudlet Crash</source>
      <translation>Mudlet 當機報告</translation>
    </message>
    <message>
      <location filename="../src/crash_reporter/main.cpp" line="79"/>
      <source>&lt;div align=&apos;center&apos;&gt;&lt;b&gt;Mudlet has encountered a problem.&lt;/b&gt;&lt;br&gt;&lt;br&gt;You can choose to send a crash report to help us improve the application.&lt;/div&gt;</source>
      <translation>&lt;div align=&apos;center&apos;&gt;&lt;b&gt;Mudlet 遇到了問題。&lt;/b&gt;&lt;br&gt;&lt;br&gt;您可以選擇發送崩潰報告以幫助我們改進應用程式。&lt;/div&amp;gt；</translation>
    </message>
    <message>
      <location filename="../src/crash_reporter/main.cpp" line="86"/>
      <source>Send this time</source>
      <translation>本次傳送</translation>
    </message>
    <message>
      <location filename="../src/crash_reporter/main.cpp" line="87"/>
      <source>Always send</source>
      <translation>一律傳送</translation>
    </message>
    <message>
      <location filename="../src/crash_reporter/main.cpp" line="88"/>
      <source>Don&apos;t send</source>
      <translation>不要傳送</translation>
    </message>
  </context>
  <context>
    <name>Discord</name>
    <message>
      <location filename="../src/discord.cpp" line="165"/>
      <source>via Mudlet</source>
      <translation>透過 Mudlet</translation>
    </message>
  </context>
  <context>
    <name>GLWidget</name>
    <message>
      <location filename="../src/glwidget.cpp" line="286"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>地圖中沒有房間 - 載入其他地圖，或從頭開始創建地圖</translation>
    </message>
    <message>
      <location filename="../src/glwidget.cpp" line="291"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>您還沒有地圖 - 加載一個地圖，或從頭開始創建地圖。</translation>
    </message>
    <message>
      <location filename="../src/glwidget.cpp" line="2119"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation>Mapper: 找不到从房间 %1 到 %2 的有效路径。</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/glwidget.cpp" line="288"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>您已載入地圖（共 %n 個房間），但 Mudlet 不知道您現在的位置。</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>GMCPAuthenticator</name>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="256"/>
      <source>[ WARN ]  - Not using your saved sign-in because this connection is not encrypted; please sign in again.</source>
      <extracomment>Shown when a saved password-less sign-in cannot be reused because this connection to the game is not encrypted.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="326"/>
      <source>[ WARN ]  - Could not save your sign-in for next time; you may need to sign in again.</source>
      <extracomment>Shown when the user opted to stay signed in but saving the sign-in token failed, so they will have to sign in again next time.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="361"/>
      <source>[ INFO ]  - Resuming your %1 sign-in with the game.</source>
      <extracomment>Shown when Mudlet asks the game to restart the browser sign-in with the remembered provider; %1 is the provider name (e.g. Discord).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="452"/>
      <source>[ WARN ]  - The game sent an invalid sign-in link; cannot continue.</source>
      <extracomment>Shown when the game sends a sign-in link with an unsupported or invalid address (not an http/https web link).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="472"/>
      <source>[ INFO ]  - To sign in, open this link in your browser: %1</source>
      <extracomment>%1 is the sign-in web address the user should open in their browser to sign in.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="479"/>
      <source>[ WARN ]  - Could not open your browser. Open this link manually to sign in: %1</source>
      <extracomment>%1 is the sign-in web address the user should open manually in their browser.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="490"/>
      <source>[ INFO ]  - Opening your browser to sign in. Complete the login there, then return here.</source>
      <extracomment>Shown after the user&apos;s browser is launched to complete an OAuth/web sign-in. %1 is the provider name (e.g. Discord).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="491"/>
      <source>[ INFO ]  - Opening your browser to sign in with %1. Complete the login there, then return here.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="519"/>
      <source>[ WARN ]  - The browser sign-in could not be completed; reconnect to try again.</source>
      <extracomment>Shown when a browser-based sign-in with the game&apos;s own account could not be completed.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="549"/>
      <source>[ WARN ]  - Cannot complete the sign-in because the connection is not encrypted.</source>
      <extracomment>Shown when a browser sign-in finished but the game connection is not encrypted, so completing it would be unsafe.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="661"/>
      <source>[ WARN ]  - Could not log in to the game, is the login information correct?</source>
      <translation>[ 警告 ]  - 無法登入遊戲，登入資訊是否正確？</translation>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="664"/>
      <source>[ WARN ]  - Could not log in to the game: %1</source>
      <extracomment>%1 shows the reason for failure, could be authentication, etc.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="800"/>
      <source>[ INFO ]  - Your saved sign-in has expired; reconnecting so you can sign in again.</source>
      <extracomment>Shown when a saved password-less sign-in is no longer accepted; Mudlet reconnects so the user can sign in again.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/GMCPAuthenticator.cpp" line="895"/>
      <source>[ INFO ]  - You&apos;ll be signed in automatically next time. Manage this under Preferences, Connection.</source>
      <extracomment>Shown once after a browser/OAuth sign-in whose reconnect token was saved, so future connects need no sign-in.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>Host</name>
    <message>
      <location filename="../src/Host.cpp" line="390"/>
      <source>Text to send to the game</source>
      <translation>要傳送至遊戲的文字</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="515"/>
      <source>[ ALERT ] - This profile will now save and close.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="832"/>
      <source>Failed to open xml file &quot;%1&quot; inside module %2 to update it. Error message was: &quot;%3&quot;.</source>
      <extracomment>This error message will appear when the xml file inside the module zip cannot be updated for some reason.</extracomment>
      <translation>無法開啟並更新 %2 模組中的 xml 文件 &quot;%1&quot;。錯誤訊息為：&quot;%3&quot;。</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="845"/>
      <source>Failed to save &quot;%1&quot; to module &quot;%2&quot;. Error message was: &quot;%3&quot;.</source>
      <extracomment>This error message will appear when a module is saved as package but cannot be done for some reason.</extracomment>
      <translation>無法將 &quot;%1&quot; 保存到模組 &quot;%2&quot;。錯誤訊息為：&quot;%3&quot;。</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1078"/>
      <source>the profile is no longer available</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1291"/>
      <source>[  OK  ]  - %1 Thanks a lot for using the Public Test Build!</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ] - %1 非常感謝您使用公開測試版本！</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="1292"/>
      <source>[  OK  ]  - %1 Help us make Mudlet better by reporting any problems.</source>
      <comment>%1 will be a random happy emoji</comment>
      <translation>[  OK  ] - %1 提交任何問題，幫助我們讓 Mudlet 變得更好。</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2125"/>
      <source>[ ERROR ] - Package install failed for &quot;%1&quot;: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2194"/>
      <source>Module &quot;%1&quot; is already installed. Please uninstall it first or choose a different name.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2233"/>
      <source>Unpacking module:
&quot;%1&quot;
please wait...</source>
      <translation>正在解壓縮模組：
&quot;%1&quot;
請稍後…</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2233"/>
      <source>Unpacking package:
&quot;%1&quot;
please wait...</source>
      <translation>正在解壓縮套件：
&quot;%1&quot;
請稍後…</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2234"/>
      <source>Unpacking</source>
      <translation>正在解壓縮</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="2309"/>
      <location filename="../src/Host.cpp" line="2361"/>
      <source>[ WARN ]  - Failed to load module &quot;%1&quot;: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3121"/>
      <source>Playing %1</source>
      <translation>正在玩 %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3126"/>
      <location filename="../src/Host.cpp" line="3135"/>
      <source>%1 at %2:%3</source>
      <extracomment>%1 is the game name and %2:%3 is game server address like: mudlet.org:23</extracomment>
      <translation>%2:%3 上的 %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="3649"/>
      <location filename="../src/Host.cpp" line="4924"/>
      <source>Map - %1</source>
      <translation>地圖 - %1</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="4935"/>
      <source>Pre-Map loading(3) report</source>
      <translation>加载地图前 (3) 的报告</translation>
    </message>
    <message>
      <location filename="../src/Host.cpp" line="4945"/>
      <source>Loading map(3) at %1 report</source>
      <translation>加载地图中 (3) 的报告，时间：%1</translation>
    </message>
  </context>
  <context>
    <name>KeyUnit</name>
    <message>
      <location filename="../src/KeyUnit.cpp" line="435"/>
      <source>no key chosen</source>
      <extracomment>Displayed when no key binding has been set</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/KeyUnit.cpp" line="442"/>
      <source>%1undefined key (code: 0x%2)</source>
      <comment>%1 is a string describing the modifier keys (e.g. &quot;shift&quot; or &quot;control&quot;) used with the key, whose &apos;code&apos; number, in %2 is not one that we have a name for. This is probably one of those extra keys around the edge of the keyboard that some people have.</comment>
      <translation>%1 未定义的键（代码：0x%2)</translation>
    </message>
  </context>
  <context>
    <name>LabelInteractionHandler</name>
    <message>
      <location filename="../src/LabelInteractionHandler.cpp" line="220"/>
      <source>Move</source>
      <extracomment>2D Mapper context menu (label) item</extracomment>
      <translation>移动</translation>
    </message>
    <message>
      <location filename="../src/LabelInteractionHandler.cpp" line="222"/>
      <source>Move label</source>
      <extracomment>2D Mapper context menu item (label) tooltip</extracomment>
      <translation>移动标签</translation>
    </message>
    <message>
      <location filename="../src/LabelInteractionHandler.cpp" line="226"/>
      <source>Delete</source>
      <extracomment>2D Mapper context menu (label) item</extracomment>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../src/LabelInteractionHandler.cpp" line="228"/>
      <source>Delete label</source>
      <extracomment>2D Mapper context menu (label) item tooltip</extracomment>
      <translation>删除标签</translation>
    </message>
  </context>
  <context>
    <name>MMCPClient</name>
    <message>
      <location filename="../src/MMCPClient.cpp" line="124"/>
      <source>[ CHAT ]  - Waiting for response from %1:%2...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="143"/>
      <source>[ CHAT ]  - You are now disconnected from &lt;unknown&gt; - %1:%2.</source>
      <extracomment>This message is used when a MMCP peer without a name disconnects, * %1 is the peer&apos;s IP address (numbers or URL), %2 is the port they are * listening on. Should be similiar to the one when we do have a name.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="149"/>
      <source>[ CHAT ]  - You are now disconnected from %1 - %2:%3.</source>
      <extracomment>This message is used when a MMCP peer with a name disconnects, * %1 is the peer&apos;s name, %2 is the peer&apos;s IP address (numbers or URL), * %3 is the port they are listening on. Should be similiar to the one when * we do not have a name.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="171"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 timed out (not accepted or denied by you).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="219"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 denied (Peer name too long (64 chars max)).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="251"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 denied (DoNotDisturb).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="278"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 is pending, use mmcp.accept(%4) or mmcp.deny(%4) to accept or deny.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="311"/>
      <source>[ CHAT ]  - Connection to %1:%2 refused.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="321"/>
      <source>[ CHAT ]  - Connection to %1 at %2:%3 rejected.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="335"/>
      <source>[ CHAT ]  - Connection to %1 at %2:%3 accepted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="387"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 accepted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="409"/>
      <source>[ CHAT ]  - Connection from %1 at %2:%3 denied.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="426"/>
      <source>[ CHAT ]  - The peer closed or refused the connection.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="429"/>
      <source>[ CHAT ]  - The peer was not found. Please check the host name and port settings.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="432"/>
      <source>[ CHAT ]  - The connection was refused by the peer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="435"/>
      <source>[ CHAT ]  - The following error occurred: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="471"/>
      <source>[ CHAT ]  - Pinging %1...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="485"/>
      <source>[ CHAT ]  - Attempting to peek at %1&apos;s public connections...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="499"/>
      <source>[ CHAT ]  - Requested connections from %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="688"/>
      <source>[ CHAT ]  - Badly formatted connection list from %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="698"/>
      <source>[ CHAT ]  - Error parsing host value from connection: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="703"/>
      <source>[ CHAT ]  - Attempting to connect to %1:%2 provided by %3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="717"/>
      <source>[ CHAT ]  - %1 is trying to request your connections!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="723"/>
      <source>[ CHAT ]  - %1 has requested your public connections...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="728"/>
      <source>[ CHAT ]  - %1 has requested your public connections, but you&apos;re ignoring connection requests...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="775"/>
      <source>%1%2%3%4(%5)%1%2%6%1</source>
      <extracomment>Incoming group message, %1, %2 and %4 are ANSI Escape codes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="791"/>
      <source>[ CHAT ]  - %1 is now known as %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="803"/>
      <source>[ CHAT ]  - %1 is trying to peek your connections!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="809"/>
      <source>[ CHAT ]  - %1 is peeking at your connections...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="814"/>
      <source>[ CHAT ]  - %1 is trying to peek your connections, but you&apos;re ignoring peek requests...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="832"/>
      <source>[ CHAT ]  - Badly formatted peek list from %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="853"/>
      <source>Id   Name                 Address         Port
==== ==================== =============== =====
%1
%2==== ==================== =============== =====%3
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="882"/>
      <source>[ CHAT ]  - Ping returned from %1: %2 ms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="885"/>
      <source>[ CHAT ]  - Bad Ping response from %1: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="903"/>
      <source>[ CHAT ]  - %1 tried to snoop you but doesn&apos;t have permission.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="912"/>
      <source>[ CHAT ]  - %1 has stopped snooping you.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPClient.cpp" line="920"/>
      <source>[ CHAT ]  - %1 has begun snooping you.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>MMCPServer</name>
    <message>
      <location filename="../src/MMCPServer.cpp" line="170"/>
      <source>[ CHAT ]  - You must specify a host.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="207"/>
      <source>[ CHAT ]  - Already connected to %1:%2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="213"/>
      <source>[ CHAT ]  - Connecting to %1:%2...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="247"/>
      <source>You chat to %1, &apos;%2&apos;</source>
      <extracomment>%1 is the name of the peer receiving the message %2</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="255"/>
      <location filename="../src/MMCPServer.cpp" line="575"/>
      <source>[ CHAT ]  - Invalid client id &apos;%1&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="290"/>
      <source>You chat to everybody, &apos;%1&apos;</source>
      <extracomment>%1 is message sent to everyone</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="364"/>
      <source>%1%2You chat to %3&lt;%4&gt;%1, &apos;%5&apos;%6</source>
      <extracomment>%1 and %3 are ASCII ESC color codes that need to be included BEFORE a * portion of text (the main message %5) and (the group name %4) * respectively and %6 is another code at the very end to reset the colors * back to &quot;normal&quot;. %2 is the prefix added to all chat messages display to us. * Please try and reproduce the positioning of those codes around the translation.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="375"/>
      <source>%1%2You try to chat to &lt;%3%4%1&gt; but it is empty and no-one hears you say: &apos;%5&apos;%6</source>
      <extracomment>%1 and %3 are ASCII ESC color codes that need to be included BEFORE a * portion of text (the main message %5) and (the group name %4) * respectively and %5 is another code at the very end to reset the colors * back to &quot;normal&quot;. %2 is the prefix added to all chat messages display to us. * Please try and reproduce the positioning of those codes around the translation.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="388"/>
      <source>Id</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="389"/>
      <source>Name</source>
      <translation>名稱</translation>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="390"/>
      <source>Address</source>
      <translation>地址</translation>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="391"/>
      <source>Port</source>
      <translation>端口</translation>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="392"/>
      <source>Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="393"/>
      <location filename="../src/MMCPServer.cpp" line="454"/>
      <source>Flags</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="394"/>
      <source>ChatClient</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="446"/>
      <source>Color Key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="446"/>
      <source>Connected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="447"/>
      <source>Pending</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="452"/>
      <source>%1:  F - %2,  I - %3,  P - %4,  S - %5
        n - %6,  N - %7</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="454"/>
      <source>Firewall</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="454"/>
      <source>Ignored</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="454"/>
      <source>Private</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="455"/>
      <source>Serving</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="455"/>
      <source>Allow Snooping</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="455"/>
      <source>Being Snooped</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="469"/>
      <source>[ CHAT ]  - Invalid chat name: tilde (~) and comma (,) are not allowed.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="489"/>
      <source>[ CHAT ]  - You are now known as %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="564"/>
      <source>[ CHAT ]  - Assigned &apos;%1&apos; to group &apos;%2&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="567"/>
      <source>[ CHAT ]  - Removed &apos;%1&apos; from group &apos;%2&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="609"/>
      <source>You emote to everyone: &apos;%1 %2&apos;</source>
      <extracomment>%1 is player&apos;s name, %2 is the emote message sent to everyone</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="614"/>
      <source>%1 %2</source>
      <translation>%1 %2</translation>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="650"/>
      <source>[ CHAT ]  - You are no longer ignoring %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="653"/>
      <source>[ CHAT ]  - You are now ignoring %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="660"/>
      <source>[ CHAT ]  - Cannot find client identified by &apos;%1&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="721"/>
      <source>[ CHAT ]  - %1 is no longer private.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="725"/>
      <source>[ CHAT ]  - %1 is now set as private.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="746"/>
      <source>[ CHAT ]  - You are no longer serving %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="753"/>
      <source>[ CHAT ]  - You are now serving %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="775"/>
      <source>[ CHAT ]  - Unable to start server: %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="780"/>
      <source>[ CHAT ]  - Started server on port %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="794"/>
      <source>[ CHAT ]  - Stopped server</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="799"/>
      <source>[ CHAT ]  - Unable to stop server, it is not running</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="813"/>
      <source>[ CHAT ]  - DoNotDisturb enabled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="815"/>
      <source>[ CHAT ]  - DoNotDisturb disabled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="833"/>
      <source>[ CHAT ]  - %1 is no longer allowed to snoop you.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/MMCPServer.cpp" line="840"/>
      <source>[ CHAT ]  - %1 can now snoop you.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>MapInfoContributorManager</name>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="160"/>
      <source>Area:&#xa0;%1 ID:&#xa0;%2 x:&#xa0;%3&#xa0;&lt;‑&gt;&#xa0;%4 y:&#xa0;%5&#xa0;&lt;‑&gt;&#xa0;%6 z:&#xa0;%7&#xa0;&lt;‑&gt;&#xa0;%8</source>
      <extracomment>%1 is the (text) name of the area, %2 is the area ID number, %3 and %4 are the minimum and maximum x coordinates, %5 and %6 for y, and %7 and %8 for z. This text uses non-breaking spaces (Unicode U+00A0) and non-breaking hyphens which are used to prevent the line being split at some places it might otherwise be. When translating, please consider at which points the text may be divided to fit onto more than one line.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="175"/>
      <source>Room Name: %1</source>
      <translation>房間名稱：%1</translation>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="196"/>
      <location filename="../src/mapInfoContributorManager.cpp" line="217"/>
      <location filename="../src/mapInfoContributorManager.cpp" line="239"/>
      <source>Room&#xa0;ID:&#xa0;%1 Position&#xa0;on&#xa0;Map: (%2,%3,%4) ‑&#xa0;%5</source>
      <extracomment>This text is shown when room(s) are (not) selected in mapper. %1 is the room ID number, and %2, %3, %4 are the x, y, and z coordinates of the current/selected room, or a room near the middle of the selection. %5 is a description like: Current player room. This text uses non-breaking spaces (Unicode &#xa0;) and a non-breaking hyphen (‑). They are used to prevent the line being split at unexpected places. When translating, please consider at which points the text may be divided to fit onto more than one line.
----------
This text is shown when room(s) are (not) selected in mapper. %1 is the room ID number, and %2, %3, %4 are the x, y, and z coordinates of the current/selected room, or a room near the middle of the selection. %5 is a description like: Current player room. This text uses non-breaking spaces (Unicode U+00A0) and a non-breaking hyphen (U+2011). They are used to prevent the line being split at unexpected places. When translating, please consider at which points the text may be divided to fit onto more than one line.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="202"/>
      <source>Current player location</source>
      <extracomment>This description is shown when NO room is selected.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mapInfoContributorManager.cpp" line="223"/>
      <source>Selected room</source>
      <extracomment>This description is shown when EXACTLY ONE room is selected.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mapInfoContributorManager.cpp" line="245"/>
      <source>Center of %n selected rooms</source>
      <extracomment>This description is shown when MORE THAN ONE room is selected.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>ModernGLWidget</name>
    <message>
      <location filename="../src/modern_glwidget.cpp" line="256"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>该地图中没有任何房间数据 - 加载另一个, 或从头开始制作新地图。</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/modern_glwidget.cpp" line="258"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>你已加载地图（%n 房间），但 Mudlet 不知道你当前在哪个房间。</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/modern_glwidget.cpp" line="261"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>您还没有地图 - 你可以加载一个现有地图，或新建一个地图。</translation>
    </message>
  </context>
  <context>
    <name>OAuthClientFlow</name>
    <message>
      <location filename="../src/OAuthClientFlow.cpp" line="249"/>
      <source>This sign-in attempt could not be verified. Please return to Mudlet and try again.</source>
      <extracomment>Shown in the user&apos;s web browser when a browser sign-in attempt could not be verified as the one Mudlet started.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/OAuthClientFlow.cpp" line="256"/>
      <source>The sign-in was not completed. You can close this tab and return to Mudlet.</source>
      <extracomment>Shown in the user&apos;s web browser when the identity provider reported that the sign-in did not complete.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/OAuthClientFlow.cpp" line="264"/>
      <source>The sign-in did not complete. Please return to Mudlet and try again.</source>
      <extracomment>Shown in the user&apos;s web browser when the identity provider&apos;s redirect did not include an authorization code.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/OAuthClientFlow.cpp" line="270"/>
      <source>You are signed in. You can close this tab and return to Mudlet.</source>
      <extracomment>Shown in the user&apos;s web browser after a successful browser sign-in.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="140"/>
      <source>! %1</source>
      <translation>! %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="143"/>
      <source>! %1 is away (%2)</source>
      <translation>! %1 離開了 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="145"/>
      <source>! %1 is back</source>
      <translation>! %1 回來了</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="152"/>
      <source>! invited %1 to %2</source>
      <translation>! 邀請 %1 加入 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="155"/>
      <source>! %2 invited to %3</source>
      <translation>! %2 被邀請加入 %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="162"/>
      <source>! You have joined %1 as %2</source>
      <translation>! 你以 %2 的名稱加入了頻道 %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="164"/>
      <source>! %1 has joined %2</source>
      <translation>! %1 已加入 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="170"/>
      <source>! %1 kicked %2</source>
      <translation>! %1 把 %2 踢出了隊伍</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="178"/>
      <source>! %1 mode is %2 %3</source>
      <translation>! %1 模式是 %2 %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="180"/>
      <source>! %1 sets mode %2 %3 %4</source>
      <translation>! %1 設置模式 %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="196"/>
      <source>[MOTD] %1%2</source>
      <translation>[MOTD] %1%2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="208"/>
      <source>! %1 has %2 users: %3</source>
      <translation>! 頻道 %1 目前共有 %2 名使用者：%3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="210"/>
      <source>! %1 has %2 users</source>
      <translation>! 頻道 %1 目前共有 %2 名使用者</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="216"/>
      <source>! %1 has changed nick to %2</source>
      <translation>! %1 將暱稱變更為 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="226"/>
      <source>! %1 replied in %2</source>
      <translation>! %1 在 %2 内回覆了</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="231"/>
      <location filename="../src/ircmessageformatter.cpp" line="281"/>
      <source>! %1 time is %2</source>
      <translation>! %1 時間是 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="236"/>
      <location filename="../src/ircmessageformatter.cpp" line="278"/>
      <source>! %1 version is %2</source>
      <translation>! %1 版本是 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="252"/>
      <source>[%1%2] %3</source>
      <translation>[%1%2] %3</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="260"/>
      <source>&amp;lt;%1%2&amp;gt; [%3] %4</source>
      <translation>&amp;lt;%1%2&amp;gt; [%3] %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="273"/>
      <source>[INFO] %1</source>
      <translation>[INFO] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="300"/>
      <location filename="../src/ircmessageformatter.cpp" line="326"/>
      <source>[ERROR] %1</source>
      <translation>[ERROR] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="310"/>
      <source>[Channel URL] %1</source>
      <translation>[Channel URL] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="319"/>
      <source>[%1] %2</source>
      <translation>[%1] %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="333"/>
      <source>! %1 has left %2</source>
      <translation>! %1 已經離開 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="335"/>
      <source>! %1 has left %2 (%3)</source>
      <translation>! %1 已經離開 %2 (%3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="343"/>
      <source>! %1 replied in %2 seconds</source>
      <translation>! %1在 %2 秒内回覆了</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="357"/>
      <source>* %1 %2</source>
      <translation>* %1 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="363"/>
      <source>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</source>
      <translation>&lt;b&gt;&amp;lt;%1&amp;gt;&lt;/b&gt; %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="370"/>
      <source>! %1 has quit</source>
      <translation>! %1 已經退出</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="372"/>
      <source>! %1 has quit (%2)</source>
      <translation>! %1 已經退出 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="379"/>
      <source>! no topic</source>
      <translation>! 沒有主題</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="388"/>
      <source>[TOPIC] %1</source>
      <translation>[TOPIC] %1</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="392"/>
      <source>! %2 cleared topic</source>
      <translation>! %2 清空了主題</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="395"/>
      <source>! %2 changed topic</source>
      <translation>! %2 變更了主題</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="401"/>
      <source>? %2 %3 %4</source>
      <translation>? %2 %3 %4</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="408"/>
      <source>[WHOIS] %1 is %2@%3 (%4)</source>
      <translation>[WHOIS] %1 is %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="409"/>
      <source>[WHOIS] %1 is connected via %2 (%3)</source>
      <translation>[WHOIS] %1 透過 %2 (%3) 連接</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="410"/>
      <source>[WHOIS] %1 is connected since %2 (idle %3)</source>
      <translation>[WHOIS] %1 連接自 %2 (閒置 %3)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="412"/>
      <source>[WHOIS] %1 is away: %2</source>
      <translation>[WHOIS] %1 離開了: %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="415"/>
      <source>[WHOIS] %1 is logged in as %2</source>
      <translation>[WHOIS] %1 以 %2 的身份登入</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="418"/>
      <source>[WHOIS] %1 is connected from %2</source>
      <translation>[WHOIS] %1 通過 %2 連結</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="421"/>
      <source>[WHOIS] %1 is using a secure connection</source>
      <translation>[WHOIS] %1 正在使用安全連結</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="424"/>
      <source>[WHOIS] %1 is on %2</source>
      <translation>[WHOIS] %1 位於 %2</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="433"/>
      <source>[WHOWAS] %1 was %2@%3 (%4)</source>
      <translation>[WHOWAS] %1 曾是 %2@%3 (%4)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="434"/>
      <source>[WHOWAS] %1 was connected via %2 (%3)</source>
      <translation>[WHOWAS] %1 通過 %2 (%3) 連結</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="436"/>
      <source>[WHOWAS] %1 was logged in as %2</source>
      <translation>[WHOWAS] %1 以 %2 的身份登入</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="444"/>
      <source>[WHO] %1 (%2)</source>
      <translation>[WHO] %1 (%2)</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="446"/>
      <source> - away</source>
      <translation> - 離開</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="449"/>
      <source> - server operator</source>
      <translation>管理员</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="457"/>
      <source>%1s</source>
      <translation>%1 秒</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="464"/>
      <source>%1 days</source>
      <translation>%1 天</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="468"/>
      <source>%1 hours</source>
      <translation>%1 小时</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="472"/>
      <source>%1 mins</source>
      <translation>%1 分鐘</translation>
    </message>
    <message>
      <location filename="../src/ircmessageformatter.cpp" line="474"/>
      <source>%1 secs</source>
      <translation>%1 秒</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="51"/>
      <source>Start element not found!</source>
      <translation>未找到啟動元素！</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="63"/>
      <source>line %1: %2</source>
      <translation>第%1行：%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/baseplistparser.cpp" line="142"/>
      <source>Expected %1 while parsing</source>
      <translation>执行时异常 %1</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/jsonparser.cpp" line="140"/>
      <source>%1 @ line %2</source>
      <translation>%1 在 第 %2 行</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="80"/>
      <source>No data found!</source>
      <translation>找不到資料。</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="87"/>
      <source>Expected object in keymap
</source>
      <translation>预期的键映射对象
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/io/keymapparser.cpp" line="127"/>
      <source>Invalid keysequence used %1
</source>
      <translation>使用的验证序列无效 %1
</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/texteditorkeymap.cpp" line="357"/>
      <source>Error parsing %1: %2 </source>
      <translation>解析 %1时出错: %2 </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/models/textgrammar.cpp" line="303"/>
      <source>Error reading file %1:%2</source>
      <translation>读取文件 %1 出错: %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="433"/>
      <source>%1 ranges</source>
      <translation>%1 范围</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="439"/>
      <source>Line %1, Column %2</source>
      <translation>行 %1，列 %2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="442"/>
      <source>, Offset %1</source>
      <translation>, 偏移 %1</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="446"/>
      <source> | %1 chars selected</source>
      <translation> | %1 个字符被选中</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="450"/>
      <source> | scope: </source>
      <translation> | 范围： </translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/texteditorcontroller.cpp" line="460"/>
      <source> (%1)</source>
      <translation> (%1)</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="373"/>
      <source>Error parsing theme %1:%2</source>
      <translation>解析主题 %1时出错:%2</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/texttheme.cpp" line="378"/>
      <source>Error theme not found %1.</source>
      <translation>未找到主题 %1.</translation>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="1940"/>
      <source>[ INFO ]  - This game seems to wrap its own lines at %1 characters, which
makes triggers awkward to write. Mudlet can undo that, so that triggers
always see whole lines and wrapping follows your window size instead:</source>
      <extracomment>%1 is the screen column that the game appears to wrap its lines at</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="1945"/>
      <source>Done - Mudlet now undoes the game&apos;s wrapping, and triggers see whole lines.</source>
      <extracomment>Confirmation shown after the player clicks the link that enables undoing the game&apos;s own line wrapping</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="1951"/>
      <source>Turn on &quot;Undo the game&apos;s own wrapping&quot; - also found in the settings under Main display</source>
      <extracomment>Tooltip on the link that enables the option to undo the game&apos;s own line wrapping</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="1953"/>
      <source>  ➜ Click here to turn that on now</source>
      <extracomment>Clickable link shown in the main window when a game that wraps its own lines is detected</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="3551"/>
      <source>Send</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="3557"/>
      <source>Prompt</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="3566"/>
      <source>Open browser to</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="3625"/>
      <source>Right-click for menu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TBuffer.cpp" line="4093"/>
      <location filename="../src/TBuffer.cpp" line="7608"/>
      <source>Click to reveal</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="804"/>
      <source>render time: %1S</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="590"/>
      <source>add trigger group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a trigger folder</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="593"/>
      <source>add trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a trigger</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="597"/>
      <source>add alias group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding an alias folder</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="600"/>
      <source>add alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding an alias</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="604"/>
      <source>add timer group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a timer folder</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="607"/>
      <source>add timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a timer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="611"/>
      <source>add script group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a script folder</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="614"/>
      <source>add script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a script</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="618"/>
      <source>add key group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a key folder</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="621"/>
      <source>add key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a key binding</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="625"/>
      <source>add button group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a button toolbar</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="628"/>
      <source>add button &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding a button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="632"/>
      <source>add group &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding an unknown folder type</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorAddItemCommand.cpp" line="635"/>
      <source>add item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for adding an unknown item type</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="849"/>
      <source>delete trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single trigger</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="852"/>
      <source>delete alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single alias</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="855"/>
      <source>delete timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single timer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="858"/>
      <source>delete script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single script</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="861"/>
      <source>delete key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single key binding</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="864"/>
      <source>delete button &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="867"/>
      <source>delete item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deleting a single unknown item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="874"/>
      <source>delete %1 triggers</source>
      <extracomment>Undo/redo menu text for deleting multiple triggers. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="877"/>
      <source>delete %1 aliases</source>
      <extracomment>Undo/redo menu text for deleting multiple aliases. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="880"/>
      <source>delete %1 timers</source>
      <extracomment>Undo/redo menu text for deleting multiple timers. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="883"/>
      <source>delete %1 scripts</source>
      <extracomment>Undo/redo menu text for deleting multiple scripts. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="886"/>
      <source>delete %1 keys</source>
      <extracomment>Undo/redo menu text for deleting multiple key bindings. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="889"/>
      <source>delete %1 buttons</source>
      <extracomment>Undo/redo menu text for deleting multiple buttons. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorDeleteItemCommand.cpp" line="892"/>
      <source>delete %1 items</source>
      <extracomment>Undo/redo menu text for deleting multiple unknown items. %1 = count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="217"/>
      <source>Trigger</source>
      <extracomment>Display name for trigger items in editor</extracomment>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="220"/>
      <source>Alias</source>
      <extracomment>Display name for alias items in editor</extracomment>
      <translation>別名</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="223"/>
      <source>Timer</source>
      <extracomment>Display name for timer items in editor</extracomment>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="226"/>
      <source>Script</source>
      <extracomment>Display name for script items in editor</extracomment>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="229"/>
      <source>Key</source>
      <extracomment>Display name for key binding items in editor</extracomment>
      <translation>按键</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="232"/>
      <source>Action</source>
      <extracomment>Display name for action/button items in editor</extracomment>
      <translation>操作</translation>
    </message>
    <message>
      <location filename="../src/EditorItemXMLHelpers.cpp" line="235"/>
      <source>Item</source>
      <extracomment>Display name for unknown items in editor</extracomment>
      <translation>項目</translation>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="269"/>
      <source>modify trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying a trigger&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="272"/>
      <source>modify alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying an alias&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="275"/>
      <source>modify timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying a timer&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="278"/>
      <source>modify script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying a script&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="281"/>
      <source>modify key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying a key binding&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="284"/>
      <source>modify button/menu/toolbar &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying a button&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorModifyPropertyCommand.cpp" line="287"/>
      <source>modify item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for modifying an unknown item&apos;s properties</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="121"/>
      <source>move trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving a trigger</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="124"/>
      <source>move alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving an alias</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="127"/>
      <source>move timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving a timer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="130"/>
      <source>move script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving a script</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="133"/>
      <source>move key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving a key binding</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="136"/>
      <source>move button &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving a button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorMoveItemCommand.cpp" line="139"/>
      <source>move item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for moving an unknown item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="144"/>
      <source>activate trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating a trigger</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="147"/>
      <source>activate alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating an alias</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="150"/>
      <source>activate timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating a timer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="153"/>
      <source>activate script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating a script</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="156"/>
      <source>activate key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating a key binding</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="159"/>
      <source>activate button &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating a button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="162"/>
      <source>activate item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for activating an unknown item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="169"/>
      <source>deactivate trigger &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating a trigger</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="172"/>
      <source>deactivate alias &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating an alias</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="175"/>
      <source>deactivate timer &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating a timer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="178"/>
      <source>deactivate script &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating a script</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="181"/>
      <source>deactivate key &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating a key binding</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="184"/>
      <source>deactivate button &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating a button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/EditorToggleActiveCommand.cpp" line="187"/>
      <source>deactivate item &quot;%1&quot;</source>
      <extracomment>Undo/redo menu text for deactivating an unknown item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/LuaInterface.cpp" line="107"/>
      <source>Cannot move variable here - the target is not a table</source>
      <extracomment>Error message shown when user tries to drag a variable onto a non-table variable</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TKey.cpp" line="225"/>
      <source>No key binding set. Click &quot;Grab New Key&quot; to assign one.</source>
      <extracomment>Error shown in the editor when a key item has no key binding assigned</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="913"/>
      <source>Telnet Protocol Handler</source>
      <extracomment>Title for the dialog asking if Mudlet should handle telnet:// and telnets:// links</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="915"/>
      <source>Another application is set to handle telnet:// and telnets:// links.</source>
      <extracomment>Text shown when another application is already handling telnet:// and telnets:// links</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="917"/>
      <source>Would you like Mudlet to handle telnet:// and telnets:// links instead?

This will allow you to click on telnet:// and telnets:// links in your browser to automatically open them in Mudlet.

You can change this later in Settings &gt; General.</source>
      <extracomment>Detailed explanation for telnet handler override prompt</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="926"/>
      <source>Don&apos;t ask again</source>
      <extracomment>Checkbox on the telnet handler prompt that suppresses future prompts</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>RoomIdLineEditDelegate</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="145"/>
      <location filename="../src/dlgRoomExits.cpp" line="224"/>
      <source>Entered number is invalid. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation>输入的数字无效。如果这样离开，该出口将在点击 &lt;tt&gt;save&lt;/tt&gt; 时删除。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="146"/>
      <location filename="../src/dlgRoomExits.cpp" line="150"/>
      <location filename="../src/dlgRoomExits.cpp" line="225"/>
      <location filename="../src/dlgRoomExits.cpp" line="229"/>
      <source>Set the number of the room that this special exit goes to.</source>
      <translation>设置该出口通向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="154"/>
      <location filename="../src/dlgRoomExits.cpp" line="233"/>
      <source>The roomID of the room that this special exit leads to is expected here. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>T2DMap</name>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="79"/>
      <source>Undo</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item</extracomment>
      <translation>復原</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="81"/>
      <source>Undo last point</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip</extracomment>
      <translation>復原最後一點</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="89"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="110"/>
      <source>Snap points to grid</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item
----------
2D Mapper context menu (custom line editing) item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="94"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="115"/>
      <source>Snap current points and keep custom line edits aligned to the map grid</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip
----------
2D Mapper context menu (custom line editing) item tooltip</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="97"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="118"/>
      <source>Move last point to target room</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item
----------
2D Mapper context menu (custom line editing) item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="101"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="122"/>
      <source>Snap the final point to the destination room</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip (enabled state)
----------
2D Mapper context menu (custom line editing) item tooltip (enabled state)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="105"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="126"/>
      <source>Select a line with a valid target room and at least one adjustable point</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip (disabled state)
----------
2D Mapper context menu (custom line editing) item tooltip (disabled state)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="109"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="130"/>
      <source>Properties</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)
----------
2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)</extracomment>
      <translation>屬性</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="111"/>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="132"/>
      <source>properties...</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)
----------
2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise</extracomment>
      <translation>屬性...</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="113"/>
      <source>Change the properties of this line</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip</extracomment>
      <translation>變更這條線段的屬性</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="117"/>
      <source>Finish</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item</extracomment>
      <translation>完成</translation>
    </message>
    <message>
      <location filename="../src/CustomLineDrawContextMenuHandler.cpp" line="119"/>
      <source>Finish drawing this line</source>
      <extracomment>2D Mapper context menu (drawing custom exit line) item tooltip</extracomment>
      <translation>完成繪製此線</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="94"/>
      <source>Create new map</source>
      <extracomment>2D Mapper context menu (no map found) item</extracomment>
      <translation>創建地圖</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="186"/>
      <source>Create new room here</source>
      <extracomment>Menu option to create a new room in the mapper</extracomment>
      <translation>在此處創建新房間</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="193"/>
      <source>Move</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>移動</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="200"/>
      <source>Configure room...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="202"/>
      <source>Set room&apos;s name and color of icon, weight and lock for speed walks, and a symbol to mark special rooms</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="209"/>
      <source>Set exits...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>設定出口...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="216"/>
      <source>Create exit line...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>创建出口线...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="219"/>
      <source>Replace an exit line with a custom line</source>
      <extracomment>2D Mapper context menu (room) item tooltip (enabled state)</extracomment>
      <translation>用自定义线替换出口线</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="224"/>
      <source>Custom exit lines are not shown and are not editable in grid mode</source>
      <extracomment>2D Mapper context menu (room) item tooltip (disabled state)</extracomment>
      <translation>自定义的出口线不可见且不可在网格模式中编辑</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="232"/>
      <source>Spread...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>展開...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="234"/>
      <source>Increase map X-Y spacing for the selected group of rooms</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation>增加地图X-Y间距为选定的集团的房间</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="241"/>
      <source>Shrink...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>收縮...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="243"/>
      <source>Decrease map X-Y spacing for the selected group of rooms</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation>减少选定房间组的 map X Y 间距</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="250"/>
      <location filename="../src/T2DMap.cpp" line="5318"/>
      <source>Delete</source>
      <extracomment>2D Mapper context menu (room) item
----------
&quot;Configure Areas&quot; buttons: delete existing area</extracomment>
      <translation>刪除</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="257"/>
      <source>Move to position...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>移动到位置...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="259"/>
      <source>Move selected room or group of rooms to the given coordinates in this area</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation>将选定的房间或房间组移动到该区域中的给定坐标</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="266"/>
      <source>Move to area...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>移动到区域...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="272"/>
      <source>Configure areas...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="274"/>
      <source>Modify and create new areas.</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="279"/>
      <source>Create label...</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>创建标签</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="281"/>
      <source>Create label to show text or an image</source>
      <extracomment>2D Mapper context menu (room) item tooltip</extracomment>
      <translation>创建标签以显示文本或图像</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="286"/>
      <source>Export area to image...</source>
      <extracomment>2D Mapper context menu (area) item</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="288"/>
      <source>Export the current area as an image file</source>
      <extracomment>2D Mapper context menu (area) item tooltip</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="301"/>
      <source>Set player location</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>设置玩家位置</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="303"/>
      <source>Set the player&apos;s current location to here</source>
      <extracomment>2D Mapper context menu (room) item tooltip (enabled state)</extracomment>
      <translation>将角色的当前位置设置在此处</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="159"/>
      <source>Switch to editing mode</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>切换到编辑模式</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="83"/>
      <source>Download from game</source>
      <extracomment>2D Mapper context menu (no map found) item. Downloads the shared map offered by the game server via MMP.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="97"/>
      <source>Load map...</source>
      <extracomment>2D Mapper context menu (no map found) item</extracomment>
      <translation>载入地图...</translation>
    </message>
    <message>
      <location filename="../src/RoomContextMenuHandler.cpp" line="162"/>
      <source>Switch to viewing mode</source>
      <extracomment>2D Mapper context menu (room) item</extracomment>
      <translation>切换到视图模式</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="75"/>
      <source>Add point</source>
      <extracomment>2D Mapper context menu (custom line editing) item</extracomment>
      <translation>添加点</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="79"/>
      <source>Divide segment by adding a new point mid-way along</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip (enabled state)</extracomment>
      <translation>通过在中途添加新点以划分线段</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="83"/>
      <source>Select a point first, then add a new point mid-way along the segment towards room</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)</extracomment>
      <translation>先选择点，再在线段中间添加新点朝向房间</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="87"/>
      <source>Remove point</source>
      <extracomment>2D Mapper context menu (custom line editing) item</extracomment>
      <translation>删除点</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="93"/>
      <source>Merge pair of segments by removing this point</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)</extracomment>
      <translation>通过移除此点合并线段对</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="96"/>
      <source>Remove last segment by removing this point</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)</extracomment>
      <translation>通过移除此点移除上个线段</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="101"/>
      <source>use &quot;delete line&quot; to remove the only segment ending in an editable point</source>
      <extracomment>(2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action &quot;delete line&quot; should match the translation for that action))</extracomment>
      <translation>使用&quot;删除连线&quot;移除以可编辑点为结尾的唯一线段。</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="106"/>
      <source>Select a point first, then remove it</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)</extracomment>
      <translation>先选择一个点, 然后将其删除</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="133"/>
      <source>Change the properties of this custom line</source>
      <translation>變更這條自定義線段的屬性</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="137"/>
      <source>Delete line</source>
      <extracomment>2D Mapper context menu (custom line editing) item</extracomment>
      <translation>删除线</translation>
    </message>
    <message>
      <location filename="../src/CustomLineEditContextMenuHandler.cpp" line="139"/>
      <source>Delete all of this custom line</source>
      <extracomment>2D Mapper context menu (custom line editing) item tooltip</extracomment>
      <translation>删除所有此自定义线</translation>
    </message>
    <message>
      <location filename="../src/SelectionRectangleHandler.cpp" line="108"/>
      <source>Drag to select multiple rooms or labels, release to finish...</source>
      <translation>拖拽选择多个房间或标签，释放完成选择……</translation>
    </message>
    <message>
      <location filename="../src/SelectionRectangleHandler.cpp" line="111"/>
      <source>Hold %1 to add rooms or labels to your current selection.</source>
      <extracomment>%1 is the platform-specific key name for Shift</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/SelectionRectangleHandler.cpp" line="113"/>
      <source>Hold %1 and drag to pan the map.</source>
      <extracomment>%1 is the platform-specific key name for Alt (Alt on Windows/Linux, Option on macOS)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4232"/>
      <location filename="../src/T2DMap.cpp" line="5925"/>
      <source>Solid line</source>
      <translation>實線</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4233"/>
      <location filename="../src/T2DMap.cpp" line="5926"/>
      <source>Dot line</source>
      <translation>點線</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4234"/>
      <location filename="../src/T2DMap.cpp" line="5927"/>
      <source>Dash line</source>
      <translation>虛線</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4235"/>
      <location filename="../src/T2DMap.cpp" line="5928"/>
      <source>Dash-dot line</source>
      <translation>点虚相间线</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4236"/>
      <location filename="../src/T2DMap.cpp" line="5929"/>
      <source>Dash-dot-dot line</source>
      <translation>点虚相间线</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4459"/>
      <source>Click to finish moving the label.</source>
      <extracomment>2D Mapper big, bottom of screen help message when moving a label</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4664"/>
      <source>Move the selection, centered on the highlighted room (%1) to:</source>
      <comment>%1 is a room number</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4670"/>
      <source>x coordinate (was %1):</source>
      <translation>x轴坐标(之前为%1)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4671"/>
      <source>y coordinate (was %1):</source>
      <translation>y轴坐标(之前为%1)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4672"/>
      <source>z coordinate (was %1):</source>
      <translation>z轴坐标(之前为%1)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4688"/>
      <source>OK</source>
      <extracomment>dialog (room(s) move) button</extracomment>
      <translation>確定</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4694"/>
      <source>Cancel</source>
      <extracomment>dialog (room(s) move) button</extracomment>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4743"/>
      <source>Click to finish moving the selected room(s).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5222"/>
      <source>[ ERROR ] - Unable to add &quot;%1&quot; as an area to the map.
See the &quot;[MAP ERROR:]&quot; message for the reason.</source>
      <comment>The &apos;[MAP ERROR:]&apos; text here should be the same as that used for the translation of &quot;[MAP ERROR:] %1&quot; in the &apos;TMap::logError(...)&apos; function.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5278"/>
      <source>Configure Areas</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5314"/>
      <source>Create</source>
      <extracomment>&quot;Configure Areas&quot; buttons: create new area</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5316"/>
      <source>Rename</source>
      <extracomment>&quot;Configure Areas&quot; buttons: rename existing area</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5320"/>
      <source>Close</source>
      <extracomment>&quot;Configure Areas&quot; buttons: close the dialog</extracomment>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5362"/>
      <source>Rename area</source>
      <extracomment>Dialog title for renaming an area</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5362"/>
      <source>New name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5370"/>
      <source>Rename failed</source>
      <extracomment>Warning message shown when renaming an area fails.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5370"/>
      <source>Unable to rename area. Name may be invalid or already in use.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5395"/>
      <source>Create area</source>
      <extracomment>Dialog title for creating a new area</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5395"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5407"/>
      <source>Create failed</source>
      <extracomment>Warning message shown when creating a new area fails.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5407"/>
      <source>Unable to create area. Name may be invalid or already in use.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5442"/>
      <location filename="../src/T2DMap.cpp" line="5450"/>
      <source>Delete failed</source>
      <extracomment>Warning message shown when trying to delete the default area.
----------
Warning message shown when trying to delete an area fails.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5442"/>
      <source>The default area cannot be deleted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5450"/>
      <source>Unable to delete area.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="6077"/>
      <location filename="../src/T2DMap.cpp" line="6111"/>
      <source>Left-click to add point, right-click to undo/change/finish...</source>
      <extracomment>2D Mapper big, bottom of screen help message</extracomment>
      <translation>单击左键添加端点，单击右键 撤销／修改／结束...</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="6122"/>
      <source>Left-click and drag a square for the size and position of your label</source>
      <extracomment>2D Mapper big, bottom of screen help message</extracomment>
      <translation>单击左键并拖动一个方形可以设置标签大小和位置</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="6995"/>
      <source>[MAP]: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="7023"/>
      <source>Unknown Area</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="7042"/>
      <source>Export Area %1 to Image</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="7042"/>
      <source>Image Files (*.png *.jpg *.jpeg *.bmp *.tiff);;All Files (*)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="7055"/>
      <source>[MAP]: Export failed - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="1170"/>
      <source>Mapper: Cannot find a path from %1 to %2 using known exits.</source>
      <translation>Mapper: 找不到从房间 %1 到 %2 的有效路径。</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="500"/>
      <source>Click to select/deselect rooms. Click headers to sort. Name column shows only if rooms are named.</source>
      <extracomment>Tooltip for multi-room selection widget in mapper</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2462"/>
      <source>You do not have a map yet - load one, or start mapping from scratch to begin.</source>
      <translation>您还没有地图 - 你可以加载一个现有地图，或新建一个地图。</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/T2DMap.cpp" line="2459"/>
      <source>You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.</source>
      <translation>
        <numerusform>你已加载地图（%n 房间），但 Mudlet 不知道你当前在哪个房间。</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="495"/>
      <source>ID</source>
      <translation>編號</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="497"/>
      <source>Name</source>
      <translation>名稱</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="2457"/>
      <source>No rooms in the map - load another one, or start mapping from scratch to begin.</source>
      <translation>该地图中没有任何房间数据 - 加载另一个, 或从头开始制作新地图。</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4970"/>
      <source>Spread out rooms</source>
      <translation>分散房间</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="4971"/>
      <source>Increase the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>居中高亮房間，按照指定倍數增加選中房間間距：</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5040"/>
      <source>Shrink in rooms</source>
      <translation>聚拢房间</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5041"/>
      <source>Decrease the spacing of
the selected rooms,
centered on the
highlighted room by a
factor of:</source>
      <translation>居中高亮房間，按照指定倍數減少選中房間間距：</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5122"/>
      <source>Load Mudlet map</source>
      <translation>载入 Mudlet 地图</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5124"/>
      <source>Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)</source>
      <comment>Do not change extensions (in braces) or the ;;s as they are used programmatically</comment>
      <translation>Mudlet 地图 (*.dat);;Xml 地图数据 (*.xml);;所有文件 (*)</translation>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5206"/>
      <source>This will create new area: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/T2DMap.cpp" line="5229"/>
      <source>[  OK  ]  - Added &quot;%1&quot; (%2) area to map.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TAlias</name>
    <message>
      <location filename="../src/TAlias.cpp" line="288"/>
      <source>Error: in &quot;Pattern:&quot;, faulty regular expression, reason: &quot;%1&quot;.</source>
      <translation>错误：在 &quot;的模式:&quot;，错误的正则表达式，原因是: &quot;%1&quot;.</translation>
    </message>
  </context>
  <context>
    <name>TArea</name>
    <message>
      <location filename="../src/TArea.cpp" line="372"/>
      <source>roomID=%1 does not exist, can not set properties of a non-existent room!</source>
      <translation>房間編號 = %1 不存在，不能替一個不存在的房間設置屬性！</translation>
    </message>
    <message>
      <location filename="../src/TArea.cpp" line="809"/>
      <source>no text</source>
      <extracomment>Default text if a label is created in mapper with no text</extracomment>
      <translation>无文本</translation>
    </message>
  </context>
  <context>
    <name>TCommandLine</name>
    <message>
      <location filename="../src/TCommandLine.cpp" line="71"/>
      <location filename="../src/TCommandLine.cpp" line="1872"/>
      <source>Show password</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="780"/>
      <source>Add to user dictionary</source>
      <translation>添加到用户字典</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="782"/>
      <source>Remove from user dictionary</source>
      <translation>从用户字典中删除</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="795"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <extracomment>This line is shown in the list of spelling suggestions on the profile&apos;s command line context menu to clearly divide up where the suggestions for correct spellings are coming from. The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which we have bundled with Mudlet; the entries about this line are the ones that the user has personally added.</extracomment>
      <translation>▼ Mudlet ▼ │ 字典建议 │ ▲ 用户 ▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="806"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <extracomment>This line is shown in the list of spelling suggestions on the profile&apos;s command line context menu to clearly divide up where the suggestions for correct spellings are coming from. The precise format might be modified as long as it is clear that the entries below this line in the menu come from the spelling dictionary that the user has chosen in the profile setting which is provided as part of the OS; the entries about this line are the ones that the user has personally added.</extracomment>
      <translation>▼ 系统 ▼ │ 字典建议 │ ▲ 用户 ▲</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="875"/>
      <source>no suggestions (system)</source>
      <extracomment>Used when the command spelling checker using the selected system dictionary has no words to suggest.</extracomment>
      <translation>没有建议 (系统)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="904"/>
      <source>no suggestions (shared)</source>
      <extracomment>Used when the command spelling checker using the dictionary shared between profile has no words to suggest.</extracomment>
      <translation>没有建议 (共享)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="910"/>
      <source>no suggestions (profile)</source>
      <extracomment>Used when the command spelling checker using the profile&apos;s own dictionary has no words to suggest.</extracomment>
      <translation>没有建议 (配置文件)</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1486"/>
      <source>Input line for &quot;%1&quot; profile.</source>
      <extracomment>Accessibility-friendly name to describe the main command line for a Mudlet profile when more than one profile is loaded, %1 is the profile name. Because this is likely to be used often it should be kept as short as possible.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1493"/>
      <location filename="../src/TCommandLine.cpp" line="1526"/>
      <location filename="../src/TCommandLine.cpp" line="1560"/>
      <source>Type in text to send to the game server for the &quot;%1&quot; profile, or enter an alias to run commands locally.</source>
      <extracomment>Accessibility-friendly description for the main command line for a Mudlet profile when more than one profile is loaded, %1 is the profile name. Because this is likely to be used often it should be kept as short as possible.
----------
Accessibility-friendly description for an extra command line on top of a console/window when more than one profile is loaded, %1 is the profile name.
----------
Accessibility-friendly description for the built-in command line of a console/window other than the main window&apos;s one when more than one profile is loaded, %1 is the profile name.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1502"/>
      <source>Input line.</source>
      <extracomment>Accessibility-friendly name to describe the main command line for a Mudlet profile when only one profile is loaded. Because this is likely to be used often it should be kept as short as possible.</extracomment>
      <translation>輸入設定.</translation>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1508"/>
      <location filename="../src/TCommandLine.cpp" line="1541"/>
      <location filename="../src/TCommandLine.cpp" line="1575"/>
      <source>Type in text to send to the game server, or enter an alias to run commands locally.</source>
      <extracomment>Accessibility-friendly description for the main command line for a Mudlet profile when only one profile is loaded. Because this is likely to be used often it should be kept as short as possible.
----------
Accessibility-friendly description for an extra command line on top of a console/window when only one profile is loaded.
----------
Accessibility-friendly description for the built-in command line of a console/window other than the main window&apos;s one when only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1520"/>
      <source>Additional input line &quot;%1&quot; on &quot;%2&quot; window of &quot;%3&quot;profile.</source>
      <extracomment>Accessibility-friendly name to describe an extra command line on top of console/window when more than one profile is loaded, %1 is the command line name, %2 is the name of the window/console that it is on and %3 is the name of the profile.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1536"/>
      <source>Additional input line &quot;%1&quot; on &quot;%2&quot; window.</source>
      <extracomment>Accessibility-friendly name to describe an extra command line on top of console/window when only one profile is loaded, %1 is the command line name and %2 is the name of the window/console that it is on.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1554"/>
      <source>Input line of &quot;%1&quot; window of &quot;%2&quot; profile.</source>
      <extracomment>Accessibility-friendly name to describe the built-in command line of a console/window other than the main one, when more than one profile is loaded, %1 is the name of the window/console and %2 is the name of the profile.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1569"/>
      <source>Input line of &quot;%1&quot; window.</source>
      <extracomment>Accessibility-friendly name to describe the built-in command line of a console/window other than the main one, when only one profile is loaded, %1 is the name of the window/console.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TCommandLine.cpp" line="1868"/>
      <source>Hide password</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TConsole</name>
    <message>
      <location filename="../src/TConsole.cpp" line="113"/>
      <source>Debug Console</source>
      <translation>调试控制台</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="442"/>
      <source>N:%1 S:%2</source>
      <extracomment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</extracomment>
      <translation>N:%1 S:%2</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="448"/>
      <source>&lt;no GA&gt; S:%1</source>
      <extracomment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</extracomment>
      <translation>&lt;no GA&gt; S:%1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2145"/>
      <source>System Message: %1</source>
      <translation>系統訊息： %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1231"/>
      <source>[ INFO ]  - Split-screen scrollback activated. Press &lt;⌘&gt;+&lt;ENTER&gt; to cancel.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1233"/>
      <source>[ INFO ]  - Split-screen scrollback activated. Press &lt;CTRL&gt;+&lt;ENTER&gt; to cancel.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2574"/>
      <source>Debug messages from all profiles are shown here.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2577"/>
      <source>Central debug console past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet central debug window when you&apos;ve scrolled up</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2579"/>
      <source>Central debug console live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of the Mudlet central debug when you&apos;ve scrolled up</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2582"/>
      <source>Central debug console.</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet central debug window when it is not scrolled up</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2591"/>
      <source>Editor&apos;s error window for profile &quot;%1&quot;, past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet profile&apos;s editor error window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2593"/>
      <source>Editor&apos;s error window for profile &quot;%1&quot;, live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of the Mudlet profile&apos;s editor error window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2596"/>
      <source>Editor&apos;s error window past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet profile&apos;s editor error window when you&apos;ve scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2598"/>
      <source>Editor&apos;s error window live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of the Mudlet profile&apos;s editor error window when you&apos;ve scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2604"/>
      <source>Editor&apos;s error window for profile &quot;%1&quot;.</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet profile&apos;s editor error window when it is not scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2607"/>
      <source>Editor&apos;s error window</source>
      <extracomment>accessibility-friendly name to describe the upper half of the Mudlet profile&apos;s editor error window when it is not scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2614"/>
      <source>Game content is shown here. It may contain subconsoles and a mapper window.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="302"/>
      <source>main window</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="403"/>
      <location filename="../src/TConsole.cpp" line="1035"/>
      <source>Start recording of replay</source>
      <extracomment>Button tooltip for the replay recording toggle button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="413"/>
      <source>Start logging game output to log file.</source>
      <extracomment>Button tooltip for the logging button</extracomment>
      <translation>开始向日志文件写入游戏输出.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="427"/>
      <source>&lt;i&gt;N:&lt;/i&gt; network latency in seconds (ping),&lt;br&gt;&lt;i&gt;S:&lt;/i&gt; system processing time (triggers).</source>
      <extracomment>Tooltip for N and S network latency indicators</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="488"/>
      <source>Search</source>
      <extracomment>search bar placeholder text</extracomment>
      <translation>搜索</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="495"/>
      <source>Search buffer.</source>
      <translation>查找缓冲区.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="498"/>
      <location filename="../src/TConsole.cpp" line="501"/>
      <source>Search Options</source>
      <translation>搜尋選項</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="505"/>
      <source>Case sensitive</source>
      <translation>區分大小寫</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="507"/>
      <source>Match case precisely</source>
      <translation>精确匹配大小写</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="520"/>
      <source>Earlier search result.</source>
      <translation>更早的搜索结果.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="530"/>
      <source>Later search result.</source>
      <translation>最近的搜索结果.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1014"/>
      <source>Failed to open replay recording file for writing.</source>
      <extracomment>Informational message displayed when replay recording file could not be opened</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1022"/>
      <source>Replay recording has started. File: %1</source>
      <translation>回放录制已经开始。文件: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1024"/>
      <source>Stop recording of replay</source>
      <extracomment>Button tooltip for the replay recording toggle button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1029"/>
      <source>Replay recording has been stopped, but couldn&apos;t be saved.</source>
      <extracomment>Informational message displayed when replay recording is stopped but could not be saved</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="1032"/>
      <source>Replay recording has been stopped. File: %1</source>
      <extracomment>Informational message displayed when replay recording is stopped</extracomment>
      <translation>回放录制已停止。文件: %1</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2285"/>
      <location filename="../src/TConsole.cpp" line="2328"/>
      <source>No search results, sorry!</source>
      <translation>未找到搜索结果</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2573"/>
      <source>Debug Console.</source>
      <translation>调试控制台.</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2623"/>
      <source>Profile &quot;%1&quot; main window past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s main window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2625"/>
      <source>Profile &quot;%1&quot; main window live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s main window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2628"/>
      <source>Profile main window past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s main window when you&apos;ve scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2630"/>
      <source>Profile main window live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s main window when you&apos;ve scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2635"/>
      <source>Profile &quot;%1&quot; main window.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s main window when it is not scrolled up, %1 is the name of the profile when more than one is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2638"/>
      <source>Profile main window.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s main window when it is not scrolled up and only one profile is loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2653"/>
      <source>Profile &quot;%1&quot; embedded window &quot;%2&quot; past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2655"/>
      <source>Profile &quot;%1&quot; embedded window &quot;%2&quot; live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2658"/>
      <source>Profile embedded window &quot;%1&quot; past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2660"/>
      <source>Profile embedded window &quot;%1&quot; live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the window.</extracomment>
      <translation>設定檔嵌入視窗 「%1」 即時內容。</translation>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2665"/>
      <source>Profile &quot;%1&quot; embedded window &quot;%2&quot;.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s sub-console window when it is not scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2668"/>
      <source>Profile embedded window &quot;%1&quot;.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s sub-console window when it is not scrolled up, %1 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2684"/>
      <source>Profile &quot;%1&quot; user window &quot;%2&quot; past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s floating/dockable user window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2686"/>
      <source>Profile &quot;%1&quot; user window &quot;%2&quot; live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s floating/dockable user window window when you&apos;ve scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2689"/>
      <source>Profile user window &quot;%1&quot; past content.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2691"/>
      <source>Profile user window &quot;%1&quot; live content.</source>
      <extracomment>accessibility-friendly name to describe the lower half of a Mudlet profile&apos;s sub-console window when you&apos;ve scrolled up, %1 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2696"/>
      <source>Profile &quot;%1&quot; user window &quot;%2&quot;.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s floating/dockable user window window when it is not scrolled up, %1 is the name of the profile when more than one is loaded and %2 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2699"/>
      <source>Profile user window &quot;%1&quot;.</source>
      <extracomment>accessibility-friendly name to describe the upper half of a Mudlet profile&apos;s floating/dockable user window window when it is not scrolled up, %1 is the name of the window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2587"/>
      <source>Error Console in editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="384"/>
      <source>Toggle time stamps</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="470"/>
      <source>Emergency stop! Stop all scripts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2600"/>
      <source>Error messages for the &quot;%1&quot; profile are shown here in the editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2610"/>
      <source>Error messages are shown here in the editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2616"/>
      <source>Main Window for &quot;%1&quot; profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2618"/>
      <source>Main Window.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2645"/>
      <source>Embedded window &quot;%1&quot; for &quot;%2&quot; profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2647"/>
      <source>Embedded window &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2649"/>
      <source>Game content or locally generated text may be sent here.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2675"/>
      <source>User window &quot;%1&quot; for &quot;%2&quot; profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2677"/>
      <source>User window &quot;%1&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TConsole.cpp" line="2680"/>
      <source>Game content or locally generated text may be sent to this window that may be floated away from the Mudlet application or docked within the main application window.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TDetachedWindow</name>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="85"/>
      <location filename="../src/TDetachedWindow.cpp" line="1338"/>
      <source>Mudlet - %1 (Detached)</source>
      <extracomment>This is the title of a Mudlet window which was detached from the main Mudlet window, and %1 is the name of the profile.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="287"/>
      <source>&amp;Close Profile</source>
      <extracomment>This is an item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="289"/>
      <source>Close the current profile</source>
      <extracomment>This explains the &quot;Close Profile&quot; item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="435"/>
      <source>&amp;Window</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="454"/>
      <source>&amp;Reattach to Main Window</source>
      <extracomment>This is an item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="456"/>
      <location filename="../src/TDetachedWindow.cpp" line="863"/>
      <source>Reattach this profile window to the main Mudlet window</source>
      <extracomment>This explains the &quot;Reattach to Main Window&quot; item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.
----------
This explains the &quot;Reattach&quot; item in the toolbar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="465"/>
      <source>Always on &amp;Top</source>
      <extracomment>This is an item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="468"/>
      <source>Keep this window always on top of other windows</source>
      <extracomment>This explains the &quot;Always on Top&quot; item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="473"/>
      <source>&amp;Minimize</source>
      <extracomment>This is an item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="740"/>
      <source>Reattach &apos;%1&apos; to Main Window</source>
      <extracomment>This is an item in the context menu when clicked on a detached tab, and %1 is the name of the profile.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="751"/>
      <source>Close Profile &apos;%1&apos;</source>
      <extracomment>This is an item in the context menu when clicked on a detached tab, and %1 is the name of the profile.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="761"/>
      <source>Close Window (All Profiles)</source>
      <extracomment>This is an item in the context menu when clicked on a detached tab.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="873"/>
      <location filename="../src/TDetachedWindow.cpp" line="882"/>
      <location filename="../src/TDetachedWindow.cpp" line="884"/>
      <source>Connect</source>
      <extracomment>This is an item in the toolbar of a detached Mudlet window.
----------
This is a sub-item of the &quot;Connect&quot; item in the toolbar of a detached Mudlet window.</extracomment>
      <translation>连接</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="888"/>
      <source>Disconnect</source>
      <extracomment>This is a sub-item of the &quot;Connect&quot; item in the toolbar of a detached Mudlet window.</extracomment>
      <translation>中斷連線</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1056"/>
      <source>Reconnect</source>
      <translation>重新连接</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="892"/>
      <location filename="../src/TDetachedWindow.cpp" line="894"/>
      <source>Close profile</source>
      <extracomment>This is a sub-item of the &quot;Connect&quot; item in the toolbar of a detached Mudlet window.</extracomment>
      <translation>关闭配置文件</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="561"/>
      <source>Show &amp;Toolbar</source>
      <extracomment>This is an item for the toolbar visibility toggle in a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="565"/>
      <source>Show or hide the toolbar</source>
      <extracomment>This explains the &quot;Show Toolbar&quot; action for toolbar visibility in a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="779"/>
      <source>Show Connection Indicators on Tabs</source>
      <extracomment>This is an item in the context menu when clicked on a detached tab.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="861"/>
      <source>Reattach</source>
      <extracomment>This is an item in the toolbar of a detached Mudlet window. It will reattach the profile to the main Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="898"/>
      <location filename="../src/TDetachedWindow.cpp" line="900"/>
      <source>Close Mudlet</source>
      <extracomment>This is a sub-item of the &quot;Connect&quot; item in the toolbar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="910"/>
      <source>Triggers</source>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="911"/>
      <source>Show and edit triggers</source>
      <translation>顯示及編輯觸發</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="915"/>
      <source>Aliases</source>
      <translation>別名</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="916"/>
      <source>Show and edit aliases</source>
      <translation>顯示及編輯別名</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="920"/>
      <source>Timers</source>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="921"/>
      <source>Show and edit timers</source>
      <translation>顯示及編輯時計</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="925"/>
      <source>Buttons</source>
      <translation>按鈕</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="926"/>
      <source>Show and edit easy buttons</source>
      <translation>顯示及編輯按鈕</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="930"/>
      <source>Scripts</source>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="931"/>
      <source>Show and edit scripts</source>
      <translation>顯示及編輯腳本</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="935"/>
      <source>Keys</source>
      <translation>熱鍵</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="936"/>
      <source>Show and edit keys</source>
      <translation>顯示及編輯熱鍵</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="940"/>
      <source>Variables</source>
      <translation>變數</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="941"/>
      <source>Show and edit Lua variables</source>
      <translation>顯示及編輯 Lua 變數</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="947"/>
      <source>Mute</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="410"/>
      <location filename="../src/TDetachedWindow.cpp" line="955"/>
      <location filename="../src/TDetachedWindow.cpp" line="957"/>
      <source>Mute all media</source>
      <extracomment>This is an item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="259"/>
      <source>&amp;Games</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;遊戲</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="262"/>
      <source>&amp;Play</source>
      <extracomment>This is an item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;開啟遊戲</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="264"/>
      <source>Configure connection details of, and make a connection to, game servers.</source>
      <extracomment>This explains the &quot;Play&quot; item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="271"/>
      <source>&amp;Disconnect</source>
      <extracomment>This is an item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;中斷連線</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="273"/>
      <source>Disconnect from the current game server.</source>
      <extracomment>This explains the &quot;Disconnect&quot; item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="278"/>
      <source>&amp;Reconnect</source>
      <extracomment>This is an item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;重新连接</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="280"/>
      <source>Disconnect and then reconnect to the current game server.</source>
      <extracomment>This explains the &quot;Reconnect&quot; item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="294"/>
      <source>Close &amp;Mudlet</source>
      <extracomment>This is an item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="296"/>
      <source>Close the entire Mudlet application</source>
      <extracomment>This explains the &quot;Close Mudlet&quot; item in the &quot;Games&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="302"/>
      <source>&amp;Toolbox</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;工具</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="305"/>
      <source>&amp;Script editor</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;腳本編輯器</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="307"/>
      <source>Opens the Editor for the different types of things that can be scripted by the user.</source>
      <extracomment>This explains the &quot;Script editor&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>打开不同类型事物编辑器以便用户写脚本。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="312"/>
      <source>Show &amp;errors</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="314"/>
      <source>Show errors from scripts that you have running</source>
      <extracomment>This explains the &quot;Show errors&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="319"/>
      <source>Show &amp;map</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="321"/>
      <source>Show or hide the game map.</source>
      <extracomment>This explains the &quot;Show map&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="326"/>
      <source>Compact &amp;input line</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="328"/>
      <source>Hide / show the search area and buttons at the bottom of the screen.</source>
      <extracomment>This explains the &quot;Compact input line&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="334"/>
      <source>&amp;Notepad</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;记事本</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="336"/>
      <source>Opens a free form text editor window for this profile that is saved between sessions.</source>
      <extracomment>This explains the &quot;Notepad&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="341"/>
      <source>&amp;Package manager</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;套件管理工具</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="343"/>
      <source>Install and remove collections of Mudlet lua items (packages).</source>
      <extracomment>This explains the &quot;Package manager&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="348"/>
      <source>Load &amp;replay</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="350"/>
      <source>Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).</source>
      <extracomment>This explains the &quot;Load replay&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>加载之前保存的游戏会话，该会话可用于测试 Mudlet lua 系统(离线!)。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="355"/>
      <source>&amp;Module manager</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;模組管理工具</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="357"/>
      <source>Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).</source>
      <extracomment>This explains the &quot;Module manager&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>安装和移除（可共享和同步的）Mudlet Lua项的合集（模块）。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="362"/>
      <source>Package &amp;exporter</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="364"/>
      <source>Gather and bundle up collections of Mudlet Lua items and other reasources into a module.</source>
      <extracomment>This explains the &quot;Package exporter&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>收集并打包Mudlet Lua项的合集以及其它资源到模块中去。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="369"/>
      <source>Record replay</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="371"/>
      <source>Toggle recording of replays.</source>
      <extracomment>This explains the &quot;Record replay&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="376"/>
      <source>Record log</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="378"/>
      <source>Toggle logging facilities.</source>
      <extracomment>This explains the &quot;Record log&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="383"/>
      <source>Emergency stop</source>
      <extracomment>This is an item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="385"/>
      <source>Toggle all triggers, aliases, timers, etc. on or off</source>
      <extracomment>This explains the &quot;Emergency stop&quot; item in the &quot;Toolbox&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="391"/>
      <source>&amp;Options</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;選項</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="394"/>
      <source>&amp;Preferences</source>
      <extracomment>This is an item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;偏好設定</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="396"/>
      <source>Configure setting for the Mudlet application globally and for the current profile.</source>
      <extracomment>This explains the &quot;Preferences&quot; item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>设定Mudlet应用的全局环境和当前的配置。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="401"/>
      <source>&amp;Timestamps</source>
      <extracomment>This is an item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="403"/>
      <source>Toggle time stamps on the main console.</source>
      <extracomment>This explains the &quot;Timestamps&quot; item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="412"/>
      <source>Mutes all media played.</source>
      <extracomment>This explains the &quot;Mute all media&quot; item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="418"/>
      <location filename="../src/TDetachedWindow.cpp" line="961"/>
      <location filename="../src/TDetachedWindow.cpp" line="963"/>
      <source>Mute sounds from Mudlet (triggers, scripts, etc.)</source>
      <extracomment>This is an item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="420"/>
      <source>Mutes media played by the Lua API and scripts.</source>
      <extracomment>This explains the &quot;Mute sounds from Mudlet (triggers, scripts, etc.)&quot; item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="426"/>
      <location filename="../src/TDetachedWindow.cpp" line="967"/>
      <location filename="../src/TDetachedWindow.cpp" line="969"/>
      <source>Mute sounds from the game (MCMP, MSP)</source>
      <extracomment>This is an item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="428"/>
      <source>Mutes media played by the game (MCMP, MSP).</source>
      <extracomment>This explains the &quot;Mute sounds from the game (MCMP, MSP)&quot; item in the &quot;Options&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="438"/>
      <source>&amp;Fullscreen</source>
      <extracomment>This is an item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="446"/>
      <source>&amp;Multiview</source>
      <extracomment>This is an item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="448"/>
      <source>Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.</source>
      <extracomment>This explains the &quot;Multiview&quot; item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>分割 Mudlet 屏幕以同时显示多个配置文件；加载的配置文件少于两个时禁用。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="475"/>
      <source>Minimize this window</source>
      <extracomment>This explains the &quot;Minimize&quot; item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="481"/>
      <source>&amp;Help</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;說明</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="484"/>
      <source>&amp;API Reference</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;使用手冊</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="486"/>
      <source>Opens the Mudlet manual in your web browser.</source>
      <extracomment>This explains the &quot;API Reference&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="491"/>
      <source>&amp;Video tutorials</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;影片教學</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="493"/>
      <source>Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.</source>
      <extracomment>This explains the &quot;Video tutorials&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>在你的系统网页浏览器中打开（在线）&quot;Mudlet的教学视频&quot;集。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="498"/>
      <source>&amp;Discord</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;Discord</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="500"/>
      <source>Open a link to Discord.</source>
      <extracomment>This explains the &quot;Discord&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="505"/>
      <source>Discord &amp;help channel</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="507"/>
      <source>Open a link to the Mudlet server on Discord.</source>
      <extracomment>This explains the &quot;Discord help channel&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="512"/>
      <source>&amp;Live help chat</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;即時協助</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="514"/>
      <source>Opens a connect to an IRC server (LiberaChat) in your system web-browser.</source>
      <extracomment>This explains the &quot;Live help chat&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="519"/>
      <source>Online &amp;forum</source>
      <extracomment>This is an item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="521"/>
      <source>Opens the (on-line) Mudlet Forum in your system web-browser.</source>
      <extracomment>This explains the &quot;Online forum&quot; item in the &quot;Help&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="527"/>
      <source>&amp;About</source>
      <extracomment>This is the name of a menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;关于</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="530"/>
      <source>About &amp;Mudlet</source>
      <extracomment>This is an item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="532"/>
      <location filename="../src/TDetachedWindow.cpp" line="1064"/>
      <source>About Mudlet version, creators, and license.</source>
      <extracomment>Tooltip for About Mudlet sub-menu item (Used in multiple places - please ensure all have the same translation).
----------
Tooltip for About Mudlet toolbar button (Used in multiple places - please ensure all have the same translation).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="538"/>
      <source>&amp;Check for updates...</source>
      <extracomment>This is an item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="540"/>
      <source>Check for newer versions of Mudlet</source>
      <extracomment>This explains the &quot;Check for updates...&quot; item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="545"/>
      <source>Show &amp;changelog</source>
      <extracomment>This is an item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="547"/>
      <source>Show the changelog for this version</source>
      <extracomment>This explains the &quot;Show changelog&quot; item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="553"/>
      <source>&amp;Report an issue</source>
      <extracomment>This is an item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>&amp;报告此问题</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="555"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <extracomment>This explains the &quot;Report an issue&quot; item in the &quot;About&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>公共测试版将更新的功能更快地送到你手中，你也能帮助我们更快地发现其中的问题。发现了什么奇怪的东西？请尽快告诉我们</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="768"/>
      <location filename="../src/TDetachedWindow.cpp" line="838"/>
      <location filename="../src/TDetachedWindow.cpp" line="1659"/>
      <source>Main Toolbar</source>
      <extracomment>This is a checkable toggle item in the context menu shown when right-clicking a tab in a detached window, to show or hide the toolbar. It appears with a checkmark when the toolbar is visible.
----------
Name of the main toolbar shown in Qt&apos;s built-in toolbar toggle menus and right-click context menus
----------
This is a checkable toggle item in the context menu shown when right-clicking the toolbar in a detached window, to show or hide the toolbar. It appears with a checkmark when the toolbar is visible.</extracomment>
      <translation>主工具栏</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="987"/>
      <source>Open Discord</source>
      <translation>開啟 Discord</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="992"/>
      <source>Mudlet chat</source>
      <translation>Mudlet 闲聊</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="993"/>
      <source>Open a link to the Mudlet server on Discord</source>
      <translation>在 Discord 上打开至 Mudlet 服务器的链接。</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1003"/>
      <source>Map</source>
      <translation>地圖</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1004"/>
      <source>Show/hide the map</source>
      <translation>顯示／隱藏地圖</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1008"/>
      <source>Manual</source>
      <translation>文件</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1009"/>
      <source>Browse reference material and documentation</source>
      <translation>瀏覽參考資料和說明文件</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1013"/>
      <source>Settings</source>
      <translation>設定</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1014"/>
      <source>See and edit profile preferences</source>
      <translation>查看並編輯偏好設定</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1018"/>
      <source>Notepad</source>
      <translation>记事本</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1019"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation>開啟記事本</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1025"/>
      <location filename="../src/TDetachedWindow.cpp" line="1035"/>
      <source>Packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1033"/>
      <source>Package Manager</source>
      <translation>套件管理工具</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1038"/>
      <source>Module Manager</source>
      <translation>模組管理工具</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1042"/>
      <source>Package Exporter</source>
      <translation>包导出器</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1051"/>
      <source>Replay</source>
      <translation>記錄回放</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1057"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation>中斷您與遊戲的連線，並再次連線</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1062"/>
      <source>About</source>
      <translation>关于</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1072"/>
      <source>Full Screen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="441"/>
      <location filename="../src/TDetachedWindow.cpp" line="1073"/>
      <source>Toggle Full Screen View</source>
      <extracomment>This explains the &quot;Fullscreen&quot; item in the &quot;Window&quot; menu in the menubar of a detached Mudlet window.</extracomment>
      <translation>切换全屏显示</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1352"/>
      <source>Connected to %1</source>
      <extracomment>This text will be added to the title of a detached Mudlet window, if it is currently connected. The whole title will be like &quot;Mudlet PROFILENAME (Detached) - Connected to GAMENAME&quot;</extracomment>
      <translation>已连接到 %1</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1355"/>
      <source>Connected</source>
      <extracomment>This text will be part of to the title of a detached Mudlet window, if it is currently connected but we don&apos;t know to where. The whole title will be like &quot;Mudlet PROFILENAME (Detached) - Connected&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1359"/>
      <source>Connecting...</source>
      <extracomment>This text will be part of the title of a detached Mudlet window, if it is about to be connected. The whole title will be like &quot;Mudlet PROFILENAME (Detached) - Connecting...&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1362"/>
      <source>Disconnected</source>
      <extracomment>This text will be part of the title of a detached Mudlet window, if it is not connected. The whole title will be like &quot;Mudlet PROFILENAME (Detached) - Disconnected&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1783"/>
      <source>%1 (Main Window)</source>
      <extracomment>This is an item in list of profiles in the &quot;Window&quot; menu of a detached Mudlet window. %1 is the name of the profile, and it is located not in the detached window, but in Mudlet&apos;s main window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1811"/>
      <source>%1 (Detached)</source>
      <extracomment>This is an item in list of profiles in the &quot;Window&quot; menu of a detached Mudlet window. %1 is the name of the profile, and it is located not in Mudlet&apos;s main window, but in the detached window.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="2800"/>
      <source>Map - %1</source>
      <extracomment>This is to create a new docked mapper widget for a profile in a detached Mudlet window. %1 is the name of the profile.</extracomment>
      <translation>地圖 - %1</translation>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1341"/>
      <source>Mudlet (Detached)</source>
      <extracomment>This is the title of a Mudlet window which was detached from the main Mudlet window, but has no profile loaded.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TDetachedWindow.cpp" line="1368"/>
      <source>Mudlet (%1 profiles) - %2 (Detached)</source>
      <extracomment>This is the title of a Mudlet window which was detached from the main Mudlet window, and has multiple profiles opened in this window. %1 is the number of profiles, %2 is the name of the profile currently shown.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TEasyButtonBar</name>
    <message>
      <location filename="../src/TEasyButtonBar.cpp" line="63"/>
      <source>Easybutton Bar - %1 - %2</source>
      <translation>简单按钮栏 - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TFeatureCallout</name>
    <message>
      <location filename="../src/TFeatureCallout.cpp" line="85"/>
      <source>Got it</source>
      <extracomment>Button that dismisses a balloon pointing out a newly added feature</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>THyperlinkVisibilityManager</name>
    <message>
      <location filename="../src/THyperlinkVisibilityManager.cpp" line="758"/>
      <source>Link hidden</source>
      <extracomment>Screen-reader announcement when an OSC 8 hyperlink is hidden by the visibility manager</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/THyperlinkVisibilityManager.cpp" line="761"/>
      <source>%n link(s) hidden</source>
      <extracomment>Screen-reader announcement when multiple OSC 8 hyperlinks are hidden at once; %n is the count</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/THyperlinkVisibilityManager.cpp" line="794"/>
      <source>Link revealed: %1</source>
      <extracomment>Screen-reader announcement when a previously hidden OSC 8 link is revealed; %1 is the original link text</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TKeySequenceEdit</name>
    <message>
      <location filename="../src/TKeySequenceEdit.cpp" line="125"/>
      <source>No shortcut set</source>
      <extracomment>Accessibility description of a keyboard shortcut editor in the preferences when no shortcut is assigned</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TLuaInterpreter</name>
    <message>
      <location filename="../src/TLuaInterpreterDiscord.cpp" line="348"/>
      <source>Playing %1</source>
      <translation>正在玩 %1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="4409"/>
      <location filename="../src/TLuaInterpreter.cpp" line="4450"/>
      <source>ERROR</source>
      <translation>錯誤</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="5277"/>
      <source>No error message available from Lua</source>
      <translation>沒有來自 Lua 的錯誤訊息</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="4413"/>
      <location filename="../src/TLuaInterpreter.cpp" line="4436"/>
      <source>object</source>
      <extracomment>object is the Mudlet alias/trigger/script, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</extracomment>
      <translation>物件</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="4416"/>
      <location filename="../src/TLuaInterpreter.cpp" line="4439"/>
      <source>function</source>
      <extracomment>function is the Lua function, used in this sample message: object:&lt;Alias1&gt; function:&lt;cure_me&gt;</extracomment>
      <translation>函數</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="5279"/>
      <source>Lua error: %1</source>
      <translation>Lua 錯誤：%1</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="5288"/>
      <source>[ ERROR ] - Cannot find Lua module %1.%2%3%4</source>
      <extracomment>%1 is the name of the module; %2 will be a line-feed inserted to put the next argument on a new line; %3 is the error message from the lua sub-system; %4 can be an additional message about the expected effect (but may be blank).</extracomment>
      <translation>[ 錯誤 ] - 找不到 Lua 模組 %1.%2%3%4</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6067"/>
      <source>Probably will not be able to access Mudlet Lua code.</source>
      <translation>可能将无法访问 Mudlet Lua 代码。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6085"/>
      <source>Some regular expression functions may not be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6092"/>
      <source>Database support will not be available.</source>
      <translation>数据库支持不可用。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6099"/>
      <source>utf8.* Lua functions won&apos;t be available.</source>
      <translation>utf8.* Lua函数不可用。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6105"/>
      <source>yajl.* Lua functions won&apos;t be available.</source>
      <translation>yajl.* Lua函数不可用。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6110"/>
      <source>lpeg.* Lua functions won&apos;t be available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6296"/>
      <source>No error message available from Lua.</source>
      <translation>Lua没有提供可用的错误信息。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6298"/>
      <source>Lua error: %1.</source>
      <translation>Lua 錯誤：%1。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6300"/>
      <source>[ ERROR ] - Cannot load code formatter, indenting functionality won&apos;t be available.</source>
      <translation>[ 錯誤 ] - 無法載入程式碼格式化，縮排功能無法使用。</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6392"/>
      <source>%1 (doesn&apos;t exist)</source>
      <comment>This file doesn&apos;t exist</comment>
      <translation>%1 (不存在)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6397"/>
      <source>%1 (isn&apos;t a file or symlink to a file)</source>
      <translation>%1 (不是一个文件或文件的快捷方式)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6410"/>
      <source>%1 (isn&apos;t a readable file or symlink to a readable file)</source>
      <translation>%1 (不是一个可读文件或文件的快捷方式)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6432"/>
      <source>%1 (couldn&apos;t read file)</source>
      <comment>This file could not be read for some reason (for example, no permission)</comment>
      <translation>%1 (不能读取文件)</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6444"/>
      <source>[ ERROR ] - Couldn&apos;t find, load and successfully run LuaGlobal.lua - your Mudlet is broken!
Tried these locations:
%1</source>
      <translation>[ 错误 ] - 无法找到并成功加载 LuaGlobal.lua - 您的Mudlet已损坏!
已尝试以下位置:
%1</translation>
    </message>
  </context>
  <context>
    <name>TMainConsole</name>
    <message>
      <location filename="../src/TMainConsole.cpp" line="339"/>
      <source>Mudlet MUD Client version: %1%2</source>
      <translation>Mudlet MUD 客户端版本: %1%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="341"/>
      <source>Mudlet, log from %1 profile</source>
      <translation>Mudlet, 日志来自用户%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="412"/>
      <source>Stop logging game output to log file.</source>
      <translation>停止向日志文件继续写入游戏记录.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="299"/>
      <source>Logging has started. Log file is %1</source>
      <translation>记录已开始. 日志保存在%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="257"/>
      <source>logfile</source>
      <extracomment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {one of two copies}).</extracomment>
      <translation>日誌文件</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="309"/>
      <source>Logging has been stopped. Log file is %1</source>
      <translation>记录已停止. 日志保存在%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="387"/>
      <location filename="../src/TMainConsole.cpp" line="410"/>
      <source>&apos;Log session starting at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <extracomment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</extracomment>
      <translation>&apos;日志会话开始于&apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="417"/>
      <source>&apos;Log session ending at &apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</source>
      <extracomment>This is the format argument to QDateTime::toString(...) and needs to follow the rules for that function {literal text must be single quoted} as well as being suitable for the translation locale</extracomment>
      <translation>&apos;日志结束于&apos;hh:mm:ss&apos; on &apos;dddd&apos;, &apos;d&apos; &apos;MMMM&apos; &apos;yyyy&apos;.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="428"/>
      <source>Start logging game output to log file.</source>
      <translation>开始向日志文件写入游戏输出.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="923"/>
      <source>Pre-Map loading(2) report</source>
      <translation>加载地图前 (2) 的报告</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="934"/>
      <source>Loading map(2) at %1 report</source>
      <translation>加载地图中 (2) 的报告, 时间: %1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1452"/>
      <source>User window - %1 - %2</source>
      <translation>用户窗口 - %1 - %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1545"/>
      <source>N:%1 S:%2</source>
      <extracomment>The first argument &apos;N&apos; represents the &apos;N&apos;etwork latency; the second &apos;S&apos; the &apos;S&apos;ystem (processing) time</extracomment>
      <translation>N:%1 S:%2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1552"/>
      <source>&lt;no GA&gt; S:%1</source>
      <extracomment>The argument &apos;S&apos; represents the &apos;S&apos;ystem (processing) time, in this situation the Game Server is not sending &quot;GoAhead&quot; signals so we cannot deduce the network latency...</extracomment>
      <translation>&lt;no GA&gt; S:%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1673"/>
      <source>Pre-Map loading(1) report</source>
      <translation>加载地图前 (1) 的报告</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1691"/>
      <source>Loading map(1) at %1 report</source>
      <translation>加载地图中 (1) 的报告，时间：%1</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1693"/>
      <source>Loading map(1) &quot;%1&quot; at %2 report</source>
      <translation>正在加载地图(1) &quot;%1&quot; 在 %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1737"/>
      <source>Pre-Map importing(1) report</source>
      <translation>导入地图前 (1) 的报告</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1760"/>
      <source>[ ERROR ]  - Map file not found, path and name used was:
%1.</source>
      <translation>[ 錯誤 ] - 找不到地圖文件，使用的路徑和檔案名稱是：%1.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1766"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; was not found).</source>
      <translation>loadMap: 错误的#1参数值 (找不到文件：&quot;%1&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1775"/>
      <source>[ INFO ]  - Map file located and opened, now parsing it...</source>
      <translation>[ 信息 ]  - 已找到并打开地图文件, 开始分析...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1782"/>
      <source>Importing map(1) &quot;%1&quot; at %2 report</source>
      <translation>正在加载地图(1) &quot;%1&quot; 在 %2</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1785"/>
      <source>[ INFO ]  - Map file located but it could not opened, please check permissions on:&quot;%1&quot;.</source>
      <translation>[ 信息 ] - 地图文件已找到，但无法打开，请检查许可权: &quot;%1&quot;。</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1788"/>
      <source>loadMap: bad argument #1 value (filename used: 
&quot;%1&quot; could not be opened for reading).</source>
      <translation>loadMap: 错误的#1参数值 (无法读取文件: &quot;%1&quot;).</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1812"/>
      <source>[ INFO ]  - Map reload request received from system...</source>
      <translation>[ 信息 ]  - 系统收到重新加载地图的请求...</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1817"/>
      <source>[  OK  ]  - ... System Map reload request completed.</source>
      <translation>[ 完成 ] - ... 系统已完成重新加载地图的请求.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="1819"/>
      <source>[ WARN ]  - ... System Map reload request failed.</source>
      <translation>[警告] - ... 系统重新加载地图失败.</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2143"/>
      <source>+--------------------------------------------------------------+
|                      system statistics                       |
+--------------------------------------------------------------+</source>
      <comment>Header for the system&apos;s statistics information displayed in the console, it is 64 &apos;narrow&apos; characters wide</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2153"/>
      <source>GMCP events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>GMCP 事件:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2158"/>
      <source>ATCP events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>ATCP 事件:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2163"/>
      <source>Channel102 events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>频道102 事件：</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2168"/>
      <source>MXP events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2173"/>
      <source>MSSP events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>MSSP 事件:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2179"/>
      <source>MSDP events:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>MSDP 事件:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2192"/>
      <source>Telnet Options:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2196"/>
      <source>Trigger Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>触发器报告:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2201"/>
      <source>Timer Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>计时器报告:</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2207"/>
      <source>Alias Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>别名报告：</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2212"/>
      <source>Keybinding Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>按键绑定报告：</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2217"/>
      <source>Script Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation>脚本报告：</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2222"/>
      <source>Gif Report:</source>
      <extracomment>Heading for the system&apos;s statistics information displayed in the console</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2272"/>
      <source>Save profile?</source>
      <translation>保存配置？</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2272"/>
      <source>Do you want to save the profile %1?</source>
      <translation>是否儲存使用者設定文件 %1？</translation>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2286"/>
      <source>Could not save profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2286"/>
      <source>Sorry, could not save your profile as &quot;%1&quot; - got the following error: &quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2295"/>
      <source>Could not save map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMainConsole.cpp" line="2296"/>
      <source>Sorry, could not save the map. Would you like to retry or close without saving the map?</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMap</name>
    <message>
      <location filename="../src/TMap.cpp" line="617"/>
      <source>[ INFO ] - CONVERTING: old style label, areaID:%1 labelID:%2.</source>
      <translation>[ 信息 ]  - 转换: 旧版标签, 区域号: %1 标签号: %2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="620"/>
      <source>[ INFO ] - Converting old style label id: %1.</source>
      <translation>[ INFO ] - 正在转换旧版标签, 标签号: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="625"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label in area with id: %1,  label id is: %2.</source>
      <translation>[警告] - 转换: 无法转换的旧版标签, 该标签位于区域: %1, 标签号: %2.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="628"/>
      <source>[ WARN ] - CONVERTING: cannot convert old style label with id: %1.</source>
      <translation>[警告] - 转换: 无法转换的旧版标签, 标签号: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="655"/>
      <source>[  OK  ]  - Auditing of map completed (%1s). Enjoy your game...</source>
      <translation>[ 完成 ]  - 审核地图完成(%1s). 祝游戏愉快...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="88"/>
      <source>Default Area</source>
      <translation>預設區域</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="89"/>
      <source>Unnamed Area</source>
      <translation>未命名的地区</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="584"/>
      <source>[ INFO ]  - Map audit starting...</source>
      <translation>[ 信息 ]  - 开始地图审核...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1125"/>
      <source>[ ERROR ] - The format version &quot;%1&quot; you are trying to save the map with is too old
for this version of Mudlet. Supported are only formats from version %2.</source>
      <extracomment>Shown when a map save asks for a format version older than this Mudlet can write. %1 is the version asked for, %2 the oldest one supported.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1581"/>
      <source>[ INFO ]  - You might wish to donate THIS map file to the Mudlet Museum!
There is so much data that it DOES NOT have that you could be
better off starting again...</source>
      <translation>[ INFO ] - 也许你应该把这个地图文件捐赠给Mudlet博物馆!
这个地图文件已经缺少了太多数据, 你最好还是重新做一个...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1647"/>
      <source>[ ALERT ] - Failed to load a Mudlet JSON Map file, reason:
%1; the file is:
&quot;%2&quot;.</source>
      <translation>[警报] - 未能加载Mudlet JSON 地图文件， 原因：
%1; 文件:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1653"/>
      <source>[ INFO ]  - Ignoring this map file.</source>
      <translation>[信息] - 忽略此地图文件。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1831"/>
      <source>[ INFO ]  - Default (reset) area (for rooms that have not been assigned to an
area) not found, adding reserved -1 id.</source>
      <translation>[ INFO ] - 找不到默认 (重置) 区域 (对某些尚未指定区域的房间) , 添加保留区域号-1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1922"/>
      <source>[ INFO ]  - Successfully read the map file (%1s), checking some
consistency details...</source>
      <translation>[ INFO ] - 读取地图文件成功 (%1s) , 正在检查某些细节的一致性...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2436"/>
      <source>Map issues</source>
      <translation>地圖問題</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2443"/>
      <source>Area issues</source>
      <translation>区域问题</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2449"/>
      <source>Area id: %1 &quot;%2&quot;</source>
      <translation>区域id: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2451"/>
      <source>Area id: %1</source>
      <translation>区域编号: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2460"/>
      <source>Room issues</source>
      <translation>房间问题</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2467"/>
      <source>Room id: %1 &quot;%2&quot;</source>
      <translation>房间id: %1 &quot;%2&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2469"/>
      <source>Room id: %1</source>
      <translation>房间编号: %1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2479"/>
      <source>End of report</source>
      <translation>报告结束</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2485"/>
      <source>[ ALERT ] - At least one thing was detected during that last map operation
that it is recommended that you review the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[警告] - 在最近的地图操作中至少发现了一个问题
建议你检查最近的报告文件:
&quot;%1&quot;
- 找到 (最近一次的) 带有下面标题的报告:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2493"/>
      <source>[ INFO ]  - The equivalent to the above information about that last map
operation has been saved for review as the most recent report in
the file:
&quot;%1&quot;
- look for the (last) report with the title:
&quot;%2&quot;.</source>
      <translation>[ INFO ] - 关于上次地图操作的信息己保存在最近的报告文件中:
&quot;%1&quot;
- 找到 (最近一次的) 带有下面标题的报告:
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2515"/>
      <source>[ WARN ]  - Attempt made to download an XML map when one has already been
requested or is being imported from a local file - wait for that
operation to complete (if it cannot be canceled) before retrying!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2524"/>
      <source>[ WARN ]  - Attempt made to download an XML map while a map import or
export is already in progress - wait for that operation to complete
before retrying!</source>
      <extracomment>Shown in the main console when a map download is refused</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2545"/>
      <source>[ WARN ]  - Attempt made to download an XML from an invalid URL.  The URL was:
%1
and the error message (may contain technical details) was:&quot;%2&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2559"/>
      <source>[ ERROR ] - Unable to use or create directory to store map.
Please check that you have permissions/access to:
&quot;%1&quot;
and there is enough space. The download operation has failed.</source>
      <translation>[ 错误 ] - 无法使用或创建目录来保存地图.
请检查您的访问权限：
&quot;%1&quot;
并且确认空间足够。下载失败。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2585"/>
      <source>[ INFO ]  - Map download initiated, please wait...</source>
      <translation>[ INFO ] - 已經開始下載地圖，請稍候…</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2627"/>
      <source>loadMap: unable to perform request, a map import or export is
already in progress.</source>
      <extracomment>Error returned by the loadMap() Lua function</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2631"/>
      <source>[ WARN ]  - Attempt made to import an XML map while a map import or
export is already in progress - wait for that operation to complete
before retrying!</source>
      <extracomment>Shown in the main console when a map import is refused</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2738"/>
      <source>[ ERROR ] - Map download encountered an error:
%1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2788"/>
      <source>[ ALERT ] - Map download failed, unable to save destination file:
%1
reason: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3095"/>
      <source>Map JSON export</source>
      <extracomment>This is a title of a progress window.</extracomment>
      <translation>地图 JSON 导出</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3315"/>
      <source>Map JSON import</source>
      <extracomment>This is a title of a progress window.</extracomment>
      <translation>地图 JSON 导入</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3096"/>
      <location filename="../src/TMap.cpp" line="3546"/>
      <source>Exporting JSON map data from %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>从 %1 中导出 JSON 地图数据
%3的 区域： %2    %5 的房间：%4    %7的标签： %6...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3229"/>
      <source>Exporting JSON map file from %1 - writing data to file:
%2 ...</source>
      <translation>正在从 %1 导出JSON 地图文件 - 将数据写入文件：
%2...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3256"/>
      <source>import or export already in progress</source>
      <translation>导入或导出正在进行中</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3262"/>
      <source>could not open file</source>
      <translation>无法打开文件</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3271"/>
      <source>could not parse file, reason: &quot;%1&quot; at offset %2</source>
      <translation>无法解析文件，原因: &quot;%1&quot; at offset %2</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3277"/>
      <source>empty Json file, no map data detected</source>
      <translation>空的 Json 文件，未检测到地图数据</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3291"/>
      <source>invalid format version &quot;%1&quot; detected</source>
      <translation>检测到无效格式版本 &quot;%1&quot;</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3296"/>
      <source>no format version detected</source>
      <translation>未检测到格式版本</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3300"/>
      <source>no areas detected</source>
      <translation>未检测到区域</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3415"/>
      <source>aborted by user</source>
      <translation>被用户终止</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="3316"/>
      <location filename="../src/TMap.cpp" line="3556"/>
      <source>Importing JSON map data to %1
Areas: %2 of: %3   Rooms: %4 of: %5   Labels: %6 of: %7...</source>
      <translation>从 %1 中导入 JSON 地图数据
%3的 区域： %2    %5 的房间：%4    %7的标签： %6...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="163"/>
      <source>[MAP ERROR:] %1</source>
      <extracomment>Used to print a map error in the Errors console in the Editor, %1 is the message text and a line-feed is also appended.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="186"/>
      <source>Can not set room with RoomID %1 to AreaID %2. Room does not exist!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="196"/>
      <source>Can not set room with RoomID %1 to AreaID %2. Area does not exist!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1116"/>
      <source>[ ERROR ] - The format version &quot;%1&quot; you are trying to save the map with is too new
for this version of Mudlet. Supported are only formats up to version %2.</source>
      <translation>[ 错误 ] - 您尝试保存的地图格式 &quot;%1&quot; 对于此版本的Mudlet而言太新
。 支持的格式仅为版本 &quot;%2&quot; 。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1141"/>
      <source>[ ALERT ] - Saving map in format version &quot;%1&quot; that is different than &quot;%2&quot; which
it was loaded as. This may be an issue if you want to share the resulting
map with others relying on the original format.</source>
      <translation>[警报] - 保存地图的格式版本 &quot;%1&quot; 不同于已加载的 &quot;%2&quot; 的格式。 如果您想要与依赖原始格式的其他人分享结果的
地图，这可能是一个问题。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1151"/>
      <source>[ WARN ]  - Saving map in format version &quot;%1&quot; different from the
recommended map version %2 for this version of Mudlet.</source>
      <translation>[注意] - 保存地图的格式版本 &quot;%1&quot; 与Mudlet这个版本的
推荐地图版本 %2 不同。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1521"/>
      <location filename="../src/TMap.cpp" line="1965"/>
      <source>[ ERROR ] - Unable to open map file for reading: &quot;%1&quot;!</source>
      <translation>[错误] - 无法打开(读取) 地图文件: &quot;%1&quot;!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1543"/>
      <source>[ ALERT ] - File does not seem to be a Mudlet Map file. The part that indicates
its format version seems to be &quot;%1&quot; and that doesn&apos;t make sense. The file is:
&quot;%2&quot;.</source>
      <translation>[警报] - 文件似乎不是一个Mudlet地图文件。其格式版本的部分似乎是 &quot;%1&quot; ，这是没有意义的。该文件是。
&quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1558"/>
      <source>[ ALERT ] - Map file is too new. Its format version &quot;%1&quot; is higher than this version of
Mudlet can handle (%2)! The file is:
&quot;%3&quot;.</source>
      <translation>[ 警告 ] - 地图文件太新了. 它的格式版本 &quot;%1&quot; 高于这个版本的
Mudlet 可以处理的 (%2)！该文件是:
&quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1565"/>
      <source>[ INFO ]  - You will need to update your Mudlet to read the map file.</source>
      <translation>[信息] - 你需要更新你的Mudlet来读取地图文件。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1574"/>
      <source>[ ALERT ] - Map file is really old. Its format version &quot;%1&quot; is so ancient that
this version of Mudlet may not gain enough information from
it but it will try! The file is: &quot;%2&quot;.</source>
      <translation>[ 警告 ] - 地图文件已经太旧, 其过时的保存格式 (%1) 导致Mudlet无法从中获得足够的信息, 但Mudlet仍会尝试读取! 这个文件是: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1588"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File:
&quot;%2&quot;,
please wait...</source>
      <translation>[ 信息 ]  - 正在读取地图文件. 格式版本: %1.  文件:
&quot;%2&quot;,
请稍等...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1593"/>
      <source>[ INFO ]  - Reading map. Format version: %1. File: &quot;%2&quot;.</source>
      <translation>[ 信息 ]  - 读取地图文件中. 版本: %1. 文件: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1981"/>
      <source>[ INFO ]  - Checking map file &quot;%1&quot;, format version &quot;%2&quot;.</source>
      <translation>[ 信息 ] - 检查地图文件 &quot;%1&quot;, 版本: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2592"/>
      <source>Downloading map file for use in %1...</source>
      <extracomment>%1 is the name of the current Mudlet profile</extracomment>
      <translation>正在下载地图文件 %1...</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2881"/>
      <location filename="../src/TMap.cpp" line="3105"/>
      <location filename="../src/TMap.cpp" line="3325"/>
      <source>Abort</source>
      <translation>中止</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="1550"/>
      <source>[ INFO ]  - Ignoring this unlikely map file.</source>
      <translation>[信息] - 忽略此地图文件。</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2594"/>
      <source>Map download</source>
      <extracomment>This is a title of a progress window.</extracomment>
      <translation>下載地圖</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2611"/>
      <source>loadMap: unable to perform request, a map is already being downloaded or
imported at user request.</source>
      <translation>loadMap: 无法执行请求, 地图已在下载中或已被用户导入.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2659"/>
      <source>Importing XML map file for use in %1...</source>
      <translation>正載匯入用於 %1 的 XML 地圖檔案……</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2659"/>
      <source>Map import</source>
      <extracomment>This is a title of a progress window.</extracomment>
      <translation>匯入地圖</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2680"/>
      <location filename="../src/TMap.cpp" line="2687"/>
      <source>loadMap: failure to import XML map file, further information may be available
in main console!</source>
      <translation>loadMap: 导入XML地图文件失败, 请前往主控制台查看更多信息!</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2722"/>
      <source>[ ALERT ] - Map download was canceled, on user&apos;s request.</source>
      <translation>[警告] - 根据用户请求取消下载地图</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2775"/>
      <source>[ ALERT ] - Map download failed, unable to open destination file:
%1.</source>
      <translation>[警告] - 下载地图失败, 无法打开目标文件: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2782"/>
      <source>[ ALERT ] - Map download failed, unable to write destination file:
%1.</source>
      <translation>[警告] - 下载地图失败, 无法写入目标文件: %1.</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2801"/>
      <source>[ INFO ]  - ... map downloaded and stored, now parsing it...</source>
      <translation>[ INFO ] - …地圖下载并存储完毕，開始分析…</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2837"/>
      <source>[ ERROR ] - Map download problem, failure in parsing destination file:
%1.</source>
      <translation>[錯誤] - 地圖下載出現錯誤，分析目標檔案失敗：%1</translation>
    </message>
    <message>
      <location filename="../src/TMap.cpp" line="2817"/>
      <source>[ ERROR ] - Map download problem, unable to read destination file:
%1.</source>
      <translation>[ 錯誤 ] - 地图下载出现错误, 无法读取目标文件: %1.</translation>
    </message>
  </context>
  <context>
    <name>TMapView</name>
    <message>
      <location filename="../src/TMapView.cpp" line="98"/>
      <source>Go up one z-level</source>
      <extracomment>Tooltip for z-level up button in secondary map view</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMapView.cpp" line="107"/>
      <source>Go down one z-level</source>
      <extracomment>Tooltip for z-level down button in secondary map view</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMapView.cpp" line="111"/>
      <source>Area:</source>
      <extracomment>Label for area selection combobox in secondary map view</extracomment>
      <translation>区域:</translation>
    </message>
  </context>
  <context>
    <name>TMapViewManager</name>
    <message>
      <location filename="../src/TMapViewManager.cpp" line="59"/>
      <source>Map View %1 - %2</source>
      <extracomment>Title for a secondary map view window, %1 is the view number, %2 is the profile name</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TMedia</name>
    <message>
      <location filename="../src/TMedia.cpp" line="380"/>
      <source>fades</source>
      <extracomment>This word is part of a sentence like &quot;Music fades&quot; when the music is about to stop.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1405"/>
      <source>Too many stopped media players. Purging stopped players.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1413"/>
      <source>Too many stopped media players. Removed oldest active player.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1508"/>
      <source>Maximum allowed active media players reached for media type. Cannot play additional media.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="682"/>
      <location filename="../src/TMedia.cpp" line="1692"/>
      <source>stops</source>
      <extracomment>This word is part of a sentence like &quot;Music stops&quot; when the music is about to stop.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1267"/>
      <source>Media error: %1</source>
      <extracomment>%1 is the media backend&apos;s own description of what went wrong, e.g. &quot;Failed to load media&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1779"/>
      <source>plays</source>
      <extracomment>This word is part of a sentence like &quot;Music plays&quot; when the music is starting to play.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="1803"/>
      <source>pauses</source>
      <extracomment>This word is part of a sentence like &quot;Music pauses&quot; when the music stops playing for a while.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="2505"/>
      <source>music</source>
      <extracomment>This word is part of a sentence like &quot;Music stops&quot; when Mudlet handles a piece of music.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="2507"/>
      <source>video</source>
      <extracomment>This word is part of a sentence like &quot;Video stops&quot; when Mudlet handles a video.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TMedia.cpp" line="2509"/>
      <source>sound</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>TRoom</name>
    <message>
      <location filename="../src/TRoom.cpp" line="87"/>
      <location filename="../src/TRoom.cpp" line="1105"/>
      <source>North</source>
      <translation>北</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="89"/>
      <source>North-east</source>
      <translation>东北</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="91"/>
      <source>North-west</source>
      <translation>西北</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="93"/>
      <location filename="../src/TRoom.cpp" line="1147"/>
      <source>South</source>
      <translation>南</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="95"/>
      <source>South-east</source>
      <translation>东南</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="97"/>
      <source>South-west</source>
      <translation>西南</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="99"/>
      <location filename="../src/TRoom.cpp" line="1189"/>
      <source>East</source>
      <translation>东</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="101"/>
      <location filename="../src/TRoom.cpp" line="1203"/>
      <source>West</source>
      <translation>西</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="103"/>
      <location filename="../src/TRoom.cpp" line="1217"/>
      <source>Up</source>
      <translation>上</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="105"/>
      <location filename="../src/TRoom.cpp" line="1231"/>
      <source>Down</source>
      <translation>下</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="107"/>
      <location filename="../src/TRoom.cpp" line="1245"/>
      <source>In</source>
      <translation>入口</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="109"/>
      <location filename="../src/TRoom.cpp" line="1259"/>
      <source>Out</source>
      <translation>出口</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="111"/>
      <source>Other</source>
      <translation>其他</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="113"/>
      <source>Unknown</source>
      <translation>未知</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1119"/>
      <source>Northeast</source>
      <translation>东北</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1133"/>
      <source>Northwest</source>
      <translation>西北</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1161"/>
      <source>Southeast</source>
      <translation>东南</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1175"/>
      <source>Southwest</source>
      <translation>西南</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1281"/>
      <source>[ WARN ]  - In room ID: %1 removing invalid (special) exit to %2 (with no name!)</source>
      <extracomment>%1 is the room ID, %2 is the destination room ID</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1294"/>
      <source>[ INFO ]  - In room with ID: %1 correcting special exit &quot;%2&quot; that was to room with an exit to invalid room: %3 to now go to: %4.</source>
      <extracomment>%1 is the room ID, %2 is the exit name, %3 is the old destination room ID, %4 is the new destination room ID</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1325"/>
      <source>[ WARN ]  - Room with ID: %1 has a special exit &quot;%2&quot; with an exit to: %3 but that room does not exist.  The exit will be removed (but the destination room ID will be stored in the room user data under a key: &quot;%4&quot;).</source>
      <extracomment>%1 is the room ID, %2 is the exit name, %3 is the destination room ID, %4 is the audit key</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1369"/>
      <source>[ INFO ]  - In room with ID: %1 special exit &quot;%2&quot; that was to room with an invalid ID: %3 that does not exist.  The exit will be removed (the bad destination room ID will be stored in the room user data under a key: &quot;%4&quot;).</source>
      <extracomment>%1 is the room ID, %2 is the exit name, %3 is the invalid destination room ID, %4 is the audit key</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1422"/>
      <source>[ INFO ]  - In room with ID: %1 found one or more surplus door items that were removed: %2.</source>
      <extracomment>%1 is the room ID, %2 is a list of door items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1439"/>
      <source>[ INFO ]  - In room with ID: %1 found one or more surplus weight items that were removed: %2.</source>
      <extracomment>%1 is the room ID, %2 is a list of weight items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1456"/>
      <source>[ INFO ]  - In room with ID: %1 found one or more surplus exit lock items that were removed: %2.</source>
      <extracomment>%1 is the room ID, %2 is a list of exit lock items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1536"/>
      <source>[ INFO ]  - In room with ID: %1 found one or more surplus custom line elements that were removed: %2.</source>
      <extracomment>%1 is the room ID, %2 is a list of custom line elements</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1563"/>
      <source>[ INFO ]  - In room with ID: %1 correcting exit &quot;%2&quot; that was to room with an exit to invalid room: %3 to now go to: %4.</source>
      <extracomment>%1 is the room ID, %2 is the exit direction, %3 is the old destination room ID, %4 is the new destination room ID</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1582"/>
      <source>[ WARN ]  - Room with ID: %1 has an exit &quot;%2&quot; to: %3 but that room does not exist.  The exit will be removed (but the destination room ID will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <extracomment>%1 is the room ID, %2 is the exit direction, %3 is the destination room ID that doesn&apos;t exist, %4 is the audit key</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1630"/>
      <source>[ ALERT ] - Room with ID: %1 has an exit &quot;%2&quot; to: %3 but also has a stub exit in the same direction!  As a real exit precludes a stub, the latter will be removed.</source>
      <extracomment>%1 is the room ID, %2 is the exit direction, %3 is the destination room ID</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1688"/>
      <source>[ INFO ]  - In room with ID: %1 exit &quot;%2&quot; that was to room with an invalid ID: %3 that does not exist.  The exit will be removed (the bad destination room ID will be stored in the room user data under a key: &quot;%4&quot;) and the exit will be turned into a stub.</source>
      <extracomment>%1 is the room ID, %2 is the exit direction, %3 is the invalid destination room ID, %4 is the audit key</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1406"/>
      <source>%1 {none}</source>
      <translation>%1 {none}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="240"/>
      <source>Cannot set exit stub in given direction in RoomID %1. There is already an exit there!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="361"/>
      <source>Requested AreaID %1 did not exist and could not be created. Note: Area numbers must be greater than zero!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="384"/>
      <source>When setting the Area for RoomID %1 it did not have a current area, this is unexpected but not a problem!</source>
      <extracomment>Although this is reported as an error it is not a problem</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1409"/>
      <source>%1 (open)</source>
      <translation>%1 (开着的)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1412"/>
      <source>%1 (closed)</source>
      <translation>%1 (关上的)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1415"/>
      <source>%1 (locked)</source>
      <translation>%1 (锁住的)</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1418"/>
      <source>%1 {invalid}</source>
      <translation>%1 {invalid}</translation>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1708"/>
      <source>It had a weight, this is recorded as user data with key: &quot;%1&quot;.</source>
      <extracomment>%1 is the audit key for the weight</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoom.cpp" line="1718"/>
      <source>[ WARN ]  - There was a custom exit line associated with the invalid exit but it has not been possible to salvage this, it has been lost!</source>
      <translation>[警告] - 有一个自定义路径线关联了非法出口, 该路径线没有任何用途, 已经被删除!</translation>
    </message>
  </context>
  <context>
    <name>TRoomDB</name>
    <message>
      <location filename="../src/TRoomDB.cpp" line="668"/>
      <source>[ WARN ]  - Problem with data structure associated with room id: %1 - that
room&apos;s data has been lost so the id is now being deleted.  This
suggests serious problems with the currently running version of
Mudlet - is your system running out of memory?</source>
      <translation>[警告] - 与房间编号: %1 相关的数据结构出现了问题 - 该
房间的数据已丢失, 正在删除房间编号. 这
表明当前运行的
Mudlet 版本存在严重问题-您的系统内存不足吗?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="676"/>
      <source>[ WARN ]  - Problem with data structure associated with this room.  The room&apos;s data has been lost so the id is now being deleted.  This suggests serious problems with the currently running version of Mudlet - is your system running out of memory?</source>
      <translation>[警告] - 与房间编号:  相关的数据结构出现了问题 - 该房间的数据已丢失, 正在删除房间编号. 这表明当前运行的Mudlet 版本存在严重问题-您的系统内存不足吗?</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="725"/>
      <source>[ ALERT ] - Area with id: %1 expected but not found, will be created.</source>
      <translation>[警告] - 区域: %1没有找到，将会被创建。</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="728"/>
      <source>[ ALERT ] - Area with this id expected but not found, will be created.</source>
      <translation>[警告] - 与此编号有关的区域没有找到, 将会被创建.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="757"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
Look for further messages related to the rooms that are supposed
to be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="773"/>
      <source>[ INFO ]  - The missing area(s) are now called:
(ID) ==&gt; &quot;name&quot;</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation>
        <numerusform>[信息] - 缺少的区域现在被表示为：
(编号)==&gt; &quot;区域名字&quot;</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="808"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1)
in map, now working out what new id numbers to use...</source>
      <translation>[警告] - 在地图中发现了(%1) 个无效的房间编号(小于+1且不等于保留编号-1)
, 正在计算用于替换的新编号...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="820"/>
      <source>[ INFO ]  - The renumbered area ids will be:
Old ==&gt; New</source>
      <translation>[信息] - 重置的区域编号将表示为：
旧编号==&gt; 新编号</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="838"/>
      <source>[ INFO ]  - The area with this bad id was renumbered to: %1.</source>
      <translation>[ 信息 ]  - 无效的区域编号已重置为: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="839"/>
      <source>[ INFO ]  - This area was renumbered from the bad id: %1.</source>
      <translation>[ 信息 ]  - 该区域是由无效区域: %1重新编号形成的.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="876"/>
      <location filename="../src/TRoomDB.cpp" line="879"/>
      <source>[ INFO ]  - Area id numbering is satisfactory.</source>
      <translation>[ 信息 ]  - 区域编号是符合要求的.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="887"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map, now working
out what new id numbers to use.</source>
      <translation>[警告] - 在地图中发现了(%1) 个无效的房间编号(小于+1)
, 正在计算用于替换的新编号...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="899"/>
      <source>[ INFO ]  - The renumbered rooms will be:</source>
      <translation>[ 信息 ]  - 重新编号的房间将会是:</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1231"/>
      <source>[  OK  ]  - The changes made are:
(ID) &quot;old name&quot; ==&gt; &quot;new name&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="813"/>
      <source>[ ALERT ] - Bad, (less than +1 and not the reserved -1) area ids found (count: %1) in map!  Look for further messages related to this for each affected area ...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="97"/>
      <source>Room not created. RoomID %1 is not allowed as room numbers must be greater than zero!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="548"/>
      <source>Area not added. An area with AreaID %1 already exists!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="566"/>
      <source>Area not added. An unnamed area (empty area name) is (no longer) permitted!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="570"/>
      <source>Area not added. An area called &quot;%1&quot; already exists!</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/TRoomDB.cpp" line="764"/>
      <source>[ ALERT ] - %n area(s) detected as missing in map: adding it/them in.
Look for further messages related to the rooms that is/are supposed to
be in this/these area(s)...</source>
      <comment>Making use of %n to allow quantity dependent message form 8-) !</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="892"/>
      <source>[ ALERT ] - Bad, (less than +1) room ids found (count: %1) in map!  Look for further messages related to this for each affected room ...</source>
      <translation>[ 警告 ] - 在地图中发现了(%1) 个无效的房间编号(小于1) , 请留意针对这些无效房间的更多消息...</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="915"/>
      <source>[ INFO ]  - This room with the bad id was renumbered to: %1.</source>
      <translation>[ 信息 ]  - 无效的房间编号已重置为: %1.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="916"/>
      <source>[ INFO ]  - This room was renumbered from the bad id: %1.</source>
      <translation>[ 信息 ]  - 该房间是由无效房间: %1重新编号形成的.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="952"/>
      <location filename="../src/TRoomDB.cpp" line="955"/>
      <source>[ INFO ]  - Room id numbering is satisfactory.</source>
      <translation>[ 信息 ]  - 房间编号是符合要求的.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="975"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ 信息 ]  - 在房间: %1中发现了重复的无效出口标识, 已清理该异常.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="980"/>
      <source>[ INFO ]  - Duplicate exit stub identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ 信息 ]  - 在房间中发现了重复的无效出口标识, 这是一个异常现象, 但是已被迅速地清理了.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="989"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room id: %1, this is an
anomaly but has been cleaned up easily.</source>
      <translation>[ 信息 ]  - 在房间: %1中发现了重复的出口锁标识, 这是一个
异常现象, 但是已被迅速地清理了.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="994"/>
      <source>[ INFO ]  - Duplicate exit lock identifiers found in room, this is an anomaly but has been cleaned up easily.</source>
      <translation>[ 信息 ]  - 在房间中发现了重复的无效出口标识, 这是一个异常现象, 但是已被迅速地清理了.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1067"/>
      <source>[ INFO ]  - This room claims to be in area id: %1, but that did not have a record of it.  The area has been updated to include this room.</source>
      <translation>[ 信息 ]  - 该房间应属于区域: %1，但是后者并没有它的记录. 为了添加该房间, 已对区域进行更新.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1073"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 rooms missing from those it
should be recording as possessing, they were:
%3
they have been added.</source>
      <translation>[ 信息 ]  - 在被记录为属于区域: %1的房间
中有 %2 个房间已丢失, 它们是:
%3
它们已经被增加.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1081"/>
      <source>[ INFO ]  - In this area there were %1 rooms missing from those it should be recorded as possessing.  They are: %2.  They have been added.</source>
      <translation>[ 信息 ]  - 在被记录为属于该区域的房间中有 %1 个房间已丢失, 它们是: %2 它们已经被增加.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1102"/>
      <source>[ INFO ]  - This room was claimed by area id: %1, but it does not belong there.  The area has been updated to not include this room.</source>
      <translation>[ 信息 ]  - 区域: %1声称拥有该房间, 但是后者并不属于那里. 为了排除后者, 已对区域进行更新.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1108"/>
      <source>[ INFO ]  - In area with id: %1 there were %2 extra rooms compared to those it
should be recording as possessing, they were:
%3
they have been removed.</source>
      <translation>[ 信息 ]  - 在区域: %1中
中有 %2 个额外的没有被记录为属于该区域的房间, 它们是:
%3
它们已经被移除.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1116"/>
      <source>[ INFO ]  - In this area there were %1 extra rooms that it should not be recorded as possessing.  They were: %2.  They have been removed.</source>
      <translation>[ 信息 ]  - 在该区域中有 %1 个额外的房间被记录为不属于该区域, 它们是: %2 它们已经被移除.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1227"/>
      <source>It has been detected that &quot;_###&quot; form suffixes have already been used, for simplicity in the renaming algorithm these will have been removed and possibly changed as Mudlet sorts this matter out, if a number assigned in this way &lt;b&gt;is&lt;/b&gt; important to you, you can change it back, provided you rename the area that has been allocated the suffix that was wanted first...!&lt;/p&gt;</source>
      <translation>检测到&quot;_###&quot;样式的后缀已被使用，为简化重命名算法，这些后缀将被删除，在Mudlet解决这个问题时，这种方式会改变。如果这种方式分配的号码对你很&lt;b&gt;重要&lt;/b&gt;，只要能用得上，你可以把它改回来。。。!&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1238"/>
      <source>&lt;nothing&gt;</source>
      <translation>&lt;无&gt;</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1241"/>
      <source>[ INFO ]  - Area name changed to prevent duplicates or unnamed ones; old name: &quot;%1&quot;, new name: &quot;%2&quot;.</source>
      <translation>[ 信息 ]  - 区域名称更改以防止重复或无效命名；原区域名称： &quot;%1&quot;，新区域名称： &quot;%2&quot;。</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1250"/>
      <source>[ ALERT ] - Empty and duplicate area names detected in Map file!</source>
      <translation>[ 警告 ] - 在地圖文件中偵測到空的和重複的區域名稱！</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1251"/>
      <source>[ INFO ]  - Mudlet had previously allowed the map to have more than one area
with the same or no name. To resolve these cases, an area without a name
here (or created in the future) will automatically be assigned the name &quot;%1&quot;.
Duplicated area names will cause all but the first encountered one to gain a
&quot;_###&quot; style suffix.
%2</source>
      <translation>[ 信息 ] - Mudlet过去曾允许地图上有一个以上的区域
具有相同区域名或没有区域名。为了解决这些情况，一个没有名字的区域
在这里（或以后创建的）将自动被赋予 &quot;%1&quot; 的名字。
重复的区域名称将导致除第一个遇到的区域外的所有区域获得
&quot;_###&quot; 的后缀。
%2</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1260"/>
      <source>[ ALERT ] - Duplicate area names detected in the Map file!</source>
      <translation>[ 警告 ] - 在地圖文件中偵測到重覆的區域名稱！</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1261"/>
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
      <translation>[信息] - 过去, 由于没有考虑某些情况，Mudlet允许
地图中存在多个具有相同名字的区域.
这会让事情变得混乱, 现在已经不允许这么做了.
  重复的区域名称将导致除了第一个被处理的区域之外所有的
区域名称都以 &quot;_###&quot; 式的后缀结尾, 其中 &quot;###&quot; 是一个递增
数字; 或许你想改变这些后缀, 试试把后缀改成
一 &quot;(子区域名称)&quot; 至于具体怎么做, 完全取决于你,
只是之后你就不能把一个区域的名称设置成
与另一个已存在的区域相同的名称了.
  如果有多个没有名字的区域, 除了第一个区域之外所有的
区域名称也都以这样的后缀结尾.
%1)</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1276"/>
      <source>[ ALERT ] - An empty area name was detected in the Map file!</source>
      <translation>[ 警告 ] - 在地圖文件中偵測到空的區域名稱！</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1279"/>
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
      <translation>[ 完成 ] - 过去, 由于没有考虑某些情况，Mudlet允许
地图中存在未命名区域.
这会让事情变得混乱, 现在已经不允许这么做了.
  为了解决这个问题, 如果一个区域没有名字(或者等以后
再命名)系统将自动分配名称 &quot;%1&quot;.
  如果这样的未命名区域有很多个, 那么除了第一个被处理的区域之外所有的
区域名称都以 &quot;_###&quot; 式的后缀结尾, 其中 &quot;###&quot; 是一个递增
数字; 或许你想改变这些后缀, 试试把后缀改成
一 &quot;(子区域名称)&quot; 至于具体怎么做, 完全取决于你,
只是之后你就不能把一个区域的名称设置成
与另一个已存在的区域相同的名称了.</translation>
    </message>
    <message>
      <location filename="../src/TRoomDB.cpp" line="1303"/>
      <source>[ INFO ]  - Default (reset) area name (for rooms that have not been assigned to an
area) not found, adding &quot;%1&quot; against the reserved -1 id.</source>
      <translation>[信息] - 找不到默认 (重置) 区域 (对某些尚未指定区域的房间) , 将区域 &quot;%1&quot; 编号设置为保留区域号-1.</translation>
    </message>
  </context>
  <context>
    <name>TTextEdit</name>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2625"/>
      <source>Select some text in the console first.</source>
      <extracomment>Tooltip shown on the console context menu&apos;s copy and search entries while they are disabled because nothing is selected</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2628"/>
      <source>Copy</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2639"/>
      <source>Copy HTML</source>
      <translation>複製為 HTML</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2643"/>
      <source>Copy as image</source>
      <translation>複製為圖片</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2647"/>
      <source>Select all</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2651"/>
      <source>Unknown</source>
      <translation>未知</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2652"/>
      <source>Search on %1</source>
      <translation>搜索 %1</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2674"/>
      <source>This console is empty, there is nothing to copy.</source>
      <extracomment>Tooltip shown on the console context menu&apos;s &quot;Copy as image&quot; entry while it is disabled because the console holds no text at all</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2689"/>
      <source>Analyse characters</source>
      <translation>分析字符</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2698"/>
      <source>Hover on this item to display the Unicode codepoints in the selection &lt;i&gt;(only the first line!)&lt;/i&gt;</source>
      <translation>将鼠标悬浮在此项上以显示 &lt;i&gt; Unicode 编码(仅第一行!)&lt;/i&gt; 的选项</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2707"/>
      <source>restore Main menu</source>
      <translation>恢复主菜单</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2709"/>
      <source>Use this to restore the Main menu to get access to controls.</source>
      <translation>点此恢复主菜单以获取对控件的访问.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2711"/>
      <source>restore Main Toolbar</source>
      <translation>恢复主工具栏</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2713"/>
      <source>Use this to restore the Main Toolbar to get access to controls.</source>
      <translation>点此恢复主工具栏以获取对控件的访问.</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2721"/>
      <source>Clear console</source>
      <translation>清空控制台</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2724"/>
      <source>*** starting new session ***</source>
      <translation>*** 开始新的会话 ***</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2933"/>
      <source>{tab}</source>
      <extracomment>Unicode U+0009 codepoint.</extracomment>
      <translation>{tab}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2935"/>
      <source>{line-feed}</source>
      <extracomment>Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.</extracomment>
      <translation>{line-feed}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2937"/>
      <source>{carriage-return}</source>
      <extracomment>Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.</extracomment>
      <translation>{carriage-return}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2939"/>
      <source>{space}</source>
      <extracomment>Unicode U+0020 codepoint.</extracomment>
      <translation>{space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2941"/>
      <source>{non-breaking space}</source>
      <extracomment>Unicode U+00A0 codepoint.</extracomment>
      <translation>{non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2943"/>
      <source>{soft hyphen}</source>
      <extracomment>Unicode U+00AD codepoint.</extracomment>
      <translation>{soft hyphen}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2945"/>
      <source>{combining grapheme joiner}</source>
      <extracomment>Unicode U+034F codepoint (badly named apparently - see Wikipedia!)</extracomment>
      <translation>{combining grapheme joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2947"/>
      <source>{ogham space mark}</source>
      <extracomment>Unicode U+1680 codepoint.</extracomment>
      <translation>{ogham space mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2949"/>
      <source>{&apos;n&apos; quad}</source>
      <extracomment>Unicode U+2000 codepoint.</extracomment>
      <translation>{&apos;n&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2951"/>
      <source>{&apos;m&apos; quad}</source>
      <extracomment>Unicode U+2001 codepoint.</extracomment>
      <translation>{&apos;m&apos; quad}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2953"/>
      <source>{&apos;n&apos; space}</source>
      <extracomment>Unicode U+2002 codepoint - En (&apos;n&apos;) wide space.</extracomment>
      <translation>{&apos;n&apos; space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2955"/>
      <source>{&apos;m&apos; space}</source>
      <extracomment>Unicode U+2003 codepoint - Em (&apos;m&apos;) wide space.</extracomment>
      <translation>{&apos;m&apos; space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2957"/>
      <source>{3-per-em space}</source>
      <extracomment>Unicode U+2004 codepoint - three-per-em (&apos;m&apos;) wide (thick) space.</extracomment>
      <translation>{3-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2959"/>
      <source>{4-per-em space}</source>
      <extracomment>Unicode U+2005 codepoint - four-per-em (&apos;m&apos;) wide (Middle) space.</extracomment>
      <translation>{4-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2961"/>
      <source>{6-per-em space}</source>
      <extracomment>Unicode U+2006 codepoint - six-per-em (&apos;m&apos;) wide (Sometimes the same as a Thin) space.</extracomment>
      <translation>{6-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2963"/>
      <source>{digit space}</source>
      <extracomment>Unicode U+2007 codepoint - figure (digit) wide space.</extracomment>
      <translation>{digit space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2965"/>
      <source>{punctuation wide space}</source>
      <extracomment>Unicode U+2008 codepoint.</extracomment>
      <translation>{punctuation wide space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2967"/>
      <source>{5-per-em space}</source>
      <extracomment>Unicode U+2009 codepoint - five-per-em (&apos;m&apos;) wide space.</extracomment>
      <translation>{5-per-em space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2969"/>
      <source>{hair width space}</source>
      <extracomment>Unicode U+200A codepoint - thinnest space.</extracomment>
      <translation>{hair width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2971"/>
      <source>{zero width space}</source>
      <extracomment>Unicode U+200B codepoint.</extracomment>
      <translation>{zero width space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2973"/>
      <source>{Zero width non-joiner}</source>
      <extracomment>Unicode U+200C codepoint.</extracomment>
      <translation>{Zero width non-joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2975"/>
      <source>{zero width joiner}</source>
      <extracomment>Unicode U+200D codepoint.</extracomment>
      <translation>{zero width joiner}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2977"/>
      <source>{left-to-right mark}</source>
      <extracomment>Unicode U+200E codepoint.</extracomment>
      <translation>{left-to-right mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2979"/>
      <source>{right-to-left mark}</source>
      <extracomment>Unicode U+200F codepoint.</extracomment>
      <translation>{right-to-left mark}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2981"/>
      <source>{line separator}</source>
      <extracomment>Unicode 0x2028 codepoint.</extracomment>
      <translation>{line separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2983"/>
      <source>{paragraph separator}</source>
      <extracomment>Unicode U+2029 codepoint.</extracomment>
      <translation>{paragraph separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2985"/>
      <source>{Left-to-right embedding}</source>
      <extracomment>Unicode U+202A codepoint.</extracomment>
      <translation>{Left-to-right embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2987"/>
      <source>{right-to-left embedding}</source>
      <extracomment>Unicode U+202B codepoint.</extracomment>
      <translation>{right-to-left embedding}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2989"/>
      <source>{pop directional formatting}</source>
      <extracomment>Unicode U+202C codepoint - pop (undo last) directional formatting.</extracomment>
      <translation>{pop directional formatting}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2991"/>
      <source>{Left-to-right override}</source>
      <extracomment>Unicode U+202D codepoint.</extracomment>
      <translation>{Left-to-right override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2993"/>
      <source>{right-to-left override}</source>
      <extracomment>Unicode U+202E codepoint.</extracomment>
      <translation>{right-to-left override}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2995"/>
      <source>{narrow width no-break space}</source>
      <extracomment>Unicode U+202F codepoint.</extracomment>
      <translation>{narrow width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2997"/>
      <source>{medium width mathematical space}</source>
      <extracomment>Unicode U+205F codepoint.</extracomment>
      <translation>{medium width mathematical space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2999"/>
      <source>{zero width non-breaking space}</source>
      <extracomment>Unicode U+2060 codepoint.</extracomment>
      <translation>{zero width non-breaking space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3001"/>
      <source>{function application}</source>
      <extracomment>Unicode U+2061 codepoint - function application (whatever that means!)</extracomment>
      <translation>{function application}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3003"/>
      <source>{invisible times}</source>
      <extracomment>Unicode U+2062 codepoint.</extracomment>
      <translation>{invisible times}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3005"/>
      <source>{invisible separator}</source>
      <extracomment>Unicode U+2063 codepoint - invisible separator or comma.</extracomment>
      <translation>{invisible separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3007"/>
      <source>{invisible plus}</source>
      <extracomment>Unicode U+2064 codepoint.</extracomment>
      <translation>{invisible plus}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3009"/>
      <source>{left-to-right isolate}</source>
      <extracomment>Unicode U+2066 codepoint.</extracomment>
      <translation>{left-to-right isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3011"/>
      <source>{right-to-left isolate}</source>
      <extracomment>Unicode U+2067 codepoint.</extracomment>
      <translation>{right-to-left isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3013"/>
      <source>{first strong isolate}</source>
      <extracomment>Unicode U+2068 codepoint.</extracomment>
      <translation>{first strong isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3015"/>
      <source>{pop directional isolate}</source>
      <extracomment>Unicode U+2069 codepoint - pop (undo last) directional isolate.</extracomment>
      <translation>{pop directional isolate}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3017"/>
      <source>{inhibit symmetrical swapping}</source>
      <extracomment>Unicode U+206A codepoint.</extracomment>
      <translation>{inhibit symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3019"/>
      <source>{activate symmetrical swapping}</source>
      <extracomment>Unicode U+206B codepoint.</extracomment>
      <translation>{activate symmetrical swapping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3021"/>
      <source>{inhibit arabic form-shaping}</source>
      <extracomment>Unicode U+206C codepoint.</extracomment>
      <translation>{inhibit arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3023"/>
      <source>{activate arabic form-shaping}</source>
      <extracomment>Unicode U+206D codepoint.</extracomment>
      <translation>{activate arabic form-shaping}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3025"/>
      <source>{national digit shapes}</source>
      <extracomment>Unicode U+206E codepoint.</extracomment>
      <translation>{national digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3027"/>
      <source>{nominal Digit shapes}</source>
      <extracomment>Unicode U+206F codepoint.</extracomment>
      <translation>{nominal Digit shapes}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3029"/>
      <source>{ideographic space}</source>
      <extracomment>Unicode U+3000 codepoint - ideographic (CJK Wide) space</extracomment>
      <translation>{ideographic space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3031"/>
      <source>{variation selector 1}</source>
      <extracomment>Unicode U+FE00 codepoint.</extracomment>
      <translation>{variation selector 1}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3033"/>
      <source>{variation selector 2}</source>
      <extracomment>Unicode U+FE01 codepoint.</extracomment>
      <translation>{variation selector 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3035"/>
      <source>{variation selector 3}</source>
      <extracomment>Unicode U+FE02 codepoint.</extracomment>
      <translation>{variation selector 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3037"/>
      <source>{variation selector 4}</source>
      <extracomment>Unicode U+FE03 codepoint.</extracomment>
      <translation>{variation selector 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3039"/>
      <source>{variation selector 5}</source>
      <extracomment>Unicode U+FE04 codepoint.</extracomment>
      <translation>{variation selector 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3041"/>
      <source>{variation selector 6}</source>
      <extracomment>Unicode U+FE05 codepoint.</extracomment>
      <translation>{variation selector 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3043"/>
      <source>{variation selector 7}</source>
      <extracomment>Unicode U+FE06 codepoint.</extracomment>
      <translation>{variation selector 7}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3045"/>
      <source>{variation selector 8}</source>
      <extracomment>Unicode U+FE07 codepoint.</extracomment>
      <translation>{variation selector 8}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3047"/>
      <source>{variation selector 9}</source>
      <extracomment>Unicode U+FE08 codepoint.</extracomment>
      <translation>{variation selector 9}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3049"/>
      <source>{variation selector 10}</source>
      <extracomment>Unicode U+FE09 codepoint.</extracomment>
      <translation>{variation selector 10}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3051"/>
      <source>{variation selector 11}</source>
      <extracomment>Unicode U+FE0A codepoint.</extracomment>
      <translation>{variation selector 11}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3053"/>
      <source>{variation selector 12}</source>
      <extracomment>Unicode U+FE0B codepoint.</extracomment>
      <translation>{variation selector 12}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3055"/>
      <source>{variation selector 13}</source>
      <extracomment>Unicode U+FE0C codepoint.</extracomment>
      <translation>{variation selector 13}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3057"/>
      <source>{variation selector 14}</source>
      <extracomment>Unicode U+FE0D codepoint.</extracomment>
      <translation>{variation selector 14}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3059"/>
      <source>{variation selector 15}</source>
      <extracomment>Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black &amp; white) rendition.</extracomment>
      <translation>{variation selector 15}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3061"/>
      <source>{variation selector 16}</source>
      <extracomment>Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured &apos;Emoji&apos; rendition.</extracomment>
      <translation>{variation selector 16}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3063"/>
      <source>{zero width no-break space}</source>
      <extracomment>Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).</extracomment>
      <translation>{zero width no-break space}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3071"/>
      <source>{interlinear annotation anchor}</source>
      <extracomment>Unicode U+FFF9 codepoint.</extracomment>
      <translation>{interlinear annotation anchor}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3073"/>
      <source>{interlinear annotation separator}</source>
      <extracomment>Unicode U+FFFA codepoint.</extracomment>
      <translation>{interlinear annotation separator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3075"/>
      <source>{interlinear annotation terminator}</source>
      <extracomment>Unicode U+FFFB codepoint</extracomment>
      <translation>{interlinear annotation terminator}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3077"/>
      <source>{object replacement character}</source>
      <extracomment>Unicode U+FFFC codepoint.</extracomment>
      <translation>{object replacement character}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3090"/>
      <location filename="../src/TTextEdit.cpp" line="3094"/>
      <location filename="../src/TTextEdit.cpp" line="3116"/>
      <source>{noncharacter}</source>
      <extracomment>Unicode codepoint in range U+FFD0 to U+FDEF - not a character
----------
Unicode codepoint in range U+FFFx - not a character.
----------
Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.</extracomment>
      <translation>{noncharacter}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3103"/>
      <source>{FitzPatrick modifier 1 or 2}</source>
      <extracomment>Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.</extracomment>
      <translation>{FitzPatrick modifier 1 or 2}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3105"/>
      <source>{FitzPatrick modifier 3}</source>
      <extracomment>Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.</extracomment>
      <translation>{FitzPatrick modifier 3}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3107"/>
      <source>{FitzPatrick modifier 4}</source>
      <extracomment>Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.</extracomment>
      <translation>{FitzPatrick modifier 4}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3109"/>
      <source>{FitzPatrick modifier 5}</source>
      <extracomment>Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.</extracomment>
      <translation>{FitzPatrick modifier 5}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3111"/>
      <source>{FitzPatrick modifier 6}</source>
      <extracomment>Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.</extracomment>
      <translation>{FitzPatrick modifier 6}</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3392"/>
      <location filename="../src/TTextEdit.cpp" line="3458"/>
      <source>Index (UTF-16)</source>
      <extracomment>1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}</extracomment>
      <translation>索引 (UTF-16)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3397"/>
      <location filename="../src/TTextEdit.cpp" line="3463"/>
      <source>U+&lt;i&gt;####&lt;/i&gt; Unicode Code-point &lt;i&gt;(High:Low Surrogates)&lt;/i&gt;</source>
      <extracomment>2nd Row heading for Text analyser output, table item is the unicode code point (will be between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3402"/>
      <location filename="../src/TTextEdit.cpp" line="3468"/>
      <source>Visual</source>
      <extracomment>3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a &apos;{&apos;...&apos;}&apos; wrapped letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}</extracomment>
      <translation>外观</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3407"/>
      <location filename="../src/TTextEdit.cpp" line="3473"/>
      <source>Index (UTF-8)</source>
      <extracomment>4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</extracomment>
      <translation>索引 (UTF-8)</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3412"/>
      <location filename="../src/TTextEdit.cpp" line="3478"/>
      <source>Byte</source>
      <extracomment>5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua system uses {this translation used 2 times}</extracomment>
      <translation>字节</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3418"/>
      <location filename="../src/TTextEdit.cpp" line="3484"/>
      <source>Lua character or code</source>
      <extracomment>6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2 times}&quot;</extracomment>
      <translation>Lua字符或代码</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3698"/>
      <source>link</source>
      <extracomment>Generic screen-reader announcement for a link with no tooltip or URL — used as fallback link description</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3703"/>
      <source>, visited</source>
      <extracomment>Appended to link announcement when the link has been previously visited</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3707"/>
      <source>, disabled</source>
      <extracomment>Appended to link announcement when the link is disabled</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3711"/>
      <source>, selected</source>
      <extracomment>Appended to link announcement when the link is selected</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="3716"/>
      <source>, has menu</source>
      <extracomment>Appended to link announcement when the link opens a menu</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="4004"/>
      <source>Wrapping to first link</source>
      <extracomment>Screen-reader announcement when forward link navigation (Tab / Ctrl+]) wraps past the last link back to the first</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="4007"/>
      <source>Wrapping to last link</source>
      <extracomment>Screen-reader announcement when backward link navigation (Shift+Tab / Ctrl+[) wraps past the first link back to the last</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="4153"/>
      <source>Jumped to start of buffer.</source>
      <extracomment>Screen-reader announcement when the user presses Ctrl+Home in caret mode to jump to the start of the buffer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="4167"/>
      <source>Jumped to latest content.</source>
      <extracomment>Screen-reader announcement when the user presses Ctrl+End in caret mode to jump to the latest (most recent) content in the buffer</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2071"/>
      <source>Mudlet, debug console extract</source>
      <translation>Mudlet, 调试控制台提取内容</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2073"/>
      <source>Mudlet, %1 mini-console extract from %2 profile</source>
      <translation>Mudlet, %1 迷你控制台提取内容, 来自 %2 配置文件</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2075"/>
      <source>Mudlet, %1 user window extract from %2 profile</source>
      <translation>Mudlet， %1 用户窗口从 %2 配置文件中提取</translation>
    </message>
    <message>
      <location filename="../src/TTextEdit.cpp" line="2077"/>
      <source>Mudlet, main console extract from %1 profile</source>
      <translation>Mudlet，主控制台从 %1 配置文件中提取</translation>
    </message>
  </context>
  <context>
    <name>TToolBar</name>
    <message>
      <location filename="../src/TToolBar.cpp" line="76"/>
      <source>Toolbar - %1 - %2</source>
      <translation>工具栏 - %1 - %2</translation>
    </message>
  </context>
  <context>
    <name>TTrigger</name>
    <message>
      <location filename="../src/TTrigger.cpp" line="120"/>
      <source>error: this trigger has no patterns defined</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="156"/>
      <source>Error: in item %1, perl regex &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>错误：在项目 %1，perl regex &quot;%2&quot; 未能编译，原因是： &quot;%3&quot;.</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="178"/>
      <source>Error: in item %1, lua function &quot;%2&quot; failed to compile, reason: &quot;%3&quot;.</source>
      <translation>错误: 在项目 %1中， lua 函数 &quot;%2&quot; 编译失败，原因: &quot;%3&quot;。</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="197"/>
      <source>Error: in item %1, no colors to match were set - at least &lt;i&gt;one&lt;/i&gt; of the foreground or background must not be &lt;i&gt;ignored&lt;/i&gt;.</source>
      <translation>错误：在 %1中，未设定拟匹配的颜色。前景色或背景色&lt;i&gt;必须&lt;/i&gt;至少设定&lt;i&gt;其中之一&lt;/i&gt;</translation>
    </message>
    <message>
      <location filename="../src/TTrigger.cpp" line="1119"/>
      <source>Trigger name=%1 expired.</source>
      <translation>触发器名称=%1 过期了.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/TTrigger.cpp" line="1124"/>
      <source>Trigger name=%1 will fire %n more time(s).</source>
      <translation>
        <numerusform>觸發名稱 =%1 將被觸發 %n 次</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>TUiTour</name>
    <message>
      <location filename="../src/TUiTour.cpp" line="61"/>
      <source>Mudlet interface tour</source>
      <extracomment>Name of the interface tour overlay, announced by screen readers</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="120"/>
      <source>Skip tour</source>
      <extracomment>Button on the interface tour that dismisses the tour</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="123"/>
      <source>Back</source>
      <extracomment>Button on the interface tour that goes back to the previous step</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="125"/>
      <location filename="../src/TUiTour.cpp" line="274"/>
      <source>Next</source>
      <extracomment>Button on the interface tour that advances to the next step</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="185"/>
      <source>Welcome to Mudlet!</source>
      <extracomment>Title of the first step of the interface tour</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="187"/>
      <source>New here? This quick tour points out the most important parts of Mudlet - it takes less than a minute. Click anywhere or use the arrow keys to move through it.</source>
      <extracomment>Body of the first step of the interface tour</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="196"/>
      <source>The game window</source>
      <extracomment>Title of the interface tour step highlighting the main text display</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="198"/>
      <source>Text from the game appears here. Scroll up to review earlier text - the newest text stays visible in a split view while you do.</source>
      <extracomment>Body of the interface tour step highlighting the main text display</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="205"/>
      <source>The input line</source>
      <extracomment>Title of the interface tour step highlighting the command input line</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="207"/>
      <source>Type game commands here and press Enter to send them. Use the up and down arrow keys to bring back commands you typed before.</source>
      <extracomment>Body of the interface tour step highlighting the command input line</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="216"/>
      <source>Automate your game</source>
      <extracomment>Title of the interface tour step highlighting the scripting tools</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="218"/>
      <source>Triggers, aliases, timers and scripts let Mudlet react to the game for you and shorten what you type. You will find them in the script editor, right here - start simple, no programming needed.</source>
      <extracomment>Body of the interface tour step highlighting the scripting tools</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="226"/>
      <source>Make Mudlet yours</source>
      <extracomment>Title of the interface tour step highlighting the preferences</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="228"/>
      <source>Fonts, colors, the map, accessibility options and much more can be adjusted in the settings, right here.</source>
      <extracomment>Body of the interface tour step highlighting the preferences</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="232"/>
      <source>That&apos;s it - have fun!</source>
      <extracomment>Title of the last step of the interface tour</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="234"/>
      <source>For a hands-on lesson, connect to the &lt;b&gt;Mudlet Tutorial&lt;/b&gt; game. And if you ever want to see this tour again, it lives in Help → Take a UI tour.</source>
      <extracomment>Body of the last step of the interface tour. The tour can be re-run via the named menu entry.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="268"/>
      <source>%1 of %2</source>
      <extracomment>Progress through the interface tour, %1 is the current step number, %2 the total number of steps</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TUiTour.cpp" line="274"/>
      <source>Finish</source>
      <extracomment>Button on the last step of the interface tour that closes it. The other label option is &quot;Next&quot;.</extracomment>
      <translation>完成</translation>
    </message>
  </context>
  <context>
    <name>TriggerUnit</name>
    <message numerus="yes">
      <location filename="../src/TriggerUnit.cpp" line="382"/>
      <source>%n trigger(s) created while processing this line have been stopped: temporary ones removed, permanent ones switched off until the profile is reloaded.</source>
      <extracomment>%n is a count of triggers. Shown in the game window when a trigger keeps creating new triggers that match the same line, which would otherwise never end</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TriggerUnit.cpp" line="387"/>
      <source>[ ERROR ] - Trigger processing stopped to prevent a freeze: a trigger (or another trigger it creates) keeps creating new triggers that match the line being processed, so that line never finishes. %1 Create the trigger once, outside its own script, or give it a pattern that does not match the line it is created on.</source>
      <extracomment>%1 is the sentence above, about the triggers that were stopped</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/TriggerUnit.cpp" line="393"/>
      <source>[ ERROR ] - Trigger processing stopped to prevent a freeze: trigger &apos;%1&apos; (or another trigger it creates) keeps creating new triggers that match the line being processed, so that line never finishes. %2 Create the trigger once, outside its own script, or give it a pattern that does not match the line it is created on.</source>
      <extracomment>%1 is the name of a trigger - the name of a trigger made by tempTrigger() and friends is its id number - and %2 is the sentence above, about the triggers that were stopped</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>UpdateDialog</name>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="20"/>
      <source>%APPNAME% update</source>
      <translation>%APPNAME% 更新</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="50"/>
      <source>Loading update information …</source>
      <translation>正在加载更新信息 …</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="87"/>
      <source>A new version of %APPNAME% is available!</source>
      <translation>有新的 %APPNAME% 版本可用！</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="113"/>
      <source>%APPNAME% %UPDATE_VERSION% is available (you have %CURRENT_VERSION%).
Would you like to update now?</source>
      <translation>%APPNAME% 已有最新的版本 %UPDATE_VERSION% (您当前版本为 %CURRENT_VERSION%)。
现在要更新吗?</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="151"/>
      <source>Changelog for %APPNAME%</source>
      <translation>%APPNAME% 的更新記錄</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="161"/>
      <source>You are using version %CURRENT_VERSION%.</source>
      <translation>您当前版本为 %CURRENT_VERSION%。</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="192"/>
      <source>There are currently no updates available.</source>
      <translation>目前沒有可用的更新。</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="208"/>
      <source>You are using %APPNAME% %CURRENT_VERSION%.</source>
      <translation>目前使用的版本為 %APPNAME% %CURRENT_VERSION%</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="272"/>
      <source>Automatically download future updates</source>
      <translation>自动下载更新</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="319"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="339"/>
      <source>Install update now</source>
      <translation>正在安装更新</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="346"/>
      <source>OK</source>
      <translation>好</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="356"/>
      <source>Remind me later</source>
      <translation>稍候再提示我</translation>
    </message>
    <message>
      <location filename="../src/updater/update_dialog.ui" line="361"/>
      <source>Skip this version</source>
      <translation>忽略此版本</translation>
    </message>
  </context>
  <context>
    <name>Updater</name>
    <message>
      <location filename="../src/updater.cpp" line="83"/>
      <location filename="../src/updater.cpp" line="370"/>
      <location filename="../src/updater.cpp" line="415"/>
      <source>Update</source>
      <extracomment>Label for the update/restart button in the main toolbar
----------
Label for the update button shown in the update dialog</extracomment>
      <translation>更新</translation>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="239"/>
      <source>Changelog Error</source>
      <extracomment>Error title for dialog shown when changelog fails to load</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="241"/>
      <source>Could not load the changelog. Please try again later.</source>
      <extracomment>Error message shown when changelog fails to load from the server</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="264"/>
      <source>No download available for version %1. Please try again later or download manually from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when no download is available for the user&apos;s platform. %1 is the version number.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="353"/>
      <location filename="../src/updater.cpp" line="398"/>
      <source>Update download failed. Please try again or download manually from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the automatic update download finished but produced no file</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="456"/>
      <source>Failed to extract the update. Please try again or download manually from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when extracting the downloaded update archive fails on Linux</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="476"/>
      <location filename="../src/updater.cpp" line="482"/>
      <location filename="../src/updater.cpp" line="496"/>
      <location filename="../src/updater.cpp" line="509"/>
      <source>Failed to install the update. Please try again or download manually from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the automatic update fails to install on Linux</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="490"/>
      <source>Failed to install the update and could not restore the previous version. Your previous version is saved at: %1 - please rename it back manually. Alternatively, download a fresh copy from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the update fails and the previous version could not be restored automatically. %1 is the file path to the backup copy.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="546"/>
      <location filename="../src/updater.cpp" line="646"/>
      <source>Update Error</source>
      <extracomment>Error title for update-related warning dialogs
----------
Error title for dialog shown when Mudlet fails to restart after updating</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="551"/>
      <source>The update installer could not be found. Please try checking for updates again.</source>
      <extracomment>Error shown when the downloaded installer file cannot be found on disk</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="563"/>
      <source>Could not prepare the update installer. Please try again or download the update manually from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the installer file cannot be copied to a temporary location for launch</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="595"/>
      <location filename="../src/updater.cpp" line="610"/>
      <source>Could not prepare the update. Please close Mudlet and run the installer manually:
%1</source>
      <extracomment>Error shown when the batch file for managing the update process cannot be written. %1 is the path to the installer.
----------
Error shown when the batch file for managing the update process cannot be created. %1 is the path to the installer.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="603"/>
      <source>Could not launch the update installer. Please restart Mudlet and try again.</source>
      <extracomment>Error shown when the update installer process fails to start</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="648"/>
      <source>Could not restart Mudlet after the update. Please start it manually.</source>
      <extracomment>Error message shown when Mudlet fails to restart after updating on Linux</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="673"/>
      <source>Restart to apply update</source>
      <extracomment>Label for the button shown after the update has been downloaded and installed, prompting user to restart</extracomment>
      <translation>重新啟動以套用更新</translation>
    </message>
    <message>
      <location filename="../src/updater.cpp" line="676"/>
      <source>Update failed</source>
      <extracomment>Label for the update button shown when the update installation failed</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>VarUnit</name>
    <message>
      <location filename="../src/VarUnit.cpp" line="120"/>
      <source>Lua functions cannot be saved.</source>
      <extracomment>Tooltip explaining why a Lua function cannot be saved</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/VarUnit.cpp" line="125"/>
      <source>Referenced variables cannot be saved.</source>
      <extracomment>Tooltip explaining why a referenced variable cannot be saved</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/VarUnit.cpp" line="132"/>
      <source>This table has %1 items, exceeding the 10,000 item limit for saved variables. Use &lt;b&gt;table.save()&lt;/b&gt; and &lt;b&gt;table.load()&lt;/b&gt; instead for better performance with large tables.</source>
      <extracomment>Tooltip explaining why a large table cannot be saved, recommending alternative methods</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/VarUnit.cpp" line="153"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation>选中的变量将被保存并会和您的配置文件一起加载。</translation>
    </message>
  </context>
  <context>
    <name>XMLexport</name>
    <message>
      <location filename="../src/XMLexport.cpp" line="816"/>
      <source>[ WARN ]  - These saved variables are nested more than %1 tables deep, so this save holds them as empty tables: %2. Store data that deep with table.save() and table.load() instead.</source>
      <extracomment>%1 is how many levels of nested tables Mudlet reads, %2 is a comma separated list of Lua variable names</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>XMLimport</name>
    <message>
      <location filename="../src/XMLimport.cpp" line="153"/>
      <source>[ ALERT ] - Sorry, the file being read:
&quot;%1&quot;
reports it has a version (%2) it must have come from a later Mudlet version,
and this one cannot read it, you need a newer Mudlet!</source>
      <translation>[警告] - 对不起, 正在读取的文件：
&quot;%1&quot;
的版本号(%2) 表明它来自更高的Mudlet版本，
当前版本的Mudlet无法读取它, 你需要更新Mudlet!</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="356"/>
      <source>Parsing area data...</source>
      <translation>正在分析区域数据...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="360"/>
      <source>Parsing room data...</source>
      <translation>正在分析房间数据...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="364"/>
      <source>Parsing environment data...</source>
      <translation>正在分析环境数据...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="372"/>
      <source>Assigning rooms to their areas...</source>
      <translation>正在为房间分配区域...</translation>
    </message>
    <message>
      <location filename="../src/XMLimport.cpp" line="579"/>
      <source>Parsing room data [count: %1]...</source>
      <translation>正在分析房间数据 [数量: %1]...</translation>
    </message>
  </context>
  <context>
    <name>about_dialog</name>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="41"/>
      <source>About Mudlet</source>
      <translation>關於</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="101"/>
      <source>Mudlet</source>
      <translation>Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="164"/>
      <source>Supporters</source>
      <translation>支持者</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="193"/>
      <source>License</source>
      <translation>授權許可</translation>
    </message>
    <message>
      <location filename="../src/ui/about_dialog.ui" line="228"/>
      <source>Third Party</source>
      <translation>第三方</translation>
    </message>
  </context>
  <context>
    <name>actions_main_area</name>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="103"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="168"/>
      <source>Button Bar Properties</source>
      <translation>按鈕欄屬性</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="226"/>
      <source>Orientation Horizontal</source>
      <translation>水平方向</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="231"/>
      <source>Orientation Vertical</source>
      <translation>垂直方向</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="246"/>
      <source>Dock Area Top</source>
      <translation>停靠頂部</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="251"/>
      <source>Dock Area Left</source>
      <translation>停靠左側</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="256"/>
      <source>Dock Area Right</source>
      <translation>停靠右側</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="261"/>
      <source>Floating Toolbar</source>
      <translation>浮動工具欄</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="284"/>
      <source>Button Properties</source>
      <translation>按鈕屬性</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="290"/>
      <source>Button Rotation:</source>
      <translation>按钮旋转:</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="310"/>
      <source>no rotation</source>
      <translation>不旋转</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="315"/>
      <source>90° rotation to the left</source>
      <translation>向左旋转 90°</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="320"/>
      <source>90° rotation to the right</source>
      <translation>向右旋转 90°</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="328"/>
      <source>Push down button</source>
      <translation>下拉按钮</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="335"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="351"/>
      <location filename="../src/ui/actions_main_area.ui" line="374"/>
      <source>Text to send to the game as-is (optional)</source>
      <translation>发送到游戏的文本 (可选)</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="358"/>
      <source>Command (up):</source>
      <translation>命令 (向上):</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="75"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your button, menu or toolbar. This will be displayed in the buttons tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;为你的按钮、菜单、工具栏选择一个好的、唯一的名字。它将显示在按钮树上。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="180"/>
      <source>Number of rows:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="199"/>
      <source>Offset of first button:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="348"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game if it is pressed. (Optional)&lt;/p&gt;&lt;p&gt;If this is a &lt;i&gt;push-down&lt;/i&gt; button then this is sent only when the button goes from the &lt;i&gt;up&lt;/i&gt; to &lt;i&gt;down&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="371"/>
      <source>&lt;p&gt;Type in one or more commands you want the button to send directly to the game when this button goes from the &lt;i&gt;down&lt;/i&gt; to &lt;i&gt;up&lt;/i&gt; state.&lt;/p&gt;&lt;p&gt;To send more complex commands, that could depend on or need to modifies variables within this profile a Lua script should be entered &lt;i&gt;instead&lt;/i&gt; in the editor area below.  Anything entered here is, literally, just sent to the game server.&lt;/p&gt;&lt;p&gt;It is permissible to use both this &lt;i&gt;and&lt;/i&gt; a Lua script - this will be sent &lt;b&gt;before&lt;/b&gt; the script is run.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="384"/>
      <source>Icon</source>
      <translation>图标</translation>
    </message>
    <message>
      <location filename="../src/ui/actions_main_area.ui" line="419"/>
      <source>Stylesheet:</source>
      <translation>樣式</translation>
    </message>
  </context>
  <context>
    <name>aliases_main_area</name>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="35"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="57"/>
      <source>choose a unique name for your alias; it will show in the tree and is needed for scripting.</source>
      <translation>为你的别名选择一个唯一的名字; 它会用在脚本中并显示在树状列表上.</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="85"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="126"/>
      <source>Pattern:</source>
      <translation>模式：</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="192"/>
      <source>enter a perl regex pattern for your alias; alias are triggers on your input</source>
      <translation>为您的别名输入一个 perl 正则表达式; 别名是针对你输入内容的触发器.</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="195"/>
      <source>^mycommand$ (example)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="148"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="170"/>
      <source>&lt;p&gt;Enter one or more commands to use if the given command matches the pattern. (Optional)&lt;/p&gt;&lt;p&gt;This could be another alias or a command to send directly to the game. For complex commands that require modification of variables within this profile, use a Lua script in the editor area below instead. It&apos;s possible to use both this field and a Lua script - the contents of this field will be used before running the script.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/aliases_main_area.ui" line="173"/>
      <source>Replacement text (optional)</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>cTelnet</name>
    <message>
      <location filename="../src/ctelnet.cpp" line="781"/>
      <source>hh:mm:ss.zzz</source>
      <translation>hh:mm:ss.zzz</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="809"/>
      <location filename="../src/ctelnet.cpp" line="865"/>
      <source>User Disconnected</source>
      <extracomment>A reason why a connection to a game server ended, could be one of several to be listed. This text used in two places, ensure the same text is used in both.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="814"/>
      <location filename="../src/ctelnet.cpp" line="873"/>
      <source>Connection/login attempt rejected by server</source>
      <extracomment>A reason why a connection to a game server ended, could be one of several to be listed. This text used in two places, ensure the same text is used in both.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1323"/>
      <source>[ ERROR ] - Internal error, no codec found for current setting of {&quot;%1&quot;}
so Mudlet cannot send data in that format to the Game Server. Please
check to see if there is an alternative that the MUD and Mudlet can
use. Mudlet will attempt to send the data using the ASCII encoding
but will be limited to only unaccented characters of basic English.
Note: this warning will only be issued once, until the encoding is
changed.</source>
      <translation>[ ERROR ] - 内部错误，没有找到当前设置的{&quot;%1&quot;} 的编码解码器。
因此Mudlet不能以这种格式向游戏服务器发送数据。
请检查是否有MUD和Mudlet可以
使用的替代方式。Mudlet将尝试使用ASCII编码
发送数据，但将仅限于基本英语的无重音字符。
注意：该警告只会发出一次，直到编码改变。</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1555"/>
      <source>[ INFO ]  - Package download cancelled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1558"/>
      <source>[ WARN ]  - Package download failed from &apos;%1&apos;, reason: %2</source>
      <extracomment>%1 is the URL, %2 is the error message</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1562"/>
      <source>
The package is hosted on a server with an SSL certificate problem. The URL may be using HTTPS when it should use HTTP, or the server&apos;s security certificate is not trusted by your system.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1578"/>
      <source>[ WARN ]  - Package download failed: could not open file &apos;%1&apos; for writing, reason: %2</source>
      <extracomment>%1 is the file path, %2 is the error message</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1589"/>
      <source>[ WARN ]  - Package download failed: could not save file, reason: %1</source>
      <extracomment>%1 is the error message</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1602"/>
      <source>[ WARN ]  - Package installation failed for &apos;%1&apos;, reason: %2</source>
      <extracomment>%1 is the package file path, %2 is the error message</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2563"/>
      <source>[ INFO ]  - This game appears to use KaVir&apos;s protocol handler, which works best when Mudlet reports its version number during connection. Version reporting in terminal type has been automatically enabled for improved color support. Reconnecting...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="556"/>
      <location filename="../src/ctelnet.cpp" line="1237"/>
      <source>[%1]</source>
      <extracomment>For an IPv6 address (which is composed of hex-digits and colons) if we want to show it with a port number appended (as a colon and then an integer between 1 and 65535) we need to wrap it with &apos;[&apos;...&apos;]&apos; to separate the latter from the former, however some Far-East locales may expect to use the wide versions of these character here.</extracomment>
      <translation>[%1]</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="559"/>
      <source>Looking up the details of server: %1:%2 ...</source>
      <extracomment>%1 is the URL or an IP address (suitably wrapped if it is an IPv6 one) of the Game Server (or Proxy); %2 is the port number.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="706"/>
      <source>[  OK  ]  - Secure connection made (IPv6).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="708"/>
      <source>[  OK  ]  - Secure connection made (IPv4).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="712"/>
      <source>[  OK  ]  - Open connection made (IPv6).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="714"/>
      <source>[  OK  ]  - Open connection made (IPv4).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="719"/>
      <source>[  OK  ]  - Connection made (IPv6).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="721"/>
      <source>[  OK  ]  - Connection made (IPv4).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="776"/>
      <source>[ INFO ]  - Connection time: %1.</source>
      <translation>[ 信息 ] - 连接时间: %1.</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/ctelnet.cpp" line="839"/>
      <source>[ ALERT ] - Socket got disconnected, for %n reason(s):
%1</source>
      <extracomment>This message is used when we have been trying to connect or we were connected securely, but the connection has been lost. It is possible with a secure connection that there is MORE than one error message to show, but for English or other locales where the singular case (%n==1) is distinct it would be perfectly feasible to replace &quot;for %n reason(s)&quot; with &quot;because&quot; for that number (1) of errors - however the text should then be repeated in the corresponding situation for an &quot;open&quot; connection which is different in that it only ever has one &quot;reason&quot; to report.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="852"/>
      <location filename="../src/ctelnet.cpp" line="885"/>
      <source>[ ALERT ] - Socket got disconnected.</source>
      <extracomment>This message is used when we have been trying to connect or we were connected securely or in an open manner, but the connection has been lost and we do not have any explaination to give to the user as to why. Anyhow, in this case we do not have anything more to say about it. This text used in two places, ensure the same translation is used in both of them.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="868"/>
      <source>Secure connections not supported by this game on this port; try turning the option off</source>
      <extracomment>A reason why a connection to a game server ended.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="895"/>
      <source>[ ALERT ] - Socket got disconnected, for reason:
%1</source>
      <extracomment>This message is used when we have been trying to connect or we were connected in an open, insecure manner, but the connection has been lost. Unlike the secure connection case there is only one error message to show; it would be desirable to use the same text for this message as the &quot;one reason&quot; (%n==1) situation for locales such as English (with a distinct form for the singular) use for the secure type of connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1021"/>
      <source>Host name lookup Failure! A connection cannot be established.
The server name is not correct, or your nameservers are not
working properly.
</source>
      <extracomment>This text is used in the (expected) case when the user has provided a URL for the Game Server rather than (unusually) an IP address. After a DNS lookup however, we have NOT found any IP addresses which means that we cannot proceed further to connect to the Game server.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1026"/>
      <source>[ ERROR ] - Unable to connect to &quot;%1&quot;.
Check your internet connection and the details entered for the game server.</source>
      <extracomment>%1 is the URL of the Game Server</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1038"/>
      <source>%1 (IPv6)</source>
      <extracomment>Used to add an IPv6 address line to the list displayed during connecting to a Host. Some, e.g. Far Eastern locales may require a different text here if they do not use spaces, or need &quot;wide&quot; &apos;(&apos; &apos;)&apos;s</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1044"/>
      <source>%1 (IPv4)</source>
      <extracomment>Used to add an IPv4 address line to the list displayed during connecting to a Host. Some, e.g. Far Eastern locales may require a different text here if they do not use spaces, or &quot;wide&quot; &apos;(&apos;...&apos;)&apos;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1063"/>
      <source>A host name could not be found for the given IP address.</source>
      <extracomment>This text is used when the user has provided a raw IP address for the Game Server rather than a URL. In this case we try to perform a &quot;reverse-lookup&quot; to see if we can identify the URL that matches it - but nothing useful was found and we&apos;ve got the original address back.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1070"/>
      <source>A host name for the IP address has been found.
It is: &quot;%1&quot;
</source>
      <extracomment>This text is used when the user has provided a raw IP address for the Game Server rather than a URL. In this case we try to perform a &quot;reverse-lookup&quot; to see if we can identify the URL that matches it - and this is used when we have something (%1) to show.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/ctelnet.cpp" line="1081"/>
      <source>The %n IP address(es) of %1 has/have been found. It/They are:</source>
      <extracomment>This text is used in the (expected) case when the user has provided a URL (%1) for the Game Server rather than (unusually) an IP address. After a DNS lookup we have found at least one but possibly more (%n) IP addresses, which will be listed (one per line) immediately afterwards.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1118"/>
      <source>Trying secure (IPv4 and IPv6) connections to proxy %1:%2 ...</source>
      <extracomment>Happy-Eyeballs (both IPv4 and IPv6 addresses available) case. %1 is the URL for the server and %2 is the port number (on BOTH addresses) for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1123"/>
      <location filename="../src/ctelnet.cpp" line="1156"/>
      <location filename="../src/ctelnet.cpp" line="1183"/>
      <source>[ INFO ]  - Attempting a secure connection to %1:%2 via proxy...</source>
      <extracomment>We don&apos;t need to worry about %1 being a raw IPv6 address here as we prohibit IP addresses for secure connections so it is a URL; %2 is the port number.
----------
%1 is a URL for the Game Server; %2 is the port number.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1129"/>
      <location filename="../src/ctelnet.cpp" line="1161"/>
      <source>Trying secure (IPv4 and IPv6) connections to %1:%2 ...</source>
      <extracomment>Happy-Eyeballs (both IPv4 and IPv6 addresses available) case. %1 is the URL for the Server and %2 is the port number (on BOTH addresses) for the connection.
----------
%1 is the URL for the Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1134"/>
      <location filename="../src/ctelnet.cpp" line="1166"/>
      <location filename="../src/ctelnet.cpp" line="1191"/>
      <source>[ INFO ]  - Attempting a secure connection to %1:%2 ...</source>
      <extracomment>We don&apos;t need to worry about %1 being a raw IPv6 address here as we prohibit IP addresses for secure connections so it is a URL; %2 is the port number.
----------
%1 is a URL for the Game Server; %2 is the port number.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1151"/>
      <source>Trying secure (IPv6) connection to %1:%2 via proxy...</source>
      <extracomment>%1 is the URL for the Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1180"/>
      <source>Trying secure (IPv4) connection to %1:%2 via proxy...</source>
      <extracomment>%1 is the URL for the Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1188"/>
      <source>Trying secure (IPv4) connection to %1:%2 ...</source>
      <extracomment>%1 is the URL for the Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1212"/>
      <source>Trying open (IPv4 and IPv6) connections to %1:%2 via proxy...</source>
      <extracomment>Happy-Eyeballs (both IPv4 and IPv6 addresses available) case. %1 is the URL for the proxy and %2 is the port number (on BOTH addresses) for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1215"/>
      <location filename="../src/ctelnet.cpp" line="1246"/>
      <location filename="../src/ctelnet.cpp" line="1273"/>
      <source>[ INFO ]  - Attempting an open connection to %1:%2 via proxy...</source>
      <extracomment>%1 is a URL for the Game Server; %2 is the port number.
----------
%1 is the URL or IPv6 address (suitably wrapped) for the Game Server and %2 is the port number.
----------
%1 is the URL or IPv4 address for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1221"/>
      <source>Trying open (IPv4 and IPv6) connections to %1:%2 ...</source>
      <extracomment>Happy-Eyeballs (both IPv4 and IPv6 addresses available) case. %1 is the URL for the Server and %2 is the port number (on BOTH addresses) for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1224"/>
      <location filename="../src/ctelnet.cpp" line="1254"/>
      <location filename="../src/ctelnet.cpp" line="1282"/>
      <source>[ INFO ]  - Attempting an open connection to %1:%2 ...</source>
      <extracomment>%1 is a URL for the Game Server; %2 is the port number.
----------
%1 is the URL or IPv6 address (suitably wrapped) for the Game Server and %2 is the port number for the connection.
----------
%1 is the URL or IPv4 address for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1242"/>
      <source>Trying open (IPv6) connection to %1:%2 via proxy...</source>
      <extracomment>%1 is the URL or IPv6 address (suitably wrapped) for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1250"/>
      <source>Trying open (IPv6) connection to %1:%2 ...</source>
      <extracomment>%1 is the URL or IPv6 address (suitably wrapped) for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1269"/>
      <source>Trying open (IPv4) connection to %1:%2 via proxy...</source>
      <extracomment>%1 is the URL or IPv4 address for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="1278"/>
      <source>Trying open (IPv4) connection to %1:%2 ...</source>
      <extracomment>%1 is the URL or IPv4 address for the Game Server and %2 is the port number for the connection.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="2582"/>
      <source>[ INFO ]  - This game appears to support MXP (Mud eXtension Protocol), but has not turned it on properly. MXP processing has been automatically enabled for clickable links, room info, and richer interactions. You can disable this setting in Settings &gt; Special Options.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="3674"/>
      <location filename="../src/ctelnet.cpp" line="4073"/>
      <source>[ INFO ]  - Upgrading the GUI to new version &apos;%1&apos; from version &apos;%2&apos;
(url=&apos;%3&apos;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4010"/>
      <source>[ INFO ]  - Downloading and installing package &apos;%1&apos;
(url=&apos;%2&apos;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4034"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4034"/>
      <source>Downloading game GUI from server...</source>
      <translation>正从服务器中下载游戏GUI...</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4335"/>
      <source>[ INFO ]  - A more secure connection on port %1 is available.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4344"/>
      <source>For data transfer protection and privacy, this connection advertises a secure port.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4345"/>
      <source>Update to port %1 and connect with encryption?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4506"/>
      <source>ERROR</source>
      <extracomment>Keep the capitalisation, the translated text at 7 letters max so it aligns nicely</extracomment>
      <translation>錯誤</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4519"/>
      <source>LUA</source>
      <translation>LUA</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4531"/>
      <source>WARN</source>
      <translation>警告</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4543"/>
      <source>ALERT</source>
      <translation>警告</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4555"/>
      <source>INFO</source>
      <translation>信息</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4567"/>
      <source>OK</source>
      <translation>好</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4578"/>
      <source>CHAT</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4821"/>
      <source>[ WARN  ]  - MCCP decompression error (%1), compression disabled.
If the display looks garbled, please reconnect to the game.</source>
      <extracomment>%1 is the decompression error description. Shown when the server sends a corrupt MCCP (compressed) data stream.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4868"/>
      <source>[ INFO ]  - Loading replay file:
&quot;%1&quot;.</source>
      <translation>[ INFO ] - 正在加载重播文件:
&quot;%1&quot;。</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4892"/>
      <source>Cannot replay file &quot;%1&quot;, error message was: &quot;replay file seems to be corrupt&quot;.</source>
      <translation>无法重放文件 &quot;%1&quot;，错误消息为: &quot;重放文件似乎已损坏&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4894"/>
      <source>[ WARN ]  - The replay has been aborted as the file seems to be corrupt.</source>
      <translation>[ 警告 ] - 重播已中止, 因为文件似乎已损坏。</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4903"/>
      <source>Cannot perform replay, another one may already be in progress. Try again when it has finished.</source>
      <translation>无法执行重放，另一个进程可能正在使用。 请在它完成后重试。</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4905"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress.
Try again when it has finished.</source>
      <translation>[ 警告 ] - 不能执行重播, 另一个重播可能已经在进行中，
请等它完成后再次尝试.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4913"/>
      <source>Cannot read file &quot;%1&quot;, error message was: &quot;%2&quot;.</source>
      <translation>无法读取文件 &quot;%1&quot;，错误消息为: &quot;%2&quot;。</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4915"/>
      <source>[ ERROR ] - Cannot read file &quot;%1&quot;,
error message was: &quot;%2&quot;.</source>
      <translation>[ INFO ] - 无法读取文件 &quot;%1&quot;,
错误信息是： &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="4954"/>
      <source>[  OK  ]  - The replay has ended.</source>
      <translation>[ 完成 ] - 重播已结束.</translation>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5096"/>
      <source>[ WARN  ]  - Too much data to process at once, some may have been lost.</source>
      <extracomment>Shown when too much data expands out of one compressed read (e.g. a decompression bomb) to process safely.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5658"/>
      <source>server %1</source>
      <extracomment>Telnet options report: server side of an option, %1 is &quot;enabled&quot; or &quot;disabled&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5658"/>
      <location filename="../src/ctelnet.cpp" line="5662"/>
      <source>enabled</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5658"/>
      <location filename="../src/ctelnet.cpp" line="5662"/>
      <source>disabled</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5662"/>
      <source>client %1</source>
      <extracomment>Telnet options report: client side of an option, %1 is &quot;enabled&quot; or &quot;disabled&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5665"/>
      <source>  %1: %2</source>
      <extracomment>Telnet option line: %1 is the option name (e.g. &quot;NAWS (31)&quot;), %2 is one or both sides</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5669"/>
      <source>  (none negotiated yet)
</source>
      <extracomment>Shown in the Telnet options statistics report when no options have been negotiated yet</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ctelnet.cpp" line="5703"/>
      <source>[ WARN ]  - This game appears to use character-at-a-time mode, which Mudlet does not support. Input may not work as expected. Consider using keybindings for immediate key response instead.</source>
      <extracomment>Warning shown when server uses character-at-a-time mode which Mudlet doesn&apos;t support</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>color_trigger</name>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="17"/>
      <source>ANSI 256 Color chooser</source>
      <translation>ANSI 256颜色选择器</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="40"/>
      <source>&lt;small&gt;Choose:&lt;ul&gt;&lt;li&gt;one of the basic 16 colors below&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;more&lt;/i&gt; button to get access to other colors in the 256-color set, then follow the instructions to select a color from that part of the 256 colors supported; if such a color is already in use then that part will already be showing&lt;/li&gt;
&lt;li&gt;click the &lt;i&gt;Default&lt;/i&gt; or &lt;i&gt;Ignore&lt;/i&gt; buttons at the bottom for a pair of other special cases&lt;/li&gt;
&lt;li&gt;click &lt;i&gt;Cancel&lt;/i&gt; to close this dialog without making any changes&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</source>
      <comment>Ensure that &quot;Default&quot;, &quot;Ignore&quot; and &quot;Cancel&quot; in this instruction are the same as used for the controls elsewhere on this dialog.</comment>
      <translation>&lt;small&gt;选择:&lt;ul&gt;&lt;li&gt;下列16个基本色中的一个&lt;/li&gt;
&lt;li&gt;点击 &lt;i&gt;更多&lt;/i&gt; 按钮可进入 256 色选择项, 按照接下来的指令选择256色中的一种; 如果该颜色已经被使用, 那么这一部分将会被显示出来&lt;/li&gt;
&lt;li&gt;点击 &lt;i&gt;默认&lt;/i&gt; 或 &lt;i&gt;忽略&lt;/i&gt; 按钮 将使用默认设定&lt;/li&gt;
&lt;li&gt;点击 &lt;i&gt;取消&lt;/i&gt; 将关闭该对话框并不作任何修改&lt;/li&gt;&lt;/ul&gt;&lt;/small&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="59"/>
      <source>Basic ANSI Colors [0-15] - click a button to select that color number directly:</source>
      <translation>基本 ANSI 颜色 [0-15] - 点击一个按钮直接选择该颜色编号：</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="241"/>
      <source>ANSI 6R x 6G x 6B Colors [16-231] - adjust red, green, blue and click button to select matching color number:</source>
      <translation>ANSI 6R x 6G x 6B Colors [16-231] - 调整红色，绿色，蓝色或单击按钮以选择匹配的颜色编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="253"/>
      <source>Red (0-5)</source>
      <translation>红色 (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="263"/>
      <source>Green (0-5)</source>
      <translation>绿色 (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="273"/>
      <source>Blue (0-5)</source>
      <translation>蓝色 (0-5)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="283"/>
      <source>16 + 36 x R + 6 x G + B =</source>
      <translation>16 + 36 x R + 6 x G + B =</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="344"/>
      <source>[16]</source>
      <translation>[16]</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="354"/>
      <source>Set to RGB value</source>
      <translation>设置为 RGB 值</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="364"/>
      <source>ANSI 24 Grays scale [232-255] - adjust gray and click button to select matching color number:</source>
      <translation>ANSI 24 灰色刻度 [232-255] -调整灰色，并单击按钮选择匹配的颜色编号：</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="376"/>
      <source>Gray (0-23)</source>
      <translation>灰色 (0-23)</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="383"/>
      <source>232 + Gr =</source>
      <translation>232 + Gr =</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="415"/>
      <source>[232]</source>
      <translation>[232]</translation>
    </message>
    <message>
      <location filename="../src/ui/color_trigger.ui" line="425"/>
      <source>Set to Grayscale value</source>
      <translation>设置为灰度值</translation>
    </message>
  </context>
  <context>
    <name>composer</name>
    <message>
      <location filename="../src/ui/composer.ui" line="14"/>
      <source>News and Message Composer</source>
      <translation>新闻和信息编写器</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="86"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="99"/>
      <source>&lt;p&gt;Save (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</source>
      <translation>&lt;p&gt;保存 (&lt;span style=&quot; color:#565656;&quot;&gt;Shift+Tab&lt;/span&gt;)&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/composer.ui" line="102"/>
      <source>Save</source>
      <translation>保存</translation>
    </message>
  </context>
  <context>
    <name>connection_profiles</name>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="25"/>
      <source>Select a profile to connect with</source>
      <translation>選擇設定檔進行連線</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="117"/>
      <source>profiles list</source>
      <translation>配置文件列表</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="395"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="414"/>
      <source>Copy</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="439"/>
      <source>New</source>
      <translation>新建</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="509"/>
      <source>welcome message</source>
      <translation>歡迎訊息</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="535"/>
      <source>Profile name:</source>
      <translation>設定名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="550"/>
      <source>Profile name</source>
      <translation>設定名稱</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="553"/>
      <source>A unique name for the profile but which is limited to a subset of ascii characters only.</source>
      <comment>Using lower case letters for &apos;ASCII&apos; may make speech synthesisers say &apos;askey&apos; which is quicker than &apos;Aay Ess Cee Eye Eye&apos;!</comment>
      <translation>配置文件的唯一名称，但仅限于ascii字符的子集。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="563"/>
      <source>Server address:</source>
      <translation>主機位址</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="584"/>
      <source>Game server URL</source>
      <translation>游戏服务器URL</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="587"/>
      <source>The Internet host name or IP address</source>
      <translation>服务器主机名或IP地址</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="594"/>
      <source>Port:</source>
      <translation>埠號</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="621"/>
      <source>Game server port</source>
      <translation>游戏服务器端口</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="529"/>
      <source>Connect to</source>
      <translation>連線資訊</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="624"/>
      <source>The port that is used together with the server name to make the connection to the game server. If not specified a default of 23 for &quot;Telnet&quot; connections is used. Secure connections may require a different port number.</source>
      <translation>端口需要和服务器名称一起用于连接到游戏服务器。如果未指定端口号，则使用&quot;Telnet&quot;连接的默认值23。安全连接可能需要另外的端口号。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="643"/>
      <source>Connect via a secure protocol</source>
      <translation>通过安全协议进行连接</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="646"/>
      <source>Make Mudlet use a secure SSL/TLS protocol instead of an unencrypted one</source>
      <translation>让Mudlet使用安全的SSL/TLS协议，而不是未加密的协议。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="652"/>
      <source>Secure:</source>
      <translation>安全：</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="673"/>
      <source>Options</source>
      <translation>選項</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="731"/>
      <source>Profile history:</source>
      <translation>歷史設定：</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="754"/>
      <source>load newest profile</source>
      <translation>載入最新的設定檔</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="759"/>
      <source>load oldest profile</source>
      <translation>載入最舊的設定檔</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="679"/>
      <source>Character name:</source>
      <translation>角色名稱</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="689"/>
      <source>The characters name</source>
      <translation>角色名稱</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="692"/>
      <source>Character name</source>
      <translation>角色名稱</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="695"/>
      <source>If provided will be sent, along with password to identify the user in the game.</source>
      <translation>如果提供密码的话，将会发送密码以识别游戏中的用户。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="776"/>
      <source>Auto-open profile</source>
      <translation>自動開啟設定文件</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="779"/>
      <source>Automatically start this profile when Mudlet is run</source>
      <translation>啟動 Mudlet 時，自動載入此設定文件</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="792"/>
      <source>Auto-reconnect</source>
      <translation>自動重新連接</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="795"/>
      <source>Automatically reconnect this profile if it should become disconnected for any reason other than the user disconnecting from the game server.</source>
      <translation>自动重新连接此配置文件，除非用户断开与游戏服务器的连接。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="718"/>
      <source>Password</source>
      <translation>密碼</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="721"/>
      <source>If provided will be sent, along with the character name to identify the user in the game.</source>
      <translation>如果提供用户名的话，将会发送至游戏中。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="824"/>
      <source>Information</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="881"/>
      <location filename="../src/ui/connection_profiles.ui" line="884"/>
      <source>Game description or your notes</source>
      <translation>游戏描述或注释</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="705"/>
      <source>Password:</source>
      <translation>密碼：</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="715"/>
      <source>Characters password. Note that the password isn&apos;t encrypted in storage</source>
      <translation>角色密碼。注意！密碼將以明文儲存。</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="773"/>
      <source>With this enabled, Mudlet will automatically start and connect on this profile when it is launched</source>
      <translation>啟用之後，每次 Mudlet 啟動時都會自動載入這份設定檔並進行連線</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="785"/>
      <source>Open profile on Mudlet start</source>
      <translation>啟動 Mudlet 時載入此設定</translation>
    </message>
    <message>
      <location filename="../src/ui/connection_profiles.ui" line="801"/>
      <source>Reconnect automatically</source>
      <translation>自動重新連線</translation>
    </message>
  </context>
  <context>
    <name>custom_line_properties</name>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="27"/>
      <source>Custom Line Properties [*]</source>
      <translation>自定义路径线属性 [*]</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="47"/>
      <source>Line Settings:</source>
      <translation>路径线设置:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="85"/>
      <source>Color:</source>
      <translation>颜色：</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="59"/>
      <source>Style:</source>
      <translation>样式:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="44"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head.&lt;/p&gt;</source>
      <translation>&lt;p&gt;选择样式，颜色以及是否使用箭头结束一行。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="123"/>
      <source>Ends with an arrow:</source>
      <translation>以箭头结尾:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="139"/>
      <source>Exit Details:</source>
      <translation>出口细节:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="154"/>
      <source>Origin:</source>
      <translation>起点:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="227"/>
      <source>Destination:</source>
      <translation>目的地:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines_properties.ui" line="189"/>
      <source>    Direction/Command:</source>
      <translation>    方向/命令:</translation>
    </message>
  </context>
  <context>
    <name>custom_lines</name>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="14"/>
      <source>Custom Line selection</source>
      <translation>自定义路径线选区:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="45"/>
      <source>Choose line format, color and arrow option and then select the exit to start drawing</source>
      <translation>选择路径线的格式, 颜色和箭头选项然后选择出口开始绘图</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="64"/>
      <source>Line Settings:</source>
      <translation>路径线设置:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="172"/>
      <source>Ends with an arrow:</source>
      <translation>以箭头结尾:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="76"/>
      <source>Style:</source>
      <translation>样式:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="118"/>
      <source>Color:</source>
      <translation>顏色</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="48"/>
      <source>&lt;p&gt;Selecting an exit immediately proceeds to drawing the first line segment from the centre point of the room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;选择一个出口会立即从房间中心绘制出第一条线段.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="61"/>
      <source>&lt;p&gt;Select Style, Color and whether to end the line with an arrow head BEFORE then choosing the exit to draw the line for...&lt;/p&gt;</source>
      <translation>&lt;p&gt;选择样式, 颜色以及是否在线条结尾前显示箭头, 然后选择退出以绘制线条...&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="188"/>
      <source>&lt;p&gt;Select a normal exit to commence drawing a line for it, buttons are shown depressed if they already have such a custom line and disabled if there is not exit in that direction.&lt;/p&gt;</source>
      <translation>&lt;p&gt;选择一个出口并开始为它绘制一条路径, 如果这些出口已经有这样的自定义路径了, 按钮会显示为按下状态, 如果该方向没有出口, 按钮会显示为不可用状态.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="191"/>
      <source>Normal Exits:</source>
      <translation>普通出口:</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="213"/>
      <source>NW</source>
      <translation>西北</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="236"/>
      <source>N</source>
      <translation>北</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="259"/>
      <source>NE</source>
      <translation>东北</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="298"/>
      <source>UP</source>
      <translation>上</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="337"/>
      <source>W</source>
      <translation>西</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="347"/>
      <source>E</source>
      <translation>东</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="357"/>
      <source>IN</source>
      <translation>进入</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="383"/>
      <source>OUT</source>
      <translation>出去</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="393"/>
      <source>SW</source>
      <translation>西南</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="403"/>
      <source>S</source>
      <translation>南</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="413"/>
      <source>SE</source>
      <translation>东南</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="423"/>
      <source>DOWN</source>
      <translation>下</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="445"/>
      <source>&lt;p&gt;Select a special exit to commence drawing a line for it, the first column is checked if the exit already has such a custom line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;选择一个特殊的出口并开始绘制一条路径, 如果一个出口已经拥有了这样一条自定义路径, 第一列会显示为勾选状态.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="504"/>
      <source>&lt;p&gt;Indicates if there is already a custom line for this special exit, will be replaced if the exit is selected.&lt;/p&gt;</source>
      <translation>&lt;p&gt;指明该特殊出口是否已经拥有一条自定义路径, 选择该出口后自定义路径会被替换.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="515"/>
      <source>&lt;p&gt;The room this special exit leads to.&lt;/p&gt;</source>
      <translation>&lt;p&gt;此房間的特殊出口通往&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="526"/>
      <source>&lt;p&gt;The command or LUA script that goes to the given room.&lt;/p&gt;</source>
      <translation>&lt;p&gt;通往指定房間的命令或 Lua 腳本。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="562"/>
      <source>&lt;p&gt;To remove a custom line: cancel this dialog, select the line and right-click to obtain a &amp;quot;delete&amp;quot; option.&lt;/p&gt;</source>
      <translation>&lt;p&gt;删除自定义路径线, 请取消此对话框, 
选择要删除的路径线并右键单击以获取 "删除" 选项。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="448"/>
      <source>Special Exits:</source>
      <translation>特殊出口：</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="500"/>
      <source>Has
custom line?</source>
      <translation>已有
自定义路径?</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="512"/>
      <source> Destination </source>
      <translation> 目的地 </translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="523"/>
      <source> Command</source>
      <translation> 命令</translation>
    </message>
    <message>
      <location filename="../src/ui/custom_lines.ui" line="568"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
  </context>
  <context>
    <name>dblsqd::Feed</name>
    <message>
      <location filename="../src/updater/Feed.cpp" line="142"/>
      <source>Update check already in progress</source>
      <extracomment>Error shown when the user triggers an update check while one is already running</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="170"/>
      <source>No download available for your platform</source>
      <extracomment>Error shown when the GitHub release has no binary matching the user&apos;s operating system</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="182"/>
      <source>This update does not publish the checksums needed to verify it. Please try again later, or download it from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the release publishes no checksums at all, so the download cannot be verified as safe to install</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="243"/>
      <source>Could not download the checksums needed to verify this update. Please try again later.</source>
      <extracomment>Error shown when the checksums needed to verify the update could not be downloaded</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="261"/>
      <source>The checksums for this update could not be read, so it cannot be verified. Please try again later.</source>
      <extracomment>Error shown when the checksum file for the update was downloaded but could not be read</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="265"/>
      <source>This update is missing a checksum for your platform, so it cannot be verified. Please try again later, or download it from https://www.mudlet.org/download/</source>
      <extracomment>Error shown when the release publishes checksums but none of them cover this platform&apos;s download</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="309"/>
      <source>Could not connect to the update server: %1</source>
      <extracomment>Error shown when the network request to the update server fails. %1 is the technical error description.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="323"/>
      <location filename="../src/updater/Feed.cpp" line="345"/>
      <source>Could not read update information from the server</source>
      <extracomment>Error shown when the server response cannot be understood
----------
Error shown when the update server response cannot be understood</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="334"/>
      <source>Update check temporarily unavailable. Please try again in a few minutes.</source>
      <extracomment>Error shown when the GitHub API rate limit has been exceeded</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="337"/>
      <source>Could not check for updates: %1</source>
      <extracomment>Error shown when the GitHub API returns an error. %1 is the error message from the server.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="391"/>
      <source>Could not create temporary file for download: %1</source>
      <extracomment>Error shown when a temporary file cannot be created for the update download. %1 is the system error message.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="401"/>
      <source>Failed to save download data: %1</source>
      <extracomment>Error shown when writing download data to disk fails. %1 is the system error message.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="412"/>
      <source>Download failed: %1</source>
      <extracomment>Error shown when the update file download fails. %1 is the network error message.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="420"/>
      <source>Download failed. Please try again.</source>
      <extracomment>Error shown when the update download completed but nothing was received</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="428"/>
      <source>Failed to save download: %1</source>
      <extracomment>Error shown when flushing the downloaded file to disk fails. %1 is the system error message.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="436"/>
      <location filename="../src/updater/Feed.cpp" line="445"/>
      <source>Failed to verify download integrity</source>
      <extracomment>Error shown when the downloaded file cannot be read back for checksum verification</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/Feed.cpp" line="454"/>
      <source>Could not verify download integrity.</source>
      <extracomment>Error shown when the downloaded file&apos;s SHA256 checksum does not match the expected value</extracomment>
      <translation>無法驗證下載完整性。</translation>
    </message>
  </context>
  <context>
    <name>dblsqd::UpdateDialog</name>
    <message>
      <location filename="../src/updater/UpdateDialog.cpp" line="597"/>
      <source>Could not open the downloaded update. You can try opening it manually:
%1</source>
      <extracomment>Error shown when the downloaded update file cannot be opened for installation. %1 is the file path.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/UpdateDialog.cpp" line="662"/>
      <source>Could not check for updates</source>
      <extracomment>Label shown in the update dialog when the update check fails due to a network or server error</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/UpdateDialog.cpp" line="677"/>
      <source>Download failed. Please try again.</source>
      <extracomment>Error shown when the download finished but no file was saved</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/UpdateDialog.cpp" line="700"/>
      <source>Download Error</source>
      <extracomment>Title for the download error warning dialog</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/updater/UpdateDialog.cpp" line="702"/>
      <source>There was an error while downloading the update.</source>
      <extracomment>Message shown in the download error warning dialog, followed by the specific error details</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>delete_profile_confirmation</name>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="14"/>
      <source>Confirm permanent profile deletion</source>
      <translation>確認永久刪除設定檔</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="26"/>
      <source>Are you sure that you&apos;d like to delete this profile? Everything (aliases, triggers, backups, etc) will be gone.

If you are, please type in the profile name as a confirmation:</source>
      <translation>你確定要刪除這份設定檔嗎？包括別名、觸發與備份的一切內容都會消失。

如果是，請輸入設定檔的名稱作為確認：</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="77"/>
      <source>Delete</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../src/ui/delete_profile_confirmation.ui" line="67"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
  </context>
  <context>
    <name>dialog</name>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="39"/>
      <source>Status</source>
      <translation>状态</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="44"/>
      <source>Symbol
(Set Font)</source>
      <translation>符號
（設定字型）</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="50"/>
      <source>Symbol
(All Fonts)</source>
      <translation>符號
（所有字型）</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="56"/>
      <source>Codepoints</source>
      <translation>编码</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="61"/>
      <source>Usage
Count</source>
      <translation>使用次数</translation>
    </message>
    <message>
      <location filename="../src/ui/glyph_usage.ui" line="67"/>
      <source>Rooms</source>
      <translation>房间</translation>
    </message>
  </context>
  <context>
    <name>directions</name>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6147"/>
      <source>north</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>北</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6149"/>
      <source>n</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>n</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6151"/>
      <source>east</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>東</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6153"/>
      <source>e</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>e</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6155"/>
      <source>south</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>南</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6157"/>
      <source>s</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>s</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6159"/>
      <source>west</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>西</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6161"/>
      <source>w</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>w</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6163"/>
      <source>northeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>東北</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6165"/>
      <source>ne</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>ne</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6167"/>
      <source>southeast</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>東南</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6169"/>
      <source>se</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>se</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6171"/>
      <source>southwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>西南</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6173"/>
      <source>sw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>sw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6175"/>
      <source>northwest</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>西北</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6177"/>
      <source>nw</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>nw</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6179"/>
      <source>in</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>入口</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6181"/>
      <source>i</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>i</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6183"/>
      <source>out</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>出口</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6185"/>
      <source>o</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>o</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6187"/>
      <source>up</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>上</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6189"/>
      <source>u</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>u</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6191"/>
      <source>down</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>下</translation>
    </message>
    <message>
      <location filename="../src/TLuaInterpreter.cpp" line="6193"/>
      <source>d</source>
      <comment>Entering this direction will move the player in the game</comment>
      <translation>d</translation>
    </message>
  </context>
  <context>
    <name>dlgAboutDialog</name>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="143"/>
      <source>&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Homepage&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://www.mudlet.org/&quot;&gt;www.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Forums&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://forums.mudlet.org/&quot;&gt;forums.mudlet.org&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Documentation&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://wiki.mudlet.org/w/Main_Page&quot;&gt;wiki.mudlet.org/w/Main_Page&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#7289DA;&quot;&gt;&lt;b&gt;Discord&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://www.mudlet.org/chat&quot;&gt;discord.gg&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Source code&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet&quot;&gt;github.com/Mudlet/Mudlet&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;Features/bugs&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/issues&quot;&gt;github.com/Mudlet/Mudlet/issues&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;</source>
      <translation>&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;官方網站&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://www.mudlet.org/&quot;&gt;http://www.mudlet.org/&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;線上論壇&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://forums.mudlet.org/&quot;&gt;http://forums.mudlet.org/&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;使用文件&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;http://wiki.mudlet.org/w/Main_Page&quot;&gt;http://wiki.mudlet.org/w/Main_Page/&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#7289DA;&quot;&gt;&lt;b&gt;Discord&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://www.mudlet.org/chat&quot;&gt;discord.gg&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;原始代碼&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet&quot;&gt;https://github.com/Mudlet/Mudlet/&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;
&lt;tr&gt;&lt;td&gt;&lt;span style=&quot;color:#40b040;&quot;&gt;&lt;b&gt;錯誤回報&lt;/b&gt;&lt;/span&gt;&lt;/td&gt;&lt;td&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/issues&quot;&gt;https://github.com/Mudlet/Mudlet/issues&lt;/a&gt;&lt;/td&gt;&lt;/tr&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="278"/>
      <source>&lt;p&gt;Others too, have make their mark on different aspects of the Mudlet project and if they have not been mentioned here it is by no means intentional! For past contributors you may see them mentioned in the &lt;b&gt;&lt;a href=&quot;https://launchpad.net/~mudlet-makers/+members#active&quot;&gt;Mudlet Makers&lt;/a&gt;&lt;/b&gt; list (on our former bug-tracking site), or for on-going contributors they may well be included in the &lt;b&gt;&lt;a href=&quot;https://github.com/Mudlet/Mudlet/graphs/contributors&quot;&gt;Contributors&lt;/a&gt;&lt;/b&gt; list on GitHub.&lt;/p&gt;
&lt;br&gt;
&lt;p&gt;Many icons are taken from the &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;&lt;u&gt;KDE4 oxygen icon theme&lt;/u&gt;&lt;/b&gt;&lt;/span&gt; at &lt;a href=&quot;https://web.archive.org/web/20130921230632/http://www.oxygen-icons.org/&quot;&gt;www.oxygen-icons.org &lt;sup&gt;{wayback machine archive}&lt;/sup&gt;&lt;/a&gt; or &lt;a href=&quot;http://www.kde.org&quot;&gt;www.kde.org&lt;/a&gt;.  Most of the rest are from Thorsten Wilms, or from Stephen Lyons combining bits of Thorsten&apos;s work with the other sources.&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Brett Duzevich&lt;/b&gt;&lt;/span&gt; and &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Ronny Ho&lt;/b&gt;&lt;/span&gt;. They have contributed many good ideas and thus helped improve the scripting framework substantially.&lt;/p&gt;
&lt;p&gt;Thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Tomas Mecir&lt;/b&gt;&lt;/span&gt; (&lt;span style=&quot;color:#0000ff;&quot;&gt;kmuddy@kmuddy.com&lt;/span&gt;) who brought us all together and inspired us with his KMuddy project. Mudlet is using some of the telnet code he wrote for his KMuddy project (&lt;a href=&quot;https://cgit.kde.org/kmuddy.git/&quot;&gt;cgit.kde.org/kmuddy.git/&lt;/a&gt;).&lt;/p&gt;
&lt;p&gt;Special thanks to &lt;span style=&quot;color:#bc8942;&quot;&gt;&lt;b&gt;Nick Gammon&lt;/b&gt;&lt;/span&gt; (&lt;a href=&quot;http://www.gammon.com.au/mushclient/mushclient.htm&quot;&gt;www.gammon.com.au/mushclient/mushclient.htm&lt;/a&gt;) for giving us some valued pieces of advice.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="321"/>
      <source>&lt;p&gt;Mudlet was originally written by Heiko Köhn, KoehnHeiko@googlemail.com.&lt;/p&gt;
&lt;p&gt;Mudlet is released under the GPL license version 2, which is reproduced below:&lt;/p&gt;</source>
      <comment>For non-english language versions please append a translation of the following to explain why the GPL is NOT reproduced in the relevant language: &apos;but only the English form is considered the official version of the license, so the following is reproduced in that language:&apos; to replace &apos;which is reproduced below:&apos;...</comment>
      <translation>&lt;p&gt;Mudlet 最初由Heiko Köhn、KoenhnHeiko@googlemail.com编写。&lt;/p&gt;
&lt;p&gt;Mudlet 在 GPL 许可证版本2 下发布，下面是转载：&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="617"/>
      <source>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; is built upon the shoulders of other projects in the FOSS world; as well as using many GPL components we also make use of some third-party software with other licenses:&lt;/p&gt;</source>
      <translation>&lt;p align=&quot;center&quot;&gt;&lt;b&gt;Mudlet&lt;/b&gt; 是建立在自由开放源码软件世界其他项目的肩膀上的；以及使用许多 GPL 组件，我们还使用一些第三方软件和其他许可证：&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="859"/>
      <source>&lt;h2&gt;&lt;u&gt;Communi IRC Library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2008-2020 The Communi Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="862"/>
      <source>&lt;p&gt;Parts of &lt;tt&gt;irctextformat.cpp&lt;/t&gt; code come from Konversation and are copyrighted to:&lt;br&gt;Copyright © 2002 Dario Abatianni &amp;lt;eisfuchs@tigress.com&amp;gt;&lt;br&gt;Copyright © 2004 Peter Simonsson &amp;lt;psn@linux.se&amp;gt;&lt;br&gt;Copyright © 2006-2008 Eike Hein &amp;lt;hein@kde.org&amp;gt;&lt;br&gt;Copyright © 2004-2009 Eli Mackenzie &amp;lt;argonel@gmail.com&amp;gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="868"/>
      <source>&lt;h2&gt;&lt;u&gt;Lua - Lua 5.1&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1994–2017 Lua.org, PUC-Rio.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="871"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaFileSystem&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2003-2020, Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="874"/>
      <source>&lt;h2&gt;&lt;u&gt;Lua_yajl - Lua 5.1 interface to yajl&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Brian Maher &amp;lt;maherb at brimworks dot com&amp;gt;&lt;br&gt;Copyright © 2009 Brian Maher&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="878"/>
      <source>&lt;h2&gt;&lt;u&gt;Luautf8 - A UTF-8 support module for Lua.&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2018 Xavier Wang&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Luautf8 - 一个支持Lua的UTF-8模块。&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2018 Xavier Wang&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="881"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaSql-Sqlite3 - Database connectivity for the Lua programming language (Sqlite3 component).&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2003-2019, The Kepler Project&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="895"/>
      <source>&lt;h2&gt;&lt;u&gt;Edbee - multi-feature editor widget&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2012-2014 by Reliable Bits Software by Blommers IT&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="913"/>
      <source>&lt;h2&gt;&lt;u&gt;Qt-Components, QsLog&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;(&lt;span style=&quot;color:red&quot;&gt;&lt;u&gt;https://bitbucket.org/razvapetru/qt-components [broken link]&lt;/u&gt;&lt;/span&gt;&lt;/h3&gt;&lt;h3&gt;&lt;small&gt;&lt;a href=&quot;https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components&quot;&gt; {&amp;quot;Wayback Machine&amp;quot; archived version}&lt;/a&gt;&lt;/small&gt;)&lt;br&gt;Copyright © 2013, Razvan Petru&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="949"/>
      <source>&lt;h2&gt;&lt;u&gt;singleshot_connect.h - part of KDToolBox&lt;/u&gt;&lt;br&gt;Github: &lt;a href=&quot;https://github.com/KDAB/KDToolBox&quot;&gt;KDToolBox&lt;/a&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2020-2021 Klarälvdalens Datakonsult AB, a KDAB Group company, &amp;lt;info@kdab.comF&amp;gt;.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="953"/>
      <source>&lt;h2&gt;&lt;u&gt;utf8_filenames.lua - modifies standard Lua functions so that they work with UTF-8 filenames on Windows&lt;/u&gt;&lt;br&gt;&lt;a href=&quot;https://gist.github.com/Egor-Skriptunoff/2458547aa3b9210a8b5f686ac08ecbf0&quot;&gt;Github GIST&lt;/a&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2019 Egor-Skriptunoff&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;utf8_filenames.lua - 修改标准的Lua函数，使它们能够在Windows上使用UTF-8文件名&lt;/u&gt;&lt;br&gt;&lt;a href=&quot;https://gist.github.com/Egor-Skriptunoff/2458547aa3b9210a8b5f686ac08ecbf0&quot;&gt;Github GIST&lt;/a&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2019 Egor-Skriptunoff&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="889"/>
      <source>&lt;h2&gt;&lt;u&gt;LuaZip - Reading files inside zip files&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Author: Danilo Tuler&lt;br&gt;Copyright © 2003-2007 Kepler Project&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;LuaZip - 在zip 文件中读取文件&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;作者：Danilo Tuler&lt;br&gt;版权所有 © 2003-2007 Kepler项目&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="153"/>
      <source>Original author, original project lead, Mudlet core coding, retired.</source>
      <extracomment>about:Heiko</extracomment>
      <translation>原作者，原始项目牵头人，Mudlet核心编码已经退出。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="156"/>
      <source>GUI design and initial feature planning. He is responsible for the project homepage and the user manual. Maintainer of the Windows, macOS, Ubuntu and generic Linux installers. Maintains the Mudlet wiki, Lua API, and handles project management, public relations &amp;amp; user help. With the project from the very beginning and is an official spokesman of the project. Since the retirement of Heiko, he has become the head of the Mudlet project.</source>
      <extracomment>about:Vadi</extracomment>
      <translation>GUI 设计和初始功能规划。 他负责项目主页和用户手册。 Windows ， macOS， Ubuntu 和通用 Linux 安装程序的维护人员。 维护 Mudlet Wiki Lua API ，并处理项目管理，公共关系 &amp; 用户帮助。 从一开始就有这个项目，是项目的官方发言人。 自从 Heiko 退休后，他就成为了 Mudlet 项目的负责人。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="163"/>
      <source>After joining in 2013, he has been poking various bits of the C++ code and GUI with a pointy stick; subsequently trying to patch over some of the holes made/found. Most recently he has been working on I18n and L10n for Mudlet 4.0.0 so if you are playing Mudlet in a language other than American English you will be seeing the results of him getting fed up with the spelling differences between what was being used and the British English his brain wanted to see.</source>
      <extracomment>about:SlySven</extracomment>
      <translation>在2013年加入后，他一直用 C++ 代码和 GUI 进行升级优化，随后尝试对所发现/发现的一些漏洞进行修补。 最近，他一直在为 Mudlet 4.0.0 在 I18n 和 L10n 工作，所以如果你以美国英语以外的语言播放 Mudlet ，那么你将看到他在不同语言之间的拼写差异的成果。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="170"/>
      <source>Former maintainer of the early Windows and Apple OSX packages. He also administers our server and helps the project in many ways.</source>
      <extracomment>about:demonnic</extracomment>
      <translation>早期的 Windows 和 Apple OSX 软件包的维护人员。 他还管理我们的服务器并以多种方式帮助项目。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="174"/>
      <source>Contributed many improvements to Mudlet&apos;s db: interface, event system, and has been around the project for a very long while assisting users.</source>
      <extracomment>about:keneanung</extracomment>
      <translation>对 Mudlet&apos;s db 做出了许多改进：界面，事件系统，并且一直陪伴该项目很长时间，同时协助用户。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="178"/>
      <source>Does a ton of work in making Mudlet, the website and the wiki accessible to you regardless of the language you speak - and promoting our genre!</source>
      <extracomment>about:Leris</extracomment>
      <translation>做了大量的工作，使 Mudlet，网站和WiKi 无论你说什么语言，你都可以访问，并推广我们的流派！</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="182"/>
      <source>Joined in 2020, reworking much of the 2D mapper and adding many Lua API features. Outside the client they build Mudlet Web, the documentation extract that powers autocompletion in code editors, and the tools that share Mudlet maps online.</source>
      <extracomment>about:Delwing</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="187"/>
      <source>Joined in 2023 and works across the whole client - script editor, preferences, package manager and mapper - along with many Lua API additions. Wrote the Mudlet Tutorial profile and maintains the Mudlet package repository.</source>
      <extracomment>about:Zooka</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="192"/>
      <source>Contributions to the Travis integration, CMake and Visual C++ build, a lot of code quality and memory management improvements.</source>
      <extracomment>about:ahmedcharles</extracomment>
      <translation>对 Travis 集成， CMake 和 Visual C++ 构建的贡献，许多代码质量和内存管理改进。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="196"/>
      <source>Developed a shared module system that allows script packages to be shared among profiles, a UI for viewing Lua variables, improvements in the mapper and all around.</source>
      <extracomment>about:Chris7</extracomment>
      <translation>开发了一个共享模块系统，允许在概要文件之间共享脚本包，用于查看 Lua 变量的 UI ，以及映射器及其周围的改进。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="200"/>
      <source>Developed the first version of our Mac OSX installer. He is the former maintainer of the Mac version of Mudlet.</source>
      <extracomment>about:Ben Carlsen</extracomment>
      <translation>开发了我们的 Mac OSX 安装程序的第一个版本。 他是 Mudlet Mac 版本的前维护者。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="204"/>
      <source>Joined in December 2009 though he&apos;s been around much longer. Contributed to the Lua API and is the former maintainer of the Lua API.</source>
      <extracomment>about:Ben Smith</extracomment>
      <translation>2009 年 12 月加入，他加入的时间更长。 为 Lua API 提供了帮助，并且是 Lua API 的前维护人员。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="208"/>
      <source>Joined in December 2009. He has contributed to the Lua API, submitted small bugfix patches and has helped with release management of 1.0.5.</source>
      <extracomment>about:Blaine von Roeder</extracomment>
      <translation>2009年 12 月加入。 他向 Lua API 提供了帮助，提交了少量的 bugfix 补丁，并帮助发布了 1.0.5的发布管理。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="212"/>
      <source>Developed the original cmake build script and he has committed a number of patches.</source>
      <extracomment>about:Bruno Bigras</extracomment>
      <translation>开发了最初的 cmake 构建脚本，他已经提交了一些补丁。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="215"/>
      <source>Contributed to the Lua API.</source>
      <extracomment>about:Carter Dewey</extracomment>
      <translation>为 Lua API 做出了贡献。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="218"/>
      <source>Developed the Vyzor GUI Manager for Mudlet.</source>
      <extracomment>about:Oneymus</extracomment>
      <translation>为 Mudlet 开发 Vyzor GUI Manager 。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="221"/>
      <source>Brought the 3D mapper back to life with camera controls, lighting and proper geometry for z-squished rooms, and has fixed a number of console and command line annoyances.</source>
      <extracomment>about:Harrison</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="225"/>
      <source>Worked wonders in rejuvenating our Website in 2017 but who prefers a little anonymity - if you are a &lt;i&gt;SpamBot&lt;/i&gt; you will not get onto our Fora now. They have also made some useful C++ core code contributions and we look forward to future reviews on and work in that area.</source>
      <extracomment>about:TheFae</extracomment>
      <translation>2017 ，我们的网站重新焕发活力，但更喜欢匿名的人 -- 如果你是 &lt;i&gt;Spambot&lt;/i&gt; ，你就不会上我们的 Fora 了。 他们还提供了一些有用的 C++ 核心代码贡献，我们期待未来对该领域的审查和工作。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="230"/>
      <source>Joining us 2017 they have given us some useful C++ and Lua contributions.</source>
      <extracomment>about:Dicene</extracomment>
      <translation>于2017 年加入我们，他们为我们提供了一些有用的 C++ 和 Lua 的贡献。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="233"/>
      <source>Contributed the Geyser layout manager for Mudlet in March 2010. It is written in Lua and aims at simplifying user GUI scripting.</source>
      <extracomment>about:James Younquist</extracomment>
      <translation>2010 年 3 月为 Mudlet 提供了 Geyser 布局经理。 它是用 Lua 编写的，旨在简化用户 GUI 脚本编制。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="237"/>
      <source>Helped develop and debug the Lua API.</source>
      <extracomment>about:John Dahlström</extracomment>
      <translation>帮助开发和调试 Lua API。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="240"/>
      <source>Implemented MMCP, so Mudlet can join MudMaster chat networks, and has contributed a range of console and Lua API fixes.</source>
      <extracomment>about:John McKisson</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="244"/>
      <source>Contributed several improvements and new features for Geyser.</source>
      <extracomment>about:Beliaar</extracomment>
      <translation>为 Geyser 提供了一些改进和新功能。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="247"/>
      <source>The original author of our Windows installer.</source>
      <extracomment>about:Leigh Stillard</extracomment>
      <translation>Windows 安装程序的原始作者。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="250"/>
      <source>Worked on the manual, forum help and helps with GUI design and documentation.</source>
      <extracomment>about:Maksym Grinenko</extracomment>
      <translation>致力于说明手册、论坛帮助和 GUI 设计和文档帮助。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="253"/>
      <source>Built much of the GUI toolkit you script with between 2020 and 2022: Adjustable Containers, Geyser&apos;s ScrollBox, animated labels and Geyser in UserWindows - plus the dark theme toggle and the Package Exporter rework.</source>
      <extracomment>about:Edru2</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="258"/>
      <source>Joined in 2018 and looks after nearly everything Mudlet plays or negotiates - MCMP media, sound and video, closed captioning, MXP, OSC 8 hyperlinks and text encodings - plus multi-window support with drag-and-drop tabs.</source>
      <extracomment>about:Mike Conley</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="263"/>
      <source>Developed a database Lua API that allows for far easier use of databases and one of the original OSX installers.</source>
      <extracomment>about:Stephen Hansen</extracomment>
      <translation>开发了一个数据库 Lua API，允许更容易使用数据库和原始 OSX 安装程序之一。</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="266"/>
      <source>Designed our beautiful logo, our splash screen, the about dialog, our website, several icons and badges. Visit his homepage at &lt;a href=&quot;http://thorwil.wordpress.com/&quot;&gt;thorwil.wordpress.com&lt;/a&gt;.</source>
      <extracomment>about:Thorsten Wilms</extracomment>
      <translation>为我们设计了美丽的标志，我们启动画面，关于对话，我们的网站，几个图标和徽章。 您可以访问他的主页 &lt;a href=&quot;http://thorwil.wordpress.com/&quot;&gt;thorwil.wordpress.com&lt;/a&gt;.</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="270"/>
      <source>Joined in 2020 and made Mudlet work far better with screen readers, alongside secure IRC connections, Discord improvements, and a batch of editor shortcuts and Lua configuration functions.</source>
      <extracomment>about:Tim Johnson</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="884"/>
      <source>&lt;h2&gt;&lt;u&gt;Lrexlib-pcre2 -  Regular expression library binding (PCRE2 flavour).&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © Reuben Thomas 2000-2020&lt;br&gt;Copyright © Shmuel Zeigerman 2004-2020 &lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="898"/>
      <source>The &lt;b&gt;edbee-lib&lt;/b&gt; widget itself incorporates other components with licences that must be noted as well, they are:</source>
      <translation>&lt;b&gt;edbee-lib&lt;/b&gt; 窗口小部件本身包含其他必须注意的许可证组件，它们是:</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="900"/>
      <source>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Onigmo (Oniguruma-mod) LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;Copyright © 2011-2014 K.Takata &amp;lt;kentkt AT csc DOT jp&amp;gt;&lt;br&gt;保留所有权利.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="905"/>
      <source>&lt;h2&gt;&lt;u&gt;Oniguruma LICENSE&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Oniguruma 许可证&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;版权所有 © 2002-2009 K.Kosako &amp;lt;sndgk393 AT ybb DOT ne DOT jp&amp;gt;&lt;br&gt;保留所有权利。&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="909"/>
      <source>&lt;h2&gt;&lt;u&gt;Ruby BSDL&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 1993-2013 Yukihiro Matsumoto.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="920"/>
      <source>&lt;h2&gt;&lt;u&gt;Dblsqd (derived work)&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Philipp Medien&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="923"/>
      <source>&lt;h2&gt;&lt;u&gt;Sparkle - macOS updater&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Sparkle - macOS升级程序&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2006-2013 Andy Matuschak.&lt;br&gt;Copyright © 2009-2013 Elgato Systems GmbH.&lt;br&gt;Copyright © 2011-2014 Kornel Lesiński.&lt;br&gt;Copyright © 2015-2017 Mayur Pawashe.&lt;br&gt;Copyright © 2014 C.W. Betts.&lt;br&gt;Copyright © 2014 Petroules Corporation.&lt;br&gt;Copyright © 2014 Big Nerd Ranch.&lt;br&gt;保留所有权利。&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="933"/>
      <source>&lt;h4&gt;bspatch.c and bsdiff.c, from bsdiff 4.3 &lt;a href=&quot;http://www.daemonology.net/bsdiff/&quot;&gt;http://www.daemonology.net/bsdiff&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2003-2005 Colin Percival.&lt;/h3&gt;&lt;h4&gt;sais.c and sais.c, from sais-lite (2010/08/07) &lt;a href=&quot;https://sites.google.com/site/yuta256/sais&quot;&gt;https://sites.google.com/site/yuta256/sais&lt;/a&gt;:&lt;/h4&gt;&lt;h3&gt;Copyright © 2008-2010 Yuta Mori.&lt;/h3&gt;&lt;h4&gt;SUDSAVerifier.m:&lt;/h4&gt;&lt;h3&gt;Copyright © 2011 Mark Hamlin.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="943"/>
      <source>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2017 Discord, Inc.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;Discord - Rich Presence - RPC library&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;版权所有 © 2017 Discord, Inc.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="946"/>
      <source>&lt;h2&gt;&lt;u&gt;QtKeyChain - Platform-independent Qt API for storing passwords securely&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</source>
      <translation>&lt;h2&gt;&lt;u&gt;QtKeyChain - Platform-independent Qt API 用于安全的存储密码&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2011-2019 Frank Osterfeld &amp;lt;frank.osterfeld@gmail.com&amp;gt;.&lt;/h3&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="958"/>
      <source>&lt;h2&gt;&lt;u&gt;Sentry Native - Crash reporting SDK&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Copyright © 2019 Sentry (https://sentry.io) and individual contributors.&lt;br&gt;All rights reserved.&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1069"/>
      <source>&lt;h2&gt;&lt;u&gt;Sword 3D Model&lt;/u&gt;&lt;/h2&gt;&lt;h3&gt;Model obtained from &lt;a href=&quot;https://sketchfab.com/3d-models/sword-07463a2658e04d6ab8a42b5639a35d63&quot;&gt;Sketchfab&lt;/a&gt;&lt;br&gt;Author: &lt;a href=&quot;https://sketchfab.com/minghau&quot;&gt;minghauLoh&lt;/a&gt;&lt;br&gt;Licensed under &lt;a href=&quot;https://creativecommons.org/licenses/by/4.0/&quot;&gt;CC BY 4.0&lt;/a&gt;&lt;/h3&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1134"/>
      <source>
                            These formidable folks will be fondly remembered forever&lt;br&gt;for their generous financial support on Mudlet&apos;s patreon:
                            </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1143"/>
      <source>
                            These formidable folks will be fondly remembered forever&lt;br&gt;for their generous financial support on &lt;a href=&quot;https://www.patreon.com/mudlet&quot;&gt;Mudlet&apos;s patreon&lt;/a&gt;:
                            </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1166"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1194"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1217"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1245"/>
      <source>Technical information:</source>
      <translation>技术信息:</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1167"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1195"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1218"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1246"/>
      <source>Version</source>
      <translation>版本</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1169"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1197"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1220"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1248"/>
      <source>OS</source>
      <translation>操作系统</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1223"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1251"/>
      <source>CPU</source>
      <extracomment>This is shown for all other OSes than Windows.</extracomment>
      <translation>中央處理器</translation>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1177"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1228"/>
      <source>Qt version (compilation)</source>
      <extracomment>This is shown when the Qt version used at run-time is different to that used during compilation - it is not the usual case.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1171"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1199"/>
      <source>CPU (64-bits)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1182"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1233"/>
      <source>Qt version (run-time)</source>
      <extracomment>This is shown when the Qt version used at run-time is different to that used during compilation - it is not the usual case.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgAboutDialog.cpp" line="1204"/>
      <location filename="../src/dlgAboutDialog.cpp" line="1255"/>
      <source>Qt version</source>
      <extracomment>This is shown when the same Qt version is used at run-time as was used during compilation - it is the usual case.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgActionMainArea</name>
    <message>
      <location filename="../src/dlgActionMainArea.cpp" line="87"/>
      <source>Number of columns:</source>
      <extracomment>A toolbar is being set to vertical orientation - so multiple rows of this number of columns</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgActionMainArea.cpp" line="90"/>
      <source>Number of rows:</source>
      <extracomment>A toolbar is being set to horizontal orientation - so multiple columns of this number of rows</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgAliasMainArea</name>
    <message>
      <location filename="../src/dlgAliasMainArea.cpp" line="37"/>
      <source>for example, ^myalias$ to match &apos;myalias&apos;</source>
      <extracomment>This text is shown as placeholder in the pattern box when no real pattern was entered, yet.</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgColorTrigger</name>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="50"/>
      <source>More colors</source>
      <translation>更多顏色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="52"/>
      <source>Click to access all 256 ANSI colors.</source>
      <translation>点击访问所有 256 种 ANSI 颜色。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="63"/>
      <source>Default</source>
      <translation>預設</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="58"/>
      <source>Click to make the color trigger ignore the text&apos;s background color - however choosing this for both foreground and background is an error.</source>
      <translation>点击新增一个忽略文本背景色的颜色触发器 -- 但是, 当它同时作用于背景色和前景色时将引发错误。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="59"/>
      <source>Click to make the color trigger ignore the text&apos;s foreground color - however choosing this for both foreground and background is an error.</source>
      <translation>点击新增一个忽略文本前景色的颜色触发器 -- 但是, 当它同时作用于背景色和前景色时将引发错误。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="65"/>
      <source>Click to make the color trigger when the text&apos;s background color has not been modified from its normal value.</source>
      <translation>当文本的背景色为异常值时，单击以进行颜色触发。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="66"/>
      <source>Click to make the color trigger when the text&apos;s foreground color has not been modified from its normal value.</source>
      <translation>当文本的前景色未从其正常值修改时，单击以进行颜色触发器。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="67"/>
      <source>Click a color to make the trigger fire only when the text&apos;s background color matches the color number indicated.</source>
      <translation>单击颜色以使触发器仅在文本的背景色与指示的颜色匹配时触发。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="68"/>
      <source>Click a color to make the trigger fire only when the text&apos;s foreground color matches the color number indicated.</source>
      <translation>单击颜色以使触发器仅在文本的背景色与指示的颜色匹配时触发。</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="73"/>
      <source>Black</source>
      <translation>黑色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="74"/>
      <source>Red</source>
      <translation>紅色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="75"/>
      <source>Green</source>
      <translation>綠色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="76"/>
      <source>Yellow</source>
      <translation>黄色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="77"/>
      <source>Blue</source>
      <translation>藍色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="78"/>
      <source>Magenta</source>
      <translation>品红色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="79"/>
      <source>Cyan</source>
      <translation>青色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="80"/>
      <source>White (Light gray)</source>
      <translation>白色 (浅灰色)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="82"/>
      <source>Light black (Dark gray)</source>
      <translation>浅黑色 (暗灰色)</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="83"/>
      <source>Light red</source>
      <translation>浅红色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="84"/>
      <source>Light green</source>
      <translation>浅绿色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="85"/>
      <source>Light yellow</source>
      <translation>淡黄色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="86"/>
      <source>Light blue</source>
      <translation>浅蓝色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="87"/>
      <source>Light magenta</source>
      <translation>浅洋红色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="88"/>
      <source>Light cyan</source>
      <translation>浅青色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="89"/>
      <source>Light white</source>
      <translation>浅白色</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="210"/>
      <source>%1 [%2]</source>
      <extracomment>Color Trigger dialog button in basic 16-color set, the first value is the name of the color, the second is the ANSI color number - for most languages modification is not likely to be needed - this text is used in two places</extracomment>
      <translation>%1 [%2]</translation>
    </message>
    <message>
      <location filename="../src/dlgColorTrigger.cpp" line="362"/>
      <source>All color options are showing.</source>
      <translation>显示所有颜色选项。</translation>
    </message>
  </context>
  <context>
    <name>dlgComposer</name>
    <message>
      <location filename="../src/dlgComposer.cpp" line="294"/>
      <source>Add to user dictionary</source>
      <extracomment>Context menu action to add a word to the user&apos;s personal dictionary</extracomment>
      <translation>添加到用户字典</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="297"/>
      <source>Remove from user dictionary</source>
      <extracomment>Context menu action to remove a word from the user&apos;s personal dictionary</extracomment>
      <translation>从用户字典中删除</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="305"/>
      <source>▼Mudlet▼ │ dictionary suggestions │ ▲User▲</source>
      <extracomment>This separator line in the spell-check context menu divides suggestions from the user&apos;s personal dictionary (above) and Mudlet&apos;s built-in dictionary (below). The symbols are decorative and help indicate the direction. This appears in the composer window.</extracomment>
      <translation>▼ Mudlet ▼ │ 字典建议 │ ▲ 用户 ▲</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="312"/>
      <source>▼System▼ │ dictionary suggestions │ ▲User▲</source>
      <extracomment>This separator line in the spell-check context menu divides suggestions from the user&apos;s personal dictionary (above) and the system dictionary (below). The symbols are decorative and help indicate the direction. This appears in the composer window.</extracomment>
      <translation>▼ 系统 ▼ │ 字典建议 │ ▲ 用户 ▲</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="359"/>
      <source>no suggestions (system)</source>
      <extracomment>Shown when the spell-checker has no suggestions from the system dictionary for the misspelled word in the composer</extracomment>
      <translation>没有建议 (系统)</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="385"/>
      <source>no suggestions (shared)</source>
      <extracomment>Shown when the spell-checker has no suggestions from the shared user dictionary for the misspelled word in the composer</extracomment>
      <translation>没有建议 (共享)</translation>
    </message>
    <message>
      <location filename="../src/dlgComposer.cpp" line="388"/>
      <source>no suggestions (profile)</source>
      <extracomment>Shown when the spell-checker has no suggestions from the profile-specific user dictionary for the misspelled word in the composer</extracomment>
      <translation>没有建议 (配置文件)</translation>
    </message>
  </context>
  <context>
    <name>dlgConnectionProfiles</name>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="196"/>
      <source>Connect</source>
      <translation>连接</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="320"/>
      <source>Characters password. Note that the password is not encrypted in storage</source>
      <translation>字符密码。请注意，密码未在存储中加密</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="398"/>
      <source>Game name: %1</source>
      <translation>游戏名称: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="400"/>
      <source>Button to select a mud game to play, double-click it to connect and start playing it.</source>
      <extracomment>Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option</extracomment>
      <translation>选择要玩的mud游戏，双击它来进行连接并开始游玩。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1416"/>
      <source>This profile is currently loaded - close it before changing the connection parameters.</source>
      <translation>本配置当前已加载——在修改连接参数前先关闭它。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1776"/>
      <source>Reset icon</source>
      <extracomment>Reset the custom picture for this profile in the connection dialog and show the default one instead</extracomment>
      <translation>重置图标</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1780"/>
      <source>Set custom icon</source>
      <extracomment>Set a custom picture to show for the profile in the connection dialog</extracomment>
      <translation>设置自定义图标</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1785"/>
      <source>Set custom color</source>
      <extracomment>Set a custom color to show for the profile in the connection dialog</extracomment>
      <translation>设置自定义颜色</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2293"/>
      <source>The %1 character is not permitted. Use one of the following:</source>
      <translation>字符 %1 不允许使用， 可选用下列其中一项：</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2325"/>
      <source>You have to enter a number. Other characters are not permitted.</source>
      <translation>你必须输入一个数字。 不允许使用其他字符。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2314"/>
      <source>This profile name is already in use.</source>
      <translation>此配置文件名称已经使用。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="896"/>
      <source>Could not rename your profile data on the computer.</source>
      <translation>无法重命名计算机上的配置文件数据。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="198"/>
      <source>Offline</source>
      <translation>离线</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="202"/>
      <source>Skip - show me the games list</source>
      <extracomment>Button shown on first launch to skip the tutorial and show the full games list</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="227"/>
      <source>&lt;p&gt;&lt;center&gt;&lt;img src=&quot;tutorialIcon&quot;/&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;big&gt;&lt;b&gt;Welcome to Mudlet!&lt;/b&gt;&lt;/big&gt;&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;Play a short guided adventure to learn&lt;br&gt;how to navigate in games, use triggers, aliases, and scripting.&lt;/center&gt;&lt;/p&gt;&lt;p&gt;&lt;center&gt;&lt;a href=&quot;mudlet-tutorial&quot;&gt;Start Tutorial&lt;/a&gt;&lt;/center&gt;&lt;/p&gt;&lt;p align=&quot;right&quot;&gt;&lt;span style=&quot; font-family:&apos;Sans&apos;;&quot;&gt;The Mudlet Team &lt;/span&gt;&lt;img src=&quot;:/icons/mudlet_main_16px.png&quot;/&gt;&lt;/p&gt;</source>
      <extracomment>Welcome message shown on first launch, focused on starting the tutorial.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="240"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1931"/>
      <source>Copy</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="242"/>
      <source>Copy settings only</source>
      <translation>仅复制设置</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="259"/>
      <source>copy profile</source>
      <translation>复制配置文件</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="260"/>
      <source>copy the entire profile to new one that will require a different new name.</source>
      <translation>将整个配置文件复制到新的配置文件中，并重命名新配置文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="272"/>
      <source>copy profile settings</source>
      <translation>复制配置文件设置</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="273"/>
      <source>copy the settings and some other parts of the profile to a new one that will require a different new name.</source>
      <translation>将设置和配置文件的某些其他部分复制到新配置文件中。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="318"/>
      <source>Characters password, stored securely in the computer&apos;s credential manager</source>
      <translation>字符密码，安全地存储在计算机的凭据管理器</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="395"/>
      <source>Click to load but not connect the selected profile.</source>
      <translation>点击加载但不连接选中的配置文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="396"/>
      <source>Click to load and connect the selected profile.</source>
      <translation>点击以加载并连接选定的配置文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="397"/>
      <source>Need to have a valid profile name, game server address and port before this button can be enabled.</source>
      <translation>在启用此按钮之前，需要具有有效的配置文件名称、游戏服务器地址和端口。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="904"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1960"/>
      <source>Could not create the new profile folder on your computer.</source>
      <translation>无法创建新的配置文件夹。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="752"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="995"/>
      <source>new profile name</source>
      <translation>新配置文件名称</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="159"/>
      <source>My games</source>
      <extracomment>Tab showing only the games the user already has profiles for</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="161"/>
      <source>All games</source>
      <extracomment>Tab showing every game Mudlet has a built-in profile for</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="163"/>
      <source>games shown</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="164"/>
      <source>Switch between showing only your own games and all of the games Mudlet knows about.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1050"/>
      <source>&apos;%1&apos; has no profile folder of its own, so there is nothing to remove.</source>
      <extracomment>%1 is a profile name that does not name a folder of its own, so there is nothing that could be removed for it</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1061"/>
      <source>Could not remove everything belonging to &apos;%1&apos;. Close it if it is open elsewhere, check that you may write to its folder, and try again.</source>
      <extracomment>%1 is a profile name. Shown when some of the profile&apos;s files could not be deleted, e.g. because another program has them open</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1158"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1168"/>
      <source>Could not open the confirmation, so &apos;%1&apos; has not been removed.</source>
      <extracomment>%1 is a profile name. Shown when the dialog asking the user to confirm a removal could not be built</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1188"/>
      <source>Deleting &apos;%1&apos;</source>
      <translation>删除 &apos;%1&apos;</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1420"/>
      <source>A profile that is in use cannot be removed</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1803"/>
      <source>Select custom image for profile (should be 120x30)</source>
      <translation>为配置文件选择自定义图像（应为 120x30）</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1803"/>
      <source>Images (%1)</source>
      <translation>图像 (%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="1891"/>
      <source>Copying...</source>
      <translation>正在复制...</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2306"/>
      <source>A profile name cannot be &quot;.&quot; or contain &quot;..&quot;, as those refer to other folders on your computer. Please pick a different name.</source>
      <extracomment>Shown when a profile name would not name a folder of its own. Keep the quoted dots as they are, they are literal characters the user typed</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2335"/>
      <source>Port number must be above zero and below 65535.</source>
      <translation>端口号须大于0且不超过65535。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2355"/>
      <source>Mudlet can not load support for secure connections.</source>
      <translation>Mudlet 无法加载安全连接支持。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2377"/>
      <source>Please enter the URL or IP address of the Game server.</source>
      <translation>请输入游戏服务器的URL或IP地址。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2396"/>
      <source>Please enter the URL of the Game server.

&lt;i&gt;SSL/TLS connections require a URL, as an IP address is not a suitable identifier for the certification of the Game Server.&lt;/i&gt;</source>
      <extracomment>Please use two line-feeds after the first line so the second line can be italicised and spaced out - if appropriate for the locale.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2415"/>
      <source>Load profile without connecting.</source>
      <translation>在不连接的情况下加载配置文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2431"/>
      <source>Please set a valid profile name, game server address and the game port before loading.</source>
      <translation>请在加载前设置有效的配置文件名称、游戏服务器地址和游戏端口。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2436"/>
      <source>Please set a valid profile name, game server address and the game port before connecting.</source>
      <translation>请在连接前设置有效的配置文件名称、游戏服务器地址和游戏端口。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2489"/>
      <source>Click to hide the password; it will also hide if another profile is selected.</source>
      <translation>单击以隐藏密码；如果选择了其他配置文件，密码也会隐藏。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2493"/>
      <source>Click to reveal the password for this profile.</source>
      <translation>点击以显示此配置文件的密码。</translation>
    </message>
    <message>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2343"/>
      <location filename="../src/dlgConnectionProfiles.cpp" line="2346"/>
      <source>Mudlet is not configured for secure connections.</source>
      <translation>Mudlet 没有配置安全连接.</translation>
    </message>
  </context>
  <context>
    <name>dlgIRC</name>
    <message>
      <location filename="../src/dlgIRC.cpp" line="106"/>
      <source>%1 closed their client.</source>
      <translation>%1 关闭了客户端。</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="118"/>
      <source>Mudlet IRC Client - %1 - %2 on %3</source>
      <translation>Mudlet IRC 客户端 - %1 %2 在 %3 上</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="132"/>
      <source>$ Starting Mudlet IRC Client...</source>
      <translation>$ 正在启动 Mudlet IRC 客户端...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="133"/>
      <source>$ Host: %1:%2</source>
      <translation>$ 主机: %1:%2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="134"/>
      <source>$ Nick: %1</source>
      <translation>$ 昵称: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="135"/>
      <source>$ Auto-Join Channels: %1</source>
      <translation>$ 自动加入频道：%1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="136"/>
      <source>$ This client supports Auto-Completion using the Tab key.</source>
      <translation>$ 客户端支持使用 Tab 键自动完成。</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="137"/>
      <source>$ Type &lt;b&gt;/help&lt;/b&gt; for commands or &lt;b&gt;/help [command]&lt;/b&gt; for command syntax.</source>
      <translation>$指令 &lt;b&gt;/help&lt;/b&gt;  或 &lt;b&gt;/help [command]&lt;/b&gt; 命令的语法。</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="198"/>
      <source>Restarting IRC Client</source>
      <translation>重启 IRC 客户端</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="376"/>
      <source>[Error] MSGLIMIT requires &lt;limit&gt; to be a whole number greater than zero!</source>
      <translation>[ 错误 ] MSGLIMIT要求 &lt;limit&gt; 为大于零的整数!</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="406"/>
      <source>[HELP] Available Commands: %1</source>
      <translation>[ 帮助 ] 可用命令: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="408"/>
      <source>[HELP] Syntax: %1</source>
      <translation>[ 帮助 ] 语法: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="416"/>
      <source>! Connected to %1.</source>
      <translation>! 已连接到 %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="417"/>
      <source>! Joining %1...</source>
      <translation>! 加入 %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="422"/>
      <source>! Connecting %1...</source>
      <translation>! 正在连接到 %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="427"/>
      <source>! Disconnected from %1.</source>
      <translation>! 已从 %1 断开连接.</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="489"/>
      <source>[ERROR] Syntax: %1</source>
      <translation>[ 错误 ] 语法: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="491"/>
      <source>[ERROR] Unknown command: %1</source>
      <translation>[ 错误 ] 无效命令: %s</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="637"/>
      <source>! The Nickname %1 is reserved. Automatically changing Nickname to: %2</source>
      <translation>! 昵称 %1 是已被占用。 自动将昵称更改为: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgIRC.cpp" line="648"/>
      <source>Your nick has changed.</source>
      <translation>你的昵称已经改变。</translation>
    </message>
  </context>
  <context>
    <name>dlgMapLabel</name>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="38"/>
      <source>Create label</source>
      <extracomment>Create label dialog title</extracomment>
      <translation>创建标签</translation>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="70"/>
      <source>Font size is automatically calculated to fit the label</source>
      <extracomment>Tooltip for font display in map label dialog</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="97"/>
      <source>Foreground color</source>
      <extracomment>2D mapper create label color dialog title</extracomment>
      <translation>前景色</translation>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="120"/>
      <source>Background color</source>
      <extracomment>2D mapper create label color dialog title</extracomment>
      <translation>背景色</translation>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="143"/>
      <source>Text outline color</source>
      <extracomment>2D mapper create label color dialog title</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="165"/>
      <source>Label font</source>
      <extracomment>2D mapper create label font dialog title</extracomment>
      <translation>標籤字型</translation>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="187"/>
      <source>Select image</source>
      <translation>選擇影像</translation>
    </message>
    <message>
      <location filename="../src/dlgMapLabel.cpp" line="246"/>
      <source>%1 %2</source>
      <extracomment>Font display format in map label dialog. %1 is font family name, %2 is style (e.g. &quot;Bold&quot;, &quot;Italic&quot;). Size excluded since it auto-scales.</extracomment>
      <translation>%1 %2</translation>
    </message>
  </context>
  <context>
    <name>dlgMapper</name>
    <message>
      <location filename="../src/dlgMapper.cpp" line="177"/>
      <source>No map yet for this profile.</source>
      <extracomment>Empty-state text shown in the mapper when the profile has no local map yet.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="182"/>
      <source>Download from game</source>
      <extracomment>Button in the mapper empty-state. Downloads a shared map offered by the game server via MMP.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="187"/>
      <source>Load map...</source>
      <extracomment>Button in the mapper empty-state. Opens a file dialog to load a .dat/.json/.xml map saved on disk.</extracomment>
      <translation>载入地图...</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="191"/>
      <source>Create new map</source>
      <extracomment>Button in the mapper empty-state. Dismisses the prompt so the user can map from scratch.</extracomment>
      <translation>新建地图</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="238"/>
      <source>Abort</source>
      <extracomment>Button label to abort an in-progress map download or import.</extracomment>
      <translation>中止</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="337"/>
      <source>Any map file (*.dat *.json *.xml)</source>
      <extracomment>File dialog filter. Keep the extensions (in braces) unchanged - they are used programmatically.</extracomment>
      <translation>任意地图文件 (*.dat *.json *.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="337"/>
      <source>Mudlet binary map (*.dat)</source>
      <translation>Mudlet 二进制地图 (*.dat)</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="337"/>
      <source>Mudlet JSON map (*.json)</source>
      <translation>Mudlet JSON 地图 (*.json)</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="337"/>
      <source>Mudlet XML map (*.xml)</source>
      <translation>Mudlet XML 地图 (*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="337"/>
      <source>Any file (*)</source>
      <translation>任意文件 (*)</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="341"/>
      <source>Load Mudlet map</source>
      <extracomment>Title of the file dialog used to pick a map file to load.</extracomment>
      <translation>载入 Mudlet 地图</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="363"/>
      <source>[ ERROR ] - Unable to load JSON map file: %1
reason: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="858"/>
      <source>Draw rooms on upper and lower levels</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="861"/>
      <source>When enabled, rooms on floors above and below the current level will be drawn with a lighter color to show the map layout context.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="866"/>
      <source>Round rooms</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="869"/>
      <source>When enabled, rooms will be drawn with round corners instead of square corners.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="874"/>
      <source>Show room IDs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="877"/>
      <source>When enabled, room IDs will be displayed on the map.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="882"/>
      <source>Show room names</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="885"/>
      <source>When enabled, room names will be displayed on the map.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="890"/>
      <source>Show map grid</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="893"/>
      <source>When enabled, grid will be shown on mapper.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="899"/>
      <source>Show map in 3D</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="902"/>
      <source>When enabled, the map will be displayed in 3D mode.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="909"/>
      <source>Info overlays</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="913"/>
      <source>New map window</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="914"/>
      <source>Open an additional map view</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="951"/>
      <source>None</source>
      <extracomment>Don&apos;t show the map overlay, &apos;none&apos; meaning no map overlay styled are enabled</extracomment>
      <translation>无</translation>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="985"/>
      <source>Map autosave failed</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="991"/>
      <source>Retry save</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgMapper.cpp" line="1001"/>
      <source>Dismiss warning</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgModuleManager</name>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="52"/>
      <source>Module Manager - %1</source>
      <translation>模組管理工具 - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Name</source>
      <translation>模組名稱</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Priority</source>
      <translation>優先順序</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Sync</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="66"/>
      <source>Module Location</source>
      <translation>模組位置</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="107"/>
      <source>Master module: saved and resynchronized across all sessions on Save Profile or session end.</source>
      <extracomment>Tooltip for master module checkbox</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="140"/>
      <source>Load Mudlet Module</source>
      <extracomment>Module manager - import modules from file dialog (multi-select enabled) Module manager - file filter for supported module types (mpackage, zip, xml)</extracomment>
      <translation>加载 Mudlet 模块</translation>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="140"/>
      <source>Mudlet Packages (*.mpackage *.zip *.xml)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgModuleManager.cpp" line="169"/>
      <source>Failed to import: %1</source>
      <extracomment>Module manager - status message shown when some modules failed to import. %1 is a comma-separated list of module names</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgNotepad</name>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="73"/>
      <source>Prepend</source>
      <extracomment>label for prepended text entry box in notepad</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="77"/>
      <source>Text to prepend to lines</source>
      <extracomment>placeholder text for text entry box in notepad - text which gets added before sending a line</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="81"/>
      <source>Stop</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="107"/>
      <source>Add new note tab (Ctrl+T)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="124"/>
      <source>Find</source>
      <extracomment>Placeholder text for the search field in notepad</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="130"/>
      <source>Find previous</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="136"/>
      <source>Find next</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="142"/>
      <source>Close find bar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="209"/>
      <source>New Note</source>
      <extracomment>Default name for a new note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="243"/>
      <source>Rename Note Tab</source>
      <extracomment>Dialog title for renaming a note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="245"/>
      <source>New name:</source>
      <extracomment>Label for the input field when renaming a note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="267"/>
      <source>New Tab</source>
      <extracomment>Context menu action to create a new note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="277"/>
      <source>Rename Tab</source>
      <extracomment>Context menu action to rename a note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="284"/>
      <source>Close Tab</source>
      <extracomment>Context menu action to close a note tab</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="292"/>
      <source>Close Other Tabs</source>
      <extracomment>Context menu action to close all note tabs except the clicked one</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgNotepad.cpp" line="386"/>
      <location filename="../src/dlgNotepad.cpp" line="400"/>
      <location filename="../src/dlgNotepad.cpp" line="412"/>
      <location filename="../src/dlgNotepad.cpp" line="418"/>
      <location filename="../src/dlgNotepad.cpp" line="426"/>
      <location filename="../src/dlgNotepad.cpp" line="441"/>
      <source>Notes</source>
      <extracomment>Name for the migrated notes tab when upgrading from single-note to tabbed notepad
----------
Default name for the first note tab</extracomment>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgPackageExporter</name>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="29"/>
      <source>Package name here</source>
      <translation>包名称</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="36"/>
      <source>or</source>
      <translation>或</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="82"/>
      <source>Check items to export</source>
      <translation>检查要导出的项</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="156"/>
      <source>Author</source>
      <translation>作者</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="175"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="358"/>
      <source>(recommended)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="213"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="241"/>
      <source>Icon size of 512x512 recommended</source>
      <translation>建议图标大小为512x512</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="260"/>
      <source>X</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="270"/>
      <source>512x512 recommended</source>
      <translation>建议 512x512</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="293"/>
      <source>one-line description (recommended)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="300"/>
      <source>Description
(e.g. how to use)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="314"/>
      <source>(recommended)

This package description is shown in the package manager.  The editor supports Commonmark markdown.  Follow the description below for a thorough example of what to include in your package description.

### Description

A full description of what this package achieves. If the package is game specific then mention that here.  Specify if the package has autoupdating or, if not, add a link in the See Also section below to the code repository.

### Usage

If this package uses aliases, show a few examples and expected output.

`&gt; alias_1`

    output of alias_1  -- indent by four spaces
    more output        -- for code blocks

If this package is a GUI implementation consider adding screenshots by directly dragging and dropping images into this editor.

### See Also

Further reading material. e.g. a link to the Mudlet wiki, forums, Github package repository or webpage.

* https://wiki.mudlet.org/w/Manual:Best_Practices#Package_and_Module_best_practices
* [Link 2 might be a webpage](https://example.org)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="355"/>
      <source>1</source>
      <translation>1</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="182"/>
      <source>Icon</source>
      <translation>图标</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="66"/>
      <source>Select what to export</source>
      <comment>This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.</comment>
      <translation>选择要导入的内容：</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="109"/>
      <source>Describe your package. Add a description, icons, assets and more.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="216"/>
      <source>Add icon</source>
      <translation>添加图标</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="280"/>
      <source>Short description</source>
      <translation>简短描述</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="345"/>
      <source>Version</source>
      <translation>版本</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="365"/>
      <location filename="../src/ui/dlgPackageExporter.ui" line="378"/>
      <source>Webpage where users can find help for this package. It is shown by the &quot;Module Help&quot; button in the Module Manager.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="368"/>
      <source>Help URL</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="391"/>
      <source>Required packages</source>
      <translation>必需的包</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="491"/>
      <source>Does this package make use of other packages? List them here as requirements. Press &apos;Delete&apos; to remove a package.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="523"/>
      <source>Include assets (images, sounds, fonts)</source>
      <translation>包括素材（圖片、聲音、字型）</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="533"/>
      <source>Drag and drop files and folders, or use the browse button below</source>
      <translation>拖放文件和文件夹，或使用下面的浏览按钮</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="581"/>
      <source>Select files to include in package</source>
      <translation>选择要包含在包中的文件</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="20"/>
      <source>Package Exporter</source>
      <translation>包导出器</translation>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="388"/>
      <source>Does this package make use of other packages? List them here as requirements.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/dlgPackageExporter.ui" line="631"/>
      <source>Select export location</source>
      <translation>选择导出位置</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="74"/>
      <source>Triggers</source>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="75"/>
      <source>Aliases</source>
      <translation>别名</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="76"/>
      <source>Timers</source>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="77"/>
      <source>Scripts</source>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="78"/>
      <source>Keys</source>
      <translation>按键</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="79"/>
      <source>Buttons</source>
      <translation>按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="102"/>
      <source>Export</source>
      <extracomment>Text for button to perform the package export on the items the user has selected.</extracomment>
      <translation>匯出</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="142"/>
      <source>Package Exporter - %1</source>
      <extracomment>Title of the window. The %1 will be replaced by the current profile&apos;s name</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="166"/>
      <source>Create Module - %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="169"/>
      <source>Enter module name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="172"/>
      <source>Create Module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="175"/>
      <source>Select where to save module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="178"/>
      <source>Select items to include in module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="186"/>
      <source>Add module description, icon, and assets (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="189"/>
      <source>Module location</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="195"/>
      <source>Module description</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="196"/>
      <source>Brief description of your module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="197"/>
      <source>Module author (recommended)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="198"/>
      <source>Module version (recommended)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="201"/>
      <source>Module dependencies</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="202"/>
      <source>Include module assets (images, sounds, fonts)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="203"/>
      <source>Select files to include in module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="211"/>
      <source>Select module dependencies</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="222"/>
      <source>(optional)

This module description is shown in the Module Manager. The editor supports Commonmark markdown.

### Description

A full description of what this module does. If the module is game-specific, mention that here.

### Usage

If this module uses aliases, show a few examples and expected output.

`&gt; alias_1`

    output of alias_1  -- indent by four spaces
    more output        -- for code blocks

### See Also

Further reading material, e.g., links to documentation or forum posts.

* https://wiki.mudlet.org/w/Manual:Modules</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="415"/>
      <source>Failed to open file &quot;%1&quot; to place into package. Error message was: &quot;%2&quot;.</source>
      <extracomment>This error message will appear when a file is to be placed into the package but the code cannot open it.</extracomment>
      <translation>无法打开要放入包的文件 &quot;%1&quot; . 错误消息为: &quot;%2&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="423"/>
      <source>Failed to add file &quot;%1&quot; to package. Error message was: &quot;%3&quot;.</source>
      <extracomment>This error message will appear when a file is to be placed into the package but cannot be done for some reason.</extracomment>
      <translation>无法添加文件&quot;%1&quot; 至包程序中 . 错误消息为: &quot;%3&quot;..</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="606"/>
      <source>package name</source>
      <extracomment>package name will be added to other fields in the &apos;required fields missing: ...&apos; tooltip when it&apos;s missing</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="628"/>
      <source>Required field missing: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="631"/>
      <source>Export package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="859"/>
      <source>Cannot create empty module. Please select at least one trigger, timer, alias, script, action, or key to include in the module.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="861"/>
      <source>Cannot create empty package. Please select at least one item to include in the package.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1035"/>
      <source>Module &quot;%1&quot; exported but installation failed: %2</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1015"/>
      <source>Module &quot;%1&quot; exported but failed to uninstall existing version</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1376"/>
      <source>Failed to open package file. Error is: &quot;%1&quot;.</source>
      <extracomment>This zipError message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer&apos;s file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.</extracomment>
      <translation>打开包文件失败。错误是: &quot;%1&quot;。</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1542"/>
      <source>Failed to zip up the package. Error is: &quot;%1&quot;.</source>
      <extracomment>This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...</extracomment>
      <translation>压缩包文件失败. 错误: &quot;%1&quot;.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1978"/>
      <source>Why not &lt;a href=&quot;https://packages.mudlet.org/upload&quot;&gt;upload&lt;/a&gt; your package for other Mudlet users?</source>
      <extracomment>Only the text outside of the &apos;a&apos; (HTML anchor) tags PLUS the verb &apos;upload&apos; in between them in the source text, (associated with uploading the resulting package to the Mudlet forums) should be translated.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageExporter.cpp" line="1995"/>
      <source>Select what to export (%n item(s))</source>
      <extracomment>This is the text shown at the top of a groupbox when there is %n (one or more) items to export in the Package exporter dialogue; the initial (and when there is no items selected) is a separate text.</extracomment>
      <translation>
        <numerusform>选择要导出的内容 (%n 项 )</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1998"/>
      <source>Select what to export</source>
      <extracomment>This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.</extracomment>
      <translation>选择要导入的内容：</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="437"/>
      <source>update installed package</source>
      <extracomment>First item in package selection dropdown - when selected, allows updating an existing installed package</extracomment>
      <translation>更新已安装的包</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="432"/>
      <source>add dependencies</source>
      <translation>添加依赖项</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="591"/>
      <location filename="../src/dlgPackageExporter.cpp" line="593"/>
      <source>Export to %1</source>
      <translation>导出至 %1</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1344"/>
      <source>cannot copy %1 to the temporary location %2 - can you double-check it?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="640"/>
      <source>Open Icon</source>
      <translation>打开图标</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="640"/>
      <source>Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</source>
      <translation>图像文件(*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="778"/>
      <source>Please enter the package name.</source>
      <translation>请输入包名.</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="888"/>
      <source>Overwrite module?</source>
      <extracomment>Title of the dialog asking whether to replace a module that already exists when creating a module</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="890"/>
      <source>A module named &quot;%1&quot; already exists.</source>
      <extracomment>%1 is the name of the module that already exists</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="893"/>
      <source>Overwrite package?</source>
      <extracomment>Title of the dialog asking whether to replace a package file that already exists when exporting</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="895"/>
      <source>A file named &quot;%1&quot; already exists.</source>
      <extracomment>%1 is the file name of the package file that would be overwritten</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="898"/>
      <source>Do you want to overwrite it?</source>
      <extracomment>Shown under the &apos;a file/module already exists&apos; text when exporting a package or creating a module</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="936"/>
      <location filename="../src/dlgPackageExporter.cpp" line="1053"/>
      <source>Exporting package...</source>
      <translation>导出包...</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="956"/>
      <source>Failed to export. Could not open the folder &quot;%1&quot; for writing. Do you have the necessary permissions and free disk-space to write to that folder?</source>
      <translation>导出失败。无法写入文件夹 &quot;%1&quot;. - 您是否拥有写入/存储的权限并有足够的磁盘空间来写入到该文件夹？</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1028"/>
      <source>Module &quot;%1&quot; created and installed successfully! Saved to: %2. You can now close this dialog.</source>
      <extracomment>%1 is the module name, %2 is a clickable link to the folder the module file was saved in</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1263"/>
      <source>Failed to export. Could not write Mudlet items to the file &quot;%1&quot;.</source>
      <extracomment>This error message is shown when all the Mudlet items cannot be written to the &apos;packageName&apos;.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened</extracomment>
      <translation>导出失败。无法写入 Mudlet 项目到文件 &quot;%1&quot;。</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1339"/>
      <source>%1 doesn&apos;t seem to exist anymore - can you double-check it?</source>
      <translation>%1 似乎不再存在了 - 你能仔细检查一下吗？</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1460"/>
      <source>Failed to add directory &quot;%1&quot; to package. Error is: &quot;%2&quot;.</source>
      <translation>未能将目录 &quot;%1&quot; 添加到包文件中。 错误为: &quot;%2&quot;。</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1503"/>
      <source>Required file &quot;%1&quot; was not found in the staging area. This area contains the Mudlet items chosen for the package, which you selected to be included in the package file. This suggests there may be a problem with that directory: &quot;%2&quot; - Do you have the necessary permissions and free disk-space?</source>
      <translation>在暂存区域中找不到所需文件 &quot;%1&quot; 此区域包含你选择要包含在包文件中的 Mudlet项目。 这表明该目录可能有问题： &quot;%2&quot; - 您是否拥有必要的权限和足够的磁盘空间？</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1038"/>
      <source>Package &quot;%1&quot; exported to: %2</source>
      <translation>包 &quot;%1&quot; 导出到: %2</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1534"/>
      <source>Export cancelled.</source>
      <translation>已取消导出</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageExporter.cpp" line="1614"/>
      <source>Where do you want to save the package?</source>
      <translation>你想将压缩包导出到哪个位置？</translation>
    </message>
  </context>
  <context>
    <name>dlgPackageManager</name>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="61"/>
      <source>Package Manager - %1</source>
      <extracomment>Package manager - window title</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="199"/>
      <source>Version </source>
      <extracomment>Package manager - label showing package version</extracomment>
      <translation>版本 </translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="323"/>
      <source>Import Mudlet Package</source>
      <extracomment>Package manager - import packages from file dialog (multi-select enabled) Package manager - file filter for supported package types (mpackage, zip, xml)</extracomment>
      <translation>导入Mudlet包</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="323"/>
      <source>Mudlet Packages (*.mpackage *.zip *.xml)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="348"/>
      <source>Failed to import: %1</source>
      <extracomment>Package manager - status message shown when some packages failed to import. %1 is a comma-separated list of package names</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="360"/>
      <source>Downloading packages...</source>
      <extracomment>Package manager - cancel button text for download progress dialog</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="360"/>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="398"/>
      <location filename="../src/dlgPackageManager.cpp" line="407"/>
      <location filename="../src/dlgPackageManager.cpp" line="456"/>
      <source>Installation Failed</source>
      <extracomment>Package manager: package couldn&apos;t be downloaded
----------
Package manager: network error, package couldn&apos;t be downloaded</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="398"/>
      <location filename="../src/dlgPackageManager.cpp" line="407"/>
      <source>Package &apos;%1&apos; not found in repository</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="456"/>
      <source>Package &apos;%1&apos; could not be downloaded due to a network error</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="556"/>
      <source>Version %1 → %2</source>
      <extracomment>Package manager - version update indicator showing old and new versions</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="783"/>
      <source>All packages are up to date.</source>
      <extracomment>Package manager - message shown in description area when no updates are available</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageManager.cpp" line="820"/>
      <source>Update (%n)</source>
      <extracomment>Message on button in package manager to update one or multiple (%n is the count) selected packages.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="823"/>
      <source>Update</source>
      <extracomment>Message on button in package manager when there are no selected packages - button will also be disabled.</extracomment>
      <translation>更新</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="826"/>
      <source>Update selected packages</source>
      <extracomment>Tooltip for button in package manager when in Updates view</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageManager.cpp" line="830"/>
      <source>Install (%n)</source>
      <extracomment>Message on button in package manager to install one or multiple (%n is the count) selected packages.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="833"/>
      <location filename="../src/dlgPackageManager.cpp" line="840"/>
      <source>Install</source>
      <extracomment>Message on button in package manager when there are no selected packages - button will also be disabled.
----------
Message on button in package manager initially and when the view is the &quot;Installed&quot; one</extracomment>
      <translation>安装</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="836"/>
      <location filename="../src/dlgPackageManager.cpp" line="843"/>
      <source>Install package from repository</source>
      <extracomment>Tooltip for button in package manager when in Explore view</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgPackageManager.cpp" line="855"/>
      <source>Remove (%n)</source>
      <extracomment>Message on button in package manager to remove one or multiple (%n is the count) selected packages.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="858"/>
      <location filename="../src/dlgPackageManager.cpp" line="862"/>
      <source>Remove</source>
      <extracomment>Message on button in package manager when there are no selected packages - button will also be disabled.
----------
Message on button in package manager initially and when the view is NOT the &quot;Installed&quot; one</extracomment>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="880"/>
      <source>Updates (%1)</source>
      <extracomment>Package manager - navigation button showing one or more available updates</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgPackageManager.cpp" line="883"/>
      <source>Updates</source>
      <extracomment>Package manager - navigation button for when there are no updates</extracomment>
      <translation>更新</translation>
    </message>
  </context>
  <context>
    <name>dlgProfilePreferences</name>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="177"/>
      <source>Location which will be used to store log files - matching logs will be appended to.</source>
      <translation>将用于存储日志文件的位置 - 匹配的日志将被附加至此。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="178"/>
      <source>Select a directory where logs will be saved.</source>
      <translation>选择保存日志的目录。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="179"/>
      <source>Reset the directory so that logs are saved to the profile&apos;s &lt;i&gt;log&lt;/i&gt; directory.</source>
      <translation>重置目录，以便将日志保存到配置文件以下的日志目录。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="183"/>
      <source>Set a custom name for your log. (New logs are appended if a log file of the same name already exists).</source>
      <translation>为您的日志设置一个名称。(如果已存在同一个名称的日志文件，新日志将被追加在旧文件的末端)。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="219"/>
      <source>Automatic updates are disabled in development builds to prevent an update from overwriting your Mudlet.</source>
      <translation>自动更新在开发版中被禁用，以防止自动更新覆盖你的Mudelet。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="250"/>
      <source>Select the only or the primary font used (depending on &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; setting) to produce the 2D mapper room symbols.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="333"/>
      <source>%1 (%2% done)</source>
      <comment>%1 is the (not-translated so users of the language can read it!) language name, %2 is percentage done.</comment>
      <translation>%1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="404"/>
      <source>Migrated all passwords to secure storage.</source>
      <translation>正在将密码迁移到安全存储器.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="415"/>
      <source>Migrated all passwords to profile storage.</source>
      <translation>将所有密码迁移到配置文件存储。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="780"/>
      <source>From the dictionary file &lt;tt&gt;%1.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="972"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00%1)</source>
      <translation>yyyy-mm-dd hh-mm-ss（例如，1970-01-01 00-00-00）</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="974"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00%1)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (例如， 1970-01-01T00-00-00%1)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="975"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01%1)</source>
      <translation>yyyy-MM-dd（连接每日登录，例如1970-01-01）</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="978"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01%1)</source>
      <translation>yyyy-mm（连接月份登录，例如1970-01）</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="979"/>
      <source>Named file (concatenate logs in one file)</source>
      <translation>命名文件（连接日志到一个文件中）</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1074"/>
      <source>Other profiles to Map to:</source>
      <translation>要映射到的其他配置文件：</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1138"/>
      <source>2D Map Room Symbol scaling factor:</source>
      <translation>2D地图房间标记缩放系数：</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1170"/>
      <source>Show &quot;%1&quot; in the map area selection</source>
      <translation>在地图区域选择中显示 &quot;%1&quot;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1253"/>
      <source>%1 (*Error, report to Mudlet Makers*)</source>
      <comment>The encoder code name is not in the mudlet class mEncodingNamesMap when it should be and the Mudlet Makers need to fix it!</comment>
      <translation>%1 (*错误, 向 Mudlet 开发者报告*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1430"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="4857"/>
      <source>Profile preferences - %1</source>
      <translation>偏好設定 - %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1904"/>
      <source>Profile preferences</source>
      <translation>配置设定</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2869"/>
      <source>Load Mudlet map</source>
      <translation>载入 Mudlet 地图</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2810"/>
      <source>Loading map - please wait...</source>
      <translation>正在加載地圖 — 請稍後……</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="186"/>
      <source>logfile</source>
      <extracomment>Must be a valid default filename for a log-file and is used if the user does not enter any other value (Ensure all instances have the same translation {one of two copies}).</extracomment>
      <translation>日志文件</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgProfilePreferences.cpp" line="197"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="3617"/>
      <source>copy to %n destination(s)</source>
      <extracomment>text on button to put the map from this profile into the other profiles to receive the map from this profile, %n is the number of other profiles that have already been selected to receive it and will be zero or more. The button will also be disabled (greyed out) in the zero case but the text will still be visible.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="411"/>
      <source>Migrated %1...</source>
      <extracomment>This notifies the user that progress is being made on profile migration by saying what profile was just migrated to store passwords securely</extracomment>
      <translation>已迁移 %1...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="735"/>
      <source>Enable spell check using Mudlet dictionary:</source>
      <extracomment>On Windows and MacOs, we have to bundle our own dictionaries with our application - and we also use them on *nix systems where we do not find the system ones</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="738"/>
      <source>Enable spell check using System dictionary:</source>
      <extracomment>On *nix systems where we find the system ones we use them</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="853"/>
      <source>&lt;p&gt;Use the maximum buffer size your system can handle (%1 lines). This will be calculated based on available memory.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1003"/>
      <source>Protocols</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1012"/>
      <source>GMCP: Generic Mud Communication Protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1025"/>
      <source>MSDP: Mud Server Data Protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1035"/>
      <source>MSSP: Mud Server Status Protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1030"/>
      <source>MSP: Mud Sound Protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1045"/>
      <source>MXP: Mud eXtension Protocol</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1040"/>
      <source>MTTS: Mud Terminal Type Standard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="245"/>
      <source>&lt;p&gt;Hide success messages in Central Debug Console for timers with intervals below this threshold. Error messages always display.&lt;/p&gt;</source>
      <extracomment>Tooltip for timer debug output minimum interval</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="249"/>
      <source>Show all map symbols, their Unicode code-points, font availability, and which rooms use them.</source>
      <extracomment>Tooltip for show glyph usage button</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="253"/>
      <source>Use only the selected font (may show � for missing symbols) or allow fallback fonts for better coverage.</source>
      <extracomment>Tooltip for map symbol font usage option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="255"/>
      <source>&lt;p&gt;Run all matching keybindings instead of just the first one. Disable for compatibility with pre-3.9.0 scripts.&lt;/p&gt;</source>
      <extracomment>Tooltip for run all keybindings option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="258"/>
      <source>&lt;p&gt;Controls display width for ambiguous East Asian characters. Auto-detects correct width for most encodings (default), or choose narrow/wide.&lt;/p&gt;</source>
      <extracomment>Tooltip for East Asian ambiguous width character option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="261"/>
      <source>&lt;p&gt;Enable context menu to analyze UTF-16/UTF-8 encoding of selected text. Useful for identifying multi-byte characters.&lt;/p&gt;</source>
      <extracomment>Tooltip for text analyzer option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="264"/>
      <source>&lt;p&gt;Control menu icon display: on, off, or auto (system default). May require restart.&lt;/p&gt;</source>
      <extracomment>Tooltip for show icons on menus option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="935"/>
      <source>The Discord desktop app must be running for Rich Presence to work. Browser and mobile clients are not supported.</source>
      <extracomment>Tooltip shown when Discord Rich Presence cannot detect a logged-in user</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1007"/>
      <source>CHARSET: Character Encoding Standard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1017"/>
      <source>MNES: Mud New-Environ Standard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1021"/>
      <source>MNES uses the same telnet option as NEW-ENVIRON, so only one can be active. MNES sends a minimal set of variables, while NEW-ENVIRON sends extended variables including OSC link support.</source>
      <extracomment>Tooltip for MNES protocol option explaining mutual exclusivity with NEW-ENVIRON</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1050"/>
      <source>NAWS: Negotiate About Window Size</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1055"/>
      <source>NEW-ENVIRON: Client Variables Standard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1060"/>
      <source>NEW-ENVIRON uses the same telnet option as MNES, so only one can be active. NEW-ENVIRON sends extended variables including OSC link support, while MNES sends a minimal set.</source>
      <extracomment>Tooltip for NEW-ENVIRON protocol option explaining mutual exclusivity with MNES</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1115"/>
      <source>%1 {Default}</source>
      <translation>%1 {Default}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1127"/>
      <source>%1 {Experimental}</source>
      <translation>%1 {Experimental}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1129"/>
      <source>%1 {For older versions}</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1363"/>
      <source>unknown error</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1364"/>
      <source>This profile could not be loaded correctly (%1). Settings cannot be saved. Close the profile and try loading an older version from &apos;Connect - Options - Profile history&apos;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1541"/>
      <source>Tab will switch between the input line and main window, and also step through hyperlinks while in caret mode. Ctrl+] and Ctrl+[ navigate links without conflicting with pane-switching. Press Enter or Space to activate the focused link, and the Menu key or Shift+F10 to open its context menu. Press Ctrl+End to jump to the latest content or Ctrl+Home to jump to the start of the buffer.</source>
      <extracomment>Screen-reader hint when the user picks Tab as the caret-mode pane-switching key, warning Tab is shared with hyperlink navigation and explaining how to activate links, open their menu, and jump to latest content. Do not translate the key names &quot;Tab&quot;, &quot;Ctrl+]&quot;, &quot;Ctrl+[&quot;, &quot;Enter&quot;, &quot;Space&quot;, &quot;Menu&quot;, &quot;Shift+F10&quot;, &quot;Ctrl+End&quot; or &quot;Ctrl+Home&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1546"/>
      <source>In caret mode, use Ctrl+] for the next hyperlink and Ctrl+[ for the previous hyperlink. Press Enter or Space to activate the focused link, and the Menu key or Shift+F10 to open its context menu. Press Ctrl+End to jump to the latest content or Ctrl+Home to jump to the start of the buffer.</source>
      <extracomment>Screen-reader hint when the user picks any caret-mode pane-switching key other than Tab, explaining how to navigate, activate and open menus on hyperlinks, and jump to latest content. Do not translate the key names &quot;Ctrl+]&quot;, &quot;Ctrl+[&quot;, &quot;Enter&quot;, &quot;Space&quot;, &quot;Menu&quot;, &quot;Shift+F10&quot;, &quot;Ctrl+End&quot; or &quot;Ctrl+Home&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1642"/>
      <source>Warning: &apos;%1&apos; and &apos;%2&apos; now share the shortcut %3 - neither will work until one of them is changed.</source>
      <extracomment>Inline warning on the shortcuts preferences page when exactly two actions have been given the same shortcut. %1 and %2 are the action names, %3 is the shortcut itself.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1649"/>
      <source>Warning: %1 now share the shortcut %2 - none of them will work until they are changed.</source>
      <extracomment>Inline warning on the shortcuts preferences page when three or more actions have been given the same shortcut. %1 is the list of action names (each already quoted), %2 is the shortcut itself.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="1660"/>
      <source>Shortcut conflict resolved.</source>
      <extracomment>Screen-reader announcement when editing the shortcuts removed the last duplicated assignment.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2114"/>
      <source>[  OK  ]  - The stored media files for this profile have been cleared.</source>
      <extracomment>Shown after the &quot;Clear stored media&quot; button in preferences empties the profile&apos;s media directory.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2207"/>
      <source>Pick color</source>
      <extracomment>Generic pick color dialog title</extracomment>
      <translation>选取颜色</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2510"/>
      <source>Forget saved sign-in?</source>
      <extracomment>Title of the dialog asking the user to confirm removing their saved sign-in.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2512"/>
      <source>This will remove the saved sign-in for this profile. You will need to sign in again next time. Continue?</source>
      <extracomment>Body of the dialog asking the user to confirm removing their saved sign-in; they will need to sign in again next time.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2531"/>
      <source>The saved sign-in has been forgotten.</source>
      <extracomment>Shown after the user&apos;s saved sign-in has actually been removed.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2535"/>
      <source>[  OK  ]  - The saved sign-in for this profile has been forgotten.</source>
      <extracomment>Shown in the main console after the user&apos;s saved sign-in has actually been removed.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2540"/>
      <source>Could not remove the saved sign-in; it may still be present.</source>
      <extracomment>Shown when removing the saved sign-in failed, so it may still be present.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2544"/>
      <source>[ WARN ]  - Could not remove the saved sign-in; it may still be present.</source>
      <extracomment>Shown in the main console when removing the saved sign-in failed, so it may still be present.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2550"/>
      <source>No changes were made to the saved sign-in.</source>
      <extracomment>Shown when the user cancels removing their saved sign-in.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2552"/>
      <source>[ INFO ]  - Cancelled: no changes were made to the saved sign-in.</source>
      <extracomment>Shown in the main console when the user cancels removing their saved sign-in.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2834"/>
      <source>Loaded map from %1.</source>
      <translation>从 %1 加载地图。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2836"/>
      <source>Could not load map from %1.</source>
      <translation>无法从 %1 加载地图。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2900"/>
      <source>Save Mudlet map</source>
      <translation>保存 Mudlet 地图</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2928"/>
      <source>Saving map - please wait...</source>
      <translation>正在保存地图 - 请稍候...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2945"/>
      <source>Saved map to %1.</source>
      <translation>保存地图到 %1。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2947"/>
      <source>Could not save map to %1.</source>
      <translation>无法将地图保存到 %1。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2978"/>
      <source>Migrating passwords to secure storage...</source>
      <translation>正在将密码迁移到安全存储器...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2985"/>
      <source>Migrating passwords to profiles...</source>
      <translation>正在将密码迁移到概要文件...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3017"/>
      <source>[ ERROR ] - Unable to use or create directory to store map for other profile &quot;%1&quot;.
Please check that you have permissions/access to:
&quot;%2&quot;
and there is enough space. The copying operation has failed.</source>
      <translation>[ 错误 ] - 无法使用或创建目录来为其他配置文件 &quot;%1&quot; 存储地图.
请检查您的访问权限：
&quot;%2&quot;
，并确认空间足够。复制操作失败。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3024"/>
      <source>Creating a destination directory failed...</source>
      <translation>创建目标目录失败...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3093"/>
      <source>Backing up current map - please wait...</source>
      <translation>正在備份目前地圖 - 請稍候...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3103"/>
      <source>Could not backup the map - saving it failed.</source>
      <translation>无法备份地图 - 保存失败。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3128"/>
      <source>Could not copy the map - failed to work out which map file we just saved the map as!</source>
      <translation>无法复制地图 - 无法确认刚刚保存的地图！</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3140"/>
      <source>Copying over map to %1 - please wait...</source>
      <translation>正在复制地图到 %1 - 请稍候...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3146"/>
      <source>Could not copy the map to %1 - unable to copy the new map file over.</source>
      <translation>无法复制地图到 %1 - 无法复制新的地图文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3150"/>
      <source>Map copied successfully to other profile %1.</source>
      <translation>地图已成功复制到其他配置文件 %1.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3161"/>
      <source>Map copied, now signalling other profiles to reload it.</source>
      <translation>地图已复制, 现在显示其他配置文件以重新加载它。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3199"/>
      <source>Where should Mudlet save log files?</source>
      <translation>Mudlet 日志文件应该保存到哪里？</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgProfilePreferences.cpp" line="3622"/>
      <source>%n selected - change destinations...</source>
      <extracomment>text on button to select other profiles to receive the map from this profile, %n is the number of other profiles that have already been selected to receive it and will always be 1 or more</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3627"/>
      <source>pick destinations...</source>
      <extracomment>text on button to select other profiles to receive the map from this profile, this is used when no profiles have been selected</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3864"/>
      <source>Could not update themes: %1</source>
      <translation>无法更新主题: %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="3867"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>从colorsublime.github.io更新主题……</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4045"/>
      <source>{missing, possibly recently deleted trigger item}</source>
      <translation>{缺少, 可能是最近删除的触发器}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4048"/>
      <source>{missing, possibly recently deleted alias item}</source>
      <translation>{缺少, 可能是最近删除的别名}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4051"/>
      <source>{missing, possibly recently deleted script item}</source>
      <translation>{缺少, 可能是最近删除的脚本}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4054"/>
      <source>{missing, possibly recently deleted timer item}</source>
      <translation>{缺少, 可能是最近删除的定时器}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4057"/>
      <source>{missing, possibly recently deleted key item}</source>
      <translation>{缺少, 可能是最近删除的按键}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4060"/>
      <source>{missing, possibly recently deleted button item}</source>
      <translation>{缺少, 可能是最近删除的按钮}</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4189"/>
      <source>The room symbol will appear like this if only symbols (glyphs) from the specific font are used.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4194"/>
      <source>The room symbol will appear like this if symbols (glyphs) from any font can be used.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4234"/>
      <source>How many rooms in the whole map have this symbol.</source>
      <translation>整个地图中有多少房间具有此符号.</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4252"/>
      <source>The rooms with this symbol, up to a maximum of thirty-two, if there are more than this, it is indicated but they are not shown.</source>
      <translation>具有此符号的房间，最多可达三十二个，如果有超过三十二个，它们依然会被提示，但不会显示出来。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4260"/>
      <source>The symbol can be made entirely from glyphs in the specified font.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4278"/>
      <source>The symbol cannot be drawn using any of the fonts in the system, either an invalid string was entered as the symbol for the indicated rooms or the map was created on a different systems with a different set of fonts available to use. You may be able to correct this by installing an additional font using whatever method is appropriate for this system or by editing the map to use a different symbol. It may be possible to do the latter via a lua script using the &lt;i&gt;getRoomChar&lt;/i&gt; and &lt;i&gt;setRoomChar&lt;/i&gt; functions.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4371"/>
      <source>Large icon</source>
      <extracomment>Discord Rich Presence large icon</extracomment>
      <translation>大图标</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4373"/>
      <source>Detail</source>
      <extracomment>Discord Rich Presence detail</extracomment>
      <translation>详细信息</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4376"/>
      <source>Small icon</source>
      <extracomment>Discord Rich Presence small icon&quot;</extracomment>
      <translation>小图标</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4378"/>
      <source>State</source>
      <extracomment>Discord Rich Presence state</extracomment>
      <translation>狀態</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4381"/>
      <source>Party size</source>
      <extracomment>Discord Rich Presence party size</extracomment>
      <translation>队伍人数</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4383"/>
      <source>Party max</source>
      <extracomment>Discord Rich Presence maximum party size</extracomment>
      <translation>最大参与方</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4385"/>
      <source>Time</source>
      <extracomment>Discord Rich Presence time until or time elapsed</extracomment>
      <translation>時間</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5000"/>
      <source>Set outer color of player room mark.</source>
      <translation>设置玩家房间标记的外部颜色。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5000"/>
      <source>Set inner color of player room mark.</source>
      <translation>设置玩家房间标记的内部颜色。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="180"/>
      <source>&lt;p&gt;This option sets the format of the log name.&lt;/p&gt;&lt;p&gt;If &lt;i&gt;Named file&lt;/i&gt; is selected, you can set a custom file name. (Logs are appended if a log file of the same name already exists.)&lt;/p&gt;</source>
      <translation>&lt;p&gt;本选项设置日志名的格式。&lt;/p&gt;&lt;p&gt;如果&lt;i&gt;选择的文件已命名了&lt;/i&gt;，你可以设置自定义文件名。（如果有相同名字的日志文件已存在，则记录会附加上。）&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="767"/>
      <source>%1 - not recognised</source>
      <translation>%1 - 未识别</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="781"/>
      <source>&lt;p&gt;Mudlet does not recognise the code &quot;%1&quot;, please report it to the Mudlet developers so we can describe it properly in future Mudlet versions!&lt;/p&gt;&lt;p&gt;The file &lt;tt&gt;%2.dic&lt;/tt&gt; (and its companion affix &lt;tt&gt;.aff&lt;/tt&gt; file) is still usable.&lt;/p&gt;</source>
      <translation>&lt;p&gt;Mudelet无法识别的代码 &quot;%1&quot;，请向Mudelet开发者报告，以便我们可以在今后的 Mudelet 版本中正确识别它！&lt;/p&gt;&lt;p&gt;文件 &lt;tt&gt;%2.dic&lt;/tt&gt; (及其伴侣 &lt;tt&gt;.faf&lt;/tt&gt; 文件) 仍然可用。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="798"/>
      <source>No Hunspell dictionary files found, spell-checking will not be available.</source>
      <translation>未找到拼写检查器的字典文件，拼写检查将不可用。</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="919"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="922"/>
      <source>Mudlet will only show Rich Presence information while you use this Discord username (useful if you have multiple Discord accounts). Leave empty to show it for any Discord account you log in to. This must be the unique Discord username that uses a restricted lowercase ASCII character set and not any &quot;Nickname&quot; that you may have set for a particular Server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="931"/>
      <source>This is the unique username using a restricted character set for the Discord account, and not necessarily the nickname that you might have set for a particular Server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="933"/>
      <source>(Not connected)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2109"/>
      <source>[ WARN ]  - Could not clear the stored media: %1.</source>
      <extracomment>Shown after the &quot;Clear stored media&quot; button in preferences fails to empty the profile&apos;s media directory. %1 is the reason, which is not translated.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2821"/>
      <source>[ ERROR ] - Unable to load JSON map file: %1
reason: %2.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2861"/>
      <source>Any map file (*.dat *.json *.xml)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>任意地图文件 (*.dat *.json *.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2862"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="2895"/>
      <source>Mudlet binary map (*.dat)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>Mudlet 二进制地图 (*.dat)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2863"/>
      <location filename="../src/dlgProfilePreferences.cpp" line="2896"/>
      <source>Mudlet JSON map (*.json)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>Mudlet JSON 地图 (*.json)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2864"/>
      <source>Mudlet XML map (*.xml)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>Mudlet XML 地图 (*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="2865"/>
      <source>Any file (*)</source>
      <comment>Do not change extensions (in braces) as they are used programmatically</comment>
      <translation>任意文件 (*)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4222"/>
      <source>&lt;p&gt;These are the sequence of hexadecimal numbers that are used by the Unicode consortium to identify the graphemes needed to create the symbol.  These numbers can be utilised to determine precisely what is to be drawn even if some fonts have glyphs that are the same for different codepoints or combination of codepoints.&lt;/p&gt;&lt;p&gt;Character entry utilities such as &lt;i&gt;charmap.exe&lt;/i&gt; on &lt;i&gt;Windows&lt;/i&gt; or &lt;i&gt;gucharmap&lt;/i&gt; on many Unix type operating systems will also use these numbers which cover everything from U+0020 {Space} to U+10FFFD the last usable number in the &lt;i&gt;Private Use Plane 16&lt;/i&gt; via most of the written marks that humanity has ever made.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4246"/>
      <source>more - not shown...</source>
      <translation>更多 - 未显示...</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4269"/>
      <source>&lt;p&gt;The symbol cannot be made entirely from glyphs in the specified font, but, using other fonts in the system, it can. Either un-check the &lt;i&gt;Only use symbols (glyphs) from chosen font&lt;/i&gt; option or try and choose another font that does have the needed glyphs.&lt;/p&gt;&lt;p&gt;&lt;i&gt;You need not close this table to try another font, changing it on the main preferences dialogue will update this table after a slight delay.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4423"/>
      <source>Map symbol usage - %1</source>
      <translation>地图符号使用情况的 %1</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4533"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.html)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (例如, 1970-01-01#00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4534"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.html)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (例如, 1970-01-01T00-00-00.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4535"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.html)</source>
      <translation>yyyy-MM-dd(连接日志, 例如 1970-01-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4536"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.html)</source>
      <translation>yyyy-MM (连接的月份记录，例如 1970-01.html)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4539"/>
      <source>yyyy-MM-dd#HH-mm-ss (e.g., 1970-01-01#00-00-00.txt)</source>
      <translation>yyyy-MM-dd#HH-mm-ss (例如, 1970-01-01#00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4540"/>
      <source>yyyy-MM-ddTHH-mm-ss (e.g., 1970-01-01T00-00-00.txt)</source>
      <translation>yyyy-MM-ddTHH-mm-ss (例如, 1970-01-01T00-00-00.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4541"/>
      <source>yyyy-MM-dd (concatenate daily logs in, e.g. 1970-01-01.txt)</source>
      <translation>yyyy-MM-dd (连接日志, 例如 1970-01-01.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="4542"/>
      <source>yyyy-MM (concatenate month logs in, e.g. 1970-01.txt)</source>
      <translation>yyyy-MM (连接的月份记录，例如 1970-01.txt)</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5077"/>
      <source>New: undo the game&apos;s own wrapping</source>
      <extracomment>Title of a balloon pointing out a newly added feature</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5079"/>
      <source>Games that wrap their own lines make triggers fiddly. Mudlet can now undo that wrapping, so triggers always see whole lines.</source>
      <extracomment>Body of the balloon, anchored to the option that rejoins lines the game server wrapped itself so that triggers match whole lines</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5131"/>
      <source>Deleting map - please wait...</source>
      <translation>正在删除地图，请稍候…</translation>
    </message>
    <message>
      <location filename="../src/dlgProfilePreferences.cpp" line="5140"/>
      <source>Deleted map.</source>
      <translation>删除地图.</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomExits</name>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="276"/>
      <source>(roomID)</source>
      <comment>Placeholder, if no roomID is set for an exit.</comment>
      <translation>(房间ID)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="277"/>
      <source>(command or Lua script)</source>
      <comment>Placeholder, if a special exit has no name/script set.</comment>
      <translation>(命令或Lua脚本)</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="442"/>
      <location filename="../src/dlgRoomExits.cpp" line="446"/>
      <location filename="../src/dlgRoomExits.cpp" line="1025"/>
      <source>Set the number of the room that this special exit goes to.</source>
      <translation>设置该出口通向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="448"/>
      <location filename="../src/dlgRoomExits.cpp" line="1844"/>
      <source>Prevent a route being created via this exit, equivalent to an infinite exit weight.</source>
      <translation>禁止通过此出口创建的路由，这会创建一个死循环出口权重。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1009"/>
      <source>The roomID of the room that this special exit leads to is expected here. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1020"/>
      <source>Entered number is invalid. If left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation>输入的数字无效。如果这样离开，该出口将在点击 &lt;tt&gt;save&lt;/tt&gt; 时删除。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1021"/>
      <source>Set the number of the room that this special exit leads to.</source>
      <translation>设置此特殊出口通向的房间的编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1035"/>
      <source>No command or Lua script entered, if left like this, this exit will be deleted when &lt;tt&gt;save&lt;/tt&gt; is clicked.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1082"/>
      <location filename="../src/dlgRoomExits.cpp" line="1091"/>
      <source>Exit to &quot;%1&quot; in area: &quot;%2&quot;.</source>
      <translation>退出区域内的 &quot;%1&quot; ： &quot;%2&quot;。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1650"/>
      <source>This is the Room ID number for this room; this &lt;b&gt;room is locked&lt;/b&gt; so it will not be used for speed-walks at all.</source>
      <extracomment>This text is a revision to the default tooltip text set for this widget in the &apos;room_exits.ui&apos; file. Bold HTML tags are used to emphasis that this room&apos;s locked status overrides any weight or lock (&quot;No route&quot;) setting of any exit that comes to it.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1102"/>
      <location filename="../src/dlgRoomExits.cpp" line="1113"/>
      <source>Exit to unnamed room in area: &quot;%1&quot;, is valid.</source>
      <translation>离开未命名房间 : &quot;%1&quot;，有效。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="454"/>
      <location filename="../src/dlgRoomExits.cpp" line="1851"/>
      <source>Positive value overrides room weight; zero uses default.</source>
      <extracomment>Tooltip for exit weight column</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="458"/>
      <location filename="../src/dlgRoomExits.cpp" line="1861"/>
      <source>No door symbol drawn on the 2D map for this exit.</source>
      <extracomment>Tooltip for no door symbol option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="461"/>
      <location filename="../src/dlgRoomExits.cpp" line="1863"/>
      <source>Green (open) door symbol drawn on the 2D map.</source>
      <extracomment>Tooltip for open door symbol option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="464"/>
      <location filename="../src/dlgRoomExits.cpp" line="1865"/>
      <source>Orange (closed) door symbol drawn on the 2D map.</source>
      <extracomment>Tooltip for closed door symbol option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="467"/>
      <location filename="../src/dlgRoomExits.cpp" line="1867"/>
      <source>Red (locked) door symbol drawn on the 2D map.</source>
      <extracomment>Tooltip for locked door symbol option</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1030"/>
      <source>Invalid room ID: exit will be deleted on save.</source>
      <extracomment>Tooltip for invalid room ID in special exits</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1037"/>
      <source>Some mapper scripts may require prefixing the keyword &quot;script:&quot;).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1107"/>
      <location filename="../src/dlgRoomExits.cpp" line="1118"/>
      <source>Exit to unnamed room is valid.</source>
      <translation>有效的出口通往未命名的房间.</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1236"/>
      <location filename="../src/dlgRoomExits.cpp" line="1237"/>
      <location filename="../src/dlgRoomExits.cpp" line="1421"/>
      <location filename="../src/dlgRoomExits.cpp" line="1680"/>
      <source>Set the number of the room northwest of this one.</source>
      <translation>设置此房间西北方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1252"/>
      <location filename="../src/dlgRoomExits.cpp" line="1253"/>
      <location filename="../src/dlgRoomExits.cpp" line="1427"/>
      <location filename="../src/dlgRoomExits.cpp" line="1692"/>
      <source>Set the number of the room north of this one.</source>
      <translation>设置此房间北边方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1268"/>
      <location filename="../src/dlgRoomExits.cpp" line="1269"/>
      <location filename="../src/dlgRoomExits.cpp" line="1434"/>
      <location filename="../src/dlgRoomExits.cpp" line="1704"/>
      <source>Set the number of the room northeast of this one.</source>
      <translation>设置此房间东北方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1284"/>
      <location filename="../src/dlgRoomExits.cpp" line="1285"/>
      <location filename="../src/dlgRoomExits.cpp" line="1441"/>
      <location filename="../src/dlgRoomExits.cpp" line="1716"/>
      <source>Set the number of the room up from this one.</source>
      <translation>设置此房间向上方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1300"/>
      <location filename="../src/dlgRoomExits.cpp" line="1301"/>
      <location filename="../src/dlgRoomExits.cpp" line="1447"/>
      <location filename="../src/dlgRoomExits.cpp" line="1728"/>
      <source>Set the number of the room west of this one.</source>
      <translation>设置此房间西边方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1316"/>
      <location filename="../src/dlgRoomExits.cpp" line="1317"/>
      <location filename="../src/dlgRoomExits.cpp" line="1453"/>
      <location filename="../src/dlgRoomExits.cpp" line="1740"/>
      <source>Set the number of the room east of this one.</source>
      <translation>设置此房间东边方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1332"/>
      <location filename="../src/dlgRoomExits.cpp" line="1333"/>
      <location filename="../src/dlgRoomExits.cpp" line="1467"/>
      <location filename="../src/dlgRoomExits.cpp" line="1752"/>
      <source>Set the number of the room down from this one.</source>
      <translation>设置此房间向下方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1348"/>
      <location filename="../src/dlgRoomExits.cpp" line="1349"/>
      <location filename="../src/dlgRoomExits.cpp" line="1474"/>
      <location filename="../src/dlgRoomExits.cpp" line="1764"/>
      <source>Set the number of the room southwest of this one.</source>
      <translation>设置此房间西南方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1364"/>
      <location filename="../src/dlgRoomExits.cpp" line="1365"/>
      <location filename="../src/dlgRoomExits.cpp" line="1480"/>
      <location filename="../src/dlgRoomExits.cpp" line="1776"/>
      <source>Set the number of the room south of this one.</source>
      <translation>设置此房间南边方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1380"/>
      <location filename="../src/dlgRoomExits.cpp" line="1381"/>
      <location filename="../src/dlgRoomExits.cpp" line="1487"/>
      <location filename="../src/dlgRoomExits.cpp" line="1788"/>
      <source>Set the number of the room southeast of this one.</source>
      <translation>设置此房间东南方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1396"/>
      <location filename="../src/dlgRoomExits.cpp" line="1397"/>
      <location filename="../src/dlgRoomExits.cpp" line="1494"/>
      <location filename="../src/dlgRoomExits.cpp" line="1800"/>
      <source>Set the number of the room in from this one.</source>
      <translation>设置此房间向 in 方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1412"/>
      <location filename="../src/dlgRoomExits.cpp" line="1413"/>
      <location filename="../src/dlgRoomExits.cpp" line="1501"/>
      <location filename="../src/dlgRoomExits.cpp" line="1812"/>
      <source>Set the number of the room out from this one.</source>
      <translation>设置此房间向 out 方向的房间编号。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1086"/>
      <location filename="../src/dlgRoomExits.cpp" line="1095"/>
      <source>Exit to &quot;%1&quot;.</source>
      <translation>退出到 &quot;%1&quot;。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1084"/>
      <location filename="../src/dlgRoomExits.cpp" line="1088"/>
      <location filename="../src/dlgRoomExits.cpp" line="1104"/>
      <location filename="../src/dlgRoomExits.cpp" line="1109"/>
      <source>&lt;b&gt;Room is locked&lt;/b&gt;, it will not be used for speed-walks for any exit that leads to it.</source>
      <extracomment>Bold HTML tags are used to emphasis that destination room locked status overrides any weight or lock (&quot;No route&quot;) setting of any exit that goes to it.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1093"/>
      <location filename="../src/dlgRoomExits.cpp" line="1097"/>
      <location filename="../src/dlgRoomExits.cpp" line="1115"/>
      <location filename="../src/dlgRoomExits.cpp" line="1120"/>
      <source>&lt;b&gt;Room&lt;/b&gt; Weight of destination: %1.</source>
      <extracomment>Bold HTML tags are used to emphasis that the value is destination room&apos;s weight whether overridden by a non-zero exit weight here or not
----------
Bold HTML tags are used to emphasis that the value is destination room&apos;s weight whether overridden by a non-zero exit weight here or not.</extracomment>
      <translation>&lt;b&gt;房间&lt;/b&gt; 目的地权重： %1。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1198"/>
      <location filename="../src/dlgRoomExits.cpp" line="1615"/>
      <source>Clear the stub exit for this exit to enter an exit roomID.</source>
      <translation>已为当前出口清除异常未知的出口标记。</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1236"/>
      <location filename="../src/dlgRoomExits.cpp" line="1252"/>
      <location filename="../src/dlgRoomExits.cpp" line="1268"/>
      <location filename="../src/dlgRoomExits.cpp" line="1284"/>
      <location filename="../src/dlgRoomExits.cpp" line="1300"/>
      <location filename="../src/dlgRoomExits.cpp" line="1316"/>
      <location filename="../src/dlgRoomExits.cpp" line="1332"/>
      <location filename="../src/dlgRoomExits.cpp" line="1348"/>
      <location filename="../src/dlgRoomExits.cpp" line="1364"/>
      <location filename="../src/dlgRoomExits.cpp" line="1380"/>
      <location filename="../src/dlgRoomExits.cpp" line="1396"/>
      <location filename="../src/dlgRoomExits.cpp" line="1412"/>
      <source>Entered number is invalid.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1660"/>
      <source>Exits for room: &quot;%1&quot; [*]</source>
      <translation>房间出口: &quot;%1&quot; [*]</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomExits.cpp" line="1662"/>
      <source>Exits for room Id: %1 [*]</source>
      <translation>房间 Id 的出口: %1 [*]</translation>
    </message>
  </context>
  <context>
    <name>dlgRoomProperties</name>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="245"/>
      <source>Enter a new room weight to use as the travel time for all of the %n selected room(s). This will be used for calculating the best path. The minimum and default is 1.</source>
      <comment>%n is the total number of rooms involved.</comment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="251"/>
      <source>To change the room weight for all of the %n selected room(s), please choose:
 • an existing room weight from the list below (sorted by most commonly used first)
 • enter a new positive integer value to use as a new weight. The default is 1.</source>
      <comment>This is for when applying a new room weight to one or more rooms and some have different weights at present. %n is the total number of rooms involved.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="618"/>
      <source>Delete room color</source>
      <extracomment>This action deletes a color from the list of all room colors</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="646"/>
      <source>OK</source>
      <extracomment>confirm room color selection dialog</extracomment>
      <translation>好</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="652"/>
      <source>Cancel</source>
      <extracomment>cancel room color selection dialog</extracomment>
      <translation>取消</translation>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="701"/>
      <source>Set a custom border color and thickness for the selected room(s). Leave at default to use the global map settings.</source>
      <extracomment>Instruction text shown in room properties dialog for the border customization section</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="714"/>
      <source>Set border color</source>
      <extracomment>Title for the color picker dialog when selecting a room border color</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="541"/>
      <source>Set symbol color</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="193"/>
      <source>Lock %n room(s), so it/they will never be used for speedwalking</source>
      <extracomment>room properties dialog, text will be shown at a checkbox, where you can set/unset a number of room&apos;s lock.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="209"/>
      <source>Hide all %n room(s).%1</source>
      <extracomment>room properties dialog, setting text for checkbox, where you can set/unset a number of room&apos;s hidden status. More than one room is being considered and some, but not all (%n) of them are hidden and in this case the checkbox also has an partially checked state to be used to leave them all unchanged. A second translatable sentance indicating the number of currently hidden rooms will be inserted as %1.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="216"/>
      <source> %n room(s) are currently hidden.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="222"/>
      <source>Leave as partially checked to not change the state of the selected rooms.</source>
      <extracomment>Tooltip to give additional information for the checkbox to control the state of being hidden when the selection includes multiple rooms and they are not all in the same state.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="229"/>
      <source>Hide (all) %n room(s).</source>
      <extracomment>room properties dialog, setting text for checkbox, where you can set/unset the hidden status of one or more rooms where %n is the total number of rooms and all of them are currently hidden or shown.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="275"/>
      <source>Enter one or more characters to set a new symbol for %n room(s).  Clear to unset.</source>
      <comment>%n is the total number of rooms involved.</comment>
      <extracomment>room properties dialog, setting symbols</extracomment>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/dlgRoomProperties.cpp" line="281"/>
      <source>To set the symbol for all %n room(s), please choose:
 • an existing symbol from the list,
 • enter one or more characters to set a new symbol,
 • clear to unset.</source>
      <comment>This is for when applying a new room symbol to one or more rooms and some have different symbols or no symbol at present. %n is the total number of rooms involved.</comment>
      <extracomment>room properties dialog, setting symbols</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="323"/>
      <location filename="../src/dlgRoomProperties.cpp" line="358"/>
      <source>%1 (count: %2)</source>
      <extracomment>Format for showing a room symbol with its usage count. %1 is the symbol itself (e.g., &quot;★&quot; or &quot;!&quot;), %2 is the number of rooms using this symbol. Example output: &quot;★ (count: 5)&quot; or &quot;! (count: 12)&quot;. The word &quot;count&quot; and the format can be translated, but ensure the numbers remain clearly associated.
----------
Format for showing a room weight with its usage count. %1 is the weight value (e.g., &quot;1&quot; or &quot;50&quot;), %2 is the number of rooms with this weight. Example output: &quot;5 (count: 3)&quot; or &quot;100 (count: 7)&quot;. The word &quot;count&quot; and the format can be translated, but ensure the numbers remain clearly associated.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="580"/>
      <location filename="../src/dlgRoomProperties.cpp" line="637"/>
      <source>Define new room color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.cpp" line="603"/>
      <source>Set room color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgRoomProperties.h" line="97"/>
      <source>(Multiple values...)</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>dlgTriggerEditor</name>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="803"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8671"/>
      <location filename="../src/dlgTriggerEditor.h" line="596"/>
      <source>Triggers</source>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="804"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="805"/>
      <source>Show Triggers</source>
      <translation>顯示觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="833"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8699"/>
      <location filename="../src/dlgTriggerEditor.h" line="602"/>
      <source>Buttons</source>
      <translation>按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="834"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="835"/>
      <source>Show Buttons</source>
      <translation>顯示按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="808"/>
      <location filename="../src/dlgTriggerEditor.h" line="597"/>
      <source>Aliases</source>
      <translation>別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="809"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="810"/>
      <source>Show Aliases</source>
      <translation>顯示別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="818"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8678"/>
      <location filename="../src/dlgTriggerEditor.h" line="599"/>
      <source>Timers</source>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="819"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="820"/>
      <source>Show Timers</source>
      <translation>顯示時計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="813"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8685"/>
      <location filename="../src/dlgTriggerEditor.h" line="598"/>
      <source>Scripts</source>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="814"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="815"/>
      <source>Show Scripts</source>
      <translation>顯示腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="823"/>
      <location filename="../src/dlgTriggerEditor.h" line="600"/>
      <source>Keys</source>
      <translation>熱鍵</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="824"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="825"/>
      <source>Show Keybindings</source>
      <translation>顯示熱鍵</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="828"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="9202"/>
      <location filename="../src/dlgTriggerEditor.h" line="601"/>
      <source>Variables</source>
      <translation>變數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="829"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="830"/>
      <source>Show Variables</source>
      <translation>顯示變數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="856"/>
      <source>Activate</source>
      <translation>啟動／停止啟用</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="857"/>
      <source>Toggle Active or Non-Active Mode for Triggers, Scripts etc.</source>
      <translation>切換觸發、腳本等的啟用與停用狀態</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="875"/>
      <source>Delete Item</source>
      <translation>删除項目</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="899"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13299"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13308"/>
      <source>Copy</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="903"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="904"/>
      <source>Copy the trigger/script/alias/etc</source>
      <translation>复制触发器/脚本/别名/等等</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="913"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13300"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13309"/>
      <source>Paste</source>
      <translation>貼上</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="917"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="918"/>
      <source>Paste triggers/scripts/aliases/etc from the clipboard</source>
      <translation>貼上觸發／腳本／別名……等</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="958"/>
      <source>Import</source>
      <translation>匯入</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="962"/>
      <source>Export</source>
      <translation>匯出</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="971"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12994"/>
      <location filename="../src/dlgTriggerEditor.h" line="595"/>
      <source>Save Profile</source>
      <translation>儲存配置</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="987"/>
      <source>Save Profile As</source>
      <translation>另存設定</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="844"/>
      <location filename="../src/dlgTriggerEditor.h" line="604"/>
      <source>Statistics</source>
      <translation>統計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="327"/>
      <source>new folder</source>
      <extracomment>Accessible description for a newly created folder, shown after the folder name</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="329"/>
      <source>new item</source>
      <extracomment>Accessible description for a newly created item, shown after the item name</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="335"/>
      <source>%1 - Editor</source>
      <translation>%1 - 编辑器</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="723"/>
      <source>*** starting new session ***</source>
      <translation>*** 开始新的会话 ***</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="849"/>
      <location filename="../src/dlgTriggerEditor.h" line="605"/>
      <source>Debug</source>
      <translation>偵錯</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="995"/>
      <source>Something went wrong loading your Mudlet profile and it could not be loaded. Try loading an older version in &apos;Connect - Options - Profile history&apos;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1022"/>
      <source>Editor Toolbar - %1 - Actions</source>
      <extracomment>This is the toolbar that is initially placed at the top of the editor.</extracomment>
      <translation>编辑器工具栏 - %1 - 操作</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1063"/>
      <source>Editor Toolbar - %1 - Items</source>
      <extracomment>This is the toolbar that is initially placed at the left side of the editor.</extracomment>
      <translation>编辑器工具栏 - %1 - 项</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1073"/>
      <source>Restore Actions toolbar</source>
      <extracomment>This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.</extracomment>
      <translation>还原操作工具栏</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1076"/>
      <source>Restore Items toolbar</source>
      <extracomment>This will restore that toolbar in the editor window, after a user has hidden it or moved it to another docking location or floated it elsewhere.</extracomment>
      <translation>还原项目工具栏</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1241"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1244"/>
      <source>Search Options</source>
      <translation>搜尋選項</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1248"/>
      <source>Case sensitive</source>
      <translation>區分大小寫</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>start of line</source>
      <translation>行首</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5005"/>
      <source>New trigger group</source>
      <translation>新建触发器组</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5005"/>
      <source>New trigger</source>
      <translation>新增觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5111"/>
      <source>New timer group</source>
      <translation>新建计时器组</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5111"/>
      <source>New timer</source>
      <translation>新增計時</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5266"/>
      <source>New key group</source>
      <translation>新按键组</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5266"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7113"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7186"/>
      <source>New key</source>
      <translation>新按键</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5355"/>
      <source>New alias group</source>
      <translation>新建别名组</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5355"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="6237"/>
      <source>New alias</source>
      <translation>新增別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5450"/>
      <source>New menu</source>
      <translation>新建菜单</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5450"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="5477"/>
      <source>New button</source>
      <translation>新增按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5477"/>
      <source>New toolbar</source>
      <translation>新建工具栏</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5552"/>
      <source>New script group</source>
      <translation>新建脚本组</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5552"/>
      <source>New script</source>
      <translation>新建脚本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="6436"/>
      <source>Alias &lt;em&gt;%1&lt;/em&gt; has an infinite loop - substitution matches its own pattern. Please fix it - this alias isn&apos;t good as it&apos;ll call itself forever.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="6741"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8536"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13788"/>
      <source>While loading the profile, this script had an error that has since been fixed, possibly by another script. The error was:%2%3</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7071"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8275"/>
      <source>Checked variables will be saved and loaded with your profile.</source>
      <translation>选中的变量将被保存并会和您的配置文件一起加载。</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7298"/>
      <source>match on the prompt line</source>
      <translation>在提示行匹配</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7302"/>
      <source>match on the prompt line (disabled)</source>
      <translation>在提示行匹配(停用)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7303"/>
      <source>A Go-Ahead (GA) signal from the game is required to make this feature work</source>
      <translation>需要游戏中有Go-Ahead (GA) 信号才能使本功能运行</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7738"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7740"/>
      <source>fault</source>
      <translation>故障</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7590"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7710"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12895"/>
      <source>Foreground color ignored</source>
      <extracomment>Color trigger ignored foreground color button, ensure all three instances have the same text</extracomment>
      <translation>忽略的前景色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="127"/>
      <source>How to add a new alias from the input line</source>
      <extracomment>Name of a selectable option for the Alias intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="130"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="167"/>
      <source>There are a &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=22609&apos;&gt;couple&lt;/a&gt; of &lt;a href=&apos;https://forums.mudlet.org/viewtopic.php?f=6&amp;t=16462&apos;&gt;packages&lt;/a&gt; that can help you.</source>
      <extracomment>Help contents of a selectable option for the Alias intro
----------
Help contents of a selectable option for the Trigger intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="132"/>
      <source>Alias can also be defined from the input line in the main profile window like this:</source>
      <extracomment>Part of the Alias intro - This introductory text will be followed by a Lua code example for a trigger.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="135"/>
      <source>My greetings</source>
      <extracomment>Part of the Alias intro, code example for an alias - This is the name of the alias which reacts on the player typing &quot;hi&quot; by saying &quot;Greetings, traveller!&quot; in game.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="137"/>
      <source>hi</source>
      <extracomment>Part of the Alias intro, code example for an alias - This is the text input from the player which will be reacted on by saying &quot;Greetings, traveller!&quot; in game.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="139"/>
      <source>say Greetings, traveller!</source>
      <extracomment>Part of the Alias intro, code example for an alias - This is the command that Mudlet will send to the game after the player typed &quot;hi&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="141"/>
      <source>We said hi!</source>
      <extracomment>Part of the Alias intro, code example for an alias - This is the confirmation text shown to the player after they typed &quot;hi&quot; and we said &quot;Greetings, traveller!&quot; in game.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="144"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="179"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="204"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="229"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="250"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="272"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="298"/>
      <source>Where to find more information</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="146"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="181"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="206"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="274"/>
      <source>Watch a &lt;a href=&apos;%1&apos;&gt;video demonstration&lt;/a&gt; of the basic functionality.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="148"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Aliases&apos;&gt;Introduction to Aliases&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="149"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="184"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="209"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="232"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="253"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="277"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="301"/>
      <source>Do you maybe have any other suggestions, questions or doubts?</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="150"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="185"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="210"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="233"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="254"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="278"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="302"/>
      <source>Join our community on &lt;a href=&apos;https://www.mudlet.org/chat&apos;&gt;Discord&lt;/a&gt; or in &lt;a href=&apos;https://forums.mudlet.org/&apos;&gt;Mudlet forums&lt;/a&gt; - See you there!</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="164"/>
      <source>How to add a new trigger from the input line</source>
      <extracomment>Name of a selectable option for the Trigger intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="169"/>
      <source>Triggers can also be defined from the input line in the main profile window like this:</source>
      <extracomment>Part of the Trigger intro - This introductory text will be followed by a Lua code example for a trigger.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="172"/>
      <source>My drink trigger</source>
      <extracomment>Part of the Trigger intro, code example for a trigger - This is the name of the trigger which reacts on &quot;You are thirsty&quot; with &quot;drink water&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="174"/>
      <source>You are thirsty.</source>
      <extracomment>Part of the Trigger intro, code example for a trigger - This is the text from game which will be triggered on, and reacted to with &quot;drink water&quot;.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="176"/>
      <source>drink water</source>
      <extracomment>Part of the Trigger intro, code example for a trigger - This is the command sent to game after we triggered on text &quot;You are thirsty.&quot; from game.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="183"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Triggers&apos;&gt;Introduction to Triggers&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="208"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Scripts&apos;&gt;Introduction to Scripts&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="224"/>
      <source>How to add a new timer from the input line</source>
      <extracomment>Name of a selectable option for the Timer intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="231"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Timers&apos;&gt;Introduction to Timers&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="241"/>
      <source>&lt;ol&gt;&lt;li&gt;Add a new group to create a &lt;strong&gt;button bar&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Add groups as &lt;strong&gt;menus&lt;/strong&gt; or sub-menus.&lt;/li&gt;&lt;li&gt;Add items as &lt;strong&gt;buttons&lt;/strong&gt; to a bar or menu.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;command&lt;/strong&gt; or script to execute when pressed.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the item. &lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Deactivated items are hidden, including all items they contain.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Click-down buttons:&lt;/strong&gt; Can define separate commands for press/release. Use getButtonState() to check state.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Button intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="252"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Buttons&apos;&gt;Introduction to Buttons&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="267"/>
      <source>How to add a new keybinding from the input line</source>
      <extracomment>Name of a selectable option for the Keys intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="276"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Keybindings&apos;&gt;Introduction to Keybindings&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="293"/>
      <source>How to add a new variable from the input line</source>
      <extracomment>Name of a selectable option for the Variable intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="300"/>
      <source>Read the &lt;a href=&apos;http://wiki.mudlet.org/w/Manual:Introduction#Variables&apos;&gt;Introduction to Variables&lt;/a&gt; for a detailed overview.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="331"/>
      <source>package item</source>
      <extracomment>Accessible description indicating an item belongs to a package, shown after the item name. Keep short, as it&apos;s appended to other descriptions like &quot;activated, package item&quot;</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="460"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13295"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13304"/>
      <source>Undo</source>
      <translation>復原</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="473"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13296"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13305"/>
      <source>Redo</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="487"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1588"/>
      <source>Undo: %1 (%2)</source>
      <extracomment>Tooltip for undo action. %1 is the action being undone (e.g., &quot;Activate trigger &quot;foo&quot;&quot;), %2 is the keyboard shortcut</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="492"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1599"/>
      <source>Undo (%1)</source>
      <extracomment>Tooltip for undo action when no specific action. %1 is the keyboard shortcut</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="501"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1593"/>
      <source>Redo: %1 (%2)</source>
      <extracomment>Tooltip for redo action. %1 is the action being redone (e.g., &quot;Activate trigger &quot;foo&quot;&quot;), %2 is the keyboard shortcut</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="506"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="1602"/>
      <source>Redo (%1)</source>
      <extracomment>Tooltip for redo action when no specific action. %1 is the keyboard shortcut</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="853"/>
      <source>Show/Hide Debug Console (%1) -&gt; system will be &lt;b&gt;&lt;i&gt;slower&lt;/i&gt;&lt;/b&gt;.</source>
      <extracomment>%1 is a keyboard shortcut, e.g. &apos;Ctrl+0&apos; on Windows/Linux or &apos;⌘0&apos; on macOS</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="868"/>
      <source>Add Item</source>
      <translation>新增項目</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="966"/>
      <source>Create Module</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="968"/>
      <source>&lt;p&gt;Create a module from selected items&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="973"/>
      <source>&lt;p&gt;Saves your profile. (%1)&lt;/p&gt;&lt;p&gt;Saves your entire profile (triggers, aliases, scripts, timers, buttons and keys, but not the map or script-specific settings) to your computer disk, so in case of a computer or program crash, all changes you have done will be retained.&lt;/p&gt;&lt;p&gt;It also makes a backup of your profile, you can load an older version of it when connecting.&lt;/p&gt;&lt;p&gt;Should there be any modules that are marked to be &quot;&lt;i&gt;synced&lt;/i&gt;&quot; this will also cause them to be saved and reloaded into other profiles if they too are active.&lt;/p&gt;</source>
      <extracomment>%1 is a keyboard shortcut, e.g. &apos;Ctrl+Shift+S&apos; on Windows/Linux or &apos;⌘⇧S&apos; on macOS</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1260"/>
      <source>Whole word</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1262"/>
      <source>Only match whole words</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1748"/>
      <source>Text to find (anywhere in the game output)</source>
      <translation>要查找的文本 ( 在游戏输出的全部缓存中)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1750"/>
      <source>Text to find (as a regular expression pattern)</source>
      <translation>要查找的文本 ( 正则表达式模式)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1752"/>
      <source>Text to find (from beginning of the line)</source>
      <translation>要查找的文本（从行开头开始）</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1754"/>
      <source>Exact line to match</source>
      <translation>精确行匹配</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1756"/>
      <source>Lua code to run (return true to match)</source>
      <translation>要运行的 lua 代码 ( 返回 true 以匹配)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4008"/>
      <source>&lt;p&gt;Unable to activate &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;: %2&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4115"/>
      <source>move items</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4308"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;: %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="4456"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4585"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4748"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="4920"/>
      <source>&lt;p&gt;&lt;b&gt;Unable to activate &quot;&lt;tt&gt;%1&lt;/tt&gt;&quot;; %2.&lt;/b&gt;&lt;/p&gt;
                     &lt;p&gt;&lt;i&gt;You will need to reactivate this after the problem has been corrected.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5216"/>
      <source>table_variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5216"/>
      <source>variable_name</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="5845"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7816"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7897"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7980"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8427"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8551"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8639"/>
      <source>This item is part of a package. To best preserve your changes, copy this item before editing as package upgrades may overwrite modifications.</source>
      <extracomment>Package item warning shown in trigger editor when editing package items. Should only be announced to screen readers once per item, not repeatedly on every edit.
----------
Package item warning banner shown in trigger editor when selecting package items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7594"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7714"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12898"/>
      <source>Default foreground color</source>
      <extracomment>Color trigger default foreground color button, ensure all three instances have the same text</extracomment>
      <translation>设置前景色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7598"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7718"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12901"/>
      <source>Foreground color [ANSI %1]</source>
      <extracomment>Color trigger ANSI foreground color button, ensure all three instances have the same text</extracomment>
      <translation>前景色 [ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7604"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7724"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12958"/>
      <source>Background color ignored</source>
      <extracomment>Color trigger ignored background color button, ensure all three instances have the same text</extracomment>
      <translation>忽略的背景色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7608"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7728"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12961"/>
      <source>Default background color</source>
      <extracomment>Color trigger default background color button, ensure all three instances have the same text</extracomment>
      <translation>默认背景颜色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7612"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7732"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12964"/>
      <source>Background color [ANSI %1]</source>
      <extracomment>Color trigger ANSI background color button, ensure all three instances have the same text</extracomment>
      <translation>背景色[ANSI %1]</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7793"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7797"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12755"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12799"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13457"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13459"/>
      <source>keep</source>
      <extracomment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</extracomment>
      <translation>保持</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="7822"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7903"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="7986"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8433"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8557"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="8645"/>
      <source>Package item. Copy before editing to preserve changes.</source>
      <extracomment>First-time educational message for screen reader users about package items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8347"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12718"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8389"/>
      <source>Menu properties</source>
      <translation>菜单属性</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8399"/>
      <source>Button properties</source>
      <translation>按鈕屬性</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8407"/>
      <source>Command (down);</source>
      <translation>命令 (Down);</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8692"/>
      <source>Aliases - Input Triggers</source>
      <translation>别名 - 输入触发器</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="8706"/>
      <source>Key Bindings</source>
      <translation>熱鍵</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9936"/>
      <source>Add Trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9937"/>
      <source>Add new trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9938"/>
      <source>Add Trigger Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9939"/>
      <source>Add new group of triggers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9940"/>
      <source>Delete Trigger</source>
      <translation>刪除觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9941"/>
      <source>Delete the selected trigger</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9942"/>
      <location filename="../src/dlgTriggerEditor.h" line="588"/>
      <source>Save Trigger</source>
      <translation>儲存觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9947"/>
      <source>Add Timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9948"/>
      <source>Add new timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9949"/>
      <source>Add Timer Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9950"/>
      <source>Add new group of timers</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9951"/>
      <source>Delete Timer</source>
      <translation>刪除時計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9952"/>
      <source>Delete the selected timer</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9953"/>
      <location filename="../src/dlgTriggerEditor.h" line="589"/>
      <source>Save Timer</source>
      <translation>儲存時計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9958"/>
      <source>Add Alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9959"/>
      <source>Add new alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9960"/>
      <source>Add Alias Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9961"/>
      <source>Add new group of aliases</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9962"/>
      <source>Delete Alias</source>
      <translation>刪除別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9963"/>
      <source>Delete the selected alias</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9964"/>
      <location filename="../src/dlgTriggerEditor.h" line="590"/>
      <source>Save Alias</source>
      <translation>儲存別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9969"/>
      <source>Add Script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9970"/>
      <source>Add new script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9971"/>
      <source>Add Script Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9972"/>
      <source>Add new group of scripts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9973"/>
      <source>Delete Script</source>
      <translation>刪除腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9974"/>
      <source>Delete the selected script</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9975"/>
      <location filename="../src/dlgTriggerEditor.h" line="591"/>
      <source>Save Script</source>
      <translation>儲存腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9980"/>
      <source>Add Button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9981"/>
      <source>Add new button</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9982"/>
      <source>Add Toolbar or Menu</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9983"/>
      <source>Add a Toolbar (top level) or Menu (lower levels) to contain menus or buttons</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9984"/>
      <source>Delete Button, Menu or Toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9985"/>
      <source>Delete the selected button, menu or toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9986"/>
      <source>Save item</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9988"/>
      <source>Apply button/menu/toolbar changes (does not save to disk).</source>
      <extracomment>Status tip for saving button changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.h" line="592"/>
      <source>Save Button</source>
      <translation>儲存按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9991"/>
      <source>Add Key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9992"/>
      <source>Add new key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9993"/>
      <source>Add Key Group</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9994"/>
      <source>Add new group of keys</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9995"/>
      <source>Delete Key</source>
      <translation>刪除熱鍵</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9996"/>
      <source>Delete the selected key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9997"/>
      <location filename="../src/dlgTriggerEditor.h" line="593"/>
      <source>Save Key</source>
      <translation>儲存熱鍵</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10002"/>
      <source>Add Variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10003"/>
      <source>Add new variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10004"/>
      <source>Add Lua table</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10005"/>
      <source>Add new Lua table</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10006"/>
      <source>Delete Variable</source>
      <translation>刪除變數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10007"/>
      <source>Delete the selected variable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10008"/>
      <location filename="../src/dlgTriggerEditor.h" line="594"/>
      <source>Save Variable</source>
      <translation>儲存變數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10808"/>
      <source>Central Debug Console</source>
      <translation>中央调试控制台</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11090"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11094"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11114"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11118"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11138"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11142"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11162"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11166"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11186"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11190"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11210"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11215"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11228"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11245"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11292"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11309"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11348"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11365"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11404"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11421"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11460"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11477"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11516"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11533"/>
      <source>Export Package:</source>
      <translation>导出包:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11090"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11094"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11114"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11118"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11138"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11142"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11162"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11166"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11186"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11190"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11210"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11215"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11228"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11292"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11348"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11404"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11460"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11516"/>
      <source>You have to choose an item for export first. Please select a tree item and then click on export again.</source>
      <translation>您必须先选择要导出的项目。 请选择一个树项，然后再次点击导出。</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11099"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11123"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11147"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11171"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11195"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11220"/>
      <source>Package %1 saved</source>
      <translation>包 %1 已保存</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11245"/>
      <source>No valid triggers found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11253"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11316"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11372"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11428"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11484"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="11540"/>
      <source>Copied %1 to clipboard</source>
      <translation>复制的 %1至到剪贴板</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11257"/>
      <source>Copied %1 triggers to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11309"/>
      <source>No valid timers found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11319"/>
      <source>Copied %1 timers to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11365"/>
      <source>No valid aliases found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11375"/>
      <source>Copied %1 aliases to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11421"/>
      <source>No valid actions found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11431"/>
      <source>Copied %1 actions to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11477"/>
      <source>No valid scripts found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11487"/>
      <source>Copied %1 scripts to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11533"/>
      <source>No valid keys found to export.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11543"/>
      <source>Copied %1 keys to clipboard</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11578"/>
      <source>Mudlet packages (*.xml)</source>
      <translation>Mudlet包(*.xml)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11578"/>
      <source>Export Item</source>
      <translation>导出项目</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11595"/>
      <source>export package:</source>
      <translation>导出包:</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11595"/>
      <source>Cannot write file %1:
%2.</source>
      <translation>无法写入文件 %1：
%2。</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11893"/>
      <source>Pasted %1 items successfully</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="11913"/>
      <source>paste</source>
      <extracomment>Undo/redo text for pasting items</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12401"/>
      <source>Import Mudlet Package</source>
      <extracomment>Trigger editor - import packages from file dialog (multi-select enabled) Trigger editor - file filter for supported package types (mpackage, zip, xml)</extracomment>
      <translation>导入Mudlet包</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12401"/>
      <source>Mudlet Packages (*.mpackage *.zip *.xml)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12444"/>
      <source>Failed to import: %1</source>
      <extracomment>Trigger editor - status message shown when some packages failed to import. %1 is a comma-separated list of package names</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12534"/>
      <source>Couldn&apos;t save profile</source>
      <translation>無法儲存使用者設定文件</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12534"/>
      <source>Sorry, couldn&apos;t save your profile - got the following error: %1</source>
      <translation>抱歉，以下錯誤導致無法儲存使用者設定文件：%1</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12544"/>
      <source>Backup Profile</source>
      <translation>备份配置文件</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12544"/>
      <source>trigger files (*.trigger *.xml)</source>
      <translation>触发器文件（*.trigger *.xml）</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12743"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="12787"/>
      <source>Keep color</source>
      <extracomment>Button in the color picker that preserves the existing text color on trigger matches</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12821"/>
      <source>Audio files(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;All files(*.*)</source>
      <extracomment>This the list of file extensions that are considered for sounds from triggers, the terms inside of the &apos;(&apos;...&apos;)&apos; and the &quot;;;&quot; are used programmatically and should not be changed.</extracomment>
      <translation>音频文件(*.aac *.mp3 *.mp4a *.oga *.ogg *.pcm *.wav *.wma);;Advanced Audio Coding-stream(*.aac);;MPEG-2 Audio Layer 3(*.mp3);;MPEG-4 Audio(*.mp4a);;Ogg Vorbis(*.oga *.ogg);;PCM Audio(*.pcm);;Wave(*.wav);;Windows Media Audio(*.wma);;所有文件(*.*)</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="14469"/>
      <source>Banner hidden. &lt;a href=&apos;undo&apos; style=&apos;color: inherit; text-decoration: underline;&apos;&gt;Undo&lt;/a&gt; | &lt;a href=&apos;hide-permanently&apos; style=&apos;color: inherit; text-decoration: underline;&apos;&gt;Hide permanently&lt;/a&gt;</source>
      <extracomment>Toast notification shown when user dismisses an editor tip banner. Allows them to undo or permanently hide the tips for this editor view type.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12714"/>
      <source>Command (down):</source>
      <translation>命令 (Down):</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9944"/>
      <source>Apply trigger changes (does not save to disk).</source>
      <extracomment>Status tip for saving trigger changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9955"/>
      <source>Apply timer changes (does not save to disk).</source>
      <extracomment>Status tip for saving timer changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9966"/>
      <source>Apply alias changes (does not save to disk).</source>
      <extracomment>Status tip for saving alias changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9977"/>
      <source>Apply script changes (does not save to disk).</source>
      <extracomment>Status tip for saving script changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="9999"/>
      <source>Apply key changes (does not save to disk).</source>
      <extracomment>Status tip for saving key changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="10010"/>
      <source>Apply variable changes (does not save to disk).</source>
      <extracomment>Status tip for saving variable changes</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12736"/>
      <source>Select foreground color to apply to matches</source>
      <translation>选择要应用到匹配项的前景色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12780"/>
      <source>Select background color to apply to matches</source>
      <translation>选择要应用于匹配项的背景色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12818"/>
      <source>Choose sound file</source>
      <translation>选择声音文件</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12874"/>
      <source>Select foreground trigger color for item %1</source>
      <translation>为项目 %1 选择前景触发器颜色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12938"/>
      <source>Select background trigger color for item %1</source>
      <translation>为项目 %1 选择背景触发器颜色</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="12987"/>
      <source>Saving…</source>
      <translation>正在保存…</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="13292"/>
      <source>Format All</source>
      <translation>全部格式化</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="13298"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13307"/>
      <source>Cut</source>
      <translation>剪下</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="13302"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="13311"/>
      <source>Select All</source>
      <translation>全選</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="13473"/>
      <source>Sound file to play when the trigger fires.</source>
      <translation>触发器触发时要播放的声音文件。</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>substring</source>
      <translation>子字串</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="118"/>
      <source>Alias react on user input.</source>
      <extracomment>Headline for the Alias intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="120"/>
      <source>How to add a new alias now</source>
      <extracomment>Name of a selectable option for the Alias intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="122"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define an input &lt;strong&gt;pattern&lt;/strong&gt; either literally or with a Perl regular expression.&lt;/li&gt;&lt;li&gt;Define a &apos;substitution&apos; &lt;strong&gt;command&lt;/strong&gt; to send to the game in clear text &lt;strong&gt;instead of the alias pattern&lt;/strong&gt;, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the alias.&lt;/li&gt;&lt;/ol&gt;</source>
      <extracomment>Help contents of a selectable option for the Alias intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="154"/>
      <source>Triggers react on game output.</source>
      <extracomment>Headline for the Trigger intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="156"/>
      <source>How to add a new trigger now</source>
      <extracomment>Name of a selectable option for the Trigger intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="158"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define a &lt;strong&gt;pattern&lt;/strong&gt; that you want to trigger on.&lt;/li&gt;&lt;li&gt;Select the appropriate pattern &lt;strong&gt;type&lt;/strong&gt;.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the trigger finds the pattern in the text from the game, or write a script for more complicated needs..&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the trigger.&lt;/li&gt;&lt;/ol&gt;</source>
      <extracomment>Help contents of a selectable option for the Trigger intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="189"/>
      <source>Scripts organize code and can react to events.</source>
      <extracomment>Headline for the Script intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="191"/>
      <source>How to add a new script now</source>
      <extracomment>Name of a selectable option for the Script intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="193"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Enter a script in the box below. You can for example define &lt;strong&gt;functions&lt;/strong&gt; to be called by other triggers, aliases, etc.&lt;/li&gt;&lt;li&gt;If you write lua &lt;strong&gt;commands&lt;/strong&gt; without defining a function, they will be run on Mudlet startup and each time you open the script for editing.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the script.&lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Scripts are run automatically when viewed, even if they are deactivated.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Script intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="199"/>
      <source>How to have a script react to events</source>
      <extracomment>Name of a selectable option for the Script intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="201"/>
      <source>&lt;p&gt;You can register a list of &lt;strong&gt;events&lt;/strong&gt; with the + and - symbols. If one of these events take place, the function with the same name as the script item itself will be called.&lt;/p&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Events can also be added to a script from the command line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua registerAnonymousEventHandler(&amp;quot;nameOfTheMudletEvent&amp;quot;, &amp;quot;nameOfYourFunctionToBeCalled&amp;quot;)&lt;/code&gt;&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Script intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="214"/>
      <source>Timers react after a timespan once or regularly.</source>
      <extracomment>Headline for the Timer intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="216"/>
      <source>How to add a new timer now</source>
      <extracomment>Name of a selectable option for the Timer intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="218"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Define the &lt;strong&gt;timespan&lt;/strong&gt; after which the timer should react in a this format: hours : minutes : seconds.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game when the time has passed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the timer.&lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; If you want the trigger to react only once and not regularly, use the Lua tempTimer() function instead.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Timer intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="226"/>
      <source>&lt;p&gt;Timers can also be defined from the input line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua tempTimer(3, function() echo(&amp;quot;hello!
&amp;quot;) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will greet you exactly 3 seconds after it was made.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Timer intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="237"/>
      <source>Buttons react on mouse clicks.</source>
      <extracomment>Headline for the Button intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="239"/>
      <source>How to add a new button now</source>
      <extracomment>Name of a selectable option for the Button intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="258"/>
      <source>Keys react on keyboard presses.</source>
      <extracomment>Headline for the Keys intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="260"/>
      <source>How to add a new keybinding now</source>
      <extracomment>Name of a selectable option for the Keys intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="262"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above.&lt;/li&gt;&lt;li&gt;Click on &lt;strong&gt;&apos;grab key&apos;&lt;/strong&gt; and then press your key combination, e.g. including modifier keys like Control, Shift, etc.&lt;/li&gt;&lt;li&gt;Define a clear text &lt;strong&gt;command&lt;/strong&gt; that you want to send to the game if the button is pressed, or write a script for more complicated needs.&lt;/li&gt;&lt;li&gt;&lt;strong&gt;Activate&lt;/strong&gt; the new key binding.&lt;/li&gt;&lt;/ol&gt;</source>
      <extracomment>Help contents of a selectable option for the Keys intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="269"/>
      <source>&lt;p&gt;Keys can be defined from the input line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua permKey(&amp;quot;my jump key&amp;quot;, &amp;quot;&amp;quot;, mudlet.key.F8, [[send(&amp;quot;jump&amp;quot;]]) end)&lt;/code&gt;&lt;/p&gt;&lt;p&gt;Pressing F8 will make you jump.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Keys intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="282"/>
      <source>Variables store information.</source>
      <extracomment>Headline for the Variable intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="284"/>
      <source>How to add a new variable now</source>
      <extracomment>Name of a selectable option for the Variable intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="286"/>
      <source>&lt;ol&gt;&lt;li&gt;Click on the &apos;Add Item&apos; icon above. To add a table instead click &apos;Add Group&apos;.&lt;/li&gt;&lt;li&gt;Select type of variable value (can be a string, integer, boolean)&lt;/li&gt;&lt;li&gt;Enter the value you want to store in this variable.&lt;/li&gt;&lt;li&gt;If you want to keep the variable in your next Mudlet sessions, check the checkbox in the list of variables to the left.&lt;/li&gt;&lt;li&gt;To remove a variable manually, set it to &apos;nil&apos; or click on the &apos;Delete&apos; icon above.&lt;/li&gt;&lt;/ol&gt;&lt;p&gt;&lt;strong&gt;Note:&lt;/strong&gt; Variables created here won&apos;t be saved when Mudlet shuts down unless you check their checkbox in the list of variables to the left. You could also create scripts with the variables instead.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Variable intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="295"/>
      <source>&lt;p&gt;Variables and tables can also be defined from the input line in the main profile window like this:&lt;/p&gt;&lt;p&gt;&lt;code&gt;lua foo = &amp;quot;bar&amp;quot;&lt;/code&gt;&lt;/p&gt;&lt;p&gt;This will create a string called &apos;foo&apos; with &apos;bar&apos; as its value.&lt;/p&gt;</source>
      <extracomment>Help contents of a selectable option for the Variable intro</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="307"/>
      <source>activated</source>
      <extracomment>Item is currently on, short enough to be spoken</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="309"/>
      <source>deactivated</source>
      <extracomment>Item is currently off, short enough to be spoken</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="311"/>
      <source>activated folder</source>
      <extracomment>Folder is currently turned on</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="313"/>
      <source>deactivated folder</source>
      <extracomment>Folder is currently turned off</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="315"/>
      <source>deactivated due to error</source>
      <extracomment>Item is currently inactive because of errors, short enough to be spoken</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="317"/>
      <source>%1 in a deactivated group</source>
      <extracomment>Item is currently turned on individually, but is member of an inactive group</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="319"/>
      <source>activated filter chain</source>
      <extracomment>A trigger that unlocks other triggers is currently turned on, short enough to be spoken</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="321"/>
      <source>deactivated filter chain</source>
      <extracomment>A trigger that unlocks other triggers is currently turned off, short enough to be spoken</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="323"/>
      <source>activated offset timer</source>
      <extracomment>A timer that starts after another timer is currently turned on</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="325"/>
      <source>deactivated offset timer</source>
      <extracomment>A timer that starts after another timer is currently turned off</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="402"/>
      <source>-- add your Lua code here</source>
      <translation>-- 在这里添加你的 Lua 代码</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="839"/>
      <location filename="../src/dlgTriggerEditor.h" line="603"/>
      <source>Errors</source>
      <translation>錯誤</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="840"/>
      <source>Show/Hide the errors console in the bottom right of this editor.</source>
      <translation>在编辑器的右下方显示/隐藏错误控制台。</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="841"/>
      <source>Show/Hide errors console</source>
      <translation>显示/隐藏错误控制台</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="845"/>
      <source>Generate a statistics summary display on the main profile console.</source>
      <translation>在主配置控制台生成一个统计摘要显示</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="846"/>
      <source>Generate statistics</source>
      <translation>生成统计</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="850"/>
      <source>Show/Hide the separate Central Debug Console - when being displayed the system will be slower.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="985"/>
      <source>Save profile (triggers, aliases, scripts, timers, buttons, keys - not the map) and synchronize modules.</source>
      <extracomment>Status tip for saving profile</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1250"/>
      <source>Match case precisely</source>
      <translation>精确匹配大小写</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1254"/>
      <source>Include variables</source>
      <translation>包含变量</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1256"/>
      <source>Search variables (slower)</source>
      <translation>搜索变量（较慢）</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1308"/>
      <source>Type</source>
      <extracomment>Heading for the first column of the search results</extracomment>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1312"/>
      <source>Where</source>
      <translation>地点</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1314"/>
      <source>What</source>
      <translation>什么</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>perl regex</source>
      <translation>Perl 正規表達式</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>exact match</source>
      <translation>完全符合</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>lua function</source>
      <translation>Lua 函數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>line spacer</source>
      <translation>行间距</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>color trigger</source>
      <translation>顏色觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1392"/>
      <source>prompt</source>
      <translation>提示</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2812"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2821"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2861"/>
      <source>Trigger</source>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="1310"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2443"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2493"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2527"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2611"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2699"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2753"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2812"/>
      <source>Name</source>
      <translation>名稱</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2502"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2507"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2536"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2541"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2620"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2625"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2762"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2767"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2821"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2826"/>
      <source>Command</source>
      <translation>指令</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2846"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2851"/>
      <source>Pattern {%1}</source>
      <translation>模式 {%1}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2577"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2582"/>
      <source>Lua code (%1:%2)</source>
      <translation>Lua 代碼（%1：%2）</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2753"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2762"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2778"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2791"/>
      <source>Alias</source>
      <translation>別名</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2778"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2783"/>
      <source>Pattern</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2699"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2717"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2732"/>
      <source>Script</source>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2717"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2722"/>
      <source>Event Handler</source>
      <translation>事件处理程序</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2611"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2620"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2637"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2663"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2678"/>
      <source>Button</source>
      <translation>按鈕</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="883"/>
      <source>Add Group (%1)</source>
      <extracomment>%1 is a keyboard shortcut, e.g. &apos;Ctrl+Shift+N&apos; on Windows/Linux or &apos;⌘⇧N&apos; on macOS</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="893"/>
      <source>&lt;p&gt;Saves the selected item. (%1)&lt;/p&gt;&lt;p&gt;Saving causes any changes to the item to take effect. It will not save to disk, so changes will be lost in case of a computer/program crash (but Save Profile to the right will be secure.)&lt;/p&gt;</source>
      <extracomment>%1 is a keyboard shortcut, e.g. &apos;Ctrl+S&apos; on Windows/Linux or &apos;⌘S&apos; on macOS</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2620"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2625"/>
      <source>Command {Down}</source>
      <translation>命令 {Down}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2637"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2642"/>
      <source>Command {Up}</source>
      <translation>命令 {Up}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2663"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2668"/>
      <source>Stylesheet {L: %1 C: %2}</source>
      <translation>样式表 {L: %1 C: %2}</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2493"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2502"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2515"/>
      <source>Timer</source>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2527"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2536"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2549"/>
      <source>Key</source>
      <translation>按键</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2443"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2457"/>
      <source>Variable</source>
      <translation>變數</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.cpp" line="2457"/>
      <location filename="../src/dlgTriggerEditor.cpp" line="2463"/>
      <source>Value</source>
      <translation>數值</translation>
    </message>
    <message>
      <location filename="../src/dlgTriggerEditor.h" line="587"/>
      <source>Save Item</source>
      <translation>儲存項目</translation>
    </message>
  </context>
  <context>
    <name>dlgVarsMainArea</name>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="54"/>
      <location filename="../src/dlgVarsMainArea.cpp" line="81"/>
      <source>Auto-Type</source>
      <translation>自动类型</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="55"/>
      <source>key (string)</source>
      <translation>按键(字符串)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="56"/>
      <source>index (integer number)</source>
      <translation>索引(整数)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="57"/>
      <source>table (use &quot;Add Group&quot; to create)</source>
      <translation>table (使用 &quot;Add Group&quot; 来创建)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="58"/>
      <source>function (cannot create from GUI)</source>
      <translation>函数(无法从GUI创建)</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="82"/>
      <source>string</source>
      <translation>字符串</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="83"/>
      <source>number</source>
      <translation>数字</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="84"/>
      <source>boolean</source>
      <translation>布尔值</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="85"/>
      <source>table</source>
      <translation>表</translation>
    </message>
    <message>
      <location filename="../src/dlgVarsMainArea.cpp" line="86"/>
      <source>function</source>
      <translation>函数</translation>
    </message>
  </context>
  <context>
    <name>edbee::TextEditorComponent</name>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="562"/>
      <source>Cut</source>
      <translation>剪下</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="563"/>
      <source>Copy</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="564"/>
      <source>Paste</source>
      <translation>貼上</translation>
    </message>
    <message>
      <location filename="../3rdparty/edbee-lib/edbee-lib/edbee/views/components/texteditorcomponent.cpp" line="566"/>
      <source>Select All</source>
      <translation>全选</translation>
    </message>
  </context>
  <context>
    <name>irc</name>
    <message>
      <location filename="../src/ui/irc.ui" line="25"/>
      <source>Mudlet IRC Client</source>
      <translation>Mudlet IRC 客户端</translation>
    </message>
  </context>
  <context>
    <name>keybindings_main_area</name>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, ideally unique, name for your key or key group. This will be displayed in the key tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;为你的按键或按键组选择一个好的、唯一的名字。它将显示在按键树上。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="61"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="90"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="100"/>
      <source>&lt;p&gt;Enter one or more commands to use if the given command matches the pattern. (Optional)&lt;/p&gt;&lt;p&gt;This could be another alias or a command to send directly to the game. For complex commands that require modification of variables within this profile, use a Lua script in the editor area below instead. It&apos;s possible to use both this field and a Lua script - the contents of this field will be used before running the script.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="103"/>
      <source>Text to send to the game (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="110"/>
      <source>Key Binding:</source>
      <translation>按键绑定：</translation>
    </message>
    <message>
      <location filename="../src/ui/keybindings_main_area.ui" line="127"/>
      <source>Grab New Key</source>
      <translation>抓取新按键</translation>
    </message>
  </context>
  <context>
    <name>lacking_mapper_script</name>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="23"/>
      <source>No mapping script found</source>
      <translation>未找到相关联的脚本</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="35"/>
      <source>&lt;p&gt;It seems that you don&apos;t have any &lt;a href=&quot;http://wiki.mudlet.org/w/Mapping_script&quot;&gt;mapping scripts&lt;/a&gt; installed yet - the mapper needs you to have one for your game, so it can track where you are and autowalk you. You can either make one yourself, or import an existing one that someone else made.&lt;/p&gt;&lt;p&gt;Would you like to see if any are available?&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="86"/>
      <source>Close</source>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../src/ui/lacking_mapper_script.ui" line="93"/>
      <source>Find some scripts</source>
      <translation>查找脚本</translation>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <location filename="../src/main.cpp" line="428"/>
      <source>Warning: %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="439"/>
      <source>       -h, --help                   displays this message.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="440"/>
      <source>       -v, --version                displays version information.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="442"/>
      <source>       -p, --profile=&lt;profile&gt;      additional profile to open, may be
                                    repeated.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="445"/>
      <source>       -o, --only=&lt;predefined&gt;      make Mudlet only show the specific
                                    predefined game, may be repeated.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="448"/>
      <source>       -f, --fullscreen             start Mudlet in fullscreen mode.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="449"/>
      <source>       --steammode                  adjusts Mudlet settings to match
                                    Steam&apos;s requirements.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="452"/>
      <source>There are other inherited options that arise from the Qt Libraries which are
less likely to be useful for normal use of this application:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="456"/>
      <source>       --dograb                     ignore any implicit or explicit -nograb.
                                    --dograb wins over --nograb even when --nograb is last on
                                    the command line.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="461"/>
      <source>       --nograb                     the application should never grab the mouse or the
                                    keyboard. This option is set by default when Mudlet is
                                    running in the gdb debugger under Linux.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="466"/>
      <source>       --nograb                     the application should never grab the mouse or the
                                    keyboard.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="470"/>
      <source>       --reverse                    sets the application&apos;s layout direction to right to left.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="471"/>
      <source>       --style=style                sets the application GUI style. Possible values depend on
                                    your system configuration. If Qt was compiled with
                                    additional styles or has additional styles as plugins
                                    these will be available to the -style command line
                                    option. You can also set the style for all Qt
                                    applications by setting the QT_STYLE_OVERRIDE environment
                                    variable.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="479"/>
      <source>       --style style                is the same as listed above.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="480"/>
      <source>       --stylesheet=stylesheet      sets the application styleSheet.
                                    The value must be a path to a file that contains the
                                    Style Sheet. Note: Relative URLs in the Style Sheet file
                                    are relative to the Style Sheet file&apos;s path.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="486"/>
      <source>       --stylesheet stylesheet      is the same as listed above.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="489"/>
      <source>       --sync                       forces the X server to perform each X client request
                                    immediately and not use buffer optimization. It makes the
                                    program easier to debug and often much slower. The --sync
                                    option is only valid for the X11 version of Qt.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="495"/>
      <source>       --widgetcount                prints debug message at the end about number of widgets
                                    left undestroyed and maximum number of widgets existing
                                    at the same time.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="499"/>
      <source>       --qmljsdebugger=1234[,block] activates the QML/JS debugger with a
                                    specified port. The number is the port value and block is
                                    optional and will make the application wait until a
                                    debugger connects to it.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="504"/>
      <source>Arguments:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="505"/>
      <source>        [FILE]                       File to install as a package</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="506"/>
      <source>Report bugs to: https://github.com/Mudlet/Mudlet/issues</source>
      <translation>报告bugs：https://github.com/Mudelet/Mudelet/issues</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="433"/>
      <source>Usage: %1 [OPTION...] [FILE] </source>
      <comment>%1 is the name of the executable as it is on this OS.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="438"/>
      <source>Options:</source>
      <translation>選項:</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="441"/>
      <source>       -s, --splashscreen           show splashscreen on startup.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="507"/>
      <source>Project home page: http://www.mudlet.org/</source>
      <translation>项目主页：http://www.mudelet.org/</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="515"/>
      <source>%1 %2%3 (with debug symbols, without optimisations)</source>
      <comment>%1 is the name of the application like mudlet or Mudlet.exe, %2 is the version number like 3.20 and %3 is a build suffix like -dev</comment>
      <translation>%1 %2%3 (带有调试符号，没有优化)</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="522"/>
      <source>Qt libraries %1 (compilation) %2 (runtime)</source>
      <comment>%1 and %2 are version numbers</comment>
      <translation>Qt库 %1 (编译) %2 (运行时)</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="524"/>
      <source>Copyright © 2008-2026  Mudlet developers</source>
      <translation>版权所有 © 2008-2026  Mudlet developers</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="525"/>
      <source>Licence GPLv2+: GNU GPL version 2 or later - http://gnu.org/licenses/gpl.html</source>
      <translation>许可证 GPLv2+: GNU GPL 版本 2 或更高版本 - http://gnu.org/licenses/gpl.html</translation>
    </message>
    <message>
      <location filename="../src/main.cpp" line="526"/>
      <source>This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/main.cpp" line="632"/>
      <source>Version: %1</source>
      <translation>版本: %1</translation>
    </message>
  </context>
  <context>
    <name>main_window</name>
    <message>
      <location filename="../src/ui/main_window.ui" line="95"/>
      <source>Toolbox</source>
      <translation>工具</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="112"/>
      <source>Options</source>
      <translation>選項</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="123"/>
      <source>Help</source>
      <translation>說明</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="135"/>
      <source>About</source>
      <translation>关于</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="144"/>
      <source>Window</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="155"/>
      <source>Games</source>
      <translation>遊戲</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="174"/>
      <source>Reattach detached windows</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="177"/>
      <source>&lt;p&gt;Reattach all detached profile windows back to the main Mudlet window.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="188"/>
      <source>Always on Top</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="191"/>
      <source>&lt;p&gt;Keep the main Mudlet window always on top of other windows.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="196"/>
      <source>Minimize</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="199"/>
      <source>&lt;p&gt;Minimize the main Mudlet window.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="204"/>
      <source>Play</source>
      <translation>開啟遊戲</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="207"/>
      <source>&lt;p&gt;Configure connection details of, and make a connection to, game servers.&lt;/p&gt;</source>
      <translation>&lt;p&gt;配置游戏服务器的连接细节并接入。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="215"/>
      <source>&lt;p&gt;Disconnect from the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;从当前游戏服务器断开。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="223"/>
      <source>&lt;p&gt;Disconnect and then reconnect to the current game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;先断开然后再连接当前游戏服务器。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="231"/>
      <source>&lt;p&gt;Configure setting for the Mudlet application globally and for the current profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;设定Mudlet应用的全局环境和当前的配置。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="242"/>
      <source>&lt;p&gt;Opens the Editor for the different types of things that can be scripted by the user.&lt;/p&gt;</source>
      <translation>&lt;p&gt;開啟腳本編輯器&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="247"/>
      <source>Show errors</source>
      <translation>顯示錯誤</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="250"/>
      <source>&lt;p&gt;Show errors from scripts that you have running&lt;/p&gt;</source>
      <translation>&lt;p&gt;显示你正在运行的脚本的错误&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="274"/>
      <source>&lt;p&gt;About Mudlet version, creators, and license.&lt;/p&gt;</source>
      <comment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in multiple places - please ensure all have the same translation).</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="282"/>
      <source>Take a UI tour</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="285"/>
      <source>&lt;p&gt;Shows a short interactive tour of the most important parts of Mudlet&apos;s interface.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="293"/>
      <source>&lt;p&gt;Opens an (on-line) collection of &quot;Educational Mudlet screencasts&quot; in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在你的系统网页浏览器中打开（在线）&quot;Mudlet的教学视频&quot;集。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="301"/>
      <source>&lt;p&gt;Load a previous saved game session that can be used to test Mudlet lua systems (off-line!).&lt;/p&gt;</source>
      <translation>&lt;p&gt;加载之前保存的游戏会话，该会话可用于测试 Mudlet lua 系统(离线!)。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="309"/>
      <source>&lt;p&gt;Opens the (on-line) Mudlet Forum in your system web-browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在網頁瀏覽器中開啟 Mudlet 線上論壇&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="322"/>
      <source>&lt;p&gt;Opens a connect to an IRC server (LiberaChat) in your system web-browser.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="330"/>
      <source>&lt;p&gt;Show or hide the game map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;显示或隐藏游戏地图。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="338"/>
      <source>&lt;p&gt;Install and remove collections of Mudlet lua items (packages).&lt;/p&gt;</source>
      <translation>&lt;p&gt;安装和删除Mudlet Lua项的合集（包）。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="346"/>
      <source>&lt;p&gt;Install and remove (share- &amp; sync-able) collections of Mudlet lua items (modules).&lt;/p&gt;</source>
      <translation>&lt;p&gt;安装和移除（可共享和同步的）Mudlet Lua项的合集（模块）。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="351"/>
      <source>Package exporter</source>
      <translation>包导出器</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="354"/>
      <source>&lt;p&gt;Gather and bundle up collections of Mudlet Lua items and other reasources into a module.&lt;/p&gt;</source>
      <translation>&lt;p&gt;收集并打包Mudlet Lua项的合集以及其它资源到模块中去。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="365"/>
      <source>Multiview</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="488"/>
      <source>Timestamps</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="496"/>
      <source>Record replay</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="504"/>
      <source>Record log</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="512"/>
      <source>Emergency stop</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="515"/>
      <source>&lt;p&gt;Toggle all triggers, aliases, timers, etc. on or off&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="520"/>
      <source>New map window</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="523"/>
      <source>&lt;p&gt;Open an additional map view window for the current profile.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="390"/>
      <source>Mute all media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="393"/>
      <source>&lt;p&gt;Mutes all media played.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="404"/>
      <source>Mute sounds from Mudlet (triggers, scripts, etc.)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="407"/>
      <source>&lt;p&gt;Mutes media played by the Lua API and scripts.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="435"/>
      <source>&lt;p&gt;Hide / show the search area and buttons at the bottom of the screen.&lt;/p&gt;</source>
      <translation>&lt;p&gt;隐藏/显示屏幕底部的搜索区域和按钮。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="440"/>
      <source>Discord</source>
      <translation>Discord</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="443"/>
      <source>&lt;p&gt;Open a link to Discord.&lt;/p&gt;</source>
      <translation>&lt;p&gt;打开至 Discord 的链接.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="451"/>
      <source>Discord help channel</source>
      <translation>Discord 頻道</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="454"/>
      <source>&lt;p&gt;Open a link to the Mudlet server on Discord.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在 Discord 上打开到 Mudlet 服务器的链接。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="459"/>
      <location filename="../src/ui/main_window.ui" line="462"/>
      <source>Report an issue</source>
      <translation>报告此问题</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="465"/>
      <source>The public test build gets newer features to you quicker, and you help us find issues in them quicker. Spotted something odd? Let us know asap!</source>
      <translation>公测版将更新的功能更快地送到你手中，你也能帮助我们更快地发现其中的问题。发现了什么奇怪的东西？请尽快告诉我们</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="470"/>
      <source>Close profile</source>
      <translation>关闭配置文件</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="475"/>
      <source>Close Mudlet</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="483"/>
      <source>Show changelog</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="491"/>
      <source>&lt;p&gt;Toggle time stamps on the main console.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="499"/>
      <source>&lt;p&gt;Toggle recording of replays.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="507"/>
      <source>&lt;p&gt;Toggle logging facilities.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="212"/>
      <source>Disconnect</source>
      <translation>中斷連線</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="220"/>
      <source>Reconnect</source>
      <translation>重新连接</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="228"/>
      <source>Preferences</source>
      <translation>偏好設定</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="239"/>
      <source>Script editor</source>
      <translation>腳本編輯器</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="255"/>
      <source>Notepad</source>
      <translation>记事本</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="263"/>
      <source>API Reference</source>
      <translation>使用手冊</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="306"/>
      <source>Online forum</source>
      <translation>線上論壇</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="271"/>
      <source>About Mudlet</source>
      <translation>关于 Mudlet</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="258"/>
      <source>&lt;p&gt;Opens a free form text editor window for this profile that is saved between sessions.&lt;/p&gt;</source>
      <translation>&lt;p&gt;打開當前配置檔的文本編輯器視窗，文件內容會在不同會話之間保存。 &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="266"/>
      <source>&lt;p&gt;Opens the Mudlet manual in your web browser.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在浏览器中打开Mudlet手册。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="290"/>
      <source>Video tutorials</source>
      <translation>影片教學</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="298"/>
      <source>Load replay</source>
      <translation>載入回放記錄</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="314"/>
      <source>Check for updates...</source>
      <translation>檢查更新</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="319"/>
      <source>Live help chat</source>
      <translation>即時協助</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="327"/>
      <source>Show map</source>
      <translation>顯示地圖</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="335"/>
      <source>Package manager</source>
      <translation>套件管理工具</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="343"/>
      <source>Module manager</source>
      <translation>模組管理工具</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="368"/>
      <source>&lt;p&gt;Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.&lt;/p&gt;</source>
      <comment>Same text is used in 2 places.</comment>
      <translation>&lt;p&gt;分割 Mudlet 屏幕以一次显示多个配置文件; 若加载的配置文件小于2个时则禁用此功能。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="379"/>
      <source>Fullscreen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="418"/>
      <source>Mute sounds from the game (MCMP, MSP)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="421"/>
      <source>&lt;p&gt;Mutes media played by the game (MCMP, MSP).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/main_window.ui" line="432"/>
      <source>Compact input line</source>
      <translation>简洁的输入行</translation>
    </message>
  </context>
  <context>
    <name>map_label</name>
    <message>
      <location filename="../src/ui/map_label.ui" line="20"/>
      <source>Map label</source>
      <translation>地图标签</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="38"/>
      <source>Type:</source>
      <translation>类型:</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="49"/>
      <source>Text</source>
      <translation>文字</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="54"/>
      <source>Image</source>
      <translation>图片</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="62"/>
      <source>Image:</source>
      <translation>图片：</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="79"/>
      <location filename="../src/ui/map_label.ui" line="133"/>
      <source>...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="86"/>
      <source>Stretch image</source>
      <translation>拉伸图片</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="93"/>
      <source>Label text:</source>
      <translation>标签文本：</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="109"/>
      <source>My Label</source>
      <translation>我的标签</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="116"/>
      <source>Font:</source>
      <translation>字型</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="140"/>
      <source>Foreground:</source>
      <translation>前景：</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="157"/>
      <source>Background:</source>
      <translation>背景：</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="174"/>
      <source>Text outline:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="202"/>
      <source>Background</source>
      <translation>背景</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="207"/>
      <source>Foreground</source>
      <translation>前景</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="215"/>
      <source>&lt;p&gt;If deselected the label will have the same size when you zoom in and out in the mapper. If it is selected the label will scale when you zoom the mapper.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="191"/>
      <source>Position:</source>
      <translation>位置:</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="218"/>
      <source>Scale with zoom</source>
      <translation>缩放比例</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="266"/>
      <source>Save</source>
      <translation>保存</translation>
    </message>
    <message>
      <location filename="../src/ui/map_label.ui" line="273"/>
      <source>Cancel</source>
      <translation>取消</translation>
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
      <location filename="../src/ui/mapper.ui" line="247"/>
      <source>Area:</source>
      <translation>区域:</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="285"/>
      <source>Map autosave failed - click for options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="447"/>
      <source>top + 1</source>
      <translation>top+1</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="597"/>
      <source>Player Icon Adjustments:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="607"/>
      <source>Height</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="614"/>
      <source>Rot X</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="621"/>
      <source>Rot Y</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="628"/>
      <source>Rot Z</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="635"/>
      <source>Scale</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="663"/>
      <source>Adjust player icon height (-2.0 to +5.0 units)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="691"/>
      <source>Rotate player icon around X axis (-180° to +180°)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="719"/>
      <source>Rotate player icon around Y axis (-180° to +180°)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="747"/>
      <source>Rotate player icon around Z axis (-180° to +180°)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="775"/>
      <source>Adjust player icon scale (0.001 to 0.02)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="782"/>
      <source>Reset Player Icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="785"/>
      <source>Reset player icon adjustments to default values</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="421"/>
      <source>bottom + 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="301"/>
      <source>≡</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="304"/>
      <source>Mapper display options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="434"/>
      <source>bottom -1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="460"/>
      <source>top - 1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="408"/>
      <source>1 level</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="356"/>
      <source>default</source>
      <translation>默认</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="369"/>
      <source>top view</source>
      <translation>顶部显示</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="382"/>
      <source>side view</source>
      <translation>侧视图</translation>
    </message>
    <message>
      <location filename="../src/ui/mapper.ui" line="395"/>
      <source>all levels</source>
      <translation>所有级别</translation>
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
      <location filename="../src/ui/module_manager.ui" line="147"/>
      <source>Uninstall</source>
      <translation>卸载</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="160"/>
      <source>Install</source>
      <translation>安装</translation>
    </message>
    <message>
      <location filename="../src/ui/module_manager.ui" line="167"/>
      <source>Module Help</source>
      <translation>模組說明</translation>
    </message>
  </context>
  <context>
    <name>mudlet</name>
    <message>
      <location filename="../src/mudlet.cpp" line="999"/>
      <source>Afrikaans</source>
      <extracomment>In the translation source texts the language is the leading term, with, generally, the (primary) country(ies) in the brackets, with a trailing language disabiguation after a &apos;-&apos; Chinese is an exception!</extracomment>
      <translation>南非荷兰文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1000"/>
      <source>Afrikaans (South Africa)</source>
      <translation>南非荷兰语(南非)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1001"/>
      <source>Aragonese</source>
      <translation>阿拉贡语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1002"/>
      <source>Aragonese (Spain)</source>
      <translation>阿拉贡(西班牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1003"/>
      <source>Arabic</source>
      <translation>阿拉伯语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1004"/>
      <source>Arabic (United Arab Emirates)</source>
      <translation>阿拉伯语(阿拉伯联合酋长国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1005"/>
      <source>Arabic (Bahrain)</source>
      <translation>阿拉伯语(巴林)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1006"/>
      <source>Arabic (Algeria)</source>
      <translation>阿拉伯语(阿尔及利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1008"/>
      <source>Arabic (India)</source>
      <translation>阿拉伯语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1009"/>
      <source>Arabic (Iraq)</source>
      <translation>阿拉伯语(伊拉克)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1010"/>
      <source>Arabic (Jordan)</source>
      <translation>阿拉伯语(约旦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1011"/>
      <source>Arabic (Kuwait)</source>
      <translation>阿拉伯语(科威特)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1012"/>
      <source>Arabic (Lebanon)</source>
      <translation>阿拉伯语(黎巴嫩)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1013"/>
      <source>Arabic (Libya)</source>
      <translation>阿拉伯语(利比亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1014"/>
      <source>Arabic (Morocco)</source>
      <translation>阿拉伯语(摩洛哥)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1015"/>
      <source>Arabic (Oman)</source>
      <translation>阿拉伯语(也门)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1016"/>
      <source>Arabic (Qatar)</source>
      <translation>阿拉伯语(卡塔尔)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1017"/>
      <source>Arabic (Saudi Arabia)</source>
      <translation>阿拉伯语(沙特阿拉伯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1018"/>
      <source>Arabic (Sudan)</source>
      <translation>阿拉伯语(约旦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1019"/>
      <source>Arabic (Syria)</source>
      <translation>阿拉伯语(叙利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1020"/>
      <source>Arabic (Tunisia)</source>
      <translation>阿拉伯语(突尼斯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1021"/>
      <source>Arabic (Yemen)</source>
      <translation>阿拉伯语(也门)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1022"/>
      <source>Belarusian</source>
      <translation>白俄罗斯语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1023"/>
      <source>Belarusian (Belarus)</source>
      <translation>白俄罗斯语(白俄罗斯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1024"/>
      <source>Belarusian (Russia)</source>
      <translation>白俄罗斯语(俄罗斯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1025"/>
      <source>Bulgarian</source>
      <translation>保加利亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1026"/>
      <source>Bulgarian (Bulgaria)</source>
      <translation>保加利亚语(保加利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1027"/>
      <source>Bangla</source>
      <translation>孟加拉语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1028"/>
      <source>Bangla (Bangladesh)</source>
      <translation>孟加拉语(孟加拉国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1029"/>
      <source>Bangla (India)</source>
      <translation>孟加拉语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1030"/>
      <source>Tibetan</source>
      <translation>藏语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1032"/>
      <source>Tibetan (China)</source>
      <translation>藏语(中国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1033"/>
      <source>Tibetan (India)</source>
      <translation>藏语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1034"/>
      <source>Breton</source>
      <translation>布列塔尼语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1035"/>
      <source>Breton (France)</source>
      <translation>布列塔尼语(法国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1036"/>
      <source>Bosnian</source>
      <translation>波斯尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1037"/>
      <source>Bosnian (Bosnia/Herzegovina)</source>
      <translation>波斯尼亚 (波斯尼亚/黑塞哥维那)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1038"/>
      <source>Bosnian (Bosnia/Herzegovina - Cyrillic alphabet)</source>
      <translation>波斯尼亚语(波斯尼亚/黑塞哥维那-西里尔字母)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1039"/>
      <source>Catalan</source>
      <translation>加泰罗尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1040"/>
      <source>Catalan (Spain)</source>
      <translation>加泰罗尼亚语(西班牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1041"/>
      <source>Catalan (Spain - Valencian)</source>
      <translation>加泰罗尼亚语(西班牙-巴伦西亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1042"/>
      <source>Central Kurdish</source>
      <translation>库尔德中部</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1043"/>
      <source>Central Kurdish (Iraq)</source>
      <translation>库尔德中部 (伊拉克)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1044"/>
      <source>Czech</source>
      <translation>捷克语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1045"/>
      <source>Czech (Czechia)</source>
      <translation>捷克(捷克共和国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1048"/>
      <source>Danish</source>
      <translation>丹麦语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1049"/>
      <source>Danish (Denmark)</source>
      <translation>丹麦语(丹麦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1050"/>
      <source>German</source>
      <translation>德文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1051"/>
      <source>German (Austria)</source>
      <translation>德语(奥地利)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1052"/>
      <source>German (Austria, revised by F M Baumann)</source>
      <translation>德语(奥地利，经F M Baumann修订)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1053"/>
      <source>German (Belgium)</source>
      <translation>德语 (比利时)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1054"/>
      <source>German (Switzerland)</source>
      <translation>德语(瑞士)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1055"/>
      <source>German (Switzerland, revised by F M Baumann)</source>
      <translation>德语(奥地利，经 F M Baumann修订)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1056"/>
      <source>German (Germany/Belgium/Luxemburg)</source>
      <translation>德语 (德语/比利时/卢森堡)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1057"/>
      <source>German (Germany/Belgium/Luxemburg, revised by F M Baumann)</source>
      <translation>德文（德国/比利时/卢森堡，经 Fm Baumann 修订）</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1058"/>
      <source>German (Liechtenstein)</source>
      <translation>德语(列支敦士登)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1059"/>
      <source>German (Luxembourg)</source>
      <translation>德语(卢森堡)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1062"/>
      <source>Greek</source>
      <translation>希腊文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1063"/>
      <source>Greek (Greece)</source>
      <translation>希腊语(希腊)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1064"/>
      <source>English</source>
      <translation>英语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1065"/>
      <source>English (Antigua/Barbuda)</source>
      <translation>英语 (安提瓜/巴布达)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1066"/>
      <source>English (Australia)</source>
      <translation>英语(澳大利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1068"/>
      <source>English (Bahamas)</source>
      <translation>英语(牙买加)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1069"/>
      <source>English (Botswana)</source>
      <translation>英语 (博茨瓦纳)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1070"/>
      <source>English (Belize)</source>
      <translation>英语(伯利兹)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1007"/>
      <source>Arabic (Egypt)</source>
      <translation>阿拉伯语(埃及)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="316"/>
      <location filename="../src/mudlet.cpp" line="318"/>
      <location filename="../src/mudlet.cpp" line="731"/>
      <source>Close profile</source>
      <translation>关闭配置文件</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="321"/>
      <location filename="../src/mudlet.cpp" line="323"/>
      <source>Close Mudlet</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="377"/>
      <source>Mute</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="384"/>
      <location filename="../src/mudlet.cpp" line="386"/>
      <location filename="../src/mudlet.cpp" line="727"/>
      <location filename="../src/mudlet.cpp" line="5296"/>
      <location filename="../src/mudlet.cpp" line="5299"/>
      <source>Mute all media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="390"/>
      <location filename="../src/mudlet.cpp" line="392"/>
      <location filename="../src/mudlet.cpp" line="5331"/>
      <source>Mute sounds from Mudlet (triggers, scripts, etc.)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="419"/>
      <source>Mudlet chat</source>
      <translation>Mudlet 闲聊</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="420"/>
      <source>Open a link to the Mudlet server on Discord</source>
      <translation>在 Discord 上打开至 Mudlet 服务器的链接。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="458"/>
      <source>Show Main Toolbar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="514"/>
      <source>Report issue</source>
      <translation>报告问题</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="519"/>
      <source>Report bugs in the public test build to help us improve Mudlet.</source>
      <extracomment>Tooltip for Report Issue button in public test builds</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="528"/>
      <location filename="../src/mudlet.cpp" line="6114"/>
      <source>About Mudlet version, creators, and license.</source>
      <extracomment>Tooltip for About Mudlet sub-menu item and main toolbar button (or menu item if an update has changed that control to have a popup menu instead) (Used in multiple places - please ensure all have the same translation).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="538"/>
      <source>Full Screen</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="719"/>
      <source>Script editor</source>
      <translation>脚本编辑器</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="720"/>
      <source>Show Map</source>
      <translation>显示地图</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="721"/>
      <source>Compact input line</source>
      <translation>简洁的输入行</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="722"/>
      <source>Preferences</source>
      <translation>首选项</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="724"/>
      <source>Package manager</source>
      <translation>包管理器</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="725"/>
      <source>Module manager</source>
      <translation>模块管理器</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="728"/>
      <source>Play</source>
      <translation>开始</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="732"/>
      <source>Toggle Time Stamps</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="733"/>
      <source>Toggle Replay</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="734"/>
      <source>Toggle Logging</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="735"/>
      <source>Toggle Emergency Stop</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="736"/>
      <source>Next profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="737"/>
      <source>Previous profile</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="740"/>
      <source>Switch to profile %1</source>
      <extracomment>Name of the keyboard shortcut that switches to the numbered profile tab, %1 is that number (1 to 9)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1031"/>
      <source>Tibetan (Bhutan)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1046"/>
      <source>Welsh</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1047"/>
      <source>Welsh (United Kingdom {Wales})</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1060"/>
      <source>Dzongkha</source>
      <translation>宗喀语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1061"/>
      <source>Dzongkha (Bhutan)</source>
      <translation>宗喀语(不丹)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1067"/>
      <source>English (Australia, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>英语(澳大利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1071"/>
      <source>English (Canada)</source>
      <translation>英语(加拿大)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1072"/>
      <source>English (Canada, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>英语(澳大利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1073"/>
      <source>English (Denmark)</source>
      <translation>英语(丹麦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1074"/>
      <source>English (United Kingdom)</source>
      <translation>英语(英国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1075"/>
      <source>English (United Kingdom, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>英语(英国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1076"/>
      <source>English (United Kingdom - &apos;ise&apos; not &apos;ize&apos;)</source>
      <comment>This dictionary prefers the British &apos;ise&apos; form over the American &apos;ize&apos; one.</comment>
      <translation>英语（英国）——使用&apos;ise&apos;而非&apos;ize&apos;。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1077"/>
      <source>English (Ghana)</source>
      <translation>英语(加纳)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1078"/>
      <source>English (Hong Kong SAR China)</source>
      <translation>英语 (中国香港特别行政区)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1079"/>
      <source>English (Ireland)</source>
      <translation>英语(爱尔兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1080"/>
      <source>English (India)</source>
      <translation>英语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1081"/>
      <source>English (Jamaica)</source>
      <translation>英语(牙买加)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1082"/>
      <source>English (Namibia)</source>
      <translation>英语(纳米比亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1083"/>
      <source>English (Nigeria)</source>
      <translation>英语(尼日利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1084"/>
      <source>English (New Zealand)</source>
      <translation>英语(新西兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1085"/>
      <source>English (Philippines)</source>
      <translation>英语(菲律宾)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1086"/>
      <source>English (Singapore)</source>
      <translation>英语(新加坡)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1087"/>
      <source>English (Trinidad/Tobago)</source>
      <translation>英语 (特立尼达/多巴哥)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1088"/>
      <source>English (United States)</source>
      <translation>英语(美国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1089"/>
      <source>English (United States, Large)</source>
      <comment>This dictionary contains larger vocabulary.</comment>
      <translation>英语(美国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1090"/>
      <source>English (South Africa)</source>
      <translation>英语(南非)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1091"/>
      <source>English (Zimbabwe)</source>
      <translation>英语(津巴布韦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1092"/>
      <source>Esperanto</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1093"/>
      <source>Spanish</source>
      <translation>西班牙文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1094"/>
      <source>Spanish (Argentina)</source>
      <translation>西班牙语(阿根廷)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1095"/>
      <source>Spanish (Bolivia)</source>
      <translation>西班牙语(玻利维亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1096"/>
      <source>Spanish (Chile)</source>
      <translation>西班牙语(智利)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1097"/>
      <source>Spanish (Colombia)</source>
      <translation>西班牙语(哥伦比亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1098"/>
      <source>Spanish (Costa Rica)</source>
      <translation>西班牙语(哥斯达黎加)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1099"/>
      <source>Spanish (Cuba)</source>
      <translation>西班牙语(智利)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1100"/>
      <source>Spanish (Dominican Republic)</source>
      <translation>西班牙语(多米尼加共和国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1101"/>
      <source>Spanish (Ecuador)</source>
      <translation>西班牙语(厄瓜多尔)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1102"/>
      <source>Spanish (Spain)</source>
      <translation>西班牙语 (西班牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1103"/>
      <source>Spanish (Guatemala)</source>
      <translation>西班牙语(危地马拉)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1104"/>
      <source>Spanish (Honduras)</source>
      <translation>西班牙语(洪都拉斯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1105"/>
      <source>Spanish (Mexico)</source>
      <translation>西班牙语(墨西哥)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1106"/>
      <source>Spanish (Nicaragua)</source>
      <translation>西班牙语(尼加拉瓜)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1107"/>
      <source>Spanish (Panama)</source>
      <translation>西班牙语(巴拿马)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1108"/>
      <source>Spanish (Peru)</source>
      <translation>西班牙(秘鲁)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1109"/>
      <source>Spanish (Puerto Rico)</source>
      <translation>西班牙语(波多黎各)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1110"/>
      <source>Spanish (Paraguay)</source>
      <translation>西班牙语(巴拉圭)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1111"/>
      <source>Spanish (El Savador)</source>
      <translation>西班牙语(圣萨尔瓦多)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1112"/>
      <source>Spanish (United States)</source>
      <translation>西班牙语(美国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1113"/>
      <source>Spanish (Uruguay)</source>
      <translation>西班牙语(乌拉圭)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1114"/>
      <source>Spanish (Venezuela)</source>
      <translation>西班牙语(委内瑞拉)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1115"/>
      <source>Estonian</source>
      <translation>爱沙尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1116"/>
      <source>Estonian (Estonia)</source>
      <translation>爱沙尼亚语(爱沙尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1117"/>
      <source>Basque</source>
      <translation>巴斯克语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1118"/>
      <source>Basque (Spain)</source>
      <translation>巴斯克语(西班牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1119"/>
      <source>Basque (France)</source>
      <translation>巴斯克(法国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1120"/>
      <location filename="../src/mudlet.cpp" line="1121"/>
      <source>Finnish</source>
      <translation>芬兰语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1122"/>
      <source>Faroese</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1123"/>
      <source>Faroese (Faroe Islands)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1124"/>
      <location filename="../src/mudlet.cpp" line="1128"/>
      <source>French</source>
      <translation>French</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1129"/>
      <source>French (Belgium)</source>
      <translation>法语(比利时)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1130"/>
      <source>French (Catalan)</source>
      <translation>法语(加泰罗尼亚语)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1131"/>
      <source>French (Switzerland)</source>
      <translation>法语(瑞士)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1132"/>
      <source>French (France)</source>
      <translation>法语(法国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1133"/>
      <source>French (Luxemburg)</source>
      <translation>法语(卢森堡)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1134"/>
      <source>French (Monaco)</source>
      <translation>法语(摩纳哥)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1135"/>
      <source>Irish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1136"/>
      <source>Gaelic</source>
      <translation>盖尔语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1137"/>
      <source>Gaelic (United Kingdom {Scots})</source>
      <translation>盖尔语(United Kingdom {Scots})</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1138"/>
      <source>Galician</source>
      <translation>加利西亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1139"/>
      <source>Galician (Spain)</source>
      <translation>加利西亚(西班牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1140"/>
      <location filename="../src/mudlet.cpp" line="1145"/>
      <source>Guarani</source>
      <translation>瓜拉尼语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1141"/>
      <location filename="../src/mudlet.cpp" line="1146"/>
      <source>Guarani (Paraguay)</source>
      <translation>瓜拉尼语(巴拉圭)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1142"/>
      <source>Gujarati</source>
      <translation>古吉拉特语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1143"/>
      <source>Gujarati (India)</source>
      <translation>古吉拉特语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1147"/>
      <source>Hebrew</source>
      <translation>希伯来语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1148"/>
      <source>Hebrew (Israel)</source>
      <translation>希伯来语(以色列)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1149"/>
      <source>Hindi</source>
      <translation>北印度语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1150"/>
      <source>Hindi (India)</source>
      <translation>印地语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1151"/>
      <source>Croatian</source>
      <translation>克罗地亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1152"/>
      <source>Croatian (Croatia)</source>
      <translation>克罗地亚语(克罗地亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1153"/>
      <source>Hungarian</source>
      <translation>匈牙利文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1154"/>
      <source>Hungarian (Hungary)</source>
      <translation>匈牙利语(匈牙利)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1155"/>
      <source>Armenian</source>
      <translation>亚美尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1156"/>
      <source>Armenian (Armenia)</source>
      <translation>亚美尼亚语(亚美尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1157"/>
      <source>Indonesian</source>
      <translation>印度尼西亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1158"/>
      <source>Indonesian (Indonesia)</source>
      <translation>印度尼西亚语 (印度尼西亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1186"/>
      <source>Mongolian</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1187"/>
      <source>Mongolian (Mongolia)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1252"/>
      <source>Tagalog</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1355"/>
      <location filename="../src/mudlet.cpp" line="1357"/>
      <source>Medievia {Custom codec for that MUD}</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1381"/>
      <source>hh:mm:ss.zzz </source>
      <extracomment>This represents the format of the timestamps shown alongside the texts in a console and might require translation for a few locales; the content is as per QDateTime::toString(...) and needs to follow the rules for that function as well as being suitable for the translation locale.</extracomment>
      <translation>hh:mm:ss.zzz </translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1385"/>
      <source>------------ </source>
      <extracomment>This represents the format of the timestamps shown for lines that do not have a timestamp in a console that is showing them. If localised this should be set to the same format and length as the smTimeStampFormat:</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1881"/>
      <source>%1 (Main Window)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1908"/>
      <source>%1 (Detached)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2426"/>
      <source>Switch games with the keyboard</source>
      <extracomment>Title of a balloon pointing out the newly added profile tab switching shortcuts</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2428"/>
      <source>Press %1 to cycle through your open games, or %2 to %3 to jump straight to one. You can change these keys in the preferences.</source>
      <extracomment>%1, %2 and %3 are keyboard shortcuts, e.g. Ctrl+Tab, Ctrl+1 and Ctrl+9 (Control-Tab, Command-1 and Command-9 on macOS)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4369"/>
      <source>Map - %1</source>
      <translation>地圖 - %1</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5160"/>
      <source>[ CHAT ]  - Auto-starting MMCP Server on port %1.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5296"/>
      <location filename="../src/mudlet.cpp" line="5299"/>
      <source>Unmute all media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5316"/>
      <source>[ INFO ]  - Mudlet and game sounds are muted. Use &quot;%1&quot; to unmute.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5317"/>
      <source>[ INFO ]  - Mudlet and game sounds are unmuted. Use &quot;%1&quot; to mute.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5385"/>
      <source>[ INFO ]  - Compact input line set. Press &quot;%1&quot; to show bottom-right buttons again.</source>
      <extracomment>Here %1 will be replaced with the keyboard shortcut, default is ALT+L.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5505"/>
      <source>Detach Tab &quot;%1&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5527"/>
      <source>Show Connection Indicators on Tabs</source>
      <translation type="unfinished"/>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="6126"/>
      <source>&lt;p&gt;About Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n update(s) is/are now available!&lt;/i&gt;&lt;p&gt;</source>
      <extracomment>This is the tooltip text for the &apos;About&apos; Mudlet main toolbar button when it has been changed by adding a menu which now contains the original &apos;About Mudlet&apos; action and a new one to access the manual update process</extracomment>
      <translation>
        <numerusform>&lt;p&gt;关于Mudlet&lt;/p&gt;&lt;p&gt;&lt;i&gt;%n 已经可以进行更新！&lt;/i&gt;&lt;p&gt;</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="6144"/>
      <source>Review %n update(s)...</source>
      <extracomment>Review update(s) menu item, %n is the count of how many updates are available</extracomment>
      <translation>
        <numerusform>检查 %n 更新...</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../src/mudlet.cpp" line="6146"/>
      <source>Review the update(s) available...</source>
      <extracomment>Tool-tip for review update(s) menu item, given that the count of how many updates are available is already shown in the menu, the %n parameter that is that number need not be used here</extracomment>
      <translation>
        <numerusform>查看可用的更新...</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1161"/>
      <source>Icelandic</source>
      <translation>冰岛语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="396"/>
      <location filename="../src/mudlet.cpp" line="398"/>
      <location filename="../src/mudlet.cpp" line="5336"/>
      <source>Mute sounds from the game (MCMP, MSP)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1162"/>
      <source>Icelandic (Iceland)</source>
      <translation>冰岛语(冰岛)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1163"/>
      <source>Italian</source>
      <translation>意大利文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1164"/>
      <source>Italian (Switzerland)</source>
      <translation>意大利语(瑞士)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1165"/>
      <source>Italian (Italy)</source>
      <translation>意大利语(意大利)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1166"/>
      <source>Kazakh</source>
      <translation>哈萨克语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1167"/>
      <source>Kazakh (Kazakhstan)</source>
      <translation>哈萨克语(哈萨克斯坦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1168"/>
      <source>Kurmanji</source>
      <translation>库尔曼吉</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1169"/>
      <source>Kurmanji {Latin-alphabet Kurdish}</source>
      <translation>库尔曼吉 {Latin-alphabet Kurdish}</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1170"/>
      <source>Korean</source>
      <translation>韩语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1171"/>
      <source>Korean (South Korea)</source>
      <translation>韩语(韩国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1172"/>
      <source>Kurdish</source>
      <translation>库尔德语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1173"/>
      <source>Kurdish (Syria)</source>
      <translation>库尔德人(叙利亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1174"/>
      <source>Kurdish (Turkey)</source>
      <translation>土耳其语(土耳其)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1175"/>
      <source>Latin</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1176"/>
      <source>Luxembourgish</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1177"/>
      <source>Luxembourgish (Luxembourg)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1178"/>
      <source>Lao</source>
      <translation>老挝语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1179"/>
      <source>Lao (Laos)</source>
      <translation>老挝(老挝)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1180"/>
      <source>Lithuanian</source>
      <translation>立陶宛语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1181"/>
      <source>Lithuanian (Lithuania)</source>
      <translation>立陶宛语(立陶宛)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1182"/>
      <source>Latvian</source>
      <translation>拉脱维亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1183"/>
      <source>Latvian (Latvia)</source>
      <translation>拉脱维亚语 (拉脱维亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1184"/>
      <source>Malayalam</source>
      <translation>马拉雅拉姆语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1185"/>
      <source>Malayalam (India)</source>
      <translation>马拉雅拉姆语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1188"/>
      <source>Norwegian Bokmål</source>
      <translation>挪威语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1189"/>
      <source>Norwegian Bokmål (Norway)</source>
      <translation>书面挪威语(挪威)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1190"/>
      <source>Nepali</source>
      <translation>尼泊尔语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1191"/>
      <source>Nepali (Nepal)</source>
      <translation>尼泊尔语(尼泊尔)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1192"/>
      <source>Dutch</source>
      <translation>荷兰文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1193"/>
      <source>Dutch (Netherlands Antilles)</source>
      <translation>荷兰语(荷属安的列斯群岛)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1194"/>
      <source>Dutch (Aruba)</source>
      <translation>荷兰语(阿鲁巴)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1195"/>
      <source>Dutch (Belgium)</source>
      <translation>荷兰语(比利时)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1196"/>
      <source>Dutch (Netherlands)</source>
      <translation>荷兰语(荷兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1197"/>
      <source>Dutch (Suriname)</source>
      <translation>荷兰语 (苏里南)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1198"/>
      <source>Norwegian Nynorsk</source>
      <translation>挪威尼诺斯克语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1199"/>
      <source>Norwegian Nynorsk (Norway)</source>
      <translation>新挪威语(挪威)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1200"/>
      <source>Occitan</source>
      <translation>奥克西坦语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1201"/>
      <source>Occitan (France)</source>
      <translation>奥克西坦语(法国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1202"/>
      <source>Polish</source>
      <translation>波兰文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1203"/>
      <source>Polish (Poland)</source>
      <translation>波兰语(波兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1204"/>
      <source>Portuguese</source>
      <translation>葡萄牙文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1205"/>
      <source>Portuguese (Brazil)</source>
      <translation>Português (Brazil)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1206"/>
      <source>Portuguese (Portugal)</source>
      <translation>葡萄牙语(葡萄牙)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1207"/>
      <source>Romanian</source>
      <translation>罗马尼亚文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1208"/>
      <source>Romanian (Romania)</source>
      <translation>罗马尼亚语(罗马尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1209"/>
      <source>Russian</source>
      <translation>俄文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1210"/>
      <source>Russian (Russia)</source>
      <translation>俄语(俄罗斯)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1211"/>
      <source>Northern Sami</source>
      <translation>北萨摩斯语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1212"/>
      <source>Northern Sami (Finland)</source>
      <translation>北萨米语(芬兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1213"/>
      <source>Northern Sami (Norway)</source>
      <translation>北萨米语(挪威)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1214"/>
      <source>Northern Sami (Sweden)</source>
      <translation>北萨米语(芬兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1219"/>
      <source>Sinhala</source>
      <translation>僧伽罗文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1220"/>
      <source>Sinhala (Sri Lanka)</source>
      <translation>僧伽罗语(斯里兰卡)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1221"/>
      <source>Slovak</source>
      <translation>斯洛伐克文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1222"/>
      <source>Slovak (Slovakia)</source>
      <translation>斯洛伐克语(斯洛伐克)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1223"/>
      <source>Slovenian</source>
      <translation>斯洛文尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1224"/>
      <source>Slovenian (Slovenia)</source>
      <translation>斯洛文尼亚语(斯洛文尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1225"/>
      <source>Somali</source>
      <translation>索马里语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1226"/>
      <source>Somali (Somalia)</source>
      <translation>索马里 (索马里)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1227"/>
      <source>Albanian</source>
      <translation>阿尔巴尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1228"/>
      <source>Albanian (Albania)</source>
      <translation>阿尔巴尼亚语(阿尔巴尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1229"/>
      <source>Serbian</source>
      <translation>塞尔维亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1230"/>
      <source>Serbian (Montenegro)</source>
      <translation>塞尔维亚语 (黑山)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1231"/>
      <source>Serbian (Serbia)</source>
      <translation>塞尔维亚 (塞尔维亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1232"/>
      <source>Serbian (Serbia - Latin-alphabet)</source>
      <translation>塞尔维亚语 (塞尔维亚-拉丁字母)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1233"/>
      <source>Serbian (former state of Yugoslavia)</source>
      <translation>塞尔维亚 (前南斯拉夫)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1234"/>
      <source>Swati</source>
      <translation>斯瓦特语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1235"/>
      <source>Swati (Swaziland)</source>
      <translation>斯瓦蒂 (斯威士兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1236"/>
      <source>Swati (South Africa)</source>
      <translation>南非荷兰语(南非)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1237"/>
      <source>Swedish</source>
      <translation>瑞典语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1238"/>
      <source>Swedish (Sweden)</source>
      <translation>瑞典语(瑞典)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1239"/>
      <source>Swedish (Finland)</source>
      <translation>瑞典语(芬兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1240"/>
      <source>Swahili</source>
      <translation>斯瓦希里语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1241"/>
      <source>Swahili (Kenya)</source>
      <translation>斯瓦希里语(肯尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1242"/>
      <source>Swahili (Tanzania)</source>
      <translation>斯瓦希里语 (坦桑尼亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1256"/>
      <source>Turkish</source>
      <translation>土耳其语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1243"/>
      <source>Telugu</source>
      <translation>泰卢固语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1244"/>
      <source>Telugu (India)</source>
      <translation>泰卢固语(印度)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1245"/>
      <source>Thai</source>
      <translation>泰语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1246"/>
      <source>Thai (Thailand)</source>
      <translation>泰国语(泰国)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1247"/>
      <source>Tigrinya</source>
      <translation>提格利尼亚语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1248"/>
      <source>Tigrinya (Eritrea)</source>
      <translation>提格里尼亚(厄立特里亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1249"/>
      <source>Tigrinya (Ethiopia)</source>
      <translation>提格里尼亚(厄立特里亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1250"/>
      <source>Turkmen</source>
      <translation>土库曼语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1251"/>
      <source>Turkmen (Turkmenistan)</source>
      <translation>土库曼语(土库曼斯坦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1253"/>
      <source>Tswana</source>
      <translation>茨瓦纳文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1254"/>
      <source>Tswana (Botswana)</source>
      <translation>茨瓦纳(博茨瓦纳)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1255"/>
      <source>Tswana (South Africa)</source>
      <translation>南非荷兰语(南非)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1258"/>
      <source>Tsonga</source>
      <translation>宗加文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1259"/>
      <source>Tsonga (South Africa)</source>
      <translation>南非荷兰语(南非)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1260"/>
      <source>Ukrainian</source>
      <translation>乌克兰语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1261"/>
      <source>Ukrainian (Ukraine)</source>
      <translation>乌克兰语(乌克兰)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1262"/>
      <source>Uzbek</source>
      <translation>乌兹别克文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1263"/>
      <source>Uzbek (Uzbekistan)</source>
      <translation>乌兹别克(乌兹别克斯坦)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1264"/>
      <source>Venda</source>
      <translation>文达语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1265"/>
      <source>Vietnamese</source>
      <translation>越南语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1266"/>
      <source>Vietnamese (Vietnam)</source>
      <translation>越南语(越南)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1273"/>
      <source>Walloon</source>
      <translation>瓦隆语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1274"/>
      <source>Xhosa</source>
      <translation>科萨语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1275"/>
      <source>Yiddish</source>
      <translation>依地语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1276"/>
      <source>Chinese</source>
      <translation>中文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1277"/>
      <source>Chinese (China - simplified)</source>
      <translation>中文(简体中文)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1278"/>
      <source>Chinese (Taiwan - traditional)</source>
      <translation>中文（繁体）</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1279"/>
      <source>Zulu</source>
      <translation>祖鲁语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1283"/>
      <source>ASCII (Basic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ASCII(基本)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1285"/>
      <source>UTF-8 (Recommended)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>UTF-8（建議使用）</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1287"/>
      <source>EUC-KR (Korean)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1289"/>
      <source>GBK (Chinese)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>GBK(中文)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1291"/>
      <source>GB18030 (Chinese)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>GB18030(中文)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1293"/>
      <source>Big5-ETen (Taiwan)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>Big5-ETen(台湾)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1295"/>
      <source>Big5-HKSCS (Hong Kong)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>Big5-HKSCS(香港)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1297"/>
      <source>ISO 8859-1 (Western European)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-1 (西欧)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1299"/>
      <source>ISO 8859-2 (Central European)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-2 (Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1301"/>
      <source>ISO 8859-3 (South European)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-3 (South European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1303"/>
      <source>ISO 8859-4 (Baltic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-4 (Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1305"/>
      <source>ISO 8859-5 (Cyrillic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-5 (Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1307"/>
      <source>ISO 8859-6 (Arabic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-6 (Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1309"/>
      <source>ISO 8859-7 (Greek)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-7 (Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1311"/>
      <source>ISO 8859-8 (Hebrew Visual)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-8 (Hebrew Visual)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1313"/>
      <source>ISO 8859-9 (Turkish)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-9 (土耳其)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1315"/>
      <source>ISO 8859-10 (Nordic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-10 (Nordic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1317"/>
      <source>ISO 8859-11 (Latin/Thai)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-11 (Latin/Thai)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1319"/>
      <source>ISO 8859-13 (Baltic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-13 (Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1321"/>
      <source>ISO 8859-14 (Celtic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-14 (Celtic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1323"/>
      <source>ISO 8859-15 (Western)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-15 (Western)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1325"/>
      <source>ISO 8859-16 (Romanian)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>ISO 8859-16 (Romanian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1327"/>
      <location filename="../src/mudlet.cpp" line="1329"/>
      <source>CP437 (OEM Font)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP437 (OEM字体)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1331"/>
      <location filename="../src/mudlet.cpp" line="1333"/>
      <source>CP667 (Mazovia)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP667 (马佐维亚)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1335"/>
      <location filename="../src/mudlet.cpp" line="1337"/>
      <source>CP737 (DOS Greek)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP737 ( DOS 希腊语)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1339"/>
      <source>CP850 (Western Europe)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP850 (Western Europe)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1341"/>
      <source>CP866 (Cyrillic/Russian)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP866 (Cyrillic/Russian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1343"/>
      <location filename="../src/mudlet.cpp" line="1345"/>
      <source>CP869 (DOS Greek 2)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP869 ( DOS 希腊语 2)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1347"/>
      <source>CP1161 (Latin/Thai)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>CP1161(拉丁文/泰文)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1349"/>
      <source>KOI8-R (Cyrillic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>KOI8-R (Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1351"/>
      <source>KOI8-U (Cyrillic/Ukrainian)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>KOI8-U (Cyrillic/Ukrainian)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1353"/>
      <source>MACINTOSH</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>MACINTOSH</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1359"/>
      <source>WINDOWS-1250 (Central European)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1250 (Central European)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1361"/>
      <source>WINDOWS-1251 (Cyrillic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1251 (Cyrillic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1363"/>
      <source>WINDOWS-1252 (Western)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1252 (Western)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1365"/>
      <source>WINDOWS-1253 (Greek)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1253 (Greek)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1367"/>
      <source>WINDOWS-1254 (Turkish)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1254 (Turkish)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1369"/>
      <source>WINDOWS-1255 (Hebrew)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1258 (希伯来)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1371"/>
      <source>WINDOWS-1256 (Arabic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1256 (Arabic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1373"/>
      <source>WINDOWS-1257 (Baltic)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1257 (Baltic)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1375"/>
      <source>WINDOWS-1258 (Vietnamese)</source>
      <extracomment>Keep the English translation intact, so if a user accidentally changes to a language they don&apos;t understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)</extracomment>
      <translation>WINDOWS-1258 (越南)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="6079"/>
      <source>Update check failed. Error: %1
</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="6242"/>
      <source>Could not open profile file: %1</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="6251"/>
      <source>[ ERROR ] - Something went wrong loading your Mudlet profile and it could not be loaded.
Try loading an older version in &apos;Connect - Options - Profile history&apos; or double-check that %1 looks correct.</source>
      <extracomment>%1 is the path and file name (i.e. the location) of the problem fil</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5319"/>
      <source>[ INFO ]  - Mudlet and game sounds are muted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5319"/>
      <source>[ INFO ]  - Mudlet and game sounds are unmuted.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5331"/>
      <source>Unmute sounds from Mudlet (Triggers, Scripts, etc.)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5336"/>
      <source>Unmute sounds from the game (MCMP, MSP)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5556"/>
      <source>Cannot load a replay as one is already in progress in this or another profile.</source>
      <translation>无法加载回放，因为此配置文件或另一个配置文件中已有一个回放进程正在进行中。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5575"/>
      <source>Replay each step with a shorter time interval between steps.</source>
      <translation>以较短的时间间隔重放每一步。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5581"/>
      <source>Replay each step with a longer time interval between steps.</source>
      <translation>以较长的时间间隔重放每一步。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="7422"/>
      <source>Hide tray icon</source>
      <translation>隐藏托盘图标</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="7427"/>
      <source>Quit Mudlet</source>
      <translation>退出 Mudlet</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="255"/>
      <location filename="../src/mudlet.cpp" line="5516"/>
      <source>Main Toolbar</source>
      <extracomment>Name of the main toolbar shown in Qt&apos;s built-in toolbar toggle menus and right-click context menus
----------
Toggle action in the tab bar context menu to show/hide the main toolbar</extracomment>
      <translation>主工具栏</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="301"/>
      <location filename="../src/mudlet.cpp" line="308"/>
      <location filename="../src/mudlet.cpp" line="310"/>
      <source>Connect</source>
      <translation>连接</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="313"/>
      <location filename="../src/mudlet.cpp" line="729"/>
      <source>Disconnect</source>
      <translation>中斷連線</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="414"/>
      <source>Open Discord</source>
      <translation>開啟 Discord</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="332"/>
      <source>Triggers</source>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="219"/>
      <source>hh:mm:ss</source>
      <extracomment>Formatting string for elapsed time display in replay playback - see QDateTime::toString(const QString&amp;) for the gory details...!</extracomment>
      <translation>hh:mm:ss</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="333"/>
      <source>Show and edit triggers</source>
      <translation>顯示及編輯觸發</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="340"/>
      <source>Aliases</source>
      <translation>别名</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="341"/>
      <source>Show and edit aliases</source>
      <translation>顯示及編輯別名</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="346"/>
      <source>Timers</source>
      <translation>時計</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="347"/>
      <source>Show and edit timers</source>
      <translation>顯示及編輯時計</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="352"/>
      <source>Buttons</source>
      <translation>按鈕</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="353"/>
      <source>Show and edit easy buttons</source>
      <translation>顯示及編輯按鈕</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="358"/>
      <source>Scripts</source>
      <translation>腳本</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="359"/>
      <source>Show and edit scripts</source>
      <translation>顯示及編輯腳本</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="364"/>
      <source>Keys</source>
      <translation>按键</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="365"/>
      <source>Show and edit keys</source>
      <translation>顯示及編輯熱鍵</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="370"/>
      <source>Variables</source>
      <translation>變數</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="371"/>
      <source>Show and edit Lua variables</source>
      <translation>顯示及編輯 Lua 變數</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="430"/>
      <source>Map</source>
      <translation>地圖</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="431"/>
      <source>Show/hide the map</source>
      <translation>顯示／隱藏地圖</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="436"/>
      <source>Manual</source>
      <translation>文件</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="437"/>
      <source>Browse reference material and documentation</source>
      <translation>瀏覽參考資料和說明文件</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="442"/>
      <source>Settings</source>
      <translation>設定</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="443"/>
      <source>See and edit profile preferences</source>
      <translation>查看並編輯偏好設定</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="451"/>
      <location filename="../src/mudlet.cpp" line="723"/>
      <source>Notepad</source>
      <translation>记事本</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="452"/>
      <source>Open a notepad that you can store your notes in</source>
      <translation>開啟記事本</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="464"/>
      <location filename="../src/mudlet.cpp" line="473"/>
      <source>Packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="471"/>
      <source>Package Manager</source>
      <translation>套件管理工具</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="476"/>
      <source>Module Manager</source>
      <translation>模組管理工具</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="480"/>
      <source>Package Exporter</source>
      <translation>包导出器</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="490"/>
      <source>Replay</source>
      <translation>記錄回放</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="495"/>
      <location filename="../src/mudlet.cpp" line="730"/>
      <source>Reconnect</source>
      <translation>重新连接</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="496"/>
      <source>Disconnects you from the game and connects once again</source>
      <translation>中斷您與遊戲的連線，並再次連線</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="501"/>
      <location filename="../src/mudlet.cpp" line="726"/>
      <source>MultiView</source>
      <translation>多视图</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="503"/>
      <source>Splits the Mudlet screen to show multiple profiles at once; disabled when less than two are loaded.</source>
      <extracomment>Same text is used in 2 places.</extracomment>
      <translation>分割 Mudlet 屏幕以同时显示多个配置文件；加载的配置文件少于两个时禁用。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="526"/>
      <location filename="../src/mudlet.cpp" line="6131"/>
      <source>About</source>
      <translation>关于</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1160"/>
      <source>Interlingue</source>
      <extracomment>, formerly known as Occidental, and not to be mistaken for Interlingua</extracomment>
      <translation>国际语</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1216"/>
      <source>Shtokavian</source>
      <extracomment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state without a state indication</extracomment>
      <translation>波斯尼亚文</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1218"/>
      <source>Shtokavian (former state of Yugoslavia)</source>
      <extracomment>This code seems to be the identifier for the prestige dialect for several languages used in the region of the former Yugoslavia state with a (withdrawn from ISO 3166) state indication</extracomment>
      <translation>波斯尼亚文（南斯拉夫）</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1257"/>
      <source>Turkish (Turkey)</source>
      <translation>土耳其语(土耳其)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1267"/>
      <location filename="../src/mudlet.cpp" line="1271"/>
      <source>Vietnamese (DauCu variant - old-style diacritics)</source>
      <translation>越南语(DauCu varient -旧式变语)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="1268"/>
      <location filename="../src/mudlet.cpp" line="1272"/>
      <source>Vietnamese (DauMoi variant - new-style diacritics)</source>
      <translation>越南语(DauCu varient -新式变语)</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2665"/>
      <location filename="../src/mudlet.cpp" line="2773"/>
      <location filename="../src/mudlet.cpp" line="5651"/>
      <source>Load a Mudlet replay.</source>
      <translation>加载 Mudlet 回放.</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4855"/>
      <source>Central Debug Console</source>
      <translation>中央调试控制台</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="539"/>
      <location filename="../src/mudlet.cpp" line="747"/>
      <source>Toggle Full Screen View</source>
      <translation>切换全屏显示</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="2582"/>
      <location filename="../src/mudlet.cpp" line="2670"/>
      <source>&lt;p&gt;Load a Mudlet replay.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Disabled until a profile is loaded.&lt;/i&gt;&lt;/p&gt;</source>
      <translation>&lt;p&gt;加载 Mudlet 回放。&lt;/p&gt;&lt;p&gt;&lt;i&gt;在加载配置文件之前禁用。&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4550"/>
      <source>%1 - notes</source>
      <translation>%1 - 笔记</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4653"/>
      <source>Select Replay</source>
      <translation>选择回放</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="4653"/>
      <source>*.dat</source>
      <translation>*.dat</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5140"/>
      <source>[  OK  ]  - Profile &quot;%1&quot; loaded in offline mode.</source>
      <translation>[ 完成 ] - 配置文件 &quot;%1&quot; 在脱机模式下加载完成。</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5573"/>
      <source>Faster</source>
      <translation>加快</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5579"/>
      <source>Slower</source>
      <translation>减慢</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5591"/>
      <location filename="../src/mudlet.cpp" line="5659"/>
      <location filename="../src/mudlet.cpp" line="5668"/>
      <source>Speed: X%1</source>
      <translation>速度: %1</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="5598"/>
      <location filename="../src/mudlet.cpp" line="5614"/>
      <source>Time: %1</source>
      <translation>时间：%1</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="6171"/>
      <source>Update installed - restart to apply</source>
      <translation>更新已安装 - 重新启动以应用</translation>
    </message>
    <message>
      <location filename="../src/mudlet.cpp" line="6302"/>
      <source>[ WARN ]  - Cannot perform replay, another one may already be in progress,
try again when it has finished.</source>
      <translation>[警告] - 不能执行重播, 另一个重播可能已经在进行中，
请等它完成后再次尝试.</translation>
    </message>
  </context>
  <context>
    <name>notes_editor</name>
    <message>
      <location filename="../src/ui/notes_editor.ui" line="49"/>
      <source>toolBar</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/notes_editor.ui" line="86"/>
      <source>Send all</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/notes_editor.ui" line="94"/>
      <source>Send line</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/notes_editor.ui" line="102"/>
      <source>Send selection</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>package_manager</name>
    <message>
      <location filename="../src/ui/package_manager.ui" line="87"/>
      <source>Explore</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="97"/>
      <source>Installed</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="110"/>
      <source>Updates</source>
      <translation>更新</translation>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="174"/>
      <location filename="../src/ui/package_manager.ui" line="177"/>
      <source>Search packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="236"/>
      <source>Install package from repository</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="239"/>
      <source>Install</source>
      <translation>安装</translation>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="249"/>
      <source>Install package from file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="252"/>
      <source>Install from file</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="265"/>
      <source>Remove package from installed packages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="268"/>
      <source>Remove</source>
      <comment>Message on button in package manager initially and when there is no packages to remove.</comment>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="321"/>
      <source>TextLabel</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="423"/>
      <source>Open package repository website</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/package_manager.ui" line="436"/>
      <source>Report an issue with this package</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>package_manager_unpack</name>
    <message>
      <location filename="../src/ui/package_manager_unpack.ui" line="24"/>
      <source>unpacking please wait ...</source>
      <translation>正在解壓縮，請稍後……</translation>
    </message>
  </context>
  <context>
    <name>profile_preferences</name>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="20"/>
      <source>Profile preferences</source>
      <translation>配置设定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="55"/>
      <source>General</source>
      <translation>一般設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="61"/>
      <source>Icon sizes</source>
      <translation>圖示大小</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="67"/>
      <source>Icon size toolbars:</source>
      <translation>工具選單上的圖示大小：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="93"/>
      <source>Icon size in tree views:</source>
      <translation>樹狀視圖中的圖示大小：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="116"/>
      <source>Show menu bar:</source>
      <translation>顯示功能表列：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="130"/>
      <location filename="../src/ui/profile_preferences.ui" line="162"/>
      <location filename="../src/ui/profile_preferences.ui" line="660"/>
      <source>Never</source>
      <translation>從不</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="135"/>
      <location filename="../src/ui/profile_preferences.ui" line="167"/>
      <source>Until a profile is loaded</source>
      <translation>加載完成</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="140"/>
      <location filename="../src/ui/profile_preferences.ui" line="172"/>
      <location filename="../src/ui/profile_preferences.ui" line="670"/>
      <source>Always</source>
      <translation>總是</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="148"/>
      <source>Show main toolbar</source>
      <translation>顯示主工具列</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="355"/>
      <source>Allow server to install script packages</source>
      <translation>允許伺服器安裝腳本套件</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="318"/>
      <source>Game protocols</source>
      <translation>遊戲協議</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="342"/>
      <location filename="../src/ui/profile_preferences.ui" line="4812"/>
      <source>Please reconnect to your game for the change to take effect</source>
      <translation>请重新连接到您的游戏，以使更改生效</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="391"/>
      <source>Log options</source>
      <translation>紀錄選項</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="403"/>
      <source>Save log files in HTML format instead of plain text</source>
      <translation>使用 HTML 格式而非純文字格式來儲存記錄檔案</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="410"/>
      <source>Add timestamps at the beginning of log lines</source>
      <translation>在紀錄文件的開頭加入時間戳記</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="417"/>
      <source>Save log files in:</source>
      <translation>儲存位置：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="434"/>
      <source>Browse...</source>
      <translation>瀏覽...</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="441"/>
      <source>Reset</source>
      <translation>重設</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="448"/>
      <source>Log format:</source>
      <translation>紀錄格式：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="461"/>
      <source>Log name:</source>
      <translation>紀錄檔名：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="481"/>
      <source>.txt</source>
      <translation>.txt</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="514"/>
      <source>Input line</source>
      <translation>輸入設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="523"/>
      <source>Input</source>
      <translation>輸入</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="541"/>
      <source>Strict UNIX line endings</source>
      <translation>在發送的指令中使用嚴格的 UNIX 換行</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="554"/>
      <source>Auto clear the input line after you sent text</source>
      <translation>發送指令後清空指令列內容</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="222"/>
      <source>&lt;p&gt;If you are playing a non-English game and seeing � instead of text, or special letters like &lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt; aren&apos;t showing right - try changing the encoding to UTF-8 or to one suggested by your game.&lt;/p&gt;&lt;p&gt;For some encodings on some Operating Systems Mudlet itself has to provide the codec needed; if that is the case for this Mudlet then there will be a &lt;tt&gt;m &lt;/tt&gt; prefixed applied to those encoding names (so if they have errors the blame can be applied correctly!)&lt;/p&gt;</source>
      <translation>&lt;p&gt;若你正在游玩非英语游戏，本应出现正常文本的地方出现了�，或是像&lt;span style=&quot; font-weight:600;&quot;&gt;ñ&lt;/span&gt;之类的特殊字符，你应该尝试将文本编码切换成UTF-8或你的游戏推荐的编码。&lt;/p&gt;&lt;p&gt;在某些操作系统上，Mudlet不得不使用自带的编码解码器。在这种情况下，对应的编码名称前将有&lt;tt&gt;m &lt;/tt&gt;的前缀。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="397"/>
      <source>&lt;p&gt;When checked will cause the date-stamp named log file to be HTML (file extension &apos;.html&apos;) which can convey color, font and other formatting information rather than a plain text (file extension &apos;.txt&apos;) format.  If changed while logging is already in progress it is necessary to stop and restart logging for this setting to take effect in a new log file.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="535"/>
      <source>&lt;p&gt;Use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="592"/>
      <source>React to all keybindings on the same key</source>
      <translation>响应同一键上的所有键绑定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="567"/>
      <source>&lt;p&gt;Highlights your input line text when scrolling through your history for easy cancellation.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="202"/>
      <source>Can you help translate Mudlet? If so, please visit: https://www.mudlet.org/translate.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="225"/>
      <source>Choose the character encoding for this game. If you see replacement characters or special letters are wrong, try UTF-8 or whichever encoding your game suggests. Encoding names prefixed with the letter m are provided by Mudlet itself rather than the operating system.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="292"/>
      <source>Show a toolbar notification if Mudlet is minimized and new data arrives.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="308"/>
      <source>Mudlet handles telnet:// and telnets:// links</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="302"/>
      <source>&lt;p&gt;If checked, Mudlet will be registered as the default handler for telnet:// and telnets:// (secure) links in your system.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="305"/>
      <source>If checked, Mudlet will be registered as the default handler for telnet:// and telnets:// (secure) links in your system.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="365"/>
      <source>This also needs GMCP to be enabled in the protocols.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="400"/>
      <source>When checked, log files are saved as HTML so colors and fonts are preserved; otherwise they are saved as plain text. If logging is already running, stop and restart it for this to take effect.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="538"/>
      <source>Use strict UNIX line endings on commands for old UNIX servers that can&apos;t handle windows line endings correctly.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="570"/>
      <source>Highlights your input line text when scrolling through your history for easy cancellation.</source>
      <translation>捲動歷史紀錄時，以高亮方式重點標示輸入指令內容以便於取消.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="678"/>
      <source>Command separator:</source>
      <translation>指令分隔符號：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="701"/>
      <source>Command line minimum height in pixels:</source>
      <translation>命令行最小高度像素：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="837"/>
      <source>Main display</source>
      <translation>顯示設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="849"/>
      <source>Font</source>
      <translation>字體設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="855"/>
      <source>Font:</source>
      <translation>字型</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="881"/>
      <source>Size:</source>
      <translation>大小：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="910"/>
      <source>&lt;p&gt;Use anti aliasing on fonts. Smoothes fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="916"/>
      <source>Enable anti-aliasing</source>
      <translation>使用平滑字型（anti-aliasing）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="957"/>
      <source>Display Border</source>
      <translation>顯示邊框</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="975"/>
      <location filename="../src/ui/profile_preferences.ui" line="994"/>
      <source>&lt;p&gt;Extra space to have before text on top - can be set to negative to move text up beyond the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="978"/>
      <source>Top border height:</source>
      <translation>頂部邊框高度：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1025"/>
      <location filename="../src/ui/profile_preferences.ui" line="1044"/>
      <source>&lt;p&gt;Extra space to have before text on the left - can be set to negative to move text left beyond the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1028"/>
      <source>Left border width:</source>
      <translation>左側邊框寬度：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1075"/>
      <location filename="../src/ui/profile_preferences.ui" line="1094"/>
      <source>&lt;p&gt;Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1078"/>
      <source>Bottom border height:</source>
      <translation>底端邊框高度：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1125"/>
      <location filename="../src/ui/profile_preferences.ui" line="1144"/>
      <source>&lt;p&gt;Extra space to have before text on the right - can be set to negative to move text right beyond the screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1128"/>
      <source>Right border width:</source>
      <translation>右側邊框寬度：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1175"/>
      <source>Word wrapping</source>
      <translation>文字換行</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1201"/>
      <source>Wrap lines at:</source>
      <translation>換行位置：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1224"/>
      <location filename="../src/ui/profile_preferences.ui" line="1278"/>
      <location filename="../src/ui/profile_preferences.ui" line="1338"/>
      <location filename="../src/ui/profile_preferences.ui" line="1378"/>
      <source>characters</source>
      <translation>字符</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1255"/>
      <source>Indent wrapped lines by:</source>
      <translation>縮進換行位置：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1309"/>
      <source>&lt;p&gt;Subsequent wrapped lines will be indented by this amount.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1494"/>
      <source>Double-click</source>
      <translation>點擊兩下（Double-Click）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1500"/>
      <source>Stop selecting a word on these characters:</source>
      <translation>在以下字符处停止选中单词：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1516"/>
      <source>&apos;&quot;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1529"/>
      <location filename="../src/ui/profile_preferences.ui" line="1740"/>
      <source>Display options</source>
      <translation>顯示選項</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1541"/>
      <source>Fix unnecessary linebreaks on GA servers</source>
      <translation>修正 GA 伺服器上非必要的換行符號</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1752"/>
      <source>Show Spaces/Tabs</source>
      <translation>顯示空白（space） / 制表符號（tab）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1765"/>
      <source>Show Line/Paragraphs</source>
      <translation>顯示行／段落</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1571"/>
      <source>Echo Lua errors to the main console</source>
      <translation>將 Lua 錯誤輸出顯示到主控制台</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1640"/>
      <source>Editor</source>
      <translation>編輯器</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1646"/>
      <source>Theme</source>
      <translation>主題</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1714"/>
      <source>Updating themes from colorsublime.github.io...</source>
      <translation>从colorsublime.github.io更新主题……</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1772"/>
      <source>&lt;p&gt;Shows bidirection Unicode characters which can be used to change the meaning of source code while remaining invisible to the eye.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1815"/>
      <source>Color view</source>
      <translation>颜色视图</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1821"/>
      <source>Select your color preferences</source>
      <translation>選擇色彩</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1827"/>
      <source>Foreground:</source>
      <translation>前景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1856"/>
      <source>Background:</source>
      <translation>背景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1882"/>
      <source>Command line foreground:</source>
      <translation>命令行前景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1905"/>
      <source>Command line background:</source>
      <translation>命令行背景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1928"/>
      <source>Command foreground:</source>
      <translation>命令前景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1954"/>
      <source>Command background:</source>
      <translation>命令背景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="183"/>
      <source>Language &amp;&amp; data encoding</source>
      <translation>語言編碼</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="189"/>
      <source>Interface language:</source>
      <translation>介面語言：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="209"/>
      <source>Server data encoding:</source>
      <translation>資料編碼：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="237"/>
      <source>Please restart Mudlet to apply the new language</source>
      <translation>请重新启动 Mudlet 以应用新语言</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="247"/>
      <source>Miscellaneous</source>
      <translation>其他</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="586"/>
      <source>&lt;p&gt;Check all Key-bindings against key-presses.&lt;/p&gt;&lt;p&gt;&lt;i&gt;Versions of Mudlet prior to &lt;b&gt;3.9.0&lt;/b&gt; would stop checking after the first matching combination of&lt;/i&gt; KeyCode &lt;i&gt;and&lt;/i&gt; KeyModifier &lt;i&gt;was found and then send the command and/or run the script of that Key-binding only.  This&lt;/i&gt; per-profile &lt;i&gt;option tells Mudlet to check and run the remaining matches; but, to retain compatibility with previous versions, defaults to the &lt;b&gt;un&lt;/b&gt;-checked state.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="733"/>
      <source>Spell checking</source>
      <translation>拼字檢查</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="739"/>
      <source>&lt;p&gt;This option controls spell-checking on the command line in the main console for this profile.&lt;/p&gt;</source>
      <translation>&lt;p&gt;本选项管理的是本配置中在主窗口命令行的拼写检查。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="745"/>
      <source>System/Mudlet dictionary:</source>
      <translation>系统/Mudlet 字典:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="775"/>
      <source>&lt;p&gt;A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;可以使用针对本配置的用户词典。这将用在命令行（里面的单词会显示为带有青色虚线的下划线）和Lua分系统中。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="781"/>
      <source>Profile</source>
      <translation>单个配置</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="788"/>
      <source>&lt;p&gt;A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.&lt;/p&gt;</source>
      <translation>&lt;p&gt;可用于在所有配置中共享的用户词典（要选择此项）。这将用在命令行（里面的单词将会显示为带有青色虚线的下划线）和Lua分系统中。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="794"/>
      <source>Shared</source>
      <translation>共享的</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="928"/>
      <source>The selected font doesn&apos;t work with Mudlet, please pick another</source>
      <translation>選擇的字型無法在 Mudlet 中使用，請選擇其他字型</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1510"/>
      <source>&lt;p&gt;Enter the characters you&apos;d like double-clicking to stop selecting text on here. If you don&apos;t enter any, double-clicking on a word will only stop at a space, and will include characters like a double or a single quote. For example, double-clicking on the word &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; in the following will select &lt;span style=&quot; font-style:italic;&quot;&gt;&amp;quot;&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &lt;span style=&quot; font-weight:600;&quot;&gt;&amp;quot;Hello!&amp;quot;&lt;/span&gt;&lt;/p&gt;&lt;p&gt;If you set the characters in the field to &lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;quot;! &lt;/span&gt;which will mean it should stop selecting on &apos; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; &amp;quot; &lt;span style=&quot; font-style:italic;&quot;&gt;or&lt;/span&gt; ! then double-clicking on &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt; will just select &lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;You say, &amp;quot;&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;!&amp;quot;&lt;/p&gt;</source>
      <translation>&lt;p&gt;在此处输入你&apos;想要双击时停止选择文本的字符。如果你啥也&apos;不输入，在单词上双击将只会在空格处停止，这将会包括进象是单双引号的字符。比如，在下面的单词&lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;上双击，将会选中&lt;span style=&quot; font-style:italic;&quot;&gt;&amp;“&lt;/span&gt;&lt;span style=&quot; font-style:italic;&quot;&gt;Hello!&amp;”&lt;/span&gt;&lt;/p&gt;&lt;p&gt;你说道，&lt;span style=&quot; font-weight:600;&quot;&gt;&amp;“Hello!&amp;”&lt;/span&gt;&lt;/p&gt;&lt;p&gt;而如果你在这里设置的字符是&lt;span style=&quot; font-weight:600;&quot;&gt;&apos;&amp;‘“！的话，&lt;/span&gt;这就意味着它会停在选择的&apos;&lt;span style=&quot; font-style:italic;&quot;&gt;'或是&lt;/span&gt;&amp;”&lt;span style=&quot; font-style:italic;&quot;&gt;或是&lt;/span&gt;！处，那么在&lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;上双击，将只会选中&lt;span style=&quot; font-style:italic;&quot;&gt;Hello&lt;/span&gt;&lt;/p&gt;&lt;p&gt;你说道，&amp;“&lt;span style=&quot; font-weight:600;&quot;&gt;Hello&lt;/span&gt;！&amp;”&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1519"/>
      <source>(characters to ignore in selection, for example &apos; or &quot;)</source>
      <translation>（在选择中忽略的字符，如，&apos; 或&quot;“）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1535"/>
      <source>&lt;p&gt;Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.&lt;/p&gt;</source>
      <translation>&lt;p&gt;有些游戏（尤其是所有的 IRE MUD）深受不能&apos;在客户端上正确显示换行之处的痛苦。启用此项能修复追加到之前提示行处的文本。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1746"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;当在编辑器的主文本编辑区域显示Lua内容时，用可见的制表符和空格标志来代替空白。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1759"/>
      <source>&lt;body&gt;&lt;p&gt;When displaying Lua contents in the main text editor area of the Editor show  line and paragraphs ends with visible marks as well as whitespace.&lt;/p&gt;</source>
      <translation>&lt;body&gt;&lt;p&gt;当在编辑器的主文本编辑区域显示Lua内容时，用可见的行和段落结束标志来代替空白。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1565"/>
      <source>&lt;p&gt;Prints Lua errors to the main console in addition to the error tab in the editor.&lt;/p&gt;</source>
      <translation>&lt;p&gt;除了在編輯器的錯誤分頁之外，也一併將 Lua 錯誤輸出顯示到主控制台&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1548"/>
      <source>Enable text analyzer</source>
      <translation>啟用文字分析</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2498"/>
      <source>Delete map:</source>
      <translation>刪除地圖：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2723"/>
      <source>Use large area exit arrows in 2D view</source>
      <translation>在2D视图中使用大号的区域出口箭头</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3068"/>
      <source>Map info background:</source>
      <translation>地圖資訊背景：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4366"/>
      <source>Allow secure connection reminder</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4360"/>
      <source>&lt;p&gt;To encourage enhanced data transfer protection and privacy, be prompted for a choice to switch to an encrypted port when advertised via Mud Server Status Protocol (MSSP).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2531"/>
      <source>&lt;p&gt;Select profiles that you want to copy map to, then press the Copy button to the right.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2547"/>
      <source>&lt;p&gt;Copy map into the selected profiles on the left.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2582"/>
      <source>&lt;p&gt;Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2624"/>
      <location filename="../src/ui/profile_preferences.ui" line="2640"/>
      <source>&lt;p&gt;On games that provide maps for download, you can press this button to get the latest map. Note that this will &lt;span style=&quot; font-weight:600;&quot;&gt;overwrite&lt;/span&gt; any changes you&apos;ve done to your map, and will use the new map only.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2733"/>
      <source>&lt;p&gt;The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2707"/>
      <source>&lt;p&gt;This enables borders around room. Color can be set in Mapper colors tab.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2691"/>
      <source>&lt;p&gt;If checked, scrolling up zooms out and scrolling down zooms in. If unchecked, scrolling up zooms in and scrolling down zooms out.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2943"/>
      <source>Symbols</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2697"/>
      <source>Invert zoom direction</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3502"/>
      <source>Player room marker</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2782"/>
      <source>Room size:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2826"/>
      <source>Exit size:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2864"/>
      <source>Border size:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2902"/>
      <source>Grid width:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3145"/>
      <source>Grid color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3841"/>
      <source>MudMaster Chat options</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3985"/>
      <source>&lt;p&gt;Show Snoop data in main console window.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3988"/>
      <source>Show Snoop data in main console window.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3991"/>
      <source>Show snoop data in main console</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3860"/>
      <source>Chat name as seen by connected chat clients.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3863"/>
      <source>MMCPUser123</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3921"/>
      <source>Port to use when connecting to another client without specifying a port along with the IP address. This is also the default port that listened for incoming connections when running a local server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3847"/>
      <source>Chat Name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3908"/>
      <source>Default Port:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3953"/>
      <source>Chat Message Prefix:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3882"/>
      <source>Add an extra blank line to vertically space out chat messages.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2758"/>
      <source>Feature sizes:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3696"/>
      <source>Current user name:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3830"/>
      <source>Hide server login time
(Discord shows activity timer when hidden)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3885"/>
      <source>Add extra line to chat messages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3943"/>
      <source>Prefix own EmoteAll messages with &apos;You emote to everybody&apos;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3946"/>
      <source>Prefix emote messages</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3963"/>
      <source>Text to display in front of chat messages.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3966"/>
      <source>&lt;CHAT&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4363"/>
      <source>To encourage enhanced data transfer protection and privacy, be prompted for a choice to switch to an encrypted port when advertised via Mud Server Status Protocol (MSSP).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4382"/>
      <source>&lt;p&gt;Forget the saved sign-in for this game, so the next connection asks you to sign in again. Use this to sign out of this device or switch accounts.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4385"/>
      <source>Forget the saved sign-in for this game, so the next connection asks you to sign in again. Use this to sign out of this device or switch accounts.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4388"/>
      <source>Forget saved sign-in</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4440"/>
      <source>&lt;p&gt;Username for logging into the proxy if required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4443"/>
      <source>Username for logging into the proxy if required.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4459"/>
      <source>&lt;p&gt;Password for logging into the proxy if required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4462"/>
      <source>Password for logging into the proxy if required.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4495"/>
      <source>Shortcuts</source>
      <translation>快捷键</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4510"/>
      <source>Main window shortcuts</source>
      <translation>主窗口快捷键</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4522"/>
      <source>To disable shortcut input &apos;Esc&apos; key.</source>
      <translation>要禁用快捷方式输入 'Esc' 键。</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4557"/>
      <source>Reset to defaults</source>
      <translation>重置設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4593"/>
      <source>&lt;p&gt;On some platforms, like macOS, the screen reader tool has issues announcing incoming text fully, without skipping. You can opt into disabling announcing new text from the game with this option to use a custom TTS instead which avoids such issues.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4596"/>
      <source>When checked, Mudlet announces incoming game text through the system screen reader. On some platforms such as macOS the system screen reader may skip text; if that happens, uncheck this and use a custom TTS solution instead.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4606"/>
      <source>Advertise screen reader use via protocols supporting this notice (NEW-ENVIRON, MNES, MTTS)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4613"/>
      <source>Enable closed caption for media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4659"/>
      <source>&lt;p&gt;In caret mode, hyperlinks in the main window can be navigated with &lt;b&gt;Ctrl+]&lt;/b&gt; (next link) and &lt;b&gt;Ctrl+[&lt;/b&gt; (previous link), activated with &lt;b&gt;Enter&lt;/b&gt; or &lt;b&gt;Space&lt;/b&gt;, and their context menu opened with the &lt;b&gt;Menu&lt;/b&gt; key or &lt;b&gt;Shift+F10&lt;/b&gt;. Press &lt;b&gt;Ctrl+End&lt;/b&gt; to jump to the latest content (Mac: &lt;b&gt;Ctrl+Fn+Right Arrow&lt;/b&gt;) or &lt;b&gt;Ctrl+Home&lt;/b&gt; to jump to the start (Mac: &lt;b&gt;Ctrl+Fn+Left Arrow&lt;/b&gt;). Choosing &lt;b&gt;Ctrl+Tab&lt;/b&gt; or &lt;b&gt;F6&lt;/b&gt; here keeps plain &lt;b&gt;Tab&lt;/b&gt; available for stepping through links.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4662"/>
      <source>In caret mode, navigate hyperlinks with Ctrl+] for the next link and Ctrl+[ for the previous link, activate the focused link with Enter or Space, and open its menu with the Menu key or Shift+F10. Press Ctrl+End to jump to the latest content or Ctrl+Home to jump to the start of the buffer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4689"/>
      <source>&lt;p&gt;Enable F3 and Shift+F3 shortcuts for searching up and down in the buffer.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4692"/>
      <source>Enable F3 and Shift+F3 shortcuts for searching up and down in the buffer.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4705"/>
      <source>When enabled, text with the blinking attribute (SGR codes 5 and 6) is displayed with a smooth pulsing effect. When disabled, blinking text is shown in italics instead.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4708"/>
      <source>Enable blinking text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4754"/>
      <source>&lt;p&gt;This option adds a line line break &lt;LF&gt; or &quot;
&quot; to your command input on empty commands. This option will rarely be necessary.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4758"/>
      <source>This option adds a line break (LF, or new-line) to your command input on empty commands. This option will rarely be necessary.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4775"/>
      <source>&lt;p&gt;Some servers use KaVir’s protocol snippet, which expects the client to provide both its name and a decimal version number during Telnet TTYPE negotiation. However, including a version number is not in accordance with the relevant RFCs as the period character is not permitted therein; so since 2024 Mudlet has stopped sending it by default. As a result, servers that rely on this information may assume Mudlet is version 1.0 or earlier, and consequently restrict color support to 16 colors instead of enabling 256-color mode.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4778"/>
      <source>When checked, send Mudlet&apos;s version number alongside its name during Telnet TTYPE negotiation. Some servers using KaVir&apos;s protocol snippet need this to enable 256-color mode; Mudlet stopped sending it by default in 2024 because the period character it contains is not allowed by the relevant RFCs.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4781"/>
      <source>Send Mudlet version in terminal type</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4788"/>
      <source>&lt;p&gt;Some servers do not negotiate Mud eXtension Protocol (MXP). When checked, this preference forces the MXP processor to be enabled. Note: To disable MXP entirely, leave this unchecked and also uncheck MXP in Choose protocols section of the General tab in Settings.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4791"/>
      <source>When checked, force the MXP processor on for servers that do not negotiate it. To disable MXP entirely, leave this unchecked and also uncheck MXP under Choose protocols on the General tab.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4794"/>
      <source>Force MXP processing on</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4828"/>
      <source>Clear stored media</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4834"/>
      <location filename="../src/ui/profile_preferences.ui" line="4850"/>
      <source>&lt;p&gt;Media files used with Mudlet&apos;s Lua API, Mud Client Media Protocol (MCMP), and Mud Sound Protocol (MSP) are cached with the game profile. You can press this button to clear the media cache. For many games the media will get downloaded again upon demand.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4837"/>
      <location filename="../src/ui/profile_preferences.ui" line="4853"/>
      <source>Media files used with Mudlet&apos;s Lua API, Mud Client Media Protocol (MCMP), and Mud Sound Protocol (MSP) are cached with the game profile. You can press this button to clear the media cache. For many games the media will get downloaded again upon demand.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4840"/>
      <source>Purge stored media files for the current profile:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4856"/>
      <source>Clear</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5087"/>
      <source>Crash report sending policy:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5098"/>
      <source>Always send</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5103"/>
      <source>Never send</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5108"/>
      <source>Ask each time</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4916"/>
      <source>&lt;p&gt;Some MUDs use a flawed interpretation of the ANSI Set Graphics Rendition (&lt;b&gt;SGR&lt;/b&gt;) code sequences for 16M color mode which only uses semi-colons and not colons to separate parameter elements i.e. instead of using a code in the form: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38:2:&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Unused&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance&amp;gt;&lt;tt&gt;:&lt;/tt&gt;&amp;lt;Tolerance Color Space (0=CIELUV; 1=CIELAB)&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;where the &lt;i&gt;Color Space Id&lt;/i&gt; is expected to be an empty string to specify the usual (default) case and all of the &lt;i&gt;Parameter Elements&lt;/i&gt; (the &quot;2&quot; and the values in the &lt;tt&gt;&amp;lt;...&amp;gt;&lt;/tt&gt;s) may, technically, be omitted; they use: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt;&lt;br&gt;or: &lt;br&gt;&lt;tt&gt;\e[&lt;/tt&gt;...&lt;tt&gt;38;2;&lt;/tt&gt;&amp;lt;Color Space Id&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Red&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Green&amp;gt;&lt;tt&gt;;&lt;/tt&gt;&amp;lt;Blue&amp;gt;&lt;tt&gt;;&lt;/tt&gt;...&lt;tt&gt;m&lt;/tt&gt; .&lt;/p&gt;&lt;p&gt;It is not possible to reliably detect the difference between these two so checking this option causes Mudlet to expect the last one with the additional (but empty!) parameter.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4919"/>
      <source>When checked, interpret 16-million-color SGR sequences using the non-standard semi-colon form some MUDs send, which includes an extra empty parameter for the color space identifier. Enable this if true-color text from your game shows the wrong colors.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4989"/>
      <source>Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the Central Debug Console useless if there is a very small interval timer running.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5026"/>
      <source>h:mm:ss.zzz</source>
      <comment>Used to set a time interval only</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1724"/>
      <source>Autocomplete</source>
      <translation>自動補全</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="295"/>
      <source>Notify on new data</source>
      <translation>新数据时通知</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="289"/>
      <source>&lt;p&gt;Show a toolbar notification if Mudlet is minimized and new data arrives.&lt;/p&gt;</source>
      <translation>&lt;p&gt;如果 Mudlet 已最小化并且新数据到达，则显示工具栏通知。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="282"/>
      <source>Auto save on exit</source>
      <translation>退出时自动保存</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="368"/>
      <source>Allow server to download and play media</source>
      <translation>允许服务器下载和播放媒体</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="264"/>
      <source>System setting</source>
      <translation>系統設定</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="269"/>
      <source>Light</source>
      <translation>亮色主题</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="274"/>
      <source>Dark</source>
      <translation>深色主题</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="253"/>
      <source>Appearance</source>
      <translation>外觀</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="573"/>
      <source>Highlight history</source>
      <translation>突出显示历史记录</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="589"/>
      <source>When checked, every key-binding that matches a key-press is run, not only the first match. Off by default for compatibility with older Mudlet behaviour.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="605"/>
      <source>&lt;p&gt;Disable password masking when servers request hidden input.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Warning:&lt;/b&gt; This is not recommended for security reasons as passwords will be visible in plain text.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="608"/>
      <source>Disable password masking when servers request hidden input. Warning: This is not recommended for security reasons as passwords will be visible in plain text.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="611"/>
      <source>Disable password masking</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="631"/>
      <source>Show sent commands:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="647"/>
      <source>&lt;p&gt;Controls how sent commands are echoed in the display box:&lt;/p&gt;
&lt;ul&gt;
&lt;li&gt;&lt;b&gt;Never&lt;/b&gt;: Commands are never shown regardless of script settings&lt;/li&gt;
&lt;li&gt;&lt;b&gt;Script controlled&lt;/b&gt;: Scripts can control visibility using send(cmd, true/false)&lt;/li&gt;
&lt;li&gt;&lt;b&gt;Always&lt;/b&gt;: Commands are always shown regardless of script settings&lt;/li&gt;
&lt;/ul&gt;
&lt;p&gt;&lt;i&gt;This can be disabled by the game server if it negotiates to use the telnet ECHO option.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="656"/>
      <source>Controls how sent commands are echoed in the display. Never means commands are never shown; Script controlled lets scripts decide via the second argument to send(); Always shows commands regardless of script settings. The game server can override this by negotiating the telnet ECHO option.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="665"/>
      <source>Script controlled</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="742"/>
      <source>This option controls spell-checking on the command line in the main console for this profile.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="778"/>
      <source>A user dictionary specific to this profile will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="791"/>
      <source>A user dictionary that is shared between all profiles (which have this option selected) will be available. This will be on the command line (words which are in it will appear with a dashed cyan underline) and in the lua sub-system.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="913"/>
      <source>Use anti aliasing on fonts. Smooths fonts if you have a high screen resolution and you can use larger fonts. Note that on low resolutions and small font sizes, the font gets blurry.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="940"/>
      <source>This font is not monospace, which may not be ideal for playing some text games:
you can use it but there could be issues with aligning columns of text</source>
      <comment>Note that this text is split into two lines so that the message is not too wide in English, please do the same for other locales where the text is the same or longer</comment>
      <translation>这个字体不是等宽字体，在玩一些文字游戏时, 可能有文本不对齐的问题.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="997"/>
      <source>Extra space to have before text on top - can be set to negative to move text up beyond the screen.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1047"/>
      <source>Extra space to have before text on the left - can be set to negative to move text left beyond the screen.</source>
      <translation>左边文本之前额外的空间——可以设置为负数来将文本向左移出屏幕.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1097"/>
      <source>Extra space to have before text on the bottom - can be set to negative to allow text to go down beyond the screen.</source>
      <translation>底部文本之前额外的空间——可以设置为负数来将文本向下移出屏幕.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1147"/>
      <source>Extra space to have before text on the right - can be set to negative to move text right beyond the screen.</source>
      <translation>右边文本之前额外的空间——可以设置为负数来将文本向右移出屏幕.</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1312"/>
      <source>Subsequent wrapped lines will be indented by this amount.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1315"/>
      <source>Indent hanging wrapped lines by:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1406"/>
      <source>Scrollback</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1430"/>
      <source>&lt;p&gt;Maximum number of lines to keep in the console buffer. When exceeded, older lines are removed in batches.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1433"/>
      <source>Maximum number of lines to keep in the console buffer. When exceeded, older lines are removed in batches.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1436"/>
      <source>Main display size:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1446"/>
      <source>&lt;p&gt;Maximum number of lines to keep in the console buffer. Minimum is 100 lines.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1449"/>
      <source>Maximum number of lines to keep in the console buffer. Minimum is 100 lines.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1465"/>
      <source>lines</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1478"/>
      <source>&lt;p&gt;Use the maximum buffer size your system can handle. This will be calculated based on available memory.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1481"/>
      <source>Use the maximum buffer size your system can handle. This will be calculated based on available memory.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1484"/>
      <source>Use maximum lines possible</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1513"/>
      <source>Characters that double-click selection should stop on. Without this, only spaces end a selection, so quotes and punctuation get included with the word. For example, entering an apostrophe, double quote and exclamation mark would make double-clicking select just the word Hello rather than &quot;Hello!&quot;.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1578"/>
      <source>Display control characters as:</source>
      <translation>顯示控制字元為：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1555"/>
      <source>Make &apos;Ambiguous&apos; E. Asian width characters wide</source>
      <translation>设置&apos;模糊的&apos;亚洲字符的宽度</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="375"/>
      <source>&lt;p&gt;OSC 8 lets a game server put clickable links in its output, which can send commands, pre-fill your input line, or open a web page. Uncheck to ignore them and to stop telling servers that Mudlet supports them.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="378"/>
      <source>When checked, clickable OSC 8 hyperlinks from the game server are shown and Mudlet advertises support for them. When unchecked, the sequences are ignored and the capability is not advertised.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="381"/>
      <source>Enable OSC 8 hyperlinks from the server</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1355"/>
      <source>&lt;p&gt;Some games wrap their own lines and offer no way to turn that off, which makes triggers awkward to write: the text a trigger sees can be split mid-sentence. This option joins those wrapped lines back together before triggers run, so triggers always see whole lines and the wrapping above applies for display instead.&lt;/p&gt;&lt;p&gt;Set the column to the width the game wraps at (very often 80). Only lines from the game are affected.&lt;/p&gt;&lt;p&gt;&lt;i&gt;This feature is experimental: it tells wrapped prose apart from prompts, tables and ASCII art by their shape, so the occasional line may still be joined or left split when it should not be.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1358"/>
      <source>Undo the game&apos;s own wrapping at:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1538"/>
      <source>Some games (notably all IRE MUDs) suffer from a bug where they don&apos;t properly communicate with the client on where a newline should be. Enable this to fix text from getting appended to the previous prompt line.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1568"/>
      <source>Prints Lua errors to the main console in addition to the error tab in the editor.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1595"/>
      <source>nothing</source>
      <translation>無</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1600"/>
      <source>Unicode Control Pictures</source>
      <translation>Unicode 控制图像</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1605"/>
      <source>CP437 (OEM Font)- like</source>
      <translation>CP437 (OEM 字体) - 类似</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1613"/>
      <source>Display whenever a tab is connected or a disconnected</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1616"/>
      <source>Show connection status on tabs</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1730"/>
      <source>Autocomplete Lua functions in code editor</source>
      <translation>在代码编辑器中自动补全(自动完成) Lua 函数</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1749"/>
      <source>When displaying Lua contents in the main text editor area of the Editor show tabs and spaces with visible marks instead of whitespace.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1762"/>
      <source>When displaying Lua contents in the main text editor area of the Editor show line and paragraph ends with visible marks as well as whitespace.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1775"/>
      <source>Shows bidirectional Unicode characters which can be used to change the meaning of source code while remaining invisible to the eye.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1778"/>
      <source>Show invisible Unicode control characters</source>
      <translation>显示不可见的 Unicode 控制字符</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1785"/>
      <source>&lt;p&gt;Shows the &lt;b&gt;unique&lt;/b&gt; ID number that Mudlet uses internally to identify each instance of an item this is the same number that the Lua API functions that create aliases, key-binding, etc. return on success. This may be useful to know when there are multiple items of the same type with the same name and will be incorporated in the names of the related items&apos; Lua scripts in the Central Debug Console output.&lt;/p&gt;&lt;p&gt;Note that although the number assigned to an item is constant during a session of the profile it may be different the next time the profile is loaded if other items are added or removed.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1788"/>
      <source>Shows Mudlet&apos;s internal ID number for each item. This is the same ID returned by Lua functions that create aliases, triggers, timers and so on, and is helpful when several items share the same name. The ID is stable during a session but may change the next time the profile is loaded.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1791"/>
      <source>Show Items&apos; ID number</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1837"/>
      <source>&lt;p&gt;The foreground color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于主控制台的默认前景颜色（除非通过Lua命令或游戏服务器进行了修改）。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1840"/>
      <source>The foreground color used by default for the main console (unless changed by a lua command or the game server).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1866"/>
      <source>&lt;p&gt;The background color used by default for the main console (unless changed by a lua command or the game server).&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于主控制台的默认背景颜色（除非通过Lua命令或游戏服务器进行了修改）。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1869"/>
      <source>The background color used by default for the main console (unless changed by a lua command or the game server).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1892"/>
      <source>&lt;p&gt;The foreground color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于主输入区的前景颜色。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1895"/>
      <source>The foreground color used for the main input area.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1915"/>
      <source>&lt;p&gt;The background color used for the main input area.&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于主输入区的背景颜色。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1918"/>
      <source>The background color used for the main input area.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1938"/>
      <source>&lt;p&gt;The foreground color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于送往游戏服务器的文本的前景颜色。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1941"/>
      <source>The foreground color used for text sent to the game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1964"/>
      <source>&lt;p&gt;The background color used for text sent to the game server.&lt;/p&gt;</source>
      <translation>&lt;p&gt;用于送往游戏服务器的文本的背景颜色。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1967"/>
      <source>The background color used for text sent to the game server.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="1993"/>
      <source>These preferences set how you want a particular color to be represented visually in the main display:</source>
      <translation>這些偏好設定是你想要在主視窗中直觀顯示的特定顏色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2003"/>
      <location filename="../src/ui/profile_preferences.ui" line="3172"/>
      <source>Black:</source>
      <translation>黑色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2013"/>
      <source>ANSI Color Number 0</source>
      <translation>ANSI颜色编号0</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2026"/>
      <location filename="../src/ui/profile_preferences.ui" line="3192"/>
      <source>Light black:</source>
      <translation>淺黑：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2036"/>
      <source>ANSI Color Number 8</source>
      <translation>ANSI颜色编号8</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2049"/>
      <location filename="../src/ui/profile_preferences.ui" line="3212"/>
      <source>Red:</source>
      <translation>紅色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2059"/>
      <source>ANSI Color Number 1</source>
      <translation>ANSI颜色编号1</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2072"/>
      <location filename="../src/ui/profile_preferences.ui" line="3232"/>
      <source>Light red:</source>
      <translation>浅红色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2082"/>
      <source>ANSI Color Number 9</source>
      <translation>ANSI颜色编号9</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2095"/>
      <location filename="../src/ui/profile_preferences.ui" line="3252"/>
      <source>Green:</source>
      <translation>綠色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2105"/>
      <source>ANSI Color Number 2</source>
      <translation>ANSI颜色编号2</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2118"/>
      <location filename="../src/ui/profile_preferences.ui" line="3272"/>
      <source>Light green:</source>
      <translation>浅绿色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2128"/>
      <source>ANSI Color Number 10</source>
      <translation>ANSI颜色编号10</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2141"/>
      <location filename="../src/ui/profile_preferences.ui" line="3292"/>
      <source>Yellow:</source>
      <translation>黄色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2151"/>
      <source>ANSI Color Number 3</source>
      <translation>ANSI颜色编号3</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2164"/>
      <location filename="../src/ui/profile_preferences.ui" line="3312"/>
      <source>Light yellow:</source>
      <translation>淡黄色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2174"/>
      <source>ANSI Color Number 11</source>
      <translation>ANSI颜色编号11</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2187"/>
      <location filename="../src/ui/profile_preferences.ui" line="3332"/>
      <source>Blue:</source>
      <translation>藍色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2197"/>
      <source>ANSI Color Number 4</source>
      <translation>ANSI颜色编号4</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2210"/>
      <location filename="../src/ui/profile_preferences.ui" line="3352"/>
      <source>Light blue:</source>
      <translation>浅蓝色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2220"/>
      <source>ANSI Color Number 12</source>
      <translation>ANSI颜色编号12</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2233"/>
      <location filename="../src/ui/profile_preferences.ui" line="3372"/>
      <source>Magenta:</source>
      <translation>洋紅：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2243"/>
      <source>ANSI Color Number 5</source>
      <translation>ANSI颜色编号5</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2256"/>
      <location filename="../src/ui/profile_preferences.ui" line="3392"/>
      <source>Light magenta:</source>
      <translation>浅洋红色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2266"/>
      <source>ANSI Color Number 13</source>
      <translation>ANSI颜色编号13</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2279"/>
      <location filename="../src/ui/profile_preferences.ui" line="3412"/>
      <source>Cyan:</source>
      <translation>青色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2289"/>
      <source>ANSI Color Number 6</source>
      <translation>ANSI颜色编号6</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2302"/>
      <location filename="../src/ui/profile_preferences.ui" line="3432"/>
      <source>Light cyan:</source>
      <translation>浅青色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2312"/>
      <source>ANSI Color Number 14</source>
      <translation>ANSI颜色编号14</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2325"/>
      <location filename="../src/ui/profile_preferences.ui" line="3452"/>
      <source>White:</source>
      <translation>白色：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2335"/>
      <source>ANSI Color Number 7</source>
      <translation>ANSI颜色编号7</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2348"/>
      <location filename="../src/ui/profile_preferences.ui" line="3472"/>
      <source>Light white:</source>
      <translation>浅白色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2358"/>
      <source>ANSI Color Number 15</source>
      <translation>ANSI颜色编号15</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2374"/>
      <source>When checked, the game server may change the 16 ANSI colors above using standard OSC P escape sequences, or reset them to their defaults using OSC R, the same as the reset button beside this option.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2384"/>
      <location filename="../src/ui/profile_preferences.ui" line="3492"/>
      <source>Reset all colors to default</source>
      <translation>將所有顏色重設為預設值</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2371"/>
      <source>&lt;p&gt;If this option is checked the Mud Server may send codes to change the above 16 colors or to reset them to their defaults by using standard ANSI &lt;tt&gt;OSC&lt;/tt&gt; Escape codes.&lt;/p&gt;&lt;p&gt;Specifically &lt;tt&gt;&amp;lt;OSC&amp;gt;Pirrggbb&amp;lt;ST&amp;gt;&lt;/tt&gt; will set the color with index &lt;i&gt;i&lt;/i&gt; to have the color with the given &lt;i&gt;rr&lt;/i&gt; red, &lt;i&gt;gg&lt;/i&gt; green and &lt;i&gt;bb&lt;/i&gt;  blue components where i is a single hex-digit (&apos;0&apos; to &apos;9&apos; or &apos;a&apos; to &apos;f&apos; or &apos;A&apos; to &apos;F&apos; to give a number between 0 an d15) and rr, gg and bb are two digit hex-digits numbers (between 0 to 255); &amp;lt;OSC&amp;gt; is &lt;i&gt;Operating System Command&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;[&lt;/tt&gt; and &amp;lt;ST&amp;gt; is the &lt;i&gt;String Terminator&lt;/i&gt; which is normally encoded as the ASCII &amp;lt;ESC&amp;gt; character followed by &lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;Conversely &lt;tt&gt;&amp;lt;OSC&amp;gt;R&amp;lt;ST&amp;gt;&lt;/tt&gt; will reset the colors to the defaults like the button to the right does.&lt;/p&gt;</source>
      <translation>&lt;p&gt;如果勾选了此项，Mud服务器可能会送出代码来修改为超过16色或通过使用标准ANSI&lt;tt&gt;OSC&lt;/tt&gt;Escape code来重置它们为它们的默认值。&lt;/p&gt;&lt;p&gt;具体地说&lt;tt&gt;&amp;&lt;OSC>Pirrggbb&amp;&lt;ST>&amp;&lt;/tt&gt;将会设置index/&lt;i&gt;&lt;/i&gt;的颜色为指定的&lt;i&gt;rr&lt;/i&gt;红色、&lt;i&gt;gg&lt;/i&gt;绿色、&lt;i&gt;bb&lt;/i&gt;蓝色所混合的颜色。其中i是1位16进制数（&apos;0&apos;到9&apos;，或&apos;a&apos;到&apos;f&apos;，或&apos;A&apos;到&apos;F&apos;为0到d15），而rr、gg、bb为2位16进制数（0到255之间）；&amp;&lt;OSC>&amp;是&lt;i&gt;Operating System Command&lt;/i&gt;，它是象ASCII那样的正常编码,&amp;&lt;ESC>&amp;字符后面为&lt;tt&gt;[&lt;/tt&gt;，而&amp;&lt;ST>&amp;是&lt;i&gt;String Terminator&lt;/i&gt;，它是象ASCII那样的正常编码，&amp;&lt;ESC>&amp;字符后面是&lt;tt&gt;\&lt;tt&gt;.&lt;/p&gt;&lt;p&gt;反之，&lt;tt&gt;&amp;&lt;OSC>&amp;R&amp;&lt;ST>&amp;&lt;/tt&gt;将会重置颜色为右边按钮的默认值。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2377"/>
      <source>Server allowed to redefine these colors</source>
      <translation>允许服务器重定义这些颜色</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2408"/>
      <source>Mapper</source>
      <translation>地圖工具</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2414"/>
      <source>Map files</source>
      <translation>地圖檔案</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2420"/>
      <source>Save your current map:</source>
      <translation>儲存目前地圖：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2450"/>
      <source>Load another map file in:</source>
      <translation>載入其他地圖：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2437"/>
      <source>&lt;p&gt;Mudlet now does some sanity checking and repairing to clean up issues that may have arisen in previous version due to faulty code or badly documented commands. However if significant problems are found the report can be quite extensive, in particular for larger maps.&lt;/p&gt;&lt;p&gt;Unless this option is set, Mudlet will reduce the amount of on-screen messages by hiding many texts and showing a suggestion to review the report file instead.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="324"/>
      <source>Choose protocols</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="362"/>
      <source>&lt;p&gt;This also needs GMCP to be enabled in the protocols.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2443"/>
      <source>report map issues on screen</source>
      <translation>在屏幕上报告地图问题</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2460"/>
      <source>choose map...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2467"/>
      <source>Or load an older version:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2484"/>
      <source>◀ load this map</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2511"/>
      <source>delete</source>
      <comment>Text on the button to delete a map, ensure the text matches the word or words `quoted` for the adjacent checkbox</comment>
      <translation>刪除</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2537"/>
      <source>pick destinations...</source>
      <comment>text on button to select other profiles to receive the map from this profile, this is used when no profiles have been selected</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2630"/>
      <source>Download latest map provided by your game:</source>
      <translation>下载由您的游戏提供的最新地图:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2662"/>
      <source>&lt;p&gt;This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer.&lt;/p&gt;&lt;p&gt;3D map view always has anti-aliasing enabled.&lt;/p&gt;</source>
      <translation>&lt;p&gt;启用适用于2D地图显示的抗锯齿（AA），这会看上去更平滑更漂亮。要是你&apos;在一台非常慢的电脑上的话，就禁用此项。&lt;/p&gt;&lt;p&gt;3D地图显示会始终启用抗锯齿的。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2675"/>
      <source>&lt;p&gt;When enabled, rooms on floors above and below the current level will be drawn with a lighter color to show the map layout context.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2440"/>
      <source>When checked, show the full map sanity-check and repair report on screen. Otherwise Mudlet hides most messages and points you at the report file instead, which is helpful for large maps.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2534"/>
      <source>Select profiles that you want to copy map to, then press the Copy button to the right.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2550"/>
      <source>Copy map into the selected profiles on the left.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2585"/>
      <source>Change this to a lower version if you need to save your map in a format that can be read by older versions of Mudlet. Doing so will lose the extra data available in the current map format.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2627"/>
      <location filename="../src/ui/profile_preferences.ui" line="2643"/>
      <source>On games that provide maps for download, you can press this button to get the latest map. Note that this will overwrite any changes you&apos;ve done to your map, and will use the new map only.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2678"/>
      <source>When enabled, rooms on floors above and below the current level will be drawn with a lighter color to show the map layout context.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2681"/>
      <source>Draw rooms on upper and lower levels</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2694"/>
      <source>If checked, scrolling up zooms out and scrolling down zooms in. If unchecked, scrolling up zooms in and scrolling down zooms out.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2736"/>
      <source>The default area (area id -1) is used by some mapper scripts as a temporary &apos;holding area&apos; for rooms before they&apos;re placed in the correct area.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2710"/>
      <source>This enables borders around room. Color can be set in Mapper colors tab.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2665"/>
      <source>This enables anti-aliasing (AA) for the 2D map view, making it look smoother and nicer. Disable this if you&apos;re on a very slow computer. 3D map view always has anti-aliasing enabled.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3508"/>
      <source>2D map player room marker style:</source>
      <translation>2D 地图玩家房间标记样式：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3521"/>
      <source>Outer ring color</source>
      <translation>外圈颜色</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3531"/>
      <source>Inner ring color</source>
      <translation>内圈颜色</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3545"/>
      <source>Original</source>
      <translation>原始</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3550"/>
      <source>Red ring</source>
      <translation>红圈</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3555"/>
      <source>Blue/Yellow ring</source>
      <translation>蓝/黄圈</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3560"/>
      <source>Custom ring</source>
      <translation>自定义圈</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3568"/>
      <source>&lt;p&gt;Percentage ratio (&lt;i&gt;the default is 120%&lt;/i&gt;) of the marker symbol to the space available for the room.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3571"/>
      <source>Percentage ratio (the default is 120%) of the marker symbol to the space available for the room.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3577"/>
      <location filename="../src/ui/profile_preferences.ui" line="3608"/>
      <source>%</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3580"/>
      <source>Outer diameter: </source>
      <translation>外径: </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3599"/>
      <source>&lt;p&gt;Percentage ratio of the inner diameter of the marker symbol to the outer one (&lt;i&gt;the default is 70%&lt;/i&gt;).&lt;/p&gt;</source>
      <translation>&lt;p&gt;标记符号内部直径与外部直径的百分比 (&lt;i&gt;默认是 70%&lt;/i&gt;)。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3602"/>
      <source>Percentage ratio of the inner diameter of the marker symbol to the outer one (the default is 70%).</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3611"/>
      <source>Inner diameter: </source>
      <translation>内径： </translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2713"/>
      <source>Show room borders</source>
      <translation>显示房间边框</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3048"/>
      <source>Room border color:</source>
      <translation>房间边框颜色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3641"/>
      <source>Chat</source>
      <translation>聊天</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4741"/>
      <source>Special options needed for some older game drivers (needs client restart to take effect)</source>
      <translation>特别选项需要一些较早的老游戏驱动（需要客户端重启才生效）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4940"/>
      <source>the computer&apos;s password manager (secure)</source>
      <translation>计算机密码管理器 (安全)</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4945"/>
      <source>plaintext with the profile (portable)</source>
      <translation>带有配置文件的明文（便携式）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5036"/>
      <source>&lt;p&gt;If checked this will cause all problem Unicode codepoints to be reported in the debug output as they occur; if cleared then each different one will only be reported once and summarized in as a table when the console in which they occurred is finally destroyed (when the profile is closed).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4922"/>
      <source>Expect Color Space Id in SGR...(3|4)8;2;...m codes</source>
      <translation>在SGR...(3|4)8;2;...m代码中预测Color Space Id</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4929"/>
      <source>Store character login passwords in:</source>
      <translation>保存角色登录密码：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4027"/>
      <source>TLS/SSL secure connection</source>
      <translation>TLS/SSL 安全连接</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4128"/>
      <source>Accept self-signed certificates</source>
      <translation>接受自签名认证</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4138"/>
      <source>Accept expired certificates</source>
      <translation>接受过期认证</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4045"/>
      <source>Certificate</source>
      <translation>憑證</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4105"/>
      <source>Serial:</source>
      <translation>序號：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4054"/>
      <source>Issuer:</source>
      <translation>發行者：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4071"/>
      <source>Issued to:</source>
      <translation>發佈至：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4088"/>
      <source>Expires:</source>
      <translation>到期：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4148"/>
      <source>Accept all certificate errors       (unsecure)</source>
      <translation>接受所有的认证错误（不安全）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2518"/>
      <source>Copy map to other profile(s):</source>
      <translation>将地图复制到其他配置文件：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2608"/>
      <source>An action above happened</source>
      <translation>发生上述操作</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2560"/>
      <source>Map format version:</source>
      <translation>地图格式版本:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2591"/>
      <location filename="../src/ui/profile_preferences.ui" line="2595"/>
      <source># {default version}</source>
      <translation># {default version}</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2618"/>
      <source>Map download</source>
      <translation>地圖下載</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2646"/>
      <source>Download</source>
      <translation>下載</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2656"/>
      <source>Map view</source>
      <translation>地圖檢視</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2949"/>
      <source>2D Map Room Symbol Font</source>
      <translation>2D地图的房间标志字体</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2668"/>
      <source>Use high quality graphics in 2D view</source>
      <translation>在 2D 檢視中使用高品質圖形</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="199"/>
      <source>&lt;p&gt;Can you help translate Mudlet?&lt;/p&gt;&lt;p&gt;If so, please visit: &lt;b&gt;https://www.mudlet.org/translate&lt;/b&gt;.&lt;/p&gt;</source>
      <translation>&lt;bp&gt;你能帮忙翻译Mudlet吗？&lt;bspan style=&quot;font-weight:600;&quot;&gt;https://www.mudlet.org/translate&lt;b/span&gt;&lt;b/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="694"/>
      <source>Text to separate commands or blank to disable</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="768"/>
      <source>User dictionary: </source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2430"/>
      <source>choose location...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2739"/>
      <source>Show the default area in map area selection</source>
      <translation>显示地图区域中的默认区域</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2966"/>
      <source>Only use symbols (glyphs) from chosen font</source>
      <translation>只使用已选字体的标志（字形）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2959"/>
      <source>Show symbol usage...</source>
      <translation>显示标志的用法：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2993"/>
      <source>Mapper colors</source>
      <translation>地圖色彩</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="2999"/>
      <source>Select your color preferences for the map display</source>
      <translation>選擇地圖顯示的顏色偏好</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3005"/>
      <source>Link color</source>
      <translation>連結顏色</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3028"/>
      <source>Background color:</source>
      <translation>背景色:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3085"/>
      <source>Lower level color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3105"/>
      <source>Upper level color:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3125"/>
      <source>Overlapping rooms border:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3647"/>
      <source>Discord Rich Presence</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3653"/>
      <source>Show full game details (if supported)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3719"/>
      <source>Show Mudlet only</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3778"/>
      <source>Disabled</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3686"/>
      <source>Discord username</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3729"/>
      <source>Large icon:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3746"/>
      <location filename="../src/ui/profile_preferences.ui" line="3805"/>
      <source>Show all</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3751"/>
      <location filename="../src/ui/profile_preferences.ui" line="3810"/>
      <source>Hide tooltip</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3756"/>
      <location filename="../src/ui/profile_preferences.ui" line="3815"/>
      <source>Hide tooltip and icon</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3764"/>
      <source>Hide details text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3771"/>
      <source>Hide state text</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3788"/>
      <source>Small icon:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3823"/>
      <source>Hide party info</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4695"/>
      <source>Enable F3 search shortcuts</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4702"/>
      <source>&lt;p&gt;When enabled, text with the blinking attribute (SGR codes 5 and 6) is displayed with a smooth pulsing effect. When disabled, blinking text is shown in italics instead.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4735"/>
      <source>Special Options</source>
      <translation>特殊選項</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4747"/>
      <source>Force compression off</source>
      <translation>强制关闭压缩</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4768"/>
      <source>Force telnet GA signal interpretation off</source>
      <translation>强制关闭telnet的 GA 信号解释</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4761"/>
      <source>Force new line on empty commands</source>
      <translation>强制在空命令上新加一行</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="3676"/>
      <source>Restrict to:</source>
      <translation>限制为:</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4882"/>
      <source>Search Engine</source>
      <translation>搜尋引擎</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4866"/>
      <source>Mudlet updates</source>
      <translation>軟體更新</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4872"/>
      <source>Disable automatic updates</source>
      <translation>禁止自動更新</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4894"/>
      <source>Other Special options</source>
      <translation>其他特殊選項</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4900"/>
      <source>Show icons on menus</source>
      <translation>在選單上顯示圖示</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4021"/>
      <source>Connection</source>
      <translation>連接</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4395"/>
      <source>Connect to the game via proxy</source>
      <translation>通过代理连接到游戏</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4410"/>
      <source>Address</source>
      <translation>地址</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4430"/>
      <source>port</source>
      <translation>連接埠</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4449"/>
      <source>username (optional)</source>
      <translation>帳號（選填）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4468"/>
      <source>password (optional)</source>
      <translation>密碼（選填）</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4992"/>
      <source>Show debug messages for timers not smaller than:</source>
      <translation>为计时器显示调试消息，不小于：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4986"/>
      <source>&lt;p&gt;Show &apos;LUA OK&apos; messages for Timers with the specified minimum interval (h:mm:ss.zzz), the minimum value (the default) shows all such messages but can render the &lt;i&gt;Central Debug Console&lt;/i&gt; useless if there is a very small interval timer running.&lt;/p&gt;</source>
      <comment>The term in &apos;...&apos; refer to a Mudlet specific thing and ought to match the corresponding translation elsewhere.</comment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5042"/>
      <source>Report all Codepoint problems immediately</source>
      <translation>立即报告所有Codepoint问题</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5049"/>
      <source>Additional text wait time:</source>
      <translation>附加文本等待时间：</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5059"/>
      <source>&lt;p&gt;&lt;i&gt;Go-Ahead&lt;/i&gt; (&lt;tt&gt;GA&lt;/tt&gt;) and &lt;i&gt;End-of-record&lt;/i&gt; (&lt;tt&gt;EOR&lt;/tt&gt;) signalling tells Mudlet when the game server is done sending text. On games that do not provide &lt;tt&gt;GA&lt;/tt&gt; or &lt;tt&gt;EOR&lt;/tt&gt;, this option controls how long Mudlet will wait for more text to arrive. Greater values will help reduce the risk that Mudlet will split a large piece of text (with unintended line-breaks in the middle) which can stop some triggers from working. Lesser values increases the risk of text getting broken up, but may make the game feel more responsive.&lt;/p&gt;&lt;p&gt;&lt;i&gt;The default value, which was what Mudlet used before this control was added, is 0.300 Seconds.&lt;/i&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5065"/>
      <source> seconds</source>
      <extracomment>For most locales a space should be included so that the text is separated from the number!</extracomment>
      <translation> 秒</translation>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4581"/>
      <location filename="../src/ui/profile_preferences.ui" line="4587"/>
      <source>Accessibility</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4599"/>
      <source>Announce incoming text in screen reader</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4631"/>
      <source>show them</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4636"/>
      <source>hide them</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4641"/>
      <source>replace with a space</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4620"/>
      <source>When the game sends blank lines:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4649"/>
      <source>Switch between input line and main window using:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4666"/>
      <source>no key</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4671"/>
      <source>Tab</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4676"/>
      <source>Ctrl+Tab</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="4681"/>
      <source>F6</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5039"/>
      <source>When checked, every problem Unicode codepoint is reported in the debug output as it occurs. When unchecked, each distinct codepoint is reported only once and a summary table is shown when the console closes.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5062"/>
      <source>How long Mudlet waits for more text on games that do not send Go-Ahead or End-of-record signals. Larger values reduce the chance that long output is split mid-sentence and breaks triggers; smaller values feel more responsive. Default is 0.300 seconds.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/profile_preferences.ui" line="5164"/>
      <source>Save</source>
      <translation>保存</translation>
    </message>
  </context>
  <context>
    <name>room_exits</name>
    <message>
      <location filename="../src/ui/room_exits.ui" line="38"/>
      <source>General exits:</source>
      <translation>一般退出：</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="91"/>
      <source>NW exit...</source>
      <translation>NW 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="101"/>
      <location filename="../src/ui/room_exits.ui" line="235"/>
      <location filename="../src/ui/room_exits.ui" line="369"/>
      <location filename="../src/ui/room_exits.ui" line="503"/>
      <location filename="../src/ui/room_exits.ui" line="640"/>
      <location filename="../src/ui/room_exits.ui" line="887"/>
      <location filename="../src/ui/room_exits.ui" line="1021"/>
      <location filename="../src/ui/room_exits.ui" line="1173"/>
      <location filename="../src/ui/room_exits.ui" line="1307"/>
      <location filename="../src/ui/room_exits.ui" line="1441"/>
      <location filename="../src/ui/room_exits.ui" line="1575"/>
      <location filename="../src/ui/room_exits.ui" line="1838"/>
      <source>&lt;p&gt;Set to a positive value to override the default (Room) Weight for using this Exit route, zero value assigns the default.&lt;/p&gt;</source>
      <translation>此出口路径时, 需设置一个非负值以覆盖缺省 (房间) 权重，设为 0 时指定为缺省值。</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="225"/>
      <source>N exit...</source>
      <translation>N 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="359"/>
      <source>NE exit...</source>
      <translation>NE 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="493"/>
      <source>Up exit...</source>
      <translation>Up 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="630"/>
      <source>W exit...</source>
      <translation>W 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="735"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="769"/>
      <source>&lt;p&gt;This is the Room ID Number for this room.</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="804"/>
      <source>Weight:</source>
      <translation>权重:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="817"/>
      <source>&lt;p&gt;This is the default weight for this room, which will be used for any exit &lt;i&gt;that leads to &lt;u&gt;this room&lt;/u&gt;&lt;/i&gt; which does not have its own value set.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="880"/>
      <source>E exit...</source>
      <translation>E 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1011"/>
      <source>Down exit...</source>
      <translation>Down 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1157"/>
      <source>SW exit...</source>
      <translation>SW 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1297"/>
      <source>S exit...</source>
      <translation>S 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1431"/>
      <source>SE exit...</source>
      <translation>SE 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1565"/>
      <source>In exit...</source>
      <translation>In 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1828"/>
      <source>Out exit...</source>
      <translation>Out 出口...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1681"/>
      <source>No route</source>
      <translation>没有路线</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1696"/>
      <source>Stub Exit</source>
      <translation>被标记为未知的出口</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="65"/>
      <location filename="../src/ui/room_exits.ui" line="199"/>
      <location filename="../src/ui/room_exits.ui" line="333"/>
      <location filename="../src/ui/room_exits.ui" line="467"/>
      <location filename="../src/ui/room_exits.ui" line="601"/>
      <location filename="../src/ui/room_exits.ui" line="854"/>
      <location filename="../src/ui/room_exits.ui" line="985"/>
      <location filename="../src/ui/room_exits.ui" line="1125"/>
      <location filename="../src/ui/room_exits.ui" line="1271"/>
      <location filename="../src/ui/room_exits.ui" line="1405"/>
      <location filename="../src/ui/room_exits.ui" line="1539"/>
      <location filename="../src/ui/room_exits.ui" line="1802"/>
      <location filename="../src/ui/room_exits.ui" line="1986"/>
      <source>&lt;p&gt;Prevent a route being created via this exit, equivalent to an infinite exit weight.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="53"/>
      <source>Northwest</source>
      <translation>西北</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="81"/>
      <location filename="../src/ui/room_exits.ui" line="215"/>
      <location filename="../src/ui/room_exits.ui" line="349"/>
      <location filename="../src/ui/room_exits.ui" line="483"/>
      <location filename="../src/ui/room_exits.ui" line="617"/>
      <location filename="../src/ui/room_exits.ui" line="870"/>
      <location filename="../src/ui/room_exits.ui" line="1001"/>
      <location filename="../src/ui/room_exits.ui" line="1147"/>
      <location filename="../src/ui/room_exits.ui" line="1287"/>
      <location filename="../src/ui/room_exits.ui" line="1421"/>
      <location filename="../src/ui/room_exits.ui" line="1555"/>
      <location filename="../src/ui/room_exits.ui" line="1818"/>
      <source>&lt;p&gt;Create an exit in this direction with unknown destination, mutually exclusive with an actual exit roomID.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在这个方向上创建一个目的地未知的出口，与实际的出口 roomID 互斥。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="114"/>
      <location filename="../src/ui/room_exits.ui" line="248"/>
      <location filename="../src/ui/room_exits.ui" line="382"/>
      <location filename="../src/ui/room_exits.ui" line="516"/>
      <location filename="../src/ui/room_exits.ui" line="653"/>
      <location filename="../src/ui/room_exits.ui" line="900"/>
      <location filename="../src/ui/room_exits.ui" line="1034"/>
      <location filename="../src/ui/room_exits.ui" line="1186"/>
      <location filename="../src/ui/room_exits.ui" line="1320"/>
      <location filename="../src/ui/room_exits.ui" line="1454"/>
      <location filename="../src/ui/room_exits.ui" line="1588"/>
      <location filename="../src/ui/room_exits.ui" line="1851"/>
      <location filename="../src/ui/room_exits.ui" line="2004"/>
      <source>&lt;p&gt;No door symbol is drawn on 2D Map for this exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="133"/>
      <location filename="../src/ui/room_exits.ui" line="267"/>
      <location filename="../src/ui/room_exits.ui" line="401"/>
      <location filename="../src/ui/room_exits.ui" line="672"/>
      <location filename="../src/ui/room_exits.ui" line="919"/>
      <location filename="../src/ui/room_exits.ui" line="1205"/>
      <location filename="../src/ui/room_exits.ui" line="1339"/>
      <location filename="../src/ui/room_exits.ui" line="1473"/>
      <source>&lt;p&gt;Green (Open) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;绿色(打开)门符号是在二维地图上绘制的，可被标记为未知出口或一个真实房间出口. &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="149"/>
      <location filename="../src/ui/room_exits.ui" line="283"/>
      <location filename="../src/ui/room_exits.ui" line="417"/>
      <location filename="../src/ui/room_exits.ui" line="688"/>
      <location filename="../src/ui/room_exits.ui" line="935"/>
      <location filename="../src/ui/room_exits.ui" line="1221"/>
      <location filename="../src/ui/room_exits.ui" line="1355"/>
      <location filename="../src/ui/room_exits.ui" line="1489"/>
      <source>&lt;p&gt;Orange (Closed) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;橘色(打开)门符号是在二维地图上绘制的，可被标记为未知出口或一个真实房间出口. &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="165"/>
      <location filename="../src/ui/room_exits.ui" line="299"/>
      <location filename="../src/ui/room_exits.ui" line="433"/>
      <location filename="../src/ui/room_exits.ui" line="704"/>
      <location filename="../src/ui/room_exits.ui" line="951"/>
      <location filename="../src/ui/room_exits.ui" line="1237"/>
      <location filename="../src/ui/room_exits.ui" line="1371"/>
      <location filename="../src/ui/room_exits.ui" line="1505"/>
      <source>&lt;p&gt;Red (Locked) door symbol is drawn on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;红色(锁上的)门符号是在二维地图上绘制的，可被标记为未知出口或一个真实房间出口. &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="187"/>
      <source>North</source>
      <translation>北</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="321"/>
      <source>Northeast</source>
      <translation>東北</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="455"/>
      <source>Up</source>
      <translation>上</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="535"/>
      <location filename="../src/ui/room_exits.ui" line="1053"/>
      <location filename="../src/ui/room_exits.ui" line="1607"/>
      <location filename="../src/ui/room_exits.ui" line="1870"/>
      <source>&lt;p&gt;A symbol is drawn with a green (Open) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在2D Map上使用绿色(Open)填充颜色绘制符号，可被标记为未知出口或一个真实房间出口.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="551"/>
      <location filename="../src/ui/room_exits.ui" line="1069"/>
      <location filename="../src/ui/room_exits.ui" line="1623"/>
      <location filename="../src/ui/room_exits.ui" line="1886"/>
      <source>&lt;p&gt;A symbol is drawn with an orange (Closed) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在2D Map上使用橘色(Open)填充颜色绘制符号，可被标记为未知出口或一个真实房间出口.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="567"/>
      <location filename="../src/ui/room_exits.ui" line="1085"/>
      <location filename="../src/ui/room_exits.ui" line="1639"/>
      <location filename="../src/ui/room_exits.ui" line="1902"/>
      <source>&lt;p&gt;A symbol is drawn with a red (Locked) fill color on 2D Map, can be set on either a stub or a real exit.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在2D Map上使用红色(锁住的)填充颜色绘制符号，可被标记为未知出口或一个真实房间出口.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="589"/>
      <source>West</source>
      <translation>西</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="729"/>
      <source>This room</source>
      <translation>這個房間</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="842"/>
      <source>East</source>
      <translation>東</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="973"/>
      <source>Down</source>
      <translation>下</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1107"/>
      <source>Southwest</source>
      <translation>西南</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1259"/>
      <source>South</source>
      <translation>南</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1393"/>
      <source>Southeast</source>
      <translation>東南</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1527"/>
      <source>In</source>
      <translation>入口</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1667"/>
      <source>Key</source>
      <translation>按键</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1708"/>
      <source>Exit RoomID number</source>
      <translation>出口房间 Id</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1735"/>
      <source>No door</source>
      <translation>没有门</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1750"/>
      <source>Open door</source>
      <translation>开启的门</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1762"/>
      <source>Closed door</source>
      <translation>关上的门</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1774"/>
      <source>Locked door</source>
      <translation>被锁住的门</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1790"/>
      <source>Out</source>
      <translation>出口</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1973"/>
      <source>Exit
Status</source>
      <translation>退出
状态</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1977"/>
      <source>&lt;p&gt;Indicates whether the exit is invalid, leads to another room in this area or leads to a room in another area.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2013"/>
      <source>&lt;p&gt;Green (Open) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;绿色 (开放的) 的符号在 2D 图上绘制。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2022"/>
      <source>&lt;p&gt;Orange (Closed) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;橙色（关闭的）的标志被绘制在2D地图上。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2031"/>
      <source>&lt;p&gt;Red (Locked) door symbol is drawn on 2D Map.&lt;/p&gt;</source>
      <translation>&lt;p&gt;红色（锁定）门的符号被绘制在2D地图上。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2108"/>
      <source>&lt;p&gt;Use this button to save any changes, will also remove any invalid Special exits.&lt;/p&gt;</source>
      <translation>&lt;p&gt;使用此按钮来保存更改，同时将除去任何无效的特殊出口。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2124"/>
      <source>&lt;p&gt;Use this button to close the dialogue without changing anything.&lt;/p&gt;</source>
      <translation>&lt;p&gt;使用此按钮来关闭对话,但不更改任何内容。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1968"/>
      <source>&lt;p&gt;Set the number of the room that this exit leads to, if set to zero the exit will be removed on saving the exits.&lt;/p&gt;</source>
      <translation>&lt;p&gt;设置此出口通向的房间ID，如果设置为零，将在保存出口时被移除。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2057"/>
      <source>&lt;p&gt;Add an empty item to Special exits to be edited as required.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2076"/>
      <source>&lt;p&gt;Press this button to deactivate the selection of a Special exit.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2111"/>
      <source>&amp;Save</source>
      <translation>&amp;保存</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1720"/>
      <source>Exit Weight (0=No override)</source>
      <translation>出口权重 (0=无覆盖)</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2127"/>
      <source>&amp;Cancel</source>
      <translation>&amp;取消</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1921"/>
      <source>Special exits:</source>
      <translation>特殊出口:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1930"/>
      <source>&lt;p&gt;Click on an item to edit/change it. To delete a Special Exit, either: select it and press the keyboard Delete key; or set its Exit roomID to less than one; or clear the name/command entry.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1964"/>
      <source>Exit
Room ID</source>
      <translation>出口
房间ID</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1982"/>
      <source>No
Route</source>
      <translation>没有路径</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1991"/>
      <source>Exit
Weight</source>
      <translation>出口权重</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="1995"/>
      <source>&lt;p&gt;Set to a positive integer value to override the default (Room) Weight for using this Exit route, a zero value assigns the default.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2000"/>
      <source>Door
None</source>
      <translation>门
无。</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2009"/>
      <source>Door
Open</source>
      <translation>开启的门</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2018"/>
      <source>Door
Closed</source>
      <translation>门
被关上</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2027"/>
      <source>Door
Locked</source>
      <translation>门
被锁住</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2036"/>
      <source>Command
or LUA script</source>
      <translation>命令或Lua脚本</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2040"/>
      <source>&lt;p&gt;Some mapper scripts may require prefixing the keyword &quot;script:&quot;.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2060"/>
      <source>&amp;Add special exit</source>
      <translation>&amp;添加特殊出口</translation>
    </message>
    <message>
      <location filename="../src/ui/room_exits.ui" line="2079"/>
      <source>&amp;End S. Exits editing</source>
      <translation>&amp;结束 S. 退出编辑</translation>
    </message>
  </context>
  <context>
    <name>room_properties</name>
    <message>
      <location filename="../src/ui/room_properties.ui" line="20"/>
      <source>Room properties</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="47"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="54"/>
      <source>Room name...</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="61"/>
      <source>Icon:</source>
      <translation>图标:</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="111"/>
      <source>Set room color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="127"/>
      <source>Symbol</source>
      <translation>符號</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="167"/>
      <source>Room symbol...</source>
      <translation>房间符号...</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="210"/>
      <source>Color of to use for the room symbol(s)</source>
      <translation>用于房间符号的颜色</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="216"/>
      <source>Set symbol color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="229"/>
      <location filename="../src/ui/room_properties.ui" line="307"/>
      <source>Reset</source>
      <extracomment>This button is located next to the button &quot;Set symbol color&quot; and will reset the symbol color back to the original color.</extracomment>
      <translation>重設</translation>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="242"/>
      <source>Border</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="288"/>
      <source>Color of the room border</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="294"/>
      <source>Set border color</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="332"/>
      <source>Thickness:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="345"/>
      <source>(use global)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="364"/>
      <source>Pathfinding</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/room_properties.ui" line="410"/>
      <source>1 (default)</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>scripts_main_area</name>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="23"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="33"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your script or script group. This will be displayed in the script tree.&lt;/p&gt;&lt;p&gt;If a function within the script is to be used to handle events entered in the list below &lt;b&gt;&lt;u&gt;it must have the same name as is entered here.&lt;/u&gt;&lt;/b&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="61"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="90"/>
      <source>Registered Events:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="137"/>
      <source>&lt;p&gt;Remove (selected) event from list.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="147"/>
      <source>Add User Event:</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="181"/>
      <source>&lt;p&gt;Add entered event name to list.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="140"/>
      <source>-</source>
      <translation>-</translation>
    </message>
    <message>
      <location filename="../src/ui/scripts_main_area.ui" line="184"/>
      <source>+</source>
      <translation>+</translation>
    </message>
  </context>
  <context>
    <name>set_room_area</name>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="14"/>
      <source>Move rooms to another area</source>
      <translation>将房间移至另一个区域</translation>
    </message>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="20"/>
      <source>Which area would you like to move the room(s) to?</source>
      <translation>您想要將房間移動至哪個區域？</translation>
    </message>
    <message>
      <location filename="../src/ui/set_room_area.ui" line="36"/>
      <source>Input new area name to create one.</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>source_editor_area</name>
    <message>
      <location filename="../src/ui/source_editor_area.ui" line="26"/>
      <source>Form</source>
      <translation>表單</translation>
    </message>
  </context>
  <context>
    <name>source_editor_find_area</name>
    <message>
      <location filename="../src/ui/source_editor_find_area.ui" line="41"/>
      <source>Find</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/source_editor_find_area.ui" line="87"/>
      <source>Replace</source>
      <translation type="unfinished"/>
    </message>
  </context>
  <context>
    <name>timers_main_area</name>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="29"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="116"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="186"/>
      <source>&lt;p&gt;milliseconds&lt;/p&gt;</source>
      <translation>&lt;p&gt;毫秒&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="220"/>
      <source>Time:</source>
      <translation>時間：</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="39"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your timer, offset-timer or timer group. This will be displayed in the timer tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;为你的定时器、分支定时器或定时器组选择一个好的名字 (最好是唯一的, 但不是必须唯一). 名字将显示在定时器树上.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="67"/>
      <source>ID:</source>
      <translation>編號：</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="141"/>
      <source>&lt;p&gt;hours&lt;/p&gt;</source>
      <translation>&lt;p&gt;小时&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="156"/>
      <source>&lt;p&gt;minutes&lt;/p&gt;</source>
      <translation>&lt;p&gt;分&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="171"/>
      <source>&lt;p&gt;seconds&lt;/p&gt;</source>
      <translation>&lt;p&gt;秒&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="235"/>
      <source>&lt;p&gt;The &lt;b&gt;hour&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;定时器将停止运行的&lt;b&gt;小时&lt;/b&gt;间隔部分。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="440"/>
      <source>&lt;p&gt;The &lt;b&gt;millisecond&lt;/b&gt; part of the interval that the timer will go off at (1000 milliseconds = 1 second).&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="310"/>
      <source>&lt;p&gt;The &lt;b&gt;minute&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;時計將停止運行&lt;b&gt;分鐘（minute）&lt;/b&gt;間隔。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="126"/>
      <source>&lt;p&gt;Enter one or more commands to use if the given command matches the pattern. (Optional)&lt;/p&gt;&lt;p&gt;This could be another alias or a command to send directly to the game. For complex commands that require modification of variables within this profile, use a Lua script in the editor area below instead. It&apos;s possible to use both this field and a Lua script - the contents of this field will be used before running the script.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="129"/>
      <source>Text to send to the game (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/timers_main_area.ui" line="375"/>
      <source>&lt;p&gt;The &lt;b&gt;second&lt;/b&gt; part of the interval that the timer will go off at.&lt;/p&gt;</source>
      <translation>&lt;p&gt;定时器将停止运行的&lt;b&gt;秒&lt;/b&gt;间隔部分。&lt;/p&gt;</translation>
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
      <location filename="../src/ui/trigger_editor.ui" line="550"/>
      <source>1</source>
      <translation>1</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="436"/>
      <source>Show normally hidden variables</source>
      <translation>顯示隱藏變數</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_editor.ui" line="476"/>
      <source>&lt;p&gt;Enter text here to search through your code.&lt;/p&gt;</source>
      <translation>&lt;p&gt;在此处输入文字以搜索您的代码。&lt;/p&gt;</translation>
    </message>
  </context>
  <context>
    <name>trigger_main_area</name>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="65"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="85"/>
      <source>&lt;p&gt;Use this control to show or hide the extra controls for the trigger; this can be used to allow more space to show the trigger &lt;i&gt;items&lt;/i&gt; on smaller screen.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="111"/>
      <source>Command:</source>
      <translation>指令：</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="155"/>
      <source>ID:</source>
      <translation>编号:</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="449"/>
      <source> lines)</source>
      <extracomment>This text is appended after the numeric value shown in the spin box (so that it and the prefix text is &quot;wrapped&quot; around it), except when the control is set to the special first value when all of them are replaced by that text.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="452"/>
      <source>AND / Multi-line (within: </source>
      <extracomment>This text is prepended before the numeric value shown in the spin box (so that it and the suffix text is &quot;wrapped&quot; around it), except when the control is set to the special first value when all of them are replaced by that text. For locales using spaces between words ensure a space is left at the end to separate the text from the number that is shown from the control after it.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="481"/>
      <source>only pass matches</source>
      <translation>仅传递匹配项</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="254"/>
      <source>&lt;p&gt;Keep firing the script for this many more lines, after the trigger or chain has matched.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="260"/>
      <source>fire length (extra lines)</source>
      <translation>触发长度(额外行数)</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="318"/>
      <source>&lt;p&gt;Play a sound file if the trigger fires.&lt;/p&gt;</source>
      <translation>&lt;p&gt;触发时播放声音文件。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="351"/>
      <source>&lt;p&gt;Use this to open a file selection dialogue to find a sound file to play when the trigger fires.&lt;/p&gt;
&lt;p&gt;&lt;i&gt;Cancelling from the file dialogue will not make any changes; to clear the file use the clear button to the right of the file name display.&lt;/i&gt;&lt;/p&gt;</source>
      <comment>This is the button used to select a sound file to be played when a trigger fires.</comment>
      <extracomment>Please ensure the text used here is duplicated within the tooltip for the QLineEdit that displays the file name selected.</extracomment>
      <translation>&lt;p&gt;使用此选项可打开文件选择对话框，查找触发器触发时要播放的声音文件。&lt;/p&gt;
&lt;p&gt;&lt;i&gt;取消文件对话框不会进行任何更改；要清除文件，请使用文件名显示右侧的清除按钮。&lt;/i&gt;&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="355"/>
      <source>Choose file...</source>
      <translation>選擇檔案⋯</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="380"/>
      <source>no file</source>
      <translation>沒有檔案</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="407"/>
      <source>&lt;p&gt;If set to any value but the first the trigger will only fire if &lt;u&gt;all&lt;/u&gt; conditions on the list have been met within the specified line delta, and captures will be saved in &lt;tt&gt;multimatches&lt;/tt&gt; instead of &lt;tt&gt;matches&lt;/tt&gt;.&lt;/p&gt;&lt;p&gt;If set to the first value the trigger will fire if &lt;u&gt;any&lt;/u&gt; condition on the list have been met.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="413"/>
      <source>handle multiple items as a</source>
      <extracomment>This text preceeds (is above) the content of the spinBox_lineMargin which also contain text with the text in the label_multiLineTrigger suffixed on the end (underneath).</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="440"/>
      <source>&lt;p&gt;&lt;b&gt;Multi-line Trigger Range&lt;/b&gt;&lt;/p&gt;&lt;p&gt;Specifies within how many consecutive lines all trigger patterns must match.&lt;/p&gt;&lt;p&gt;&lt;b&gt;Example:&lt;/b&gt; If set to 3 and pattern 1 matches on line 100, pattern 2 must match somewhere between lines 100-103 for the trigger to fire.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="446"/>
      <source>OR / Multi-item</source>
      <extracomment>This text represents what is shown in the spinBox_lineMargin control when it is at it minimum value and replaces the normal value and the normal prefix and suffix that would otherwise surround it before this or those elements are inserted in the middle of the groupBox_multiLineTrigger and the label_multiLineTrigger text.</extracomment>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="465"/>
      <source>trigger</source>
      <extracomment>This text follows (is beneath) the content of the spinBox_lineMargin which also contain text with the groupBox_multiLineTrigger prefixed at the beginning (above).</extracomment>
      <translation>觸發</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="478"/>
      <source>&lt;p&gt;Do not pass whole line to children.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="495"/>
      <source>&lt;p&gt;Enable this to highlight the matching text by changing the fore and background colors to the ones selected here.&lt;/p&gt;</source>
      <translation>&lt;p&gt;启用此功能，通过将前景色和背景色更改为此处选择的颜色来突出显示匹配的文本。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="551"/>
      <source>Background</source>
      <translation>背景</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="544"/>
      <location filename="../src/ui/triggers_main_area.ui" line="567"/>
      <source>keep</source>
      <comment>Keep the existing colour on matches to highlight. Use shortest word possible so it fits on the button</comment>
      <translation>保持</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="528"/>
      <source>Foreground</source>
      <translation>前景</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="321"/>
      <source>play sound</source>
      <translation>播放聲音</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="78"/>
      <source>&lt;p&gt;Choose a good, (ideally, though it need not be, unique) name for your trigger or trigger group. This will be displayed in the trigger tree.&lt;/p&gt;</source>
      <translation>&lt;p&gt;为你的触发器或触发器组选择一个好的名字 (最好是唯一的, 但不是必须唯一). 名字将显示在触发器树上.&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="124"/>
      <source>&lt;p&gt;Enter one or more commands to use if the given command matches the pattern. (Optional)&lt;/p&gt;&lt;p&gt;This could be another alias or a command to send directly to the game. For complex commands that require modification of variables within this profile, use a Lua script in the editor area below instead. It&apos;s possible to use both this field and a Lua script - the contents of this field will be used before running the script.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="127"/>
      <source>Text to send to the game (optional)</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="211"/>
      <source>&lt;p&gt;Match all occurrences of the pattern in the line.&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="488"/>
      <source>match all</source>
      <translation>全部符合</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="302"/>
      <source>&lt;p&gt;How many more lines, after the one that fired the trigger, should be passed to the trigger&apos;s children?&lt;/p&gt;</source>
      <translation>&lt;p&gt;在触发的那一行之后，还有多少行应该被传递给触发器的子句？&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="371"/>
      <source>&lt;p&gt;Sound file to play when the trigger fires.&lt;/p&gt;</source>
      <comment>This is the tooltip for the QLineEdit that shows - but does not permit changing - the sound file used for a trigger.</comment>
      <translation>&lt;p&gt;触发触发器时要播放的声音文件. &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="393"/>
      <source>&lt;p&gt;Click to remove the sound file set for this trigger.&lt;/p&gt;</source>
      <translation>&lt;p&gt;单击以删除为该触发器设置的声音文件. &lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/triggers_main_area.ui" line="501"/>
      <source>highlight</source>
      <translation>重點提示</translation>
    </message>
  </context>
  <context>
    <name>trigger_pattern_edit</name>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="113"/>
      <source>Foreground color ignored</source>
      <translation>忽略的前景色</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="132"/>
      <source>Background color ignored</source>
      <translation>忽略的背景色</translation>
    </message>
    <message>
      <location filename="../src/ui/trigger_pattern_edit.ui" line="154"/>
      <source>match on the prompt line</source>
      <translation>在提示行匹配</translation>
    </message>
  </context>
  <context>
    <name>vars_main_area</name>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="62"/>
      <source>Name:</source>
      <translation>名稱：</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="94"/>
      <source>⏴ Key type:</source>
      <translation>⏴ 键类型:</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="81"/>
      <source>&lt;p&gt;Set the &lt;i&gt;global variable&lt;/i&gt; or the &lt;i&gt;table entry&lt;/i&gt; name here. The name has to start with a letter, but can contain a mix of letters and numbers.&lt;/p&gt;</source>
      <translation>在此处&lt;p&gt;设置&lt;i&gt;全局变量&lt;/i&gt;或&lt;i&gt;表项&lt;/i&gt;名。名字必须以字母开头，但可以混合字母和数字。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="114"/>
      <location filename="../src/ui/vars_main_area.ui" line="171"/>
      <source>Auto-Type</source>
      <translation>自動類型</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="84"/>
      <source>Variable name ...</source>
      <translation>變數名稱 ...</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="107"/>
      <source>&lt;p&gt;Tables can store values either in a list, and/or a hashmap.&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;list&lt;/b&gt;, &lt;i&gt;unique indexed keys&lt;/i&gt; represent values - so you can have values at &lt;i&gt;1, 2, 3...&lt;/i&gt;&lt;/p&gt;&lt;p&gt;In a &lt;b&gt;map&lt;/b&gt; {a.k.a. an &lt;i&gt;associative array}&lt;/i&gt;, &lt;i&gt;unique keys&lt;/i&gt; represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).&lt;/p&gt;&lt;p&gt;This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.&lt;/p&gt;&lt;p&gt;In other cases it displays other entities (&lt;span style=&quot; font-style:italic;&quot;&gt;functions&lt;/span&gt;) which cannot be modified from here.&lt;/p&gt;&lt;p&gt;&lt;br/&gt;&lt;/p&gt;</source>
      <translation type="unfinished"/>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="119"/>
      <source>key (string)</source>
      <translation>按键(string)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="124"/>
      <source>index (integer)</source>
      <translation>索引(整数)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="129"/>
      <location filename="../src/ui/vars_main_area.ui" line="191"/>
      <source>table</source>
      <translation>表</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="134"/>
      <source>function
(cannot create
from GUI)</source>
      <translation>函数
(无法从GUI创建)</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="144"/>
      <source>&lt;p&gt;If checked this item (and its children, if applicable) does not show in area to the left unless &lt;b&gt;Show normally hidden variables&lt;/b&gt; is checked.&lt;/p&gt;</source>
      <translation>&lt;p&gt;如果勾选了此项（以及它的子项，如果可用的话），将不会把此处显示到左边，除非勾选了&lt;b&gt;显示正常隐藏的变量&lt;/b&gt;。&lt;/p&gt;</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="147"/>
      <source>hidden variable</source>
      <translation>隱藏變數</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="154"/>
      <source>⏷ Value type:</source>
      <translation>⏷ 數值類型：</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="176"/>
      <source>string</source>
      <translation>字串</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="181"/>
      <source>number</source>
      <translation>數字</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="186"/>
      <source>boolean</source>
      <translation>布林值</translation>
    </message>
    <message>
      <location filename="../src/ui/vars_main_area.ui" line="196"/>
      <source>function</source>
      <translation>函数</translation>
    </message>
  </context>
</TS>
