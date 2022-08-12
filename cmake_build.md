# cmake编译

现在KikoPlay可用cmake编译，需要使用[vcpkg](https://github.com/microsoft/vcpkg)来管理依赖

```bash
git clone https://github.com/microsoft/vcpkg
```

在Windows中：

```powershell
.\vcpkg\bootstrap-vcpkg.bat
```

在Unix中：

```bash
./vcpkg/bootstrap-vcpkg.sh
```

然后我们clone该存储库，确认CMake已经安装，然后键入下面命令来配置CMake：

```bash
cmake -B build -S . "-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
```

> 注意，`[path to vcpkg]`是你vcpkg根目录的路径

如果你的系统已经安装了QT，那么可以通过指定`-DUSE_VCPKG_QT=FALSE`选项来让vcpkg不构建QT依赖，而直接从系统中寻找QT库。如果你是linux系统，并且使用vcpkg来依赖QT，那么可能需要安装如下的第三方包才能继续(以ubuntu举例)：

```bash
sudo apt install libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxkbcommon-dev libxkbcommon-x11-dev libxcb-xinerama0-dev
```

该存储库还依赖于libmpv，所以如果你是linux系统，你需要安装这个包：

```bash
sudo apt install libmpv-dev
```


开始编译：

```bash
cmake --build build --config Release
```

> vcpkg安装的时间可能会很长，因为它会从源代码构建QT，请耐心等待


## 多核编译

在linux中，我们可以使用

```bash
cmake --build build --config Release -- -j <core_number>
```

来指定make并行编译的数量，加快编译速度

在Windows中，我们可以通过在配置前添加`\MP`标志来开启MSBuild的并行编译：

```powershell
cmake -B build -S . "-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_FLAGS=/MP ${CMAKE_CXX_FLAGS}"
cmake --build build --config Release
```

或者你可以直接使用Ninja，它默认就是并行编译