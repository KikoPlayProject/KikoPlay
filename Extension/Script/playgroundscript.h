#ifndef PLAYGROUNDSCRIPT_H
#define PLAYGROUNDSCRIPT_H

#include "scriptbase.h"

class PlaygroundScript : public ScriptBase
{
public:
    PlaygroundScript();
    void run(const QString &scriptContent);

    using PrintCallback = std::function<void(const QString &)>;
    void setPrintCallback(PrintCallback cb);
private:
    PrintCallback printCallBack;
    static int print(lua_State *L);
};

#endif // PLAYGROUNDSCRIPT_H
