#ifndef SIMQ_TUI_D_AUTH
#define SIMQ_TUI_D_AUTH

#include <final/final.h>
#include "dialog.h"
#include "events.h"
#include "input.h"
#include "button.h"

namespace simq::tui {
    class DialogAuth final: public Dialog {
        private:
        Input *inputAuth = nullptr;
        Button *buttonExit = nullptr;
        FWidget *parent = nullptr;
        finalcut::FLabel *labelErr = nullptr;
        finalcut::FLabel *labelInput = nullptr;
        bool inputAuthIsEmpty = true;
        bool isClose = true;
        void emit( Events::Event ev );

        public:
        DialogAuth( finalcut::FWidget *parent ): Dialog( parent ) {
            this->parent = parent;
            inputAuth = new Input( simq::tui::Events::Entity::ENTITY_INPUT_AUTH, this );
            buttonExit = new Button( simq::tui::Events::Entity::ENTITY_BUTTON_EXIT, this );
            labelErr = new finalcut::FLabel( this );
            labelErr->setForegroundColor( finalcut::FColor::Red );
            labelInput = new finalcut::FLabel( this );
        }
        void initLayout() override;
        void onUserEvent( finalcut::FUserEvent *ev ) override;
        void onClose( finalcut::FCloseEvent *ev ) override;

    };
}

#endif

