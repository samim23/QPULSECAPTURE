#include <QDateTime>
#include "mainwindow.h"
//------------------------------------------------------------------------------------

#define FRAME_WIDTH 480
#define FRAME_HEIGHT 320
#define FRAME_MARGIN 5

//------------------------------------------------------------------------------------
const char * MainWindow::QPlotDialogName[]=
{
    "Signal vs frame",
    "Amplitude spectrum",
    "Frame time vs frame",
    "Filter output vs frame",
    "Signal phase diagram",
};
//------------------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
{
    setWindowTitle(APP_NAME);
    setMinimumSize(FRAME_WIDTH, FRAME_HEIGHT);

    //--------------------------------------------------------------
    pt_display = new QImageWidget(); // Widgets without a parent are “top level” (independent) widgets. All other widgets are drawn within their parent
    pt_mainLayout = new QVBoxLayout();
    pt_display->setLayout(pt_mainLayout);
    this->setCentralWidget(pt_display);

    //--------------------------------------------------------------
    pt_infoLabel = new QLabel(tr("<i>Choose a menu option, or make right-click to invoke a context menu</i>"));
    pt_infoLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    pt_infoLabel->setAlignment(Qt::AlignCenter);
    pt_infoLabel->setWordWrap( true );
    pt_infoLabel->setFont( QFont("MS Shell Dlg 2", 12, QFont::Normal) );
    pt_mainLayout->addWidget(pt_infoLabel);

    //--------------------------------------------------------------
    createActions();
    createMenus();
    createThreads();

    //--------------------------------------------------------------
    m_dialogSetCounter = 0;

    //--------------------------------------------------------------
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(MS_INTERVAL);
    m_timer.stop();

    //--------------------------------------------------------------
    resize(570, 480);
    statusBar()->showMessage(tr("A context menu is available by right-clicking"));
}
//------------------------------------------------------------------------------------

void MainWindow::createActions()
{
    pt_openSessionAct = new QAction(tr("New &session"),this);
    pt_openSessionAct->setStatusTip(tr("Open new measurement session"));
    connect(pt_openSessionAct, SIGNAL(triggered()), this, SLOT(configure_and_start_session()));

    pt_pauseAct = new QAction(tr("&Pause"), this);
    pt_pauseAct->setStatusTip(tr("Stop a measurement session"));
    connect(pt_pauseAct, SIGNAL(triggered()), this, SLOT(onpause()));

    pt_resumeAct = new QAction(tr("&Resume"), this);
    pt_resumeAct->setStatusTip(tr("Resume a measurement session"));
    connect(pt_resumeAct, SIGNAL(triggered()), this, SLOT(onresume()));

    pt_exitAct = new QAction(tr("E&xit"), this);
    pt_exitAct->setStatusTip(tr("Application exit"));
    connect(pt_exitAct, SIGNAL(triggered()), this, SLOT(close()));

    pt_aboutAct = new QAction(tr("&About"), this);
    pt_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(pt_aboutAct, SIGNAL(triggered()), this, SLOT(show_about()));

    pt_helpAct = new QAction(tr("&Help"), this);
    pt_helpAct->setStatusTip(tr("Show the application's Help"));
    connect(pt_helpAct, SIGNAL(triggered()), this, SLOT(show_help()));

    pt_deviceResAct = new QAction(tr("&Resolution"), this);
    pt_deviceResAct->setStatusTip(tr("Open a video device resolution dialog"));
    connect(pt_deviceResAct, SIGNAL(triggered()), this, SLOT(opendeviceresolutiondialog()));

    pt_deviceSetAct = new QAction(tr("&Preset"), this);
    pt_deviceSetAct->setStatusTip(tr("Open a video device settings dialog"));
    connect(pt_deviceSetAct, SIGNAL(triggered()), this, SLOT(opendevicesettingsdialog()));

    pt_DirectShowAct = new QAction(tr("&DSpreset"), this);
    pt_DirectShowAct->setStatusTip(tr("Open a device-driver embedded settings dialog"));
    connect(pt_DirectShowAct, SIGNAL(triggered()), this, SLOT(callDirectShowSdialog()));

    pt_openPlotDialog = new QAction(tr("&New plot"), this);
    pt_openPlotDialog->setStatusTip(tr("Create a new window for the visualization of appropriate process"));
    connect(pt_openPlotDialog, SIGNAL(triggered()), this, SLOT(createPlotDialog()));

    pt_recordAct = new QAction(tr("&Record"), this);
    pt_recordAct->setStatusTip(tr("Save all computation results to output .txt file"));
    pt_recordAct->setCheckable(true);
    connect(pt_recordAct, &QAction::triggered, this, &MainWindow::saveRecord);

    pt_colorActGroup = new QActionGroup(this);

    pt_redAct = new QAction(tr("Enroll red"), pt_colorActGroup);
    pt_redAct->setStatusTip(tr("Process red color channel"));
    pt_redAct->setCheckable(true);

    pt_greenAct = new QAction(tr("Enroll green"), pt_colorActGroup);
    pt_greenAct->setStatusTip(tr("Process green color channel"));
    pt_greenAct->setCheckable(true);

    pt_blueAct = new QAction(tr("Enroll blue"), pt_colorActGroup);
    pt_blueAct->setStatusTip(tr("Process blue color channel"));
    pt_blueAct->setCheckable(true);

    pt_greenAct->setChecked(true);

    pt_mapper = new QSignalMapper(this);
    pt_mapper->setMapping(pt_redAct, QHarmonicProcessor::Red);
    pt_mapper->setMapping(pt_greenAct, QHarmonicProcessor::Green);
    pt_mapper->setMapping(pt_blueAct, QHarmonicProcessor::Blue);
    connect(pt_redAct, SIGNAL(triggered()), pt_mapper, SLOT(map()));
    connect(pt_greenAct, SIGNAL(triggered()), pt_mapper, SLOT(map()));
    connect(pt_blueAct, SIGNAL(triggered()), pt_mapper, SLOT(map()));
    connect(pt_mapper, SIGNAL(mapped(int)), this, SLOT(changeColor(int)));
}

