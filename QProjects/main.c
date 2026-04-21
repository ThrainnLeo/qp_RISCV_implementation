#include "qpc.h"          // QP/C real-time event framework
#include "bsp.h"          // Board Support Package

//............................................................................
int main(int argc, char *argv[]) {
    QF_init();            // initialize the framework
    // initialize the BSP and instantiate/start the AOs
    BSP_init((argc > 1) ? argv[1] : (void *)0);
    return QF_run();      // run the framework
}