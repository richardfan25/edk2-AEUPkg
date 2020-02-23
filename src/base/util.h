#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <stdint.h>

typedef struct _progress_t{
    char	chr;		/*tip char*/
    char	*title;	/*tip string*/
    int		max;		/*maximum value*/
    float	offset;
    char	*pro;
} progress_t;

#define PROGRESS_NUM_STYLE 0
#define PROGRESS_CHR_STYLE 1
#define PROGRESS_BGC_STYLE 2

extern void progress_init(progress_t *, char *, int);

extern void progress_show(progress_t *, float);

extern void progress_destroy(progress_t *);

extern uint8_t dec_to_str(uint16_t dec, uint8_t *str, uint8_t pad_len);
#endif	/*_UTIL_H_*/
