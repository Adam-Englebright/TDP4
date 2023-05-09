#include "pico/stdlib.h"
#include "Stepper.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>

#include "XY_coordinate_array.h"

#define STEP_FREQ 100
#define ENABLE_PIN 28
#define MS1_PIN 27
#define MS2_PIN 26
#define MS3_PIN 22
#define RESET_PIN 21
#define SLEEP_PIN 20
#define STEP_PIN 19
#define DIR_PIN 18
#define COUNTER_PIN 17

#define F_BUTTON 0
#define B_BUTTON 1
#define I2C_BUTTON 13
#define MISC_BUTTON 16

#define LED1_PIN 15
#define LED2_PIN 14

#define GPIO_SDA0 4
#define GPIO_SCL0 5
#define GPIO_SDA1 2
#define GPIO_SCL1 3
#define SLAVE_ADDR 52
#define T3_ADDR 53
#define XY_ADDR 55
#define Z_ADDR 56
#define CONTROL_HEADER 97

#define Z_RISE_POS 0
#define Z_DROP_POS 37000

#define X_OFFSET 0
#define Y_OFFSET 0

bool z_arm_in_position = true;
bool xy_arm_in_position = true;
bool currently_master = false;
bool start = false;


// Initalise stepper control object.
Stepper stepper(STEP_FREQ, ENABLE_PIN, RESET_PIN, SLEEP_PIN, STEP_PIN, DIR_PIN, MS1_PIN, MS2_PIN, MS3_PIN, COUNTER_PIN);


// Function to be called when I2C transmission is recieved.
void i2c0_irq_handler()
{
    printf("In the i2c IRQ\n");
    size_t how_many = i2c_get_read_available(i2c0);
    
    uint8_t buf[how_many];

    // Read data transmitted.
    i2c_read_raw_blocking(i2c0, buf, how_many);

    // If handover signal is recieved, set our uC to master.
    if (buf[0] == 3) {
        currently_master = true;
        gpio_put(LED1_PIN, 0);
        gpio_put(LED2_PIN, 0);
        printf("We are now master!\n");
    }
    else {
        gpio_put(LED1_PIN, 1);
        gpio_put(LED2_PIN, 1);
        printf("Unknown message recieved!\n");
    }

    // Clear interrupt.
    i2c0->hw->clr_stop_det;
}


// Create callback function that will handle interupts from GPIO button inputs.
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == F_BUTTON && events == GPIO_IRQ_EDGE_RISE) {
        stepper.forward();
        printf("Pressing button 1 to push plunger forward\n");
    } else if (gpio == F_BUTTON && events == GPIO_IRQ_EDGE_FALL) {
        stepper.stop();
        printf("Releasing button 1 to stop plunger\n");
    } else if (gpio == B_BUTTON && events == GPIO_IRQ_EDGE_RISE) {
        stepper.backward();
        printf("Pressing button 2 to pull plunger backward\n");
    } else if (gpio == B_BUTTON && events == GPIO_IRQ_EDGE_FALL) {
        stepper.stop();
        printf("Releasing button 2 to stop plunger\n");
    } else if (gpio == I2C_BUTTON && events == GPIO_IRQ_EDGE_RISE) {
        printf("Pressing button 3 to start our process\n");
        if (currently_master && stepper.is_enabled()) { // Should only be able to start if we have already been given mastership and stepper is enabled
            printf("Start flag set.\n");
            gpio_put(LED1_PIN, 1);
            gpio_put(LED2_PIN, 0);
            start = true;
        } else if (currently_master && !stepper.is_enabled()) {
            printf("Connot start! Stepper is not enabled!\n");
        } else if (!currently_master && stepper.is_enabled()) {
            printf("Connot start! We are not master!\n");
        } else {
            printf("Connot start! We are not master and the stepper is not enabled!\n");
        }
    } else if (gpio == MISC_BUTTON && events == GPIO_IRQ_EDGE_RISE) {
        if (stepper.is_enabled())
            stepper.disable();
        else
            stepper.enable();
    }
}


// Function for sending Z control commands.
void control_z(uint32_t z_micron_pos, bool apply_paste) {
    uint8_t z_data[sizeof(z_micron_pos)+1];
    z_data[0] = CONTROL_HEADER;  // Header
    uint8_t z_response_data[1] = {2};
    int z_return_val;

    memcpy(&z_data[1], &z_micron_pos, sizeof(z_micron_pos));
    printf("Writing %08X in chunks of %02X %02X %02X %02X, with %02X header.\n", z_micron_pos, z_data[1], z_data[2], z_data[3], z_data[4], z_data[0]);
    z_return_val = i2c_write_blocking(i2c1, Z_ADDR, z_data, sizeof(z_data), false);
    printf("Return value is %d\n", z_return_val);
    z_arm_in_position = false;

    i2c_read_blocking(i2c1, Z_ADDR, z_response_data, sizeof(z_response_data), false);
    if (z_response_data[0] == 0) {
        printf("Z arm not in position\n");
    } else if (z_response_data[0] == 1 && apply_paste) {
        printf("Z arm is now in position! Applying paste\n");
        z_arm_in_position = true;
        stepper.forward_by(15);
    } else if (z_response_data[0] == 1 && !apply_paste) {
        printf("Z arm is now in position!\n");
        z_arm_in_position = true;
    } else {
        printf("Something's not right\n");
    }
}


