/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdFwCommon.h
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
#ifndef LRDFWCOMMON_H
#define LRDFWCOMMON_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QString>
#include <QtNetwork>

/******************************************************************************/
// Constants
/******************************************************************************/
const QString gstrOrganisationName     = "Laird Connectivity";
const QString gstrURLLinuxNonRootSetup = "https://github.com/LairdCP/UwTerminalX/wiki/Granting-non-root-USB-device-access-(Linux)";

/******************************************************************************/
// Type definitons
/******************************************************************************/
typedef char            ByteArrayType;          //For MSVC

/******************************************************************************/
// Macros
/******************************************************************************/
#define COMPILE_ASSERT(i) extern uint32_t compile_assert_test[(uint32_t)i]

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
//Error codes which map up with the FUP module
enum FUP_ERROR_CODES
{
    FUP_ERROR_WRITE                                 = 0x01,
    FUP_ERROR_READ,
    FUP_ERROR_ERASE,
    FUP_ERROR_UNRECOGNISED,
    FUP_ERROR_PLATFORM,
    FUP_ERROR_BYPASS_WRITE,
    FUP_ERROR_BYPASS_NOTEMPTY,
    FUP_ERROR_INVALID_ADDRESS,
    FUP_ERROR_INVALID_KEY,
    FUP_ERROR_INVALID_SOFTWARE_ID,
    FUP_ERROR_ERASE_QSPI_CMD_FAIL,
    FUP_ERROR_ERASE_FLASH_ERASE_FAIL,
    FUP_ERROR_ERASE_QSPI_ERASE_FAIL,
    FUP_ERROR_ERASE_UNKNOWN,
    FUP_ERROR_TIMER_FAILED,
    FUP_ERROR_PARTITION_NOT_LOADED,
    FUP_ERROR_INVALID_QUERY_ID,
    FUP_ERROR_INVALID_QUERY_SUB_ID,
    FUP_ERROR_INVALID_PARTITON,
    FUP_ERROR_PARTITON_EMPTY,
    FUP_ERROR_REGRESSION_TEST_FAIL,
    FUP_ERROR_DEBUGOUTPUT_UART_SIZE,
    FUP_ERROR_DEBUGOUTPUT_ERR,
    FUP_ERROR_WORM_NOT_SET,
    FUP_ERROR_WORM_SET,
    FUP_ERROR_WORM_INVALID_ID,
    FUP_ERROR_WORM_INVALID_HEX,
    FUP_ERROR_WORM_INVALID_DATA_SIZE,
    FUP_ERROR_WRITE_PROTECTED,
    FUP_ERROR_READ_ACCESS_DENIED,
    FUP_ERROR_NO_LICENSE,
    FUP_ERROR_INVALID_VALUE,
    FUP_ERROR_READBACK_PROTECTED,
    FUP_ERROR_READ_SIZE_TOO_LARGE,
    FUP_ERROR_WRITE_SIZE_TOO_LARGE,
    FUP_ERROR_ERASE_CONFIRM_NOT_VALID,
    FUP_ERROR_CHECKSUM,
    FUP_ERROR_INVALID_BRIDGE_FLOW,
    FUP_ERROR_INVALID_BRIDGE_BAUD,
    FUP_ERROR_BRIDGE_FAILED,
    FUP_ERROR_INPUT_WRITE_NOT_ALLOWED,
    FUP_ERROR_FLASH_LOCKED_WITH_KEY,
    FUP_ATMEL_ERROR_FLASH_KEY_MISSING,
    FUP_ATMEL_ERROR_FULL_ERASE_DISABLED,
    FUP_ATMEL_ERROR_HARDWARE_ERROR,
    FUP_ATMEL_ERROR_FLASH_SECTION_IN_USE,
    FUP_ATMEL_ERROR_FLASH_RESERVED_FOR_USER,
    FUP_ATMEL_ERROR_HARDWARE_NOT_INITIALISED,
    FUP_ATMEL_ERROR_TOO_MANY_ATTEMPTS_DENIED,
    FUP_ATMEL_ERROR_VERIFY_AREA_TOO_SMALL,

    FUP_ATMEL_ERROR_MAX
};

