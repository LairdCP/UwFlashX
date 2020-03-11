/******************************************************************************
** Copyright (C) 2015-2020 Laird Connectivity
**
** Project: UwFlashX
**
** Module: LrdPopup.cpp
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
#include "LrdPopup.h"
#include "ui_LrdPopup.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
// Constructor
//=============================================================================
LrdPopupMessage::LrdPopupMessage(QWidget *parent):QDialog(parent), ui(new Ui::LrdPopupMessage)
{
    //Setup window to be a dialog
    this->setWindowFlags((Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
    ui->setupUi(this);

    //Set defaults
    nPopupID = 0;
    bAllowCloseWithEscape = true;
}

//=============================================================================
// Clean up destructor
//=============================================================================
LrdPopupMessage::~LrdPopupMessage(
    )
{
    delete ui;
}

//=============================================================================
// Left button has been clicked, close dialogue and send signal
//=============================================================================
void
LrdPopupMessage::on_btn_Left_Button_clicked(
    )
{
    nButtonSelected = POPUP_LEFT_BUTTON;
    this->close();
}

//=============================================================================
// Right button has been clicked, close dialogue and send signal
//=============================================================================
void
LrdPopupMessage::on_btn_Right_Button_clicked(
    )
{
    nButtonSelected = POPUP_RIGHT_BUTTON;
    this->close();
}

//=============================================================================
// Set the popup dialog message
//=============================================================================
void LrdPopupMessage::SetMessage(
    QString *strMsg
    )
{
    //Update popup message
    ui->text_Message->setPlainText(*strMsg);
    nButtonSelected = POPUP_ESCAPE;
}

//=============================================================================
// Sets which button(s) should be displayed
//=============================================================================
void
LrdPopupMessage::SetButtonVisibility(
    uint8_t nButtons
    )
{
    if (nButtons == POPUP_LEFT_BUTTON)
    {
        //Show only left button
        ui->btn_Right_Button->setEnabled(false);
        ui->btn_Right_Button->setVisible(false);
    }
    else if (nButtons == POPUP_BOTH_BUTTONS)
    {
        //Show both buttons
        ui->btn_Right_Button->setEnabled(true);
        ui->btn_Right_Button->setVisible(true);
    }
}

//=============================================================================
// Sets the text of one of the buttons
//=============================================================================
void
LrdPopupMessage::SetButtonText(
    uint8_t nButton,
    QString strText
    )
{
    if (nButton == POPUP_LEFT_BUTTON)
    {
        //Update text on left button
        ui->btn_Left_Button->setText(strText);
    }
    else if (nButton == POPUP_RIGHT_BUTTON)
    {
        //Update text on right button
        ui->btn_Right_Button->setText(strText);
    }
}

//=============================================================================
// Sets the ID of the popup
//=============================================================================
void
LrdPopupMessage::SetPopupID(
    uint8_t nID
    )
{
    nPopupID = nID;
}

//=============================================================================
// Enables or disables pressing escape (Esc) to close the window
//=============================================================================
void
LrdPopupMessage::SetEscapeKeyHandling(
    bool AllowClose
    )
{
    bAllowCloseWithEscape = AllowClose;
}

//=============================================================================
// Sets which button should be the default
//=============================================================================
void
LrdPopupMessage::SetDefaultButton(
    uint8_t nButton
    )
{
    if (nButton == POPUP_LEFT_BUTTON)
    {
        ui->btn_Left_Button->setFocus();
    }
    else if (nButton == POPUP_RIGHT_BUTTON && ui->btn_Right_Button->isVisible() == true)
    {
        ui->btn_Right_Button->setFocus();
    }
}

//=============================================================================
// Over-ride for the reject action to control if the window can be closed
//=============================================================================
void
LrdPopupMessage::reject(
    )
{
    if (bAllowCloseWithEscape == true || nButtonSelected != POPUP_ESCAPE)
    {
        //Button was selected or not required, allow close
        QDialog::reject();
        emit Button_Clicked(nPopupID, nButtonSelected);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
