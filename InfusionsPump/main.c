#include "qpc.h"
#include "bsp.h"
#include "app.h"

Q_DEFINE_THIS_FILE

// Kö-minne: En för chefen, och en 2D-array för de tre medicinerna
// "[10]", Detta anger antalet händelser (events) som kan ligga i kön samtidigt för ett specifikt Active Object.
// Funktion: Om switchen skickar signaler snabbare än vad medicinen hinner hantera dem, lagras de i dessa 10 platser.
static QEvt const *l_pumpMgrQueueSto[10];
static QEvt const *l_medQueueSto[N_MEDS][10];

// Händelsepool (Event Pool)
// "2000": Detta är det totala antalet bytes i minnet som avsätts för att skapa nya händelser (som ALARM_SIG eller START_MED_SIG).
static uint32_t l_evtPoolSto[2000 / sizeof(uint32_t)]; 

// Prenumerationslista för ALARM och RESUME
static QSubscrList l_subscrList[MAX_PUB_SIG];

int main(void) {
    BSP_init();      
    QF_init();

    // Initiera poolen och prenumerationslistan
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
                      (uint8_t)(i + 1),        // Prioritet (1, 2, 3)
                      l_medQueueSto[i],        // Kö-minne för denna medicin
                      Q_DIM(l_medQueueSto[i]), // Storlek på kön
                      (void *)0, 0U, (QEvt *)0);
    }

    return QF_run(); // Starta ramverket
}