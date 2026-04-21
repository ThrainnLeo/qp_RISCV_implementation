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

    //Skicka tick till QP-ramverket --> mycket viktigt för händelsekön 
    QTIMEEVT_TICK_X(0U, &l_clock_tick);
}


//============================================================================
// BSP...

void BSP_init(void const * const arg) {
    Q_UNUSED_PAR(arg);

    //Aktivera klockan för GPIO Port B
    rcu_periph_clock_enable(RCU_GPIOB);

    //Konfigurera PB0, PB1 och PB2 som Push-Pull utgångar (50MHz hastighet)
    gpio_init(  GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
                GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    // initialize QS software tracing...
    if (!QS_INIT(arg)) {
        Q_ERROR();
    }

    // QS dictionaries...
    QS_OBJ_DICTIONARY(&l_clock_tick); // --> Kan också kunna stå l_SysTick_Handler i parantesen 
    QS_SIG_DICTIONARY(TIMEOUT_SIG, (void *)0);
    QS_USR_DICTIONARY(LED_STAT);

    // setup the QS filters...
    QS_GLB_FILTER(QS_GRP_ALL);  // enable all records
    QS_GLB_FILTER(-QS_QF_TICK); // exclude the tick record

    // mutable events not used -- no need to call QF_poolInit()
    // publish-subscribe not used -- no need to call QActive_psInit()

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


void QF_onStartup(void) {
    // Lås upp ECLIC (Räknaren i kärnan på CPU:n) för Timer (7) från gd32vf103_eclic.c
    eclic_irq_enable(CLIC_INT_TMR, 1, 1);
 
    // Sätt första larmet
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t start_time = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = start_time + tick_step;

    QF_INT_ENABLE(); // Aktivera QP:s interna avbrottshantering
}

void BSP_ledOn(void) {
    //Kan vara på reset beror på om det är low-level 
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 |GPIO_PIN_2);    // application-specific record

    QS_BEGIN_ID(LED_STAT, AO_Blinky->prio)
        QS_STR("ON"); // LED status
    QS_END()
}
//............................................................................
void BSP_ledOff(void) {
    //Kan vara på set beror på om det är low-level 
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);    // application-specific record

    QS_BEGIN_ID(LED_STAT, AO_Blinky->prio)
        QS_STR("OFF"); // LED status
    QS_END()
}
//============================================================================
// QF...

//............................................................................
void QF_onCleanup(void) {
}

//............................................................................
void QV_onIdle(void){
    // Eventuell loggning för Q-Spy kan ske här

    QF_INT_ENABLE(); //Detta är väldigt viktigt att ha med 

    // Sätt RISC-V i "Wait For Interrupt"-läge
    //__asm__ volatile ("wfi");
    //Kanske inte behövs nu men Detta gör att CPU:n pausar tills nästa tick eller knapptryck
}
//............................................................................

//============================================================================
// QS callbacks...
#ifdef Q_SPY

uint8_t QS_onStartup(void const *arg){
    //Massor av kod som implementerar UART 

    return 1U;

}

void QS_onCleanup(void) {
}

QSTimeCtr QS_onGetTime(void) { // NOTE: invoked with interrupts DISABLED
    return (QSTimeCtr)(*(uint32_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME));
}

void QS_onFlush(void){
    /*
    uint8_t const *b;
    // Hämta ett byte i taget från QS-bufferten och skicka via UART
    while ((b = QS_getByte()) != (uint8_t const *)0) {
        // Vänta tills sändningsregistret är tomt
        while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
        // Skicka byten
        usart_data_transmit(USART0, *b);

    }
    */
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