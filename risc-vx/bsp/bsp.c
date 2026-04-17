// RISC-V --> Hårdvarokod: adresser, subrutiner osv
//---------------------------------------------------
#include "gd32vf103.h"      //Allmänna funktioner
#include "gd32vf103_rcu.h"  // Klockhantering 
#include "gd32vf103_gpio.h" // Pinnstyrning 
//---------------------------------------------------

// QP filer
//---------------------------------------------------
#include "qpc.h"
#include "bsp.h"
#include "qp_port.h"
#include "app.h"
//---------------------------------------------------

#include <stdio.h>  // for printf()/fprintf()
#include <stdlib.h> // for exit() --> får se om vi behöver ha kvar den 

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

void SysTick_Handler(void){ // --> Vet inte om det här är rätt 
    QTIMEEVT_TICK_X(0U, &l_SysTick_Handler); //time events at rate 0
    //QF_TICK_X(0U, (void *)0); //Om inte den övre funkar, ta den 
}

//============================================================================
// BSP...

void BSP_init(void const * const arg) {
    Q_UNUSED_PAR(arg);

    //Aktivera klockan för GPIO Port B
    rcu_periph_clock_enable(RCU_GPIOB);

    //Konfigurera PB0, PB1 och PB2 som Push-Pull utgångar (50MHz hastighet)
    gpio_init(  GPIOB, GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_50MHZ, GPIO_PIN_0 | 
                GPIO_PIN_1 | GPIO_PIN_2);

    //Släck alla LEDs direkt vid start
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    // Konfigurera ECLIC för system-timern --> Kolla om detta behövs (kolla vad detta innebär)
    eclic_irq_enable(CLIC_INT_TMR, 0, 0);

    //--------------------------------Den ger fel i termilalen----------------------------------------------------------
    // Ställ in mtimecmp för att ge ett avbrott (t.ex. varje 10ms) --> Kolla om detta behövs (kolla vad detta innebär)
    //uint64_t next_tick = get_timer_value() + (SystemCoreClock / 100); 
    //set_timer_value(next_tick);

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
void BSP_ledOn(void) {
// Fysisk styrning av PB0
    gpio_bit_set(GPIOB, GPIO_PIN_0);    // application-specific record

    QS_BEGIN_ID(LED_STAT, AO_Blinky->prio)
        QS_STR("ON"); // LED status
    QS_END()
}
//............................................................................
void BSP_ledOff(void) {
// Fysisk styrning av PB0
    gpio_bit_set(GPIOB, GPIO_PIN_0);    // application-specific record

    QS_BEGIN_ID(LED_STAT, AO_Blinky->prio)
        QS_STR("OFF"); // LED status
    QS_END()
}

 void MTIMER_IRQHandler(void) {
    // Skriv till mtimecmp för att sätta nästa avbrottstid (viktigt!)
    // För GD32VF103 (Bumblebee core) */
    
    // Läs nuvarande mtime och sätt mtimecmp till nuvarande + intervall
    uint64_t mtime = *(uint64_t volatile *)(0xD1000000 + 0xBFF8); // Adress för mtime
    *(uint64_t volatile *)(0xD1000000 + 0x4000) = mtime + BSP_TICKS_PER_SEC ; // Adress för mtimecmp

    //Meddela QP att ett tick har gått */
    //Detta anropar den där generiska koden i qf_time.c som du visade tidigare
    QTIMEEVT_TICK_X(0U, (void *)0); 
 }

//============================================================================
// QF...

//Den anropas av ramverket precis innan det börjar köra händelse-loopen. 
//På en RISC-V är det här du faktiskt "låser upp dörren" för avbrott.
void QF_onStartup(void) {
    //Konfigurera Machine Timer (mtime/mtimecmp) här om du inte gjort det i BSP_init
    //Ställ in det ALLRA FÖRSTA alarmet
    uint64_t mtime = *(uint64_t volatile *)(0xD1000000 + 0xBFF8);
    *(uint64_t volatile *)(0xD1000000 + 0x4000) = mtime + BSP_TICKS_PER_SEC;
    
    //Aktivera Machine Timer Interrupt i mie-registret (bit 7)
    __asm__ volatile ("csrs mie, %0" :: "r"(0x80)); 

    // Aktivera globala avbrott (MIE-biten i mstatus)
    QF_INT_ENABLE();
}
//............................................................................
void QF_onCleanup(void) {
}
//............................................................................

void QV_onIdle(void){
    // Eventuell loggning för Q-Spy kan ske här

    // Sätt RISC-V i "Wait For Interrupt"-läge
    // Detta gör att CPU:n pausar tills nästa tick eller knapptryck
    __asm__ volatile ("wfi");
}
//............................................................................
void QF_onClockTick(void) {

    QS_RX_INPUT(); // handle the QS-RX input
    QS_OUTPUT();   // handle the QS output
}

//============================================================================
// QS callbacks...
#ifdef Q_SPY

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