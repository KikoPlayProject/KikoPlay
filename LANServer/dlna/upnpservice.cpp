#include "upnpservice.h"
#include <QXmlStreamWriter>
#include <QDateTime>

UPnPAction *UPnPService::getAction(const QString &name)
{
    UPnPActionDesc *desc = nullptr;
    for(QSharedPointer<UPnPActionDesc> &d : actionDescs)
    {
        if(d->name == name)
        {
            desc = d.get();
            break;
        }
    }
    if(!desc) return nullptr;
    return new UPnPAction(*this, *desc);
}

void UPnPService::addSubscriber(const UPnPEventSubscriber &subscriber)
{
    QMutexLocker locker(&subscriberLock);
    subscribers.append(QSharedPointer<UPnPEventSubscriber>::create(subscriber));
}

void UPnPService::removeSubscriber(const QString &sid)
{
    QMutexLocker locker(&subscriberLock);
    auto iter = std::remove_if(subscribers.begin(), subscribers.end(), [&sid](const QSharedPointer<UPnPEventSubscriber> &s){
        return s->sid == sid;
    });
    subscribers.erase(iter, subscribers.end());
}

void UPnPService::cleanSubscribers()
{
    QMutexLocker locker(&subscriberLock);
    qint64 now = QDateTime::currentSecsSinceEpoch();
    auto iter = std::remove_if(subscribers.begin(), subscribers.end(), [&now](const QSharedPointer<UPnPEventSubscriber> &s){
        return s->timeout < now;
    });
    subscribers.erase(iter, subscribers.end());
}

void UPnPService::dumpSCDPXml(QByteArray &content)
{
    QXmlStreamWriter writer(&content);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("scpd");
    writer.writeDefaultNamespace("urn:schemas-upnp-org:service-1-0");

    writer.writeStartElement("specVersion");
    writer.writeTextElement("", "major", "1");
    writer.writeTextElement("", "minor", "0");
    writer.writeEndElement();

    writer.writeStartElement("actionList");
    for(const QSharedPointer<UPnPActionDesc> &actDesc : actionDescs)
    {
        writer.writeStartElement("action");
        writer.writeTextElement("", "name", actDesc->name);
        writer.writeStartElement("argumentList");
        for(const QSharedPointer<UPnPArgDesc> &argDesc : actDesc->argDescs)
        {
            writer.writeStartElement("argument");
            writer.writeTextElement("", "name", argDesc->name);
            writer.writeTextElement("", "direction", argDesc->d == UPnPArgDesc::IN ? "in" : "out");
            writer.writeTextElement("", "relatedStateVariable", argDesc->relatedStateVariable.name);
            writer.writeEndElement();
        }
        writer.writeEndElement();
        writer.writeEndElement();
    }
    writer.writeEndElement();

    writer.writeStartElement("serviceStateTable");
    for(const QSharedPointer<UPnPStateVariable> &stateVariable : stateVariables)
    {
        writer.writeStartElement("stateVariable");
        writer.writeAttribute("sendEvents", stateVariable->sendEvent? "yes":"no");
        writer.writeTextElement("", "name", stateVariable->name);
        writer.writeTextElement("", "dataType", stateVariable->dataType);
        if(!stateVariable->defaultValue.isEmpty())
        {
            writer.writeTextElement("", "defaultValue", stateVariable->defaultValue);
        }
        if(!stateVariable->allowedValues.isEmpty())
        {
            writer.writeStartElement("allowedValueList");
            for(const QString &val : stateVariable->allowedValues)
            {
                writer.writeTextElement("", "allowedValue", val);
            }
            writer.writeEndElement();
        }
        if(stateVariable->hasRange)
        {
            writer.writeStartElement("allowedValueRange");
            writer.writeTextElement("", "minimum", QString::number(stateVariable->range.min));
            writer.writeTextElement("", "maximum", QString::number(stateVariable->range.max));
            writer.writeTextElement("", "step", QString::number(stateVariable->range.step));
            writer.writeEndElement();
        }
        writer.writeEndElement();
    }
    writer.writeEndElement();   //serviceStateTable

    writer.writeEndElement();   //scpd
    writer.writeEndDocument();

}

