#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "lock_radio.h"
#include "openThings.h"
#include "../energenie/radio.h"
#include "../energenie/hrfm69.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to perform the receiving and locking for the Energenie ENER314-RT board
** It also provides the common functions for mutex locking, and radio initialisation
**
** Author: Phil Grainger - @Achronite, March 2019
*/

/* declare radio lock for multi-threading */
pthread_mutexattr_t attr;
pthread_mutex_t radio_mutex;

static bool initialised = false;

enum deviceTypes deviceType = DT_CONTROL; // types of devices in use to control loop behaviour

static struct RADIO_MSG RxMsgs[RX_MSGS];
static int pRxMsgHead = 0; // pointer to the next slot to load a Rx msg into in our Rx Msg FIFO
static int pRxMsgTail = 0; // pointer to the next msg to read from Rx Msg FIFO, if head=tail the buffer is empty

/* init_ener314rt() - initialise radio adaptor in a multi-threaded environment
**
** mutex locking is performed here AND RETAINED if lock=true
**
** @Achronite - March 2019
** 
*/
int init_ener314rt(int lock)
{
    int ret = -1;

    if (!initialised)
    {
        //initialise mutex
        printf("init_ener314(): initialising\n");

        // set mutex type to not deadlock if relocking the same mutex
        if ((ret = pthread_mutexattr_init(&attr)) == 0)
        {
            if ((ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)) == 0)
            {
                if ((ret = pthread_mutex_init(&radio_mutex, NULL)) != 0)
                {
                    // mutex failure
                    printf("init_ener314(): mutex_init failed, ret=%d\n", ret);
                    TRACE_OUTS("init_ener314(): mutex init failed err=");
                    TRACE_OUTN(ret);
                    TRACE_NL();

                    if (lock && (ret == EBUSY))
                    {
                        // another process has the lock; lets spin on the lock assuming that another thread has initialised the radio & mutex
                        printf("init_ener314(): initialised already, await lock\n");
                        return pthread_mutex_lock(&radio_mutex);
                    }
                }

                // mutex initialised; lock it
                if ((ret = pthread_mutex_lock(&radio_mutex)) == 0)
                {
                    // thread safe, set initialised
                    initialised = true;
                    printf("init_ener314(): mutex created & locked\n");
                    radio_init();
                    // place radio into standby mode for now, the nodes will change this later anyway
                    radio_standby();

                    if (!lock)
                    {
                        // unlock mutex if not required to be retained
                        ret = pthread_mutex_unlock(&radio_mutex);
                    }
                }
                printf("init_ener314(): initialised, ret=%d\n", ret);
            }
            else
            {
                printf("init_ener314(): settype failed ret=%d\n", ret);
            }
        }
        else
        {
            printf("init_ener314(): exattr_init failed ret=%d\n", ret);
        }
    }
    return ret;
}

/*
** lock_ener314rt() - lock the mutex to ensure we have exclusive access to radio adaptor in multithreaded environment
**
** Copes with conditions where we already have the lock
*/
int lock_ener314rt()
{
    int ret = -1;

    // Always check that the adaptor & mutex have been initialised, and if not do it
    if (!initialised)
    {
        printf("lock_ener314rt(): attempt to lock before lock initialised, initialising lock\n");
        // init and lock
        if ((ret = init_ener314rt(true)) != 0)
        {
            // init already done, not locked
            printf("lock_ener314rt() init failed %d, locking\n", ret);
            ret = pthread_mutex_lock(&radio_mutex);
            if (ret == EDEADLK)
            {
                printf("lock_ener314rt(): mutex already locked!\n");
                ret = 0;
            }
        };
    }
    else
    {
        // lock radio now
        printf("[lock(%d", (int)pthread_self());
        fflush(stdout);
        ret = pthread_mutex_lock(&radio_mutex);
        printf("-%d-", (int)pthread_self());
        fflush(stdout);

        // cater for if we already have the lock
        if (ret != 0)
        {
            if (ret == EDEADLK)
            {
                printf("ERROR lock_ener314rt(%d): mutex already locked!\n", ret);
                ret = 0;
            }
            else
            {
                printf("ERROR lock_ener314rt(%d): failed\n", ret);
            }
            //printf("lock_ener314rt(%d): mutex got\n",deviceMode);
        }
    }

    return ret;
}

