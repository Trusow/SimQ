#ifndef SIMQ_TUI_BUTTON
#define SIMQ_TUI_BUTTON

#include <final/final.h>
#include "events.h"

namespace simq::tui {
    class Button final: public finalcut::FButton {
        private:
        Events::Entity entity;
        FWidget *parent = nullptr;

        void emit( Events::Event event );

        public:
        Button( Events::Entity entity, finalcut::FWidget *parent ): finalcut::FButton( parent ) {
            this->entity = entity;
            this->parent = parent;
        }

        void onMouseUp( finalcut::FMouseEvent *ev ) override;
        void onKeyUp( finalcut::FKeyEvent *ev ) override;
    };
}

#endif
