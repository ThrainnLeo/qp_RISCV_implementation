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
    QF_INT_DISABLE();
    // Tänd B0 och B1 för att indikera krasch (nu med SET)
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1); 
    for (;;);
}

//Signaturen måste vara samma som den definerade funktionen i start.S
// Definiera frekvensen utanför funktionen, t.ex. högst upp i bsp.c
// För GD32VF103 tickar timern på en fjärdedel av CPU-klockan (108MHz / 4)
#define ACTUAL_TIMER_FREQ (108000000U / 4U) 

void eclic_mtip_handler(void) __attribute__((interrupt));

void eclic_mtip_handler(void) {
    // 1. Timer-hantering (behåll denna exakt som den är)
    uint32_t tick_step = 2700000U;
    uint64_t volatile *mtime    = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);

    *mtimecmp = *mtime + tick_step;

    //BSP_ledToggle(2); // <--- Lägg till denna rad för att SE att timern lever
    // 2. Meddela QP att tiden går (viktigt för blinkande LED!)
    //QF_TICK_X(0U, (void *)0); 
    QTIMEEVT_TICK_X(0U, (void *)0);

    // 3. Säkerhetscheck: Om AO_PumpMgr inte hunnit starta, avbryt här
    if (AO_PumpMgr == (QActive *)0) {
        return; 
    }

    // 4. Läs av knapparna (A8, A7, A6, A5)
    uint8_t nowA8 = gpio_input_bit_get(GPIOA, GPIO_PIN_8);
    uint8_t nowA7 = gpio_input_bit_get(GPIOA, GPIO_PIN_7);
    uint8_t nowA6 = gpio_input_bit_get(GPIOA, GPIO_PIN_6);
    uint8_t nowA5 = gpio_input_bit_get(GPIOA, GPIO_PIN_5);

    static uint8_t lastA8 = 1U, lastA7 = 1U, lastA6 = 1U, lastA5 = 1U;

    // --- Medicin 0 (Switch A8) ---
    if (nowA8 != lastA8) {
        lastA8 = nowA8;
        MedEvt *mev = Q_NEW(MedEvt, (nowA8 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 0U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Medicin 1 (Switch A7) ---
    if (nowA7 != lastA7) {
        lastA7 = nowA7;
        MedEvt *mev = Q_NEW(MedEvt, (nowA7 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 1U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Medicin 2 (Switch A6) ---
    if (nowA6 != lastA6) {
        lastA6 = nowA6;
        MedEvt *mev = Q_NEW(MedEvt, (nowA6 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 2U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Larm/Paus (Switch A5) ---
    if (nowA5 != lastA5) {
        lastA5 = nowA5;
        QEvt *e = Q_NEW(QEvt, (nowA5 == 0U) ? ALARM_SIG : RESUME_SIG);
        QACTIVE_POST(AO_PumpMgr, e, 0U);
    }
}
/*void mtimer_handler(void) {
    uint32_t tick_step = 270000U;
    uint64_t volatile *mtime    = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);

    *mtimecmp = *mtime + tick_step;

    // Meddela QP att ett tick har skett
    QF_TICK_X(0U, (void *)0); 
}
void eclic_mtip_handler(void) {
    uint32_t tick_step = ACTUAL_TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);
    *mtimecmp += tick_step;

    QF_TICK_X(0U, (void *)0); 

    // Säkerhetsspärr: Om chefen (PumpMgr) inte är startad än, gör ingenting mer
    if (AO_PumpMgr == (QActive *)0) {
        return; 
    }

    uint8_t nowA8 = gpio_input_bit_get(GPIOA, GPIO_PIN_8);
    uint8_t nowA7 = gpio_input_bit_get(GPIOA, GPIO_PIN_7);
    uint8_t nowA6 = gpio_input_bit_get(GPIOA, GPIO_PIN_6);
    uint8_t nowA5 = gpio_input_bit_get(GPIOA, GPIO_PIN_5);

    static uint8_t lastA8 = 1U, lastA7 = 1U, lastA6 = 1U, lastA5 = 1U;

    // --- Med 0 ---
    if (nowA8 != lastA8) {
        lastA8 = nowA8;
        MedEvt *mev = Q_NEW(MedEvt, (nowA8 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 0U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Med 1 ---
    if (nowA7 != lastA7) {
        lastA7 = nowA7;
        MedEvt *mev = Q_NEW(MedEvt, (nowA7 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 1U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Med 2 ---
    if (nowA6 != lastA6) {
        lastA6 = nowA6;
        MedEvt *mev = Q_NEW(MedEvt, (nowA6 == 0U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 2U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Larm ---
    if (nowA5 != lastA5) {
        lastA5 = nowA5;
        QEvt *e = Q_NEW(QEvt, (nowA5 == 0U) ? ALARM_SIG : RESUME_SIG);
        QACTIVE_POST(AO_PumpMgr, e, 0U);
    }
}*/

//============================================================================
// BSP...
//test verition av blinky med switch vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
void BSP_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    // SLÄCK genom att sätta bitarna INNAN de blir utgångar (vissa MCU kräver detta)
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
}
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void BSP_ledOn(uint8_t id) {
    // SET tänder nu lampan (3.3V)
    gpio_bit_set(GPIOB, (1U << id));
}

void BSP_ledOff(uint8_t id) {
    // RESET släcker nu lampan (0V)
    gpio_bit_reset(GPIOB, (1U << id));
}

void BSP_ledAllOff(void) {
    // Släck alla med reset
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
}

void BSP_ledToggle(uint8_t id) {
    // XOR fungerar fortfarande likadant för att byta tillstånd
    GPIO_OCTL(GPIOB) ^= (1U << id);
}
//............................................................................

void QF_onStartup(void) {
    // 1. Använd den officiella konstanten för Timer Interrupt
    // På GD32VF103 är detta ofta 7. Testa att skriva siffran 7 om namnet inte funkar.
    eclic_irq_enable(7, 1, 1); 
 
    // 2. Beräkna steget (270 000 för 100 ticks/sek)
    uint32_t tick_step = 270000U; 
    
    uint64_t volatile *mtime    = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);
    
    // Sätt första triggern lite längre fram i tiden för att ge CPU andrum
    *mtimecmp = *mtime + (tick_step * 2);

    // 3. Aktivera globala avbrott
    eclic_global_interrupt_enable();
}
/*void QF_onStartup(void) {
    // Aktiverar ECLIC (Räknaren i kärnan på CPU:n) för Timer (7) från gd32vf103_eclic.c
    eclic_irq_enable(CLIC_INT_TMR, 1, 1);
 
    // Sätt första larmet
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t start_time = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = start_time + tick_step;

    QF_INT_ENABLE(); // Aktivera QP:s interna avbrottshantering
}*/

/* Idle-loop som anropas av QP när inga händelser finns att hantera */
void QV_onIdle(void) {
    // så att timern kan väcka systemet igen.
    QF_INT_ENABLE(); 
}
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