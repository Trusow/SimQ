#include "button.h"

void simq::tui::Button::emit( Events::Event event ) {
    Events::CallbackEvent ev;
    ev.entity = entity;
    ev.event = event;

    finalcut::FUserEvent uev(finalcut::Event::User, 0);

    uev.setData(ev);
    finalcut::FApplication::sendEvent( parent, &uev );
}

void simq::tui::Button::onMouseUp( finalcut::FMouseEvent *ev ) {
    FButton::onMouseUp( ev );

    emit( Events::Event::EVENT_CLICK_BUTTON );
}

void simq::tui::Button::onKeyUp( finalcut::FKeyEvent *ev ) {
    FButton::onKeyUp( ev );

    if( !isEnterKey( ev->key() ) ) {
        return;
    }

    emit( Events::Event::EVENT_CLICK_BUTTON );
}