//Error strings for above error codes - MUST be a 1:1 mapping
static QString pFUPErrorStrings[] = {
    "Write failed",
    "Read failed",
    "Erase failed",
    "Unrecognised command",
    "Invalid platform",
    "Bypass write",
    "Bypass not empty",
    "Supplied address is not valid",
    "Supplied key does not exist",
    "Supplied software ID does not exist",
    "Erase failed (hardware error)",
    "Erase failed (hardware error)",
    "Erase failed (hardware error)",
    "Erase failed (hardware error)",
    "Timer creation failed",
    "Partition data is not loaded",
    "Invalid query ID",
    "Invalid query sub ID",
    "Supplied partition ID is invalid",
    "No partition exists with supplied ID",
    "Regression test failed",
    "Trying to output too much data",
    "Error whilst outputting data",
    "WORM value not set",
    "WORM value already set",
    "WORM ID not valid",
    "WORM value requires hex data",
    "WORM value size not valid",
    "Write protection active",
    "Read access denied",
    "Module does not have a valid license",
    "Supplied value is incorrect",
    "Readback protection is active",
    "Specified read size is too large",
    "Specified write size is too large",
    "Erase confirmation code not valid",
    "Supplied checksum is incorrect",
    "Bridge flow control value not value",
    "Bridge baud rate value not value",
    "Bridge failed",
    "Read only value, write not allowed",
    "Flash is locked with a key"
    "Flash key is missing",
    "Full erase is not allowed",
    "Hardware error",
    "Flash section is in use",
    "Flash section is reserved for user-application use",
    "Hardware is not initialised",
    "Invalid, access denied",
    "Verification area is too small"
};
//If you see a compile error here it is because the above list has not been adjusted and is incorrect
COMPILE_ASSERT((sizeof(pFUPErrorStrings)/sizeof(pFUPErrorStrings[0])) == (FUP_ATMEL_ERROR_MAX-FUP_ERROR_WRITE-1));

//Module codes
enum MODULES
{
    MODULE_UART = 0,
    MODULE_UPDATE,
    MODULE_UWF,
    MODULE_SETTINGS,
    MODULE_APPLICATION_UPDATE,
    MODULE_BOOTLOADER_ENTRANCE
};

//Exit codes - add new codes to the top ONLY and move the lowest number to the top line and decrement appropriately
enum EXIT_CODES
{
    //Always leave this element here and decrement it when a new error code is added
    EXIT_CODE_BOTTOM_COUNT = -46,

