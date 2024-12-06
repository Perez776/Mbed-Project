
#include "mbed.h"
#include "NHD_0216HZ.h"
#include "platform/mbed_thread.h"
#include <iostream>
#include <string>

#define DEBOUNCE_DELAY 200ms  // Define a debounce delay (200 milliseconds)

//Outputs
DigitalOut laser(D6);
PwmOut speaker(D3);	

//Display LCD
void initial_display();
void update_display(int value, int size, int x, int y);

//Interrupts
InterruptIn trigger(D4);
InterruptIn sensor(D7);
InterruptIn reloadButton(D2);
InterruptIn switchShootingMode(D8);

Timer debounceTimer;

//Sound FX
float gunShot = 1.0f / 200.0f; //4khz
float hitMarkerSound = 1.0f / 500.0f;

//Ammo And Reloading
int max_ammo = 500;
int ammo = max_ammo;
int reloadTime = 7;
volatile bool reloading = false;
volatile bool reloadFinished = false;
Ticker reloadTimer;
void reloadTimer_ISR();
void reloadButton_ISR();

//Shooting
int burst_time = 4000;
bool automatic = true;
bool isShooting = false;
volatile bool triggerPressed = false;
Timer burstTimer;
void shoot();
void trigger_ISR();

//Deaths
int deaths = 0;
int respawnTime = 5;
bool isRespawning = false; //Check if respaqning
volatile bool playBuzzer = false;  // Flag to control buzzer in main loop
volatile bool death = false; //if player is dead
Ticker respawnTimer; //timer for respawn
void respawnTimer_ISR(); //ISR for respawnTimer
void sensor_ISR();

//Shooting Mode
int currentMode = 0;
string currentMode_str = "A";
volatile bool switchingModes = false;
void switchShootingMode_ISR();

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
    switchShootingMode.rise(&switchShootingMode_ISR);

    initial_display();

    debounceTimer.start();  

    while (true) {

        //play buzzer when dying and increment death count
        if (playBuzzer) {
            speaker = 0.1;  // Activate the buzzer
            deaths++; //add death
            printf("Total Deaths: %d \n", deaths);
            update_display(deaths, 4, 3, 1); //Display to LCD

            ThisThread::sleep_for(3000ms);  // Sleep for 3s to let buzzer play and respawn
            playBuzzer = false;  // Reset buzzer flag
            speaker = 0;
        } else {
            speaker = 0;  // Turn off buzzer
        }

        //When Trigger Button is pressed
        if (triggerPressed && ammo > 0) {

            printf("Shooting ... \n");
            triggerPressed = false; // Reset flag

            //If Automatic Shooting
           if (automatic) {
                while (trigger == 0) { // While trigger is pressed
                    if (!isShooting) {
                        shoot();
                    }
                    ThisThread::sleep_for(10ms);
                }
                laser = 0;
                isShooting = false;
            } 
            //Burst Shooting
            else {
                shoot();  //Semi-Automatic Or Burst
            }
        }

        //Disaply new ammo count
        if(reloadFinished) {
            reloadFinished = false;
            
            update_display(ammo, 4, 6, 1);
            printf("Finished Reloading ---\n");
        }

        //Display current shooting mode
        if(switchingModes == true) {
            switchingModes = false;
            printf("Switching Mode: %d \n", currentMode);
            set_cursor(15, 1);
            print_lcd(currentMode_str); // Display the new ammo value
        }

        ThisThread::sleep_for(10ms); 
    }
}

