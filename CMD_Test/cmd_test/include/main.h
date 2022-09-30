#ifndef __MAIN_H
#define __MAIN_H

#define MAX_BUFFER_SIZE 1024
#define wait_ms(ms) _wait_us_inline((int)1000 * ms)

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

#endif