// Function for sending XY control commands.
void control_xy(uint32_t x_micron_pos, uint32_t y_micron_pos) {
    uint8_t xy_data[sizeof(x_micron_pos)+sizeof(y_micron_pos)+1];
    xy_data[0] = CONTROL_HEADER;  // Header
    uint8_t xy_response_data[1] = {2};
    int xy_return_val;

    memcpy(&xy_data[1], &x_micron_pos, sizeof(x_micron_pos));
    memcpy(&xy_data[5], &y_micron_pos, sizeof(y_micron_pos));
    printf("Writing %08X and %08X in chunks of %02X %02X %02X %02X and %02X %02X %02X %02X, with %02X header.\n",
    x_micron_pos, y_micron_pos, xy_data[1], xy_data[2], xy_data[3], xy_data[4], xy_data[5], xy_data[6], xy_data[7], xy_data[8], xy_data[0]);
    xy_return_val = i2c_write_blocking(i2c1, XY_ADDR, xy_data, sizeof(xy_data), false);
    printf("Return value is %d\n", xy_return_val);
    xy_arm_in_position = false;

    i2c_read_blocking(i2c1, XY_ADDR, xy_response_data, sizeof(xy_response_data), false);
    if (xy_response_data[0] == 0) {
        printf("XY not in position\n");
    } else if (xy_response_data[0] == 1) {
        printf("XY is now in position!\n");
        xy_arm_in_position = true;
    } else {
        printf("Something's not right\n");
    }
}


int main(void)
{  
    // Set up USB comms for print debugging.
    stdio_init_all();

    // Set up GPIO input on pins 0, 1, 13, and 16 for control buttons.
    gpio_init(F_BUTTON);
    gpio_set_dir(F_BUTTON, GPIO_IN);
    gpio_pull_down(F_BUTTON);  // Button connects to 3V3, so we need to pull down upon button release.

    gpio_init(B_BUTTON);
    gpio_set_dir(B_BUTTON, GPIO_IN);
    gpio_pull_down(B_BUTTON);  // Same as above.

    gpio_init(I2C_BUTTON);
    gpio_set_dir(I2C_BUTTON, GPIO_IN);
    gpio_pull_down(I2C_BUTTON);

    gpio_init(MISC_BUTTON);
    gpio_set_dir(MISC_BUTTON, GPIO_IN);
    gpio_pull_down(MISC_BUTTON);

    // Set up GPIO outputs for LEDs, red LED off and green LED on (slave configuration) by default.
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_put(LED2_PIN, 1);


    // Set up I2C0 as slave.
    i2c_init(i2c0, 100000);
    i2c_set_slave_mode(i2c0, true, SLAVE_ADDR);
    gpio_set_function(GPIO_SDA0, GPIO_FUNC_I2C);
    gpio_set_function(GPIO_SCL0, GPIO_FUNC_I2C);
    gpio_pull_up(GPIO_SDA0);
    gpio_pull_up(GPIO_SCL0);


    // Enable the I2C interrupts we want to process. Interrupt on stop signal.
    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS;

    // Only interrupt if we are addressed.
    i2c0->hw->enable = 0;
    hw_set_bits(&i2c0->hw->con, I2C_IC_CON_STOP_DET_IFADDRESSED_BITS);
    i2c0->hw->enable = 1;

    // Set up the interrupt handler to service I2C interrupts.
    irq_set_exclusive_handler(I2C0_IRQ, &i2c0_irq_handler);

    // Enable I2C interrupt.
    irq_set_enabled(I2C0_IRQ, true);


    // set up I2C1 as master.
    i2c_init(i2c1, 100000);
    gpio_set_function(GPIO_SDA1, GPIO_FUNC_I2C);
    gpio_set_function(GPIO_SCL1, GPIO_FUNC_I2C);
    gpio_pull_up(GPIO_SDA1);
    gpio_pull_up(GPIO_SCL1);


    // Set up interupts on the button inputs.
    gpio_set_irq_enabled_with_callback(F_BUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(B_BUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(I2C_BUTTON, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(MISC_BUTTON, GPIO_IRQ_EDGE_RISE, true);


    // Loop forever.
    while (true) {
        printf("Working...\n");
        sleep_ms(1000);
        if (start && currently_master) {
            printf("Starting!\n");

            // Iterate over array of xy positions for paste application.
            // Number of iterations is calculated by dividing the total number of bytes in the array by 8,
            // since each sub-array containing xy coordinates is 8 bytes in size (4 bytes for each 32 bit coordinate)
            for (int i=0; i<(sizeof(xy_coords)/sizeof(xy_coords[0])); i++) {
                // Move XY into correct position.
                control_xy(xy_coords[i][0] + X_OFFSET, xy_coords[i][1] + Y_OFFSET);

                // Move Z down, apply paste, move Z back up.
                control_z(Z_DROP_POS, true);
                sleep_ms(2000);
                control_z(Z_RISE_POS, false);
            }

            // We are finished with paste application now, so reset XYZ position and handover mastership.
            printf("Finished with paste application, resetting to 0, 0, 0 XYZ\n");
            control_xy(0, 0);
            //control_z(0, false);

            // Send master handover message.
            printf("Sending handover message.");
            uint8_t handover_data[1] = {3};
            i2c_write_blocking(i2c1, T3_ADDR, handover_data, sizeof(handover_data), false);
            currently_master = false;
        }
    }
}
