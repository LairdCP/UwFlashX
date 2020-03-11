/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdSettings.cpp
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
#include "LrdSettings.h"
#include <QDebug>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Constructor
//=============================================================================
LrdSettings::LrdSettings(
    QObject *parent
    ) : QObject(parent)
{
    SetConfigDefaults();
}

//=============================================================================
// Destructor
//=============================================================================
LrdSettings::~LrdSettings(
    )
{
    mapSettings.clear();
    if (IsPersistentConfigOpen())
    {
        //Close persistent configuration
        ClosePersistentConfig();
    }
}

//=============================================================================
// Change a settings value
//=============================================================================
qint16
LrdSettings::SetConfigOption(
    CONFIG_TYPES cnfType,
    QVariant varValue
    )
{
    if (!(cnfType > CONFIG_ID_MIN && cnfType < CONFIG_ID_MAX))
    {
        //Invalid key
        return EXIT_CODE_INVALID_SETTINGS_ID;
    }

    //Get type of setting
    QVariant varTmp;
    if (cnfType == OUTPUT_DEVICE)
    {
        varTmp = DEFAULT_CONFIG_OUTPUT_DEVICE;
    }
    else if (cnfType == FIRMWARE_FILE)
    {
        varTmp = DEFAULT_CONFIG_FIRMWARE_FILE;
    }
    else if (cnfType == APPLICATION_BAUD)
    {
        varTmp = DEFAULT_CONFIG_APPLICATION_BAUD;
    }
    else if (cnfType == BOOTLOADER_BAUD)
    {
        varTmp = DEFAULT_CONFIG_BOOTLOADER_BAUD;
    }
    else if (cnfType == ACTIVE_BAUD)
    {
        varTmp = DEFAULT_CONFIG_ACTIVE_BAUD;
    }
    else if (cnfType == MAX_BAUD)
    {
        varTmp = DEFAULT_CONFIG_MAX_BAUD;
    }
    else if (cnfType == EXACT_BAUD)
    {
        varTmp = DEFAULT_CONFIG_EXACT_BAUD;
    }
    else if (cnfType == REBOOT_MODULE_BEFORE_UPDATE)
    {
        varTmp = DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE;
    }
    else if (cnfType == REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS)
    {
        varTmp = DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS;
    }
    else if (cnfType == REBOOT_MODULE_AFTER_UPDATE)
    {
        varTmp = DEFAULT_CONFIG_REBOOT_MODULE_AFTER_UPDATE;
    }
    else if (cnfType == VERIFY_DATA)
    {
        varTmp = DEFAULT_CONFIG_VERIFY_DATA;
    }
    else if (cnfType == BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE)
    {
        varTmp = DEFAULT_CONFIG_BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE;
    }
    else if (cnfType == UNLOCK_KEY)
    {
        varTmp = DEFAULT_CONFIG_UNLOCK_KEY;
    }
    else if (cnfType == BOOTLOADER_ENTER_METHOD)
    {
        varTmp = DEFAULT_CONFIG_BOOTLOADER_ENTER_METHOD;
    }
    else if (cnfType == UART_VERBOSITY)
    {
        varTmp = DEFAULT_CONFIG_UART_VERBOSITY;
    }
    else if (cnfType == UPDATE_VERBOSITY)
    {
        varTmp = DEFAULT_CONFIG_UPDATE_VERBOSITY;
    }
    else if (cnfType == UWF_VERBOSITY)
    {
        varTmp = DEFAULT_CONFIG_UWF_VERBOSITY;
    }
    else if (cnfType == SETTINGS_VERBOSITY)
    {
        varTmp = DEFAULT_CONFIG_SETTINGS_VERBOSITY;
    }
    else if (cnfType == BOOTLOADER_ENTRANCE_WARNINGS_DISABLED)
    {
        varTmp = DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_WARNINGS_DISABLED;
    }
    else if (cnfType == BOOTLOADER_ENTRANCE_ERRORS_DISABLED)
    {
        varTmp = DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_ERRORS_DISABLED;
    }
    else if (cnfType == VALIDATE_UWF)
    {
        varTmp = DEFAULT_CONFIG_VALIDATE_UWF;
    }

    if (varValue.type() != varTmp.type())
    {
        //Invalid type
        return EXIT_CODE_INVALID_SETTINGS_TYPE;
    }

    mapSettings[cnfType] = varValue;

    return EXIT_CODE_SUCCESS;
}

