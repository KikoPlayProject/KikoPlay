Linux上我测试了Ubuntu 18.04 x64，Manjaro 18.0.4 x64，其他系统可自行编译

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

Manjaro上的编译更为简单：
 1. 安装mpv
 2. 下载编译安装[QHttpEngine](https://github.com/nitroshare/qhttpengine)
 3. 开始编译，进入KikoPlay工程目录：
     ```
     qmake
     make
     ```

Gentoo上的编译安装流程：

 1. 添加[Gentoo GURU overlay](https://github.com/gentoo/guru)
 2. 直接安装 ``media-video/kikoplay``，会自动解决所有依赖关系以及编译好。

    ```bash
    sudo layman -a guru
    sudo emerge media-video/kikoplay
    ```

编译成功后得到KikoPlay文件，可直接运行./KikoPlay，如果提示缺少libqhttpengine等库，可尝试将编译QHttpEngine得到的库放到/usr/lib目录下，也可以将/usr/local/lib加入LD_LIBRARY_PATH环境变量中。下载功能需要aria2c，可自行编译或者下载后放到KikoPlay同一目录下
