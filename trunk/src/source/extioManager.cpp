// extioManager.cpp implements programming the serial port for DMX
// source file created by Allen Seitz 11/14/2013

#include "extioManager.h"
#include "inputManager.h"
#include "lightsManager.h"

#define ARDUINO_WAIT_TIME 4000
#define ARDUINO_TIMEOUT 8000

extern InputManager im;
extern LightsManager lm;

extioManager::extioManager()
{
	hSerial = INVALID_HANDLE_VALUE;
	comPort = -1;
	isConnected = false;
	isTalking = false;
	powerOnTime = 0;
	connectionState = 0;
	timeSinceLastLampUpdate = 0;
}

void extioManager::initialize()
{
	for ( int i = 0; i < PACKET_SIZE; i++ )
	{
		inputBuffer[i] = 0;
	}

//#ifdef DMXDEBUG
//	comPort = TRUSTED_COM_PORT - 1;
//#endif
}

bool extioManager::updateInitialize(UTIME dt)
{
	if ( !isConnected )
	{
		comPort++;
		if ( comPort < 10 )
		{
			char port[] = "\\\\.\\COM0";

			port[7] = comPort + '0';
			attemptConnection(port);
			connectionState = 1;
		}
		else
		{
			al_trace("Can't find the IO board on any com port.\r\n");
			comPort = -1;
			connectionState = 0;
			return false;
		}

		powerOnTime = 0;
	}
	else
	{
		powerOnTime += dt;

		if ( connectionState == 1 )
		{
			if ( powerOnTime >= ARDUINO_WAIT_TIME )
			{
				WriteData("DMX", PACKET_SIZE);
				connectionState = 2;
			}
		}
		else if ( connectionState == 2 )
		{
			if ( isPacketReady() )
			{
				ReadData(inputBuffer, PACKET_SIZE);
				if ( inputBuffer[0] == 'O' && inputBuffer[1] == 'K' && inputBuffer[2] == '!' )
				{
					isTalking = true;
					connectionState = 3;
					al_trace("Handshake accepted! Found the IO board on port %d.\r\n", comPort);
					WriteData("I", 1); // begin the input request loop
				}
				else
				{
					// abort - try another port
					powerOnTime = 0;
					isConnected = false;
					connectionState = 0;
					al_trace("The handshake was incorrect on port %d. Checking other ports.\r\n", comPort);
				}
			}
			else if ( powerOnTime >= ARDUINO_TIMEOUT )
			{
				// abort - try another port
				powerOnTime = 0;
				isConnected = false;
				connectionState = 0;
				al_trace("IO board timeout on port %d. Checking other ports.\r\n", comPort);
			}
		}
		else if ( connectionState == 3 )
		{
			al_trace("Do not call updateInitialize() after success!\r\n");
		}
	}

	return true;
}

bool extioManager::isReady()
{
	return isConnected && isTalking;
}

void extioManager::update(UTIME dt)
{
	if ( isConnected )
	{
		powerOnTime += dt;
		timeSinceLastLampUpdate += dt;
	}
	if ( !isReady() )
	{
		return;
	}

	// actually do work here
	if ( isPacketReady() )
	{
		ReadData(inputBuffer, PACKET_SIZE);
		im.processInputFromExtio(inputBuffer);
		WriteData("I", 1); // continue requesting input in a loop forever
	}
}

void extioManager::updateLamps()
{
	char b0 = 0, b1 = 0, b2 = 0;

	if ( timeSinceLastLampUpdate < 50 )
	{
		//al_trace("--STIFLED LAMP UPDATE-- too soon of an update.\r\n");
		return; // don't do that
	}
	timeSinceLastLampUpdate = 0;

	// write the LEDs
	WriteData("1", 1);
	for ( int i = 0; i < 8; i++ )
	{
		b0 |= lm.getLamp(30+i) ? 1 << i : 0;
		b1 |= lm.getLamp(38+i) ? 1 << i : 0;
		b2 |= lm.getLamp(46+i) ? 1 << i : 0;
	}
	WriteData(&b0, 1);
	WriteData(&b1, 1);
	WriteData(&b2, 1);

	//al_trace("LAMP %d %d %d\n", b0, b1, b2);

	// write the other lamps
	b0 = b1 = b2 = 0;
	WriteData("2", 1);
	for ( int i = 0; i < 6; i++ )
	{
		b0 |= lm.getLamp(24+i) ? 1 << i : 0;
	}
	b1 |= lm.getLamp(spotlightA) ? 1 << 0 : 0;
	b1 |= lm.getLamp(spotlightB) ? 1 << 1 : 0;
	b1 |= lm.getLamp(spotlightC) ? 1 << 2 : 0;
	WriteData(&b0, 1);
	WriteData(&b1, 1);
}

bool extioManager::attemptConnection(const char* port)
{
	isConnected = false;

	hSerial = CreateFile(port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if(hSerial==INVALID_HANDLE_VALUE)
	{
		//al_trace("The IO board isn't on %s\n", port);
		return false; // it's okay - we'll try another port
	}

	//If connected we try to set the comm parameters
	DCB dcbSerialParams = {0};

	if ( !GetCommState(hSerial, &dcbSerialParams) )
	{
		al_trace("extioManager: failed to GET serial parameters!");
		return false;
	}

	//Define serial connection parameters for the arduino board
	dcbSerialParams.BaudRate = CBR_9600;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if( !SetCommState(hSerial, &dcbSerialParams) )
	{
		al_trace("extioManager: failed to SET serial parameters!");
		return false;
	}

	isConnected = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// LOW LEVEL SERIAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

int extioManager::ReadData(unsigned char *buffer, unsigned int nbChar)
{
    //Number of bytes we'll have read
    DWORD bytesRead;
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    //Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->hSerial, &this->errors, &this->status);

    //Check if there is something to read
    if(this->status.cbInQue>0)
    {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        if(this->status.cbInQue>nbChar)
        {
            toRead = nbChar;
        }
        else
        {
            toRead = this->status.cbInQue;
        }

        //Try to read the require number of chars, and return the number of read bytes on success
        if(ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            return bytesRead;
        }
    }

    //If nothing has been read, or that an error was detected return -1
    return -1;
}

bool extioManager::WriteData(char *buffer, unsigned int nbChar)
{
    DWORD bytesSend;

    //Try to write the buffer on the Serial port
    if(!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0))
    {
        //In case it don't work get comm error and return false
        ClearCommError(this->hSerial, &this->errors, &this->status);

        return false;
    }
    else
        return true;
}

bool extioManager::isPacketReady()
{
    ClearCommError(this->hSerial, &this->errors, &this->status);

    return this->status.cbInQue >= PACKET_SIZE;
}