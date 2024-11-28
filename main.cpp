/*----------------------------------------------------------------------------
LAB EXERCISE 5.3 - SPI interface
SERIAL COMMUNICATION
 ----------------------------------------
 Interface the LCD display using SPI protocol
	
	GOOD LUCK!
 *----------------------------------------------------------------------------*/
#include "mbed.h"
#include "NHD_0216HZ.h"
#include "platform/mbed_thread.h"
#include <iostream>

// Initialise the digital pin LED1 as an output
DigitalOut laser(D6);
DigitalIn sensor(D7);
//DigitalOut led(D4);
PwmOut speaker(D3);	
/*----------------------------------------------------------------------------
 MAIN function
 *----------------------------------------------------------------------------*/
void setLaser();

float minPeriod = 1.0f / 220.0f; //4khz
int score = 0;
char score_str[20];
int ammo = 10;   
char ammo_str[20];   

int main() {
    laser = 1; // Turn on the laser at the start

    init_spi();
    init_lcd();

     // Set the cursor and print a message
    set_cursor(0, 0);  // Move to top left corner
    print_lcd("P1S"); // Print a message

    set_cursor(4, 0);  // Move to top left corner
    print_lcd("P1A"); // Print a message

    set_cursor(0, 1);         // Move to the second row
    sprintf(score_str, "%d", score);  // Convert integer to string
    print_lcd(score_str); 

    set_cursor(4, 1);         // Move to the second row
    sprintf(ammo_str, "%d", ammo);  // Convert integer to string
    print_lcd(ammo_str); 
    
    set_cursor(9, 0);  // Move to top left corner
    print_lcd("P2S"); // Print a message

    set_cursor(9, 1);  // Move to top left corner
    print_lcd("0"); // Print a message

    set_cursor(13, 0);  // Move to top left corner
    print_lcd("P2A"); // Print a message

    set_cursor(13, 1);  // Move to top left corner
    print_lcd("10"); // Print a message

    while (true) {
        setLaser();
    }

}

void setLaser() {
    // Read the sensor value
    bool value = sensor.read();

    // If the sensor senses laser, turn on LED and buzzer
    if (value == 1) {
        //led = 1;        // Turn LED on
        speaker = 0.1;     // Turn buzzer on
        speaker.period(minPeriod); //

        score++;
        sprintf(score_str, "%d", score);  // Convert integer to string
        set_cursor(0, 1);         // Move to the second row
        print_lcd(score_str); 

        ammo--;
        sprintf(ammo_str, "%d", ammo);  // Convert integer to string
        set_cursor(4, 1);         // Move to the second row
        print_lcd(ammo_str); 


        printf("Intruder detected!\n");
    } else {
        //led = 0;        // Turn LED off
        speaker = 0;     // Turn buzzer off
    }

    // Delay for stability (optional)
    ThisThread::sleep_for(100);
}

// *******************************ARM University Program Copyright (c) ARM Ltd 2019*************************************
