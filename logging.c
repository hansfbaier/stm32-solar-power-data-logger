/*
 * logging.c
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#include "logging.h"
#include "sdio_sd.h"
#include "ff.h"
#include "printf.h"

FRESULT scan_files (char* path);
int SD_TotalSize(void);

EnergyLogger solarLogger;
EnergyLogger houseLogger;

char path[32]="0:";

static FATFS fs;         /* Work area (file system object) for logical drive */
static FIL fsrc;         /* file objects */   
static FRESULT res;
static UINT br;

void Init_Logging(void)
{
    if (SD_Detect() == SD_PRESENT)
    {
        printf("-- SD card detected\r\n");
    }
    else
    {
        printf("-- Please connect a SD card \r\n");
        while (SD_Detect() != SD_PRESENT);
        printf("-- SD card connected\r\n");
    }

    f_mount(0, &fs);

    res = f_open(&fsrc, "0:/WattLog.TXT", FA_CREATE_NEW | FA_WRITE);

    if (res == FR_OK)
    {
        //res = f_write(&fsrc, textFileBuffer, sizeof(textFileBuffer), &br);     
        printf("WattLog.TXT created\r\n");

        f_close(&fsrc);
    }
    else if (res == FR_EXIST)
    {
        printf("WattLog.TXT exists\r\n");
    }

    scan_files(path);
    SD_TotalSize();
}

void addImp(EnergyLogger *logger)
{
    ++logger->currentImps;
}

void newBin(EnergyLogger *logger)
{
    logger->bins[logger->currentBin] = logger->currentImps;
    logger->currentImps = 0;
    logger->currentBin = (logger->currentBin + 1) % NUM_BINS;
}

int getBin(EnergyLogger *logger, int binNo)
{
    return logger->bins[binNo];
}

int lastBinNo(EnergyLogger *logger)
{
    return logger->currentBin - 1;
}

int getLastBin(EnergyLogger *logger)
{
    return getBin(logger, lastBinNo(logger));
}

FRESULT scan_files (char* path)
{
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;
#if _USE_LFN
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            if (fno.fname[0] == '.') continue;
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            printf("%s/%s \r\n", path, fn);
        }
    }

    return res;
}

int SD_TotalSize(void)
{
    FATFS *fs;
    DWORD fre_clust;        

    res = f_getfree("0:", &fre_clust, &fs); 
    if ( res==FR_OK ) 
    {
      /* Print free space in unit of MB (assuming 512 bytes/sector) */
      printf("\r\n%d MB total drive space.\r\n"
           "%d MB available.\r\n",
           ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );
        
      return ENABLE;
    }
    else 
      return DISABLE;   
}


void SDIO_IRQHandler(void)
{
  SD_ProcessIRQSrc();
}
