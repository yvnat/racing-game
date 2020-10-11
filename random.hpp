#pragma once
#include <random>
#include <ctime>
#include <string>

namespace rng {
	std::random_device _rand;
	std::default_random_engine _eng{time(0)};
	bool randBool(float p) {
		std::uniform_real_distribution<float> d(0.0, 1.0);
		return d(_eng) < p;
	}
	int randInt(int min, int max) {	//does not include max
		std::uniform_int_distribution<> d(min, max);
		return d(_eng);
	}
	float randFloat(float min, float max) {	//does not include max
		std::uniform_real_distribution<> d(min, max);
		return d(_eng);
	}
	template<typename T> T randElement(std::vector<T> array) {
		return array[randInt(0, array.size() - 1)];
	}
	std::string randomGenericName(int length) {
		std::string name = "";
		std::string vowels[] = {"a", "e", "o", "u", "i"};
		std::string consonents[] = {"b", "c", "d", "e", "f", "g", "h", "j", "k", "l", "m", "n", "p", "q", "r", "s", "t", "v", "w", "x", "y", "z"};
		for (int i = 0; i < length; ++i) {
			if (i%2 == 0) {
				name += vowels[randInt(0, sizeof(vowels) / sizeof(vowels[0]))];
			} else {
				name += consonents[randInt(0, sizeof(vowels) / sizeof(vowels[0]))];
			}
		}
		return name;
	}
}