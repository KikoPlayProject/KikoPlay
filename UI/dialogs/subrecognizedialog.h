#ifndef SUBRECOGNIZEDIALOG_H
#define SUBRECOGNIZEDIALOG_H
#include "UI/framelessdialog.h"

class SubtitleRecognizer;
class SubtitleTranslator;
struct SubFile;

class SubRecognizeDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    SubRecognizeDialog(const QString &videoFile, QWidget *parent = nullptr);

private:
    SubtitleRecognizer *task{nullptr};
};

class SubRecognizeEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    SubRecognizeEditDialog(const SubFile &_subFile, const QString &videoFilePath, bool translate = false, QWidget *parent = nullptr);
    SubRecognizeEditDialog(const QString &subFilePath, const QString &videoFilePath, bool translate = false, QWidget *parent = nullptr);

private:
    void init(const SubFile &_subFile, const QString &videoFilePath, bool translate = false);
    SubtitleTranslator *task{nullptr};

};

class TranslatorConfigEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    TranslatorConfigEditDialog(QWidget *parent = nullptr);

};
#endif // SUBRECOGNIZEDIALOG_H
