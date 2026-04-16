#include "qpc.h"          // QP/C real-time event framework
#include "bsp.h"          // Board Support Package

//............................................................................
int main() {
    QF_init();            // initialize the framework and the RT kernel
    BSP_init((void *)0);  // initialize the BSP and instantiate/start the AOs
    return QF_run();      // run the framework
}