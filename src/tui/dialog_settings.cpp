#include "dialog_settings.h"

void simq::tui::DialogSettings::initLayout() {
    setText( "Manage settings" );
    setGeometry(finalcut::FPoint{10, 5}, finalcut::FSize{30, 9});

    finalcut::FDialog::initLayout();
}

void simq::tui::DialogSettings::onClose( finalcut::FCloseEvent *_ ) {
    hide();
    Events::emit( parent, entity, Events::Event::EVENT_HIDE_DIALOG );
}
