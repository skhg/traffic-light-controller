# traffic-light-controller
The [_Ampelmännchen_](https://en.wikipedia.org/wiki/Ampelm%C3%A4nnchen) are a popular symbol from East Germany, which display on the pedestrian traffic lights at every street corner. There's even a Berlin-based [retail chain](https://www.ampelmann.de/) that's inspired by their design.

I came across a set of secondhand Ampelmann signals at a flea market, and I decided to make something fun with them.

<ANIMATION/VIDEO HERE>

## Overview

I wanted to build something with a _Berlin aesthetic_, and would be fun for visitors to my home to interact with. Unfortunately our visitor numbers [declined severely this year](https://en.wikipedia.org/wiki/COVID-19_pandemic), but still hoping for a great reaction in 2021...?

The lights are switched on and off by solid-state relays. These can be a bit more expensive than an [optocoupler relay](https://www.amazon.de/-/en/gp/product/B078Q326KT/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) but don't make any [noise](https://youtu.be/FhQLq-eqfEc?t=2) as they switch, which is preferable for this project. To control the relays we need a WiFi connected, Arduino compatible board - so the trusty ESP8266 NodeMCU works perfectly here. 220V AC power is required anyway so there is no need for batteries or any low-power considerations.

The Arduino acts as a web application server (kind-of, see below), and also as an API server to control the lights themselves. Since we can have multiple users controlling the lights at once, each client subscribes to a websocket event stream, to get updates on the lights' status, without polling. More on this below too.

Since we have the option to run the lights in "party mode" where they pulse to a a rhythm, the arduino code also needs to run some time-based logic that can't block anything else the board is doing. This isn't too difficult, and there are some useful [resources](https://forum.arduino.cc/index.php?topic=503368.0) online which explain how to "[do multiple things at once](https://forum.arduino.cc/index.php?topic=223286.0)" with arduino.

## Materials Required

Required components:
 * 1 set of Ampelmann traffic lights
 * 1 [NodeMCU v3 ESP8266](https://www.amazon.de/-/en/gp/product/B074Q2WM1Y/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
 * 1 [Multiple Channel Solid-State Relay](https://www.amazon.de/-/en/gp/product/B07DK6P9CV/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1)
 * Assorted small [M2](https://www.amazon.de/-/en/gp/product/B07SGP8TWS/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1), [M3, M4](https://www.amazon.de/-/en/gp/product/B00B22VHPC/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1) nuts and bolts
 * 2 [DHT11](https://www.amazon.de/-/en/gp/product/B07Q3H24LL/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) temperature sensors
 * A [220V power cable](https://www.amazon.de/-/en/gp/product/B07NSSHP9S/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) capable of taking a few amps, that you can cut apart for inner wiring
 * 2 10kΩ [resistors](https://www.amazon.de/-/en/gp/product/B07Q87JZ9G/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
 * [Perfboard](https://www.amazon.de/-/en/gp/product/B07BDKG68Q/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
 * [JST Headers](https://www.amazon.de/YIXISI-Connector-JST-XH-Female-Adapter/dp/B082ZLYRRN/ref=sr_1_1_sspa?dchild=1&keywords=jst+kit&qid=1605108398&sr=8-1-spons&psc=1&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUFITU9OODc2TFg4MzEmZW5jcnlwdGVkSWQ9QTAxNTU4NzMyOVdJRFVETUhCV1Y3JmVuY3J5cHRlZEFkSWQ9QTAwODA5ODVQWFZCU00yNTJBSlYmd2lkZ2V0TmFtZT1zcF9hdGYmYWN0aW9uPWNsaWNrUmVkaXJlY3QmZG9Ob3RMb2dDbGljaz10cnVl)
 * [Ribbon Cable](https://www.amazon.de/-/en/gp/product/B076CLY8NH/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
 * A little [wooden board](https://www.amazon.de/-/en/gp/product/B07D76MKFY/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)
 * Double-sided sticky tape
 * A generic USB power supply
 * A power strip
 
Other necessary basic tools:
 * Soldering iron
 * Pliers
 * Screwdriver
 * Wire stripper
 * Power drill
 
## Construction

## Software

### Overview

### Webapp

### Webapp Deployment

### API

### Arduino Code

## Circuit diagram

## References

# to do
WiFi connection with REST endpoints to set states

POST /red/on
POST /red/off
POST /red/level/{0% ... 100%}

POST /green/on
POST /green/off
POST /green/level/{0% ... 100%}



send PWM signal to the dimmer board


check the input current from wall -> transformer
check the output current & voltage from transformer -> bulb
check what kind of transformer i need to buy, must be dimmable
