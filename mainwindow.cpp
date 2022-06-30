/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  mainwindow.cpp
**
** Notes:
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QComboBox>
#include <QString>
#include <QDesktopServices>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Main window constructor
//=============================================================================
MainWindow::MainWindow(
    QWidget *parent
    ) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //Constructor
    ui->setupUi(this);

    //Add version to title and about tab
    this->setWindowTitle(QString(this->windowTitle()).append(" ").append(APP_VERSION));
    ui->lbl_CurrentVersion->setText(ui->lbl_CurrentVersion->text().append(APP_VERSION));

    //Create settings
    pSettingsHandle = new LrdSettings();
    MallocFailCheck(pSettingsHandle);
    pSettingsHandle->SetConfigDefaults();

    //Create firmware update object
    pFwUpd = new LrdFwUpd();
    MallocFailCheck(pFwUpd);
    pFwUpd->SetSettingsObject(pSettingsHandle);

    //Initialise popup message
    gpmErrorForm = new LrdPopupMessage(this);
    MallocFailCheck(gpmErrorForm);

    //Setup signals
    connect(pFwUpd, SIGNAL(CurrentAction(uint32_t,uint32_t,QString)), this, SLOT(CurrentAction(uint32_t,uint32_t,QString)));
    connect(pFwUpd, SIGNAL(PercentComplete(int8_t,int8_t)), this, SLOT(ProgressUpdate(int8_t,int8_t)));
    connect(pFwUpd, SIGNAL(FirmwareUpdateActive(bool)), this, SLOT(StartStopUpgrade(bool)));
    connect(pFwUpd, SIGNAL(Error(uint32_t,int32_t)), this, SLOT(ModuleError(uint32_t,int32_t)));
    connect(pFwUpd, SIGNAL(Finished(bool,qint64)), this, SLOT(UpgradeFinished(bool,qint64)));
#ifdef __linux__
    connect(pFwUpd, SIGNAL(SerialPortNameChanged(QString*)), this, SLOT(SerialPortNameChanged(QString*)));
#endif

    //Create error object
    pErrHandler = new LrdErr();
    MallocFailCheck(pErrHandler);

    //Set default error code to none
    nErrorCode = EXIT_CODE_SUCCESS;

    //Disable verbose messages by default
    nVerbosity = VERBOSITY_NONE;

    //Populate list of serial devices
    RefreshSerialDevices();

    //Accept file drops
    setAcceptDrops(true);

    //Set button icon to refresh symbol
    ui->btn_Refresh->setIcon(this->style()->standardIcon(QStyle::SP_BrowserReload));

    //Remove conditional objects
#ifdef SKIPUPDATECHECK
    ui->btn_UpdateCheck->deleteLater();
    ui->lbl_LatestVersion->deleteLater();
    ui->lbl_CurrentVersion->setAlignment(Qt::AlignCenter);
    ui->check_Enable_SSL->deleteLater();
#else
#ifndef UseSSL
    //Non-SSL build, disable SSL checkbox
    ui->check_Enable_SSL->setChecked(false);
    ui->check_Enable_SSL->setEnabled(false);
