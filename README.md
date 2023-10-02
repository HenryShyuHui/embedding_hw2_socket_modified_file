# server

server_cube.py檔下載，並且將最上面的 HOST = '填入電腦的IP位置'
server_cube.py 這份檔案為socket 的server端，在接收roll, pitch, yaw 資料之後，會將其可視化

# modified file

若原本的網址無法import進去Mbed OS，可以按照下列步驟
1. import wifi_socket 的範例程式 https://github.com/ARMmbed/mbed-os-example-sockets
2. 將source/main.cpp更改為這裡的main.cpp
3. 將mbed_app.json更改為這裡的mbed_app.json
4. 在"mbed_app.json"中，修改hostname->value為自己電腦的IP位置；wifi-ssid修改為要連接網路的名稱；wifi-passward輸入該網路的密碼。
