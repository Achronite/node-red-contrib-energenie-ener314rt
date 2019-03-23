#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "init_loop.h"
#include "../energenie/radio.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to perform the receive loop for the Energenie ENER314-RT board
** It also provides the common functions for mutex locking, and radio initialisation
**
** Author: Phil Grainger - @Achronite, March 2019
*/

/* declare radio lock for multi-threading */
pthread_mutex_t radio_mutex;

static bool initialised = false;

enum deviceTypes{Control, Monitor, Both}deviceType = Control;       // types of devices in use to control loop behaviour

int init_ener314rt(void){

    int ret = 0;

    printf("init_ener314(): called\n");
    if (!initialised){
        //initialise mutex
        printf("init_ener314(): initialising\n");
        if ((ret = pthread_mutex_init(&radio_mutex, NULL)) != 0){
            // ignore errors, we could just be reinitialisng already inited var
            TRACE_OUTS("radio_init: mutex init failed err=");
            TRACE_OUTN(ret);
            TRACE_NL();
        } else {
            if ((ret = pthread_mutex_lock(&radio_mutex)) == 0){
                initialised = true;
                radio_init();
                // place radio into standby mode, this may need to change to support FSK or receive mode
                radio_standby();

                ret = pthread_mutex_unlock(&radio_mutex);
            }
        }

    }
    return ret;
}

int lock_ener314rt(void){
    // Always check that the adaptor & mutex have been initialised, and if not do it
    if (!initialised){
        init_ener314rt();
    }
    // mutex access radio adaptor
    return pthread_mutex_lock(&radio_mutex);
}

int unlock_ener314rt(void){
    //unlock mutex
    return pthread_mutex_unlock(&radio_mutex);
}