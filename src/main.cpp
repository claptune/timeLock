#include <ESPUI.h>
#include "ESPAsyncWebServer.h"
#include "arduino-timer.h"

// clarify variables
uint16_t SwitchElementId;
uint16_t SliderElementId;
uint16_t LabelId;
uint64_t SliderVal_min; // SliderValue in min
uint64_t SliderVal_ms; // SliderValue in ms
uint32_t update_interval_label = 1000; // update interval for timing label
static uint32_t nextTime;

// generating needed objects for timer
auto timer = timer_create_default();
auto ticks = timer.ticks(); 
uint64_t seconds_left;

// clarify Pins
const int RELAIS_PIN = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create Networks
IPAddress local_IP(192,168,1,2);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Replace with your network credentials
const char* ssid = "CHOOSE_YOUR_SSID";
const char* password = "CHOSSE_YOUR_PASSWORD";

void sliderCallback(Control* sender, int type)
{
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);

	// update SliderVal to current value
	SliderVal_min = (sender->value).toInt();
	SliderVal_ms = SliderVal_min*6e4; // convert to milisecond
	//SliderVal_ms = SliderVal_min*6e2; // for debugging

	// print actual SliderVal
	Serial.print("SliderVal in minutes:");
	Serial.println(SliderVal_min);
	Serial.print("SliderVal in milliseconds:");
	Serial.println(SliderVal_ms);
}

void setOffCallback()
{	
	// cancelling timer if needed
	timer.cancel();

	// deactivate pin
	Serial.print("deactivate...");
	digitalWrite(RELAIS_PIN, LOW);

	// set switch status to off
	if (bool(ESPUI.getControl(SwitchElementId)) == true) {
		ESPUI.updateSwitcher(SwitchElementId, false);
	}

	// set remaining time to 0
	ESPUI.updateLabel(LabelId, String(0.0));
}

bool setOffCallbackHandler(void *argument)
{
	setOffCallback();
	return false;
}

void OnOffSwitchCallback(Control* sender, int value)
{
    switch (value)
    {
    case S_ACTIVE:
        Serial.print("activate...");
		digitalWrite(RELAIS_PIN, HIGH);
		// if timer value is given, do automatic deactivation after time
		if (SliderVal_ms > 0) {
			// setting timer, if time expires calls setOff function
			timer.in(SliderVal_ms, setOffCallbackHandler);
		}
		else {
			ESPUI.updateLabel(LabelId, "infinity");
		}
        break;

    case S_INACTIVE:
		setOffCallback();
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}

void setup() {
	// declare needed pins
	pinMode(RELAIS_PIN, OUTPUT);

	// add serial support
	Serial.begin(115200);
	
	// set up wifi 
	WiFi.setSleep(false); //For the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)

	Serial.print("Setting soft-AP configuration ... ");
	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	Serial.print("Setting soft-AP ... ");
	Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");

	Serial.print("Soft-AP IP address = ");
	Serial.println(WiFi.softAPIP());

	// configure the UI
	SwitchElementId = ESPUI.addControl(ControlType::Switcher, "Start / Stopp (manual control if no time is given)", "On/Off", ControlColor::Peterriver, Control::noParent, &OnOffSwitchCallback);
	SliderElementId = ESPUI.addControl(ControlType::Slider, "Time [minutes]", "0", ControlColor::Alizarin, Control::noParent, &sliderCallback);
	ESPUI.addControl(Min, "", "0", None, SliderElementId);
	ESPUI.addControl(Max, "", "180", None, SliderElementId);
	LabelId = ESPUI.label("Time left [seconds]", ControlColor::Dark);
	
	// starting the UI
	ESPUI.begin("TimeLock");
}

void loop() {
	auto ticks = timer.tick();
	seconds_left = ticks/1e3; // convert miliseconds to seconds

	// Serial.print("seconds left");
	// Serial.println(seconds_left);
  
	if (millis() - nextTime >= update_interval_label && SliderVal_ms > 0)
	{
		// update next time
		nextTime += update_interval_label;
		ESPUI.updateLabel(LabelId, String(seconds_left));
	}
	
}
