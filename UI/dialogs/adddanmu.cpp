#include "adddanmu.h"

#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QHeaderView>
#include <QButtonGroup>
#include <QAction>
#include <QFileDialog>

#include "Common/threadtask.h"
#include "Play/Danmu/danmuprovider.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "Play/Playlist/playlistitem.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaPivot.h"
#include "UI/widgets/elidedlabel.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"
#include "selectepisodedialog.h"
#include "danmuview.h"
#include "Play/Danmu/blocker.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/danmuscript.h"
#include "UI/widgets/scriptsearchoptionpanel.h"
#include "UI/widgets/ktagpanel.h"
#include "globalobjects.h"
#include "Common/notifier.h"
namespace
{

    class RelWordCache
    {
    public:
        explicit RelWordCache():cacheChanged(false)
        {
             QFile cacheFile(GlobalObjects::context()->dataPath + "relCache");
             if(cacheFile.open(QIODevice::ReadOnly))
             {
                QDataStream fs(&cacheFile);
                fs>>relWordHash;
             }
        }
        QStringList get(const QString &word)
        {
            return relWordHash.value(word);
        }
        void put(const QString &word, const QString &rel)
        {
            if(!relWordHash.contains(word))
            {
                relWordHash.insert(word, {rel});
            }
            else
            {
                QStringList &rels=relWordHash[word];
                if(rels.contains(rel)) rels.removeOne(rel);
                rels.push_front(rel);
            }
            cacheChanged=true;
        }
        void save()
        {
            if(cacheChanged)
            {
                QFile cacheFile(GlobalObjects::context()->dataPath + "relCache");
                if(cacheFile.open(QIODevice::WriteOnly))
                {
                    QDataStream fs(&cacheFile);
                    for(auto &list:relWordHash)
                    {
                        if(list.count()>5)
                        {
                            list.erase(list.begin()+1, list.end());
                        }
                    }
                    fs<<relWordHash;
                }
            }
            cacheChanged=false;
        }
    private:
        QHash<QString, QStringList> relWordHash;
        bool cacheChanged;
    };
    RelWordCache *relCache=nullptr;
}
AddDanmu::AddDanmu(const PlayListItem *item,QWidget *parent,bool autoPauseVideo,const QStringList &poolList) : CFramelessDialog(tr("Add Danmu"),parent,true,true,autoPauseVideo),
    curItem(item), danmuPools(poolList), processCounter(0)
{
    if (!relCache) relCache=new RelWordCache();
    Pool *pool = nullptr;
    if(item) pool=GlobalObjects::danmuManager->getPool(item->poolID, false);
    defaultPool = pool ? pool->toEp().toString() : "";

    QVBoxLayout *danmuVLayout=new QVBoxLayout(this);
    auto margins = danmuVLayout->contentsMargins();
    margins.setTop(0);
    danmuVLayout->setContentsMargins(margins);

    tab = new ElaPivot(this);
    tab->appendPivot(tr("Search"));
    tab->appendPivot(tr("URL"));
    tab->appendPivot(tr("Staging"));
    tab->setCurrentIndex(0);

    danmuVLayout->addWidget(tab);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage());
    contentStackLayout->addWidget(setupURLPage());
    contentStackLayout->addWidget(setupSelectedPage());
    danmuVLayout->addLayout(contentStackLayout);

    QObject::connect(tab, &ElaPivot::pCurrentIndexChanged, this, [=](){
        contentStackLayout->setCurrentIndex(tab->getCurrentIndex());
    });

    QString itemInfo(item?(item->animeTitle.isEmpty()?item->title:QString("%1-%2").arg(item->animeTitle).arg(item->title)):"");
    QLabel *itemInfoLabel=new QLabel(itemInfo,this);
    itemInfoLabel->setFont(QFont(GlobalObjects::normalFont,10,QFont::Bold));
    itemInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    danmuVLayout->addWidget(itemInfoLabel);
    if (!item) itemInfoLabel->hide();

    keywordEdit->setText(item?(item->animeTitle.isEmpty()?item->title:item->animeTitle):"");
    keywordEdit->installEventFilter(this);
    searchButton->setAutoDefault(false);
    searchButton->setDefault(false);
    if (item && !item->animeTitle.isEmpty())
    {
        themeWord=item->animeTitle;
        relWordWidget->setRelWordList(relCache->get(themeWord));
    }
    setSizeSettingKey("DialogSize/AddDanmu", QSize(800, 600));
}

