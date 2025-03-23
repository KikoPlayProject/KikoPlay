#include "checkupdate.h"
#include <QLabel>
#include <QGridLayout>
#include <QFile>
#include "Common/kupdater.h"
#include "globalobjects.h"
CheckUpdate::CheckUpdate(QWidget *parent) : CFramelessDialog(tr("Check For Updates"),parent)
{
    QLabel *curVersionLabel = new QLabel(this);
    QLabel *newVersionLabel = new QLabel(this);
    QLabel *versionDescLabel = new QLabel(this);

    QGridLayout *versionGLayout=new QGridLayout(this);
    versionGLayout->setRowStretch(2, 1);
    versionGLayout->setColumnStretch(2, 1);
    versionGLayout->addWidget(curVersionLabel, 0, 1, 1, 2);
    versionGLayout->addWidget(newVersionLabel, 1, 1, 1, 2);
    versionGLayout->addWidget(versionDescLabel, 2, 1, 1, 2);

    curVersionLabel->setText(tr("Current Version: %1").arg(GlobalObjects::kikoVersion));
    newVersionLabel->setText(tr("Checking for updates..."));
    versionDescLabel->setOpenExternalLinks(true);
    versionDescLabel->setTextInteractionFlags(Qt::TextInteractionFlag::TextSelectableByMouse);
    versionDescLabel->hide();

    showBusyState(true);
    QObject::connect(KUpdater::instance(), &KUpdater::checkDone, this, [=](){
        auto updater = KUpdater::instance();
        if (updater->hasNewVersion())
        {
            newVersionLabel->setText(tr("Find new Version: %1  <a href=\"%2\">Click To Download</a>").arg(updater->version()).arg(updater->versionURL()));
            newVersionLabel->setOpenExternalLinks(true);
            versionDescLabel->setText(updater->vesrionDescription());
            versionDescLabel->show();
            resize(QSize(360*logicalDpiX()/96, 220*logicalDpiY()/96));
        }
        else
        {
            newVersionLabel->setText(updater->errInfo().isEmpty()? tr("Already Newast") : updater->errInfo());
        }
        showBusyState(false);
    });
    KUpdater::instance()->check();
}
