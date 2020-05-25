#pragma once

#include "robin_hood.h"

template <typename K, typename V, class H = std::hash<K>,
          class E = std::equal_to<K>>
using HashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename K, class H = std::hash<K>, class E = std::equal_to<K>>
using HashSet = robin_hood::unordered_set<K, H, E>;
