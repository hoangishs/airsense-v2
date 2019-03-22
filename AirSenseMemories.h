#ifndef AirSenseMemories_h
#define AirSenseMemories_h

#include "AirSenseStructDefine.h"

#include <FS.h>

#define FLASH_COUNT_SIZE 3
#define FLASH_DATA_SIZE 16
#define FLASH_QUEUE_SIZE 100000     //FLASH_DATA_SIZE*NUM_DATA

bool deQueueFlash(const char* _fileName);
bool checkQueueFlash(uint8_t* _data, const char* _fileName);
bool viewFlash(uint32_t* __front, uint32_t* __rear, const char* _fileName);
void initQueueFlash(const char* _fileName);
bool enQueueFlash(uint8_t* _data, const char* _fileName);
bool isQueueFlashEmpty(const char* _fileName);
void emptyQueue(const char* _fileName);
bool getPreData(uint8_t* _data, const char* _fileName);
bool fixTimeError(uint32_t* _mark, const char* _fileName);
uint8_t getFlashCount(File _f, uint32_t* __front, uint32_t* __rear);
void convertToByte(uint32_t _data, uint8_t* _toData, uint8_t size);

#endif
