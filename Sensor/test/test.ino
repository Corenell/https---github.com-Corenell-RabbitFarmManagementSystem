void setup(){
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  digitalWrite(7,LOW);
}

void loop(){
  digitalWrite(6,HIGH);//

  for(long i=1;i<=110000;i++){//3200为发射脉冲数，对应电机旋转角度，最大32768
    digitalWrite(5,HIGH);
    delayMicroseconds(26);//脉冲周期为2*100ms，对应转速，不能太大，不然扭矩小会丢步
    digitalWrite(5,LOW);
    delayMicroseconds(26);
  }

  delay(1000);
  digitalWrite(6,LOW);//反转
  for(long i=1;i<=110000;i++){
    digitalWrite(5,HIGH);
    delayMicroseconds(26);
    digitalWrite(5,LOW);
    delayMicroseconds(26);
  }
  delay(1000);
}

