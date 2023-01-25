#include <string>
#include <string_view>
#include <vector>

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
  std::vector<std::string_view> result;
    size_t pos = str.find_first_not_of(" ");
    const size_t pos_end = str.npos;
    while (pos != pos_end) {
        size_t space = str.find(" ", pos);
        result.push_back(space == pos_end ? (str.substr(pos)) : (str.substr(pos, space - pos)));
        pos = str.find_first_not_of(" ", space);
    }

    return result;
}