/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUwf.cpp
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
#include "LrdFwUwf.h"
#include <QDebug>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Constructor
//=============================================================================
LrdFwUwf::LrdFwUwf(
    QObject *parent,
    LrdSettings *pSettings
    ) : QObject(parent)
{
    pUpgradeFile = NULL;
    pSettingsHandle = pSettings;
}

//=============================================================================
// Destructor
//=============================================================================
LrdFwUwf::~LrdFwUwf(
    )
{
    if (pUpgradeFile != NULL)
    {
        //Clean up open file
        pUpgradeFile->close();
        delete pUpgradeFile;
        pUpgradeFile = NULL;
    }
}

//=============================================================================
// Set the settings handle
//=============================================================================
void
LrdFwUwf::SetSettingsObject(
    LrdSettings *pSettings
    )
{
    pSettingsHandle = pSettings;
}

//=============================================================================
// Opens a uwf file for reading
//=============================================================================
bool
LrdFwUwf::Open(
    )
{
    if (pSettingsHandle == nullptr)
    {
        //Settings handle is not set
        nLastErrorCode = EXIT_CODE_SETTINGS_HANDLE_NULL;
        emit Error(MODULE_UART, EXIT_CODE_SETTINGS_HANDLE_NULL);
        return false;
    }

    nVerbosity = pSettingsHandle->GetConfigOption(UWF_VERBOSITY).toUInt();
    pUpgradeFile = new QFile(pSettingsHandle->GetConfigOption(FIRMWARE_FILE).toString());
    MallocFailCheck(pUpgradeFile);
    if (!pUpgradeFile->exists())
    {
        //File does not exist
        delete pUpgradeFile;
        pUpgradeFile = NULL;
        nLastErrorCode = EXIT_CODE_UWF_FILE_NOT_FOUND;
        emit Error(MODULE_UWF, EXIT_CODE_UWF_FILE_NOT_FOUND);
        return false;
    }

    if (!pUpgradeFile->open(QFile::ReadOnly))
    {
        //Cannot get read only access
        delete pUpgradeFile;
        pUpgradeFile = NULL;
        nLastErrorCode = EXIT_CODE_UWF_FILE_FAILED_TO_OPEN;
        emit Error(MODULE_UWF, EXIT_CODE_UWF_FILE_FAILED_TO_OPEN);
        return false;
    }

    //File opened
    return true;
}

//=============================================================================
// Closes open uwf file
//=============================================================================
void
LrdFwUwf::Close(
    )
{
    if (pUpgradeFile != NULL && pUpgradeFile->isOpen())
    {
        pUpgradeFile->close();
        delete pUpgradeFile;
        pUpgradeFile = NULL;
    }
}

//=============================================================================
// Reads data from uwf file
//=============================================================================
QByteArray
LrdFwUwf::Read(
    qint32 nBytes
    )
{
    return pUpgradeFile->read(nBytes);
}

//=============================================================================
// Seeks to the desired offset in a uwf file
//=============================================================================
bool
LrdFwUwf::Seek(
    qint8 nType,
    qint32 nPosition
    )
{
    if (nType == SEEK_CURRENT)
    {
        //Current position modification
        pUpgradeFile->seek(pUpgradeFile->pos() + nPosition);
    }
    else if (nType == SEEK_FROM_BEGINNING)
    {
        //Seek from beginning
        pUpgradeFile->seek(nPosition);
    }
    else if (nType == SEEK_FROM_END)
    {
        //Seek from end
        //Not implemented
        return false;
    }
    else
    {
        //Invalid
        return false;
    }

    return true;
}

//=============================================================================
// Returns true if a uwf file is open, otherwise returns false
//=============================================================================
bool
LrdFwUwf::IsOpen(
    )
{
    if (pUpgradeFile == NULL || pUpgradeFile->isOpen() == false)
    {
        //File is not open
        return false;
    }

    //File is open
    return true;
}

//=============================================================================
// Returns the total size of the uwf file
//=============================================================================
qint32
LrdFwUwf::TotalSize(
    )
{
    if (pUpgradeFile == NULL || pUpgradeFile->isOpen() == false)
    {
        //File not open
        return 0;
    }
    return pUpgradeFile->size();
}

//=============================================================================
// Returns the current offset into the opened uwf file
//=============================================================================
qint32
LrdFwUwf::CurrentPosition(
    )
{
    if (pUpgradeFile == NULL || pUpgradeFile->isOpen() == false)
    {
        //File not open
        return 0;
    }
    return pUpgradeFile->pos();
}

//=============================================================================
// Returns true if the end of the uwf file has been encountered
//=============================================================================
bool
LrdFwUwf::AtEnd(
    )
{
    if (pUpgradeFile == NULL || pUpgradeFile->isOpen() == false)
    {
        //File not open
        return true;
    }

    return pUpgradeFile->atEnd();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
