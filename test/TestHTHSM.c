#include "HTHSM.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

HTHSM_Fsm fsm;

HTHSM_STATE_DEF(0,stateA);
HTHSM_SUBSTATE_DEF(1,stateB, stateA);
HTHSM_SUBSTATE_DEF(2,stateC, stateA);
HTHSM_SUBSTATE_DEF(3,stateD, stateC);
HTHSM_STATE_DEF(4,stateE);

enum {
    SIG_1 = HTHSM_SIG_USER_START,
    SIG_2,
    SIG_3,
    SIG_4,
    SIG_5,
    SIG_6,     
};

const HTHSM_State * __lowestCommonSuperstate(const HTHSM_State* const state1, const HTHSM_State* const state2);

// Queue variables for storing a record of what state handlers are called on what events
typedef struct Entry Entry;

typedef struct{
  const HTHSM_State * state;
  HTHSM_Event event;
} HandledEvent;

struct Entry{
  HandledEvent handledEvent;
  TAILQ_ENTRY(Entry) links;
};


TAILQ_HEAD(Queue, Entry) queue;


void addToQueue(const HTHSM_State * const state, HTHSM_Event const * event) {
  Entry *elem;
  elem = malloc(sizeof(Entry));
  if (elem) {
    elem->handledEvent.state = state;
    memcpy(&(elem->handledEvent.event), event, sizeof(HTHSM_Event));
  }
  TAILQ_INSERT_TAIL(&queue, elem, links);
}

Entry removeFromQueue() {
	Entry elem;
  	Entry* pFirstElem;
  	pFirstElem = TAILQ_FIRST(&queue);
  	memcpy(&elem, (Entry*)pFirstElem, sizeof(Entry));
  	TAILQ_REMOVE(&queue, pFirstElem, links);
  	free(pFirstElem);
  	return elem;
}

void emptyQueue(){
	Entry *elem;
	TAILQ_FOREACH(elem, &queue, links) {
		TAILQ_REMOVE(&queue, elem, links);
    }
}

uint8_t queueSize(){
	uint8_t count = 0;
	Entry *elem;
	TAILQ_FOREACH(elem, &queue, links) {
		count++;
    }
    return count;
}

void printQueue(){
	Entry *elem;
	TAILQ_FOREACH(elem, &queue, links) {
		printf("State: %d\t Event Signal: %d\t Event Param: %d\n",elem->handledEvent.state->identifier, elem->handledEvent.event.sig, elem->handledEvent.event.param);
    }
}
		




HTHSM_return_t stateA_fxn(HTHSM_Fsm *fsm, HTHSM_Event const * pEvent){
	addToQueue(stateA, pEvent);
	return HTHSM_CONTINUE;
}

HTHSM_return_t stateB_fxn(HTHSM_Fsm *fsm, HTHSM_Event const * pEvent){
	addToQueue(stateB, pEvent);
	return HTHSM_CONTINUE;
}

HTHSM_return_t stateC_fxn(HTHSM_Fsm *fsm, HTHSM_Event const * pEvent){
	addToQueue(stateC, pEvent);
	return HTHSM_CONTINUE;
}

HTHSM_return_t stateD_fxn(HTHSM_Fsm *fsm, HTHSM_Event const * pEvent){
	addToQueue(stateD, pEvent);
	switch(pEvent->sig){
		case SIG_1:
			return HTHSM_CONTINUE;
		case SIG_2:
			return HTHSM_SUPPRESS_SUPERSTATES;
		case SIG_3:
			return HTHSM_SUPPRESS_IMMEDIATE_SUPERSTATE;
		case SIG_4:
			HTHSM_Transition(fsm, stateB);
			break;
		case SIG_5:
			HTHSM_Transition(fsm, stateE);
			break;
		case SIG_6:
			HTHSM_Transition(fsm, stateA);
			break;
		default:
			break;
	}
 	return HTHSM_CONTINUE;
}

HTHSM_return_t stateE_fxn(HTHSM_Fsm *fsm, HTHSM_Event const * pEvent){
	addToQueue(stateE, pEvent);
	return HTHSM_CONTINUE;
}


void assertEventsHandled(HandledEvent* pexpectation, size_t count){
	TEST_ASSERT_EQUAL(count, queueSize());
	Entry *elem;
	uint8_t i=0;
	TAILQ_FOREACH(elem, &queue, links) {
		TEST_ASSERT_EQUAL(pexpectation[i].state, elem->handledEvent.state);
		TEST_ASSERT_EQUAL(pexpectation[i].event.sig, elem->handledEvent.event.sig);
		i++;
    }
}

void setUp(void)
{
	TAILQ_INIT(&queue);
}

void tearDown(void)
{

}