    //Add new error codes below here at the top
    EXIT_CODE_ERASE_SECTOR_MAPPING_NOT_FOUND,
    EXIT_CODE_BOOTLOADER_UNLOCK_KEY_INVALID_SIZE,
    EXIT_CODE_BOOTLOADER_ENTRANCE_STRING_DESCRIPTOR_FAILED,
    EXIT_CODE_BOOTLOADER_VERIFICATION_FAILED,
    EXIT_CODE_RETURN_CODE_ERROR,
    EXIT_CODE_CTS_TIMEOUT,
    EXIT_CODE_BAUD_RATE_ERROR,
    EXIT_CODE_UWF_FILE_NOT_FOUND,
    EXIT_CODE_UWF_FILE_FAILED_TO_OPEN,
    EXIT_CODE_UWF_FILE_INVALID_SIZE,
    EXIT_CODE_UWF_FILE_COMMAND_INVALID,
    EXIT_CODE_UWF_FILE_PACKET_LENGTH_INVALID,
    EXIT_CODE_UWF_FILE_NOT_VALID,
    EXIT_CODE_SERIAL_PORT_FAILED_TO_OPEN,
    EXIT_CODE_SERIAL_PORT_COMMAND_TIMEOUT,
    EXIT_CODE_SERIAL_PORT_EXACT_BAUD_NOT_FOUND,
    EXIT_CODE_SERIAL_PORT_MAX_BAUD_UNSUITABLE,
    EXIT_CODE_SERIAL_PORT_DEVICE_UNPLUGGED,
    EXIT_CODE_SERIAL_PORT_REOPEN_FAILED,
    EXIT_CODE_SETTINGS_HANDLE_NULL,
    EXIT_CODE_INVALID_SETTINGS_ID,
    EXIT_CODE_INVALID_SETTINGS_TYPE,
    EXIT_CODE_INVALID_SETTINGS_NOT_SET,
    EXIT_CODE_IP_RESOLUTION_FAILED,
    EXIT_CODE_IP_RESOLUTION_NO_ADDRESS,
    EXIT_CODE_HTTP_RESPONSE_CODE_ERROR,
    EXIT_CODE_HTTP_RESPONSE_NOT_VALID_JSON,
    EXIT_CODE_HTTP_RESPONSE_SERVER_ERROR,
    EXIT_CODE_HTTP_RESPONSE_ERROR,
    EXIT_CODE_BOOTLOADER_ENTRANCE_CANCELLED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_BITBANG_MODE_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_OPEN_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_DATA_WRITE_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_PORT_CLOSE_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_INVALID_DEVICE,
    EXIT_CODE_BOOTLOADER_ENTRANCE_WRONG_COMPILER,
    EXIT_CODE_BOOTLOADER_ENTRANCE_LIBFTDI_INIT_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_DEVICE_DESCRIPTOR_FETCH_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_DEVICE_NOT_FOUND,
    EXIT_CODE_BOOTLOADER_ENTRANCE_SERIAL_NUMBER_NOT_VALID,
    EXIT_CODE_BOOTLOADER_ENTRANCE_CHUNK_SIZE_SET_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_USB_RESET_FAILED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_SETUP_REQUIRED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_NOT_SUPPORTED,
    EXIT_CODE_BOOTLOADER_ENTRANCE_FAILED,
    EXIT_CODE_SUCCESS,
    EXIT_CODE_ERROR_CODE_BASE                                       = 0x100
};
//If you see a compile error here it is because the above list has been adjusted and is incorrect
COMPILE_ASSERT(EXIT_CODE_SUCCESS == 0);

//This is a list of error strings which match up with the error codes above - IN THE SAME ORDER
//EXIT_CODE_BOTTOM_COUNT is not part of this list and neither is EXIT_CODE_ERROR_CODE_BASE
//The last description should be for EXIT_CODE_SUCCESS, this list is in descending order
static QString pErrorStrings[] = {
    "A sector mapping was not found when attempting to erase sector data",
    "Specified bootloader unlock key length is not valid",
    "USB get string descriptor failed",
    "Data verification failed",
    "Firmware upgrade failed",
    "CTS timeout whilst attempting to communicate with module",
    "Failed to open serial port at specified baud rate",
    "Specified Uwf/Ubu file was not found",
    "Specified Uwf/Ubu file could not be opened for read access",
    "Specified Uwf/Ubu file is too small or large",
    "Specified Uwf/Ubu file has an invalid command",
    "Specified Uwf/Ubu file has a command with an invalid length",
    "Specified Uwf/Ubu file is not valid",
    "Serial port failed to open",
    "Command sent to module has timed out",
    "Exact baud-rate not supported by module or serial port",
    "Failed to select a baudrate under the defined maximum",
    "Serial device has been unplugged",
    "Serial port re-open failed",
    "Settings handle is null",
    "Settings key does not exist",
    "Settings type does not match existing type",
    "Settings value has not been set",
    "IP resolution failed",
    "IP resolution completed by returned no IP address",
    "HTTP response error",
    "HTTP responded with invalid JSON data",
    "HTTP responded with server error",
    "HTTP response error",
    "Enter bootloader operation cancelled by user",
    "FTDI bitbang mode set failed",
    "USB port open failed",
    "USB data write failed",
    "USB port close failed",
    "Invalid USB device specified",
    "FTDI library not available for enter bootloader operation",
    "LibFTDI initialisation failed",
    "USB device descriptor fetch failed",
    "USB device was not found",
    "FTDI serial number is not valid",
    "USB chunk size set failed",
    "USB reset failed",
    "Further system setup required",
    "FTDI-based reset functionality not supported on Mac, please run on Linux/Windows",
    "Bootloader entrance failed",
    "No error"
};
//If you see a compile error here it is because the above list has not been adjusted and is incorrect
COMPILE_ASSERT((sizeof(pErrorStrings)/sizeof(pErrorStrings[0])) == (-EXIT_CODE_BOTTOM_COUNT));

