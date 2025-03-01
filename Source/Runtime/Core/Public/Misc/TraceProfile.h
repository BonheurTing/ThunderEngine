#pragma once

#include <tracy/TracyC.h>
#include "tracy/Tracy.hpp"

#define ThunderZoneScoped(...) ZoneScoped(__VA_ARGS__)
#define ThunderZoneScopedN(...) ZoneScopedN(__VA_ARGS__)

#define ThunderFrameMark FrameMark

#define ThunderTracyCSetThreadName(...) TracyCSetThreadName(__VA_ARGS__)