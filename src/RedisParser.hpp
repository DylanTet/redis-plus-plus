#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct MapValue {
  std::string value;
  std::chrono::steady_clock::time_point expireTime;
};

class RedisParser {
public:
  RedisParser(std::string configFileName = "", std::string configFilePath = "")
      : m_configFileName(configFileName), m_configFilePath(configFilePath) {}

  std::string encodeRedisString(const std::string &responseString) {
    std::string response = "$" + std::to_string(responseString.length());
    response += "\r\n" + responseString + "\r\n";
    return response;
  }

  std::string encodeRedisArray(std::vector<std::string> &responseList) {
    std::string responseArray =
        "*" + std::to_string(responseList.size()) + "\r\n";
    for (const std::string &word : responseList) {
      responseArray += "$";
      responseArray += std::to_string(word.length()) + "\r\n";
      responseArray += word + "\r\n";
    }

    return responseArray;
  }

  std::vector<std::string> decodeRedisString(const std::string &rawBuffer) {
    std::vector<std::string> result;
    std::string_view token = "\r\n";
    size_t start = 0;
    size_t end = rawBuffer.find(token);

    while (end != std::string::npos) {
      result.emplace_back(rawBuffer.substr(start, end - start));
      start = end + token.length();
      end = rawBuffer.find(token, start);
    }

    if (start < rawBuffer.length()) {
      result.emplace_back(rawBuffer.substr(start));
    }

    return result;
  }

  std::string handleConfigCommand(const std::vector<std::string> &commandList) {
    std::string parameter = commandList[4];
    std::string result;
    std::transform(parameter.begin(), parameter.end(), parameter.begin(),
                   ::tolower);

    if (parameter == "get") {
      if (commandList[6] == "dir") {
        std::vector<std::string> wordList = {"dir", m_configFilePath};
        result = encodeRedisArray(wordList);
      }

      if (commandList[6] == "dbfilename") {
        std::vector<std::string> wordList = {"dbfilename", m_configFileName};
        result = encodeRedisArray(wordList);
      }
    }

    return result;
  }

  std::string handleRedisCommand(const std::string &command,
                                 const std::vector<std::string> &commandList) {
    std::string result;
    if (command == "echo") {
      result = encodeRedisString(commandList[4]);
    }

    if (command == "ping") {
      result = encodeRedisString("PONG");
    }

    if (command == "set") {
      if (commandList.size() > 7) {
        int64_t expireValue = std::stoll(commandList[10]);
        internalMap.insert(
            {commandList[4],
             {commandList[6], std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(expireValue)}});
      } else {
        internalMap.insert(
            {commandList[4],
             {commandList[6], std::chrono::steady_clock::time_point::max()}});
      }

      result = encodeRedisString("OK");
    }

    if (command == "get") {
      auto found = internalMap.find(commandList[4]);
      if (found != internalMap.end() &&
          found->second.expireTime > std::chrono::steady_clock::now()) {
        std::string value = found->second.value;
        result = encodeRedisString(value);
      } else {
        result = nullBulkString;
      }
    }

    if (command == "config") {
      result = handleConfigCommand(commandList);
    }

    return result;
  }

  std::string handleRedisArray(const std::vector<std::string> &commandList) {
    // [0] is the length of array
    // [1] is the length of the command

    std::string command = commandList[2];
    std::string response;
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    response = handleRedisCommand(command, commandList);

    return response;
  }

  std::string handleRedisCommandBuffer(const std::string &commandBuffer) {
    std::string res;
    std::vector<std::string> commandList = decodeRedisString(commandBuffer);

    if (commandList[0].find("*") != std::string::npos) {
      res = handleRedisArray(commandList);
    }

    return res;
  }

private:
  std::unordered_map<std::string, MapValue> internalMap;
  std::string nullBulkString = "$-1\r\n";
  std::string m_configFileName;
  std::string m_configFilePath;
};
