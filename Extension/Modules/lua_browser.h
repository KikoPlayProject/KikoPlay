#ifndef LUA_BROWSER_H
#define LUA_BROWSER_H
#include "modulebase.h"
#include <QObject>
#include "UI/framelessdialog.h"

class QWebEngineView;
class QLabel;

namespace Extension
{

class BrowserDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    BrowserDialog(QWebEngineView *webView, QWidget *parent = nullptr);
    ~BrowserDialog();

    void setTip(const QString &text);

signals:
    void dialogClose();

private:
    QWebEngineView *_view{nullptr};
    QLabel *_tipLabel{nullptr};
    // CFramelessDialog interface
protected:
    virtual void onClose();
};

class BrowserData : public QObject
{
    Q_OBJECT
public:
    BrowserData(QWebEngineView *_view);
    ~BrowserData();

    QWebEngineView *view{nullptr};

    static const char *MetaName;
    static BrowserData *checkBrowser(lua_State *L, int pos);

    static int browserGC(lua_State *L);
    static int load(lua_State *L);
    static int show(lua_State *L);

    static int html(lua_State *L);
    static int runjs(lua_State *L);
};

class Browser : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int create(lua_State *L);
    static int cookie(lua_State *L);
    static int ua(lua_State *L);
    static int setua(lua_State *L);
};

}

#endif // LUA_BROWSER_H
