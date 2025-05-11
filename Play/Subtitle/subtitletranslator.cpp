#include "subtitletranslator.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/logger.h"
#include <QSettings>

#define SETTING_KEY_SUB_TRANSLATOR_CONFIGS   "SubRecogize/TranslatorConfigs"


SubtitleTranslator::SubtitleTranslator(const TranslatorConfig &config, const SubFile &subFile) : KTask{"sub_translator"},
    _config(config)
{
    _subItems.reserve(subFile.items.size());
    for (const SubItem &item : subFile.items)
    {
        _subItems.append(item.text);
    }
}

SubtitleTranslator::SubtitleTranslator(const TranslatorConfig &config, const QStringList &items) : KTask{"sub_translator"},
    _config(config), _subItems(items)
{

}

Network::Reply SubtitleTranslator::post(const QString& url, const QByteArray& data, const QStringList& header)
{
    QUrl queryUrl(url);
    QNetworkRequest request;
    if (header.size() >= 2)
    {
        for (int i = 0; i < header.size(); i += 2)
        {
            request.setRawHeader(header[i].toUtf8(), header[i + 1].toUtf8());
        }
    }
    request.setUrl(queryUrl);

    QNetworkAccessManager* manager = Network::getManager();
    manager->setCookieJar(nullptr);

    QTimer timer;
    timer.setInterval(Network::timeout);
    timer.setSingleShot(true);
    QNetworkReply* reply = manager->post(request, data);

    QEventLoop eventLoop;
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(this, &SubtitleTranslator::taskCancel, &eventLoop, [&]() {
        reply->abort();
        eventLoop.quit();
    }, Qt::QueuedConnection);

    timer.start();
    eventLoop.exec();

    Network::Reply replyObj;

    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            replyObj.hasError = false;
            replyObj.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            replyObj.content = reply->readAll();
            replyObj.headers = reply->rawHeaderPairs();
        }
        else
        {
            replyObj.hasError = true;
            replyObj.errInfo = reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        replyObj.hasError = true;
        replyObj.errInfo = QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    return replyObj;
}


TaskStatus SubtitleTranslator::runTask()
{
    const QStringList headers {
        "Content-Type", "application/json",
        "Authorization",  "Bearer " + _config.apiKey
    };
    const QVariantMap initMsg {
        { "role", "system" },
        { "content", _config.prompt },
    };
    QVariantList msgList = { initMsg };
    setInfo(tr("Start translation..."), NM_HIDE);
    for (int i = 0; i < _subItems.size();)
    {
        if (_cancelFlag) break;
        QVariantMap curMsg {
            { "role", "user" },
            { "content", _subItems.mid(i, _config.batchSize).join('\n') },
        };
        msgList.append(curMsg);

        QVariantList curMsgList { _config.postHistorySub ? msgList : QVariantList{ initMsg, curMsg} };
        const QVariantMap payLoad = {
            { "model", _config.model },
            { "messages", curMsgList },
        };
        const QByteArray payloadData = QJsonDocument::fromVariant(payLoad).toJson(QJsonDocument::Compact);
        auto reply = post(_config.url, payloadData, headers);
        if (reply.hasError)
        {
            if (_cancelFlag) break;
            setInfo(tr("An error occurred, retry after sleeping for 5 seconds..."), NM_ERROR | NM_HIDE);
            Logger::logger()->log(Logger::APP, QString("An error occurred: %1").arg(reply.errInfo));
            QThread::sleep(5);
            reply = post(_config.url, payloadData, headers);
        }
        if (reply.hasError)
        {
            if (_cancelFlag) break;
            setInfo(tr("Some subtitles failed to be translated: %1~%2").arg(i).arg(i + _config.batchSize), NM_ERROR | NM_HIDE);
            Logger::logger()->log(Logger::APP, QString("Some subtitles failed to be translated: %1").arg(reply.errInfo));
            i += _config.batchSize;
            continue;
        }
        Logger::logger()->log(Logger::APP, QString("task %1+%2 reply: ").arg(i).arg(_config.batchSize).arg(reply.content));
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(reply.content, &jsonError);
        if (jsonError.error == QJsonParseError::NoError && doc.isObject())
        {
            QJsonObject replyObj = doc.object();

            QJsonValue msgValue = Network::getValue(replyObj, "choices/0/message");
            if (!msgValue.isUndefined() && msgValue.isObject())
            {
                QJsonObject msgObj = msgValue.toObject();
                if (msgObj.contains("content") && msgObj["content"].isString())
                {
                    QStringList translatedItems = msgObj["content"].toString().split('\n');
                    emit subTranslated(translatedItems, i);
                    msgList.append(msgObj);
                    emit progreeUpdated(qMin((i + _config.batchSize) / static_cast<double>(_subItems.size()) * 100, 100.0));
                }
            }
        }
        i += _config.batchSize;
        QThread::sleep(2);
    }
    return _cancelFlag ? TaskStatus::Cancelled : TaskStatus::Finished;
}

