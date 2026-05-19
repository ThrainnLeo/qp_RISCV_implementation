#include "qpc.h"
#include "bsp.h"
#include "app.h"

//============================================================================
Q_DEFINE_THIS_FILE  // Filnamn för QP-assertioner
//============================================================================

/* ===========================================================================
 * Globalt minne och köer för QP-ramverket
 * 
 * - l_pumpMgrQueueSto: Kö-minne för PumpManager (chefen). Rymmer upp till 10 
 *   asynkrona händelser samtidigt.
 * - l_medQueueSto: En 2D-array som tilldelar 10 köplatser var till de tre 
 *   individuella medicinpumparna (arbetarna).
 * - l_evtPoolSto: Händelsepoolen (Event Pool) där dynamiska händelser (t.ex. 
 *   START_MED_SIG) skapas och raderas. Avsätter totalt 2000 bytes.
 * - l_subscrList: Systemets prenumerationslista för Publish-Subscribe-mönstret, 
 *   vilket används för att sprida globala signaler som ALARM och RESUME.
 * =========================================================================== */
static QEvt const *l_pumpMgrQueueSto[10];
static QEvt const *l_medQueueSto[N_MEDS][10];
static uint32_t l_evtPoolSto[2000 / sizeof(uint32_t)]; 
static QSubscrList l_subscrList[MAX_PUB_SIG];

/* ===========================================================================
 * main
 * 
 * Systemets startpunkt. Ansvarar för att initiera hårdvaran, konfigurera 
 * QP-ramverkets interna minnespooler, samt instansiera och starta alla 
 * Active Objects (trådar/tillståndsmaskiner) med unika prioriteter.
 * 
 * Arkitektur och prioritering:
 * - PumpMgr (Chefen) tilldelas prioritet 15 (högsta i detta system).
 * - Medicinpumparna (Arbetarna) tilldelas dynamiskt prioritet 1, 2 och 3 
 *   via en loop.
 * 
 * Funktionen avslutas med att lämna över hela kontrollen till QP-ramverkets
 * händelseloop via QF_run().
 * =========================================================================== */
int main(void) {
    BSP_init();      
    QF_init();

    QF_poolInit(l_evtPoolSto, sizeof(l_evtPoolSto), 64U);
    QF_psInit(l_subscrList, Q_DIM(l_subscrList));
    
    // 1. Starta PumpMgr (Chefen)
    PumpMgr_ctor();
    QACTIVE_START(AO_PumpMgr, 
                  15U,                       
                  l_pumpMgrQueueSto, Q_DIM(l_pumpMgrQueueSto),    
                  (void *)0, 0U, (QEvt *)0);

    // 2. Starta Medicinerna (Arbetarna)
    for (uint8_t i = 0U; i < N_MEDS; ++i) {
        Medicine_ctor(i);
        QACTIVE_START(AO_Medicine[i], 
                      (uint8_t)(i + 1),
                      l_medQueueSto[i],
                      Q_DIM(l_medQueueSto[i]),
                      (void *)0, 0U, (QEvt *)0);
    }
    return QF_run();
}