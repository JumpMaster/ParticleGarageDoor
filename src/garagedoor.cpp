#include "papertrail.h"
#include "mqtt.h"
#include "secrets.h"

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setDoorLock(bool locked);
void triggerDoor(const char *command, bool voiceCommand);
void release_button();
void sendNotification(const char *message);

unsigned long trigger_length = 500UL; // 1/2 second
unsigned long door_bounce_length = 1000UL; // 1 second
unsigned long max_door_move_time = 25000UL; // 25 seconds
unsigned long last_state_change;
int relay_pin = A0;

unsigned long resetTime = 0;

int front_sensor = D0;
int rear_sensor = D1;

enum {CLOSED, CLOSING, OPEN, OPENING, STUCK};
bool isLocked = true;
bool isArmed = true;
bool isAnyoneHome = false;
bool isDebug = false;
int door_state = CLOSED;

MQTT mqttClient(mqttServer, 1883, mqttCallback);
unsigned long lastMqttConnectAttempt;
const int mqttConnectAtemptTimeout = 5000;

Timer release_button_timer(trigger_length, release_button, true);

PapertrailLogHandler papertrailHandler(papertrailAddress, papertrailPort, "Garage Door");

ApplicationWatchdog wd(60000, System.reset);

void random_seed_from_cloud(unsigned seed) {
   srand(seed);
}

void triggerDoor(const char *command) {
    
    if (strcmp(command, "open") == 0) {
        if (door_state == OPEN)
            return;
        if (isLocked) {
            sendNotification("Garage door received an open command while locked");
            Log.info("Garage door received an open command while locked");
            return;
        }
    } else if (strcmp(command, "close") == 0) {
        if (door_state == CLOSED)
            return;
    } else if (strcmp(command, "stop") == 0) {
        if (door_state != OPENING && door_state != CLOSING)
            return;
    } else if (strcmp(command, "override") != 0) {
        Log.info("Not override, exiting");
        return;
    }
    
    digitalWrite(relay_pin, HIGH);
    release_button_timer.start();
}

void release_button() {
    digitalWrite(relay_pin, LOW);
}

void checkLockState() {
    bool newLockState = isArmed || !isAnyoneHome;

    if (isLocked != newLockState)
        setDoorLock(newLockState);
}

// recieve message
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0';

    if (isDebug)
        Log.info("%s - %s", topic, p);

    if (strcmp(topic, "utilities/isAnyoneHome") == 0) {

        if (strcmp(p, "true") == 0)
            isAnyoneHome = true;
        else
            isAnyoneHome = false;

        checkLockState();

    } else if (strcmp(topic, "home/security/alarm") == 0) {
        if (strstr(p, "disarmed")) {
            isArmed = false;
        } else {
            isArmed = true;
        }
        checkLockState();
    } else if (strcmp(topic, "home/garage/door/set") == 0) {
        triggerDoor(p);
    }
}

void connectToMQTT() {
    lastMqttConnectAttempt = millis();
    bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
    if (mqttConnected) {
        Log.info("MQTT Connected");
        mqttClient.subscribe("home/security/alarm");
        mqttClient.subscribe("home/garage/door/set");
        mqttClient.subscribe("utilities/#");
    } else
        Log.info("MQTT failed to connect");
}

void sendNotification(const char *message) {
    if (mqttClient.isConnected())
        mqttClient.publish("home/notification/low", message, false);
}

void setDoorLock(bool locked) {
    if (isLocked != locked) {
        isLocked = locked;
        
        if (isLocked) {
            sendNotification("Garage door is locked");
            Log.info("Garage door is locked");
        } else {
            sendNotification("Garage door is unlocked");
            Log.info("Garage door is unlocked");
        }
    }   
}

void setDoorState(int newState, bool locked) {
    
    if (newState != door_state) {
        //Do something to broadcast the state change?
        door_state = newState;
        char doorPosition[4];
        doorPosition[0] = '\0';

        if (newState == OPEN) {
            snprintf(doorPosition, sizeof(doorPosition), "%d", 100);
        } else if (newState == CLOSED) {
            snprintf(doorPosition, sizeof(doorPosition), "%d", 0);
            
        }
        
        if (doorPosition[0] != '\0' && mqttClient.isConnected()) {
            mqttClient.publish("home/garage/door/state", doorPosition, true);
        }
    }
}

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
STARTUP(System.enableFeature(FEATURE_RESET_INFO));
SYSTEM_THREAD(ENABLED)

void setup() {
    pinMode(relay_pin, OUTPUT);
    pinMode(front_sensor, INPUT_PULLUP);
    pinMode(rear_sensor, INPUT_PULLUP);
    
    waitUntil(Particle.connected);

    do
    {
        resetTime = Time.now();
        delay(10);
    } while (resetTime < 1000000 && millis() < 20000);

    connectToMQTT();
    
    Particle.variable("reset-time", &resetTime, INT);
    Particle.publishVitals(900);
}

void loop() {
    int _front_sensor_state = digitalRead(front_sensor);
    int _rear_sensor_state = digitalRead(rear_sensor);
    int new_door_state = door_state;
    
    if (door_state != OPENING && _front_sensor_state == LOW && _rear_sensor_state == HIGH)
        new_door_state = CLOSED;
    else if (door_state != CLOSING &&_front_sensor_state == HIGH && _rear_sensor_state == LOW)
        new_door_state = OPEN;
    else if (door_state != STUCK && _front_sensor_state == HIGH && _rear_sensor_state == HIGH) {
        if (door_state != OPENING && door_state != CLOSING) {
            if (door_state == OPEN)
                new_door_state = CLOSING;
            else if (door_state == CLOSED)
                new_door_state = OPENING;
        }
    }
    
    if (new_door_state != door_state &&
        (millis() > (last_state_change + door_bounce_length))) {
        last_state_change = millis();

        setDoorState(new_door_state, isLocked);
    }
    
    if ((door_state == CLOSING || door_state == OPENING) && millis() > (last_state_change + max_door_move_time)) {
            setDoorState(STUCK, isLocked);
            sendNotification("Garage door is stuck");
    }
    
    if (mqttClient.isConnected()) {
        mqttClient.loop();
    } else if (millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout)) {
        Log.info("MQTT Disconnected");
        connectToMQTT();
    }

    wd.checkin(); // resets the AWDT count
}