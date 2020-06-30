#include "settings.h"
#include <QListWidget>
#include <QSettings>
#include <QLabel>
#include <QGridLayout>
#include <QStackedLayout>
#include "globalobjects.h"
#include "settings/stylepage.h"
#include "settings/mpvpage.h"
#include "settings/downloadpage.h"
#include "settings/lanserverpage.h"


Settings::Settings(Page page, QWidget *parent) : CFramelessDialog(tr("Settings"), parent, true)
{    
    QListWidget *pageList = new QListWidget(this);
    pageList->setFont(QFont("Microsoft YaHei UI",10));
    pageList->setObjectName(QStringLiteral("SettingPageList"));
    pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    pageSLayout = new QStackedLayout;
    pageSLayout->setContentsMargins(0, 0, 0, 0);

    QStringList pageNames{tr("Appearance"), tr("MPV Param"), tr("Download"), tr("LAN Server")};

    for(int i=PAGE_UI;i<PAGE_STOP;++i)
    {
        pages[Page(i)] =nullptr;
        pageList->addItem(pageNames[i]);
        pageSLayout->addWidget(new QWidget(this));
    }

    getOrCreatePage(page);
    pageSLayout->setCurrentIndex(page);
    pageList->setCurrentRow(page, QItemSelectionModel::Select);

    QObject::connect(pageList, &QListWidget::currentRowChanged, this, [this](int index){
        getOrCreatePage(Page(index));
        pageSLayout->setCurrentIndex(Page(index));
    });

    QGridLayout *gLayout = new QGridLayout(this);
    gLayout->setContentsMargins(0, 0, 0, 0);
    gLayout->addWidget(pageList, 0, 0);
    gLayout->addLayout(pageSLayout, 0, 1);
    gLayout->setColumnStretch(1, 1);

    resize(GlobalObjects::appSetting->value("DialogSize/Setting",QSize(400*logicalDpiX()/96,360*logicalDpiY()/96)).toSize());
}


void Settings::onAccept()
{
    for(auto &p : pages)
    {
        if(p) p->onClose();
    }
    GlobalObjects::appSetting->setValue("DialogSize/Setting",size());
    CFramelessDialog::onAccept();
}

void Settings::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/Setting",size());
    CFramelessDialog::onClose();
}

SettingPage *Settings::getOrCreatePage(Page p)
{
    if(!pages[p])
    {
        switch (p)
        {
        case PAGE_UI:
            pages[p] = new StylePage(this);
            break;
        case PAGE_MPV:
            pages[p] = new MPVPage(this);
            break;
        case PAGE_DOWN:
            pages[p] = new DownloadPage(this);
            break;
        case PAGE_LAN:
            pages[p] = new LANServerPage(this);
            break;
        default:
            return nullptr;
        }
        QScopedPointer<QLayoutItem>(pageSLayout->replaceWidget(pageSLayout->widget(p), pages[p]));
    }
    return pages[p];
}

