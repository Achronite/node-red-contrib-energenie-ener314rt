#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
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

enum deviceTypes deviceType = DT_CONTROL;       // types of devices in use to control loop behaviour

/* init_ener314rt() - initialise radio adaptor in multi-threaded environment
**
** mutex locking is performed here AND RETAINED
**
** @Achronite - March 2019
** 
*/
int init_ener314rt(void){

    int ret = -1;

    if (!initialised){
        //initialise mutex
        printf("init_ener314(): initialising\n");
        if ((ret = pthread_mutex_init(&radio_mutex, NULL)) != 0){
            // if mutex already initialised, we need to quit as another thread has beat us to it
            printf("init_ener314(): mutex_init failed, ret=%d\n",ret);
            TRACE_OUTS("radio_init: mutex init failed err=");
            TRACE_OUTN(ret);
            TRACE_NL();
        } else {
            // mutex initialised; lock it
            if ((ret = pthread_mutex_lock(&radio_mutex)) == 0){
                // thread safe, set initialised
                initialised = true;
                printf("init_ener314(): mutex created & locked\n");
                radio_init();
                // place radio into standby mode, this may need to change to support FSK or receive mode
                radio_standby();
            }
            printf("init_ener314(): initialised, ret=%d\n",ret);
        }

    }
    return ret;
}

int lock_ener314rt(enum deviceTypes deviceMode){
    int ret = 0;

    // Always check that the adaptor & mutex have been initialised, and if not do it
    if (!initialised){
        // init and lock
        if ((ret = init_ener314rt()) != 0){
            // init already done, not locked
            printf("lock_ener314rt(%d) init failed %d, locking\n",deviceMode,ret);
            ret = pthread_mutex_lock(&radio_mutex);
        };
    } else {
        // check the mode we are in, before taking over the radio adaptor, we may need to flush the buffer before destroying it
        //printf("lock_ener314rt(%d)\n",deviceMode);
        if (deviceMode == DT_CONTROL){
            if ( radio_is_receive_waiting() ){
                /* wait a sec to allow for monitor action to complete */
                //printf("lock_ener314rt(%d): awaiting Rx\n",deviceMode);
                sleep(1);
                if ( radio_is_receive_waiting() ){
                    // We can't wait any longer
                    printf("lock_ener314rt(): message lost\n");
                }
            };
        }
        // lock radio now
        //printf("lock_ener314rt(%d): mutex called\n",deviceMode);
        ret = pthread_mutex_lock(&radio_mutex);
        //printf("lock_ener314rt(%d): mutex got\n",deviceMode);
    }

    return ret;

}

int unlock_ener314rt(void){
    //unlock mutex
    return pthread_mutex_unlock(&radio_mutex);
}

/*
** elegant shutdown of radio adaptor
*/
void close_ener314rt(void){
    //Elegant shutdown of ener314rt
    printf("close_ener314(): called\n");
    if (lock_ener314rt(DT_MONITOR) == 0){
        // we have the lock, do all the tidying
        radio_finished();
        initialised = false;
        pthread_mutex_destroy(&radio_mutex);
        printf("close_ener314(): done\n");
    }

}