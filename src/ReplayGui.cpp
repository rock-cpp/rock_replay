#include "ReplayGui.h"

#include "LogFileHelper.hpp"

#include <QFileDialog>
#include <QMessageBox>

ReplayGui::ReplayGui(QMainWindow* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // initing models
    tasksModel = new QStandardItemModel();
    ui.taskNameList->setModel(tasksModel);
    ui.taskNameList->setAlternatingRowColors(true);

    tasksModel->setColumnCount(2);
    tasksModel->setHorizontalHeaderLabels(QStringList({"Taskname", "Type"}));
    ui.taskNameList->setColumnWidth(0, 300);

    if(this->palette().color(QPalette::Window).black() > 150) // dark theme used, background must be dark as well
    {
        ui.taskNameList->setPalette(this->palette().color(QPalette::Window));
    }

    // timer
    statusUpdateTimer = new QTimer();
    statusUpdateTimer->setInterval(10);
    checkFinishedTimer = new QTimer();
    checkFinishedTimer->setInterval(10);

    QPalette palette;
    palette.setColor(QPalette::Window, Qt::red);
    ui.curPortName->setPalette(palette);

    // icons
    playIcon.addFile(
        QString::fromUtf8(":/icons/icons/Icons-master/picol_latest_prerelease_svg/controls_play.svg"), QSize(), QIcon::Normal, QIcon::On);
    pauseIcon.addFile(
        QString::fromUtf8(":/icons/icons/Icons-master/picol_latest_prerelease_svg/controls_pause.svg"), QSize(), QIcon::Normal, QIcon::On);

    // slot connections
    QObject::connect(ui.playButton, SIGNAL(clicked()), this, SLOT(togglePlay()));
    QObject::connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(stopPlay()));
    QObject::connect(ui.forwardButton, SIGNAL(clicked()), this, SLOT(forward()));
    QObject::connect(ui.backwardButton, SIGNAL(clicked()), this, SLOT(backward()));
    QObject::connect(ui.intervalAButton, SIGNAL(clicked()), this, SLOT(setIntervalA()));
    QObject::connect(ui.intervalBButton, SIGNAL(clicked()), this, SLOT(setIntervalB()));
    QObject::connect(statusUpdateTimer, SIGNAL(timeout()), this, SLOT(statusUpdate()));
    QObject::connect(ui.speedBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeedBox()));
    QObject::connect(ui.progressSlider, SIGNAL(sliderReleased()), this, SLOT(progressSliderUpdate()));
    QObject::connect(checkFinishedTimer, SIGNAL(timeout()), this, SLOT(handleRestart()));
    QObject::connect(ui.infoAbout, SIGNAL(triggered()), this, SLOT(showInfoAbout()));
    QObject::connect(ui.actionOpenLogfile, SIGNAL(triggered()), this, SLOT(showOpenFile()));
    QObject::connect(tasksModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(handleItemChanged(QStandardItem*)));
}

ReplayGui::~ReplayGui()
{
    replayHandler.stop();
}

void ReplayGui::initReplayHandler(
    const std::vector<std::string>& fileNames, const std::string& prefix, const std::vector<std::string>& whiteList,
    const std::map<std::string, std::string>& renamings)
{
    replayHandler.init(fileNames, prefix, whiteList, renamings);

    QString title;
    // window title
    switch(fileNames.size())
    {
    case 0:
        title = "Rock-Replay";
        break;
    case 1:
        title = QString(fileNames[0].c_str());
        break;
    default:
        title = QString("Multi Logfile Replay");
        break;
    }

    replayHandler.setReplaySpeed(ui.speedBox->value());
    ui.progressSlider->setMaximum(replayHandler.getMaxIndex());

    this->setWindowTitle(title);
    statusUpdate();
}

// #######################################
// ######### SLOT IMPLEMENTATION #########
// #######################################

void ReplayGui::setIntervalA()
{
    if(replayHandler.getMinSpan())
    {
        ui.intervalAButton->setText("A");
        replayHandler.setMinSpan(0);
    }
    else if(replayHandler.getCurIndex())
    {
        ui.intervalAButton->setText("X");
        replayHandler.setMinSpan(replayHandler.getCurIndex());
    }

    statusUpdate();
}

void ReplayGui::setIntervalB()
{
    if(replayHandler.getMaxSpan() != replayHandler.getMaxIndex())
    {
        ui.intervalBButton->setText("B");
        replayHandler.setMaxSpan(replayHandler.getMaxIndex());
    }
    else if(replayHandler.getCurIndex() != replayHandler.getMaxIndex())
    {
        ui.intervalBButton->setText("X");
        replayHandler.setMaxSpan(replayHandler.getCurIndex());
    }

    statusUpdate();
}

void ReplayGui::togglePlay()
{
    if(replayHandler.getMaxIndex() && !replayHandler.isPlaying())
    {
        checkFinishedTimer->start();
        replayHandler.play();
        setGuiPlaying();
    }
    else
    {
        checkFinishedTimer->stop();
        replayHandler.pause();
        setGuiPaused();
    }
}

