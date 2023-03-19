#include "main.h"

void simq::tui::Main::initLayout() {
    dialogAuth = new DialogAuth( this );
    dialogGroups = new DialogGroups( this );
    dialogSettings = new DialogSettings( this );
    dialogGroups->hide();
    dialogGroups->setShadow();
    dialogSettings->hide();
    dialogSettings->setShadow();
    
    hide();
    menuBar.hide();
    FDialog::initLayout();

}

void simq::tui::Main::onUserEvent( finalcut::FUserEvent *ev ) {
    Events::CallbackEvent event = ev->getData<Events::CallbackEvent>();

    if( event.entity == Events::Entity::ENTITY_D_AUTH ) {
        switch( event.event ) {
            case Events::Event::EVENT_CLOSE:
                isAuth = true;
                close();
                break;
            case Events::Event::EVENT_AUTH:
                isAuth = true;
                menuBar.show();
                toggleGroups();

                menuQuit.addCallback( "clicked", this, &simq::tui::Main::close );
                menuGroups.addCallback( "clicked", this, &simq::tui::Main::toggleGroups );
                menuSettings.addCallback( "clicked", this, &simq::tui::Main::toggleSettings );
                break;
        }
    } else if( event.entity == Events::Entity::ENTITY_D_GROUPS ) {
        switch( event.event ) {
            case Events::Event::EVENT_HIDE_DIALOG:
                menuGroups.setEnable();
                menuBar.setFocus();
                menuBar.redraw();
                break;

        }
    } else if( event.entity == Events::Entity::ENTITY_D_SETTINGS ) {
        switch( event.event ) {
            case Events::Event::EVENT_HIDE_DIALOG:
                menuSettings.setEnable();
                menuBar.setFocus();
                menuBar.redraw();
                break;
        }
    }
}

void simq::tui::Main::onClose( finalcut::FCloseEvent *ev ) {
    if( !isAuth ) {
        return;
    }

    FDialog::onClose( ev );
}

void simq::tui::Main::toggleGroups() {
    menuGroups.setDisable();
    menuSettings.setEnable();
    menuBar.unsetFocus();
    menuBar.redraw();
    dialogGroups->show();
    dialogSettings->hide();
    dialogGroups->setFocus();
}

void simq::tui::Main::toggleSettings() {
    menuSettings.setDisable();
    menuGroups.setEnable();
    menuBar.unsetFocus();
    menuBar.redraw();
    dialogGroups->hide();
    dialogSettings->show();
    dialogSettings->setFocus();
}
