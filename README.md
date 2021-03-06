# Iota Powered Plants
Welcome to my Hackathon entry for the [iot2tangle hackathon](https://hackathon.iot2tangle.io/) in the Smart Agriculture category!
</br>
<img src="https://i.imgur.com/zAZC994.jpeg" width="700">
</br>

**For more detailed instructions on how to set this project up yourself please visit [The Iota Powered Plants Wiki](https://github.com/Crelde/iotapoweredplants/wiki)**

My name is Christian and I am a software developer in a global bioscience company. I am 28 years old and got my masters from the IT University of Copenhagen. In my daily life i spend most of my time developing digital services for our end customers. I have a big passion for DLT and have been following iota for a long time. I am very happy to be able to dip my toes in the water and try out integrating something with iota. I am a huge fan of data being more accessible, widespread and less silo'ed, so naturally this is a hackathon I HAD to participate in!  
## Demo video
Please check [this link](https://i.imgur.com/Fvk012V.mp4) for a demonstration of the project

## Purpose
I set out to create a project to monitor and control my basil plants! This is just a small scale project, but the idea is to illustrate how this setup could be used in, for example an Indoor/Vertical Farming plant. There are a number of reasons why this would be a real benefit for businesses in this segment:
* There is a lot of datapoints to monitor the production.
* It could make production more autonomous, by utilizing the datapoints and tailor the watering/fertilizing directly to the individual plant.
* It would be possible to provide **tamperproof data** about the plants to end-consumers since the data is on the Iota tangle.

There might be some differences to my basil plant and indoor farming plants. Many indoor farming plants doesn't use soil, but instead of the soil sensor that I am showcasing it could just as well be a pH sensor.

## Component overview
Here I have sketched the project setup, to give you an initial understanding of the base components that make up the project!

![](https://i.imgur.com/FTKDHhv.png)

Below you can read a bit more about the individual components and find the link to their project. (Or you can click the folders in the filelist)

# XDK 110 with extra analog sensors
For my IoT datapoints I've tweaked the XDK110 to work with analog sensors through the Extension Hub. Read more about it and try to check it out yourself and start sending extra datapoints to the Tangle! [Sensor Project](https://github.com/Crelde/iotapoweredplants/tree/main/XDK110_IotaPlantSensor)
</br>
<img src="https://i.imgur.com/zAiXOzh.jpg" width="300">
</br>

# ESP32 Setup to control water pump based on tangle data
In order to act on the data points, I have used the [ESP32 http-receiver](https://github.com/iot2tangle/ESP32/tree/main/http-receiver) project made by iot2tangle. The only changes I've made in this project is the actions, so I can turn on the water pump by listening to the data from the Moisture sensor.
Whenever the ESP32 reads new sensor values, it will check if the moisture is below a certain threshold. If conditions are met the ESP32 will turn on the relay for 5 seconds. During this time the water pump will be pumping! After the 5 seconds the relay is turned off for 1 minute to give some time for the water to get into the soil so we don't flood the plant completely.
Find the project in the [http-receiver folder](https://github.com/Crelde/iotapoweredplants/tree/main/ESP32_http-receiver)
</br>
<img src="https://i.imgur.com/J4ODtdy.jpeg" width="300">
</br>


# Live visualizations of sensor data integrated into Keepy
I have modified the original Keepy project from iot2tangle to include a visualization page! You can setup this version and you will have a new endpoint on your keepy server. **/viz**.

Here you will be able to see your sensor data as it comes in, with option to live refresh the data! Check out the project [here](https://github.com/Crelde/iotapoweredplants/tree/main/KeepyWithVizualization) The instructions to setup is exactly the same as the original Keepy project.

**You can also try it out live [here](http://94.16.114.51:3002/viz) by using my keepy installation on the VPS I set up.**
If you are lucky I am still sending data from my sensor setup! :) 

## Here you can see a gif of the visualization page in action!
![](https://i.imgur.com/oDCF556.gif)

## Verify Data on the Tangle
This page also enables the user to verify that the data hasn't been tampered with, by checking the tangle data stream. (You have to select a channel before the link is available)

# Bill of Materials
## XDK110 sensor setup
* 1x [XDK 110](https://developer.bosch.com/web/xdk/buy)
* 1x [Soil Sensor](https://www.banggood.com/20Pcs-Soil-Hygrometer-Humidity-Detection-Module-Moisture-Sensor-p-1023059.html)
* Jumper Wires
## ESP32 Controller setup
* 1x [ESP32 Development board](https://minielektro.dk/esp32-udviklingskort-med-wifi-bluetooth.html)
* 1x [Single Channel Relay Module](https://www.banggood.com/1-Channel-5V-Relay-Control-Module-Low-Level-Trigger-Optocoupler-Isolation-p-1556669.html)
* 1x [Submersible Water Pump](https://www.amazon.com/Priming-Water-Cooled-Cooling-Circulation-Interface/dp/B07FMR73ZF)
* 1meter of PVC tube 5/16
* Jumper Wires

# Ideas for further work
Here are some ideas that I think makes sense to further develop for this setup
## Integrate growth lights
It would be cool to be able to control the lighting for the plants. I don't currently own growth lights but it would be pretty easy to hook it up to a relay and start controlling the light exposure for the plants. The XDK also has a light sensor to monitor the exposure.
## Analyze plant growth
I think it would be possible to use an RGB sensor to monitor plant growth as suggested in this [research article](https://www.researchgate.net/publication/224245297_Low_cost_colour_sensors_for_monitoring_plant_growth_in_a_laboratory). By gathering this information you could try to improve the conditions for better growth. 
## Data about fertilizer used
Based on the sensors we could control the fertilizer by adding it to the watertank. In a bigger supply chain setup, it could also be really cool to prove that the plants have only been fertilized with organic fertilizers and no pesticides. 
