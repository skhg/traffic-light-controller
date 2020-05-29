# traffic-light-controller
Arduino controller for pedestrian traffic lights with dimming

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
