#include "adduritask.h"
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QDir>
#include <QMessageBox>
#include "Download/dirselectwidget.h"
AddUriTask::AddUriTask(QWidget *parent) : CFramelessDialog(tr("Add URI"),parent,true)
{
    uriEdit=new QPlainTextEdit(this);
    uriEdit->setObjectName(QStringLiteral("UriEdit"));
    uriEdit->setPlaceholderText(tr("One link per line, multiple links separated by line breaks"));
    uriEdit->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    dirSelect=new DirSelectWidget(this);

    QVBoxLayout *dialogVLayout=new QVBoxLayout(this);
    dialogVLayout->addWidget(uriEdit);
    dialogVLayout->addWidget(dirSelect);
    resize(320*logicalDpiX()/96,360*logicalDpiY()/96);
}

void AddUriTask::onAccept()
{
    if(!dirSelect->isValid())
    {
        QMessageBox::information(this,tr("Error"),tr("Dir is invaild"));
        return;
    }
    this->dir=dirSelect->getDir();
    QStringList uris=uriEdit->toPlainText().split('\n',QString::SkipEmptyParts);
    for(auto iter=uris.begin();iter!=uris.end();)
    {
        if((*iter).trimmed().isEmpty())
            iter=uris.erase(iter);
        else
            iter++;
    }
    if(uris.count()==0)
    {
        QMessageBox::information(this,tr("Error"),tr("Uri is invaild"));
        return;
    }
    this->uriList=uris;
    CFramelessDialog::onAccept();
}
