#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
 public:
  static_assert(std::is_integral_v<Key>,
                "ConcurrentMap supports only integer keys"s);

  struct Access {
    // Сделал вывод, что
    // надёжнее не использовать конструктор из-за неопределённого порядка
    // инициализации полей.
    // Лучше конструировать как Access{lock_guard{mtx}, Value}

    // Access(std::mutex& m, Value& val) : guard(m), ref_to_value(val) {}
    std::lock_guard<std::mutex> g;
    Value& ref_to_value;
  };

  explicit ConcurrentMap(size_t bucket_count)
      : joint_map_(bucket_count) {}

  Access operator[](const Key& key) {
    const size_t i = static_cast<size_t>(key) % joint_map_.size();
    return Access{std::lock_guard<std::mutex>{joint_map_[i].mutex}, joint_map_[i].map[key]};
  }

  void Erase(const Key& key) {
    const size_t i = static_cast<size_t>(key) % joint_map_.size();
    std::lock_guard<std::mutex> g(joint_map_[i].mutex);
    joint_map_[i].map.erase(key);
  }

  std::map<Key, Value> BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for (auto& submap : joint_map_) {
      std::lock_guard g(submap.mutex);
      result.insert(submap.map.cbegin(), submap.map.cend());
    }
    return result;
  }

 private:
  struct Submap {
    std::map<Key, Value> map;
    std::mutex mutex;
  };

  std::vector<Submap> joint_map_;
};