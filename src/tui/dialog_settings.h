#ifndef SIMQ_TUI_D_SETTINGS
#define SIMQ_TUI_D_SETTINGS

#include <final/final.h>
#include "dialog.h"
#include "events.h"

namespace simq::tui {
    class DialogSettings final: public Dialog {
        private:
            Events::Entity entity = Events::Entity::ENTITY_D_SETTINGS;
            FWidget *parent = nullptr;
        public:
            DialogSettings( finalcut::FWidget *parent ): Dialog( parent ) {
                this->parent = parent;
            };
            void initLayout() override;
            void onClose( finalcut::FCloseEvent *ev ) override;
    };

}

#endif
