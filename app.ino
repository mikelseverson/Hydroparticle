#include "Adafruit_DHT/Adafruit_DHT.h"
#define address 99             //default I2C ID number for EZO EC Circuit.

//Websever
TCPClient webClient;
TCPServer webServer = TCPServer(80);
char myIpAddress[24];

//PH Tracking
char computerdata[20];           //we make a 20 byte character array to hold incoming data from a pc/mac/other.
byte code = 0;                   //used to hold the I2C response code.
char ec_data[48];                //we make a 48 byte character array to hold incoming data from the EC circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the EC Circuit.
byte i = 0;                      //counter used for ec_data array.
int delay_time = 1400;           //used to change the delay needed depending on the command sent to the EZO Class EC Circuit.
char *ph;                        //char pointer used in string parsing.

//Air Humidity and Temperature
#define DHTPIN 2
#define DHTTYPE DHT22
int tempF;
int humidity;
DHT dht(DHTPIN, DHTTYPE);


void setup() 
{
  Serial.begin(9600);
  Wire.begin();
  dht.begin();

  Particle.variable("tempF", tempF);
  Particle.variable("humidity", humidity);
  
  
  Particle.variable("ipAddress", myIpAddress, STRING);
  IPAddress myIp = WiFi.localIP();
  sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Particle.publish("ip", String(myIpAddress));
  webServer.begin();
  
}

void debugPH() {
    switch (code) {
        case 1:
            Particle.publish("Success");
            break;
        case 2:
            Particle.publish("Failed");
            break;
        case 254:
            Particle.publish("Pending");
            break;
        case 255:
            Particle.publish("No Data");
            break;
    }
}

void loop() {
    server();
    readPH();
    readAir();
}

void server() {
    if (webClient.connected() && webClient.available()) {
        serveWebpage();
    }
    else {
        webClient = webServer.available();
    } 
}

void readAir() {
    tempF = dht.getTempFarenheit();
    humidity = dht.getHumidity();
    if(tempF > 32) {
        Particle.publish("tempF", String(tempF));
    }
    if(humidity > 0 && humidity < 100) {
        Particle.publish("humidity", String(humidity));
    }
    delay(1000);
}

void readPH() {


    delay_time=1600;                            //Need to set the delay to 1.8 seconds when taking a reading

    Wire.beginTransmission(address);            //call the circuit by its ID number.
    Wire.write("r");                            //transmit the command that was sent through the serial port.
    Wire.endTransmission();                     //end the I2C data transmission.

    delay(delay_time);                          //wait the correct amount of time for the circuit to complete its instruction.

    Wire.requestFrom(address, 48, 1);           //call the circuit and request 48 bytes (this is more then we need).
    code = Wire.read();                         //the first byte is the response code, we read this separately.
    
    while (Wire.available()) {                  //are there bytes to receive.
      in_char = Wire.read();                    //receive a byte.
      ec_data[i] = in_char;                     //load this byte into our array.
      i += 1;                                   //incur the counter for the array element.
      if (in_char == 0) {                       //if we see that we have been sent a null command.
        i = 0;                                  //reset the counter i to 0.
        Wire.endTransmission();                 //end the I2C data transmission.
        break;                                  //exit the while loop.
      }
    }
       
    debugPH();
    Particle.publish("data", ec_data);
    ph = strtok(ec_data, ",");
    Particle.publish("PH", String(ph));
    
    delay(5000-delay_time);

}

void serveWebpage() {
    webClient.println("<html> \
    PH: " + String(ph) + "<br /> \
    Temperature: " + String(tempF) + "<br /> \
    Humidity: " + String(humidity) + "%" +  
                     "</html>\n\n");
    webClient.flush();
    webClient.stop();
    delay(100);
}