#include "animesearch.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "MediaLibrary/animeprovider.h"
#include "Common/notifier.h"
#include "widgets/scriptsearchoptionpanel.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "Extension/Script/scriptmanager.h"
#define AnimeRole Qt::UserRole+1

AnimeSearch::AnimeSearch(Anime *anime, QWidget *parent) : CFramelessDialog(tr("Bangumi Search"),parent,true,true,false)
{
    scriptCombo = new ElaComboBox(this);
    scriptOptionPanel = new ScriptSearchOptionPanel(this);
    QObject::connect(scriptCombo, &QComboBox::currentTextChanged, this, [=](const QString &){
        QString curId = scriptCombo->currentData().toString();
        scriptOptionPanel->setScript(GlobalObjects::scriptManager->getScript(curId));
        if(scriptOptionPanel->hasOptions()) scriptOptionPanel->show();
        else scriptOptionPanel->hide();
    });
    for (const auto &p : GlobalObjects::animeProvider->getSearchProviders())
    {
        scriptCombo->addItem(p.first, p.second);
    }
	if (anime)
	{
		int animeScriptIndex = scriptCombo->findData(anime->scriptId());
		if (animeScriptIndex != -1)
		{
			scriptCombo->setCurrentIndex(animeScriptIndex);
		}
	}

    searchWordEdit = new ElaLineEdit(anime?anime->name():"",this);
    searchButton = new KPushButton(tr("Search"), this);
    QObject::connect(searchButton, &QPushButton::clicked, this, &AnimeSearch::search);

    bangumiList = new QTreeView(this);
    bangumiList->setRootIsDecorated(false);
    bangumiList->setFont(font());
    bangumiList->setItemDelegate(new KTreeviewItemDelegate(this));
    bangumiList->setSelectionMode(QAbstractItemView::SingleSelection);
    QHeaderView *bgmHeader = bangumiList->header();
    bgmHeader->resizeSection(0, 300);
    bgmHeader->setFont(font());
    model = new BgmListModel(this);
    bangumiList->setModel(model);

    QObject::connect(bangumiList, &QTreeView::doubleClicked,[=](const QModelIndex &index){
        curSelectedAnime = index.data(AnimeRole).value<AnimeLite>();
        CFramelessDialog::onAccept();
    });

    QGridLayout *bgmGLayout = new QGridLayout(this);
    bgmGLayout->addWidget(scriptCombo, 0, 0);
    bgmGLayout->addWidget(searchWordEdit, 0, 1);
    bgmGLayout->addWidget(searchButton, 0, 2);
    bgmGLayout->addWidget(scriptOptionPanel,2,0,1,3);
    bgmGLayout->addWidget(bangumiList,3,0,1,3);
    bgmGLayout->setRowStretch(3,1);
    bgmGLayout->setColumnStretch(1,1);

    setSizeSettingKey("DialogSize/AnimeSearch", QSize(460, 320));
}

void AnimeSearch::search()
{
    QString keyword=searchWordEdit->text().trimmed();
    if (keyword.isEmpty())return;
    if (scriptCombo->currentData().isNull()) return;
    searchButton->setEnabled(false);
    searchWordEdit->setEnabled(false);
    showBusyState(true);
    QList<AnimeLite> animes;
    QMap<QString, QString> searchOptions;
    if (scriptOptionPanel->hasOptions() && scriptOptionPanel->changed())
    {
        searchOptions = scriptOptionPanel->getOptionVals();
    }
    ScriptState state = GlobalObjects::animeProvider->animeSearch(scriptCombo->currentData().toString(), keyword, searchOptions, animes);
    if (state)
    {
        model->setAnimes(animes);
    } else {
        showMessage(state.info, NM_ERROR | NM_HIDE);
    }
    searchButton->setEnabled(true);
    searchWordEdit->setEnabled(true);
    showBusyState(false);
}

void AnimeSearch::onAccept()
{
    if (!bangumiList->selectionModel()->hasSelection())
    {
        showMessage(tr("You need to choose one"), NM_ERROR | NM_HIDE);
        return;
    }
    else
    {
        curSelectedAnime = bangumiList->selectionModel()->selectedRows().last().data(AnimeRole).value<AnimeLite>();
    }
    CFramelessDialog::onAccept();
}

void BgmListModel::setAnimes(const QList<AnimeLite> &animes)
{
    beginResetModel();
    _animes = animes;
    endResetModel();
}

QVariant BgmListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const AnimeLite &anime = _animes[index.row()];
    Columns col = static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::TITLE:
        {
            return anime.name;
        }
        case Columns::EXTRA:
        {
            return anime.extras;
        }
        default:
            break;
        }
        break;
    }
    case AnimeRole:
        return QVariant::fromValue(anime);
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant BgmListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static const QStringList headers{ tr("Title"), tr("Extra"), };
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    return QVariant();
}
