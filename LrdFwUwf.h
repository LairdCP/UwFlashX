/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUwf.h
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
#ifndef LRDFWUWF_H
#define LRDFWUWF_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QFile>
#include "LrdFwCommon.h"
#include "LrdSettings.h"
#include "LrdErr.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
enum SEEK_TYPES
{
    SEEK_CURRENT,
    SEEK_FROM_BEGINNING,
    SEEK_FROM_END
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdFwUwf : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdFwUwf(
        QObject *parent = nullptr,
        LrdSettings *pSettings = nullptr
        );
    ~LrdFwUwf(
        );
    void
    SetSettingsObject(
        LrdSettings *pSettings
        );
    bool
    Open(
        );
    void
    Close(
        );
    QByteArray
    Read(
        qint32 nBytes
        );
    bool
    Seek(
        qint8 nType,
        qint32 nPosition
        );
    bool
    IsOpen(
        );
    qint32
    TotalSize(
        );
    qint32
    CurrentPosition(
        );
    bool
    AtEnd(
        );

signals:
    void
    Error(
        uint32_t nModule,
        int32_t nErrorCode
        );

private:
    QFile          *pUpgradeFile = NULL;    //Pointer to the upgrade file handle
    LrdSettings    *pSettingsHandle = NULL; //Pointer to the settings object
    qint16         nLastErrorCode;          //Last error code
    uint8_t        nVerbosity;              //The verbosity level of the output
};

#endif // LRDFWUWF_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
