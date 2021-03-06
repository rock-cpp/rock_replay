#include "LogTask.hpp"

#include "LogFileHelper.hpp"

#include <base-logging/Logging.hpp>
#include <rtt/TaskContext.hpp>
#include <rtt/base/OutputPortInterface.hpp>
#include <rtt/transports/corba/CorbaDispatcher.hpp>
#include <rtt/transports/corba/TaskContextServer.hpp>
#include <rtt/typelib/TypelibMarshallerBase.hpp>
#include <rtt/types/Types.hpp>
#include <string>

LogTask::LogTask(const std::string& taskName, const std::string& prefix, const std::string& renaming)
    : prefixedName(prefix + taskName)
    , originalName(taskName)
{
    try
    {
        if(!renaming.empty())
        {
            prefixedName = prefix + renaming;
        }

        task = std::unique_ptr<RTT::TaskContext>(new RTT::TaskContext(prefixedName));
        RTT::corba::TaskContextServer::Create(task.get());
        RTT::corba::CorbaDispatcher* dispatcher = RTT::corba::CorbaDispatcher::Instance(task->ports());
        dispatcher->setScheduler(ORO_SCHED_OTHER);
        dispatcher->setPriority(RTT::os::LowestPriority);

        LOG_INFO_S << "created task " << taskName;
    }
    catch(std::runtime_error& e)
    {
        LOG_WARN_S << "could not create task " << taskName << " because " << e.what();
    }
}

LogTask::~LogTask()
{
    RTT::corba::TaskContextServer::CleanupServer(task.get());
}

void LogTask::activateLoggingForPort(const std::string& portName, bool activate)
{
    for(const auto& idx2Port : streamIdx2Port)
    {
        if(idx2Port.second->name == portName)
        {
            idx2Port.second->active = activate;
        }
    }
}

bool LogTask::addStream(pocolog_cpp::InputDataStream& stream)
{
    if(isStreamForThisTask(stream))
    {
        const auto portName = LogFileHelper::splitStreamName(stream.getName()).second;

        auto portHandle = createPortHandle(portName, stream);
        if(portHandle && task && !task->getPort(portName))
        {
            task->ports()->addPort(portHandle->port->getName(), *portHandle->port);
            streamIdx2Port.emplace(stream.getIndex(), std::move(portHandle));
            return true;
        }
    }

    return false;
}

std::unique_ptr<LogTask::PortHandle> LogTask::createPortHandle(const std::string& portName, pocolog_cpp::InputDataStream& inputStream)
{
    RTT::types::TypeInfoRepository::shared_ptr ti = RTT::types::TypeInfoRepository::Instance();
    RTT::types::TypeInfo* type = ti->type(inputStream.getCXXType());
    if(!type)
    {
        LOG_WARN_S << "cannot find " << inputStream.getCXXType() << " in the type info repository";
        return nullptr;
    }

    RTT::base::OutputPortInterface* writer = type->outputPort(portName);
    auto typekitTransport = dynamic_cast<orogen_transports::TypelibMarshallerBase*>(type->getProtocol(orogen_transports::TYPELIB_MARSHALLER_ID));
    if(!typekitTransport)
    {
        LOG_WARN_S << "cannot report ports of type " << type->getTypeName() << " as no typekit generated by orogen defines it";
        return nullptr;
    }

    // TODO: check if local type is same as logfile type

    try
    {
        auto transportHandle = typekitTransport->createSample();
        PortHandle* portHandle =
            new PortHandle(portName, typekitTransport, typekitTransport->getDataSource(transportHandle), transportHandle, writer, true, inputStream);
        return std::unique_ptr<PortHandle>(portHandle);
    }
    catch(const RTT::internal::bad_assignment& ba)
    {
        return nullptr;
    }
}

bool LogTask::replaySample(uint64_t streamIndex, uint64_t indexInStream)
{
    auto& portHandle = streamIdx2Port.at(streamIndex);

    bool canPortBeSkippedResult;
    if(canPortBeSkipped(canPortBeSkippedResult, portHandle))
    {
        return canPortBeSkippedResult;
    }

    bool sampleCanBeUnmarshaled = unmarshalSample(portHandle, indexInStream);
    if(sampleCanBeUnmarshaled)
    {
        checkTaskStateChange(portHandle);
        portHandle->port->write(portHandle->sample);
    }

    return sampleCanBeUnmarshaled;
}

bool LogTask::canPortBeSkipped(bool& result, std::unique_ptr<PortHandle>& portHandle)
{
    if(!portHandle->active || !portHandle->port)
    {
        result = false;
        return true;
    }

    if(!portHandle->port->connected())
    {
        result = true;
        return true;
    }

    return false;
}

bool LogTask::unmarshalSample(std::unique_ptr<PortHandle>& portHandle, uint64_t indexInStream)
{
    std::vector<uint8_t> data;
    if(!portHandle->inputDataStream.getSampleData(data, indexInStream))
    {
        LOG_WARN_S << "Warning, could not replay sample: " << portHandle->inputDataStream.getName() << " " << indexInStream;
        return false;
    }

    try
    {
        portHandle->transport->unmarshal(data, portHandle->transportHandle);
    }
    catch(...)
    {
        LOG_ERROR_S << "caught marshall error...";
        return false;
    }

    return true;
}

void LogTask::checkTaskStateChange(std::unique_ptr<PortHandle>& portHandle)
{
    if(portHandle->name == "state")
    {
        switch(std::stoi(portHandle->sample.get()->toString()))
        {
        case 0: // INIT
            task->configure();
            break;
        case 1: // PRE_OPERATIONAL
            break;
        case 2: // FATAL_ERROR
            break;
        case 3: // EXCEPTION
            break;
        case 4: // STOPPED
            task->stop();
            break;
        case 5: // RUNNING
            task->start();
            break;
        case 6: // RUNTIME_ERROR
            break;
        default:
            task->start();
        }
    }
}

bool LogTask::isStreamForThisTask(const pocolog_cpp::InputDataStream& inputStream)
{
    auto taskName = LogFileHelper::splitStreamName(inputStream.getName()).first;
    return prefixedName.find(taskName) != std::string::npos || originalName.find(taskName) != std::string::npos;
}

LogTask::PortCollection LogTask::getPortCollection()
{
    PortCollection collection;
    for(const auto& portHandlePair : streamIdx2Port)
    {
        const auto& portHandle = portHandlePair.second;
        collection.emplace_back(portHandle->name, portHandle->inputDataStream.getCXXType());
    }

    return collection;
}

std::string LogTask::getName()
{
    return prefixedName;
}

bool LogTask::isValid()
{
    return task.get();
}