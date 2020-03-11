/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUpd.h
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
#ifndef LRDFWUPD_H
#define LRDFWUPD_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QMetaType>
#include <QTimer>
#include <QElapsedTimer>
#include "LrdFwCommon.h"
#include "LrdFwUART.h"
#include "LrdFwUwf.h"
#include "LrdFwBlEnter.h"
#include "LrdErr.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
//Modes (nCMode)
enum APPLICATION_MODE
{
    MODE_IDLE,
    MODE_ENTER_BOOTLOADER,
    MODE_PLATFORM_COMMAND,
    MODE_ERASE_COMMAND,
    MODE_WRITE_COMMAND,
    MODE_RESET,
    MODE_BOOTLOADER_VERSION,
    MODE_SUPPORTED_FUNCTIONS,
    MODE_SUPPORTED_OPTIONS,
    MODE_SET_OPTIONS,
    MODE_UNLOCK
};

//Submodes (nCSubMode)
enum APPLICATION_SUB_MODE
{
    SUBMODE_NONE,
    SUBMODE_WRITE_ADDRESS,
    SUBMODE_WRITE_DATA,
    SUBMODE_VERIFY_DATA,
    SUBMODE_QUERY_MAX_ERASE_LENGTH,
    SUBMODE_QUERY_MAX_WRITE_LENGTH,
    SUBMODE_QUERY_MAX_CHECKSUM_LENGTH,
    SUBMODE_QUERY_MAX_VERIFY_CHECKSUM_LENGTH,
    SUBMODE_QUERY_MAX_BAUDRATE,
    SUBMODE_QUERY_MAX_ERASE_PER_COMMAND,
    SUBMODE_QUERY_MAX_WRITE_PER_COMMAND,
    SUBMODE_QUERY_MAX_CHECKSUM_PER_COMMAND,
    SUBMODE_QUERY_ERASE_SIZES,
    SUBMODE_QUERY_BAUD_RATES,
    SUBMODE_QUERY_SUPPORTED_ERASE_SIZE,
    SUBMODE_QUERY_SUPPORTED_BAUD_RATE,
    SUBMODE_SET_ERASE_LENGTH,
    SUBMODE_SET_WRITE_LENGTH,
    SUBMODE_SET_VERIFY_CHECKSUM_LENGTH,
    SUBMODE_SET_CHECKSUM_LENGTH,
    SUBMODE_SET_BAUD_RATE,
    SUBMODE_RESET_VIA_BREAK
};

//Structure to hold information on a flash device
typedef struct
{
    uint8_t  nHandle;
    uint32_t nBaseAddr;
    uint8_t  nBanks;
    uint32_t nBankSize;
    uint8_t  nBankSelection;
} DeviceStruct;

typedef struct
{
    uint32_t nOffset;
    uint32_t nTotalSize;
    uint32_t nSectors;
    uint32_t nSectorSize;
} SectorStruct;

/******************************************************************************/
// Defines
/******************************************************************************/
//Commands for the bootloader
#define COMMAND_ENTER_BOOTLOADER                      "AT+FUP\r"
#define COMMAND_BOOTLOADER_VERSION                    "V"
#define COMMAND_TARGET_PLATFORM                       "p"
#define COMMAND_ERASE_SECTION                         "e"
#define COMMAND_WRITE_SECTION                         "w"
#define COMMAND_DATA_SECTION                          "d"
#define COMMAND_VERIFY_SECTION                        "v"
#define COMMAND_REBOOT_MODULE                         "z"
#define COMMAND_SETTINGS_QUERY                        "o"
#define COMMAND_SETTINGS_SET                          "s"
#define COMMAND_UNLOCK                                "u"
#define COMMAND_SUPPORTED_FEATURES                    "?"

//Default values, these need to be left alone to maintain compatibility with bootloader v3
#define DEFAULT_ERASE_COMMAND_LENGTH                  0
#define DEFAULT_WRITE_COMMAND_LENGTH                  1
#define DEFAULT_CHECKSUM_COMMAND_LENGTH               1
#define DEFAULT_VERIFY_CHECKSUM_COMMAND_LENGTH        4
#define DEFAULT_WRITE_SIZE                            252

