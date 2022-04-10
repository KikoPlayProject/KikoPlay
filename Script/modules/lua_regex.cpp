#include "lua_regex.h"
#include "../scriptbase.h"

namespace LuaModule
{

void Regex::setup()
{
    const luaL_Reg regexFuncs[] = {
        {"regex", regex},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", regexFuncs);

    const luaL_Reg regexMemberFuncs[] = {
        {"find", regexFind},
        {"gmatch", regexMatch},
        {"gsub", regexSub},
        {"setpattern", regexSetPattern},
        {"__gc", regexGC},
        {nullptr, nullptr}
    };
    const luaL_Reg gmatchMemberFuncs[] = {
        {"__gc", regexMatchGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs("meta.kiko.regex", regexMemberFuncs);
    registerMemberFuncs("meta.kiko.regex.matchiter", gmatchMemberFuncs);
}

QRegularExpression::PatternOptions Regex::getPatternOptions(const char *options)
{
    auto flags = QRegularExpression::PatternOptions(QRegularExpression::NoPatternOption);
    if(!options) return flags;
    while(*options)
    {
        switch(*options++)
        {
        case 'i': flags |= QRegularExpression::CaseInsensitiveOption; break;
        case 's': flags |= QRegularExpression::DotMatchesEverythingOption; break;
        case 'm': flags |= QRegularExpression::MultilineOption; break;
        case 'x': flags |= QRegularExpression::ExtendedPatternSyntaxOption; break;
        }
    }
    return flags;
}

int Regex::regex(lua_State *L)
{
    int n = lua_gettop(L);
    const char *pattern = nullptr, *options = nullptr;
    if(n > 0 && lua_type(L, 1) == LUA_TSTRING)
    {
        pattern = lua_tostring(L, 1);
    }
    if(n > 1)
    {
        luaL_argcheck(L, n == 2, 3, "too many arguments, 1-2 strings expected");
        luaL_argcheck(L, lua_type(L, 2)==LUA_TSTRING, 2, "expect string as pattern option");
        options = lua_tostring(L, 2);
    }
    QRegularExpression **regex = (QRegularExpression **)lua_newuserdata(L, sizeof(QRegularExpression *));
    luaL_getmetatable(L, "meta.kiko.regex");
    lua_setmetatable(L, -2);  // regex meta
    *regex = new QRegularExpression(pattern, getPatternOptions(options)); // regex
    return 1;
}

QRegularExpression *Regex::checkRegex(lua_State *L, bool validate)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.regex");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.regex' expected");
    auto regex = *(QRegularExpression **)ud;
    if(validate && !regex->isValid())
    {
        QString errInfo("invalid regex at %1: %2, fix via method setpattern(<pattern>)");
        errInfo = errInfo.arg(QString::number(regex->patternErrorOffset()), regex->errorString());
        luaL_error(L, errInfo.toStdString().c_str());
    }
    return regex;
}

int Regex::regexSetPattern(lua_State *L)
{
    QRegularExpression *regex = checkRegex(L);
    int params = lua_gettop(L); // regex(kiko.regex) pattern(string) options(string)
    if(params < 4 && lua_type(L, 2) == LUA_TSTRING)
    {
        regex->setPattern(lua_tostring(L, 2));
        if(params == 3)
        {
            luaL_argcheck(L, lua_type(L, 3)==LUA_TSTRING, 3, "expect string as pattern option");
            regex->setPatternOptions(getPatternOptions(lua_tostring(L, 3)));
        }
    }
    else
    {
        luaL_argerror(L, 4, "too many arguments or wrong type (expect string), setting pattern to none");
    }
    return 0;
}

int Regex::regexGC(lua_State *L) {
    QRegularExpression *regex = checkRegex(L);
    if(regex) delete regex;
    return 0;
}

int Regex::regexFind(lua_State *L)
{
    QRegularExpression *regex = checkRegex(L, true);
    int params = lua_gettop(L); // regex(kiko.regex) target(string) initpos(number)
    const char *errMsg = nullptr;
    int initpos = 0;
    if(params < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        errMsg = "expect string as match target";
    }
    else if(params == 3 && lua_type(L, 3) == LUA_TNUMBER)
    {
        initpos = lua_tonumber(L, 3);
        if(initpos > 0) initpos--; // 1-based
    }
    else if(params != 2)
    {
        errMsg = "expect number as initial matching position";
    }
    if(errMsg)
    {
        luaL_error(L, "expect: regex(kiko.regex) target(string) initpos(number)");
    }
    QRegularExpressionMatch match = regex->match(QString(lua_tostring(L, 2)), initpos);
    if(match.hasMatch())
    {
        lua_pushinteger(L, match.capturedStart() + 1); // 1-based
        lua_pushinteger(L, match.capturedEnd()); // 0-based, after the ending position of the substring captured
        QStringList groups = match.capturedTexts();
        int retSize = 2;
        if(groups.size() > 1)
        {
            for(int i = 1; i < groups.size(); ++i)
            {
                lua_pushstring(L, groups.at(i).toStdString().c_str());
                ++retSize;
            }
        }
        return retSize;
    }
    return 0;
}

QRegularExpressionMatchIterator *Regex::checkRegexMatch(lua_State *L, int idx)
{
    void *ud = luaL_checkudata(L, idx, "meta.kiko.regex.matchiter");
    luaL_argcheck(L, ud != NULL, idx, "`kiko.regex.matchiter' expected");
    return *(QRegularExpressionMatchIterator **)ud;
}

int Regex::regexMatchGC(lua_State *L) {
    auto *it = checkRegexMatch(L, 1);
    if(it) delete it;
    return 0;
}

int Regex::regexMatchIterator(lua_State *L)
{
    auto *it = checkRegexMatch(L, lua_upvalueindex(1));
    if(it->hasNext())
    {
        const auto next = it->next();
        const auto groups = next.capturedTexts();
        for(const QString &matched : groups)
        {
            lua_pushstring(L, matched.toStdString().c_str());
        }
        return groups.size();
    }
    return 0;
}

int Regex::regexMatch(lua_State *L)
{
    QRegularExpression *regex = checkRegex(L);
    if(lua_gettop(L) != 2 || lua_type(L, 2) != LUA_TSTRING) // regex(kiko.regex) target(string)
    {
        luaL_error(L, "expect string as match target");
        return 0;
    }
    auto **it = (QRegularExpressionMatchIterator **)lua_newuserdata(L, sizeof(QRegularExpressionMatchIterator *));
    luaL_getmetatable(L, "meta.kiko.regex.matchiter");
    lua_setmetatable(L, -2);  // matchiter meta
    *it = new QRegularExpressionMatchIterator(regex->globalMatch(lua_tostring(L, 2))); // matchiter
    lua_pushcclosure(L, &regexMatchIterator, 1);
    return 1;
}

int Regex::regexSub(lua_State *L)
{
    QRegularExpression *regex = checkRegex(L, true);
    int params = lua_gettop(L); // regex(kiko.regex) target(string) repl(string/table/function)
    const int replParamType = (params==3 && lua_type(L, 2)==LUA_TSTRING) ? lua_type(L, 3) : LUA_TNONE;
    if(replParamType!=LUA_TSTRING && replParamType!=LUA_TTABLE && replParamType!=LUA_TFUNCTION)
    {
        luaL_error(L, "expect: target(string), repl(string/table/function)");
    }
    QString target(lua_tostring(L, 2));
    if (replParamType == LUA_TSTRING)
    {
        lua_pushstring(L, target.replace(*regex, QString(lua_tostring(L, 3))).toStdString().c_str());
        return 1;
    }
    QRegularExpressionMatchIterator it = regex->globalMatch(lua_tostring(L, 2));
    QString replResult;
    if(replParamType == LUA_TTABLE)
    {
        int lastCapturedEnd = 0;
        const QVariantMap replTable = ScriptBase::getValue(L).toMap();
        while(it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            replResult.append(target.mid(lastCapturedEnd, match.capturedStart() - lastCapturedEnd));
            lastCapturedEnd = match.capturedEnd();

            const QString captureText = match.captured();
            const int captureGroupSize = match.capturedTexts().size();
            const int blockStart = match.capturedStart();
            int captureEnd = 0, cursor = match.capturedStart();
            for(int i = 1; i < captureGroupSize; ++i)
            {
                if(cursor >= match.capturedEnd(i)) continue; // skip groups starting before cursor position
                const int start = match.capturedStart(i);
                replResult.append(captureText.mid(captureEnd, start - blockStart - captureEnd));
                captureEnd = start - blockStart;
                if(replTable.contains(match.captured(i)) && !replTable[match.captured(i)].toString().isNull())
                {
                    replResult.append(replTable[match.captured(i)].toString());
                    cursor = match.capturedEnd(i);
                    captureEnd = cursor - blockStart;
                }
            }
            replResult.append(captureText.mid(captureEnd));
        }
        replResult.append(target.mid(lastCapturedEnd));
    }
    else // LUA_TFUNCTION
    {
        int lastCapturedEnd = 0;
        for(int matchCount = 1; it.hasNext(); ++matchCount)
        {
            QRegularExpressionMatch match = it.next();
            replResult.append(target.mid(lastCapturedEnd, match.capturedStart() - lastCapturedEnd));
            lastCapturedEnd = match.capturedEnd();
            lua_pushvalue(L, 3);  // duplicate replacement function
            const auto groups = match.capturedTexts(); // cf. gmatch
            for(const QString &matched : groups)
            {
                lua_pushstring(L, matched.toStdString().c_str());
            }
            if(lua_pcall(L, groups.size(), 1, 0) == 0) // replace
            {
                const char *retStr = lua_tostring(L, -1);
                replResult.append(retStr ? QString(retStr) : match.captured());
            }
            else
            {
                luaL_error(L, "replacement function failed on match %d: %s", matchCount, lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
        replResult.append(target.mid(lastCapturedEnd));
    }
    lua_pushstring(L, replResult.toStdString().c_str());
    return 1;
}

}
