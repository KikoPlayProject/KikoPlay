#ifndef ADDRULE_H
#define ADDRULE_H

#include "framelessdialog.h"
#include <QAbstractItemModel>
struct DownloadRule;
class QLineEdit;
class QComboBox;
class QSpinBox;
class DirSelectWidget;
class QTreeView;
class ScriptSearchOptionPanel;
class PreviewModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PreviewModel(QObject *parent = nullptr) : QAbstractItemModel(parent){}
private:
    struct SearchItem
    {
        QString title;
        int size;
        bool matched;
    };
    QList<SearchItem> searchResults;
    QString lastSearchWord,lastScriptId;
    float getSize(const QString &sizeStr);
    bool checkSize(int size, int min, int max);
public:
    void search(const QString &searchWord, const QString &scriptId, const QMap<QString, QString> *options=nullptr);
    void filter(const QString &filterWord, int minSize, int maxSize);
signals:
    void showError(const QString &err);
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:searchResults.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:2;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

class AddRule : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddRule(DownloadRule *rule=nullptr, QWidget *parent = nullptr);
    DownloadRule *curRule;
private:
    bool addRule;
    QLineEdit *nameEdit, *searchWordEdit, *filterWordEdit;
    QComboBox *scriptIdCombo, *downloadCombo;
    ScriptSearchOptionPanel *scriptOptionPanel;
    QSpinBox *minSizeSpin, *maxSizeSpin, *searchIntervalSpin;
    DirSelectWidget *filePathSelector;
    QTreeView *preview;
    PreviewModel *previewModel;

    void setupSignals();
    // CFramelessDialog interface
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // ADDRULE_H
