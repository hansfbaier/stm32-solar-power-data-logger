/*
 * logging.c
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#include "logging.h"
#include "energygraph.h"
#include "sdio_sd.h"
#include "ff.h"
#include "printf.h"
#include "ugui.h"

FRESULT scan_files (char* path);
int SD_TotalSize(void);
void Restore_Today(void);

EnergyLogger solarLogger;
EnergyLogger houseLogger;

char path[32]="0:";

static FATFS fs;         /* Work area (file system object) for logical drive */
static FIL fsrc;         /* file objects */   
static FRESULT res;
static UINT br;

void PrintFileError(FRESULT res, const char message[])
{
    char buf[48];
    UG_ConsoleSetForecolor(C_RED);
    sprintf(buf, "error '%d' %s\r\n", res, message);
    UG_ConsolePutString(buf);
    UG_ConsoleSetForecolor(C_GREEN);
}

void Init_Logging(void)
{
    uint32_t rtc = RTC_GetCounter();
    int slot = (rtc % ONE_DAY) / (5 * 60);
    solarLogger.currentBinNo = slot;
    houseLogger.currentBinNo = slot;
    
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

    res = f_open(&fsrc, LOGFILENAME, FA_CREATE_NEW | FA_WRITE);

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
        res = f_open(&fsrc, LOGFILENAME, FA_OPEN_EXISTING | FA_WRITE | FA_READ);
        if (FR_OK != res)
        {
            PrintFileError(res, "opening log file");
        }
        
        Restore_Today();
        
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
    ++logger->bins[logger->currentBinNo];
    ++logger->impsToday;
}

void newBin(EnergyLogger *logger)
{    
    logger->currentBinNo = (logger->currentBinNo + 1) % NUM_BINS;
    logger->bins[logger->currentBinNo] = 0;
    if (0 == logger->currentBinNo)
    {
        logger->impsToday = 0;
    }
}

int getBin(EnergyLogger *logger, int binNo)
{
    return logger->bins[binNo];
}

int  getCurrentBin(EnergyLogger *logger)
{
    return getBin(logger, logger->currentBinNo);
}

int getLastBinNo(EnergyLogger *logger)
{
    if (0 == logger->currentBinNo)
    {
        return NUM_BINS - 1;
    }
    return logger->currentBinNo - 1;
}

int getLastBin(EnergyLogger *logger)
{
    return getBin(logger, getLastBinNo(logger));
}

void Write_Log_Entry(void)
{
    char buf[128];
    int day = TODAY;
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

void seek_until(char delimiter)
{
    char buf[2];
    unsigned int bytes_read;

    while (!f_eof(&fsrc))
    {
        res = f_read(&fsrc, buf, 1, &bytes_read);
        if (FR_OK != res)
        {
            PrintFileError(res, "reading from file");
            return;
        }
        
        if (delimiter == buf[0]) break;
    }
}

int my_atoi(char *p)
{
    int k = 0;
    while (' ' == *p) p++;
    while (*p) {
      k = k*10 + (*p) - '0';
      p++;
    }
    return k;
}

int read_number_until(char delimiter)
{
    static char buf[32];
    unsigned int bytes_read = 0;
    int i = 0;
    
    while (1)
    {
        res = f_read(&fsrc, buf + i, 1, &bytes_read);
        if (FR_OK != res)
        {
            PrintFileError(res, "reading from file");
            return -1;
        }
                
        if (delimiter == buf[i] || f_eof(&fsrc))
        {
            buf[i] = 0;
            break;
        }

        i++;
    }
    
    return my_atoi(buf);
}

void Restore_Today(void)
{
    res = f_lseek(&fsrc, 0);
    if (FR_OK != res)
    {
        PrintFileError(res, "seeking start of log file");
        return;
    }
    
    int day = 0;
    int today = TODAY;
    do
    {
        seek_until('\n');
        day = read_number_until(',');        
    } while (day != today);
        
    if (-1 == day) goto bye;

    while (!f_eof(&fsrc))
    {
        int hour = read_number_until(':');
        int minute = read_number_until(':');
        int bin = (hour * 60 + minute) / 5;
        seek_until(',');
        int solarImps = read_number_until(',');
        int houseImps = read_number_until('\n');
        day = read_number_until(',');
        solarLogger.currentBinNo = bin;
        solarLogger.bins[bin] = solarImps;
        solarLogger.impsToday += solarImps;
        houseLogger.currentBinNo = bin;
        houseLogger.bins[bin] = houseImps;        
        houseLogger.impsToday += houseImps;
        
        plotBin(bin);
    }
    
    bye:
    res = f_lseek(&fsrc, fsrc.fsize);
    if (FR_OK != res)
    {
        PrintFileError(res, "seeking start of log file");
    }
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
      char buf[32];
      sprintf(buf,
              "%d MB of %d MB.",
              (fre_clust * fs->csize) / 2 / 1024,
              ((fs->n_fatent - 2) * fs->csize ) / 2 / 1024);
        
      UG_SetForecolor(C_WHITE);
      UG_PutString(0, 0, buf);
      
      return ENABLE;
    }
    else 
      return DISABLE;   
}


void SDIO_IRQHandler(void)
{
  SD_ProcessIRQSrc();
}
