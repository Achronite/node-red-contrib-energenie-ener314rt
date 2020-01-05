//#define NAPI_VERSION 3
#define NAPI_EXPERIMENTAL // needed for threadsafe functions (Dec 2019)
#include <node_api.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "lock_radio.h"
#include "openThings.h"
#include "ook_send.h"
#include "../energenie/trace.h"

/*
** napi_energenie.c: node.js Native API (N-API) wrapper for energenie C calls.
**
** The purpose of this file is to contain all the N-API specific calls for the rest of the C functions.  These N-API
** functions do all of the type conversions and function exposure so that the C calls can be used from node.js javascript
** using the standard 'require' statement, hiding all of the complexity from javascript calls.
**
** N-API replaces the older FFI / ref library calls used in v0.1 as these libraries are no longer being maintained.  N-API should allow
** this code to be Application Binary (ABI) compatiable across multiples platforms.
**
** Author: Phil Grainger - @Achronite, August 2019 - January 2020
**
** Conventions Used:
**  nv_   n-api value
**  nf_   n-api wrapper of synchronous function
**  af_   n-api wrapper of asynchronous functions
**  ccb_  complete callback function for async work (private)
**  xcb_  execute callback function for async work (private)
**
** threadsafe versions:
**  tf_  n-api wrapper for threadsafe asynchronous functions
**  tr_  return callback function that excutes the callback from a threadsafe function (private)
**  tx_  execute callback function for threadsafe async work (private)
**  tc_  complete callback function for threadsafe async work (private)
*/

// local prototypes (for async calls)
void xcb_openThings_receive(napi_env env, void *data);
void ccb_openThings_receive(napi_env env, napi_status status, void *data);

// async carrier for data passing
typedef struct carrier
{
    int32_t _input;
    char _buf[500];
    int _result;
    napi_value _callback;
    napi_async_work _request;
    napi_async_work _tsfn;
} carrier;

typedef struct
{
    napi_async_work work;
    napi_threadsafe_function tsfn;
    bool monitor;
    uint32_t timeout;
} AddonData;

// ----------FILE--------- lock_radio.c

/* N-API function (nf_) wrapper for:
** int init_ener314rt(int lock)
*/
napi_value nf_init_ener314rt(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 1; // 1 passed in arg
    napi_value argv[1];
    napi_value nv_ret;
    int ret;
    bool lock = false;

    // napi_get_cb_info is used to fetch our arguments as an array of N-API values
    // We’ll need to define the total number of arguments expected or the argument count (argc)
    // and will leave the last two arguments which are this and the data pointer as null
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to parse arguments");
    }

    // Ensure that the first argument 'lock' is a boolean
    napi_valuetype type_of_argument;
    status = napi_typeof(env, argv[0], &type_of_argument);
    if (status != napi_ok || type_of_argument != napi_boolean)
    {
        napi_throw_type_error(env, NULL, "Param lock not boolean");
    }
    else
    {
        /* Get lock value from js (boolean)
        We’ll pass the function the env/instance, the N-API value and a pointer to our defined bool to be populated with the results.
        */
        status = napi_get_value_bool(env, argv[0], &lock);

        /* We can validate our statuses against the enum napi_ok and throw a JS error using the napi_throw_error call which accepts env, 
        an optional error code which we will leave as null and a C string char* as the message.
        This way if anything goes wrong we will throw a meaningful error to JS-land.
        */
        if (status != napi_ok)
        {
            napi_throw_error(env, NULL, "Invalid number was passed as argument");
        }
    }

    //printf("calling init_ener314rt(%s) argc=%d,type=%d,lock=%d\n", lock ? "true" : "false", argc, type_of_argument, lock);

    // Call C routine
    ret = init_ener314rt(lock);

    //printf("init_ener314rt() returned %d\n", ret);

    // convert return value into JS value
    status = napi_create_int32(env, ret, &nv_ret);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

/* N-API function (nf_) wrapper for:
** extern void close_ener314rt(void);
*/
napi_value nf_close_ener314rt(napi_env env, napi_callback_info info)
{
    TRACE_OUTS("calling close_ener314rt()\n");

    // Call C routine to tidyup radio device
    close_ener314rt();

    return 0;
}

