#include "store.hpp"
#include <string.h>
#include <lfs.h>

#define KEY_FILE_SIZE_BYTES 1024
#define ARDUINOJSON_ENABLE_STD_STREAM

const char *f = MBED_LITTLEFS_FILE_PREFIX "/keys.json";

const char *onkyoJson = "[{\"id\":0,\"address\":27858,\"code\":203},{\"id\":1,\"address\":6610,\"code\":192},{\"id\":2,\"address\":28114,\"code\":151},{\"id\":3,\"address\":28114,\"code\":130},{\"id\":4,\"address\":28114,\"code\":131},{\"id\":5,\"address\":722,\"code\":222},{\"id\":6,\"address\":722,\"code\":213},{\"id\":7,\"address\":722,\"code\":214},{\"id\":8,\"address\":722,\"code\":215},{\"id\":9,\"address\":722,\"code\":216},{\"id\":10,\"address\":722,\"code\":217},{\"id\":11,\"address\":722,\"code\":218},{\"id\":12,\"address\":722,\"code\":219},{\"id\":13,\"address\":722,\"code\":220},{\"id\":14,\"address\":722,\"code\":221},{\"id\":15,\"address\":28114,\"code\":93},{\"id\":16,\"address\":28114,\"code\":149},{\"id\":17,\"address\":27858,\"code\":136},{\"id\":18,\"address\":28114,\"code\":2},{\"id\":19,\"address\":722,\"code\":144},{\"id\":20,\"address\":722,\"code\":143},{\"id\":21,\"address\":722,\"code\":129},{\"id\":22,\"address\":722,\"code\":141},{\"id\":23,\"address\":722,\"code\":128},{\"id\":24,\"address\":722,\"code\":145},{\"id\":25,\"address\":722,\"code\":142},{\"id\":26,\"address\":28114,\"code\":3},{\"id\":27,\"address\":44242,\"code\":208},{\"id\":28,\"address\":44242,\"code\":209},{\"id\":29,\"address\":44242,\"code\":210},{\"id\":30,\"address\":27858,\"code\":78},{\"id\":31,\"address\":28114,\"code\":5},{\"id\":32,\"address\":6610,\"code\":135},{\"id\":33,\"address\":6610,\"code\":130},{\"id\":34,\"address\":6610,\"code\":131}][{\"id\":0,\"address\":27858,\"code\":203},{\"id\":1,\"address\":6610,\"code\":192},{\"id\":2,\"address\":28114,\"code\":151},{\"id\":3,\"address\":28114,\"code\":130},{\"id\":4,\"address\":28114,\"code\":131},{\"id\":5,\"address\":722,\"code\":222},{\"id\":6,\"address\":722,\"code\":213},{\"id\":7,\"address\":722,\"code\":214},{\"id\":8,\"address\":722,\"code\":215},{\"id\":9,\"address\":722,\"code\":216},{\"id\":10,\"address\":722,\"code\":217},{\"id\":11,\"address\":722,\"code\":218},{\"id\":12,\"address\":722,\"code\":219},{\"id\":13,\"address\":722,\"code\":220},{\"id\":14,\"address\":722,\"code\":221},{\"id\":15,\"address\":28114,\"code\":93},{\"id\":16,\"address\":28114,\"code\":149},{\"id\":17,\"address\":27858,\"code\":136},{\"id\":18,\"address\":28114,\"code\":2},{\"id\":19,\"address\":722,\"code\":144},{\"id\":20,\"address\":722,\"code\":143},{\"id\":21,\"address\":722,\"code\":129},{\"id\":22,\"address\":722,\"code\":141},{\"id\":23,\"address\":722,\"code\":128},{\"id\":24,\"address\":722,\"code\":145},{\"id\":25,\"address\":722,\"code\":142},{\"id\":26,\"address\":28114,\"code\":3},{\"id\":27,\"address\":44242,\"code\":208},{\"id\":28,\"address\":44242,\"code\":209},{\"id\":29,\"address\":44242,\"code\":210},{\"id\":30,\"address\":27858,\"code\":78},{\"id\":31,\"address\":28114,\"code\":5},{\"id\":32,\"address\":6610,\"code\":135},{\"id\":33,\"address\":6610,\"code\":130},{\"id\":34,\"address\":6610,\"code\":131}]";

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
    Serial.print(key.address, HEX);
    Serial.print(", ");
    Serial.print("code: ");
    Serial.println(key.code, HEX);
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
    // size_t _size = sizeof(keys) / sizeof(RemoteKey);
    DynamicJsonDocument doc(capacity);
    // JsonArray _array = doc.to<JsonArray>();

    for (int i = 0; i < COMMANDS_SIZE; i++)
    {
        RemoteKey key = keys[i];
        printKey(key);
        JsonObject o = doc.createNestedObject();
        o["id"] = key.id;
        o["address"] = key.address;
        o["code"] = key.code;
        // _array[i] = o;
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
    if (!file)
    {
        Serial.println("no file, creating keys");
        strcpy(input, onkyoJson);
        // for (int i = 0; i < COMMANDS_SIZE; i++)
        // {
        //     storedKeys[i] = {(unsigned short)i, (unsigned char)i, (unsigned char)i};
        //     Serial.println(storedKeys[i].id);
        // }
        // return StorageError::MissingFile;
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
    Serial.println(input);

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
    RemoteKey keys[COMMANDS_SIZE];
    int count = jsonKeys.size();
    Serial.println("start deserializing to keys");
    for (int i = 0; i < count; i++)
    {
        JsonObject k = jsonKeys[i];
        keys[i] = {k["address"], k["code"], k["id"]};
        printKey(keys[i]);
    }
    storedKeys = keys;
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

int StoreClass::putKey(RemoteKey key, bool saveAfter = false)
{
    storedKeys[key.id] = key;
    if (saveAfter)
    {
        return save(storedKeys, f);
    }
    else
    {
        return StorageError::Ok;
    }
}
