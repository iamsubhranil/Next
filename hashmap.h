#pragma once

#include "robin_hood.h"

template <typename K, typename V>
using HashMap = robin_hood::unordered_map<K, V>;

template <typename K> using HashSet = robin_hood::unordered_set<K>;
