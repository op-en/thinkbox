# Open Energy - Thinking inside a box

Waterproof sensor boxes for on-the-go IoT.


## Running

Copy the code in each file to a file on http://build.particle.io and run!

## Wiring

All pins on the photon/electron are 5V tolerant except A3 and DAC. So plug your 5V boards right in!

At the top of each file is documentation of where to connect the sensors.

## Backend

A node-red script unpacks the data and saves it in influxdb. See node-red.json.

#### Note

Timestamps are unix with millisecond precision, and these don't fit in a 32 bit uint. So we use a 64bit in instead. But there is no support for the sprintf parameter %llu (I guess they disabled it in the compiler), so I wrote a manual function to convert a 64bit int to it's string representation.
