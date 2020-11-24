# Iota Powered Plants
Welcome to my Hackathon entry for the [iot2tangle hackathon](https://hackathon.iot2tangle.io/)!

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

