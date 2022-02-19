#include "scriptbase.h"
#include <QFile>
#include <QRegularExpression>
#include <QVariant>
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include "Common/notifier.h"
#include "Common/logger.h"
#include <QSysInfo>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#define LOG_INFO(info, scriptId) Logger::logger()->log(Logger::Script, QString("[%1]%2").arg(scriptId, info))
#define LOG_ERROR(info, scriptId) Logger::logger()->log(Logger::Script, QString("[ERROR][%1]%2").arg(scriptId, info))
namespace
{
static void pushNetworkReply(lua_State *L, const Network::Reply &reply)
{
    lua_newtable(L); // table

    lua_pushstring(L, "statusCode"); // table key
    lua_pushnumber(L, reply.statusCode); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "hasError"); // table key
    lua_pushboolean(L, reply.hasError); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "errInfo"); // table key
    lua_pushstring(L, reply.errInfo.toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "content"); // table key
    lua_pushlstring(L, reply.content.constData(), reply.content.size()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "headers"); // table key
    lua_newtable(L); // table key table
    for(auto &p : reply.headers)
    {
        lua_pushlstring(L, p.first.constData(), p.first.size()); // table key table key
        lua_pushlstring(L, p.second.constData(), p.second.size()); // table key table key value
        lua_rawset(L, -3); //table key table
    }
    lua_rawset(L, -3); //table
}
static int httpGet(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <query> <header> <redirect>
        if(params==0 || params>4) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        QUrlQuery query;
        QStringList headers;
        bool redirect = true;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = ScriptBase::getValue(L);
            lua_pop(L, 1);
			if (q.type() != QVariant::Map && !(q.type() == QVariant::List && q.toList().size() == 0)) break;
            auto qmap = q.toMap();
            for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
            {
                query.addQueryItem(iter.key(),iter.value().toString());
            }
        }
        if(params > 2)  //has header
        {
            lua_pushvalue(L, 3);
            auto h = ScriptBase::getValue(L);
            lua_pop(L, 1);
			if (h.type() != QVariant::Map && !(h.type() == QVariant::List && h.toList().size() == 0)) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        if(params > 3)
        {
            redirect = lua_toboolean(L, 4);
        }
        Network::Reply &&reply = Network::httpGet(curl,query,headers,redirect);
        if(!reply.hasError)
        {
            lua_pushnil(L);
            pushNetworkReply(L, reply);
        }
        else
        {
            lua_pushstring(L, reply.errInfo.toStdString().c_str());
            lua_pushnil(L);
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httpget: param error, expect: url(string), <query(table)>, <header(table)>, <redirect=true>");
    lua_pushnil(L);
    return 2;
}
static int httpPost(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <data> <header>
        if(params<2 || params>3) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        if(lua_type(L, 2)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        size_t dataLength = 0;
        const char *data = lua_tolstring(L, 2, &dataLength);
        QByteArray cdata(data, dataLength);
        QStringList headers;
        if(params > 2)  //has header
        {
            auto h = ScriptBase::getValue(L);
			if (h.type() != QVariant::Map && !(h.type() == QVariant::List && h.toList().size() == 0)) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        Network::Reply &&reply = Network::httpPost(curl,cdata,headers);
        if(!reply.hasError)
        {
            lua_pushnil(L);
            pushNetworkReply(L, reply);
        }
        else
        {
            lua_pushstring(L, reply.errInfo.toStdString().c_str());
            lua_pushnil(L);
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httppost: param error, expect: url(string), <data(string)>, <header(table)>");
    lua_pushnil(L);
    return 2;
}
static int json2table(lua_State *L)
{
    int params = lua_gettop(L);  //jsonstr
    if(params!=1 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "json2table: param error, expect: jsonstr(string)");
        lua_pushnil(L);
        return 2;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 1, &dataLength);
    QByteArray cdata(data, dataLength);
    try {
       auto jdoc = Network::toJson(cdata);
       lua_pushnil(L);
       if(jdoc.isArray())
       {
           ScriptBase::pushValue(L, jdoc.array().toVariantList());
       }
       else if(jdoc.isObject())
       {
           ScriptBase::pushValue(L, jdoc.object().toVariantMap());
       }
       else
       {
           lua_newtable(L);
       }
       return 2;
    } catch (Network::NetworkError &err) {
        lua_pushstring(L,err.errorInfo.toStdString().c_str());
        lua_pushnil(L);
        return 2;
    }
}
static int table2json(lua_State *L)
{
    int params = lua_gettop(L);
    if(params!=1 || lua_type(L, 1)!=LUA_TTABLE)
    {
        lua_pushstring(L, "table2json: param error, expect: table");
        lua_pushnil(L);
        return 2;
    }
    QVariant table = ScriptBase::getValue(L);
    QByteArray json = QJsonDocument::fromVariant(table).toJson(QJsonDocument::JsonFormat::Indented);
    lua_pushnil(L);
    lua_pushstring(L, json.constData());
    return 2;
}
static int httpGetBatch(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //urls([u1,u2,...]) <querys([{xx=xx,...},{xx=xx,...},...])> <headers([{xx=xx,..},{xx=xx,..},...])>, <redirect=true>
        if(params==0 || params>3) break;
        if(lua_type(L, 1)!=LUA_TTABLE) break;
        lua_pushvalue(L, 1);
        auto us = ScriptBase::getValue(L);
        lua_pop(L, 1);
        if(!us.canConvert(QVariant::StringList)) break;
        auto urls = us.toStringList();
        QList<QUrlQuery> querys;
        QList<QStringList> headers;
        bool redirect = true;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = ScriptBase::getValue(L);
            lua_pop(L, 1);
            if(q.type()!=QVariant::List) break;
            auto qs = q.toList();
            if(qs.size()>0 && qs.size()!=urls.size()) break;
            for(auto &qobj : qs)
            {
                auto qmap = qobj.toMap();
                QUrlQuery query;
                for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
                {
                    query.addQueryItem(iter.key(),iter.value().toString());
                }
                querys.append(query);
            }
        }
        if(params > 2)  //has header
        {
            lua_pushvalue(L, 3);
            auto h = ScriptBase::getValue(L);
            lua_pop(L, 1);
            if(h.type()!=QVariant::List) break;
            auto hs = h.toList();
            if(hs.size()>0 && hs.size()!=urls.size()) break;
            for(auto &hobj : hs)
            {
                auto hmap = hobj.toMap();
                QStringList header;
                for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
                {
                    header<<iter.key()<<iter.value().toString();
                }
                headers.append(header);
            }
        }
        if(params > 3)
        {
            redirect = lua_toboolean(L, 4);
        }
        QList<Network::Reply> &&content = Network::httpGetBatch(urls,querys,headers); //[[hasError, content], [], ...]
        lua_pushnil(L);
        lua_newtable(L); // table
        for(int i=0; i<content.size(); ++i)
        {
            pushNetworkReply(L, content[i]); //table reply
            lua_rawseti(L, -2, i+1); //table
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httpgetbatch: param error, expect: urls(string array), <querys(table array)>, <headers(table array)>, <redirect=true>");
    lua_pushnil(L);
    return 2;
}
static int compress(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error, expect: content(string), <type(gzip)>");
        lua_pushnil(L);
        return 2;
    }
    bool useGzip = false;
    if(params == 2 && lua_type(L, 2)==LUA_TSTRING)
    {
        const char *method = lua_tostring(L, 2);
        if(strcmp(method, "gzip")==0) useGzip = true;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 2, &dataLength);
    QByteArray cdata(data, dataLength), outdata;
    int ret = 0;
    if(useGzip) ret = Network::gzipCompress(cdata, outdata);
    else ret = Network::gzipCompress(cdata, outdata);
    if(ret!=0)
    {
        lua_pushstring(L, "compress: data error");
        lua_pushnil(L);
        return 2;
    }
    lua_pushnil(L);
    lua_pushlstring(L, outdata.constData(), outdata.size());
    return 2;
}
static int decompress(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error, expect: content(string), <type(inflate(default)/gzip)>");
        lua_pushnil(L);
        return 2;
    }
    bool useGzip = false;
    if(params == 2 && lua_type(L, 2)==LUA_TSTRING)
    {
        const char *method = lua_tostring(L, 2);
        if(strcmp(method, "gzip")==0) useGzip = true;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 1, &dataLength);
    QByteArray cdata(data, dataLength), outdata;
    int ret = 0;
    if(useGzip) ret = Network::gzipDecompress(cdata, outdata);
    else ret = Network::decompress(cdata, outdata);
    if(ret!=0)
    {
        lua_pushstring(L, "decompress: data error");
        lua_pushnil(L);
        return 2;
    }
    lua_pushnil(L);
    lua_pushlstring(L, outdata.constData(), outdata.size());
    return 2;
}
static int writeSetting(lua_State *L)
{
    int params = lua_gettop(L);  //key value
    if(params!=2 || lua_type(L, 1)!=LUA_TSTRING || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushstring(L, "writesetting: param error, expect: key(string), value(string)");
        lua_pushnil(L);
        return 2;
    }
    lua_pushstring(L, "kiko_scriptobj");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ScriptBase *script = (ScriptBase *)lua_topointer(L, -1);
    lua_pop(L, 1);
    QString errInfo = script->setOption(lua_tostring(L, 1), lua_tostring(L, 2), false);
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    lua_pushnil(L);
    return 2;
}
static int execute(lua_State *L)
{
    int params = lua_gettop(L);  // detached(bool) program(string) args(array)
    if(params!=3 || lua_type(L, 1)!=LUA_TBOOLEAN || lua_type(L, 2)!=LUA_TSTRING || lua_type(L, 3)!=LUA_TTABLE)
    {
        lua_pushstring(L, "execute: param error, expect: detached(bool), program(string), args(array)");
        lua_pushnil(L);
        return 2;
    }
    bool detached = lua_toboolean(L, 1);
    QString program = lua_tostring(L, 2);
    QStringList args = ScriptBase::getValue(L).toStringList();
    if(detached)
    {
        bool ret = QProcess::startDetached(program, args);
        lua_pushnil(L);
        lua_pushboolean(L, ret);
    }
    else
    {
        int ret = QProcess::execute(program, args);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
    }
    return 2;
}
static int hashData(lua_State *L)
{
    int params = lua_gettop(L);  // path(string) <filesize(bytes,number)>
    if(params==0 || params>4 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "md5file: param error, expect: path/data(string), <ispath(boolean,default=true)>, <filesize(number)>, <algorithm(default=md5)>");
        lua_pushnil(L);
        return 2;
    }
    size_t len = 0;
    const char *pd = lua_tolstring(L, 1, &len);
    bool isPath = true;
    if(params > 1)  //has isdata
    {
        isPath = lua_toboolean(L, 2);
    }
    qint64 size = 0;
    if(params > 2)  //has filesize
    {
        size = lua_tonumber(L, 3);
    }
    static QHash<QString, QCryptographicHash::Algorithm> algorithms({
        {"md4", QCryptographicHash::Md4},
        {"md5", QCryptographicHash::Md5},
        {"sha1", QCryptographicHash::Sha1},
        {"sha224", QCryptographicHash::Sha224},
        {"sha256", QCryptographicHash::Sha256},
        {"sha384", QCryptographicHash::Sha384},
        {"sha512", QCryptographicHash::Sha512}
    });
    QCryptographicHash::Algorithm algo = QCryptographicHash::Md5;
    if(params > 3)  //has algorithm
    {
        algo = algorithms.value(QString(lua_tostring(L, 4)).toLower(), QCryptographicHash::Md5);
    }
    QByteArray hashResult;
    if(isPath)
    {
        QFile file(pd);
        if(!file.open(QIODevice::ReadOnly))
        {
            lua_pushstring(L, QString("md5file: open '%1' failed").arg(pd).toStdString().c_str());
            lua_pushnil(L);
            return 2;
        }
        QByteArray fileData(size==0?file.readAll():file.read(size));
        hashResult = QCryptographicHash::hash(fileData, algo).toHex();

    }
    else
    {
        QByteArray hashInput(pd, len);
        hashResult = QCryptographicHash::hash(hashInput, algo).toHex();
    }
    lua_pushnil(L);
    lua_pushlstring(L, hashResult.data(), hashResult.size());
    return 2;
}
static int logger(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0) return 0;
    lua_pushstring(L, "kiko_scriptobj");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ScriptBase *script = (ScriptBase *)lua_topointer(L, -1);
    lua_pop(L, 1);
    if(params > 1)
    {
        QStringList vals;
        for(int i=1;i<=params;++i)
        {
            if(lua_type(L, i) == LUA_TTABLE)
            {
                vals.append("[Table]");
            }
            else
            {
                lua_pushvalue(L, i);
                vals.append(ScriptBase::getValue(L).toString());
                lua_pop(L, 1);
            }
        }
        LOG_INFO(vals.join('\t'), script->id());
        return 0;
    }
    lua_pushvalue(L, 1);
    auto val = ScriptBase::getValue(L);
    lua_pop(L, 1);
    QString logText;
    if(val.type()==QVariant::List || val.type()==QVariant::Map)
    {
        QString json(QJsonDocument::fromVariant(val).toJson(QJsonDocument::JsonFormat::Indented));
        LOG_INFO("Show Table: ", script->id());
        Logger::logger()->log(Logger::Script, json);
    }
    else
    {
        LOG_INFO(val.toString(), script->id());
    }
    return 0;
}
static int message(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || lua_type(L, 1)!=LUA_TSTRING) return 0;
    lua_pushstring(L, "kiko_scriptobj");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ScriptBase *script = (ScriptBase *)lua_topointer(L, -1);
    lua_pop(L, 1);
    QString message(lua_tostring(L, 1));
    int flags = NotifyMessageFlag::NM_HIDE;
    if(params > 1 && lua_type(L, 2)==LUA_TNUMBER)
    {
        flags = lua_tonumber(L, 2);
    }
    switch (script->type())
    {
    case ScriptType::LIBRARY:
        Notifier::getNotifier()->showMessage(Notifier::NotifyType::LIBRARY_NOTIFY, message, flags);
        break;
    default:
        break;
    }
    return 0;
}
static int dialog(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || lua_type(L, 1)!=LUA_TTABLE) return 0;
    QVariant table = ScriptBase::getValue(L, false);
    if(table.type()==QVariant::Map)
    {
        QVariantMap map = table.toMap();
        QVariantMap inputs;
        if(map.contains("title"))
            inputs["title"] = QString(map["title"].toByteArray());
        if(map.contains("tip"))
            inputs["tip"] = QString(map["tip"].toByteArray());
        if(map.contains("text"))
            inputs["text"] = QString(map["text"].toByteArray());
        if(map.contains("image"))
            inputs["image"] = map["image"].toByteArray();
        QStringList rets = Notifier::getNotifier()->showDialog(Notifier::MAIN_DIALOG_NOTIFY, inputs).toStringList();
        lua_pushstring(L, rets[0].toStdString().c_str()); // accept/reject
        lua_pushstring(L, rets[1].toStdString().c_str());  //text
        return 2;
    }
    return 0;
}
#ifdef Q_OS_WIN
const QString stTrans(const QString &str, bool toSimplified)
{
    WORD wLanguageID = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    LCID Locale = MAKELCID(wLanguageID, SORT_CHINESE_PRCP);
    QScopedPointer<QChar> buf(new QChar[str.length()]);
    LCMapString(Locale,toSimplified?LCMAP_SIMPLIFIED_CHINESE:LCMAP_TRADITIONAL_CHINESE,reinterpret_cast<LPCWSTR>(str.constData()),str.length(),reinterpret_cast<LPWSTR>(buf.data()),str.length());
    return QString(buf.data(), str.length());
}
#endif
static int simplifiedTraditionalTrans(lua_State *L)
{
#ifdef Q_OS_WIN
    int params = lua_gettop(L);  // str(string) toSimpOrTrad(bool)
    if(params!=2 ||  lua_type(L, 1)!=LUA_TSTRING || lua_type(L, 2)!=LUA_TBOOLEAN)
    {
        lua_pushstring(L, "stTrans: param error, expect: str(string) toSimpOrTrad(bool, true: toSimp)");
        lua_pushnil(L);
        return 2;
    }
    QString input(lua_tostring(L, 1));
    QString trans(stTrans(input, lua_toboolean(L, 2)));
    lua_pushnil(L);
    lua_pushstring(L, trans.toStdString().c_str());
    return 2;
#else
    lua_pushnil(L);
    lua_pushvalue(L, 1);
    return 2;
#endif
}
static int envInfo(lua_State *L)
{
    lua_newtable(L); // table

    lua_pushstring(L, "os"); // table key
    lua_pushstring(L, QSysInfo::productType().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "os_version"); // table key
    lua_pushstring(L, QSysInfo::productVersion().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    QFile version(":/res/version.json");
    version.open(QIODevice::ReadOnly);
    QJsonObject curVersionObj = QJsonDocument::fromJson(version.readAll()).object();
    QString versionStr=curVersionObj.value("Version").toString();

    lua_pushstring(L, "kikoplay"); // table key
    lua_pushstring(L, versionStr.toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    return 1;
}
//RegEx--------------------------
class RegEx
{
    QRegularExpression re;
    static QRegularExpression::PatternOptions parsePatternOptions(const char *options)
    {
        auto flags = QRegularExpression::PatternOptions();
        if(options!=nullptr)
            for(int i=0;options[i]!='\0';++i)
                switch(options[i])
                {
                case 'i': flags |= QRegularExpression::CaseInsensitiveOption; break;
                case 's': flags |= QRegularExpression::DotMatchesEverythingOption; break;
                case 'm': flags |= QRegularExpression::MultilineOption; break;
                case 'x': flags |= QRegularExpression::ExtendedPatternSyntaxOption; break;
                } // ignore unsupported options
        return flags;
    }
    QString errorMessage() const
    {
        return "invalid regex at "+QString::number(re.patternErrorOffset())+": "+re.errorString();
    }
public:
    explicit RegEx(const char *pattern, const char *options = nullptr)
        :re(QString(pattern),parsePatternOptions(options)){}
    void setPattern(const char *pattern) {re.setPattern(QString(pattern));}
    void setPatternOptions(const char *options) {re.setPatternOptions(parsePatternOptions(options));}
    const QString getPattern() const {return re.pattern();}
    const QString getPatternOptions() const
    {
        QString options("");
        auto flags = re.patternOptions();
        if(flags.testFlag(QRegularExpression::CaseInsensitiveOption)) options.append('i');
        if(flags.testFlag(QRegularExpression::DotMatchesEverythingOption)) options.append('s');
        if(flags.testFlag(QRegularExpression::MultilineOption)) options.append('m');
        if(flags.testFlag(QRegularExpression::ExtendedPatternSyntaxOption)) options.append('x');
        return options;
    };

    QString replace(QString &s, const QString &after) {return s.replace(re, after);}
    QRegularExpressionMatch matchOnce(const QString &s, int offset = 0) const {return re.match(s, offset);}
    QRegularExpressionMatchIterator match(const QString &s) const
    {
        if(!re.isValid()) throw std::logic_error(errorMessage().toStdString());
        return re.globalMatch(s);
    }
};
static int regex(lua_State *L)
{
    int n = lua_gettop(L);
    const char *pattern = nullptr;
    const char *options = nullptr;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        pattern = lua_tostring(L, 1);
    }
    if(n > 1)
    {
        luaL_argcheck(L, lua_type(L, 2)==LUA_TSTRING, 2, "expect string as pattern option");
        options = lua_tostring(L, 2);
        luaL_argcheck(L, n==2, 3, "too many arguments, 1-2 strings expected");
    }
    auto **regex = (RegEx **)lua_newuserdata(L, sizeof(RegEx *));
    luaL_getmetatable(L, "meta.kiko.regex");
    lua_setmetatable(L, -2);  // regex meta
    *regex = new RegEx(pattern, options); // meta
    return 1;
}
static RegEx *checkRegex(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.regex");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.regex' expected");
    return *(RegEx **)ud;
}
static int regexSetPattern(lua_State *L)
{
    RegEx *regex = checkRegex(L);
    int params = lua_gettop(L); // regex(kiko.regex) pattern(string) options(string)
    const char *pattern = nullptr;
    const char *options = nullptr;
    if(params<4 && lua_type(L, 2)==LUA_TSTRING)
    {
        pattern = lua_tostring(L, 2);
        if(params==3)
        {
            luaL_argcheck(L, lua_type(L, 3)==LUA_TSTRING, 3, "expect string as pattern option");
            options = lua_tostring(L, 3);
            regex->setPatternOptions(options);
        }
    }
    else
    {
        luaL_argerror(L, 4, "too many arguments or wrong type (expect string), setting pattern to none");
    }
    regex->setPattern(pattern);
    return 0;
}
static int regexGC(lua_State *L) {
    RegEx *regex = checkRegex(L);
    if(regex) delete regex;
    return 0;
}
static int regexFind(lua_State *L)
{
    RegEx *regex = checkRegex(L);
    int params = lua_gettop(L); // regex(kiko.regex) target(string) initpos(number)
    const char *errMsg = nullptr;
    int initpos = 0;
    if(params<2 || lua_type(L, 2)!=LUA_TSTRING)
    {
        errMsg = "expect string as match target";
    }
    else if(params==3 && lua_type(L, 3)==LUA_TNUMBER)
    {
        initpos = lua_tonumber(L, 3);
        if(initpos>0) initpos--; // 1-based
    }
    else if(params!=2)
    {
        errMsg = "expect number as initial matching position";
    }
    if(errMsg==nullptr)
    {
        const auto match = regex->matchOnce(QString(lua_tostring(L, 2)), initpos);
        const auto groups = match.capturedTexts();
        int count = 0;
        for(;count<groups.size();++count)
        {
            lua_createtable(L, 3, 0);
            lua_pushinteger(L, match.capturedStart(count)+1); // 1-based
            lua_seti(L, -2, 1);
            lua_pushinteger(L, match.capturedEnd(count)); // 0-based
            lua_seti(L, -2, 2);
            lua_pushstring(L, groups.at(count).toStdString().c_str());
            lua_seti(L, -2, 3);
        }
        if(count>0) return count;
    }
    else
    {
        luaL_error(L, "expect: regex(kiko.regex) target(string) initpos(number)");
    }
    lua_pushnil(L);
    return 1;
}
static QRegularExpressionMatchIterator *checkRegexMatch(lua_State *L, int idx)
{
    void *ud = luaL_checkudata(L, idx, "meta.kiko.regex.matchiter");
    luaL_argcheck(L, ud != NULL, idx, "`kiko.regex.matchiter' expected");
    return *(QRegularExpressionMatchIterator **)ud;
}
static int regexMatchGC(lua_State *L) {
    auto *it = checkRegexMatch(L, 1);
    if(it) delete it;
    return 0;
}
static int regexMatchIterator(lua_State *L)
{
    auto *it = checkRegexMatch(L, lua_upvalueindex(1));
    if(it->hasNext())
    {
        const auto next = it->next();
        const auto groups = next.capturedTexts();
        int count = 0;
        for(;count<groups.size();++count) // include whole match every time
        {
            lua_pushstring(L, groups.at(count).toStdString().c_str());
        }
        return count;
    }
    if(it) delete it;
    return 0;
}
static int regexMatch(lua_State *L)
{
    RegEx *regex = checkRegex(L);
    if(lua_gettop(L)!=2 || lua_type(L, 2)!=LUA_TSTRING) // regex(kiko.regex) target(string)
    {
        luaL_error(L, "expect string as match target");
        lua_pushnil(L);
        return 1;
    }
    try {
        auto *it = new QRegularExpressionMatchIterator(regex->match(lua_tostring(L, 2)));
        auto **pit = (Q_TYPEOF(it) *)lua_newuserdata(L, sizeof(Q_TYPEOF(it)));
        luaL_getmetatable(L, "meta.kiko.regex.matchiter");
        lua_setmetatable(L, -2);  // matchiter meta
        *pit = it; // meta
        lua_pushcclosure(L, &regexMatchIterator, 1);
    } catch(std::exception &e) {
        luaL_error(L, (QString(e.what())+", fix via method setpattern(<pattern>)").toStdString().c_str());
        lua_pushnil(L);
    }
    return 1;
}
static int regexSub(lua_State *L)
{
    RegEx *regex = checkRegex(L);
    int params = lua_gettop(L); // regex(kiko.regex) target(string) repl(string/table/function)
    const int replParamType = (params==3 && lua_type(L, 2)==LUA_TSTRING) ? lua_type(L, 3) : LUA_TNONE;
    if(replParamType!=LUA_TSTRING && replParamType!=LUA_TTABLE && replParamType!=LUA_TFUNCTION)
    {
        luaL_error(L, "expect: target(string), repl(string/table/function)");
        lua_pushnil(L);
        return 1;
    }
    QString target = QString(lua_tostring(L, 2));
    if (replParamType==LUA_TSTRING)
    {
        lua_pushstring(L, regex->replace(target, QString(lua_tostring(L, 3))).toStdString().c_str());
        return 1;
    }
    QRegularExpressionMatchIterator it;
    try {
        it = regex->match(lua_tostring(L, 2));
    } catch(std::exception &e) {
        luaL_error(L, (QString(e.what())+", fix via method setpattern(<pattern>)").toStdString().c_str());
        lua_pushnil(L);
        return 1;
    }
    const QVariantMap replTable = ScriptBase::getValue(L).toMap(); // empty if not table
    QString replResult;
    int lastCapturedEnd = 0;
    for(int matchCount=1;it.hasNext();++matchCount)
    {
        const auto next = it.next();
        replResult.append(target.mid(lastCapturedEnd, next.capturedStart()-lastCapturedEnd));
        QString replText;
        lastCapturedEnd = next.capturedEnd();
        const int nRet = next.capturedTexts().size();
        if(replParamType==LUA_TTABLE)
        {
            replText = next.captured(); // load group 0
            QString groupReplText;
            const int start0 = next.capturedStart();
            int cursor = start0, offset = 0, start, len;
            for(int i=1;i<=nRet && cursor<next.capturedEnd();++i) // unlike in Lua, look up every capturing group
            {
                if(cursor>=next.capturedEnd(i)) continue; // skip groups starting before cursor position
                if(replTable.contains(next.captured(i)) && !replTable[next.captured(i)].toString().isNull())
                {
                    start = next.capturedStart(i)-start0+offset; // adjust local start by last offset
                    len = next.capturedLength(i);
                    groupReplText = replTable[next.captured(i)].toString();
                    offset += groupReplText.length()-len; // update offset by length difference due to replacement
                    replText.remove(start, len).insert(start, groupReplText); // replace
                    cursor = next.capturedEnd(i);
                }
            }
        }
        else // LUA_TFUNCTION
        {
            lua_pushvalue(L, 3); // duplicate replacement function
            for(int i=0;i<nRet;++i)
            {
                lua_pushstring(L, next.captured(i).toStdString().c_str()); // cf. gmatch
            }
            if(lua_pcall(L, nRet, 1, 0)==0) // replace
            {
                replText = QString(lua_tostring(L, -1));
                if(replText==NULL) // fall back if return value not of type string
                {
                    replText = next.captured();
                }
            }
            else
            {
                luaL_error(L, "replacement function failed on match %d: %s", matchCount, lua_tostring(L, -1));
                replText = next.captured();
            }
            lua_pop(L, 1);
        }
        replResult.append(replText);
    }
    lua_pushstring(L, replResult.append(target.mid(lastCapturedEnd)).toStdString().c_str());
    return 1;
}
//RegEx End----------------------
//XmlReader----------------------
static int xmlreader (lua_State *L)
{
    int n = lua_gettop(L);
    const char *data = nullptr;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        data = lua_tostring(L, 1);
    }
    QXmlStreamReader **reader = (QXmlStreamReader **)lua_newuserdata(L, sizeof(QXmlStreamReader *));
    luaL_getmetatable(L, "meta.kiko.xmlreader");
    lua_setmetatable(L, -2);  // reader meta
    *reader = new QXmlStreamReader(data); //meta
    return 1;
}
static QXmlStreamReader *checkXmlReader(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.xmlreader");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.xmlreader' expected");
    return *(QXmlStreamReader **)ud;
}
static int xrAddData(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) > 1 && lua_type(L, 2)==LUA_TSTRING)
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 2, &len);
        reader->addData(QByteArray(data, len));
    }
    return 0;
}
static int xrClear(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->clear();
    return 0;
}
static int xrEnd(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool atEnd = reader->atEnd();
    lua_pushboolean(L, atEnd);
    return 1;
}
static int xrReadNext(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->readNext();
    return 0;
}
static int xrStartElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isStartElem = reader->isStartElement();
    lua_pushboolean(L, isStartElem);
    return 1;
}
static int xrEndElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isEndElem = reader->isEndElement();
    lua_pushboolean(L, isEndElem);
    return 1;
}
static int xrName(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->name().toString().toStdString().c_str());
    return 1;
}
static int xrAttr(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) == 1 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *attrName = lua_tostring(L, 2);
    lua_pushstring(L, reader->attributes().value(attrName).toString().toStdString().c_str());
    return 1;
}
static int xrHasAttr(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) == 1 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *attrName = lua_tostring(L, 2);
    lua_pushboolean(L, reader->attributes().hasAttribute(attrName));
    return 1;
}
static int xrReadElemText(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->readElementText().toStdString().c_str());
    return 1;
}
static int xrError(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    QString errInfo = reader->errorString();
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    return 1;
}
static int xmlreaderGC (lua_State *L) {
    QXmlStreamReader *reader = checkXmlReader(L);
    if(reader) delete reader;
    return 0;
}
//XmlReader End------------------

