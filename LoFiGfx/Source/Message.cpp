//
// Created by starr on 2024/6/20.
//
#include <iostream>
#include <string_view>

#include "Message.h"

using namespace LoFi;

void MessageManager::Log(MessageType type, std::string_view content) {
      switch(type) {
            case MessageType::Normal:
                  NormalCount++;
            std::printf("[Normal]: %s\n", content.data());
            break;
            case MessageType::Warning:
                  WarningCount++;
            std::printf("[Warning]: %s\n", content.data());

            break;
            case MessageType::Error:
                  ErrorCount++;
            std::printf("[Error]: %s\n", content.data());
            break;
            default: return;
      }

      Messages.emplace_front(type, std::string(content));
}

void MessageManager::Clear() {
            Messages.clear();
}

std::string MessageManager::Get(int MessageCount) {
      std::string result {};
      for (const auto& msg : Messages) {
            MessageCount--;
            result += msg.Content + "\n";
            if(MessageCount == 0) {
                  break;
            }
      }
      return result;
}
