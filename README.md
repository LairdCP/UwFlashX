# Laird Connectivity UwFlashX

## About

UwFlashX is a cross-platform utility for updating firmware on Laird Connectivity's range of wireless modules, and uses Qt 5. The code uses functionality only supported in Qt 5.9 or greater. UwFlashX has been tested on Windows, Mac, Arch Linux and Ubuntu Linux and on the Raspberry Pi running Raspbian.

## Downloading

Pre-compiled builds can be found by clicking the [Releases](https://github.com/LairdCP/UwFlashX/releases) tab on Github, builds are available for Linux (32-bit and 64-bit builds, and a 32-bit ARM Raspberry Pi build), Windows (32-bit build) and Mac (64-bit build) . Please note that the SSL builds include encryption when using online functionality and should only be used in countries where encryption is legal, non-SSL builds are also available from the release page - it is important to note that the non-SSL Windows build does not support the FTDI reset method used on the BL654 USB dongle or Pinnacle 100 and the SSL version should be used instead - it is important to note that the mac build does not support the FTDI reset method used on the BL654 USB dongle or Pinnacle 100 and a different operating system should be used instead if this feature is required.

It is recommended that you download the **SSL** build of UwFlashX for your system.

## Setup

### Windows:

Download and open the zip file, extract the files to a folder on your computer and double click 'UwFlashX.exe' to run UwFlashX.

If using the SSL version of UwFlashX, then the Visual Studio 2015 runtime files are required which are available on the [Microsoft site](https://www.microsoft.com/en-gb/download/details.aspx?id=48145).

### Mac:

(**Mac OS X version 10.10 or later is required if using the pre-compiled binaries, as Secure Transport is built into OS X there is no non-SSL build available for OS X systems**): Download and open the dmg file, open it to mount it as a drive on your computer, go to the disk drive and copy the file UwFlashX to folder on your computer. You can run UwFlashX by double clicking the icon - if you are greeted with a warning that the executable is untrusted then you can run it by right clicking it and selecting the 'run' option. If this does not work then you may need to view the executable security settings on your mac.

### Linux (Including Raspberry Pi):

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf UwFlashX_<version>.tar.gz -C ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch UwFlashX, either double click on the executable and click the 'run' button (if asked), or execute it from a terminal as so:

	./UwFlashX

Before running, you may need to install some additional libraries, please see https://github.com/LairdCP/UwTerminalX/wiki/Installing for further details. You may also be required to add a udev rule to grant non-root users access to USB devices, please see https://github.com/LairdCP/UwTerminalX/wiki/Granting-non-root-USB-device-access-(Linux) for details.

## Module configuration

Modules must have UART access with hardware flow control lines in order to be upgraded using UwFlashX. Any modules requiring FTDI-reset functionality are not supported on mac and for Windows requires the SSL build. There is information on the wiki which explains [how to configure UwFlashX to upgrade specific modules](https://github.com/LairdCP/UwFlashX/wiki/UwFlashX-Setup).

## Help and contributing

There are various instructions and help pages available on the [wiki site](https://github.com/LairdCP/UwFlashX/wiki/), additionally some of the pages on the [UwTerminalX wiki site](https://github.com/LairdCP/UwTerminalX/wiki/) are also applicable to UwFlashX.

Laird Connectivity encourages people to branch/fork UwFlashX to modify the code and accepts pull requests to merge these changes back into the main repository.

## Mailing list/discussion

There is a mailing list available for dicussion about UwTerminalX and UwFlashX which is available on https://groups.io/g/UwTerminalX

## Companian Applications

 * [UwTerminalX](https://github.com/LairdCP/UwTerminalX): a cross-platform utility for communicating and downloading applications onto Laird Connectivity's range of wireless modules
 * [MultiDeviceLoader](https://github.com/LairdCP/MultiDeviceLoader): an application that can be used to XCompile a file and download it to multiple modules at the same time with various options including running the application or renaming it.

## Compiling

For details on compiling, please refer to [the UwTerminalX wiki](https://github.com/LairdCP/UwTerminalX/wiki/Compiling) and adapt the commands for the UwFlashX repository.

## License

UwFlashX is released under the [GPLv3 license](https://github.com/LairdCP/UwFlashX/blob/master/LICENSE).