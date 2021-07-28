//  Copyright © 2017 Ray Kampmeier
//  [Released under MIT License. Please refer to license.txt for details]

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HTHSM_h
#define HTHSM_h

#include <stdint.h>
#include <stdbool.h>

#define HTHSM_CONCAT_2(p1, p2)      HTHSM_CONCAT_2_(p1, p2)
#define HTHSM_CONCAT_2_(p1, p2)     p1##p2

#ifndef HTHSM_MAX_HIERARCHY_DEPTH
#define HTHSM_MAX_HIERARCHY_DEPTH 5
#endif

#ifndef NULL
#define NULL 0
#endif

typedef enum
{
    HTHSM_CONTINUE = 0,
    HTHSM_SUPPRESS_SUPERSTATES,  // Bypass all superstates' event handling
    HTHSM_SUPPRESS_IMMEDIATE_SUPERSTATE  // Bypass only the immediate superstate's event handling
} HTHSM_return_t;

typedef struct HTHSM_Fsm HTHSM_Fsm;
typedef struct HTHSM_State HTHSM_State;
typedef struct HTHSM_Event HTHSM_Event;
typedef HTHSM_return_t (*HTHSM_State_fn)(HTHSM_Fsm *, HTHSM_Event const *);
typedef short HTHSM_Signal;

enum {
    HTHSM_SIG_INIT = 1,
    HTHSM_SIG_ENTRY,
    HTHSM_SIG_EXIT,
    HTHSM_SIG_USER_START        /*!< first signal for the user applications */
};

struct HTHSM_Event{
    HTHSM_Signal sig;
    uint32_t param;
};

struct HTHSM_State
{
    HTHSM_State_fn const handlerFn;
    const HTHSM_State * const superState;
    uint8_t identifier;       // For debugging
};

struct HTHSM_Fsm
{
   const HTHSM_State *pState__; /* the active state */
   const HTHSM_State *pTransitionTarget;      // State to transition to after dispatching current event
   const HTHSM_Event *pLatestEvent;           // Most recent event to be dispatched, or that's in the process of being dispatched
   HTHSM_State_fn fnGenericEvtHandler;  // A generic event handler function that's called on every event.  This can be used for debuggings or logging.
};


#define HTHSM_STATE_DEF(debug_id, varName) \
    static HTHSM_return_t HTHSM_CONCAT_2(varName,_fxn) (HTHSM_Fsm * me, HTHSM_Event const * pEvent); \
    static const HTHSM_State HTHSM_CONCAT_2(varName,_data) = { HTHSM_CONCAT_2(varName,_fxn), NULL, debug_id}; \
    static const HTHSM_State  * const varName = &HTHSM_CONCAT_2(varName,_data)

#define HTHSM_SUBSTATE_DEF(debug_id, varName, superstate) \
    static HTHSM_return_t HTHSM_CONCAT_2(varName,_fxn) (HTHSM_Fsm * me, HTHSM_Event const * pEvent); \
    static const HTHSM_State HTHSM_CONCAT_2(varName,_data) = { HTHSM_CONCAT_2(varName,_fxn), &HTHSM_CONCAT_2(superstate,_data), debug_id}; \
    static const HTHSM_State  * const varName = &HTHSM_CONCAT_2(varName,_data)


void HTHSM_FsmCtor(HTHSM_Fsm *me, const HTHSM_State * const initialState, HTHSM_State_fn const fnGenericEvtHandler);
void HTHSM_FsmInit(HTHSM_Fsm *me);
// Call HTHSM_Transition() to register a state transition to be performed AFTER the active state
// AND its superstates have completed dispatching the current event.
// Only call HTHSM_Transition() in the context of a HTHSM_State's event handler function.
// This operation will be ignored if called during an entry or exit event.
void HTHSM_Transition(HTHSM_Fsm *me, const HTHSM_State * const target);
// HTHSM_Dispatch() should be called from a single location within the application's scheduler (event queue). 
// Do not call HTHSM_Dispatch from within a state function
void HTHSM_Dispatch(HTHSM_Fsm *me, HTHSM_Event *e);


#endif /* HTHSM_h */

#ifdef __cplusplus
}
#endif
