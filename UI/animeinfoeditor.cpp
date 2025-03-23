#include "animeinfoeditor.h"
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeView>
#include <QListView>
#include <QDateEdit>
#include <QGridLayout>
#include <QSplitter>
#include <QAction>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include "Common/flowlayout.h"
#include "Common/notifier.h"
#include "MediaLibrary/animeinfo.h"
#include "MediaLibrary/animeworker.h"
#include "UI/ela/ElaCalendarPicker.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/inputdialog.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/fonticonbutton.h"
#include "UI/widgets/klineedit.h"
#include "UI/widgets/kplaintextedit.h"

namespace
{

class StaffItemDelegate: public QStyledItemDelegate
{
public:
    explicit StaffItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) { }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return new KLineEdit(parent);
    }
    inline void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const override
    {
        editor->setGeometry(option.rect);
    }
};

}

AnimeInfoEditor::AnimeInfoEditor(Anime *anime, QWidget *parent) :
    CFramelessDialog(tr("Edit Anime Info - %1").arg(anime->name()), parent, true, true, false),
    airDateChanged(false), epsChanged(false), staffChanged(false), descChanged(false), curAnime(anime)
{
    QLabel *airTimeLabel = new QLabel(tr("Air Date"), this);
    dateEdit = new ElaCalendarPicker(this);
    dateEdit->setSelectedDate(QDate::fromString(anime->airDate(), "yyyy-MM-dd"));
    QLabel *epsLabel = new QLabel(tr("Episodes"), this);
    epsSpin = new ElaSpinBox(this);
    epsSpin->setRange(0, INT_MAX);
    epsSpin->setValue(anime->epCount());

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(airTimeLabel);
    hLayout->addWidget(dateEdit);
    hLayout->addSpacing(20);
    hLayout->addWidget(epsLabel);
    hLayout->addWidget(epsSpin);
    hLayout->addStretch(1);

    QLabel *aliasLabel = new QLabel(tr("Alias"), this);
    FontIconButton *addAliasBtn = new FontIconButton(QChar(0xe667), "", 12, 10, 0, this);
    addAliasBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    QHBoxLayout *hAliasLayout = new QHBoxLayout;
    hAliasLayout->setContentsMargins(0, 0, 0, 0);
    hAliasLayout->addWidget(aliasLabel);
    hAliasLayout->addWidget(addAliasBtn);
    hAliasLayout->addStretch(1);
    AliasPanel *aliasPanel = new AliasPanel(anime->name(), this);

    QLabel *staffLabel = new QLabel(tr("Staff Info"), this);
    QTreeView *staffView = new QTreeView(this);
    staffView->setContextMenuPolicy(Qt::CustomContextMenu);
    staffView->setEditTriggers(QAbstractItemView::DoubleClicked);
    staffView->setRootIsDecorated(false);
    staffView->setItemDelegate(new StaffItemDelegate(staffView));
    staffView->setAlternatingRowColors(true);
    staffModel = new StaffModel(anime, this);
    staffView->setModel(staffModel);

    QLabel *descLabel = new QLabel(tr("Description"), this);
    descEdit = new KTextEdit(this);
    descEdit->setPlainText(anime->description());

    QWidget *pageContainer = new QWidget(this);
    QVBoxLayout *pageVLayout = new QVBoxLayout(pageContainer);
    pageVLayout->setContentsMargins(0, 0, 0, 0);
    pageVLayout->addLayout(hLayout);
    pageVLayout->addLayout(hAliasLayout);
    pageVLayout->addWidget(aliasPanel);
    pageVLayout->addWidget(descLabel);
    pageVLayout->addWidget(descEdit, 1);
    pageVLayout->addWidget(staffLabel);
    pageVLayout->addWidget(staffView, 1);
    // pageVLayout->addStretch(1);

    QScrollArea *pageScrollArea = new QScrollArea(this);
    pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageScrollArea->setWidget(pageContainer);
    pageScrollArea->setWidgetResizable(true);
    new FloatScrollBar(pageScrollArea->verticalScrollBar(), pageScrollArea);

    QHBoxLayout *pHLayout = new QHBoxLayout(this);
    pHLayout->addWidget(pageScrollArea);


    QObject::connect(dateEdit, &ElaCalendarPicker::selectedDateChanged, this, [=](){
        airDateChanged = true;
    });

    QObject::connect(epsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](){
        epsChanged = true;
    });

    QObject::connect(staffModel, &StaffModel::staffChanged, this, [=](){
        staffChanged = true;
    });

    QObject::connect(descEdit, &QTextEdit::textChanged, this, [=](){
        descChanged = true;
    });

    QObject::connect(addAliasBtn, &FontIconButton::clicked, this, [=](){
        LineInputDialog input(tr("Add Alias"), tr("Alias"), "", "DialogSize/AddAlias", false, this);
        if (QDialog::Accepted == input.exec())
        {
            const QString nAlias = input.text.trimmed();
            if (nAlias == anime->name())
            {
                showMessage(tr("The alias cannot be the same as the anime name"), NotifyMessageFlag::NM_ERROR);
                return;
            }
            if (AnimeWorker::instance()->hasAnime(nAlias))
            {
                showMessage(tr("The alias already exists"), NotifyMessageFlag::NM_ERROR);
                return;
            }
            if (AnimeWorker::instance()->addAlias(anime->name(), nAlias))
            {
                aliasPanel->addAlias(nAlias);
            }
            else
            {
                showMessage(tr("Failed to add the alias"), NotifyMessageFlag::NM_ERROR);
            }
        }
    });

    ElaMenu *actionMenu = new ElaMenu(staffView);
    QObject::connect(staffView, &QLabel::customContextMenuRequested, this, [=](){
        actionMenu->exec(QCursor::pos());
    });
    QAction *actAddStaff = actionMenu->addAction(tr("Add Staff"));
    QObject::connect(actAddStaff, &QAction::triggered, this, [=](){
        auto index = staffModel->addStaff();
        staffView->scrollTo(index);
        staffView->edit(index);
    });
    QAction *actRemoveStaff = actionMenu->addAction(tr("Remove Staff"));
    QObject::connect(actRemoveStaff, &QAction::triggered, this, [=](){
        auto selection = staffView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        staffModel->removeStaff(selection.first());
    });

    setSizeSettingKey("DialogSize/AnimeInfoEdit", QSize(400, 500));
}

