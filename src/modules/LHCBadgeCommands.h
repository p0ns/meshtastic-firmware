#pragma once

#include <cctype>
#include <cstddef>
#include <string>

namespace lhc_badge
{

inline bool matchShortCommand(const std::string &command, char alias, size_t *argumentOffset = nullptr)
{
    const size_t aliasIndex = !command.empty() && command.front() == '/' ? 1 : 0;
    if (aliasIndex >= command.size() || command[aliasIndex] != alias) {
        return false;
    }

    const size_t offset = aliasIndex + 1;
    if (offset < command.size() && !std::isspace(static_cast<unsigned char>(command[offset]))) {
        return false;
    }

    if (argumentOffset) {
        *argumentOffset = offset;
    }
    return true;
}

inline bool isShortCommand(const std::string &command, char alias)
{
    size_t argumentOffset = 0;
    return matchShortCommand(command, alias, &argumentOffset) && argumentOffset == command.size();
}

} // namespace lhc_badge
