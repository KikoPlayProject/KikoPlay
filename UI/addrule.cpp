#include "addrule.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QTreeView>
#include <QDateTime>
#include <QSettings>
#include "globalobjects.h"
#include "Download/autodownloadmanager.h"
#include "widgets/dirselectwidget.h"
#include "Download/Script/scriptmanager.h"

AddRule::AddRule(DownloadRule *rule, QWidget *parent) : CFramelessDialog(tr("Add Rule"),parent, true, true, false)
{
    if(rule)
    {
        addRule=false;
        curRule=rule;
        setTitle(tr("Edit Rule"));
    }
    else
    {
        addRule=true;
        curRule=new DownloadRule();
    }

    QLabel *nameLabel=new QLabel(tr("Title:"), this);
    nameEdit=new QLineEdit(curRule->name, this);

    QLabel *searchWordLabel=new QLabel(tr("Search Word:"),this);
    searchWordEdit=new QLineEdit(curRule->searchWord,this);

    QLabel *filterWordLabel=new QLabel(tr("Filter Word(Separator: ';'):"),this);
    filterWordEdit=new QLineEdit(curRule->filterWord,this);

    QLabel *scriptLabel=new QLabel(tr("Search Script:"),this);
    scriptIdCombo=new QComboBox(this);
    for(auto &item:GlobalObjects::scriptManager->getScriptList())
    {
        scriptIdCombo->addItem(item.title,item.id);
    }

    int normalScriptIndex=scriptIdCombo->findData(GlobalObjects::scriptManager->getNormalScriptId());
    scriptIdCombo->setCurrentIndex(addRule?normalScriptIndex:scriptIdCombo->findData(curRule->scriptId));

    QLabel *downloadLabel=new QLabel(tr("After discovering new uri:"),this);
    downloadCombo=new QComboBox(this);
    downloadCombo->addItems(QStringList({tr("Add download task"), tr("Put it in the staging area")}));
    downloadCombo->setCurrentIndex(curRule->download?0:1);

    QLabel *minMaxSizeLabel=new QLabel(tr("Min/Max Size(MB,0: No limit):"),this);
    minSizeSpin=new QSpinBox(this);
    minSizeSpin->setRange(0, INT_MAX);
    minSizeSpin->setValue(curRule->minSize);
    maxSizeSpin=new QSpinBox(this);
    maxSizeSpin->setMinimum(0);
    maxSizeSpin->setRange(0, INT_MAX);
    maxSizeSpin->setValue(curRule->maxSize);

    QLabel *searchIntervalSpinLabel=new QLabel(tr("Search Interval(min):"),this);
    searchIntervalSpin=new QSpinBox(this);
    searchIntervalSpin->setRange(1, INT_MAX);
    searchIntervalSpin->setValue(curRule->searchInterval);

    QLabel *filePathLabel=new QLabel(tr("File Path:"),this);
    filePathSelector=new DirSelectWidget(this);
    filePathSelector->setDir(curRule->filePath);

    QLabel *previewLabel=new QLabel(tr("Preview:"), this);
    preview=new QTreeView(this);
    previewModel = new PreviewModel(this);
    preview->setRootIsDecorated(false);
    preview->setSelectionMode(QAbstractItemView::ExtendedSelection);
    preview->setAlternatingRowColors(true);
    preview->setModel(previewModel);
    QObject::connect(previewModel, &PreviewModel::showError, this, [this](const QString &err){
        showMessage(err, 1);
    });

    QGridLayout *addRuleGLayout=new QGridLayout(this);
    addRuleGLayout->addWidget(nameLabel, 0, 0);
    addRuleGLayout->addWidget(nameEdit, 0, 1);
    addRuleGLayout->addWidget(searchWordLabel, 1, 0);
    addRuleGLayout->addWidget(searchWordEdit, 1, 1);
    addRuleGLayout->addWidget(filterWordLabel, 2, 0);
    addRuleGLayout->addWidget(filterWordEdit, 2, 1);
    addRuleGLayout->addWidget(scriptLabel, 3, 0);
    addRuleGLayout->addWidget(scriptIdCombo, 3, 1);
    addRuleGLayout->addWidget(downloadLabel, 4, 0);
    addRuleGLayout->addWidget(downloadCombo, 4, 1);
    addRuleGLayout->addWidget(minMaxSizeLabel, 5, 0);
    QHBoxLayout *minMaxHLayout = new QHBoxLayout();
    minMaxHLayout->setContentsMargins(0,0,0,0);
    minMaxHLayout->addWidget(minSizeSpin);
    minMaxHLayout->addWidget(maxSizeSpin);
    addRuleGLayout->addLayout(minMaxHLayout, 5, 1);
    addRuleGLayout->addWidget(searchIntervalSpinLabel, 6, 0);
    addRuleGLayout->addWidget(searchIntervalSpin, 6, 1);
    addRuleGLayout->addWidget(filePathLabel, 7, 0);
    addRuleGLayout->addWidget(filePathSelector, 7, 1);
    addRuleGLayout->addWidget(previewLabel, 8, 0);
    addRuleGLayout->addWidget(preview, 9,0,1,2);

    addRuleGLayout->setRowStretch(9, 1);
    addRuleGLayout->setColumnStretch(1,1);

    setupSignals();
    resize(GlobalObjects::appSetting->value("DialogSize/AddRule",QSize(400*logicalDpiX()/96,600*logicalDpiY()/96)).toSize());
}

