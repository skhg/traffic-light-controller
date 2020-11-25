/**
 * Copyright 2020 Jack Higgins : https://github.com/skhg
 * All components of this project are licensed under the MIT License.
 * See the LICENSE file for details.
 */

#include <DHT.h>

#define TEMPSENSOR_RED_PIN D3
#define TEMPSENSOR_GREEN_PIN D4
#define DHT_SENSOR_TYPE DHT11

DHT TEMP_SENSOR_RED(TEMPSENSOR_RED_PIN, DHT_SENSOR_TYPE);
DHT TEMP_SENSOR_GREEN(TEMPSENSOR_GREEN_PIN, DHT_SENSOR_TYPE);

void setup() {
  Serial.begin(115200);

  TEMP_SENSOR_RED.begin();
  TEMP_SENSOR_GREEN.begin();
}

void loop() {
  delay(5000);

  Serial.println("Red Sensor");
  double hum = TEMP_SENSOR_RED.readHumidity();
  double temp = TEMP_SENSOR_RED.readTemperature();
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");

  Serial.println("---");

  Serial.println("Green Sensor");
  hum = TEMP_SENSOR_GREEN.readHumidity();
  temp = TEMP_SENSOR_GREEN.readTemperature();
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");
}
