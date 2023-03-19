#ifndef SIMQ_TUI_D
#define SIMQ_TUI_D

#include <final/final.h>
#include "button.h"

namespace simq::tui {
    class Dialog: public finalcut::FDialog {
        private:
            bool isAjustSize = true;
            void resetAjustSize();
        public:
            Dialog( finalcut::FWidget *parent ): finalcut::FDialog( parent ) {
                addCallback( "mouse-move", this, &simq::tui::Dialog::resetAjustSize );
                setTitlebarButtonVisibility( false );
                setShadow();
            }
            void adjustSize() override;
    };
}

#endif
