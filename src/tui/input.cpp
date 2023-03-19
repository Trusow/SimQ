#include "input.h"

void simq::tui::Input::onKeyPress( finalcut::FKeyEvent *ev ) {
    FLineEdit::onKeyPress( ev );

    auto text = getText();
    Events::CallbackEvent event;
    event.entity = entity;

    if( isEnterKey( ev->key() ) ) {
        event.event = Events::Event::EVENT_ENTER;
    } else {
        event.event = text.isEmpty() ? Events::Event::EVENT_CLEAR_TEXT : Events::Event::EVENT_INPUT_TEXT;
    }

    finalcut::FUserEvent uev(finalcut::Event::User, 0);

    uev.setData(event);
    finalcut::FApplication::sendEvent( parent, &uev );
}
