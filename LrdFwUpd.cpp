/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUpd.cpp
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
#include "LrdFwUpd.h"
#include <QDebug>
#if defined(__linux__) || defined(__APPLE__)
//Linux or mac, required include for usleep
#include <unistd.h>
#endif
#ifdef _WIN32
//Required for sleep
#include <windows.h>
#endif

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Constructor
//=============================================================================
LrdFwUpd::LrdFwUpd(
    QObject *parent,
    LrdSettings *pSettings
    ) : QObject(parent)
{
    //Assign settings handle
    pSettingsHandle = pSettings;

    //Create device object
    pDevice = new LrdFwUART(nullptr, pSettings);
    MallocFailCheck(pDevice);

    //Create Uwf file reader object
    pUwfData = new LrdFwUwf(nullptr, pSettings);
    MallocFailCheck(pUwfData);

#if !defined(TARGET_OS_MAC)
    //Create bootloader entrance object
    pBlEnter = new LrdFwBlEnter(nullptr, pSettings);
    MallocFailCheck(pBlEnter);
#endif

    //Connect signals
    connect(pDevice, SIGNAL(Receive(QByteArray*)), this, SLOT(ModuleDataReceived(QByteArray*)));
    connect(pDevice, SIGNAL(Error(uint32_t,int32_t)), this, SLOT(ModuleError(uint32_t,int32_t)));
    connect(pUwfData, SIGNAL(Error(uint32_t,int32_t)), this, SLOT(ModuleError(uint32_t,int32_t)));
#if !defined(TARGET_OS_MAC)
    connect(pBlEnter, SIGNAL(Error(uint32_t,int32_t)), this, SLOT(ModuleError(uint32_t,int32_t)));
#endif

    //Setup command timeout timer
    tmrCommandTimeoutTimer = new QTimer();
    MallocFailCheck(tmrCommandTimeoutTimer);
    tmrCommandTimeoutTimer->setInterval(COMMAND_TIMEOUT_PERIOD_MS);
    tmrCommandTimeoutTimer->setSingleShot(false);
    connect(tmrCommandTimeoutTimer, SIGNAL(timeout()), this, SLOT(CommandTimeout()));

    //Set variables to null
    tmrBaudRateChangeTimer = NULL;
    tmrDeviceReadyTimer = NULL;
    tmrRestartTimer = NULL;

    //No errors have occured
    nLastErrorCode = EXIT_CODE_SUCCESS;

    //Disable verbose messages by default
    nVerbosity = VERBOSITY_NONE;
}

//=============================================================================
// Destructor
//=============================================================================
LrdFwUpd::~LrdFwUpd(
    )
{
    //Disconnect signals
    disconnect(this, SLOT(ModuleDataReceived(QByteArray*)));
    disconnect(this, SLOT(ModuleError(uint32_t,int32_t)));

    if (pDevice->IsOpen())
    {
        //Clean up serial port
        pDevice->Close();
    }

    while (lstDevices.count() > 0)
    {
        //Clear list of registered devices
        DeviceStruct *device = lstDevices.last();
        lstDevices.pop_back();
        delete device;
    }

    if (elptmrUpgradeTime.isValid())
    {
        //Invalidate timer
        elptmrUpgradeTime.invalidate();
    }

    delete pDevice;
    delete pUwfData;

    if (tmrCommandTimeoutTimer != NULL)
    {
        //Clean up command timeout timer
        delete tmrCommandTimeoutTimer;
    }
}

//=============================================================================
// Set the settings handle
//=============================================================================
void
LrdFwUpd::SetSettingsObject(
    LrdSettings *pSettings
    )
{
    pSettingsHandle = pSettings;
    pDevice->SetSettingsObject(pSettings);
    pUwfData->SetSettingsObject(pSettings);
#if !defined(TARGET_OS_MAC)
    pBlEnter->SetSettingsObject(pSettings);
#endif
}

//=============================================================================
// Starts the update process
//=============================================================================
bool
LrdFwUpd::StartUpdate(
    )
{
    //
    if (pSettingsHandle == nullptr)
    {
        //Settings handle is not set
        nLastErrorCode = EXIT_CODE_SETTINGS_HANDLE_NULL;
        emit Error(MODULE_UPDATE, EXIT_CODE_SETTINGS_HANDLE_NULL);
        return false;
    }

    nVerbosity = pSettingsHandle->GetConfigOption(UPDATE_VERBOSITY).toUInt();

    //Check if the bootloader unlock key is valid
    uint8_t nUnlockKeySize = pSettingsHandle->GetConfigOption(UNLOCK_KEY).toByteArray().length();
    if (nUnlockKeySize > 0 && nUnlockKeySize != FUP_BOOTLOADER_UNLOCK_KEY_SIZE)
    {
        //The bootloader unlock key is an invalid length
        nLastErrorCode = EXIT_CODE_BOOTLOADER_UNLOCK_KEY_INVALID_SIZE;
        emit Error(MODULE_UPDATE, EXIT_CODE_BOOTLOADER_UNLOCK_KEY_INVALID_SIZE);
        return false;
    }

    if (pUwfData->Open() == false)
    {
        return false;
    }

    nFileSize = pUwfData->TotalSize();

    //Check if the file size is valid
    if (nFileSize < UWF_COMMAND_HEADER_LENGTH || nFileSize > UWF_FILE_MAX_SIZE_BYTES)
    {
        //Filesize is too small or large, not a valid uwf file
        pUwfData->Close();
        emit CurrentAction(MODULE_UPDATE, 0, "Selected upgrade file is too small or large and is not valid.");
        emit Error(MODULE_UPDATE, EXIT_CODE_UWF_FILE_INVALID_SIZE);
        return false;
    }

    //If enabled, validate that the upgrade file does not have any errors in it
    bool bValidateUpgradeFile = pSettingsHandle->GetConfigOption(UPDATE_VERBOSITY).toUInt();
    if (bValidateUpgradeFile == true && ValidateUpgradeFile() == false)
    {
        //The upgrade file is not valid and contains an error
        pUwfData->Close();
        emit CurrentAction(MODULE_UPDATE, 0, "Selected upgrade file is not valid.");
        emit Error(MODULE_UPDATE, EXIT_CODE_UWF_FILE_NOT_VALID);
        return false;
    }

    if (bValidateUpgradeFile == true)
    {
        //Rewind to the beginning of the uwf file
        pUwfData->Seek(SEEK_FROM_BEGINNING, 0);
    }

    //Check if the first command is a valid
    QByteArray baTmpPktHeader = pUwfData->Read(UWF_COMMAND_HEADER_LENGTH);
    uint8_t nCmdID = (uint8_t)baTmpPktHeader[UWF_OFFSET_HEADER_COMMAND_ID];

    if (nCmdID != UWF_COMMAND_TARGET_PLATFORM && nCmdID != UWF_COMMAND_REGISTER && nCmdID != UWF_COMMAND_SELECT && nCmdID != UWF_COMMAND_SECTOR_MAP && nCmdID != UWF_COMMAND_ERASE && nCmdID != UWF_COMMAND_WRITE && nCmdID != UWF_COMMAND_QUERY && nCmdID != UWF_COMMAND_UNREGISTER)
    {
        emit CurrentAction(MODULE_UPDATE, 0, QString("Selected upgrade file has unknown command 0x").append(QString::number(nCmdID, 16)).append(" and is not valid."));
        emit Error(MODULE_UPDATE, EXIT_CODE_UWF_FILE_COMMAND_INVALID);
        return false;
    }

    //Check if packet length is valid
    uint32_t nPktLen = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTmpPktHeader, UWF_OFFSET_HEADER_PACKET_LENGTH, nPktLen);

    if (nPktLen > nFileSize - UWF_COMMAND_HEADER_LENGTH || nPktLen > UWF_FILE_MAX_PACKET_SIZE_BYTES)
    {
        emit CurrentAction(MODULE_UPDATE, 0, QString("Selected upgrade file has command of length ").append(QString::number(nPktLen)).append(" and is not valid."));
        emit Error(MODULE_UPDATE, EXIT_CODE_UWF_FILE_PACKET_LENGTH_INVALID);
        return false;
    }

    //Valid upgrade file, rewind to beginning of file
    pUwfData->Seek(SEEK_FROM_BEGINNING, 0);

    //Set defaults
    nMaxEraseLengthCmd = DEFAULT_ERASE_COMMAND_LENGTH;
    nMaxWriteLengthCmd = DEFAULT_WRITE_COMMAND_LENGTH;
    nMaxChecksumLengthCmd = DEFAULT_CHECKSUM_COMMAND_LENGTH;
    nMaxVerifyChecksumLengthCmd = DEFAULT_VERIFY_CHECKSUM_COMMAND_LENGTH;
    nMaxWriteSize = DEFAULT_WRITE_SIZE;
    nActiveWriteSize = DEFAULT_WRITE_SIZE;
    nMaxChecksumSize = DEFAULT_CHECKSUM_COMMAND_LENGTH;
    nActiveEraseLengthCmd = DEFAULT_ERASE_COMMAND_LENGTH;
    nActiveWriteLengthCmd = DEFAULT_WRITE_COMMAND_LENGTH;
    nActiveChecksumLengthCmd = DEFAULT_CHECKSUM_COMMAND_LENGTH;
    nActiveVerifyChecksumLengthCmd = DEFAULT_VERIFY_CHECKSUM_COMMAND_LENGTH;

    //Check if module should be restarted prior to upgrade by using a UART BREAK
    if (pSettingsHandle->GetConfigOption(REBOOT_MODULE_BEFORE_UPDATE).toBool() == true)
    {
        //Module should be rebooted, BAUD rate does not matter so let's use the bootloader BAUD rate
        pSettingsHandle->SetConfigOption(ACTIVE_BAUD, pSettingsHandle->GetConfigOption(BOOTLOADER_BAUD));
        if (pDevice->Open() == false)
        {
            //Failed to open
            emit CurrentAction(MODULE_UPDATE, 0, "Serial port opening failed.");
            return false;
        }

        //Set DTR to the desired state, enable BREAK, wait for a period of time, then disable BREAK
        pDevice->SetDTR(pSettingsHandle->GetConfigOption(REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS).toBool());
        pDevice->SetBreak(true);
#ifdef _WIN32
        Sleep(REBOOT_MODULE_BREAK_ON_TIME_MS);
#else
        usleep(REBOOT_MODULE_BREAK_ON_TIME_US);
#endif
        pDevice->SetBreak(false);

        //Setup a CTS timer to check if the module has successfully rebooted
        nDeviceReadyChecks = 0;
        tmrDeviceReadyTimer = new QTimer();
        MallocFailCheck(tmrDeviceReadyTimer);
        tmrDeviceReadyTimer->setInterval(FUP_DEVICE_READY_TIMER_TIME_MS);
        tmrDeviceReadyTimer->setSingleShot(false);
        connect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceRebootReadyTimerTimeout()));
        tmrDeviceReadyTimer->start();
    }
    else
    {
        //Continue with the upgrade process right away
        ContinueUpgrade();
    }

    return true;
}

