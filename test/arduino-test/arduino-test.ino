#include <SoftwareSerial.h>
#define num_byte_from_esp 10
#define num_byte_send_2_esp 26

#define debug true

SoftwareSerial espSerial(3,2);

uint8_t data_from_esp_buff[num_byte_from_esp];
uint8_t data_from_esp_i=0;
uint32_t last=0;
uint32_t lastT=0;
bool checkSum();

void setup()
{
    Serial.begin(9600);
    espSerial.begin(9600);
    if(debug) Serial.println("setup done");
}
void loop()
{
    if(millis()-last>10000)
    {
        last=millis();
        uint8_t data2esp_buff[num_byte_send_2_esp]={66,77,4,5,6,7,8,9,20,20,30,30,40,40,40,50,50,50,60,60,60,70,70,70,0,0};
        uint16_t a=0;
        for(uint8_t i=0;i<num_byte_send_2_esp-2;i++)
        {
            a+=data2esp_buff[i];
        }
        data2esp_buff[num_byte_send_2_esp-2]=a/256;
        data2esp_buff[num_byte_send_2_esp-1]=a%256;
        if(debug) Serial.print(" - data: ");
        for(uint8_t i=0;i<num_byte_send_2_esp;i++)
        {
            espSerial.write(data2esp_buff[i]);
            if(debug) Serial.print(data2esp_buff[i]);
            if(debug) Serial.print(" ");
        }
        if(debug) Serial.println();
    }
    else if(millis()-lastT>3000)
    {
        lastT=millis();
        uint8_t timeRequest_buff[4]={66,88,0,66+88};
        if(debug) Serial.print(" - time: ");
        espSerial.write(timeRequest_buff[0]);
        if(debug) Serial.print(timeRequest_buff[0]);
        if(debug) Serial.print(" ");
        espSerial.write(timeRequest_buff[1]);
        if(debug) Serial.print(timeRequest_buff[1]);
        if(debug) Serial.print(" ");
        for(uint8_t i=0;i<num_byte_send_2_esp-3;i++)
        {
            espSerial.write(timeRequest_buff[2]);
            if(debug) Serial.print(timeRequest_buff[2]);
            if(debug) Serial.print(" ");
        }
        espSerial.write(timeRequest_buff[3]);
        if(debug) Serial.print(timeRequest_buff[3]);
        if(debug) Serial.println();
    }
}

bool checkSum()
{
    uint16_t check1=0;
    for (uint8_t i = 0; i < num_byte_from_esp-2; ++i)
    {
        check1+=data_from_esp_buff[i];
    }
    uint16_t check2=data_from_esp_buff[num_byte_from_esp-2]*256+data_from_esp_buff[num_byte_from_esp-1];
    if(check1==check2)
    {
        if(debug) Serial.print(millis());
        if(debug) Serial.print(" - ");
        if(debug) Serial.print(check1);
        if(debug) Serial.print(":");
        if(debug) Serial.println(check2);
        return true;
    }
    else
    {
        return false;
    }
}
