// Arduino animatronics controller with ultrasonic triggers and 8 relay outputs
// Uses some code from example at https://create.arduino.cc/projecthub/abdularbi17/ultrasonic-sensor-hc-sr04-with-arduino-tutorial-327ff6

// Supports up to 2 sensors, ultrasonic rangefinder or ir beam break and up to 8 relays in 2 separate zones.

// pin names
#define echoPin1 2  // To sensor 1 signal pin if IR, to echo pin if ultrasonic (blue wire)
#define trigPin1 3  // To sensor 1 trigger pin if ultrasonic, not used for IR (green wire)
#define echoPin2 4  // To sensor 2 signal pin if IR, to echo pin if ultrasonic
#define trigPin2 5  // To sensor 2 trigger pin if ultrasonic, not used for IR
#define relay1 6    // To relay 1 trigger
#define relay2 7    // To relay 2 trigger
#define relay3 8    // To relay 3 trigger
#define relay4 9    // To relay 4 trigger
#define relay5 10   // To relay 5 trigger
#define relay6 11   // To relay 6 trigger
#define relay7 12   // To relay 7 trigger
#define relay8 13   // To relay 8 trigger

// sensors
#define sensorCount 2    // Total sensors
#define sensorPins 4     // Number of pins for sensor input, sensorCount * 2
#define sensorDisable 0  // Sensor not installed
#define sensorUS 1       // Ultrasonic range sensor
#define sensorIR 2       // IR beam sensor

// relay zone names, should be as many zones as possible sensors
#define relayCount 8    // Total relays
#define zone1 0         // Relay in zone 1 controlled by sensor 1
#define zone2 1         // Relay in zone 2 controlled by sensor 2
#define relayDisable 2  // Relay not used / connected

#define triggerDelay 60000  // How long to wait between sensor trips

// defines variables
struct relay {
  int zone;                     // zone relay is in (relayDisable if not used)
  int pin;                      // relay trigger pin
  int delayTime;                // delay until activation after triggered in ms
  // end configurable values
  unsigned long activationTime; // when to trigger 
};
relay relays[relayCount] =  // Relay config
{
  {zone1,        relay1, 0},
  {zone1,        relay2, 30000},
  {zone1,        relay3, 0},
  {zone1,        relay4, 0},
  {relayDisable, relay5, 0},
  {relayDisable, relay6, 0},
  {relayDisable, relay7, 0},
  {relayDisable, relay8, 0}
};
struct sensor {
  int conf;               // what type of sensor (IR, US, disabled?)
  int echo;               // echo pin
  int trigger;            // trigger pin
  // end configurable values
  int backgroundDistance; // for US, how far to background
  int state;              // triggered / clear?
  unsigned long previousMillis;     // when last triggered
};
sensor sensors[sensorCount] =   // Sensor configuration
{
  {sensorUS,      echoPin1, trigPin1}, // sensor 1
  {sensorDisable, echoPin2, trigPin2}  // sensor 2
};


int getDistance(int which) {
  long duration; // variable for the duration of sound wave travel
  int distance; // variable for the distance measurement
  // Clears the trigger condition
  digitalWrite(sensors[which].trigger, LOW);
  delayMicroseconds(2);
  // Sets the trigger HIGH (ACTIVE) for 10 microseconds
  digitalWrite(sensors[which].trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensors[which].trigger, LOW);
  // Reads the echoPin1, returns the sound wave travel time in microseconds
  duration = pulseIn(sensors[which].echo, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  return distance;
}

void setup() {
  int accumDistance;
  // set up relay pins and turn relays off
  for(int i = 0; i < relayCount; i++)
  {
    if(relays[i].zone != relayDisable)
    {
      pinMode(relays[i].pin, OUTPUT);
      digitalWrite(relays[i].pin, HIGH);
    }
  }  
  // Set up sensors
  for(int i = 0; i < sensorCount; i++)
  {
    if(sensors[i].conf == sensorUS)
    {
      pinMode(sensors[i].trigger, OUTPUT);  // Sets the trigger pin as OUTPUT
      pinMode(sensors[i].echo, INPUT);      // Sets the echo pin as INPUT
      // calculate distance to background, take 10 measurements and average
      accumDistance = 0;
      for(int j = 0; j < 10; j++)
      {
        accumDistance += getDistance(i);
      }
      sensors[i].backgroundDistance = accumDistance / 10;
    }
    else if(sensors[i].conf == sensorIR)
    {
      pinMode(sensors[i].echo, INPUT);
      digitalWrite(sensors[i].echo, HIGH);  // turn the pullup on
    }
  }
}

void setTriggers(int zone, unsigned long when)
{
    // turn on
  for(int i = 0; i < relayCount; i++)
  {
    if(relays[i].zone == zone)
    {
      relays[i].activationTime = when + relays[i].delayTime;
    }
  }
}

void triggerRelays()
{
  unsigned long currentMillis = millis();
  for(int i = 0; i < relayCount; i++)
  {
    if((relays[i].activationTime != 0) && (currentMillis >= relays[i].activationTime))
    {
      relays[i].activationTime = 0;
      digitalWrite(relays[i].pin, LOW);
    }
  }
  delay(250); // leave on for quarter of a second
  for(int i = 0; i < relayCount; i++)
  {
    digitalWrite(relays[i].pin, HIGH);
  }
}

void loop() {
  int distance, distance1, distance2, state;
  bool triggered = false;
  unsigned long currentMillis = millis();
  for(int i = 0; i < sensorCount; i++)
  {
    if(sensors[i].conf == sensorUS)
    {
      distance1 = getDistance(i);
      distance2 = getDistance(i);
      distance = (distance1 + distance2) / 2;
      if((distance >= (sensors[i].backgroundDistance + 20)) || (distance <= (sensors[i].backgroundDistance - 20)))
      {
        sensors[i].backgroundDistance = distance;            
        triggered = true;
      }
    }
    if(sensors[i].conf == sensorIR)
    {
      state = digitalRead(sensors[i].echo);
      if(state != sensors[i].state)
      {
        sensors[i].state = state;
        triggered = true;
      }
    }
    if(triggered)
    {
      triggered = false;      
      if((sensors[i].previousMillis + triggerDelay) < currentMillis)
      {
        setTriggers(i, currentMillis);
        sensors[i].previousMillis = currentMillis;
      }
    }    
  }
  triggerRelays();
}
