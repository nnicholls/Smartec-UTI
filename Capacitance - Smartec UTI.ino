
//GLOBAL BOOL VARIABLES

//UTI DIO LINES
bool FastMode = false;
bool HighRange = true;
bool PowerDown = false;
bool Shift = false;

//UTI SELECTION LINES
bool SL1 = false;
bool SL2 = false;
bool SL3 = true;
bool SL4 = true;

//MUX DIO LINES
bool ShiftStatus = false;
bool ClearStatus = false;
bool ClockStatus = false;
bool SerialStatus = false;

//Timing, Averaging, Channel Count Constants
const int ClockDelay = 50;
const int SampleAveraging = 1;
const int Channels = 9;

//Allocate memmory for tables
unsigned long MeasuredPeriod = 0;
unsigned long MeasurementHistory[Channels][SampleAveraging];
float AverageMeasurement[Channels];

//CHANNEL TO REFERENCE CAPACITANCE MAPPING

//Note OffsetChannel and Reference channel are zero indexed
const int OffsetChannel = 0 + 1;
const int ReferenceChannel = 5 + 1;

//2.2 pF reference capacitor
const float ReferenceValue = 2.2;

//PIN ASSIGNMENTS

//MULTIPLEXER DIGITAL PINS
const int ClearPin = 2;
const int ShiftPin = 3;
const int SerialPin = 4;
const int ClockPin = 5;

//UTI DIGITAL PINS
const int FastModePin = 6;
const int PowerDownPin = 7;
const int SignalPin = 8;
const int HighRangePin = 9;

//UTI SELECTION LINES
int const SL1Pin = 10;
int const SL2Pin = 11;
int const SL3Pin = 12;
int const SL4Pin = 13;



void setup() {
  
  // open the serial port at 9600 bps:
  Serial.begin(9600);
  
  Serial.println("");
  Serial.println("Capacitance Sensor IC - UTI");

  //print debug message
  Serial.println("");
  Serial.println("28/10/2018 - 15:55");
  Serial.println("Pin 6 - 2.2 pF");
  Serial.println("Pin 7 - 6.6 pF");
  Serial.println("All other outputs floating"); 
  Serial.println("");
  
  Serial.println("Starting Up");

  //test leds
  Serial.println("Pin Cylce Test");
  PinCycleTest();

  //Set pin modes for the multiplexer digital lines
  pinMode(ShiftPin, INPUT);
  pinMode(ClearPin, OUTPUT);
  pinMode(SerialPin, OUTPUT);
  pinMode(ClockPin, OUTPUT);

  //Set pin modes for the UTI capacitance sensor - mode selection lines
  pinMode(SL1Pin, OUTPUT);
  pinMode(SL2Pin, OUTPUT);
  pinMode(SL3Pin, OUTPUT);
  pinMode(SL4Pin, OUTPUT);

  //Set pin modes for the UTI capacitance sensor - DIO lines
  pinMode(FastModePin, OUTPUT);
  pinMode(PowerDownPin, OUTPUT);
  pinMode(SignalPin, OUTPUT);
  pinMode(HighRangePin, OUTPUT);


  //Set DIO pinouts on UTI chip, using the initialised bool values
  //Fast, PowerUpDown, HighRange
  WriteCapSensorDIO();

  //Initialise the MUX lines
  ShiftStatus = digitalRead(ShiftPin);
  digitalWrite(ClearPin, ClearStatus);
  digitalWrite(ClockPin, ClockStatus);
  digitalWrite(SerialPin, SerialStatus);
  
  //SL1, SL2, SL3, SL4
  SetSelectionLines();

  //Turn off UTI chip
  PowerOffCapSensor();

  delay(100);

  //Turn on UTI chip
  PowerOnCapSensor;

}




void loop() {

  Serial.println("");
  Serial.println("");
  
  ClearMUX();
  
  
  unsigned int EachChannel = 0;
  unsigned int EachMeasurement = 0;
  
  unsigned long SUM;
  
  bool ShiftValues[Channels];

   
  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    SUM = 0;
    
    for(EachMeasurement=0; EachMeasurement<SampleAveraging; EachMeasurement++){
      MeasuredPeriod = ReadSinglePeriod();
      MeasurementHistory[EachChannel][EachMeasurement] = MeasuredPeriod;
      SUM = SUM + MeasuredPeriod;
    }
    
    AverageMeasurement[EachChannel] = (float) SUM / float(SampleAveraging);
    ReadShift();
    ShiftValues[EachChannel] = Shift;

    //Move to the next channel
    TickClock(ClockDelay);  
  }

  Serial.println("");
  Serial.println("***** SHIFT LINE OUTPUT (Single) *****");
  Serial.println("");
  
  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    //Print channel number
    Serial.print("Channel - " + String (EachChannel + 1) + " - ");
    //Print average time
    Serial.println("Shift - " + String (ShiftValues[EachChannel],BIN));
  }


  Serial.println("");
  Serial.println("***** TIME OUTPUT (Single) *****");
  Serial.println("");
  
  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    //Print channel number
    Serial.print("Channel - " + String (EachChannel + 1) + " - ");
    //Print average time
    Serial.println(String(MeasurementHistory[EachChannel][0]) + " us - ");
  }

  Serial.println("");
  Serial.println("***** TIME OUTPUT (Average) *****");
  Serial.println("");
  
  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    //Print channel number
    Serial.print("Channel - " + String (EachChannel + 1) + " - ");
    //Print average time
    Serial.println(String(AverageMeasurement[EachChannel]) + " us - ");
  }

  Serial.println("");
  Serial.println("***** pF OUTPUT (Single) *****");
  Serial.println("");

  unsigned long OffsetTime = MeasurementHistory[OffsetChannel][0];
  unsigned long ReferenceTime = MeasurementHistory[ReferenceChannel][0];
  
  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    //Print channel number
    Serial.print("Channel - " + String (EachChannel + 1) + " - ");
    //Print average pF value
    Serial.println(String( CapacitanceValue(OffsetTime, ReferenceTime, MeasurementHistory[EachChannel][0], ReferenceValue)));
    
  }

  Serial.println("");
  Serial.println("***** pF OUTPUT (Average) *****");
  Serial.println("");

  for(EachChannel=0; EachChannel<Channels; EachChannel++){
    //Print channel number
    Serial.print("Channel - " + String (EachChannel + 1) + " - ");
    //Print average pF value
    Serial.println(String( CapacitanceValueFloat(OffsetTime, ReferenceTime, AverageMeasurement[EachChannel], ReferenceValue)));
    
  }

}


