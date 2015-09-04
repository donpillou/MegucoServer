
#include <nstd/Console.h>
#include <nstd/File.h>
#include <nstd/Thread.h>

#include "Main.h"
#include "User2.h"
#include "Market2.h"
#include "Session2.h"

int_t main(int_t argc, char_t* argv[])
{
  String binaryDir = File::dirname(String(argv[0], String::length(argv[0])));

  // initialize connection handler
  Main main;

  // load market list
  main.addBotMarket("Bitstamp/BTC/USD", binaryDir + "Brokers/BitstampBtcUsd");

  // load bots
  main.addBotEngine("BetBot", binaryDir + "Bots/BetBot");
  main.addBotEngine("BetBot2", binaryDir + "Bots/BetBot2");
  main.addBotEngine("FlipBot", binaryDir + "Bots/FlipBot");
  main.addBotEngine("TestBot", binaryDir + "Bots/TestBot");

  // main loop
  for(;; Thread::sleep(10 * 1000))
  {
    // connect to zlimdb server
    if(!main.connect())
    {
        Console::errorf("error: Could not connect to zlimdb server: %s\n", (const char_t*)main.getErrorString());
        continue;
    }
    Console::printf("Connected to zlimdb server.\n");

    // run connection handler loop
    main.process();

    Console::errorf("error: Lost connection to zlimdb server: %s\n", (const char_t*)main.getErrorString());
  }
  return 0;
}

Main::~Main()
{
  disconnect();
}

void_t Main::addBotMarket(const String& name, const String& executable)
{
  BotMarket botMarket = {name, executable};
  botMarketsByName.append(name, botMarket);
}

void_t Main::addBotEngine(const String& name, const String& executable)
{
  BotEngine botEngine = {name, executable};
  botEnginesByName.append(name, botEngine);
}

void_t Main::disconnect()
{
  connection.close();
  for(HashMap<String, User2*>::Iterator i = users.begin(), end = users.end(); i != end; ++i)
    delete *i;
  users.clear();
  processes.clear();
  processesByTable.clear();
  tables.clear();
}

