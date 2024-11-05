/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>

#define COMPRESSOR_RELAY_PIN 22
#define POWER_NTC_PIN 28
#define LED_STATUS_PIN 21

#define NTC_B_VALUE 3450
#define NTC_VOLTAGE 3.3

#define SETTEMP -18.0
#define HYSTERESIS 3

double readTemp(void) {

  // Power NTC sensor
  gpio_put(POWER_NTC_PIN, true);
  sleep_ms(500);

  // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
  const float conversion_factor = 3.3f / (1 << 12);
  uint16_t result = adc_read();
  printf("Raw value: 0x%03x, voltage: %f V\n", result,
         result * conversion_factor);

  // Use B-constant
  // ==============
  // http://en.wikipedia.org/wiki/Thermistor
  // R1 = (R2V - R2V2) / V2  R2= 10K, V = 3.3V,  V2 = adc * voltage/4094
  // T = B / ln(r/Rinf)
  // Rinf = R0 e (-B/T0), R0=10K, T0 = 273.15 + 25 = 298.15

  double Rinf = 10000.0 * exp(NTC_B_VALUE / -298.15);

  // V2 = adc * voltage/4096
  double v = NTC_VOLTAGE * (double)result / 4096;

  // R1 = (R2V - R2V2) / V2  R2= 10K, V = 5V,  V2 = adc * voltage/1024
  double resistance = (10000.0 * (NTC_VOLTAGE - v)) / v;

  // itemp = r;
  double temp = ((double)NTC_B_VALUE) / log(resistance / Rinf);
  // itemp = log(r/Rinf);
  temp -= 273.15; // Convert Kelvin to Celsius

  // avarage = testadc;
  /*  https://learn.adafruit.com/thermistor/using-a-thermistor
  avarage = (1023/avarage) - 1;
  avarage = 10000 / avarage;      // Resistance of termistor
  //temp = avarage/10000;           // (R/Ro)
  temp = 10000/avarage;
  temp = log(temp);               // ln(R/Ro)
  temp /= B;                      // 1/B * ln(R/Ro)
  temp += 1.0 / (25 + 273.15);    // + (1/To)
  temp = 1.0 / temp;              // Invert
  temp -= 273.15;
  */

  // Unpower NTC sensor
  gpio_put(POWER_NTC_PIN, false);

  printf("Temperature: %f C\n", temp);
  return temp;
}

int main() {
  stdio_init_all();
  printf("Fridge Controller, measuring Thermistor on GPIO26\n");

  // Init compressor pin
  gpio_init(COMPRESSOR_RELAY_PIN);
  gpio_set_drive_strength(COMPRESSOR_RELAY_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_dir(COMPRESSOR_RELAY_PIN, GPIO_OUT);
  gpio_set_pulls(COMPRESSOR_RELAY_PIN, true, false);
  gpio_put(COMPRESSOR_RELAY_PIN, false);

  // Init compressor pin
  gpio_init(POWER_NTC_PIN);
  gpio_set_drive_strength(POWER_NTC_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_dir(POWER_NTC_PIN, GPIO_OUT);
  gpio_set_pulls(POWER_NTC_PIN, true, false);
  gpio_put(POWER_NTC_PIN, false);

  // Init compressor pin
  gpio_init(LED_STATUS_PIN);
  gpio_set_drive_strength(LED_STATUS_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_dir(LED_STATUS_PIN, GPIO_OUT);
  gpio_set_pulls(COMPRESSOR_RELAY_PIN, true, false);
  gpio_put(LED_STATUS_PIN, false);

  adc_init();

  // Make sure GPIO is high-impedance, no pullups etc
  adc_gpio_init(26);
  // Select ADC input 0 (GPIO26)
  adc_select_input(0);

  // Work loop
  while (1) {

    // if (gpio_get(COMPRESSOR_RELAY_PIN)) {
    //   printf("off\n");
    //   gpio_put(COMPRESSOR_RELAY_PIN, false);
    // } else {
    //   printf("on\n");
    //   gpio_put(COMPRESSOR_RELAY_PIN, true);
    // }

    double currtemp = readTemp();
    if (currtemp < SETTEMP) {
      // Less then setpoint
      gpio_put(COMPRESSOR_RELAY_PIN, false);
      gpio_put(LED_STATUS_PIN, false);
    } else if (currtemp > (SETTEMP + HYSTERESIS)) {
      // Above (setpoint + hysteresis)
      gpio_put(COMPRESSOR_RELAY_PIN, true);
      gpio_put(LED_STATUS_PIN, true);
    }

    sleep_ms(10000);
  }
}