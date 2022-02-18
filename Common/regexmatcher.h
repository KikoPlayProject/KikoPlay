#ifndef REGEXMATCHER_H
#define REGEXMATCHER_H
#include <QRegularExpression>
class RegExMatcher
{
    QRegularExpression re;
    QMap<QString, QRegularExpressionMatchIterator> itmap;
    static constexpr const char *possibleCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static constexpr const int randomStringLength = 12;
    static QRegularExpression::PatternOptions parsePatternOptions(const char *options);
    QString randomId() const;
    inline QString errorMessage() const
    {
        return "invalid regex at "+QString::number(re.patternErrorOffset())+": "+re.errorString();
    }
public:
    explicit RegExMatcher(const char *pattern, const char *options = nullptr);
    inline void setPattern(const char *pattern) {setPattern(QString(pattern));}
    inline void setPattern(const QString &pattern) {re.setPattern(pattern);}
    inline void setPatternOptions(const QString &options) {setPatternOptions(options.toStdString().c_str());}
    inline void setPatternOptions(const char *options) {re.setPatternOptions(parsePatternOptions(options));}
    inline QString getPattern() const {return re.pattern();}
    QString getPatternOptions() const;
    inline QString addNewMatch(const char *s) {return addNewMatch(QString(s));}
    QString addNewMatch(const QString &s);

    inline QRegularExpressionMatch activeMatchGotoNext(const QString &k)
    {
        if(!activeMatchHasNext(k) && containsActiveMatch(k)) removeActiveMatch(k);
        return itmap[k].next();
    }
    inline bool activeMatchHasNext(const QString &k) {return itmap.contains(k) && itmap[k].hasNext();}
    inline QRegularExpressionMatch activeMatchPeekNext(const QString &k) {return itmap[k].peekNext();}
    inline bool containsActiveMatch(const QString &k) {return itmap.contains(k);}
    inline int removeActiveMatch(const QString &k) {return itmap.remove(k);}
    inline QString replace(QString &s, const QString &after) {return s.replace(re, after);}
};

#endif // REGEXMATCHER_H
