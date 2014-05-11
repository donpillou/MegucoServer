
#pragma once

#include <nstd/String.h>
#include <nstd/List.h>
#include <nstd/Buffer.h>
#include "Tools/Socket.h"
#include "BotProtocol.h"

class BotConnection
{
public:
  BotConnection() : sessionId(0), balanceBase(0.), balanceComm(0.) {}

  bool_t connect(uint16_t port);
  void_t close() {socket.close();}
  bool_t isOpen() const {return socket.isOpen();}
  const String& getErrorString() const {return error;}

  const String& getMarketAdapterName() const {return marketAdapterName;}
  double getBalanceBase() const {return balanceBase;}
  double getBalanceComm() const {return balanceComm;}

  bool_t addLogMessage(const String& message);
  bool_t getMarketBalance(BotProtocol::MarketBalance& balance);
  bool_t getSessionTransactions(List<BotProtocol::Transaction>& transactions);
  bool_t getSessionOrders(List<BotProtocol::Order>& orders);

  bool_t createSessionTransaction(const BotProtocol::Transaction& transaction, uint32_t& id);
  bool_t removeSessionTransaction(uint32_t id);
  bool_t createSessionOrder(const BotProtocol::Order& order, uint32_t& id);
  bool_t removeSessionOrder(uint32_t id);

private:
  Socket socket;
  String error;
  Buffer recvBuffer;
  uint32_t sessionId;
  uint32_t marketId;
  String marketAdapterName;
  double balanceBase;
  double balanceComm;

private:
  bool_t createEntity(const void_t* data, size_t size, uint32_t& id);
  bool_t removeEntity(uint32_t type, uint32_t id);
  //bool_t sendPing();
  bool_t sendControlSession(BotProtocol::ControlSession::Command cmd);
  template<class E> bool_t sendControlMarket(BotProtocol::ControlMarket::Command cmd, List<E>& result);
  template<class E> bool_t sendControlSession(BotProtocol::ControlSession::Command cmd, List<E>& result);
  bool_t sendMessage(BotProtocol::MessageType type, uint32_t requestId, const void_t* data, size_t size);
  bool_t receiveMessage(BotProtocol::Header& header, byte_t*& data, size_t& size);
};
