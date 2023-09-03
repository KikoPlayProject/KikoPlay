#ifndef LUATABLEMODEL_H
#define LUATABLEMODEL_H

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QSet>
struct lua_State;
struct LuaItem
{
    LuaItem(LuaItem * parent = nullptr) : mParent(parent)
    {
        if(parent) parent->mChilds.append(this);
    }
    ~LuaItem()
    {
        qDeleteAll(mChilds);
    }

    enum LuaType
    {
        NIL,
        BOOLEAN,
        LIGHTUSERDATA,
        NUMBER,
        STRING,
        TABLE,
        FUNCTION,
        USERDATA,
        THREAD
    };

    QString key, value;
    LuaType keyType, valType;

    QVector<LuaItem*> mChilds;
    LuaItem * mParent;
};
class LuaTableModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LuaTableModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
    void setRoot(LuaItem *root);

    enum struct Columns
    {
        KEY,
        VALUE
    };
    const QStringList headers = {tr("Key"), tr("Value")};
    const QStringList luaTypes= {"nil", "boolean", "lightuserdata", "number", "string", "table", "function", "userdata", "thread"};

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &) const override {return headers.size();}

private:
    QSharedPointer<LuaItem> mRootItem;
};

void buildLuaItemTree(lua_State *L, LuaItem *parent, QSet<quintptr> &tableHash);

#endif // LUATABLEMODEL_H
