#ifndef DHTSensor_Adafruit_h
#define DHTSensor_Adafruit_h

class DHTSensor_Adafruit : public ESP8266Controller
{

public:
	// sensor read interval = 1s
	int interval = 1000;
	bool isCelcius = true;

	// store current time
	unsigned long currentMillis = millis();
	// store last time sensor reading was taken
	unsigned long previousMillis = 0;

	// time when first entered loop() function for this object
	unsigned long startTime = 0;

public:
	DHTSensor_Adafruit(char *nam, uint8_t _pin, uint8_t capCount, int start_address) : ESP8266Controller(nam, _pin, capCount, start_address)
	{
		DEBUG_PRINTLN("DHTSensor_Adafruit::ESP8266Controller");

		pinMode(pin, INPUT);

		strcpy(capabilities[0]._name, "Humidity");
		capabilities[0]._value_min = 0;
		capabilities[0]._value_max = 100;
		capabilities[0]._value = 0;

		strcpy(capabilities[1]._name, "Temperature");
		capabilities[1]._value_min = 0;
		capabilities[1]._value_max = 1000;
		capabilities[1]._value = 0;

		strcpy(capabilities[1]._name, "Interval");
		capabilities[1]._value_min = 5;
		capabilities[1]._value_max = 300;
		capabilities[1]._value = 5;

		strcpy(capabilities[1]._name, "Celcius");
		capabilities[1]._value_min = 0;
		capabilities[1]._value_max = 1;
		capabilities[1]._value = 1;
	}

public:
	virtual void loop();
};

void DHTSensor_Adafruit::loop()
{

	// check to see if it's time to read the DHT pin
	currentMillis = millis();

	// update EEPROM if capabilities changed
	if (currentMillis - lastEepromUpdate > eeprom_update_interval)
	{
		// DEBUG_PRINTLN();

		lastEepromUpdate = millis();
		// DEBUG_PRINT("[MAIN] Free heap: ");
		// Serial.println(ESP.getFreeHeap(), DEC);

		// save if required
		if (eepromUpdatePending == true)
		{

			DEBUG_PRINT("saveCapabilities pin ");
			DEBUG_PRINTLN(pin);
			saveCapabilities();
		}
	}
}

#endif
