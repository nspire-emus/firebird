<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>ConfigPageDebug</name>
    <message>
        <source>Remote GDB debugging</source>
        <translation>远程 GDB 调试</translation>
    </message>
    <message>
        <source>If enabled, a remote GDB debugger can be connected to the port and be used for debugging.</source>
        <translation>启用后，远程 GDB 调试器可连接到此端口进行调试。</translation>
    </message>
    <message>
        <source>Enable GDB stub on Port</source>
        <translation>在端口上启用 GDB stub</translation>
    </message>
    <message>
        <source>Remote access to internal debugger</source>
        <translation>远程访问内部调试器</translation>
    </message>
    <message>
        <source>Enable this to access the internal debugger via TCP (telnet/netcat), like for firebird-send.</source>
        <translation>启用后，可通过 TCP（telnet/netcat）访问内部调试器，例如使用 firebird-send。</translation>
    </message>
    <message>
        <source>Enable internal debugger on Port</source>
        <translation>在端口上启用内部调试器</translation>
    </message>
    <message>
        <source>Enter into Debugger</source>
        <translation>进入调试器</translation>
    </message>
    <message>
        <source>Configure which situations cause the emulator to trap into the debugger.</source>
        <translation>配置模拟器在何种情况下中断并进入调试器。</translation>
    </message>
    <message>
        <source>Enter Debugger on Startup</source>
        <translation>启动时进入调试器</translation>
    </message>
    <message>
        <source>Enter Debugger on Warnings and Errors</source>
        <translation>出现警告和错误时进入调试器</translation>
    </message>
    <message>
        <source>Print a message on Warnings</source>
        <translation>出现警告时输出消息</translation>
    </message>
</context>
<context>
    <name>ConfigPageEmulation</name>
    <message>
        <source>Startup</source>
        <translation>启动</translation>
    </message>
    <message>
        <source>When opening Firebird, the selected Kit will be started. If available, it will resume the emulation from the provided snapshot.</source>
        <translation>打开 Firebird 时，将按所选配置方案启动模拟器。如有快照，将从指定快照恢复运行。</translation>
    </message>
    <message>
        <source>Choose the Kit selected on startup and after restarting. If the checkbox is active, it will be launched when Firebird starts.</source>
        <translation>选择启动或重启后使用的配置方案。勾选后，Firebird 启动时会自动按此方案启动模拟器。</translation>
    </message>
    <message>
        <source>On Startup, run Kit</source>
        <translation>启动时自动运行所选配置方案</translation>
    </message>
    <message>
        <source>Shutdown</source>
        <translation>关闭</translation>
    </message>
    <message>
        <source>When closing firebird using the back button, save the current state to the current snapshot. Does not work when firebird is in the background.</source>
        <translation>使用返回按钮关闭 Firebird 时，将当前状态保存到当前快照。Firebird 在后台运行时无效。</translation>
    </message>
    <message>
        <source>On Application end, save the current state to the current snapshot.</source>
        <translation>应用程序退出时，将当前状态保存到当前快照。</translation>
    </message>
    <message>
        <source>Save snapshot on shutdown</source>
        <translation>关闭时保存快照</translation>
    </message>
    <message>
        <source>UI Preferences</source>
        <translation>界面首选项</translation>
    </message>
    <message>
        <source>Change the side of the keypad in landscape orientation.</source>
        <translation>更改横屏时键盘所在的一侧。</translation>
    </message>
    <message>
        <source>Left-handed mode</source>
        <translation>左手模式</translation>
    </message>
