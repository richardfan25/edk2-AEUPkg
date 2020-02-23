#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../platform/adaptate.h"
#include "util.h"

/**
 * initialize the progress bar.
 * @max = 0
 * @val = 0
 *
 * @param	style
 * @param	tip words.
 */
/*==============================================================*/
void progress_init(
    progress_t *bar, char *title, int max)
{
    bar->chr = '=';
    bar->title = title;
    bar->max = max;
    bar->offset = 100 / (float)max;
    bar->pro = (char *) malloc(max+1);
    memset(bar->pro, 32, max);
    memset(bar->pro+max, 0x00, 1);
}

/*==============================================================*/
void progress_show( progress_t *bar, float bit )
{
    int val = (int)(bit * bar->max);

    memset(bar->pro, bar->chr, val);
    printf(PROGRESS_BUS_STRING, 
        bar->title, bar->pro, (int)(bar->offset * val));
    fflush(stdout);
}

/*==============================================================*/
//destroy the the progress bar.
void progress_destroy(progress_t *bar)
{
    free(bar->pro);
}

/*==============================================================*/
uint8_t dec_to_str(uint16_t dec, uint8_t *str, uint8_t pad_len)
{
	uint16_t	div;		// divisor
	uint8_t		digit, out;	// each digit, output flag
	uint8_t		*buf = str;
	uint8_t		len, dcnt;

	// max unsigned integer = 65535, so divior=10000 at starting
	out = 0;
	len = 0;
	dcnt = 5;
	for (div=10000; div>0; div/=10, dcnt--)
	{
		// get each digit 6,5,5,3,5
		digit = (uint8_t)(dec / div);

		// pad_len = 2, dec = 8,  output = "08"
		// pad_len = 1, dec = 8,  output = "8"
		// pad_len = 3, dec = 14, output = "014"
		if (dcnt <= pad_len)
			out = 1;

		// if digit <> 0, set output flag
		// if dec=0, we want to output '0' instead of "00000"
		// if div==1, forced output, if only one digit (dec=0~9)
		if (digit || div == 1)
			out = 1;

		if (out)
		{
			if (out > 1)
				digit = (uint8_t)(dec / div);	// get digit

			*buf++ = (digit + '0');
			len++;

			dec -= (digit*div);
			out++;
		}
	}

	return len;
}



