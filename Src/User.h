
#pragma once

#include <nstd/String.h>
#include <nstd/HashMap.h>
#include <nstd/HashSet.h>

#include "BotProtocol.h"

class ServerHandler;
class ClientHandler;
class Session;
class BotEngine;
class MarketAdapter;
class Market;

class User
{
public:
  User(ServerHandler& serverHandler, const String& userName, const byte_t (&key)[32], const byte_t (&pwhmac)[32]);
  ~User();

  void_t registerClient(ClientHandler& client) {clients.append(&client);}
  void_t unregisterClient(ClientHandler& client) {clients.remove(&client);}

  Session* createSession(const String& name, BotEngine& engine, Market& market, double balanceBase, double balanceComm);
  bool_t deleteSession(uint32_t id);
  Session* findSession(uint32_t id) {return *sessions.find(id);}

  Market* createMarket(MarketAdapter& marketAdapter, const String& username, const String& key, const String& secret);
  bool_t deleteMarket(uint32_t id);
  Market* findMarket(uint32_t id) {return *markets.find(id);}

  void_t sendEntity(const void_t* data, size_t size);
  void_t removeEntity(BotProtocol::EntityType type, uint32_t id);
  
  const String& getUserName() const {return userName;}
  const byte_t* getKey() const {return key;}
  const byte_t* getPwHmac() const {return pwhmac;}
  const HashMap<uint32_t, Session*>& getSessions() const {return sessions;}
  const HashMap<uint32_t, Market*>& getMarkets() const {return markets;}
  
  bool_t loadData();
  bool_t saveData();

private:
  ServerHandler& serverHandler;
  String userName;
  byte_t key[32];
  byte_t pwhmac[32];
  HashSet<ClientHandler*> clients;
  HashMap<uint32_t, Session*> sessions;
  HashMap<uint32_t, Market*> markets;
  uint32_t nextEntityId;
};
