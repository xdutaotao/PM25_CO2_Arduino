
// include the library code:
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

#include <SoftwareSerial.h>

//软串口, Rx-D8, 接传感器的Tx
SoftwareSerial altSerial(9, 8); //for PM25 G5, TX<->D9
SoftwareSerial altSerial_CO2(10, 11); //for 二氧化碳数字 TX<->D10， RX<->D11

//攀藤G5数据格式
struct _panteng
{
  unsigned char len[2];
  unsigned char pm1_0_cf1[2]; //PM1.0
  unsigned char pm2_5_cf1[2]; //PM2.5
  unsigned char pm10_0_cf1[2]; //PM10
  unsigned char pm1_0[2]; //PM1.0
  unsigned char pm2_5[2]; //PM2.5
  unsigned char pm10_0[2]; //PM10.0
  unsigned char d[20];
} panteng;



void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.init();
  lcd.backlight();
  lcd.print("Welcome!!!");
  //USB向PC发送串口数据
  Serial.begin(115200);
  altSerial.begin(9600); //软串口连传感器,读取数据

  altSerial_CO2.begin(9600); //

  lcd.clear();
}

void read_PM25()
{
  unsigned char c;
  char str[100];
  static int state = 0;
  static int count = 0;
  static int time = 0;
  int pm1_0, pm2_5, pm10_0;
  int pm1_0_cf1, pm2_5_cf1, pm10_0_cf1;
  int i;

  //必须调用一次，做切换
  altSerial.listen();
  //PM25传感器是每1s一次串口可读
  while (altSerial.available())
  {

    c = altSerial.read();

    switch (state)
    {
      case 0:
        if (c == 0x42)
          state = 1;

        break;

      case 1:
        if (c == 0x4d)
        {
          state = 2;
          count = 0;
        }

        break;

      case 2:
        ((unsigned char*)&panteng)[count++] = c;
        sprintf(str, "%02X ", c);

        if (count > 28)
        {
          state = 0;
          pm1_0 = panteng.pm1_0[0] * 256 + panteng.pm1_0[1];
          pm2_5 = panteng.pm2_5[0] * 256 + panteng.pm2_5[1];
          pm10_0 = panteng.pm10_0[0] * 256 + panteng.pm10_0[1];

          pm1_0_cf1 = panteng.pm1_0_cf1[0] * 256 + panteng.pm1_0_cf1[1];
          pm2_5_cf1 = panteng.pm2_5_cf1[0] * 256 + panteng.pm2_5_cf1[1];
          pm10_0_cf1 = panteng.pm10_0_cf1[0] * 256 + panteng.pm10_0_cf1[1];

          sprintf(str, "%d,%d,%d,%d,%d,%d,%d", time++, pm1_0_cf1, pm2_5_cf1, pm10_0_cf1, pm1_0, pm2_5, pm10_0);
          Serial.println(str);
          snprintf(str, 16, "MB:%d,%d,%d      ", pm1_0_cf1, pm2_5_cf1, pm10_0_cf1);
          //lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(str);
          snprintf(str, 16, "GB:%d,%d,%d      ", pm1_0, pm2_5, pm10_0);
          //lcd.setCursor(0, 1);
          //lcd.print(str);
          break;
        }

        break;

      default:
        break;
    }
  }
}

//用此数据发送给C02传感器来激发串口输出
unsigned char CO2_DataCMD[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
unsigned char CO2_Data[20];
int CO2_value = 0;
void read_CO2()
{
  int i = 0;
  CO2_value = 0;
  //必须调用一次，做切换
  altSerial_CO2.listen();
  for (; i < 9; i++) {
    altSerial_CO2.write(CO2_DataCMD[i]);
  }

  lcd.setCursor(0, 1);
  uint8_t check = 0;
  int idx = 0;
  int len = 0;
  int state = 0;
  //读取计数
  while (altSerial_CO2.available())
  {
    unsigned char b = altSerial_CO2.read();

    // update checksum
    check += b;
    char tmp[20];

    switch (state) {
      case 0: //START
        if (b == 0xFF) {
          check = 0;
          state = 1;
        }
        break;
      case 1: // CMD
        if (b == 0x86) { // 0x86 is DATA COMMAND
          idx = 0;
          len = 6;
          state = 2;
        } else {
          state = 0;
        }
        break;
      case 2: //DATA
        CO2_Data[idx++] = b;
        if (idx == len) {
          state = 3;
        }
        break;
      case 3: //CHECK
        {
          state = 0;
          CO2_value = CO2_Data[0] * 256 + CO2_Data[1];
          char str[16];
          snprintf(str, 16, "CO2: %d PPM   ", CO2_value);
          lcd.setCursor(0, 1);
          lcd.print(str);
        }
        return;
      default:
        state = 0;
        break;
    }
  }
}

int count = 0;
void loop()
{

  //读取PM25
  read_PM25();


  //5s读取一次
  if (count % 5 == 0) {
    //读取CO2
    count = 0;
    read_CO2();
  }
  count ++;
  delay(1000);
}
