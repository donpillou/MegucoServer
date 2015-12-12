
#pragma once

#include <nstd/Buffer.h>

#include <megucoprotocol.h>

#include "Tools/ZlimdbConnection.h"

class User2;

class Broker
{
public:
  Broker(User2& user, uint64_t brokerId) : user(user), brokerId(brokerId), 
    brokerTableId(0), balanceTableId(0), ordersTableId(0), transactionsTableId(0), logTableId(0) {}

  User2& getUser() {return user;}
  uint64_t getBrokerId() const {return brokerId;}

  void_t setBrokerTableId(uint32_t tableId) {brokerTableId = tableId;}
  uint32_t getBrokerTableId() const {return brokerTableId;}
  void_t setBalanceTableId(uint32_t tableId) {balanceTableId = tableId;}
  uint32_t getBalanceTableId() const {return balanceTableId;}
  void_t setOrdersTableId(uint32_t tableId) {ordersTableId = tableId;}
  uint32_t getOrdersTableId() const {return ordersTableId;}
  void_t setTransactionsTableId(uint32_t tableId) {transactionsTableId = tableId;}
  uint32_t getTransactionsTableId() const {return transactionsTableId;}
  void_t setLogTableId(uint32_t tableId) {logTableId = tableId;}
  uint32_t getLogTableId() const {return logTableId;}

  meguco_user_broker_state getState() const {return (meguco_user_broker_state)((const meguco_user_broker_entity*)(const byte_t*)brokerEntity)->state;}
  void_t setState(meguco_user_broker_state state) {((meguco_user_broker_entity*)(byte_t*)brokerEntity)->state = state;}
  void_t setEntity(const meguco_user_broker_entity& entity) {brokerEntity.assign((const byte_t*)&entity, entity.entity.size);}
  const zlimdb_entity& getEntity() const {return ((const meguco_user_broker_entity*)(const byte_t*)brokerEntity)->entity;}

private:
  User2& user;
  uint64_t brokerId;
  uint32_t brokerTableId;
  uint32_t balanceTableId;
  uint32_t ordersTableId;
  uint32_t transactionsTableId;
  uint32_t logTableId;
  Buffer brokerEntity;
  uint32_t processId;
};