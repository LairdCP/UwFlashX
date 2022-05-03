/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  mainwindow.h
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QMainWindow>
#include <QFileDialog>
#include <QMimeData>
#include <QTimer>
#include <QDebug>
#include "LrdFwUpd.h"
#include "LrdFwUART.h"
#include "LrdSettings.h"
#include "LrdErr.h"
#include "LrdFwCommon.h"
#ifndef SKIPUPDATECHECK
#include "LrdAppUpd.h"
#endif
#include "LrdFwBlEnter.h"
#include "LrdPopup.h"

/******************************************************************************/
// Defines
/******************************************************************************/
#define APP_NAME                                      "UwFlashX"            //Application name
#define APP_VERSION                                   "1.01"                //Application version

/******************************************************************************/
// Constants
/******************************************************************************/
//Note that these must all be in CAPITALS for case sensitive comparison code
const QString strOptionAutoMode                     = "AUTOMODE";
const QString strOptionAutoExit                     = "AUTOEXIT";
const QString strOptionCom                          = "COM";
const QString strOptionPort                         = "PORT";
const QString strOptionUwf                          = "UWF";
const QString strOptionUbu                          = "UBU";
const QString strOptionKey                          = "KEY";
const QString strOptionApplicationBaud              = "APPLICATIONBAUD";
const QString strOptionBootloaderBaud               = "BOOTLOADERBAUD";
const QString strOptionMaxBaud                      = "MAXBAUD";
const QString strOptionExactBaud                    = "EXACTBAUD";
const QString strOptionDisableEnhanced              = "DISABLEENHANCED";
const QString strOptionReboot                       = "REBOOT";
const QString strOptionVerify                       = "VERIFY";
const QString strOptionUARTBREAK                    = "UARTBREAK";
const QString strOptionDTS                          = "DTS";
const QString strOptionEntrance                     = "ENTRANCE";
const QString strOptionNoPrompts                    = "NOPROMPTS";
const QString strOptionSeperateCharacter            = "=";

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class MainWindow;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        QWidget *parent = 0
        );
    ~MainWindow(
        );

private slots:
    void
    on_btn_FileSelector_clicked(
        );
    void
    on_btn_Refresh_clicked(
        );
    void
    on_btn_Go_clicked(
        );
    void
    RefreshSerialDevices(
        );
    void
    on_combo_COM_currentIndexChanged(
        int
        );
    void
    StartStopUpgrade(
        bool bStart
        );
    void
    DroppedFile(
        QString strFilename
        );
    void
    on_check_MaximumBaud_stateChanged(
        int
        );
    void
    on_check_ExactBaud_stateChanged(
        int
        );
    void
    DelayedExit(
        );
    void
    CurrentAction(
        uint32_t nModule,
        uint32_t nActionID,
        QString strActionName
        );
    void
    ProgressUpdate(
        int8_t nTaskPercent,
        int8_t nOverallPercent
        );
    void
    ModuleError(
        uint32_t nModule,
        int32_t nErrorCode
        );
    void
    UpgradeFinished(
        bool bSuccess,
        qint64 nUpgradeTimeMS
        );
    void
    on_tab_Selector_currentChanged(
        int
        );
#ifndef SKIPUPDATECHECK
    void
    on_btn_UpdateCheck_clicked(
        );
#endif
    void
    on_check_FTDI_Override_ID_stateChanged(
        int
        );
    void
    on_combo_Bootloader_Enter_Method_currentIndexChanged(
        int
        );
    void
    ApplicationUpdateResponse(
        VERSION_RESPONSES nResponse,
        QString strNewVersion
        );
#ifdef __linux__
    void
    SerialPortNameChanged(
        QString *pNewPortName
        );
#endif
    void
    on_btn_Help_clicked(
        );
    void
    on_check_BREAK_stateChanged(
        int
        );
    void
    on_label_Setup_Help_linkActivated(
        const QString &link
        );

protected:
    void
    dragEnterEvent(
        QDragEnterEvent *dragEvent
        );
    void
    dropEvent(
        QDropEvent *dropEvent
        );

private:
    Ui::MainWindow  *ui;                                //GUI object
    LrdFwUpd        *pFwUpd = NULL;                     //Firmware update object
    LrdSettings     *pSettingsHandle = NULL;            //Settings object
    LrdErr          *pErrHandler = NULL;                //Error handler object
#ifndef SKIPUPDATECHECK
    LrdAppUpd       *pAppUpdate = NULL;                 //Application update check object
#endif
    bool            bArgAutoexit;                       //Set to true if the application should automatically exit
    int32_t         nErrorCode;                         //The current error or sucess code of the upgrade process
    uint8_t         nVerbosity;                         //The verbosity level of the output
    QTimer          *tmrExitApplicationTimer = NULL;    //Timer used to exit application in autoexit mode
    LrdPopupMessage *gpmErrorForm = NULL;               //Popup error/question form
};

#endif // MAINWINDOW_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