bool_t Main::connect()
{
  disconnect();

  if(!connection.connect(*this))
    return error = connection.getErrorString(), false;

  Buffer buffer(ZLIMDB_MAX_MESSAGE_SIZE);

  // update broker types table
  {
    HashMap<String, uint64_t> knownBotMarkets;
    uint32_t botMarketsTableId;
    if(!connection.createTable("brokers", botMarketsTableId))
      return error = connection.getErrorString(), false;
    if(!connection.query(botMarketsTableId))
      return error = connection.getErrorString(), false;
    String marketName;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      for(const meguco_broker_type_entity* brokerType; brokerType = (const meguco_broker_type_entity*)zlimdb_get_entity(sizeof(meguco_broker_type_entity), &data, &size);)
      {
        if(!ZlimdbConnection::getString(brokerType->entity, sizeof(*brokerType), brokerType->name_size, marketName))
          continue;
        knownBotMarkets.append(marketName, brokerType->entity.id);
      }
    }
    if(connection.getErrno() != 0)
      return error = connection.getErrorString(), false;
    buffer.resize(ZLIMDB_MAX_MESSAGE_SIZE);
    for(HashMap<String, BotMarket>::Iterator i = botMarketsByName.begin(), end = botMarketsByName.end(); i != end; ++i)
    {
      const String& marketName = i.key();
      HashMap<String, uint64_t>::Iterator it = knownBotMarkets.find(marketName);
      if(it == knownBotMarkets.end())
      {
        meguco_broker_type_entity* brokerType = (meguco_broker_type_entity*)(byte_t*)buffer;
        ZlimdbConnection::setEntityHeader(brokerType->entity, 0, 0, sizeof(*brokerType) + marketName.length());
        ZlimdbConnection::setString(brokerType->entity, brokerType->name_size, sizeof(*brokerType), marketName);
        uint64_t id;
        if(!connection.add(botMarketsTableId, brokerType->entity, id))
          return error = connection.getErrorString(), false;
        botMarkets.append(id, &*i);
      }
      else
      {
        knownBotMarkets.remove(it);
        botMarkets.append(*it, &*i);
      }
    }
    for(HashMap<String, uint64_t>::Iterator i = knownBotMarkets.begin(), end = knownBotMarkets.end(); i != end; ++i)
      connection.remove(botMarketsTableId, *i);
  }

  // update bot types table
  {
    HashMap<String, uint64_t> knownBotEngines;
    uint32_t botEnginesTableId;
    if(!connection.createTable("bots", botEnginesTableId))
      return error = connection.getErrorString(), false;
    if(!connection.query(botEnginesTableId))
      return error = connection.getErrorString(), false;
    String engineName;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      for(const meguco_bot_type_entity* botType; botType = (const meguco_bot_type_entity*)zlimdb_get_entity(sizeof(meguco_bot_type_entity), &data, &size);)
      {
        if(!ZlimdbConnection::getString(botType->entity, sizeof(*botType), botType->name_size, engineName))
          continue;
        knownBotEngines.append(engineName, botType->entity.id);
      }
    }
    if(connection.getErrno() != 0)
      return error = connection.getErrorString(), false;
    buffer.resize(ZLIMDB_MAX_MESSAGE_SIZE);
    for(HashMap<String, BotEngine>::Iterator i = botEnginesByName.begin(), end = botEnginesByName.end(); i != end; ++i)
    {
      const String& botEngineName = i.key();
      HashMap<String, uint64_t>::Iterator it = knownBotEngines.find(botEngineName);
      if(it == knownBotEngines.end())
      {
        meguco_bot_type_entity* botType = (meguco_bot_type_entity*)(byte_t*)buffer;
        ZlimdbConnection::setEntityHeader(botType->entity, 0, 0, sizeof(*botType) + botEngineName.length());
        ZlimdbConnection::setString(botType->entity, botType->name_size, sizeof(*botType), botEngineName);
        uint64_t id;
        if(!connection.add(botEnginesTableId, botType->entity, id))
          return error = connection.getErrorString(), false;
        botEngines.append(id, &*i);
      }
      else
      {
        knownBotEngines.remove(it);
        botEngines.append(*it, &*i);
      }
    }
    for(HashMap<String, uint64_t>::Iterator i = knownBotEngines.begin(), end = knownBotEngines.end(); i != end; ++i)
      connection.remove(botEnginesTableId, *i);
  }

  // get processes
  if(!connection.createTable("processes", processesTableId))
      return error = connection.getErrorString(), false;
  if(!connection.subscribe(processesTableId))
    return error = connection.getErrorString(), false;
  {
    String tableName;
    HashMap<uint64_t, String> processes;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      String command;
      for(const meguco_process_entity* process; process = (const meguco_process_entity*)zlimdb_get_entity(sizeof(meguco_process_entity), &data, &size);)
      {
        if(!ZlimdbConnection::getString(process->entity, sizeof(*process), process->cmd_size, command))
          continue;
        processes.append(process->entity.id, command);
      }
    }
    if(connection.getErrno() != 0)
      return error = connection.getErrorString(), false;
    for(HashMap<uint64_t, String>::Iterator i = processes.begin(), end = processes.end(); i != end; ++i)
      addedProcess(i.key(), *i);
  }

  // get table list
  if(!connection.subscribe(zlimdb_table_tables))
    return error = connection.getErrorString(), false;
  {
    String tableName;
    HashMap<uint64_t, String> tables;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      for(const zlimdb_table_entity* botMarket; botMarket = (const zlimdb_table_entity*)zlimdb_get_entity(sizeof(zlimdb_table_entity), &data, &size);)
      {
        if(!ZlimdbConnection::getString(botMarket->entity, sizeof(*botMarket), botMarket->name_size, tableName))
          continue;
        tables.append((uint32_t)botMarket->entity.id, tableName);
      }
    }
    if(connection.getErrno() != 0)
      return error = connection.getErrorString(), false;
    for(HashMap<uint64_t, String>::Iterator i = tables.begin(), end = tables.end(); i != end; ++i)
      addedTable((uint32_t)i.key(), *i);
  }

  return true;
}

bool_t Main::process()
{
  for(;;)
    if(!connection.process())
      return false;
}

User2* Main::createUser(const String& name)
{
  User2* user = new User2();
  users.append(name, user);
  return user;
}