/* N-API function (nf_) wrapper for:
**  int send_radio_msg(RADIO_MODULATION mod, uint8_t *payload, uint8_t len, uint8_t times)
*/
napi_value nf_send_radio_msg(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 3; // 4 passed in args
    napi_value argv[3];
    napi_value nv_ret;
    int ret;
    napi_valuetype type_of_argument;
    unsigned int len, xmits, mod;
    unsigned char *msg;

    // get args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        // we cant recover from this error
        napi_throw_error(env, NULL, "Failed to parse arguments");
        ret = -10;
    }
    else
    {
        // Parse all the arguments
        // 0: RADIO_MODULATION mod
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "modulation not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[0], &mod);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid modulation");
        }

        // 1: uint8_t xmits
        status = napi_typeof(env, argv[1], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "xmits not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[1], &xmits);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid xmits");
        }

        // 2: uint8_t *payload and length
        status = napi_get_buffer_info(env, argv[2], (void **)(&msg), &len);
        if (status != napi_ok)
            napi_throw_error(env, NULL, "Invalid Message");

        // 2: uint8_t len
        /*
        status = napi_typeof(env, argv[2], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "Length not number");
        }
        else
        {
            status = napi_get_value_bool(env, argv[2], &len);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid length");
        }
        */

        // Call C routine
        ret = send_radio_msg(mod, msg, len, xmits);
    }

    // convert return value into JS value
    status = napi_create_int32(env, ret, &nv_ret);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

// ----------FILE--------- openThings.c
/*
 unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits);
 unsigned char openThings_deviceList(char *devices, bool scan);
 char openThings_receive(char *OTmsg );
*/

/* N-API function (nf_) wrapper for:
**  unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits); 
*/
napi_value nf_openThings_switch(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 4; // 4 passed in args
    napi_value argv[4];
    napi_value nv_ret;
    int ret;
    napi_valuetype type_of_argument;

    unsigned int iProductId, xmits;
    unsigned int iDeviceId;
    bool bSwitchState;

    // get args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        // we cant recover from this error
        napi_throw_error(env, NULL, "Failed to parse arguments");
        ret = -10;
    }
    else
    {
        // Parse all the arguments, first 3 need to be valid to proceed
        // 0: unsigned char iProductId
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "ProductId not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[0], &iProductId);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid ProductId");
        }

        // 1: unsigned int iDeviceId
        status = napi_typeof(env, argv[1], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "DeviceId not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[1], &iDeviceId);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid DeviceId");
        }

        // 2: unsigned char bSwitchState
        status = napi_typeof(env, argv[2], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_boolean)
        {
            napi_throw_type_error(env, NULL, "SwitchState not boolean");
        }
        else
        {
            status = napi_get_value_bool(env, argv[2], &bSwitchState);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid SwitchState");
        }

        // 3: unsigned char xmits
        status = napi_typeof(env, argv[3], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "xmits not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[3], &xmits);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid xmits");
        }

        //printf("calling openThings_switch(%d,%d,%d,%d)\n", iProductId, iDeviceId, bSwitchState, xmits);

        // Call C routine
        ret = openThings_switch(iProductId, iDeviceId, bSwitchState, xmits);

        //printf("openThings_switch() returned %d\n", ret);
    }

    // convert return value into JS value
    status = napi_create_int32(env, ret, &nv_ret);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

/* N-API function (nf_) wrapper for:
**  unsigned char openThings_deviceList(char *devices, bool scan);
** NOTE: devices is a returned buffer, but we will 'return' it instead
*/
napi_value nf_openThings_deviceList(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 1; // 2 passed in args
    napi_value argv[1];
    napi_value nv_ret;
    int ret;
    napi_valuetype type_of_argument;

    char buf[500];
    bool scan;

    // get args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to parse arguments");
        scan = false;
    }
    else
    {
        // 0: bool scan (was 1)
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_boolean)
        {
            napi_throw_type_error(env, NULL, "scan not boolean");
        }
        else
        {
            status = napi_get_value_bool(env, argv[0], &scan);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid scan");
        }
    }

    //printf("calling openThings_deviceList(buf,%d)\n", scan);

    // Call C routine
    ret = openThings_deviceList(buf, scan);

    if (ret > 0)
    {

        //printf("openThings_switch() returned %d. devices=%s\n", ret, buf);

        // convert return string into JS value
        status = napi_create_string_latin1(env, buf, NAPI_AUTO_LENGTH, &nv_ret);
    }
    else
    {
        status = napi_create_int32(env, ret, &nv_ret);
    }

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

