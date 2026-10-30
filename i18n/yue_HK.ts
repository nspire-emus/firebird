<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="yue_HK">
<context>
    <name>ConfigPageDebug</name>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="11"/>
        <source>Remote GDB debugging</source>
        <translation>遠端 GDB 除錯</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="20"/>
        <source>If enabled, a remote GDB debugger can be connected to the port and be used for debugging.</source>
        <translation>啟用後，可以將遠端 GDB 除錯器連接到呢個連接埠進行除錯。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="33"/>
        <source>Enable GDB stub on Port</source>
        <translatorcomment>是未完成？这个功能貌似确实无法使用，但还是这么翻好了。</translatorcomment>
        <translation>在連接埠啟用 GDB</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="59"/>
        <source>Remote access to internal debugger</source>
        <translation>遠端存取內部除錯器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="69"/>
        <source>Enable this to access the internal debugger via TCP (telnet/netcat), like for firebird-send.</source>
        <translation>啟用呢個選項，就可以透過 TCP（telnet/netcat）存取內部除錯器，例如供 firebird-send 使用。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="81"/>
        <source>Enable internal debugger on Port</source>
        <translation>在連接埠啟用內部除錯器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="106"/>
        <source>Enter into Debugger</source>
        <translation>進入除錯器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="115"/>
        <source>Configure which situations cause the emulator to trap into the debugger.</source>
        <translation>設定模擬器喺邊啲情況下進入除錯器。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="121"/>
        <source>Enter Debugger on Startup</source>
        <translation>啟動時進入除錯器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="132"/>
        <source>Enter Debugger on Warnings and Errors</source>
        <translation>出現警告或錯誤時進入除錯器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="143"/>
        <source>Print a message on Warnings</source>
        <translation>出現警告時輸出訊息</translation>
    </message>
</context>
<context>
    <name>ConfigPageEmulation</name>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="11"/>
        <source>Startup</source>
        <translation>啟動</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="23"/>
        <source>When opening Firebird, the selected Kit will be started. If available, it will resume the emulation from the provided snapshot.</source>
        <translation>開啟 Firebird 時會啟動所選嘅 Kit；如果有可用快照，就會由快照恢復運行。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="25"/>
        <source>Choose the Kit selected on startup and after restarting. If the checkbox is active, it will be launched when Firebird starts.</source>
        <translation>設定啟動同重新啟動後預設選用嘅 Kit。剔選呢項後，Firebird 啟動時就會自動啟動呢個 Kit。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="39"/>
        <source>On Startup, run Kit</source>
        <translation>啟動時運行 Kit</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="59"/>
        <source>Shutdown</source>
        <translation>結束</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="71"/>
        <source>When closing firebird using the back button, save the current state to the current snapshot. Does not work when firebird is in the background.</source>
        <translation>用「返回」按鈕結束 Firebird 時，會將目前狀態儲存到目前快照。Firebird 喺背景運行時，呢個設定唔會生效。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="73"/>
        <source>On Application end, save the current state to the current snapshot.</source>
        <translation>結束程式時，會將目前狀態儲存到目前快照。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="80"/>
        <source>Save snapshot on shutdown</source>
        <translation>結束時儲存快照</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="91"/>
        <source>UI Preferences</source>
        <translation>介面設定</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="101"/>
        <source>Change the side of the keypad in landscape orientation.</source>
        <translation>調整橫向顯示時鍵盤喺左邊定右邊。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="107"/>
        <source>Left-handed mode</source>
        <translation>左手模式</translation>
    </message>
