#include "settings.h"
#include <QListWidget>
#include <QSettings>
#include <QLabel>
#include <QGridLayout>
#include <QStackedLayout>
#include <QScrollArea>
#include "UI/widgets/floatscrollbar.h"
#include "globalobjects.h"
#include "settings/generalpage.h"
#include "settings/danmupage.h"
#include "settings/playerpage.h"
#include "settings/playlistpage.h"
#include "settings/downloadpage.h"
#include "settings/lanserverpage.h"
#include "settings/scriptpage.h"
#include "settings/keyactionpage.h"
#include "settings/apppage.h"

Settings::Settings(Page page, QWidget *parent) : CFramelessDialog(tr("Settings"), parent)
{    
    QListWidget *pageList = new QListWidget(this);
    pageList->setFont(QFont(GlobalObjects::normalFont, 11));
    pageList->setObjectName(QStringLiteral("SettingPageList"));
    pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageList->setFixedWidth(180);

    pageSLayout = new QStackedLayout;

    const QStringList pageNames{
        tr("General"),
        tr("Danmu"),
        tr("Player"),
        tr("Playlist"),
        tr("Shortcut Key"),
        tr("Download"),
        tr("LAN Server"),
        tr("Script"),
        tr("Extension App"),
    };

    for (int i = PAGE_GENERAL; i < PAGE_STOP; ++i)
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
    gLayout->setHorizontalSpacing(10);
    gLayout->setContentsMargins(4, 4, 4, 4);
    gLayout->addWidget(pageList, 0, 0);
    gLayout->addLayout(pageSLayout, 0, 1);
    gLayout->setColumnStretch(1, 1);

    setSizeSettingKey("DialogSize/Setting",QSize(400, 400));
}

void Settings::onClose()
{
    for(auto &p : pages)
    {
        if(p) p->onClose();
    }
    CFramelessDialog::onClose();
}

SettingPage *Settings::getOrCreatePage(Page p)
{
    if(!pages[p])
    {
        switch (p)
        {
        case PAGE_GENERAL:
            pages[p] = new GeneralPage(this);
            break;
        case PAGE_DANMU:
            pages[p] = new DanmuPage(this);
            break;
        case PAGE_PLAYER:
            pages[p] = new PlayerPage(this);
            break;
        case PAGE_PLAYLIST:
            pages[p] = new PlaylistPage(this);
            break;
        case PAGE_KEYACTION:
            pages[p] = new KeyActionPage(this);
            break;
        case PAGE_DOWN:
            pages[p] = new DownloadPage(this);
            break;
        case PAGE_LAN:
            pages[p] = new LANServerPage(this);
            break;
        case PAGE_SCRIPT:
            pages[p] = new ScriptPage(this);
            break;
        case PAGE_APP:
            pages[p] = new AppPage(this);
            break;
        default:
            return nullptr;
        }
        QObject::connect(pages[p], &SettingPage::showBusyState, this, &Settings::showBusyState);
        QObject::connect(pages[p], &SettingPage::showMessage, this, &Settings::showMessage);

        QScrollArea *pageScrollArea = new QScrollArea(this);
        pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageScrollArea->setWidget(pages[p]);
        pageScrollArea->setWidgetResizable(true);
        new FloatScrollBar(pageScrollArea->verticalScrollBar(), pageScrollArea);
        QScopedPointer<QLayoutItem>(pageSLayout->replaceWidget(pageSLayout->widget(p), pageScrollArea));
    }
    return pages[p];
}

