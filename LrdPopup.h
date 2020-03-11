/******************************************************************************
** Copyright (C) 2015-2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module: LrdPopup.h
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
#ifndef LRDPOPUPMESSAGE_H
#define LRDPOPUPMESSAGE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>

enum POPUP_BUTTONS
{
    POPUP_LEFT_BUTTON       = 0,
    POPUP_RIGHT_BUTTON,
    POPUP_BOTH_BUTTONS,
    POPUP_ESCAPE
};

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class LrdPopupMessage;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdPopupMessage : public QDialog
{
    Q_OBJECT

public:
    explicit LrdPopupMessage(
        QWidget *parent = 0
        );
    ~LrdPopupMessage(
        );
    void
    SetMessage(
        QString *strMsg
        );
    void
    SetButtonVisibility(
        uint8_t nButtons
        );
    void
    SetButtonText(
        uint8_t nButton,
        QString strText
        );
    void
    SetPopupID(
        uint8_t nID
        );
    void
    SetEscapeKeyHandling(
        bool AllowClose
        );
    void
    SetDefaultButton(
        uint8_t nButton
        );
    void
    reject(
        );

private slots:
    void
    on_btn_Left_Button_clicked(
        );
    void
    on_btn_Right_Button_clicked(
        );

signals:
    void
    Button_Clicked(
        uint8_t nPopupID,
        uint8_t nButton
        );

private:
    Ui::LrdPopupMessage *ui;
    uint8_t nPopupID;
    bool bAllowCloseWithEscape;
    uint8_t nButtonSelected;
};

#endif // LRDPOPUPMESSAGE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
