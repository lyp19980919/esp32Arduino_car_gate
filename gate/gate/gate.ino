#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t carMac[] = {0x38, 0x3E, 0x51, 0x6F, 0xBD, 0xF8};

typedef struct struct_message {
    char cmd;
    uint8_t speed;
} struct_message;

struct_message outgoingCmd;

// 当前指令状态
char currentCmd = 'S';
uint8_t currentSpeed = 50;

// 发送间隔
#define SEND_INTERVAL_MS 100
unsigned long lastSendTime = 0;

void OnDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    // 可选调试
}

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW 初始化失败");
        return;
    }
    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, carMac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("添加 peer 失败");
        return;
    }

    Serial.println("网关就绪");
    Serial.println("输入 F/B/L/R/S 切换指令，输入 F150 可同时设速度");
}

void loop() {
    // ====== 读取串口输入，更新当前指令 ======
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() >= 1) {
            char c = input.charAt(0);
            if (c >= 'a' && c <= 'z') c -= 32;

            if (c == 'F' || c == 'B' || c == 'L' || c == 'R' || c == 'S') {
                currentCmd = c;

                if (input.length() > 1) {
                    int spd = input.substring(1).toInt();
                    currentSpeed = constrain(spd, 0, 255);
                }

                Serial.printf("当前指令: %c, 速度: %d\n", currentCmd, currentSpeed);
            }
        }
    }

    // ====== 周期性发送当前指令 ======
    if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = millis();
        outgoingCmd.cmd = currentCmd;
        outgoingCmd.speed = currentSpeed;
        esp_now_send(carMac, (uint8_t *)&outgoingCmd, sizeof(outgoingCmd));
    }
}