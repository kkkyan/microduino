#define music_num_MAX 8        //歌曲最大数，可改

#include <SoftwareSerial.h>
//用户自定义部分------------------------

#include <Wire.h>

//EEPROM---------------------
#include <EEPROM.h>
#define EEPROM_write(address, p) {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) EEPROM.write(address+i, pp[i]);}
#define EEPROM_read(address, p)  {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) pp[i]=EEPROM.read(address+i);}

struct config_type
{
  int EEPROM_music_num;       //歌曲的数目
  int EEPROM_music_vol;       //歌曲的音量
};

//用户自定义部分------------------------
#include "audio.h"   //"arduino.h"是控制音频文件

#include "U8glib.h"
//-------字体设置，大、中、小
#define setFont_L u8g.setFont(u8g_font_7x13)
#define setFont_M u8g.setFont(u8g_font_fixed_v0r)
#define setFont_S u8g.setFont(u8g_font_fixed_v0r)
/*
font:
 u8g_font_7x13
 u8g_font_fixed_v0r
 u8g_font_chikitar
 u8g_font_osb21
 u8g_font_courB14r
 u8g_font_courB24n
 u8g_font_9x18Br
 */

//屏幕类型--------
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

#define init_draw 500	//主界面刷新时间
unsigned long timer_draw;

int MENU_FONT = 1;	//初始化字体大小 0：小，1：中，2：大

boolean music_status = false;	//歌曲播放状态
int music_num = 1;		//歌曲序号
int music_vol=20;               //音量0~30


boolean key,key_cache;          //定义mp3的开关

unsigned long music_vol_time_cache=0;     
unsigned long music_vol_time=0;           
boolean music_vol_sta=false;              

int uiStep()         //切歌
{
  //Serial.println(analogRead(A6));
  if(analogRead(A6)<10)     //如果引脚A6的模拟量电压值小于10
  {
    delay(50);       //延迟50毫秒输出
    if(analogRead(A6)<10)         //如果引脚A6的模拟量电压值小于10
      return 1;      //回到动作1
  }
  if(analogRead(A6)>100 && analogRead(A6)<180)       //如果引脚A6模拟量电压值在150到400之间
  {
    delay(50);       //延迟50毫秒输出
    if(analogRead(A6)>100 && analogRead(A6)<180)     //如果引脚A6模拟量电压值在150到400之间
      return 2;      //回到动作2
  }
  if(analogRead(A6)>180 && analogRead(A6)<250)       //如果引脚A6模拟量电压值在450到600之间
  {
    delay(50);       //延迟50毫秒输出
    if(analogRead(A6)>180 && analogRead(A6)<250)     //如果引脚A6模拟量电压值在450到600之间
      return 3;      //回到动作3 
  }
  return 0;
}

void setup()        //创建无返回值函数
{
  Serial.begin(9600);    //初始化串口通信，并将波特率设置为9600

  //EEPROM---------------------
  eeprom_READ();

  //初始化mp3模块
  audio_init(DEVICE_TF,MODE_One_END,music_vol);		//初始化mp3模块

  // u8g.setRot180();  // rotate screen, if required
}