//------------------------------------------------------------------------------------

void MainWindow::createMenus()
{
    pt_fileMenu = this->menuBar()->addMenu(tr("&File"));
    pt_fileMenu->addAction(pt_openSessionAct);
    pt_fileMenu->addSeparator();
    pt_fileMenu->addAction(pt_exitAct);

    //------------------------------------------------
    pt_optionsMenu = menuBar()->addMenu(tr("&Options"));
    pt_optionsMenu->addAction(pt_openPlotDialog);
    pt_optionsMenu->addAction(pt_recordAct);
    pt_optionsMenu->addSeparator();
    pt_optionsMenu->addActions(pt_colorActGroup->actions());
    pt_optionsMenu->setEnabled(false);

    //-------------------------------------------------
    pt_deviceMenu = menuBar()->addMenu(tr("&Video"));
    pt_deviceMenu->addAction(pt_deviceSetAct);
    pt_deviceMenu->addAction(pt_deviceResAct);
    pt_deviceMenu->addSeparator();
    pt_deviceMenu->addAction(pt_DirectShowAct);

    //--------------------------------------------------
    pt_helpMenu = menuBar()->addMenu(tr("&Help"));
    pt_helpMenu->addAction(pt_helpAct);
    pt_helpMenu->addAction(pt_aboutAct);
}

//------------------------------------------------------------------------------------

void MainWindow::createThreads()
{
    //-------------------Pointers for objects------------------------
    pt_improcThread = new QThread(this); // Make an own QThread for opencv interface
    pt_opencvProcessor = new QOpencvProcessor();
    pt_opencvProcessor->moveToThread( pt_improcThread );
    connect(pt_improcThread, &QThread::finished, pt_opencvProcessor, &QObject::deleteLater);
    //---------------------------------------------------------------

    pt_harmonicProcessor = NULL;
    pt_harmonicThread = NULL;

    //--------------------QVideoCapture------------------------------
    pt_videoCapture = new QVideoCapture(this);

    //----------Register openCV types in Qt meta-type system---------
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<cv::Rect>("cv::Rect");

    //----------------------Connections------------------------------
    connect(pt_opencvProcessor, SIGNAL(frameProcessed(cv::Mat,double)), pt_display, SLOT(updateImage(cv::Mat,double)));
    connect(pt_display, SIGNAL(rect_was_entered(cv::Rect)), pt_opencvProcessor, SLOT(updateRect(cv::Rect)));
    connect(pt_opencvProcessor, SIGNAL(selectRegion(const char*)), pt_display, SLOT(set_warning_status(const char*)));
    connect(pt_videoCapture, SIGNAL(frame_was_captured(cv::Mat)), pt_opencvProcessor, SLOT(processRegion(cv::Mat)));
    //----------------------Thread start-----------------------------
    pt_improcThread->start();
}

