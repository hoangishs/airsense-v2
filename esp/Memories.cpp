#include "./Memories.h"

void convertToByte(uint32_t _data, uint8_t* _toData, uint8_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    _toData[size - i - 1] = _data & 0xff;
    _data = _data >> 8;
  }
}

uint8_t getFlashCount(File _f, uint32_t* __front, uint32_t* __rear)
{
  uint8_t _flashCountSize;
  _f.seek(0);
  _f.read(&_flashCountSize, 1);

  _f.seek(1);
  uint8_t _front[_flashCountSize];
  _f.read(_front, _flashCountSize);// front in text file

  _f.seek(_flashCountSize + 1);
  uint8_t _rear[_flashCountSize];
  _f.read(_rear, _flashCountSize);// rear in text file

  *__front = 0;
  *__rear = 0;

  for (size_t i = 0; i < _flashCountSize; i++)
  {
    *__front += _front[i] << (8 * (_flashCountSize - i - 1));
    *__rear += _rear[i] << (8 * (_flashCountSize - i - 1));
  }
  return _flashCountSize;
}

bool isQueueFlashEmpty(const char* _fileName)
{
  uint32_t _front = 0;
  uint32_t _rear = 0;
  viewFlash(&_front, &_rear, _fileName);
  if (_front == _rear)
    return true;
  else
    return false;
}

bool deQueueFlash(const char* _fileName)
{
  File _f = SPIFFS.open(_fileName, "r+");
  if (_f)
  {
    uint32_t __front = 0;
    uint32_t __rear = 0;

    uint8_t _flashCountSize = getFlashCount(_f, &__front, &__rear);

    if (__front == __rear)
    {
      return false;// file empty
    }

    DEBUG.print("deQueueFlash ");
    DEBUG.print(__front);
    DEBUG.print(" ");
    DEBUG.print(__rear);

    __front = (__front + FLASH_DATA_SIZE) % FLASH_QUEUE_SIZE;

    DEBUG.print(" -> ");
    DEBUG.print(__front);
    DEBUG.print(" ");
    DEBUG.println(__rear);

    uint8_t _front[_flashCountSize];

    convertToByte(__front, _front, _flashCountSize);

    _f.seek(1);
    _f.write(_front, _flashCountSize);// new front in text file

    _f.close();
    return true;// deQueue
  }
  else
  {
    return false;// text file is not exists
  }
}

bool checkQueueFlash(uint8_t* _data, const char* _fileName)
{
  File _f = SPIFFS.open(_fileName, "r");
  if (_f)
  {
    uint32_t __front = 0;
    uint32_t __rear = 0;

    getFlashCount(_f, &__front, &__rear);

    if (__front == __rear)
    {
      return false;// file empty
    }

    _f.seek(__front);
    _f.read(_data, FLASH_DATA_SIZE); // data

    DEBUG.print("checkQueueFlash ");
    for (size_t i = 0; i < FLASH_DATA_SIZE; i++)
    {
      DEBUG.print(_data[i]);
      DEBUG.print(" ");
    }
    DEBUG.println();

    _f.close();

    return true;
  }
  else
  {
    return false;// text file is not exists
  }
}

bool viewFlash(uint32_t* __front, uint32_t* __rear, const char* _fileName)
{
  File _f = SPIFFS.open(_fileName, "r");
  if (_f)
  {
    getFlashCount(_f, __front, __rear);

    _f.close();

    return true;
  }
  else
  {
    return false;
  }
}

void initQueueFlash(const char* _fileName)
{
  if (FLASH_QUEUE_SIZE % FLASH_DATA_SIZE != 0)
  {
    return;//
  }

  File _f;

  if (SPIFFS.exists(_fileName))
  {
    _f = SPIFFS.open(_fileName, "r+");

    _f.seek(0);
    uint8_t _flashCountSize = _f.read();

    DEBUG.print("initQueueFlash ");
    DEBUG.println(_flashCountSize);

    if (_flashCountSize == FLASH_COUNT_SIZE)
    {
      _f.close();
      return;//
    }
  }
  else
  {
    _f = SPIFFS.open(_fileName, "w");
  }

  uint8_t count[FLASH_COUNT_SIZE] = { 0 };
  count[FLASH_COUNT_SIZE - 1] = 2 * FLASH_COUNT_SIZE;

  _f.seek(0);
  _f.write(FLASH_COUNT_SIZE);

  _f.seek(1);
  _f.write(count, FLASH_COUNT_SIZE);
  _f.seek(FLASH_COUNT_SIZE + 1);
  _f.write(count, FLASH_COUNT_SIZE);

  _f.close();
}

bool enQueueFlash(uint8_t* _data, const char* _fileName)
{
  File _f = SPIFFS.open(_fileName, "r+");
  if (_f)
  {
    uint32_t __front = 0;
    uint32_t __rear = 0;

    uint8_t _flashCountSize = getFlashCount(_f, &__front, &__rear);

    _f.seek(__rear);
    _f.write(_data, FLASH_DATA_SIZE);

    DEBUG.print("enQueueFlash ");
    for (size_t i = 0; i < FLASH_DATA_SIZE; i++)
    {
      DEBUG.print(_data[i]);
      DEBUG.print(" ");
    }
    DEBUG.println();

    if (__front == (__rear + FLASH_DATA_SIZE) % FLASH_QUEUE_SIZE)
    {
      __front = (__front + FLASH_DATA_SIZE) % FLASH_QUEUE_SIZE;

      uint8_t _front[_flashCountSize];

      convertToByte(__front, _front, _flashCountSize);

      _f.seek(1);
      _f.write(_front, _flashCountSize);
    }

    DEBUG.print("enQueueFlash ");
    DEBUG.print(__front);
    DEBUG.print(" ");
    DEBUG.print(__rear);

    __rear = (__rear + FLASH_DATA_SIZE) % FLASH_QUEUE_SIZE;

    DEBUG.print(" -> ");
    DEBUG.print(__front);
    DEBUG.print(" ");
    DEBUG.println(__rear);

    uint8_t _rear[_flashCountSize];

    convertToByte(__rear, _rear, _flashCountSize);

    _f.seek(_flashCountSize + 1);
    _f.write(_rear, _flashCountSize);

    _f.close();

    return true;
  }
  else
  {
    return false;
  }
}