/*----------------------------------------------------------------------------*/
//Change Mode From automatic, semi, burst and adjust reload time.
void switchShootingMode_ISR() {

    if (debounceTimer.read_ms() > DEBOUNCE_DELAY.count()) {

        currentMode++;        

        if(currentMode == 3) {
            currentMode = 0;
        }

        switchingModes = true;

        if(currentMode == 0) {
            automatic = true;
            currentMode_str = "A";
            reloadTime = 7;
            max_ammo = 500;
        }
        if(currentMode == 1) {
            automatic = false;
            burst_time = 1000;
            currentMode_str = "S";
            reloadTime = 2;
            max_ammo = 300;
            ammo = max_ammo;
        }
        if(currentMode == 2) {
            automatic = false;
            burst_time = 3000;
            currentMode_str = "B";
            reloadTime = 5;
            max_ammo = 400;
        }

        debounceTimer.reset(); 
    }
}
/*----------------------------------------------------------------------------*/
//ISR for when reload timer stops
void reloadTimer_ISR() {
    if (reloading) {
        ammo = max_ammo; //give max ammo
        reloading = false;
        reloadFinished = true;
        reloadTimer.detach();  //Detach the ticker interrupt
    }
}

/*----------------------------------------------------------------------------*/
//ISR for when reload button is pressed
void reloadButton_ISR() {
    if (!reloading) {  // Start reloading only if not already in progress
        reloading = true;
        reloadTimer.attach(&reloadTimer_ISR, reloadTime); //Start timer
    }
}

/*----------------------------------------------------------------------------*/
//ISR for when respawn timer stops
void respawnTimer_ISR() {
    if (death) {
        death = false;  // Reset death flag
        isRespawning = false;  // Reset respawning flag
        respawnTimer.detach();  //Detach the ticker interrupt
    }
}

/*----------------------------------------------------------------------------*/
//ISR For reciever. (when getting shot)
void sensor_ISR() {
    //Add death and respawn
    if (!isRespawning) { 
        death = true; 
        laser = 0;
        isRespawning = true; 
        playBuzzer = true; 

        respawnTimer.attach(&respawnTimer_ISR, respawnTime);
    }
}

/*----------------------------------------------------------------------------*/
void trigger_ISR() { 
    if (debounceTimer.read_ms() > DEBOUNCE_DELAY.count() && !laser) {
        triggerPressed = true;
        debounceTimer.reset();  // Reset the debounce timer
    }
}

/*----------------------------------------------------------------------------*/
//Shooting Function that also handles ammo count.
void shoot() {
    // If already shooting, don't trigger another shot
    if (isShooting) {
        return;
    }

    // Cancel reload if it was in progress, then shoot
    if (reloading) {
        printf("Reloading canceled due to shooting!\n");
        reloading = false;
        reloadTimer.detach();  // Stop the reload timer
        return;
    }

    isShooting = true;  // flag to indicate shooting in progress
    laser = 1;   
    
    burstTimer.start();  // Start the burst timer
    //Burst shot
    while (burstTimer.read_ms() < burst_time && ammo > 0) {

        if(death || reloading) {
            //reloading = false;
            break;
        }

        //For automatic, keep shooting bursts rapidly
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
            update_display(ammo, 4, 6, 1);
        }


        ammo--;
        printf("Remaining Ammo: %d \n", ammo);
        update_display(ammo, 4, 6, 1);
    }

    laser = 0;  // Turn off laser after burst time
    
    burstTimer.stop();  // Stop the burst timer
    burstTimer.reset(); // Reset the timer

    // Cooldown after burst
    if (!automatic) {
        ThisThread::sleep_for(1000); // Brief cooldown period
    }

    isShooting = false;  // Reset flag to allow next shot
}

/*----------------------------------------------------------------------------*/
//Update LCD Display Stats
void update_display(int value, int size, int x, int y) {
    // Clear previous ammo value
    set_cursor(x, y); 
    print_lcd("   "); 

    // Display the new ammo value
    char value_str[size]; // 4 = (0-999)
    sprintf(value_str, "%d", value);  // Convert ammo to string
    set_cursor(x, y);
    print_lcd(value_str); // Display the new ammo value
}

/*----------------------------------------------------------------------------*/
//Initial LCD Display
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
    update_display(max_ammo, 4, 6, 1);   

    //Shooting Mode
    set_cursor(15, 0);  
    print_lcd("M"); 

    set_cursor(15, 1);         
    print_lcd("A");   

    /*
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
    */
}