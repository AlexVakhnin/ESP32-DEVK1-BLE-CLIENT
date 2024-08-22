#include <Arduino.h>
#include <U8g2lib.h>

extern void ble_client_init();
extern void ble_client_handle();

void disp_val(String str);

//I2C for SH1106 OLED 128x64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void state_logo();

void setup() {
  Serial.begin(115200);

  //OLED SH1106 128x64
  u8g2.begin();
  state_logo();
  delay(2000);

  ble_client_init();
}

void loop() {

  ble_client_handle();

  delay(1000); // Delay a second between loops.


}

void state_logo(){
  u8g2.clearBuffer();					// clear display buffer
  u8g2.setFont(u8g2_font_open_iconic_embedded_4x_t);//open iconic
  u8g2.drawStr(48,64-15,"N");
  u8g2.sendBuffer();
}


//получаемую от сервера BLE строку - выдать на дисплей
void disp_val(String str){
  int n = str.length(); 
  String fstr = str.substring(0,n-5); //удалить "°C\r\n" в конце строки..
  int val = fstr.toInt(); //входящую строку в число..
  String str_value ="---";
  if (val > -40 && val < 150 && str[n-5]==194 && str[n-4]==176){  //проверяем..
    str_value = String(val);
  }
  u8g2.clearBuffer();					// clear display buffer
  u8g2.setFont(u8g2_font_logisoso62_tn);//big font
  u8g2.drawStr(2,63,str_value.c_str());
  u8g2.sendBuffer();
}
