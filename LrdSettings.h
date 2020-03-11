/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdSettings.h
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
#ifndef LRDSETTINGS_H
#define LRDSETTINGS_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QFile>
#include <QSettings>
#include "LrdFwCommon.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
enum CONFIG_TYPES
{
    CONFIG_ID_MIN = 0,
    OUTPUT_DEVICE,
    FIRMWARE_FILE,
    APPLICATION_BAUD,
    BOOTLOADER_BAUD,
    ACTIVE_BAUD,
    MAX_BAUD,
    EXACT_BAUD,
    BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE,
    REBOOT_MODULE_BEFORE_UPDATE,
    REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS,
    REBOOT_MODULE_AFTER_UPDATE,
    VERIFY_DATA,
    UNLOCK_KEY,
    BOOTLOADER_ENTER_METHOD,
    UART_VERBOSITY,
    UPDATE_VERBOSITY,
    UWF_VERBOSITY,
    SETTINGS_VERBOSITY,
    UPDATE_SERVER_HOST,
    BOOTLOADER_ENTRANCE_WARNINGS_DISABLED,
    BOOTLOADER_ENTRANCE_ERRORS_DISABLED,
    VALIDATE_UWF,

    CONFIG_ID_MAX
};

enum CONFIG_ERRORS
{
    CONFIG_ERROR_NONE = 0,

    CONFIG_ERROR_NOT_OPEN,
    CONFIG_ERROR_ALREADY_OPEN,
    CONFIG_ERROR_FAILED_TO_OPEN,
    CONFIG_ERROR_NOT_SET,
    CONFIG_ERROR_FAILED_TO_REMOVE,
    CONFIG_ERROR_SETTINGS_FILENAME_NOT_FOUND,

    CONFIG_ERROR_MAX
};

/******************************************************************************/
// Constants
/******************************************************************************/
const QString    DEFAULT_CONFIG_OUTPUT_DEVICE                             = "";
const QString    DEFAULT_CONFIG_FIRMWARE_FILE                             = "";
const quint32    DEFAULT_CONFIG_APPLICATION_BAUD                          = 0;
const quint32    DEFAULT_CONFIG_BOOTLOADER_BAUD                           = 0;
const quint32    DEFAULT_CONFIG_ACTIVE_BAUD                               = 0;
const quint32    DEFAULT_CONFIG_MAX_BAUD                                  = 0;
const quint32    DEFAULT_CONFIG_EXACT_BAUD                                = 0;
const bool       DEFAULT_CONFIG_BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE = false;
const bool       DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE               = false;
const bool       DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS    = false;
const bool       DEFAULT_CONFIG_REBOOT_MODULE_AFTER_UPDATE                = true;
const bool       DEFAULT_CONFIG_VERIFY_DATA                               = true;
const QByteArray DEFAULT_CONFIG_UNLOCK_KEY                                = QByteArray();
const quint8     DEFAULT_CONFIG_BOOTLOADER_ENTER_METHOD                   = 0;
const quint8     DEFAULT_CONFIG_UART_VERBOSITY                            = 0;
const quint8     DEFAULT_CONFIG_UPDATE_VERBOSITY                          = 0;
const quint8     DEFAULT_CONFIG_UWF_VERBOSITY                             = 0;
const quint8     DEFAULT_CONFIG_SETTINGS_VERBOSITY                        = 0;
const QString    DEFAULT_CONFIG_UPDATE_SERVER_HOST                        = "uwterminalx.lairdconnect.com";
const bool       DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_WARNINGS_DISABLED     = false;
const bool       DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_ERRORS_DISABLED       = false;
const bool       DEFAULT_CONFIG_VALIDATE_UWF                              = true;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdSettings : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdSettings(
        QObject *parent = nullptr
        );
    ~LrdSettings(
        );

    qint16
    SetConfigOption(
        CONFIG_TYPES cnfType,
        QVariant varValue
        );
    QVariant
    GetConfigOption(
        CONFIG_TYPES cnfType
        );
    QVariant
    GetConfigOptionWithError(
        CONFIG_TYPES cnfType,
        qint32 *pError
        );
    void
    SetConfigDefaults(
        );
    CONFIG_ERRORS
    OpenPersistentConfig(
        QString strProduct
        );
    CONFIG_ERRORS
    ClosePersistentConfig(
        );
    bool
    IsPersistentConfigOpen(
        );
    bool
    IsPersistentConfigReadOnly(
        );
    CONFIG_ERRORS
    SetPersistentConfigOption(
        QString strKey,
        QVariant varValue
        );
    QVariant
    GetPersistentConfigOption(
        QString strKey
        );
    QVariant
    GetPersistentConfigOptionWithError(
        QString strKey,
        qint32 *pError
        );
    QVariant
    GetPersistentConfigOption(
        QString strKey,
        QVariant varDefault
        );
    QVariant
    GetPersistentConfigOptionWithError(
        QString strKey,
        QVariant varDefault,
        qint32 *pError
        );
    CONFIG_ERRORS
    ErasePersistentConfig(
        bool bEraseFile
        );

private:
    QMap<qint16, QVariant> mapSettings; //Settings array object
    QSettings *pSettings = NULL; //Handle to settings
};

#endif // LRDSETTINGS_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
