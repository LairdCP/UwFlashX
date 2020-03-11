/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module: LrdFwBlEnter.h
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
#ifndef LRDFWBLENTER_H
#define LRDFWBLENTER_H


/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QMessageBox>
#include "LrdFwCommon.h"
#include "LrdSettings.h"
#if !defined(TARGET_OS_MAC)
#if !defined(SKIPFTDI)
//Conditional compile for FTDI-based bootloader entrance methods
#if defined(_WIN32) && defined(_MSC_VER)
//MSVC build on windows
#define FTD2XX_STATIC
#include "FTDI/ftd2xx.h"
#elif defined(__linux__)
//Linux
#include <unistd.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#endif
#endif
#endif

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
enum ENTER_BOOTLOADER_TYPES
{
    ENTER_BOOTLOADER_NONE,
    ENTER_BOOTLOADER_AT_FUP,
    ENTER_BOOTLOADER_BL654_USB,
    ENTER_BOOTLOADER_PINNACLE100
};

/******************************************************************************/
// Constants
/******************************************************************************/
#if !defined(SKIPFTDI) && !defined(TARGET_OS_MAC)
//Pin definitions for modules
const uint8_t  FTDI_USB_RX_DIO                                                      = 1;
const uint8_t  FTDI_USB_RTS_DIO                                                     = 4;
const uint8_t  FTDI_USB_BL654_VSP_DIO                                               = 32;
const uint8_t  FTDI_USB_NRESET_DIO                                                  = 64;
const uint8_t  FTDI_USB_BL654_AUTORUN_DIO                                           = 128;
const uint8_t  FTDI_USB_PINNACLE100_ENTER_BL_DIO                                    = 128;

const uint32_t FTDI_USB_RESET_DELAY_WINDOWS_MS                                      = 400;     //Delay after completing reset in ms (Windows)
const uint32_t FTDI_USB_RESET_DELAY_LINUX_US                                        = 400000;  //Delay after completing reset in us (Linux)

const uint16_t FTDI_DEVICE_VENDOR_ID                                                = 0x0403;
const uint16_t FTDI_DEVICE_PRODUCT_ID                                               = 0x6001;
const uint16_t FTDI_DEVICE_SERIAL_NUMBER_SIZE                                       = 8;
const uint16_t FTDI_DEVICE_SERIAL_NUMBER_MAX_SIZE                                   = 16;

const uint8_t  FTDI_LIBUSB_INTERFACE_NUMBER                                         = 0;       //Interface number for libusb

const QString  FTDI_MANUFACTURER_NAME                                               = "FTDI";

#ifdef _WIN32
//Windows
#else
//Linux, these error codes come from LIBFTDI version 1.4 documentation: https://www.intra2net.com/en/developer/libftdi/documentation/group__libftdi.html
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_SUCCESS                              = 0;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_USB_DEVICE_NOT_FOUND                 = -3;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_UNABLE_TO_OPEN                       = -4;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_UNABLE_TO_CLAIM                      = -5;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_RESET_FAILED                         = -6;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_SET_BAUDRATE_FAILED                  = -7;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_GET_PRODUCTION_DESCRIPTION_FAILED    = -8;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_GET_SERIAL_NUMBER_FAILED             = -9;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_LIBUSB_GET_DEVICE_LIST_FAILED        = -12;
const int32_t LIBFTDI_ERROR_CODE_USB_OPEN_DESC_LIBUSB_GET_DEVICE_DESCRIPTOR_FAILED  = -13;

const int32_t LIBFTDI_ERROR_CODE_SET_BITMODE_SUCCESS                                = 0;
const int32_t LIBFTDI_ERROR_CODE_SET_BITMODE_CANNOT_ENABLE_BITBANG                  = -1;
const int32_t LIBFTDI_ERROR_CODE_SET_BITMODE_USB_DEVICE_NOT_AVAILABLE               = -2;

const int32_t LIBFTDI_ERROR_CODE_SET_CHUNKSIZE_SUCCESS                              = 0;
const int32_t LIBFTDI_ERROR_CODE_SET_CHUNKSIZE_CONTEXT_INVALID                      = -1;

const int32_t LIBFTDI_ERROR_CODE_WRITE_DATA_WRITTEN                                 = 1;
const int32_t LIBFTDI_ERROR_CODE_WRITE_DATA_USB_BULK_WRITE_ERROR                    = -1;
const int32_t LIBFTDI_ERROR_CODE_WRITE_DATA_USB_DEVICE_NOT_AVAILABLE                = -666;

const int32_t LIBFTDI_ERROR_CODE_RESET_SUCCESS                                      = 0;
const int32_t LIBFTDI_ERROR_CODE_RESET_RESET_FAILED                                 = -1;
const int32_t LIBFTDI_ERROR_CODE_RESET_DEVICE_UNAVAILABLE                           = -2;

const int32_t LIBFTDI_ERROR_CODE_CLOSE_SUCCESS                                      = 0;
const int32_t LIBFTDI_ERROR_CODE_CLOSE_RELEASE_FAILED                               = -1;
const int32_t LIBFTDI_ERROR_CODE_CLOSE_CONTEXT_INVALID                              = -3;

const uint8_t LIBFTDI_BITMODE_BITMASK_RESET                                         = 0;
#endif
#endif

#if !defined(TARGET_OS_MAC)

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdFwBlEnter : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdFwBlEnter(
        QObject *parent = nullptr,
        LrdSettings *pSettings = nullptr
        );
    ~LrdFwBlEnter(
        );
    void
    SetSettingsObject(
        LrdSettings *pSettings
        );
    bool
    EnterBootloader(
        uint8_t nType,
        QString strSerialPort,
        QString strFTDIOverride,
#ifdef __linux__
        QString *pNewSerialPort,
#endif
        bool bSkipWarning,
        bool bSkipError
        );

signals:
    void
    Error(
        uint32_t nModule,
        int32_t nErrorCode
        );

private:
    bool
    IsValidSerial(
        QString *pSerial
        );

    LrdSettings *pSettingsHandle = NULL; //Contains the handle for the settings object
};

#endif

#endif // LRDFWBLENTER_H
/******************************************************************************/
// END OF FILE
/******************************************************************************/
