
int Red1 = 4; 
int Green1 = 16; 
int Blue1 = 17;  
      
void setup()    
{   
         pinMode(Red1, OUTPUT); 
         pinMode(Green1, OUTPUT); 
         pinMode(Blue1, OUTPUT); 
}    
      
void loop()  // run over and over again  
{    
          // Basic colors:  
          color(255, 0, 0); // 红色亮
          delay(1000); // 延时一秒 
          color(0,255, 0); //绿色亮
          delay(1000); //延时一秒
          color(0, 0, 255); // 蓝色灯亮  
          delay(1000); //延时一秒
  
          // Example blended colors:  
          color(255,255,0); // 黄色  
          delay(1000); //延时一秒
          color(255,255,255); // 白色  
          delay(1000); //延时一秒
          color(128,0,255); // 紫色  
          delay(1000); //延时一秒
          color(0,0,0); // t关闭led  
          delay(1000); //延时一秒  
}     
     
void color (unsigned char red, unsigned char green, unsigned char blue)  //颜色控制函数 （这里是共阳，我们后面是共阴，这个函数后面直接用r/g/b，不用225减）
{    
          analogWrite(Red1, red);   
          analogWrite(Green1, blue); 
          analogWrite(Blue1, green); 
}