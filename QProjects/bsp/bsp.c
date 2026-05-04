// RISC-V --> Hårdvarokod: adresser, subrutiner osv
//---------------------------------------------------
#include "gd32vf103.h"      //Allmänna funktioner
#include "gd32vf103_rcu.h"  // Klockhantering 
#include "gd32vf103_gpio.h" // Pinnstyrning 
#include "n200_timer.h"
//---------------------------------------------------

// QP filer
//---------------------------------------------------
#include "qpc.h"
#include "bsp.h"
#include "qp_port.h"
#include "app.h"
//---------------------------------------------------

#include <stdio.h>  // for printf()/fprintf()
#include <stdlib.h> // for exit()

//============================================================================
Q_DEFINE_THIS_FILE  // file name for assertions

// local variables -----------------------------------------------------------
#ifdef Q_SPY
    enum AppRecords { // application-specific trace records
        LED_STAT = QS_USER,
    };

    // QSpy source IDs
    static QSpyId const l_clock_tick = { QS_ID_AP };
#endif // Q_SPY

//============================================================================
// Error handler
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    // NOTE: this implementation of the error handler is intended only
    // for debugging and MUST be changed for deployment of the application
    // (assuming that you ship your production code with assertions enabled).
    fprintf(stderr, "Assertion failed in %s:%d", module, id); 
    QS_ASSERTION(module, id, 10000U); // report assertion to QS
    exit(-1);
}

//Signaturen måste vara samma som den definerade funktionen i start.S
void eclic_mtip_handler(void) {

    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t next_tick = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = next_tick  + tick_step;

    //Skicka tick till QP-ramverket
    QTIMEEVT_TICK_X(0U, 0U); 
    //Började med 0U som sender och det funkar också men jag tror att om man ska koppla QSPY ska man ha det så enligt andra ex
}
//============================================================================
// BSP...

void BSP_init(void const * const arg) {
    //Aktivera klockan för GPIO Port B
    rcu_periph_clock_enable(RCU_GPIOB);

    //Konfigurera PB0, PB1 och PB2 som Push-Pull utgångar (50MHz hastighet)
    gpio_init(  GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
                GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);


    // instantiate and start AOs/threads...
    Blinky_ctor();             //Logiken för blinky exemplet 
    static QEvtPtr blinkyQueueSto[10];
    QActive_start(AO_Blinky,
        1U,                    // QP prio. of the AO
        blinkyQueueSto,        // event queue storage
        Q_DIM(blinkyQueueSto), // queue length [events]
        (void *)0, 0U,         // no stack storage
        (void *)0);            // no initialization param
}

//............................................................................
void BSP_ledOn(void) {
    //Kan vara på reset beror på om det är low-level 
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 |GPIO_PIN_2);    // application-specific record
}

//............................................................................
void BSP_ledOff(void) {
    //Kan vara på set beror på om det är low-level 
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);    // application-specific record
}
//============================================================================
// QF callbacks...
void QF_onCleanup(void) {
}

void QF_onStartup(void) {
    // Aktiverar ECLIC (Räknaren i kärnan på CPU:n) för Timer (7) från gd32vf103_eclic.c
    eclic_irq_enable(CLIC_INT_TMR, 1, 1);
 
    // Sätt första larmet
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t start_time = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = start_time + tick_step;

    QF_INT_ENABLE(); // Aktivera QP:s interna avbrottshantering
}
//............................................................................

//============================================================================
// QV callbacks...
void QV_onIdle(void){

    QF_INT_ENABLE(); //Detta är väldigt viktigt att ha med 

    // Sätt RISC-V i "Wait For Interrupt"-läge
    //__asm__ volatile ("wfi");
    //Kanske inte behövs nu men Detta gör att CPU:n pausar tills nästa tick eller knapptryck
}
//............................................................................

//============================================================================
// QS callbacks...
// Dessa callbacks används för att implementera QSpy (Spårningsverktyget)
#ifdef Q_SPY

uint8_t QS_onStartup(void const *arg){ 
}

void QS_onCleanup(void) {
}

QSTimeCtr QS_onGetTime(void) { // NOTE: invoked with interrupts DISABLED
}

void QS_onFlush(void){
    
}

void QS_onReset(void){

}

//............................................................................
void QS_onCommand(uint8_t cmdId,
    uint32_t param1, uint32_t param2, uint32_t param3)
{
    Q_UNUSED_PAR(cmdId);
    Q_UNUSED_PAR(param1);
    Q_UNUSED_PAR(param2);
    Q_UNUSED_PAR(param3);
}
#endif // Q_SPY




/*
// Läs nuvarande värde i väckarklockan (mtimecmp)
    //uint64_t current_time = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);
    //Flytta fram larmet exakt en tidsenhet (TICKRATE) genom att utgå från current_time istället för mtime undviker vi 
    //att tiden "driver" om avbrottet blir lite försenat.
    //*(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = current_time + (TIMER_FREQ/BSP_TICKS_PER_SEC);
*/


/*
//eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO1); //Sätt prioritetsgruppering för ECLIC (RISC-V specifik)
    //eclic_irq_enable(CLIC_INT_TMR, 1, 1); //Aktivera Timer-avbrottet i ECLIC med Level 1, Priority 1
    //Detta bestämmer hur ofta ramverket kollar på systemet (var 10ms --> 100 ggr i sek)
    //uint64_t mtime = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME); //Läs nuvarande tid från hårdvaruklockan (mtime)
    //*(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = mtime + (TIMER_FREQ/BSP_TICKS_PER_SEC); //Vi lägger till TICK_RATE till nuvarande tid
*/