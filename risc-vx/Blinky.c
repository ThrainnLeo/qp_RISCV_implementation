/* 
Replace this file with your code. Put your source files in this directory and any libraries in the lib folder. 
If your main program should be assembly-language replace this file with main.S instead.

Libraries (other than vendor SDK and gcc libraries) must have .h-files in /lib/[library name]/include/ and
.c-files in /lib/[library name]/src/ to be included automatically.
*/
#include "qpc.h"    // QP/C real-time event framework
#include "bsp.h"    // Board Support Package
#include "app.h"    // Application

typedef struct Blinky {
// protected:
    QActive super;

// private:
    QTimeEvt timeEvt;

// public:
} Blinky;

extern Blinky Blinky_inst;

// protected:
static QState Blinky_initial(Blinky * const me, void const * const par);
static QState Blinky_off(Blinky * const me, QEvt const * const e);
static QState Blinky_on(Blinky * const me, QEvt const * const e);


void Blinky_ctor(void) {
    Blinky * const me = &Blinky_inst;
    QActive_ctor(&me->super, Q_STATE_CAST(&Blinky_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
}


//${AOs::Blinky} .............................................................
Blinky Blinky_inst;

//${AOs::Blinky::SM} .........................................................
static QState Blinky_initial(Blinky * const me, void const * const par) {
    //${AOs::Blinky::SM::initial}
    Q_UNUSED_PAR(par);

    QS_OBJ_DICTIONARY(&Blinky_inst);
    QS_OBJ_DICTIONARY(&Blinky_inst.timeEvt);

    QTimeEvt_armX(&me->timeEvt,
        BSP_TICKS_PER_SEC/2U, BSP_TICKS_PER_SEC/2U);

    QS_FUN_DICTIONARY(&Blinky_off);
    QS_FUN_DICTIONARY(&Blinky_on);

    return Q_TRAN(&Blinky_off);
}

//${AOs::Blinky::SM::off} ....................................................
static QState Blinky_off(Blinky * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        //${AOs::Blinky::SM::off}
        case Q_ENTRY_SIG: {
            BSP_ledOff();
            status_ = Q_HANDLED();
            break;
        }
        //${AOs::Blinky::SM::off::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = Q_TRAN(&Blinky_on);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

//${AOs::Blinky::SM::on} .....................................................
static QState Blinky_on(Blinky * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        //${AOs::Blinky::SM::on}
        case Q_ENTRY_SIG: {
            BSP_ledOn();
            status_ = Q_HANDLED();
            break;
        }
        //${AOs::Blinky::SM::on::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = Q_TRAN(&Blinky_off);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
//$enddef${AOs::Blinky} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


