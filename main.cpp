
#include "mbed.h"
#include "NHD_0216HZ.h"
#include "platform/mbed_thread.h"
#include <iostream>

#define DEBOUNCE_DELAY 200ms  // Define a debounce delay (200 milliseconds)

#define HIGH_PRIORITY   0   // Highest priority
#define LOW_PRIORITY    255 // Lowest priority

DigitalOut laser(D6);
//DigitalOut led(D4);
PwmOut speaker(D3);	

void shoot();
void initial_display();
void update_ammo_display(int ammo);

InterruptIn trigger(D4);
InterruptIn sensor(D7);


void trigger_ISR();
void sensor_ISR();
Timer debounceTimer;
Timer burstTimer;


float gunShot = 1.0f / 200.0f; //4khz
float hitMarkerSound = 1.0f / 500.0f;

int Kills = 0;
char kills_str[20];
int deaths = 0;
char deaths_str[20];   
int max_ammo = 15;
int ammo = max_ammo;
int reload_time = 3000;

int burst_time = 2000;
bool automatic = false;

bool isShooting = false;

volatile bool triggerPressed = false;


/*----------------------------------------------------------------------------
 MAIN function
 *----------------------------------------------------------------------------*/
int main() {

    laser = 0;
    speaker = 0;     
    speaker.period(gunShot); //

    trigger.fall(&trigger_ISR); 
    sensor.fall(&sensor_ISR); 

    //trigger.set_priority(LOW_PRIORITY);
    initial_display();

    debounceTimer.start();  

    while (true) {

        if (triggerPressed) {
            printf("Shooting ... \n");
            triggerPressed = false; // Reset flag

           if (automatic) {
                while (trigger == 0) { // While trigger is pressed
                    if (!isShooting) {
                        shoot();
                    }
                    ThisThread::sleep_for(10ms);
                }
                laser = 0;
                speaker = 0;
                isShooting = false;
            } else {
                shoot();  //Semi-Automatic Or Burst
            }

        }

    }
}

void sensor_ISR() { 
    speaker = 0.1;     // Turn buzzer on
    speaker.period(hitMarkerSound); //
}

void trigger_ISR() { 
    if (debounceTimer.read_ms() > DEBOUNCE_DELAY.count() && !laser) {
        triggerPressed = true;
        debounceTimer.reset();  // Reset the debounce timer
    }
}

void shoot() {
    if (isShooting) {
        // If already shooting, don't trigger another shot
        return;
    }

    isShooting = true;  // flag to indicate shooting in progress
    laser = 1;   
    speaker = 0.8; // Set PWM duty cycle
    speaker.period(gunShot);
    
    
    burstTimer.start();  // Start the burst timer
    while (burstTimer.read_ms() < burst_time) {

        if(automatic) {
           if (trigger == 1) {  // If trigger is released, stop shooting immediately
            laser = 0;
            speaker = 0;
            burstTimer.stop();  // Stop the burst timer
            burstTimer.reset(); // Reset the timer
            isShooting = false;  // Reset the flag
            return;
           }
        }
    }

    laser = 0;  // Turn off laser after burst time
    speaker = 0; // Turn off buzzer
    
    burstTimer.stop();  // Stop the burst timer
    burstTimer.reset(); // Reset the timer

    // Cooldown after burst
      if (!automatic) {
        ThisThread::sleep_for(1000); // Brief cooldown period
    }

    isShooting = false;  // Reset flag to allow next shot

    //ammo--;
        
    /*
    update_ammo_display(ammo);

        if(ammo == 0) {
            speaker = 0;
            laser = 0;
            //printf("reloading ...");
            //ThisThread::sleep_for(reload_time);
            ammo = max_ammo;
            update_ammo_display(ammo);
        }

        //printf("Intruder detected!\n");
    } else {
        //led = 0;        // Turn LED off
        speaker = 0;     // Turn buzzer off
        laser = 0;
    }
*/
    //ThisThread::sleep_for(100);
}

void update_ammo_display(int ammo) {
    // Clear previous ammo value
    set_cursor(6, 1); 
    print_lcd("   "); 

    // Display the new ammo value
    char ammo_str[4]; // (0-999)
    sprintf(ammo_str, "%d", ammo);  // Convert ammo to string
    set_cursor(6, 1);
    print_lcd(ammo_str); // Display the new ammo value
}


/*

void shoot() {

    if (trigger == 0) {
        laser = 1;

        speaker = 0.1;     // Turn buzzer on
        speaker.period(minPeriod); //

        Kills++;
        sprintf(kills_str, "%d", Kills);  // Convert integer to string
        set_cursor(0, 1);         // Move to the second row
        print_lcd(kills_str); 

        ammo--;
        update_ammo_display(ammo);

        if(ammo == 0) {
            speaker = 0;
            laser = 0;
            //printf("reloading ...");
            //ThisThread::sleep_for(reload_time);
            ammo = max_ammo;
            update_ammo_display(ammo);
        }

        //printf("Intruder detected!\n");
    } else {
        //led = 0;        // Turn LED off
        speaker = 0;     // Turn buzzer off
        laser = 0;
    }

    //ThisThread::sleep_for(100);
}

void update_ammo_display(int ammo) {
    // Clear previous ammo value
    set_cursor(6, 1); 
    print_lcd("   "); 

    // Display the new ammo value
    char ammo_str[4]; // (0-999)
    sprintf(ammo_str, "%d", ammo);  // Convert ammo to string
    set_cursor(6, 1);
    print_lcd(ammo_str); // Display the new ammo value
}
*/

void initial_display() {
    init_spi();
    init_lcd();
    // Kills
    set_cursor(0, 0); 
    print_lcd("K");

    set_cursor(0, 1);     
    print_lcd("0"); 

    //Deaths
    set_cursor(3, 0);
    print_lcd("D"); 

    set_cursor(3, 1);
    print_lcd("0"); 
    
    //Ammo
    set_cursor(6, 0);  
    print_lcd("Ammo"); 

    set_cursor(6, 1);         
    update_ammo_display(max_ammo);
    
     //Red Team
    set_cursor(11, 0);
    print_lcd("R");

    set_cursor(11, 1); 
    print_lcd("0");
    
    //Green Team
    set_cursor(14, 0);  
    print_lcd("G"); 

    set_cursor(14, 1);  
    print_lcd("0");
}
