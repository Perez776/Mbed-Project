
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
InterruptIn reloadButton(D2);

void trigger_ISR();
void sensor_ISR();
void reloadButton_ISR();
Timer debounceTimer;
Timer burstTimer;

float gunShot = 1.0f / 200.0f; //4khz
float hitMarkerSound = 1.0f / 500.0f;

//Display
int Kills = 0;
char kills_str[20];
int deaths = 0;
char deaths_str[20];   

//Ammo And Reloading
int max_ammo = 500;
int ammo = max_ammo;
int reloadTime = 3;
Ticker reloadTimer;
void reloadTimer_ISR();
volatile bool reloading = false;

//shooting
int burst_time = 4000;
bool automatic = false;
bool isShooting = false;
volatile bool triggerPressed = false;

//Deaths
int respawnTime = 3;
bool isRespawning = false; //Check if respaqning
volatile bool playBuzzer = false;  // Flag to control buzzer in main loop
volatile bool death = false;
Ticker respawnTimer;
void timer_ISR();

/*----------------------------------------------------------------------------
 MAIN function
 *----------------------------------------------------------------------------*/
int main() {

    laser = 0;
    speaker = 0;     
    speaker.period(gunShot); //

    trigger.fall(&trigger_ISR); 
    sensor.rise(&sensor_ISR); 
    reloadButton.fall(&reloadButton_ISR); 

    initial_display();

    debounceTimer.start();  

    while (true) {

        //play buzzer when dying and increment death count
        if (playBuzzer) {
            speaker = 0.1;  // Activate the buzzer
            //speaker.period(hitMarkerSound);  // Set buzzer to hit sound frequency
            deaths++; //add death
            printf("Total Deaths: %d \n", deaths);

            ThisThread::sleep_for(3000ms);  // Sleep for 1s to let buzzer play
            playBuzzer = false;  // Reset buzzer flag
            speaker = 0;
        } else {
            speaker = 0;  // Turn off buzzer
        }

        if (triggerPressed && ammo > 0) {

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
                isShooting = false;
            } else {
                shoot();  //Semi-Automatic Or Burst
            }
        }
    

        ThisThread::sleep_for(10ms); 
    }
}

//ISR for when reload timer stops
void reloadTimer_ISR() {
    if (reloading) {
        ammo = max_ammo;
        reloading = false;
        reloadTimer.detach();  //Detach the ticker interrupt
    }
}

void reloadButton_ISR() {
    if (!reloading) {  // Start reloading only if not already in progress
        reloading = true;
        reloadTimer.attach(&reloadTimer_ISR, reloadTime);
    }
}


//ISR for when respawn timer stops
void timer_ISR() {
    if (death) {
        death = false;  // Reset death flag
        isRespawning = false;  // Reset respawning flag
        respawnTimer.detach();  //Detach the ticker interrupt
    }
}

//For reciever
void sensor_ISR() {
    //Add death and respawn
    if (!isRespawning) { 
        death = true; 
        laser = 0;
        isRespawning = true; 
        playBuzzer = true; 

        respawnTimer.attach(&timer_ISR, respawnTime);
    }
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

    // Cancel reload if it was in progress, then shoot
    if (reloading) {
        printf("Reloading canceled due to shooting!\n");
        reloading = false;
        reloadTimer.detach();  // Stop the reload timer
    }

    isShooting = true;  // flag to indicate shooting in progress
    laser = 1;   

    
    
    burstTimer.start();  // Start the burst timer
    while (burstTimer.read_ms() < burst_time && ammo > 0) {

        if(death || reloading) {
            //reloading = false;
            break;
        }

        if(automatic) {
           if (trigger == 1 || ammo < 1) {  // If trigger is released, stop shooting immediately
            laser = 0;
            burstTimer.stop();  // Stop the burst timer
            burstTimer.reset(); // Reset the timer
            isShooting = false;  // Reset the flag

            return;
           }

            ammo--;
            printf("Remaining Ammo: %d \n", ammo);
        }


        ammo--;
        printf("Remaining Ammo: %d \n", ammo);
    }

    laser = 0;  // Turn off laser after burst time
    
    burstTimer.stop();  // Stop the burst timer
    burstTimer.reset(); // Reset the timer

    // Cooldown after burst
    if (!automatic) {
        ThisThread::sleep_for(1000); // Brief cooldown period
    }

    isShooting = false;  // Reset flag to allow next shot


        
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