void ReplayGui::handleRestart()
{
    if(replayHandler.hasFinished())
    {
        stopPlay();
        statusUpdate();
        checkFinishedTimer->stop();
        if(ui.repeatButton->isChecked())
        {
            checkFinishedTimer->start();
            togglePlay();
        }
    }
}

void ReplayGui::setGuiPlaying()
{
    ui.playButton->setChecked(true);
    ui.playButton->setIcon(pauseIcon);
    statusUpdateTimer->start();
    checkFinishedTimer->start();
    ui.forwardButton->setEnabled(false);
    ui.backwardButton->setEnabled(false);
    ui.progressSlider->setEnabled(false);
    ui.intervalAButton->setEnabled(false);
    ui.intervalBButton->setEnabled(false);
    statusUpdate();
}

void ReplayGui::setGuiPaused()
{
    ui.playButton->setIcon(playIcon);
    ui.playButton->setChecked(false);
    statusUpdateTimer->stop();
    checkFinishedTimer->stop();
    ui.forwardButton->setEnabled(true);
    ui.backwardButton->setEnabled(true);
    ui.progressSlider->setEnabled(true);
    ui.intervalAButton->setEnabled(true);
    ui.intervalBButton->setEnabled(true);
    statusUpdate();
}

void ReplayGui::statusUpdate()
{
    QString interval = "    [" + QString::number(replayHandler.getMinSpan()) + "/" + QString::number(replayHandler.getMaxSpan()) + "]";
    ui.curSampleNum->setText(QString::number(replayHandler.getCurIndex()) + "/" + QString::number(replayHandler.getMaxIndex()) + interval);
    ui.curTimestamp->setText(replayHandler.getCurTimeStamp().c_str());
    ui.curPortName->setText(replayHandler.getCurSamplePortName().c_str());
    ui.curPortName->setAutoFillBackground(!replayHandler.canSampleBeReplayed());
    ui.progressSlider->setSliderPosition(replayHandler.getCurIndex());
    ui.speedBar->setValue(replayHandler.getCurrentSpeed() * 100);
}

void ReplayGui::stopPlay()
{
    replayHandler.stop();
    setGuiPaused();
    statusUpdate();
}

void ReplayGui::setSpeedBox()
{
    double speed = ui.speedBox->value();
    replayHandler.setReplaySpeed(speed);
}

void ReplayGui::backward()
{
    replayHandler.previous();
    statusUpdate();
}

void ReplayGui::forward()
{
    replayHandler.next();
    statusUpdate();
}

void ReplayGui::progressSliderUpdate()
{
    replayHandler.setSampleIndex(ui.progressSlider->value());
    statusUpdate();
}

void ReplayGui::showOpenFile()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select logfile(s)", "", "Logfiles: *.log");
    QStringList fileNamesCopy = fileNames; // doc says so

    std::vector<std::string> stdStrings;
    for(QList<QString>::iterator it = fileNamesCopy.begin(); it != fileNamesCopy.end(); it++)
    {
        stdStrings.push_back(it->toStdString());
    }

    stopPlay();
    replayHandler.deinit();
    initReplayHandler(stdStrings, "");
    updateTaskView();
    statusUpdate();
}

void ReplayGui::showInfoAbout()
{
    QMessageBox::information(this, "Credits", QString("PICOL iconset: http://www.picol.org\n"), QMessageBox::Ok, 0);
}

void ReplayGui::handleItemChanged(QStandardItem* item)
{
    const QModelIndex index = tasksModel->indexFromItem(item);
    QItemSelectionModel* selModel = ui.taskNameList->selectionModel();
    const std::string portName = item->text().toStdString();
    replayHandler.activateReplayForPort(item->parent()->text().toStdString(), portName, item->checkState() == Qt::Checked);
    selModel->select(QItemSelection(index, index), item->checkState() == Qt::Checked ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

void ReplayGui::updateTaskView()
{
    while(tasksModel->rowCount() > 0)
    {
        QList<QStandardItem*> rows = tasksModel->takeRow(0);
        for(QStandardItem* item : rows)
            delete item;
    }

    for(const auto& taskName2Ports : replayHandler.getTaskNamesWithPorts())
    {
        auto* task = new QStandardItem(taskName2Ports.first.c_str());
        task->setCheckable(false);

        QList<QStandardItem*> portTypes;
        for(const auto& portName : taskName2Ports.second)
        {
            QStandardItem* port = new QStandardItem(portName.first.c_str());
            port->setCheckable(true);
            port->setData(Qt::Checked, Qt::CheckStateRole);
            task->appendRow(port);
            portTypes.push_back(new QStandardItem(portName.second.c_str()));
        }

        task->appendColumn(portTypes);

        tasksModel->appendRow(task);
    }
}
