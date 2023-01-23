# TinyML---Pothole-Detection
Machine learning model on a MCU that detects uneven roads 

## Training
Train_Detection_Model.ipynb constains the code used to create and convert an autoencoder model using TensorFlow. The train dataset contains gyroscope and accelorometer readings in the Y-axis of an IMU . 

## Deployment
The model is deployed on Arduino BLE Sense. The code to run the model is in the car_driver folder.

## Demo
*Indicate moving over a pthole by lighting Red LED*
![Screenshot (507)](https://user-images.githubusercontent.com/76756708/214176357-ad68c617-8414-4e9e-af95-90e590bc92d5.png)