</context>
<context>
    <name>ConfigPageFileTransfer</name>
    <message>
        <source>File Transfer</source>
        <translation>文件传输</translation>
    </message>
    <message>
        <source>If you are unable to use the main window&apos;s file transfer using either drag&apos;n&apos;drop or the file explorer, you can send files here.</source>
        <translation>如果无法通过拖放或文件浏览器使用主窗口的文件传输功能，可以在此发送文件。</translation>
    </message>
    <message>
        <source>Here you can send files into the target folder specified below.</source>
        <translation>可以在此将文件发送到下方指定的目标文件夹。</translation>
    </message>
    <message>
        <source>TNS Documents</source>
        <translation>TNS 文档</translation>
    </message>
    <message>
        <source>Operating Systems</source>
        <translation>操作系统</translation>
    </message>
    <message>
        <source>Starting</source>
        <translation>正在启动</translation>
    </message>
    <message>
        <source>Send files</source>
        <translation>发送文件</translation>
    </message>
    <message>
        <source>Leave Press-to-Test mode</source>
        <translation>退出 Press-to-Test 模式</translation>
    </message>
    <message>
        <source>Status:</source>
        <translation>状态：</translation>
    </message>
    <message>
        <source>Idle</source>
        <translation>空闲</translation>
    </message>
    <message>
        <source>Failed!</source>
        <translation>失败！</translation>
    </message>
    <message>
        <source>Done!</source>
        <translation>完成！</translation>
    </message>
    <message>
        <source>Target Directory</source>
        <translation>目标目录</translation>
    </message>
    <message>
        <source>When dragging files onto Firebird, it will try to send them to the emulated system.</source>
        <translation>将文件拖到 Firebird 上时，程序会尝试把它们发送到模拟系统。</translation>
    </message>
    <message>
        <source>Target folder for dropped files:</source>
        <translation>拖入文件的目标文件夹：</translation>
    </message>
</context>
<context>
    <name>ConfigPageKits</name>
    <message>
        <source>Kit Properties</source>
        <translation>配置方案属性</translation>
    </message>
    <message>
        <source>You need to specify files for Boot1 and Flash</source>
        <translation>必须为 Boot1 和闪存镜像指定文件</translation>
    </message>
    <message>
        <source>Name:</source>
        <translation>名称：</translation>
    </message>
    <message>
        <source>Name</source>
        <translation>名称</translation>
    </message>
    <message>
        <source>Boot1:</source>
        <translation>Boot1：</translation>
    </message>
    <message>
        <source>Flash:</source>
        <translation>闪存镜像：</translation>
    </message>
    <message>
        <source>Snapshot:</source>
        <translation>快照：</translation>
    </message>
</context>
<context>
    <name>ConfigPagesModel</name>
    <message>
        <source>Flash &amp; Boot1</source>
        <translation>闪存 &amp; Boot1</translation>
    </message>
    <message>
        <source>Emulation</source>
        <translation>模拟</translation>
    </message>
    <message>
        <source>File Transfer</source>
        <translation>文件传输</translation>
    </message>
    <message>
        <source>Debugging</source>
        <translation>调试</translation>
    </message>
</context>
<context>
    <name>FBAboutDialog</name>
    <message>
        <source>About Firebird</source>
        <translation>关于 Firebird</translation>
    </message>
    <message>
        <source>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;On GitHub&lt;/a&gt;</source>
        <translation>&lt;h3&gt;Firebird %1&lt;/h3&gt;&lt;a href=&apos;https://github.com/nspire-emus/firebird&apos;&gt;GitHub 项目主页&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Authors:&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;Based on nspire_emu v0.70 by Goplat&lt;br&gt;&lt;br&gt;This work is licensed under the GPLv3.&lt;br&gt;To view a copy of this license, visit &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</source>
        <translation>作者：&lt;br&gt;Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;基于 Goplat 开发的 nspire_emu v0.70&lt;br&gt;&lt;br&gt;本软件采用 GPLv3 许可证授权。&lt;br&gt;许可证文本请访问 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
    <message>
        <source>Checking for update</source>
        <translation>正在检查更新</translation>
    </message>
    <message>
        <source>Ok</source>
        <translation>确定</translation>
    </message>
    <message>
        <source>Check for Update</source>
        <translation>检查更新</translation>
    </message>
    <message>
        <source>No updates for -dev builds available.</source>
        <translation>开发版不提供更新。</translation>
    </message>
    <message>
        <source>Checking for updates...</source>
        <translation>正在检查更新……</translation>
    </message>
    <message>
        <source>Checking failed (%1)</source>
        <translation>检查失败（%1）</translation>
    </message>
    <message>
        <source>No newer version available.</source>
        <translation>没有可用的新版本。</translation>
    </message>
    <message>
        <source>&lt;b&gt;Newer version (%1) available &lt;a href=&apos;%2&apos;&gt;on GitHub&lt;/a&gt;!&lt;/b&gt;</source>
        <translation>&lt;b&gt;新版本（%1）已在 &lt;a href=&apos;%2&apos;&gt;GitHub&lt;/a&gt; 上发布！&lt;/b&gt;</translation>
    </message>
    <message>
        <source>Checking failed (invalid tag name)</source>
        <translation>检查失败（标签名称无效）</translation>
    </message>
