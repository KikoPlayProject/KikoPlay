#ifndef RESSEARCHWINDOW_H
#define RESSEARCHWINDOW_H

#include <QWidget>
class QTreeView;
class QLabel;
class QLineEdit;
class SearchListModel;
class QPushButton;
class QComboBox;
class DialogTip;
class ResSearchWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ResSearchWindow(QWidget *parent = nullptr);
    void search(const QString &keyword, bool setSearchEdit=false);
private:
    QTreeView *searchListView;
    QLabel *totalPageTip,*busyLabel;
    QLineEdit *searchEdit,*pageEdit;
    QPushButton *prevPage, *nextPage;
    QComboBox *scriptCombo;
    DialogTip *dialogTip;
    SearchListModel *searchListModel;
    int totalPage,currentPage;
    bool isSearching;
    QString currentKeyword,currentScriptId;
    void setEnable(bool on);
    void pageTurning(int page);
signals:
    void addTask(const QStringList &urls);
protected:
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // RESSEARCHWINDOW_H
