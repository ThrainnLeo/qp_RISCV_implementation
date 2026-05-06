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


// BSP... ---------------------------------------------------------------------------------------------------------------

// Error handler...
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    QF_INT_DISABLE();
    // Tänd B0 och B1 för att indikera krasch
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1); 
    for (;;);
}
// Error handler


// eclic_mtip_handler...
void eclic_mtip_handler(void) {

    uint32_t tick_step = TIMER_FREQ / BSP_TICKS_PER_SEC;
    uint64_t next_tick = *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIME);
    *(uint64_t volatile *)(TIMER_CTRL_ADDR + TIMER_MTIMECMP) = next_tick + tick_step;

    QTIMEEVT_TICK_X(0U, (void *)0);

    // Säkerhetscheck: Om AO_PumpMgr inte hunnit starta, avbryt här
    if (AO_PumpMgr == (QActive *)0 || AO_Medicine[0] == (QActive *)0) {
        return; 
    }

    // Läs av switchar (A8, A7, A6, A5)
    uint8_t nowA8 = gpio_input_bit_get(GPIOA, GPIO_PIN_8);
    uint8_t nowA7 = gpio_input_bit_get(GPIOA, GPIO_PIN_7);
    uint8_t nowA6 = gpio_input_bit_get(GPIOA, GPIO_PIN_6);
    uint8_t nowA5 = gpio_input_bit_get(GPIOA, GPIO_PIN_5);

    // Deklarerar minnesvariabler (tillstånds-variabler)
    static uint8_t lastA8, lastA7, lastA6, lastA5;

    // Logik som gäller alla switchar (A8 - A5):
    // Kantdetektering: Kontrollera om switchen har ändrat läge sedan förra kollen (10 ms sedan).
    // Detta förhindrar att vi skickar tusentals identiska händelser när switchen inte ändrar läge stilla.
    if (nowA8 != lastA8) {
        lastA8 = nowA8;
        // Skapa en ny händelse (Event). Om pinnen är hög (1U) skickas START, annars STOP.
        // Vi använder Q_NEW för att hämta minne från händelsepoolen.
        MedEvt *mev = Q_NEW(MedEvt, (nowA8 == 1U) ? START_MED_SIG : STOP_MED_SIG);
        // Identifiera att detta gäller den första medicinpumpen. (LED B0) som switch A8 styr.
        mev->medId = 0U;
        // Posta händelsen till PumpMgr:s kö för asynkron bearbetning.
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
    // Logik skillnad: vi skapar inte ett nytt MedEvt som vi gör för switch A8-A6.
    if (nowA5 != lastA5) {
        lastA5 = nowA5;
        QEvt *e = Q_NEW(QEvt, (nowA5 == 1U) ? ALARM_SIG : RESUME_SIG);
        QACTIVE_POST(AO_PumpMgr, e, 0U);
    }
}
// eclic_mtip_handler ^^


// BSP_init...
void BSP_init(void) {
    // Aktivera klockorna för GPIO-port A och B.
    // Måste slå på strömmen/klockan till varje modul innan den kan användas.
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    // Nollställ (släck) LED-pinnarna på port B (B0, B1, B2) innan de aktiveras.
    // Detta förhindrar att lamporna blinkar till okontrollerat vid start.
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    // Konfigurera port B (LEDs) som utgångar (Push-Pull).
    // MODE_OUT_PP gör att pinnarna kan driva ström till dioderna med 50MHz hastighet.
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    // Konfigurera port A (Switchar) som ingångar med internt Pull-Up motstånd (IPU).
    // IPU tvingar värdet till 1 (hög) när switchen är öppen. 
    // När switchen stängs dras pinnen till jord och värdet blir 0 (låg).
    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, 
              GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5);
}
// BSP_init ^^


// Lamp Logik...
void BSP_ledOn(uint8_t id) {
    // SET tänder nu lampan (3.3V)
    gpio_bit_set(GPIOB, (1U << id));
}

// .....................................................................

void BSP_ledOff(uint8_t id) {
    // RESET släcker nu lampan (0V)
    gpio_bit_reset(GPIOB, (1U << id));
}

// .....................................................................

void BSP_ledAllOff(void) {
    // Släck alla med reset
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
}

// .....................................................................

void BSP_ledToggle(uint8_t id) {
    // XOR fungerar fortfarande likadant för att byta tillstånd
    GPIO_OCTL(GPIOB) ^= (1U << id);   
}
// Lamp Logik ^^


// QF_onStartup...
void QF_onStartup(void) {
    // Aktivera avbrott (Interrupt) för systemtimern i ECLIC (Enhanced Core Local Interrupt Controller).
    // CLIC_INT_TMR: Identifierar att det är just timern som ska slå på.
    // 1 (andra parametern): Anger avbrottets prioritet (1 är lägsta).
    // 1 (tredje parametern): Anger avbrottets sub-prioritet.
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
// QF_onStartup ^^


// QV_onIdle...
//Idle-loop som anropas av QP när inga händelser finns att hantera
void QV_onIdle(void) {
    // så att timern kan väcka systemet igen.
    QF_INT_ENABLE(); 
}
// QV_onIdle ^^

// BSP ^^ ---------------------------------------------------------------------------------------------------------------