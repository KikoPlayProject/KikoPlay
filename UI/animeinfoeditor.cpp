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
#include "Common/notifier.h"
#include "MediaLibrary/animeinfo.h"
#include "MediaLibrary/animeworker.h"

AnimeInfoEditor::AnimeInfoEditor(Anime *anime, QWidget *parent) :
    CFramelessDialog(tr("Edit Anime Info - %1").arg(anime->name()), parent, true, true, false),
    airDateChanged(false), epsChanged(false), staffChanged(false), descChanged(false), curAnime(anime)
{
    QWidget *leftContainer = new QWidget(this);
    QLabel *airTimeLabel = new QLabel(tr("Air Date"), leftContainer);
    dateEdit = new QDateEdit(leftContainer);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setDate(QDate::fromString(anime->airDate(), "yyyy-MM-dd"));
    QObject::connect(dateEdit, &QDateTimeEdit::dateChanged, this, [=](){
        airDateChanged = true;
    });

    QLabel *epsLabel = new QLabel(tr("Episodes"), leftContainer);
    epsEdit = new QLineEdit(QString::number(anime->epCount()), leftContainer);
    QIntValidator *epValidator=new QIntValidator(leftContainer);
    epValidator->setBottom(0);
    epsEdit->setValidator(epValidator);
    QObject::connect(epsEdit, &QLineEdit::textChanged, this, [=](){
        epsChanged = true;
    });

    QLabel *staffLabel = new QLabel(tr("Staff Info"), leftContainer);
    QTreeView *staffView = new QTreeView(leftContainer);
    staffView->setContextMenuPolicy(Qt::ActionsContextMenu);
    staffView->setEditTriggers(QAbstractItemView::DoubleClicked);
    staffView->setRootIsDecorated(false);
    staffView->setAlternatingRowColors(true);
    staffModel = new StaffModel(anime, this);
    QObject::connect(staffModel, &StaffModel::staffChanged, this, [=](){
        staffChanged = true;
    });

    staffView->setModel(staffModel);
    QAction *actAddStaff = new QAction(tr("Add Staff"), staffView);
    QObject::connect(actAddStaff, &QAction::triggered, this, [=](){
        auto index = staffModel->addStaff();
        staffView->scrollTo(index);
        staffView->edit(index);
    });
    QAction *actRemoveStaff = new QAction(tr("Remove Staff"), staffView);
    QObject::connect(actRemoveStaff, &QAction::triggered, this, [=](){
        auto selection = staffView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        staffModel->removeStaff(selection.first());
    });
    staffView->addAction(actAddStaff);
    staffView->addAction(actRemoveStaff);


    QLabel *aliasLabel = new QLabel(tr("Alias"), leftContainer);
    QListView *aliasView = new QListView(leftContainer);
    AliasModel *aliasModel = new AliasModel(anime->name(), this);
    QObject::connect(aliasModel, &AliasModel::addFailed, this, [=](){
        showMessage(tr("Add Alias Failed: The alias is illegal"), NM_HIDE | NM_ERROR);
    });
    aliasView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    aliasView->setContextMenuPolicy(Qt::ActionsContextMenu);
    aliasView->setSelectionMode(QAbstractItemView::SingleSelection);
    aliasView->setModel(aliasModel);
    QAction *actAddAlias = new QAction(tr("Add Alias"), aliasView);
    QObject::connect(actAddAlias, &QAction::triggered, this, [=](){
        auto index = aliasModel->addAlias();
        aliasView->scrollTo(index);
        aliasView->edit(index);
    });
    QAction *actRemoveAlias = new QAction(tr("Remove Alias"), aliasView);
    QObject::connect(actRemoveAlias, &QAction::triggered, this, [=](){
        auto selection = aliasView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        aliasModel->removeAlias(selection.first());
    });
    aliasView->addAction(actAddAlias);
    aliasView->addAction(actRemoveAlias);


    QGridLayout *leftGLayout = new QGridLayout(leftContainer);
    leftGLayout->setContentsMargins(0, 0, 0, 0);
    leftGLayout->addWidget(airTimeLabel, 0, 0);
    leftGLayout->addWidget(dateEdit, 0, 1);
    leftGLayout->addWidget(epsLabel, 1, 0);
    leftGLayout->addWidget(epsEdit, 1, 1);
    leftGLayout->addWidget(staffLabel, 2, 0);
    leftGLayout->addWidget(staffView, 3, 0, 1, 2);
    leftGLayout->addWidget(aliasLabel, 4, 0);
    leftGLayout->addWidget(aliasView, 5, 0, 1, 2);
    leftGLayout->setColumnStretch(1, 1);
    leftGLayout->setRowStretch(3, 2);
    leftGLayout->setRowStretch(5, 1);


    QWidget *rightContainer = new QWidget(this);
    QLabel *descLabel = new QLabel(tr("Description"), rightContainer);
    descEdit = new QTextEdit(rightContainer);
    descEdit->setPlainText(anime->description());
    QObject::connect(descEdit, &QTextEdit::textChanged, this, [=](){
        descChanged = true;
    });

    QGridLayout *rightGLayout = new QGridLayout(rightContainer);
    rightGLayout->setContentsMargins(0, 0, 0, 0);
    rightGLayout->addWidget(descLabel, 0, 0);
    rightGLayout->addWidget(descEdit, 1, 0);
    rightGLayout->setRowStretch(1, 1);


    QSplitter *contentSplitter = new QSplitter(this);
    contentSplitter->setObjectName(QStringLiteral("NormalSplitter"));
    contentSplitter->addWidget(leftContainer);
    contentSplitter->addWidget(rightContainer);
    contentSplitter->setStretchFactor(0,4);
    contentSplitter->setStretchFactor(1,5);
    contentSplitter->setCollapsible(0, false);
    contentSplitter->setCollapsible(1, false);

    QHBoxLayout *editorHLayout = new QHBoxLayout(this);
    editorHLayout->setContentsMargins(0, 4*logicalDpiY()/96, 0, 0);
    editorHLayout->addWidget(contentSplitter);

    setSizeSettingKey("DialogSize/AnimeInfoEdit", QSize(500*logicalDpiX()/96, 400*logicalDpiY()/96));
}

const QString AnimeInfoEditor::airDate() const
{
    return dateEdit->date().toString("yyyy-MM-dd");
}

int AnimeInfoEditor::epCount() const
{
    QString epText = epsEdit->text().trimmed();
    return epText.isEmpty()? curAnime->epCount() : epText.toInt();
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
