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
#include <CapacitiveSensor.h>

#include <avr/wdt.h> //watchdog timer will reset arduino if hangs for an amount of time

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



CapacitiveSensor   cs_1 = CapacitiveSensor(5,4);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil

CapacitiveSensor   cs_2 = CapacitiveSensor(5,6);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil
  
CapacitiveSensor   cs_3 = CapacitiveSensor(5,7);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil
  
CapacitiveSensor   cs_4 = CapacitiveSensor(5,8);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil


void setup(void)
{

 wdt_disable(); //in case everything gets stuck in an endless reboot loop
  
  //
  // Role
  //


//pinMode(0,INPUT); //free
//pinMode(1,INPUT); //free
pinMode(2,OUTPUT);  //sonar trig
pinMode(3,INPUT);   //sonar eho
pinMode(4,OUTPUT);   //capacitive trig //temporarily sonar power
pinMode(5,INPUT);   //capacitive sense 1
pinMode(6,INPUT);   //capacitive sense 2
pinMode(7,INPUT);   //capacitive sense 3
pinMode(8,INPUT);   //capacitive sense 4

pinMode(A0,INPUT);  //blue sensor
pinMode(A1,INPUT);  //red sensor
pinMode(A2,INPUT);  //green sensor
pinMode(A3,INPUT);  //yellow sensor
pinMode(A4,INPUT);  //V sense4
pinMode(A5,INPUT);  //V sense3
pinMode(A6,INPUT);  //V sense2
pinMode(A7,INPUT);  //V sense1

  
cs_1.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   
cs_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   
cs_3.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   
cs_4.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   
  
  // read the address pin, establish our role
  if ( 1 ) //set this one to send data
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

wdt_enable(WDTO_8S);

}

static uint32_t message_count = 0;

unsigned long timeint=0;


    int ana[8];
    int digi[9];

void loop(void)
{
  wdt_reset(); //reset timer every loop, if loop takes more than 4 seconds the arduino reboots
 
  digitalWrite(4,1);
  delay(5);
  float distance=sonar(2,3);
  
  digitalWrite(4,0);
if (distance >= 400 || distance <= 2){
    Serial.print("Distance = ");
    Serial.println("Out of range");
  }
  else {
    Serial.print("Distance = ");
    Serial.print(distance);
    Serial.println(" cm");
    //delay(500);
  }

  wdt_reset();

    
    //capacitanceRead();
    
    wdt_reset();

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

    //Serial.print("Now sending: "+ (String)temp1 + "\n");
    //radio.startWrite(&temp1 , sizeof( temp1) );
    messagesSent++;
    Serial.println("Messages sent: " + (String)messagesSent + ", Aks recieved: " + (String)aksRecieved + ". Sucess rate: " + (String)((float)aksRecieved/messagesSent*100) + "%");

    
    // Try again soon


////    digi[0]=digitalRead(0); //nc
    digi[1]=digitalRead(1); //cap reset switch
  //  digi[2]=digitalRead(2);  //distance
  //  digi[3]=digitalRead(3);
  //  digi[4]=digitalRead(4);
  //  digi[5]=digitalRead(5);  //cap1
  //  digi[6]=digitalRead(6);  //cap2
  //  digi[7]=digitalRead(7);  //cap3
//    digi[8]=digitalRead(8);  //cap4
   
    ana[0]=digitalRead(A0);  //blue in
    ana[1]=digitalRead(A1);  //red in
    ana[2]=digitalRead(A2);  //green in
    ana[3]=digitalRead(A3);  //yllow in
    ana[4]=analogRead(A4);   //volt 4
    ana[5]=analogRead(A5);   //volt 3
    ana[6]=analogRead(A6);   //volt 2
    ana[7]=analogRead(A7);   //volt 1

   Serial.print("Analog 0 = " + (String)ana[0] + " Analog 1 = " + (String)ana[1] + " Analog 2 = " + (String)ana[2] + " Analog 3 = " + (String)ana[3] + " Analog 4 = ");
   Serial.println((String)ana[4] + " Analog 5 = " + (String)ana[5] + " Analog 6 = " + (String)ana[6] + " Analog 7 = " + (String)ana[7]);

   Serial.print("Digital 0 = " + (String)digi[0] + "Digital 1 = " + (String)digi[1] + "Digital 2 = " + (String)digi[2] + "Digital 3 = " + (String)digi[3] + "Digital 4 = ");
   Serial.println((String)digi[4] + "Digital 5 = " + (String)digi[5] + "Digital 6 = " + (String)digi[6] + "Digital 7 = " + (String)digi[7] + "Digital 8 = " + (String)digi[8]);

   
    senddata2();
    
    //delay(200);
  }
  //check_radio();
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


      // Add an ack packet for the next time around.  This is a simple
      // packet counter
      radio.writeAckPayload( 1, &message_count, sizeof(message_count) );
      ++message_count;
    }
  }
}

void senddata2(){
  
    char temp1[30];
    String temp2="V1=" + String(ana[0])+"V2=" + String(ana[1])+"V3=" + String(ana[2])+"V4=" + String(ana[3])+"V";
    temp2.toCharArray(temp1,30);

    Serial.print("Now sending: "+ (String)temp1 + "\n");
    radio.startWrite(&temp1 , sizeof( temp1) );
    //char temp1[30];
    temp2="V5=" + String(ana[4])+"V6=" + String(ana[5])+"V7=" + String(ana[6])+"V8=" + String(ana[7])+"V";
    temp2.toCharArray(temp1,30);

    Serial.print("Now sending: "+ (String)temp1 + "\n");
    radio.startWrite(&temp1 , sizeof( temp1) );

    
    temp2="D2=" + String(digi[2])+ "D5=" + String(digi[5])+"D6=" + String(digi[6])+"D7=" + String(digi[7])+"D8=" + String(digi[8])+"D";
    temp2.toCharArray(temp1,30);

    Serial.print("Now sending: "+ (String)temp1 + "\n");
    radio.startWrite(&temp1 , sizeof( temp1) );
    
  
  }


float sonar(int trigPin,int echoPin) {

  
  float duration, distance;
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) * 0.0344;
  digi[2]=(int)duration;
  return distance;
  
}

void capacitanceRead(){
  
  long total1 =  cs_1.capacitiveSensor(30);
  long total2 =  cs_2.capacitiveSensor(30);
  long total3 =  cs_3.capacitiveSensor(30);
  long total4 =  cs_4.capacitiveSensor(30);

  digi[5]=total1;
  digi[6]=total2;
  digi[7]=total3;
  digi[8]=total4;

    Serial.print("Cap sensors: ");
    Serial.print(total1);                  // print sensor output 1
    Serial.print("\t");
    Serial.print(total2);                  // print sensor output 2
    Serial.print("\t");
    Serial.print(total3);                // print sensor output 3
    Serial.print("\t");
    Serial.println(total4);                  // print sensor output 2
    
  
  }



  

// vim:ai:cin:sts=2 sw=2 ft=cpp
