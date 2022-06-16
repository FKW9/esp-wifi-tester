#include <Wire.h>
#include "hte501.h"

// single measurement, clock strech enable, 10Bit resolution (lowest)
unsigned char Command[] = {0x2C, 0x10};

void setupSensor(void)
{
    Wire.begin(2, 0);
}

void wireWrite(unsigned char buf[], int to, bool stopmessage)
{
    Wire.beginTransmission(HTE_ADDRESS);
    for (int i = 0; i <= to; i++)
    {
        Wire.write(buf[i]);
    }
    Wire.endTransmission(stopmessage);
}

void wireRead(unsigned char buf[], int to)
{
    int i = 0;
    Wire.requestFrom(HTE_ADDRESS, to);
    while (Wire.available())
    {
        buf[i++] = Wire.read();
    }
}

unsigned char calcCrc8(unsigned char buf[], unsigned char from, unsigned char to)
{
    unsigned char crcVal = 0xFF;
    unsigned char i = 0;
    unsigned char j = 0;
    for (i = from; i <= to; i++)
    {
        int curVal = buf[i];
        for (j = 0; j < 8; j++)
        {
            if (((crcVal ^ curVal) & 0x80) != 0)
            {
                crcVal = ((crcVal << 1) ^ 0x31);
            }
            else
            {
                crcVal = (crcVal << 1);
            }
            curVal = curVal << 1;
        }
    }
    return crcVal;
}

int getTemperatureHumidity(float &temperature, float &humidity)
{
    unsigned char i2cResponse[6];
    wireWrite(Command, 1, false);
    wireRead(i2cResponse, 6);
    if (i2cResponse[2] == calcCrc8(i2cResponse, 0, 1) && i2cResponse[5] == calcCrc8(i2cResponse, 3, 4))
    {
        temperature = ((float)(i2cResponse[0]) * 256 + i2cResponse[1]) / 100;
        humidity = ((float)(i2cResponse[3]) * 256 + i2cResponse[4]) / 100;
        return 1;
    }
    else
        return 0;
}