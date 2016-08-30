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
#include "ugui.h"

FRESULT scan_files (char* path);
int SD_TotalSize(void);

EnergyLogger solarLogger;
EnergyLogger houseLogger;

char path[32]="0:";

static FATFS fs;         /* Work area (file system object) for logical drive */
static FIL fsrc;         /* file objects */   
static FRESULT res;
static UINT br;

void PrintFileError(FRESULT res, char message[])
{
    char buf[48];
    UG_ConsoleSetForecolor(C_RED);
    sprintf(buf, "error '%d' %s\r\n", res, message);
    UG_ConsolePutString(buf);
    UG_ConsoleSetForecolor(C_GREEN);
}

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

    res = f_open(&fsrc, "0:/WattLog.csv", FA_CREATE_NEW | FA_WRITE);

    if (res == FR_OK)
    {
        char header[] = "Day,Time,Solar Imps,House Imps\r\n";
        res = f_write(&fsrc, header, sizeof(header), &br);
        if (FR_OK != res)
        {
            PrintFileError(res, "creating log file");
        }
        else
        {
            printf("WattLog.csv created\r\n");
        }
    }
    else if (res == FR_EXIST)
    {
        printf("WattLog.csv exists\r\n");
        res = f_open(&fsrc, "0:/WattLog.csv", FA_OPEN_EXISTING | FA_WRITE);
        if (FR_OK != res)
        {
            PrintFileError(res, "opening log file");
        }
        
        // go to end of file
        res = f_lseek(&fsrc, fsrc.fsize);
        if (FR_OK != res)
        {
            PrintFileError(res, "seeking log file");
        }
    }

    //scan_files(path);
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

void Write_Log_Entry(void)
{
    char buf[128];
    int day = (int)(RTC_GetCounter() / (24 * 60 * 60));
    sprintf(buf, "%d,%s,%4d,%4d\n", day, Time_As_String(), getLastBin(&solarLogger), getLastBin(&houseLogger));
    res = f_write(&fsrc, buf, strlen(buf), &br);
    if (FR_OK != res)
    {
        PrintFileError(res, "writing log entry");
    }
    res = f_sync(&fsrc);
    if (FR_OK != res)
    {
        PrintFileError(res, "syncing log file");
    }
    UG_ConsoleSetForecolor(C_GREEN);
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
      printf("%d MB of %d MB free.\r\n",
             (fre_clust * fs->csize) / 2 / 1024,
             ((fs->n_fatent - 2) * fs->csize ) / 2 / 1024);
        
      return ENABLE;
    }
    else 
      return DISABLE;   
}


void SDIO_IRQHandler(void)
{
  SD_ProcessIRQSrc();
}
