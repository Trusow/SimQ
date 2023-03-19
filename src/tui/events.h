#ifndef SIMQ_TUI_EVENTS
#define SIMQ_TUI_EVENTS

#include <final/final.h>

namespace simq::tui {
    class Events {
        public:
        enum Event {
            EVENT_CLEAR_TEXT,
            EVENT_INPUT_TEXT,
            EVENT_ENTER,
            EVENT_CLICK_BUTTON,
            EVENT_AUTH,
            EVENT_CLOSE,
            EVENT_HIDE_DIALOG,
        };

        enum Entity {
            ENTITY_BUTTON_AUTH,
            ENTITY_BUTTON_EXIT,
            ENTITY_INPUT_AUTH,
            ENTITY_BUTTON_GROUPS_TEST,
            ENTITY_BUTTON_GROUPS_TEST_EXIT,
            ENTITY_D_AUTH,
            ENTITY_D_GROUPS,
            ENTITY_D_SETTINGS,
        };

        struct CallbackEvent {
            Event event;
            Entity entity;
        };

        static void emit( finalcut::FWidget *widget, Entity entity, Event event ) {
            Events::CallbackEvent ev;
            ev.entity = entity;
            ev.event = event;

            finalcut::FUserEvent uev(finalcut::Event::User, 0);

            uev.setData(ev);
            finalcut::FApplication::sendEvent( widget, &uev );
        }
    };
}


#endif
