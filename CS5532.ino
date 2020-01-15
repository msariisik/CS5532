#include <SPI.h>
#include "CS5532_lib.h"		// cs5532 definitions.

#define CS5532_CS_PIN 4
#define CS5532_READY 108

void SPI_Writing_Word (unsigned int data)
{
  SPI.transfer(CS5532_CS_PIN,(data >> 8) & 0xFF, SPI_CONTINUE);
  SPI.transfer(CS5532_CS_PIN,data & 0xFF);
}

void SPI_Writing_Long (unsigned long data)
{
  SPI.transfer(CS5532_CS_PIN,(data >> 24) & 0xFF, SPI_CONTINUE);
  SPI.transfer(CS5532_CS_PIN,(data >> 16) & 0xFF, SPI_CONTINUE);
  SPI.transfer(CS5532_CS_PIN,(data >> 8) & 0xFF, SPI_CONTINUE);
  SPI.transfer(CS5532_CS_PIN,data & 0xFF);
}

unsigned long SPI_Reading_Long()
{
  unsigned long ret = 0;
  ret = SPI.transfer(CS5532_CS_PIN,0, SPI_CONTINUE);
  ret = (ret << 8) + SPI.transfer(CS5532_CS_PIN,0, SPI_CONTINUE);
  ret = (ret << 8) + SPI.transfer(CS5532_CS_PIN,0, SPI_CONTINUE);
  ret = (ret << 8) + SPI.transfer(CS5532_CS_PIN,0);
  return ret;
}

unsigned long CS5532_Init(void)   //after that the user should be reset
{
  char i = 0;
  //reset precedure
  for (i=0; i<15; i++)
    SPI.transfer(CS5532_CS_PIN, CS5532_SYNC1, SPI_CONTINUE);
  SPI.transfer(CS5532_CS_PIN, CS5532_SYNC0);
  delay(50);
}

unsigned long CS5532_Reset(void)
{
  SPI.transfer(CS5532_CS_PIN,CS5532_CMD_WRITE_CONF, SPI_CONTINUE);
  SPI_Writing_Long(0x20000000);

//
//  SPI.transfer(CS5532_CS_PIN,CS5532_CMD_WRITE_CONF);
//  SPI_Writing_Long(0x00000000);
}

unsigned long CS5532_Reset_Done(void)
{
  unsigned long RV;
  unsigned char RV_bit;

  SPI.transfer(CS5532_CS_PIN,CS5532_CMD_READ_CONF, SPI_CONTINUE);
  RV = SPI_Reading_Long();
  
  Serial.println(RV, BIN);
  
  RV_bit = RV && 0x10000000;

  Serial.println(RV_bit, BIN);

  if (RV_bit == 1)
  {
  Serial.println("Reset is Done");
  SPI.transfer(CS5532_CS_PIN,CS5532_CMD_WRITE_CONF, SPI_CONTINUE);
  SPI_Writing_Long(0x00000000);
  }
}
void CS5532_VREF(void)
{
  unsigned long vref;
  unsigned char vref_bit;
  
  SPI.transfer(CS5532_CS_PIN, CS5532_CMD_WRITE_CONF, SPI_CONTINUE);
  SPI_Writing_Long(0x00000000);
  
  SPI.transfer(CS5532_CS_PIN, CS5532_CMD_READ_CONF, SPI_CONTINUE);
  vref = SPI_Reading_Long();
  Serial.print("config setup: ");
  Serial.println(vref,BIN);
   
  if(vref == 0x00000000)
  vref_bit = 1;
  
  Serial.print("VREF must be 1 = ");
  Serial.println(vref_bit,BIN);
}


