#include "qpc.h"
#include "bsp.h"
#include "manager.h"
#include "med_a.h"
#include "signals.h" // Behövs för att veta vad START_A_SIG är

/* 1. Skapa plats för händelsepoolen (behövs för Q_NEW) */
static QEvt l_evtPoolSto[20]; 

int main() {
    QF_init();  /* Initiera ramverket */
    BSP_init(); /* Initiera hårdvaran (utan argumentet 0) */

    /* 2. Initiera händelsepoolen så att Q_NEW fungerar i Manager */
    QF_poolInit(l_evtPoolSto, sizeof(l_evtPoolSto), sizeof(l_evtPoolSto[0]));

    /* 3. Anropa konstruktorerna */
    Manager_ctor(AO_Manager);
    MED_A_ctor(AO_MED_A);

    /* 4. Starta Manager (Prioritet 2) */
    static QEvt const *l_managerQueueSto[10];
    QActive_start((QActive *)AO_Manager, 
                  2U,                        /* Prioritet */
                  l_managerQueueSto, 10U,    /* Kö-storlek */
                  (void *)0, 0U,             /* Stack (behövs ej för QV-port) */
                  (void *)0);                /* Initierings-event */

    /* 5. Starta MED_A (Prioritet 1) */
    static QEvt const *l_medaQueueSto[10];
    QActive_start((QActive *)AO_MED_A, 
                  1U, 
                  l_medaQueueSto, 10U, 
                  (void *)0, 0U, 
                  (void *)0);

    return QF_run(); /* Starta exekveringen */
}