//=============================================================================
// Return a settings value
//=============================================================================
QVariant
LrdSettings::GetConfigOption(
    CONFIG_TYPES cnfType
    )
{
    //Returns configuation option
    if (!(cnfType > CONFIG_ID_MIN && cnfType < CONFIG_ID_MAX))
    {
        //Invalid key
        return QVariant::Invalid;
    }
    else if (!mapSettings.contains(cnfType))
    {
        //Key is not set
        return QVariant::Invalid;
    }
    return mapSettings[cnfType];
}

//=============================================================================
// Return a settings value, with error information if it fails
//=============================================================================
QVariant
LrdSettings::GetConfigOptionWithError(
    CONFIG_TYPES cnfType,
    qint32 *pError
    )
{
    //Returns configuration option and an error code if it couldn't be returned
    if (!(cnfType > CONFIG_ID_MIN && cnfType < CONFIG_ID_MAX))
    {
        //Invalid key
        *pError = EXIT_CODE_INVALID_SETTINGS_ID;
        return QVariant::Invalid;
    }
    else if (!mapSettings.contains(cnfType))
    {
        //Key is not set
        *pError = EXIT_CODE_INVALID_SETTINGS_NOT_SET;
        return QVariant::Invalid;
    }
    return mapSettings[cnfType];
}

//=============================================================================
// Return settings values back to defaults
//=============================================================================
void
LrdSettings::SetConfigDefaults(
    )
{
    //Sets configuration values back to defaults
    mapSettings[OUTPUT_DEVICE] = DEFAULT_CONFIG_OUTPUT_DEVICE;
    mapSettings[FIRMWARE_FILE] = DEFAULT_CONFIG_FIRMWARE_FILE;
    mapSettings[APPLICATION_BAUD] = DEFAULT_CONFIG_APPLICATION_BAUD;
    mapSettings[BOOTLOADER_BAUD] = DEFAULT_CONFIG_BOOTLOADER_BAUD;
    mapSettings[ACTIVE_BAUD] = DEFAULT_CONFIG_ACTIVE_BAUD;
    mapSettings[MAX_BAUD] = DEFAULT_CONFIG_MAX_BAUD;
    mapSettings[EXACT_BAUD] = DEFAULT_CONFIG_EXACT_BAUD;
    mapSettings[BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE] = DEFAULT_CONFIG_BOOTLOADER_ENHANCED_FUNCTIONALITY_DISABLE;
    mapSettings[REBOOT_MODULE_BEFORE_UPDATE] = DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE;
    mapSettings[REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS] = DEFAULT_CONFIG_REBOOT_MODULE_BEFORE_UPDATE_DTR_STATUS;
    mapSettings[REBOOT_MODULE_AFTER_UPDATE] = DEFAULT_CONFIG_REBOOT_MODULE_AFTER_UPDATE;
    mapSettings[VERIFY_DATA] = DEFAULT_CONFIG_VERIFY_DATA;
    mapSettings[UNLOCK_KEY] = DEFAULT_CONFIG_UNLOCK_KEY;
    mapSettings[BOOTLOADER_ENTER_METHOD] = DEFAULT_CONFIG_BOOTLOADER_ENTER_METHOD;
    mapSettings[UART_VERBOSITY] = DEFAULT_CONFIG_UART_VERBOSITY;
    mapSettings[UPDATE_VERBOSITY] = DEFAULT_CONFIG_UPDATE_VERBOSITY;
    mapSettings[UWF_VERBOSITY] = DEFAULT_CONFIG_UWF_VERBOSITY;
    mapSettings[SETTINGS_VERBOSITY] = DEFAULT_CONFIG_SETTINGS_VERBOSITY;
    mapSettings[UPDATE_SERVER_HOST] = DEFAULT_CONFIG_UPDATE_SERVER_HOST;
    mapSettings[BOOTLOADER_ENTRANCE_WARNINGS_DISABLED] = DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_WARNINGS_DISABLED;
    mapSettings[BOOTLOADER_ENTRANCE_ERRORS_DISABLED] = DEFAULT_CONFIG_BOOTLOADER_ENTRANCE_ERRORS_DISABLED;
    mapSettings[VALIDATE_UWF] = DEFAULT_CONFIG_VALIDATE_UWF;
}

//=============================================================================
// Opens the persistent configuration storage
//=============================================================================
CONFIG_ERRORS
LrdSettings::OpenPersistentConfig(
    QString strProduct
    )
{
    if (IsPersistentConfigOpen() == true)
    {
        //Already open
        return CONFIG_ERROR_ALREADY_OPEN;
    }

    //Open settings
    pSettings = NULL;
    pSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, gstrOrganisationName, strProduct);

    if (pSettings == NULL)
    {
        //Failed to open
        return CONFIG_ERROR_FAILED_TO_OPEN;
    }

    return CONFIG_ERROR_NONE;
}