void UPnPService::dumpStateVariables(QByteArray &content)
{
    QXmlStreamWriter writer(&content);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeNamespace("urn:schemas-upnp-org:event-1-0", "e");
    writer.writeStartElement("urn:schemas-upnp-org:event-1-0", "propertyset");
    for(const QSharedPointer<UPnPStateVariable> &stateVariable : stateVariables)
    {
        if(stateVariable->sendEvent)
        {
            writer.writeStartElement("urn:schemas-upnp-org:event-1-0", "property");
            writer.writeTextElement(stateVariable->name, stateVariable->value);
            writer.writeEndElement();
        }
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

UPnPStateVariable &UPnPService::addStateVariable(const QString &name, const QString &dataType)
{
    if(stateVarMap.contains(name)) return *stateVarMap[name];
    QSharedPointer<UPnPStateVariable> var(new UPnPStateVariable);
    var->name = name;
    var->dataType = dataType;
    stateVarMap[name] = var;
    stateVariables.push_back(var);
    return *var;
}

QSharedPointer<UPnPStateVariable> UPnPService::getStateVariable(const QString &name)
{
    if(stateVarMap.contains(name)) return stateVarMap[name];
    return nullptr;
}

UPnPActionDesc &UPnPService::addActionDesc(const QString &name)
{
    QSharedPointer<UPnPActionDesc> actDesc(new UPnPActionDesc);
    actDesc->name = name;
    actionDescs.push_back(actDesc);
    return *actDesc;
}

bool UPnPAction::setArg(const QString &name, const QString &val)
{
    const UPnPArgDesc *argDesc = nullptr;
    for(const QSharedPointer<UPnPArgDesc> &d : desc.argDescs)
    {
        if(d->name == name)
        {
            argDesc = d.get();
            break;
        }
    }
    if(!argDesc) return false;
    argMap.insert(name, QSharedPointer<UPnPArg>::create(*argDesc));
    argMap[name]->val = val;
    return true;
}

void UPnPAction::fillStateVariableArg()
{
    for(const QSharedPointer<UPnPArgDesc> &d : desc.argDescs)
    {
        if(d->d == UPnPArgDesc::OUT)
        {
            if(!argMap.contains(d->name))
            {
                argMap.insert(d->name, QSharedPointer<UPnPArg>::create(*d));
            }
            argMap[d->name]->val = d->relatedStateVariable.value;
        }
    }
}

bool UPnPAction::verifyArgs()
{
    for(const QSharedPointer<UPnPArgDesc> &d : desc.argDescs)
    {
        if(d->d == UPnPArgDesc::OUT) continue;
        if(!argMap.contains(d->name)) return false;
    }
    return true;
}

void UPnPAction::writeResponse(QByteArray &buffer)
{
    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeNamespace("http://schemas.xmlsoap.org/soap/envelope/", "s");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Envelope");
    writer.writeAttribute("http://schemas.xmlsoap.org/soap/envelope/", "encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Body");

    writer.writeStartElement("u:" + desc.name + "Response");
    writer.writeNamespace(service.serviceType, "u");
    for(const QSharedPointer<UPnPArgDesc> &d : desc.argDescs)
    {
        if(d->d == UPnPArgDesc::OUT)
        {
            assert(argMap.contains(d->name));
            writer.writeTextElement(d->name, argMap.value(d->name)->val);
        }
    }
    writer.writeEndElement();

    writer.writeEndElement();    //Body
    writer.writeEndElement();    //Envelope
    writer.writeEndDocument();
}

void UPnPAction::writeErrorResponse(QByteArray &buffer)
{
    QXmlStreamWriter writer(&buffer);
    writer.writeStartDocument();
    writer.writeNamespace("http://schemas.xmlsoap.org/soap/envelope/", "s");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Envelope");
    writer.writeAttribute("http://schemas.xmlsoap.org/soap/envelope/", "encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Body");

    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Fault");
    writer.writeTextElement("faultcode", "s:Client");
    writer.writeTextElement("faultstring", "UPnPError");
    writer.writeStartElement("detail");
    writer.writeStartElement("UPnPError");
    writer.writeDefaultNamespace("urn:schemas-upnp-org:control-1-0");
    writer.writeTextElement("errorCode", QString::number(errCode));
    writer.writeTextElement("errorDescription", errDesc);
    writer.writeEndElement();
    writer.writeEndElement();    //detail
    writer.writeEndElement();    //Fault

    writer.writeEndElement();    //Body
    writer.writeEndElement();    //Envelope
    writer.writeEndDocument();
}

UPnPArgDesc &UPnPActionDesc::addArgDesc(const QString &name, const UPnPStateVariable &stateVal, UPnPArgDesc::Direction d)
{
    QSharedPointer<UPnPArgDesc> argDesc(new UPnPArgDesc(stateVal));
    argDesc->name = name;
    argDesc->d = d;
    argDescs.push_back(argDesc);
    return *argDesc;
}
