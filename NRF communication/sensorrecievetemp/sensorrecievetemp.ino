/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Interrupt-driven test for native target
 *
 * This example is the friendliest for the native target because it doesn't do
 * any polling.  Made a slight change to call done() at the end of setup.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "MemoryFree.h"
#include "FastLED.h"

#define DATA_PIN    A5  //for ledstrip


#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    22
CRGB leds[NUM_LEDS];


#include <avr/wdt.h> //watchdog timer will reset arduino if hangs for an amount of time

float datain[16];
wdt_reset();

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 8 & 9

RF24 radio(9,10);
int RFCHANNEL=76;


// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const short role_pin = 7;

//
// Topology
//

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin
//

// The various roles supported by this sketch
typedef enum { role_sender = 1, role_receiver } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Sender", "Receiver"};

// The role of the current running sketch
role_e role;

// Interrupt handler, check the radio because we got an IRQ
void check_radio(void);
long timecheck=0;

int aksRecieved=0;
int messagesSent=0;

void setup(void)
{

 wdt_disable(); //in case everything gets stuck in an endless reboot loop
 for (int i=0;i<16;i++){datain[i]=0;}
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // read the address pin, establish our role
  if ( 0 ) //set role recieve
    role = role_sender;
  else
    role = role_receiver;

  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/pingpair_irq/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  //
  // Setup and configure rf radio
  //

  radio.begin();
  //radio.setPALevel( RF24_PA_LOW );
 radio.setDataRate(RF24_2MBPS);  //speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps 
 //higher data rate appears to be more relsiable, but apparently has lower range, which shouldn't be a problem for most stuff
 radio.setChannel(RFCHANNEL);
  radio.printDetails();
  // We will be using the Ack Payload feature, so please enable it
  radio.enableAckPayload();
  
  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens a single pipe for these two nodes to communicate
  // back and forth.  One listens on it, the other talks to it.

  if ( role == role_sender )
  {
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
  }

  //
  // Start listening
  //

  if ( role == role_receiver )
    radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

//  radio.printDetails();

  //
  // Attach interrupt handler to interrupt #0 (using pin 2)
  // on BOTH the sender and receiver
  //

  attachInterrupt(0, check_radio, FALLING);

  //
  // On the native target, this is as far as we get
  //
#if NATIVE
  done();
#endif

datain[8]=0;

//wdt_enable(WDTO_4S);

}

static uint32_t message_count = 0;

unsigned long timeint=0;


int temp=0;

void loop(void)
{
  wdt_reset(); //reset timer every loop, if loop takes more than 4 seconds the arduino reboots
  
//  tone(3,880,200);
//  delay(200);
//  tone(3,880,200);
//  delay(200);
//
//  tone(3,880,200);
//  delay(200);
//
//  tone(3,880,200);
//  delay(200);

datain[8]=datain[8]+1;


for (int j=0;j<255;j++){
for (int i=0;i<NUM_LEDS;i++){
     
    
    
  leds[i].r=j/(i+1);
  
  leds[i].b=0;
  
  leds[i].g=255-j;


}
 FastLED.show();  
 delay(10);
}


for (int j=0;j<255;j++){
for (int i=0;i<NUM_LEDS;i++){
     
    
    
  leds[i].r=(255-j)/(i+1);
  
  leds[i].b=j/(22-i+1);
  
  leds[i].g=0;


}
 FastLED.show();  
 delay(10);
}


for (int j=0;j<255;j++){
for (int i=0;i<NUM_LEDS;i++){
     
    
    
 leds[i].r=0;
 leds[i].b=255-j/(22-i+1);
 leds[i].g=j;


}
 FastLED.show();  
 delay(10);
 
}


for (int j=0;j<255;j++){
for (int i=0;i<NUM_LEDS;i++){
     
    
    
//  leds[i].r=0;
  
//  leds[i].b=255-j/(22-i+1);
  
//  leds[i].g=j;


}
 FastLED.show();  
 delay(10);
 
}

 datain[2]=1;

 //ledstrip();

//delay(1000);
 datain[0]=1;

 
   //   ledstrip();

      
//delay(1000);

 datain[2]=0;

 datain[1]=1;

 
     // ledstrip();

//delay(1000);

 datain[0]=0;


//delay(1000); 


      
  //
  // Sender role.  Repeatedly send the current time
  //
  //  radio.printDetails();
  if (role == role_sender)
  {
    // Take the time, and send it.
    unsigned long time1 = millis();
    char temp1[30];
    String temp2="time = " + String(time1);
    temp2.toCharArray(temp1,30);

    Serial.print("Now sending: "+ (String)temp1 + "\n");
    radio.startWrite(&temp1 , sizeof( temp1) );
    messagesSent++;
    Serial.println("Messages sent: " + (String)messagesSent + ", Aks recieved: " + (String)aksRecieved + ". Sucess rate: " + (String)((float)aksRecieved/messagesSent*100) + "%");
    // Try again soon
    //delay(2000);
  }
  check_radio();
  
  
  if (datain[4]>150){digitalWrite(5,1); tone(3,880,200);} else{digitalWrite(5,0);}
  if (datain[5]>150){digitalWrite(6,1); tone(3,987,200);} else{digitalWrite(6,0);}
  if (datain[6]>150){digitalWrite(7,1); tone(3,1046,200);} else{digitalWrite(7,0);}
  if (datain[7]>150){digitalWrite(8,1); tone(3,1174,200);} else{digitalWrite(8,0);}
  
  
  if ((millis()-timecheck)>4000){
    
    printf("not had any activity in 4 seconds \n");
    if ( role == role_receiver ){
      radio.stopListening();
      
      radio.startListening();
      }
    timecheck=millis();
    }
  //
  // Receiver role: Does nothing!  All the work is in IRQ
  //

  if (role == role_receiver){
    if (millis()-timeint>60000){
      radio.printDetails();
      timeint=millis();
      }
    }

}


