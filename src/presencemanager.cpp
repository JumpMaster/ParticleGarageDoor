#include "presencemanager.h"
#include "Particle.h"

extern PublishQueue pq;

PresenceManager::PresenceManager() { }

void PresenceManager::subscribe() {

}

void PresenceManager::process() {
}

void PresenceManager::PublishUser(User *user) {
    //pq.publish("presence", String::format("{\"method\":\"user\",\"type\":\"update\",\"user\":\"%s\",\"location\":\"%s\"}", user->name, user->location));
    pq.publish("TEST", String::format("{\"user\":\"%s\",\"location\":\"%s\"}", user->name, user->location));
}

void PresenceManager::PublishUsers() {
    for (uint8_t i = 0; i < users.size(); i++)
        PublishUser(&users[i]);
}

void PresenceManager::handler(const char *eventName, const char *data) {
}

void PresenceManager::updateUser(const char *name, const char *location) {

    bool new_user = true;

    for (uint8_t i = 0; i < users.size(); i++) {
        if (strcmp(users[i].name, name) == 0) {
            strcpy(users[i].location, location);
            new_user = false;
        }
    }
    
    if (new_user) {
        User new_user;
        strcpy(new_user.name, name);
        strcpy(new_user.location, location);
        users.push_back(new_user);
    }
}

bool PresenceManager::isAnyone(const char *location) {
    for (uint8_t i = 0; i < users.size(); i++) {
        if (strcmp(users[i].location, location) == 0)
            return true;
    }
    
    return false;
}

bool PresenceManager::isEveryone(const char *location) {
    for (uint8_t i = 0; i < users.size(); i++) {
        if (strcmp(users[i].location, location) != 0)
            return false;
    }
    
    return true;
}

bool PresenceManager::isUser(const char *user, const char *location) {
    for (uint8_t i = 0; i < users.size(); i++) {
        if (strcmp(users[i].name, user) == 0 && strcmp(users[i].location, location) == 0)
            return true;
    }
    
    return false;
}

char *PresenceManager::whereIs(const char *name) {
    for (uint8_t i = 0; i < users.size(); i++) {
        if (strcmp(users[i].name, name) == 0)
            return users[i].location;
    }
    
    return NULL;
}