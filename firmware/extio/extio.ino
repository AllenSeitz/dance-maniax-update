/* comment */

String inputBuffer;
String outputBuffer;

boolean foundSoftware = false;

void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  // reply to any messages sent
  if ( outputBuffer.length() > 0 )
  {
    Serial.print(outputBuffer);
    outputBuffer = String();
  }
  
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

    // process that packet
    if ( inputBuffer.length() >= 3 )
    {
      if (inputBuffer == "DMX") 
      {
        outputBuffer = String("OK!");
        foundSoftware = true;
      }
      else if ( inputBuffer.startsWith("PIN") )
      {
        outputBuffer = String("PON");
      }
      else
      {
        //outputBuffer = inputBuffer;
        outputBuffer = String("000");
        outputBuffer[0] = inputBuffer[0] + 1;
        outputBuffer[1] = inputBuffer[1] + 1;
        outputBuffer[2] = inputBuffer[2] + 1;
      }
      
      inputBuffer = String();
    }
  }
}