/* N-API function (nf_) wrapper for:
**  char openThings_receive(char *OTmsg );
** NOTE: OTmsg is output param from C call, but we will 'return' it to node
*/
napi_value nf_openThings_receive(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value nv_ret;
    int ret;
    size_t argc = 1; // 1 passed in arg
    napi_value argv[1];
    uint32_t timeout = 0;
    const int buflen = 500;

    char buf[buflen];

    // Get the wait option from args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to parse arguments");
    }

    // Ensure that the first argument 'wait' is a number
    napi_valuetype type_of_argument;
    status = napi_typeof(env, argv[0], &type_of_argument);
    if (status != napi_ok || type_of_argument != napi_number)
    {
        napi_throw_type_error(env, NULL, "Param timeout is not an int32");
    }
    else
    {
        //* Get wait time value from js (int)
        status = napi_get_value_uint32(env, argv[0], &timeout);
    }
    //printf("calling openThings_receive()\n");

    // Call C routine
    ret = openThings_receive(buf, buflen, timeout);

    //printf("openThings_receive() returned %d. message=%s\n", ret, buf);

    if (ret > 0)
    {
        // convert return string into JS value only if values exist
        status = napi_create_string_latin1(env, buf, NAPI_AUTO_LENGTH, &nv_ret);
    }
    else
    {
        status = napi_create_int32(env, ret, &nv_ret);
    }

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

/* N-API function (nf_) wrapper for:
**  unsigned char openThings_cache_cmd(unsigned int iDeviceId, uint8_t command, uint32_t data)
**
** Input params from JS:
**  0: iDeviceId
**  1: command
**  2: data  (set to 0 if unused)
*/
napi_value nf_openThings_cache_cmd(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 3; // passed in args
    napi_value argv[3];
    napi_value nv_ret;
    int ret = -10;
    napi_valuetype type_of_argument;

    unsigned int iDeviceId;
    uint32_t command;
    uint32_t data;

    // get args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        // we cant recover from this error
        napi_throw_error(env, NULL, "Failed to parse arguments");
    }
    else
    {
        // Parse all the arguments, all need to be valid to proceed
        // 0: unsigned int iDeviceId
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "DeviceId not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[0], &iDeviceId);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid DeviceId");
        }

        // 1: uint8_t command
        status = napi_typeof(env, argv[1], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "command not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[1], &command);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid command");
        }

        // 2: uint32_t data                                                     TODO: Allow for optional data
        status = napi_typeof(env, argv[2], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "data not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[2], &data);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid data");
        }

        // Call C routine

        ret = openThings_cache_cmd(iDeviceId, command, data);
    }

    // convert return value into JS value
    status = napi_create_int32(env, ret, &nv_ret);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

/*
** N-API Continuous monitoring thread version - this uses a continously executing async worker thread and 
** utilises a threadsafe function to return the monitor messages back to javascript (node.js).
**
** This version was specifically designed to cope with the energenie thermostic radiator valves as they
** only have a small Rx window.
**
** @Achronite - December 2019
**
*/