#endif
#endif

    //Check command line
    QStringList slArgs = QCoreApplication::arguments();
    unsigned char chi = 1;
    bool bArgAutomode = false;
    bArgAutoexit = false;
    while (chi < slArgs.length())
    {
        if (slArgs[chi].toUpper() == strOptionAutoMode)
        {
            //Automatically run
            bArgAutomode = true;
        }
        else if (slArgs[chi].toUpper() == strOptionAutoExit)
        {
            //Automatically exit
            bArgAutoexit = true;
        }
        else if ((slArgs[chi].length() > (strOptionCom.length() + strOptionSeperateCharacter.length()) &&
                  slArgs[chi].left(strOptionCom.length()).toUpper() == strOptionCom &&
                  slArgs[chi].mid(strOptionCom.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)||
                 (slArgs[chi].length() > (strOptionPort.length() + strOptionSeperateCharacter.length()) &&
                  slArgs[chi].left(strOptionPort.length()).toUpper() == strOptionPort))
        {
            //Serial port
            ui->combo_COM->setCurrentText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
        }
        else if (slArgs[chi].length() > (strOptionUwf.length() + strOptionSeperateCharacter.length()) &&
                 (slArgs[chi].left(strOptionUwf.length()).toUpper() == strOptionUwf ||
                  slArgs[chi].left(strOptionUbu.length()).toUpper() == strOptionUbu) &&
                 slArgs[chi].mid(strOptionUwf.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //UWF file
            ui->edit_Filename->setText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
        }
        else if (slArgs[chi].length() > (strOptionKey.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionKey.length()).toUpper() == strOptionKey &&
                 slArgs[chi].mid(strOptionKey.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Unlock key
            ui->edit_Key->setText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
        }
        else if (slArgs[chi].length() > (strOptionApplicationBaud.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionApplicationBaud.length()).toUpper() == strOptionApplicationBaud &&
                 slArgs[chi].mid(strOptionApplicationBaud.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Module start up baud rate
            ui->combo_InitialApplicationBaud->setCurrentText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
        }
        else if (slArgs[chi].length() > (strOptionBootloaderBaud.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionBootloaderBaud.length()).toUpper() == strOptionBootloaderBaud &&
                 slArgs[chi].mid(strOptionBootloaderBaud.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Module start up baud rate
            ui->combo_InitialBootloaderBaud->setCurrentText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
        }
        else if (slArgs[chi].length() > (strOptionMaxBaud.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionMaxBaud.length()).toUpper() == strOptionMaxBaud &&
                 slArgs[chi].mid(strOptionMaxBaud.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Maximum UART baud rate during firmware upgrade
            ui->combo_MaxBaud->setCurrentText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
            ui->check_MaximumBaud->setEnabled(true);
            ui->check_MaximumBaud->setChecked(true);
        }
        else if (slArgs[chi].length() > (strOptionExactBaud.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionExactBaud.length()).toUpper() == strOptionExactBaud &&
                 slArgs[chi].mid(strOptionExactBaud.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Specify exact UART baud rate during firmware upgrade
            ui->combo_ExactBaud->setCurrentText(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()));
            ui->check_ExactBaud->setEnabled(true);
            ui->check_ExactBaud->setChecked(true);
        }
        else if (slArgs[chi].toUpper() == strOptionDisableEnhanced)
        {
            //Disable enhanced bootloader functionality
            ui->check_DisableEnhanced->setChecked(true);
        }
        else if (slArgs[chi].length() > (strOptionReboot.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionReboot.length()).toUpper() == strOptionReboot &&
                 slArgs[chi].mid(strOptionReboot.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Reboot module after upgrade
            ui->check_Restart->setChecked((slArgs[chi][slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()] == '0' ? false : true));
        }
        else if (slArgs[chi].length() > (strOptionVerify.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionVerify.length()).toUpper() == strOptionVerify &&
                 slArgs[chi].mid(strOptionVerify.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Verify written data
            ui->check_Verify_Data->setChecked((slArgs[chi][slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()] == '0' ? false : true));
        }
        else if (slArgs[chi].length() > (strOptionEntrance.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionEntrance.length()).toUpper() == strOptionEntrance &&
                 slArgs[chi].mid(strOptionEntrance.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //Bootloader entrance method
            ui->combo_Bootloader_Enter_Method->setCurrentIndex(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()).toUInt());
        }
        else if (slArgs[chi].length() > (strOptionUARTBREAK.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionUARTBREAK.length()).toUpper() == strOptionUARTBREAK &&
                 slArgs[chi].mid(strOptionUARTBREAK.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //UART BREAK prior to upgrade
            ui->check_BREAK->setChecked((slArgs[chi][slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()] == '0' ? false : true));
        }
        else if (slArgs[chi].length() > (strOptionDTS.length() + strOptionSeperateCharacter.length()) &&
                 slArgs[chi].left(strOptionDTS.length()).toUpper() == strOptionDTS &&
                 slArgs[chi].mid(strOptionDTS.length(), strOptionSeperateCharacter.length()).toUpper() == strOptionSeperateCharacter)
        {
            //DTS assertion prior to upgrade
            ui->combo_BREAK_DTR->setCurrentIndex(slArgs[chi].mid(slArgs[chi].indexOf(strOptionSeperateCharacter)+strOptionSeperateCharacter.length()).toUInt());
        }
        else if (slArgs[chi].toUpper() == strOptionNoPrompts)
        {
            //Disable all prompts
            ui->check_Bootloader_Enter_Warning_Disable->setChecked(true);
            ui->check_Bootloader_Enter_Error_Disable->setChecked(true);
        }
        ++chi;
    }

    //Update GUI elements
    on_combo_Bootloader_Enter_Method_currentIndexChanged(0);
    on_check_FTDI_Override_ID_stateChanged(0);
    on_check_ExactBaud_stateChanged(0);
    on_check_MaximumBaud_stateChanged(0);

    if (bArgAutomode == true)
    {
        //Start upgrade process
        on_btn_Go_clicked();
    }
}

//=============================================================================
// Main window destructor
//=============================================================================
MainWindow::~MainWindow(
    )
{
    //Clear signals
    disconnect(this, SLOT(CurrentAction(uint32_t,uint32_t,QString)));
    disconnect(this, SLOT(ProgressUpdate(int8_t,int8_t)));
    disconnect(this, SLOT(StartStopUpgrade(bool)));
    disconnect(this, SLOT(ModuleError(uint32_t,int32_t)));
    disconnect(this, SLOT(UpgradeFinished(bool,qint64)));
    disconnect(this, SLOT(DelayedExit()));
#ifdef __linux__
    disconnect(this, SLOT(SerialPortNameChanged(QString*)));
#endif

    //Delete objects
    delete pFwUpd;
    delete pSettingsHandle;
    delete pErrHandler;

#ifndef SKIPUPDATECHECK
    if (pAppUpdate != NULL)
    {
        disconnect(this, SLOT(ApplicationUpdateResponse(VERSION_RESPONSES,QString)));
        delete pAppUpdate;
    }
#endif

    if (tmrExitApplicationTimer != NULL)
    {
        delete tmrExitApplicationTimer;
    }

    //Delete UI
    delete ui;
}

//=============================================================================
// Upgrade file selection button clicked
//=============================================================================
void
MainWindow::on_btn_FileSelector_clicked(
    )
{
    //File selection button clicked
    QString strFilename = QFileDialog::getOpenFileName(this, "Open uwf/ubu firmware upgrade file", "", "Uwf/Ubu Files (*.uwf *.ubu);;All Files (*)");
    if (!strFilename.isEmpty())
    {
        //Update filename
        ui->edit_Filename->setText(strFilename);
    }
}

//=============================================================================
// Refresh list of serial devices
//=============================================================================
void
MainWindow::RefreshSerialDevices(
    )
{
    //Clears and refreshes the list of serial devices
    QString strPrev = "";
    QRegularExpression reTempRE("^(\\D*?)(\\d+)$");
    QList<int> lstEntries;
    lstEntries.clear();

    if (ui->combo_COM->count() > 0)
    {
        //Remember previous option
        strPrev = ui->combo_COM->currentText();
    }
    ui->combo_COM->clear();
    LrdFwUART *pTmpDev = NULL;
    pTmpDev = new LrdFwUART();
    MallocFailCheck(pTmpDev);

    //Get list of devices
    QList<QVariant> lstDevices = pTmpDev->GetDevices();
    delete pTmpDev;

    //Add them to the list
    uint8_t l = 0;
    while (l < lstDevices.count())
    {
        QRegularExpressionMatch remTempREM = reTempRE.match(lstDevices[l].toString());
        if (remTempREM.hasMatch() == true)
        {
            //Can sort this item
            int i = lstEntries.count()-1;
            while (i >= 0)
            {
                if (remTempREM.captured(2).toInt() > lstEntries[i])
                {
                    //Found correct order position, add here
                    ui->combo_COM->insertItem(i+1, lstDevices[l].toString());
                    lstEntries.insert(i+1, remTempREM.captured(2).toInt());
                    i = -1;
                }
                --i;
            }

            if (i == -1)
            {
                //Position not found, add to beginning
                ui->combo_COM->insertItem(0, lstDevices[l].toString());
                lstEntries.insert(0, remTempREM.captured(2).toInt());
            }
        }
        else
        {
            //Cannot sort this item
            ui->combo_COM->insertItem(ui->combo_COM->count(), lstDevices[l].toString());
        }
        ++l;
    }

    //Search for previous item if one was selected
    if (strPrev == "")
    {
        //Select first item
        ui->combo_COM->setCurrentIndex(0);
    }
    else
    {
        //Search for previous
        int i = 0;
        while (i < ui->combo_COM->count())
        {
            if (ui->combo_COM->itemText(i) == strPrev)
            {
                //Found previous item
                ui->combo_COM->setCurrentIndex(i);
                break;
            }
            ++i;
        }
    }

    //Update serial port info
    on_combo_COM_currentIndexChanged(0);
}

//=============================================================================
// Callback when the 'Refresh' button has been clicked
//=============================================================================
void
MainWindow::on_btn_Refresh_clicked(
    )
{
    RefreshSerialDevices();
}

//=============================================================================
// Callback when the 'Go' button has been clicked
//=============================================================================
void
MainWindow::on_btn_Go_clicked(
    )
{
    StartStopUpgrade(true);
    ui->statusBar->clearMessage();

    //Load all the settings in
    pSettingsHandle->SetConfigOption(FIRMWARE_FILE, ui->edit_Filename->text());
    pSettingsHandle->SetConfigOption(OUTPUT_DEVICE, ui->combo_COM->currentText());
    pSettingsHandle->SetConfigOption(APPLICATION_BAUD, ui->combo_InitialApplicationBaud->currentText().toUInt());
    pSettingsHandle->SetConfigOption(BOOTLOADER_BAUD, ui->combo_InitialBootloaderBaud->currentText().toUInt());
    pSettingsHandle->SetConfigOption(BOOTLOADER_ENTER_METHOD, ui->combo_Bootloader_Enter_Method->currentIndex());
    pSettingsHandle->SetConfigOption(MAX_BAUD, (ui->check_MaximumBaud->isChecked() ? ui->combo_MaxBaud->currentText().toUInt() : (quint32)0));
    pSettingsHandle->SetConfigOption(EXACT_BAUD, (ui->check_ExactBaud->isChecked() ? ui->combo_ExactBaud->currentText().toUInt() : (quint32)0));
    pSettingsHandle->SetConfigOption(REBOOT_MODULE_BEFORE_UPDATE, ui->check_BREAK->isChecked());
    pSettingsHandle->SetConfigOption(REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS, (bool)ui->combo_BREAK_DTR->currentIndex());
    pSettingsHandle->SetConfigOption(REBOOT_MODULE_AFTER_UPDATE, ui->check_Restart->isChecked());
    pSettingsHandle->SetConfigOption(VERIFY_DATA, ui->check_Verify_Data->isChecked());
    pSettingsHandle->SetConfigOption(BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE, ui->check_DisableEnhanced->isChecked());
    pSettingsHandle->SetConfigOption(UNLOCK_KEY, ui->edit_Key->text().toUtf8());
    pSettingsHandle->SetConfigOption(BOOTLOADER_ENTRANCE_WARNINGS_DISABLED, ui->check_Bootloader_Enter_Warning_Disable->isChecked());
    pSettingsHandle->SetConfigOption(BOOTLOADER_ENTRANCE_ERRORS_DISABLED, ui->check_Bootloader_Enter_Error_Disable->isChecked());
    pSettingsHandle->SetConfigOption(VALIDATE_UWF, ui->check_Upgrade_File_Validity->isChecked());

    //Disable verbosity
    pSettingsHandle->SetConfigOption(UART_VERBOSITY, (quint8)0);
    pSettingsHandle->SetConfigOption(UPDATE_VERBOSITY, (quint8)0);
    pSettingsHandle->SetConfigOption(UWF_VERBOSITY, (quint8)0);
    pSettingsHandle->SetConfigOption(SETTINGS_VERBOSITY, (quint8)0);

    //Change status text
    ui->label_UpgradeStatus->setText("Updating...");

    //Send the command to enter the bootloader
    if (pFwUpd->StartUpdate() == false)
    {
        //Firmware update failed to start
        StartStopUpgrade(false);
    }
}

//=============================================================================
// Callback when the currently selected serial port has been changed
//=============================================================================
void
MainWindow::on_combo_COM_currentIndexChanged(
    int
    )
{
    //Serial port selection has been changed, update text
    if (ui->combo_COM->currentText().length() > 0)
    {
        LrdFwUART *pTmpDev = NULL;
        pTmpDev = new LrdFwUART();
        MallocFailCheck(pTmpDev);
        ui->statusBar->showMessage(pTmpDev->GetDetails(ui->combo_COM->currentText()));
        delete pTmpDev;
    }
    else
    {
        //Clear text as no port is selected
        ui->statusBar->clearMessage();
    }
}

//=============================================================================
// Enables or disables the interactive UI elements
//=============================================================================
void
MainWindow::StartStopUpgrade(
    bool bStart
    )
{
    //Set status of UI elements
    ui->edit_Filename->setDisabled(bStart);
    ui->btn_FileSelector->setDisabled(bStart);
    ui->combo_COM->setDisabled(bStart);
    ui->btn_Refresh->setDisabled(bStart);
    ui->btn_Go->setDisabled(bStart);
    ui->edit_Key->setDisabled(bStart);
    ui->check_DisableEnhanced->setDisabled(bStart);
    ui->combo_InitialApplicationBaud->setDisabled(bStart);
    ui->combo_InitialBootloaderBaud->setDisabled(bStart);
    ui->check_MaximumBaud->setDisabled(bStart);
    ui->check_ExactBaud->setDisabled(bStart);
    ui->check_DisableEnhanced->setDisabled(bStart);
    ui->check_Restart->setDisabled(bStart);
    ui->check_Verify_Data->setDisabled(bStart);
    ui->combo_Bootloader_Enter_Method->setDisabled(bStart);
    ui->check_Bootloader_Enter_Warning_Disable->setDisabled(bStart);
    ui->check_Bootloader_Enter_Error_Disable->setDisabled(bStart);
    ui->check_FTDI_Override_ID->setDisabled(bStart);
    ui->check_Upgrade_File_Validity->setDisabled(bStart);
    ui->check_BREAK->setDisabled(bStart);
    ui->combo_BREAK_DTR->setDisabled(bStart);
#ifndef SKIPUPDATECHECK
    ui->btn_UpdateCheck->setDisabled(bStart);
#endif
    if (ui->check_MaximumBaud->isChecked())
    {
        ui->combo_MaxBaud->setDisabled(bStart);
    }
    if (ui->check_ExactBaud->isChecked())
    {
        ui->combo_ExactBaud->setDisabled(bStart);
    }
    if (ui->check_FTDI_Override_ID->isChecked())
    {
        ui->edit_FTDI_ID->setDisabled(bStart);
    }

    //Set if file drops are allowed
    setAcceptDrops(!bStart);

    if (bStart == true)
    {
        //Reset progress bars
        ui->text_Log->clear();
        ui->progress_Current->setValue(0);
        ui->progress_Overall->setValue(0);
    }
    else
    {
        if (bArgAutoexit == true && nErrorCode != EXIT_CODE_SUCCESS)
        {
            //Exit application
            QApplication::exit(nErrorCode);

            //This exists in case the error happens too fast after starting the application in which case the above exit statement will not work
            tmrExitApplicationTimer = new QTimer();
            connect(tmrExitApplicationTimer, SIGNAL(timeout()), this, SLOT(DelayedExit()));
            tmrExitApplicationTimer->start(DELAYED_EXIT_TIME_MS);
        }
        else
        {
            //Run events which trigger if some form elements should be active
            on_check_MaximumBaud_stateChanged(0);
            on_check_ExactBaud_stateChanged(0);
            on_check_FTDI_Override_ID_stateChanged(0);
            on_combo_Bootloader_Enter_Method_currentIndexChanged(0);
            on_check_BREAK_stateChanged(0);
        }
    }
}

//=============================================================================
// Callback when a file has been dropped onto the window
//=============================================================================
void
MainWindow::DroppedFile(
    QString strFilename
    )
{
    if (!pFwUpd->IsUpdateInProgress() && (strFilename.right(4).toLower() == ".uwf" || strFilename.right(4).toLower() == ".ubu"))
    {
        //Firmware upgrade file, update the filename
        ui->edit_Filename->setText(strFilename);
    }
}

//=============================================================================
// Callback whilst a file is being dragged over the window
//=============================================================================
void
MainWindow::dragEnterEvent(
    QDragEnterEvent *dragEvent
    )
{
    //A file is being dragged onto the window
    if (dragEvent->mimeData()->urls().count() == 1 && !pFwUpd->IsUpdateInProgress() && ui->edit_Filename->isEnabled())
    {
        //Nothing is running, serial handle is open and a single file is being dragged - accept action
        dragEvent->acceptProposedAction();
    }
    else
    {
        //Terminal is busy, serial port is closed or more than 1 file was dropped
        dragEvent->ignore();
    }
}

//=============================================================================
// Callback when a file has been dropped onto the window
//=============================================================================
void
MainWindow::dropEvent(
    QDropEvent *dropEvent
    )
{
    //A file has been dragged onto the window - send this file if possible
    QList<QUrl> lstURLs = dropEvent->mimeData()->urls();
    if (lstURLs.isEmpty())
    {
        //No files
        return;
    }
    else if (lstURLs.length() > 1)
    {
        //More than 1 file - ignore
        return;
    }

    QString strFileName = lstURLs.first().toLocalFile();
    if (strFileName.isEmpty())
    {
        //Invalid filename
        return;
    }

    //Pass to other function call
    DroppedFile(strFileName);
}

//=============================================================================
// Callback when the maximum baud rate enable checkbox status is changed
//=============================================================================
void
MainWindow::on_check_MaximumBaud_stateChanged(
    int
    )
{
    ui->combo_MaxBaud->setEnabled(ui->check_MaximumBaud->isChecked());
    ui->check_ExactBaud->setEnabled(!ui->check_MaximumBaud->isChecked());
}

//=============================================================================
// Callback when the exact baud rate enable checkbox status is changed
//=============================================================================
void
MainWindow::on_check_ExactBaud_stateChanged(
    int
    )
{
    ui->combo_ExactBaud->setEnabled(ui->check_ExactBaud->isChecked());
    ui->check_MaximumBaud->setEnabled(!ui->check_ExactBaud->isChecked());
}

//=============================================================================
// Function for exiting application after a delay
//=============================================================================
void
MainWindow::DelayedExit(
    )
{
    QApplication::exit(nErrorCode);
}

//=============================================================================
// Slot for current action updates
//=============================================================================
void
MainWindow::CurrentAction(
    uint32_t,
    uint32_t,
    QString strActionName
    )
{
    //Append to log view
    ui->text_Log->appendPlainText(strActionName);
}

//=============================================================================
// Slot for progress updates
//=============================================================================
void
MainWindow::ProgressUpdate(
    int8_t nTaskPercent,
    int8_t nOverallPercent
    )
{
    if (nTaskPercent != -1)
    {
        //Update current progress percent
        ui->progress_Current->setValue(nTaskPercent);
    }

    if (nOverallPercent != -1)
    {
        //Update overall progress percent
        ui->progress_Overall->setValue(nOverallPercent);
    }
}

//=============================================================================
// Slot for error handler
//=============================================================================
void
MainWindow::ModuleError(
    uint32_t,
    int32_t nErrorCode
    )
{
    this->nErrorCode = nErrorCode;
    ui->label_UpgradeStatus->setText(QString("Failed. ").append(pErrHandler->ErrorCodeToString(nErrorCode, true)));
    ui->text_Log->appendPlainText(ui->label_UpgradeStatus->text());
}

//=============================================================================
// Slot for upgrade finished
//=============================================================================
void
MainWindow::UpgradeFinished(
    bool bSuccess,
    qint64 nUpgradeTimeMS
    )
{
    if (bSuccess == true)
    {
        ui->label_UpgradeStatus->setText(QString("Completed successfully in ").append(QString::number(nUpgradeTimeMS)).append("ms!"));
    }
    else
    {
        ui->label_UpgradeStatus->setText(QString("Failed with error code ").append(QString::number(nErrorCode)).append(" (").append(pErrHandler->ErrorCodeToString(nErrorCode, false)).append(") after ").append(QString::number(nUpgradeTimeMS)).append("ms!"));
    }

    if (bArgAutoexit == true)
    {
        //Exit application
        QApplication::exit((bSuccess == true ? EXIT_CODE_SUCCESS : nErrorCode));
    }
}

//=============================================================================
// When tab selection is changed
//=============================================================================
void
MainWindow::on_tab_Selector_currentChanged(
    int
    )
{
    if (ui->tab_Selector->currentIndex() == ui->tab_Selector->indexOf(ui->tab_About) && ui->edit_Licenses->toPlainText().isEmpty())
    {
        //Load licenses text on demand to save RAM
        ui->edit_Licenses->setPlainText(QString(
            "UwFlashX uses the Qt framework version 5, which is licensed under the GPLv3 (not including later versions).\n"
            "UwFlashX uses and may be linked statically to various other libraries including Xau, XCB, expat, fontconfig, zlib, bz2, harfbuzz, freetype, udev, dbus, icu, unicode, UPX, OpenSSL, libftdi, libusb, FTDI D2XX. The licenses for these libraries are provided below:\n\n\n\n"
            "\n"
            "Lib Xau:\n"
            "Copyright 1988, 1993, 1994, 1998 The Open Group\n"
            "Permission to use, copy, modify, distribute, and sell this software and its\n"
            "documentation for any purpose is hereby granted without fee, provided that\n"
            "the above copyright notice appear in all copies and that both that\n"
            "copyright notice and this permission notice appear in supporting\n"
            "documentation.\n"
            "The above copyright notice and this permission notice shall be included in\n"
            "all copies or substantial portions of the Software.\n"
            "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
            "OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN\n"
            "AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
            "CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n"
            "Except as contained in this notice, the name of The Open Group shall not be\n"
            "used in advertising or otherwise to promote the sale, use or other dealings\n"
            "in this Software without prior written authorization from The Open Group.\n\n\n"
            "\n"
            "xcb:\n"
            "Copyright (C) 2001-2006 Bart Massey, Jamey Sharp, and Josh Triplett.\n"
            "All Rights Reserved.\n\n"
            "Permission is hereby granted, free of charge, to any person\n"
            "obtaining a copy of this software and associated\n"
            "documentation files (the 'Software'), to deal in the\n"
            "Software without restriction, including without limitation\n"
            "the rights to use, copy, modify, merge, publish, distribute,\n"
            "sublicense, and/or sell copies of the Software, and to\n"
            "permit persons to whom the Software is furnished to do so,\n"
            "subject to the following conditions:\n\n"
            "The above copyright notice and this permission notice shall\n"
            "be included in all copies or substantial portions of the\n"
            "Software.\n\n"
            "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY\n"
            "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\n"
            "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR\n"
            "PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS\n"
            "BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER\n"
            "IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
            "OTHER DEALINGS IN THE SOFTWARE.\n\n"
            "Except as contained in this notice, the names of the authors\n"
            "or their institutions shall not be used in advertising or\n"
            "otherwise to promote the sale, use or other dealings in this\n"
            "Software without prior written authorization from the\n"
            "authors.\n\n\n"
            "\n"
            "expat:\n\n"
            "Copyright (C) 1998, 1999, 2000 Thai Open Source Software Center Ltd\n"
            " and Clark Cooper\n"
            "Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.\n"
            "Permission is hereby granted, free of charge, to any person obtaining\n"
            "a copy of this software and associated documentation files (the\n"
            "'Software'), to deal in the Software without restriction, including\n"
            "without limitation the rights to use, copy, modify, merge, publish,\n"
            "distribute, sublicense, and/or sell copies of the Software, and to\n"
            "permit persons to whom the Software is furnished to do so, subject to\n"
            "the following conditions:\n\n"
            "The above copyright notice and this permission notice shall be included\n"
            "in all copies or substantial portions of the Software.\n\n"
            "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,\n"
            "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
            "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
            "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\n"
            "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\n"
            "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE\n"
            "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n\n"
            "\n"
            "fontconfig:\n"
            "Copyright (C) 2001,2003 Keith Packard\n\n"
            "Permission to use, copy, modify, distribute, and sell this software and its\n"
            "documentation for any purpose is hereby granted without fee, provided that\n"
            "the above copyright notice appear in all copies and that both that\n"
            "copyright notice and this permission notice appear in supporting\n"
            "documentation, and that the name of Keith Packard not be used in\n"
            "advertising or publicity pertaining to distribution of the software without\n"
            "specific, written prior permission. Keith Packard makes no\n"
            "representations about the suitability of this software for any purpose. It\n"
            "is provided 'as is' without express or implied warranty.\n\n"
            "KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,\n"
            "INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO\n"
            "EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR\n"
            "CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\n"
            "DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n"
            "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n"
            "PERFORMANCE OF THIS SOFTWARE.\n\n\n"
            "\n"
            "z:\n\n"
            "(C) 1995-2013 Jean-loup Gailly and Mark Adler\n\n"
            "This software is provided 'as-is', without any express or implied\n"
            "warranty. In no event will the authors be held liable for any damages\n"
            "arising from the use of this software.\n\n"
            "Permission is granted to anyone to use this software for any purpose,\n"
            "including commercial applications, and to alter it and redistribute it\n"
            "freely, subject to the following restrictions:\n\n"
            "1. The origin of this software must not be misrepresented; you must not\n"
            "claim that you wrote the original software. If you use this software\n"
            "in a product, an acknowledgment in the product documentation would be\n"
            "appreciated but is not required.\n"
            "2. Altered source versions must be plainly marked as such, and must not be\n"
            "misrepresented as being the original software.\n"
            "3. This notice may not be removed or altered from any source distribution.\n\n"
            "Jean-loup Gailly Mark Adler\n"
            "jloup@gzip.org madler@alumni.caltech.edu\n\n\n"
            "\n"
            "bz2:\n\n"
            "This program, 'bzip2', the associated library 'libbzip2', and all\n"
            "documentation, are copyright (C) 1996-2010 Julian R Seward. All\n"
            "rights reserved.\n\n"
            "Redistribution and use in source and binary forms, with or without\n"
            "modification, are permitted provided that the following conditions\n"
            "are met:\n\n"
            "1. Redistributions of source code must retain the above copyright\n"
            " notice, this list of conditions and the following disclaimer.\n\n"
            "2. The origin of this software must not be misrepresented; you must\n"
            " not claim that you wrote the original software. If you use this\n"
            " software in a product, an acknowledgment in the product\n"
            " documentation would be appreciated but is not required.\n\n"
            "3. Altered source versions must be plainly marked as such, and must\n"
            " not be misrepresented as being the original software.\n\n"
            "4. The name of the author may not be used to endorse or promote\n\n"
            " products derived from this software without specific prior written\n"
            " permission.\n\n"
            "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS\n"
            "OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
            "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n\n"
            "ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY\n"
            "DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
            "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE\n"
            "GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
            "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n"
            "WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
            "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
            "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n"
            "Julian Seward, jseward@bzip.org\n"
            "bzip2/libbzip2 version 1.0.6 of 6 September 2010\n\n\n"
            "\n"
            "harfbuzz:\n\n"
            "HarfBuzz is licensed under the so-called 'Old MIT' license. Details follow.\n\n"
            "Copyright (C) 2010,2011,2012 Google, Inc.\n"
            "Copyright (C) 2012 Mozilla Foundation\n"
            "Copyright (C) 2011 Codethink Limited\n"
            "Copyright (C) 2008,2010 Nokia Corporation and/or its subsidiary(-ies)\n"
            "Copyright (C) 2009 Keith Stribley\n"
            "Copyright (C) 2009 Martin Hosken and SIL International\n"
            "Copyright (C) 2007 Chris Wilson\n"
            "Copyright (C) 2006 Behdad Esfahbod\n"
            "Copyright (C) 2005 David Turner\n"
            "Copyright (C) 2004,2007,2008,2009,2010 Red Hat, Inc.\n"
            "Copyright (C) 1998-2004 David Turner and Werner Lemberg\n\n"
            "For full copyright notices consult the individual files in the package.\n\n"
            "Permission is hereby granted, without written agreement and without\n"
            "license or royalty fees, to use, copy, modify, and distribute this\n"
            "software and its documentation for any purpose, provided that the\n"
            "above copyright notice and the following two paragraphs appear in\n"
            "all copies of this software.\n\n"
            "IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR\n"
            "DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES\n"
            "ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN\n"
            "IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH\n"
            "DAMAGE.\n\n"
            "THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,\n"
            "BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"
            "FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS\n"
            "ON AN 'AS IS' BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO\n"
            "PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.\n\n\n"
            "\n"
            "freetype:\n\n"
            "The FreeType 2 font engine is copyrighted work and cannot be used\n"
            "legally without a software license. In order to make this project\n"
            "usable to a vast majority of developers, we distribute it under two\n"
            "mutually exclusive open-source licenses.\n\n"
            "This means that *you* must choose *one* of the two licenses described\n"
            "below, then obey all its terms and conditions when using FreeType 2 in\n"
            "any of your projects or products.\n\n"
            " - The FreeType License, found in the file `FTL.TXT', which is similar\n"
            " to the original BSD license *with* an advertising clause that forces\n"
            " you to explicitly cite the FreeType project in your product's\n"
            " documentation. All details are in the license file. This license\n"
            " is suited to products which don't use the GNU General Public\n"
            " License.\n\n"
            " Note that this license is compatible to the GNU General Public\n"
            " License version 3, but not version 2.\n\n"
            " - The GNU General Public License version 2, found in `GPLv2.TXT' (any\n"
            " later version can be used also), for programs which already use the\n"
            " GPL. Note that the FTL is incompatible with GPLv2 due to its\n"
            " advertisement clause.\n\n"
            "The contributed BDF and PCF drivers come with a license similar to that\n"
            "of the X Window System. It is compatible to the above two licenses (see\n"
            "file src/bdf/README and src/pcf/README).\n\n"
            "The gzip module uses the zlib license (see src/gzip/zlib.h) which too is\n"
            "compatible to the above two licenses.\n\n"
            "The MD5 checksum support (only used for debugging in development builds)\n"
            "is in the public domain.\n\n\n"
            "\n"
            "udev:\n\n"
            "Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>\n"
            "Copyright (C) 2003-2010 Kay Sievers <kay@vrfy.org>\n\n"
            "This program is free software: you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation, either version 2 of the License, or\n"
            "(at your option) any later version.\n\n"
            "This program is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
            "GNU General Public License for more details.\n\n"
            "You should have received a copy of the GNU General Public License\n"
            "along with this program. If not, see <http://www.gnu.org/licenses/>.\n\n\n"
            "\n"
            "dbus:\n\n"
            "D-Bus is licensed to you under your choice of the Academic Free\n"
            "License version 2.1, or the GNU General Public License version 2\n"
            "(or, at your option any later version).\n\n\n"
            "\n"
            "icu:\n\n"
            "ICU License - ICU 1.8.1 and later\n"
            "COPYRIGHT AND PERMISSION NOTICE\n"
            "Copyright (c) 1995-2015 International Business Machines Corporation and others\n"
            "All rights reserved.\n"
            "Permission is hereby granted, free of charge, to any person obtaining\n"
            "a copy of this software and associated documentation files (the 'Software'),\n"
            "to deal in the Software without restriction, including without limitation the\n"
            "rights to use, copy, modify, merge, publish, distribute, and/or sell copies of\n"
            "the Software, and to permit persons to whom the Software is furnished to do so,\n"
            "provided that the above copyright notice(s) and this permission notice appear\n"
            "in all copies of the Software and that both the above copyright notice(s) and\n"
            "this permission notice appear in supporting documentation.\n"
            "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN\n"
            "NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE\n"
            "FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES\n"
            "WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF\n"
            "CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION\n"
            "WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n"
            "Except as contained in this notice, the name of a copyright holder shall not be\n"
            "used in advertising or otherwise to promote the sale, use or other dealings in\n"
            "this Software without prior written authorization of the copyright holder.\n\n\n"
            "\n"
            "Unicode:\n\n"
            "COPYRIGHT AND PERMISSION NOTICE\n\n"
            "Copyright (C) 1991-2015 Unicode, Inc. All rights reserved.\n"
            "Distributed under the Terms of Use in\n"
            "http://www.unicode.org/copyright.html.\n\n"
            "Permission is hereby granted, free of charge, to any person obtaining\n\n"
            "a copy of the Unicode data files and any associated documentation\n"
            "(the 'Data Files') or Unicode software and any associated documentation\n"
            "(the 'Software') to deal in the Data Files or Software\n"
            "without restriction, including without limitation the rights to use,\n"
            "copy, modify, merge, publish, distribute, and/or sell copies of\n"
            "the Data Files or Software, and to permit persons to whom the Data Files\n"
            "or Software are furnished to do so, provided that\n"
            "(a) this copyright and permission notice appear with all copies\n"
            "of the Data Files or Software,\n"
            "(b) this copyright and permission notice appear in associated\n"
            "documentation, and\n"
            "(c) there is clear notice in each modified Data File or in the Software\n"
            "as well as in the documentation associated with the Data File(s) or\n"
            "Software that the data or software has been modified.\n\n"
            "THE DATA FILES AND SOFTWARE ARE PROVIDED 'AS IS', WITHOUT WARRANTY OF\n"
            "ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\n"
            "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
            "NONINFRINGEMENT OF THIRD PARTY RIGHTS.\n\n"
            "IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS\n"
            "NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL\n"
            "DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\n"
            "DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n"
            "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n"
            "PERFORMANCE OF THE DATA FILES OR SOFTWARE.\n\n"
            "Except as contained in this notice, the name of a copyright holder\n"
            "shall not be used in advertising or otherwise to promote the sale,\n"
            "use or other dealings in these Data Files or Software without prior\n"
            "written authorization of the copyright holder.\n\n\n"
            "\n"
            "UPX:\n\n"
            "Copyright (C) 1996-2013 Markus Franz Xaver Johannes Oberhumer\n"
            "Copyright (C) 1996-2013 László Molnár\n"
            "Copyright (C) 2000-2013 John F. Reiser\n\n"
            "All Rights Reserved. This program may be used freely, and you are welcome to redistribute and/or modify it under certain conditions.\n\n"
            "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the UPX License Agreement for more details: http://upx.sourceforge.net/upx-license.html"
            "\n"
            "OpenSSL:\n\n"
            "Copyright (c) 1998-2016 The OpenSSL Project. All rights reserved.\n\n"
            "Redistribution and use in source and binary forms, with or without\n"
            "modification, are permitted provided that the following conditions\n"
            "are met:\n\n"
            "1. Redistributions of source code must retain the above copyright\n"
            " notice, this list of conditions and the following disclaimer. \n\n"
            "2. Redistributions in binary form must reproduce the above copyright\n"
            " notice, this list of conditions and the following disclaimer in\n"
            " the documentation and/or other materials provided with the\n"
            " distribution.\n\n"
            "3. All advertising materials mentioning features or use of this\n"
            " software must display the following acknowledgment:\n"
            " 'This product includes software developed by the OpenSSL Project\n"
            " for use in the OpenSSL Toolkit. (http://www.openssl.org/)'\n\n"
            "4. The names 'OpenSSL Toolkit' and 'OpenSSL Project' must not be used to\n"
            " endorse or promote products derived from this software without\n"
            " prior written permission. For written permission, please contact\n"
            " openssl-core@openssl.org.\n\n"
            "5. Products derived from this software may not be called 'OpenSSL'\n"
            " nor may 'OpenSSL' appear in their names without prior written\n"
            " permission of the OpenSSL Project.\n\n"
            "6. Redistributions of any form whatsoever must retain the following\n"
            " acknowledgment:\n"
            " 'This product includes software developed by the OpenSSL Project\n"
            " for use in the OpenSSL Toolkit (http://www.openssl.org/)'\n\n"
            "THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY\n"
            "EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
            "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n"
            "PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE OpenSSL PROJECT OR\n"
            "ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
            "SPECIAL, EXEMPLARY, OR").append(" CONSEQUENTIAL DAMAGES (INCLUDING, BUT\n"
            "NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
            "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
            "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,\n"
            "STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
            "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED\n"
            "OF THE POSSIBILITY OF SUCH DAMAGE.\n"
            "====================================================================\n\n"
            "This product includes cryptographic software written by Eric Young\n"
            "(eay@cryptsoft.com). This product includes software written by Tim\n"
            "Hudson (tjh@cryptsoft.com).\n\n\n"
            " Original SSLeay License\n"
            " -----------------------\n\n"
            "Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)\n"
            "All rights reserved.\n\n"
            "This package is an SSL implementation written\n"
            "by Eric Young (eay@cryptsoft.com).\n"
            "The implementation was written so as to conform with Netscapes SSL.\n\n"
            "This library is free for commercial and non-commercial use as long as\n"
            "the following conditions are aheared to. The following conditions\n"
            "apply to all code found in this distribution, be it the RC4, RSA,\n"
            "lhash, DES, etc., code; not just the SSL code. The SSL documentation\n"
            "included with this distribution is covered by the same copyright terms\n"
            "except that the holder is Tim Hudson (tjh@cryptsoft.com).\n\n"
            "Copyright remains Eric Young's, and as such any Copyright notices in\n"
            "the code are not to be removed.\n"
            "If this package is used in a product, Eric Young should be given attribution\n"
            "as the author of the parts of the library used.\n"
            "This can be in the form of a textual message at program startup or\n"
            "in documentation (online or textual) provided with the package.\n\n"
            "Redistribution and use in source and binary forms, with or without\n"
            "modification, are permitted provided that the following conditions\n"
            "are met:\n"
            "1. Redistributions of source code must retain the copyright\n"
            " notice, this list of conditions and the following disclaimer.\n"
            "2. Redistributions in binary form must reproduce the above copyright\n"
            " notice, this list of conditions and the following disclaimer in the\n"
            " documentation and/or other materials provided with the distribution.\n"
            "3. All advertising materials mentioning features or use of this software\n"
            " must display the following acknowledgement:\n"
            " 'This product includes cryptographic software written by\n"
            " Eric Young (eay@cryptsoft.com)'\n"
            " The word 'cryptographic' can be left out if the rouines from the library\n"
            " being used are not cryptographic related :-).\n"
            "4. If you include any Windows specific code (or a derivative thereof) from \n"
            " the apps directory (application code) you must include an acknowledgement:\n"
            " 'This product includes software written by Tim Hudson (tjh@cryptsoft.com)'\n\n"
            "THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND\n"
            "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
            "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
            "ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n"
            "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
            "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
            "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
            "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
            "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
            "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
            "SUCH DAMAGE.\n\n"
            "The licence and distribution terms for any publically available version or\n"
            "derivative of this code cannot be changed. i.e. this code cannot simply be\n"
            "copied and put under another distribution licence\n"
            "[including the GNU Public Licence.]\n\n"
            "\n"
            "libftdi:\n\n"
            "The C library \"libftdi\" is distributed under the\n"
            "GNU Library General Public License version 2.\n\n"
            "\n"
            "libusb:\n\n"
            "The C library \"libusb\" is distributed under the\n"
            "GNU Library Lesser General Public License version 2.1.\n\n"
            "\n"
            "FTDI D2XX:\n\n"
            "This software is provided by Future Technology Devices International Limited ``as is''"
            " and any express or implied warranties, including, but not limited to, the implied "
            "warranties of merchantability and fitness for a particular purpose are disclaimed. "
            "In no event shall future technology devices international limited be liable for any "
            "direct, indirect, incidental, special, exemplary, or consequential damages (including, "
            "but not limited to, procurement of substitute goods or services; loss of use, data, or "
            "profits; or business interruption) however caused and on any theory of liability, "
            "whether in contract, strict liability, or tort (including negligence or otherwise) "
            "arising in any way out of the use of this software, even if advised of the possibility "
            "of such damage.\n"
            "FTDI drivers may be used only in conjunction with products based on FTDI parts.\n"
            "FTDI drivers may be distributed in any form as long as license information is not modified."
        ));
    }
}

#ifndef SKIPUPDATECHECK
//=============================================================================
// Check for newer versions of application
//=============================================================================
void
MainWindow::on_btn_UpdateCheck_clicked(
    )
{
    if (pAppUpdate == NULL)
    {
        //Create update object
        pAppUpdate = new LrdAppUpd();
        MallocFailCheck(pAppUpdate);
        pAppUpdate->SetSettingsObject(pSettingsHandle);
        connect(pAppUpdate, SIGNAL(Error(uint32_t,int32_t)), this, SLOT(ModuleError(uint32_t,int32_t)));
        connect(pAppUpdate, SIGNAL(VersionResponse(VERSION_RESPONSES,QString)), this, SLOT(ApplicationUpdateResponse(VERSION_RESPONSES,QString)));
    }

    //Update status
    ui->lbl_LatestVersion->setText("Checking for updates...");
    QPalette palBGColour = QPalette();
    ui->lbl_LatestVersion->setPalette(palBGColour);

    //Check for updates
    pAppUpdate->UpdateCheck(
#ifdef UseSSL
        ui->check_Enable_SSL->isChecked(),
#endif
        APP_NAME, APP_VERSION);
    ui->btn_UpdateCheck->setEnabled(false);
}
#endif

//=============================================================================
// When the FTDI override checkbox status is changes
//=============================================================================
void
MainWindow::on_check_FTDI_Override_ID_stateChanged(
    int
    )
{
    ui->edit_FTDI_ID->setEnabled(ui->check_FTDI_Override_ID->isChecked());
}

//=============================================================================
// When the bootloader entrance combo box is changed
//=============================================================================
void
MainWindow::on_combo_Bootloader_Enter_Method_currentIndexChanged(
    int
    )
{
    if (ui->combo_Bootloader_Enter_Method->currentIndex() == ENTER_BOOTLOADER_AT_FUP)
    {
        //AT+FUP method
        ui->combo_InitialApplicationBaud->setEnabled(true);
        ui->check_FTDI_Override_ID->setEnabled(false);
        ui->edit_FTDI_ID->setEnabled(false);
    }
    else if (ui->combo_Bootloader_Enter_Method->currentIndex() == ENTER_BOOTLOADER_BL654_USB)
    {
        //FTDI-based and AT+FUP method
        ui->combo_InitialApplicationBaud->setEnabled(true);
        ui->check_FTDI_Override_ID->setEnabled(true);
        ui->edit_FTDI_ID->setEnabled(ui->check_FTDI_Override_ID->isChecked());
    }
    else if (ui->combo_Bootloader_Enter_Method->currentIndex() == ENTER_BOOTLOADER_PINNACLE100)
    {
        //FTDI-based method
        ui->combo_InitialApplicationBaud->setEnabled(false);
        ui->check_FTDI_Override_ID->setEnabled(true);
        ui->edit_FTDI_ID->setEnabled(ui->check_FTDI_Override_ID->isChecked());
    }
    else
    {
        //None
        ui->combo_InitialApplicationBaud->setEnabled(false);
        ui->check_FTDI_Override_ID->setEnabled(false);
        ui->edit_FTDI_ID->setEnabled(false);
    }
}

//=============================================================================
// Slot for application update response
//=============================================================================
void
MainWindow::ApplicationUpdateResponse(
    VERSION_RESPONSES nResponse,
    QString strNewVersion
    )
{
    if (nResponse == VERSION_RESPONSE_CURRENT)
    {
        //Latest version
        ui->lbl_LatestVersion->setText("No updates available.");
        QPalette palBGColour = QPalette();
        ui->lbl_LatestVersion->setPalette(palBGColour);
    }
    else if (nResponse == VERSION_RESPONSE_OUTDATED)
    {
        //Update is available
        ui->lbl_LatestVersion->setText(QString("Update is available! Version ").append(strNewVersion));
        QPalette palBGColour = QPalette();
        palBGColour.setColor(QPalette::Active, QPalette::WindowText, Qt::darkGreen);
        palBGColour.setColor(QPalette::Inactive, QPalette::WindowText, Qt::darkGreen);
        palBGColour.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGreen);
        ui->lbl_LatestVersion->setPalette(palBGColour);
    }
    else if (nResponse == VERSION_RESPONSE_ERROR)
    {
        //Error occured
        ui->lbl_LatestVersion->setText("Network error occured.");
        QPalette palBGColour = QPalette();
        palBGColour.setColor(QPalette::Active, QPalette::WindowText, Qt::red);
        palBGColour.setColor(QPalette::Inactive, QPalette::WindowText, Qt::red);
        palBGColour.setColor(QPalette::Disabled, QPalette::WindowText, Qt::red);
        ui->lbl_LatestVersion->setPalette(palBGColour);

        //Display error details
        QString strMessage = QString("Error whilst checking for updates: ").append(strNewVersion);
        gpmErrorForm->SetButtonVisibility(POPUP_LEFT_BUTTON);
        gpmErrorForm->SetButtonText(POPUP_LEFT_BUTTON, "&Close");
        gpmErrorForm->SetEscapeKeyHandling(true);
        gpmErrorForm->SetDefaultButton(POPUP_LEFT_BUTTON);
        gpmErrorForm->SetMessage(&strMessage);
        gpmErrorForm->show();
    }
    ui->btn_UpdateCheck->setEnabled(true);
}

#ifdef __linux__
//=============================================================================
// Slot for if serial port name is updated after entering bootloader
//=============================================================================
void
MainWindow::SerialPortNameChanged(
    QString *pNewPortName
    )
{
    ui->combo_COM->setCurrentText(*pNewPortName);
}
#endif

//=============================================================================
// Open the help file if it exists, or browse to it online if not
//=============================================================================
void
MainWindow::on_btn_Help_clicked(
    )
{
#ifndef __APPLE__
    if (QFile::exists("UwFlashX_Help.pdf"))
    {
        //File present - open
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo("UwFlashX_Help.pdf").absoluteFilePath()));
    }
    else
#endif
    {
        //File not present, open on website instead
        if (QDesktopServices::openUrl(QUrl(QString("http://").append(pSettingsHandle->GetConfigOption(UPDATE_SERVER_HOST).toString()).append("/UwFlashX_help.pdf"))) == false)
        {
            //Failed to open
            QString strMessage = tr("Help file (UwFlashX_Help.pdf) was not found and an error occured whilst attempting to open the online version.");
            gpmErrorForm->SetButtonVisibility(POPUP_LEFT_BUTTON);
            gpmErrorForm->SetButtonText(POPUP_LEFT_BUTTON, "&Close");
            gpmErrorForm->SetEscapeKeyHandling(true);
            gpmErrorForm->SetDefaultButton(POPUP_LEFT_BUTTON);
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
        }
    }
}

//=============================================================================
// Callback when UART BREAK checkbox status is changed
//=============================================================================
void
MainWindow::on_check_BREAK_stateChanged(
    int
    )
{
    ui->combo_BREAK_DTR->setEnabled(ui->check_BREAK->isChecked());
}

//=============================================================================
// Callback when the setup link is clicked
//=============================================================================
void
MainWindow::on_label_Setup_Help_linkActivated(
    const QString &link
    )
{
    if (QDesktopServices::openUrl(QUrl(link)) == false)
    {
        //Failed to open
        QString strMessage = QString("An error occured whilst attempting to open the online help link, please browse to ").append(link).append(" in your web browser.");
        gpmErrorForm->SetButtonVisibility(POPUP_LEFT_BUTTON);
        gpmErrorForm->SetButtonText(POPUP_LEFT_BUTTON, "&Close");
        gpmErrorForm->SetEscapeKeyHandling(true);
        gpmErrorForm->SetDefaultButton(POPUP_LEFT_BUTTON);
        gpmErrorForm->SetMessage(&strMessage);
        gpmErrorForm->show();
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