//------------------------------------------------------------------------------------

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(pt_openSessionAct);
    menu.addSeparator();
    menu.addAction(pt_pauseAct);
    menu.addAction(pt_resumeAct);
    menu.exec(event->globalPos());
}

//------------------------------------------------------------------------------------

bool MainWindow::openvideofile()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Select file"), "/video", tr("Video (*.avi *.mp4 *wmv)"));
    while ( !pt_videoCapture->openfile(fileName) )
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open video file!"), QMessageBox::Ok | QMessageBox::Open, this, Qt::Dialog);
        if(msgBox.exec() == QMessageBox::Open)
        {
            fileName = QFileDialog::getOpenFileName(this,tr("Select file"), "/video", tr("Video (*.avi *.mp4 *wmv)"));
        }
        else
        {
            return false;
        }
    }
    if ( pt_infoLabel ) // just remove label
    {
        pt_mainLayout->removeWidget(pt_infoLabel);
        delete pt_infoLabel;
        pt_infoLabel = NULL;
    }
    return true;
}

//------------------------------------------------------------------------------------

bool MainWindow::opendevice()
{
    pt_videoCapture->open_deviceSelectDialog();
    while ( !pt_videoCapture->opendevice() )
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open video capture device!"), QMessageBox::Ok | QMessageBox::Open, this, Qt::Dialog);
        if(msgBox.exec() == QMessageBox::Open)
        {
            pt_videoCapture->open_deviceSelectDialog();
        }
        else
        {
            return false;
        }
    }   
    if ( pt_infoLabel ) // just remove label
    {
        pt_mainLayout->removeWidget(pt_infoLabel);
        delete pt_infoLabel;
        pt_infoLabel = NULL;
    }
    return true;
}

//------------------------------------------------------------------------------------

void MainWindow::show_about()
{
   QDialog *aboutdialog = new QDialog();
   aboutdialog->setWindowTitle("About dialog");
   aboutdialog->setFixedSize(256,128);

   QVBoxLayout *templayout = new QVBoxLayout();
   templayout->setMargin(5);

   QLabel *projectname = new QLabel( QString(APP_NAME) + " " + QString(APP_VERSION) );
   projectname->setFrameStyle(QFrame::Box | QFrame::Raised);
   projectname->setAlignment(Qt::AlignCenter);
   QLabel *projectauthors = new QLabel( QString(APP_AUTHOR) + "\n\n" + QString(APP_COMPANY) + "\n\n" + QString(APP_RELEASE_DATE) );
   projectauthors->setWordWrap(true);
   projectauthors->setAlignment(Qt::AlignCenter);
   QLabel *hyperlink = new QLabel( APP_EMAIL );
   hyperlink->setToolTip("Tap here to send an email");
   hyperlink->setOpenExternalLinks(true);
   hyperlink->setAlignment(Qt::AlignCenter);

   templayout->addWidget(projectname);
   templayout->addWidget(projectauthors);
   templayout->addWidget(hyperlink);

   aboutdialog->setLayout(templayout);
   aboutdialog->exec();

   delete hyperlink;
   delete projectauthors;
   delete projectname;
   delete templayout;
   delete aboutdialog;
}

//------------------------------------------------------------------------------------

void MainWindow::show_help()
{
    if (!QDesktopServices::openUrl(QUrl("file:///" + QDir::currentPath() + "/QVideoProcessing.txt", QUrl::TolerantMode))) // runs the ShellExecute function on Windows
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open help file"), QMessageBox::Ok, this, Qt::Dialog);
        msgBox.exec();
    }
}

//------------------------------------------------------------------------------------

MainWindow::~MainWindow()
{
    if(m_saveFile.isOpen())
    {
        m_saveFile.close();
    }

    pt_videoCapture->close();

    pt_improcThread->quit();
    pt_improcThread->wait();

    if(pt_harmonicThread)
    {
        pt_harmonicThread->quit();
        pt_harmonicThread->wait();
    }
}

//------------------------------------------------------------------------------------

void MainWindow::onpause()
{
    pt_videoCapture->pause();
    m_timer.stop();
}

//------------------------------------------------------------------------------------

void MainWindow::onresume()
{
    pt_videoCapture->resume();
    m_timer.start();
    pt_opencvProcessor->updateClock();
}

//-----------------------------------------------------------------------------------