</context>
<context>
    <name>FBConfigDialog</name>
    <message>
        <source>Firebird Emu Configuration</source>
        <translation>Firebird Emu 配置</translation>
    </message>
    <message>
        <source>Changes are saved automatically</source>
        <translation>更改会自动保存</translation>
    </message>
    <message>
        <source>Ok</source>
        <translation>确定</translation>
    </message>
</context>
<context>
    <name>FileSelect</name>
    <message>
        <source>(none)</source>
        <translation>（无）</translation>
    </message>
</context>
<context>
    <name>FlashDialog</name>
    <message>
        <source>Create Flash Image</source>
        <translation>创建闪存镜像</translation>
    </message>
    <message>
        <source>Model:</source>
        <translation>型号：</translation>
    </message>
    <message>
        <source>CX Subtype:</source>
        <translation>CX 子类型：</translation>
    </message>
    <message>
        <source>Manuf:</source>
        <translation>Manuf：</translation>
    </message>
    <message>
        <source>Boot2:</source>
        <translation>Boot2：</translation>
    </message>
    <message>
        <source>OS:</source>
        <translation>操作系统：</translation>
    </message>
    <message>
        <source>Bootloader:</source>
        <translation>Bootloader：</translation>
    </message>
    <message>
        <source>Installer:</source>
        <translation>安装程序：</translation>
    </message>
    <message>
        <source>Diags:</source>
        <translation>Diags：</translation>
    </message>
    <message>
        <source>Manuf required for CX II</source>
        <translation>CX II 需要 Manuf</translation>
    </message>
    <message>
        <source>Flash saving failed</source>
        <translation>闪存镜像保存失败</translation>
    </message>
    <message>
        <source>Saving the flash file failed!</source>
        <translation>保存闪存文件失败！</translation>
    </message>