void AddDanmu::search()
{
    QString keyword=keywordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    QString tmpProviderId=sourceCombo->currentData().toString();
    if(tmpProviderId.isEmpty()) return;
    beginProcrss();
    searchResultWidget->setEnabled(false);
    QList<DanmuSource> results;
    QMap<QString, QString> searchOptions;
    if(scriptOptionPanel->hasOptions() && scriptOptionPanel->changed())
    {
        searchOptions = scriptOptionPanel->getOptionVals();
    }
    showMessage(tr("Searching..."),  NM_PROCESS | NM_SHOWCANCEL);
    TaskContext ctx(this);
    QObject::connect(this, &CFramelessDialog::cancelClicked, &ctx, &TaskContext::cancel);
    auto ret = GlobalObjects::danmuProvider->danmuSearch(tmpProviderId, keyword, searchOptions, results, &ctx);
    showMessage(tr("Down"),  NM_HIDE | NM_HIDE_ONCE);
    if(ret)
    {
        if(!themeWord.isEmpty() && themeWord!=keyword)
        {
            relCache->put(themeWord, keyword);
        }
        providerId=tmpProviderId;
        searchResultWidget->clear();
        searchResultWidget->setEnabled(true);
        for(auto &item: results)
        {
            SearchItemWidget *itemWidget=new SearchItemWidget(item);
            QObject::connect(itemWidget, &SearchItemWidget::addSearchItem, itemWidget, [this](SearchItemWidget *item){
                beginProcrss();
                searchResultWidget->setEnabled(false);
                QList<DanmuSource> results;
                auto ret = GlobalObjects::danmuProvider->getEpInfo(&item->source, results);
                if(ret) addSearchItem(results);
                else showMessage(ret.info, NM_ERROR | NM_HIDE);
                endProcess();
                searchResultWidget->setEnabled(true);
            });
            QListWidgetItem *listItem=new QListWidgetItem(searchResultWidget);
            searchResultWidget->setItemWidget(listItem,itemWidget);
            listItem->setSizeHint(itemWidget->sizeHint());
            QCoreApplication::processEvents();
        }
    } else {
        showMessage(ret.info, NM_ERROR | NM_HIDE);
    }
    searchResultWidget->setEnabled(true);
    endProcess();
}

int AddDanmu::addSearchItem(QList<DanmuSource> &sources)
{
    if(sources.empty()) return 0;
    ScriptState retState(ScriptState::S_NORM);
    int succNum = 0;
    auto addSearchSource = [=](const DanmuSource &src){
        SearchDanmuInfo danmuInfo;
        danmuInfo.pool = defaultPool;
        DanmuSource *nSrc = nullptr;
        auto ret = GlobalObjects::danmuProvider->downloadDanmu(&src, danmuInfo.danmus, &nSrc);
        if (ret)
        {
            int srcCount = danmuInfo.danmus.count();
            GlobalObjects::blocker->preFilter(danmuInfo.danmus);
            int filterCount = srcCount - danmuInfo.danmus.count();
            if (filterCount > 0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));

            danmuInfo.src = nSrc? *nSrc : src;
            danmuInfo.src.count = danmuInfo.danmus.count();
            danmuInfoList.append(danmuInfo);

            DanmuItemWidget *itemWidget = new DanmuItemWidget(danmuInfoList, danmuInfoList.size() - 1, danmuPools);
            QListWidgetItem *listItem = new QListWidgetItem(selectedDanmuView);
            selectedDanmuView->setItemWidget(listItem, itemWidget);
            listItem->setSizeHint(itemWidget->sizeHint());
            QObject::connect(itemWidget, &DanmuItemWidget::setPoolIndexFrom, this, [=](int poolIndex){
                int row = selectedDanmuView->row(listItem);
                setPoolIdInSequence(row, poolIndex);
            });
        }
        if (nSrc) delete nSrc;
        return ret;
    };
    if(sources.size() == 1)
    {
        auto ret = addSearchSource(sources.front());
        if (ret)
        {
            succNum++;
        }
        else
        {
            showMessage(ret.info, NM_ERROR | NM_HIDE);
        }
    }
    else
    {
        SelectEpisode selectEpisode(sources, this);
        if (QDialog::Accepted == selectEpisode.exec())
        {
            for (auto &sourceItem:sources)
            {
                if (addSearchSource(sourceItem))
                {
                    ++succNum;
                }
            }
        }
    }
    tab->setPivotText(2, tr("Staging(%1)").arg(danmuInfoList.count()));
    return succNum;
}

