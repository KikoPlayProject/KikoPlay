#include "animeepisodeeditor.h"
#include "Common/lrucache.h"
#include "Common/notifier.h"
#include "Extension/Script/scriptbase.h"
#include "MediaLibrary/animeprovider.h"
#include "UI/widgets/kpushbutton.h"
#include "ela/ElaComboBox.h"
#include "ela/ElaLineEdit.h"
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QStyledItemDelegate>
#include <QAbstractItemView>
#include <QStandardItem>
#include "globalobjects.h"

namespace
{
static LRUCache<QString, QVector<EpInfo>> &epCache()
{
    static LRUCache<QString, QVector<EpInfo>> cache{"EpInfo"};
    return cache;
}
class EpComboItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        {
            const QString type = index.data(Qt::AccessibleDescriptionRole).toString();
            if (type == QLatin1String("child"))
            {
                QStyleOptionViewItem childOption = option;
                int indent = option.fontMetrics.horizontalAdvance(QString(4, QChar(' ')));
                childOption.rect.adjust(indent, 0, 0, 0);
                childOption.textElideMode = Qt::ElideNone;
                QStyledItemDelegate::paint(painter, childOption, index);
            }
            else
            {
                QStyledItemDelegate::paint(painter, option, index);
            }
        }
    }
};
}

AnimeEpisodeEditor::AnimeEpisodeEditor(Anime *anime, const EpInfo &ep, QWidget *parent) :
    CFramelessDialog(tr("Edit Episode"), parent, true, true, false), curAnime(anime), curEp(ep)
{
    QLabel *epTypeTip=new QLabel(tr("Episode Type"), this);
    ElaComboBox *epTypeCombo = new ElaComboBox(this);
    epTypeCombo->addItems(QStringList(std::begin(EpTypeName), std::end(EpTypeName)));

    QLabel *epIndexTip = new QLabel(tr("Episode Index"), this);
    ElaLineEdit *epIndexEdit = new ElaLineEdit(this);
    epIndexEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d+\\.?(\\d+)?"), epIndexEdit));
    epIndexEdit->setFixedHeight(34);

    QLabel *epTitleTip=new QLabel(tr("Episode Title"), this);
    ElaComboBox *epEdit = new ElaComboBox(this);
    epEdit->view()->setItemDelegate(new EpComboItemDelegate(this));
    epEdit->setEditable(true);
    QPushButton *fetchEpNameBtn = new KPushButton(tr("Fetch Epsoide Name"), this);
    fetchEpNameBtn->setEnabled(!epCache().contains(curAnime->name()));

    QGridLayout *epEditorGLayout = new QGridLayout(this);
    epEditorGLayout->addWidget(epTypeTip, 0, 0);
    epEditorGLayout->addWidget(epTypeCombo, 1, 0);
    epEditorGLayout->addWidget(epIndexTip, 0, 1);
    epEditorGLayout->addWidget(epIndexEdit, 1, 1);
    epEditorGLayout->addWidget(epTitleTip, 2, 0);
    epEditorGLayout->addWidget(fetchEpNameBtn, 2, 1, Qt::AlignVCenter | Qt::AlignRight);
    epEditorGLayout->addWidget(epEdit, 3, 0, 1, 2);

    epEditorGLayout->setColumnStretch(0, 1);
    epEditorGLayout->setColumnStretch(1, 1);

    epTypeCombo->setCurrentIndex(curEp.type == EpType::UNKNOWN ? EpType::EP : curEp.type - 1);
    epIndexEdit->setText(QString::number(curEp.index));
    auto refreshEpList = [=](){
        epEdit->blockSignals(true);
        epList = epCache().get(curAnime->name());
        epEdit->clear();
        int lastType = -1;
        int index = 0;
        for (auto &ep : epList)
        {
            if (static_cast<int>(ep.type) != lastType)
            {
                addParentItem(epEdit, EpTypeName[static_cast<int>(ep.type) - 1]);
            }
            addChildItem(epEdit, ep.toString(), index++);
            lastType = static_cast<int>(ep.type);
        }
        epEdit->setEditText(curEp.name);
        epEdit->blockSignals(false);
    };
    refreshEpList();

    QObject::connect(fetchEpNameBtn, &QPushButton::clicked, this, [=](){
        if (!epCache().contains(curAnime->name()))
        {
            showMessage(tr("Fetching Episode List...."), NotifyMessageFlag::NM_DARKNESS_BACK | NotifyMessageFlag::NM_PROCESS);
            QVector<EpInfo> results;
            ScriptState state = GlobalObjects::animeProvider->getEp(curAnime, results);
            if(state)
            {
                showMessage(tr("Fetch Down"), NM_HIDE);
                epCache().put(curAnime->name(), results);
                fetchEpNameBtn->setEnabled(false);
            }
            else
            {
                showMessage(state.info, NotifyMessageFlag::NM_ERROR | NM_HIDE);
            }
        }
        refreshEpList();
    });

    QObject::connect(epEdit, (void (QComboBox::*)(int))&QComboBox::activated, this, [=](int index){
        int pos = epEdit->itemData(index, EpRole).toInt();
        if(pos < 0 || pos >= epList.size()) return;
        const EpInfo &ep = epList[pos];
        epEdit->setEditText(ep.name);
        epIndexEdit->setText(QString::number(ep.index));
        epTypeCombo->setCurrentIndex(ep.type == EpType::UNKNOWN ? EpType::EP : ep.type-1);
    });

    QObject::connect(epTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
        this->curEp.type = EpType(index + 1);
        epChanged = true;
    });
    QObject::connect(epEdit, &QComboBox::currentTextChanged, this, [=](const QString &text){
        this->curEp.name = text.trimmed();
        epChanged = true;
    });
    QObject::connect(epIndexEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        this->curEp.index = text.toDouble();
        epChanged = true;
    });

    setSizeSettingKey("DialogSize/AnimeEpEditor", QSize(420, 120));
}

void AnimeEpisodeEditor::addParentItem(QComboBox *combo, const QString &text) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    item->setData("parent", Qt::AccessibleDescriptionRole);
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}

void AnimeEpisodeEditor::addChildItem(QComboBox *combo, const QString &text, int epListIndex) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setData("child", Qt::AccessibleDescriptionRole);
    item->setData(epListIndex, EpRole);
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}
