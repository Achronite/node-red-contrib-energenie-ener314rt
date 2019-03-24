/* init_loop.h
 *
 */

#ifndef _INIT_H
#define _INIT_H

#include <pthread.h>

// Variable types
enum deviceTypes{DT_CONTROL = 1, DT_MONITOR, DT_DISCOVER};

// function prototypes
int init_ener314rt(void);
int lock_ener314rt(enum deviceTypes);
int unlock_ener314rt(void);
extern void close_ener314rt(void);

#endif

//globals
//extern pthread_mutex_t radio_mutex;

/***** END OF FILE *****/
