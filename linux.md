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

## 手动编译

手动编译现在各大平台基本一致，Linux 方面各发行版大同小异，保证开发工具集和 zlib、mpv 的开发包都安装好即可。

对于 Ubuntu 22.04 LTS/Debian 11：
  ```bash
  sudo apt install build-essential
  sudo apt install qtbase5-dev qt5-qmake libmpv-dev zlib1g-dev

  ```
对于 Fedora 35（暂不支持 Fedora 36）：
  ```bash
  sudo dnf group install "C Development Tools and Libraries"
  sudo dnf config-manager --add-repo=https://negativo17.org/repos/fedora-multimedia.repo
  sudo dnf install qt5-qtbase-devel mpv-libs-devel zlib-devel

  ```

编译时直接执行 `qmake build.pro`（Fedora 下为 `qmake-qt5`），然后 `make -j <线程数>` 即可。

编译成功后得到 `KikoPlay` 文件，可直接运行 `./KikoPlay`。下载功能需要 `aria2c`，可自行编译或者下载后放到 `KikoPlay` 同一目录下。
