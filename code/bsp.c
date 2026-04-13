#include "qpc.h"
#include "bsp.h"
#include "gpio.h"

// En enkel stub för tick-hantering
static uint_fast8_t l_tickStub; 

void BSP_init(void) {
    // 1. Konfigurera klockor för din RISC-V
    // 2. Initiera GPIO för t.ex. en LED
    // - Pinnen som vi ska använda ska vara av typen I/O (kanke pinne PA5 eller PC13)
}

void QF_onStartup(void) {
    // Konfigurera mtime-registret för 1ms intervall
    // Aktivera timer-avbrott i mstatus och mie
}

// Denna anropas vid varje timer-avbrott
void timer_isr(void) {
    QTIMEEVT_TICK_X(0U, &l_tickStub); // Informera QP om tick
}

void QV_onIdle(void) {
    __asm__ volatile ("wfi"); // Spara ström
}