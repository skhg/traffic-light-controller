# traffic-light-controller
The [_Ampelmännchen_](https://en.wikipedia.org/wiki/Ampelm%C3%A4nnchen) are a popular symbol from East Germany, which display on the pedestrian traffic lights at every street corner. There's even a Berlin-based [retail chain](https://www.ampelmann.de/) that's inspired by their design.

I came across a set of secondhand Ampelmann signals at a flea market, and I decided to make something fun with them.

<ANIMATION/VIDEO HERE>

## Overview

**Health Warning:** This project uses 220V mains power. I am not an electrician. Follow these instructions at your own risk.

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

The traffic light comes with 2 steel mounts, which are hollow. At the front, it has two doors which open to reveal the bulbs, reflectors and the transformers behind. These traffic bulbs are designed to run on 10.5V (30VA) as is printed on the transformer label. Behind the reflector, there's actually a lot of empty space. In a normal traffic light installation, all the control logic would be handled by an external box, but we want to have everything self-contained. So we will make the most of the space available.

The ESP8266 and the relay board are mounted onto a small wooden board, that itself is bolted to the back of the traffic light case.

The live and neutral wires coming from the 220V mains need to be split. Neutral connects to each of the transformers, as before. Live connects to two of the relays. Each of the relays then needs to be connected to neutral on both of the transformers. This completes the circuit, and when the relay is in it's "on" state, the light will come on.

The two temperature sensors are mounted on to identical mini-perfboards. They are attached to the upper edge of each reflector section with double-sided sticky tape, and connected to the main board with ribbon cable.

Once everything is hooked up, we can test it out, and close it up.

## Software

There are a few moving parts in the software here, so i've split this into a couple of sections.

### Overview

The software is intended to allow users on the same LAN to control the traffic light through a browser, without any prior authorisation needed. Security is not considered for this project. The browser shows the state of the system in real time. 

### Webapp

The webapp is a very simple responsive single-page application. It shows:
* Red on/off
* Green on/off
* Party mode on/off
* Temperature of the red and green enclosures
* Currently playing song information.

Code is under the [/webapp](/webapp) directory. No external dependencies are required since it's just relying on standard browser features, like [CSS transform](https://developer.mozilla.org/en-US/docs/Web/CSS/transform), [WebSockets](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket), and [XHR](https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/Using_XMLHttpRequest). You can preview the running application [here](http://jackhiggins.ie/traffic-light-controller/) although it won't control anything, since of course you're not on my LAN. If you visited it from the local network, at e.g. `http://traffic-light.local.lan` it would work in full.

Upon loading the page, the application makes a GET request to find the current status at `/api/status`, and then opens a WebSocket connection to the server on port `81`. Subsequent status updates will always come via the WebSocket, in order to keep multiple clients in sync.

On startup we also detect if the user is on a mobile or desktop device, to handle either touch events or click events. We actually use the `touchend` event to send commands to the server, because this performed more reliably on the iPhone X. Swiping up from the bottom of the screen to exit Safari was firing the `touchstart` event, making it impossible to exit the app without turning on the green light!

Finally, we want to reduce load on the server wherever possible. Remember the ESP8266 is [running](https://docs.zerynth.com/latest/reference/boards/nodemcu3/docs/) on an _80MHz_ processor with only about _50kB_ of RAM. It is NOT a beefy device. So when the browser is inactive, we disconnect the websocket. When the tab or browser reopens, we again check for status, and reconnect the WebSocket.

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
