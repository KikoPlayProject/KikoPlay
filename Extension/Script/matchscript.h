#ifndef MATCHSCRIPT_H
#define MATCHSCRIPT_H

#include "scriptbase.h"
#include "MediaLibrary/animeinfo.h"

class MatchScript : public ScriptBase
{
public:
    MatchScript();
    ScriptState loadScript(const QString &scriptPath);
public:
    ScriptState match(const QString &path, MatchResult &result);
private:
    const char *matchFunc = "match";
};

#endif // MATCHSCRIPT_H