void AddRule::setupSignals()
{
    QObject::connect(searchWordEdit, &QLineEdit::editingFinished, this, [this](){
       if(searchWordEdit->text().trimmed().isEmpty()) return;
       showBusyState(true);
       previewModel->search(searchWordEdit->text().trimmed(), scriptIdCombo->currentData().toString());
       showBusyState(false);
       previewModel->filter(filterWordEdit->text().trimmed(), minSizeSpin->value(), maxSizeSpin->value());
    });
    QObject::connect(filterWordEdit, &QLineEdit::editingFinished, this, [this](){
        previewModel->filter(filterWordEdit->text().trimmed(), minSizeSpin->value(), maxSizeSpin->value());
    });
    QObject::connect(minSizeSpin, &QSpinBox::editingFinished, this, [this](){
        previewModel->filter(filterWordEdit->text().trimmed(), minSizeSpin->value(), maxSizeSpin->value());
    });
    QObject::connect(minSizeSpin, &QSpinBox::editingFinished, this, [this](){
        previewModel->filter(filterWordEdit->text().trimmed(), minSizeSpin->value(), maxSizeSpin->value());
    });
}

void AddRule::onAccept()
{
    if(nameEdit->text().trimmed().isEmpty())
    {
        showMessage(tr("Title can't be empty"), 1);
        return;
    }
    if(searchWordEdit->text().trimmed().isEmpty())
    {
        showMessage(tr("Search Word can't be empty"), 1);
        return;
    }
    if(!filePathSelector->isValid())
    {
        showMessage(tr("File Path is invaild"), 1);
        return;
    }
    if(minSizeSpin->value()>=maxSizeSpin->value() && maxSizeSpin->value()>0)
    {
        showMessage(tr("Min Size should be less than Max Size"), 1);
        return;
    }
    curRule->name=nameEdit->text().trimmed();
    bool searchWordChanged=(curRule->searchWord!=searchWordEdit->text().trimmed());
    curRule->searchWord=searchWordEdit->text().trimmed();
    curRule->filterWord=filterWordEdit->text().trimmed();
    curRule->minSize=minSizeSpin->value();
    curRule->maxSize=maxSizeSpin->value();
    curRule->filePath=filePathSelector->getDir();
    bool scriptChanged=(curRule->scriptId!=scriptIdCombo->currentData().toString());
    curRule->scriptId=scriptIdCombo->currentData().toString();
    curRule->searchInterval=searchIntervalSpin->value();
    curRule->download=downloadCombo->currentIndex()==0?true:false;
    if(searchWordChanged || scriptChanged)
    {
        showBusyState(true);
        int pageCount;
        QList<ResItem> resList;
        QString errInfo = GlobalObjects::scriptManager->search(curRule->scriptId, curRule->searchWord, 1, pageCount, resList);
        if(errInfo.isEmpty())
        {
            for(int i=0; i<3 && i<resList.size(); ++i)
            {
                curRule->lastCheckPosition<<resList.at(i).title;
            }
            curRule->lastCheckTime=QDateTime::currentDateTime().toSecsSinceEpoch();
        }
        showBusyState(false);
    }
    GlobalObjects::appSetting->setValue("DialogSize/AddRule",size());
    CFramelessDialog::onAccept();
}

