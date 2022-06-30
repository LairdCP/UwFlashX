/******************************************************************************
** Copyright (C) 2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwUART.cpp
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
#include "LrdFwUART.h"
#include <QDebug>

//=============================================================================
// Constructor
//=============================================================================
LrdFwUART::LrdFwUART(
    QObject *parent,
    LrdSettings *pSettings
    ) : QObject(parent)
{
    //Connect serial signals
    connect(&spSerialPort, SIGNAL(readyRead()), this, SLOT(SerialRead()));
    connect(&spSerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    connect(&spSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));
    connect(&spSerialPort, SIGNAL(aboutToClose()), this, SLOT(SerialPortClosing()));
    pSettingsHandle = pSettings;

    //No errors have occured
    nLastErrorCode = EXIT_CODE_SUCCESS;

    //Disable verbose messages by default
    nVerbosity = VERBOSITY_NONE;
}

//=============================================================================
// Destructor
//=============================================================================
LrdFwUART::~LrdFwUART(
    )
{
    //Remove serial signals
    disconnect(&spSerialPort, SIGNAL(readyRead()), this, SLOT(SerialRead()));
    disconnect(&spSerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    disconnect(&spSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));
    disconnect(&spSerialPort, SIGNAL(aboutToClose()), this, SLOT(SerialPortClosing()));

    if (spSerialPort.isOpen())
    {
        //Close serial port
        spSerialPort.flush();
        spSerialPort.close();
    }
}

//=============================================================================
// Set the settings handle
//=============================================================================
void
LrdFwUART::SetSettingsObject(
    LrdSettings *pSettings
    )
{
    pSettingsHandle = pSettings;
}

//=============================================================================
// Returns true if serial port is open
//=============================================================================
bool
LrdFwUART::IsOpen(
    )
{
    return spSerialPort.isOpen();
}

//=============================================================================
// Opens serial port, returns true on success otherwise returns false
//=============================================================================
bool
LrdFwUART::Open(
    )
{
    //Set serial port configuration options and open port
    if (pSettingsHandle == nullptr)
    {
        //Settings handle is not set
        nLastErrorCode = EXIT_CODE_SETTINGS_HANDLE_NULL;
        emit Error(MODULE_UART, EXIT_CODE_SETTINGS_HANDLE_NULL);
        return false;
    }

    //Set the verbosity
    nVerbosity = pSettingsHandle->GetConfigOption(UART_VERBOSITY).toUInt();

    //Configure serial port
    spSerialPort.setPortName(pSettingsHandle->GetConfigOption(OUTPUT_DEVICE).toString());
    spSerialPort.setBaudRate(pSettingsHandle->GetConfigOption(ACTIVE_BAUD).toULongLong());
    spSerialPort.setDataBits(QSerialPort::Data8);
    spSerialPort.setStopBits(QSerialPort::OneStop);
    spSerialPort.setParity(QSerialPort::NoParity);
    spSerialPort.setFlowControl(QSerialPort::HardwareControl);

    //Open it
    if (!spSerialPort.open(QSerialPort::ReadWrite))
    {
        nLastErrorCode = EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN;
        emit Error(MODULE_UART, EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN);
        return false;
    }

    //Port opened
    bUARTOpen = true;
    return true;
}

//=============================================================================
// Closes serial port, if open
//=============================================================================
void
LrdFwUART::Close(
    )
{
    bUARTOpen = false;
    if (spSerialPort.isOpen())
    {
        //Port is open, close it
        spSerialPort.flush();
        spSerialPort.close();
    }
}

//=============================================================================
// Transmits data out the serial port
//=============================================================================
void
LrdFwUART::Transmit(
    QByteArray baData
    )
{
    spSerialPort.write(baData);
}

//=============================================================================
// Returns a list of serial devices
//=============================================================================
QList<QVariant>
LrdFwUART::GetDevices(
    )
{
    QList<QVariant> lstDevices;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        lstDevices.append(info.portName());
    }
    return lstDevices;
}

//=============================================================================
// Returns details on a serial device
//=============================================================================
QString
LrdFwUART::GetDetails(
    QString strPort
    )
{
    QSerialPortInfo spiSerialInfo(strPort);
    if (!spiSerialInfo.isNull())
    {
        //Port exists
        QString strDisplayText(spiSerialInfo.description());
        if (spiSerialInfo.manufacturer().length() > 1)
        {
            //Add manufacturer
            strDisplayText.append(" (").append(spiSerialInfo.manufacturer()).append(")");
        }
        if (spiSerialInfo.serialNumber().length() > 1)
        {
            //Add serial
            strDisplayText.append(" [").append(spiSerialInfo.serialNumber()).append("]");
        }
        return strDisplayText;
    }
    else
    {
        //No such port
        return "Invalid serial port selected";
    }
}

//=============================================================================
// Serial port error handler
//=============================================================================
void
LrdFwUART::SerialError(
    QSerialPort::SerialPortError speErrorCode
    )
{
    if (speErrorCode == QSerialPort::NoError)
    {
        //No error. Why this is ever emitted is a mystery to me.
        return;
    }
#if QT_VERSION < 0x050700
    //As of Qt 5.7 these are now deprecated. It is being left in as a conditional compile for anyone using older versions of Qt to prevent these errors closing the serial port.
    else if (speErrorCode == QSerialPort::ParityError)
    {
        //Parity error
    }
    else if (speErrorCode == QSerialPort::FramingError)
    {
        //Framing error
    }
#endif
    else if (speErrorCode == QSerialPort::ResourceError || speErrorCode == QSerialPort::PermissionError)
    {
        //Resource error or permission error (device unplugged?)
        if (bUARTOpen == true)
        {
            //Only emit an error if the UART is open
            bUARTOpen = false;
            emit Error(MODULE_UART, EXIT_CODE_SERIAL_PORT_DEVICE_UNPLUGGED);
        }
    }
}

//=============================================================================
// Callback for serial bytes being transmitted out of the UART
//=============================================================================
void
LrdFwUART::SerialBytesWritten(
    qint64
    )
{
}

//=============================================================================
// Callback for serial port closing event
//=============================================================================
void
LrdFwUART::SerialPortClosing(
    )
{
    bUARTOpen = false;
}

//=============================================================================
// Callback when there is data to read from the serial port
//=============================================================================
void
LrdFwUART::SerialRead(
    )
{
    //Append received data into buffer
    QByteArray baOrigData = spSerialPort.readAll();
    if (nVerbosity >= VERBOSITY_COMMANDS)
    {
        qDebug() << baOrigData;
    }
    emit Receive(&baOrigData);
}

//=============================================================================
// Checks if device is ready to communicate (if CTS is clear)
//=============================================================================
bool
LrdFwUART::DeviceReady(
    )
{
    if (spSerialPort.pinoutSignals() & QSerialPort::ClearToSendSignal)
    {
        return true;
    }
    return false;
}

//=============================================================================
// Sets DTR to be high or low
//=============================================================================
void
LrdFwUART::SetDTR(
    bool bEnabled
    )
{
    spSerialPort.setDataTerminalReady(bEnabled);
}

//=============================================================================
// Applies or removes BREAK from a serial port
//=============================================================================
void
LrdFwUART::SetBreak(
    bool bEnabled
    )
{
    spSerialPort.setBreakEnabled(bEnabled);
}

//=============================================================================
// Returns the last error code from the serial port
//=============================================================================
qint16
LrdFwUART::GetLastErrorCode(
    )
{
    return nLastErrorCode;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
