/*********************OLED**********************/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Rtc_Pcf8563.h>
Rtc_Pcf8563 rtc;  //init the real time clock

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

/*********************OLED DONE****************/

/*               温湿度传感器                   */
#include <AM2321.h>
AM2321 am2321;
/*                 温湿度 done                 */

/*********************Timer********************/
#include <Timer.h>   //中断
#include <Event.h>
Timer t;
unsigned short int timeID;
unsigned short int checkID;
unsigned short int waterID;
/*********************Timer Done************/

/*********************IRrecv**********************/
#include <IRremote.h>
int RECV_PIN = 10;
IRrecv irrecv(RECV_PIN);
decode_results ir_res;
/*********************IRrecv done****************/


/*EPPROM*/
#include <avr/eeprom.h>
#define EEPROM_write(address,var)\
  eeprom_write_block((const void *)&(var),(void*)(address),sizeof(var))
#define EEPROM_read(address,var)\
  eeprom_read_block((void *)&(var),(const void *)(address),sizeof(var))
/*EPPROM DONE*/

bool flag_alarm;    //闹钟开关
bool onAlarm = 0;       //闹钟运行状态
bool flag_fan;      //电扇开关
bool flag_fanTime;  //风扇定时启动开关
bool flag_fanClose; //风扇定时关闭开关
bool flag_waterl;   //水泵开关

unsigned short int aH;  //闹钟小时
unsigned short int aM;  //闹钟分钟
unsigned short int fH;  //风扇小时
unsigned short int fM;  //风扇分钟
unsigned short int fOn; //风扇持续时间
unsigned short int fcH; //风扇关闭小时
unsigned short int fcM; //风扇关闭分钟
//unsigned short int fLevel;  //风扇风力
unsigned short int coolState = 0; //cool模式状态

char ans[] = {'A', 'A', 'B', 'C', 'D', 'C', 'A', 'C'};
uint8_t now_ques = 0;
uint8_t last_ques = 0;
#include <SoftReset.h>    //软重启必须头文件

/*蓝牙通信*/
#define my_Serial Serial1 //定义串口通讯为串口1
String msg = ""; //定义一个字符串



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  my_Serial.begin(9600);
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D/0x3C (for the 128x64)
  // init done
  //display.display();
  //delay(2000);
  //display.clearDisplay();

  // draw a single pixel
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);

  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();

  //settime(16, 7, 6, 3, 10, 26, 10);//年，月，日，星期，时，分，秒  //已下载，请勿取消注释

  irrecv.enableIRIn();

  EEPROM_read(0, flag_alarm);
  //EEPROM_read(1,flag_fan);
  flag_fan = 0;
  //EEPROM_read(2,flag_fanTime);
  flag_fanTime = 0;
  //EEPROM_read(3,flag_fanClose);
  flag_fanClose = 0;
  EEPROM_read(4, aH);
  EEPROM_read(6, aM);
  EEPROM_read(8, fH);
  EEPROM_read(10, fM);
  EEPROM_read(12, fOn);
  //EEPROM_read(14, fLevel);

  timeID = t.every(500, timeshow);
  checkID = t.every(1000, checkState);
  waterID = t.every(1000, cool);
  //pinMode(sensorPin_1, INPUT);
  //pinMode(sensorPin_2, INPUT);

  //delay(2000);
  //answer();
  //  while(1){
  //    //每收到一次信号，向通信另一端反馈一次
  //    if (Serial.available())//监视到串口监视器的数据
  //      my_Serial.println(Serial.read());//将数据写入BT
  //    if (my_Serial.available() > 0)  //如果串口有数据输入
  //    {
  //      msg = my_Serial.readStringUntil('\n'); //获取换行符前所有的内容
  //      Serial.println(msg);                   //在串口监视器显示收到的msg中的字符串
  //    }
  //  }
}

