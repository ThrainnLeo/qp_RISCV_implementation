#ifndef APP_H_
#define APP_H_

//$declare${App} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${App::AppSignals} .........................................................
enum AppSignals {
    TIMEOUT_SIG = Q_USER_SIG,
    MAX_SIG // the last signal
};

//${App::AO_Blinky} ..........................................................
extern QActive * const AO_Blinky;

//${App::Blinky_ctor} ........................................................
void Blinky_ctor(void);
//$enddecl${App} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif // APP_H_