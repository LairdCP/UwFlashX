/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  LrdErr.h
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
#ifndef LRDERR_H
#define LRDERR_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include "LrdFwCommon.h"

/******************************************************************************/
// Condition Compile Defines
/******************************************************************************/
#ifdef MALLOC_DEBUGGING
#include <QDebug>
#define MallocFailCheck(pPointer) \
{ \
    if (pPointer == NULL) \
    { \
        qDebug().noquote().nospace() << "Malloc failure - file: " << __FILE__ << " line: " << __LINE__; \
    } \
    LrdErr::MallocFailHandler(pPointer); \
}
#else
#define MallocFailCheck(pPointer) LrdErr::MallocFailHandler(pPointer)
#endif

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdErr : public QObject
{
    Q_OBJECT
public:
    explicit
    LrdErr(
        QObject *parent = nullptr
        );
    ~LrdErr(
        );
    QString
    ErrorCodeToString(
        int32_t nErrorCodeID,
        bool bShowErrorCode
        );
    static
    void
    MallocFailHandler(
        void *pPointer
        );
};

#endif // LRDERR_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