void loop() {
  t.update();
  //每收到一次信号，向通信另一端反馈一次
  if (Serial.available())//监视到串口监视器的数据
    my_Serial.write(Serial.read());//将数据写入BT
  if (my_Serial.available())//监视到BT串口的数据
    Serial.write(my_Serial.read());//将数据在串口监视器打印出来
  if (irrecv.decode(&ir_res)) {
    switch (ir_res.value) {
      case 33456255:    //A 启动闹钟
        flag_alarm = !flag_alarm;
        EEPROM_write(0, flag_alarm);
        break;
      case 33431775:    //D 设定闹钟
        setAlert();
        break;
      case 33439935:    //B cool模式
        switch(coolState){
          case 0:
            coolState = 1;
            break;
          case 1:
            coolState = 2;
            my_Serial.print("C");
            break;
          default:
            coolState = 0;
            my_Serial.print("c");
            break;
        }
        break;
      case 33468495:    //Mute 风扇定时关闭
        setFanClose();
        break;
      case 33448095:    //E 风扇定时启动
        setFanStart();
        break;
      case 33441975:    //On 启动风扇
        flag_fan = !flag_fan;
        EEPROM_write(1, flag_fan);
        (flag_fan == 1) ? my_Serial.println("F") : my_Serial.println("f");
//        if (fLevel == 0) {
//          fLevel = 50;
//          EEPROM_write(14, fLevel);
//        }
//        my_Serial.println(fLevel);
        break;
      case 33444015:    //Reset reset the system time
        settime();
        break;
      case 33427695:  //OK test
        my_Serial.println("a");
        onAlarm = 0;
        break;
//      case 33464415:  //up 调整风扇风力
//        if (flag_fan == true && fLevel < 100) {
//          fLevel++;
//          EEPROM_write(14, fLevel);
//          my_Serial.println(fLevel);
//        }
//        break;
//      case 33478695:  //down 调整风扇风力
//        if (flag_fan == true && fLevel > 0) {
//          fLevel--;
//          EEPROM_write(14, fLevel);
//          my_Serial.println(fLevel);
//          if (fLevel == 0) {
//            flag_fan = 0;
//            EEPROM_write(1, flag_fan);
//            my_Serial.println("f");
//          }
//        }
//        break;
      default:
        break;
    }
    irrecv.resume(); // Receive the next value
  }

}

void settime() {
  unsigned short int otime[5];
  otime[0] = rtc.getYear();
  otime[1] = rtc.getMonth();
  otime[2] = rtc.getDay();
  otime[3] = rtc.getHour();
  otime[4] = rtc.getMinute();
  //otime[5] = rtc.getSecond();

  char state[]={'Y','M','D','H','i'};
  unsigned short int i = 0;
  t.stop(timeID);
  bool setPoint = 0;  //0设置小时，1设置分钟
  while (i < 5) {
    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setCursor(0,0);
  
    display.setTextSize(1);
    display.print("Set System Time:  ");
    display.println(state[i]);
    
    display.setTextSize(2);

    display.print(" ");
    display.print(2000 + otime[0]);
    display.print("-");
    display.print(otime[1]);
    display.print("-");
    display.print(otime[2]);

    display.print("   ");
    display.print(otime[3]);
    display.print(":");
    display.print(otime[4]);

    display.display();
    if (irrecv.decode(&ir_res)) {
      switch (ir_res.value) {
        case 33464415:
          otime[i]++;
          break;
        case 33478695:
          otime[i]--;
          break;
        case 33480735:
          if (i != 0) i--;
          break;
        case 33460335:
          i++;
          break;
        case 33427695:
          i = 7;
          break;
        case 33441975:    //On 退出
          timeID = t.every(500, timeshow);
          irrecv.resume();
          return;
        default:
          break;
      }
      irrecv.resume();
    }
  }
  //clear out the registers
  rtc.initClock();
  //set a time to start with.
  //day, weekday, month, century(1=1900, 0=2000), year(0-99)
  rtc.setDate(otime[2], 1, otime[1], 0, otime[0]);
  //hr, min, sec
  rtc.setTime(otime[3], otime[4], 0);

  display.clearDisplay();
  display.println("success!please wait for rebooting");
  display.display();
  delay(2000);

  soft_restart();
}

