#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "struct-device.h"
#include "json/struct-json.h"
#include "json/decoder-i2t-json.h"
#include "config.h"
#include "devices.h"

/*** ACTION/PERFORMANCE FUNCTION *** This is the function where you will define the actions to be performed from the data retrieved from Tangle **/
void action(struct json *j)
{
	if (j->relative_timestamp < 300) // If the delay is greater than 300 seconds (5 minutes) it is possible that the Streams channel is not receiving any more messages
	{
		printf("\nACTIONS:	-- Relay 1 (GPIO26), 2 (GPIO27), 3 (GPIO14) and 4 (GPIO12) Available --\n");
		// React to Moisture sensor
		if (j->sensor[7].isEnable == true)
		{
			float moisture = atof(j->sensor[7].value[0]);
			printf("Reacting to Soil Moisture Sensor -> ");
			if (moisture > 2600) // If moisture sensor reports greater than 2600, Turn on water pump!
			{
				set_relay_GPIO(0, 1); // Put in HIGH RELAY 1
				printf("The plant is dry, turning on water pump! \n\n");
				udelay_basics(5000000); // Wait 5 seconds
				set_relay_GPIO(0, 0);	// Put in LOW RELAY 1
				printf("The plant has been watered for 5 seconds, turning off water pump! \n\n");
				printf("Waiting 1 minute in order to make sure we don't overwater the plant! \n\n");
				udelay_basics(60000000); // Wait 1 minute
				printf("1 minute has passed! \n\n");
			}
			else if (moisture < 2600)
			{
				printf("The soil is moist enough, do nothing!.\n\n");
			}
		}
	}
}
/***************************************************************/

void config(struct device *z)
{
	/* User assignments */
	z->id = id_name;

	z->addr = address;
	z->addr_port = port;

#ifdef MQTT
	z->user_mqtt = user;
	z->pass_mqtt = password;
	z->top = topic;
#endif

#ifdef MICROCONTROLLER
	z->ssid_wifi = ssid_WiFi;
	z->pass_wifi = pass_WiFi;
#endif

	z->isEnable_relay[0] = isEnable_Relay_1;
	z->isEnable_relay[1] = isEnable_Relay_2;
	z->isEnable_relay[2] = isEnable_Relay_3;
	z->isEnable_relay[3] = isEnable_Relay_4;

	z->interv = interval;
}

void initPeripherals(long *c)
{
	*c = 0; // Init counter

#ifdef SHELLPRINT
	welcome_msg(); // Printf in shell
#endif

	init_LEDs();
	init_i2c();
	init_SPI();
	init_relay();
}

void led_blinks(int led, int iter, int usec) // LED Blink function-> led: 0 Green LED, 1 Red LED - iter: iterations quantity - usec: delay time in usec
{
	int i;
	for (i = 0; i < iter; i++)
	{
		led_GPIO(led, 1);
		udelay_basics(usec);
		led_GPIO(led, 0);
		udelay_basics(usec);
	}
}

void connectNetwork(struct device *z, bool first_t)
{
#ifdef MICROCONTROLLER
	if (first_t)
	{
		while (!connectAttempt(z->ssid_wifi, z->pass_wifi)) /* Attempt to connect to the network via WiFi, in RaspberryPi only check connection to the network. */
		{
			led_blinks(0, 1, 600000); // Blink in green GREEN - ERROR 0 (No WiFi connection);
			led_blinks(1, 1, 600000); // Blink in green RED - ERROR 0 (No WiFi connection);
		}
	}
#endif
	if (!init_socket(z->addr, z->addr_port, z->user_mqtt, z->pass_mqtt, first_t)) /* Check Endpoint */
	{
		udelay_basics(100000);
		led_blinks(1, 3, 70000); // Blink in green RED - ERROR 1 (Bad connection with the endpoint);
	}
}

bool get_data_tangle(char *js, struct device *z, long *c)
{
	++(*c);
	d_collect_msg(c);

	bool b_socket = get_json(js, z->addr, z->addr_port, z->top, z->user_mqtt, z->pass_mqtt, z->interv);
	if (b_socket)
	{
		led_blinks(0, 2, 60000); // Blink in green LED;
		print_json(js);
	}
	else
		led_blinks(1, 3, 70000); // Blink in green RED - ERROR 1 (Bad connection with the endpoint);

	return b_socket;
}

void decode_json(char *js, struct json *j)
{
	printf("Decoding I2T Json...");
	if (recover_json(js, j))
	{
		printf("		Decoding completed successfully\n\nAvailable Variables in Device '%s' (according to last Json received):\n", j->id);
		for (int i = 0; i < MAX_SENSORS; i++)
		{
			if (j->sensor[i].isEnable)
			{
				printf("    - Sensor[%d] -> %s    -- ", i, j->sensor[i].id);
				for (int k = 0; k < j->sensor[i].num_values; k++)
				{
					printf("'%s.%s' - ", j->sensor[i].id, j->sensor[i].name[k]);
				}
				printf("\n");
			}
		}
		j->relative_timestamp = 12;
		printf("        -- Data published in Tangle %ld seconds ago -- (Harcoded value, this feature is not available yet)\n", j->relative_timestamp);
	}
	else
		printf("\nThe Json is not I2T format, please use the -Json Iot2Tangle format- to decode\n");
}

void clear_data(struct json *j)
{
	for (int i = 0; i < MAX_SENSORS; i++)
	{
		j->sensor[i].isEnable = false;
		sprintf(j->sensor[i].id, " ");
		j->sensor[i].num_values = 0;
		for (int k = 0; k < MAX_VALUE; k++)
		{
			sprintf(j->sensor[i].name[k], " ");
			sprintf(j->sensor[i].value[k], " ");
		}
	}
}

void t_delay(long d, long l)
{
	if (l >= d) /* To prevent crashes */
		l = d;
	udelay_basics((d - l) * 1000000); /* Time set by user  minus  loss time by operation */
}

long take_time()
{
	return take_time_basics();
}
