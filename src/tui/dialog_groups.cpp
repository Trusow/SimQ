#include "dialog_groups.h"

void simq::tui::DialogGroups::initLayout() {
    setText( "Manage groups" );
    setGeometry(finalcut::FPoint{10, 5}, finalcut::FSize{30, 9});

    button->setGeometry(finalcut::FPoint{1, 1}, finalcut::FSize{10, 1});
    button->setText( "test" );

    finalcut::FDialog::initLayout();
}

void simq::tui::DialogGroups::onClose( finalcut::FCloseEvent *_ ) {
    hide();
    Events::emit( parent, entity, Events::Event::EVENT_HIDE_DIALOG );
}

void simq::tui::DialogGroups::onUserEvent( finalcut::FUserEvent *ev ) {
    Events::CallbackEvent event = ev->getData<Events::CallbackEvent>();

    if( event.entity == Events::Entity::ENTITY_BUTTON_GROUPS_TEST ) {
        switch( event.event ) {
            case Events::Event::EVENT_CLICK_BUTTON:
                dialogTest->show();
                unsetFocus();
                dialogTest->setFocus();
                //dialogTest->setModal();
                break;
        }
    }

}