//Commands in Uwf files
#define UWF_COMMAND_TARGET_PLATFORM                   'T'
#define UWF_COMMAND_REGISTER                          'G'
#define UWF_COMMAND_SELECT                            'S'
#define UWF_COMMAND_SECTOR_MAP                        'M'
#define UWF_COMMAND_ERASE                             'E'
#define UWF_COMMAND_WRITE                             'W'
#define UWF_COMMAND_QUERY                             'Q'
#define UWF_COMMAND_UNREGISTER                        'U'

//Lengths of individual commands in Uwf files
#define UWF_COMMAND_HEADER_LENGTH                     6
#define UWF_TARGET_PLATFORM_LENGTH                    4
#define UWF_REGISTER_DEVICE_LENGTH                    11
#define UWF_SELECT_DEVICE_LENGTH                      2
#define UWF_SECTOR_MAP_LENGTH                         8
#define UWF_ERASE_BLOCK_LENGTH                        8
#define UWF_WRITE_BLOCK_LENGTH                        8
#define UWF_UNREGISTER_DEVICE_LENGTH                  1

//Value offsets in commands
#define UWF_OFFSET_HEADER_COMMAND_ID                  0
#define UWF_OFFSET_HEADER_FUTURE                      1
#define UWF_OFFSET_HEADER_PACKET_LENGTH               2
#define UWF_OFFSET_REGISTER_HANDLE                    0
#define UWF_OFFSET_REGISTER_BASE_ADDRESS              1
#define UWF_OFFSET_REGISTER_BANKS                     5
#define UWF_OFFSET_REGISTER_BANK_SIZE                 6
#define UWF_OFFSET_REGISTER_BANK_SELECTION            10
#define UWF_OFFSET_SELECT_FLASH                       0
#define UWF_OFFSET_SELECT_BANK                        1
#define UWF_OFFSET_SECTOR_MAP_SECTORS                 0
#define UWF_OFFSET_SECTOR_MAP_SECTOR_SIZE             4
#define UWF_OFFSET_ERASE_OFFSET                       0
#define UWF_OFFSET_ERASE_SIZE                         4
#define UWF_OFFSET_WRITE_OFFSET                       0
#define UWF_OFFSET_WRITE_FLAGS                        4
#define UWF_OFFSET_UNREGISTER_HANDLE                  0