//=============================================================================
// Continues with the update process either right away or after a module reboot
//=============================================================================
void
LrdFwUpd::ContinueUpgrade(
    )
{
    //Check if there is a special bootloader entrance method
    uint8_t nBlEnterType = pSettingsHandle->GetConfigOption(BOOTLOADER_ENTER_METHOD).toUInt();
    pSettingsHandle->SetConfigOption(ACTIVE_BAUD, pSettingsHandle->GetConfigOption((nBlEnterType == ENTER_BOOTLOADER_BL654_USB || nBlEnterType == ENTER_BOOTLOADER_AT_FUP ? APPLICATION_BAUD : BOOTLOADER_BAUD)));

    if (nBlEnterType == ENTER_BOOTLOADER_NONE)
    {
        if (pDevice->Open() == false)
        {
            //Failed to open
            emit CurrentAction(MODULE_UPDATE, 0, "Serial port opening failed.");
            UpdateFailed(EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN);
            return;
        }
    }
    else
    {
        if (nBlEnterType == ENTER_BOOTLOADER_BL654_USB || nBlEnterType == ENTER_BOOTLOADER_PINNACLE100)
        {
#if defined(__APPLE__)
            //No support for libfti/libusb on mac as of yet
            emit CurrentAction(MODULE_UPDATE, 0, "FTDI-based reset functionality is not supported on Macs, please use the Linux or Windows utility for this functionality.");
            UpdateFailed(EXIT_CODE_BOOTLOADER_ENTRANCE_NOT_SUPPORTED);
            return;
#else
            //Use FTDI to enter bootloader
            emit CurrentAction(MODULE_UPDATE, 0, "Waiting for module to enter bootloader...");
#ifdef __linux__
            QString strNewPortName;
#endif

            if (pBlEnter->EnterBootloader(nBlEnterType, pSettingsHandle->GetConfigOption(OUTPUT_DEVICE).toString(), "",
#ifdef __linux__
                &strNewPortName,
#endif
                pSettingsHandle->GetConfigOption(BOOTLOADER_ENTRANCE_WARNINGS_DISABLED).toBool(), pSettingsHandle->GetConfigOption(BOOTLOADER_ENTRANCE_ERRORS_DISABLED).toBool()) == false)
            {
                //Failed to enter bootloader mode
                emit CurrentAction(MODULE_UPDATE, 0, "Error whilst attempting to enter bootloader mode.");
                UpdateFailed(EXIT_CODE_BOOTLOADER_ENTRANCE_FAILED);
                return;
            }

#ifdef __linux__
            if (!strNewPortName.isNull() && !strNewPortName.isEmpty())
            {
                //Update serial port
                pSettingsHandle->SetConfigOption(OUTPUT_DEVICE, strNewPortName);
                emit SerialPortNameChanged(&strNewPortName);
            }
#endif

            if (nBlEnterType == ENTER_BOOTLOADER_PINNACLE100)
            {
                //Re-open serial port
                if (pDevice->Open() == false)
                {
                    //Failed to open
                    emit CurrentAction(MODULE_UPDATE, 0, "Serial port opening failed.");
                    UpdateFailed(EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN);
                    return;
                }
            }
#endif
        }

        if (nBlEnterType == ENTER_BOOTLOADER_BL654_USB || nBlEnterType == ENTER_BOOTLOADER_AT_FUP)
        {
            //Use AT+FUP entrance
            if (pDevice->Open() == false)
            {
                //Failed to open
                emit CurrentAction(MODULE_UPDATE, 0, "Serial port opening failed.");
                UpdateFailed(EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN);
                return;
            }

            nCMode = MODE_ENTER_BOOTLOADER;
            pDevice->Transmit(COMMAND_ENTER_BOOTLOADER);
            if (nVerbosity >= VERBOSITY_COMMANDS)
            {
                qDebug() << COMMAND_ENTER_BOOTLOADER;
            }

            if (nBlEnterType == ENTER_BOOTLOADER_AT_FUP)
            {
                emit CurrentAction(MODULE_UPDATE, 0, "Waiting for module to enter bootloader...");
            }
        }
    }

    //Waiting for module to become ready
    nDeviceReadyChecks = 0;
    tmrDeviceReadyTimer = new QTimer();
    MallocFailCheck(tmrDeviceReadyTimer);
    tmrDeviceReadyTimer->setInterval(FUP_DEVICE_READY_TIMER_TIME_MS);
    tmrDeviceReadyTimer->setSingleShot(false);
    connect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceReadyTimerTimeout()));
    tmrDeviceReadyTimer->start();
}

//=============================================================================
// Callback when the CTS timer has elapsed (for entering bootloader mode)
//=============================================================================
void
LrdFwUpd::DeviceReadyTimerTimeout(
    )
{
    //Check CTS status
    if (pDevice->DeviceReady() == true)
    {
        //Device is now ready
        tmrDeviceReadyTimer->stop();
        disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceReadyTimerTimeout()));
        delete tmrDeviceReadyTimer;
        tmrDeviceReadyTimer = NULL;
        uint8_t nBlEnterType = pSettingsHandle->GetConfigOption(BOOTLOADER_ENTER_METHOD).toUInt();
        if (nBlEnterType == ENTER_BOOTLOADER_BL654_USB || nBlEnterType == ENTER_BOOTLOADER_AT_FUP)
        {
            if (pSettingsHandle->GetConfigOption(BOOTLOADER_BAUD) != pSettingsHandle->GetConfigOption(ACTIVE_BAUD))
            {
                //Different baud rates, close and re-open with new baud rate
                pDevice->Close();
                pSettingsHandle->SetConfigOption(ACTIVE_BAUD, pSettingsHandle->GetConfigOption(BOOTLOADER_BAUD));
                if (pDevice->Open() == false)
                {
                    UpdateFailed(EXIT_CODE_SERIAL_PORT_REOPEN_FAILED);
                    return;
                }
            }
        }

        //Module is ready
        bResentFirstBootloaderCommand = false;
        elptmrUpgradeTime.start();
        nCMode = MODE_BOOTLOADER_VERSION;
        CSubMode = SUBMODE_NONE;
        pDevice->Transmit(COMMAND_BOOTLOADER_VERSION);
        if (nVerbosity >= VERBOSITY_COMMANDS)
        {
            qDebug() << COMMAND_BOOTLOADER_VERSION;
        }
        tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
    }
    else
    {
        //Device is not yet ready
        ++nDeviceReadyChecks;
        if (nDeviceReadyChecks > DEVICE_READY_CHECKS_BEFORE_FAILING)
        {
            //Timeout
            tmrDeviceReadyTimer->stop();
            disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceReadyTimerTimeout()));
            delete tmrDeviceReadyTimer;
            tmrDeviceReadyTimer = NULL;
            UpdateFailed(EXIT_CODE_CTS_TIMEOUT);
        }
    }
}

//=============================================================================
// Callback when the CTS timer has elapsed (for resetting module)
//=============================================================================
void
LrdFwUpd::DeviceRebootReadyTimerTimeout(
    )
{
    //Check CTS status
    if (pDevice->DeviceReady() == true)
    {
        //Device is now ready
        tmrDeviceReadyTimer->stop();
        disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceRebootReadyTimerTimeout()));
        delete tmrDeviceReadyTimer;
        tmrDeviceReadyTimer = NULL;

        //Close the port used for the BREAK process
        pDevice->Close();

        //Continue with the update setup process
        ContinueUpgrade();
    }
    else
    {
        //Device is not yet ready
        ++nDeviceReadyChecks;
        if (nDeviceReadyChecks > DEVICE_READY_CHECKS_BEFORE_FAILING)
        {
            //Timeout
            tmrDeviceReadyTimer->stop();
            disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceRebootReadyTimerTimeout()));
            delete tmrDeviceReadyTimer;
            tmrDeviceReadyTimer = NULL;
            pDevice->Close();
            UpdateFailed(EXIT_CODE_CTS_TIMEOUT);
        }
    }
}

//=============================================================================
// Returns true if an update is in progress
//=============================================================================
bool LrdFwUpd::IsUpdateInProgress()
{
    return pUwfData->IsOpen();
}

//=============================================================================
// Processes target platform commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandTargetPlatform(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength != UWF_TARGET_PLATFORM_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct target platform packet
    QByteArray baTargetData = COMMAND_TARGET_PLATFORM;
    baTargetData.append(pUwfData->Read(nLength));
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint32_t nTargetID = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, 1, nTargetID);
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tTarget - ID: ").append(QString::number(nTargetID, 16)));

    nCMode = MODE_PLATFORM_COMMAND;
    pDevice->Transmit(baTargetData);
    if (nVerbosity >= VERBOSITY_COMMANDS)
    {
        qDebug() << baTargetData;
    }

    return FUNCTION_RETURN_CODE_SUCCESS_DONE;
}

