/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdAppUpd.cpp
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
#include "LrdAppUpd.h"

//=============================================================================
// Constructor
//=============================================================================
LrdAppUpd::LrdAppUpd(
    QObject *parent,
    LrdSettings *pSettings
    ) : QObject(parent)
{
    pSettingsHandle = pSettings;

    //Setup QNetwork object
    gnmManager = new QNetworkAccessManager();
    connect(gnmManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
#ifdef UseSSL
    connect(gnmManager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
#endif

#ifdef UseSSL
    //Load SSL certificate
    QFile certFile(":/certificates/UwTerminalX_new.crt");
    if (certFile.open(QIODevice::ReadOnly))
    {
        //Load certificate data
        QSslConfiguration sslcConfig;
        sslcLairdConnectivitySSL = new QSslCertificate(certFile.readAll());
        sslcConfig.addCaCertificate(*sslcLairdConnectivitySSL);
        certFile.close();
    }
#endif
}

//=============================================================================
// Destructor
//=============================================================================
LrdAppUpd::~LrdAppUpd(
    )
{
    if (gnmManager != NULL)
    {
        //Clear up network manager
        disconnect(this, SLOT(replyFinished(QNetworkReply*)));
#ifdef UseSSL
        disconnect(this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
#endif
        delete gnmManager;
    }
}

//=============================================================================
// Set the settings handle
//=============================================================================
void
LrdAppUpd::SetSettingsObject(
    LrdSettings *pSettings
    )
{
    pSettingsHandle = pSettings;
}

#ifdef USE_DNS_LOOKUP_FUNCTION
//=============================================================================
// Legacy DNS lookup function which was previously used to avoid a segfault
//=============================================================================
bool
LrdAppUpd::LookupDNSName(
    )
{
    //Function to lookup hostname of the cloud XCompile server (a workaround for a bug causing a segmentation fault on Linux)
    if (gstrResolvedServer == "")
    {
        //Name not yet resolved
        QHostInfo hiIP = QHostInfo::fromName(pSettingsHandle->GetConfigOption(UPDATE_SERVER_HOST).toString());
        if (hiIP.error() == QHostInfo::NoError)
        {
            //Resolved hostname
            if (hiIP.addresses().isEmpty())
            {
                //No results returned
                emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_IP_RESOLUTION_NO_ADDRESS);
                return false;
            }
            else
            {
                //Found the host
                gstrResolvedServer = hiIP.addresses().first().toString();

                //Setup QNetwork object
                gnmManager = new QNetworkAccessManager();
                connect(gnmManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
#ifdef UseSSL
                connect(gnmManager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
#endif
                return true;
            }
        }
        else
        {
            //Failed to resolve hostname
            emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_IP_RESOLUTION_FAILED);
            return false;
        }
    }
    return true;
}
#endif

//=============================================================================
// Check for any updates to the application
//=============================================================================
bool
LrdAppUpd::UpdateCheck(
#ifdef UseSSL
    bool bUseSSL,
#endif
    QString strAppName,
    QString strAppVersion
    )
{
    //Send request to check for UwTerminalX updates
#ifdef USE_DNS_LOOKUP_FUNCTION
    if (LookupDNSName() == true)
#endif
    {
        gnmrReply = gnmManager->get(QNetworkRequest(QUrl(QString("http").
#ifdef UseSSL
            append((bUseSSL == true ? "s" : "")).
#endif
            append("://").append(
#ifdef USE_DNS_LOOKUP_FUNCTION
        gstrResolvedServer
#else
        pSettingsHandle->GetConfigOption(UPDATE_SERVER_HOST).toString()
#endif
        ).append("/update_app.php?App=").append(strAppName).append("&Ver=").append(strAppVersion).append("&OS=").append(
#ifdef _WIN32
    //Windows
    #ifdef _WIN64
        //Windows 64-bit
        "W64"
    #else
        //Windows 32-bit
        "W32"
    #endif
#elif __APPLE__
        //OSX
        "OSX"
#else
    //Assume Linux
    #ifdef __aarch64__
        //ARM64
        "LxARM64"
    #elif __arm__
        //ARM
        "LxARM"
    #elif __x86_64__
        //x86_64
        "Lx86_64"
    #elif __i386
        //x86
        "Lx86"
    #else
        //Unknown
        "LxOth"
    #endif
#endif
        ))));

        return true;
    }

    return false;
}

//=============================================================================
// Callback when a network request reply has been received
//=============================================================================
void
LrdAppUpd::replyFinished(
    QNetworkReply* nrReply
    )
{
    //Response received from online server
    if (nrReply->error() != QNetworkReply::NoError && nrReply->error() != QNetworkReply::ServiceUnavailableError)
    {
        //An error occured: send error message message if operation wasn't cancelled
        if (nrReply->error() != QNetworkReply::OperationCanceledError)
        {
            //Output error message
            emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_HTTP_RESPONSE_CODE_ERROR);
            emit VersionResponse(VERSION_RESPONSE_ERROR, nrReply->errorString());
        }
    }
    else
    {
        //Update response
        QByteArray baTmpBA = nrReply->readAll();
        QJsonParseError jpeJsonError;
        QJsonDocument jdJsonData = QJsonDocument::fromJson(baTmpBA, &jpeJsonError);

        if (jpeJsonError.error == QJsonParseError::NoError)
        {
            //Decoded JSON
            QJsonObject joJsonObject = jdJsonData.object();

            if (joJsonObject["Result"].toString() == "-1")
            {
                //Outdated version
                emit VersionResponse(VERSION_RESPONSE_OUTDATED, joJsonObject["Version"].toString());
            }
            else if (joJsonObject["Result"].toString() == "-2")
            {
                //Server error
                emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_HTTP_RESPONSE_SERVER_ERROR);
                emit VersionResponse(VERSION_RESPONSE_ERROR, "");
            }
            else if (joJsonObject["Result"].toString() == "1")
            {
                //Version is OK
                emit VersionResponse(VERSION_RESPONSE_CURRENT, "");
            }
            else
            {
                //Server responded with error
                emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_HTTP_RESPONSE_ERROR);
                emit VersionResponse(VERSION_RESPONSE_ERROR, joJsonObject["Error"].toString());
            }
        }
        else
        {
            //Error whilst decoding JSON
            emit Error(MODULE_APPLICATION_UPDATE, EXIT_CODE_HTTP_RESPONSE_NOT_VALID_JSON);
            emit VersionResponse(VERSION_RESPONSE_ERROR, "");
        }
    }

    //Queue the network reply object to be deleted
    nrReply->deleteLater();
}

//=============================================================================
// Callback if an SSL error is encountered
//=============================================================================
#ifdef UseSSL
void
LrdAppUpd::sslErrors(
    QNetworkReply* nrReply,
    QList<QSslError> lstSSLErrors
    )
{
    //Error detected with SSL
    if (sslcLairdConnectivitySSL != NULL && nrReply->sslConfiguration().peerCertificate() == *sslcLairdConnectivitySSL)
    {
        //Server certificate matches
        nrReply->ignoreSslErrors(lstSSLErrors);
    }
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
