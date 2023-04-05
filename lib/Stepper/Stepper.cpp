// Library for implementing a class for control of the stepper motor driver.

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "Stepper.h"



// Global variables to store PWM slice numbers so that they can be used in the interrupt handler bellow.
uint slice_num_global;
uint slice_num_counter_global;


// Interrupt handler for use by Stepper class.
void on_pwm_wrap(void)
{
    // Disable step PWM output and counter PWM input.
    pwm_set_enabled(slice_num_global, false);
    pwm_set_enabled(slice_num_counter_global, false);
    // Ensure counter is reset to 0 in the counter PWM.
    pwm_set_counter(slice_num_counter_global, 0);
    // Clear interrupt flag.
    pwm_clear_irq(slice_num_counter_global);
}


// Constructor will take stepper frequency, and gpio ID numbers that are used to interface with the driver.
// These will be used to initalise the appropriate pins as digital and PWM outputs, for control of the driver.
Stepper::Stepper(uint step_freq, uint enable_port, uint reset_port, uint sleep_port, uint step_port, 
                 uint direction_port, uint ms1_port, uint ms2_port, uint ms3_port, uint counter_port)
    // Member initalization list (excluding "slice_num", which will be assigned in the body)
    : enable_port{ enable_port }
    , sleep{ sleep_port }
    , reset{ reset_port }
    , step{ step_port }
    , dir{ direction_port }
    , ms1{ ms1_port }
    , ms2{ ms2_port }
    , ms3{ ms3_port }
    , counter{ counter_port }
{
    // --------------- Step PWM ---------------
    // Set up step port as PWM output with specified frequency, initially off.

    // Tell the step pin that the PWM is in charge of its value.
    gpio_set_function(step_port, GPIO_FUNC_PWM);

    // Figure out which slice we just connected to the step pin. Assign this value to "slice_num" for later use.
    slice_num = pwm_gpio_to_slice_num(step_port);

    // Assign slice number to global variable so it can be used in the interrupt handler.
    slice_num_global = slice_num;

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();

    // Calculate divider to achieve the PWM frequency specified, using a 125MHz system clock speed,
    // and 65536 (2**16) clock ticks before wrapping (65535 wrap value). 
    // For our application, the PWM frequency achieved does not need to be very precise.
    float div = (125000000/65536)/step_freq;
    pwm_config_set_clkdiv(&config, div);

    // Load the configuration into our PWM slice, initialy not running.
    pwm_init(slice_num, &config, false);

    // Set PWM counter compare values for both channels on the selected slice.
    // The internal counter is compared against these values to set the output high or low, for each channel.
    // Strictly speaking, we don't need to set both channels, since we only have one PWM output.
    // However, it does mean we don't need to find what channel our output pin coresponds to.
    // Set this to achieve a 50% duty cycle (using a wrap value of 65535 (the default)).
    pwm_set_both_levels(slice_num, 32768, 32768);


    // --------------- PWM Counter ---------------
    // Set up port as PWM input to count the pulses from the above PWM output.
    // This will be used to trigger an interrupt to stop the PWM output once a certain number of pulses are counted,
    // when using the forward_by() and backward_by() methods.

    // Make sure counter pin is on channel B of PWM slice and not on the same slice as the step PWM.
    assert(pwm_gpio_to_channel(counter_port) == PWM_CHAN_B);
    slice_num_counter = pwm_gpio_to_slice_num(counter_port);
    assert(slice_num_counter != slice_num);

    // Assign slice number to global variable so it can be used in the interrupt handler.
    slice_num_counter_global = slice_num_counter;

    // Get default slice configuration.
    pwm_config counter_config = pwm_get_default_config();

    // Set up slice to take rising edge inputs on channel B (from the step PWM output) to increment internal counter.
    pwm_config_set_clkdiv_mode(&counter_config, PWM_DIV_B_RISING);

    // Set up interrupt for when the internal counter wraps
    pwm_clear_irq(slice_num_counter);
    pwm_set_irq_enabled(slice_num_counter, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Load the configuration into our PWM slice, initialy not running, and set up GPIO pin as PWM.
    pwm_init(slice_num_counter, &counter_config, false);
    gpio_set_function(counter_port, GPIO_FUNC_PWM);



    // --------------- Enable Control ---------------
    // Set up the enable pin as a digital output. Default to output high to disable the driver.
    gpio_init(enable_port);
    gpio_set_dir(enable_port, GPIO_OUT);
    gpio_put(enable_port, 1);


    // --------------- Sleep Control ---------------
    // Set up the sleep pin as a digital output. Default to output high to enable the driver.
    gpio_init(sleep_port);
    gpio_set_dir(sleep_port, GPIO_OUT);
    gpio_put(sleep_port, 1);


    // --------------- Reset Control ---------------
    // Set up the reset pin as a digital output. Default to output high to enable the driver.
    gpio_init(reset_port);
    gpio_set_dir(reset_port, GPIO_OUT);
    gpio_put(reset_port, 1);


    // --------------- Direction Control ---------------
    // Set up the direction pin as a digital output. No need to set a default value.
    gpio_init(direction_port);
    gpio_set_dir(direction_port, GPIO_OUT);


    // --------------- Microstep Control ---------------
    // Set up the "ms" microstep pins as digital outputs. Default to output low on all pins for full stepping.
    gpio_init(ms1_port);
    gpio_set_dir(ms1_port, GPIO_OUT);
    gpio_put(ms1_port, 0);

    gpio_init(ms2_port);
    gpio_set_dir(ms2_port, GPIO_OUT);
    gpio_put(ms2_port, 0);

    gpio_init(ms3_port);
    gpio_set_dir(ms3_port, GPIO_OUT);
    gpio_put(ms3_port, 0);
}


// The forward() method will set the direction and step PWM pins to move the actuator forwards.
void Stepper::forward(void)
{
    gpio_put(dir, 0);
    pwm_set_enabled(slice_num, true);
}


// The backward() method will set the direction and step PWM pins to move the actuator backwards.
void Stepper::backward(void)
{
    gpio_put(dir, 1);
    pwm_set_enabled(slice_num, true);
}


// The stop() method will disable the PWM output to stop the actuator.
void Stepper::stop(void)
{
    pwm_set_enabled(slice_num, false);
}


// The forward_by() method will move actuator forwards by a specified number of steps.
void Stepper::forward_by(uint steps)
{
    gpio_put(dir, 0);
    pwm_set_wrap(slice_num_counter, steps-1);
    pwm_set_enabled(slice_num_counter, true);
    pwm_set_enabled(slice_num, true);
}


// The backward_by() method will move actuator backward by a specified number of steps.
void Stepper::backward_by(uint steps)
{
    gpio_put(dir, 1);
    pwm_set_wrap(slice_num_counter, steps-1);
    pwm_set_enabled(slice_num_counter, true);
    pwm_set_enabled(slice_num, true);
}


// The enable() method will enable the driver.
void Stepper::enable(void)
{
    gpio_put(enable_port, 0);  // Enable port is active low
}


// The disable() method will enable the driver.
void Stepper::disable(void)
{
    gpio_put(enable_port, 1);  // Enable port is inactive high
}