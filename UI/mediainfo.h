#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include "framelessdialog.h"

class MediaInfo : public CFramelessDialog
{
    Q_OBJECT
public:
    MediaInfo(QWidget *parent = nullptr);

private:
    struct TextBlock
    {
        QString text, blockVar;
        int start=0, end=1, step=1;
        bool cond = true;
    };
    template<typename  T>
    bool cp(const T &lhs, const T &rhs, char op, bool hasEq)
    {
        switch (op)
        {
        case '>':
            return hasEq? lhs>=rhs : lhs>rhs;
        case '<':
            return hasEq? lhs<=rhs : lhs<rhs;
        case '=':
            return lhs == rhs;
        }
        return false;
    }
    QString expandMediaInfo();
    void evalCommand(QList<QString> &commandStack, QList<TextBlock> &textStack);
};

#endif // MEDIAINFO_H
