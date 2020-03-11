/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdErr.cpp
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
#include <QString>
#include "LrdErr.h"
#include "LrdFwCommon.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Constructor
//=============================================================================
LrdErr::LrdErr(
    QObject *parent
    ) : QObject(parent)
{
}

//=============================================================================
// Destructor
//=============================================================================
LrdErr::~LrdErr(
    )
{
}

//=============================================================================
// Error handling function
//=============================================================================
QString
LrdErr::ErrorCodeToString(
    int32_t nErrorCodeID,
    bool bShowErrorCode
    )
{
    QString strError = "";

    if (nErrorCodeID == EXIT_CODE_SUCCESS)
    {
        //No error
        return strError;
    }

    if (bShowErrorCode == true)
    {
        strError = "Error code ";
        strError.append(QString::number(nErrorCodeID, 10));

        if (nErrorCodeID > EXIT_CODE_SUCCESS)
        {
            //Include hex code for FUP errors
            strError.append(" (0x");
            strError.append(QString::number(nErrorCodeID, 16));
            strError.append(")");
        }
        strError.append(": ");
    }

    if (nErrorCodeID < EXIT_CODE_SUCCESS)
    {
        //Application error code, account for 0-index
        --nErrorCodeID;

        if (nErrorCodeID < EXIT_CODE_BOTTOM_COUNT)
        {
            //Unknown error code
            strError.append("<Unknown error>");
        }
        else
        {
            //Append error text
            strError.append(pErrorStrings[nErrorCodeID-EXIT_CODE_BOTTOM_COUNT]);
        }
    }
    else if (nErrorCodeID > EXIT_CODE_SUCCESS)
    {
        //FUP error code
        if (nErrorCodeID >= FUP_ATMEL_ERROR_MAX)
        {
            //Unknown error
            strError.append("<Unknown error>");
        }
        else
        {
            //Append error text
            strError.append(pFUPErrorStrings[nErrorCodeID-FUP_ERROR_WRITE]);
        }
    }

    return strError;
}

//=============================================================================
// Will throw an exception if a malloc failure occurs
//=============================================================================
void
LrdErr::MallocFailHandler(
    void *pPointer
    )
{
    if (pPointer == NULL)
    {
        throw std::bad_alloc();
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
