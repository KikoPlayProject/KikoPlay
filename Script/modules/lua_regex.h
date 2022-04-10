#ifndef LUA_REGEX_H
#define LUA_REGEX_H

#include "modulebase.h"
#include <QRegularExpression>

namespace  LuaModule
{
class Regex : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static QRegularExpression::PatternOptions getPatternOptions(const char *options);
    static int regex(lua_State *L);
    static QRegularExpression *checkRegex(lua_State *L, bool validate = false);
    static int regexSetPattern(lua_State *L);
    static int regexGC(lua_State *L);
    static int regexFind(lua_State *L);
    static QRegularExpressionMatchIterator *checkRegexMatch(lua_State *L, int idx);
    static int regexMatchGC(lua_State *L);
    static int regexMatchIterator(lua_State *L);
    static int regexMatch(lua_State *L);
    static int regexSub(lua_State *L);
};
}
#endif // LUA_REGEX_H
