#include "pico/stdlib.h"
#include "Stepper.h"
#include "hardware/i2c.h"
#include <stdio.h>

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

#define GPIO_SDA0 6
#define GPIO_SCL0 7
#define GPIO_SDA1 4
#define GPIO_SCL1 5
#define SLAVE_ADDR 8


// Function to be called when I2C transmission is recieved.
void i2c0_irq_handler()
{
    printf("In the i2c IRQ\n");
    size_t how_many = i2c_get_read_available(i2c0);
    
    uint8_t buf[how_many];

    // Read data transmitted.
    i2c_read_raw_blocking(i2c0, buf, how_many);

    // Set LEDs according to data sent.
    if (buf[0] == 0) {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,1);
    }
    else if (buf[0] == 1) {
        gpio_put(LED1_PIN,1);
        gpio_put(LED2_PIN,1);
    }
    else if (buf[0] == 2) {
        gpio_put(LED1_PIN,1);
        gpio_put(LED2_PIN,1);
    }
    else {
        gpio_put(LED1_PIN,0);
        gpio_put(LED2_PIN,0);
    }

    // Clear interrupt.
    i2c0->hw->clr_stop_det;
}

// Create callback function that will handle interupts from GPIO button inputs.
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == 0 && events == GPIO_IRQ_EDGE_RISE) {
        printf("Pressing button 1\n");
        const uint8_t data[] = {0};
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
    }
    else if (gpio == 1 && events == GPIO_IRQ_EDGE_RISE) {
        printf("Pressing button 2\n");
        const uint8_t data[] = {1};
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
    }
    else if (gpio == 13 && events == GPIO_IRQ_EDGE_RISE) {
        printf("Pressing button 3\n");
        const uint8_t data[] = {2};
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
    }
}




// Initalise stepper control object.
Stepper stepper(STEP_FREQ, ENABLE_PIN, RESET_PIN, SLEEP_PIN, STEP_PIN, DIR_PIN, MS1_PIN, MS2_PIN, MS3_PIN, COUNTER_PIN);


int main(void)
{  
    // Set up USB comms for print debugging.
    stdio_init_all();

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


    // Set up I2C0 as slave.
    i2c_init(i2c0, 100000);
    i2c_set_slave_mode(i2c0, true, SLAVE_ADDR);
    gpio_set_function(GPIO_SDA0, GPIO_FUNC_I2C);
    gpio_set_function(GPIO_SCL0, GPIO_FUNC_I2C);
    gpio_pull_up(GPIO_SDA0);
    gpio_pull_up(GPIO_SCL0);

    // Enable the I2C interrupts we want to process.
    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS;

    // Set up the interrupt handler to service I2C interrupts.
    irq_set_exclusive_handler(I2C0_IRQ, i2c0_irq_handler);

    // Enable I2C interrupt.
    irq_set_enabled(I2C0_IRQ, true);


    // set up I2C1 as master.
    i2c_init(i2c1, 100000);
    gpio_set_function(GPIO_SDA1, GPIO_FUNC_I2C);
    gpio_set_function(GPIO_SCL1, GPIO_FUNC_I2C);
    gpio_pull_up(GPIO_SDA1);
    gpio_pull_up(GPIO_SCL1);


    // Set up interupts on the button inputs.
    gpio_set_irq_enabled_with_callback(0, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(1, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(13, GPIO_IRQ_EDGE_RISE, true);


    // Loop forever.
    uint8_t data[] = {0};
    while (true) {
        printf("Writing 0\n");
        data[0] = 0;
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
        sleep_ms(1000);

        printf("Writing 1\n");
        data[0] = 1;
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
        sleep_ms(1000);

        printf("Writing 2\n");
        data[0] = 2;
        i2c_write_blocking(i2c1, SLAVE_ADDR, data, sizeof(data), false);
        sleep_ms(1000);
    };
}
