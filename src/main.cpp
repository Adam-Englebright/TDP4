#include "pico/stdlib.h"
#include "Stepper.h"

#define STEP_FREQ 500
#define ENABLE_PIN 28
#define MS1_PIN 27
#define MS2_PIN 26
#define MS3_PIN 22
#define RESET_PIN 21
#define SLEEP_PIN 20
#define STEP_PIN 19
#define DIR_PIN 18


// Initalise stepper control object with 100Hz step frequency.
Stepper stepper(STEP_FREQ, ENABLE_PIN, RESET_PIN, SLEEP_PIN, STEP_PIN, DIR_PIN, MS1_PIN, MS2_PIN, MS3_PIN);


// Create callback function that will handle interupts from GPIO button inputs.
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == 0 && events == GPIO_IRQ_EDGE_RISE) {
        stepper.forward();
    }
    else if (gpio == 1 && events == GPIO_IRQ_EDGE_RISE) {
        stepper.backward();
    }
    else {
        stepper.stop();
    }
}


int main(void)
{  
    // Set up GPIO input on pins 0 and 1 for control buttons.
    gpio_init(0);
    gpio_set_dir(0, GPIO_IN);
    gpio_pull_down(0);  // Button connects to 3V3, so we need to pull down upon button release.

    gpio_init(1);
    gpio_set_dir(1, GPIO_IN);
    gpio_pull_down(1);  // Same as above.


    // Set up interupts on the two input pins for rising and falling edges.
    gpio_set_irq_enabled_with_callback(0, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);


    // Loop forever.
    while (true);
}
