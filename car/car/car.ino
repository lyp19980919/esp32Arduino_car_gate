#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ====== TB6612 引脚定义（STBY 接 +5V，代码不控制） ======
#define PWMA  4                  
#define AIN1  5                 // 右轮方向 1
#define AIN2  13                // 右轮方向 2
#define BIN1  16                // 左轮方向 1
#define BIN2  17                // 左轮方向 2
#define PWMB  15

// ====== PWM 配置 ======
#define PWM_FREQ       1000
#define PWM_RESOLUTION 8

// ====== 通信超时时间（毫秒） ======
#define CMD_TIMEOUT_MS 500

// ====== 接收数据结构 ======
typedef struct struct_message {
    char cmd;
    uint8_t speed;
} struct_message;

struct_message incomingCmd;

// ====== 超时保护状态 ======
volatile unsigned long lastCmdTime = 0;
volatile bool motorRunning = false;

// ====== 读取 MAC ======
void readMacAddress() {
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK) {
        Serial.printf("小车 MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      baseMac[0], baseMac[1], baseMac[2],
                      baseMac[3], baseMac[4], baseMac[5]);
    } else {
        Serial.println("读取 MAC 失败");
    }
}

// ====== 电机控制 ======
void setMotor(int leftSpeed, int rightSpeed) {
    // ===== 左轮：B 通道 =====
    if (leftSpeed > 0) {
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    } else if (leftSpeed < 0) {
        digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
    } else {
        digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
    }
    ledcWrite(PWMB, abs(leftSpeed));

    // ===== 右轮：A 通道 =====
    if (rightSpeed > 0) {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    } else if (rightSpeed < 0) {
        digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
    } else {
        digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    }
    ledcWrite(PWMA, abs(rightSpeed));
}

// ====== 执行指令 ======
void executeCmd(char c, uint8_t speed) {
    int baseSpeed = speed;
    int left = 0, right = 0;

    switch (c) {
        case 'F': left =  baseSpeed; right =  baseSpeed; break;
        case 'B': left = -baseSpeed; right = -baseSpeed; break;
        case 'L': left = -baseSpeed; right =  baseSpeed; break;
        case 'R': left =  baseSpeed; right = -baseSpeed; break;
        case 'S': left =  0;         right =  0;         break;
        default: return;
    }

    setMotor(left, right);
    motorRunning = (c != 'S');
}

// ====== ESP-NOW 接收回调 ======
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if (len != sizeof(incomingCmd)) return;
    memcpy(&incomingCmd, incomingData, sizeof(incomingCmd));

    lastCmdTime = millis();
    executeCmd(incomingCmd.cmd, incomingCmd.speed);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

    ledcAttach(PWMA, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PWMB, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(PWMA, 0);
    ledcWrite(PWMB, 0);

    WiFi.mode(WIFI_STA);
    WiFi.STA.begin();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    readMacAddress();

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW 初始化失败");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    lastCmdTime = millis();
    Serial.println("小车就绪，等待 ESP-NOW 指令...");
}

void loop() {
    if (motorRunning && (millis() - lastCmdTime > CMD_TIMEOUT_MS)) {
        setMotor(0, 0);
        motorRunning = false;
        Serial.println("通信超时，电机停止");
    }
}