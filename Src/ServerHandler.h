
#pragma once

#include <nstd/HashMap.h>

#include "Tools/Server.h"

class ClientHandler;
class User;
class Session;
class Engine;
class Market;

class ServerHandler : public Server::Listener
{
public:
  ServerHandler(uint16_t port) : port(port), nextEntityId(1) {}
  ~ServerHandler();

  void_t addEngine(const String& path);
  const HashMap<uint32_t, Engine*>& getEngines() const {return engines;}
  bool_t addUser(const String& userName, const String& password);
  User* findUser(const String& userName);
  Session* findSessionByPid(uint32_t pid);
  void_t registerSession(uint32_t pid, Session& session);
  void_t unregisterSession(uint32_t pid);
  void_t addMarket(const String& name, const String& currencyBase, const String& currencyComm);
  const HashMap<uint32_t, Market*>& getMarkets() const {return markets;}

  bool_t loadData();
  bool_t saveData();

private:
  uint16_t port;
  uint32_t nextEntityId;
  HashMap<uint64_t, ClientHandler*> clients;
  HashMap<String, User*> users;
  HashMap<uint32_t, Session*> sessions;
  HashMap<uint32_t, Engine*> engines;
  HashMap<uint32_t, Market*> markets;

  virtual void_t acceptedClient(Server::Client& client, uint32_t addr, uint16_t port);
  virtual void_t closedClient(Server::Client& client);
};
