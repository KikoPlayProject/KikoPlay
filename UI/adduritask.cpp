#include "adduritask.h"
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QDir>
#include "widgets/dirselectwidget.h"
#include "Common/notifier.h"
AddUriTask::AddUriTask(QWidget *parent, const QStringList &uris, const QString &path) : CFramelessDialog(tr("Add URI"),parent,true,true,false)
{
    uriEdit=new QPlainTextEdit(this);
    uriEdit->setObjectName(QStringLiteral("UriEdit"));
    uriEdit->setPlaceholderText(tr("One link per line, multiple links separated by line breaks"));
    uriEdit->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    if(uris.count()>0)
    {
        uriEdit->setPlainText(uris.join('\n'));
    }

    dirSelect=new DirSelectWidget(this);
    if(!path.isEmpty())dirSelect->setDir(path);
    QVBoxLayout *dialogVLayout=new QVBoxLayout(this);
    dialogVLayout->addWidget(uriEdit);
    dialogVLayout->addWidget(dirSelect);
    resize(320*logicalDpiX()/96,360*logicalDpiY()/96);
}

void AddUriTask::onAccept()
{
    if(!dirSelect->isValid())
    {
        showMessage(tr("Dir is invaild"), NM_ERROR | NM_HIDE);
        return;
    }
    this->dir=dirSelect->getDir();
    QStringList uris=uriEdit->toPlainText().split('\n',Qt::SkipEmptyParts);
    for(auto iter=uris.begin();iter!=uris.end();)
    {
        if((*iter).trimmed().isEmpty())
            iter=uris.erase(iter);
        else
            iter++;
    }
    if(uris.count()==0)
    {
        showMessage(tr("Uri is invaild"), NM_ERROR | NM_HIDE);
        return;
    }
    this->uriList=uris;
    CFramelessDialog::onAccept();
}
