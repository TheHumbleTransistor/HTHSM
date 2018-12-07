//  Copyright © 2017 Ray Kampmeier
//  [Released under MIT License. Please refer to license.txt for details]

#include "HTHSM.h"

static HTHSM_Event const initEvt = { HTHSM_SIG_INIT, 0};
static HTHSM_Event const entryEvt = { HTHSM_SIG_ENTRY, 0};
static HTHSM_Event const exitEvt = { HTHSM_SIG_EXIT, 0};

void __Transition(HTHSM_Fsm *me, const HTHSM_State * const target);
void __Dispatch(HTHSM_Fsm *me, const HTHSM_Event * const e, const HTHSM_State * const superstateToStopBefore, bool ascendingOrder);


void HTHSM_FsmCtor(HTHSM_Fsm *me, const HTHSM_State * const initialState, HTHSM_State_fn const fnGenericEvtHandler){
    me->pState__ = (HTHSM_State*)initialState;
    me->pTransitionTarget = NULL;
    me->pLatestEvent = NULL;
    me->fnGenericEvtHandler = fnGenericEvtHandler;
}

void HTHSM_FsmInit(HTHSM_Fsm *me){
    __Dispatch(me, (HTHSM_Event*)&initEvt, me->pState__->superState, false); // Only dispatch in the target state. No superstates
    __Dispatch(me, (HTHSM_Event*)&entryEvt, NULL, true);
}

// Finds the "Lowest Common Ancestor" between two states
const HTHSM_State * __lowestCommonSuperstate(const HTHSM_State* const state1, const HTHSM_State* const state2){
    for(const HTHSM_State* pState1Superstate = state1; pState1Superstate != NULL; pState1Superstate = pState1Superstate->superState){
        for(const HTHSM_State* pState2Superstate = state2; pState2Superstate != NULL; pState2Superstate = pState2Superstate->superState){
            if(pState1Superstate == pState2Superstate){
                return pState1Superstate;
            }
        }
    }
    return NULL;
}

void __Transition(HTHSM_Fsm *me, const HTHSM_State * const target)
{
    const HTHSM_State* lowestCommonSuperstate = __lowestCommonSuperstate(me->pState__, (HTHSM_State *)target);
    /* exit the source and its superstates, stopping before reaching a superstate in common with the target */
    __Dispatch(me, (HTHSM_Event*)&exitEvt, lowestCommonSuperstate, false);
    me->pState__ = (HTHSM_State*)target;
    /* enter starting at the lowest common state and working down to the target state*/
    __Dispatch(me, (HTHSM_Event*)&entryEvt,lowestCommonSuperstate, true);
}

void HTHSM_Transition(HTHSM_Fsm *me, const HTHSM_State * const target)
{
    if(me->pLatestEvent
      && (me->pLatestEvent->sig != HTHSM_SIG_ENTRY)
      && (me->pLatestEvent->sig != HTHSM_SIG_EXIT)
    ){
      // Save the transition target.  The transition will be made upon completion of the active event handling.
      me->pTransitionTarget = (HTHSM_State*) target;
    }
}

void HTHSM_Dispatch(HTHSM_Fsm *me, HTHSM_Event *e){
  __Dispatch(me, e, NULL, false);
}

void __Dispatch(HTHSM_Fsm *me, const HTHSM_Event * const e, const HTHSM_State * const superstateToStopBefore, bool descending){

    static const HTHSM_State *stateList[HTHSM_MAX_HIERARCHY_DEPTH] = {NULL};
    int8_t stateListIndex = 0;

    me->pLatestEvent = e;

    if(me->fnGenericEvtHandler){
        (me->fnGenericEvtHandler)(me, e);
    }

    // We dispatch starting with the active state, and then move up through super states, until we've reached a common superstate
    for(const HTHSM_State * pState = me->pState__; ((pState != NULL) && (pState != superstateToStopBefore)); pState = pState->superState){
        if(descending){
            stateList[stateListIndex] = pState;
            stateListIndex++;
            continue;
        }
        uint8_t retVal = (pState->handlerFn)(me, e);
        if( retVal == HTHSM_SUPPRESS_SUPERSTATES){
            break;
        }else if( retVal == HTHSM_SUPPRESS_IMMEDIATE_SUPERSTATE){
            if(pState->superState != NULL){
                pState = pState->superState; // Skip over the next
            }
            continue;
        }
    }

    if(descending){
        stateListIndex--;
        for(; stateListIndex>=0; stateListIndex--){
            (stateList[stateListIndex]->handlerFn)(me, e);
        }
    }

    // Check if we should perform a transition
    // Because transitions are can only be executed outside of Entry and Exit events,
    // this HTHSM_Dispatch() function is restricted to be, at max, recursively executed only once by the __Transition() call
    if(me->pTransitionTarget != NULL){
        const HTHSM_State * pTransitionTarget = me->pTransitionTarget;
        me->pTransitionTarget = NULL;
        __Transition(me, pTransitionTarget);
    }
}
