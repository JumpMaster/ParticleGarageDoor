#ifndef __PRESENCEMANAGER_H__
#define __PRESENCEMANAGER_H__

#include "Particle.h"
#include <queue>

#define PLATFORM_PHOTON  6
#define PLATFORM_ARGON	12
#define PLATFORM_XENON	14

typedef struct User
{
  public:
    int id;
    char name[20];
	char location[20];
} User;

class PresenceManager
{
  public:
    PresenceManager();
    bool isAnyone(const char *location);
    bool isEveryone(const char *location);
    bool isUser(const char *name, const char *location);
    char *whereIs(const char *name);
    void updateUser(const char *name, const char *location);
  protected:
    std::vector<User> users;
};

#endif  // End of __PRESENCEMANAGER_H__ definition check