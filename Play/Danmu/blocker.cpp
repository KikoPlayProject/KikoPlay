#include "blocker.h"
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#define BlockNameRole Qt::UserRole+1
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
    case 0:
    case 5:
    {
        QLineEdit *lineEdit=static_cast<QLineEdit *>(editor);
        lineEdit->setText(index.data(col==0?BlockNameRole:Qt::DisplayRole).toString());
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
    blockFileName=GlobalObjects::dataPath+"block.xml";
    QFile blockFile(blockFileName);
    bool ret=blockFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    beginResetModel();
    QXmlStreamReader reader(&blockFile);
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="rule")
            {
                BlockRule *rule=new BlockRule;
                rule->id=reader.attributes().value("id").toInt();
                if(rule->id>=maxId)maxId=rule->id+1;
                rule->name=reader.attributes().value("name").toString();
                rule->blockField=BlockRule::Field(reader.attributes().value("field").toInt());
                rule->relation=BlockRule::Relation(reader.attributes().value("relation").toInt());
                rule->isRegExp=(reader.attributes().value("isRegExp")=="true");
                rule->enable=(reader.attributes().value("enable")=="true");
                rule->content=reader.readElementText().trimmed();
                blockList.append(rule);
            }
        }
        reader.readNext();
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
    rule->name=tr("New Rule");
    rule->blockField=BlockRule::Field::DanmuText;
    rule->relation=BlockRule::Relation::Contain;
    rule->isRegExp=true;
    rule->enable=true;
	int insertPosition = blockList.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition);
    blockList.append(rule);
    endInsertRows();
}

void Blocker::addBlockRule(BlockRule *rule)
{
    rule->id=maxId++;
    int insertPosition = blockList.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition);
    blockList.append(rule);
    endInsertRows();
    saveBlockRules();
    GlobalObjects::danmuPool->testBlockRule(rule);
}

void Blocker::removeBlockRule(const QModelIndexList &deleteIndexes)
{
    QList<int> rows;
    foreach (const QModelIndex &index, deleteIndexes)
    {
        if (index.isValid())
        {
            int row=index.row();
            rows.append(row);
        }
    }
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
    saveBlockRules();
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

bool Blocker::isBlocked(DanmuComment *danmu)
{
    for(BlockRule *rule:blockList)
    {
        if(rule->blockTest(danmu))
        {
            return true;
        }
    }
    return false;
}

void Blocker::save()
{
    if(ruleChanged)
    {
        saveBlockRules();
        ruleChanged=false;
    }
}

void Blocker::saveBlockRules()
{
    QFile blockFile(blockFileName);
    bool ret=blockFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&blockFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("rules");
    for(BlockRule *rule:blockList)
    {
        writer.writeStartElement("rule");
        writer.writeAttribute("id", QString::number(rule->id));
        writer.writeAttribute("name", rule->name);
        writer.writeAttribute("field", QString::number(rule->blockField));
        writer.writeAttribute("relation", QString::number(rule->relation));
        writer.writeAttribute("isRegExp", rule->isRegExp?"true":"false");
        writer.writeAttribute("enable", rule->enable?"true":"false");
        writer.writeCharacters(rule->content);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
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
            return QString("%1 - %2").arg(rule->id).arg(rule->name);
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
    case BlockNameRole:
        return rule->name;
    }
	
	return QVariant();
}

bool Blocker::setData(const QModelIndex &index, const QVariant &value, int)
{
    int row=index.row(),col=index.column();
    BlockRule *rule=blockList.at(row);
    switch (col)
    {
    case 0:
        rule->name=value.toString();
        break;
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
        rule->re.reset();
        break;
    default:
        return false;
    }
    emit dataChanged(index,index);
    GlobalObjects::danmuPool->testBlockRule(rule);
    ruleChanged=true;
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
    if (index.isValid())
	{
		if(col==1||col==4)
			return  Qt::ItemIsUserCheckable | defaultFlags;
        else
			return  Qt::ItemIsEditable | defaultFlags;
	}  
	return defaultFlags;
}



