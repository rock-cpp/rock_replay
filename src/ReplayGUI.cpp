#include "ReplayGUI.h"
#include <vizkit3d/Vizkit3DWidget.hpp>
#include <qwt_abstract_scale_draw.h>


ReplayGui::ReplayGui(QMainWindow *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    
    // initing  lists
    taskNameList = new QStringList();
    
    // initing models
    taskNameListModel = new QStringListModel(*taskNameList);
    ui.taskNameList->setModel(taskNameListModel);    
    
    // timer
    statusUpdateTimer = new QTimer();
    statusUpdateTimer->setInterval(10);
    
    // speed bar
    ui.speedBar->setMinimum(0);
    ui.speedBar->setFormat("paused");
    ui.speedBar->setValue(0);

    // labels
    
    // plot
    ui.qwtPlot->enableAxis(QwtPlot::yLeft, false);
    ui.qwtPlot->enableAxis(QwtPlot::xBottom, false);
    
    // icons
    playIcon.addFile(QString::fromUtf8(":/icons/icons/Icons-master/picol_latest_prerelease_svg/controls_play.svg"), QSize(), QIcon::Normal, QIcon::On);
    pauseIcon.addFile(QString::fromUtf8(":/icons/icons/Icons-master/picol_latest_prerelease_svg/controls_pause.svg"), QSize(), QIcon::Normal, QIcon::On);
    
    // slot connections
    QObject::connect(ui.playButton, SIGNAL(clicked()), this, SLOT(togglePlay()));
    QObject::connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(stopPlay()));
    QObject::connect(statusUpdateTimer, SIGNAL(timeout()), this, SLOT(statusUpdate()));
    QObject::connect(ui.speedBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeedBox()));
    QObject::connect(ui.speedSlider, SIGNAL(sliderReleased()), this, SLOT(setSpeedSlider()));
    
    vizkit3d::Vizkit3DWidget *vizkit3dWidget = new vizkit3d::Vizkit3DWidget();
    QStringList *plugins = vizkit3dWidget->getAvailablePlugins();
    pluginRepo = new Vizkit3dPluginRepository(*plugins);
    delete plugins;
    
}

ReplayGui::~ReplayGui()
{
    delete replayHandler;
    delete pluginRepo;
}


void ReplayGui::initReplayHandler(int argc, char* argv[])
{
    replayHandler = new ReplayHandler(argc, argv);
    
    // speed bar
    ui.speedBar->setMaximum(100);
    
    // progress bar
    ui.progressSlider->setMaximum(replayHandler->getMaxIndex());
    
    // labels
    ui.label_sample_count->setText(QString(("/ " + std::to_string(replayHandler->getMaxIndex())).c_str()));
    
    // window title
    if(argc > 1) 
        this->setWindowTitle(QString(argv[1]));
}


void ReplayGui::updateTaskNames()
{
    for(const std::pair<std::string, LogTask*>& cur : replayHandler->getAllLogTasks())
    {        
        std::string taskName = cur.first;
        for(const std::string& portName : cur.second->getTaskContext()->ports()->getPortNames())
        {
            taskNameList->append(QString((taskName + "::" + portName).c_str()));
        }
       
    }

    taskNameListModel->setStringList(*taskNameList);
    
}



int ReplayGui::boxToSlider(double val)
{
    if(val <= 1)
    {
        return val * 50;
    }
    else if(val <= 1000)
    {
        return (val / 40.0) + 50;
    }
    else
    {
        return (val / 400.0) + 75;
    }
}

double ReplayGui::sliderToBox(int val)
{
    if(val <= 50)
    {
        return val / 50.0;
    }
    else if(val <= 75)
    {
        return (val - 50) * 40.0;
    }
    else
    {
        return (val - 75) * 400.0;
    }
}





// #######################################
// ######### SLOT IMPLEMENTATION #########
// #######################################

void ReplayGui::togglePlay()
{
    replayHandler->toggle();
    if(ui.playButton->isChecked())
    {
        ui.playButton->setIcon(pauseIcon);
        ui.playButton->setChecked(true);
        ui.speedBar->setFormat("%p%");
        statusUpdateTimer->start();
    }
    else
    {
        ui.playButton->setIcon(playIcon);
        ui.playButton->setChecked(false);
        statusUpdateTimer->stop();
        ui.speedBar->setValue(0);
        ui.speedBar->setFormat("paused");
    }
}


void ReplayGui::statusUpdate()
{
    ui.speedBar->setValue(replayHandler->getCurrentSpeed() * ui.speedBar->maximum());
    ui.curSampleNum->setText(QString::number(replayHandler->getCurIndex()));
    ui.curTimestamp->setText(replayHandler->getCurTimeStamp().c_str());
    ui.curPortName->setText(replayHandler->getCurSamplePortName().c_str());
    ui.progressSlider->setSliderPosition(replayHandler->getCurIndex());
}

void ReplayGui::stopPlay()
{
    replayHandler->stop();
    ui.playButton->setIcon(playIcon);
    ui.playButton->setChecked(false);
    statusUpdateTimer->stop();
    ui.speedBar->setValue(0);
    ui.speedBar->setFormat("paused");
    statusUpdate();
}


void ReplayGui::setSpeedBox()
{
    double speed = ui.speedBox->value();
    replayHandler->setReplayFactor(speed);
    ui.speedSlider->setValue(boxToSlider(speed));
    
}

void ReplayGui::setSpeedSlider()
{
    double speed = sliderToBox(ui.speedSlider->value());
    replayHandler->setReplayFactor(speed);
    ui.speedBox->setValue(speed);
}