//HTMLParserSax------------------
static int htmlparser (lua_State *L)
{
    int n = lua_gettop(L);
    const char *data = nullptr;
    size_t length = 0;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        data = lua_tolstring(L, -1, &length);
    }
    HTMLParserSax **parser = (HTMLParserSax **)lua_newuserdata(L, sizeof(HTMLParserSax *));
    luaL_getmetatable(L, "meta.kiko.htmlparser");
    lua_setmetatable(L, -2);  // reader meta
    *parser = new HTMLParserSax(QByteArray(data, length)); //reader
    return 1;
}
static HTMLParserSax *checkHTMLParser(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.htmlparser");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.htmlparser' expected");
    return *(HTMLParserSax **)ud;
}
static int hpAddData(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    if(lua_gettop(L) > 1 && lua_type(L, 2)==LUA_TSTRING)
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 2, &len);
        parser->addData(QByteArray(data, len));
    }
    return 0;
}
static int hpSeekTo(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!=2 || lua_type(L, 2)!=LUA_TNUMBER)
    {
        return 0;
    }
    int pos = lua_tonumber(L, 2);
    parser->seekTo(pos);
    return 0;
}
static int hpReadNext(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    parser->readNext();
    return 0;
}
static int hpEnd(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    bool atEnd = parser->atEnd();
    lua_pushboolean(L, atEnd);
    return 1;
}
static int hpCurPos(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int pos = parser->curPos();
    lua_pushnumber(L, pos);
    return 1;
}
static int hpReadContentText(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    QByteArray content(parser->readContentText());
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}
static int hpReadContentUntil(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!=3 || lua_type(L, 2)!=LUA_TSTRING || lua_type(L, 3)!=LUA_TBOOLEAN)
    {
        lua_pushnil(L);
        return 1;
    }
    QByteArray nodeName(lua_tostring(L, 2));
    bool isStart = lua_toboolean(L, 3);
    QByteArray content(parser->readContentUntil(nodeName, isStart));
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}
static int hpIsStartNode(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    lua_pushboolean(L, parser->isStartNode());
    return 1;
}
static int hpCurrentNode(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    lua_pushstring(L, parser->currentNode().constData());
    return 1;
}
static int hpCurrentNodeProperty(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!= 2 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    QByteArray nodeName(lua_tostring(L, 2));
    lua_pushstring(L, parser->currentNodeProperty(nodeName).constData());
    return 1;
}
static int htmlParserGC (lua_State *L) {
    HTMLParserSax *parser = checkHTMLParser(L);
    if(parser) delete parser;
    return 0;
}
//HTMLParserSax End--------------

