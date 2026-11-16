#include <Arduino.h>

// Queue handle
QueueHandle_t numberQueue;

// Task 1: Produces numbers
void producerTask(void *parameter) {
  int counter = 1;

  while (true) {
    // Send number to queue (wait up to 100 ms if full)
    if (xQueueSend(numberQueue, &counter, pdMS_TO_TICKS(100)) == pdPASS) {
      Serial.printf("Produced: %d\n", counter);
      counter++;
    }

    // Wait 1 second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Task 2: Consumes numbers
void consumerTask(void *parameter) {
  int receivedNumber;

  while (true) {
    // Wait indefinitely until a number is available
    if (xQueueReceive(numberQueue, &receivedNumber, portMAX_DELAY) == pdPASS) {
      Serial.printf("Consumed: %d\n", receivedNumber);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Create queue: 10 entries, each containing an int
  numberQueue = xQueueCreate(10, sizeof(int));

  if (numberQueue == NULL) {
    Serial.println("ERROR: Queue creation failed!");
    while (true) {
      delay(1000);
    }
  }

  // Create producer task on Core 0
  xTaskCreatePinnedToCore(
    producerTask,
    "Producer",
    2048,
    NULL,
    1,
    NULL,
    0
  );

  // Create consumer task on Core 1
  xTaskCreatePinnedToCore(
    consumerTask,
    "Consumer",
    2048,
    NULL,
    1,
    NULL,
    1
  );

  Serial.println("FreeRTOS tasks started.");
}

void loop() {
  // Nothing to do here
  vTaskDelay(portMAX_DELAY);
}