//=============================================================================
// Processes register device commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandRegisterDevice(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength != UWF_REGISTER_DEVICE_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct register device packet
    QByteArray baTargetData = pUwfData->Read(nLength);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint8_t nHandle = (uint8_t)baTargetData[UWF_OFFSET_REGISTER_HANDLE];
    uint32_t nBaseAddr = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_REGISTER_BASE_ADDRESS, nBaseAddr);
    uint8_t nBanks = (uint8_t)baTargetData[UWF_OFFSET_REGISTER_BANKS];
    uint32_t nBankSize = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_REGISTER_BANK_SIZE, nBankSize);
    uint8_t nBankSelection = (uint8_t)baTargetData[UWF_OFFSET_REGISTER_BANK_SELECTION];
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tRegister - Handle: ").append(QString::number(nHandle)).append(", Base address: 0x").append(QString::number(nBaseAddr, 16)).append(", Banks: ").append(QString::number(nBanks)).append(", Selection: ").append(QString::number(nBankSelection)));

    if (lstDevices.isEmpty())
    {
        //Select first device by default
        nActiveDevice = 0;
        nActiveBank = 0;
    }

    DeviceStruct *device = new DeviceStruct();
    MallocFailCheck(device);
    device->nHandle = nHandle;
    device->nBaseAddr = nBaseAddr;
    device->nBanks = nBanks;
    device->nBankSize = nBankSize;
    device->nBankSelection = nBankSelection;
    lstDevices.append(device);

    return FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
}

//=============================================================================
// Processes select device commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandSelectDevice(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength != UWF_SELECT_DEVICE_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct select device packet
    QByteArray baTargetData = pUwfData->Read(nLength);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint8_t nFlash = (uint8_t)baTargetData[UWF_OFFSET_SELECT_FLASH];
    uint8_t nBank = (uint8_t)baTargetData[UWF_OFFSET_SELECT_BANK];
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tSelect - Flash: ").append(QString::number(nFlash)).append(", Bank: ").append(QString::number(nBank)));

    nActiveDevice = nFlash;
    nActiveBank = nBank;

    uint8_t i = 0;
    while (i < lstDevices.count())
    {
        if (lstDevices.at(i)->nHandle == nActiveDevice)
        {
            //Found it, select it
            nActiveDeviceIndex = i;
            break;
        }
        ++i;
    }

    return FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
}

//=============================================================================
// Processes sector map commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandSectorMap(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength < UWF_SECTOR_MAP_LENGTH || (nLength % UWF_SECTOR_MAP_LENGTH) != 0)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct sector map packet
    QByteArray baTargetData = pUwfData->Read(nLength);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint32_t nSectors = 0;
    uint32_t nSectorSize = 0;
    uint8_t nCurrentPosition = 0;
    emit CurrentAction(MODULE_UPDATE, 0, "\tSector Map:");

    while (nCurrentPosition < nLength)
    {
        //Get the number of sectors and sector size
        ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_SECTOR_MAP_SECTORS + nCurrentPosition, nSectors);
        ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_SECTOR_MAP_SECTOR_SIZE + nCurrentPosition, nSectorSize);

        //Add the sectors to the internal map
        SectorStruct *pSS = new SectorStruct();
        if (nCurrentPosition == 0)
        {
            //This is the default
            nActiveSectorSize = nSectorSize;
            pSS->nOffset = lstDevices[nActiveDevice]->nBaseAddr;
        }
        else
        {
            //This is an additional mapping
            pSS->nOffset = lstSectorMap.last()->nOffset + lstSectorMap.last()->nTotalSize;
        }
        pSS->nSectors = nSectors;
        pSS->nSectorSize = nSectorSize;
        pSS->nTotalSize = nSectors * nSectorSize;
        lstSectorMap.append(pSS);

        //Output details
        emit CurrentAction(MODULE_UPDATE, 0, QString("\t\tSectors: ").append(QString::number(nSectors)).append(", Sector size: 0x").append(QString::number(nSectorSize, 16)));

        //Next sector
        nCurrentPosition += UWF_SECTOR_MAP_LENGTH;
    }

    return FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
}

//=============================================================================
// Processes erase block commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandEraseBlock(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength != UWF_ERASE_BLOCK_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct erase packet
    QByteArray baTargetData = pUwfData->Read(UWF_ERASE_BLOCK_LENGTH);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint32_t nOffset = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_ERASE_OFFSET, nOffset);
    uint32_t nSize = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_ERASE_SIZE, nSize);

    //Start erase process
    nEraseStart = lstDevices[nActiveDeviceIndex]->nBaseAddr + nOffset;
    nEraseSize = nSize;
    nEraseWholeSize = nSize;
    nCMode = MODE_ERASE_COMMAND;
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tErase - Offset: 0x").append(QString::number(nOffset, 16)).append(", Address: 0x").append(QString::number(nEraseStart, 16)).append(", Size: 0x").append(QString::number(nSize, 16)));
    if (nActiveEraseLengthCmd > 0)
    {
        //Use new version of command with size specifier
        uint8_t i = 0;
        while (i < lstEraseSizes.count())
        {
            if (lstEraseSizes.at(i) >= nEraseSize)
            {
                break;
            }
            ++i;
        }

        if (i >= lstEraseSizes.count())
        {
            //Cap erase size
            i = lstEraseSizes.count()-1;
        }

        //Send erase command
        QByteArray baAddr = COMMAND_ERASE_SECTION;
        ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nEraseStart);
        baAddr.append((uint8_t)i);
        pDevice->Transmit(baAddr);
        if (nVerbosity >= VERBOSITY_COMMANDS)
        {
            qDebug() << baAddr;
        }

        //Update debug output
        emit CurrentAction(MODULE_UPDATE, 0, QString("Erasing 0x").append(QString::number(nEraseStart, 16)).append(" - 0x").append(QString::number(nEraseStart + lstEraseSizes.at(i), 16)));

        //Increment the current erase position
        nEraseStart += lstEraseSizes.at(i);
        if (nEraseSize < lstEraseSizes.at(i))
        {
            //Final erase message
            nEraseSize = 0;
        }
        else
        {
            //Decrement size left to erase
            nEraseSize -= lstEraseSizes.at(i);
        }
    }
    else
    {
        //Use classic command without size specified
        QByteArray baAddr = COMMAND_ERASE_SECTION;
        ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nEraseStart);
        pDevice->Transmit(baAddr);
        if (nVerbosity >= VERBOSITY_COMMANDS)
        {
                qDebug() << baAddr;
        }

        //Find which sector map this erase section applies to
        uint8_t i = 0;
        while (i < lstSectorMap.length())
        {
            if (nEraseStart >= lstSectorMap[i]->nOffset && nEraseStart < (lstSectorMap[i]->nOffset + lstSectorMap[i]->nTotalSize))
            {
                //Correct sector found
                break;
            }
            ++i;
        }

        if (i == lstSectorMap.length())
        {
            //No match found in sector map
            return FUNCTION_RETURN_CODE_SECTOR_MAPPING_NOT_FOUND;
        }

        nActiveSectorSize = lstSectorMap[i]->nSectorSize;
        nActiveEraseSectorLeft = lstSectorMap[i]->nSectors - ((nEraseStart - lstSectorMap[i]->nOffset) / nActiveSectorSize);

        //Update debug output
        emit CurrentAction(MODULE_UPDATE, 0, QString("Erasing 0x").append(QString::number(nEraseStart, 16)).append(" - 0x").append(QString::number(nEraseStart + nActiveSectorSize, 16)));

        //Increment the current erase position
        nEraseStart += nActiveSectorSize;
        --nActiveEraseSectorLeft;
        if (nEraseSize < nActiveSectorSize)
        {
            //Final erase message
            nEraseSize = 0;
        }
        else
        {
            //Decrement size left to erase
            nEraseSize -= nActiveSectorSize;
        }
    }

    return FUNCTION_RETURN_CODE_SUCCESS_DONE;
}

//=============================================================================
// Processes write block commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandWriteBlock(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength < UWF_WRITE_BLOCK_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct write data packet
    QByteArray baTargetData = pUwfData->Read(UWF_WRITE_BLOCK_LENGTH);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint32_t nOffset = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_WRITE_OFFSET, nOffset);
    uint32_t nFlags = 0;
    ENDIAN_FLIP_BYTEARRAY_TO_UI32(baTargetData, UWF_OFFSET_WRITE_FLAGS, nFlags);

    //Extract the data
    nWriteStart = lstDevices[nActiveDeviceIndex]->nBaseAddr + nOffset;
    nWriteSize = nLength - UWF_WRITE_BLOCK_LENGTH;
    nWriteWholeSize = nWriteSize;
    nCMode = MODE_WRITE_COMMAND;
    CSubMode = SUBMODE_WRITE_DATA;
    nDataSize = nActiveWriteSize;
    if (nDataSize > nWriteSize)
    {
        //Limit data size
        nDataSize = nWriteSize;
    }
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tWrite - Offset: 0x").append(QString::number(nOffset, 16)).append(", Address: 0x").append(QString::number(nWriteStart, 16)).append(", Flags: 0x").append(QString::number(nFlags, 16)).append(", Size: 0x").append(QString::number((nLength - 8), 16)));

    //Check if verification is enabled
    bVerifyActive = pSettingsHandle->GetConfigOption(VERIFY_DATA).toBool();
    if (bVerifyActive == true)
    {
        //Reset verification variables and set first adddress
        nVerifyChecksum = 0;
        nVerifyAddress = nWriteStart;
        nVerifySize = 0;
    }

    //Send write address command
    QByteArray baAddr = COMMAND_WRITE_SECTION;
    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nWriteStart);
    if (nActiveWriteLengthCmd == FUP_LENGTH_4BYTE)
    {
        //4-byte data size field
        ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nDataSize);
    }
    else if (nActiveWriteLengthCmd == FUP_LENGTH_2BYTE)
    {
        //2-byte data size field
        ENDIAN_FLIP_UI16_TO_BYTEARRAY(baAddr, nDataSize);
    }
    else if (nActiveWriteLengthCmd == FUP_LENGTH_1BYTE)
    {
        //1-byte data size field
        baAddr.append((uint8_t)nDataSize);
    }
    pDevice->Transmit(baAddr);
    if (nVerbosity >= VERBOSITY_COMMANDS)
    {
        qDebug() << baAddr;
    }

    return FUNCTION_RETURN_CODE_SUCCESS_DONE;
}

