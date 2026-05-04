#include "qpc.h"
#include "bsp.h"
#include "app.h"

Q_DEFINE_THIS_FILE

// 1. Deklarera kö-minnet (viktigt för att få bort det röda strecket på rad 35)
static QEvt const *l_medicineQueueSto[N_MEDS][10];
static QEvt const *l_pumpMgrQueueSto[10];

// 2. Dina pool- och sub-listor (behåll de små storlekarna för säkerhets skull just nu)
static union {
    uint64_t align;
    uint32_t storage[1000]; 
} l_evtPool;

static QSubscrList l_subscrList[128];

int main(void) {
    BSP_init();      
    // Flytta BSP_ledAllOff till INSIDAN av BSP_init för att vara säker
    
    QF_init();
    QF_psInit(l_subscrList, 128U); 
    QF_poolInit(l_evtPool.storage, sizeof(l_evtPool.storage), 64U);
    
    PumpMgr_ctor();
        QACTIVE_START(AO_PumpMgr, 
                      15U,                       
                      l_pumpMgrQueueSto, 10U,    
                      (void *)0, 0U, (QEvt *)0);

        // 2. Starta Medicinerna
        for (uint8_t i = 0U; i < N_MEDS; ++i) {
            Medicine_ctor(i);
            QACTIVE_START(AO_Medicine[i], 
                      (uint8_t)(i + 1), 
                      l_medicineQueueSto[i], 10U, 
                      (void *)0, 0U, (QEvt *)0);
        }
    //while(1);
    return QF_run();
}