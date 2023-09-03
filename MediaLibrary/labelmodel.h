#ifndef LABELMODEL_H
#define LABELMODEL_H
#include <QAbstractItemModel>
#include <QtCore>
#include <QColor>
#include "tagnode.h"
struct SelectedLabelInfo
{
    QStringList epPathTags, customTags, customPrefixTags;
    QSet<QString> timeTags, scriptTags;
};
class LabelModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum BrushType
    {
        TopLevel,
        ChildLevel
    };

    explicit LabelModel(QObject *parent = nullptr);

    ~LabelModel();
    static LabelModel *instance()
    {
        static LabelModel animeLabelModel;
        return &animeLabelModel;
    }

    void loadLabels();

    void selectedLabelList(const QModelIndexList &indexes,  SelectedLabelInfo &selectLabels);

    const QMap<QString, QSet<QString>> &epTags() const {return epPathAnimes;}
    const QMap<QString, QSet<QString>> &customTags() const {return tagAnimes;}

    void setBrushColor(BrushType type, const QColor &color);
public:
    void addTag(const QString &animeName, const QString &tag, TagNode::TagType type);
    int addCustomTags(const QString &animeName, const QStringList &tagList);
    void removeTag(const QString &animeName, const QString &tag, TagNode::TagType type);
    void removeTag(const QString &animeName, const QString &airDate, const QString &scriptId);
    void removeTag(const QModelIndex &index);
    const TagNode *getTag(const QModelIndex &index);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    inline virtual int columnCount(const QModelIndex &) const override {return 1;}
    virtual QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
signals:
    void tagRemoved(const QString &tag);
    void tagCheckedChanged();
private:
    TagNode *root;
    QMap<QString, QSet<QString>> tagAnimes, epPathAnimes;
    enum CategoryTag
    {
        C_SCRIPT, C_TIME, C_FILE, C_CUSTOM
    };
    TagNode *cateTags[C_CUSTOM+1];
    QColor foregroundColor[2];

    void addAnimeInfoTag();
    void addEpPathTag();
    void addCustomTag();

    TagNode *insertNode(TagNode *parent, const QString &strPath, int count, TagNode::TagType type, char split = '/');
    TagNode *insertNodeIndex(TagNode *parent, const QString &strPath, int count, TagNode::TagType type, char split = '/');
    void removeNodeIndex(TagNode *parent, const QString &strPath, bool removeAll = false, char split = '/');
    void setCheckStatus(TagNode *node, bool checked);

    void selectScript(QSet<QString> &scriptTags);
    void selectTime(QSet<QString> &timeTags);
    void selectFile(QStringList &paths);
};
class LabelProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LabelProxyModel(QObject *parent=nullptr):QSortFilterProxyModel(parent){}
    
public:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // LABELMODEL_H