</context>
<context>
    <name>ConfigPageFileTransfer</name>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="12"/>
        <source>File Transfer</source>
        <translation>檔案傳輸</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="21"/>
        <source>If you are unable to use the main window&apos;s file transfer using either drag&apos;n&apos;drop or the file explorer, you can send files here.</source>
        <translation>如果喺主視窗拖放檔案或者用檔案瀏覽器都傳送唔到檔案，可以喺呢度傳送。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="29"/>
        <source>Here you can send files into the target folder specified below.</source>
        <translation>你可以喺呢度將檔案傳送到下方指定嘅目標資料夾。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="37"/>
        <source>TNS Documents</source>
        <translation>TNS 文件</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="37"/>
        <source>Operating Systems</source>
        <translation>操作系統</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="40"/>
        <source>Starting</source>
        <translation>正開始傳送</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="52"/>
        <source>Send files</source>
        <translation>傳送檔案</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="65"/>
        <source>Leave Press-to-Test mode</source>
        <translation>離開測驗模式</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="79"/>
        <source>Status:</source>
        <translation>狀態：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="85"/>
        <source>Idle</source>
        <translation>閒置</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="92"/>
        <source>Failed!</source>
        <translation>失敗！</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="96"/>
        <source>Done!</source>
        <translation>完成！</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="112"/>
        <source>Target Directory</source>
        <translation>目標資料夾</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="121"/>
        <source>When dragging files onto Firebird, it will try to send them to the emulated system.</source>
        <translatorcomment>模拟系统听起来有点怪，译作客户机可能更好，但为忠实原文还是不做修改。</translatorcomment>
        <translation>將檔案拖放到 Firebird 上，Firebird 就會嘗試將檔案傳送到模擬嘅計數機。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="130"/>
        <source>Target folder for dropped files:</source>
        <translation>拖放檔案嘅目標資料夾：</translation>
    </message>
</context>
<context>
    <name>ConfigPageKits</name>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="30"/>
        <source>Kit Properties</source>
        <translation>Kit 屬性</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="42"/>
        <source>You need to specify files for Boot1 and Flash</source>
        <translation>你需要為 Boot1 同 Flash 指定檔案</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="46"/>
        <source>Name:</source>
        <translation>名稱：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="52"/>
        <source>Name</source>
        <translation>名稱</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="64"/>
        <source>Boot1:</source>
        <translation>Boot1：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="80"/>
        <source>Flash:</source>
        <translation>Flash：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="105"/>
        <source>Snapshot:</source>
        <translation>快照：</translation>
    </message>
</context>
<context>
    <name>ConfigPagesModel</name>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="5"/>
        <source>Flash &amp; Boot1</source>
        <translation>Flash &amp; Boot1</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="10"/>
        <source>Emulation</source>
        <translation>模擬</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="15"/>
        <source>File Transfer</source>
        <translation>檔案傳輸</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="20"/>
        <source>Debugging</source>
        <translation>除錯</translation>
    </message>
</context>
<context>
    <name>FBAboutDialog</name>
    <message>
        <location filename="../fbaboutdialog.cpp" line="66"/>
        <source>About Firebird</source>
        <translation>關於 Firebird</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="67"/>
        <source>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;On GitHub&lt;/a&gt;</source>
        <translation>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;GitHub 專案主頁&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="70"/>
        <source>Authors:&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;Based on nspire_emu v0.70 by Goplat&lt;br&gt;&lt;br&gt;This work is licensed under the GPLv3.&lt;br&gt;To view a copy of this license, visit &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</source>
        <translation>作者：&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;以 Goplat 開發嘅 nspire_emu v0.70 為基礎&lt;br&gt;&lt;br&gt;呢個作品採用 GPLv3 授權。&lt;br&gt;如果想睇授權條款全文，請瀏覽 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="80"/>
        <source>Checking for update</source>
        <translation>正檢查更新……</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="82"/>
        <source>Ok</source>
        <translation>確定</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="83"/>
        <source>Check for Update</source>
        <translation>檢查更新</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="96"/>
        <source>No updates for -dev builds available.</source>
        <translation>開發版（-dev）暫時唔支援檢查更新。</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="101"/>
        <source>Checking for updates...</source>
        <translation>正檢查更新……</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="121"/>
        <source>Checking failed (%1)</source>
        <translation>檢查失敗（%1）</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="131"/>
        <source>No newer version available.</source>
        <translation>已經係最新版本。</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="136"/>
        <source>&lt;b&gt;Newer version (%1) available &lt;a href=&apos;%2&apos;&gt;on GitHub&lt;/a&gt;!&lt;/b&gt;</source>
        <translation>&lt;b&gt;有新版本（%1），可以到 &lt;a href=&apos;%2&apos;&gt;GitHub&lt;/a&gt; 查看！&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="140"/>
        <source>Checking failed (invalid tag name)</source>
        <translation>檢查失敗（版本標籤名稱無效）</translation>
    </message>
