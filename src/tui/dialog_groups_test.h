#ifndef SIMQ_TUI_D_GROUPS_TEST
#define SIMQ_TUI_D_GROUPS_TEST

#include <final/final.h>
#include "dialog.h"


namespace simq::tui {
    class DialogGroupsTest final: public Dialog {
        private:

        public:
        DialogGroupsTest( finalcut::FWidget *parent ): Dialog( parent ) {
            setText( "Test" );
        }
    };
}

#endif