float CapacitanceValueFloat(unsigned long OffsetTimeArg, unsigned long ReferenceTimeArg, float MeasuredTimeArg, float ReferenceValueArg){

  float M = 0;

  float MtSUBOt = MeasuredTimeArg - float(OffsetTimeArg);
  long RftSUBOt = long(ReferenceTimeArg) - long(OffsetTimeArg);

  //M = (float) (MeasuredTimeArg - OffsetTimeArg)/(ReferenceTimeArg - OffsetTimeArg);
  M = MtSUBOt/float(RftSUBOt);
  return ReferenceValue * M;
    
}


float CapacitanceValue(unsigned long OffsetTimeArg, unsigned long ReferenceTimeArg, unsigned long MeasuredTimeArg, float ReferenceValueArg){

  float M = 0;

  long MtSUBOt = long(MeasuredTimeArg) - long(OffsetTimeArg);
  long RftSUBOt = long(ReferenceTimeArg) - long(OffsetTimeArg);

  //M = (float) (MeasuredTimeArg - OffsetTimeArg)/(ReferenceTimeArg - OffsetTimeArg);
  M = float(MtSUBOt)/float(RftSUBOt);
  return ReferenceValue * M;
  
}

void PinCycleTest(){

  int EachPin=0;

  //Turn all LED off
  for(EachPin=2; EachPin<= 13; EachPin++){
    pinMode(EachPin,OUTPUT);
    digitalWrite(EachPin,0);
  }

  //Flash each pin for 250 ms
  for(EachPin=2; EachPin<= 13; EachPin++){
    digitalWrite(EachPin,1);
    delay(250);
    digitalWrite(EachPin,0);
  }

  
  
}


void ReadShift(){

  int ShiftValue = 0;

  ShiftValue = digitalRead(ShiftPin);

  Shift = (ShiftValue == 1);
}

void ClearMUX(){

  //Serial.println("Clear - Set MUX to Output 1");

  //ClearPin = 0 is Clear, ClearPin = 1 is not clear
  digitalWrite(ClearPin, 1);  
  digitalWrite(SerialPin, 0);
  TickClock(ClockDelay);
  
  //Set Clear Active
  digitalWrite(ClearPin, 0);
  TickClock(ClockDelay);

  //Disable Clear
  digitalWrite(ClearPin, 1);
  TickClock(ClockDelay);

  digitalWrite(SerialPin, 1);
  TickClock(ClockDelay);

  digitalWrite(SerialPin, 0);
  
}

void TickClock(unsigned long TickLength) {

  pinMode(ClockPin, OUTPUT);
  
  digitalWrite(ClockPin, 1);
  delay(TickLength);
  digitalWrite(ClockPin, 0);
  delay(TickLength);
  
}

void SetSelectionLines() {

  digitalWrite(SL1Pin, SL1);
  digitalWrite(SL2Pin, SL2);
  digitalWrite(SL3Pin, SL3);
  digitalWrite(SL4Pin, SL4);
}


unsigned long ReadSinglePeriod() {
  
  //PowerOffCapSensor();
  //WriteCapSensorDIO();

  pinMode(SignalPin, INPUT);
  
  if (PowerDown==true){
    PowerOnCapSensor();
  }
   
  //Serial.println("Reading Measurement Period");
  return pulseIn(SignalPin, HIGH);

}

void PowerOffCapSensor() {

  //Serial.println("Powering Down Capacitance Sensor IC");
  pinMode(PowerDownPin, OUTPUT);
  digitalWrite(PowerDownPin, 0);

  PowerDown = true;
}

void PowerOnCapSensor() {
  
  //Serial.println("Powering Up Capacitance Sensor Chip");
  pinMode(PowerDownPin, OUTPUT);
  digitalWrite(PowerDownPin, 1);

  PowerDown = false;
}

void WriteCapSensorDIO() {

  //Serial.println("Writing Capacitance Sensor IC - Digital IO");

  pinMode(FastModePin, OUTPUT);
  pinMode(PowerDownPin, OUTPUT);
  pinMode(HighRangePin, OUTPUT);

  digitalWrite(FastModePin, FastMode);
  digitalWrite(PowerDownPin, PowerDown);
  digitalWrite(HighRangePin, HighRange);
  
}

void ReadCapSensorDIO() {

  //Serial.println("Reading Capacitance Sensor IC - Digital IO");

  FastMode = digitalRead(FastModePin);
  PowerDown = digitalRead(PowerDownPin);
  HighRange = digitalRead(HighRangePin);

}

