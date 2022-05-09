#include "store.hpp"
#include <string.h>
#include <lfs.h>

#define CVC707V
#include "defaults.h"

#define FORCE_DEFAULT_KEYS false

#define KEY_FILE_SIZE_BYTES 1024
#define ARDUINOJSON_ENABLE_STD_STREAM

const char *f = MBED_LITTLEFS_FILE_PREFIX "/keys.json";


RemoteKey storedKeys [COMMANDS_SIZE] = {}; 

StoreClass::StoreClass(void)
{
}

bool StoreClass::initFS()
{
    fsStore = new LittleFS_MBED();
    return fsStore->init();
}

bool StoreClass::exists(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}

// const int capacity = JSON_ARRAY_SIZE(45) + 45 * JSON_OBJECT_SIZE(3);
const int capacity = 3072;

void printKey(RemoteKey key)
{
    Serial.print("key => ");
    Serial.print("id: ");
    Serial.print(key.id);
    Serial.print(", ");
    Serial.print("address: ");
    Serial.print(key.address);
    Serial.print(", ");
    Serial.print("code: ");
    Serial.println(key.code);
}

bool StoreClass::save(RemoteKey keys[], const char *filename)
{
    Serial.println("start saving");
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        Serial.println("save failed");
        return false;
    }
    Serial.println("save file ok");
    DynamicJsonDocument doc(capacity);

    for (int i = 0; i < COMMANDS_SIZE; i++)
    {
        RemoteKey key = keys[i];
        printKey(key);
        JsonObject o = doc.createNestedObject();
        o["id"] = i;
        o["address"] = key.address;
        o["code"] = key.code;
    }
    Serial.println("create objects");
    char ser[4096];
    size_t success = serializeJson(doc, ser);
    serializeJson(doc, Serial);
    Serial.println(ser);
    if (success)
    {
        fputs(ser, file);
    }
    fclose(file);
    return success;
}

int StoreClass::saveStoredKeys(const char *filename)
{
    return save(storedKeys, filename);
}

int StoreClass::loadKeys(const char *filename)
{
    char input[4096];

    FILE *file = fopen(filename, "r");
    if (FORCE_DEFAULT_KEYS || !file)
    {
        Serial.println("no file, creating keys");
        strcpy(input, defaultKeys);
    }
    else
    {
        Serial.println("start read");
        char c;
        uint32_t numRead = 1;
        int idx = 0;
        while (numRead)
        {
            numRead = fread((uint8_t *)&c, sizeof(c), 1, file);
            if (numRead)
            {
                input[idx] = c;
            }
            idx++;
        }
        fclose(file);
    }

    Serial.println("input:");
    
    String out = String(input);
    out.trim();
    Serial.println(out);

    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, input);
    if (err != DeserializationError::Ok)
    {
        for (int i = 0; i < COMMANDS_SIZE; i++)
        {
            storedKeys[i] = {(unsigned short)i, (unsigned char)i, (unsigned char)i};
        }
        return StorageError::ParsingError;
    }

    JsonArray jsonKeys = doc.as<JsonArray>();
    int count = jsonKeys.size();
    Serial.println("start deserializing to keys");
    for (int i = 0; i < count; i++)
    {
        JsonObject k = jsonKeys[i];
        storedKeys[i] = {k["address"], k["code"], k["id"]};
        printKey(storedKeys[i]);
    }
    return StorageError::Ok;
}

const char *StoreClass::errorMsg(int error)
{
    if (error == Ok)
        return "Ok";
    else if (error == MissingFile)
        return "MissingFile";
    else if (error == EmptyFile)
        return "Empty File";
    else if (error == ParsingError)
        return "Parsing error";
    else if (error == SaveFailed)
        return "Save file failed";
    else
        return "Unknown Error";
}

RemoteKey StoreClass::getKey(int id)
{
    return storedKeys[id];
}

int StoreClass::putKey(int index, RemoteKey key, bool saveAfter = false)
{
    if (index != key.id) {
        Serial.println(String("save id mismatch: ") + index + ", " + key.id);
        return StorageError::SaveFailed; 
    }

    storedKeys[index] = key;
    if (saveAfter)
    {
        return save(storedKeys, f);
    }
    else
    {
        return StorageError::Ok;
    }
}
