#ifndef SUBTITLETRANSLATOR_H
#define SUBTITLETRANSLATOR_H

#include "Common/taskpool.h"
#include "Common/network.h"
#include "subitem.h"

struct TranslatorConfig
{
    TranslatorConfig(const QString &_name) : name(_name) {}

    QString name;
    QString url;
    QString apiKey;
    QString model;
    QString prompt;
    int batchSize{20};
    bool postHistorySub{false};
    QString tip;

    bool isValid() const { return !url.isEmpty() && !model.isEmpty() && !apiKey.isEmpty() && !prompt.isEmpty() && batchSize > 0; }
    void loadJson(const QJsonObject &obj);
};
Q_DECLARE_METATYPE(TranslatorConfig)
Q_DECLARE_METATYPE(QList<TranslatorConfig>)
QDataStream &operator<<(QDataStream &out, const QList<TranslatorConfig> &l);
QDataStream &operator>>(QDataStream &in, QList<TranslatorConfig> &l);

class TranslatorConfigManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    static TranslatorConfigManager *instance();

    explicit TranslatorConfigManager(QObject *parent = nullptr);

    void addConfig(const QString &name);
    void removeConfig(int index);
    TranslatorConfig config(int index) const { return index >= 0 && index < _configs.size() ? _configs[index] : TranslatorConfig(""); }

private:
    QList<TranslatorConfig> _configs;
    void updateSettings();

    // QAbstractItemModel interface
public:
    enum struct Columns
    {
        NAME,
        URL,
        API_KEY,
        MODEL,
        PROMPT,
        BATCH_SIZE,
        POST_HISTORY,
        TIP,
    };

    inline virtual int columnCount(const QModelIndex &) const { return 7;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const { return parent.isValid() ? QModelIndex() : createIndex(row,column); }
    virtual QModelIndex parent(const QModelIndex &child) const { return QModelIndex(); }
    virtual int rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : _configs.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
};

class SubtitleTranslator : public KTask
{
    Q_OBJECT
public:
    SubtitleTranslator(const TranslatorConfig &config, const SubFile &subFile);
    SubtitleTranslator(const TranslatorConfig &config, const QStringList &items);

private:
    TranslatorConfig _config;
    QStringList _subItems;

    Network::Reply post(const QString &url, const QByteArray &data, const QStringList &header);

protected:
    virtual TaskStatus runTask() override;

signals:
    void subTranslated(const QStringList &translatedSubs, int startPos);
};

#endif // SUBTITLETRANSLATOR_H