</context>
<context>
    <name>FBConfigDialog</name>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="9"/>
        <source>Firebird Emu Configuration</source>
        <translation>Firebird 模擬器設定</translation>
    </message>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="46"/>
        <source>Changes are saved automatically</source>
        <translation>更改會自動儲存</translation>
    </message>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="52"/>
        <source>Ok</source>
        <translation>確定</translation>
    </message>
</context>
<context>
    <name>FileSelect</name>
    <message>
        <location filename="../qml/Firebird/UIComponents/FileSelect.qml" line="48"/>
        <source>(none)</source>
        <translation>未選擇</translation>
    </message>
</context>
<context>
    <name>FlashDialog</name>
    <message>
        <location filename="../qml/FlashDialog.qml" line="10"/>
        <source>Create Flash Image</source>
        <translation>建立 Flash 映像</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="33"/>
        <source>Model:</source>
        <translation>型號：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="49"/>
        <source>CX Subtype:</source>
        <translation>CX 子型號：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="62"/>
        <source>Manuf:</source>
        <translation>Manuf：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="76"/>
        <source>Boot2:</source>
        <translation>Boot2：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="89"/>
        <source>OS:</source>
        <translation>OS：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="104"/>
        <source>Bootloader:</source>
        <translation>Bootloader：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="117"/>
        <source>Installer:</source>
        <translation>Installer：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="130"/>
        <source>Diags:</source>
        <translation>Diags：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="139"/>
        <source>Manuf required for CX II</source>
        <translation>CX II 型號需要 Manuf 檔案</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="151"/>
        <source>Flash saving failed</source>
        <translation>儲存 Flash 檔案失敗</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="152"/>
        <source>Saving the flash file failed!</source>
        <translation>儲存唔到 Flash 檔案！</translation>
    </message>
