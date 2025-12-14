/*
 * Traffic Lights Control for Raspberry Pi 2
 * F28HS Lab - Week 5
 * 
 * Controls 3 LEDs (Red, Yellow, Green) to simulate traffic light sequence
 * Optional: Button on pin 26 for pedestrian crossing
 * 
 * Compile: gcc -o traffic_lights traffic_lights.c
 * Run: sudo ./traffic_lights
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// GPIO Register base address for RPi2
#define BCM2708_PERI_BASE 0x3F000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO pin definitions (BCM numbering)
#define RED_LED    10
#define YELLOW_LED 11
#define GREEN_LED  13
#define BUTTON     26

// Timing definitions (in seconds)
#define RED_DURATION 3
#define RED_YELLOW_DURATION 2
#define GREEN_DURATION 5
#define YELLOW_BLINK_COUNT 3
#define YELLOW_BLINK_DELAY 500000  // 0.5 seconds in microseconds

// GPIO register offsets
#define GPFSEL0   0
#define GPFSEL1   1
#define GPFSEL2   2
#define GPSET0    7
#define GPCLR0    10
#define GPLEV0    13

// Bit masks as literals (using #define makes them literals)
#define PIN10_MASK 0b10000000000
#define PIN11_MASK 0b100000000000
#define PIN13_MASK 0b10000000000000
#define PIN26_MASK 0b100000000000000000000000000

volatile unsigned *gpio;

// Function to set up memory mapping for GPIO
int setup_gpio() {
    int mem_fd;
    void *gpio_map;
    
    // Open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
        printf("Error: can't open /dev/mem (try running with sudo)\n");
        return -1;
    }
    
    // Map GPIO memory
    gpio_map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        GPIO_BASE
    );
    
    close(mem_fd);
    
    if (gpio_map == MAP_FAILED) {
        printf("Error: mmap failed\n");
        return -1;
    }
    
    gpio = (volatile unsigned *)gpio_map;
    return 0;
}

// Initialize GPIO pins using literals
void init_gpio() {
    unsigned int current_val;
    
    // Configure pins 10, 11, 13 in GPFSEL1 as outputs
    // Read current value
    current_val = *(gpio + GPFSEL1);
    
    // Clear bits for pins 10, 11, 13 and set to output (001)
    // Pin 10: bits 0-2, Pin 11: bits 3-5, Pin 13: bits 9-11
    current_val &= 0b11111111111111111111000000000000; // Clear bits 0-11
    current_val |= 0b00000000000000000000001001001001; // Set output mode
    *(gpio + GPFSEL1) = current_val;
    
    // Configure pin 26 in GPFSEL2 as input
    current_val = *(gpio + GPFSEL2);
    current_val &= 0b11111111111000111111111111111111; // Clear bits 18-20
    *(gpio + GPFSEL2) = current_val;
    
    // Turn off all LEDs initially using literals
    *(gpio + GPCLR0) = PIN13_MASK;  // Clear pin 13 (green)
    *(gpio + GPCLR0) = PIN11_MASK;  // Clear pin 11 (yellow)
    *(gpio + GPCLR0) = PIN10_MASK;  // Clear pin 10 (red)
}

// Turn on RED LED using literal
void red_on() {
    *(gpio + GPSET0) = PIN10_MASK;
}

// Turn off RED LED using literal
void red_off() {
    *(gpio + GPCLR0) = PIN10_MASK;
}

// Turn on YELLOW LED using literal
void yellow_on() {
    *(gpio + GPSET0) = PIN11_MASK;
}

// Turn off YELLOW LED using literal
void yellow_off() {
    *(gpio + GPCLR0) = PIN11_MASK;
}

// Turn on GREEN LED using literal
void green_on() {
    *(gpio + GPSET0) = PIN13_MASK;
}

// Turn off GREEN LED using literal
void green_off() {
    *(gpio + GPCLR0) = PIN13_MASK;
}

// Check if button is pressed (returns 1 if pressed, 0 otherwise)
int button_pressed() {
    unsigned int level = *(gpio + GPLEV0);
    return (level & PIN26_MASK) == 0;  // Active low with pull-up
}

// Wait for button press or timeout
void wait_for_button_or_timeout(int timeout_seconds) {
    printf("Waiting for pedestrian button or timeout (%d seconds)...\n", timeout_seconds);
    for (int i = 0; i < timeout_seconds * 10; i++) {
        if (button_pressed()) {
            printf("Button pressed! Changing lights...\n");
            usleep(300000);  // Debounce delay
            return;
        }
        usleep(100000);  // Check every 0.1 seconds
    }
    printf("Timeout - changing lights...\n");
}

// Traffic light sequence
void traffic_light_sequence() {
    printf("\n=== Traffic Light Sequence Starting ===\n\n");
    
    // Phase 1: RED ON (stop)
    printf("Phase 1: RED ON (STOP)\n");
    *(gpio + GPSET0) = PIN10_MASK;  // Red on
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPCLR0) = PIN13_MASK;  // Green off
    sleep(RED_DURATION);
    
    // Phase 2: RED ON, YELLOW ON (attention - prepare to go)
    printf("Phase 2: RED + YELLOW ON (ATTENTION - PREPARE TO GO)\n");
    *(gpio + GPSET0) = PIN10_MASK;  // Red on
    *(gpio + GPSET0) = PIN11_MASK;  // Yellow on
    *(gpio + GPCLR0) = PIN13_MASK;  // Green off
    sleep(RED_YELLOW_DURATION);
    
    // Phase 3: GREEN ON (go)
    printf("Phase 3: GREEN ON (GO)\n");
    *(gpio + GPCLR0) = PIN10_MASK;  // Red off
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPSET0) = PIN13_MASK;  // Green on
    sleep(GREEN_DURATION);
    
    // Phase 4: PAUSE or BUTTON PRESS
    *(gpio + GPCLR0) = PIN10_MASK;  // Red off
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPSET0) = PIN13_MASK;  // Green on
    wait_for_button_or_timeout(5);
    
    // Phase 5: GREEN ON (continue go)
    printf("Phase 5: GREEN ON (CONTINUE GO)\n");
    *(gpio + GPCLR0) = PIN10_MASK;  // Red off
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPSET0) = PIN13_MASK;  // Green on
    sleep(2);
    
    // Phase 6: YELLOW BLINKING (attention - prepare to stop)
    printf("Phase 6: YELLOW BLINKING (ATTENTION - PREPARE TO STOP)\n");
    *(gpio + GPCLR0) = PIN10_MASK;  // Red off
    *(gpio + GPCLR0) = PIN13_MASK;  // Green off
    for (int i = 0; i < YELLOW_BLINK_COUNT; i++) {
        *(gpio + GPSET0) = PIN11_MASK;  // Yellow on
        usleep(YELLOW_BLINK_DELAY);
        *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
        usleep(YELLOW_BLINK_DELAY);
    }
    
    // Phase 7: RED ON (stop)
    printf("Phase 7: RED ON (STOP)\n");
    *(gpio + GPSET0) = PIN10_MASK;  // Red on
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPCLR0) = PIN13_MASK;  // Green off
    sleep(RED_DURATION);
    
    printf("\n=== Sequence Complete ===\n\n");
}

int main(int argc, char **argv) {
    printf("Traffic Lights Control - F28HS Lab Week 5\n");
    printf("==========================================\n\n");
    
    // Setup GPIO
    if (setup_gpio() == -1) {
        return 1;
    }
    
    // Initialize GPIO pins
    init_gpio();
    
    printf("GPIO initialized successfully.\n");
    printf("Red LED: GPIO 10 (BCM)\n");
    printf("Yellow LED: GPIO 11 (BCM)\n");
    printf("Green LED: GPIO 13 (BCM)\n");
    printf("Button (optional): GPIO 26 (BCM)\n\n");
    
    // Run traffic light sequence once
    traffic_light_sequence();
    
    // Turn off all LEDs
    *(gpio + GPCLR0) = PIN10_MASK;  // Red off
    *(gpio + GPCLR0) = PIN11_MASK;  // Yellow off
    *(gpio + GPCLR0) = PIN13_MASK;  // Green off
    
    printf("Program finished. All LEDs turned off.\n");
    
    return 0;
}