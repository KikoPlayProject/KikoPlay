#ifndef MPVSHORTCUTPAGE_H
#define MPVSHORTCUTPAGE_H
#include <QAbstractItemModel>
#include <QSet>
#include "settingpage.h"
#include "UI/framelessdialog.h"
class QKeySequenceEdit;
class QLineEdit;
typedef QPair<QString, QPair<QString, QString>> ShortCutInfo;
Q_DECLARE_METATYPE(QList<ShortCutInfo>)
class ShortcutModel  : public QAbstractItemModel
{
    Q_OBJECT
public:
    ShortcutModel(QObject *parent = nullptr);
    ~ShortcutModel();
    const QHash<QString, QPair<QString, QString>> &shortcuts() const {return shortcutHash;}
    void addShortcut(const ShortCutInfo &newShortcut);
    void removeShortcut(const QModelIndex &index);
    void modifyShortcut(const QModelIndex &index, const ShortCutInfo &newShortcut);
public:
    enum struct Columns
    {
        KEY,
        COMMAND,
        DESC,
        NONE
    };
    QStringList headers = {tr("Shortcut Key"), tr("MPV Command"),tr("Description")};
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:shortcutList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:(int)Columns::NONE;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
    QList<ShortCutInfo> shortcutList;
    QHash<QString, QPair<QString, QString>> shortcutHash;
    bool modified;
};

class MPVShortcutPage : public SettingPage
{
    Q_OBJECT
public:
    MPVShortcutPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
};

class ShortcutEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    ShortcutEditDialog(bool add, ShortcutModel *model, const QModelIndex &index, QWidget *parent = nullptr);
private:
    ShortcutModel *shortcutModel;
    QKeySequenceEdit *keyEdit;
    QLineEdit *commandEdit;
    QLineEdit *descEdit;
    bool addShortcut;
    QSet<QString> existKeys;
public:
    QString key;
    QString command;
    QString desc;
protected:
    void onAccept() override;
};
QDataStream &operator<<(QDataStream &out, const QList<ShortCutInfo> &l);
QDataStream &operator>>(QDataStream &in, QList<ShortCutInfo> &l);
#endif // MPVSHORTCUTPAGE_H
