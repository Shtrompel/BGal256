#include "plugin.hpp"

#include "components/DigitalControl.hpp"
#include "utils/SorterArray.hpp"
#include "utils/Globals.hpp"
#include "utils/MathUtils.hpp"

#include <map>

struct SorterArray;
struct SortStep;

enum SettingsGroup
{
	NONE,
	MAIN,
	FILTER,
	KEY
};


typedef Widget SorterArrayBase;

constexpr int MAX_SHUFFLE_SKIPS = 256;
constexpr int MAX_TRAVERSAL_SKIPS = 256;
constexpr int MIN_KEY_OFFSET = -36;
constexpr int MAX_KEY_OFFSET = 36;

// The code is bad don't read it please
struct SorterArrayWidget : Widget
{

	SorterArray *sorterArray = nullptr;

	const NVGcolor backgroundColor = nvgRGB(
		0, 0, 0);
	const NVGcolor defaultColor = nvgRGB(
		255, 255, 255);
	const NVGcolor greenColor = nvgRGB(
		COLOR_ACCENT_YELLOW[0],
		COLOR_ACCENT_YELLOW[1],
		COLOR_ACCENT_YELLOW[2]);
	const NVGcolor redColor = nvgRGB(
		COLOR_ACCENT_RED[0],
		COLOR_ACCENT_RED[1],
		COLOR_ACCENT_RED[2]);
	const NVGcolor blueColor = nvgRGB(
		COLOR_ACCENT_BLUE[0],
		COLOR_ACCENT_BLUE[1],
		COLOR_ACCENT_BLUE[2]);
	
	bool dragging = false;
	Vec dragPosition;

	float width, scale;

	SettingsGroup menuState;
	bool enableMenu = false;
	bool enableFilters = false;
	bool enableKey = false;
	float arraySizeFloat{};
	float slowerShuffleFloat{};
	float slowerTraversalFloat{};
	float keyOffsetFloat = 0.5;
	
	std::map<std::string, Scale>
		scalesMap;


	DigitalControl *enableMenuControl = nullptr;
	DigitalControl *changeAlgorithmControl = nullptr;
	DigitalControl *arraySizeControl = nullptr;
	DigitalControl *slowerShuffleControl = nullptr;
	DigitalControl *slowerTraversalControl = nullptr;
	
	DigitalControl *enableFiltersControl = nullptr;
	DigitalControl *filterEventsControls[6] = {nullptr};

	DigitalControl *enableKeyOutControl = nullptr;
	DigitalControl *enableKeyControl = nullptr;
	DigitalControl *scaleControl = nullptr;
	DigitalControl *keyOffsetControl = nullptr;

	std::string changeAlgorithmStr{};
	std::string arraySizeString{};

	std::string shuffleSkipStr{};
	std::string traversalSkipStr{};
	std::string keyScaleStr{};
	std::string keyOffsetStr{};


	SorterArrayWidget(
		SorterArray *array, Vec pos, Vec size);

	void setMenuState(bool enable, SettingsGroup);

	void changeAlgorithm(t_algorithmtype type);

	void changeArraySize(int size, bool reset = true);


	void changeShuffleSkip(int frames);

	void changeTraversalSkip(int frames);

	void changeOutScale(const std::string&);

	void changeOutKeyOffset(int offset);

	void updateUIFromState();


	void drawText(
		const DrawArgs &args,
		const Vec &pos,
		const std::string &text,
		float width = 90);

	void drawRect(const DrawArgs &args, int i, const NVGcolor &color);

	void draw(const DrawArgs &args) override;

	void onButton(const event::Button &e) override;

	void onDragStart(const DragStartEvent &e) override;

	void onDragMove(const DragMoveEvent &e) override;

	void onDragEnd(const DragEndEvent &e) override;
};