void_t Main::addedEntity(uint32_t tableId, const zlimdb_entity& entity)
{
  if(tableId == zlimdb_table_tables)
  {
    if(entity.size < sizeof(zlimdb_table_entity))
      return;
    const zlimdb_table_entity* tableEntity = (const zlimdb_table_entity*)&entity;
    String tableName;
    if(!ZlimdbConnection::getString(tableEntity->entity, sizeof(*tableEntity), tableEntity->name_size, tableName))
      return;
    addedTable((uint32_t)tableEntity->entity.id, tableName);
  }
  else if(tableId == processesTableId)
  {
    if(entity.size < sizeof(meguco_process_entity))
      return;
    const meguco_process_entity* process = (const meguco_process_entity*)&entity;
    String command;
    if(!ZlimdbConnection::getString(process->entity, sizeof(*process), process->cmd_size, command))
      return;
    addedProcess(process->entity.id, command);
  }
  else
  {
    // get table type
    // call addedUserSession or so
  }
}

/*
void_t Main::updatedEntity(uint32_t tableId, const zlimdb_entity& entity)
{
  if(entity.id != 1)
    return;
  HashMap<uint32_t, Table>::Iterator it = tables.find(tableId);
  if(it != tables.end())
  {
    Table& table = *it;
    switch(table.type)
    {
    case userBroker:
      if(entity.size >= sizeof(meguco_user_broker_entity))
        updatedUserBroker(*(Market2*)table.object, *(const meguco_user_broker_entity*)&entity);
      break;
    case userSession:
      if(entity.size >= sizeof(meguco_user_session_entity))
        updatedUserSession(*(Session2*)table.object, *(const meguco_user_session_entity*)&entity);
      break;
    }
  }
}
*/
void_t Main::removedEntity(uint32_t tableId, uint64_t entityId)
{
  if(tableId == zlimdb_table_tables)
    removedTable((uint32_t)entityId);
  else if(tableId == processesTableId)
    removedProcess(entityId);
}

void_t Main::addedTable(uint32_t entityId, const String& tableName)
{
  if(tableName.startsWith("users/") && tableName.endsWith("/broker"))
  {
    String userTable = tableName.substr(6, tableName.length() - (6 + 7));
    const char_t* userNameEnd = userTable.find('/');
    if(!userNameEnd)
      return;
    String userName = userTable.substr(0, userNameEnd - userTable);

    // subscribe to broker
    if(!connection.subscribe(entityId))
      return;
    Buffer buffer;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      for(const meguco_user_broker_entity* userBroker; userBroker = (const meguco_user_broker_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_entity), &data, &size);)
      {
        if(userBroker->entity.id != 1)
          continue;
        addedUserBroker(entityId, userName, *userBroker);
      }
    }
    if(connection.getErrno() != 0)
      return;
  }
  if(tableName.startsWith("users/") && tableName.endsWith("/session"))
  {
    String userTable = tableName.substr(6, tableName.length() - (6 + 8));
    const char_t* userNameEnd = userTable.find('/');
    if(!userNameEnd)
      return;
    String userName = userTable.substr(0, userNameEnd - userTable);

    // subscribe to session
    if(!connection.subscribe(entityId))
      return;
    Buffer buffer;
    while(connection.getResponse(buffer))
    {
      void* data = (byte_t*)buffer;
      uint32_t size = buffer.size();
      for(const meguco_user_session_entity* userSession; userSession = (const meguco_user_session_entity*)zlimdb_get_entity(sizeof(meguco_user_session_entity), &data, &size);)
      {
        if(userSession->entity.id != 1)
          continue;
        addedUserSession(entityId, userName, *userSession);
      }
    }
    if(connection.getErrno() != 0)
      return;
  }
}

void_t Main::addedProcess(uint64_t entityId, const String& command)
{
  if(command.startsWith("Brokers/"))
  {
    const char_t* param = command.find(' ');
    if(param)
    {
      String paramStr = command.substr(param - command + 1);
      uint32_t tableId = paramStr.toUInt();
      Process processData = {userBroker, entityId, tableId};
      Process& process = processes.append(entityId, processData);
      processesByTable.append(tableId, &process);
      HashMap<uint32_t, Table>::Iterator it = tables.find(tableId);
      if(it != tables.end() && it->type == userBroker)
      {
        Market2* market = (Market2*)it->object;
        market->setState(meguco_user_broker_running);
        if(!connection.update(tableId, market->getEntity()))
          return;
      }
    }
  }
  if(command.startsWith("Bots/"))
  {
    const char_t* param = command.find(' ');
    if(param)
    {
      String paramStr = command.substr(param - command + 1);
      uint32_t tableId = paramStr.toUInt();
      Process processData = {userSession, entityId, tableId};
      Process& process = processes.append(entityId, processData);
      processesByTable.append(tableId, &process);
      HashMap<uint32_t, Table>::Iterator it = tables.find(tableId);
      if(it != tables.end() && it->type == userSession)
      {
        Session2* session = (Session2*)it->object;
        session->setState(meguco_user_session_running);
        if(!connection.update(tableId, session->getEntity()))
          return;
      }
    }
  }
}