//Query/Set IDs for bootloader settings
#define FUP_OPTION_CURRENT_ERASE_LEN_BYTES            0x0000  //Following can be queried and set
#define FUP_OPTION_CURRENT_READ_LEN_BYTES             0x0001
#define FUP_OPTION_CURRENT_WRITE_LEN_BYTES            0x0002
#define FUP_OPTION_CURRENT_CHECKSUM_LEN_BYTES         0x0003
#define FUP_OPTION_CURRENT_VERIFY_CHECKSUM_LEN_BYTES  0x0004
#define FUP_OPTION_CURRENT_BAUDRATE                   0x0005
#define FUP_OPTION_MIN_ERASE_LEN_BYTES                0x0020  //Following are query only
#define FUP_OPTION_MIN_READ_LEN_BYTES                 0x0021
#define FUP_OPTION_MIN_WRITE_LEN_BYTES                0x0022
#define FUP_OPTION_MIN_CHECKSUM_LEN_BYTES             0x0023
#define FUP_OPTION_MIN_VERIFY_CHECKSUM_LEN_BYTES      0x0024
#define FUP_OPTION_MIN_BAUDRATE                       0x0025
#define FUP_OPTION_MAX_ERASE_LEN_BYTES                0x0040
#define FUP_OPTION_MAX_READ_LEN_BYTES                 0x0041
#define FUP_OPTION_MAX_WRITE_LEN_BYTES                0x0042
#define FUP_OPTION_MAX_CHECKSUM_LEN_BYTES             0x0043
#define FUP_OPTION_MAX_VERIFY_CHECKSUM_LEN_BYTES      0x0044
#define FUP_OPTION_MAX_BAUDRATE                       0x0045
#define FUP_OPTION_DEFAULT_ERASE_LEN_BYTES            0x0060
#define FUP_OPTION_DEFAULT_READ_LEN_BYTES             0x0061
#define FUP_OPTION_DEFAULT_WRITE_LEN_BYTES            0x0062
#define FUP_OPTION_DEFAULT_CHECKSUM_LEN_BYTES         0x0063
#define FUP_OPTION_DEFAULT_VERIFY_CHECKSUM_LEN_BYTES  0x0064
#define FUP_OPTION_DEFAULT_BAUDRATE                   0x0065
#define FUP_OPTION_MIN_ERASE_SIZE_PER_CMD             0x0080
#define FUP_OPTION_MIN_READ_SIZE_PER_CMD              0x0081
#define FUP_OPTION_MIN_WRITE_SIZE_PER_CMD             0x0082
#define FUP_OPTION_MIN_CHECKSUM_SIZE_PER_CMD          0x0083
#define FUP_OPTION_MIN_VERIFY_CHECKSUM_SIZE_PER_CMD   0x0084
#define FUP_OPTION_MAX_ERASE_SIZE_PER_CMD             0x00A0
#define FUP_OPTION_MAX_READ_SIZE_PER_CMD              0x00A1
#define FUP_OPTION_MAX_WRITE_SIZE_PER_CMD             0x00A2
#define FUP_OPTION_MAX_CHECKSUM_SIZE_PER_CMD          0x00A3
#define FUP_OPTION_MAX_VERIFY_CHECKSUM_SIZE_PER_CMD   0x00A4
#define FUP_OPTION_ERASE_SIZES_PER_CMD                0x00B0
//0x00B1-0x00B3 purposely left empty
#define FUP_OPTION_SUPPORTED_BAUDRATES                0x00B5

//Offsets for commands and repsonses
#define FUP_OFFSET_BOOTLOADER_QUERY_VALUE             4
#define FUP_OFFSET_BOOTLOADER_QUERY_MORE_DATA         8
#define FUP_OFFSET_PACKET_TYPE                        0
#define FUP_OFFSET_ERROR_ERROR_CODE                   1

//Responses to bootloader queries if more data is present or not
#define FUP_BOOTLOADER_QUERY_MORE_DATA_NO             0
#define FUP_BOOTLOADER_QUERY_MORE_DATA_YES            1

//Responses to FUP commands
#define FUP_RESPONSE_ACKNOWLEDGE                      'a'
#define FUP_RESPONSE_NOT_ACKNOWLEDGE                  'n'
#define FUP_RESPONSE_ERROR                            'f'
#define FUP_RESPONSE_VERSION                          'V'
#define FUP_RESPONSE_BOOTLOADER_QUERY                 'o'
#define FUP_RESPONSE_BOOTLOADER_SET                   's'
#define FUP_RESPONSE_SUPPORTED_FEATURES               '?'

//Lengths of FUP response commands
#define FUP_RESPONSE_LENGTH_ACKNOWLEDGE               1
#define FUP_RESPONSE_LENGTH_ERROR                     2
#define FUP_RESPONSE_LENGTH_QUERY_RESPONSE            9
#define FUP_RESPONSE_LENGTH_SET_RESPONSE              4
#define FUP_RESPONSE_LENGTH_UNLOCK_RESPONSE           4
#define FUP_RESPONSE_LENGTH_VERSION                   6
#define FUP_RESPONSE_LENGTH_FEATURES_SUPPORTED        9

//Version numbed used to differentiate legacy and enhanced bootloaders
#define FUP_EXTENDED_VERSION_NUMBER                   '6'

//Size, in bytes, of a bootloader unlock key
#define FUP_BOOTLOADER_UNLOCK_KEY_SIZE                64

//Time (in ms) between checking if a module is ready with the CTS line
#define FUP_DEVICE_READY_TIMER_TIME_MS                250

