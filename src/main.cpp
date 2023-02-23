#include "pico/stdlib.h"
#include "Stepper.h"
#include "Wire.h"

#define STEP_FREQ 15
#define ENABLE_PIN 28
#define MS1_PIN 27
#define MS2_PIN 26
#define MS3_PIN 22
#define RESET_PIN 21
#define SLEEP_PIN 20
#define STEP_PIN 19
#define DIR_PIN 18
#define COUNTER_PIN 17

#define LED1_PIN 15
#define LED2_PIN 14


// Function to be called when I2C transmission is recieved.
void receiveEvent(int howMany)
{
    gpio_put(LED1_PIN,0);
    gpio_put(LED2_PIN,1);
    
    char buf[howMany];

    // Read data transmitted.
    for (int i=0; i<howMany; i++) {
        buf[i] = Wire.read();
    }

    // Set LEDs according to data sent.
    if (buf[0] == 0) {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,1);
    }
    else if (buf[0] == 1) {
        gpio_put(LED1_PIN,1);
        gpio_put(LED2_PIN,0);
    }
    else if (buf[0] == 2) {
        gpio_put(LED1_PIN,1);
        gpio_put(LED2_PIN,1);
    }
    else {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,0);
    }
}

// Create callback function that will handle interupts from GPIO button inputs.
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == 0 && events == GPIO_IRQ_EDGE_RISE) {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,1);
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)0);
        Wire1.endTransmission();
    }
    else if (gpio == 1 && events == GPIO_IRQ_EDGE_RISE) {
        gpio_put(LED1_PIN,1);
        gpio_put(LED2_PIN,0);
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)1);
        Wire1.endTransmission();
    }
    else if (gpio == 13 && events == GPIO_IRQ_EDGE_RISE) {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,0);
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)2);
        Wire1.endTransmission();
    }
}




// Initalise stepper control object.
Stepper stepper(STEP_FREQ, ENABLE_PIN, RESET_PIN, SLEEP_PIN, STEP_PIN, DIR_PIN, MS1_PIN, MS2_PIN, MS3_PIN, COUNTER_PIN);


int main(void)
{  
    // Set up GPIO input on pins 0, 1, and 13 for control buttons.
    gpio_init(0);
    gpio_set_dir(0, GPIO_IN);
    gpio_pull_down(0);  // Button connects to 3V3, so we need to pull down upon button release.

    gpio_init(1);
    gpio_set_dir(1, GPIO_IN);
    gpio_pull_down(1);  // Same as above.

    gpio_init(13);
    gpio_set_dir(13, GPIO_IN);
    gpio_pull_down(13);

    // Set up GPIO outputs for LEDs, off by default.
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_put(LED2_PIN, 0);


    // Set up I2C.
    Wire.begin(6, 7, 8);  // SDA, SCL, Address
    Wire1.begin(4, 5, 9);

    Wire.onReceive(receiveEvent);


    // Set up interupts on the button inputs.
    gpio_set_irq_enabled_with_callback(0, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(1, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(13, GPIO_IRQ_EDGE_RISE, true);


    // Loop forever.
    while (true) {
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)0);
        Wire1.endTransmission();
        sleep_ms(1000);
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)1);
        Wire1.endTransmission();
        sleep_ms(1000);
        Wire1.beginTransmission(8);
        Wire1.write((uint8_t)2);
        Wire1.endTransmission();
    };
}
