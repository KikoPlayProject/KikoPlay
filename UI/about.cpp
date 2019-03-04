#include "about.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QtCore>
About::About(QWidget *parent) : CFramelessDialog("",parent)
{
    QLabel *logo=new QLabel(this);
    logo->setPixmap(QPixmap(":/res/images/kikoplay-4.png"));
    logo->setAlignment(Qt::AlignCenter);
    QFile version(":/res/version.json");
    version.open(QIODevice::ReadOnly);
    QJsonObject curVersionObj = QJsonDocument::fromJson(version.readAll()).object();
    QString versionStr=curVersionObj.value("Version").toString();
    QLabel *info=new QLabel(tr("KikoPlay - A Full-featured Danmu Player<br/>"
                               "%1 (C) 2019 Kikyou <a href=\"https://github.com/Protostars/KikoPlay\">github</a><br/>"
							   "Exchange & BUG Report: 874761809(QQ Group)<br/>"
                               "KikoPlay is based on the following projects:").arg(versionStr),this);
    info->setOpenExternalLinks(true);
    info->setAlignment(Qt::AlignCenter);
    QPlainTextEdit *libsInfo=new QPlainTextEdit(this);
    libsInfo->setReadOnly(true);
    libsInfo->appendPlainText("Qt 5.12\n"
                              "libmpv 0.29.1\n"
                              "aria2 1.34\n"
                              "Qt-Nice-Frameless-Window\n"
                              "zlib 1.2.11\n"
                              "QHttpEngine 1.01\n"
                              "Lua 5.3");
    QVBoxLayout *abVLayout=new QVBoxLayout(this);
    abVLayout->addWidget(logo);
    abVLayout->addWidget(info);
    abVLayout->addWidget(libsInfo);
    setResizeable(false);
}
