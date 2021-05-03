Linux上我测试了Ubuntu 18.04 x64，Manjaro 18.0.4 x64，其他系统可自行编译

## Ubuntu 18.04
Ubuntu 18.04 x64上编译的大概流程：

 1. 安装Qt(测试安装的是Qt 5.12.3) 
 2. 安装OpenGL Library:
     ```
     sudo apt-get install build-essential
     sudo apt-get install build-essential libgl1-mesa-dev
     ```
 3. 安装libmpv和zlib:
     ```
     sudo apt-get install libmpv-dev
     sudo apt-get install zlib1g-dev
     ```
 4. 下载编译安装[QHttpEngine](https://github.com/nitroshare/qhttpengine)
 5. 下载Lua 5.3.4编译，得到liblua.a
 6. 准备好KikoPlay.pro文件，现在链接部分是这样的：
     ```
     contains(QT_ARCH, i386){
         win32: LIBS += -L$$PWD/lib/ -llibmpv.dll
         win32: LIBS += -L$$PWD/lib/ -lzlibstat
         win32: LIBS += -L$$PWD/lib/ -lqhttpengine
     }else{
         win32: LIBS += -L$$PWD/lib/x64/ -llibmpv.dll
         win32: LIBS += -L$$PWD/lib/x64/ -lzlibstat
         win32: LIBS += -L$$PWD/lib/x64/ -lqhttpengine
         win32: LIBS += -L$$PWD/lib/x64/ -llua53
         unix{
             LIBS += -L/usr/lib/x86_64-linux-gnu/ -lmpv
             LIBS += -L/usr/lib/x86_64-linux-gnu/ -lz
             LIBS += -L/usr/lib/x86_64-linux-gnu/ -lm
             LIBS += -L$$PWD/lib/x64/linux/ -llua
             LIBS += -L/usr/local/lib/ -lqhttpengine
             LIBS += -L/usr/lib/x86_64-linux-gnu/ -ldl
         }
     }  
     ```
    注意unix部分链接的外部库的路径，默认liblua.a的位置是KikoPlay工程目录下lib/x64/linux目录下，编译好之后可以放到这里
 7. 开始编译，进入KikoPlay工程目录：
     ```
     qmake
     make
     ```

## ArchLinux/Manjaro

### 从 [AUR](https://aur.archlinux.org/packages/kikoplay/) 安装

ArchLinux 可以直接通过支持 AUR 的包管理工具安装（具体命令格式参照相应包管理工具），如：

  ```bash
  yay -S kikoplay
  ```

### 手动编译安装

 1. 安装 mpv
     ```bash
     pacman -S mpv
     ```
 2. 下载编译安装 [QHttpEngine](https://github.com/nitroshare/qhttpengine)（请参考官方文档）
 3. 开始编译，进入 KikoPlay 工程目录：
     ```bash
     qmake
     make
     ```

## Gentoo

Gentoo上的编译与安装流程：

 1. 添加[Gentoo GURU overlay](https://github.com/gentoo/guru)

     ```bash
    sudo eselect repository enable guru && sudo emerge --sync
    ```
 2. 直接安装 ``media-video/kikoplay``，会自动解决所有依赖关系以及编译好。

    ```bash
    sudo emerge media-video/kikoplay
    ```

## 备注

编译成功后得到 `KikoPlay` 文件，可直接运行 `./KikoPlay`，如果提示缺少 libqhttpengine 等库，可尝试将编译 QHttpEngine 得到的库放到 `/usr/lib` 目录下，也可以将 `/usr/local/lib` 加入 `LD_LIBRARY_PATH` 环境变量中。下载功能需要 `aria2c`，可自行编译或者下载后放到 `KikoPlay` 同一目录下。
