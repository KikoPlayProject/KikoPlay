#include "regexmatcher.h"
#define __I CaseInsensitiveOption
#define __S DotMatchesEverythingOption
#define __M MultilineOption
#define __X ExtendedPatternSyntaxOption
#define P_FLAG(f) QRegularExpression::__ ## f

QRegularExpression::PatternOptions RegExMatcher::parsePatternOptions(const char *options)
{
    auto flags = QRegularExpression::PatternOptions();
    if(options!=nullptr)
        for(int i=0;options[i]!='\0';++i)
            switch(options[i])
            {
            case 'i': flags |= P_FLAG(I); break;
            case 's': flags |= P_FLAG(S); break;
            case 'm': flags |= P_FLAG(M); break;
            case 'x': flags |= P_FLAG(X); break;
            } // ignore unsupported options
    return flags;
}

RegExMatcher::RegExMatcher(const char *pattern, const char *options)
    :re(QString(pattern),parsePatternOptions(options)),itmap(){}

QString RegExMatcher::getPatternOptions() const
{
    QString options("");
    auto flags = re.patternOptions();
    if(flags.testFlag(P_FLAG(I))) options.append('i');
    if(flags.testFlag(P_FLAG(S))) options.append('s');
    if(flags.testFlag(P_FLAG(M))) options.append('m');
    if(flags.testFlag(P_FLAG(X))) options.append('x');
    return options;
};

QString RegExMatcher::addNewMatch(const QString &s)
{
    if(!re.isValid()) throw std::logic_error(errorMessage().toStdString());
    const auto id = randomId();
    itmap[id] = re.globalMatch(s);
    return id;
}

QString RegExMatcher::randomId() const
{
    QString randomString;
    for(int i=0;i<randomStringLength;++i)
    {
        int idx = qrand() % strlen(possibleCharacters);
        randomString.append(possibleCharacters[idx]);
    }
    return randomString;
}