//=============================================================================
// Closes the persistent configuration storage
//=============================================================================
CONFIG_ERRORS
LrdSettings::ClosePersistentConfig(
    )
{
    if (pSettings == NULL)
    {
        return CONFIG_ERROR_NOT_OPEN;
    }
    delete pSettings;
    pSettings = NULL;
    return CONFIG_ERROR_NONE;
}

//=============================================================================
// Returns true if persistent configuration storage is open, otherwise false
//=============================================================================
bool
LrdSettings::IsPersistentConfigOpen(
    )
{
    if (pSettings == NULL)
    {
        return false;
    }
    return true;
}

//=============================================================================
// Returns true if persistent configuration storage is read only
//=============================================================================
bool
LrdSettings::IsPersistentConfigReadOnly(
    )
{
    if (pSettings == NULL)
    {
        return true;
    }

    return !pSettings->isWritable();
}

//=============================================================================
// Sets a persistent configuration value
//=============================================================================
CONFIG_ERRORS
LrdSettings::SetPersistentConfigOption(
    QString strKey,
    QVariant varValue
    )
{
    if (pSettings == NULL)
    {
        return CONFIG_ERROR_NOT_OPEN;
    }

    pSettings->setValue(strKey, varValue);
    return CONFIG_ERROR_NONE;
}

//=============================================================================
// Gets a persistent configuration value, without default
//=============================================================================
QVariant
LrdSettings::GetPersistentConfigOption(
    QString strKey
    )
{
    if (pSettings == NULL)
    {
        return QVariant();
    }
    return pSettings->value(strKey);
}

//=============================================================================
// Gets a persistent configuration value, without default, with error code
//=============================================================================
QVariant
LrdSettings::GetPersistentConfigOptionWithError(
    QString strKey,
    qint32 *pError
    )
{
    if (pSettings == NULL)
    {
        *pError = CONFIG_ERROR_NOT_OPEN;
        return QVariant();
    }

    if (!pSettings->contains(strKey))
    {
        //Value doesn't exist
        *pError = CONFIG_ERROR_NOT_SET;
        return QVariant();
    }

    //Return value
    return pSettings->value(strKey);
}

//=============================================================================
// Gets a persistent configuration value
//=============================================================================
QVariant
LrdSettings::GetPersistentConfigOption(
    QString strKey,
    QVariant varDefault
    )
{
    if (pSettings == NULL)
    {
        return QVariant();
    }

    if (!pSettings->contains(strKey))
    {
        //Value doesn't exist
        return varDefault;
    }

    //Return value
    return pSettings->value(strKey);
}

//=============================================================================
// Gets a persistent configuration value, with error code
//=============================================================================
QVariant
LrdSettings::GetPersistentConfigOptionWithError(
    QString strKey,
    QVariant varDefault,
    qint32 *pError
    )
{
    if (pSettings == NULL)
    {
        *pError = CONFIG_ERROR_NOT_OPEN;
        return QVariant();
    }

    if (!pSettings->contains(strKey))
    {
        //Value doesn't exist
        *pError = CONFIG_ERROR_NOT_SET;
        return varDefault;
    }

    //Return value
    return pSettings->value(strKey);
}

//=============================================================================
// Removes persistent configuration
//=============================================================================
CONFIG_ERRORS
LrdSettings::ErasePersistentConfig(
    bool bEraseFile
    )
{
    if (pSettings == NULL)
    {
        return CONFIG_ERROR_NOT_OPEN;
    }
    pSettings->clear();

    if (bEraseFile)
    {
        //Erase file too
        if (!pSettings->fileName().isNull() && !pSettings->fileName().isEmpty())
        {
            //We have a filename
            QString strPersistConfigFilename = pSettings->fileName();
            CONFIG_ERRORS nError = ClosePersistentConfig();
            if (nError != CONFIG_ERROR_NONE)
            {
                return nError;
            }

            //Remove file
            QFile fileTmp(strPersistConfigFilename);
            if (fileTmp.remove() == false)
            {
                return CONFIG_ERROR_FAILED_TO_REMOVE;
            }
        }
        else
        {
            //Failed to remove because we couldn't get a filename
            return CONFIG_ERROR_SETTINGS_FILENAME_NOT_FOUND;
        }
    }

    //Completed
    return CONFIG_ERROR_NONE;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