// This is the threadsafe function called from the worker thread.  It is responsible for converting data Rx by the worker
// thread to napi_value items that can be passed into JavaScript, and for calling the JavaScript function
// passed in by the parent
static void tr_openThings_receive_thread(napi_env env, napi_value js_cb, void *context, void *data)
{
    // This parameter is not used.
    (void)context;
    //napi_status status;

    TRACE_OUTS("tr_openThings_receive_thread buf=");

    // Retrieve the buffer from the item created by the worker thread.
    //int the_prime = *(int*)data;
    char *buf = (char *)data;
    TRACE_OUTS(buf);

    // env and js_cb may both be NULL if Node.js is in its cleanup phase, and
    // items are left over from earlier thread-safe calls from the worker thread.
    // When env is NULL, we simply skip over the call into Javascript and free the
    // items.
    if (env != NULL)
    {
        napi_value undefined, js_buf;

        // Convert the integer to a napi_value.
        //assert(napi_create_int32(env, the_prime, &js_the_prime) == napi_ok);
        assert(napi_create_string_latin1(env, buf, NAPI_AUTO_LENGTH, &js_buf) == napi_ok);
        //status = napi_create_string_latin1(env, buf, NAPI_AUTO_LENGTH, &js_buf);
        //TRACE_OUTS("create_str=");
        //TRACE_OUTN(status);

        // Retrieve the JavaScript `undefined` value so we can use it as the `this`
        // value of the JavaScript function call.
        assert(napi_get_undefined(env, &undefined) == napi_ok);

        // Call the JavaScript function and pass it the prime that the secondary
        // thread found.
        assert(napi_call_function(env,
                                  undefined,
                                  js_cb,
                                  1,
                                  &js_buf,
                                  NULL) == napi_ok);
    }

    // Free the item created by the worker thread.
    free(data);
}

// N-API Internal function - primary execution thread, runs Rx commands in a loop whilst monitoring is active
// This function runs on a worker thread. It has no access to the JavaScript
// environment except through the thread-safe function.
static void tx_openThings_receive_thread(napi_env env, void *data)
{
    AddonData *addon_data = (AddonData *)data;
    int result;
    //napi_status status;

    TRACE_OUTS("tx_openThings_receive_thread starting\n");
    // We bracket the use of the thread-safe function by this thread by a call to
    // napi_acquire_threadsafe_function() here, and by a call to
    // napi_release_threadsafe_function() immediately prior to thread exit.
    assert(napi_acquire_threadsafe_function(addon_data->tsfn) == napi_ok);

    // Call the receive in a monitor loop, calling the tsfn when we have recieved a valid message via the radio adaptor
    addon_data->monitor = true;

    // Call Rx in a loop until we are told to stop, this uses an async function so shouldnt block node.js
    do
    {
        // Allocate the buffer from the heap. The JavaScript marshaller (tr_openThings_receive_thread)
        // will free this item after having sent it to JavaScript.
        //int* the_prime = malloc(sizeof(*the_prime));
        char *buf = malloc(500 * sizeof(char));

        result = openThings_receive(buf, 500, addon_data->timeout);
        if (result > 0)
        {
            // we have received a valid OpenThings message, call threadsafe function to notify js consumer
            // this also frees the malloc'ed memory
            assert(napi_call_threadsafe_function(addon_data->tsfn,
                                                 buf,
                                                 napi_tsfn_blocking) == napi_ok);
        }
        else
        {
            // we need to free instead
            TRACE_OUTS("*");
            free(buf);
        }
    } while (addon_data->monitor);

    // Indicate that monitoring is closing so there will be no further use of the thread-safe function.
    assert(napi_release_threadsafe_function(addon_data->tsfn, napi_tsfn_abort) == napi_ok);
    TRACE_OUTS("tx_ monitor thread completed\n");
}

// N-API Internal function - run when ts async function completes
// This function runs on the main thread after `tx_openThings_receive_thread` completes (when monitoring is stopped).
static void tc_openThings_receive_thread(napi_env env, napi_status status, void *data)
{
    AddonData *addon_data = (AddonData *)data;

    TRACE_OUTS("tc_ monitor thread closing...\n");
    // Clean up the thread-safe function and the work item associated with this
    // run.
    assert(napi_release_threadsafe_function(addon_data->tsfn,
                                            napi_tsfn_release) == napi_ok);
    assert(napi_delete_async_work(env, addon_data->work) == napi_ok);

    // Set both values to NULL so JavaScript can order a new run of the thread.
    addon_data->work = NULL;
    addon_data->tsfn = NULL;

    TRACE_OUTS("tc_ done\n");
}