/******************************************************************************/
// Defines
/******************************************************************************/

//Verbosity levels
#define VERBOSITY_NONE                                0
#define VERBOSITY_MODES                               1
#define VERBOSITY_COMMANDS                            2
#define VERBOSITY_TIMEOUTS                            3

//Return codes when running process command functions
#define FUNCTION_RETURN_CODE_SUCCESS_DONE             0
#define FUNCTION_RETURN_CODE_SUCCESS_NEXT_PACKET      1
#define FUNCTION_RETURN_CODE_INVALID_LENGTH           -2
#define FUNCTION_RETURN_CODE_SECTOR_MAPPING_NOT_FOUND -3

//The period in ms in which a command is considered timed out
#define COMMAND_TIMEOUT_PERIOD_MS                     2000

//Period used for the restart phase of the module
#define RESTART_TIMER_PERIOD_MS                       250

//Number of bits per byte in the serial protocol, used for timeout assessment
#define SERIAL_BITS_PER_BYTE                          10

//Spreading factor when used to calculate timeouts 100-x, a value of 80 = allow 20% extra
#define SERIAL_TIMEOUT_SPREAD_FACTOR                  80

//Number of device ready checks to perform before failing upgrade
#define DEVICE_READY_CHECKS_BEFORE_FAILING            20

//Period used for delayed exit timer in ms
#define DELAYED_EXIT_TIME_MS                          5

//Maximum size of a valid uwf file, 16MB
#define UWF_FILE_MAX_SIZE_BYTES                       16777216

//Maximum size of a single packet, 10MB
#define UWF_FILE_MAX_PACKET_SIZE_BYTES                10485760

//BREAK on time used when optionally resetting a module
#define REBOOT_MODULE_BREAK_ON_TIME_MS                80
#define REBOOT_MODULE_BREAK_ON_TIME_US                (REBOOT_MODULE_BREAK_ON_TIME_MS * 1000)

//Endian conversion macros
#define ENDIAN_FLIP_BYTEARRAY_TO_UI64(baBuffer, nPosition, nOutput) \
    nOutput = (uint8_t)baBuffer[nPosition+7];                       \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+6];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+5];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+4];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+3];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+2];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+1];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition];

#define ENDIAN_FLIP_BYTEARRAY_TO_UI32(baBuffer, nPosition, nOutput) \
    nOutput = (uint8_t)baBuffer[nPosition+3];                       \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+2];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition+1];                      \
    nOutput = nOutput << 8;                                         \
    nOutput |= (uint8_t)baBuffer[nPosition];

#define ENDIAN_FLIP_UI16_TO_BYTEARRAY(baBuffer, nInput)             \
    baBuffer.append((uint8_t)((nInput & 0xff)));                    \
    baBuffer.append((uint8_t)((nInput & 0xff00) >> 8));

#define ENDIAN_FLIP_UI16_CHAR_TO_BYTEARRAY(baBuffer, nInput)        \
    baBuffer.append((ByteArrayType)((nInput & 0xff)));              \
    baBuffer.append((ByteArrayType)((nInput & 0xff00) >> 8));

#define ENDIAN_FLIP_UI32_TO_BYTEARRAY(baBuffer, nInput)             \
    baBuffer.append((uint8_t)((nInput & 0xff)));                    \
    baBuffer.append((uint8_t)((nInput & 0xff00) >> 8));             \
    baBuffer.append((uint8_t)((nInput & 0xff0000) >> 16));          \
    baBuffer.append((uint8_t)((nInput & 0xff000000) >> 24));

#if QT_VERSION <= 0x050900
//Older version of Qt
#if defined(__APPLE__)
#define UseSSL //Mac has SSL built in so enable it
#endif
#else
#if (QT_CONFIG(ssl) == 1)
#define UseSSL //By default enable SSL if Qt supports it (requires OpenSSL runtime libraries). Comment this line out to build without SSL support or if you get errors when communicating with the server
#endif
#endif

#endif // LRDFWCOMMON_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
