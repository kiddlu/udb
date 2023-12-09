#ifndef _UDB_DEBUG_H_
#define _UDB_DEBUG_H_

#define UDB_LAYER_DEF    (0x01)
#define UDB_LAYER_UVC_HS (0x02)
#define UDB_LAYER_CDC_HS (0x03)
#define UDB_LAYER_NET    (0x04)

void udb_debug_bridge(char layer, char *cmd, char *resp, int *resp_len);

#endif