/* N-API function (tf_) wrapper for:
**  static napi_value tf_openThings_receive_thread(napi_env env, napi_callback_info info)
**
** Use this function if you have any 'monitor' devices.
**
** This version creates a monitoring thread 
**
** Create a thread-safe function and an async queue work item. We pass the
** thread-safe function to the async queue work item so that messages can be returned
** into JavaScript from the worker thread on which the tx_openThings_receive_thread callback runs.
**
** JS Input params:
**  0: timeout
**  1: callback
*/
static napi_value tf_openThings_receive_thread(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value work_name;
    napi_status status;
    AddonData *addon_data;

    //TRACE_OUTS("tf_openThings_receive_thread>\n");

    // Retrieve the JavaScript args
    assert(napi_get_cb_info(env,
                            info,
                            &argc,
                            argv,
                            NULL,
                            (void **)(&addon_data)) == napi_ok);

    // Ensure that the monitor thread isn't already in progress.
    if (addon_data->work == NULL)
    {
        // Ensure that the first argument 'wait' is a number
        napi_valuetype type_of_argument;
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (type_of_argument != napi_number)
        {
            TRACE_OUTN(type_of_argument);
            TRACE_NL();
            napi_throw_type_error(env, NULL, "Param timeout is not a uint32 number");
        }
        else
        {
            // 0: get timeout
            status = napi_get_value_uint32(env, argv[0], &addon_data->timeout);

            if (status != napi_ok)
            {
                //timeout value incorrect, set default as 5s
                TRACE_OUTS("tf_ WARNING: Setting default timeout to 5000ms");
                addon_data->timeout = 5000;
            }

            // Create a string to describe this asynchronous operation.
            assert(napi_create_string_utf8(env,
                                           "ener314rt:OTRxThread",
                                           NAPI_AUTO_LENGTH,
                                           &work_name) == napi_ok);

            // Convert the callback retrieved from JavaScript into a thread-safe function
            // which we can call from a worker thread.
            status = napi_create_threadsafe_function(env,
                                                     argv[1],
                                                     NULL,
                                                     work_name,
                                                     0,
                                                     1,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     tr_openThings_receive_thread,
                                                     &(addon_data->tsfn));
            if (status != napi_ok)
            {
                TRACE_OUTN(status);
                napi_throw_error(env, NULL, "Unable to create tsfn");
            }
            TRACE_OUTS("tf_ napi_create_threadsafe_function done\n");

            // Create an async work item, passing in the addon data, which will give the
            // worker thread access to the above-created thread-safe function.
            assert(napi_create_async_work(env,
                                          NULL,
                                          work_name,
                                          tx_openThings_receive_thread,
                                          tc_openThings_receive_thread,
                                          addon_data,
                                          &(addon_data->work)) == napi_ok);

            // Queue the work item for execution.
            assert(napi_queue_async_work(env, addon_data->work) == napi_ok);
            TRACE_OUTS("tf_ monitor thread started, timeout=");
            TRACE_OUTN(addon_data->timeout);
            TRACE_NL();
            // This causes `undefined` to be returned to JavaScript.
        }
    } else {
        TRACE_OUTS("tf_ monitor thread still active, not started\n");
    }

    return NULL;
}

// Free the per-addon-instance data.
static void addon_getting_unloaded(napi_env env, void *data, void *hint)
{
    AddonData *addon_data = (AddonData *)data;

    TRACE_OUTS("addon_getting_unloaded>\n");
    assert(addon_data->work == NULL &&
           "No work item in progress at module unload");
    free(addon_data);
}

/* N-API function (nf_) for:
**  stop_monitoring()
**
** Calling this function notifies the monitor thread to close
*/
static napi_value nf_stop_openThings_receive_thread(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value argv[1];
    AddonData *addon_data;

    TRACE_OUTS("nf_stop_openThings_receive_thread called\n");

    // Retrieve the JavaScript data pointer
    assert(napi_get_cb_info(env,
                            info,
                            &argc,
                            argv,
                            NULL,
                            (void **)(&addon_data)) == napi_ok);

    // Ask monitor thread to stop
    addon_data->monitor = false;

    return 0;
}

// ----------FILE--------- ook_send.c

