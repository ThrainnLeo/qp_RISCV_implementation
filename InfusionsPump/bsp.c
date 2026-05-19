// RISC-V -------------------------------------------
#include "gd32vf103.h"
#include "gd32vf103_rcu.h"
#include "gd32vf103_gpio.h"
#include "n200_timer.h"
//---------------------------------------------------

// QP -----------------------------------------------
#include "qpc.h"
#include "bsp.h"
#include "qp_port.h"
#include "app.h"
//---------------------------------------------------

//============================================================================
Q_DEFINE_THIS_FILE  // file name for assertions
//============================================================================

/* ===========================================================================
 * Q_onError
 * 
 * Global felhanterare för QP-ramverket. Anropas automatiskt om en assertion 
 * (ett kritiskt programfel) fallerar. Funktionen stänger av alla avbrott, 
 * tänder LED B0 och B1 för att indikera en krasch, och går sedan in i en 
 * permanent loop för att frysa systemet.
 * =========================================================================== */
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    QF_INT_DISABLE();
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1); 
    for (;;);
}

/* ===========================================================================
 * eclic_mtip_handler
 * 
 * Systemets timer-avbrottshanterare som körs var 10:e millisekund (100 Hz).
 * 
 * Central funktionalitet:
 * 1. Beräknar och schemalägger nästa hårdvaruavbrott genom att addera ett 
 *    tidssteg (tick_step) till den interna 64-bitars klockan (MTIME) och 
 *    skriva till jämförelseregistret (MTIMECMP).
 * 2. Driver QP:s interna tidshändelser via QTIMEEVT_TICK_X().
 * 3. Läser av de fyra switcharna (A8-A5) och använder kantdetektering 
 *    (jämförelse med föregående tillstånd) för att upptäcka när en switch 
 *    ändrar läge. Detta förhindrar att identiska signaler skickas kontinuerligt.
 * 4. Allokerar och postar relevanta händelser (START/STOP för medicinpumpar, 
 *    samt ALARM/RESUME för larm) till Pump-managerns kö.
 * =========================================================================== */
void eclic_mtip_handler(void) {
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t next_tick = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = next_tick + tick_step;

    QTIMEEVT_TICK_X(0U, (void *)0);

    if (AO_PumpMgr == (QActive *)0 || AO_Medicine[0] == (QActive *)0) {
        return; 
    }

    uint8_t nowA8 = gpio_input_bit_get(GPIOA, GPIO_PIN_8);
    uint8_t nowA7 = gpio_input_bit_get(GPIOA, GPIO_PIN_7);
    uint8_t nowA6 = gpio_input_bit_get(GPIOA, GPIO_PIN_6);
    uint8_t nowA5 = gpio_input_bit_get(GPIOA, GPIO_PIN_5);

    static uint8_t lastA8, lastA7, lastA6, lastA5;

    // --- Medicin 0 (Switch A8) ---
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

    // --- Larm (Switch A5) ---
    if (nowA5 != lastA5) {
        lastA5 = nowA5;
        QEvt *e = Q_NEW(QEvt, (nowA5 == 1U) ? ALARM_SIG : RESUME_SIG);
        QACTIVE_POST(AO_PumpMgr, e, 0U);
    }
}

/* ===========================================================================
 * BSP_init
 * 
 * Initierar mikrokontrollerns hårdvaruperiferi. Aktiverar klockorna för 
 * GPIO-port A och B, nollställer samt konfigurerar LED-pinnarna (B0-B2) som 
 * digitala utgångar (Push-Pull), och konfigurerar switch-pinnarna (A8-A5) 
 * som ingångar med internt pull-up-motstånd (IPU).
 * =========================================================================== */
void BSP_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
}

/* ===========================================================================
 * LED- / Lampkontrollfunktioner
 * 
 * Abstraktionslager för att styra kretskortets lysdioder (port B).
 * - BSP_ledOn/Off: Tänder eller släcker en specifik LED via bit-shifting.
 * - BSP_ledAllOff: Släcker alla tre lysdioder samtidigt.
 * - BSP_ledToggle: Skiftar tillstånd på en LED genom att invertera biten (XOR).
 * =========================================================================== */
void BSP_ledOn(uint8_t id) {
    gpio_bit_set(GPIOB, (1U << id));
}

void BSP_ledOff(uint8_t id) {
    gpio_bit_reset(GPIOB, (1U << id));
}

void BSP_ledAllOff(void) {
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
}

void BSP_ledToggle(uint8_t id) {
    GPIO_OCTL(GPIOB) ^= (1U << id);   
}

/* ===========================================================================
 * QF_onStartup
 * 
 * Konfigurerar och startar hårdvarutimern precis innan QP-ramverkets 
 * händelseloop drar igång. Aktiverar timer-avbrottet i avbrottshanteraren 
 * (ECLIC) med lägsta prioritet, beräknar starttiden för det första avbrottet 
 * med ett litet extra andrum för CPU:n, och slår slutligen på globala avbrott.
 * =========================================================================== */
void QF_onStartup(void) {
    eclic_irq_enable(CLIC_INT_TMR, 1, 1); 
 
    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC; 
    
    uint64_t volatile *mtime    = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    uint64_t volatile *mtimecmp = (uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP);

    *mtimecmp = *mtime + (tick_step * 2);

    QF_INT_ENABLE();
}

/* ===========================================================================
 * QV_onIdle
 * 
 * QP-ramverkets idle-loop som körs automatiskt när det inte finns några 
 * händelser kvar att hantera i systemet. Ser till att globala avbrott är 
 * aktiverade så att systemtimern kan väcka processorn igen.
 * =========================================================================== */
void QV_onIdle(void) {
    QF_INT_ENABLE(); 
}