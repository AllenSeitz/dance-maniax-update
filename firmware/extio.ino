/* comment */

String inputBuffer;
String outputBuffer;

boolean runningError = false;

void setup() 
{
  Serial.begin(9600);
  
  // declare input pins and output pins
  for ( int i = 2; i <= 21; i++ )
  {
    if ( i == 13 ) // do not hook up - not a normal pin
    {
      continue;
    }
    pinMode(i, INPUT_PULLUP);
  }
  pinMode(A5, INPUT_PULLUP);
  pinMode(A6, INPUT_PULLUP);
  pinMode(A7, INPUT_PULLUP);
  for ( int i = 24; i <= 53; i++ )
  {
    pinMode(i, OUTPUT);
  }  
  pinMode(A5, OUTPUT);
  pinMode(A6, OUTPUT);
  pinMode(A7, OUTPUT);
}

void loop() 
{  
  // video amp 
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent()
{
  while (Serial.available())
  {
    // keep listening until an entire packet (3 bytes) is read
    char inChar = (char)Serial.read(); 
    inputBuffer += inChar;

    if ( inputBuffer.charAt(0) == 'C' ) // pulse coin counter once
    {
      // TODO: coin counter is on pin 22
      inputBuffer = String();
    }
    else if ( inputBuffer.charAt(0) == 'L' ) // lockout signal engage
    {
      // TODO: lockout coil is on pin 23
      inputBuffer = String();
    }
    else if ( inputBuffer.charAt(0) == 'U' ) // unlock the coin mech
    {
      // TODO: lockout coil is on pin 23
      inputBuffer = String();
    }
    else if ( inputBuffer.charAt(0) == 'I' ) // please reply with input
    {
      prepareInputPacket();
      inputBuffer = String();
    }
    else if ( inputBuffer.charAt(0) == '1' ) // LED lights on the orbs
    {
      if ( inputBuffer.length() >= 4 )
      {
        for ( int i = 0; i < 8; i++ )
        {
          digitalWrite(i+30, (inputBuffer[1] & (1<<i)) != 0 ? HIGH : LOW);
          digitalWrite(i+38, (inputBuffer[2] & (1<<i)) != 0 ? HIGH : LOW);
          digitalWrite(i+46, (inputBuffer[3] & (1<<i)) != 0 ? HIGH : LOW);
        }
        inputBuffer = String();
      }
    }
    else if ( inputBuffer.charAt(0) == '2' ) // menu buttons and spot lights
    {
      if ( inputBuffer.length() >= 3 )
      {
        for ( int i = 0; i < 6; i++ )
        {
          digitalWrite(i+24, (inputBuffer[1] & (1<<i)) != 0 ? HIGH : LOW);
        }
        digitalWrite(A13, (inputBuffer[2] & (1<<0)) != 0 ? HIGH : LOW); // center spotlight (yellow)
        digitalWrite(A14, (inputBuffer[2] & (1<<1)) != 0 ? HIGH : LOW); // middle spotlight pair
        digitalWrite(A15, (inputBuffer[2] & (1<<2)) != 0 ? HIGH : LOW); // outer spotlight pair
        
        inputBuffer = String();
      }
    }
    else if ( inputBuffer.charAt(0) == 'D' ) // special initialization packet
    {
      if ( inputBuffer.length() >= 3 )
      {
        if (inputBuffer.startsWith("DMX"))
        {
          outputBuffer = String("OK!");
          inputBuffer = String();
          runningError = false;
        }
        else
        {
          outputBuffer = String("BAD");
          inputBuffer = String();
          runningError = true; // what are you sending me?
        }
      }
    }
    else
    {
      inputBuffer = String();
      runningError = true; // unrecognized packet type, are we out of sync?      
    }
    
    // did we create a response?
    if ( outputBuffer.length() > 0 )
    {
      Serial.print(outputBuffer);
      
      outputBuffer = String();
    }
  }
}

void prepareInputPacket()
{
  const int pins0[] = { A5, A6, 2, 3, 4, 5, 6, 7 };
  const int pins1[] = { 8, 9, 10, 11, 12, 13, 14, 15 };
  const int pins2[] = { 16, 17, 18, 19, 20, 21, A7, 12 }; // pin 12 is not hooked up

  outputBuffer = "BAD";
  if ( !runningError )
  {
    outputBuffer[0] = packThePins(pins0);
    outputBuffer[1] = packThePins(pins1);
    outputBuffer[2] = packThePins(pins2);
  }  
}

// a helper function for prepareInputPacket
unsigned char packThePins(const int pins[])
{
    unsigned char c = 0;
    for ( int i = 0; i < 8; i++ )
    {
        if ( digitalRead(pins[i]) == HIGH )
        {
            c |= 1 << i;
        }
    }
    return c;
}