void_t Main::addedUserBroker(uint32_t tableId, const String& userName, const meguco_user_broker_entity& userBroker)
{
  BotMarket* botMarket = *botMarkets.find(userBroker.bot_market_id);
  User2* user = findUser(userName);
  if(!user)
    user = createUser(userName);
  Market2* market = user->createBroker(tableId, userBroker, botMarket ? botMarket->executable : String());
  Table table = {Main::userBroker, market};
  tables.append(tableId, table);

  // update user broker state
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(tableId);
  meguco_user_broker_state state = meguco_user_broker_stopped;
  if(it != processesByTable.end() && (*it)->type == Main::userBroker)
    state = meguco_user_broker_running;
  if (state != market->getState())
  {
    market->setState(state);
    connection.update(tableId, market->getEntity());
  }

  // start broker process if it is not already running
  if (state != meguco_user_broker_running)
  {
      String command = market->getExecutable() + " " + String::fromUInt(tableId);
      Buffer buffer;
      buffer.resize(sizeof(meguco_process_entity) + command.length());
      meguco_process_entity* process = (meguco_process_entity*)(byte_t*)buffer;
      ZlimdbConnection::setEntityHeader(process->entity, 0, 0, buffer.size());
      ZlimdbConnection::setString(process->entity, process->cmd_size, sizeof(*process), command);
      uint64_t id;
      connection.add(processesTableId, process->entity, id);
  }
}

/*
bool_t Main::setUserBrokerState(Market2& market, meguco_user_broker_state state)
{
  uint32_t tableId = market.getTableId();
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(tableId);
  if(it != processesByTable.end() && (*it)->type != userBroker)
    it = processesByTable.end();
  if(state == meguco_user_broker_starting || state == meguco_user_broker_running)
  {
    if(it == processesByTable.end())
    {
      if(market.getState() != meguco_user_broker_starting)
      {
        market.setState(meguco_user_broker_starting);
        if(!connection.update(tableId, market.getEntity()))
          return false;
      }
      String command = market.getExecutable() + " " + String::fromUInt(tableId);
      Buffer buffer;
      buffer.resize(sizeof(meguco_process_entity) + command.length());
      meguco_process_entity* process = (meguco_process_entity*)(byte_t*)buffer;
      ZlimdbConnection::setEntityHeader(process->entity, 0, 0, buffer.size());
      ZlimdbConnection::setString(process->entity, process->cmd_size, sizeof(*process), command);
      uint64_t id;
      if(!connection.add(processesTableId, process->entity, id))
        return false;
    }
    else if(market.getState() != meguco_user_broker_running)
    {
      market.setState(meguco_user_broker_running);
      if(!connection.update(tableId, market.getEntity()))
        return false;
    }
  }
  else
  {
    if(it != processesByTable.end())
    {
      if(market.getState() != meguco_user_broker_stopping)
      {
        market.setState(meguco_user_broker_stopping);
        if(!connection.update(tableId, market.getEntity()))
          return false;
      }
      if(!connection.remove(processesTableId, (*it)->entityId))
        return false;
    }
    else if(market.getState() != meguco_user_broker_stopped)
    {
      market.setState(meguco_user_broker_stopped);
      if(!connection.update(tableId, market.getEntity()))
        return false;
    }
  }
  return true;
}

bool_t Main::setUserSessionState(Session2& session, meguco_user_session_state state)
{
  uint32_t tableId = session.getTableId();
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(tableId);
  if(it != processesByTable.end() && (*it)->type != userSession)
    it = processesByTable.end();
  if(state == meguco_user_session_starting || state == meguco_user_session_running)
  {
    if(it == processesByTable.end())
    {
      if(session.getState() != meguco_user_session_starting)
      {
        session.setState(meguco_user_session_starting);
        if(!connection.update(tableId, session.getEntity()))
          return false;
      }
      String command = session.getExecutable() + " " + String::fromUInt(tableId);
      Buffer buffer;
      buffer.resize(sizeof(meguco_process_entity) + command.length());
      meguco_process_entity* process = (meguco_process_entity*)(byte_t*)buffer;
      ZlimdbConnection::setEntityHeader(process->entity, 0, 0, buffer.size());
      ZlimdbConnection::setString(process->entity, process->cmd_size, sizeof(*process), command);
      uint64_t id;
      if(!connection.add(processesTableId, process->entity, id))
        return false;
    }
    else if(session.getState() != meguco_user_session_running)
    {
      session.setState(meguco_user_session_running);
      if(!connection.update(tableId, session.getEntity()))
        return false;
    }
  }
  else
  {
    if(it != processesByTable.end())
    {
      if(session.getState() != meguco_user_session_stopping)
      {
        session.setState(meguco_user_session_stopping);
        if(!connection.update(tableId, session.getEntity()))
          return false;
      }
      if(!connection.remove(processesTableId, (*it)->entityId))
        return false;
    }
    else if(session.getState() != meguco_user_session_stopped)
    {
      session.setState(meguco_user_session_stopped);
      if(!connection.update(tableId, session.getEntity()))
        return false;
    }
  }
  return true;
}
*/
void_t Main::addedUserSession(uint32_t tableId, const String& userName, const meguco_user_session_entity& userSession)
{
  BotEngine* botEngine = *botEngines.find(userSession.bot_engine_id);
  User2* user = findUser(userName);
  if(!user)
    user = createUser(userName);
  Session2* session = user->createSession(tableId, userSession, botEngine ? botEngine->executable : String());
  Table table = {Main::userSession, session};
  tables.append(tableId, table);

  // update user session state
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(tableId);
  meguco_user_session_state state = meguco_user_session_stopped;
  if(it != processesByTable.end() && (*it)->type == Main::userSession)
    state = meguco_user_session_running;
  if (state != session->getState())
  {
    session->setState(state);
    connection.update(tableId, session->getEntity());
  }
}

