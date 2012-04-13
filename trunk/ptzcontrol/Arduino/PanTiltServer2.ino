/*
 Pan/Tilt Servo
 Allows control of pan/tilt servos via a TCP server using the Ethernet shield.
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Pan Servo on pin 5 (Uno)
 * Tilt Servo on Pin 6 (Uno)
 
 Modified from example ChatServer and sweep
 by Josiah Bryan <josiahbryan@gmail.com>
 2011-12-15

 Original ChatServer example:
 created 18 Dec 2009
 by David A. Mellis
 modified 10 August 2010
 by Tom Igoe
 
 */

#include <SPI.h>
#include <Ethernet.h>

#include <Servo.h> 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/*
IPAddress ip(10,1,5,45);
IPAddress gateway(10,0,0,1);
IPAddress subnet(255, 0, 0, 0);
*/
IPAddress ip(192,168,0,35);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255, 255, 255, 0);


// Server on port 3729
EthernetServer server(3729);
EthernetClient client = 0; // Client needs to have global scope so it can be called
			 // from functions outside of loop, but we don't know
			 // what client is yet, so creating an empty object

/** Adjust these values for your servo and setup, if necessary **/
int servoPin     =  3;    // control pin for servo motor
int minPulse     =  600;  // minimum servo position
int maxPulse     =  2400; // maximum servo position
int turnRate     =  100;  // servo turn rate increment (larger value, faster rate)
int refreshTime  =  20;   // time (ms) between pulses (50Hz)

/** The Arduino will calculate these values for you **/
int centerServo;         // center servo position
int pulseWidth;          // servo pulse width
int moveServo;           // raw user input
long lastPulse   = 0;    // recorded time (ms) of the last pulse
int lastPulseWidth = 0;

// Servos for pan/tilt
Servo panServo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
Servo tiltServo;

Servo servo1; Servo servo2; Servo servo3;

int lastX=0, lastY=0, lastZ=0;
int loop_cnt=0;

//Analog read pins
const int xPin = 2;
const int yPin = 0;
const int zPin = 1;

int zoomPulseTime = 500; // ms
int zoomMinusVal  = 135;
int zoomMidVal    = 145;
int zoomPlusVal   = 155;

void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  // open the serial port
  Serial.begin(9600);
/*  
  servo1.attach(3);
  servo2.attach(5); 
  servo3.attach(6);
*/
  servo1.attach(9);
  servo2.attach(5);
  servo3.attach(6);
  servo3.write(129);
  servo1.write(145);
  servo2.write(115);

  //servo3.write(129);
  
  Serial.begin(19200);
  Serial.println("Ready");

}

void loop() {
  static int v = 0;
  int i;

    
  // check to see if text received
  if (client && client.connected())
  {
      if(client.available()) 
      {
           char ch = client.read();
      
           switch(ch) {
            case '0'...'9':
              v = v * 10 + ch - '0';
              break;
            case 's':
              if(v < 35) v = 35; // minimum tilt
              if(v < 300)
                servo2.write(v);
              else
                servo2.writeMicroseconds(v);
              client.print("rx: s [tilt] "); client.println(v);              
              v = 0;
              break;
            /*
            case 'w':
              if(v < 300)
                servo1.write(v);
              else
                servo1.writeMicroseconds(v);
//              client.print("rx: w "); client.println(v);              
              v = 0;
              break;
            */
            case 'w':
              //servo1.write(v);
              client.print("orig v rx:     "); client.println(v);
              v = v-100;
              if(v < -100) v = -100;
              if(v > +100) v = +100;
            
              if(v != 0)
              {
                
                //int zoomMinusVal  = 135;
                //int zoomMidVal    = 145;
                //int zoomPlusVal   = 155;

                servo1.write(v < 0 ? zoomMinusVal : zoomPlusVal);
                int totalDelay = zoomPulseTime * abs(v);
                client.print("zoomPulseTime: "); client.println(zoomPulseTime);
                client.print("v:             "); client.println(v);
                client.print("totalDelay:    "); client.println(totalDelay);
                //for(i=0; i < abs(v); i++)
                  delay(totalDelay);
                servo1.write(zoomMidVal);
              }
            
              v = 0;
              break;

            case 'u':
              if(v < 300)
                servo3.write(v);
              else
                servo3.writeMicroseconds(v);
              client.print("rx: u [pan] "); client.println(v);
              v = 0;
              break;
            case 'z':
              if(v < 0)    v = 10;
              if(v > 1000) v = 1000;
              zoomPulseTime = v;
              v = 0;
              break;
            case 'a':
              zoomMinusVal = v;
              v = 0;
              break;
            case 'b':
              zoomMidVal = v;
              v = 0;
              break;
            case 'c':
              zoomPlusVal = v;
              v = 0;
              break;
            case 'q':
              if(v==0)
              {
                  int xRead = analogRead(xPin);
                  int yRead = analogRead(yPin);
                  int zRead = analogRead(zPin);
                  
                  client.print(xRead);
                  client.print(",");
                  client.print(yRead);
                  client.print(",");
                  client.print(zRead);
                  client.print("\n");
              }
              break;
          }
      }
  }
  else
  {
        client = server.available(); 
  }

}











