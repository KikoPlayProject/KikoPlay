# KikoPlay - NOT ONLY A Full-Featured Danmu Player

---
不仅仅是全功能弹幕播放器
[![latest packaged version(s)](https://repology.org/badge/latest-versions/kikoplay.svg)](https://repology.org/project/kikoplay/versions)
[![GitHub All Releases](https://img.shields.io/github/downloads/KikoPlayProject/KikoPlay/total)](https://github.com/KikoPlayProject/KikoPlay/releases)

<div align=center><img src="res/images/kikoplay-4.png" />
</div>

## 特性

 - OpenGL渲染，流畅的弹幕体验，支持正常滚动模式/直播模式显示弹幕
 - libmpv播放内核，保留mpv灵活的参数设置，支持实时进度条预览
 - 树形播放列表，可随意组织视频文件
 - 支持所有主流视频网站弹幕搜索下载，同时可以通过脚本支持更多弹幕来源
 - 灵活的弹幕屏蔽规则设定，支持自动合并相似弹幕、分析标注弹幕事件，提升观看体验
 - 支持批量管理弹幕池、弹幕时间轴调整，更好地处理本地视频和网站上的视频时长不一致的情况
 - 强大的资料库功能，支持通过多种方式记录并组织你看过的动画，通过脚本扩展可以支持更多信息来源
 - 局域网服务，你可以通过网页在其他设备上观看，现在还有[Android端](https://github.com/KikoPlayProject/KikoPlay-Android-LAN)可供选择
 - DLNA功能支持投屏到其他设备，也可以作为媒体服务器供其他设备访问
 - 集成每日放送、资源搜索、aria2下载、自动下载等功能，在KikoPlay里即可完成下载、观看、管理等全部操作
 - .........

任何人都可以为KikoPlay编写脚本来支持更多弹幕、视频资料以及资源来源，[脚本仓库](https://github.com/KikoPlayProject/KikoPlayScript)  
还可以通过KikoPlay的Web API实现其他功能，[Web API参考](web/README.md)

## 编译

KikoPlay基于以下项目：

 - Qt 5.15.2
 - [libmpv](https://github.com/mpv-player/mpv)
 - [aria2](https://github.com/aria2/aria2)
 - [Qt-Nice-Frameless-Window](https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window)
 - zlib 1.2.11
 - [QtWebApp](http://stefanfrings.de/qtwebapp/index.html)
 - Lua 5.3

编译环境： Windows平台使用MSVC2019，gcc 7.3.0(其他版本未测试)，其他平台依照平台推荐配置（包管理器/Xcode）

Windows上使用Qt Creator打开build.pro文件后可直接编译，Linux/macOS亦然，当然也可以手动切换到目录执行`qmake`和`make`

Linux各发行版的说明参见[这里](linux.md)

macOS使用Homebrew包管理器：https://github.com/KikoPlayProject/Homebrew-KikoPlay

自从0.2.3版本后只提供64位版本，不建议使用32位版本

### cmake编译

关于cmake编译请参见[这里](cmake_build.md) （cmake编译方法为其他网友提供，随着版本更新可能失效，需要用户自行调整）


## 下载

可以从[Github](https://github.com/KikoPlayProject/KikoPlay/releases) 或 [百度网盘](https://pan.baidu.com/s/1gyT0FU9rioaa77znhAUx2w) 或 QQ群文件 下载Windows版

## 反馈

如果有问题，欢迎创建issue或者联系我:
dx_8820832#yeah.net（#→@），或者加QQ群874761809反馈

## 一起来让KikoPlay更好！

 - 为主程序贡献代码，包括但不限于新功能开发、BUG修复等等
 - 为KikoPlay编写脚本，支持更多弹幕、资料或者资源来源，脚本仓库在[这里](https://github.com/KikoPlayProject/KikoPlayScript)
 - 提升局域网网页访问体验，目前的网页是在[这里](https://github.com/KikoPlayProject/KikoPlay/tree/master/web)
 - 改进[Android局域网客户端]((https://github.com/KikoPlayProject/KikoPlay-Android-LAN))
 - 开发其他平台上的局域网客户端
 - 测试，找出BUG并反馈
 - ......
 - Star or Fork

## 截图

![](screenshot/KikoPlay1.jpg)
![](screenshot/KikoPlay1-2.jpg)
![](screenshot/KikoPlay-Ubuntu.png)
![](screenshot/KikoPlay2.jpg)
![](screenshot/KikoPlay3.jpg)
![](screenshot/KikoPlay4.jpg)
![](screenshot/KikoPlay_web.jpg)
