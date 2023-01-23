#include <Arduino_LSM9DS1.h>

#include <TensorFlowLite.h>

#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "autoencoder_model_data.h"

#define RED 22     
#define BLUE 24     
#define GREEN 23


namespace {
  constexpr int kTensorArenaSize = 10 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];

  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;  


}  

bool commandRecv = false;  // flag used for indicating receipt of commands from serial port

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU");
    while (1)
      ;
  }
  pinMode(D12, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  static tflite::MicroErrorReporter micro_error_reporter;  // NOLINT
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(autoencoder_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::MicroMutableOpResolver<3> micro_op_resolver;  // NOLINT
  micro_op_resolver.AddMean();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddRelu();

  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();

  TfLiteTensor* model_input = interpreter->input(0);
   Serial.println("h");

  TfLiteTensor* model_output = interpreter->output(0);

}

void printArr(float *arr, int size) {
  Serial.print("");
  for (int i = 0; i < size - 1; i++) {
    Serial.print(arr[i], 3);
    Serial.print(",");
  }
  Serial.print(arr[size - 1], 3);
  Serial.println("");
}

void prepare_input(float *arr, float *out_arr) {
  int i = 0;
  int j = 0;
  while (i <= 58) {
    out_arr[j] = (arr[i] + arr[i+1]) / 2;
    j++;
    i += 2;
  }
}

float calculateMAE(float *arr1, float *arr2, int len) {
  float sum = 0;
  for (int i = 0; i < len; i++) {
    sum += abs(arr1[i] - arr2[i]);
  }
  return sum / len;
}

void loop() {
  // put your main code here, to run repeatedly:
  String command;


  while (Serial.available()) {
    char c = Serial.read();
    if ((c != '\n') && (c != '\r')) {
      command.concat(c);
    } else if (c == '\r') {
      commandRecv = true;
      command.toLowerCase();
    }
  }

  if (commandRecv || true) {
    commandRecv = false;
    if (command == "a" || true) {

      // Serial.println("start");
      digitalWrite(D12, HIGH);
      float ax, ay, az;
      float gx, gy, gz;
      float gX[60] = {};
      float gY[60] = {};
      float gZ[60] = {};
      float aX[60] = {};
      float aY[60] = {};
      float aZ[60] = {};
      for (int i = 0; i < 60; i++) {
        uint8_t r = 0;
        if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(ax, ay, az);
          r++;
        }
        if (IMU.gyroscopeAvailable()) {
          IMU.readGyroscope(gx, gy, gz);
          r++;
        }
        if (r == 2) {
          gX[i] = gx;
          gY[i] = gy;
          gZ[i] = gz;
          aX[i] = ax;
          aY[i] = ay;
          aZ[i] = az;
        } else {
          Serial.println("Not read");
          i--;
        }
        delay(10);
      }
      digitalWrite(D12, LOW);
      // Serial.println("stop");
      // Serial.println("Printing New Readings");
      // Serial.println("Printing GX");
      // printArr(gX, 60);
      // Serial.println("Printing GY");
      // printArr(gY, 60);
      float to_input[30] = {};
      prepare_input(gY, to_input);

      TfLiteTensor* model_input = interpreter->input(0);
      Serial.println(model_input->type == kTfLiteInt8);
      Serial.println(model_input->type == kTfLiteFloat32);
      for (int i = 0; i < 30; ++i) {
        model_input->data.f[i] = to_input[i];
      }

      TfLiteStatus invoke_status = interpreter->Invoke();
      if (invoke_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed");
        return;
      }
      TfLiteTensor* output = interpreter->output(0);

      float to_output[30] = {};
      for (int i = 0; i < 30; ++i) {
        to_output[i] = output->data.f[i];
      }
      printArr(to_output, 30);

      float mae = calculateMAE(to_input, to_output, 30);

      if (mae < 21) {
        Serial.println("Normal Road");
        // digitalWrite(LEDG, LOW); // turn the LED on (HIGH is the voltage level)
        // delay(200);
        // digitalWrite(LEDG, HIGH);
      } else {
        Serial.println("Uneven Road");
        digitalWrite(LEDR, LOW); // turn the LED on (HIGH is the voltage level)
        delay(1000);
        digitalWrite(LEDR, HIGH);
      }


      // Serial.println("Printing GZ");
      // printArr(gZ, 60);
      // Serial.println("Printing AX");
      // printArr(aX, 60);
      // Serial.println("Printing AY");
      // printArr(aY, 60);
      // Serial.println("Printing AZ");
      // printArr(aZ, 60);
    }
  }
}
