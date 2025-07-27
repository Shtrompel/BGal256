#include "ClothingManager.hpp"

#include "plugin.hpp"


ClothingManager::ClothingManager(std::vector<float>* steps)
{
    this->steps = steps;
}

int ClothingManager::getClothId(t_clothtype clothType)
{
    if (0 <= clothType && clothType < CLOTHTYPE_COUNT)
    {
	    return clothCurrent[clothType];
    }
    return 0;
}

int ClothingManager::getClothCounter(t_clothtype clothType)
{
    if (0 <= clothType && clothType < CLOTHTYPE_COUNT)
    {
	    return clothCounter[clothType];
    }
    return 0;
}

void ClothingManager::changeCloth(t_clothtype clothType, int id)
{
    if (0 <= clothType && clothType < CLOTHTYPE_COUNT)
    {
	    clothCurrent[clothType] = id;
        steps->operator[](clothType) = (float)id /
            getClothCounter(clothType);
    }
}

void ClothingManager::loadCloth(t_clothtype clothType)
{
	clothCounter[clothType] += 1;
}

void ClothingManager::debugPrint()
{
	for (int i = 0; i < 4; ++i)
	{
		int j = getClothCounter(i);
		DEBUG("ClothCount: %d -> %d", i, j);

		j = getClothId(i);
		DEBUG("ClothId: %d -> %d", i, j);
	}
}