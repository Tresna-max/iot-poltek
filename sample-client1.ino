#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

Adafruit_BMP280 bme; // I2C

const int Sensor_Pin = A0;
unsigned int Sensitivity = 185;   // 185mV/A for 5A, 100 mV/A for 20A and 66mV/A for 30A Module
float Vpp = 0; // peak-peak voltage 
float Vrms = 0; // rms voltage
float Irms = 0; // rms current
float Supply_Voltage = 233.0;           // reading from DMM
float Vcc = 5.0;         // ADC reference voltage // voltage at 5V pin 
float power = 0;         // power in watt              
float Wh =0 ;             // Energy in kWh
unsigned long last_time =0;
unsigned long current_time =0;
unsigned long interval = 100;
unsigned int calibration = 100;  // V2 slider calibrates this
unsigned int pF = 85;           // Power Factor default 95
float bill_amount = 0;   // 30 day cost as present energy usage incl approx PF 
unsigned int energyTariff = 1467.28; // Energy cost in INR per unit (kWh)

float Temp;
float Press;
float Altitude;

const char* ssid = "DDNS";
const char* password = "DellDDNS132";

byte server[] = {192, 168, 1, 15};   //eg: This is the IP address of your PC once XAMPP

WiFiClient client;

void setup()
{
 Serial.begin(115200);
  delay(100);
  
  // setup BMP280
  Serial.println(F("BMP280 test"));
  
  if (!bme.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Got IP: "); 
  Serial.print(WiFi.localIP());
  
  Serial.println("");
  delay(1000);
  Serial.println("connecting...");
}

void loop() {
  
  getACS712(); //start calculating current and power
				// Irms, power and Wh 
  Temp = bme.readTemperature();
  Press = bme.readPressure();
  Altitude = bme.readAltitude(1013.25); // this should be adjusted to your local forcase
    
  Sending_To_phpmyadmindatabase(Irms, power, Wh, Temp, Press, Altitude);
  delay(5000); // every 5 secs

}

void Sending_To_phpmyadmindatabase(float Irms, float power, float Wh, float Temp, float Press, float Altitude){   //CONNECT>   
   if (client.connect(server, 8082)) {
    Serial.println("connected to local server");
    // Make a HTTP request:

    Serial.print("GET /www/powerServer.php?Irms=");
	  Serial.print(Irms);
	
    client.print("GET /www/powerServer.php?Irms=");     //YOUR URL
	  client.print(Irms);
    
    client.print("&power=");
	  client.print(power);
    Serial.print("&power=");  
    Serial.print(power);
	
	  client.print("&Wh=");
	  client.print(Wh);
    Serial.print("&Wh=");  
    Serial.print(Wh);
	
	  client.print("&Temp=");
	  client.print(Temp);
    Serial.print("&Temp=");  
    Serial.print(Temp);
	
	  client.print("&Press=");
	  client.print(Press);
    Serial.print("&Press=");  
    Serial.print(Press);
	
	  client.print("&Altitude=");
	  client.print(Altitude);
    Serial.print("&Altitude=");  
    Serial.println(Altitude);
	
    client.print(" ");      //SPACE BEFORE HTTP/1.1
    client.print("HTTP/1.1");
    client.println();

    client.println("Host: 192.168.1.15"); // Add your server IP address here. If you forgott>    
	  client.println("Connection: close");
    client.println();

  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
}



void getACS712() {  // for AC
  Vpp = getVPP();
  Vrms = (Vpp/2.0) *0.707; 

//  Vrms = Vrms - (calibration / 10000.0);     // calibtrate to zero with slider

  Irms = (Vrms * 1000)/Sensitivity ;
  if((Irms > -0.015) && (Irms < 0.008)){  // remove low end chatter
    Irms = 0.0;
  }
  
  power= (Supply_Voltage * Irms) * (pF / 100.0); 
  
  last_time = current_time;
  current_time = millis();  
  
  Wh = Wh+  power *(( current_time -last_time) /3600000.0) ; // calculating energy in Watt-Hour
  
  bill_amount = Wh * (energyTariff/1000);
  Serial.print("Irms:  "); 
  Serial.print(String(Irms, 3));
  Serial.println(" A");
  Serial.print("Power: ");   
  Serial.print(String(power, 3)); 
  Serial.println(" W"); 
  Serial.print("  Bill Amount: INR"); 
  Serial.println(String(bill_amount, 2));   
}

float getVPP()
{
  float result; 
  int readValue;                
  int maxValue = 0;             
  int minValue = 1024;          
  uint32_t start_time = millis();
  while((millis()-start_time) < 950) //read every 0.95 Sec
  {
     readValue = analogRead(Sensor_Pin);    
     if (readValue > maxValue) 
     {         
         maxValue = readValue; 
     }
     if (readValue < minValue) 
     {          
         minValue = readValue;
     }
  } 
   result = ((maxValue - minValue) * Vcc) / 1024.0;  
   return result;
}
