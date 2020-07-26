#include "blocker.h"
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Common/network.h"
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
    case 6:
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
                rule->usePreFilter=(reader.attributes().value("preFilter")=="true");
                rule->content=reader.readElementText().trimmed();
                rule->blockCount = 0;
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

void Blocker::addBlockRule(BlockRule::Field field)
{
    BlockRule *rule=new BlockRule;
    rule->id=maxId++;
    rule->name=tr("New Rule");
    rule->blockField=field;
    rule->relation=BlockRule::Relation::Contain;
    rule->isRegExp=true;
    rule->enable=true;
    rule->usePreFilter=false;
    rule->blockCount=0;
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

void Blocker::resetBlockCount()
{
    for(BlockRule *rule:blockList)
    {
        rule->blockCount = 0;
    }
}

void Blocker::removeBlockRule(const QModelIndexList &deleteIndexes)
{
    QList<int> rows;
    for (const QModelIndex &index : deleteIndexes)
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

void Blocker::preFilter(QList<DanmuComment *> &danmuList)
{
    QList<BlockRule *> preFilterRules;
    for(BlockRule *rule:blockList)
    {
        if(rule->usePreFilter)
            preFilterRules<<rule;
    }
    if(preFilterRules.isEmpty()) return;

    for(auto iter=danmuList.begin();iter!=danmuList.end();)
    {
        bool removed=false;
        for(BlockRule *rule:preFilterRules)
        {
            if(rule->blockTest(*iter))
            {
                removed=true;
                break;
            }
        }
        if(removed)
        {
            delete *iter;
            iter=danmuList.erase(iter);
        }
        else ++iter;
    }
}

int Blocker::exportRules(const QString &fileName)
{
    QFile kbrFile(fileName);
    bool ret=kbrFile.open(QIODevice::WriteOnly);
    if(!ret) return -1;
    QByteArray content;
    QBuffer buffer(&content);
    buffer.open(QIODevice::WriteOnly);
    QDataStream ds(&buffer);
    ds<<blockList.count();
    for(BlockRule *rule:blockList)
    {
        ds<<rule->name<<static_cast<int>(rule->blockField)<<static_cast<int>(rule->relation)
         <<rule->isRegExp<<rule->enable<<rule->usePreFilter<<rule->content;
    }
    QByteArray compressedContent;
    Network::gzipCompress(content,compressedContent);
    QDataStream fs(&kbrFile);
    fs<<QString("kbr")<<compressedContent;
    return 0;
}

int Blocker::importRules(const QString &fileName)
{
    QFile kbr(fileName);
    bool ret=kbr.open(QIODevice::ReadOnly);
    if(!ret) return -1;
    QDataStream fs(&kbr);
    QString head,comment;
    fs>>head;
    if(head!="kbr") return -2;

    QByteArray compressedConent,content;
    fs>>compressedConent;
    if(Network::gzipDecompress(compressedConent,content)!=0) return -2;
    QBuffer buffer(&content);
    buffer.open(QIODevice::ReadOnly);
    QDataStream ds(&buffer);
    int count=0;
    ds>>count;
    if(count<=0) return 0;

    int insertPosition = blockList.count();
    beginInsertRows(QModelIndex(), insertPosition, insertPosition+count-1);
    while(count-- >0)
    {
        BlockRule *rule=new BlockRule;
        rule->id=maxId++;
        int field,relation;
        ds>>rule->name>>field>>relation>>rule->isRegExp>>rule->enable
                     >>rule->usePreFilter>>rule->content;
        rule->blockField=BlockRule::Field(field);
        rule->relation=BlockRule::Relation(relation);
		blockList << rule;
        GlobalObjects::danmuPool->testBlockRule(rule);
    }
    endInsertRows();
    saveBlockRules();
    return 0;
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
        writer.writeAttribute("preFilter", rule->usePreFilter?"true":"false");
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
            return QString("%1 - %2 - %3").arg(rule->id).arg(rule->name).arg(rule->blockCount);
        }
        else if(col==2)
        {
            return fields.at(rule->blockField);
        }
        else if(col==3)
        {
            return relations.at(rule->relation);
        }
        else if(col==6)
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
        else if(col==5)
            return rule->usePreFilter?Qt::Checked:Qt::Unchecked;
    }
        break;
    case Qt::DecorationRole:
        if(rule->blockField==BlockRule::Field::DanmuColor && col==6)
        {
            bool ok;
            int color=rule->content.toInt(&ok,16);
            if(ok)
                return QColor(color>>16,(color>>8)&0xff,color&0xff);
        }
        break;
    case Qt::ToolTipRole:
        if(col==5) return tr("After pre-filtering is enabled, block rules will be applied when downloading danmu, which will not be added to the database");
        else if(col==6) return rule->content;
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
        if(rule->usePreFilter==(value==Qt::Checked))return false;
        rule->usePreFilter=(value==Qt::Checked);
        break;
    case 6:
        if(rule->content==value.toString())return false;
        rule->content=value.toString();
        rule->re.reset();
        break;
    default:
        return false;
    }
    emit dataChanged(index,index);
    if(col != 0 && col != 5)
        GlobalObjects::danmuPool->testBlockRule(rule);
    ruleChanged=true;
    return true;
}

QVariant Blocker::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<7)return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags Blocker::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
	int col = index.column();
    if (index.isValid())
	{
        if(col==1||col==4||col==5)
			return  Qt::ItemIsUserCheckable | defaultFlags;
        else
			return  Qt::ItemIsEditable | defaultFlags;
	}  
	return defaultFlags;
}


void BlockProxyModel::setField(BlockRule::Field field)
{
    this->field = field;
    invalidateFilter();
}

bool BlockProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 2, source_parent);
    return field == GlobalObjects::blocker->fields.indexOf(index.data(Qt::DisplayRole).toString());
}