void AddDanmu::addURL()
{
    const QStringList urls = urlEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    if(urls.isEmpty()) return;
    addUrlButton->setEnabled(false);
    urlEdit->setEnabled(false);
    beginProcrss();
    int addNum = 0;
    TaskContext ctx(this);
    QObject::connect(this, &CFramelessDialog::cancelClicked, &ctx, &TaskContext::cancel);
    for (const QString &url : urls)
    {
        if (ctx.isCancelRequested()) break;
        QList<DanmuSource> results;
        showMessage(tr("Adding: %1").arg(url), NM_PROCESS | NM_SHOWCANCEL);
        auto ret = GlobalObjects::danmuProvider->getURLInfo(url, results, &ctx);
        if (!ret)
        {
            showMessage(ret.info, NM_ERROR | NM_PROCESS);
        }
        else
        {
            if (addSearchItem(results) > 0) ++addNum;
        }
    }
    showMessage(tr("Add %1 URL(s)").arg(addNum), NM_HIDE);
    addUrlButton->setEnabled(true);
    urlEdit->setEnabled(true);
    endProcess();
}

QWidget *AddDanmu::setupSearchPage()
{
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(QFont(GlobalObjects::normalFont,10));
    sourceCombo=new ElaComboBox(searchPage);
    scriptOptionPanel = new ScriptSearchOptionPanel(searchPage);
    QObject::connect(sourceCombo, &QComboBox::currentTextChanged, this, [=](const QString &){
        QString curId = sourceCombo->currentData().toString();
        scriptOptionPanel->setScript(GlobalObjects::scriptManager->getScript(curId));
        if(scriptOptionPanel->hasOptions()) scriptOptionPanel->show();
        else scriptOptionPanel->hide();
    });
    const QString defaultScriptId = GlobalObjects::appSetting->value("Script/DefaultDanmuScript").toString();
    int index = 0, defaultIndex = -1;
    for(const auto &p : GlobalObjects::danmuProvider->getSearchProviders())
    {
        sourceCombo->addItem(p.first, p.second);  //p: <name, id>
        if (p.second == defaultScriptId)
        {
            defaultIndex = index;
        }
        ++index;
    }
    if (defaultIndex != -1) sourceCombo->setCurrentIndex(defaultIndex);
    keywordEdit = new ElaLineEdit(searchPage);
    searchButton = new KPushButton(tr("Search"), searchPage);
    QObject::connect(searchButton, &QPushButton::clicked, this, &AddDanmu::search);
    relWordWidget=new RelWordWidget(this);
    searchResultWidget = new QListWidget(searchPage);
    searchResultWidget->setObjectName(QStringLiteral("DanmuSearchResList"));
    searchResultWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QObject::connect(relWordWidget, &RelWordWidget::relWordClicked, [this](const QString &relWord){
       keywordEdit->setText(relWord);
       search();
    });
    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->setContentsMargins(0, 0, 0, 0);
    searchPageGLayout->addWidget(sourceCombo,0,0);
    searchPageGLayout->addWidget(keywordEdit,0,1);
    searchPageGLayout->addWidget(searchButton,0,2);
    searchPageGLayout->addWidget(scriptOptionPanel,1,0,1,3);
    searchPageGLayout->addWidget(relWordWidget,2,0,1,3);
    searchPageGLayout->addWidget(searchResultWidget,3,0,1,3);
    searchPageGLayout->setColumnStretch(1,1);
    searchPageGLayout->setRowStretch(3,1);

    return searchPage;
}

