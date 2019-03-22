#include "./esp.h"
void setup() 
{
    Serial.begin(9600);//txrx0
    if(debug) Serial.setDebugOutput(true);
   // Serial1.begin(9600);//gpio2 esp
    pinMode(PIN_BUTTON, INPUT);
	pinMode(14, OUTPUT);
    WiFi.mode(WIFI_STA);
    mqtt_begin();
    SPIFFS.begin();
    f_init(fileName);
	digitalWrite(14, LOW);
    if(debug) Serial.println(" - setup done");
}
//gui qua serial1?
//id con wifi de check tren MQTT
//baud 9600
void loop() 
{
    if (longPress())
    {
        if (WiFi.beginSmartConfig())
        {
			digitalWrite(14, HIGH);
            if(debug) Serial.println(" - start config wifi");
        }
        else
        {
            if(debug) Serial.println(" - start config wifi false");
        }
    } 
    if(Serial.available() > 0)
    {
        data_buff[data_i]=Serial.read();
        if(data_buff[0]==66) data_i++;
        if(data_i==num_byte_data_from_arduino)
        {
            data_i=0;
            if(data_buff[1]==77)
            {
                uint16_t check=0;
                for(uint8_t i=0;i<num_byte_data_from_arduino-2;i++)
                {
                    check+=data_buff[i];
                }
                uint16_t check2=data_buff[num_byte_data_from_arduino-2]*256+data_buff[num_byte_data_from_arduino-1];
                if(check==check2)
                {
                    //enQueue
                    data arduino_data;
                    arduino_data.temp=data_buff[8];
                    arduino_data.dot_temp=data_buff[9];
                    arduino_data.hum=data_buff[10];
                    arduino_data.dot_hum=data_buff[11];
					
                    arduino_data.pm1=data_buff[12]*256+data_buff[13];
                    arduino_data.dot_pm1=data_buff[14];
                    arduino_data.pm25=data_buff[15]*256+data_buff[16];
                    arduino_data.dot_pm25=data_buff[17];
                    arduino_data.pm10=data_buff[18]*256+data_buff[19];
                    arduino_data.dot_pm10=data_buff[20];

                    struct tm t;
                    time_t epoch;
                    
                    t.tm_hour = data_buff[2];
                    t.tm_min = data_buff[3];
                    t.tm_sec = data_buff[4];
                    t.tm_mday = data_buff[5];          // Day of the month                    
                    t.tm_mon = data_buff[6]-1;           // Month, 0 - jan
                    t.tm_year = data_buff[7]+100;                    
                    t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
                    epoch = mktime(&t);
                    
                    arduino_data.epoch_time=epoch;
                    
                    if(debug) Serial.print(front);
                    if(debug) Serial.print(" ");
                    if(debug) Serial.println(rear);
                    
                    enQueue(arduino_data);
             
                    if(debug) Serial.print(front);
                    if(debug) Serial.print(" ");
                    if(debug) Serial.println(rear);
                }
            }
            else if(data_buff[1]==88)
            {
                if(WiFi.status() == WL_CONNECTED)
                {
                    lastGetTime=millis();
                    while(true)
                    {
                        if(millis()-lastGetTime>500)
                            break;
                        dateTime = NTPch.getNTPtime(7.0, 0);        
                        if(dateTime.valid)
                        {
                            //if got time, send time to arduino
                            uint8_t time2arduino_buff[num_byte_send_2_arduino]={66,88,dateTime.hour,dateTime.minute,dateTime.second,dateTime.day,dateTime.month,dateTime.year-2000,0,0};
                            uint16_t a=0;
                            for(uint8_t i=0;i<num_byte_send_2_arduino-2;i++)
                            {
                                a+=time2arduino_buff[i];
                            }
                            time2arduino_buff[num_byte_send_2_arduino-2]=a/256;
                            time2arduino_buff[num_byte_send_2_arduino-1]=a%256;
                            if(debug) Serial.print(" - time: ");
                            for(uint8_t i=0;i<num_byte_send_2_arduino;i++)
                            {
                                Serial.write(time2arduino_buff[i]);
                                if(debug) Serial.print(time2arduino_buff[i]);
                                if(debug) Serial.print(" ");
                            }
                            if(debug) Serial.println();
                            break;
                        }
                    }
                }
            }
        }
    }
    else if(WiFi.status() == WL_CONNECTED)
    {
        if(isQueueEmpty()==false)
        {
            //if queue is not empty, publish data to server
            if(mqttClient.connected())
            {
                if(debug) Serial.print(front);
                if(debug) Serial.print(" ");
                if(debug) Serial.println(rear);
                if(debug) Serial.print(" - publish:.. ");
                data send_data=deQueue();
                char mes[256]={0};
                sprintf(mes,"{\"data\":{\"tem\":\"%d,%d\",\"humi\":\"%d,%d\",\"pm1\":\"%d,%d\",\"pm2p5\":\"%d,%d\",\"pm10\":\"%d,%d\",\"CO\":\"%d\",\"time\":\"%d\"}}",send_data.temp,send_data.dot_temp,send_data.hum,send_data.dot_hum,send_data.pm1,send_data.dot_pm1,send_data.pm25,send_data.dot_pm25,send_data.pm10,send_data.dot_pm10,send_data.pm25,send_data.epoch_time);
                if(mqttClient.publish(topic,mes,true))
                {
                    if(debug) Serial.println(mes);
                }
                else
                {
                    if(debug) Serial.println("publish false");
                    enQueue(send_data);
                }
                if(debug) Serial.print(front);
                if(debug) Serial.print(" ");
                if(debug) Serial.println(rear);
                mqttClient.loop();
            }
            else if(millis()-lastMqttReconnect>1000)
            {
                lastMqttReconnect=millis();
                if(debug) Serial.println(" - mqtt reconnect ");
                mqttClient.connect(espID);
            }
        }
    }
}
