#pragma once

#include "system.h"

#include <functional>
using namespace std;
#include <GLM/glm.hpp>
using namespace glm;


struct StorageText : public Storage
{
	StorageText() : Text([]{ return ""; }) {}

	function<string(void)> Text;
	uvec2 Position;
};