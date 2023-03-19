#ifndef SIMQ_TUI_MAIN
#define SIMQ_TUI_MAIN

#include <final/final.h>
#include "input.h"
#include "events.h"
#include "button.h"
#include "dialog_auth.h"
#include "dialog_groups.h"
#include "dialog_settings.h"

namespace simq::tui {
    class Main final: public finalcut::FDialog {
        private:
        bool inputAuthIsEmpty = true;
        DialogAuth *dialogAuth = nullptr;
        DialogGroups *dialogGroups = nullptr;
        DialogSettings *dialogSettings = nullptr;

        bool isAuth = false;

        finalcut::FMenuBar menuBar{this};
        finalcut::FMenuItem menuGroups{ "&Groups", &menuBar };
        finalcut::FMenuItem menuSettings{ "&Settings", &menuBar };
        finalcut::FMenuItem menuAbout{ "&About", &menuBar };
        finalcut::FMenuItem menuQuit{ "&Quit", &menuBar };
        void toggleGroups();
        void toggleSettings();

        public:
        Main( finalcut::FWidget *parent ): finalcut::FDialog( parent ) {
        }
        void initLayout() override;
        void onUserEvent( finalcut::FUserEvent *ev ) override;
        void onClose( finalcut::FCloseEvent *ev ) override;
    };
}

#endif