const QString AnimeInfoEditor::airDate() const
{
    return dateEdit->getSelectedDate().toString("yyyy-MM-dd");
}

int AnimeInfoEditor::epCount() const
{
    return epsSpin->value();
}

const QVector<QPair<QString, QString> > &AnimeInfoEditor::staffs() const
{
    return staffModel->staffs;
}

const QString AnimeInfoEditor::desc() const
{
    return descEdit->toPlainText();
}


AliasModel::AliasModel(const QString &animeName, QObject *parent):QAbstractItemModel(parent), anime(animeName)
{
    beginResetModel();
    aliasList = AnimeWorker::instance()->getAlias(animeName);
    endResetModel();
}

QModelIndex AliasModel::addAlias()
{
    beginInsertRows(QModelIndex(), aliasList.size(), aliasList.size());
    aliasList.append("");
    endInsertRows();
    return index(aliasList.size()-1, 0, QModelIndex());
}

void AliasModel::removeAlias(const QModelIndex &index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    AnimeWorker::instance()->removeAlias(anime, aliasList[index.row()], true);
    aliasList.removeAt(index.row());
    endRemoveRows();
}

QVariant AliasModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    if(role == Qt::DisplayRole)
    {
        return aliasList[index.row()];
    }
    return QVariant();
}

