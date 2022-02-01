#include "mpvshortcutpage.h"
#include "globalobjects.h"
#include "Common/notifier.h"
#include "Play/Video/mpvplayer.h"
#include <QTreeView>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QSettings>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QLabel>
#include <QAction>
#include <QHeaderView>
#define ShortCutRole Qt::UserRole+1

MPVShortcutPage::MPVShortcutPage(QWidget *parent) : SettingPage(parent)
{
    ShortcutModel *model = new ShortcutModel(this);

    QTreeView *shortcutView=new QTreeView(this);
    shortcutView->setRootIsDecorated(false);
    shortcutView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    shortcutView->setModel(model);
    shortcutView->setAlternatingRowColors(true);
    shortcutView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    shortcutView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    shortcutView->header()->resizeSection(1, 200*logicalDpiX()/96);
    shortcutView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    QPushButton *addShortcut=new QPushButton(tr("Add"),this);
    QCheckBox *directKeyMode=new QCheckBox(tr("Direct Key Mode"), this);
    directKeyMode->setToolTip(tr("In direct mode, KikoPlay will pass key events directly to MPV\nSet shortcuts through MPV Parameter input-conf"));

    QGridLayout *shortcutGLayout=new QGridLayout(this);
    shortcutGLayout->setContentsMargins(0, 0, 0, 0);
    shortcutGLayout->addWidget(addShortcut, 0, 0);
    shortcutGLayout->addWidget(directKeyMode, 0, 1);
    shortcutGLayout->addWidget(shortcutView, 1, 0, 1, 3);
    shortcutGLayout->setRowStretch(1, 1);
    shortcutGLayout->setColumnStretch(2, 1);

    QObject::connect(directKeyMode,&QCheckBox::stateChanged,[=](int state){
       bool enable = state == Qt::Checked;
       addShortcut->setEnabled(!enable);
       shortcutView->setEnabled(!enable);
       GlobalObjects::mpvplayer->setDirectKeyMode(enable);
    });
    directKeyMode->setChecked(GlobalObjects::mpvplayer->enableDirectKey());

    QObject::connect(addShortcut, &QPushButton::clicked, this, [=](){
        ShortcutEditDialog addShortcut(true, model, QModelIndex(), this);
        if(QDialog::Accepted==addShortcut.exec())
        {
            model->addShortcut({addShortcut.key, {addShortcut.command, addShortcut.desc}});
        }
    });

    QAction *actRemove = new QAction(tr("Remove"), this);
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        auto selection = shortcutView->selectionModel()->selectedRows((int)ShortcutModel::Columns::KEY);
        if (selection.size() == 0)return;
        std::sort(selection.begin(), selection.end(), [](const QModelIndex &a, const QModelIndex &b){
           return a.row()>=b.row();
        });
        for(const QModelIndex &index : selection)
        {
            model->removeShortcut(index);
        }
    });
    QAction *actModify = new QAction(tr("Modify"), this);
    QObject::connect(actModify, &QAction::triggered, this, [=](){
        auto selection = shortcutView->selectionModel()->selectedRows((int)ShortcutModel::Columns::KEY);
        if (selection.size() == 0)return;
        QModelIndex index(selection.last());
        if(!index.isValid()) return;
        ShortcutEditDialog addShortcut(false, model, index, this);
        if(QDialog::Accepted==addShortcut.exec())
        {
            model->modifyShortcut(index, {addShortcut.key, {addShortcut.command, addShortcut.desc}});
        }
    });
    shortcutView->addAction(actModify);
    shortcutView->addAction(actRemove);
    QObject::connect(shortcutView, &QTreeView::doubleClicked,[=](const QModelIndex &index){
        if(!index.isValid()) return;
        ShortcutEditDialog addShortcut(false, model, index, this);
        if(QDialog::Accepted==addShortcut.exec())
        {
            model->modifyShortcut(index, {addShortcut.key, {addShortcut.command, addShortcut.desc}});
        }
    });
}

void MPVShortcutPage::onAccept()
{

}

void MPVShortcutPage::onClose()
{

}

ShortcutModel::ShortcutModel(QObject *parent) : QAbstractItemModel(parent), modified(false)
{
    shortcutList = GlobalObjects::appSetting->value("Play/MPVShortcuts").value<QList<ShortCutInfo>>();
    for(auto &s : shortcutList)
    {
        shortcutHash.insert(s.first, s.second);
    }
}

ShortcutModel::~ShortcutModel()
{
    if(modified)
    {
        GlobalObjects::appSetting->setValue("Play/MPVShortcuts", QVariant::fromValue(shortcutList));
    }
}

void ShortcutModel::addShortcut(const ShortCutInfo &newShortcut)
{
    beginInsertRows(QModelIndex(), shortcutList.size(), shortcutList.size());
    shortcutList.append(newShortcut);
    GlobalObjects::mpvplayer->modifyShortcut(newShortcut.first, newShortcut.first,
                                             newShortcut.second.first);
    shortcutHash.insert(newShortcut.first, newShortcut.second);
    endInsertRows();
    modified = true;
}