void check_radio(void)
{
  // What happened?
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);

  // Have we successfully transmitted?
  if ( tx )
  {
    timecheck=millis();
    if ( role == role_sender )
      printf("Send:OK\n\r");

    if ( role == role_receiver )
      printf("Ack Payload:Sent\n\r");
  }

  // Have we failed to transmit?
  if ( fail )
  {
    timecheck=millis();
    if ( role == role_sender )
      printf("Send:Failed\n\r");
      

    if ( role == role_receiver )
      printf("Ack Payload:Failed\n\r");
  }

  // Transmitter can power down for now, because
  // the transmission is done.
  if ( ( tx || fail ) && ( role == role_sender ) )
    radio.powerDown();

  // Did we receive a message?
  if ( rx )
  {
    timecheck=millis();
    // If we're the sender, we've received an ack payload
    if ( role == role_sender )
    {
      radio.read(&message_count,sizeof(message_count));
      printf("Ack:%lu\n\r",(unsigned long)message_count);
      aksRecieved++;
    }

    // If we're the receiver, we've received a time message
    if ( role == role_receiver )
    {
      // Get this payload and dump it
      //static unsigned long got_time;
      int len = radio.getDynamicPayloadSize();
      char message_string[30];
      radio.read( &message_string, sizeof(message_string) );
      Serial.print("Got payload string ");
      Serial.println(message_string);
      int k1=0;
      int k2=0;
      int k3=0;
      int k4=0;
      int k5=0;
      int k6=0;
      if (message_string[1]=='1'){
        Serial.print("got m1");

        //find location of the four "V's"
        k1=0;
        k2=2;
        while (message_string[k2]!='V'){k2++;}
        
        k3=k2+2;
        while (message_string[k3]!='V'){k3++;}
        
        k4=k3+2;
        while (message_string[k4]!='V'){k4++;}
        
        k5=k4+2;
        while (message_string[k5]!='V'){k5++;}
        
        datain[0]=((String)message_string).substring(k1+3,k2).toFloat();
        datain[1]=((String)message_string).substring(k2+3,k3).toFloat();
        datain[2]=((String)message_string).substring(k3+3,k4).toFloat();
        datain[3]=((String)message_string).substring(k4+3,k5).toFloat();
      
Serial.print("d1 = "+(String) datain[0]+"d2 = "+(String) datain[1]+"d3 = "+(String) datain[2]+"d4 = "+(String) datain[3]);
      }
        
      else if (message_string[1]=='5'){
        Serial.print("got m2");

        //find location of the four "V's"
        k1=0;
        k2=2;
        while (message_string[k2]!='V'){k2++;}
        
        k3=k2+2;
        while (message_string[k3]!='V'){k3++;}
        
        k4=k3+2;
        while (message_string[k4]!='V'){k4++;}
        
        k5=k4+2;
        while (message_string[k5]!='V'){k5++;}
        
        datain[4]=((String)message_string).substring(k1+3,k2).toFloat();
        datain[5]=((String)message_string).substring(k2+3,k3).toFloat();
        datain[6]=((String)message_string).substring(k3+3,k4).toFloat();
        datain[7]=((String)message_string).substring(k4+3,k5).toFloat();
      
Serial.print("d4 = "+(String) datain[4]+"d5 = "+(String) datain[5]+"d6 = "+(String) datain[6]+"d7 = "+(String) datain[7]);
        }
      else if (message_string[1]=='2'){
        Serial.print("got m3");

        //find location of the four "D's"
        k1=0;
        k2=2;
        while (message_string[k2]!='D'){k2++;}
        
        k3=k2+2;
        while (message_string[k3]!='D'){k3++;}
        
        k4=k3+2;
        while (message_string[k4]!='D'){k4++;}
        
        k5=k4+2;
        while (message_string[k5]!='D'){k5++;}
        
        k6=k5+2;
        while (message_string[k6]!='D'){k6++;}
        
        datain[8]=((String)message_string).substring(k1+3,k2).toFloat();
        datain[9]=((String)message_string).substring(k2+3,k3).toFloat();
        datain[10]=((String)message_string).substring(k3+3,k4).toFloat();
        datain[11]=((String)message_string).substring(k4+3,k5).toFloat();
        datain[12]=((String)message_string).substring(k5+3,k6).toFloat();
      
Serial.print("d8 = "+(String) datain[8]+"d9 = "+(String) datain[9]+"d10 = "+(String) datain[10]+"d11 = "+(String) datain[11]+"d12 = "+(String) datain[12]);
        }
      // Add an ack packet for the next time around.  This is a simple
      // packet counter
      radio.writeAckPayload( 1, &message_count, sizeof(message_count) );
      ++message_count;
    }
  }
}

void ledstrip()
 {

    for (int i=0;i<NUM_LEDS;i++){
    leds[i].r=00;
    leds[i].b=00;
    leds[i].g=00;
    
    }
    for (int i=0;i<min(NUM_LEDS,datain[8]);i++){
     
    
    
    if(datain[0]){leds[i].r=00;}else{leds[i].r=200;}
    if(datain[1]){leds[i].b=00;}else{leds[i].b=200;}
    if(datain[2]){leds[i].g=00;}else{leds[i].g=200;}
    if(datain[3]){digitalWrite(2,1);}else{digitalWrite(2,0);}
    
    
    
    }
  FastLED.show();  
  }

// vim:ai:cin:sts=2 sw=2 ft=cpp