//Maximum size (in bytes) that a single verify command can check
#define FUP_VERIFY_COMMAND_MAXIMUM_SIZE               65535

//Size of bytes
#define FUP_LENGTH_4BYTE                              sizeof(uint32_t)
#define FUP_LENGTH_2BYTE                              sizeof(uint16_t)
#define FUP_LENGTH_1BYTE                              sizeof(uint8_t)

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdFwUpd : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdFwUpd(
        QObject *parent = nullptr,
        LrdSettings *pSettings = nullptr
        );
    ~LrdFwUpd(
        );
    void
    SetSettingsObject(
        LrdSettings *pSettings
        );
    bool
    StartUpdate(
        );
    void
    ContinueUpgrade(
        );
    bool
    IsUpdateInProgress(
        );
    void
    UpdateProgress(
        uint8_t *pCurrentTaskPercent,
        uint8_t *pOverallPercent
        );
    qint16
    GetLastErrorCode(
        );

signals:
    void
    Error(
        uint32_t nModule,
        int32_t nErrorCode
        );
    void
    PercentComplete(
        int8_t nCurrentTaskPercent,
        int8_t nOverallPercent
        );
    void
    Finished(
        bool bSuccessful,
        qint64 nUpgradeTimeMS
        );
    void
    CurrentAction(
        uint32_t nModule,
        uint32_t nActionID,
        QString strActionName
        );
    void
    FirmwareUpdateActive(
        bool bStatus
        );
#ifdef __linux__
    void
    SerialPortNameChanged(
        QString *pNewName
        );
#endif

public slots:
    void
    ModuleDataReceived(
        QByteArray *baOrigData
        );
    void
    DeviceReadyTimerTimeout(
        );
    void
    DeviceRebootReadyTimerTimeout(
        );
    void
    CommandTimeout(
        );
    void
    BaudRateChangeTimerTimeout(
        );
    void
    RestartTimerTimeout(
        );
    void
    ModuleError(
        uint32_t nModule,
        int32_t nErrorCode
        );

private:
    int8_t
    ProcessCommandTargetPlatform(
        uint32_t nLength
        );
    int8_t
    ProcessCommandRegisterDevice(
        uint32_t nLength
        );
    int8_t
    ProcessCommandSelectDevice(
        uint32_t nLength
        );
    int8_t
    ProcessCommandSectorMap(
        uint32_t nLength
        );
    int8_t
    ProcessCommandEraseBlock(
        uint32_t nLength
        );
    int8_t
    ProcessCommandWriteBlock(
        uint32_t nLength
        );
    int8_t
    ProcessCommandUnregister(
        uint32_t nLength
        );
    void
    UpdateFailed(
        int32_t nErrorCode
        );
    void
    NextPacket(
        );
    bool
    SupportedFunctions(
        );
    bool
    SupportedOptions(
        );
    void
    CleanUp(
        bool bSuccess
        );
    bool
    ValidateUpgradeFile(
        );

    LrdFwUART               *pDevice = NULL;                //UART object
    LrdFwUwf                *pUwfData = NULL;               //Uwf reader object
    LrdSettings             *pSettingsHandle = NULL;        //Settings object
#if !defined(TARGET_OS_MAC)
    LrdFwBlEnter            *pBlEnter = NULL;               //Bootloader entrance object
