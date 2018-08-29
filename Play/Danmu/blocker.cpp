#include "blocker.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int col=index.column();
    switch (col)
    {
    case 2:
    case 3:
    {
        QComboBox *combo=new QComboBox(parent);
        combo->setFrame(false);
        combo->addItems(col==2?GlobalObjects::blocker->fields:GlobalObjects::blocker->relations);
        return combo;
    }
    default:
    {
        return QStyledItemDelegate::createEditor(parent,option,index);
    }
    }
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int col=index.column();
    switch (col)
    {
    case 2:
    case 3:
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        combo->setCurrentIndex(col==2?GlobalObjects::blocker->fields.indexOf(index.data(Qt::DisplayRole).toString()):
                                      GlobalObjects::blocker->relations.indexOf(index.data(Qt::DisplayRole).toString()));
        break;
    }
    case 5:
    {
        QLineEdit *lineEdit=static_cast<QLineEdit *>(editor);
        lineEdit->setText(index.data(Qt::DisplayRole).toString());
        break;
    }
    default:
    {
        QStyledItemDelegate::setEditorData(editor,index);
        break;
    }
    }
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    int col=index.column();
    if(col!=2 && col!=3)
    {
        QStyledItemDelegate::setModelData(editor,model,index);
        return;
    }
    QComboBox *combo = static_cast<QComboBox*>(editor);
    model->setData(index,combo->currentIndex(),Qt::EditRole);
}

Blocker::Blocker(QObject *parent):QAbstractItemModel(parent),maxId(1)
{
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec(QString("select * from block"));
    int idNo=query.record().indexOf("Id"),
        fieldNo = query.record().indexOf("Field"),
        relationNo=query.record().indexOf("Relation"),
        isRegExpNo=query.record().indexOf("IsRegExp"),
        onNo=query.record().indexOf("Enable"),
        contentNo=query.record().indexOf("Content");
    beginResetModel();
    while (query.next())
    {
        BlockRule *rule=new BlockRule;
        rule->id=query.value(idNo).toInt();
        if(rule->id>=maxId)maxId=rule->id+1;
        rule->blockField=BlockRule::Field(query.value(fieldNo).toInt());
        rule->relation=BlockRule::Relation(query.value(relationNo).toInt());
        rule->isRegExp=query.value(isRegExpNo).toBool();
        rule->enable=query.value(onNo).toBool();
        rule->content=query.value(contentNo).toString();
        blockList.append(rule);
    }
    endResetModel();
}

Blocker::~Blocker()
{
    qDeleteAll(blockList);
}

void Blocker::addBlockRule()
{
    BlockRule *rule=new BlockRule;
    rule->id=maxId++;
    rule->blockField=BlockRule::Field::DanmuText;
    rule->relation=BlockRule::Relation::Contain;
    rule->isRegExp=true;
    rule->enable=true;
	int insertPosition = blockList.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition);
    blockList.append(rule);
    endInsertRows();
    insertToDB(rule);
}

void Blocker::addBlockRule(BlockRule *rule)
{
    rule->id=maxId++;
    int insertPosition = blockList.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition);
    blockList.append(rule);
    endInsertRows();
    insertToDB(rule);
    GlobalObjects::danmuPool->testBlockRule(rule);
}

void Blocker::removeBlockRule(const QModelIndexList &deleteIndexes)
{
    QSqlDatabase db=QSqlDatabase::database("MT");
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("delete from block where Id=?");
    db.transaction();
    QList<int> rows;
    foreach (const QModelIndex &index, deleteIndexes)
    {
        if (index.isValid())
        {
            int row=index.row();
            rows.append(row);
            query.bindValue(0,blockList.at(row)->id);
            query.exec();
        }
    }
    db.commit();
    std::sort(rows.rbegin(),rows.rend());
    for(auto iter=rows.begin();iter!=rows.end();++iter)
    {
        BlockRule *rule=blockList.at(*iter);
        rule->enable=false;
        GlobalObjects::danmuPool->testBlockRule(rule);
        beginRemoveRows(QModelIndex(), *iter, *iter);
        blockList.removeAt(*iter);
        endRemoveRows();
		delete rule;
    }
}

