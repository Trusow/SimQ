#ifndef SIMQ_TUI_INPUT
#define SIMQ_TUI_INPUT

#include <final/final.h>
#include "events.h"

namespace simq::tui {
    class Input final: public finalcut::FLineEdit {
        private:
        Events::Entity entity;
        FWidget *parent = nullptr;
        
        public:
        Input( Events::Entity entity, finalcut::FWidget *parent ): finalcut::FLineEdit( parent ) {
            this->entity = entity;
            this->parent = parent;
        }

        void onKeyPress( finalcut::FKeyEvent *ev ) override;
    };
}

#endif
