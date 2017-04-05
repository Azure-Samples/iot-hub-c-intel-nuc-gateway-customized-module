/*
 * IoT Gateway BLE Script - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include "azure_c_shared_utility/gballoc.h"

#include <stddef.h>
#include <azure_c_shared_utility/strings.h>
#include <ctype.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "my_module.h"
#include "broker.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"

#include <parson.h>

static int messageCount = 0;
const char * temperature_uuid = "F000AA01-0451-4000-B000-000000000000";
BROKER_HANDLE myBroker;
void* MyModule_ParseConfigurationFromJson(const char* configuration)
{
    (void)configuration;
    return NULL;
}

void MyModule_FreeConfiguration(void * configuration)
{
    (void)configuration;
    return;
}

MODULE_HANDLE MyModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    myBroker = broker;
    (void)configuration;
    return (MODULE_HANDLE)0x42;
}

void MyModule_Destroy(MODULE_HANDLE module)
{
    (void)module;
}

float getTemperature(const CONSTBUFFER * buffer)
{
    const float SCALE_LSB = 0.03125;
    if (buffer->size == 4)
    {
        uint16_t* temps = (uint16_t *)buffer->buffer;
        uint16_t rawAmbTemp = temps[0];
        int it = (int)((rawAmbTemp) >> 2);
        return (float)it * SCALE_LSB;
    }
    return 0;
}

float parseTemperature(const CONSTBUFFER * buffer)
{
    float result = 0;
    JSON_Value * json = json_parse_string((const char *)(buffer -> buffer));
    if (json == NULL)
    {
        LogError("Unable to parse json string");
    }
    else
    {
        JSON_Object* root = json_value_get_object(json);
        if (root == NULL)
        {
            LogError("unable to json_value_get_object");
        }
        else
        {
            result = (float)json_object_get_number(root, "temperature");
        }
        json_value_free(json);
    }
    return result;
}

void MyModule_Receive(MODULE_HANDLE module, MESSAGE_HANDLE message)
{
    if (message != NULL)
    {
        CONSTMAP_HANDLE props = Message_GetProperties(message);
        if (props != NULL)
        {
            MAP_HANDLE newMap = Map_Create(NULL);
            const char* source = ConstMap_GetValue(props, GW_SOURCE_PROPERTY);
            const char * macAddr = ConstMap_GetValue(props, GW_MAC_ADDRESS_PROPERTY);
            if (source != NULL && strcmp(source, GW_SOURCE_BLE_TELEMETRY) == 0)
            {
                Map_Add(newMap, GW_SOURCE_PROPERTY, GW_SOURCE_BLE_TELEMETRY);
                Map_Add(newMap, GW_MAC_ADDRESS_PROPERTY, macAddr);
                const char* characteristic_uuid = ConstMap_GetValue(props, GW_CHARACTERISTIC_UUID_PROPERTY);
                const CONSTBUFFER* buffer = Message_GetContent(message);
                char content[256];
                float temperature;
                if (buffer != NULL && characteristic_uuid != NULL)
                {
                    if (g_ascii_strcasecmp(temperature_uuid, characteristic_uuid) == 0)
                    {
                        // it is the temperature data
                        temperature = getTemperature(buffer);
                    }
                }
                else if (buffer != NULL) 
                {
                    temperature = parseTemperature(buffer);
                }

                sprintf_s(content, sizeof(content), "{\"deviceId\": \"Intel NUC Gateway\", \"messageId\": %d, \"temperature\": %f}", ++messageCount, temperature);


                if(Map_Add(newMap, "temperatureAlert", temperature > 30 ? "true" : "false") != MAP_OK)
                {
                    LogError("Failed to set source property");
                }
                
                MESSAGE_CONFIG newMessageConfig =
                {
                    strlen(content),
                    (const unsigned char *)content,
                    newMap
                };
                MESSAGE_HANDLE newMessage = Message_Create(&newMessageConfig);
                if (newMessage != NULL)
                {
                    printf("sending message: %s\r\n", content);
                    if(Broker_Publish(myBroker, module, newMessage) != BROKER_OK)
                    {
                        LogError("Failed to publish\r\n");                    
                    }
                    Message_Destroy(newMessage);
                }
                else
                {
                    LogError("Failed to create message\r\n");
                }
                
                Map_Destroy(newMap);
            }
        }
        else
        {
            LogError("Message_GetProperties for the message returned NULL");
        }
    }
    else
    {
        LogError("message is NULL");
    }
}

void MyModule_Start(MODULE_HANDLE module)
{
    (void)module;
    return;
}

static const MODULE_API_1 Module_GetApi_Impl =
{
    {MODULE_API_VERSION_1},

    MyModule_ParseConfigurationFromJson,
    MyModule_FreeConfiguration,
    MyModule_Create,
    MyModule_Destroy,
    MyModule_Receive,
    MyModule_Start
};

MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
{
    (void)gateway_api_version;
    return (const MODULE_API*)&Module_GetApi_Impl;
}
