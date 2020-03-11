/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUART.h
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
#ifndef LRDFWUART_H
#define LRDFWUART_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include "LrdFwCommon.h"
#include "LrdSettings.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdFwUART : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdFwUART(
        QObject *parent = nullptr,
        LrdSettings *pSettings = nullptr
        );
    ~LrdFwUART(
        );
    bool
    IsOpen(
        );
    bool
    Open(
        );
    void
    Close(
        );
    void
    Transmit(
        QByteArray baData
        );
    void
    SetSettingsObject(
        LrdSettings *pSettings
        );
    bool
    DeviceReady(
        );
    void
    SetDTR(
        bool bEnabled
        );
    void
    SetBreak(
        bool bEnabled
        );
    QList<QVariant>
    GetDevices(
        );
    QString
    GetDetails(
        QString strPort
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
    Receive(
        QByteArray *pData
        );

private slots:
    void
    SerialRead(
        );
    void
    SerialError(
        QSerialPort::SerialPortError speErrorCode
        );
    void
    SerialBytesWritten(
        qint64 intByteCount
        );
    void
    SerialPortClosing(
        );
private:
    QSerialPort    spSerialPort;            //Contains the handle for the serial port
    LrdSettings    *pSettingsHandle = NULL; //Contains the handle for the settings object
    uint8_t        nVerbosity;              //The verbosity level of the output
    qint16         nLastErrorCode;          //Last error code
    bool           bUARTOpen;               //If the port is open (prevents duplicate error being reported if port could not be opened)
};

#endif // LRDFWUART_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