static const luaL_Reg kikoFuncs[] = {
    {"httpget", httpGet},
    {"httpgetbatch", httpGetBatch},
    {"httppost", httpPost},
    {"json2table", json2table},
    {"table2json", table2json},
    {"compress", compress},
    {"decompress", decompress},
    {"writesetting", writeSetting},
    {"regex", regex},
    {"xmlreader", xmlreader},
    {"htmlparser", htmlparser},
    {"execute", execute},
    {"hashdata", hashData},
    {"log", logger},
    {"message", message},
    {"dialog", dialog},
    {"sttrans", simplifiedTraditionalTrans},
    {"envinfo", envInfo},
    {nullptr, nullptr}
};
static const luaL_Reg regexFuncs[] = {
    {"find", regexFind},
    {"gmatch", regexMatch},
    {"gsub", regexSub},
    {"setpattern", regexSetPattern},
    {"__gc", regexGC},
    {nullptr, nullptr}
};
static const luaL_Reg gmatchFuncs[] = {
    {"__gc", regexMatchGC},
    {nullptr, nullptr}
};
static const luaL_Reg xmlreaderFuncs[] = {
    {"adddata", xrAddData},
    {"clear", xrClear},
    {"atend", xrEnd},
    {"readnext", xrReadNext},
    {"startelem", xrStartElem},
    {"endelem", xrEndElem},
    {"name", xrName},
    {"attr", xrAttr},
    {"hasattr", xrHasAttr},
    {"elemtext", xrReadElemText},
    {"error", xrError},
    {"__gc", xmlreaderGC},
    {nullptr, nullptr}
};
static const luaL_Reg htmlparserFuncs [] = {
    {"adddata", hpAddData},
    {"seekto", hpSeekTo},
    {"atend", hpEnd},
    {"readnext", hpReadNext},
    {"curpos", hpCurPos},
    {"readcontent", hpReadContentText},
    {"readuntil", hpReadContentUntil},
    {"start", hpIsStartNode},
    {"curnode", hpCurrentNode},
    {"curproperty", hpCurrentNodeProperty},
    {"__gc", htmlParserGC},
    {nullptr, nullptr}
};
}
ScriptBase::ScriptBase() : L(nullptr), settingsUpdated(false),hasSetOptionFunc(false),sType(ScriptType::UNKNOWN_STYPE)
{
    L = luaL_newstate();
    if(L)
    {
        luaL_openlibs(L);
        registerFuncs("kiko", kikoFuncs);

        luaL_newmetatable(L, "meta.kiko.regex");
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2); // pushes the metatable
        lua_rawset(L, -3); // metatable.__index = metatable
        luaL_setfuncs(L, regexFuncs, 0);
        lua_pop(L, 1);

        luaL_newmetatable(L, "meta.kiko.regex.matchiter");
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2); // pushes the metatable
        lua_rawset(L, -3); // metatable.__index = metatable
        luaL_setfuncs(L, gmatchFuncs, 0);
        lua_pop(L, 1);

        luaL_newmetatable(L, "meta.kiko.xmlreader");
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2); // pushes the metatable
        lua_rawset(L, -3); // metatable.__index = metatable
        luaL_setfuncs(L, xmlreaderFuncs, 0);
        lua_pop(L, 1);

        luaL_newmetatable(L, "meta.kiko.htmlparser");
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2); // pushes the metatable
        lua_rawset(L, -3); // metatable.__index = metatable
        luaL_setfuncs(L, htmlparserFuncs, 0);
        lua_pop(L, 1);

        lua_pushstring(L, "kiko_scriptobj");
        lua_pushlightuserdata(L, (void *)this);
        lua_settable(L, LUA_REGISTRYINDEX);
    }
}

