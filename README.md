﻿# KikoPlay - A Full-Featured Danmu Player
---
全功能弹幕播放器

## 特性
 - 基于libmpv，支持多种格式，支持mpv灵活的参数设置
 - 树形播放列表，可以随意组织你的番剧
 - 支持搜索下载多个网站的弹幕
 - 媒体库可以记录你看过的番剧，同时从Bangumi上获取详细信息
 - 支持局域网服务，你可以通过网页在其他设备上观看
 - 集成下载功能
 - .........

## 编译

KikoPlay基于以下项目：

 - Qt 5.10.1
 - [libmpv](https://github.com/mpv-player/mpv)
 - [aria2](https://github.com/aria2/aria2)
 - [Qt-Nice-Frameless-Window](https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window)
 - zlib 1.2.11
 - [QHttpEngine](https://github.com/nitroshare/qhttpengine)

编译环境：
MSVC2015 32bit

运行时需要将mpv-1.dll、qhttpengine.dll和aria2c.exe放在可执行文件所在的目录中，这两个文件可在百度网盘的Release中找到，也可以自行编译libmpv、aria2。

## 截图

![](screenshot/KikoPlay1.jpg)
![](screenshot/KikoPlay2.jpg)
![](screenshot/KikoPlay3.jpg)
![](screenshot/KikoPlay_web.jpg)

## 下载

最新版本均在百度网盘发布
[百度网盘](https://pan.baidu.com/s/1gyT0FU9rioaa77znhAUx2w)

## 反馈

如果有问题，欢迎创建issue或者联系我: dx_8820832#yeah.net（#→@），或者加QQ群874761809反馈
