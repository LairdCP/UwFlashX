/******************************************************************************
** Copyright (C) 2019 Laird Connectivity
**
** Project: UwFlashX
**
** Module:  main.cpp
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

#if defined(SKIPGUI)
#error "SKIPGUI is not currently a supported build configuration."
#endif

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "mainwindow.h"
#include <QApplication>

//=============================================================================
//=============================================================================
int
main(
    int argc,
    char *argv[]
    )
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
