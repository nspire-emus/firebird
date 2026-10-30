<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>ConfigPageDebug</name>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="11"/>
        <source>Remote GDB debugging</source>
        <translation>远程 GDB 调试</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="20"/>
        <source>If enabled, a remote GDB debugger can be connected to the port and be used for debugging.</source>
        <translation>启用后，可将远程 GDB 调试器连接到此端口进行调试。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="33"/>
        <source>Enable GDB stub on Port</source>
        <translatorcomment>是未完成？这个功能貌似确实无法使用，但还是这么翻好了。</translatorcomment>
        <translation>在端口上启用 GDB</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="59"/>
        <source>Remote access to internal debugger</source>
        <translation>远程访问内部调试器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="69"/>
        <source>Enable this to access the internal debugger via TCP (telnet/netcat), like for firebird-send.</source>
        <translation>启用此选项可通过 TCP（telnet/netcat）访问内部调试器，例如 firebird-send。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="81"/>
        <source>Enable internal debugger on Port</source>
        <translation>在端口上启用内部调试器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="106"/>
        <source>Enter into Debugger</source>
        <translation>进入调试器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="115"/>
        <source>Configure which situations cause the emulator to trap into the debugger.</source>
        <translation>设置进入调试器的条件。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="121"/>
        <source>Enter Debugger on Startup</source>
        <translation>在启动时进入调试器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="132"/>
        <source>Enter Debugger on Warnings and Errors</source>
        <translation>出现警告或错误时进入调试器</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageDebug.qml" line="143"/>
        <source>Print a message on Warnings</source>
        <translation>出现警告时输出消息</translation>
    </message>
</context>
<context>
    <name>ConfigPageEmulation</name>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="11"/>
        <source>Startup</source>
        <translation>启动</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="23"/>
        <source>When opening Firebird, the selected Kit will be started. If available, it will resume the emulation from the provided snapshot.</source>
        <translation>打开 Firebird 时启动所选 Kit；如果有可用快照，则从快照恢复运行。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="25"/>
        <source>Choose the Kit selected on startup and after restarting. If the checkbox is active, it will be launched when Firebird starts.</source>
        <translation>设置启动及重启后默认选用的 Kit。勾选此项后，Firebird 启动时将自动启动该 Kit。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="39"/>
        <source>On Startup, run Kit</source>
        <translation>启动时运行 Kit</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="59"/>
        <source>Shutdown</source>
        <translation>退出</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="71"/>
        <source>When closing firebird using the back button, save the current state to the current snapshot. Does not work when firebird is in the background.</source>
        <translation>通过“返回”按钮退出 Firebird 时，将当前状态保存到当前快照。Firebird 在后台运行时，此设置无效。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="73"/>
        <source>On Application end, save the current state to the current snapshot.</source>
        <translation>退出应用时，将当前状态保存至当前快照。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="80"/>
        <source>Save snapshot on shutdown</source>
        <translation>退出时保存快照</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="91"/>
        <source>UI Preferences</source>
        <translation>界面设置</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageEmulation.qml" line="101"/>
        <source>Change the side of the keypad in landscape orientation.</source>
        <translation>调整横屏时键盘的左右位置。</translation>
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
        <translation>文件传输</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="21"/>
        <source>If you are unable to use the main window&apos;s file transfer using either drag&apos;n&apos;drop or the file explorer, you can send files here.</source>
        <translation>如果无法通过主窗口拖放文件或使用文件浏览器传输文件，可以在此处发送。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="29"/>
        <source>Here you can send files into the target folder specified below.</source>
        <translation>文件将发送到下方指定的目标文件夹。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="37"/>
        <source>TNS Documents</source>
        <translation>TNS 文档</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="37"/>
        <source>Operating Systems</source>
        <translation>操作系统</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="40"/>
        <source>Starting</source>
        <translation>正开始传输</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="52"/>
        <source>Send files</source>
        <translation>发送文件</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="65"/>
        <source>Leave Press-to-Test mode</source>
        <translation>退出测验模式</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="79"/>
        <source>Status:</source>
        <translation>状态：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="85"/>
        <source>Idle</source>
        <translation>空闲</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="92"/>
        <source>Failed!</source>
        <translation>失败！</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="96"/>
        <source>Done!</source>
        <translation>完成！</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="112"/>
        <source>Target Directory</source>
        <translation>目标文件夹</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="121"/>
        <source>When dragging files onto Firebird, it will try to send them to the emulated system.</source>
        <translatorcomment>模拟系统听起来有点怪，译作客户机可能更好，但为忠实原文还是不做修改。</translatorcomment>
        <translation>将文件拖放到 Firebird 上，Firebird 会尝试将其发送到模拟的计算器中。</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageFileTransfer.qml" line="130"/>
        <source>Target folder for dropped files:</source>
        <translation>拖放文件的目标文件夹：</translation>
    </message>
