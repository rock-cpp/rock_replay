#pragma once

#include "ReplayHandler.hpp"
#include "ui_main.h"

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

/**
 * @brief Class representing the Qt main window.
 *
 */
class ReplayGui : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     *
     * @param parent: Possible parent to set.
     */
    ReplayGui(QMainWindow* parent = 0);

    /**
     * @brief Destructor.
     *
     */
    ~ReplayGui();

    /**
     * @brief Updates the task view when the model changed.
     *
     */
    void updateTaskView();

    /**
     * @brief Inits the underlying replay handler with file names to load and a optional log task prefix.
     *
     * @param fileNames: List of file names to load.
     * @param prefix: Optional prefix to set for all log tasks.
     * @param whiteList: List of regular expressions to filter whitelisted streams.
     * @param renamings: Map of renamings.
     */
    void initReplayHandler(
        const std::vector<std::string>& fileNames, const std::string& prefix, const std::vector<std::string>& whiteList = {},
        const std::map<std::string, std::string>& renamings = {});

protected:
    /**
     * @brief Ui main window containing graphical elements.
     *
     */
    Ui::MainWindow ui;

    /**
     * @brief Replay handler.
     *
     */
    ReplayHandler replayHandler;

    /**
     * @brief Task model to pass to main ui.
     *
     */
    QStandardItemModel* tasksModel;

private:
    /**
     * @brief Play icon.
     *
     */
    QIcon playIcon;

    /**
     * @brief Pause icon.
     *
     */
    QIcon pauseIcon;

    /**
     * @brief Periodic status update timer.
     *
     */
    QTimer* statusUpdateTimer;

    /**
     * @brief Timer to check periodically if replaying is finished.
     *
     */
    QTimer* checkFinishedTimer;

    /**
     * @brief Sets the gui in a paused mode, inverting icons and enabling certain interactions.
     *
     */
    void setGuiPaused();

    /**
     * @brief Sets the gui in a playing mode, inverting icons and disabling certain interactions.
     *
     */
    void setGuiPlaying();

public slots:

    /**
     * @brief Handles when a task view item changed.
     *
     */
    void handleItemChanged(QStandardItem* item);

    /**
     * @brief Toggles the play/pause mode.
     *
     */
    void togglePlay();

    /**
     * @brief Stops playing.
     *
     */
    void stopPlay();

    /**
     * @brief Updates the whole gui when replay handler parameters are changed.
     *
     */
    void statusUpdate();

    /**
     * @brief Forwards the value of the speed box to the replay handler.
     *
     */
    void setSpeedBox();

    /**
     * @brief Sets the replay index pointer one step further.
     *
     */
    void forward();

    /**
     * @brief Sets the replay index pointer one step back.
     *
     */
    void backward();

    /**
     * @brief Forwards the value of the progress slider to the replay handler.
     *
     */
    void progressSliderUpdate();

    /**
     * @brief Shows the about info and credits.
     *
     */
    void showInfoAbout();

    /**
     * @brief Opens a dialog to open new log files.
     *
     */
    void showOpenFile();

    /**
     * @brief Handles a restart of replaying when repeat option is enabled.
     *
     */
    void handleRestart();

    /**
     * @brief Sets the minimum span.
     *
     */
    void setIntervalA();

    /**
     * @brief Sets the maximum span.
     *
     */
    void setIntervalB();
};