void test_lowestCommonSuperstate(void){
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateA, stateA));

	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateB, stateC));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateC, stateB));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateB, stateD));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateD, stateB));

	TEST_ASSERT_EQUAL(NULL, __lowestCommonSuperstate(stateA, stateE));
	TEST_ASSERT_EQUAL(NULL, __lowestCommonSuperstate(stateB, stateE));
	TEST_ASSERT_EQUAL(NULL, __lowestCommonSuperstate(stateC, stateE));
	TEST_ASSERT_EQUAL(NULL, __lowestCommonSuperstate(stateD, stateE));

	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateA, stateC));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateA, stateD));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateC, stateA));
	TEST_ASSERT_EQUAL(stateA, __lowestCommonSuperstate(stateD, stateA));
}

void test_initialization(void){
	HandledEvent expectation[] = {
		{stateB, {HTHSM_SIG_INIT, 0}},
		{stateA, {HTHSM_SIG_ENTRY, 0}},
		{stateB, {HTHSM_SIG_ENTRY, 0}},
	};
	HTHSM_FsmCtor(&fsm, stateB, NULL);

	TEST_ASSERT_EQUAL(0, queueSize());
	HTHSM_FsmInit(&fsm);
	
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
	
}

void test_hierarchyWithDepth3(void)
{
	HTHSM_Event event = {SIG_1, 0};
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
		{stateC, {event.sig, 0}},
		{stateA, {event.sig, 0}},
	};
	
	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}


void test_hierarchyWithDepth3_suppressSuperstates(void)
{
	HTHSM_Event event = {SIG_2, 0};
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
	};

	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}

void test_hierarchyWithDepth3_suppressImmediateSuperstates(void)
{
	HTHSM_Event event = {SIG_3, 0};
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
		{stateA, {event.sig, 0}},
	};

	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}

void test_hierarchyWithDepth3_transition_toLSA(void)
{
	HTHSM_Event event = {SIG_6, 0};
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
		{stateC, {event.sig, 0}},
		{stateA, {event.sig, 0}},
		{stateD, {HTHSM_SIG_EXIT, 0}},
		{stateC, {HTHSM_SIG_EXIT, 0}},
	};

	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}

void test_hierarchyWithDepth3_transition_withLSA(void)
{
	HTHSM_Event event = {SIG_4, 0}; // targeting state B
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
		{stateC, {event.sig, 0}},
		{stateA, {event.sig, 0}},
		{stateD, {HTHSM_SIG_EXIT, 0}},
		{stateC, {HTHSM_SIG_EXIT, 0}},
		{stateB, {HTHSM_SIG_ENTRY, 0}},
	};

	
	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}

void test_hierarchyWithDepth3_transition_withoutLSA(void)
{
	HTHSM_Event event = {SIG_5, 0}; // targeting state B
	HandledEvent expectation[] = {
		{stateD, {event.sig, 0}},
		{stateC, {event.sig, 0}},
		{stateA, {event.sig, 0}},
		{stateD, {HTHSM_SIG_EXIT, 0}},
		{stateC, {HTHSM_SIG_EXIT, 0}},
		{stateA, {HTHSM_SIG_EXIT, 0}},
		{stateE, {HTHSM_SIG_ENTRY, 0}},
	};

	
	HTHSM_FsmCtor(&fsm, stateD, NULL);
	HTHSM_FsmInit(&fsm);
	emptyQueue();
	TEST_ASSERT_EQUAL(0, queueSize());

	HTHSM_Dispatch(&fsm, &event);
	assertEventsHandled(expectation, sizeof(expectation)/sizeof(expectation[0]));
}

void test_stateIsActiveMethod(void) {
    /*
    HTHSM_STATE_DEF(0,stateA);
    HTHSM_SUBSTATE_DEF(1,stateB, stateA);
    HTHSM_SUBSTATE_DEF(2,stateC, stateA);
    HTHSM_SUBSTATE_DEF(3,stateD, stateC);
    HTHSM_STATE_DEF(4,stateE);
     */
    HTHSM_Event event = {SIG_6, 0};

    HTHSM_FsmCtor(&fsm, stateD, NULL);
    HTHSM_FsmInit(&fsm);

    // Confirm that the state machine acknowledges it's inside stateD and each of its superstates
    TEST_ASSERT_TRUE(HTHSM_StateIsActive(&fsm, stateD));
    TEST_ASSERT_TRUE(HTHSM_StateIsActive(&fsm, stateC));
    TEST_ASSERT_TRUE(HTHSM_StateIsActive(&fsm, stateA));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateB));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateE));

    HTHSM_Dispatch(&fsm, &event); // Will cause a transition to StateA

    // Confirm that the state machine acknowledges it's inside stateA and none others
    TEST_ASSERT_TRUE(HTHSM_StateIsActive(&fsm, stateA));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateB));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateC));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateD));
    TEST_ASSERT_FALSE(HTHSM_StateIsActive(&fsm, stateE));
}