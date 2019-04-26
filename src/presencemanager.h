#ifndef __PRESENCEMANAGER_H__
#define __PRESENCEMANAGER_H__

#include "application.h"
#include "publishqueue.h"

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
    void subscribe();
    void process();
    void handler(const char *eventName, const char *data);
    void syncRequest();
    bool isAnyone(const char *location);
    bool isEveryone(const char *location);
    bool isUser(const char *name, const char *location);
    char *whereIs(const char *name);
    bool hasSync;
    void updateUser(const char *name, const char *location);
    void PublishUsers();
  protected:
    const unsigned long masterCallbackTime = 60000; // 1 minutes
    void PublishUser(User *user);
    void ExportUser(const char *name, const char *location);
    std::vector<User> users;
    unsigned long lastMasterUpdate;
    int nodeID;
    int masterID;
    int clusterUsers;
};

#endif  // End of __PRESENCEMANAGER_H__ definition check