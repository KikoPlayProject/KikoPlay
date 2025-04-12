#include "lua_browser.h"
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrlQuery>
#include <QApplication>
#include <QWebEngineUrlScheme>
#include <QThread>
#include "Extension/Common/ext_common.h"
#include "Common/browser.h"
#include <QStandardPaths>

namespace Extension
{
const char *BrowserData::MetaName = "meta.kiko.browser";

BrowserData::BrowserData(QWebEngineView *_view) : QObject(nullptr), view(_view)
{

}

BrowserData::~BrowserData()
{
    if (view)
    {
        BrowserManager::instance()->release(view);
        view = nullptr;
    }
}

BrowserData *BrowserData::checkBrowser(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, BrowserData::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "browser expected");
    return *(BrowserData **)ud;
}

int BrowserData::browserGC(lua_State *L)
{
    BrowserData *d = checkBrowser(L, 1);
    if (d)
    {
        delete d;
    }
    return 0;
}

int BrowserData::load(lua_State *L)
{
    BrowserData *d = checkBrowser(L, 1);
    if (!d || !d->view || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        return 0;
    }
    // load(url, params, timeout)
    QUrl url(lua_tostring(L, 2));
    if (lua_gettop(L) > 2 && lua_istable(L, 3))
    {
        QUrlQuery query;
        lua_pushvalue(L, 3);
        auto q = getValue(L, false);
        lua_pop(L, 1);
        auto qmap = q.toMap();
        for (auto iter = qmap.constBegin(); iter != qmap.constEnd(); ++iter)
        {
            query.addQueryItem(iter.key(), iter.value().toString());
        }
        url.setQuery(query);
    }
    int timeout = 15*1000;
    if (lua_gettop(L) > 3 && lua_isinteger(L, 4))
    {
        timeout = lua_tointeger(L, 4);
        timeout = qMax(timeout, 1000);
    }

    std::promise<bool> cbPromise;
    std::future<bool> flag = cbPromise.get_future();

    auto conn = QObject::connect(d->view, &QWebEngineView::loadFinished, d->view, [&](bool ok){
        cbPromise.set_value(ok);
    });

    auto view = d->view;
    QMetaObject::invokeMethod(d->view, [=](){
        view->setUrl(url);
    });

    auto status = flag.wait_for(std::chrono::milliseconds{timeout});
    lua_pushboolean(L, status == std::future_status::ready ? flag.get() : false);
    QObject::disconnect(conn);
    return 1;
}

int BrowserData::show(lua_State *L)
{
    BrowserData *d = checkBrowser(L, 1);
    if (!d || !d->view)
    {
        return 0;
    }
    // show(tip)
    QString tipStr;
    if (lua_gettop(L) > 1 && lua_isstring(L, 2))
    {
        tipStr = lua_tostring(L, 2);
    }

    std::promise<bool> cbPromise;
    std::future<bool> flag = cbPromise.get_future();

    QMetaObject::invokeMethod(d->view, [&](){
        BrowserDialog dialog(d->view);
        dialog.setTip(tipStr);
        dialog.exec();
        cbPromise.set_value(true);
    });

    flag.wait();

    return 0;
}

int BrowserData::html(lua_State *L)
{
    BrowserData *d = checkBrowser(L, 1);
    if (!d || !d->view)
    {
        return 0;
    }

    std::promise<bool> cbPromise;
    std::future<bool> flag = cbPromise.get_future();

    QString content;
    auto resCallBack = [&](QVariant html){
        content = html.toString();
        cbPromise.set_value(true);
    };
    QMetaObject::invokeMethod(d->view, [=](){
        d->view->page()->runJavaScript("document.documentElement.outerHTML", resCallBack);
    });
    flag.get();

    lua_pushstring(L, content.toStdString().c_str());
    return 1;
}

int BrowserData::runjs(lua_State *L)
{
    BrowserData *d = checkBrowser(L, 1);
    if (!d || !d->view || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        return 0;
    }

    QString js(lua_tostring(L, 2));
    std::promise<bool> cbPromise;
    std::future<bool> flag = cbPromise.get_future();

    QVariant result;
    auto resCallBack = [&](QVariant res){
        result = res;
        cbPromise.set_value(true);
    };
    QMetaObject::invokeMethod(d->view, [=](){
        d->view->page()->runJavaScript(js, resCallBack);
    });
    flag.get();

    pushValue(L, result);
    return 1;
}

void Browser::setup()
{
    const luaL_Reg browserMembers[] = {
        {"load",  BrowserData::load},
        {"show",  BrowserData::show},
        {"html",  BrowserData::html},
        {"runjs",  BrowserData::runjs},
        {"__gc",  BrowserData::browserGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs(BrowserData::MetaName, browserMembers);

    const luaL_Reg funcs[] = {
        {"create", create},
        {"cookie", cookie},
        {"ua", ua},
        {"setua", setua},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "browser"}, funcs);
}

int Browser::create(lua_State *L)
{
    BrowserData **p = (BrowserData **)lua_newuserdata(L, sizeof(BrowserData *));
    luaL_getmetatable(L, BrowserData::MetaName);
    lua_setmetatable(L, -2);
    BrowserData *d = new BrowserData(BrowserManager::instance()->get());
    *p = d;
    return 1;
}

int Browser::cookie(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        auto allCookies = BrowserManager::instance()->getAllCookie();
        QVariantMap cookies;
        for (auto iter = allCookies.begin(); iter != allCookies.end(); ++iter)
        {
            QVariantMap vMap;
            for (auto vIter = iter.value().begin(); vIter != iter.value().end(); ++vIter)
            {
                vMap[vIter.key()] = vIter.value();
            }
            cookies[iter.key()] = vMap;
        }
        pushValue(L, cookies);
    }
    else if (lua_gettop(L) > 0 && lua_isstring(L, 1))
    {
        const QString domain = lua_tostring(L, 1);
        if (lua_gettop(L) > 1 && lua_isstring(L, 2))
        {
            const QString path = lua_tostring(L, 2);
            pushValue(L, BrowserManager::instance()->getDomainPathCookie(domain, path));
        }
        else
        {
            auto domainCookies = BrowserManager::instance()->getDomainCookie(domain);
            QVariantMap cookies;
            for (auto iter = domainCookies.begin(); iter != domainCookies.end(); ++iter)
            {
                cookies[iter.key()] = iter.value();
            }
            pushValue(L, cookies);
        }
    }
    return 1;
}

int Browser::ua(lua_State *L)
{
    pushValue(L, BrowserManager::instance()->getUserAgent());
    return 1;
}

int Browser::setua(lua_State *L)
{
    if (lua_gettop(L) < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        return 0;
    }

    QString ua(lua_tostring(L, 1));
    BrowserManager::instance()->setUserAgent(ua);
    return 0;
}

BrowserDialog::BrowserDialog(QWebEngineView *webView, QWidget *parent) : CFramelessDialog(tr("KikoPlay WebView"), parent, false)
{
    _view = webView;
    _tipLabel = new QLabel(this);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(_tipLabel);
    vLayout->addWidget(_view, 1);

    _view->show();
    _tipLabel->hide();

}

BrowserDialog::~BrowserDialog()
{
    if (_view)
    {
        _view->setParent(nullptr);
        _view->hide();
        _view = nullptr;
    }
}

void BrowserDialog::setTip(const QString &text)
{
    _tipLabel->setText(text);
    _tipLabel->adjustSize();
    _tipLabel->setVisible(!text.isEmpty());
}

void BrowserDialog::onClose()
{
    emit dialogClose();
    CFramelessDialog::onClose();
}

}