void MainWindow::opendeviceresolutiondialog()
{
    onpause();
    if( !pt_videoCapture->open_resolutionDialog() )
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open device resolution dialog!"), QMessageBox::Ok, this, Qt::Dialog);
        msgBox.exec();
    }
    onresume();
}

//-----------------------------------------------------------------------------------

void MainWindow::opendevicesettingsdialog()
{
    if( !pt_videoCapture->open_settingsDialog() )
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open device settings dialog!"), QMessageBox::Ok, this, Qt::Dialog);
        msgBox.exec();
    }
}

//-----------------------------------------------------------------------------------

void MainWindow::callDirectShowSdialog()
{   
    if (!QProcess::startDetached(QString("WVCF_utility.exe"),QStringList("-l")))// runs the ShellExecute function on Windows
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open utility!"), QMessageBox::Ok, this, Qt::Dialog);
        msgBox.exec();
    } 
}

//----------------------------------------------------------------------------------------

void MainWindow::configure_and_start_session()
{
    this->onpause();
    QSettingsDialog dialog;
    if(dialog.exec() == QDialog::Accepted)
    {     
        if(pt_harmonicProcessor)
        {
            pt_harmonicThread->quit();
            pt_harmonicThread->wait();
        }
        //------------------Close all opened plot dialogs---------------------
        closeAllDialogs();
        //---------------------Harmonic processor------------------------
        pt_harmonicThread = new QThread(this);
        pt_harmonicProcessor = new QHarmonicProcessor(NULL, dialog.get_datalength(), dialog.get_bufferlength());
        pt_harmonicProcessor->moveToThread(pt_harmonicThread);
        connect(pt_harmonicThread, SIGNAL(finished()),pt_harmonicProcessor, SLOT(deleteLater()));
        connect(pt_harmonicThread, SIGNAL(finished()),pt_harmonicThread, SLOT(deleteLater()));
        connect(pt_opencvProcessor, SIGNAL(colorsEvaluated(ulong,ulong,ulong,ulong,double)), pt_harmonicProcessor, SLOT(WriteToDataOneColor(ulong,ulong,ulong,ulong,double)));
        connect(pt_harmonicProcessor, SIGNAL(TooNoisy(qreal)), pt_display, SLOT(clearFrequencyString(qreal)));
        connect(pt_harmonicProcessor, SIGNAL(HRfrequencyWasUpdated(qreal,qreal,bool)), pt_display, SLOT(updateValues(qreal,qreal,bool)));
        //---------------------------------------------------------------
        if(m_saveFile.isOpen())
        {
            m_saveFile.close();
            pt_recordAct->setChecked(false);
        }
        //---------------------------------------------------------------
        if(dialog.get_customPatientFlag())
        {
            if(pt_harmonicProcessor->loadThresholds(dialog.get_stringDistribution().toLocal8Bit().constData(),(QHarmonicProcessor::SexID)dialog.get_patientSex(),dialog.get_patientAge(),(QHarmonicProcessor::TwoSideAlpha)dialog.get_patientPercentile()) == QHarmonicProcessor::FileExistanceError)
            {
                QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("Can not open population distribution file"), QMessageBox::Ok, this, Qt::Dialog);
                msgBox.exec();
            }
        }      
        //--------------------------------------------------------------
        if(dialog.get_FFTflag())
        {
            connect(&m_timer, SIGNAL(timeout()), pt_harmonicProcessor, SLOT(ComputeFrequency()));
        }
        else
        {
            connect(&m_timer, SIGNAL(timeout()), pt_harmonicProcessor, SLOT(CountFrequency()));
        }
        //--------------------------------------------------------------
        pt_harmonicThread->start();
        pt_optionsMenu->setEnabled(true);

        if(dialog.getSourceFlag())
        {
            this->opendevice();
        }
        else
        {
            this->openvideofile();
        }
        m_timer.setInterval( dialog.get_timerValue() );
        this->statusBar()->showMessage(tr("Plot options available through Menu->Options->New plot"));
    }
    this->onresume();
}

//------------------------------------------------------------------------------------------

