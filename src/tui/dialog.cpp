#include "dialog.h"

void simq::tui::Dialog::adjustSize() {
    if( !isAjustSize ) {
        //return;
    }

    setX( 1 + ( int( getDesktopWidth() ) - int( getWidth() ) ) / 2, false );
    setY( 1 + ( int( getDesktopHeight() ) - int( getHeight() ) ) / 2, false);
    finalcut::FDialog::adjustSize();
}

void simq::tui::Dialog::resetAjustSize() {
    isAjustSize = false;
}