void timeshow() {     //显示时间
  am2321.read();      //获取温湿度

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5,0);
  
        //日期时间
  //display.print(" ");
  display.print(rtc.formatDate(2));
  display.print(" ");
  display.println(rtc.formatTime());
  //display.print(state1);
  //display.print("   ");
  //display.print(state2);

          //温度湿度
  display.print(am2321.temperature / 10.0, 1);
  display.print("C");
  display.print(" H");
  display.print(am2321.humidity / 10);
  display.print("%");
  //display.setTextSize(1);

  if (flag_fan) {
    switch (coolState) {
      case 1: display.print("A"); break;
      case 2: display.print("K"); break;
      default: display.print(" "); break;
    }
    display.print("F");
    //display.print(fLevel);
  } else {
    display.print("   ");
  }

  if (flag_alarm == 1) {
    display.print(" A");
  } else {
    display.print(" ");
  }

  if (flag_fanTime) {
    display.print(" T");
  } else {
    display.print(" ");
  }

  if (flag_fanClose) {
    display.print(" ");
    display.println(fOn);
  }


  display.display();
}

void setAlert() {   //设置闹钟时间
  t.stop(timeID);
  bool setPoint = 0;  //0设置小时，1设置分钟
  while (1) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Set Alarm:  ");
    
    display.setTextSize(3);

    display.print("  ");
    display.print(aH);
    display.print(":");
    display.println(aM);

    if (!setPoint) {
      display.drawLine(10, 40, 40, 40, WHITE);
      display.display();
      if (irrecv.decode(&ir_res)) {
        switch (ir_res.value) {
          case 33464415:
            if (aH != 23)  aH++;
            break;
          case 33478695:
            if (aH != 00)  aH--;
            break;
          case 33427695:
            setPoint = 1;
            break;
          case 33441975:    //On 退出
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          default:
            break;
        }
        irrecv.resume();
      }
    } else {
      display.drawLine(60, 40, 100, 40, WHITE);
      display.display();
      if (irrecv.decode(&ir_res)) {
        switch (ir_res.value) {
          case 33464415:
            if (aM != 59)  aM++;
            break;
          case 33478695:
            if (aM != 00)  aM--;
            break;
          case 33427695:
            EEPROM_write(4, aH);
            EEPROM_write(6, aM);
            flag_alarm = 1;
            EEPROM_write(0, flag_alarm);
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          case 33441975:    //On 退出
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          default:
            break;
        }
        irrecv.resume();
      }
    }
  }
}

void setFanStart() {
  if (flag_fanTime == 1) {
    flag_fanTime = 0 ;
    EEPROM_write(2, flag_fanTime);
    return;
  }


  t.stop(timeID);
  bool setPoint = 0;  //0设置小时，1设置分钟

  while (1) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Set Fan Start Time:");
    
    display.setTextSize(3);

    display.print("  ");
    display.print(fH);
    display.print(":");
    display.println(fM);

    if (!setPoint) {
      display.drawLine(10, 40, 40, 40, WHITE);
      display.display();
      if (irrecv.decode(&ir_res)) {
        switch (ir_res.value) {
          case 33464415:
            if (fH != 23)  fH++;
            break;
          case 33478695:
            if (fH != 00)  fH--;
            break;
          case 33427695:
            setPoint = 1;
            break;
          case 33441975:    //On 退出
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          default:
            break;
        }
        irrecv.resume();
      }
    } else {
      display.drawLine(60, 40, 100, 40, WHITE);
      display.display();
      if (irrecv.decode(&ir_res)) {
        switch (ir_res.value) {
          case 33464415:
            if (fM != 59)  fM++;
            break;
          case 33478695:
            if (fM != 00)  fM--;
            break;
          case 33427695:
            EEPROM_write(8, fH);
            EEPROM_write(10, fM);
            flag_fanTime = 1;
            EEPROM_write(2, flag_fanTime);
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          case 33441975:    //On 退出
            timeID = t.every(500, timeshow);
            irrecv.resume();
            return;
          default:
            break;
        }
        irrecv.resume();
      }
    }
  }
}

