#ifndef _LIST_H
#define _LIST_H

bool in_support_list(unsigned short vid, unsigned short pid);

bool in_support_list_only_vid(unsigned short vid);

bool in_support_list_all(unsigned short vid,
                         unsigned short pid,
                         unsigned char  usbclass);

void *get_support_list_guid(unsigned short vid, unsigned short pid);

unsigned char get_support_list_usbclass(unsigned short vid, unsigned short pid);

unsigned char get_support_list_backend(unsigned short vid, unsigned short pid);

unsigned short get_support_list_prot_size(unsigned short vid,
                                          unsigned short pid);

unsigned char get_support_list_selector(unsigned short vid, unsigned short pid);

unsigned char get_support_list_unitid(unsigned short vid, unsigned short pid);

int print_support_list(void);

#endif