ScriptBase::~ScriptBase()
{
    if(L)
    {
        QMutexLocker locker(&scriptLock);
        lua_close(L);
        L = nullptr;
    }
    if(settingsUpdated)
    {
        QString scriptPath(scriptMeta["path"]);
        int suffixPos = scriptPath.lastIndexOf('.');
        QFile settingSaved(scriptPath.mid(0,suffixPos)+".json");
        if(settingSaved.open(QFile::WriteOnly))
        {
            QMap<QString, QVariant> settingMap;
            for(const auto &item : scriptSettings)
                settingMap[item.key] = item.value;
            QJsonDocument doc(QJsonObject::fromVariantMap(settingMap));
            settingSaved.write(doc.toJson(QJsonDocument::Indented));
        }
        settingsUpdated = false;
    }
}

ScriptState ScriptBase::setOption(int index, const QString &value, bool callLua)
{
    if(scriptSettings.size()<=index) return "OutRange";
    scriptSettings[index].value = value;
    settingsUpdated = true;
    QString errInfo;
    if(callLua)
    {
        setTable(luaSettingsTable, scriptSettings[index].key, value);
        if(hasSetOptionFunc)
            call(luaSetOptionFunc, {scriptSettings[index].key, value}, 0, errInfo);
    }
    return errInfo;
}

