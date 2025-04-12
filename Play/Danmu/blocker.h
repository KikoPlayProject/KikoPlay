#ifndef BLOCKER_H
#define BLOCKER_H
#include <QAbstractItemModel>
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "common.h"
class ComboBoxDelegate : public KTreeviewItemDelegate
{
    Q_OBJECT

public:
    ComboBoxDelegate(QObject *parent = nullptr):KTreeviewItemDelegate(parent){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const override
    {
        editor->setGeometry(option.rect);
    }
};
class Blocker : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit Blocker(QObject *parent = nullptr);
    ~Blocker();
    enum struct Columns
    {
        ID,
        BLOCKED,
        ENABLE,
        FIELD,
        RELATION,
        REGEXP,
        PREFILTER,
        CONTENT
    };
    const QStringList fields={tr("Text"),tr("Color"),tr("User")};
    const QStringList relations={tr("Contain"),tr("Equal"),tr("NotEqual")};
    const QStringList headers={tr("Id-Title"),tr("Blocked"),tr("Enable"),tr("Field"),tr("Relation"),tr("RegExp"),tr("PreFilter"),tr("Content")};
    
public:
    void addBlockRule(BlockRule::Field field = BlockRule::Field::DanmuText);
    void addBlockRule(BlockRule *rule);
    void resetBlockCount();
    void removeBlockRule(const QModelIndexList &deleteIndexes);
    template<typename Iter>
    void checkDanmu(Iter begin, Iter end, bool updateRuleCount=true)
    {
        for(Iter i = begin; i != end; ++i)
        {
            for(BlockRule *rule:blockList)
            {
                if(rule->blockTest(&(**i), updateRuleCount))
                {
                    (*i)->blockBy=rule->id;
                    break;
                }
            }
        }
    }

    bool isBlocked(DanmuComment *danmu);
    void save();
    void preFilter(QVector<DanmuComment *> &danmuList);
    bool exportRules(const QString &fileName);
    int importRules(const QString &fileName);
    bool exportXmlRules(const QString &fileName);
    int importXmlRules(const QString &fileName);
private:
    QVector<BlockRule *> blockList;
    int maxId;
    bool ruleChanged;
    QString blockFileName;
    void saveBlockRules();


    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:blockList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};
class BlockProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    BlockRule::Field field;
public:
    explicit BlockProxyModel(QObject *parent=nullptr):QSortFilterProxyModel(parent), field(BlockRule::Field::DanmuText){}
    void setField(BlockRule::Field field);
public:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // BLOCKER_H