QWidget *AddDanmu::setupURLPage()
{
    QWidget *urlPage=new QWidget(this);
    urlPage->setFont(QFont(GlobalObjects::normalFont,10));

    urlEdit = new KPlainTextEdit(urlPage);
    urlEdit->setPlaceholderText(tr("One URL per line"));

    addUrlButton = new KPushButton(tr("Add URL"), urlPage);
    addUrlButton->setMinimumWidth(150);
    addUrlButton->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    QObject::connect(addUrlButton,&QPushButton::clicked,this,&AddDanmu::addURL);

    QLabel *urlTipLabel=new QLabel(tr("Supported URL:"),urlPage);
    QTextEdit *supportUrlInfo=new QTextEdit(urlPage);
    supportUrlInfo->setFont(QFont(GlobalObjects::normalFont,10));
    const QList<QPair<QString, QStringList>> scriptSampleURLS = GlobalObjects::danmuProvider->getSampleURLs();
    for(const auto &pair : scriptSampleURLS)
    {
        supportUrlInfo->setFontWeight(QFont::Bold);
        supportUrlInfo->append(pair.first);
        supportUrlInfo->setFontWeight(QFont::Light);
        supportUrlInfo->append(pair.second.join('\n'));
    }
    supportUrlInfo->setReadOnly(true);
    QTextCursor cursor = supportUrlInfo->textCursor();
    cursor.movePosition(QTextCursor::Start);
    supportUrlInfo->setTextCursor(cursor);

    QVBoxLayout *localVLayout=new QVBoxLayout(urlPage);
    localVLayout->setContentsMargins(0, 0, 0, 0);
    localVLayout->addWidget(urlEdit);
    localVLayout->addWidget(addUrlButton);
    localVLayout->addSpacing(10);
    localVLayout->addWidget(urlTipLabel);
    localVLayout->addWidget(supportUrlInfo);

    return urlPage;
}

QWidget *AddDanmu::setupSelectedPage()
{
    QWidget *selectedPage = new QWidget(this);
    QPushButton *addLocalSrcBtn = new KPushButton(tr("Add Local Danmu File"), selectedPage);
    selectedPage->setFont(QFont(GlobalObjects::normalFont, 12));
    QLabel *tipLabel = new QLabel(tr("Select danmu you want to add:"), selectedPage);
    selectedDanmuView = new QListWidget(selectedPage);
    selectedDanmuView->setObjectName(QStringLiteral("SelectedDanmuView"));
    selectedDanmuView->setFont(selectedPage->font());
    selectedDanmuView->setDragEnabled(true);
    selectedDanmuView->setAcceptDrops(true);
    selectedDanmuView->setDragDropMode(QAbstractItemView::InternalMove);
    selectedDanmuView->setDropIndicatorShown(true);
    selectedDanmuView->setDefaultDropAction(Qt::MoveAction);
    selectedDanmuView->setSelectionMode(QListWidget::SingleSelection);
    selectedDanmuView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    QGridLayout *spGLayout = new QGridLayout(selectedPage);
    spGLayout->setContentsMargins(0, 0, 0, 0);
    spGLayout->addWidget(tipLabel, 0, 0);
    spGLayout->addWidget(addLocalSrcBtn, 0, 2);
    spGLayout->addWidget(selectedDanmuView, 1, 0, 1, 3);
    spGLayout->setRowStretch(1, 1);
    spGLayout->setColumnStretch(1, 1);

    QObject::connect(addLocalSrcBtn, &QPushButton::clicked, this, [this](){
        const QString dialogPath = (!curItem || curItem->path.isEmpty()) ? "" : QFileInfo(curItem->path).absolutePath();
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select Xml File"), dialogPath, "Xml File(*.xml) ");
        if (!files.isEmpty())
        {
            for (auto &file: files)
            {
                SearchDanmuInfo danmuInfo;
                danmuInfo.pool = defaultPool;

                LocalProvider::LoadXmlDanmuFile(file, danmuInfo.danmus);
                int srcCount = danmuInfo.danmus.count();
                GlobalObjects::blocker->preFilter(danmuInfo.danmus);
                int filterCount = srcCount - danmuInfo.danmus.count();
                if (filterCount > 0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));

                danmuInfo.src.scriptData = file;
                danmuInfo.src.title = file.mid(file.lastIndexOf('/')+1);
                danmuInfo.src.count = danmuInfo.danmus.count();

                danmuInfoList.append(danmuInfo);

                DanmuItemWidget *itemWidget = new DanmuItemWidget(danmuInfoList, danmuInfoList.size() - 1, danmuPools);
                QListWidgetItem *listItem = new QListWidgetItem(selectedDanmuView);
                selectedDanmuView->setItemWidget(listItem, itemWidget);
                listItem->setSizeHint(itemWidget->sizeHint());
                QObject::connect(itemWidget, &DanmuItemWidget::setPoolIndexFrom, this, [=](int poolIndex){
                    int row = selectedDanmuView->row(listItem);
                    setPoolIdInSequence(row, poolIndex);
                });

                tab->setPivotText(2, tr("Staging(%1)").arg(danmuInfoList.count()));
            }
        }
    });

    return selectedPage;
}

