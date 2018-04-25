#include <LedControl.h>

int DIN_PIN = 12;
int CS_PIN =  11;
int CLK_PIN = 10;

/* ================>  KOL69
 * const uint64_t IMAGES[] = {
  0x1109050303050911,
  0x22120a06060a1222,
  0x4424140c0c142444,
  0x8848281818284888,
  0x1090503030509010,
  0x2020a06060a02020,
  0x404040c0c0404040,
  0x8080808080808080,
  0x0000000000000000,
  0x0000000000000000,
  0x0000010101010000,
  0x0001020202020100,
  0x0102040404040201,
  0x0609101010100906,
  0x0c1221212121120c,
  0x1824424242422418,
  0x3048848484844830,
  0x6090080808089060,
  0xc0201010101020c0,
  0x8040202020204080,
  0x0080404040408000,
  0x0000808080800000,
  0x0000000000000000,
  0x0100000000000000,
  0x0300000000000000,
  0x0700000000000000,
  0x0f00000000000000,
  0x1f00000000000000,
  0x1f01010101010101,
  0x3e02020202020202,
  0x7c04040404040404,
  0xf808080808080808,
  0xf010101010101010,
  0xe020202020202020,
  0xc040404040404040,
  0x8080808080808080,
  0x0000000000000000,
  0x1824241c04042418,
  0x1824241c04042418,
  0x1824241c04042418,
  0x1824241c04042418,
  0x1824203824242418,
  0x1824203824242418,
  0x1824203824242418,
  0x1824203824242418
};
const int IMAGES_LEN = sizeof(IMAGES)/8;
*/
const uint64_t IMAGES[] = {
  0x081c3e7f7f772200,
  0x10387cfefeee4400,
  0xefc783010111bbff,
  0xf7e3c1808088ddff,
  0xf7e3c1808088ddff,
  0xefc783010111bbff,
  0x10387cfefeee4400,
  0x081c3e7f7f772200,
  0x081c3e7f7f772200,
  0x10387cfefeee4400,
  0xefc783010111bbff,
  0xf7e3c1808088ddff,
  0xf7e3c1808088ddff,
  0xefc783010111bbff,
  0x10387cfefeee4400,
  0x081c3e7f7f772200
};
const int IMAGES_LEN = sizeof(IMAGES)/8;


LedControl display = LedControl(DIN_PIN,CLK_PIN,CS_PIN);

void setup() {
  // put your setup code here, to run once:
  display.clearDisplay(0);
  display.shutdown(0,false);
  display.setIntensity(0,10);
}
void displayImage(uint64_t image){
  for(int i = 0; i < 8; i++){
    byte row = (image >> i*8) & 0xFF;
    for(int j = 0; j < 8; j++){
      display.setLed(0,i,j,bitRead(row,j));
    }
  }
}
int i =0;
void loop() {
  // put your main code here, to run repeatedly:
  displayImage(IMAGES[i]);
  if(++i >= IMAGES_LEN){
    i = 0;
  }
  delay(200);
 }