void loop()            //无返回值Loop函数
{ 
  int vol=uiStep();	//检测输入动作
  //  Serial.print("vol:");
  //  Serial.println(vol);

  if(vol==1) key=true;          //按一次开关,为动作1 
  else key=false;               //否则不做为动作1

  if(!key && key_cache)		//按下松开后
  {
    key_cache=key;		//缓存作判断用
    music_status=!music_status;	//播放或暂停
    if(music_status == true)	//播放
    {
      Serial.println("play");   //串口输出 “play”（工作
//      audio_choose(1);
      audio_play();              //音频工作
    }
    else	//暂停
    {
      Serial.println("pause");   //串口输出 “pause”（暂停）
      audio_pause();              //音频暂停工作
    }
  }
  else
  {
    key_cache=key;		  //缓存作判断用
  }

  if(vol==0)                      //如果不对开关操作为动作0
  {
    //    Serial.println("no");
    music_vol_time_cache=millis();
    music_vol_time=music_vol_time_cache;
    music_vol_sta=false;          //音频不工作
  }
  else if(vol==2)                 //向右拨动开关为动作2
  {
    music_vol_time=millis()-music_vol_time_cache;
    //    if(music_vol_time>200)  //拨动开关时间大于0.2秒
    delay(500);                   //延迟0.5秒
    if(uiStep()==0 && !music_vol_sta)
    {
      Serial.println("next");    //歌曲输出下一个（向右波动1次开关）

      music_num++;	//歌曲序号加
      if(music_num>music_num_MAX)	//限制歌曲序号范围，如果歌曲序号大于30
      {
        music_num=1;   //歌曲序号返回1
      }
      audio_choose(music_num);
      audio_play();
      //        delay(500);      //延迟0。5秒
      music_status=true;         //音频状态为工作
      eeprom_WRITE();

    }
    else if(music_vol_time>1500)   //拨动开关时间大于1.5秒
    {
      music_vol_sta=true;          //音量工作
      music_vol++;                 //音量+1
      if(music_vol>30) music_vol=30;      //若音量大于30，则音量为30
      audio_vol(music_vol);
      Serial.println("++");       //Serial.println函数输出“+1”
      delay(100);                  //延迟0.1秒 
      eeprom_WRITE();
    }
  }
  else if(vol==3)         //向左拨动开关为动作3
  {
    music_vol_time=millis()-music_vol_time_cache;
    //    if(music_vol_time>200)       //延迟0.2秒
    delay(500);//延迟0.5秒
    if(uiStep()==0 && !music_vol_sta)
    {
      Serial.println("perv");

      music_num--;	//歌曲序号减1
      if(music_num<1)	//限制歌曲序号范围，如果歌曲序号小于1
      {
        music_num=music_num_MAX;     //歌曲序号为最大（30）
      }
      audio_choose(music_num);       //音频选择歌曲序号
      audio_play();                  //音频工作
      //        delay(500);          //延迟0.5秒
      music_status=true;             //音频工作
      eeprom_WRITE();
    }
    else if(music_vol_time>1500)     //拨动开关时间大于1.5秒
    {
      music_vol_sta=true;            //音频工作
      music_vol--;                   //音量减1
      if(music_vol<1) music_vol=1;   //如果音量小于1，音量为1
      audio_vol(music_vol);
      Serial.println("--");         //Serial.println函数输出“-1”
      delay(100);                    //延迟0.1秒
      eeprom_WRITE();
    }
  }

  if(millis()-timer_draw>init_draw)
  {
    u8g.firstPage();  
    do {
      draw();
    } 
    while( u8g.nextPage() );
    timer_draw=millis();
  }
}

void eeprom_WRITE()
{
  config_type config;  		// 定义结构变量config，并定义config的内容
  config.EEPROM_music_num=music_num;
  config.EEPROM_music_vol=music_vol;

  EEPROM_write(0, config); 	// 变量config存储到EEPROM，地址0写入
}

void eeprom_READ()
{
  config_type config_readback;
  EEPROM_read(0, config_readback);
  music_num=config_readback.EEPROM_music_num;
  music_vol=config_readback.EEPROM_music_vol;
}

unsigned int freeRam () {
  extern unsigned int __heap_start, *__brkval; 
  unsigned int v; 
  return (unsigned int) &v - (__brkval == 0 ? (unsigned int) &__heap_start : (unsigned int) __brkval); 
}

//主界面，可自由定义
void draw()
{
  setFont_L;

  u8g.setPrintPos(4, 16);
  u8g.print("Music_sta:");
  u8g.print(music_status ? "play" : "pause");

  u8g.setPrintPos(4, 16*2);
  u8g.print("Music_vol:");
  u8g.print(music_vol);
  u8g.print("/30");
  u8g.setPrintPos(4, 16*3);
  u8g.print("Music_num:");
  u8g.print(music_num);
  u8g.print("/");
  u8g.print(music_num_MAX);
  u8g.setPrintPos(4, 16*4);
  u8g.print("....Microduino....");
}
