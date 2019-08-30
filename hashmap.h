#pragma once

#include "robin_hood.h"

template <typename K, typename V>
using HashMap = robin_hood::unordered_map<K, V>;