/* N-API function (nf_) wrapper for:
**  unsigned char ook_switch(unsigned int iZone, unsigned int iSwitchNum, unsigned char bSwitchState, unsigned char xmits)
*/
napi_value nf_ook_switch(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 4; // 4 passed in args
    napi_value argv[4];
    napi_value nv_ret;
    int ret;
    napi_valuetype type_of_argument;

    unsigned int iZone = 0, iSwitchNum = 1, xmits = 20;
    bool bSwitchState;

    // get args
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (status != napi_ok)
    {
        // we cant recover from this error
        napi_throw_error(env, NULL, "Failed to parse arguments");
        ret = -10;
    }
    else
    {
        // Parse all the arguments, first 3 need to be valid to proceed
        // 0: unsigned int iZone
        status = napi_typeof(env, argv[0], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "Zone not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[0], &iZone);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid Zone");
        }

        // 1: unsigned int iSwitchNum
        status = napi_typeof(env, argv[1], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "SwitchNum not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[1], &iSwitchNum);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid SwitchNum");
        }

        // 2: unsigned char bSwitchState
        status = napi_typeof(env, argv[2], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_boolean)
        {
            napi_throw_type_error(env, NULL, "SwitchState not boolean");
        }
        else
        {
            status = napi_get_value_bool(env, argv[2], &bSwitchState);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid SwitchState");
        }

        // 3: unsigned char xmits
        status = napi_typeof(env, argv[3], &type_of_argument);
        if (status != napi_ok || type_of_argument != napi_number)
        {
            napi_throw_type_error(env, NULL, "xmits not number");
        }
        else
        {
            status = napi_get_value_uint32(env, argv[3], &xmits);

            if (status != napi_ok)
                napi_throw_error(env, NULL, "Invalid xmits");
        }

        //printf("calling ook_send(%d,%d,%d,%d)\n", iZone, iSwitchNum, bSwitchState, xmits);

        // Call C routine
        ret = ook_switch(iZone, iSwitchNum, bSwitchState, xmits);

        //printf("OokSend() returned %d\n", ret);
    }

    // convert return value into JS value
    status = napi_create_int32(env, ret, &nv_ret);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to create return value");
    }

    return nv_ret;
}

// ----------------- END WRAPPERS -----------------------------

/*
** Initialise N-API functions and properties for the C library to be used from node.js / node-red
*/
napi_value Init(napi_env env, napi_value exports)
{
    napi_status status;

    TRACE_OUTS("napi_energenie.Init() called\n");

    // Define addon-level data associated with tsfn
    AddonData *addon_data = (AddonData *)malloc(sizeof(*addon_data));
    addon_data->work = NULL;

    // Export all functions to allow javascript calls, just by using something like ener314rt.<function>()
    //
    // Method taken from: https://github.com/1995parham/Napi101/blob/master/src/bye.c
    // This allows all functions to be defined in 1 go, replacing napi_create_function() & napi_set_named_property() calls
    //
    napi_property_descriptor props[] = {
        {.utf8name = "initEner314rt",
         .method = nf_init_ener314rt,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "closeEner314rt",
         .method = nf_close_ener314rt,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "openThingsSwitch",
         .method = nf_openThings_switch,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "openThingsDeviceList",
         .method = nf_openThings_deviceList,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "openThingsReceive",
         .method = nf_openThings_receive,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "openThingsCacheCmd",
         .method = nf_openThings_cache_cmd,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "openThingsReceiveThread",
         .method = tf_openThings_receive_thread,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = addon_data}, // note need to pass data
        {.utf8name = "stopMonitoring",
         .method = nf_stop_openThings_receive_thread,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = addon_data},
        {.utf8name = "ookSwitch",
         .method = nf_ook_switch,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL},
        {.utf8name = "sendRadioMsg",
         .method = nf_send_radio_msg,
         .getter = NULL,
         .setter = NULL,
         .value = NULL,
         .attributes = napi_default,
         .data = NULL}};

    // This method allows definition of multiple properties on a given object 'exports'
    // The properties are defined using property descriptors (see napi_property_descriptor).
    // Given an array of such property descriptors, this API will set the properties on the object one at a time, as defined by DefineOwnProperty()
    status = napi_define_properties(env, exports, sizeof(props) / sizeof(*props), props);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to populate exports");
    }

    // Associate the addon data with the exports object, to make sure that when
    // the addon gets unloaded our data gets freed.
    assert(napi_wrap(env,
                     exports,
                     addon_data,
                     addon_getting_unloaded,
                     NULL,
                     NULL) == napi_ok);

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
