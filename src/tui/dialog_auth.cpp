#include "dialog_auth.h"

void simq::tui::DialogAuth::emit( Events::Event event ) {
    Events::emit( parent, Events::Entity::ENTITY_D_AUTH, event );
}

void simq::tui::DialogAuth::initLayout() {
    setGeometry(finalcut::FPoint{10, 5}, finalcut::FSize{30, 9});
    setText( "SimQ auth" );

    labelInput->setText( "Please enter password\nand press <Enter>" );
    labelInput->setGeometry(finalcut::FPoint{1, 1}, finalcut::FSize{27, 2});

    inputAuth->setInputType( finalcut::FLineEdit::InputType::Password );
    inputAuth->setGeometry(finalcut::FPoint{1, 3}, finalcut::FSize{27, 1});
    inputAuth->setFocus();

    buttonExit->setText( "Exit" );
    buttonExit->setGeometry(finalcut::FPoint{1, 5}, finalcut::FSize{8, 1});

    labelErr->setGeometry(finalcut::FPoint{11, 5}, finalcut::FSize{15, 1});
    labelErr->hide();

    FDialog::initLayout();
}

void simq::tui::DialogAuth::onClose( finalcut::FCloseEvent *ev ) {
    FDialog::onClose( ev );

    if( isClose ) {
        emit( Events::Event::EVENT_CLOSE );
    }
}

void simq::tui::DialogAuth::onUserEvent( finalcut::FUserEvent *ev ) {
    Events::CallbackEvent event = ev->getData<Events::CallbackEvent>();

    if( event.entity == Events::Entity::ENTITY_INPUT_AUTH ) {
        switch( event.event ) {
            case Events::Event::EVENT_CLEAR_TEXT:
            labelErr->hide();
            inputAuthIsEmpty = true;
            break;
            case Events::Event::EVENT_INPUT_TEXT:
            labelErr->hide();
            inputAuthIsEmpty = false;
            break;
            case Events::Event::EVENT_ENTER:
            if( !inputAuthIsEmpty ) {
                if( inputAuth->getText() == "simq" ) {
                    isClose = false;
                    close();
                    emit( Events::Event::EVENT_AUTH );
                } else {
                    inputAuth->setText( "" );
                    inputAuth->redraw();
                    inputAuthIsEmpty = true;
                    labelErr->setText( "Wrong password!" );
                    labelErr->show();
                    labelErr->redraw();
                }
            } else {
                labelErr->setText( "Empty password!" );
                labelErr->show();
                labelErr->redraw();
            }
            break;
        }
    } else if( event.entity == Events::Entity::ENTITY_BUTTON_EXIT ) {
        switch( event.event ) {
            case Events::Event::EVENT_CLICK_BUTTON:
            close();
            break;
        }
    }

}
