#ifndef GLOBALS_H
#define GLOBALS_H

#include <cstdint>
#include <array>
#include <string>
#include <iostream>
#include <list>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <ctime>
#include <chrono>
#include <memory>
#include <cstring>

#ifdef CHESS_GUI
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#endif

uint64_t invert_bits(uint64_t bits);
uint64_t invert_bytes(uint64_t bits);


#endif
