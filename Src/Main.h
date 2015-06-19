
#pragma once

#include <nstd/HashMap.h>
#include <nstd/List.h>
#include <nstd/Mutex.h>

#include "Tools/ZlimdbConnection.h"
#include "Tools/ProcessManager.h"

class Main : public ZlimdbConnection::Callback, public ProcessManager::Callback
{
public:
  Main(const String& binaryDir) : binaryDir(binaryDir) {}
  bool_t init();
  const String& getErrorString() const {return error;}
  bool_t connect();
  bool_t process();

private:
  struct Process
  {
    uint64_t entityId;
    String command;
    uint32_t processId;
  };

private:
  String error;
  String binaryDir;
  ZlimdbConnection connection;
  uint32_t processesTableId;
  ProcessManager processManager;
  HashMap<uint32_t, Process> processesById;
  HashMap<uint64_t, Process*> processes;
  Mutex mutex;
  List<uint32_t> terminatedProcesses;

private:
  void_t removeProcess(uint32_t processId);
  bool_t killProcess(uint32_t processId);

  void_t addedProcess(const Process& process);
  void_t removedProcess(uint64_t entityId);

private: // ProcessManager::Callback
  virtual void_t terminatedProcess(uint32_t processId);

private: // ZlimdbConnection::Callback
  virtual void_t addedEntity(uint32_t tableId, const zlimdb_entity& entity);
  virtual void_t updatedEntity(uint32_t tableId, const zlimdb_entity& entity) {}
  virtual void_t removedEntity(uint32_t tableId, uint64_t entityId);
  virtual void_t controlEntity(uint32_t tableId, uint64_t entityId, uint32_t controlCode, const Buffer& buffer) {}
};