TranslatorConfigManager *TranslatorConfigManager::instance()
{
    static TranslatorConfigManager manager;
    return &manager;
}

TranslatorConfigManager::TranslatorConfigManager(QObject *parent)
{
    _configs = GlobalObjects::appSetting->value(SETTING_KEY_SUB_TRANSLATOR_CONFIGS).value<QList<TranslatorConfig>>();
    if (_configs.empty())
    {
        QFile defaultTranslatorConfigFile(":/res/translators.json");
        defaultTranslatorConfigFile.open(QFile::ReadOnly);
        if(defaultTranslatorConfigFile.isOpen())
        {
            QJsonArray translators(Network::toJson(defaultTranslatorConfigFile.readAll()).array());
            for (const QJsonValue &config : translators)
            {
                _configs.emplaceBack("");
                _configs.back().loadJson(config.toObject());
            }
        }
    }
}

void TranslatorConfigManager::addConfig(const QString &name)
{
    beginInsertRows(QModelIndex(), _configs.size(), _configs.size());
    _configs.emplaceBack(name);
    updateSettings();
    endInsertRows();
}

void TranslatorConfigManager::removeConfig(int index)
{
    if (index >= 0 && index < _configs.size())
    {
        beginRemoveRows(QModelIndex(), index, index);
        _configs.removeAt(index);
        updateSettings();
        endRemoveRows();
    }
}

void TranslatorConfigManager::updateSettings()
{
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_TRANSLATOR_CONFIGS, QVariant::fromValue(_configs));
}

QVariant TranslatorConfigManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const TranslatorConfig &item = _configs[index.row()];
    Columns col = static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::EditRole:
    {
        switch (col)
        {
        case Columns::NAME:
            return item.name;
        case Columns::URL:
            return item.url;
        case Columns::API_KEY:
            return item.apiKey;
        case Columns::MODEL:
            return item.model;
        case Columns::PROMPT:
            return item.prompt;
        case Columns::BATCH_SIZE:
            return item.batchSize;
        case Columns::POST_HISTORY:
            return item.postHistorySub;
        case Columns::TIP:
            return item.tip;
        }
        break;
    }
    default:
        return QVariant();
    }
    return QVariant();
}

bool TranslatorConfigManager::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Columns col = static_cast<Columns>(index.column());
    TranslatorConfig &item = _configs[index.row()];
    switch (col)
    {
    case Columns::URL:
        item.url = value.toString();
        break;
    case Columns::API_KEY:
        item.apiKey = value.toString();
        break;
    case Columns::MODEL:
        item.model = value.toString();
        break;
    case Columns::PROMPT:
        item.prompt = value.toString();
        break;
    case Columns::BATCH_SIZE:
        item.batchSize = value.toInt();
        break;
    case Columns::POST_HISTORY:
        item.postHistorySub = value.toBool();
        break;
    default:
        return false;
    }
    updateSettings();
    return true;
}

QDataStream &operator<<(QDataStream &out, const QList<TranslatorConfig> &l)
{
    int s = l.size();
    out << s;
    for (const auto &item : l)
    {
        out << item.name << item.url << item.apiKey << item.model << item.prompt << item.batchSize << item.postHistorySub << item.tip;
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, QList<TranslatorConfig> &l)
{
    int lSize = 0;
    in >> lSize;
    for (int i = 0; i < lSize; ++i)
    {
        l.emplaceBack("");
        TranslatorConfig &config = l.back();
        in >> config.name >> config.url >> config.apiKey >> config.model >> config.prompt >> config.batchSize >> config.postHistorySub >> config.tip;
    }
    return in;
}

void TranslatorConfig::loadJson(const QJsonObject &obj)
{
    name = obj.value("name").toString();
    url = obj.value("url").toString();
    apiKey = obj.value("apiKey").toString();
    model = obj.value("model").toString();
    prompt = obj.value("prompt").toString();
    tip = obj.value("tip").toString();
}