void AddDanmu::beginProcrss()
{
    processCounter++;
    showBusyState(true);
    searchButton->setEnabled(false);
    relWordWidget->setEnabled(false);
    keywordEdit->setEnabled(false);
}

void AddDanmu::endProcess()
{
    processCounter--;
    if(processCounter==0)
    {
        searchButton->setEnabled(true);
        keywordEdit->setEnabled(true);
        relWordWidget->setEnabled(true);
        showBusyState(false);
    }
}

void AddDanmu::setPoolIdInSequence(int row, int poolIndex)
{
    for (int i = row + 1; i < selectedDanmuView->count(); ++i)
    {
        auto danmuWidget = static_cast<DanmuItemWidget *>(selectedDanmuView->itemWidget(selectedDanmuView->item(i)));
        if (danmuWidget->checked())
        {
            danmuWidget->setPoolIndex(++poolIndex);
        }
    }
}

void AddDanmu::onAccept()
{
    QList<SearchDanmuInfo> finalInfoList;
    for (int i = 0; i < selectedDanmuView->count(); ++i)
    {
        auto danmuWidget = static_cast<DanmuItemWidget *>(selectedDanmuView->itemWidget(selectedDanmuView->item(i)));
        int listIndex = danmuWidget->listIndex();
        if (danmuWidget->checked())
        {
            finalInfoList.append(danmuInfoList[listIndex]);
        }
        else
        {
            qDeleteAll(danmuInfoList[listIndex].danmus);
        }
    }
    danmuInfoList.swap(finalInfoList);
    relCache->save();
    if (!providerId.isEmpty()) GlobalObjects::appSetting->setValue("Script/DefaultDanmuScript", providerId);
    CFramelessDialog::onAccept();
}

void AddDanmu::onClose()
{
    for (auto &info : danmuInfoList)
    {
        qDeleteAll(info.danmus);
    }
    relCache->save();
    if (!providerId.isEmpty()) GlobalObjects::appSetting->setValue("Script/DefaultDanmuScript", providerId);
    CFramelessDialog::onClose();
}

bool AddDanmu::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key()==Qt::Key_Enter || keyEvent->key()==Qt::Key_Return)
        {
            if(watched == keywordEdit)
            {
                search();
                return true;
            }
            else if(watched==urlEdit)
            {
                addURL();
                return true;
            }
        }
        return false;
    }
    return CFramelessDialog::eventFilter(watched, event);
}

SearchItemWidget::SearchItemWidget(const DanmuSource &item):source(item)
{
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QLabel *titleLabel=new QLabel(this);
	QLabel *descLabel = new QLabel(this);
    titleLabel->setToolTip(item.title);
	titleLabel->adjustSize();
    titleLabel->setText(QString("<font size=\"5\" face=\"%2\" color=\"#f0a277\">%1</font>").arg(item.title, GlobalObjects::normalFont));
    titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    descLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    descLabel->setText(QString("<font size=\"3\" color=\"#d0d0d0\" face=\"%2\">%1</font>").arg(item.desc.trimmed(), GlobalObjects::normalFont));
    descLabel->setToolTip(item.desc);
    QPushButton *addItemButton = new KPushButton(tr("Add"), this);
    QObject::connect(addItemButton,&QPushButton::clicked,this,[this,addItemButton](){
		addItemButton->setEnabled(false);
        emit addSearchItem(this);
		addItemButton->setEnabled(true);
	});
    QGridLayout *searchPageGLayout=new QGridLayout(this);
    searchPageGLayout->addWidget(titleLabel,0,0);
    searchPageGLayout->addWidget(descLabel,1,0);
    searchPageGLayout->addWidget(addItemButton,0,1,2,1,Qt::AlignCenter);
    searchPageGLayout->setColumnMinimumWidth(1,50*logicalDpiX()/96);
    searchPageGLayout->setColumnStretch(0,6);
    searchPageGLayout->setColumnStretch(1,1);
}