ScriptState ScriptBase::setOption(const QString &key, const QString &value, bool callLua)
{
    QString errInfo;
    for(auto &item : scriptSettings)
    {
        if(item.key == key)
        {
            item.value = value;
            settingsUpdated = true;
            if(callLua)
            {
                setTable(luaSettingsTable, key, value);
                if(hasSetOptionFunc)
                    call(luaSetOptionFunc, {key, value}, 0, errInfo);
            }
            break;
        }
    }
    return errInfo;
}

ScriptState ScriptBase::loadScript(const QString &path)
{
    if(!L) return "Script Error: Wrong Lua State";
    QFile luaFile(path);
    luaFile.open(QFile::ReadOnly);
    if(!luaFile.isOpen()) return "Open Script File Failed";
    QString luaScript(luaFile.readAll());
    QString errInfo;
    if(luaL_loadstring(L,luaScript.toStdString().c_str()) || lua_pcall(L,0,0,0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return errInfo;
    }
    errInfo = loadMeta(path);
    if(!errInfo.isEmpty()) return errInfo;
    loadSettings(path);
    return errInfo;
}

QVariantList ScriptBase::call(const char *fname, const QVariantList &params, int nRet, QString &errInfo)
{
    if(!L)
    {
        errInfo = "Wrong Lua State";
        LOG_ERROR(errInfo, "Lua");
        return QVariantList();
    }
    if(lua_getglobal(L, fname) != LUA_TFUNCTION)
    {
        errInfo = QString("%1 is not founded").arg(fname);
        LOG_ERROR(errInfo, id());
        return QVariantList();
    }
    for(auto &p : params)
    {
        pushValue(L, p);
    }
    if(lua_pcall(L, params.size(), nRet, 0))
    {
        errInfo=QString(lua_tostring(L, -1));
        lua_pop(L,1);
        LOG_ERROR(errInfo, id());
        return QVariantList();
    }
    QVariantList rets;
    for(int i=0; i<nRet; ++i)
    {
        rets.append(getValue(L));
        lua_pop(L, 1);
    }
    std::reverse(rets.begin(), rets.end());
    return rets;
}

QVariant ScriptBase::get(const char *name)
{
    if(!L) return QVariant();
    lua_getglobal(L, name);
    QVariant val = getValue(L);
    lua_pop(L, 1);
    return val;
}

void ScriptBase::set(const char *name, const QVariant &val)
{
    if(!L) return;
    pushValue(L, val);
    lua_setglobal(L, name);
}

ScriptState ScriptBase::setTable(const char *tname, const QVariant &key, const QVariant &val)
{
    if(!L) return "Script Error: Wrong Lua State";
    int type = lua_getglobal(L, tname);
    if(type == LUA_TTABLE)
    {
        pushValue(L, key);
        pushValue(L, val);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return "";
    }
    lua_pop(L, 1);
    return QString("No table with name %1").arg(tname);
}

bool ScriptBase::checkType(const char *name, int type)
{
    if(!L) return false;
    int ct = lua_getglobal(L, name);
    lua_pop(L, 1);
    return ct == type;
}

void ScriptBase::pushValue(lua_State *L, const QVariant &val)
{
    if(!L) return;
    switch (val.type())
    {
    case QVariant::Int:
	case QVariant::LongLong:
	case QVariant::UInt:
	case QVariant::ULongLong:
    case QVariant::Double:
        lua_pushnumber(L, val.toDouble());
        break;
    case QVariant::String:
        lua_pushstring(L, val.toString().toStdString().c_str());
        break;
    case QVariant::Bool:
        lua_pushboolean(L, val.toBool());
        break;
    case QVariant::Invalid:
        lua_pushnil(L);
        break;
    case QVariant::List:
    {
        lua_newtable(L); // table
        const auto &l = val.toList();
        for(int i=0; i<l.size(); ++i)
        {
            pushValue(L, l.value(i));
            lua_rawseti(L, -2, i+1);
        }
        break;
    }
    case QVariant::Map:
    {
        lua_newtable(L); // table
        const auto &m = val.toMap();
        for(auto i = m.constBegin(); i!=m.constEnd(); ++i)
        {
            lua_pushstring(L, i.key().toStdString().c_str()); // table key
            pushValue(L, i.value()); // table key value
            lua_rawset(L, -3);
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

QVariant ScriptBase::getValue(lua_State *L, bool useString)
{
    if(!L) return QVariant();
    if(lua_gettop(L)==0) return QVariant();
    switch (lua_type(L, -1))
    {
    case LUA_TNUMBER:
    {
        double d = lua_tonumber(L, -1);
        return d;
    }
    case LUA_TBOOLEAN:
    {
        return bool(lua_toboolean(L, -1));
    }
    case LUA_TSTRING:
    {
        size_t len = 0;
        const char *s = (const char *)lua_tolstring(L, -1, &len);
		if (useString) return QString(s);
		else return QByteArray(s, len);
    }
    case LUA_TTABLE:
    {
        // If all keys are integers, and they're in sequence, take it as an list.
        int count = 0;
        for (int i = 1; ; ++i)
        {
            lua_pushinteger(L, i); // n
            lua_gettable(L, -2); // t[n]
            bool empty = lua_isnil(L, -1); // t[n]
            lua_pop(L, 1); // -
            if (empty)
            {
                count = i - 1;
                break;
            }
        }
        size_t length = getTableLength(L, -1);
        if(count < 0 || count != length) // map
        {
            QVariantMap map;
            lua_pushnil(L); // t nil
            while (lua_next(L, -2)) { // t key value
                if (lua_type(L, -2) != LUA_TSTRING)
                {
                    luaL_error(L, "key must be a string, but got %s",
                               lua_typename(L, lua_type(L, -2)));
                }
                QString key(lua_tostring(L, -2));
                map[key] = getValue(L, useString);
                lua_pop(L, 1); // key
            }
            return map;
        }
        else  // array
        {
            QVariantList list;
            for (int i = 0; ;++i) {
                lua_pushinteger(L, i + 1); //t n1
                lua_gettable(L, -2); //t t[n1]
				if (lua_isnil(L, -1))
				{
					lua_pop(L, 1);
					break;
				}
                list.append(getValue(L, useString));
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

int ScriptBase::getTableLength(lua_State *L, int pos)
{
    if(!L) return 0;
    if (pos < 0)  pos = lua_gettop(L) + (pos + 1);
    lua_pushnil(L); // nil
    int length = 0;
    while (lua_next(L, pos))  // key value
    {
        ++length;
        lua_pop(L, 1); // key
    }
    return length;
}

QString ScriptBase::loadMeta(const QString &scriptPath)
{
    QString errInfo;
    QVariant scriptInfo = get(luaMetaTable);
    if(!scriptInfo.canConvert(QVariant::Map))
    {
        errInfo = "Script Error: no info table";
        LOG_ERROR(errInfo, "KikoPlay");
        return errInfo;
    }
    QVariantMap scriptInfoMap = scriptInfo.toMap();
    scriptMeta.clear();
    for(auto iter = scriptInfoMap.constBegin(); iter != scriptInfoMap.constEnd(); ++iter)
    {
        if(iter.value().canConvert(QVariant::String))
        {
            scriptMeta[iter.key()] = iter.value().toString();
        }
    }
    QFileInfo scriptFileInfo(scriptPath);
    scriptMeta["path"] = scriptFileInfo.absoluteFilePath();
    scriptMeta["time"] = QString::number(scriptFileInfo.fileTime(QFile::FileModificationTime).toSecsSinceEpoch());
    if(!scriptMeta.contains("id")) scriptMeta["id"] = scriptMeta["path"];
    if(!scriptMeta.contains("name")) scriptMeta["name"] = scriptFileInfo.baseName();
    return errInfo;
}

void ScriptBase::loadSettings(const QString &scriptPath)
{
    //settings = {key = {title = 'xx', default='', desc='', choices=',..,'},...}
    QString errInfo;
    QVariant settings = get(luaSettingsTable);
    hasSetOptionFunc = checkType(luaSetOptionFunc, LUA_TFUNCTION);
    if(!settings.canConvert(QVariant::Map)) return;
    QVariantMap settingMap = settings.toMap();
    scriptSettings.clear();
    for(auto iter = settingMap.constBegin(); iter != settingMap.constEnd(); ++iter)
    {
        if(iter.value().canConvert(QVariant::Map))
        {
            QVariantMap settingItemMap = iter.value().toMap();
            scriptSettings.append(
            {
                settingItemMap.value("title").toString(),
                settingItemMap.value("desc", "").toString(),
                settingItemMap.value("choices", "").toString(),
                iter.key(),
                settingItemMap.value("default", "").toString()
            });
        }
    }

    int suffixPos = scriptPath.lastIndexOf('.');
    QFile settingSaved(scriptPath.mid(0,suffixPos)+".json");
    if(settingSaved.open(QFile::ReadOnly))
    {
        QJsonObject sObj(Network::toJson(settingSaved.readAll()).object());
        QHash<QString, ScriptSettingItem *> itemHash;
        for(auto &item: scriptSettings)
        {
            itemHash[item.key] = &item;
        }
        for(auto iter = sObj.constBegin(); iter != sObj.constEnd(); ++iter)
        {
            ScriptSettingItem *item = itemHash.value(iter.key(), nullptr);
            if(item)
            {
                item->value = iter.value().toString("");
            }
        }
    }

    lua_newtable(L);
    for(const auto &item: scriptSettings)
    {
        lua_pushstring(L, item.key.toStdString().c_str());
        lua_pushstring(L, item.value.toStdString().c_str());
        lua_settable(L, -3);
    }
    lua_setglobal(L, luaSettingsTable);
}

void ScriptBase::registerFuncs(const char *tname, const luaL_Reg *funcs)
{
    if(!L) return;
    lua_getglobal(L, tname);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_setglobal(L, tname);
}
