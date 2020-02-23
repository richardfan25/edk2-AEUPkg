#include <stdlib.h>
#include <string.h>

#include "platform/adaptate.h"
#include "base\util.h"
#include "base\rdc.h"
#include "is200_test.h"

/*==============================================================*/
int is200_binary_dump_out(char *name)
{
	uint32_t	addr;
	FILE 		*pFile = NULL;
	uint8_t		*pBuf = NULL;
	
	fprintf(stderr, "Dump file : %s\n",name);
	if((pBuf = (uint8_t *)malloc(0x8000)) == NULL){
		fprintf(stderr, "ERROR: failed to backup.\n");
		return -1;
	}
	
	pFile = fopen(name, "w");
	if(pFile == NULL){
		fprintf(stderr, "ERROR: Create file failed.\n");
		free(pBuf);
		return -1;
	}
	else{
		fseek(pFile, 0, SEEK_SET);
		for(addr = 0; addr < 0x100000; addr += 0x8000){
			if(rdc_fla_read(addr, pBuf, 0x8000) != 0x8000){
				fprintf(stderr, "ERROR: Read firmware failed.\n");
				fclose(pFile);
				free(pBuf);
				return -1;
			}

			fprintf(stderr, "%0.1f%%\r",(float)addr / 0x100000 * 100);

			if(fwrite(pBuf, 1, 0x8000, pFile) != 0x8000){
				fprintf(stderr, "ERROR: Write to file failed.\n");
				fclose(pFile);
				free(pBuf);
				return -1;
			}
		}
		fclose(pFile);
		//fprintf(stderr, "processing... 100%%\n");
	}
	fprintf(stderr, "100%%\n");
	free(pBuf);
	return 0;
}
