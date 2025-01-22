#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class RedisParser {
public:
  RedisParser() {}

  std::string encodeRedisString(const std::string &responseString) {
    std::string response = "$" + std::to_string(responseString.length());
    response += "\r\n" + responseString + "\r\n";
    return response;
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

  std::string handleRedisArray(const std::vector<std::string> &commandList) {
    // [0] is the length of array
    // [1] is the length of the command

    for (auto word : commandList) {
      std::cout << word << '\n';
    }

    std::string command = commandList[2];
    std::cout << "Command: " << command << '\n';
    std::string response;
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    if (command == "echo") {
      response = encodeRedisString(commandList[4]);
    }

    if (command == "ping") {
      response = encodeRedisString("PONG");
    }

    if (command == "set") {
      internalMap.insert({commandList[4], commandList[6]});
      response = encodeRedisString("OK");
    }

    if (command == "get") {
      std::cout << "Running get\n";
      auto found = internalMap.find(commandList[4]);
      if (found != internalMap.end()) {
        response = encodeRedisString(found->second);
      } else {
        response = nullBulkString;
      }
    }

    return response;
  }

  std::string handleRedisCommand(const std::string &commandBuffer) {
    std::string res;
    std::vector<std::string> commandList = decodeRedisString(commandBuffer);

    if (commandList[0].find("*") != std::string::npos) {
      res = handleRedisArray(commandList);
    }

    return res;
  }

private:
  std::unordered_map<std::string, std::string> internalMap;
  std::string nullBulkString = "$-1\r\n";
};