//=============================================================================
// Processes unregister commands
//=============================================================================
int8_t
LrdFwUpd::ProcessCommandUnregister(
    uint32_t nLength
    )
{
    //Check length of command
    if (nLength < UWF_UNREGISTER_DEVICE_LENGTH)
    {
        //Packet length is not valid
        return FUNCTION_RETURN_CODE_INVALID_LENGTH;
    }

    //Read in data and construct unregister device packet
    QByteArray baTargetData = pUwfData->Read(nLength);
    emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
    uint8_t nHandle = (uint8_t)baTargetData[UWF_OFFSET_UNREGISTER_HANDLE];
    emit CurrentAction(MODULE_UPDATE, 0, QString("\tUnregister - Handle: ").append(QString::number(nHandle)));

    uint8_t i = 0;
    while (i < lstDevices.count())
    {
        if (lstDevices.at(i)->nHandle == nHandle)
        {
            //Found it, remove it
            DeviceStruct *device = lstDevices.at(i);
            lstDevices.removeAt(i);
            delete device;
            break;
        }
        ++i;
    }

    return FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
}

//=============================================================================
// Cleans up when an update fails
//=============================================================================
void
LrdFwUpd::UpdateFailed(
    int32_t nErrorCode
    )
{
    //Send information back up
    qint64 nUpgradeTime = 0;
    if (elptmrUpgradeTime.isValid())
    {
        //Get upgrade time
        nUpgradeTime = elptmrUpgradeTime.elapsed();
        elptmrUpgradeTime.invalidate();
        emit CurrentAction(MODULE_UPDATE, 0, QString("Firmware upgrade failed, total time: ").append(QString::number(nUpgradeTime)).append("ms"));
    }
    else
    {
        //Do not include time
        emit CurrentAction(MODULE_UPDATE, 0, QString("Firmware upgrade failed."));
    }
    emit Error(MODULE_UPDATE, nErrorCode);
    CleanUp(false);

    //Signal parent
    emit Finished(false, nUpgradeTime);
}