void AddRule::onClose()
{
    if(addRule) delete curRule;
    GlobalObjects::appSetting->setValue("DialogSize/AddRule",size());
    CFramelessDialog::onClose();
}


float PreviewModel::getSize(const QString &sizeStr)
{
    static QRegExp re("(\\d+(?:\\.\\d+)?)\\s*([KkMmGgTt])i?B|b");
    if(re.indexIn(sizeStr)==-1) return 0.f;
    if(re.matchedLength()!=sizeStr.length()) return 0.f;
    QStringList captured=re.capturedTexts();
    float size = captured[1].toFloat();
    char s = captured[2][0].toUpper().toLatin1();
    switch (s)
    {
    case 'K':
        size /= 1024;
        break;
    case 'T':
        size *= 1024;
    case 'G':
        size *= 1024;
    default:
        break;
    }
    return size;
}

bool PreviewModel::checkSize(int size, int min, int max)
{
    if(min==0 && max==0) return true;
    if(min>0 && max==0)
    {
        return size>min;
    }
    if(min==0 && max>0)
    {
        return size<max;
    }
    return size>min && size<max;
}

void PreviewModel::search(const QString &searchWord, const QString &scriptId)
{
    if(searchWord==lastSearchWord) return;
    int pageCount;
    QList<ResItem> resList;
    QString errInfo = GlobalObjects::scriptManager->search(scriptId, searchWord, 1, pageCount, resList);
    beginResetModel();
    searchResults.clear();
    if(errInfo.isEmpty())
    {
        for(const ResItem &item:resList)
        {
            SearchItem sItem({item.title, static_cast<int>(getSize(item.size)), true});
            searchResults<<sItem;
        }
        lastSearchWord=searchWord;
    }
    else
    {
        emit showError(errInfo);
    }
    endResetModel();
}

void PreviewModel::filter(const QString &filterWord, int minSize, int maxSize)
{
    QStringList filters(filterWord.split(';'));
    QList<QRegExp> filterRegExps;
    for(const QString &s:filters)
        filterRegExps<<QRegExp(s);
    int i=0;
    for(auto &item:searchResults)
    {
        bool misMatch=false;
        for(auto &re:filterRegExps)
        {
            if(!item.title.contains(re))
            {
                misMatch=true;
                break;
            }
        }
        item.matched=!misMatch && checkSize(item.size, minSize, maxSize);
        QModelIndex index(createIndex(i, 0));
        emit dataChanged(index, index.sibling(i,1));
        ++i;
    }
}

QVariant PreviewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int col=index.column();
    const SearchItem &item=searchResults.at(index.row());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case 0:
            return item.size;
        case 1:
            return item.title;
        }
    }
    else if(role==Qt::ToolTipRole)
    {
        if(col==1)
            return item.title;
    }
    else if(role==Qt::BackgroundRole)
    {
        static QBrush matchedBrush(QColor(176,245,69));
        if(item.matched) return matchedBrush;
    }
    return QVariant();
}

QVariant PreviewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Size(MB)"),tr("Title")});
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<2)return headers.at(section);
    }
    return QVariant();
}
