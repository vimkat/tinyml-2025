// Include the Arduino library here (something like your_project_inference.h) 
// In the Arduino IDE see **File > Examples > Your project name - Edge Impulse > Static buffer** to get the exact name
#include <CirclingCruisingDetector_inferencing.h>
#include <Arduino_LSM9DS1.h> // IMU (GYRO)
#include <Arduino_LPS22HB.h> // BARO

#define FREQUENCY_HZ        EI_CLASSIFIER_FREQUENCY
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))
#define CONFIDENCE          0.5
#define HISTORY_COUNT       5

static unsigned long last_interval_ms = 0;
// to classify 1 frame of data you need EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE values
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
// keep track of where we are in the feature array
size_t feature_ix = 0;
// the best matching class of the classification
int result_class = -1;
int history[HISTORY_COUNT];

void setup() {
    Serial.begin(115200);
    Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  // Initialize LED pins as outputs
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  init_int_array(history, HISTORY_COUNT, -1);
}

void loop() {
    float g_x, g_y, g_z;

    if (millis() > last_interval_ms + INTERVAL_MS) {
        last_interval_ms = millis();

        // read sensor data in exactly the same way as in the Data Forwarder example
        // IMU.readAcceleration(x, y, z);

        if (IMU.gyroscopeAvailable()) {
          IMU.readGyroscope(g_x, g_y, g_z);
        } else {
          Serial.println("no gyro data??");
        }

        // fill the features buffer
        features[feature_ix++] = g_x;
        features[feature_ix++] = g_y;
        features[feature_ix++] = g_z;
        features[feature_ix++] = BARO.readPressure() * 10;

        // features buffer full? then classify!
        if (feature_ix == EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
            ei_impulse_result_t result;

            // create signal from features frame
            signal_t signal;
            numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

            // run classifier
            EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
            ei_printf("run_classifier returned: %d\n", res);
            if (res != 0) return;

            // print predictions
            ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result.timing.dsp, result.timing.classification, result.timing.anomaly);

            // print the predictions
            result_class = -1;
            float max_value = 0;
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("%s:\t%.5f\n", result.classification[ix].label, result.classification[ix].value);
                if (result.classification[ix].value > max_value && result.classification[ix].value > CONFIDENCE) {
                  result_class = ix;
                  max_value = result.classification[ix].value;
                }
            }

            // make LED reflect the current class
            determine_class();
            show_class();

            // reset features frame
            feature_ix = 0;
        }
    }
}

void determine_class() {
  // No class got confidence, keep old class
  if (result_class == -1) return;

  // shift array by one, save newest class
  memmove(&history[1], &history[0], (HISTORY_COUNT-1)*sizeof(int));
  history[0] = result_class;

  Serial.print("History: ");
  print_array(history, HISTORY_COUNT);
  
  // create histogram from history
  int counts[EI_CLASSIFIER_LABEL_COUNT];
  init_int_array(counts, EI_CLASSIFIER_LABEL_COUNT, 0);
  for (int i = 0; i < HISTORY_COUNT; ++i) {
    counts[history[i]]++;
  }

  Serial.print("Counts: ");
  print_array(counts, EI_CLASSIFIER_LABEL_COUNT);

  // determine best class
  result_class = -1;
  int max_value = 0;
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
    if (counts[i] > max_value) {
      result_class = i;
      max_value = counts[i];
    }
  }
}

void init_int_array(int* array, int count, int value) {
  for (int i = 0; i < count; ++i)
    array[i] = value;
}

void print_array(int* array, int count) {
  Serial.print("[ ");
  for (int i = 0; i < count; i ++) {
    Serial.print(array[i]);
    Serial.print(" ");
  }
  Serial.println("]");
}

void show_class() {
  switch (result_class) {
    case 0: // Circling
      // BLUE
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, LOW);
      Serial.println("=> DETECTED: CIRCLING");
      break;
    case 1: // Cruising
      // GREEN
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, LOW);
      digitalWrite(LEDB, HIGH);
      Serial.println("=> DETECTED: CRUISING");
      break;
    case 2: // Grounded
      // RED
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      Serial.println("=> DETECTED: GROUNDED");
      break;
    default:
      Serial.println("No class got the required confidence, keeping old class.");
      break;
  }
}

void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
    }
}