</context>
<context>
    <name>ConfigPageKits</name>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="30"/>
        <source>Kit Properties</source>
        <translation>Kit 属性</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="42"/>
        <source>You need to specify files for Boot1 and Flash</source>
        <translation>您需要为 Boot1 和 Flash 指定文件</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="46"/>
        <source>Name:</source>
        <translation>名称：</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPageKits.qml" line="52"/>
        <source>Name</source>
        <translation>名称</translation>
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
        <translation>模拟</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="15"/>
        <source>File Transfer</source>
        <translation>文件传输</translation>
    </message>
    <message>
        <location filename="../qml/ConfigPagesModel.qml" line="20"/>
        <source>Debugging</source>
        <translation>调试</translation>
    </message>
</context>
<context>
    <name>FBAboutDialog</name>
    <message>
        <location filename="../fbaboutdialog.cpp" line="66"/>
        <source>About Firebird</source>
        <translation>关于 Firebird</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="67"/>
        <source>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;On GitHub&lt;/a&gt;</source>
        <translation>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;GitHub 项目主页&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="70"/>
        <source>Authors:&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;Based on nspire_emu v0.70 by Goplat&lt;br&gt;&lt;br&gt;This work is licensed under the GPLv3.&lt;br&gt;To view a copy of this license, visit &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</source>
        <translation>作者：&lt;br&gt;法比安·福格特 (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;阿德里安·贝特朗 (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;安东尼奥·巴斯克斯(&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;利奥尼尔·德布鲁克斯 (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;丹尼斯·阿瓦舒罗夫 (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;基于 Goplat 开发的 nspire_emu v0.70&lt;br&gt;&lt;br&gt;本作品采用GPLv3许可证授权。&lt;br&gt;欲查看本许可协议的副本，请访问 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="80"/>
        <source>Checking for update</source>
        <translation>正检查更新……</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="82"/>
        <source>Ok</source>
        <translation>确定</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="83"/>
        <source>Check for Update</source>
        <translation>检查更新</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="96"/>
        <source>No updates for -dev builds available.</source>
        <translation>开发版（-dev）暂不支持检查更新。</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="101"/>
        <source>Checking for updates...</source>
        <translation>正检查更新……</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="121"/>
        <source>Checking failed (%1)</source>
        <translation>检查失败（%1）</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="131"/>
        <source>No newer version available.</source>
        <translation>当前已是最新版本。</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="136"/>
        <source>&lt;b&gt;Newer version (%1) available &lt;a href=&apos;%2&apos;&gt;on GitHub&lt;/a&gt;!&lt;/b&gt;</source>
        <translation>&lt;b&gt;发现新版本（%1），可前往 &lt;a href=&apos;%2&apos;&gt;GitHub&lt;/a&gt; 查看！&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../fbaboutdialog.cpp" line="140"/>
        <source>Checking failed (invalid tag name)</source>
        <translation>检查失败（版本标签名无效）</translation>
    </message>
</context>
<context>
    <name>FBConfigDialog</name>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="9"/>
        <source>Firebird Emu Configuration</source>
        <translation>Firebird 模拟器设置</translation>
    </message>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="46"/>
        <source>Changes are saved automatically</source>
        <translation>更改将自动保存</translation>
    </message>
    <message>
        <location filename="../qml/FBConfigDialog.qml" line="52"/>
        <source>Ok</source>
        <translation>确定</translation>
    </message>
</context>
<context>
    <name>FileSelect</name>
    <message>
        <location filename="../qml/Firebird/UIComponents/FileSelect.qml" line="48"/>
        <source>(none)</source>
        <translation>未选择</translation>
    </message>
</context>
<context>
    <name>FlashDialog</name>
    <message>
        <location filename="../qml/FlashDialog.qml" line="10"/>
        <source>Create Flash Image</source>
        <translation>创建 Flash 镜像</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="33"/>
        <source>Model:</source>
        <translation>机型：</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="49"/>
        <source>CX Subtype:</source>
        <translation>CX 子型号：</translation>
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
        <translation>CX II 机型需要 Manuf 文件</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="151"/>
        <source>Flash saving failed</source>
        <translation>保存 Flash 文件失败</translation>
    </message>
    <message>
        <location filename="../qml/FlashDialog.qml" line="152"/>
        <source>Saving the flash file failed!</source>
        <translation>保存 Flash 文件失败！</translation>
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
        <translation>复制</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="17"/>
        <location filename="../mainwindow.cpp" line="862"/>
        <source>Firebird Emu</source>
        <translation>Firebird 模拟器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="80"/>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="100"/>
        <source>Pause</source>
        <translation>暂停</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="123"/>
        <source>Screenshot</source>
        <translation>截图</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="143"/>
        <source>Connect</source>
        <translation>连接</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="167"/>
        <source>Enable Fullspeed</source>
        <translation>启用全速模式</translation>
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
        <translation>调试器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="261"/>
        <location filename="../mainwindow.cpp" line="575"/>
        <source>File Transfer</source>
        <translation>文件传输</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="274"/>
        <source>Filename</source>
        <translation>文件名</translation>
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
        <translation>串口监视器</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="344"/>
        <location filename="../mainwindow.cpp" line="573"/>
        <source>Keypad</source>
        <translation>键盘</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="398"/>
        <source>&amp;Emulation</source>
        <translation>模拟(&amp;E)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="402"/>
        <source>Restart &amp;with Kit</source>
        <translation>使用 Kit 重启(&amp;W)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="411"/>
        <source>&amp;Boot Diags with Kit</source>
        <translation>使用 Kit 启动 Diags(&amp;B)</translation>
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
        <translation>语言</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="461"/>
        <source>S&amp;napshot</source>
        <translation>快照(&amp;N)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="471"/>
        <source>Abo&amp;ut</source>
        <translation>关于(&amp;U)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="488"/>
        <source>&amp;Reset</source>
        <translation>重置(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="497"/>
        <source>Enter &amp;Debugger</source>
        <translation>进入调试器(&amp;D)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="506"/>
        <source>&amp;Quit</source>
        <translation>退出(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="518"/>
        <source>&amp;Pause</source>
        <translation>暂停(&amp;P)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="521"/>
        <source>Pause execution</source>
        <translation>暂停运行</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="530"/>
        <location filename="../mainwindow.cpp" line="492"/>
        <source>Re&amp;start</source>
        <translation>重启(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="539"/>
        <source>&amp;Screenshot</source>
        <translation>截图(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="552"/>
        <source>Connect &amp;USB</source>
        <translation>连接 USB(&amp;U)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="561"/>
        <source>&amp;Save</source>
        <translation>保存(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="570"/>
        <source>&amp;Create Flash</source>
        <translation>创建 Flash(&amp;C)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="575"/>
        <source>Send &amp;file over XModem</source>
        <translation>通过 XModem 发送文件(&amp;F)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="584"/>
        <source>&amp;Suspend</source>
        <translation>保存快照(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="593"/>
        <source>&amp;Resume</source>
        <translation>从快照恢复(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="602"/>
        <source>Save &amp;to file</source>
        <translation>保存到文件(&amp;T)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="611"/>
        <source>Load &amp;from file</source>
        <translation>从文件加载(&amp;F)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="623"/>
        <source>&amp;Record GIF</source>
        <translation>录制 GIF(&amp;R)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="632"/>
        <source>&amp;About Firebird</source>
        <translation>关于 Firebird(&amp;A)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="637"/>
        <source>About &amp;Qt</source>
        <translation>关于 Qt(&amp;Q)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="645"/>
        <source>&amp;External LCD</source>
        <translation>独立屏幕窗口(&amp;E)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="654"/>
        <source>&amp;Configuration</source>
        <translation>设置(&amp;C)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="663"/>
        <source>Switch to Mobile UI</source>
        <translation>切换到移动端界面</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="668"/>
        <source>Leave &amp;PTT</source>
        <translation>退出测验模式(&amp;P)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="209"/>
        <location filename="../mainwindow.cpp" line="228"/>
        <source>Start the emulation via Emulation-&gt;Start.</source>
        <translation>请通过“模拟→启动”启动模拟。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="215"/>
        <source>Default Kit not found</source>
        <translation>未找到默认 Kit</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="266"/>
        <source>Language change</source>
        <translation>切换语言</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="266"/>
        <source>No translation available for this language :(</source>
        <translation>此语言暂无可用翻译 :(</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="380"/>
        <source>Download failed</source>
        <translation>下载失败</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="380"/>
        <source>Could not download file.</source>
        <translation>无法下载文件。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="476"/>
        <location filename="../mainwindow.cpp" line="757"/>
        <source>Could not resume</source>
        <translation>无法从快照恢复</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="476"/>
        <source>Try to restart this app.</source>
        <translation>请尝试重启应用。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="492"/>
        <source>&amp;Start</source>
        <translation>启动(&amp;S)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="521"/>
        <source>Docks</source>
        <translation>面板</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="524"/>
        <source>Enable UI edit mode</source>
        <translation>调整面板布局</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="585"/>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="592"/>
        <source>Save Screenshot</source>
        <translation>保存截图</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="592"/>
        <source>PNG images (*.png)</source>
        <translation>PNG 图像 (*.png)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="597"/>
        <source>Screenshot failed</source>
        <translation>截图失败</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="597"/>
        <source>Failed to save screenshot!</source>
        <translation>保存截图失败！</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="615"/>
        <source>Save Recording</source>
        <translation>保存 GIF 动画</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="615"/>
        <source>GIF images (*.gif)</source>
        <translation>GIF 图像 (*.gif)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="625"/>
        <source>Failed recording GIF</source>
        <translation>录制 GIF 失败</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="625"/>
        <source>A failure occured during recording</source>
        <translation>录制过程中发生错误</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="645"/>
        <location filename="../mainwindow.cpp" line="647"/>
        <source>Disconnect USB</source>
        <translation>断开 USB 连接</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="645"/>
        <location filename="../mainwindow.cpp" line="647"/>
        <source>Connect USB</source>
        <translation>连接 USB</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="674"/>
        <source>Can&apos;t resume</source>
        <translation>无法从快照恢复</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="674"/>
        <location filename="../mainwindow.cpp" line="685"/>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>当前 Kit 尚未设置快照文件</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="685"/>
        <source>Can&apos;t suspend</source>
        <translation>无法保存快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="690"/>
        <source>Select snapshot to resume from</source>
        <translation>选择用于恢复模拟的快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="697"/>
        <source>Select snapshot to suspend to</source>
        <translation>选择快照保存位置</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="745"/>
        <source>Emulation started</source>
        <translation>模拟已启动</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="747"/>
        <source>Could not start the emulation</source>
        <translation>无法启动模拟</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="747"/>
        <source>Starting the emulation failed.
Are the paths to boot1 and flash correct?</source>
        <translation>启动模拟失败。
请检查 Boot1 和 Flash 的路径是否正确。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="755"/>
        <source>Emulation resumed from snapshot</source>
        <translation>已从快照恢复模拟</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="757"/>
        <source>Resuming failed.
Try to fix the issue and try again.</source>
        <translation>从快照恢复失败。
请解决问题后重试。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="763"/>
        <source>Snapshot saved</source>
        <translation>快照已保存</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="765"/>
        <source>Could not suspend</source>
        <translation>无法保存快照</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="765"/>
        <source>Suspending failed.
Try to fix the issue and try again.</source>
        <translation>保存快照失败。
请解决问题后重试。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="779"/>
        <source>Emulation stopped</source>
        <translation>模拟已停止</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="859"/>
        <source>Firebird Emu - %1</source>
        <translation>Firebird 模拟器 - %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="884"/>
        <source>No boot1 set</source>
        <translation>尚未设置 Boot1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="884"/>
        <source>Before you can start the emulation, you have to select a proper boot1 file.</source>
        <translation>启动模拟前，您需要选择合适的 Boot1 文件。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="890"/>
        <source>No flash image loaded</source>
        <translation>尚未加载 Flash 镜像</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="890"/>
        <source>Before you can start the emulation, you have to load a proper flash file.
You can create one via Flash-&gt;Create Flash in the menu.</source>
        <translation>启动模拟前，您需要加载合适的 Flash 文件。
您可以通过菜单中的“Flash→创建 Flash”创建该文件。</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="898"/>
        <source>Restart needed</source>
        <translation>需要重启</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="898"/>
        <source>Failed to restart emulator. Close and reopen this app.
</source>
        <translation>模拟器重启失败。请关闭应用后重新打开。
</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="944"/>
        <source>Select file to send</source>
        <translation>选择要发送的文件</translation>
    </message>
</context>
<context>
    <name>MobileUI</name>
    <message>
        <location filename="../qml/MobileUI.qml" line="65"/>
        <source>Suspend failed</source>
        <translation>保存快照失败</translation>
    </message>
    <message>
        <location filename="../qml/MobileUI.qml" line="66"/>
        <source>Suspending the emulation failed. Do you still want to quit Firebird?</source>
        <translation>保存快照失败。您仍要退出 Firebird 吗？</translation>
    </message>
</context>
<context>
    <name>MobileUIConfig</name>
    <message>
        <location filename="../qml/MobileUIConfig.qml" line="66"/>
        <source>Changes are saved automatically</source>
        <translation>更改将自动保存</translation>
    </message>
</context>
<context>
    <name>MobileUIDrawer</name>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="42"/>
        <source>Start</source>
        <translation>启动</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="57"/>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="69"/>
        <source>Resume</source>
        <translation>从快照恢复</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="84"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="89"/>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="90"/>
        <source>Failed to save changes!</source>
        <translation>保存更改失败！</translation>
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
        <translation>Flash 已保存，但尚未设置快照保存位置。
您将无法从快照恢复模拟。</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="127"/>
        <source>Desktop UI</source>
        <translation>桌面界面</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="136"/>
        <source>Configuration</source>
        <translation>设置</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="156"/>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <location filename="../qml/MobileUIDrawer.qml" line="175"/>
        <source>About Firebird</source>
        <translation>关于 Firebird</translation>
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
        <translation>作者：&lt;br&gt;法比安·福格特 (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;阿德里安·贝特朗 (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;安东尼奥·巴斯克斯(&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;利奥尼尔·德布鲁克斯 (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;丹尼斯·阿瓦舒罗夫 (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;基于 Goplat 开发的 nspire_emu v0.70&lt;br&gt;&lt;br&gt;本作品采用GPLv3许可证授权。&lt;br&gt;欲查看本许可协议的副本，请访问 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
</context>
<context>
    <name>QMLBridge</name>
    <message>
        <location filename="../qmlbridge.cpp" line="52"/>
        <source>Default</source>
        <translation>默认</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="305"/>
        <source>None</source>
        <translation>未选择</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="383"/>
        <location filename="../qmlbridge.cpp" line="401"/>
        <location filename="../qmlbridge.cpp" line="416"/>
        <source>Open failed</source>
        <translation>打开失败</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="392"/>
        <source>Found %1 instead</source>
        <translation>实际识别为 %1</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="525"/>
        <source>Could not stop emulation</source>
        <translation>无法停止模拟</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="533"/>
        <source>Starting emulation</source>
        <translation>正启动模拟</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="537"/>
        <source>No boot1 or flash selected.
Swipe keypad left for configuration.</source>
        <translation>尚未选择 Boot1 或 Flash。
请向左滑动键盘打开设置。</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="554"/>
        <source>Suspending emulation</source>
        <translation>正保存快照</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="559"/>
        <location filename="../qmlbridge.cpp" line="575"/>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>当前 Kit 尚未设置快照文件</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="566"/>
        <source>Resuming emulation</source>
        <translation>正从快照恢复模拟</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="666"/>
        <source>Emulation started</source>
        <translation>模拟已启动</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="668"/>
        <source>Couldn&apos;t start emulation</source>
        <translation>无法启动模拟</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="674"/>
        <source>Emulation resumed</source>
        <translation>已从快照恢复模拟</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="676"/>
        <source>Could not resume</source>
        <translation>无法从快照恢复</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="682"/>
        <source>Flash and snapshot saved</source>
        <translation>Flash 和快照已保存</translation>
    </message>
    <message>
        <location filename="../qmlbridge.cpp" line="684"/>
        <source>Couldn&apos;t save snapshot</source>
        <translation>无法保存快照</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../qtframebuffer.cpp" line="60"/>
        <source>LCD turned off</source>
        <translation>屏幕已关闭</translation>
    </message>
    <message>
        <location filename="../qtframebuffer.cpp" line="74"/>
        <source>In debugger</source>
        <translation>调试中</translation>
    </message>
</context>
<context>
    <name>USBLinkTreeWidget</name>
    <message>
        <location filename="../usblinktreewidget.cpp" line="82"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="91"/>
        <source>New folder</source>
        <translation>新建文件夹</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="109"/>
        <source>Download</source>
        <translation>下载</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="129"/>
        <source>Too much</source>
        <translation>文件过大</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="299"/>
        <source>Chose save location</source>
        <translation>选择保存位置</translation>
    </message>
    <message>
        <location filename="../usblinktreewidget.cpp" line="299"/>
        <source>TNS file (*.tns)</source>
        <translation>TNS 文件 (*.tns)</translation>
    </message>
</context>
<context>
    <name>VerticalSwipeBar</name>
    <message>
        <location filename="../qml/Firebird/UIComponents/VerticalSwipeBar.qml" line="15"/>
        <source>Swipe here</source>
        <translation>在此处滑动</translation>
    </message>
</context>
</TS>
