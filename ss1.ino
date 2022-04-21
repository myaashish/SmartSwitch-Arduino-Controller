/*
Author: Aashish Parmar (parmaraashish3@gmail.com)

This program creates a client end for Home Automation Project
on Arduino. It makes the system capable to receive commands 
from Interet and Local Network and can save state of
different switches to recover after power outage.
*/

#include<SPI.h>
#include<Ethernet.h>
#include<EthernetUdp.h>

EthernetUDP Udp;
EthernetClient cl;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE], temp[50], num[10];
String msg;
boolean flag;
int i;
unsigned long time[9], mil[9];

/*
Write switch status and time to file. Time will ensure some
duration between multiple power cycles for a switch.
*/
void fiwrite() {
  digitalWrite(4, LOW);
  digitalWrite(10, HIGH);
  myFile = SD.open("time.txt", FILE_WRITE);
  
  if (myFile) {
    for(i = 2; i <= 10; i++) {
      myFile.println(time[i]);
      myFile.println("\n");
    }
    myFile.close();
  }
  
  if (myFile) {
    myFile = SD.open("state.txt", FILE_WRITE);
    for(i = 2; i <= 10; i++) {
      if(digitalRead(i)) {
        myFile.println("1");
      }
      else {
        myFile.println("0");
      }
    }
    myFile.close();
  }
  digitalWrite(4, HIGH);
  digitalWrite(10, LOW);
  }

/*
Initializes the controller with  network and file
configurations. It also sets previous switch states before
any power outage.
*/
void setup() {
  Serial.begin(9600);  
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  
  while(!SD.begin(4)) {
    ;
  }
  
  IPAddress ip(192, 168, 10, 1);  
  byte mac[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  myFile = SD.open("num.txt");
  i = 0;
  
  while (myFile.available()) {
    num[i] = char(myFile.read());    
    Serial.write(num[i]);
    ++i;
  }
  num[i] = '\0';
  myFile.close();

  myFile = SD.open("time.txt");
  i = 0;
  
  while (myFile.available()) {
    time[i] = myFile.parseInt();
    ++i;
  }
  myFile.close();
  
  // reset all power connections during startup
  for(i = 2; i <= 9; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  myFile = SD.open("state.txt");
  i = 2;
  
  while (myFile.available()) {
    if(char(myFile.read()) == '1') {
      digitalWrite(i, HIGH);
    }
    else {
      digitalWrite(i, LOW);
    }
    ++i;
  }
  myFile.close();

  Ethernet.begin(mac, ip);
  
  Udp.begin(19537);  

  msg.remove(0);
  msg = "http://website.com/dbxyz.php?pinum=";
  msg += num;
  cl.connect(msg.c_str(), 80);
}

/*
This function receives switch ON/OFF message from user and
updates current status in file.
*/
void loop() {
  msg.remove(0);
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    i = 0;
    flag = true;
    while(i < 10)
    {
      if(packetBuffer[i+2] != num[i])
      {
        flag = false;
        break;
      }
      ++i;
    }
    if(flag) {
      Serial.print(packetBuffer[0]);
      if(packetBuffer[0] == '0') {
        for(i = 2; i <= 10; i++) {
          digitalWrite(i, LOW);
          time[i] += mil[i];
        }
      }
      else if(packetBuffer[0] == 'z') {
        Udp.beginPacket(Udp.remoteIP(), 80);
        for(i = 0; i <= 8; i++) {
          if(digitalRead(i+2))
            msg += '1';
          else
            msg += '0';
        }
        msg += '_';
        msg += num;
        Udp.write(msg.c_str());
        Udp.endPacket();
      }
      else {
        i = packetBuffer[0] - 48;
        Serial.print(i);
        if(digitalRead(i+1)) {
          time[i-1] += mil[i-1];
        }
        else {
          mil[i-1] = millis();
        }
        if(i != 3) {
          digitalWrite(i+1, !digitalRead(i+1));
        }
        else {
          digitalWrite(11, !digitalRead(11));
        }      
      }
    }
    fiwrite();
  }
}