void MainWindow::createPlotDialog()
{
    if(m_dialogSetCounter < LIMIT_OF_DIALOGS_NUMBER)
    {
        QDialog dialog;
        QVBoxLayout centralLayout;
        dialog.setLayout(&centralLayout);
        QGroupBox groupBox(tr("Select appropriate plot type:"));
        QHBoxLayout buttonsLayout;
        centralLayout.addWidget(&groupBox);
        centralLayout.addLayout(&buttonsLayout);
        QPushButton acceptButton("Accept");
        QPushButton rejectButton("Cancel");
        buttonsLayout.addWidget(&acceptButton);
        buttonsLayout.addWidget(&rejectButton);
        connect(&acceptButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(&rejectButton, &QPushButton::clicked, &dialog, &QDialog::reject);

        QVBoxLayout groupBoxLayout;
        groupBox.setLayout(&groupBoxLayout);
        QComboBox dialogTypeComboBox;
        groupBoxLayout.addWidget(&dialogTypeComboBox);
        for(quint8 i = 0; i < sizeof(QPlotDialogName)/sizeof(char*); i++)
        {
            dialogTypeComboBox.addItem( QPlotDialogName[i] );
        }
        dialogTypeComboBox.setCurrentIndex(0);

        dialog.setWindowTitle("Plot select dialog");
        dialog.setMinimumSize(256,128);

        if(dialog.exec() == QDialog::Accepted)
        {
            pt_dialogSet[ m_dialogSetCounter ] = new QDialog(NULL, Qt::Window);
            pt_dialogSet[ m_dialogSetCounter ]->setWindowTitle(dialogTypeComboBox.currentText() + " plot");
            pt_dialogSet[ m_dialogSetCounter ]->setAttribute(Qt::WA_DeleteOnClose, true);
            connect(pt_dialogSet[ m_dialogSetCounter ], SIGNAL(destroyed()), this, SLOT(decrease_dialogSetCounter()));
            pt_dialogSet[ m_dialogSetCounter ]->setMinimumSize(FRAME_WIDTH, FRAME_HEIGHT);

            QVBoxLayout *pt_layout = new QVBoxLayout( pt_dialogSet[ m_dialogSetCounter ] );
            pt_layout->setMargin(FRAME_MARGIN);
            QEasyPlot *pt_plot = new QEasyPlot( pt_dialogSet[ m_dialogSetCounter ] );
            pt_layout->addWidget( pt_plot );          
                switch(dialogTypeComboBox.currentIndex())
                {
                    case 0: // Signal trace
                        connect(pt_harmonicProcessor, SIGNAL(CNSignalWasUpdated(const qreal*,quint16)), pt_plot, SLOT(set_externalArray(const qreal*,quint16)));
                        pt_plot->set_axis_names("Frame","Centered & normalized signal");
                        pt_plot->set_vertical_Borders(-5.0,5.0);
                        pt_plot->set_coordinatesPrecision(0,2);
                        break;
                    case 1: // Spectrum trace
                        connect(pt_harmonicProcessor, SIGNAL(SpectrumWasUpdated(const qreal*,quint16)), pt_plot, SLOT(set_externalArray(const qreal*,quint16)));
                        pt_plot->set_axis_names("Freq.count","DFT amplitude spectrum");
                        pt_plot->set_vertical_Borders(0.0,25000.0);
                        pt_plot->set_coordinatesPrecision(0,1);
                        pt_plot->set_DrawRegime(QEasyPlot::FilledTraceRegime);
                        break;
                    case 2: // Time trace
                        connect(pt_harmonicProcessor, SIGNAL(ptTimeWasUpdated(const qreal*,quint16)), pt_plot, SLOT(set_externalArray(const qreal*,quint16)));
                        pt_plot->set_axis_names("Frame","processing period per frame, ms");
                        pt_plot->set_vertical_Borders(0.0,100.0);
                        pt_plot->set_coordinatesPrecision(0,2);
                        pt_plot->set_DrawRegime(QEasyPlot::FilledTraceRegime);
                        break;
                    case 3: // Digital filter output
                        connect(pt_harmonicProcessor, SIGNAL(pt_YoutputWasUpdated(const qreal*,quint16)), pt_plot, SLOT(set_externalArray(const qreal*,quint16)));
                        pt_plot->set_axis_names("Frame","Digital derivative after smoothing");
                        pt_plot->set_vertical_Borders(-2.0,2.0);
                        pt_plot->set_coordinatesPrecision(0,2);
                    break;
                    case 4: // signal phase shift
                        connect(pt_harmonicProcessor, SIGNAL(CNSignalWasUpdated(const qreal*,quint16)), pt_plot, SLOT(set_externalArray(const qreal*,quint16)));
                        pt_plot->set_DrawRegime(QEasyPlot::PhaseRegime);
                        pt_plot->set_axis_names("Signal count","Signal count");
                        pt_plot->set_vertical_Borders(-5.0,5.0);
                        pt_plot->set_horizontal_Borders(-5.0, 5.0);
                        pt_plot->set_X_Ticks(11);
                        pt_plot->set_coordinatesPrecision(2,2);
                    break;
                }
            pt_dialogSet[ m_dialogSetCounter ]->setContextMenuPolicy(Qt::ActionsContextMenu);
            QAction *pt_actionFont = new QAction(tr("Axis font"), pt_dialogSet[ m_dialogSetCounter ]);
            connect(pt_actionFont, SIGNAL(triggered()), pt_plot, SLOT(open_fontSelectDialog()));
            QAction *pt_actionTraceColor = new QAction(tr("Trace color"), pt_dialogSet[ m_dialogSetCounter ]);
            connect(pt_actionTraceColor, SIGNAL(triggered()), pt_plot, SLOT(open_traceColorDialog()));
            QAction *pt_actionBGColor = new QAction(tr("BG color"), pt_dialogSet[ m_dialogSetCounter ]);
            connect(pt_actionBGColor, SIGNAL(triggered()), pt_plot, SLOT(open_backgroundColorDialog()));
            QAction *pt_actionCSColor = new QAction(tr("CS color"), pt_dialogSet[ m_dialogSetCounter ]);
            connect(pt_actionCSColor, SIGNAL(triggered()), pt_plot, SLOT(open_coordinatesystemColorDialog()));
            pt_dialogSet[ m_dialogSetCounter ]->addAction(pt_actionTraceColor);
            pt_dialogSet[ m_dialogSetCounter ]->addAction(pt_actionBGColor);
            pt_dialogSet[ m_dialogSetCounter ]->addAction(pt_actionCSColor);
            pt_dialogSet[ m_dialogSetCounter ]->addAction(pt_actionFont);
            pt_dialogSet[ m_dialogSetCounter ]->show();
            m_dialogSetCounter++;
        }
    }
    else
    {
        QMessageBox msgBox(QMessageBox::Information, this->windowTitle(), tr("You came up to a limit of dialogs ailable"), QMessageBox::Ok, this, Qt::Dialog);
        msgBox.exec();
    }

}

//------------------------------------------------------------------------------------------

void MainWindow::decrease_dialogSetCounter()
{
    m_dialogSetCounter--;
}

//-------------------------------------------------------------------------------------------

void MainWindow::make_record_to_file(qreal signalValue, qreal meanRed, qreal meanGreen, qreal meanBlue, qreal freqValue, qreal snrValue)
{
    if(m_saveFile.isOpen())
    {
        m_textStream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz")
                     << "\t" << signalValue << "\t" << meanRed << "\t" << meanGreen
                     << "\t" << meanBlue << "\t" << freqValue << "\t" << snrValue << "\n";
    }
}

//-------------------------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent*)
{
    closeAllDialogs();
}

//-------------------------------------------------------------------------------------------

void MainWindow::closeAllDialogs()
{
    for(qint8 i = m_dialogSetCounter; i > 0; i--)
    {
        pt_dialogSet[ i-1 ]->close(); // there is no need to explicitly decrement m_dialogSetCounter value because pt_dialogSet[i] was preset to Qt::WA_DeleteOnClose flag and on_destroy of pt_dialogSet[i] 'this' will decrease counter automatically
    };
}

//--------------------------------------------------------------------------------------------

void MainWindow::saveRecord()
{
    if(m_saveFile.isOpen())
    {
        m_saveFile.close();
    }
    m_saveFile.setFileName(QFileDialog::getSaveFileName(this, tr("Save file"), "/records", "Text file (*.txt)"));
    if(m_saveFile.open(QIODevice::WriteOnly))
    {
        pt_recordAct->setChecked(true);
        m_textStream.setDevice(&m_saveFile);
        m_textStream << "dd.MM.yyyy hh:mm:ss.zzz\tCNSignal\tMeanRed\tMeanGreen\tMeanBlue\tPulseRate,bpm\tSNR,dB\n";
        connect(pt_harmonicProcessor, SIGNAL(SignalActualValues(qreal,qreal,qreal,qreal,qreal,qreal)), this, SLOT(make_record_to_file(qreal,qreal,qreal,qreal,qreal,qreal)));
    }
    else
    {
        pt_recordAct->setChecked(false);
    }
}

//--------------------------------------------------------------------------------------------

void MainWindow::changeColor(int value)
{
    if(pt_harmonicProcessor)
        pt_harmonicProcessor->switch_to_channel((QHarmonicProcessor::colorChannel)value);
}


