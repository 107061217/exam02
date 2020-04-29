#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "mbed_events.h"
#include"math.h"
#include "algorithm"
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7
#define PI 3.1416

DigitalOut led1(LED1);
InterruptIn sw2(SW2);
EventQueue q1;
Thread t1;

I2C i2c( PTD9,PTD8);
Serial pc(USBTX, USBRX);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
void acc(float * t, int16_t acc16, uint8_t * res);
void que_acc(float * t, int16_t acc16, uint8_t * res);

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}

void acc(float * t, int16_t acc16, uint8_t * res) {
   int num = 0;
   float x_data[100];
   float y_data[100];
   float z_data[100];
   float a_y[100];
   float dis_y[100];
   float horiz[100];
   for (num=0;num<100;num++) {
      FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);
      acc16 = (res[0] << 6) | (res[1] >> 2);
      if (acc16 > UINT14_MAX/2)
         acc16 -= UINT14_MAX;
      t[0] = ((float)acc16) / 4096.0f;

      acc16 = (res[2] << 6) | (res[3] >> 2);
      if (acc16 > UINT14_MAX/2)
         acc16 -= UINT14_MAX;
      t[1] = ((float)acc16) / 4096.0f;

      acc16 = (res[4] << 6) | (res[5] >> 2);
      if (acc16 > UINT14_MAX/2)
         acc16 -= UINT14_MAX;
      t[2] = ((float)acc16) / 4096.0f;

      x_data[num] = t[0];
      y_data[num] = t[1];
      z_data[num] = t[2];
      led1 = !led1;
      wait(0.1);  
   }
   for(num=1;num<100;num++){
      a_y[num] = y_data[num] - y_data[num-1];
      dis_y[num] = 9.8*a_y[num]*0.01/2;
       //9.8*a_y*t^2/2   
   }
   for(num=1;num<100;num++){
        if(dis_y[num-1] < 0.05 ||dis_y[num-1] > -0.05){
           dis_y[num]=dis_y[num]+dis_y[num-1];
           horiz[num-1] = 0;
        }
        else if(dis_y[num-1] > 0.05 || dis_y[num-1] < -0.05) {
           horiz[num-1] = 1;
           dis_y[num-1] = 0;     
        }
   }
   for (num=0;num<100;num++) {
      pc.printf("%1.4f\r\n%1.4f\r\n%1.4f\r\n%1.4f\r\n", x_data[num], y_data[num], z_data[num],horiz[num]);
      num++;
      wait(0.05);
   }
}

void que_acc(float * t, int16_t acc16, uint8_t * res) {
   q1.call(&acc, t, acc16, res);
}

int main() {

   pc.baud(115200);

   uint8_t who_am_i, data[2], res[6];
   int16_t acc16;
   float t[3];
   led1 = 1;

   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
   data[1] |= 0x01;
   data[0] = FXOS8700Q_CTRL_REG1;
   FXOS8700CQ_writeRegs(data, 2);

   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

   t1.start(callback(&q1, &EventQueue::dispatch_forever));  
   sw2.fall(q1.event(&que_acc, t, acc16, &res[1]));  
}