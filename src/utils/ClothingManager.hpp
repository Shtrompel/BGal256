#ifndef _CLOTHING_MANAGER
#define _CLOTHING_MANAGER

#include "../utils/Globals.hpp"
#include <array>
#include <vector>

struct ClothingManager
{
	// Clothing info
	// array = [HEAD, SHIRT, PANTS, SHOES]
	// The id of the current worn cloth, can be null for none
	std::array<int, 4> clothCurrent = {0, 0, 0, 0};
    // Count the amount of each loaded cloth type
    std::array<int, 4> clothCounter = {0, 0, 0, 0};

	std::vector<float>* steps = nullptr;

	ClothingManager(std::vector<float>* steps);

	int getClothId(t_clothtype clothType);

	int getClothCounter(t_clothtype clothType);

	void changeCloth(t_clothtype clothType, int id);

    void loadCloth(t_clothtype clothType);

    void debugPrint();
};


#endif // _CLOTHING_MANAGER
