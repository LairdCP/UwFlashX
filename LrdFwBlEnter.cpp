/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module: LrdFwBlEnter.cpp
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

#if !defined(__APPLE__)

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "LrdFwBlEnter.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

//=============================================================================
// Constructor
//=============================================================================
LrdFwBlEnter::LrdFwBlEnter(
    QObject *parent,
    LrdSettings *pSettings
    ) : QObject(parent)
{
    pSettingsHandle = pSettings;
}

//=============================================================================
// Destructor
//=============================================================================
LrdFwBlEnter::~LrdFwBlEnter(
    )
{

}

//=============================================================================
// Set the settings handle
//=============================================================================
void
LrdFwBlEnter::SetSettingsObject(
    LrdSettings *pSettings
    )
{
    pSettingsHandle = pSettings;
}

#if !defined(SKIPFTDI) && !defined(TARGET_OS_MAC)
//=============================================================================
// Workaround for issue with FTDI library
//=============================================================================
extern "C"
{
    volatile void __imp_printf(...)
    {
         return;
    }
}

//=============================================================================
// Returns true if an FTDI serial number is valid
//=============================================================================
bool
LrdFwBlEnter::IsValidSerial(
    QString *pSerial
    )
{
    uint8_t i = 0;
    uint8_t l = pSerial->length();
    while (i < l)
    {
        if (!((*pSerial)[i] >= 'a' && (*pSerial)[i] <= 'z') &&
                !((*pSerial)[i] >= 'A' && (*pSerial)[i] <= 'Z') &&
                !((*pSerial)[i] >= '0' && (*pSerial)[i] <= '9') &&
                (*pSerial)[i] != '-')
        {
            //Invalid character
            return false;
        }
        ++i;
    }

    return true;
}

