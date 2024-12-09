#include "mbed.h"
#include "platform/mbed_thread.h"

// Define LEDs
DigitalOut greenLED(D8);
DigitalOut redLED(D9);

// Other peripherals and variables
DigitalOut laser(D6);
PwmOut speaker(D3);

InterruptIn trigger(D4);
InterruptIn sensor(D7);
InterruptIn reloadButton(D10);

Timer debounceTimer;
Ticker respawnTimer;
Timer sensorDebounceTimer;

float gunShot = 1.0f / 200.0f; // 4 kHz
int max_ammo = 15;
int ammo = max_ammo;

bool isRespawning = false;
bool death = false;

volatile bool triggerPressed = false;
volatile bool playBuzzer = false;
volatile bool reloadPressed = false;
volatile bool sensorHit = false; // Flag for sensor hit

int respawnTime = 3;
int maxHits = 5;  // Maximum number of allowed hits
int currentHits = 0; // Current number of hits

// Function Declarations
void update_led_status();
void trigger_ISR();
void sensor_ISR();
void reload_ISR();
void respawn_logic();
void shoot();
void reload_ammo();
void update_ammo_display(int ammo);
void initial_display();
void test_laser();

/*----------------------------------------------------------------------------*/
int main() {
    laser = 0;
    speaker = 0;
    debounceTimer.start();
    sensorDebounceTimer.start();
    
    // Attach ISRs
    trigger.fall(&trigger_ISR);
    sensor.rise(&sensor_ISR);
    reloadButton.fall(&reload_ISR);

    initial_display();

    // Set initial LED states
    greenLED = 1; // Green LED on initially
    redLED = 0;   // Red LED off initially

    while (true) {
        if (playBuzzer) {
            speaker = 0.1;
            ThisThread::sleep_for(3000ms);
            playBuzzer = false;
            death = true;
            speaker = 0;
            // Set flag for respawn
            isRespawning = true;
        }

        if (triggerPressed) {
            triggerPressed = false;
            shoot();
        }

        if (reloadPressed) {
            reloadPressed = false;
            reload_ammo();
        }

        if (sensorHit) {
            sensorHit = false; // Clear the flag
            if (currentHits < maxHits) {
                currentHits++; // Increment the hit count
                if (currentHits >= maxHits) {
                    death = true; // If max hits are reached, trigger death
                    laser = 0;
                    isRespawning = true;
                    playBuzzer = true;
                }
            }
        }

        // Update LED status
        update_led_status();

        // Handle respawn logic if needed
        if (isRespawning) {
            respawn_logic();
        }

        ThisThread::sleep_for(10ms);
    }
}

/*----------------------------------------------------------------------------*/
// Update the status of the red and green LEDs based on ammo and respawn state
void update_led_status() {
    if (death) {
        greenLED = 0;  // Turn off green LED
        redLED = 1;    // Turn on red LED
    } else if (ammo == 0) {
        greenLED = 0;  // Turn off green LED
        for (int i = 0; i < 10; i++) { // Flash red LED 10 times
            redLED = !redLED;
            ThisThread::sleep_for(100ms);
        }
    } else {
        greenLED = 1;  // Keep green LED on
        redLED = 0;    // Ensure red LED is off
    }
}

/*----------------------------------------------------------------------------*/
// ISR for trigger press
void trigger_ISR() {
    if (debounceTimer.read_ms() > 200) {
        triggerPressed = true;
        debounceTimer.reset();
    }
}

/*----------------------------------------------------------------------------*/
// ISR for sensor hit (death logic)
void sensor_ISR() {

    if (sensorDebounceTimer.read_ms() > 200) {  // 200ms debounce period
        sensorHit = true;  // Set the flag to indicate the sensor was hit
        sensorDebounceTimer.reset();  // Reset the debounce timer
    }
}

/*----------------------------------------------------------------------------*/
// ISR for reload button press
void reload_ISR() {
    if (debounceTimer.read_ms() > 200) {
        reloadPressed = true;
        debounceTimer.reset();
    }
}

/*----------------------------------------------------------------------------*/
// Respawn logic handled in the main loop
void respawn_logic() {
    // Reset death flag
    death = false;               
    isRespawning = false;        // Reset respawning state
    currentHits = 0;             // Reset the hit count
    ammo = max_ammo;             // Refill ammo after respawn
    update_ammo_display(ammo);   // Update the ammo display

    // Reset laser and speaker
    laser = 0;
    speaker = 0;

    printf("Respawn completed. You are back in the game!\n");
}

/*----------------------------------------------------------------------------*/
// Shooting logic
void shoot() {
    if (ammo > 0) {
        printf("Shooting! Ammo remaining: %d\n", ammo);
        laser = 1;        // Turn on the laser
        speaker = 0.1;    // Activate speaker at low power
        ThisThread::sleep_for(900ms); // Simulate shot for 700 ms (longer shot)
        laser = 0;        // Turn off the laser
        speaker = 0;      // Turn off the speaker

        ammo--;           // Decrease ammo count
        update_ammo_display(ammo); // Update ammo display
    } else {
        printf("Out of ammo! Reload to continue.\n");
    }
}

/*----------------------------------------------------------------------------*/
// Reload ammo
void reload_ammo() {
    ammo = max_ammo;
    printf("Ammo reloaded!\n");
    update_ammo_display(ammo);
}

/*----------------------------------------------------------------------------*/
// Update ammo display on LCD
void update_ammo_display(int ammo) {
    printf("Ammo: %d\n", ammo); // Replace with LCD update logic
}

/*----------------------------------------------------------------------------*/
// Initial display setup
void initial_display() {
    printf("Game Start\n"); 
    printf("Health: OK\n"); // Health status
    update_ammo_display(max_ammo);
}

/*----------------------------------------------------------------------------*/
// Laser and speaker test function
void test_laser() {
    printf("Activating laser and speaker...\n");
    laser = 1;      // Turn on the laser
    speaker = 0.1; 
    ThisThread::sleep_for(500ms); // Keep active for 500 ms
    laser = 0;      // Turn off the laser
    speaker = 0;    // Turn off the speaker
    printf("Laser and speaker test complete.\n");
}
