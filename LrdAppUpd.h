/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdAppUpd.h
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
#ifndef LRDAPPUPD_H
#define LRDAPPUPD_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include "LrdFwCommon.h"
#include "LrdSettings.h"
#ifdef UseSSL
#include <QSslSocket>
#endif

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
enum VERSION_RESPONSES
{
    VERSION_RESPONSE_CURRENT,
    VERSION_RESPONSE_OUTDATED,
    VERSION_RESPONSE_ERROR
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdAppUpd : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdAppUpd(
        QObject *parent = nullptr,
        LrdSettings *pSettings = nullptr
        );
    ~LrdAppUpd(
        );
    void
    SetSettingsObject(
        LrdSettings *pSettings
        );
#ifdef USE_DNS_LOOKUP_FUNCTION
    bool
    LookupDNSName(
        );
#endif
    bool
    UpdateCheck(
#ifdef UseSSL
        bool bUseSSL,
#endif
        QString strAppName,
        QString strAppVersion
        );

signals:
    void
    Error(
        uint32_t nModule,
        int32_t nErrorCode
        );
    void
    VersionResponse(
        VERSION_RESPONSES nResponse,
        QString strNewVersion
        );

private slots:
    void
    replyFinished(
        QNetworkReply* nrReply
        );
#ifdef UseSSL
    void
    sslErrors(
        QNetworkReply* nrReply,
        QList<QSslError> lstSSLErrors
        );
#endif

private:
    LrdSettings             *pSettingsHandle          = NULL;    //Settings object
    QNetworkAccessManager   *gnmManager;                         //Network access manager
    QNetworkReply           *gnmrReply;                          //Network reply
    QString                 gstrResolvedServer;                  //Holds the resolved hostname of the XCompile server
#ifdef UseSSL
    QSslCertificate         *sslcLairdConnectivitySSL = NULL;    //Holds the Laird Connectivity UwTerminalX server SSL certificate
#endif
};

#endif // LRDAPPUPD_H
/******************************************************************************/
// END OF FILE
/******************************************************************************/
