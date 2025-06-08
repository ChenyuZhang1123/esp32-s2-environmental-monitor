#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// OLED 显示屏设置
#define SCREEN_WIDTH 128 // OLED 屏幕宽度
#define SCREEN_HEIGHT 64 // OLED 屏幕高度
#define OLED_RESET    -1 // OLED 复位引脚
#define SDA_PIN  39      // OLED SDA 数据线连接到 GPIO39
#define SCL_PIN  37      // OLED SCL 时钟线连接到 GPIO37
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // 创建OLED显示对象

// DS18B20 温度传感器设置
#define ONE_WIRE_BUS_PIN 33  // 18B20 DQ 数据线连接到 GPIO33
OneWire oneWire(ONE_WIRE_BUS_PIN);             // 创建OneWire对象
DallasTemperature sensors(&oneWire);           // 将OneWire对象传递给DallasTemperature库
DeviceAddress tempDeviceAddress;               // 用于存储检测到的18B20传感器地址

// 蜂鸣器设置
#define BUZZER_PIN 35       // 蜂鸣器控制引脚连接到 GPIO35

// 温度阈值
const float TEMP_THRESHOLD_HIGH = 25.0; // 高温报警阈值
const float TEMP_THRESHOLD_LOW = 10.0;  // 低温报警阈值

void setup() {
  Serial.begin(115200); // 初始化串口通信，波特率115200
  Serial.println("ESP32-S2 Environmental Monitor Booting"); 

  // 初始化OLED
  Wire.begin(SDA_PIN, SCL_PIN); // 初始化I2C总线，并指定SDA和SCL引脚

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 初始化OLED
    Serial.println(F("OLED initialization failed!")); 
    for(;;); // 初始化失败则停止执行
  }
  display.display(); // 显示初始化内容
  delay(1000);
  display.clearDisplay(); // 清空屏幕
  display.setTextSize(1);             // 设置文字大小
  display.setTextColor(SSD1306_WHITE); // 设置文字颜色为白色
  display.setCursor(0,0);             // 设置光标起始位置
  display.println("System Booting..."); 
  display.display(); // 更新OLED显示
  delay(1000);

  // 初始化18B20温度传感器
  sensors.begin(); // 启动传感器库
  if (!sensors.getAddress(tempDeviceAddress, 0)) { // 尝试获取总线上第一个传感器的地址
    Serial.println("DS18B20 sensor not found!"); 
    display.clearDisplay();
    display.setCursor(0,10);
    display.println("18B20 Error!"); 
    display.display();
  } else {
    Serial.print("DS18B20 Address: "); 
    printAddress(tempDeviceAddress); // 打印传感器地址到串口
    Serial.println();
    sensors.setResolution(tempDeviceAddress, 12); // 设置传感器精度为12位
  }

  // 初始化蜂鸣器引脚
  pinMode(BUZZER_PIN, OUTPUT);      // 将蜂鸣器引脚设置为输出模式
  digitalWrite(BUZZER_PIN, LOW);   // 确保蜂鸣器初始状态为关闭

  Serial.println("Initialization complete."); 
}

void loop() {
  sensors.requestTemperatures(); // 发送命令获取温度读数
  float temperatureC = sensors.getTempC(tempDeviceAddress); // 读取摄氏温度

  Serial.print("Current Temp: "); 
  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data"); 
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Temp Read Error!"); 
    display.display();
    beepError(); // 蜂鸣器发出错误提示音
    delay(2000); // 等待一段时间再尝试
    return;      // 结束本次loop，避免后续操作使用错误数据
  }
  Serial.print(temperatureC);
  Serial.println(" C"); // 改为英文单位

  // 在OLED上显示信息
  display.clearDisplay();
  display.setTextSize(2);             // 设置较大字体显示温度
  display.setCursor(0, 10);
  display.print(temperatureC, 1);     // 显示温度，保留一位小数
  display.print(" ");
  display.setTextSize(1);             // 切换回小字体显示单位
  display.cp437(true);                // 启用Adafruit特定字符集以显示度数符号
  display.write(167);                 // 写入度数符号 (°)
  display.setTextSize(2);
  display.print("C");

  display.setTextSize(1);             // 小字体显示阈值信息
  display.setCursor(0, 40);
  display.print("Threshold: ");       
  display.print(TEMP_THRESHOLD_HIGH, 1);
  display.write(167); // 度数符号
  display.print("C");

  display.display(); // 更新OLED显示内容

  // 温度报警逻辑
  if (temperatureC > TEMP_THRESHOLD_HIGH) {
    Serial.println("High temperature alarm!"); 
    beepAlarm(3, 200, 100); // 蜂鸣器发出3声警报
  } else {
    digitalWrite(BUZZER_PIN, LOW); // 温度正常，关闭蜂鸣器
  }

  delay(2000); // 每2秒更新一次数据
}

// 辅助函数：以十六进制打印传感器地址
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0"); // 不足两位补零
    Serial.print(deviceAddress[i], HEX);
  }
}

// 辅助函数：蜂鸣器报警声 (响count次，每次持续duration毫秒，间隔pause毫秒)
void beepAlarm(int count, int duration, int pause) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH); // 打开蜂鸣器
    delay(duration);                // 持续发声
    digitalWrite(BUZZER_PIN, LOW);  // 关闭蜂鸣器
    if (i < count - 1) {            // 如果不是最后一声，则暂停
      delay(pause);
    }
  }
}

// 辅助函数：蜂鸣器错误提示音 (长响一声)
void beepError() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500); // 持续0.5秒
  digitalWrite(BUZZER_PIN, LOW);
}