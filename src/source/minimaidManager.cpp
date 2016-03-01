// minimaidManager.h implements HID support for a certain custom IO board
// source file created by Allen Seitz 02/07/2016

#include "minimaidManager.h"
#include "inputManager.h"
#include "lightsManager.h"

extern InputManager im;
extern LightsManager lm;

struct mm_input {                                                              
    uint8_t report_id;                                                         
    uint8_t dip_switches;                                                      
    uint32_t jamma;                                                            
    uint8_t ext_in;                                                            
};                                                                             

struct mm_output {                                                             
    uint8_t report_id;                                                         
    uint8_t ext_output;
    uint8_t lightsA;
    uint8_t lightsB;
    uint8_t lightsC;
    uint8_t lightsD;
    uint8_t blue_led;                                                          
    uint8_t kbd_enable;                                                        
    uint8_t aux_flags;                                                         
}; 

// four molexes fit on the minimaid, each with up to 8 pins
// the menu buttons go on ext_output
int minimaidUpperLED[8] = { lampBlue0, lampRed0, lampBlue1, lampRed1, lampBlue0+1, lampRed0+1, lampBlue1+1, lampRed1+1 };
int minimaidLeftLED[8] = { lampBlue2a, lampRed2a, lampBlue2b, lampRed2b, lampBlue3a, lampRed3a, lampBlue3b, lampRed3b };
int minimaidRightLED[8] = { lampBlue2a+1, lampRed2a+1, lampBlue2b+1, lampRed2b+1, lampBlue3a+1, lampRed3a+1, lampBlue3b+1, lampRed3b+1 };
int minimaidSpotLamps[8] = { spotlightA, spotlightB, spotlightC, NULL, NULL, NULL, NULL, NULL };
//int minimaidMenuLamps[8] = { lampRight, lampLeft, lampStart, lampRight+1, lampLeft+1, lampStart+1, NULL, NULL };
int minimaidMenuLamps[8] = { NULL, NULL, lampRight, lampStart, lampLeft, lampRight+1, lampStart+1, lampLeft+1 };

minimaidManager::minimaidManager()
{
	handle = NULL;
	powerOnTime = 0;
	timeSinceLastLampUpdate = 0;
}

void minimaidManager::initialize()
{
	// enumerate every hid device in the system, look for a "Minimaid", and try to open it
	struct hid_device_info* myDevices = hid_enumerate(0, 0); // vid = 0, pid = 0, matches everything 	
	struct hid_device_info* p = myDevices;
	int numDevice = 1;
	while ( p != NULL )
	{
		al_trace("---------------------------------------------------------\n");
		al_trace("DEVICE #%d = %ls\n", numDevice, p->product_string);
		al_trace("VID: %d, PID: %d\n", p->vendor_id, p->product_id);
		al_trace("PATH: %s\n", p->path);
		al_trace("---------------------------------------------------------\n");

		// since there can me multiple "devices" under Windows for a single IO board, try to open them all until one works
		if ( handle == NULL && lstrcmpW(p->product_string, L"Minimaid JAMMA IO Board") == 0 )
		{
			handle = hid_open_path(p->path);
		}

		numDevice++;
		p = p->next;
	}
	hid_free_enumeration(myDevices);

	// this would be the simpler way to open a Minimaid, except for the duplicate devices problem
	//handle = hid_open(0xBEEF, 0x5730, NULL);

	// Read the Manufacturer String - sample code, not used for anything
	if ( handle != NULL )
	{
		wchar_t wstr[256] = L"";

		int res = hid_get_manufacturer_string(handle, wstr, 256);
		al_trace("MINIMAID DETECTED: Manufacturer = %ls\n", wstr);
	}
}

bool minimaidManager::updateInitialize(UTIME dt)
{
	// easy. if initialize didn't find it right away, then it's not plugged in
	if ( handle == NULL )
	{
		return false;
	}
	return true;
}

bool minimaidManager::isReady()
{
	return handle != NULL;
}

void minimaidManager::update(UTIME dt)
{
	if ( !isReady() )
	{
		return;
	}
	powerOnTime += dt;
	timeSinceLastLampUpdate += dt;

	// read input from the hid
}

void minimaidManager::updateLamps()
{
	static int minUpdateTime = 50; // milliseconds minimum between updates sent to the board
	char b0 = 0, b1 = 0, b2 = 0;

	if ( timeSinceLastLampUpdate < minUpdateTime )
	{
		//al_trace("--STIFLED LAMP UPDATE-- too soon of an update.\r\n");
		return; // don't do that
	}
	timeSinceLastLampUpdate = 0;

	// prepare the output structure - I'm not sure what most of thse do to be honest
	struct mm_output out;
	out.report_id = 0;
	out.ext_output = 0; // set in the next code block
	out.blue_led = 0;
	out.aux_flags = 0; // called "hax" in the original api? what for?
	out.kbd_enable = 1; // might be a good idea to see what the bindings are, and if that would be easier?
	out.lightsA = out.lightsB = out.lightsC = out.lightsD = 0; // set in the next code block

	// set lamp bits!
	for ( int i = 0; i < 8; i++ )
	{
		out.lightsA |= lm.getLamp(minimaidUpperLED[i]) ? 1 << i : 0;
		out.lightsC |= lm.getLamp(minimaidLeftLED[i]) ? 1 << i : 0;
		out.lightsB |= lm.getLamp(minimaidRightLED[i]) ? 1 << i : 0;
		if ( minimaidSpotLamps[i] != 0 )
		{
			out.lightsD |= lm.getLamp(minimaidSpotLamps[i]) ? 1 << i : 0; // these go on the short space
		}
		if ( minimaidMenuLamps[i] != 0 )
		{
			out.ext_output |= lm.getLamp(minimaidMenuLamps[i]) ? 1 << i : 0; // these go on the special side add-on
		}
	}

	/* dirty testing
	static int numUpdates = 0;
	int mmm = numUpdates % 8;
	out.lightsA = 1 << mmm;
	out.lightsB = 1 << mmm;
	out.lightsC = 1 << mmm;
	out.lightsD = 1 << mmm;
	numUpdates++;
	minUpdateTime = 1000;
	//*/

	//al_trace("LAMP %d\n", out.lightsA);

	// move the mm_output struct into an array of chars for hid_write
	unsigned char outstream[128];
	memcpy_s(&outstream, 128, &out, sizeof(mm_output));
	hid_write(handle, outstream, sizeof(mm_output));
}