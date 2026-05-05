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

//============================================================================
// Error handler
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    QF_INT_DISABLE();
    // Tänd B0 och B1 för att indikera krasch
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1); 
    for (;;);
}

void eclic_mtip_handler(void) {
    // 1. Timer-hantering (behåll denna exakt som den är)
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t next_tick = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = next_tick + tick_step;

    QTIMEEVT_TICK_X(0U, (void *)0);

    // 3. Säkerhetscheck: Om AO_PumpMgr inte hunnit starta, avbryt här
    if (AO_PumpMgr == (QActive *)0 || AO_Medicine[0] == (QActive *)0) {
        return; 
    }

    // 4. Läs av switchar (A8, A7, A6, A5)
    uint8_t nowA8 = gpio_input_bit_get(GPIOA, GPIO_PIN_8);
    uint8_t nowA7 = gpio_input_bit_get(GPIOA, GPIO_PIN_7);
    uint8_t nowA6 = gpio_input_bit_get(GPIOA, GPIO_PIN_6);
    uint8_t nowA5 = gpio_input_bit_get(GPIOA, GPIO_PIN_5);

    static uint8_t lastA8, lastA7, lastA6, lastA5;

    if (nowA8 != lastA8) {
        lastA8 = nowA8;
        MedEvt *mev = Q_NEW(MedEvt, (nowA8 == 1U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 0U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Medicin 1 (Switch A7) ---
    if (nowA7 != lastA7) {
        lastA7 = nowA7;
        MedEvt *mev = Q_NEW(MedEvt, (nowA7 == 1U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 1U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Medicin 2 (Switch A6) ---
    if (nowA6 != lastA6) {
        lastA6 = nowA6;
        MedEvt *mev = Q_NEW(MedEvt, (nowA6 == 1U) ? START_MED_SIG : STOP_MED_SIG);
        mev->medId = 2U;
        QACTIVE_POST(AO_PumpMgr, (QEvt *)mev, 0U);
    }

    // --- Larm/Paus (Switch A5) ---
    if (nowA5 != lastA5) {
        lastA5 = nowA5;
        QEvt *e = Q_NEW(QEvt, (nowA5 == 1U) ? ALARM_SIG : RESUME_SIG);
        QACTIVE_POST(AO_PumpMgr, e, 0U);
    }
}

//============================================================================
void BSP_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
}

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
    // Använd den officiella konstanten för Timer Interrupt
    eclic_irq_enable(CLIC_INT_TMR, 1, 1); 
 
    // Beräkna steget ("TIMER_FREQ / BSP_TICKS_PER_SE" för 100 ticks/sek)
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC; 
    
    uint64_t volatile *mtime    = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);
    
    // Sätt första triggern lite längre fram i tiden för att ge CPU andrum
    *mtimecmp = *mtime + (tick_step * 2);

    // Aktivera QP:s interna avbrottshantering
    QF_INT_ENABLE();
}


/* Idle-loop som anropas av QP när inga händelser finns att hantera */
void QV_onIdle(void) {
    // så att timern kan väcka systemet igen.
    QF_INT_ENABLE(); 
}