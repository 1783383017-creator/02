PPSSPP Mouse Camera V2
========================

这是 V2 原型，使用 Windows Raw Input 读取鼠标 ΔX/ΔY。
F8 开关鼠标镜头，F9 退出。

重要：
本版本仍通过 Windows 键盘输入把鼠标移动转换为 PPSSPP 已绑定的
右摇杆方向，因此它已经是 Raw Input 无限鼠标思路，但还不是直接写入
PPSSPP 内部的模拟摇杆轴。

PPSSPP 设置：
右摇杆 Left  -> Left Arrow
右摇杆 Right -> Right Arrow
右摇杆 Up    -> Up Arrow
右摇杆 Down  -> Down Arrow

编译：
1. Windows 安装 MinGW-w64（包含 g++）。
2. 把本文件夹放到电脑。
3. 双击 build.bat。
4. 生成 PPSSPP_Mouse_Camera_V2.exe。

操作：
F8 = 开/关
F9 = 退出

如果鼠标镜头方向反了，可以在源码里把 g_invertY 改成 true。
灵敏度在 g_sensX / g_sensY 调整。

下一步可以继续做 V3：
直接输出虚拟 Xbox/DS4 右摇杆轴，真正做到模拟摇杆级别的平滑镜头。


========================
GitHub 免费自动编译
========================

1. 登录 GitHub。
2. 新建一个 Repository（仓库）。
3. 把这个文件夹里的全部内容上传到仓库。
4. 打开仓库顶部的 Actions。
5. 选择 “Build Windows EXE”。
6. 点击 “Run workflow”。
7. 等待 Windows 云端编译完成。
8. 打开完成的任务，在 Artifacts 下载：
   PPSSPP_Mouse_Camera_V2.zip

整个编译过程在 GitHub 的 Windows runner 上完成，你自己的电脑不需要安装
Visual Studio、MinGW 或 CMake。

注意：GitHub 页面名称可能随界面语言略有不同。
