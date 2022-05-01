#define _LFS_LOGLEVEL_ 1
#define RP2040_FS_SIZE_KB 512

#define FORCE_REFORMAT false
#include <LittleFS_Mbed_RP2040.h>
#include <ArduinoJson.h>

// #ifndef __BMK10R_H
#include "BKM10Rduino.h"
// #endif

enum StorageError
{
    Ok,
    MissingFile = -1,
    EmptyFile = -2,
    ParsingError = -3,
    SaveFailed = -4
};

class StoreClass
{
public:
    StoreClass(void);

    bool initFS();
    bool exists(char *filename);
    bool write(char *data, char *filename);
    bool save(RemoteKey keys[], const char *filename);
    int saveStoredKeys(const char *filename);
    int loadKeys(const char *filename);
    const char *errorMsg(int error);
    RemoteKey getKey(int id);
    int putKey(int index, RemoteKey key, bool saveAfter);

private:
    LittleFS_MBED *fsStore;
};

extern StoreClass Store;