//=============================================================================
// Enters bootloader mode on the target module via FTDI functionality
//=============================================================================
bool
LrdFwBlEnter::EnterBootloader(
    uint8_t nType,
    QString strSerialPort,
    QString strFTDIOverride,
#ifdef __linux__
    QString *pNewSerialPort,
#endif
    bool bSkipWarning,
    bool bSkipError
    )
{
    //Enters bootloader mode
    if (nType != ENTER_BOOTLOADER_BL654_USB && nType != ENTER_BOOTLOADER_PINNACLE100)
    {
        //Invalid type specified
        return false;
    }

#ifdef _WIN32
#ifdef _MSC_VER
    QSerialPortInfo spiSerialInfo(strSerialPort);
    if (spiSerialInfo.isValid() && spiSerialInfo.manufacturer().indexOf(FTDI_MANUFACTURER_NAME) != -1)
    {
        //Valid FTDI device, proceed
        if (bSkipWarning == false)
        {
            if (QMessageBox::question(NULL, "Continue?", QString("This feature allows automatically entering the bootloader on certain modules, please be sure that you have selected the correct device before continuing as using it with the wrong device may cause unforeseen issues and potential hardware damage which Laird Connectivity claims no responsibility and accepts no liability for.\r\n\r\nAre you sure ").append(strSerialPort).append(" is the correct port and '").append(spiSerialInfo.description()).append("' (").append(spiSerialInfo.manufacturer()).append(") [").append(spiSerialInfo.serialNumber()).append("] the correct description for your device?"), QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
            {
                //Cancel operation
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_CANCELLED);
                return false;
            }
        }

        //Windows, MSVC build
        FT_STATUS ftsStatus;
        FT_HANDLE fthHandle;

        //Check that the serial number is valid
        QString strFTDISerial = spiSerialInfo.serialNumber().left(FTDI_DEVICE_SERIAL_NUMBER_SIZE);
        if (!strFTDIOverride.isNull() && !strFTDIOverride.isEmpty())
        {
            //User provided FTDI ID over-ride
            strFTDISerial = strFTDIOverride;
        }
        else
        {
            if (IsValidSerial(&strFTDISerial) == false)
            {
                //Serial number is not valid
                if (bSkipError == false)
                {
                    QMessageBox::critical(NULL, "Error retrieving FTDI serial number", QString("There was an error retrieving the serial number for your FTDI device, please open a bug report on the UwFlashX github page (link can be clicked from the 'About' tab) and provide the following details:\r\n\r\nPort: ").append(strSerialPort).append("\r\nManufacturer: ").append(spiSerialInfo.manufacturer()).append("\r\nFull serial number: ").append(spiSerialInfo.serialNumber()).append("\r\nTrucated serial number: ").append(strFTDISerial).append("\r\nVendor ID: ").append(QString::number(spiSerialInfo.vendorIdentifier())).append("\r\nProduct ID: ").append(QString::number(spiSerialInfo.productIdentifier())).append("\r\nDescription: ").append(spiSerialInfo.description()).append("\r\nSystem Location: ").append(spiSerialInfo.systemLocation()).append("\r\n\r\nAnd also download and run FT_PROG from the FTDI website, click 'Devices' -> 'Scan and Parse' and attach a screenshot of the utility."));
                }
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_SERIAL_NUMBER_NOT_VALID);
                return false;
            }
        }

        //Open device in D2xx mode
        ftsStatus = FT_OpenEx((void*)strFTDISerial.toStdString().c_str(), FT_OPEN_BY_SERIAL_NUMBER, &fthHandle);
        if (ftsStatus == FT_OK)
        {
            //Opened successfully
            char baTxBuffer;
            DWORD unBytesWritten;

            //Set a bit bang baud rate
            ftsStatus = FT_SetBaudRate(fthHandle, FT_BAUD_1200);

            //Enable syncronous bit bang mode
            if (nType == ENTER_BOOTLOADER_BL654_USB)
            {
                ftsStatus = FT_SetBitMode(fthHandle, (FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO | FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_BL654_AUTORUN_DIO), FT_BITMODE_ASYNC_BITBANG);
            }
            else if (nType == ENTER_BOOTLOADER_PINNACLE100)
            {
                ftsStatus = FT_SetBitMode(fthHandle, (FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO | FTDI_USB_PINNACLE100_ENTER_BL_DIO | FTDI_USB_NRESET_DIO), FT_BITMODE_ASYNC_BITBANG);
            }

            if (ftsStatus != FT_OK)
            {
                //Failed to set bitbang mode
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_BITBANG_MODE_FAILED);
                return false;
            }

            //Disable autorun
            if (nType == ENTER_BOOTLOADER_BL654_USB)
            {
                baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            else if (nType == ENTER_BOOTLOADER_PINNACLE100)
            {
                baTxBuffer = FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            ftsStatus = FT_Write(fthHandle, &baTxBuffer, sizeof(baTxBuffer), &unBytesWritten);
            if (ftsStatus != FT_OK)
            {
                //Failed to write data to port
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
                return false;
            }
            Sleep(FTDI_USB_RESET_DELAY_WINDOWS_MS);

            //Reset module
            if (nType == ENTER_BOOTLOADER_BL654_USB)
            {
                baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            else if (nType == ENTER_BOOTLOADER_PINNACLE100)
            {
                baTxBuffer = FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            ftsStatus = FT_Write(fthHandle, &baTxBuffer, sizeof(baTxBuffer), &unBytesWritten);
            if (ftsStatus != FT_OK)
            {
                //Failed to write data to port
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
                return false;
            }
            Sleep(FTDI_USB_RESET_DELAY_WINDOWS_MS);

            //Stay out of autorun
            if (nType == ENTER_BOOTLOADER_BL654_USB)
            {
                baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            else if (nType == ENTER_BOOTLOADER_PINNACLE100)
            {
                baTxBuffer = FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
            }
            ftsStatus = FT_Write(fthHandle, &baTxBuffer, sizeof(baTxBuffer), &unBytesWritten);
            if (ftsStatus != FT_OK)
            {
                //Failed to write data to port
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
                return false;
            }
            Sleep(FTDI_USB_RESET_DELAY_WINDOWS_MS);

            //Exit bit bang mode
            ftsStatus = FT_SetBitMode(fthHandle, FTDI_LIBUSB_INTERFACE_NUMBER, FT_BITMODE_RESET);
            if (ftsStatus != FT_OK)
            {
                //Failed to reset port (disable bitbang)
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
                return false;
            }

            //Close direct FTDI access
            ftsStatus = FT_Close(fthHandle);
            if (ftsStatus != FT_OK)
            {
                //Failed to close port
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_CLOSE_FAILED);
                return false;
            }
        }
        else
        {
            //Failed to open device
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_OPEN_FAILED);
            return false;
        }
    }
    else
    {
        //Invalid or non-FTDI device
        emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_INVALID_DEVICE);
        return false;
    }
#else
    //Windows, MinGW (or other) build
    if (bSkipError == false)
    {
        QMessageBox::information(NULL, "MinGW builds not supported", "Due to FTDI drivers only being provided for visual studio, MinGW builds of UwFlashX are unable to use this functionality, please either use a MSVC version of UwFlashX or build the application manually from source using visual studio.", QMessageBox::Ok);
    }
    emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_WRONG_COMPILER);
    return false;
#endif
#else
    //Linux
    QSerialPortInfo spiSerialInfo(strSerialPort);
    QString strOldSerialPortSerialNumber;
    if (spiSerialInfo.isValid() && spiSerialInfo.manufacturer().indexOf(FTDI_MANUFACTURER_NAME) != -1)
    {
        //Valid FTDI device, proceed
        if (bSkipWarning == false)
        {
            if (QMessageBox::question(NULL, "Continue?", QString("This feature allows automatically entering the bootloader on certain modules, please be sure that you have selected the correct device before continuing as using it with the wrong device may cause unforeseen issues and potential hardware damage which Laird Connectivity claims no responsibility and accepts no liability for.\r\n\r\nAre you sure ").append(strSerialPort).append(" is the correct port and '").append(QString(spiSerialInfo.description()).append("' (").append(spiSerialInfo.manufacturer()).append(") [").append(spiSerialInfo.serialNumber()).append("] the correct description for your device?\r\n\r\nNote that you require libftdi and libusb (version 1.0) for this to work.")), QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
            {
                //Cancel operation
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_CANCELLED);
                return false;
            }
        }

        //Exit autorun mode
        strOldSerialPortSerialNumber = spiSerialInfo.serialNumber();
        struct ftdi_context *ftContext;
        int nStatus = 0;
        unsigned char baTxBuffer;
        libusb_context *usbContext = NULL;
        libusb_device **usbdDevice = NULL;
        ssize_t nDevicesFound = 0;
        unsigned char strSerialNumber[FTDI_DEVICE_SERIAL_NUMBER_MAX_SIZE];

        if (bSkipWarning == false)
        {
            //Show setup warning message
            bool bStopProcess = false;
            CONFIG_ERRORS nErrorCode;
            nErrorCode = pSettingsHandle->OpenPersistentConfig(APP_NAME);
            QVariant varShowNonRootWarning = pSettingsHandle->GetPersistentConfigOption("LinuxShownNonRootUSBWarning", false);
            if (varShowNonRootWarning.isNull() || varShowNonRootWarning == false)
            {
                //Show warning
                if (QMessageBox::warning(NULL, "Confirm Linux Setup", "Have you followed the instructions on the UwTerminalX wiki page for setting up udev rules for USB devices for non-root users? Without this setup stage being completed, the process for exiting autorun may fail.\r\n\r\nClick 'no' to be taken to the UwTerminalX Linux setup wiki page. This message will only be displayed once.", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
                {
                    //Open web page with Linux non-root user setup instructions
                    bStopProcess = true;
                    nErrorCode = pSettingsHandle->SetPersistentConfigOption("LinuxShownNonRootUSBWarning", true);
                    if (QDesktopServices::openUrl(QUrl(gstrURLLinuxNonRootSetup)) == false)
                    {
                        //Failed to open URL
                        QMessageBox::critical(NULL, "Failed to open URL", QString("An error occured whilst attempting to open a web browser, please ensure you have a web browser installed and configured. URL: ").append(gstrURLLinuxNonRootSetup), QMessageBox::Ok);
                    }
                }
            }
            nErrorCode = pSettingsHandle->ClosePersistentConfig();

            if (bStopProcess == true)
            {
                //Prevent opening the port until the user has configured their system
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_SETUP_REQUIRED);
                return false;
            }
        }

        if ((ftContext = ftdi_new()) == NULL)
        {
            //Failed to initialise libftdi
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_LIBFTDI_INIT_FAILED);
            return false;
        }

        //Check that the serial number is valid
        QString strFTDISerial = spiSerialInfo.serialNumber().left(FTDI_DEVICE_SERIAL_NUMBER_SIZE);
        if (!strFTDIOverride.isNull() && !strFTDIOverride.isEmpty())
        {
            //User provided FTDI ID over-ride
            strFTDISerial = strFTDIOverride;
        }

        //Open the serial device
        nStatus = ftdi_usb_open_desc(ftContext, FTDI_DEVICE_VENDOR_ID, FTDI_DEVICE_PRODUCT_ID, NULL, strFTDISerial.toStdString().c_str());

        if (nStatus < LIBFTDI_ERROR_CODE_USB_OPEN_DESC_SUCCESS && nStatus != LIBFTDI_ERROR_CODE_USB_OPEN_DESC_UNABLE_TO_CLAIM)
        {
            //Failed to open FTDI device
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_OPEN_FAILED);
            return false;
        }

        //Enable bitbang mode
        if (nType == ENTER_BOOTLOADER_BL654_USB)
        {
            nStatus = ftdi_set_bitmode(ftContext, (FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO | FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_BL654_AUTORUN_DIO), BITMODE_BITBANG);
        }
        else if (nType == ENTER_BOOTLOADER_PINNACLE100)
        {
            nStatus = ftdi_set_bitmode(ftContext, (FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO | FTDI_USB_PINNACLE100_ENTER_BL_DIO | FTDI_USB_NRESET_DIO), BITMODE_BITBANG);
        }

        if (nStatus != LIBFTDI_ERROR_CODE_SET_BITMODE_SUCCESS)
        {
            //Failed to set bitbang mode
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_BITBANG_MODE_FAILED);
            return false;
        }

        //Set writes to 1 byte chunks
        nStatus = ftdi_write_data_set_chunksize(ftContext, sizeof(baTxBuffer));
        if (nStatus != LIBFTDI_ERROR_CODE_SET_CHUNKSIZE_SUCCESS)
        {
            //Failed to set chunk size
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_CHUNK_SIZE_SET_FAILED);
            return false;
        }

        //Disable autorun
        if (nType == ENTER_BOOTLOADER_BL654_USB)
        {
            baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        else if (nType == ENTER_BOOTLOADER_PINNACLE100)
        {
            baTxBuffer = FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        nStatus = ftdi_write_data(ftContext, &baTxBuffer, sizeof(baTxBuffer));
        if (nStatus <= LIBFTDI_ERROR_CODE_WRITE_DATA_USB_BULK_WRITE_ERROR)
        {
            //Failed to write data
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
            return false;
        }
        usleep(FTDI_USB_RESET_DELAY_LINUX_US);

        //Reset module
        if (nType == ENTER_BOOTLOADER_BL654_USB)
        {
            baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        else if (nType == ENTER_BOOTLOADER_PINNACLE100)
        {
            baTxBuffer = FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        nStatus = ftdi_write_data(ftContext, &baTxBuffer, sizeof(baTxBuffer));
        if (nStatus <= LIBFTDI_ERROR_CODE_WRITE_DATA_USB_BULK_WRITE_ERROR)
        {
            //Failed to write data
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
            return false;
        }
        usleep(FTDI_USB_RESET_DELAY_LINUX_US);

        //Exit reset
        if (nType == ENTER_BOOTLOADER_BL654_USB)
        {
            baTxBuffer = FTDI_USB_BL654_VSP_DIO | FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        else if (nType == ENTER_BOOTLOADER_PINNACLE100)
        {
            baTxBuffer = FTDI_USB_NRESET_DIO | FTDI_USB_RX_DIO | FTDI_USB_RTS_DIO;
        }
        nStatus = ftdi_write_data(ftContext, &baTxBuffer, sizeof(baTxBuffer));
        if (nStatus <= LIBFTDI_ERROR_CODE_WRITE_DATA_USB_BULK_WRITE_ERROR)
        {
            //Failed to write data
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED);
            return false;
        }
        usleep(FTDI_USB_RESET_DELAY_LINUX_US);

        //Exit bitbang mode
        nStatus = ftdi_set_bitmode(ftContext, LIBFTDI_BITMODE_BITMASK_RESET, BITMODE_RESET);
        if (nStatus != LIBFTDI_ERROR_CODE_SET_BITMODE_SUCCESS)
        {
            //Failed to reset bitbang mode
            ftdi_usb_reset(ftContext);
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_BITBANG_MODE_FAILED);
            return false;
        }

        //Reset the FTDI device
        nStatus = ftdi_usb_reset(ftContext);
        if (nStatus != LIBFTDI_ERROR_CODE_RESET_SUCCESS)
        {
            //Failed to reset device
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_USB_RESET_FAILED);
            return false;
        }
        nStatus = ftdi_usb_close(ftContext);
        if (nStatus != LIBFTDI_ERROR_CODE_CLOSE_SUCCESS)
        {
            //Failed to close port
            ftdi_free(ftContext);
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_CLOSE_FAILED);
            return false;
        }

        //Free the FTDI context
        ftdi_free(ftContext);

        //Initialise libusb to reset driver
        nStatus = libusb_init(&usbContext);
        if (nStatus != LIBUSB_SUCCESS)
        {
            //libusb initialisation failed
            emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_LIBFTDI_INIT_FAILED);
            return false;
        }

        //Search for the target device
        nDevicesFound = libusb_get_device_list(usbContext, &usbdDevice);
        for (ssize_t idx = 0; idx < nDevicesFound; ++idx)
        {
            libusb_device *device = usbdDevice[idx];
            struct libusb_device_descriptor desc;

            nStatus = libusb_get_device_descriptor(device, &desc);
            if (nStatus != LIBUSB_SUCCESS)
            {
                //Failed to get device descriptor
                emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DEVICE_DESCRIPTOR_FETCH_FAILED);
                return false;
            }

            //Check device is an FTDI adapter
            if (desc.idVendor == FTDI_DEVICE_VENDOR_ID && desc.idProduct == FTDI_DEVICE_PRODUCT_ID)
            {
                //FTDI device, check serial number
                libusb_device_handle *handle = NULL;
                nStatus = libusb_open(device, &handle);
                if (nStatus != LIBUSB_SUCCESS)
                {
                    //Failed to open USB device
                    libusb_exit(usbContext);
                    emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_OPEN_FAILED);
                    return false;
                }

                nStatus = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, strSerialNumber, sizeof(strSerialNumber)-1);
                if (nStatus < LIBUSB_SUCCESS)
                {
                    libusb_close(handle);
                    libusb_exit(usbContext);
                    emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_STRING_DESCRIPTOR_FAILED);
                    return false;
                }

                if (strcmp((char *)strSerialNumber, (char *)strFTDISerial.toStdString().c_str()) == 0)
                {
                    //Found the USB device
                    if (libusb_kernel_driver_active(handle, FTDI_LIBUSB_INTERFACE_NUMBER))
                    {
                        //Relinquish FTDI bitbang driver
                        nStatus = libusb_detach_kernel_driver(handle, FTDI_LIBUSB_INTERFACE_NUMBER);
                    }
                    nStatus = libusb_claim_interface(handle, FTDI_LIBUSB_INTERFACE_NUMBER);
                    nStatus = libusb_release_interface(handle, FTDI_LIBUSB_INTERFACE_NUMBER);

                    //Revert to default (UART) driver
                    nStatus = libusb_attach_kernel_driver(handle, FTDI_LIBUSB_INTERFACE_NUMBER);

                    //Clean up and finish
                    libusb_close(handle);
                    libusb_exit(usbContext);

                    if (pNewSerialPort != NULL)
                    {
                        //Return the new serial port name as it might be different from the orignal if devices have been reordered or plugged in/out during the process
                        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
                        {
                            if (info.serialNumber() == strOldSerialPortSerialNumber)
                            {
                                *pNewSerialPort = info.portName();
                            }
                        }
                    }

                    //Wait for device to reset
                    usleep(FTDI_USB_RESET_DELAY_LINUX_US);

                    //Finished
                    return true;
                }

                //Close device
                libusb_close(handle);
            }
        }

        //Device was not found
        libusb_exit(usbContext);
        emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_DEVICE_NOT_FOUND);
        return false;
    }
    else
    {
        //Invalid or non-FTDI device
        emit Error(MODULE_BOOTLOADER_ENTRANCE, EXIT_CODE_BOOTLOADER_ENTRANCE_INVALID_DEVICE);
        return false;
    }
#endif
    return true;
}
#endif

#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