</context>
<context>
    <name>KitList</name>
    <message>
        <source>Remove</source>
        <translation>移除</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <source>Firebird Emu</source>
        <translation>Firebird Emu</translation>
    </message>
    <message>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <source>Pause</source>
        <translation>暂停</translation>
    </message>
    <message>
        <source>Screenshot</source>
        <translation>截图</translation>
    </message>
    <message>
        <source>Connect</source>
        <translation>连接</translation>
    </message>
    <message>
        <source>Enable Fullspeed</source>
        <translation>启用全速模式</translation>
    </message>
    <message>
        <source>Speed: 0 %</source>
        <translation>速度：0 %</translation>
    </message>
    <message>
        <source>Debugger</source>
        <translation>调试器</translation>
    </message>
    <message>
        <source>File Transfer</source>
        <translation>文件传输</translation>
    </message>
    <message>
        <source>Filename</source>
        <translation>文件名</translation>
    </message>
    <message>
        <source>Size</source>
        <translation>大小</translation>
    </message>
    <message>
        <source>Serial Monitor</source>
        <translation>串口监视器</translation>
    </message>
    <message>
        <source>Keypad</source>
        <translation>键盘</translation>
    </message>
    <message>
        <source>&amp;Emulation</source>
        <translation>模拟(&amp;E)</translation>
    </message>
    <message>
        <source>Restart &amp;with Kit</source>
        <translation>使用配置方案重启(&amp;W)</translation>
    </message>
    <message>
        <source>&amp;Boot Diags with Kit</source>
        <translation>使用配置方案启动 Diags(&amp;B)</translation>
    </message>
    <message>
        <source>&amp;Flash</source>
        <translation>闪存(&amp;F)</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation>工具(&amp;T)</translation>
    </message>
    <message>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <source>S&amp;napshot</source>
        <translation>快照(&amp;N)</translation>
    </message>
    <message>
        <source>Abo&amp;ut</source>
        <translation>关于(&amp;U)</translation>
    </message>
    <message>
        <source>&amp;Reset</source>
        <translation>重置(&amp;R)</translation>
    </message>
    <message>
        <source>Enter &amp;Debugger</source>
        <translation>进入调试器(&amp;D)</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation>退出(&amp;Q)</translation>
    </message>
    <message>
        <source>&amp;Pause</source>
        <translation>暂停(&amp;P)</translation>
    </message>
    <message>
        <source>Pause execution</source>
        <translation>暂停执行</translation>
    </message>
    <message>
        <source>Re&amp;start</source>
        <translation>重启(&amp;S)</translation>
    </message>
    <message>
        <source>&amp;Screenshot</source>
        <translation>截图(&amp;S)</translation>
    </message>
    <message>
        <source>Connect &amp;USB</source>
        <translation>连接 USB(&amp;U)</translation>
    </message>
    <message>
        <source>&amp;Save</source>
        <translation>保存(&amp;S)</translation>
    </message>
    <message>
        <source>&amp;Create Flash</source>
        <translation>创建闪存镜像(&amp;C)</translation>
    </message>
    <message>
        <source>Send &amp;file over XModem</source>
        <translation>通过 XModem 发送文件(&amp;F)</translation>
    </message>
    <message>
        <source>&amp;Suspend</source>
        <translation>挂起(&amp;S)</translation>
    </message>
    <message>
        <source>&amp;Resume</source>
        <translation>恢复(&amp;R)</translation>
    </message>
    <message>
        <source>Save &amp;to file</source>
        <translation>保存到文件(&amp;T)</translation>
    </message>
    <message>
        <source>Load &amp;from file</source>
        <translation>从文件加载(&amp;F)</translation>
    </message>
    <message>
        <source>&amp;Record GIF</source>
        <translation>录制 GIF(&amp;R)</translation>
    </message>
    <message>
        <source>&amp;About Firebird</source>
        <translation>关于 Firebird(&amp;A)</translation>
    </message>
    <message>
        <source>About &amp;Qt</source>
        <translation>关于 Qt(&amp;Q)</translation>
    </message>
    <message>
        <source>&amp;External LCD</source>
        <translation>外部 LCD(&amp;E)</translation>
    </message>
    <message>
        <source>&amp;Configuration</source>
        <translation>配置(&amp;C)</translation>
    </message>
    <message>
        <source>Switch to Mobile UI</source>
        <translation>切换到移动版界面</translation>
    </message>
    <message>
        <source>Leave &amp;PTT</source>
        <translation>退出 PTT(&amp;P)</translation>
    </message>
    <message>
        <source>Start the emulation via Emulation-&gt;Start.</source>
        <translation>请通过“模拟→启动”开始模拟。</translation>
    </message>
    <message>
        <source>Default Kit not found</source>
        <translation>找不到默认配置方案</translation>
    </message>
    <message>
        <source>Language change</source>
        <translation>语言更改</translation>
    </message>
    <message>
        <source>No translation available for this language :(</source>
        <translation>此语言没有可用的翻译 :(</translation>
    </message>
    <message>
        <source>Download failed</source>
        <translation>下载失败</translation>
    </message>
    <message>
        <source>Could not download file.</source>
        <translation>无法下载文件。</translation>
    </message>
    <message>
        <source>Could not resume</source>
        <translation>无法恢复</translation>
    </message>
    <message>
        <source>Try to restart this app.</source>
        <translation>请尝试重启此应用程序。</translation>
    </message>
    <message>
        <source>&amp;Start</source>
        <translation>启动(&amp;S)</translation>
    </message>
    <message>
        <source>Docks</source>
        <translation>面板</translation>
    </message>
    <message>
        <source>Enable UI edit mode</source>
        <translation>允许调整面板布局</translation>
    </message>
    <message>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <source>Save Screenshot</source>
        <translation>保存截图</translation>
    </message>
    <message>
        <source>PNG images (*.png)</source>
        <translation>PNG 图像 (*.png)</translation>
    </message>
    <message>
        <source>Screenshot failed</source>
        <translation>截图失败</translation>
    </message>
    <message>
        <source>Failed to save screenshot!</source>
        <translation>保存截图失败！</translation>
    </message>
    <message>
        <source>Save Recording</source>
        <translation>保存录像</translation>
    </message>
    <message>
        <source>GIF images (*.gif)</source>
        <translation>GIF 图像 (*.gif)</translation>
    </message>
    <message>
        <source>Failed recording GIF</source>
        <translation>GIF 录制失败</translation>
    </message>
    <message>
        <source>A failure occured during recording</source>
        <translation>录制过程中发生错误</translation>
    </message>
    <message>
        <source>Disconnect USB</source>
        <translation>断开 USB</translation>
    </message>
    <message>
        <source>Connect USB</source>
        <translation>连接 USB</translation>
    </message>
    <message>
        <source>Can&apos;t resume</source>
        <translation>无法恢复</translation>
    </message>
    <message>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>当前配置方案未指定快照文件</translation>
    </message>
    <message>
        <source>Can&apos;t suspend</source>
        <translation>无法挂起</translation>
    </message>
    <message>
        <source>Select snapshot to resume from</source>
        <translation>选择用于恢复的快照</translation>
    </message>
    <message>
        <source>Select snapshot to suspend to</source>
        <translation>选择用于保存挂起状态的快照</translation>
    </message>
    <message>
        <source>Emulation started</source>
        <translation>模拟已启动</translation>
    </message>
    <message>
        <source>Could not start the emulation</source>
        <translation>无法启动模拟</translation>
    </message>
    <message>
        <source>Starting the emulation failed.
Are the paths to boot1 and flash correct?</source>
        <translation>启动模拟失败。
Boot1 和闪存镜像的路径是否正确？</translation>
    </message>
    <message>
        <source>Emulation resumed from snapshot</source>
        <translation>已从快照恢复模拟</translation>
    </message>
    <message>
        <source>Resuming failed.
Try to fix the issue and try again.</source>
        <translation>恢复失败。
请解决问题后重试。</translation>
    </message>
    <message>
        <source>Snapshot saved</source>
        <translation>快照已保存</translation>
    </message>
    <message>
        <source>Could not suspend</source>
        <translation>无法挂起</translation>
    </message>
    <message>
        <source>Suspending failed.
Try to fix the issue and try again.</source>
        <translation>挂起失败。
请解决问题后重试。</translation>
    </message>
    <message>
        <source>Emulation stopped</source>
        <translation>模拟已停止</translation>
    </message>
    <message>
        <source>Firebird Emu - %1</source>
        <translation>Firebird Emu - %1</translation>
    </message>
    <message>
        <source>No boot1 set</source>
        <translation>未设置 Boot1</translation>
    </message>
    <message>
        <source>Before you can start the emulation, you have to select a proper boot1 file.</source>
        <translation>开始模拟前，必须选择有效的 Boot1 文件。</translation>
    </message>
    <message>
        <source>No flash image loaded</source>
        <translation>未加载闪存镜像</translation>
    </message>
    <message>
        <source>Before you can start the emulation, you have to load a proper flash file.
You can create one via Flash-&gt;Create Flash in the menu.</source>
        <translation>开始模拟前，必须加载有效的闪存文件。
可以通过菜单中的“闪存→创建闪存镜像”创建一个。</translation>
    </message>
    <message>
        <source>Restart needed</source>
        <translation>需要重启</translation>
    </message>
    <message>
        <source>Failed to restart emulator. Close and reopen this app.
</source>
        <translation>模拟器重启失败。请关闭并重新打开此应用程序。
</translation>
    </message>
    <message>
        <source>Select file to send</source>
        <translation>选择要发送的文件</translation>
    </message>
</context>
<context>
    <name>MobileUI</name>
    <message>
        <source>Suspend failed</source>
        <translation>挂起失败</translation>
    </message>
    <message>
        <source>Suspending the emulation failed. Do you still want to quit Firebird?</source>
        <translation>挂起模拟失败。仍要退出 Firebird 吗？</translation>
    </message>
</context>
<context>
    <name>MobileUIConfig</name>
    <message>
        <source>Changes are saved automatically</source>
        <translation>更改会自动保存</translation>
    </message>
</context>
<context>
    <name>MobileUIDrawer</name>
    <message>
        <source>Start</source>
        <translation>启动</translation>
    </message>
    <message>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <source>Resume</source>
        <translation>恢复</translation>
    </message>
    <message>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <source>Error</source>
        <translation>错误</translation>
    </message>
    <message>
        <source>Failed to save changes!</source>
        <translation>保存更改失败！</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <source>Flash saved, but no snapshot location configured.
You won&apos;t be able to resume.</source>
        <translation>闪存镜像已保存，但未配置快照位置。
之后将无法恢复。</translation>
    </message>
    <message>
        <source>Desktop UI</source>
        <translation>桌面版界面</translation>
    </message>
    <message>
        <source>Configuration</source>
        <translation>配置</translation>
    </message>
    <message>
        <source>Speed: %1 %</source>
        <translation>速度：%1 %</translation>
    </message>
    <message>
        <source>About Firebird</source>
        <translation>关于 Firebird</translation>
    </message>
    <message>
        <source>Authors:&lt;br&gt;
                               Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;
                               Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;
                               Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;
                               Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;
                               Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;
                               Based on nspire_emu v0.70 by Goplat&lt;br&gt;&lt;br&gt;
                               This work is licensed under the GPLv3.&lt;br&gt;
                               To view a copy of this license, visit &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</source>
        <translation>作者：&lt;br&gt;
                               Fabian Vogt (&lt;a href=&apos;https://github.com/Vogtinator&apos;&gt;Vogtinator&lt;/a&gt;)&lt;br&gt;
                               Adrien Bertrand (&lt;a href=&apos;https://github.com/adriweb&apos;&gt;Adriweb&lt;/a&gt;)&lt;br&gt;
                               Antonio Vasquez (&lt;a href=&apos;https://github.com/antoniovazquezblanco&apos;&gt;antoniovazquezblanco&lt;/a&gt;)&lt;br&gt;
                               Lionel Debroux (&lt;a href=&apos;https://github.com/debrouxl&apos;&gt;debrouxl&lt;/a&gt;)&lt;br&gt;
                               Denis Avashurov (&lt;a href=&apos;https://github.com/denisps&apos;&gt;denisps&lt;/a&gt;)&lt;br&gt;
                               基于 Goplat 开发的 nspire_emu v0.70&lt;br&gt;&lt;br&gt;
                               本软件采用 GPLv3 许可证授权。&lt;br&gt;
                               许可证文本请访问 &lt;a href=&apos;https://www.gnu.org/licenses/gpl-3.0.html&apos;&gt;https://www.gnu.org/licenses/gpl-3.0.html&lt;/a&gt;</translation>
    </message>
</context>
<context>
    <name>QMLBridge</name>
    <message>
        <source>Default</source>
        <translation>默认</translation>
    </message>
    <message>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <source>Open failed</source>
        <translation>打开失败</translation>
    </message>
    <message>
        <source>Found %1 instead</source>
        <translation>实际找到的是 %1</translation>
    </message>
    <message>
        <source>Could not stop emulation</source>
        <translation>无法停止模拟</translation>
    </message>
    <message>
        <source>Starting emulation</source>
        <translation>正在启动模拟</translation>
    </message>
    <message>
        <source>No boot1 or flash selected.
Swipe keypad left for configuration.</source>
        <translation>未选择 Boot1 或闪存镜像。
向左滑动键盘以进行配置。</translation>
    </message>
    <message>
        <source>Suspending emulation</source>
        <translation>正在挂起模拟</translation>
    </message>
    <message>
        <source>The current kit does not have a snapshot file configured</source>
        <translation>当前配置方案未指定快照文件</translation>
    </message>
    <message>
        <source>Resuming emulation</source>
        <translation>正在恢复模拟</translation>
    </message>
    <message>
        <source>Emulation started</source>
        <translation>模拟已启动</translation>
    </message>
    <message>
        <source>Couldn&apos;t start emulation</source>
        <translation>无法启动模拟</translation>
    </message>
    <message>
        <source>Emulation resumed</source>
        <translation>模拟已恢复</translation>
    </message>
    <message>
        <source>Could not resume</source>
        <translation>无法恢复</translation>
    </message>
    <message>
        <source>Flash and snapshot saved</source>
        <translation>闪存镜像和快照已保存</translation>
    </message>
    <message>
        <source>Couldn&apos;t save snapshot</source>
        <translation>无法保存快照</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>LCD turned off</source>
        <translation>LCD 已关闭</translation>
    </message>
    <message>
        <source>In debugger</source>
        <translation>已进入调试器</translation>
    </message>
</context>
<context>
    <name>USBLinkTreeWidget</name>
    <message>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <source>New folder</source>
        <translation>新建文件夹</translation>
    </message>
    <message>
        <source>Download</source>
        <translation>下载</translation>
    </message>
    <message>
        <source>Too much</source>
        <translation>内容过多</translation>
    </message>
    <message>
        <source>Chose save location</source>
        <translation>选择保存位置</translation>
    </message>
    <message>
        <source>TNS file (*.tns)</source>
        <translation>TNS 文件 (*.tns)</translation>
    </message>
</context>
<context>
    <name>VerticalSwipeBar</name>
    <message>
        <source>Swipe here</source>
        <translation>在此处滑动</translation>
    </message>
</context>
</TS>