//=============================================================================
// Loads the next packet from the uwf file
//=============================================================================
void
LrdFwUpd::NextPacket(
    )
{
    //Process the next packet from the upgrade file
    QByteArray baPktHeader;
    int8_t nStatus = FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
    while (nStatus == FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET)
    {
        //Assume no more loops required
        nStatus = FUNCTION_RETURN_CODE_SUCCESS_DONE;

        if (pUwfData->AtEnd())
        {
            //Upgrade has finished, reset module
            nCMode = MODE_RESET;
            CSubMode = SUBMODE_NONE;

            //Stop command timeout timer
            tmrCommandTimeoutTimer->stop();

            if (pSettingsHandle->GetConfigOption(REBOOT_MODULE_AFTER_UPDATE) == false)
            {
                //Do not reboot module
                qint64 nUpgradeTime = elptmrUpgradeTime.elapsed();
                elptmrUpgradeTime.invalidate();
                emit CurrentAction(MODULE_UPDATE, 0, QString("Firmware upgrade completed in ").append(QString::number(nUpgradeTime)).append("ms (module left in bootloader mode at ").append(QString::number(lstUARTSpeeds.at(nChosenBaudRateIndex-1))).append(" baud)"));
                CleanUp(true);

                //Signal parent
                emit Finished(true, nUpgradeTime);

                return;
            }

            //Configure restart timer
            tmrRestartTimer = new QTimer();
            MallocFailCheck(tmrRestartTimer);
            tmrRestartTimer->setInterval(RESTART_TIMER_PERIOD_MS);
            tmrRestartTimer->setSingleShot(false);
            connect(tmrRestartTimer, SIGNAL(timeout()), this, SLOT(RestartTimerTimeout()));

            if (bNewBootloader == true)
            {
                //New version of bootloader - send restart command
                pDevice->Transmit(COMMAND_REBOOT_MODULE);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << COMMAND_REBOOT_MODULE;
                }
            }
            else
            {
                //Old version of bootloader - reset via BREAK
                pDevice->SetBreak(true);
                CSubMode = SUBMODE_RESET_VIA_BREAK;
            }

            //Start the restart timer
            tmrRestartTimer->start();
            return;
        }

        //Restart command timeout timer
        tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);

        //Read in the data for the next packet
        baPktHeader = pUwfData->Read(UWF_COMMAND_HEADER_LENGTH);
        emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
        uint8_t nCmdID = (uint8_t)baPktHeader[UWF_OFFSET_HEADER_COMMAND_ID];
        uint32_t nPktLen = 0;
        ENDIAN_FLIP_BYTEARRAY_TO_UI32(baPktHeader, UWF_OFFSET_HEADER_PACKET_LENGTH, nPktLen);
        emit CurrentAction(MODULE_UPDATE, 0, QString("Pkt: ").append(QString::number(nCmdID)).append(" | ").append((char)nCmdID).append(", Len: ").append(QString::number(nPktLen)));
        if (nCmdID == UWF_COMMAND_TARGET_PLATFORM)
        {
            //Target platform
            nStatus = ProcessCommandTargetPlatform(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_REGISTER)
        {
            //Register device
            nStatus = ProcessCommandRegisterDevice(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_SELECT)
        {
            //Select device
            nStatus = ProcessCommandSelectDevice(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_SECTOR_MAP)
        {
            //Sector map
            nStatus = ProcessCommandSectorMap(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_ERASE)
        {
            //Erase
            nStatus = ProcessCommandEraseBlock(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_WRITE)
        {
            //Write
            nStatus = ProcessCommandWriteBlock(nPktLen);
        }
        else if (nCmdID == UWF_COMMAND_UNREGISTER)
        {
            //Unregister
            nStatus = ProcessCommandUnregister(nPktLen);
        }
        else
        {
            //Unknown command
            emit CurrentAction(MODULE_UPDATE, 0, QString("Unknown command encountered: ").append((char)nCmdID));
            pUwfData->Read(nPktLen);
            emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
            nStatus = FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET;
        }
    }

    if (nStatus < FUNCTION_RETURN_CODE_SUCCESS_DONE)
    {
        //Command execution failure
        UpdateFailed((nStatus == FUNCTION_RETURN_CODE_SECTOR_MAPPING_NOT_FOUND ? EXIT_CODE_ERASE_SECTOR_MAPPING_NOT_FOUND : EXIT_CODE_RETURN_CODE_ERROR));
    }
}

//=============================================================================
// Validates that an upgrade file is valid
//=============================================================================
bool
LrdFwUpd::ValidateUpgradeFile(
    )
{
    QByteArray baPktHeader;
    bool bFailed = false;
    while (!pUwfData->AtEnd() && bFailed == false)
    {
        //Read in the data for the next packet
        baPktHeader = pUwfData->Read(UWF_COMMAND_HEADER_LENGTH);
        uint8_t nCmdID = (uint8_t)baPktHeader[UWF_OFFSET_HEADER_COMMAND_ID];
        uint32_t nPktLen = 0;
        ENDIAN_FLIP_BYTEARRAY_TO_UI32(baPktHeader, UWF_OFFSET_HEADER_PACKET_LENGTH, nPktLen);
        if (nCmdID == UWF_COMMAND_TARGET_PLATFORM && nPktLen != UWF_TARGET_PLATFORM_LENGTH)
        {
            //Target platform command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Target platform command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_REGISTER && nPktLen != UWF_REGISTER_DEVICE_LENGTH)
        {
            //Register device command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Register device command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_SELECT && nPktLen != UWF_SELECT_DEVICE_LENGTH)
        {
            //Select device command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Select device command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_SECTOR_MAP && (nPktLen < UWF_SECTOR_MAP_LENGTH || (nPktLen % UWF_SECTOR_MAP_LENGTH) != 0))
        {
            //Sector map command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Sector map command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_ERASE && nPktLen != UWF_ERASE_BLOCK_LENGTH)
        {
            //Erase command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Erase command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_WRITE && nPktLen < UWF_WRITE_BLOCK_LENGTH)
        {
            //Write command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Write command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID == UWF_COMMAND_UNREGISTER && nPktLen < UWF_UNREGISTER_DEVICE_LENGTH)
        {
            //Unregister command with invalid size
            emit CurrentAction(MODULE_UPDATE, 0, QString("Unregister command has length 0x").append(QString::number(nPktLen, 16)).append(" and is not valid."));
            bFailed = true;
        }
        else if (nCmdID != UWF_COMMAND_TARGET_PLATFORM && nCmdID != UWF_COMMAND_REGISTER && nCmdID != UWF_COMMAND_SELECT && nCmdID != UWF_COMMAND_SECTOR_MAP && nCmdID != UWF_COMMAND_ERASE && nCmdID != UWF_COMMAND_WRITE && nCmdID != UWF_COMMAND_UNREGISTER )
        {
            //Unknown command
            emit CurrentAction(MODULE_UPDATE, 0, QString("Selected upgrade file has unknown command 0x").append(QString::number(nCmdID, 16)).append(" and is not valid."));
            emit Error(MODULE_UPDATE, EXIT_CODE_UWF_FILE_COMMAND_INVALID);
            bFailed = true;
        }
        else
        {
            //Valid command, skip data for this command to process next command
            pUwfData->Seek(SEEK_CURRENT, nPktLen);
        }
    }

    return bFailed;
}

//=============================================================================
// Starts process of requesting supported functions from bootloader
//=============================================================================
bool
LrdFwUpd::SupportedFunctions(
    )
{
    //Get supported options
    nCMode = MODE_SUPPORTED_FUNCTIONS;
    pDevice->Transmit(COMMAND_SUPPORTED_FEATURES);
    if (nVerbosity >= VERBOSITY_COMMANDS)
    {
        qDebug() << COMMAND_SUPPORTED_FEATURES;
    }
    return true;
}

//=============================================================================
// Starts process of requesting supported options from bootloader
//=============================================================================
bool
LrdFwUpd::SupportedOptions(
    )
{
    //Get supported options
    nCMode = MODE_SUPPORTED_OPTIONS;
    CSubMode = SUBMODE_QUERY_MAX_ERASE_LENGTH;

    //Check for error
    QByteArray baTmp = COMMAND_SETTINGS_QUERY;
    ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_ERASE_LEN_BYTES);
    baTmp.append((ByteArrayType)0x00);
    pDevice->Transmit(baTmp);
    if (nVerbosity >= VERBOSITY_COMMANDS)
    {
        qDebug() << baTmp;
    }
    return true;
}

//=============================================================================
//=============================================================================
void
LrdFwUpd::ModuleDataReceived(
    QByteArray *baOrigData
    )
{
    baReceivedData.append(*baOrigData);
    uint16_t nRemoveBytes = 0;

    //Check which mode is active
    if (nCMode == MODE_PLATFORM_COMMAND)
    {
        //Target ID
        if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ACKNOWLEDGE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ACKNOWLEDGE)
        {
            //Valid device
            nCMode = MODE_IDLE;
            if (bNewBootloader == true)
            {
                //New bootloader: get supported options
                tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
                SupportedFunctions();
            }
            else
            {
                //Old bootloader: continue with update
                NextPacket();
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_ACKNOWLEDGE;
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Error
            UpdateFailed(baReceivedData.at(1));
            return;
        }
        else
        {
            //Unknown
        }
    }
    else if (nCMode == MODE_ENTER_BOOTLOADER)
    {
        //Dump any response received
        nRemoveBytes = baReceivedData.length();
    }
    else if (nCMode == MODE_ERASE_COMMAND)
    {
        //Erase
        if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ACKNOWLEDGE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ACKNOWLEDGE)
        {
            //Erased successfully
            tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
            if (nEraseSize > 0)
            {
                if (nActiveEraseLengthCmd > 0)
                {
                    //Use new version of command with size specifier
                    uint8_t i = 0;
                    while (i < lstEraseSizes.count())
                    {
                        //Find a suitable erase size for this section
                        if (lstEraseSizes.at(i) > nEraseSize)
                        {
                            break;
                        }
                        ++i;
                    }

                    //Use next lower size, if available
                    if (i > 0)
                    {
                        --i;
                    }

                    //Send erase command
                    QByteArray baAddr = COMMAND_ERASE_SECTION;
                    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nEraseStart);
                    baAddr.append((uint8_t)i);
                    pDevice->Transmit(baAddr);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baAddr;
                    }

                    //Update log
                    emit CurrentAction(MODULE_UPDATE, 0, QString("Erasing 0x").append(QString::number(nEraseStart, 16)).append(" - 0x").append(QString::number(nEraseStart + lstEraseSizes.at(i), 16)));

                    //Increment the current erase position
                    nEraseStart += lstEraseSizes.at(i);
                    if (nEraseSize < lstEraseSizes.at(i))
                    {
                        //Final erase message
                        nEraseSize = 0;
                    }
                    else
                    {
                        //Decrement size left to erase
                        nEraseSize -= lstEraseSizes.at(i);
                    }
                }
                else
                {
                    //Use classic command without size specified
                    QByteArray baAddr = COMMAND_ERASE_SECTION;
                    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nEraseStart);
                    pDevice->Transmit(baAddr);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baAddr;
                    }

                    if (nActiveEraseSectorLeft == 0)
                    {
                        //Sector mapping has finished, find which sector map the next part of this erase section applies to
                        uint8_t i = 0;
                        while (i < lstSectorMap.length())
                        {
                            if (nEraseStart >= lstSectorMap[i]->nOffset && nEraseStart < (lstSectorMap[i]->nOffset + lstSectorMap[i]->nTotalSize))
                            {
                                //Correct sector found
                                break;
                            }
                            ++i;
                        }

                        if (i == lstSectorMap.length())
                        {
                            //No match found in sector map
                            UpdateFailed(EXIT_CODE_ERASE_SECTOR_MAPPING_NOT_FOUND);
                            return;
                        }

                        //Set the sector values
                        nActiveSectorSize = lstSectorMap[i]->nSectorSize;
                        nActiveEraseSectorLeft = lstSectorMap[i]->nSectors - ((nEraseStart - lstSectorMap[i]->nOffset) / nActiveSectorSize);
                    }

                    //Update log
                    emit CurrentAction(MODULE_UPDATE, 0, QString("Erasing 0x").append(QString::number(nEraseStart, 16)).append(" - 0x").append(QString::number(nEraseStart + nActiveSectorSize, 16)));

                    //Increment the current erase position
                    nEraseStart += nActiveSectorSize;
                    --nActiveEraseSectorLeft;
                    if (nEraseSize < nActiveSectorSize)
                    {
                        //Final erase message
                        nEraseSize = 0;
                    }
                    else
                    {
                        //Decrement size left to erase
                        nEraseSize -= nActiveSectorSize;
                    }
                }
                emit PercentComplete(100 - ((nEraseSize * 100) / nEraseWholeSize), -1);
            }
            else
            {
                nCMode = MODE_IDLE;
                NextPacket();
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_ACKNOWLEDGE;
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Error
            UpdateFailed(baReceivedData.at(1));
        }
        else
        {
            //Unknown
        }
    }
    else if (nCMode == MODE_WRITE_COMMAND)
    {
        if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ACKNOWLEDGE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ACKNOWLEDGE)
        {
            tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
            if (CSubMode == SUBMODE_WRITE_DATA)
            {
                //Wrote address, write data
                nWriteStart += nDataSize;
                nWriteSize -= nDataSize;

                emit PercentComplete(100 - ((nWriteSize * 100) / nWriteWholeSize), -1);

                //Create data section packet
                QByteArray baTmpDat;
                baTmpDat.append(COMMAND_DATA_SECTION);
                baTmpDat.append(pUwfData->Read(nDataSize));
                emit PercentComplete(-1, (pUwfData->CurrentPosition() * 100) / nFileSize);
                uint16_t i = 0;
                uint32_t nChecksum = 0;
                CSubMode = SUBMODE_WRITE_ADDRESS;

                //Generate a checksum
                while (i < nDataSize)
                {
                    nChecksum += (uint8_t)baTmpDat[i+1];
                    ++i;
                }

                if (bVerifyActive == true)
                {
                    //Append checksum and increase size of verification section
                    nVerifySize += nDataSize;
                    nVerifyChecksum += nChecksum;

                    if ((nVerifySize + nDataSize) > FUP_VERIFY_COMMAND_MAXIMUM_SIZE)
                    {
                        //The verification size limit has been reached, the next state should be to verify the data on the module
                        CSubMode = SUBMODE_VERIFY_DATA;
                    }
                }

                //Append checksum
                if (nActiveChecksumLengthCmd == FUP_LENGTH_4BYTE)
                {
                    //32-bit checksum (4 bytes)
                    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baTmpDat, nChecksum);
                }
                else if (nActiveChecksumLengthCmd == FUP_LENGTH_2BYTE)
                {
                    //16-bit checksum (2 bytes)
                    ENDIAN_FLIP_UI16_TO_BYTEARRAY(baTmpDat, nChecksum);
                }
                else if (nActiveChecksumLengthCmd == FUP_LENGTH_1BYTE)
                {
                    //8-bit checksum (1 byte)
                    baTmpDat.append((uint8_t)nChecksum);
                }
                pDevice->Transmit(baTmpDat);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmpDat;
                }
                if (nVerbosity >= VERBOSITY_MODES)
                {
                    emit CurrentAction(MODULE_UPDATE, 0, QString("Got: ").append(QString::number((uint8_t)nChecksum, 16)));
                    emit CurrentAction(MODULE_UPDATE, 0, QString("SubMode ").append(QString::number(SUBMODE_WRITE_DATA)).append(", ").append(QString::number(nDataSize)).append(", ").append(QString::number(nWriteStart)).append(", ").append(QString::number(nWriteSize)).append(", ").append(QString::number(nChecksum)));
                }

                if (nVerbosity >= VERBOSITY_TIMEOUTS)
                {
                    qDebug() << "Timeout timer set to " << (nDataSize / (pSettingsHandle->GetConfigOption(ACTIVE_BAUD).toULongLong() / SERIAL_BITS_PER_BYTE)) * 100 * 1000 / SERIAL_TIMEOUT_SPREAD_FACTOR + COMMAND_TIMEOUT_PERIOD_MS;
                }
                tmrCommandTimeoutTimer->start((nDataSize / (pSettingsHandle->GetConfigOption(ACTIVE_BAUD).toULongLong() / SERIAL_BITS_PER_BYTE) * 100 / SERIAL_TIMEOUT_SPREAD_FACTOR) * 1000 + COMMAND_TIMEOUT_PERIOD_MS);
            }
            else if (CSubMode == SUBMODE_VERIFY_DATA)
            {
                //Wrote data, verify data
                CSubMode = SUBMODE_WRITE_ADDRESS;

                //Create data section packet
                QByteArray baTmpDat = COMMAND_VERIFY_SECTION;
                ENDIAN_FLIP_UI32_TO_BYTEARRAY(baTmpDat, nVerifyAddress);
                ENDIAN_FLIP_UI32_TO_BYTEARRAY(baTmpDat, nVerifySize);

                //Add the checksum
                if (nActiveVerifyChecksumLengthCmd == FUP_LENGTH_4BYTE)
                {
                    //32-bit checksum (4 bytes)
                    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baTmpDat, nVerifyChecksum);
                }
                else if (nActiveVerifyChecksumLengthCmd == FUP_LENGTH_2BYTE)
                {
                    //16-bit checksum (2 bytes)
                    ENDIAN_FLIP_UI16_TO_BYTEARRAY(baTmpDat, nVerifyChecksum);
                }
                else if (nActiveVerifyChecksumLengthCmd == FUP_LENGTH_1BYTE)
                {
                    //8-bit checksum (1 byte)
                    baTmpDat.append((uint8_t)nVerifyChecksum);
                }
                pDevice->Transmit(baTmpDat);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmpDat;
                }
                if (nVerbosity >= VERBOSITY_MODES)
                {
                    emit CurrentAction(MODULE_UPDATE, 0, QString("SubMode ").append(QString::number(SUBMODE_VERIFY_DATA)).append(", ").append(QString::number(nVerifyAddress)).append(", ").append(QString::number(nVerifySize)).append(", ").append(QString::number(nVerifyChecksum)));
                }

                //Reset variables for next checksum
                nVerifyAddress += nVerifySize;
                nVerifySize = 0;
                nVerifyChecksum = 0;
            }
            else if (CSubMode == SUBMODE_WRITE_ADDRESS)
            {
                //Wrote data, write address
                if (nDataSize > 0)
                {
                    CSubMode = SUBMODE_WRITE_DATA;
                    nDataSize = nActiveWriteSize;
                    if (nDataSize > nWriteSize)
                    {
                        nDataSize = nWriteSize;
                    }

                    QByteArray baAddr = COMMAND_WRITE_SECTION;
                    ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nWriteStart);
                    if (nActiveWriteLengthCmd == FUP_LENGTH_4BYTE)
                    {
                        //4-byte data size field
                        ENDIAN_FLIP_UI32_TO_BYTEARRAY(baAddr, nDataSize);
                    }
                    else if (nActiveWriteLengthCmd == FUP_LENGTH_2BYTE)
                    {
                        //2-byte data size field
                        ENDIAN_FLIP_UI16_TO_BYTEARRAY(baAddr, nDataSize);
                    }
                    else if (nActiveWriteLengthCmd == FUP_LENGTH_1BYTE)
                    {
                        //1-byte data size field
                        baAddr.append((uint8_t)nDataSize);
                    }
                    pDevice->Transmit(baAddr);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baAddr;
                    }
                    if (nVerbosity >= VERBOSITY_MODES)
                    {
                        emit CurrentAction(MODULE_UPDATE, 0, QString("SubMode ").append(QString::number(SUBMODE_WRITE_ADDRESS)).append(", ").append(QString::number(nDataSize)).append(", ").append(QString::number(nWriteStart)).append(", ").append(QString::number(nWriteSize)));
                    }
                }
                else
                {
                    nCMode = MODE_IDLE;
                    CSubMode = SUBMODE_NONE;
                    NextPacket();
                }
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_ACKNOWLEDGE;
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Error
            UpdateFailed(baReceivedData.at(sizeof(FUP_RESPONSE_ERROR)));
        }
        else if (CSubMode == SUBMODE_WRITE_ADDRESS && baReceivedData.length() >= sizeof(FUP_RESPONSE_NOT_ACKNOWLEDGE) && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_NOT_ACKNOWLEDGE)
        {
            //Verification failure
            emit CurrentAction(MODULE_UPDATE, 0, QString("Features: ").append(QString::number((uint8_t)baReceivedData[1], 16)).append(QString::number((uint8_t)baReceivedData[2], 16)).append(QString::number((uint8_t)baReceivedData[3], 16)).append(QString::number((uint8_t)baReceivedData[4], 16)).append(QString::number((uint8_t)baReceivedData[5], 16)).append(QString::number((uint8_t)baReceivedData[6], 16)).append(QString::number((uint8_t)baReceivedData[7], 16)).append(QString::number((uint8_t)baReceivedData[8], 16)));
            UpdateFailed(EXIT_CODE_BOOTLOADER_VERIFICATION_FAILED);
        }
        else
        {
            //Unknown
        }
    }
    else if (nCMode == MODE_RESET)
    {
        //
        if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Restart commmand not supported
            pDevice->SetBreak(true);
            CSubMode = SUBMODE_RESET_VIA_BREAK;
            tmrRestartTimer->start();

            nRemoveBytes = FUP_RESPONSE_LENGTH_ERROR;
        }
    }
    else if (nCMode == MODE_BOOTLOADER_VERSION)
    {
        //Bootloader version response
        if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_VERSION && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_VERSION)
        {
            //Version response received
            emit CurrentAction(MODULE_UPDATE, 0, QString("Bootloader version ").append(baReceivedData.mid(sizeof(FUP_RESPONSE_VERSION))));

            if (baReceivedData.at(sizeof(FUP_RESPONSE_VERSION)) >= FUP_EXTENDED_VERSION_NUMBER && pSettingsHandle->GetConfigOption(BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE) == false)
            {
                //New bootloader
                bNewBootloader = true;
            }
            else
            {
                //Old bootloader (or forced to classic mode)
                bNewBootloader = false;
            }

            //
            NextPacket();
        }

        nRemoveBytes = baReceivedData.length();
    }
    else if (nCMode == MODE_SUPPORTED_FUNCTIONS)
    {
        //Bootloader queries
        if (baReceivedData.length() == FUP_RESPONSE_LENGTH_FEATURES_SUPPORTED && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_SUPPORTED_FEATURES)
        {
            //Received option
            emit CurrentAction(MODULE_UPDATE, 0, QString("Features: ").append(QString::number((uint8_t)baReceivedData[1], 16)).append(QString::number((uint8_t)baReceivedData[2], 16)).append(QString::number((uint8_t)baReceivedData[3], 16)).append(QString::number((uint8_t)baReceivedData[4], 16)).append(QString::number((uint8_t)baReceivedData[5], 16)).append(QString::number((uint8_t)baReceivedData[6], 16)).append(QString::number((uint8_t)baReceivedData[7], 16)).append(QString::number((uint8_t)baReceivedData[8], 16)));
            nRemoveBytes = baReceivedData.length();

            SupportedOptions();
        }
    }
    else if (nCMode == MODE_SUPPORTED_OPTIONS)
    {
        //Bootloader queries
        if (baReceivedData.length() == FUP_RESPONSE_LENGTH_QUERY_RESPONSE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_BOOTLOADER_QUERY)
        {
            //Received option
            uint32_t nValue = 0;
            ENDIAN_FLIP_BYTEARRAY_TO_UI32(baReceivedData, FUP_OFFSET_BOOTLOADER_QUERY_VALUE, nValue);
            uint8_t nMoreData = (uint8_t)baReceivedData[FUP_OFFSET_BOOTLOADER_QUERY_MORE_DATA];
            tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);

            if (CSubMode == SUBMODE_QUERY_MAX_ERASE_LENGTH)
            {
                //Maximum erase length query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Maximum erase bytes: ").append(QString::number(nValue)));
                nMaxEraseLengthCmd = nValue;

                CSubMode = SUBMODE_QUERY_MAX_WRITE_LENGTH;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_WRITE_LEN_BYTES);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_WRITE_LENGTH)
            {
                //Maximum write length query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Maximum write bytes: ").append(QString::number(nValue)));
                nMaxWriteLengthCmd = nValue;

                CSubMode = SUBMODE_QUERY_MAX_CHECKSUM_LENGTH;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_CHECKSUM_LEN_BYTES);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_CHECKSUM_LENGTH)
            {
                //Maximum checksum length query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Maximum checksum bytes: ").append(QString::number(nValue)));
                nMaxChecksumLengthCmd = nValue;
                CSubMode = SUBMODE_QUERY_MAX_VERIFY_CHECKSUM_LENGTH;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_VERIFY_CHECKSUM_LEN_BYTES);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_VERIFY_CHECKSUM_LENGTH)
            {
                //Maximum checksum length query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Maximum verify checksum bytes: ").append(QString::number(nValue)));
                nMaxVerifyChecksumLengthCmd = nValue;

                CSubMode = SUBMODE_QUERY_MAX_BAUDRATE;

                //Check for error
                //40 - 43 then a0 - a2 then b0 and b3
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_BAUDRATE);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_BAUDRATE)
            {
                //Maximum baudrate query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Max baudrate: ").append(QString::number(nValue)));

                CSubMode = SUBMODE_QUERY_MAX_ERASE_PER_COMMAND;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_ERASE_SIZE_PER_CMD);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_ERASE_PER_COMMAND)
            {
                //Maximum erase bytes size per command query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Max erase bytes per command: ").append(QString::number(nValue)));

                CSubMode = SUBMODE_QUERY_MAX_WRITE_PER_COMMAND;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_WRITE_SIZE_PER_CMD);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_WRITE_PER_COMMAND)
            {
                //Maximum write bytes per command query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Max write bytes per command: ").append(QString::number(nValue)));
                nMaxWriteSize = nValue;
                nActiveWriteSize = nValue;

                CSubMode = SUBMODE_QUERY_MAX_CHECKSUM_PER_COMMAND;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_MAX_CHECKSUM_SIZE_PER_CMD);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_MAX_CHECKSUM_PER_COMMAND)
            {
                //Maximum checksum bytes per command query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Max checksum bytes per command: ").append(QString::number(nValue)));
                nMaxChecksumSize = nValue;

                CSubMode = SUBMODE_QUERY_ERASE_SIZES;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_ERASE_SIZES_PER_CMD);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_ERASE_SIZES)
            {
                //Supported erase sizes query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Number of supported erase sizes: ").append(QString::number(nValue)));

                CSubMode = SUBMODE_QUERY_BAUD_RATES;

                //Check for error
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_SUPPORTED_BAUDRATES);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_QUERY_BAUD_RATES)
            {
                //Supported UART baud rates query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("Number of supported UART baud rates: ").append(QString::number(nValue)));

                CSubMode = SUBMODE_QUERY_SUPPORTED_ERASE_SIZE;

                //Check for error
                nTmpResponseIndex = 1;
                QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_ERASE_SIZES_PER_CMD);
                baTmp.append(nTmpResponseIndex);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
                emit CurrentAction(MODULE_UPDATE, 0, "Supported erase sizes:");
            }
            else if (CSubMode == SUBMODE_QUERY_SUPPORTED_ERASE_SIZE)
            {
                //Single supported erase size query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("\t").append(QString::number(nValue)).append(" (0x").append(QString::number(nValue, 16)).append(")"));
                lstEraseSizes.append(nValue);

                if (nMoreData == FUP_BOOTLOADER_QUERY_MORE_DATA_YES)
                {
                    //More data to get
                    //Check for error
                    ++nTmpResponseIndex;
                    QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                    ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_ERASE_SIZES_PER_CMD);
                    baTmp.append(nTmpResponseIndex);
                    pDevice->Transmit(baTmp);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baTmp;
                    }
                }
                else
                {
                    //Finished getting values
                    CSubMode = SUBMODE_QUERY_SUPPORTED_BAUD_RATE;
                    //Check for error
                    nTmpResponseIndex = 1;
                    QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                    ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_SUPPORTED_BAUDRATES);
                    baTmp.append(nTmpResponseIndex);
                    pDevice->Transmit(baTmp);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baTmp;
                    }
                    emit CurrentAction(MODULE_UPDATE, 0, "Supported UART baud rates:");
                }
            }
            else if (CSubMode == SUBMODE_QUERY_SUPPORTED_BAUD_RATE)
            {
                //Single supported UART baud rate query response
                emit CurrentAction(MODULE_UPDATE, 0, QString("\t").append(QString::number(nValue)));
                lstUARTSpeeds.append(nValue);

                if (nMoreData == FUP_BOOTLOADER_QUERY_MORE_DATA_YES)
                {
                    //More data to get
                    //Check for error
                    ++nTmpResponseIndex;
                    QByteArray baTmp = COMMAND_SETTINGS_QUERY;
                    ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_SUPPORTED_BAUDRATES);
                    baTmp.append(nTmpResponseIndex);
                    pDevice->Transmit(baTmp);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baTmp;
                    }
                }
                else
                {
                    //Finished getting bootloader settings, change settings
                    nCMode = MODE_SET_OPTIONS;
                    CSubMode = SUBMODE_SET_ERASE_LENGTH;

                    if (nMaxEraseLengthCmd == 1)
                    {
                        //Increase erase size to 1 byte
                        nActiveEraseLengthCmd = 1;
                    }
                    else
                    {
                        //Keep erase size as 0 bytes
                        nActiveEraseLengthCmd = 0;
                    }

                    //Create and send command
                    QByteArray baTmp = COMMAND_SETTINGS_SET;
                    ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_CURRENT_ERASE_LEN_BYTES);
                    baTmp.append(nActiveEraseLengthCmd);
                    baTmp.append((ByteArrayType)0x00);
                    baTmp.append((ByteArrayType)0x00);
                    baTmp.append((ByteArrayType)0x00);
                    pDevice->Transmit(baTmp);
                    if (nVerbosity >= VERBOSITY_COMMANDS)
                    {
                        qDebug() << baTmp;
                    }
                }
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_QUERY_RESPONSE;
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Error
            UpdateFailed(baReceivedData.at(sizeof(FUP_OFFSET_ERROR_ERROR_CODE)));
        }
        else
        {
            //Unknown
        }
    }
    else if (nCMode == MODE_SET_OPTIONS)
    {
        //Bootloader setting
        if (baReceivedData.length() == FUP_RESPONSE_LENGTH_ACKNOWLEDGE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ACKNOWLEDGE)
        {
            tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
            if (CSubMode == SUBMODE_SET_ERASE_LENGTH)
            {
                //Erase size has been set
                CSubMode = SUBMODE_SET_WRITE_LENGTH;

                if (nMaxWriteLengthCmd >= 2)
                {
                    //Increase write size to 2 bytes
                    nActiveWriteLengthCmd = 2;
                }
                else
                {
                    //Keep write size as 1 byte
                    nActiveWriteLengthCmd = 1;
                }

                //Create and send packet
                QByteArray baTmp = COMMAND_SETTINGS_SET;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_CURRENT_WRITE_LEN_BYTES);
                baTmp.append(nActiveWriteLengthCmd);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_SET_WRITE_LENGTH)
            {
                //Write size has been set
                CSubMode = SUBMODE_SET_VERIFY_CHECKSUM_LENGTH;

                if (nMaxChecksumLengthCmd >= FUP_LENGTH_4BYTE)
                {
                    //Increase checksum size to 4 bytes
                    nActiveChecksumLengthCmd = FUP_LENGTH_4BYTE;
                }
                else if (nMaxChecksumLengthCmd >= FUP_LENGTH_2BYTE)
                {
                    //Increase checksum size to 2 bytes
                    nActiveChecksumLengthCmd = FUP_LENGTH_2BYTE;
                }
                else
                {
                    //Keep checksum size as 1 byte
                    nActiveChecksumLengthCmd = FUP_LENGTH_1BYTE;
                }

                //Create and send packet
                QByteArray baTmp = COMMAND_SETTINGS_SET;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_CURRENT_CHECKSUM_LEN_BYTES);
                baTmp.append(nActiveChecksumLengthCmd);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_SET_VERIFY_CHECKSUM_LENGTH)
            {
                //Write size has been set
                CSubMode = SUBMODE_SET_CHECKSUM_LENGTH;

                if (nMaxVerifyChecksumLengthCmd >= FUP_LENGTH_4BYTE)
                {
                    //Increase checksum size to 4 bytes
                    nActiveVerifyChecksumLengthCmd = FUP_LENGTH_4BYTE;
                }
                else if (nMaxVerifyChecksumLengthCmd >= FUP_LENGTH_2BYTE)
                {
                    //Increase checksum size to 2 bytes
                    nActiveVerifyChecksumLengthCmd = FUP_LENGTH_2BYTE;
                }
                else
                {
                    //Keep checksum size as 1 byte
                    nActiveVerifyChecksumLengthCmd = FUP_LENGTH_1BYTE;
                }

                //Create and send packet
                QByteArray baTmp = COMMAND_SETTINGS_SET;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_CURRENT_VERIFY_CHECKSUM_LEN_BYTES);
                baTmp.append(nActiveVerifyChecksumLengthCmd);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
            }
            else if (CSubMode == SUBMODE_SET_CHECKSUM_LENGTH)
            {
                //Checksum size has been set, increase baud rate
                CSubMode = SUBMODE_SET_BAUD_RATE;
                uint8_t nBaudRateIndex = lstUARTSpeeds.count();

                if (pSettingsHandle->GetConfigOption(EXACT_BAUD) != 0)
                {
                    //Limit to exact baud rate
                    uint8_t i = 0;
                    while (i < lstUARTSpeeds.count())
                    {
                        if (lstUARTSpeeds.at(i) == pSettingsHandle->GetConfigOption(EXACT_BAUD).toUInt())
                        {
                            nBaudRateIndex = i + 1;
                            break;
                        }
                        ++i;
                    }

                    if (i == lstUARTSpeeds.count())
                    {
                        //No suitable baud rate available
                        emit CurrentAction(MODULE_UPDATE, 0, QString("Unable to find desired baud rate: ").append(pSettingsHandle->GetConfigOption(EXACT_BAUD).toString()));
                        UpdateFailed(EXIT_CODE_SERIAL_PORT_EXACT_BAUD_NOT_FOUND);
                        return;
                    }
                }
                if (pSettingsHandle->GetConfigOption(MAX_BAUD) != 0)
                {
                    //Cap to maximum baud rate
                    uint32_t nMaxBaud = pSettingsHandle->GetConfigOption(MAX_BAUD).toUInt();
                    uint8_t i = 0;
                    nBaudRateIndex++;
                    while (i < lstUARTSpeeds.count())
                    {
                        if (lstUARTSpeeds.at(i) <= nMaxBaud)
                        {
                            nBaudRateIndex = i + 1;
                        }
                        ++i;
                    }

                    if (nBaudRateIndex == lstUARTSpeeds.count()+1)
                    {
                        //No suitable baud rate available
                        emit CurrentAction(MODULE_UPDATE, 0, QString("Unable to select baud rate under maximum of: ").append(QString::number(nMaxBaud)));
                        UpdateFailed(EXIT_CODE_SERIAL_PORT_MAX_BAUD_UNSUITABLE);
                        return;
                    }
                }

                //
                nChosenBaudRateIndex = nBaudRateIndex;

                //Create and send packet
                QByteArray baTmp = COMMAND_SETTINGS_SET;
                ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baTmp, FUP_OPTION_CURRENT_BAUDRATE);
                baTmp.append((uint8_t)nBaudRateIndex);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                baTmp.append((ByteArrayType)0x00);
                pDevice->Transmit(baTmp);
                if (nVerbosity >= VERBOSITY_COMMANDS)
                {
                    qDebug() << baTmp;
                }
                tmrBaudRateChangeTimer = new QTimer();
                MallocFailCheck(tmrBaudRateChangeTimer);
                tmrBaudRateChangeTimer->setInterval(300);
                tmrBaudRateChangeTimer->setSingleShot(true);
                connect(tmrBaudRateChangeTimer, SIGNAL(timeout()), this, SLOT(BaudRateChangeTimerTimeout()));
                tmrBaudRateChangeTimer->start();
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_SET_RESPONSE;
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Error
            UpdateFailed(baReceivedData.at(1));
        }
        else
        {
            //Unknown
        }
    }
    else if (nCMode == MODE_UNLOCK)
    {
        //
        if (baReceivedData.length() == FUP_RESPONSE_LENGTH_ACKNOWLEDGE && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ACKNOWLEDGE)
        {
            //Bootloader unlocked
            emit CurrentAction(MODULE_UPDATE, 0, "Bootloader unlocked");

            nRemoveBytes = FUP_RESPONSE_LENGTH_UNLOCK_RESPONSE;

            nCMode = MODE_IDLE;
            CSubMode = SUBMODE_NONE;
            NextPacket();
        }
        else if (baReceivedData.length() >= FUP_RESPONSE_LENGTH_ERROR && baReceivedData[FUP_OFFSET_PACKET_TYPE] == FUP_RESPONSE_ERROR)
        {
            //Got an error
            if (baReceivedData.at(1) == FUP_ERROR_WORM_NOT_SET)
            {
                //Module is not locked, continue with firmware upgrade but display a message
                emit CurrentAction(MODULE_UPDATE, 0, "Bootloader key was supplied but module is not locked");
                nCMode = MODE_IDLE;
                CSubMode = SUBMODE_NONE;
                NextPacket();
            }
            else
            {
                //Other error
                emit Error(MODULE_UPDATE, baReceivedData.at(1));
            }

            nRemoveBytes = FUP_RESPONSE_LENGTH_ERROR;
        }
    }

    if (nRemoveBytes > 0)
    {
        //Remove bytes from the front of the buffer
        baReceivedData.remove(0, nRemoveBytes);
    }
}

