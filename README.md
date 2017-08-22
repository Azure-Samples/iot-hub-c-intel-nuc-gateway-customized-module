---
services: iot-hub
platforms: c
author: xshi
---

# azure iot gateway ble data convertor module

This is a native module written in C programming language, which can be loaded by the Azure IoT Gateway SDK.

The convertor module will receive the message sent from a BLE or Simulated_Device module, and then convert the data into json format as
``` json
{"deviceId": "Intel NUC Gateway", "messageId": 0, "temperature": 0.0}
```

## compile the module
clone this repo into a linux system (Intel NUC), then run the following command:

``` bash
chmod 777 build.sh  # change the build script runnable
sed -i -e "s/\r$\/\/" build.sh  # remove the invalid windows character
./build.sh
```

Note the `libmy_module.so` binary file's absolutely path.

## config the gateway's config
1. Choose the `ble_gateway.json` or `simulated_device_cloud_upload.json` according to whether you run a BLE sample or simulated_device sample
2. Add a new module as `MyModule` using the following json

    ```json
    {
      "name": "MyModule",
      "loader": {
        "name": "native",
        "entrypoint":{
          "module.path": "[Your libmy_module.so path]"
        }
      },
      "args": null
    },
    ```

3. Modify the `links` part
    a. if you are using the BLE sample, modify the `links` part

      from:

      ```json
      {
          "source": "SensorTag",
          "sink": "mapping"
      }
      ```

      to:

      ```json
      {
          "source": "SensorTag",
          "sink": "MyModule"
      },
      {
          "source": "MyModule",
          "sink": "mapping"
      }
      ```
    b. if you are using the simulated_device_cloud_upload sample, modify the `links` part

      from:

      ```json
      {
          "source": "BLE",
          "sink": "mapping"
      }
      ```

      to:

      ```json
      {
          "source": "BLE",
          "sink": "MyModule"
      },
      {
          "source": "MyModule",
          "sink": "mapping"
      }
      ```