int unlock_ener314rt(void)
{
    int ret = 0;
    //unlock mutex
    printf("%d)]", (int)pthread_self());
    fflush(stdout);
    ret = pthread_mutex_unlock(&radio_mutex);
    if (ret != 0)
    {
        printf("ERROR unlock_ener314rt(%d)\n", ret);
    }
    return ret;

    // TODO: Place radio back into the correct mode, or standby
}

/*
** elegant shutdown of radio adaptor
*/
void close_ener314rt(void)
{
    //Elegant shutdown of ener314rt
    printf("close_ener314(): called\n");
    if (lock_ener314rt(DT_MONITOR) == 0)
    {
        // we have the lock, do all the tidying
        radio_finished();
        initialised = false;
        deviceType = DT_CONTROL;    // set back to control mode only
        pthread_mutex_destroy(&radio_mutex);
        printf("close_ener314(): done\n");
        fflush(stdout);
    }
    else
    {
        printf("close_ener314(): couldnt get lock\n");
    }
}

/*
** empty_radio_Rx_buffer() - empties the radio receive buffer of any messages into RxMsgs as quickly as possible
**
** Need to mutex lock before calling
** Leave the radio in receive mode if monitoring, as this could have been the first time we have been called
**
** returns the # of messages read, -1 if error getting lock
**
** TODO: Buffer is currently cyclic and destructive, we could loose messages, but I've made the assumption we always need
**       the latest messages
**
*/
int empty_radio_Rx_buffer(enum deviceTypes rxMode)
{
    int i, recs = 0;

    // Put us into monitor as soon as we know about it
    if (rxMode == DT_MONITOR)
        deviceType = DT_MONITOR;


    if (deviceType == DT_MONITOR  || rxMode == DT_LEARN)
    {
        // only receive data if we are in monitor mode
        // Set FSK mode receive for OpenThings devices (Energenie OOK devices dont generally transmit!)
        radio_setmode(RADIO_MODULATION_FSK, HRF_MODE_RECEIVER);

        i = 0;
        // do we have any messages waiting?
        while (radio_is_receive_waiting() && (i++ < RX_MSGS))
        {
            if (radio_get_payload_cbp(RxMsgs[pRxMsgHead].msg, MAX_FIFO_BUFFER) == RADIO_RESULT_OK)
            {
                recs++;
                // TODO: Only store valid OpenThings messages?

                // record message timestamp
                RxMsgs[pRxMsgHead].t = time(0);

                // wrap round buffer for next Rx
                if (++pRxMsgHead == RX_MSGS)
                    pRxMsgHead = 0;
            }
            else
            {
                printf("empty_radio_Rx_buffer(): error getting payload\n");
                break;
            }
        }
    }
    //unlock_ener314rt();
    return recs;
};

/*
** pop_msg() - returns next unread message from Rx queue
**
** returns -1 if no messages, or # of msg remaining in FIFO
*/
int pop_RxMsg(struct RADIO_MSG *rxMsg)
{
    int ret = -1;

    printf("<%d-%d>", pRxMsgHead, pRxMsgTail);
    if (pRxMsgHead != pRxMsgTail)
    {
        memcpy(rxMsg->msg, RxMsgs[pRxMsgTail].msg, sizeof(rxMsg->msg));
        rxMsg->t = RxMsgs[pRxMsgTail].t;

        // move tail to next msg in buffer
        if (++pRxMsgTail == RX_MSGS)
            pRxMsgTail = 0;

        if (pRxMsgHead >= pRxMsgTail)
        {
            ret = pRxMsgHead - pRxMsgTail;
        }
        else
        {
            ret = pRxMsgHead + RX_MSGS - pRxMsgTail;
        }
    }

    printf("pop_RxMsg(%d): %d\n", ret, (int)rxMsg->t);

    return ret;
}