//=============================================================================
//=============================================================================
void
LrdFwUpd::CleanUp(
    bool bSuccess
    )
{

    if (bSuccess == true && bNewBootloader == true && pSettingsHandle->GetConfigOption(REBOOT_MODULE_AFTER_UPDATE) == false)
    {
        //Show message about baud rate being different
        if (!lstUARTSpeeds.isEmpty() && lstUARTSpeeds.count() >= nChosenBaudRateIndex && nChosenBaudRateIndex != 0)
        {
            emit CurrentAction(MODULE_UPDATE, 0, QString("*NOTE*: Module baudrate has been left at ").append(QString::number(lstUARTSpeeds.at(nChosenBaudRateIndex-1))));
        }
    }

    if (pUwfData->IsOpen())
    {
        //Close open upgrade file
        pUwfData->Close();
    }

    if (tmrBaudRateChangeTimer != NULL)
    {
        //Clear up the baud rate timer
        disconnect(this, SLOT(BaudRateChangeTimerTimeout()));
        delete tmrBaudRateChangeTimer;
        tmrBaudRateChangeTimer = NULL;
    }

    if (tmrDeviceReadyTimer != NULL)
    {
        //Clear up CTS change timer
        if (tmrDeviceReadyTimer->isActive())
        {
            tmrDeviceReadyTimer->stop();
        }
        disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceReadyTimerTimeout()));
        disconnect(tmrDeviceReadyTimer, SIGNAL(timeout()), this, SLOT(DeviceRebootReadyTimerTimeout()));
        delete tmrDeviceReadyTimer;
        tmrDeviceReadyTimer = NULL;
    }

    if (tmrCommandTimeoutTimer->isActive())
    {
        //Stop command timeout timer
        tmrCommandTimeoutTimer->stop();
    }

    if (pDevice->IsOpen())
    {
        //Close the serial port
        pDevice->Close();
    }

    while (lstDevices.count() > 0)
    {
        //Clear the device struct
        DeviceStruct *pDevice = lstDevices.last();
        lstDevices.pop_back();
        delete pDevice;
    }

    while (lstSectorMap.count() > 0)
    {
        //Clear the sector map struct
        SectorStruct *pSector = lstSectorMap.last();
        lstSectorMap.pop_back();
        delete pSector;
    }

    if (elptmrUpgradeTime.isValid())
    {
        //Invalidate the timer
        elptmrUpgradeTime.invalidate();
    }

    //Reset all variables back to default
    nCMode = MODE_IDLE;
    CSubMode = SUBMODE_NONE;
    nActiveDevice = 0;
    nActiveBank = 0;
    nEraseStart = 0;
    nEraseSize = 0;
    nWriteStart = 0;
    nWriteSize = 0;
    nActiveSectorSize = 0;
    nActiveEraseSectorLeft = 0;
    nDataSize = 0;
    nDeviceReadyChecks = 0;
    nActiveDeviceIndex = 0;
    lstEraseSizes.clear();
    lstUARTSpeeds.clear();
    nMaxEraseLengthCmd = 0;
    nMaxWriteLengthCmd = 0;
    nMaxChecksumLengthCmd = 0;
    nMaxVerifyChecksumLengthCmd = 0;
    nMaxWriteSize = 0;
    nMaxChecksumSize = 0;
    nChosenBaudRateIndex = 0;
    bNewBootloader = false;
    bResentFirstBootloaderCommand = false;
    baReceivedData.clear();

    //Send message back to parent
    emit FirmwareUpdateActive(false);
}

