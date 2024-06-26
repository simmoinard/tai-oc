#include "WaterFlow.h"
#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"

//Set these OTAA parameters to match your app/node in TTN
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x6D, 0x1E };
uint8_t appEui[] = { 0x89, 0x79, 0x08, 0x74, 0x87, 0x92, 0x38, 0x49 };
uint8_t appKey[] = { 0xFD, 0xFA, 0x3E, 0x69, 0x8C, 0x50, 0xB3, 0xF4, 0xDA, 0x5B, 0x5E, 0x1A, 0x35, 0x1D, 0x41, 0x9F };
int temps = 180; // Indiquez dans cette ligne la fréquence d'envoi de données, en secondes. (Ne pas aller plus bas que 3minutes, soit 180sec)

uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
static uint8_t counter=0;
uint8_t lora_data[5];
uint8_t downlink ;

WaterFlow waterFlow(ADC2, 65);
WaterFlow waterFlow2(ADC3, 65);

unsigned long beforeTime;

///////////////////////////////////////////////////
//Some utilities for going into low power mode
TimerEvent_t sleepTimer;
//Records whether our sleep/low power timer expired
bool sleepTimerExpired;

static void wakeUp()
{
  sleepTimerExpired=true;
}

void count() {
  waterFlow.pulseCount();
}

static void lowPowerSleep(uint32_t sleeptime)
{
  sleepTimerExpired=false;
  TimerInit( &sleepTimer, &wakeUp );
  TimerSetValue( &sleepTimer, sleeptime );
  TimerStart( &sleepTimer );
  //Low power handler also gets interrupted by other timers
  //So wait until our timer had expired
  while (!sleepTimerExpired) lowPowerHandler();
  TimerStop( &sleepTimer );
}

///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  pinMode(GPIO7,OUTPUT);
  digitalWrite(GPIO7,LOW);
  LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION);
 
  //Enable ADR
  LoRaWAN.setAdaptiveDR(true);

  while (1) {
    Serial.print("Joining... ");
    LoRaWAN.joinOTAA(appEui, appKey, devEui);
    if (!LoRaWAN.isJoined()) {
      //In this example we just loop until we're joined, but you could
      //also go and start doing other things and try again later
      Serial.println("JOIN FAILED! Sleeping for 30 seconds");
      lowPowerSleep(30000);
    } else {
      Serial.println("JOINED");
      break;
    }
  }

  waterFlow.begin(count);
  waterFlow2.begin(count);

}


///////////////////////////////////////////////////
void loop()
{
  counter++;
  delay(10);
  uint8_t voltage = getBatteryVoltage()/50; //Voltage in %
  digitalWrite(GPIO7,HIGH);
  delay(1500);

  beforeTime = millis();

  while ((millis() - beforeTime) < 1000){
  waterFlow.read();
  }
  waterFlow.clearVolume();
  int debit = waterFlow.getFlowRateOfMinute()*100;
    Serial.print(debit);
    Serial.println(" L/m   ");

  beforeTime = millis();

  while ((millis() - beforeTime) < 1000){
  waterFlow2.read();
  }
  waterFlow2.clearVolume();
  int debit2 = waterFlow2.getFlowRateOfMinute()*100;
    Serial.print(debit2);
    Serial.println(" L/m   ");
 
  digitalWrite(GPIO7,LOW);
  
  Serial.printf("\ndebit : %d\n", debit);
  Serial.printf("\ndebit2 : %d\n", debit2);

  Serial.printf("\nVoltage : %d\n", voltage);
  lora_data[0] = voltage;
  lora_data[1] = highByte(debit);
  lora_data[2] = lowByte(debit);
  lora_data[3] = highByte(debit2);
  lora_data[4] = lowByte(debit2);
  //Now send the data. The parameters are "data size, data pointer, port, request ack"
  Serial.printf("\nSending packet with counter=%d\n", counter);
  Serial.printf("\nValue 1 to send : %d\n", lora_data[1]);

  //Here we send confirmed packed (ACK requested) only for the first two (remember there is a fair use policy)
  bool requestack=counter<2?true:false;
  if (LoRaWAN.send(sizeof(lora_data), lora_data, 1, requestack)) {
    Serial.println("Send OK");
  } else {
    Serial.println("Send FAILED");
  }

  lowPowerSleep(temps*1000);  
  delay(10);
}