bool AliasModel::setData(const QModelIndex &index, const QVariant &value, int)
{
    if(!index.isValid() || anime.isEmpty()) return false;
    QString alias = value.toString();
    auto remove = [=](int row){
        beginRemoveRows(QModelIndex(), row, row);
        aliasList.removeAt(row);
        endRemoveRows();
    };
    if(alias.isEmpty())
    {
        remove(index.row());
        return false;
    }
    if(alias == anime)
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
    if(AnimeWorker::instance()->hasAnime(alias))
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
    if(AnimeWorker::instance()->addAlias(anime, alias))
    {
        aliasList[index.row()] = alias;
        return true;
    }
    else
    {
        remove(index.row());
        emit addFailed();
        return false;
    }
}

StaffModel::StaffModel(const Anime *anime, QObject *parent) : QAbstractItemModel(parent)
{
    beginResetModel();
    staffs = anime->staffList();
    endResetModel();
}

QModelIndex StaffModel::addStaff()
{
    beginInsertRows(QModelIndex(), staffs.size(), staffs.size());
    staffs.append(QPair<QString, QString>("", ""));
    endInsertRows();
    return index(staffs.size()-1, 0, QModelIndex());
}

void StaffModel::removeStaff(const QModelIndex &index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    staffs.removeAt(index.row());
    emit staffChanged();
    endRemoveRows();
}

QVariant StaffModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section < headers.size())return headers.at(section);
    }
    return QVariant();
}

QVariant StaffModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    Columns col = static_cast<Columns>(index.column());
    const auto &kv = staffs[index.row()];
    if(role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::EditRole)
    {
        switch (col)
        {
        case Columns::TYPE:
            return kv.first;
        case Columns::STAFF:
            return kv.second;
        }
    }
    return QVariant();
}

bool StaffModel::setData(const QModelIndex &index, const QVariant &value, int)
{
    if(!index.isValid()) return false;
    Columns col = static_cast<Columns>(index.column());
    QString val = value.toString().trimmed().replace(';', "");
    if(val.isEmpty() && col == Columns::TYPE)
    {
        if(staffs[index.row()].first.isEmpty())
        {
            beginRemoveRows(QModelIndex(), index.row(), index.row());
            staffs.removeAt(index.row());
            endRemoveRows();
        }
        return false;
    }
    switch (col)
    {
    case Columns::TYPE:
        staffs[index.row()].first = val;
        break;
    case Columns::STAFF:
        staffs[index.row()].second = val;
        break;
    default:
        return false;
    }
    emit staffChanged();
    return true;
}

AliasPanel::AliasPanel(const QString &animeName, QWidget *parent) : QWidget(parent), anime(animeName)
{
    setLayout(new FlowLayout(this));
    layout()->setContentsMargins(0, 0, 0, 0);
    for (const QString &alias : AnimeWorker::instance()->getAlias(animeName))
    {
        addAlias(alias);
    }
    setVisible(!aliasList.isEmpty());
}

void AliasPanel::addAlias(const QString &alias)
{
    const int curIndex = aliasList.size();
    aliasList.append(alias);
    QLabel *aliasItem = new QLabel(alias, this);
    aliasItem->setObjectName(QStringLiteral("AliasItem"));
    this->layout()->addWidget(aliasItem);
    setVisible(true);
    aliasItem->setContextMenuPolicy(Qt::CustomContextMenu);

    ElaMenu *actionMenu = new ElaMenu(aliasItem);
    QObject::connect(aliasItem, &QLabel::customContextMenuRequested, this, [=](){
        actionMenu->exec(QCursor::pos());
    });

    QAction *actRemoveAlias = actionMenu->addAction(tr("Remove Alias"));
    QObject::connect(actRemoveAlias, &QAction::triggered, this, [=](){
        AnimeWorker::instance()->removeAlias(anime, alias, true);
        aliasItem->deleteLater();
        aliasList.removeAt(curIndex);
        if (aliasList.isEmpty()) setVisible(false);
    });
}

QSize AliasPanel::sizeHint() const
{
    return layout()->sizeHint();
}
