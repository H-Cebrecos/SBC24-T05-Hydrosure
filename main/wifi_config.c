#ifndef CONF
#define CONF
#include "sdkconfig.h"
#include <stdbool.h>

static bool new_config = false;
static bool connection_success = false;
char ssid[32] = "SBC";
char passw[64] = "SBCwifi$";

#endif