void_t Main::removedTable(uint32_t tableId)
{
  HashMap<uint32_t, Table>::Iterator it = tables.find(tableId);
  if(it != tables.end())
  {
    Table& table = *it;
    switch(table.type)
    {
    case userBroker:
      removedUserBroker(*(Market2*)table.object);
      break;
    case userSession:
      removedUserSession(*(Session2*)table.object);
      break;
    }
  }
}

void_t Main::removedProcess(uint64_t entityId)
{
  HashMap<uint64_t, Process>::Iterator it = processes.find(entityId);
  if(it != processes.end())
  {
    Process& process = *it;
    {
      HashMap<uint32_t, Table>::Iterator it = tables.find(process.tableId);
      if(it != tables.end())
      {
        Table& table = *it;
        switch(process.type)
        {
        case userBroker:
          {
            Market2* market = (Market2*)table.object;
            if(market->getState() != meguco_user_broker_stopped)
            {
              market->setState(meguco_user_broker_stopped);
              connection.update(market->getTableId(), market->getEntity());
            }
          }
          break;
        case userSession:
          {
            Session2* session = (Session2*)table.object;
            if(session->getState() != meguco_user_session_stopped)
            {
              session->setState(meguco_user_session_stopped);
              connection.update(session->getTableId(), session->getEntity());
            }
          }
          break;
        }
      }
    }
    processesByTable.remove(process.tableId);
    processes.remove(it);
  }
}

void_t Main::removedUserBroker(Market2& market)
{
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(market.getTableId());
  if(it != processesByTable.end() && (*it)->type == userBroker)
    connection.remove(processesTableId, (*it)->entityId);
  tables.remove(market.getTableId());
  market.getUser().deleteMarket(market);
}

void_t Main::removedUserSession(Session2& session)
{
  HashMap<uint32_t, Process*>::Iterator it = processesByTable.find(session.getTableId());
  if(it != processesByTable.end() && (*it)->type == userSession)
    connection.remove(processesTableId, (*it)->entityId);
  tables.remove(session.getTableId());
  session.getUser().deleteSession(session);
}