QSize SearchItemWidget::sizeHint() const
{
    return layout()->sizeHint();
}

RelWordWidget::RelWordWidget(QWidget *parent):QWidget(parent)
{
    QHBoxLayout *relHLayout = new QHBoxLayout(this);
    auto contentMargins = this->contentsMargins();
    contentMargins.setTop(0);
    contentMargins.setBottom(0);
    relHLayout->setContentsMargins(contentMargins);
    relHLayout->addStretch(1);
    setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
}

void RelWordWidget::setRelWordList(const QStringList &relWords)
{
    qDeleteAll(relBtns);
    for(auto &rel:relWords)
    {
        QPushButton *relBtn=new QPushButton(rel, this);
        relBtn->setObjectName(QStringLiteral("LinkFlatButton"));
        QObject::connect(relBtn, &QPushButton::clicked, this, [relBtn, this](){
           emit relWordClicked(relBtn->text());
        });
        relBtns<<relBtn;
        static_cast<QHBoxLayout *>(layout())->insertWidget(0,relBtn);
    }
    if(relBtns.isEmpty()) hide();
    else show();
}

QSize RelWordWidget::sizeHint() const
{
    return layout()->sizeHint();
}

DanmuItemWidget::DanmuItemWidget(QList<SearchDanmuInfo> &danmuList, int index, const QStringList &poolList) :
    _danmuList(danmuList), _index(index), _danmuPools(poolList)
{
    SearchDanmuInfo &info = danmuList[index];

    ElidedLabel *titleLabel = new ElidedLabel(info.src.title, this);
    titleLabel->setFont(QFont(GlobalObjects::normalFont, 14));
    titleLabel->setFontColor(QColor(240, 240, 240));
    titleLabel->setToolTip(info.src.title);
    KTagPanel *srcTags = new KTagPanel(this, 10);
    QHBoxLayout *titleHLayout = new QHBoxLayout;
    titleHLayout->setContentsMargins(0, 0, 0, 0);
    titleHLayout->setSpacing(4);
    titleHLayout->addWidget(srcTags, 0, Qt::AlignLeft | Qt::AlignVCenter);
    titleHLayout->addWidget(titleLabel);
    titleLabel->setMinimumWidth(20);

    initSrcTags(srcTags, info.src);


    poolCombo = new ElaComboBox(this);
    poolCombo->setMaximumWidth(260);
    static_cast<ElaComboBox *>(poolCombo)->setWordWrap(true);

    QPushButton *autoSetPoolBtn = new KPushButton(this);
    autoSetPoolBtn->setToolTip(tr("Set PoolId in Sequence"));
    autoSetPoolBtn->setObjectName(QStringLiteral("AutoSetPoolBtn"));
    autoSetPoolBtn->setFont(*GlobalObjects::iconfont);
    autoSetPoolBtn->setText(QChar(0xe6ed));
    QHBoxLayout *poolHLayout = new QHBoxLayout;
    poolHLayout->setContentsMargins(0, 0, 0, 0);
    poolHLayout->addWidget(poolCombo);
    poolHLayout->addWidget(autoSetPoolBtn);

    if (poolList.empty())
    {
        poolCombo->setVisible(false);
        autoSetPoolBtn->setVisible(false);
    }
    else
    {
        poolCombo->addItems(poolList);
        poolCombo->setCurrentIndex(poolList.indexOf(info.pool));
    }
    QObject::connect(poolCombo, &QComboBox::currentIndexChanged, this, [=](int index){
        if (index >= 0 && index < _danmuPools.size())
        {
            _danmuList[_index].pool = _danmuPools[index];
        }
    });
    QObject::connect(autoSetPoolBtn, &QPushButton::clicked, this, [=](){
        emit setPoolIndexFrom(poolCombo->currentIndex());
    });

    ElaCheckBox *srcCheck = new ElaCheckBox(" ", this);
    srcCheck->setChecked(info.checked);
    QObject::connect(srcCheck, &QCheckBox::clicked, this, [=](bool checked){
        _danmuList[_index].checked = checked;
    });

    QGridLayout *itemGLayout=new QGridLayout(this);
    itemGLayout->setContentsMargins(8, 0, 8, 0);
    itemGLayout->addWidget(srcCheck, 0, 0);
    itemGLayout->addItem(titleHLayout, 0, 1);
    itemGLayout->addItem(poolHLayout, 0, 2);
    itemGLayout->setColumnStretch(1,1);
}