//=============================================================================
//=============================================================================
void
LrdFwUpd::CommandTimeout(
    )
{
    if (nCMode == MODE_BOOTLOADER_VERSION && CSubMode == SUBMODE_NONE && bResentFirstBootloaderCommand == false)
    {
        //Send command again
        pDevice->Transmit(COMMAND_BOOTLOADER_VERSION);
        if (nVerbosity >= VERBOSITY_COMMANDS)
        {
            qDebug() << COMMAND_BOOTLOADER_VERSION;
        }
        bResentFirstBootloaderCommand = true;
        tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
        return;
    }

    //Add to log view
    emit CurrentAction(MODULE_UPDATE, 0, "Failed to get a response to a command");
    UpdateFailed(EXIT_CODE_SERIAL_PORT_COMMAND_TIMEOUT);
}

//=============================================================================
// Callback when the baud rate change timer has elapsed
//=============================================================================
void
LrdFwUpd::BaudRateChangeTimerTimeout(
    )
{
    pDevice->Close();
    pSettingsHandle->SetConfigOption(ACTIVE_BAUD, lstUARTSpeeds.at(nChosenBaudRateIndex-1));

    disconnect(this, SLOT(BaudRateChangeTimerTimeout()));
    delete tmrBaudRateChangeTimer;
    tmrBaudRateChangeTimer = NULL;

    if (pDevice->Open() == false)
    {
        emit CurrentAction(MODULE_UPDATE, 0, QString("Failed to re-open serial port at baud rate: ").append(QString::number(lstUARTSpeeds.at(lstUARTSpeeds.count()-1))));
        UpdateFailed(EXIT_CODE_BAUD_RATE_ERROR);
        return;
    }

    emit CurrentAction(MODULE_UPDATE, 0, QString("Baud rate changed to ").append(QString::number(lstUARTSpeeds.at(nChosenBaudRateIndex-1))));

    if (pSettingsHandle->GetConfigOption(UNLOCK_KEY).isValid() && !pSettingsHandle->GetConfigOption(UNLOCK_KEY).toString().isEmpty())
    {
        //Send unlock key
        nCMode = MODE_UNLOCK;
        CSubMode = SUBMODE_NONE;

        QByteArray baUnlockCommand = COMMAND_UNLOCK;
        baUnlockCommand.append(pSettingsHandle->GetConfigOption(UNLOCK_KEY).toByteArray());

        pDevice->Transmit(baUnlockCommand);
        if (nVerbosity >= VERBOSITY_COMMANDS)
        {
            qDebug() << pSettingsHandle->GetConfigOption(UNLOCK_KEY).toByteArray();
        }

        //
        tmrCommandTimeoutTimer->start(COMMAND_TIMEOUT_PERIOD_MS);
    }
    else
    {
        nCMode = MODE_IDLE;
        CSubMode = SUBMODE_NONE;
        NextPacket();
    }
}