void setFanClose() {
  if (flag_fan == 0) {  //风扇启动时设置才有效
    return ;
  }
  if (flag_fanClose == 1) {
    flag_fanClose = 0;
    return ;
  }

  t.stop(timeID);
  while (1) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);

    display.println("Set Fan Keeping Time:");

    display.setTextSize(2);
    display.print(fOn);
    display.println(" Hours");

    display.display();
    if (irrecv.decode(&ir_res)) {
      switch (ir_res.value) {
        case 33464415:   //up
          if (fOn != 6)  fOn++;
          break;
        case 33478695:   //down
          if (fOn != 01)  fOn--;
          break;
        case 33427695:   //ok
          EEPROM_write(12, fOn);
          flag_fanClose = 1;
          fcH = rtc.getHour() + fOn;
          fcM = rtc.getMinute();
          timeID = t.every(500, timeshow);
          irrecv.resume();
          return;
        case 33441975:    //On 退出
          timeID = t.every(500, timeshow);
          irrecv.resume();
          return;
      }
      irrecv.resume();
    }
  }
}

void cool() {   //根据环境喷水模式
  if(coolState == 1){
    am2321.read();
    if((am2321.temperature / 10.0) > 30 && (am2321.humidity / 10) < 40){
      my_Serial.print("C");
    }else{
      my_Serial.print("c");
    }
  }
}

void checkState() {   //检查时间，发送命令
  unsigned short int now_Hour, now_Min;
  now_Hour = rtc.getHour();
  now_Min = rtc.getMinute();

  if (flag_alarm == 1  && (now_Hour == aH && now_Min == aM && rtc.getSecond() == 0)) { //闹钟
    my_Serial.println("A");
    onAlarm = 1;
    answer();
    delay(2000);
    checkID = t.every(1000, checkState);
  }

  if (flag_fanTime == 1  && (now_Hour == fH && now_Min == fM && rtc.getSecond() == 0)) { //风扇定时启动
    my_Serial.println("F");
    //my_Serial.println(fLevel);
    flag_fan = 1;
    flag_fanTime = 0;
    EEPROM_write(2, flag_fanTime);
  }

  if (flag_fanClose == 1  && (now_Hour == fcH && now_Min == fcM && rtc.getSecond() == 0)) { //风扇定时关闭
    my_Serial.println("f");
    flag_fan = 0;
    flag_fanClose = 0;
    EEPROM_write(3, flag_fanClose);
  }

}

void answer() {
  t.stop(timeID);
  t.stop(checkID);

  while (now_ques < 5) {
    display.clearDisplay();

    while (my_Serial.read() >= 0) {}  //清空串口值，很重要！真的很重要！！！

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);

    int no = rand() % 8;

    if (last_ques == no) {    //相同题目跳过
      continue;
    }

    last_ques = no;
    my_Serial.println(no);

    while (my_Serial.available() <= 0) {
    }

    msg = my_Serial.readStringUntil('\n');

    display.print(msg);

    Serial.println(msg);
    Serial.println(no);

    display.display();

    irrecv.resume();
    while (!irrecv.decode(&ir_res)) {
    }
    switch (ir_res.value) {
      case 33456255:   //A
        (ans[no] == 'A') ? now_ques++ : now_ques = 0;
        break;
      case 33439935:   //B
        (ans[no] == 'B') ? now_ques++ : now_ques = 0;
        break;
      case 33472575:   //C
        (ans[no] == 'C') ? now_ques++ : now_ques = 0;
        break;
      case 33431775:  //D
        (ans[no] == 'D') ? now_ques++ : now_ques = 0;
        break;
    }
    irrecv.resume();

    if (now_ques == 5) {
      my_Serial.println('a');
      now_ques = 0;
      onAlarm = 0;
      timeID = t.every(500, timeshow);
      last_ques=9;
      return ;
    }
    delay(2000);
  }
}


