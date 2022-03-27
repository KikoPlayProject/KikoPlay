#include "episodeitem.h"
#include "episodesmodel.h"
#include "Common/notifier.h"
#include "Common/lrucache.h"
#include "animeprovider.h"
#include "Play/Video/mpvplayer.h"
#include "globalobjects.h"
#include <QComboBox>
#include <QStandardItem>
#include <QLineEdit>
#include <QGridLayout>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QDateTimeEdit>

namespace
{
    LRUCache<QString, QVector<EpInfo>> epCache{"EpInfo"};
    class EpComboItemDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            {
                QString type = index.data(Qt::AccessibleDescriptionRole ).toString();
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

EpInfoEditWidget::EpInfoEditWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("EpInfoEditWidget"));
    epTypeCombo = new QComboBox(this);
    epTypeCombo->addItems(QList<QString>(std::begin(EpTypeName), std::end(EpTypeName)));
    epIndexEdit = new QLineEdit(this);
    epIndexEdit->setValidator(new QRegExpValidator(QRegExp("\\d+\\.?(\\d+)?"),epIndexEdit));
    epNameEdit = new QComboBox(this);
    epNameEdit->setEditable(true);
    epNameEdit->view()->setItemDelegate(new EpComboItemDelegate(this));

    QObject::connect(epNameEdit, (void (QComboBox::*)(int))&QComboBox::activated, this, [=](int index){
       int pos = epNameEdit->itemData(index, EpRole).toInt();
       if(pos < 0 || pos >= epList.size()) return;
       const EpInfo &ep = epList[pos];
       epNameEdit->setEditText(ep.name);
       epIndexEdit->setText(QString::number(ep.index));
       epTypeCombo->setCurrentIndex(ep.type==EpType::UNKNOWN?EpType::EP:ep.type-1);
    });

    QGridLayout *editGLayout = new QGridLayout(this);
    editGLayout->setContentsMargins(0, 0, 0, 0);
    editGLayout->setSpacing(0);
    editGLayout->setRowStretch(0, 0);
    editGLayout->setColumnStretch(1, 1);
    editGLayout->setColumnStretch(2, 4);
    editGLayout->addWidget(epTypeCombo, 0, 0);
    editGLayout->addWidget(epIndexEdit, 0, 1);
    editGLayout->addWidget(epNameEdit, 0, 2);
}

void EpInfoEditWidget::setEpInfo(const EpInfo &curEp, const QVector<EpInfo> &eps)
{
    epList = eps;
    epNameEdit->clear();
    epTypeCombo->setCurrentIndex(curEp.type==EpType::UNKNOWN?EpType::EP:curEp.type-1);
    epIndexEdit->setText(QString::number(curEp.index));
    int lastType = -1;
    int index = 0;
    for(auto &ep : epList)
    {
        if(static_cast<int>(ep.type)!=lastType)
        {
            addParentItem(epNameEdit, EpTypeName[static_cast<int>(ep.type)-1]);
        }
        addChildItem(epNameEdit, ep.toString(), index++);
        lastType = static_cast<int>(ep.type);
    }
	epNameEdit->setEditText(curEp.name);
}

const EpInfo EpInfoEditWidget::getEp() const
{
    EpInfo ep;
    ep.type = EpType(epTypeCombo->currentIndex()+1);
    ep.index = epIndexEdit->text().toDouble();
    ep.name = epNameEdit->lineEdit()->text();
    return ep;
}

void EpInfoEditWidget::addParentItem(QComboBox *combo, const QString &text) const
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

void EpInfoEditWidget::addChildItem(QComboBox *combo, const QString &text, int epListIndex) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setData("child", Qt::AccessibleDescriptionRole);
    item->setData(epListIndex, EpRole);
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}

void EpInfoEditWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QStyleOption styleOpt;
    styleOpt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOpt, &painter, this);
}

QWidget *EpItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    EpisodesModel::Columns col=EpisodesModel::Columns(index.column());
    if(col==EpisodesModel::Columns::TITLE)
    {
        EpInfoEditWidget *epEditor = new EpInfoEditWidget(parent);
        return epEditor;
    }
    else if(col==EpisodesModel::Columns::FINISHTIME)
    {
        QDateTimeEdit *dateTimeEdit = new QDateTimeEdit(parent);
        dateTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
        return dateTimeEdit;
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void EpItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(!curAnime) return;
    EpisodesModel::Columns col=EpisodesModel::Columns(index.column());
    switch (col)
    {
    case EpisodesModel::Columns::TITLE:
    {
        EpInfoEditWidget *epEditor = static_cast<EpInfoEditWidget *>(editor);
        if(!epCache.contains(curAnime->name()) && autoGetEpInfo)
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Episode List...."), NotifyMessageFlag::NM_DARKNESS_BACK|NotifyMessageFlag::NM_PROCESS);
			QVector<EpInfo> results;
            ScriptState state = GlobalObjects::animeProvider->getEp(curAnime, results);
            if(state)
            {
                Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetch Down"), NM_HIDE);
                epCache.put(curAnime->name(), results);
            }
            else Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, state.info, NotifyMessageFlag::NM_ERROR|NM_HIDE);
        }
        epEditor->setEpInfo(index.data(EpisodesModel::EpRole).value<EpInfo>(), epCache.get(curAnime->name()));
        break;
    }
    case EpisodesModel::Columns::LOCALFILE:
    {
        QString file = QFileDialog::getOpenFileName(nullptr,tr("Select Media File"),"",
                                                    QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        QLineEdit *lineEdit=static_cast<QLineEdit *>(editor);
        if(!file.isNull()) lineEdit->setText(file);
        else lineEdit->setText(index.data(Qt::DisplayRole).toString());
        break;
    }
    case EpisodesModel::Columns::FINISHTIME:
    {
        QDateTimeEdit *dateTimeEdit = static_cast<QDateTimeEdit *>(editor);
        auto ep = index.data(EpisodesModel::EpRole).value<EpInfo>();
        dateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(ep.finishTime));
        break;
    }
    default:
        break;
    }
}

void EpItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(!curAnime) return;
    EpisodesModel::Columns col=EpisodesModel::Columns(index.column());
    switch (col)
    {
    case EpisodesModel::Columns::TITLE:
    {
        EpInfoEditWidget *epEditor = static_cast<EpInfoEditWidget *>(editor);
        EpInfo ep = epEditor->getEp();
        model->setData(index, QVariant::fromValue<EpInfo>(ep));
        break;
    }
    case EpisodesModel::Columns::FINISHTIME:
    {
        QDateTimeEdit *dateTimeEdit = static_cast<QDateTimeEdit *>(editor);
        model->setData(index, dateTimeEdit->dateTime().toSecsSinceEpoch());
        break;
    }
    default:
        QStyledItemDelegate::setModelData(editor,model,index);
    }
}
