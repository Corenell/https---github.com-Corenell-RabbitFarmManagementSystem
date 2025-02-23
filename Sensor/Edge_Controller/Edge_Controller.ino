// 定义LED和按钮的引脚
const int ledPins[] = {2, 3, 4, 5};        // LED连接的GPIO引脚
const int buttonPins[] = {6, 7, 8, 9};     // 按钮连接的GPIO引脚

// 状态控制变量
int state = 0;  //云端接收值：不执行
int buttonPressed[4] = {0}; // 记录按钮按压状态
int prevButtonStates[4] = {LOW}; // 按钮前一次状态记录，滤除抖动/长按
int response = 0;  //返回云端值：未执行完成

/*
void MQTT(){
  //需要时，让state = 1
}
*/

void illume(){
  //云端指令为1时
  if (state == 1) {
    // 点亮所有LED
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], HIGH);
    }
    
    // 重置所有按钮状态
    memset(buttonPressed, 0, sizeof(buttonPressed));
    state = 2;  // 清除指令标记
  }
}

void bottom(){
  if(state == 2){
    
    int allPressed = 1; // 假设按钮全部按下

    for (int i = 0; i < 4; i++) {
    int currentState = digitalRead(buttonPins[i]);  //读取currentState

    if (currentState == HIGH && prevButtonStates[i] == LOW) {  //检测到按下动作 且 上一次按钮状态为未按下
      if (buttonPressed[i] == 0) {  //如果 按钮是否按下 为否
        digitalWrite(ledPins[i], LOW);  // 熄灭对应LED
        buttonPressed[i] = 1;        // 标记按钮已按下
      }
    }
    prevButtonStates[i] = currentState; // 保存当前状态
    
    // 检查是否全部按钮已操作
    if (buttonPressed[i] == 0) {  //如果 按钮是否按下 为否
      allPressed = 0;  //则按钮未全部按下
    }
  }

  
  if (allPressed) {  // 全部按钮操按下后
   state = 0;  //state置0，等待下一次云端任务
   response = 1;  //返回云端值：执行完成
  }
  }
}

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
  illume();
  bottom();
  /*
  if(response == 1){
    //MQTT发送
  }
  */
  delay(20); // 消抖延时兼功耗控制
}