</context>
<context>
    <name>KitList</name>
    <message>
        <location filename="../qml/Firebird/UIComponents/KitList.qml" line="83"/>
        <source>Remove</source>
        <translation>移除</translation>
    </message>
    <message>
        <location filename="../qml/Firebird/UIComponents/KitList.qml" line="100"/>
        <source>Copy</source>
        <translation>複製</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="17"/>
        <location filename="../mainwindow.cpp" line="862"/>
        <source>Firebird Emu</source>
        <translation>Firebird 模擬器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="80"/>
        <source>Reset</source>
        <translation>重設</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="100"/>
        <source>Pause</source>
        <translation>暫停</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="123"/>
        <source>Screenshot</source>
        <translation>截圖</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="143"/>
        <source>Connect</source>
        <translation>連接</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="167"/>
        <source>Enable Fullspeed</source>
        <translation>啟用全速模式</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="170"/>
        <source>Speed: 0 %</source>
        <translation>速度：0 %</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="217"/>
        <location filename="../mainwindow.cpp" line="579"/>
        <source>Debugger</source>
        <translation>除錯器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="261"/>
        <location filename="../mainwindow.cpp" line="575"/>
        <source>File Transfer</source>
        <translation>檔案傳輸</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="274"/>
        <source>Filename</source>
        <translation>檔名</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="279"/>
        <source>Size</source>
        <translation>大小</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="320"/>
        <location filename="../mainwindow.cpp" line="577"/>
        <source>Serial Monitor</source>
        <translation>串口監視器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="344"/>
        <location filename="../mainwindow.cpp" line="573"/>
        <source>Keypad</source>
        <translation>鍵盤</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="398"/>
        <source>&amp;Emulation</source>
        <translation>模擬(&amp;E)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="402"/>
        <source>Restart &amp;with Kit</source>
        <translation>使用 Kit 重新啟動(&amp;W)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="411"/>
        <source>&amp;Boot Diags with Kit</source>
        <translation>使用 Kit 啟動 Diags(&amp;B)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="431"/>
        <source>&amp;Flash</source>
        <translation>Flash(&amp;F)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="438"/>
        <source>&amp;Tools</source>
        <translation>工具(&amp;T)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="442"/>
        <source>Language</source>
        <translation>語言</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="461"/>
        <source>S&amp;napshot</source>
        <translation>快照(&amp;N)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="471"/>
        <source>Abo&amp;ut</source>
        <translation>關於(&amp;U)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="488"/>
        <source>&amp;Reset</source>
        <translation>重設(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="497"/>
        <source>Enter &amp;Debugger</source>
        <translation>進入除錯器(&amp;D)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="506"/>
        <source>&amp;Quit</source>
        <translation>結束(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="518"/>
        <source>&amp;Pause</source>
        <translation>暫停(&amp;P)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="521"/>
        <source>Pause execution</source>
        <translation>暫停運行</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="530"/>
        <location filename="../mainwindow.cpp" line="492"/>
        <source>Re&amp;start</source>
        <translation>重新啟動(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="539"/>
        <source>&amp;Screenshot</source>
        <translation>截圖(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="552"/>
        <source>Connect &amp;USB</source>
        <translation>連接 USB(&amp;U)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="561"/>
        <source>&amp;Save</source>
        <translation>儲存(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="570"/>
        <source>&amp;Create Flash</source>
        <translation>建立 Flash(&amp;C)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="575"/>
        <source>Send &amp;file over XModem</source>
        <translation>透過 XModem 傳送檔案(&amp;F)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="584"/>
        <source>&amp;Suspend</source>
        <translation>儲存快照(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="593"/>
        <source>&amp;Resume</source>
        <translation>由快照恢復(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="602"/>
        <source>Save &amp;to file</source>
        <translation>儲存到檔案(&amp;T)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="611"/>
        <source>Load &amp;from file</source>
        <translation>由檔案載入(&amp;F)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="623"/>
        <source>&amp;Record GIF</source>
        <translation>錄製 GIF(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="632"/>
        <source>&amp;About Firebird</source>
        <translation>關於 Firebird(&amp;A)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="637"/>
        <source>About &amp;Qt</source>
        <translation>關於 Qt(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="645"/>
        <source>&amp;External LCD</source>
        <translation>獨立屏幕視窗(&amp;E)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="654"/>
        <source>&amp;Configuration</source>
        <translation>設定(&amp;C)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="663"/>
        <source>Switch to Mobile UI</source>
        <translation>切換到流動版介面</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="668"/>
        <source>Leave &amp;PTT</source>
        <translation>離開測驗模式(&amp;P)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="209"/>
        <location filename="../mainwindow.cpp" line="228"/>
        <source>Start the emulation via Emulation-&gt;Start.</source>
        <translation>請用「模擬→啟動」啟動模擬。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="215"/>
        <source>Default Kit not found</source>
        <translation>搵唔到預設 Kit</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="266"/>
        <source>Language change</source>
        <translation>切換語言</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="266"/>
        <source>No translation available for this language :(</source>
        <translation>呢種語言暫時未有翻譯 :(</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="380"/>
        <source>Download failed</source>
        <translation>下載失敗</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="380"/>
        <source>Could not download file.</source>
        <translation>下載唔到檔案。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="476"/>
        <location filename="../mainwindow.cpp" line="757"/>
        <source>Could not resume</source>
        <translation>無法由快照恢復</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="476"/>
        <source>Try to restart this app.</source>
        <translation>請試吓重新啟動呢個程式。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="492"/>
        <source>&amp;Start</source>
        <translation>啟動(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="521"/>
        <source>Docks</source>
        <translation>面板</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="524"/>
        <source>Enable UI edit mode</source>
        <translation>調整面板佈局</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="585"/>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="592"/>
        <source>Save Screenshot</source>
        <translation>儲存截圖</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="592"/>
        <source>PNG images (*.png)</source>
        <translation>PNG 圖像 (*.png)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="597"/>
        <source>Screenshot failed</source>
        <translation>截圖失敗</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="597"/>
        <source>Failed to save screenshot!</source>
        <translation>儲存唔到截圖！</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="615"/>
        <source>Save Recording</source>
        <translation>儲存 GIF 動畫</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="615"/>
        <source>GIF images (*.gif)</source>
        <translation>GIF 圖像 (*.gif)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="625"/>
        <source>Failed recording GIF</source>
        <translation>錄製 GIF 失敗</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="625"/>
        <source>A failure occured during recording</source>
        <translation>錄製期間出咗錯</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="645"/>
        <location filename="../mainwindow.cpp" line="647"/>
        <source>Disconnect USB</source>
        <translation>中斷 USB 連接</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="645"/>
        <location filename="../mainwindow.cpp" line="647"/>
        <source>Connect USB</source>
        <translation>連接 USB</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="674"/>
        <source>Can&apos;t resume</source>
        <translation>無法由快照恢復</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="674"/>
        <location filename="../mainwindow.cpp" line="685"/>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>目前嘅 Kit 未設定快照檔案</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="685"/>
        <source>Can&apos;t suspend</source>
        <translation>無法儲存快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="690"/>
        <source>Select snapshot to resume from</source>
        <translation>選擇用嚟恢復模擬嘅快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="697"/>
        <source>Select snapshot to suspend to</source>
        <translation>選擇快照儲存位置</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="745"/>
        <source>Emulation started</source>
        <translation>模擬已啟動</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="747"/>
        <source>Could not start the emulation</source>
        <translation>無法啟動模擬</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="747"/>
        <source>Starting the emulation failed.
Are the paths to boot1 and flash correct?</source>
        <translation>啟動模擬失敗。
請檢查 Boot1 同 Flash 嘅路徑啱唔啱。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="755"/>
        <source>Emulation resumed from snapshot</source>
        <translation>已由快照恢復模擬</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="757"/>
        <source>Resuming failed.
Try to fix the issue and try again.</source>
        <translation>由快照恢復失敗。
請解決問題後再試。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="763"/>
        <source>Snapshot saved</source>
        <translation>快照已儲存</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="765"/>
        <source>Could not suspend</source>
        <translation>無法儲存快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="765"/>
        <source>Suspending failed.
Try to fix the issue and try again.</source>
        <translation>儲存快照失敗。
請解決問題後再試。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="779"/>
        <source>Emulation stopped</source>
        <translation>模擬已停止</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="859"/>
        <source>Firebird Emu - %1</source>
        <translation>Firebird 模擬器 - %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="884"/>
        <source>No boot1 set</source>
        <translation>未設定 Boot1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="884"/>
        <source>Before you can start the emulation, you have to select a proper boot1 file.</source>
        <translation>啟動模擬之前，你需要選擇合適嘅 Boot1 檔案。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="890"/>
        <source>No flash image loaded</source>
        <translation>未載入 Flash 映像</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="890"/>
        <source>Before you can start the emulation, you have to load a proper flash file.
You can create one via Flash-&gt;Create Flash in the menu.</source>
        <translation>啟動模擬之前，你需要載入合適嘅 Flash 檔案。
你可以用選單入面嘅「Flash→建立 Flash」建立呢個檔案。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="898"/>
        <source>Restart needed</source>
        <translation>需要重新啟動</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="898"/>
        <source>Failed to restart emulator. Close and reopen this app.
</source>
        <translation>模擬器重新啟動失敗。請關閉程式再開啟。
</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="944"/>
        <source>Select file to send</source>
        <translation>選擇要傳送嘅檔案</translation>
    </message>
</context>
<context>
    <name>MobileUI</name>
    <message>
        <location filename="../qml/MobileUI.qml" line="65"/>
        <source>Suspend failed</source>
        <translation>儲存快照失敗</translation>
    </message>
    <message>
        <location filename="../qml/MobileUI.qml" line="66"/>
        <source>Suspending the emulation failed. Do you still want to quit Firebird?</source>
        <translation>儲存快照失敗。你係咪仍然要結束 Firebird？</translation>
    </message>
</context>
<context>
    <name>MobileUIConfig</name>
    <message>
        <location filename="../qml/MobileUIConfig.qml" line="66"/>
        <source>Changes are saved automatically</source>
        <translation>更改會自動儲存</translation>
    </message>
</context>
<context>
    <name>MobileUIDrawer</name>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="42"/>
        <source>Start</source>
        <translation>啟動</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="57"/>
        <source>Reset</source>
        <translation>重設</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="69"/>
        <source>Resume</source>
        <translation>由快照恢復</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="84"/>
        <source>Save</source>
        <translation>儲存</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="89"/>
        <source>Error</source>
        <translation>錯誤</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="90"/>
        <source>Failed to save changes!</source>
        <translation>儲存唔到更改！</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="96"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="97"/>
        <source>Flash saved, but no snapshot location configured.
You won&apos;t be able to resume.</source>
        <translation>Flash 已儲存，但未設定快照儲存位置。
之後就唔可以由快照恢復模擬。</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="127"/>
        <source>Desktop UI</source>
        <translation>桌面版介面</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="136"/>
        <source>Configuration</source>
        <translation>設定</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="156"/>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="175"/>
        <source>About Firebird</source>
        <translation>關於 Firebird</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="176"/>
        <source>Authors:&lt;br&gt;
                               Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;
                               Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;
                               Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;
                               Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;
                               Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;
                               Based on nspire_emu v0.70 by Goplat&lt;br&gt;&lt;br&gt;
                               This work is licensed under the GPLv3.&lt;br&gt;
                               To view a copy of this license, visit &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</source>
        <translation>作者：&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;以 Goplat 開發嘅 nspire_emu v0.70 為基礎&lt;br&gt;&lt;br&gt;呢個作品採用 GPLv3 授權。&lt;br&gt;如果想睇授權條款全文，請瀏覽 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
</context>
<context>
    <name>QMLBridge</name>
    <message>
        <location filename="../qmlbridge.cpp" line="52"/>
        <source>Default</source>
        <translation>預設</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="305"/>
        <source>None</source>
        <translation>未選擇</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="383"/>
        <location filename="../qmlbridge.cpp" line="401"/>
        <location filename="../qmlbridge.cpp" line="416"/>
        <source>Open failed</source>
        <translation>開啟失敗</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="392"/>
        <source>Found %1 instead</source>
        <translation>實際辨識為 %1</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="525"/>
        <source>Could not stop emulation</source>
        <translation>無法停止模擬</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="533"/>
        <source>Starting emulation</source>
        <translation>正啟動模擬</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="537"/>
        <source>No boot1 or flash selected.
Swipe keypad left for configuration.</source>
        <translation>未選擇 Boot1 或 Flash。
請向左掃鍵盤開啟設定。</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="554"/>
        <source>Suspending emulation</source>
        <translation>正儲存快照</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="559"/>
        <location filename="../qmlbridge.cpp" line="575"/>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>目前嘅 Kit 未設定快照檔案</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="566"/>
        <source>Resuming emulation</source>
        <translation>正由快照恢復模擬</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="666"/>
        <source>Emulation started</source>
        <translation>模擬已啟動</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="668"/>
        <source>Couldn&apos;t start emulation</source>
        <translation>無法啟動模擬</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="674"/>
        <source>Emulation resumed</source>
        <translation>已由快照恢復模擬</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="676"/>
        <source>Could not resume</source>
        <translation>無法由快照恢復</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="682"/>
        <source>Flash and snapshot saved</source>
        <translation>Flash 同快照已儲存</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="684"/>
        <source>Couldn&apos;t save snapshot</source>
        <translation>無法儲存快照</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../qtframebuffer.cpp" line="60"/>
        <source>LCD turned off</source>
        <translation>屏幕已關閉</translation>
    </message>
    <message>
        <location filename="../qtframebuffer.cpp" line="74"/>
        <source>In debugger</source>
        <translation>除錯中</translation>
    </message>
</context>
<context>
    <name>USBLinkTreeWidget</name>
    <message>
        <location filename="../usblinktreewidget.cpp" line="82"/>
        <source>Delete</source>
        <translation>刪除</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="91"/>
        <source>New folder</source>
        <translation>新增資料夾</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="109"/>
        <source>Download</source>
        <translation>下載</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="129"/>
        <source>Too much</source>
        <translation>檔案太大</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="299"/>
        <source>Chose save location</source>
        <translation>選擇儲存位置</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="299"/>
        <source>TNS file (*.tns)</source>
        <translation>TNS 檔案 (*.tns)</translation>
    </message>
</context>
<context>
    <name>VerticalSwipeBar</name>
    <message>
        <location filename="../qml/Firebird/UIComponents/VerticalSwipeBar.qml" line="15"/>
        <source>Swipe here</source>
        <translation>喺呢度掃動</translation>
    </message>
</context>
</TS>