void CS5532_Config(void)
{
  unsigned long bip1;
  unsigned long bip2;
  unsigned long offs;
  unsigned long gain;

    SPI.transfer(CS5532_CS_PIN, CS5532_CMD_READ_INDV_CH_SETUP1, SPI_CONTINUE);
    bip1 = SPI_Reading_Long();

  Serial.print("INDV_CH_SETUP1: ");
  Serial.println(bip1,BIN);

    SPI.transfer(CS5532_CS_PIN, CS5532_CMD_WRITE_INDV_CH_SETUP1, SPI_CONTINUE);
    SPI_Writing_Long(0x3200000);

    SPI.transfer(CS5532_CS_PIN, CS5532_CMD_READ_INDV_CH_SETUP1, SPI_CONTINUE);
    bip1 = SPI_Reading_Long();
  
  Serial.print("INDV_CH_SETUP1_new: ");
  Serial.println(bip1,HEX);
}

void CS5532_Single_Conversion(void)
{
 // CS5532_CS_LOW;
 // SPI.transfer(CS5532_CS_PIN,CS5532_CMD_WRITE_CONF);
 // SPI_Writing_Long(0x00000000);
 // CS5532_CS_HIGH;
  SPI.transfer(CS5532_CS_PIN, CS5532_CMD_SINGLE_CONV_CH_SETUP1);
}

void CS5532_Continuous_Conversion(void)
{
  SPI.transfer(CS5532_CS_PIN,CS5532_CMD_CONTINUOUS_CONV_CH_SETUP1);
}

char CS5532_ReadADC(unsigned char *DataBuff)
{
  unsigned char i;
  unsigned char *p_DataBuff;
  p_DataBuff = DataBuff;

//  digitalRead(CS5532_READY);
  if(CS5532_READY == 1)
  {
   Serial.println("MISO is 1");
    return(0XFF);
  }

  //  CS5532_CS_LOW;
  SPI.transfer(CS5532_CS_PIN,CS5532_NULL, SPI_CONTINUE); //The first 8 SCLKs are used to clear the SDO flag

  for(i = 4; i > 0; i--)
    *p_DataBuff++ = SPI.transfer(CS5532_CS_PIN,0, SPI_CONTINUE);
//  CS5532_CS_HIGH;

  return(0x0);
}

void setup()
{
  Serial.begin(9600);
  SPI.begin(CS5532_CS_PIN);
  digitalWrite(CS5532_CS_PIN, OUTPUT);
  SPI.setDataMode(CS5532_CS_PIN,SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  //pinMode (CS5532_CS_PIN, OUTPUT);
  //SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setClockDivider(CS5532_CS_PIN,21);       // arduino DUE
  Serial.println("sending...");
  delay(2000);
   CS5532_Init();
  Serial.println("Init is Done");
   CS5532_Reset();
   delay(10);
   CS5532_Reset_Done();
   delay(10);
   CS5532_VREF();
   delay(10);
   CS5532_Config();
   delay(10);
  Serial.println("All is Done");
   CS5532_Continuous_Conversion();
   delay(10);
  Serial.println("conversion is sent");
}

void loop()
{
  unsigned char tempbuff[4] = {0x0, 0x0, 0x0 ,0x0};
  float data;
  unsigned char OF;
  unsigned long res = 0;

  CS5532_ReadADC(tempbuff);

//    Serial.print("rawdata:");
//    Serial.print("TOP:");
//    Serial.print(tempbuff[0],BIN);
//    Serial.print("   MSB:");
//    Serial.print(tempbuff[1],BIN);
//    Serial.print("   MDL:");
//    Serial.print(tempbuff[2],BIN);
//    Serial.print("   LSB:");
//    Serial.println(tempbuff[3],BIN);
//    delay(1000);

  OF = tempbuff[3] & 0x04;

  if (OF == 0x4)
  {
    Serial.println("error");
    delay(1000);
  }
  else
  {
    res = tempbuff[0];
    res = (res << 8) + tempbuff [1];
    res = (res << 8) + tempbuff [2];
    res = 0xFFFFFF - res;
    res = res + 1;
    Serial.print("CH0:  ");
    Serial.println(res,DEC);
//    Serial.print("rawdata:");
//    Serial.print(tempbuff[0],BIN);
//    Serial.print("  ");
//    Serial.print(tempbuff[1],BIN);
//    Serial.print("  ");
//    Serial.println(tempbuff[2],BIN);
    delay(1000);
  }
}















//End
