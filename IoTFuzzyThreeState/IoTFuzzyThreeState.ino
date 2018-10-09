/****************************************************************************/
/* IoT - Fuzzy [temperature sensor, gas sensor] - Project  with  ThingSpeak */
/****************************************************************************/

/* Connections for RAW testing by typing commands on serial monitor
   ----------------------------------------------------------------
   Esp-01 TX ---> Arduino MEGA 1
   Esp-01 RX ---> Arduino MEGA 0
   ESP-01 VCC,ESP-01 CH_PD ---> Arduino MEGA 3V3 power rail
   ESP-01 GND --> GND
   
   MSP430 VCC 3V3 --> Arduino MEGA 3V3 power rail
   MSP430 GND --> GND
 
*/
/* Connections - Final Setup
   >>>>>>>>>>>>>>>>>>>>>>>>>    
   
   Esp-01 TX ---> Arduino MEGA RX19
   Esp-01 RX ---> Arduino MEGA TX18
   ESP-01 VCC,ESP-01 CH_PD ---> Arduino MEGA 3V3 power rail
   ESP-01 GND --> GND
   
   MSP430 VCC 3V3 --> Arduino MEGA 3V3 power rail
   MSP430 GND --> GND
   
   Sensors to be connected on analog/digtal pins using 5V
   Piezo speaker is connected to piezoPin and controlled with tone() and noTone()
   #Any other pair of TX and RX MEGA pins can be used . 19,18 form Serial1()
*/

//Analog pins
int sensor1=A0;
int sensor2=A7;
//int sensor3=A2;
//int sensor4=A7;
//int sensor5in=12;
//int sensor5out=13;

int led1=10;
int led2=11;
int piezoPin=2;

float tempRead;
float gasRead;
//float lightRead;
//float soundRead;
//float ultrasonicRead;

String HOST;
String PORT;
String POST;
String POST_CrispFuzzyOP;
String API_WR;
String API_RD;
String ChID;

int postLen;
int postLentc;       

uint32_t period; //used for timer
int speaker_Alarm=0;


void setup(){

  Serial.begin(9600);
  Serial1.begin(9600);

  //Pins
  pinMode(A0,INPUT);
  pinMode(A3,INPUT);
  pinMode(10,OUTPUT);
  pinMode(11,OUTPUT);
  
  //Network
  String AP ="Access point name";
  String PASS ="Password";

  //Cloud
  HOST="api.thingspeak.com";
  PORT="80";
  API_WR="WRITE KEY"; //API write key
  API_RD="READ KEY";//API read key
  ChID="CHANNEL ID";//Channel ID
  
  //Sensor fileds
  //------------------
  //field1 - Temperature
  //field2 - Gas Leak 
  //field6 - Fire/gas Leak
  


  ATCommand("AT",150);
  
  ATCommand("AT+CWMODE=1",150);

  ATCommand("AT+CWJAP?",150);

  ATCommand("AT+CWJAP=\""+AP+"\",\""+PASS+"\"",15000); //Connecting to a wifi network
  Serial.println();

  ATCommand("AT+CWJAP?",150); //Checking if connected to any access point

  ATCommand("AT+CIFSR",300);  //Obtaining IP
  Serial.println();   

  period = 0.25 * 60000L;    //Duration( 0.25=15seconds )

}

void loop(){

  Serial.println("Sensor Data");
 
  //Looping for atleast 15 seconds before posting new data set.
  for( uint32_t tStart = millis();  (millis()-tStart) < period;  ){
    tempRead = (float(((analogRead(sensor1)*(5.0/1024))+0.5)*100))-58;
    gasRead = float(analogRead(sensor2)*(5.0/1024));
    Serial.println("temp="+String(tempRead)+"C  gasC="+String(gasRead)+"V");
    delay(200);
 }

  delay(500);
  Serial1.flush();
  //**Posting Data to Thingspeak Channel**
 
  POST="GET /update?api_key="+API_WR+"&field1="+String(tempRead)+"&field2="+String(gasRead);
  
  postLen = POST.length()+4;
  
  Serial.println("\n"+POST+"\n");
  
  ATCommand("AT+CIPMUX=1",1000);  //***************************************888888cipmux=0 or 1

  ATCommand("AT+CIPSTART=0,\"TCP\",\""+HOST+"\","+PORT,1000);

  ATCommand("AT+CIPSEND=0,"+String(postLen),100);
  
  ATCommand(POST,100);
  ATCommand("",5000);

  ATCommand("AT+CIPCLOSE",5000); //***************************************888888cipclose or cipclose=1

  
  Serial.print("  Successfully posted (below)sensors data to ThingSpeak ");
  Serial.println("temp="+String(tempRead)+"C  gasC="+String(gasRead)+"V"+"\n"  );

  delay(32000); 

  //**Reading latest crisp output uploaded to channel field from fuzzy inference system computation**
  POST_CrispFuzzyOP="GET /channels/"+ChID+"/fields/6/last?api_key="+API_RD+"&results=1";

  postLentc = POST_CrispFuzzyOP.length()+4;

  Serial.println(POST_CrispFuzzyOP+"\n");
  
  ATCommand("AT+CIPMUX=1",1000);  //***************************************888888cipmux=0 or 1

  ATCommand("AT+CIPSTART=0,\"TCP\",\"184.106.153.149\","+PORT,1000);
  ATCommand("AT+CIPSEND=0,"+String(postLentc),100);
  
  ATCommand(POST_CrispFuzzyOP,400);
  ATCommandwRespDo("",5000);   // important function()
  
  ATCommand("AT+CIPCLOSE",5000);

  
}



void ATResponse(){
  while(Serial1.available()){
    Serial.write(Serial1.read());
  }
  Serial.println();
  
}

void ATCommand(String AT,int Delay){
  Serial1.print(AT+"\r\n");
  delay(Delay);
  ATResponse();
  delay(50);
}

void RespSys(float h){
  if(h<=26.67){
     Serial.println("Env state 01 : Normal levels - temp / gas");
     noTone(piezoPin); // Mute piezo buzzer
     digitalWrite(led1,LOW);
     digitalWrite(led2,HIGH);
  }
  if (h>26.67 && h<=53.33){
     Serial.println("Env state 10 : High levels - temp / gas");
     tone(piezoPin,h*10);
     digitalWrite(led1,HIGH);
     digitalWrite(led2,LOW);
  }
  if(h>53.33){
     Serial.println("Env state 11 : Critically/Explosive levels - temp / gas");
     tone(piezoPin,h*15);
     digitalWrite(led1,HIGH);
     digitalWrite(led2,HIGH);        
  }
}

void ATResponseImplementor(){
  String recev="";
  if(Serial1.available()>0)               
    {
      delay(1000);                         
      while(Serial1.available())            
      {
        recev+=(char(Serial1.read()));       
      }                   
    }            
  String a = recev.substring(39,45);
  float h=a.toFloat();
  Serial.println(">>>> Fire/gas Leak Sense Value >>>>>\n");
  Serial.println(h);
  RespSys(h);
}

void ATCommandwRespDo(String AT,int Delay){
  Serial1.print(AT+"\r\n");
  delay(Delay);
  ATResponseImplementor();
  delay(50);
}

int find_text(String sr, String recd)      //function to search a Substring in Received msg
 {
    int foundpos = -1;
    for (int i = 0; (i < recd.length() - sr.length()); i++) 
    {
      if (recd.substring(i,sr.length()+i) == sr) 
      {
        foundpos = 1;
      }
    }
    return foundpos;
 } 




