#ifndef _CONFIG_H
#define _CONFIG_H

#define UDB_PROT_SIZE_DEFAULT   (64)
#define UDB_PROT_SIZE_NET_UDP   (1470)
#define UDB_NET_UDP_SERVER_PORT (5164)

#define UDB_BACKEND_STD (0x01)
#define UDB_BACKEND_MCU (0x02)

#define USB_CLASS_UVC (0x01)
#define USB_CLASS_CDC (0x02)
#define USB_CLASS_HID (0x03)
#define NET_CLASS_UDP (0x04)

#define USB_XFER_CTRL (0x01)
#define USB_XFER_BULK (0x02)
#define USB_XFER_ISOC (0x03)
#define USB_XFER_INTR (0x04)

#endif
