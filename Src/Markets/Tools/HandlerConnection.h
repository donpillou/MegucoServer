
#pragma once

#include <nstd/String.h>
#include <nstd/List.h>
#include <nstd/Buffer.h>

#include "Tools/Socket.h"
#include "BotProtocol.h"

class HandlerConnection
{
public:
  HandlerConnection() {}

  bool_t connect(uint16_t port);
  void_t close() {socket.close();}
  bool_t isOpen() const {return socket.isOpen();}
  const String& getErrorString() const {return error;}

  const String& getUserName() const {return userName;}
  const String& getKey() const {return key;}
  const String& getSecret() const {return secret;}

  bool_t sendMessage(BotProtocol::MessageType type, uint32_t requestId, const void_t* data, size_t size);
  bool_t sendMessageHeader(BotProtocol::MessageType type, uint32_t requestId, size_t dataSize);
  bool_t sendMessageData(const void_t* data, size_t size);
  bool_t sendErrorResponse(BotProtocol::MessageType messageType, uint32_t requestId, const BotProtocol::Entity* entity, const String& errorMessage);
  bool_t receiveMessage(BotProtocol::Header& header, byte_t*& data, size_t& size);

private:
  Socket socket;
  String error;
  Buffer recvBuffer;

  String userName;
  String key;
  String secret;
};