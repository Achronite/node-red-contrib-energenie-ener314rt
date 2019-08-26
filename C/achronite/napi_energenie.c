#define NAPI_VERSION 3
#include <node_api.h>
#include <stdio.h>
#include <unistd.h>

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
** Author: Phil Grainger - @Achronite, August 2019
**
** Conventions Used:
**  nf_   n-api wrapper of synchronous function
**  nv_   n-api value
**  na_   n-api wrapper of asynchronous functions
**  xcb_  execute callback function for async work
**  ccb_  complete callback function for async work
*/

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

    // Call C routine
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
** NOTE: OTmsg is output param, but we will 'return' it
*/
napi_value nf_openThings_receive(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value nv_ret;
    int ret;

    char buf[500];

    // no args

    //printf("calling openThings_receive()\n");

    // Call C routine
    ret = openThings_receive(buf);

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

        // 2: uint32_t data
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

// ----------FILE--------- ook_send.c
/*
 unsigned char ook_send(unsigned int iZone, unsigned int iSwitchNum, unsigned char bSwitchState, unsigned char xmits);
*/

/* N-API function (nf_) wrapper for:
**  unsigned char ook_send(unsigned int iZone, unsigned int iSwitchNum, unsigned char bSwitchState, unsigned char xmits)
*/
napi_value nf_ook_send(napi_env env, napi_callback_info info)
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
        ret = ook_send(iZone, iSwitchNum, bSwitchState, xmits);

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

/*
void bye_async_execute(napi_env env, void *data)
{
	sleep(1);
	printf("Hello async\n");
}

void bye_async_complete(napi_env env, napi_status status, void* data)
{
	printf("Hello completed async\n");
}

napi_value bye_async(napi_env env, napi_callback_info info)
{
	napi_value retval;
	napi_async_work work;
	napi_value async_resource_name;

	/*
	 * napi_status napi_create_async_work(napi_env env,
                                   napi_value async_resource,
                                   napi_value async_resource_name,
                                   napi_async_execute_callback execute,
                                   napi_async_complete_callback complete,
                                   void* data,
                                   napi_async_work* result);
	 * async_resource_name should be a null-terminated, UTF-8-encoded string.
	 * Note: The async_resource_name identifier is provided by the user and should be representative of the type of async work being performed. 
     * It is also recommended to apply namespacing to the identifier, e.g. by including the module name.
	 * See the async_hooks documentation for more information.
	 //
	napi_create_string_utf8(env, "bye:sleep", -1, &async_resource_name);
	napi_create_async_work(env, NULL, async_resource_name, bye_async_execute, bye_async_complete, NULL, &work);
	napi_queue_async_work(env, work);

	napi_create_int64(env, 6473, &retval);

	return retval;
}
*/

// ----------------- END WRAPPERS -----------------------------

/*
** Initialise N-API functions and properties for the C library to be used from node.js / node-red
*/
napi_value Init(napi_env env, napi_value exports)
{
    napi_status status;

    TRACE_OUTS("napi_energenie.Init() called\n");

    // Export all functions to allow javascript calls, just by using something like ener314rt.<function>()
    //
    // Method taken from: https://github.com/1995parham/Napi101/blob/master/src/bye.c
    // This allows all functions to be defined in 1 go, replacing napi_creat_function() & napi_set_named_property() calls
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
        {.utf8name = "ookSend",
         .method = nf_ook_send,
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
    /*
        {.utf8name = "numDevices",
         .method = NULL,
         .getter = ng_numDevices,
         .setter = NULL,
         .value = nv_numDevices,
         .attributes = napi_writable,
         .data = NULL}};
         */

    // This method allows definition of multiple properties on a given object 'exports'
    // The properties are defined using property descriptors (see napi_property_descriptor).
    // Given an array of such property descriptors, this API will set the properties on the object one at a time, as defined by DefineOwnProperty()
    status = napi_define_properties(env, exports, sizeof(props) / sizeof(*props), props);

    if (status != napi_ok)
    {
        napi_throw_error(env, NULL, "Unable to populate exports");
    }

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
