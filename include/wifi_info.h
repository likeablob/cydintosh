#ifndef WIFI_INFO_H
#define WIFI_INFO_H

#include "umac_ipc.h"

void wifi_get_ap_list(umac_ipc_response_t *resp);
void wifi_get_status(umac_ipc_response_t *resp);

#endif