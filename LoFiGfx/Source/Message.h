//
// Created by starr on 2024/6/20.
//

#include <list>
#include <string>
#include <string_view>
#include "Marcos.h"

namespace LoFi {

      enum class MessageType : uint8_t {
            Normal,
            Warning,
            Error
      };

      struct Message {
            MessageType Type;
            std::string Content;
      };

      class MessageManager {
      public:
            MessageManager() = delete;

            ~MessageManager() = delete;

            NO_COPY_MOVE_CONS(MessageManager);

            static void addMessage(MessageType type, std::string_view content);

            static void clearMessages();

            static std::string getMessages(int messageCount = 0);
      private:
            inline static std::list<Message> Messages;
            inline static uint32_t ErrorCount = 0;
            inline static uint32_t WarningCount = 0;
            inline static uint32_t NormalCount = 0;
      };
}