//=============================================================================
// Callback when the restart timer has elapsed
//=============================================================================
void
LrdFwUpd::RestartTimerTimeout(
    )
{
    //Timer for restarting module has timed out
    tmrRestartTimer->stop();
    disconnect(this, SLOT(RestartTimerTimeout()));
    delete tmrRestartTimer;
    tmrRestartTimer = NULL;

    if (CSubMode == SUBMODE_RESET_VIA_BREAK)
    {
        //Disable BREAK
        pDevice->SetBreak(false);
    }

    //Show upgrade message
    qint64 nUpgradeTime = elptmrUpgradeTime.elapsed();
    elptmrUpgradeTime.invalidate();
    emit CurrentAction(MODULE_UPDATE, 0, QString("Firmware upgrade completed in ").append(QString::number(nUpgradeTime)).append("ms"));
    CleanUp(true);

    //Signal parent
    emit Finished(true, nUpgradeTime);
}

//=============================================================================
// Returns the last error code from the update module
//=============================================================================
qint16
LrdFwUpd::GetLastErrorCode(
    )
{
    return nLastErrorCode;
}

//=============================================================================
// Error handler for child modules
//=============================================================================
void
LrdFwUpd::ModuleError(
    uint32_t nModule,
    int32_t nErrorCode
    )
{
    //Pass error up to parent
    emit Error(nModule, nErrorCode);
    if (nModule == MODULE_UART)
    {
        //Error from the UART
        if (nErrorCode == EXIT_CODE_SERIAL_PORT_DEVICE_UNPLUGGED)
        {
            CleanUp(false);
        }
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
