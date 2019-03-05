# Automated Irrigating System [![Build Status](https://travis-ci.org/thangktran/Automated-Irrigating-System.svg?branch=master)](https://travis-ci.org/thangktran/Automated-Irrigating-System)

A fully atomated irrgating system with hardware temperature monitoring and REST API.

## Features
- Multiple water outputs.
- Configurable Water-Schedule (how often a plant should be water per day, time to water, which week day to water, amount of water in ml).
- REST API to get temperature information, water tank status or to manually control the target and amount of water output.
- Durable and Consistent Water-Schedule-Store (in case of power shortage, crash).
- Data serialization between Arduino Mega and ESP8266.
- Access to NTP Server.
- Precise water volume control.
- Simple Pre-water-supply-empty and water-supply-empty sensor.

## Requirements
### Microcontroller
```
- Arduino Mega 2560 (or similar).
- ESP8266 (Any version).
```

### Electronics
```
- Solenoid valves.
- Opto-coupled relays.
- 1N4007 diodes (Flyback diodes).
- NTC thermistor.
- Buck converters.
- Pump.
```

### Software
```
Arduino IDE.
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)
