#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "rlog.h"

//ensure
#ifdef printf
#undef printf
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))

typedef struct
{
    size_t  rbuf_size;
    size_t  rbuf_usedsize;
    uint8_t rbuf_full;
    void   *rbuf_start;
    void   *rbuf_end;
    void   *rbuf_head;
} rbuf_t;

#if 0
rbuf_t *rbuf_new(size_t size)
{
    rbuf_t *rbuf = malloc(sizeof(rbuf_t));
    if (rbuf == NULL) return NULL;
    rbuf->rbuf_start = malloc(size);
    if (rbuf->rbuf_start == NULL) return NULL;
    rbuf->rbuf_size = size;
    rbuf->rbuf_head = NULL; 
    rbuf->rbuf_tail = rbuf->rbuf_start;
    return rbuf;
}

void rbuf_free(rbuf_t *rbuf) {
    free(rbuf->rbuf_start);
    free(rbuf);
}
#endif
void rbuf_init(rbuf_t *rbuf, void *buf, size_t size)
{
    if (buf == NULL || size <= 0)
        return;

    rbuf->rbuf_start    = buf;
    rbuf->rbuf_end      = buf + size;
    rbuf->rbuf_size     = size;
    rbuf->rbuf_usedsize = 0;
    rbuf->rbuf_full     = 0;
    rbuf->rbuf_head     = buf;

    return;
}

void rbuf_flush(rbuf_t *rbuf)
{
    rbuf->rbuf_head = rbuf->rbuf_start;

    memset(rbuf->rbuf_start, 0, rbuf->rbuf_size);
}

size_t rbuf_size(const rbuf_t *rbuf)
{
    return rbuf->rbuf_size;
}

size_t rbuf_usedsize(const rbuf_t *rbuf)
{
    return rbuf->rbuf_usedsize;
}

void rbuf_put(rbuf_t *rbuf, const void *data, size_t count)
{
    size_t left = (rbuf->rbuf_end - rbuf->rbuf_head);
    if (left >= count)
    {
        memcpy(rbuf->rbuf_head, data, count);
        rbuf->rbuf_head += count;
    }
    else
    {
        size_t over_size = count - left;
        memcpy(rbuf->rbuf_head, data, left);
        memcpy(rbuf->rbuf_start, (data + left), over_size);
        rbuf->rbuf_head = rbuf->rbuf_start + over_size;
    }

    if (rbuf->rbuf_full == 0)
    {
        rbuf->rbuf_usedsize += count;
        if (rbuf->rbuf_usedsize > rbuf->rbuf_size)
        {
            rbuf->rbuf_full     = 1;
            rbuf->rbuf_usedsize = 512;
        }
    }

    return;
}

void rbuf_get(rbuf_t *rbuf, void *data, size_t count, size_t offset)
{
    void *off_head;

    if (rbuf->rbuf_full == 1)
    {
        off_head = rbuf->rbuf_head + offset;
    }
    else
    {
        off_head = rbuf->rbuf_start + offset;
    }

    size_t left = (rbuf->rbuf_end - off_head);
    if (left >= count)
    {
        memcpy(data, off_head, count);
    }
    else
    {
        size_t over_size = count - left;
        memcpy(data, off_head, left);
        memcpy((data + left), rbuf->rbuf_start, over_size);
    }

    return;
}

//======================================================================

static rbuf_t g_rlog_hdl;
static char   g_rlog_buf[RLOG_MAX_LOG_BUF_SIZE + 1];

void rlog_init(void)
{
    rbuf_init(&g_rlog_hdl, g_rlog_buf, RLOG_MAX_LOG_BUF_SIZE);

    return;
}

int rlog_printf2buf(const char *fmt, ...)
{
    char    tmp_buf[RLOG_MAX_ONE_LOG_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp_buf, (RLOG_MAX_ONE_LOG_SIZE - 1), fmt, args);

    rbuf_put(&g_rlog_hdl, tmp_buf, strlen(tmp_buf));

    // printf("%s", tmp_buf);
    return strlen(tmp_buf);
}

void rlog_dump(void *data, size_t count, size_t offset)
{
    if (offset > rbuf_size(&g_rlog_hdl))
    {
        return;
    }

    rbuf_get(&g_rlog_hdl, data, count, offset);

    return;
}

int rlog_size(void)
{
    return (rbuf_usedsize(&g_rlog_hdl));
}

#if 0
static void dumphex(void *data, unsigned int size)
{
	char ascii[17];
	unsigned int i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

void rlog_dumphex(void)
{
    printf("\nstart %p, end %p, head %p usedsize %ld\n", 
        g_rlog_hdl.rbuf_start,
        g_rlog_hdl.rbuf_end,
        g_rlog_hdl.rbuf_head,
        g_rlog_hdl.rbuf_usedsize);

    dumphex(g_rlog_hdl.rbuf_start, 512);

    return;
}
#else
void rlog_dumphex(void)
{
}
#endif
