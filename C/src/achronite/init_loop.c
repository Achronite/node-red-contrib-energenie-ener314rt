#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "init_loop.h"
#include "../energenie/radio.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to perform the receive loop for the Energenie ENER314-RT board
**
** Also provides the common functions for mutex locking, and radio initialisation
**
** Author: Phil Grainger - @Achronite, March 2019
*/

/* radio lock for multi-threading */
pthread_mutex_t radio_mutex;

static bool initialised = false;

int init_ener314rt(void){

    int ret = 0;

    if (!initialised){
        //initialise mutex
        printf("init_ener314()\n");

        if ((ret = pthread_mutex_init(&radio_mutex, NULL)) != 0){
            TRACE_OUTS("radio_init: mutex init failed\n");
            return ret;     
        };

        ret = pthread_mutex_lock(&radio_mutex);
        radio_init();
        // place radio into standby mode, this may need to change to support FSK or receive mode
        radio_standby();
        initialised = true;
    }
    return ret;
}

int lock_ener314rt(void){
    // Always check that the adaptor & mutex have been initialised, and if not do it
    if (!initialised){
        //this also locks
        return init_ener314rt();
    } else {
        // mutex access radio adaptor
        return pthread_mutex_lock(&radio_mutex);
    }
}

int unlock_ener314rt(void){
    //unlock mutex
    return pthread_mutex_unlock(&radio_mutex);
}