#endif
    uint8_t                 nCMode;                         //Current mode of application (APPLICATION_MODE)
    uint8_t                 CSubMode;                       //Current sub-mode of active move (APPLICATION_SUB_MODE)
    QList<DeviceStruct *>   lstDevices;                     //Holds the list of flash devices on the module
    QList<SectorStruct *>   lstSectorMap;                   //Holds the list of sectors and their mapping on the module
    uint8_t                 nActiveDevice;                  //The active flash device
    uint8_t                 nActiveBank;                    //The active bank of the active flash device
    uint32_t                nEraseStart;                    //The current position for an erase operation
    uint32_t                nEraseSize;                     //The amount left for an erase operation
    uint32_t                nEraseWholeSize;                //The whole size of an erase operation (used for current task percent)
    uint32_t                nWriteStart;                    //The current position for a write operation
    uint32_t                nWriteSize;                     //The amount left for a write operation
    uint32_t                nWriteWholeSize;                //The whole size of a write operation (used for current task percent)
    uint32_t                nActiveEraseSectorLeft;         //Number of sectors left in the current sector mapping for the current erase task
    uint32_t                nActiveSectorSize;              //Currently active sector size
    uint32_t                nDataSize;                      //The amount of data in a single write block instruction
    QTimer                  *tmrDeviceReadyTimer = NULL;    //Timer used for checking if the device is ready to start communication in bootloader mode
    QTimer                  *tmrBaudRateChangeTimer = NULL; //Timer used for checking if an error is received when changing baud rates
    QTimer                  *tmrRestartTimer = NULL;        //Timer used for restarting the module
    QTimer                  *tmrCommandTimeoutTimer = NULL; //Timer used to check if a command sent has timed out
    uint8_t                 nDeviceReadyChecks;             //Number of times device has been checked to see if it is ready
    uint8_t                 nActiveDeviceIndex;             //The currently active flash device index
    uint32_t                nFileSize;                      //The total size of the upgrade file
    QList<quint32>          lstEraseSizes;                  //Holds the list of supported erase sizes (enhanced bootloader only)
    QList<quint32>          lstUARTSpeeds;                  //Holds the list of supported baud rates (enhanced bootloader only)

    uint8_t                 nMaxEraseLengthCmd;             //Maximum erase length (bytes per command) response from module (enhanced bootloader only)
    uint8_t                 nMaxWriteLengthCmd;             //Maximum write length (byte per command) response from module (enhanced bootloader only)
    uint8_t                 nMaxChecksumLengthCmd;          //Maximum checksum length (bytes per command) response from module (enhanced bootloader only)
    uint8_t                 nMaxVerifyChecksumLengthCmd;    //Maximum verification checksum length (bytes per command) response from module (enhanced bootloader only)
    uint32_t                nMaxWriteSize;                  //Maximum number of bytes per data packet command response from module (enhanced bootloader only)
    uint32_t                nMaxChecksumSize;               //Maximum number of bytes per data checksum command response from module (enhanced bootloader only)

    uint8_t                 nActiveEraseLengthCmd;          //The active erase size in bytes per field for a single command
    uint8_t                 nActiveWriteLengthCmd;          //The active write size in bytes per field for a single command
    uint8_t                 nActiveChecksumLengthCmd;       //The active checksum size in bytes per field for a single command
    uint8_t                 nActiveVerifyChecksumLengthCmd; //The active checksum size in bytes per field for a single verify command
    uint32_t                nActiveWriteSize;               //The maximum number of bytes per data command

    bool                    bNewBootloader;                 //If the module has an old version of the bootloader or new (with enhanced features)
    bool                    bArgAutoexit;                   //Set to true if the application should automatically exit
    uint8_t                 nTmpResponseIndex;              //Temporary index used when getting a list of baud rates and erase sizes from module (enhanced bootloader only)
    QElapsedTimer           elptmrUpgradeTime;              //Timer used to measure amount of time that an upgrade takes
    QByteArray              baReceivedData;                 //Byte array buffer for data received from the module
    uint8_t                 nVerbosity;                     //The verbosity level of the output
    uint8_t                 nChosenBaudRateIndex;           //The index into lstUARTSpeeds which specifies the currently active baud rate
    qint16                  nLastErrorCode;                 //Last error code reported
    bool                    bResentFirstBootloaderCommand;  //Will be set to true if the first command has been reset (and has timed out) to allow for a retry
    bool                    bVerifyActive;                  //Cached value of if verification is enabled
    uint32_t                nVerifyChecksum;                //Checksum used for verification command
    uint32_t                nVerifyAddress;                 //Address used for verification command
    uint32_t                nVerifySize;                    //Size used for verification command
};

#endif // LRDFWUPD_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
