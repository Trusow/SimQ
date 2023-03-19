#include <final/final.h> 
#include "src/tui/main.h"

int main() {
    finalcut::FApplication app( 0, nullptr );
    simq::tui::Main main( &app );

    finalcut::FWidget::setMainWidget(&main);
    main.show();
    return app.exec();
}
