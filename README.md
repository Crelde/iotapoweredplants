# Iota Powered Plants
Welcome to my Hackathon entry for the [iot2tangle hackathon](https://hackathon.iot2tangle.io/) in the Smart Agriculture category!

## Purpose
I set out to create a project to monitor and control my basil plants! This is just a small scale project, but the idea is to illustrate how this setup could be used in for example an Indoor/Vertical Farming plant. There are a number of reasons why this would be a real benefit for businesses in this segment:
* There is a lot of datapoints to monitor the production.
* It could make production more autonomous.
* It would be possible to provide **tamperproof data** about the plants to end-consumers since the data is on the Iota tangle.

There might be some differences to my basil plant and indoor farming plants. Many indoor farming plants doesn't use soil, but instead of the soil sensor that I am showcasing it could just as well be a pH sensor.

## Component overview
Here I have sketched the project setup, to get an initial understanding of the base components that make up the project!
![](https://i.imgur.com/FTKDHhv.png)

This repository consists of a few different code projects that make up the entirety of my entry.

# Sensor data
For my IoT datapoints I've tweaked the XDK110 to work with analog sensors through the Extension Hub. Read more about it and try to check it out yourself and start sending extra datapoints to the Tangle! [Sensor Project](https://github.com/Crelde/iotapoweredplants/tree/main/XDK110_IotaPlantSensor)
</br>
<img src="https://i.imgur.com/zAiXOzh.jpg" width="300">
</br>

# IoT Controller
In order to act on the data points, I have used the [ESP32 http-receiver](https://github.com/iot2tangle/ESP32/tree/main/http-receiver) project made by iot2tangle. The only changes I've made in this project is the actions, so I can turn on the water pump by listening to the Moisture sensor over Keepy. Find the project in the [http-receiver folder](https://github.com/Crelde/iotapoweredplants/tree/main/ESP32_http-receiver)

# Extended Keepy
I have modified the [Keepy project](https://github.com/iot2tangle/Keepy) to include a visualization page! You can setup this version and you will have a new endpoint on your keepy server. **/viz**. Here you will be able to see a view of your sensor data as it comes in, with option to live refresh the data! Check out the project [here](https://github.com/Crelde/iotapoweredplants/tree/main/KeepyWithVizualization) The instructions to setup is exactly the same as the original Keepy project.

## Here you can see a gif of the visualization page in action!
![](https://i.imgur.com/oDCF556.gif)

## Verify Data on the Tangle
This page also enables the user to verify that the data hasn't been tampered with, by checking the tangle data stream. (You have to select a channel before the link is available)

# Bill of Materials
## XDK110 sensor setup
* 1x XDK 110
* 1x Soil Sensor
* Jumper Wires
## ESP32 Controller setup
* 1x ESP32 Development board
* 1x Single Channel Relay Module
* 1x Submersible Water Pump
* Jumper Wires

# Ideas for further work
Here are some ideas that I think makes sense to further develop for this setup
## Integrate growth lights
It would be cool to be able to control the lighting for the plants. I don't currently own growth lights but it would be pretty easy to hook it up to a relay and start controlling the light exposure for the plants. The XDK also has a light sensor to monitor the exposure.
## Analyze plant growth
I think it would be possible to use an RGB sensor to monitor plant growth as suggested in this [research article](https://www.researchgate.net/publication/224245297_Low_cost_colour_sensors_for_monitoring_plant_growth_in_a_laboratory) that way you could try to correlate the sensor datapoints and improve the conditions for better growth. 