void Blocker::checkDanmu(QList<DanmuComment *> &danmuList)
{
    for(DanmuComment *danmu:danmuList)
    {
        for(BlockRule *rule:blockList)
        {
            if(rule->blockTest(danmu))
            {
                danmu->blockBy=rule->id;
                break;
            }
        }
    }
}

void Blocker::checkDanmu(QList<QSharedPointer<DanmuComment> > &danmuList)
{
    for(QSharedPointer<DanmuComment> &danmu:danmuList)
    {
        for(BlockRule *rule:blockList)
        {
            if(rule->blockTest(danmu.data()))
            {
                danmu->blockBy=rule->id;
                break;
            }
        }
    }
}

void Blocker::updateDB(int row, int col)
{
    BlockRule *rule=blockList.at(row);
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare(QString("update block set %1=? where Id=?").arg(colToDBRecords.at(col)));
    switch (col)
    {
    case 1:
        query.bindValue(0,rule->enable);
        break;
    case 2:
        query.bindValue(0,(int)rule->blockField);
        break;
    case 3:
        query.bindValue(0,(int)rule->relation);
        break;
    case 4:
        query.bindValue(0,rule->isRegExp);
        break;
    case 5:
        query.bindValue(0,rule->content);
        break;
    }
    query.bindValue(1,rule->id);
    query.exec();
}

void Blocker::insertToDB(BlockRule *rule)
{
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("insert into block values(?,?,?,?,?,?)");
    query.bindValue(0,rule->id);
    query.bindValue(1,(int)rule->blockField);
    query.bindValue(2,(int)rule->relation);
    query.bindValue(3,rule->isRegExp);
    query.bindValue(4,rule->enable);
    query.bindValue(5, rule->content);
    query.exec();
}

QVariant Blocker::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    BlockRule *rule=blockList.at(index.row());
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            return rule->id;
        }
        else if(col==2)
        {
            return fields.at(rule->blockField);
        }
        else if(col==3)
        {
            return relations.at(rule->relation);
        }
        else if(col==5)
        {
            return rule->content;
        }
    }
        break;
    case Qt::CheckStateRole:
    {
        if(col==1)
            return rule->enable?Qt::Checked:Qt::Unchecked;
        else if(col==4)
            return rule->isRegExp?Qt::Checked:Qt::Unchecked;
    }
        break;
    case Qt::DecorationRole:
        if(rule->blockField==BlockRule::Field::DanmuColor && col==5)
        {
            bool ok;
            int color=rule->content.toInt(&ok,16);
            if(ok)
                return QColor(color>>16,(color>>8)&0xff,color&0xff);
        }
        break;
    }
	
	return QVariant();
}

bool Blocker::setData(const QModelIndex &index, const QVariant &value, int)
{
    int row=index.row(),col=index.column();
    BlockRule *rule=blockList.at(row);
    switch (col)
    {
    case 1:
        if(rule->enable==(value==Qt::Checked))return false;
        rule->enable=(value==Qt::Checked);
        break;
    case 2:
        if(rule->blockField==BlockRule::Field(value.toInt()))return false;
        rule->blockField=BlockRule::Field(value.toInt());
        break;
    case 3:
        if(rule->relation==BlockRule::Relation(value.toInt()))return false;
        rule->relation=BlockRule::Relation(value.toInt());
        break;
    case 4:
        if(rule->isRegExp==(value==Qt::Checked))return false;
        rule->isRegExp=(value==Qt::Checked);
        break;
    case 5:
        if(rule->content==value.toString())return false;
        rule->content=value.toString();
        break;
    default:
        return false;
    }
    updateDB(row,col);
    emit dataChanged(index,index);
    GlobalObjects::danmuPool->testBlockRule(rule);
    return true;
}

QVariant Blocker::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<6)return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags Blocker::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
	int col = index.column();
	if (index.isValid() && index.column() > 0)
	{
		if(col==1||col==4)
			return  Qt::ItemIsUserCheckable | defaultFlags;
		else if(col>0)
			return  Qt::ItemIsEditable | defaultFlags;
	}  
	return defaultFlags;
}