void DanmuItemWidget::setPoolIndex(int index)
{
    poolCombo->setCurrentIndex(qMin(index, _danmuPools.size() - 1));
}

int DanmuItemWidget::getPoolIndex() const
{
    return poolCombo->currentIndex();
}

QSize DanmuItemWidget::sizeHint() const
{
    return layout()->sizeHint();
}

void DanmuItemWidget::initSrcTags(KTagPanel *tagPanel, const DanmuSource &src)
{
    auto curScript = GlobalObjects::scriptManager->getScript(src.scriptId).dynamicCast<DanmuScript>();
    KTagPanel::TagItem mainTag;
    mainTag.textColor = Qt::white;
    mainTag.flag = DanmuSourceTag::FLAG_SCRIPT;
    if (curScript)
    {
        mainTag.text = curScript->name();
        mainTag.tooltip = src.url.isEmpty() ? curScript->name() : src.url;
        mainTag.bgColor = curScript->labelColor();
        if (!curScript->scriptIcon().isNull()) mainTag.icon = curScript->scriptIcon();
    }
    else
    {
        if (src.isKikoSource())
        {
            mainTag.text = "Kiko";
            mainTag.tooltip = tr("KikoPlay Source");
            mainTag.bgColor = QColor{ 19, 165, 200 };
            mainTag.icon = QIcon(":/res/images/kikoplay.svg");
        }
        else
        {
            mainTag.text = tr("Local");
            mainTag.tooltip = tr("Local Source");
            mainTag.bgColor = QColor{ 43, 106, 176 };
        }
    }
    if (src.duration > 0)
    {
        if (mainTag.icon.isNull())
            mainTag.text += "|" + src.durationStr() ;
        else
            mainTag.text = src.durationStr();
    }
    tagPanel->addTag(mainTag);
    auto& countTag = tagPanel->addTag(QString::number(src.count), tr("%1 danmus").arg(src.count), QIcon(), mainTag.bgColor);
    countTag.flag = DanmuSourceTag::FLAG_COUNT;

    for (int i = 0; i < src.tags.size(); ++i)
    {
        auto &tag = src.tags[i];
        KTagPanel::TagItem srcTag;
        srcTag.text = tag.text;
        srcTag.tooltip = tag.tooltip;
        srcTag.bgColor = tag.bgColor == -1 ? mainTag.bgColor : tag.bgColor;
        srcTag.textColor = tag.textColor == -1 ? mainTag.textColor : tag.textColor;
        if (!tag.iconSVG.isEmpty())
        {
            srcTag.setIconFromSvgStr(tag.iconSVG);
        }
        srcTag.flag = DanmuSourceTag::FLAG_CUSTOM + i;
        tagPanel->addTag(srcTag);
    }

    QObject::connect(tagPanel, &KTagPanel::tagClicked, this, [=](int index){
        const DanmuSource &src = _danmuList[_index].src;
        const auto &panelTag = tagPanel->tag(index);
        int tagIndex = panelTag.flag;
        if (tagIndex == DanmuSourceTag::FLAG_SCRIPT)
        {
            if (!src.url.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(src.url));
            }
        }
        else if (tagIndex == DanmuSourceTag::FLAG_COUNT)
        {
            DanmuView view(&_danmuList[_index].danmus, this);
            view.exec();
        }
        else if (tagIndex >= DanmuSourceTag::FLAG_CUSTOM)
        {
            tagIndex -= DanmuSourceTag::FLAG_CUSTOM;
            if (tagIndex < src.tags.size() && !src.tags[tagIndex].link.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(src.tags[tagIndex].link));
            }
        }
    });
}

