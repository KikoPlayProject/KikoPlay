#ifndef BLOCKER_H
#define BLOCKER_H
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include "common.h"
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ComboBoxDelegate(QObject *parent = nullptr):QStyledItemDelegate(parent){}

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

    const QStringList fields={tr("Text"),tr("Color"),tr("User")};
    const QStringList relations={tr("Contain"),tr("Equal"),tr("NotEqual")};
    const QStringList headers={tr("Id&Title"),tr("Enable"),tr("Field"),tr("Relation"),tr("RegExp"),tr("PreFilter"),tr("Content")};
    
public slots:
	void addBlockRule();
    void addBlockRule(BlockRule *rule);
    void removeBlockRule(const QModelIndexList &deleteIndexes);
	void checkDanmu(QList<DanmuComment *> &danmuList);
	void checkDanmu(QList<QSharedPointer<DanmuComment> > &danmuList);
    bool isBlocked(DanmuComment *danmu);
    void save();
    void preFilter(QList<DanmuComment *> &danmuList);
    int exportRules(const QString &fileName);
    int importRules(const QString &fileName);
private:
    QList<BlockRule *> blockList;
    int maxId;
    bool ruleChanged;
    QString blockFileName;
    void saveBlockRules();
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:blockList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:7;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // BLOCKER_H
