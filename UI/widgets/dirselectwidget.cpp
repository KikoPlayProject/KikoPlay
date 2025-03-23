#include "dirselectwidget.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QStorageInfo>
#include <QSettings>
#include <QApplication>
#include "Download/util.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
DirSelectWidget::DirSelectWidget(QWidget *parent) : QWidget(parent), dirChanged(false)
{
    dirList = GlobalObjects::appSetting->value("Download/PathList", QStringList()).toStringList();
    dirEdit = new ElaComboBox(this);
    static_cast<ElaComboBox *>(dirEdit)->setEditable(true);
    dirEdit->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    dirEdit->addItems(dirList);
    dirEdit->setCurrentIndex(0);
    dirEdit->setFont(QFont(GlobalObjects::normalFont, 13));


    QLabel *spaceTip=new QLabel(this);
    spaceTip->setObjectName(QStringLiteral("SpaceTip"));
    QHBoxLayout *dirHLayout=new QHBoxLayout();
    dirHLayout->addStretch(1);
    dirHLayout->addWidget(spaceTip);
    dirHLayout->addSpacing(32);
    dirHLayout->setContentsMargins(0, 0, 0, 0);
    spaceTip->setFont(QFont(GlobalObjects::normalFont, 8));
    dirEdit->setLayout(dirHLayout);
    freeSpace=getAvailableBytes(getDir());
    spaceTip->setText(formatSize(false,freeSpace));

    QObject::connect(dirEdit,&QComboBox::editTextChanged,[this,spaceTip](const QString &dir){
        freeSpace=getAvailableBytes(dir);
        spaceTip->setText(formatSize(false,freeSpace));
		dirChanged = true;
    });

    QPushButton *selectDir = new KPushButton(this);
    GlobalObjects::iconfont->setPointSize(18);
    selectDir->setFont(*GlobalObjects::iconfont);
    selectDir->setText(QChar(0xe616));
    selectDir->setFixedHeight(40);

    QObject::connect(selectDir,&QPushButton::clicked,[this](){
        QString directory = QFileDialog::getExistingDirectory(this,
                                    tr("Select folder"), "",
                                    QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
        if (!directory.isEmpty())
        {
            dirEdit->setCurrentText(directory);
        }
    });

    QHBoxLayout *dirGroupHLayout=new QHBoxLayout(this);
    dirGroupHLayout->setContentsMargins(0,0,0,0);
    dirGroupHLayout->setSpacing(0);
    dirGroupHLayout->addWidget(dirEdit);
    dirGroupHLayout->addWidget(selectDir);
}

DirSelectWidget::~DirSelectWidget()
{
    QString newDir(getDir());
    if(!dirList.contains(newDir))
    {
        if(dirList.count()>4)
            dirList.pop_back();
        dirList.prepend(newDir);
    }
	else
	{
		dirList.move(dirList.indexOf(newDir), 0);
	}
	if (dirChanged)
	{
		GlobalObjects::appSetting->setValue("Download/PathList", dirList);
	}
}

void DirSelectWidget::setDir(const QString &dir)
{
    dirEdit->setCurrentText(dir);
}

QString DirSelectWidget::getDir() const
{
    QDir dir(dirEdit->currentText());
    return dir.absolutePath();
}

bool DirSelectWidget::isValid()
{
    QDir dir(dirEdit->currentText());
    if(dirEdit->currentText().trimmed().isEmpty() || !dir.exists())
    {
        return false;
    }
    return true;
}

qint64 DirSelectWidget::getAvailableBytes(const QString &dir)
{
    QStorageInfo storage(dir);
    qint64 availableBytes=storage.bytesAvailable();
    if(availableBytes<0)availableBytes=0;
    return availableBytes;
}
