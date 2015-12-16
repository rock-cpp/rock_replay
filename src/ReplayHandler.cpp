#include "ReplayHandler.hpp"

ReplayHandler::ReplayHandler(int argc, char** argv)
{
    RTT::corba::TaskContextServer::InitOrb(argc, argv);

    char *installDir = getenv("AUTOPROJ_CURRENT_ROOT");
    
    if(!installDir)
    {
//         std::cout << "Error, could not find AUTOPROJ_CURRENT_ROOT env.sh not sourced ?" << std::endl;
        throw std::runtime_error("Error, could not find AUTOPROJ_CURRENT_ROOT env.sh not sourced ?");
    }
    
    std::cout << "Loading all typekits " << std::endl;
    orocos_cpp::PluginHelper::loadAllPluginsInDir(std::string(installDir) + "/install/lib/orocos/gnulinux/types/");
    orocos_cpp::PluginHelper::loadAllPluginsInDir(std::string(installDir) + "/install/lib/orocos/types/");

    
    for(int i = 1; i < argc; i++)
    {
        filenames.push_back(argv[i]);
    }
    
    multiIndex = new pocolog_cpp::MultiFileIndex(filenames);
    streamToTask.resize(multiIndex->getAllStreams().size());

    for(pocolog_cpp::Stream *st : multiIndex->getAllStreams())
    {
        pocolog_cpp::InputDataStream *inputSt = dynamic_cast<pocolog_cpp::InputDataStream *>(st);
        if(!inputSt)
            continue;       

        std::string taskName = st->getName();
        taskName = taskName.substr(0, taskName.find_last_of('.'));
        
        LogTask *task = nullptr;
        auto it = logTasks.find(taskName);
        if(it == logTasks.end())
        {
            task = new LogTask(taskName);
            logTasks.insert(std::make_pair(taskName, task));
        }
        else
        {
            task = it->second;
        }
        task->addStream(*inputSt);

        size_t gIdx = multiIndex->getGlobalStreamIdx(st); 
        if(gIdx > streamToTask.size())
            throw std::runtime_error("Mixup detected");

        streamToTask[gIdx] = task;
    }

    replayFactor = 1000.;
    actualSpeed = replayFactor;
    curIndex = 0;
    finished = false;
    play = false;
    
    replayThread = new boost::thread(boost::bind(&ReplayHandler::replaySamples, boost::ref(*this)));
    
}


ReplayHandler::~ReplayHandler()
{
    delete multiIndex;
    delete replayThread;
}

void ReplayHandler::replaySample(size_t index) const
{
    try 
    {
        size_t globalStreamIndex = multiIndex->getGlobalStreamIdx(index);
        pocolog_cpp::InputDataStream *inputSt = dynamic_cast<pocolog_cpp::InputDataStream *>(multiIndex->getSampleStream(index));
        std::cout << "Gidx is " << globalStreamIndex << std::endl;
    
        streamToTask[globalStreamIndex]->replaySample(*inputSt, multiIndex->getPosInStream(index)); 
    } 
    catch(...)
    {
        std::cout << "Warning: ignoring corrupt sample: " << index << "/" << multiIndex->getSize() << std::endl;
    }
}



void ReplayHandler::replaySamples()
{
    boost::unique_lock<boost::mutex> lock(mut);
    
    std::cout << "Replaying all samples" << std::endl;
    base::Time start(base::Time::now()), lastExecute(base::Time::now());

    size_t allSamples = multiIndex->getSize();
    
    while(!finished)
    {
        
        while(!play)
            cond.wait(lock);
        
        if(allSamples > 0 && curIndex <= allSamples)
        {
            const base::Time curStamp(getTimeStamp(curIndex == 0 ? 0 : curIndex-1)), nextStamp(getTimeStamp(curIndex));
            const base::Time duration(nextStamp - curStamp);
            if(duration.toMicroseconds() < 0)
            {
                std::cout << "Warning: invalid sample order" << std::endl;
            }
            else
            {
                int64_t timeSinceLastExecute = (base::Time::now() - lastExecute).toMicroseconds();
                int64_t sleepDuration = (duration.toMicroseconds() / replayFactor);
                if(timeSinceLastExecute < sleepDuration)
                {
                    usleep(sleepDuration - timeSinceLastExecute);
                    actualSpeed = replayFactor;
                }
                else if(timeSinceLastExecute == sleepDuration)
                {
                    actualSpeed = replayFactor;
                }
                else
                {
                    if(timeSinceLastExecute == 0)
                        timeSinceLastExecute = 1;
                    
                    actualSpeed = static_cast<double>((double)duration.toMicroseconds() / (double)timeSinceLastExecute);
                }           
                
            }
                        
            //TODO check if chronological ordering is right
            replaySample(curIndex);
            curIndex++;
        }
        else
            play = false;
        
    }
    
}

const base::Time ReplayHandler::getTimeStamp(size_t globalIndex) const
{    
    size_t globalStreamIndex = multiIndex->getGlobalStreamIdx(globalIndex);
    pocolog_cpp::Index &idx = multiIndex->getSampleStream(globalStreamIndex)->getFileIndex();
    return idx.getSampleTime(multiIndex->getPosInStream(globalIndex));
}


double ReplayHandler::getActualSpeed() const
{
    return actualSpeed;
}



void ReplayHandler::setReplayFactor(double factor)
{
    this->replayFactor = factor;
}


void ReplayHandler::next()
{

}

void ReplayHandler::previous()
{

}


void ReplayHandler::setSampleIndex(uint index)
{

}

void ReplayHandler::toggle()
{
    play = !play;
    if(play)
        cond.notify_one();
}






