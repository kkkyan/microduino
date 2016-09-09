/*蓝牙通信*/
#define my_Serial Serial1 //定义串口通讯为串口1
String msg = ""; //定义一个字符串
char order = 0;  //命令
int fz = 0;     //风力

/*********************Timer********************/
#include <Timer.h>   //中断
#include <Event.h>
Timer t;
unsigned short int alarmID;
unsigned short int fanID;
unsigned short int servoID;
unsigned short int alarm_eID;
unsigned short int delayID;
/*********************Timer Done************/

/*********************Timer********************/
#include <Servo.h>
Servo myservo;
unsigned short int pos = 0;
/*********************Timer Done**************/


/*********************Player********************/
#include <SoftwareSerial.h>
//用户自定义部分------------------------

#include <Wire.h>

#include "audio.h"   //"audio.h"是控制音频文件

int music_vol=30;               //音量0~30
int fileNum = 9;
/*********************Player done************/

//buzzer
#define buzzer_pin 2

/*F大调频率*/
#define NTDL1 349
#define NTDL2 392
#define NTDL3 440
#define NTDL4 464
#define NTDL5 523
#define NTDL6 587
#define NTDL7 659

#define NTD1 698
#define NTD2 784
#define NTD3 880
#define NTD4 932
#define NTD5 1046
#define NTD6 1175
#define NTD7 1318

#define NTDH1 1396
#define NTDH2 1568
#define NTDH3 1760
#define NTDH4 1865
#define NTDH5 2092
#define NTDH6 2347
#define NTDH7 2632
/*F大调频率Done*/

/*歌曲小星星*/
int tune[] = {  //频率
  NTD1, NTD1, NTD5, NTD5, NTD6, NTD6, NTD5,
  NTD4, NTD4, NTD3, NTD3, NTD2, NTD2, NTD1,
  NTD5, NTD5, NTD4, NTD4, NTD3, NTD3, NTD2,
  NTD5, NTD5, NTD4, NTD4, NTD3, NTD3, NTD2,
  NTD1, NTD1, NTD5, NTD5, NTD6, NTD6, NTD5,
  NTD4, NTD4, NTD3, NTD3, NTD2, NTD2, NTD1
};
float durt[] = {  //节拍
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2
};
int lenth = 0; //音乐长度
int music_p = 0;

bool flag_alarm = 0;    //闹钟开关
bool flag_fan = 0;      //风扇开关
bool flag_water = 0;    //水泵开关

//D6，D8控制1A，1B的电机
#define OUT2A 5
#define OUT2B 7
int fan_pin = 8;

//Dealy
int delay_pin = 10;

//闹钟
String ques[] = {"A.quetch  B.quatch  C.quitch  D.cuetch",
                 "A.queue   B.quire   C.cue     D.cute",
                 "A.hubbit  B.habit   C.habbit  D.hubit",
                 "A.megezineB.megazineC.magazineD.magezine",
                 "A.mijorityB.mojorityC.majarityD.majority",
                 "A.hite    B.hate    C.haet    D.heat",
                 "A.hug     B.hag     C.hage    D.huge",
                 "A.azuer   B.azaur   C.azure   D.asure"
                };
void setup()
{
  //init BT
  Serial.begin(9600);
  my_Serial.begin(9600);


  //init Motor
  pinMode(fan_pin, OUTPUT);
  pinMode(delay_pin, OUTPUT);
  pinMode(OUT2A , OUTPUT);
  pinMode(OUT2B , OUTPUT);
  myservo.attach(9);   //Servo pin 9

  alarmID = t.every(1000, playMusic);
  alarm_eID = t.every(2000, english);
  //fanID = t.every(1000, onFan);
  servoID = t.every(15, onServo);
  delayID = t.every(5000, water);
  t.every(10, recMsg);

  lenth = sizeof(tune) / sizeof(tune[0]); //计算长度

  //Audio

  //初始化mp3模块
  audio_init(DEVICE_TF,MODE_One_END,music_vol);    //初始化mp3模块

  digitalWrite(OUT2A, HIGH);
  digitalWrite(OUT2B, HIGH);
}

void loop()
{
  t.update();

  if (flag_fan == 1) {  //舵机控制
    if (pos == 360) {
      pos = 0;
    } else {
      pos++;
    }
    delay(20);
  }

}

void playMusic() {    //闹钟演奏

  //  for (int x = 0; x < lenth; x++) {
  //    recMsg();
  if (flag_alarm == 0) {
    noTone(buzzer_pin);
    music_p = 0;
    return;
  }
  tone(buzzer_pin, tune[music_p]);
  delay(500 * durt[music_p]); //这里用来根据节拍调节延时，500这个指数可以自己调整，在该音乐中，我发现用500比较合适。
  noTone(buzzer_pin);

  if (music_p != lenth) {
    music_p++;
  } else {
    music_p = 0;
  }
  //  }
  //  delay(2000);
}
//
//void onFan() {  //风扇控制
//  if (flag_fan == 0) {
//    digitalWrite(fan_pin, LOW);
//  } else {
//    if (fz != 0 && flag_alarm == 0) {
//      int lv = float(fz) / 100 * 255;
//      Serial.println(lv);
//      analogWrite(fan_pin, lv);
//    }
//
//  }
//}

void onServo() {  //舵机控制
  if (flag_fan == 0) {
    myservo.detach();
    return ;
  } else {
    myservo.attach(9);
    int finalPos = 0;
    if (pos > 180) {
      finalPos = 360 - pos;
    } else {
      finalPos = pos;
    }
    Serial.println(finalPos);
    myservo.write(finalPos);              // tell servo to go to position in variable 'pos'
  }
}

void recMsg() {
  //if (Serial.available())//监视到串口监视器的数据
  //  my_Serial.write(Serial.read());//将数据写入BT

  if (my_Serial.available() > 0) {  //如果串口有数据输入
    msg = my_Serial.readStringUntil('\n'); //获取换行符前所有的内容
    Serial.println(msg);                   //在串口监视器显示收到的msg中的字符串
    fz = atoi(msg.c_str());
    //my_Serial.println(fz);          //打印当前风力
    if (fz == 0) {
      order = msg[0];
      //my_Serial.println(order);     //打印命令
      switch (order) {
        case 'A':
          flag_alarm = 1;
          break;
        case 'a':
          flag_alarm = 0;
          break;
        case 'F':
          flag_fan = 1;
          digitalWrite(fan_pin, HIGH);
          break;
        case 'f':
          flag_fan = 0;
          digitalWrite(fan_pin, LOW);
          break;
        case 'C':
          flag_water = 1;
          break;
        case 'c':
          flag_water = 0;
          break;
      }
    }
  }

}

void english() {  //单词播放函数
  if (flag_alarm == 0) {
    return;
  } else {
    
    int no = fz + 1;  //获取播放单词
    audio_choose(no);
    audio_play();
    
    if (fileNum != no) {
      while(my_Serial.read() >= 0){}    //清空串口值，很重要！真的很重要！！！
      
      my_Serial.println(ques[fz]);
      //my_Serial.flush();
      //Serial.println(ques[fz]);
      fileNum = no;
    }
  } 
}

void water(){
  if(flag_water == 0){
    digitalWrite(delay_pin, LOW);
    return;
  }else{
    digitalWrite(delay_pin,HIGH);
    delay(1000);
    digitalWrite(delay_pin,LOW);
  }
}

