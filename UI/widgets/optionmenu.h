#ifndef OPTIONMENU_H
#define OPTIONMENU_H

#include <QWidget>
#include <QStackedWidget>
#include <QVariant>
#include <QScrollArea>
class OptionMenuPanel;
class OptionMenuItem;
class FloatScrollBar;
class QVBoxLayout;
class QHBoxLayout;
class QPropertyAnimation;
class OptionMenu : public QStackedWidget
{
    Q_OBJECT
public:
    explicit OptionMenu(QWidget *parent = nullptr);
    virtual QSize sizeHint() const;

    enum SizeAnchor
    {
        BottomRight, BottomLeft, TopLeft, TopRight
    };

    OptionMenuPanel *defaultMenuPanel() { return _defaultPanel; };
    OptionMenuPanel *addMenuPanel(const QString &name = "");
    OptionMenuPanel *getPanel(const QString &name);
    void switchToPanel(OptionMenuPanel *menuPanel, SizeAnchor anchor = BottomRight);
    void back(SizeAnchor anchor = BottomRight);
    void setPanel(OptionMenuPanel *menuPanel, SizeAnchor anchor = BottomRight);
    void updateSize(const QSize &size, SizeAnchor anchor = BottomRight);

signals:
private:
    OptionMenuPanel *_defaultPanel{nullptr};
    QVector<OptionMenuPanel *> panelStack;
    QPropertyAnimation* geoAnimation{nullptr};
    QMap<QString, OptionMenuPanel *> panelMap;
};

class OptionMenuPanel : public QScrollArea
{
    Q_OBJECT
    friend class OptionMenu;
public:
    explicit OptionMenuPanel(OptionMenu *menu);
    virtual QSize sizeHint() const;

    OptionMenuItem *addMenu(const QString &text);
    OptionMenuItem *addTitleMenu(const QString &text);
    void addSpliter();
    QVector<OptionMenuItem *> getCheckGroupMenus(const QString &group);
    void clearCheckGroup(const QString &group);

    void removeMenu(OptionMenuItem *menu);
    void removeFlag(const QString &flag);
private:
    QWidget *_itemContainer{nullptr};
    FloatScrollBar *_scroll{nullptr};
    int _maxHeight{350};
    OptionMenu *_menu;
    QVBoxLayout *_menuLayout{nullptr};
    QVector<OptionMenuItem *> menus;

};

class OptionMenuItem : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(OptionMenuItem)
    friend class OptionMenuPanel;
protected:
    explicit OptionMenuItem(OptionMenu *menu, OptionMenuPanel *panel);

public:
    virtual QSize sizeHint() const override;
    QString text() const { return _text; }
    void setText(const QString &text);
    void setInfoText(const QString &text);
    void setCheckable(bool checkable = false) { _checkAble = checkable; }
    bool isChecking() const { return _isChecking; }
    void setChecking(bool check);
    void setShowSubIndicator(bool showIndicator = true) { _showSubIndicator = showIndicator; }
    void setSelectAble(bool selectAble) { _selectAble = selectAble; }
    void setWidget(QWidget *widget, bool full = false);
    void setData(const QVariant &data) { _menuData = data; }
    QVariant menuData() const { return _menuData; }
    void setCheckGroup(const QString &group) { _checkGroup = group; }
    void setFlag(const QString &flag) { _flag = flag; }
    void setTextCenter(bool center) { _textCenter = center; }
    void setFixHeight(int h) { _fixHeight = h; setFixedHeight(h);}
    void setMinWidth(int w) { _minWidth = w; }
    void setMarginRight(int m) { _contentMarginRight = m; }

    OptionMenuPanel *addSubMenuPanel();
    void switchTo();
signals:
    void click();
protected:
    virtual bool event(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
protected:
    OptionMenu *_menu;
    OptionMenuPanel *_panel;

    QString _text;
    QString _infoText;
    int _contentMarginLeft{14};
    int _contentMarginRight{8};
    int _iconWidth{32};
    int _maxInfoTextWidth{80};
    int _maxTextWidth{240};
    int _fixHeight{40};
    int _minWidth{240};
    bool _checkAble{false};
    bool _showSubIndicator{true};
    bool _isChecking{false};
    bool _selectAble{true};
    bool _textCenter{false};
    QHBoxLayout *_menuHLayout{nullptr};
    QVariant _menuData;
    QString _checkGroup;
    QString _flag;

    OptionMenuPanel *_subMenuPanel{nullptr};
};

class TitleMenuItem : public OptionMenuItem
{
    Q_OBJECT
    Q_DISABLE_COPY(TitleMenuItem)
    friend class OptionMenuPanel;
protected:
    explicit TitleMenuItem(OptionMenu *menu, OptionMenuPanel *panel);
};

class MenuSpliter : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(MenuSpliter)
    friend class OptionMenuPanel;
protected:
    explicit MenuSpliter(OptionMenuPanel *panel);
protected:
    virtual void paintEvent(QPaintEvent* event) override;
};

#endif // OPTIONMENU_H
