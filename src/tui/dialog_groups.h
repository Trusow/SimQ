#ifndef SIMQ_TUI_D_GROUPS
#define SIMQ_TUI_D_GROUPS

#include <final/final.h>
#include "dialog.h"
#include "button.h"
#include "events.h"
#include "dialog_groups_test.h"

namespace simq::tui {
    class DialogGroups final: public Dialog {
        private:
            Events::Entity entity = Events::Entity::ENTITY_D_GROUPS;
            Button *button = nullptr;
            FWidget *parent = nullptr;
            DialogGroupsTest *dialogTest = nullptr;
            
        public:
            DialogGroups( finalcut::FWidget *parent ): Dialog( parent ) {
                this->parent = parent;
                button = new Button( Events::ENTITY_BUTTON_GROUPS_TEST, this );
                dialogTest = new DialogGroupsTest( this );
                dialogTest->hide();
            };
            void initLayout() override;
            void onClose( finalcut::FCloseEvent *ev ) override;
            void onUserEvent( finalcut::FUserEvent *ev ) override;
    };
}

#endif
