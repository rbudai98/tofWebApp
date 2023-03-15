# tofWebApp
tof_webserver

## Architecture 
* Arm based low memory and processing footprint webserver: httpd:2.4-alpine
* JavaScript protobuf library https://github.com/protobufjs/protobuf.js/

To Do:
- [x] Add webserver for SD card
- [ ] Initial webpage with websocket connection
- [ ] Setup and link protobuf to webserver
- [ ] Basic commands between tof_webserver and java-script webserver
- [ ] Image transfer and blob handle for visualization


## Setup NXP
Installing neccessary packages/sources for ubuntu
```console
bash setup.bash
```

## Run webserver
```console
bash run.sh
```

Website is available on ```https://10.42.0.1:8080```