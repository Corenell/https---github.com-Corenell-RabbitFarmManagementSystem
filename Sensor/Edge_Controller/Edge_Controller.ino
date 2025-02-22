// 定义LED和按钮的引脚
const int ledPins[] = {2, 3, 4, 5};        // LED连接的GPIO引脚
const int buttonPins[] = {6, 7, 8, 9};     // 按钮连接的GPIO引脚

// 状态控制变量
bool cloudCommand = false;  //不执行
bool buttonPressed[4] = {false}; // 记录按钮按压状态
int prevButtonStates[4] = {LOW}; // 按钮前一次状态记录，滤除抖动/长按

// 云端返回参数（只需要定义变量）
bool cloudResponse = false;

void setup() {
  // 初始化LED引脚为输出模式
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);  // 初始状态熄灭
  }
  
  // 初始化按钮引脚为输入模式
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT);  // 按下高电平，释放低电平
  }
}

void loop() {
  //从IOT接收cloudCommand（只发送一次）


  //云端指令为true时
  if (cloudCommand) {
    // 点亮所有LED
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], HIGH);
    }
    
    // 重置所有按钮状态
    memset(buttonPressed, 0, sizeof(buttonPressed));
    cloudCommand = false;  // 清除指令标记
  }

  // 按钮状态检测
  bool allPressed = true;
  for (int i = 0; i < 4; i++) {
    int currentState = digitalRead(buttonPins[i]);

    // 检测按下动作
    if (currentState == HIGH && prevButtonStates[i] == LOW) {
      if (!buttonPressed[i]) {
        digitalWrite(ledPins[i], LOW);  // 熄灭对应LED
        buttonPressed[i] = true;        // 标记按钮已操作
      }
    }
    prevButtonStates[i] = currentState; // 保存当前状态
    
    // 检查是否全部按钮已操作
    if (!buttonPressed[i]) {
      allPressed = false;
    }
  }

  // 全部按钮操作完成后执行
  if (allPressed) {
    cloudResponse = true;  // 返回参数：已全部执行
    //发送到IOT平台


    // 重置系统状态（开始新循环）
    cloudCommand = false;   // 重置
    cloudResponse = false; // 重置返回参数
  }

  delay(20); // 消抖延时兼功耗控制
}