void ShortcutModel::removeShortcut(const QModelIndex &index)
{
    if(!index.isValid()) return;
    QString key = shortcutList[index.row()].first;
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    GlobalObjects::mpvplayer->modifyShortcut(key, "", "");
    shortcutList.removeAt(index.row());
    shortcutHash.remove(key);
    endRemoveRows();
    modified = true;
}

void ShortcutModel::modifyShortcut(const QModelIndex &index, const ShortCutInfo &newShortcut)
{
    int row = index.row();
    QString key = shortcutList[row].first;
    shortcutHash.remove(key);
    shortcutList[row] = newShortcut;
    shortcutHash.insert(newShortcut.first, newShortcut.second);
    GlobalObjects::mpvplayer->modifyShortcut(key, newShortcut.first,
                                             newShortcut.second.first);
    emit dataChanged(index, index.siblingAtColumn((int)Columns::DESC));
    modified = true;
}

QVariant ShortcutModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    ShortcutModel::Columns col=static_cast<Columns>(index.column());
    const auto &shortcut=shortcutList.at(index.row());
    if(role==Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::KEY:
            return shortcut.first;
        case Columns::COMMAND:
            return shortcut.second.first;
        case Columns::DESC:
            return shortcut.second.second;
        default:
            break;
        }
    }
    else if(role == ShortCutRole)
    {
        return QVariant::fromValue(shortcut);
    }
    return QVariant();
}

QVariant ShortcutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}


ShortcutEditDialog::ShortcutEditDialog(bool add, ShortcutModel *model, const QModelIndex &index, QWidget *parent) :
    CFramelessDialog(tr("Edit Shortcut Key"), parent, true), shortcutModel(model), addShortcut(add)
{
    if(!add)
    {
        auto shortcutInfo = model->data(index, ShortCutRole).value<QPair<QString, QPair<QString, QString>>>();
        key = shortcutInfo.first;
        command = shortcutInfo.second.first;
        desc = shortcutInfo.second.second;
    }
    const auto &shortcutHash = shortcutModel->shortcuts();
    for(auto iter = shortcutHash.cbegin(); iter != shortcutHash.cend(); ++iter)
    {
        existKeys.insert(iter.key());
    }
    existKeys.remove(key);
    QLabel *shortcutTip = new QLabel(tr("Shortcut Key"), this);
    QLabel *commandTip = new QLabel(tr("MPV Command"), this);
    QLabel *descTip = new QLabel(tr("Description"), this);
    keyEdit = new QKeySequenceEdit(QKeySequence(key),this);
    commandEdit = new QLineEdit(command, this);
    descEdit = new QLineEdit(desc, this);

    QGridLayout *shortcutGLayout=new QGridLayout(this);
    shortcutGLayout->setContentsMargins(0, 0, 0, 0);
    shortcutGLayout->addWidget(shortcutTip,0,0);
    shortcutGLayout->addWidget(keyEdit,0,1);
    shortcutGLayout->addWidget(commandTip,1,0);
    shortcutGLayout->addWidget(commandEdit,1,1);
    shortcutGLayout->addWidget(descTip,2,0);
    shortcutGLayout->addWidget(descEdit,2,1);
    // shortcutGLayout->setRowStretch(1,1);
    shortcutGLayout->setColumnStretch(1,1);

    resize(GlobalObjects::appSetting->value("DialogSize/ShortcutEdit",QSize(400*logicalDpiX()/96,120*logicalDpiY()/96)).toSize());
}

void ShortcutEditDialog::onAccept()
{
    if(keyEdit->keySequence().isEmpty() || commandEdit->text().isEmpty())
    {
        showMessage(tr("Key/Command should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    key = keyEdit->keySequence().toString();
    command = commandEdit->text();
    desc = descEdit->text();
    const auto &shortcutHash = shortcutModel->shortcuts();
    if (existKeys.contains(key))
    {
        showMessage(tr("Shortcut key conflicts with \"%1\"").arg(shortcutHash[key].first), NM_ERROR | NM_HIDE);
        return;
    }
    GlobalObjects::appSetting->setValue("DialogSize/ShortcutEdit",size());
    CFramelessDialog::onAccept();
}

QDataStream &operator<<(QDataStream &out, const QList<ShortCutInfo> &l) {
    int s = l.size();
    out << s;
    if (s) for (int i = 0; i < s; ++i) out << l[i].first << l[i].second.first << l[i].second.second;
    return out;
}

QDataStream &operator>>(QDataStream &in, QList<ShortCutInfo> &l) {
    if (!l.empty()) l.clear();
    int s = 0;
    in >> s;
    if (s)
    {
        l.reserve(s);
        for (int i = 0; i < s; ++i) {
            QString key, command, desc;
            in >> key >> command >> desc;
            l.append({key, {command, desc}});
        }
    }
    return in;
}
