#include "widgets/SorterArrayScreen.hpp"

#include "components/DigitalHSliderTooltip.hpp"

bool loadScales(std::vector<Scale> &scales)
{
    std::string path = rack::asset::plugin(pluginInstance, "res/data/Scales.json");

    json_error_t error{};
    json_t *root = json_load_file(path.c_str(), 0, &error);
    if (!root)
    {
        WARN("JSON error: %s (line %d)", error.text, error.line);
        return false;
    }

    if (!json_is_array(root))
    {
        WARN("Root is not an array");
        json_decref(root);
        return false;
    }

    size_t index;
    json_t *value;

    json_array_foreach(root, index, value)
    {
        Scale scale;
        json_t *j_name = json_object_get(value, "name");
        json_t *j_intervals = json_object_get(value, "intervals");

        if (!json_is_string(j_name) || !json_is_array(j_intervals))
        {
            WARN("Invalid scale object at index %d", (int)index);
            continue;
        }

        scale.name = json_string_value(j_name);

        scale.intervals.clear();
        size_t interval_index;
        json_t *interval_value;
        json_array_foreach(j_intervals, interval_index, interval_value)
        {
            if (!json_is_integer(interval_value))
            {
                WARN("Non-integer interval in %s", scale.name.c_str());
                continue;
            }
            scale.intervals.push_back(json_integer_value(interval_value));
        }

        scales.push_back(scale);
    }

    json_decref(root);
    return true;
}

struct ArraySizeSlider : DigitalHorizontalSlider
{
    float WIDTH = 74.0f;
    float HEIGHT = 10.0f;

    DigitalHSliderTooltip *tooltip = new DigitalHSliderTooltip();
    std::function<std::string()> tooltipCallback;

    ArraySizeSlider(float *value)
        : DigitalHorizontalSlider(value) // Call the constructor of the base class
    {
        // You can now initialize SlewSlider's additional members, if any
    }

    // void draw(const DrawArgs &args) override

    void drawLayer(const DrawArgs &args, int layer) override
    {
        if (layer == 1)
        {
            DigitalHorizontalSlider::draw(args);

            if (tooltip && tooltipCallback)
            {
                std::string label = tooltipCallback();
                tooltip->setAttributes(
                    label,
                    getValue() * WIDTH + thumb_rectangle.size.x / 2,
                    0.0);
                tooltip->drawTooltip(args);
            }
        }
    }

    void onHoverKey(const HoverKeyEvent &e) override
    {
        DigitalHorizontalSlider::onHoverKey(e);
    }

    void onDragMove(const event::DragMove &e) override
    {

        DigitalHorizontalSlider::onDragMove(e);
        tooltip->activateTooltip();
    }

    void onDragEnd(const event::DragEnd &e) override
    {
        DigitalHorizontalSlider::onDragEnd(e);
        tooltip->deactivateTooltip();
    }
};

SorterArrayWidget::SorterArrayWidget(
    SorterArray *array,
    Vec pos,
    Vec size)
{

    this->box.size = size;

    if (!array)
        return;

    this->sorterArray = array;

    if (sorterArray)
        changeAlgorithm(sorterArray->currentType);

    if (1)
    {
        ArraySizeSlider *arraySizeSlider = new ArraySizeSlider(&arraySizeFloat);
        arraySizeSlider->tooltipCallback = [this]()
        {
            return std::to_string((int)(this->arraySizeFloat * MAX_ARRAY_SIZE));
        };
        arraySizeSlider->setOnRelease(
            [this](float value)
            {
                this->changeArraySize((int)(value * MAX_ARRAY_SIZE));
            });
        this->arraySizeControl = new DigitalControl(
            Vec(120, 10),
            "Array Size:",
            "",
            arraySizeSlider,
            nullptr);
        addChild(arraySizeControl);

        DigitalToggle *changeAlgorithmToggle = new DigitalToggle(nullptr);
        changeAlgorithmToggle->setOnPress(
            [this](bool state)
            {
                auto menu = rack::createMenu();
                menu->addChild(rack::createMenuLabel("Sorting Algorithm"));
                for (const auto &pair : this->sorterArray->algorithms)
                {
                    // id -> int
                    // algorithm -> SorterAlgorithm*
                    // algorithm->name -> std::string

                    menu->addChild(createCheckMenuItem(
                        pair.second->name, "",
                        [=]()
                        {
                            return pair.first == this->sorterArray->currentType;
                        },
                        [=]()
                        {
                            changeAlgorithm(pair.first);
                        }));
                }
            });
        this->changeAlgorithmControl = new DigitalControl(
            Vec(120, 55),
            "Change Algorithm", "Change Algorithm",
            changeAlgorithmToggle,
            nullptr);

        addChild(changeAlgorithmControl);

        ArraySizeSlider *slowerShuffleSlider = new ArraySizeSlider(&slowerShuffleFloat);
        slowerShuffleSlider->tooltipCallback = [this]()
        {
            return std::to_string((int)(1 + this->slowerShuffleFloat * (MAX_SHUFFLE_SKIPS - 1)));
        };
        slowerShuffleSlider->setOnRelease(
            [this](float value)
            {
                this->changeShuffleSkip(
                    (int)(value * (MAX_SHUFFLE_SKIPS - 1)) + 1);
            });
        this->slowerShuffleControl = new DigitalControl(
            Vec(120, 100),
            "S. Skips:",
            "",
            slowerShuffleSlider,
            nullptr);
        addChild(slowerShuffleControl);

        ArraySizeSlider *slowerTraversalSlider = new ArraySizeSlider(&slowerTraversalFloat);
        slowerTraversalSlider->tooltipCallback = [this]()
        {
            return std::to_string(
                (int)(1 + this->slowerTraversalFloat * (MAX_TRAVERSAL_SKIPS - 1)));
        };
        slowerTraversalSlider->setOnRelease(
            [this](float value)
            {
                this->changeTraversalSkip(
                    (int)(value * (MAX_TRAVERSAL_SKIPS - 1)) + 1);
            });
        this->slowerTraversalControl = new DigitalControl(
            Vec(120, 145),
            "T. Skips:",
            "",
            slowerTraversalSlider,
            nullptr);
        addChild(slowerTraversalControl);
    }

    if (1)
    {
        std::string asd[] = {"Cmp", "Swap", "Move", "Set", "Read", "Remove"};
        for (int i = 0; i < 6; ++i)
        {
            DigitalSwitch *filterEventControl;

            filterEventControl = new DigitalSwitch(
                &sorterArray->filterEvent[i]);

            filterEventsControls[i] = new DigitalControl(
                Vec(120, 5 + i * 20),
                std::string("Filter ") + asd[i] + " X",
                std::string("Filter ") + asd[i] + " ✓",
                filterEventControl,
                nullptr);

            addChild(filterEventsControls[i]);
        }
    }

    std::vector<Scale> scales;
    loadScales(scales);
    for (const Scale &scale : scales)
        scalesMap[scale.name] = scale;

    if (1)
    {
        DigitalToggle *enableKeyToggle = new DigitalToggle(
            &this->sorterArray->enableKeyOutput);
        enableKeyToggle->setOnPress(
            [this](bool state) {});
        this->enableKeyControl = new DigitalControl(
            Vec(120, 10),
            "Scale Disabled", "Scale Enabled",
            enableKeyToggle,
            nullptr);
        addChild(enableKeyControl);

        DigitalToggle *scaleToggle = new DigitalToggle(nullptr);
        scaleToggle->setOnPress(
            [this, scales](bool state)
            {
                auto menu = rack::createMenu();
                menu->addChild(rack::createMenuLabel("Output Scale"));
                for (const auto &pair : scales)
                {
                    menu->addChild(createCheckMenuItem(
                        pair.name, "",
                        [=]()
                        {
                            return this->sorterArray->outScale.name ==
                                   pair.name;
                        },
                        [=]()
                        {
                            this->changeOutScale(pair.name);
                        }));
                }
            });
        this->scaleControl = new DigitalControl(
            Vec(120, 35),
            "Change Scale", "Change Scale",
            scaleToggle,
            nullptr);
        addChild(scaleControl);

        ArraySizeSlider *keyOffsetlSlider = new ArraySizeSlider(&keyOffsetFloat);
        keyOffsetlSlider->tooltipCallback = [this]()
        {
            float v = this->keyOffsetFloat;
            v *= MAX_KEY_OFFSET - MIN_KEY_OFFSET;
            v += MIN_KEY_OFFSET;
            return std::to_string((int)(v));
        };
        keyOffsetlSlider->setOnRelease(
            [this](float value)
            {
                float v = this->keyOffsetFloat;
                v *= MAX_KEY_OFFSET - MIN_KEY_OFFSET;
                v += MIN_KEY_OFFSET;
                // return std::to_string((int)(v));
                this->changeOutKeyOffset(v);
            });
        this->keyOffsetControl = new DigitalControl(
            Vec(120, 75),
            "Key Offset",
            "",
            keyOffsetlSlider,
            nullptr);
        addChild(keyOffsetControl);
    }

    if (1)
    {
        DigitalToggle *enableMenuToggle = new DigitalToggle(&enableMenu);
        enableMenuToggle->setOnPress(
            [=](bool state)
            {
                this->setMenuState(state, SettingsGroup::MAIN);
            });
        this->enableMenuControl = new DigitalControl(
            Vec(10, 10),
            "Show Menu",
            "Hide Menu",
            enableMenuToggle,
            nullptr);
        addChild(enableMenuControl);

        DigitalToggle *enableFiltersToggle = new DigitalToggle(&enableFilters);
        enableFiltersToggle->setOnPress(
            [=](bool state)
            {
                this->setMenuState(state, SettingsGroup::FILTER);
            });
        this->enableFiltersControl = new DigitalControl(
            Vec(10, 30),
            "Show Filters",
            "Hide Filters",
            enableFiltersToggle,
            nullptr);
        addChild(enableFiltersControl);

        DigitalToggle *enableKeyOutToggle = new DigitalToggle(&enableKey);
        enableKeyOutToggle->setOnPress(
            [=](bool state)
            {
                this->setMenuState(state, SettingsGroup::KEY);
            });
        this->enableKeyOutControl = new DigitalControl(
            Vec(10, 50),
            "Show Key Out",
            "Hide Key Out",
            enableKeyOutToggle,
            nullptr);
        addChild(enableKeyOutControl);
    }

    setMenuState(enableMenu, SettingsGroup::MAIN);

    // changeArraySize(sorterArray->array.size());
}
void SorterArrayWidget::setMenuState(
    bool enable,
    SettingsGroup group)
{
    this->menuState = group;

    bool enableGroup;
    {
        enableGroup = false;
        if (group == SettingsGroup::MAIN)
        {
            enableGroup = enable;
        }
        if (changeAlgorithmControl)
            changeAlgorithmControl->setVisible(enableGroup);
        if (arraySizeControl)
            arraySizeControl->setVisible(enableGroup);
        if (enableMenuControl)
            enableMenuControl->setVisible(enableGroup);
        if (slowerShuffleControl)
            slowerShuffleControl->setVisible(enableGroup);
        if (slowerTraversalControl)
            slowerTraversalControl->setVisible(enableGroup);
    }

    {
        enableGroup = false;
        if (group == SettingsGroup::FILTER)
        {
            enableGroup = enable;
        }
        for (int i = 0; i < 6; ++i)
        {
            if (filterEventsControls[i])
                filterEventsControls[i]->setVisible(enableGroup);
        }
        if (enableFiltersControl)
            enableFiltersControl->setVisible(enableGroup);
    }

    {
        enableGroup = false;
        if (group == SettingsGroup::KEY)
        {
            enableGroup = enable;
        }
        if (enableKeyControl)
            enableKeyControl->setVisible(enableGroup);
        if (scaleControl)
            scaleControl->setVisible(enableGroup);
        if (keyOffsetControl)
            keyOffsetControl->setVisible(enableGroup);
        if (enableKeyOutControl)
            enableKeyOutControl->setVisible(enableGroup);
    }

    if (!enable)
    {
        if (enableMenuControl)
            enableMenuControl->setVisible(true);
        if (enableFiltersControl)
            enableFiltersControl->setVisible(true);
        if (enableKeyOutControl)
            enableKeyOutControl->setVisible(true);
    }
}

void SorterArrayWidget::changeAlgorithm(t_algorithmtype type)
{
    this->sorterArray->changeAlgorithm(type);

    char buffer[20];
    snprintf(buffer, 20, "Unknown Sort");
    if (sorterArray)
    {
        t_algorithmtype t = sorterArray->currentType;
        auto itr = sorterArray->algorithms.find(t);
        if (itr != sorterArray->algorithms.end())
            snprintf(buffer, 20, "%s Sort", itr->second->name.c_str());
    }
    this->changeAlgorithmStr = std::string(buffer);
}

void SorterArrayWidget::changeShuffleSkip(int frames)
{
    sorterArray->shuffleSkip = (frames);

    if (slowerShuffleControl && slowerShuffleControl->control)
    {
        Widget *w = slowerShuffleControl->control;
        ArraySizeSlider *slider;
        slider = dynamic_cast<ArraySizeSlider *>(w);
        if (slider)
        {
            float x = (frames - 1);
            x /= (MAX_SHUFFLE_SKIPS - 1);
            slider->internalValue = x;
        }
    }
}

void SorterArrayWidget::changeTraversalSkip(int frames)
{
    sorterArray->traverseSkip = (frames);

    if (slowerTraversalControl && slowerTraversalControl->control)
    {
        Widget *w = slowerTraversalControl->control;
        ArraySizeSlider *slider;
        slider = dynamic_cast<ArraySizeSlider *>(w);
        if (slider)
        {
            float x = (frames - 1);
            x /= (MAX_TRAVERSAL_SKIPS - 1);
            slider->internalValue = x;
        }
    }
}

void SorterArrayWidget::changeOutScale(const std::string &name)
{
    std::string s = "";
    const Scale &scale = this->scalesMap[name];
    for (int i = 0; i < (int)scale.intervals.size(); ++i)
    {
        s += std::to_string(scale.intervals[i]) + ", ";
    }
    // DEBUG("oum59k835y %s", s.c_str());

    this->sorterArray->outScale = this->scalesMap[name];
}

void SorterArrayWidget::changeOutKeyOffset(int offset)
{
    this->sorterArray->keyOffset = offset;

    if (keyOffsetControl && keyOffsetControl->control)
    {
        Widget *w = keyOffsetControl->control;
        ArraySizeSlider *slider;
        slider = dynamic_cast<ArraySizeSlider *>(w);
        if (slider)
        {
            float x = offset;
            x -= MIN_KEY_OFFSET;
            x /= (MAX_KEY_OFFSET - MIN_KEY_OFFSET);
            slider->internalValue = x;
        }
    }
}

void SorterArrayWidget::updateUIFromState()
{
    if (!sorterArray)
        return;

    // Update array size control
    arraySizeFloat = static_cast<float>(sorterArray->arraySize) / MAX_ARRAY_SIZE;
    arraySizeString = "Size: " + std::to_string(sorterArray->arraySize);

    changeAlgorithm(this->sorterArray->currentType);
    changeArraySize((int)this->sorterArray->array.size(), false);
    changeShuffleSkip(this->sorterArray->shuffleSkip);
    changeTraversalSkip(this->sorterArray->traverseFrames);
    changeOutScale(this->sorterArray->outScale.name);
    changeOutKeyOffset(this->sorterArray->keyOffset);

    // Update shuffle skip control
    if (sorterArray->shuffleSkip >= 1 && sorterArray->shuffleSkip <= MAX_SHUFFLE_SKIPS)
    {
        slowerShuffleFloat = static_cast<float>(sorterArray->shuffleSkip - 1) / (MAX_SHUFFLE_SKIPS - 1);
    }

    // Update traversal skip control
    if (sorterArray->traverseSkip >= 1 && sorterArray->traverseSkip <= MAX_TRAVERSAL_SKIPS)
    {
        slowerTraversalFloat = static_cast<float>(sorterArray->traverseSkip - 1) / (MAX_TRAVERSAL_SKIPS - 1);
    }

    // Update key offset control
    if (sorterArray->keyOffset >= MIN_KEY_OFFSET && sorterArray->keyOffset <= MAX_KEY_OFFSET)
    {
        keyOffsetFloat = static_cast<float>(sorterArray->keyOffset - MIN_KEY_OFFSET) /
                         (MAX_KEY_OFFSET - MIN_KEY_OFFSET);
    }
}

void SorterArrayWidget::changeArraySize(int size, bool reset)
{
    if (!sorterArray->processingFinished)
        return;

    if (reset)
        this->sorterArray->reset(size, __LINE__, __FILE__);

    char buffer[20];
    sprintf(buffer, "Size: %d", (int)(this->sorterArray->array.size()));
    this->arraySizeString = std::string(buffer);

    if (arraySizeControl && arraySizeControl->control)
    {
        Widget *w = arraySizeControl->control;
        ArraySizeSlider *slider;
        slider = dynamic_cast<ArraySizeSlider *>(w);
        if (slider)
        {
            slider->internalValue = (float)size / MAX_ARRAY_SIZE;
        }
    }
}

void SorterArrayWidget::drawText(
    const DrawArgs &args,
    const Vec &pos,
    const std::string &text,
    float width)
{
    float height = 18;
    // nvgBeginPath(args.vg);
    // nvgRoundedRect(args.vg, pos.x, pos.y, width, height, 2.5f);
    // nvgFillColor(args.vg, nvgRGB(49, 42, 9));
    // nvgFill(args.vg);

    nvgFontSize(args.vg, 12);
    nvgFontFaceId(args.vg, 0);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    nvgFillColor(args.vg, nvgRGB(234, 202, 63));
    nvgText(args.vg, pos.x + 8, pos.y + height / 2, text.c_str(), NULL);
}

void SorterArrayWidget::drawRect(
    const DrawArgs &args,
    int i,
    const NVGcolor &color)
{
    float x = width * i;
    float val = sorterArray->array[i];
    float height = val * scale;

    nvgBeginPath(args.vg);
    nvgRect(
        args.vg,
        x,
        box.size.y - height,
        width + 0.1,
        height);
    nvgFillColor(args.vg, color);
    nvgFill(args.vg);
    nvgClosePath(args.vg);
}

void SorterArrayWidget::draw(const DrawArgs &args)
{
    if (!sorterArray)
    {
        auto demoImage = APP->window->loadImage(
            asset::plugin(pluginInstance, "res/images/SortDemo.png"));

        if (demoImage && demoImage->handle >= 0)
        {
            NVGpaint paint = nvgImagePattern(
                args.vg,
                0, 0,                   // x, y position
                box.size.x, box.size.y, // size
                0.0f,                   // no rotation
                demoImage->handle,      // image handle
                1.0f                    // alpha
            );

            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgFillPaint(args.vg, paint);
            nvgFill(args.vg);
        }

        return;
    }

    int n = static_cast<int>(sorterArray->arraySize);

    this->width = 0.0f;
    if (!sorterArray->array.empty())
        this->width = box.size.x / (int)sorterArray->array.size();
    if (n)
        this->scale = box.size.y / n;

    {
        nvgBeginPath(args.vg);
        nvgRect(
            args.vg,
            0, 0,
            box.size.x, box.size.y);
        nvgFillColor(args.vg, backgroundColor);
        nvgFill(args.vg);
        nvgClosePath(args.vg);
    }

    if (sorterArray && !sorterArray->array.empty())
    {
        for (int i = 0; i < (int)sorterArray->array.size(); ++i)
        {
            drawRect(args, i, defaultColor);
        }

        if (sorterArray->eventCurrent)
        {
            auto &elements = sorterArray->eventCurrent->elements;
            for (SelectedType &st : elements)
            {
                NVGcolor color = defaultColor;
                switch (st.elementEvent)
                {
                case ELEMENT_EVENT_READ:
                    color = greenColor;
                    break;
                case ELEMENT_EVENT_WRITE:
                    color = redColor;
                    break;
                case ELEMENT_EVENT_REMOVE:
                    continue;
                default:
                    color = blueColor;
                    break;
                }

                drawRect(args, st.index, color);
            }
        }
    }

    if (enableMenu || enableFilters || enableKey)
    {
        NVGcolor color = nvgRGBA(255 - 234, 255 - 202, 255 - 63, 127);

        int xPos = 110;

        nvgBeginPath(args.vg);
        nvgRect(
            args.vg,
            xPos,
            0,
            box.size.x - xPos,
            box.size.y);
        nvgFillColor(args.vg, color);
        nvgFill(args.vg);
        nvgClosePath(args.vg);

        nvgBeginPath(args.vg);
        nvgRect(
            args.vg,
            xPos,
            0,
            5,
            box.size.y);
        nvgFillColor(args.vg, color);
        nvgFill(args.vg);
        nvgClosePath(args.vg);
    }

    // Draw children
    SorterArrayBase::draw(args);

    if (enableMenu)
    {
        drawText(args, {120, 30}, arraySizeString);
        drawText(args, {120, 75}, changeAlgorithmStr, 120);
    }
    else if (enableFilters)
    {
    }
    else if (enableKey)
    {
        drawText(args, {120, 52}, sorterArray->outScale.name, 120);
    }
    
    {
        std::string string = "";
        if (!sorterArray->array.empty())
        {
            string = "";
        }
        else if (!sorterArray->processingFinished)
        {
            string = "Calculating...";
        }
        else
        {
            switch (sorterArray->section)
            {
            case SECTION_SHUFFLE:
                string = "Currenly: Shuffling";
                break;
            case SECTION_STEP:
                string = "Currenly: Sorting";
                break;
            case SECTION_TRAVERSE:
                string = "Currenly: Traversing";
                break;
            default:
                string = "Currenly: Doing Nothing";
                break;
            }
        }

        drawText(args, {50, 100}, string);
    }
}

void SorterArrayWidget::onButton(const event::Button &e)
{
    SorterArrayBase::onButton(e);
    if (e.isConsumed())
        return;

    bool ctrl = (APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL;
    if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS && !ctrl)
    {
        // e.consume(this);
        dragPosition = e.pos;

        // e.stopPropagating();
        e.consume(this);
    }
}

void SorterArrayWidget::onDragStart(const DragStartEvent &e)
{
    SorterArrayBase::onDragStart(e);
    if (e.isConsumed())
        return;
    e.stopPropagating();

    dragging = true;
}

// based on code from https://github.com/mgunyho/PdArray/blob/master/src/Array.cpp
void SorterArrayWidget::onDragMove(const DragMoveEvent &e)
{
    SorterArrayBase::onDragMove(e);
    if (e.isConsumed())
        return;
    e.stopPropagating();

    Vec dragPosition_old = dragPosition;
    float zoom = getAbsoluteZoom();
    dragPosition = dragPosition.plus(e.mouseDelta.div(zoom));

    // int() rounds down, so the upper limit of rescale is buffer.size() without -1.
    if (this->sorterArray->array.empty())
        return;
    int s = this->sorterArray->array.size();
    math::Vec bs = box.size;
    int i1 = clamp(int(map(dragPosition_old.x, 0, bs.x, 0, s)), 0, s - 1);
    int i2 = clamp(int(map(dragPosition.x, 0, bs.x, 0, s)), 0, s - 1);

    if (abs(i1 - i2) < 2)
    {
        float y = clamp(map(dragPosition.y, 0, bs.y, (float)s, 0.f), 0.f, (float)s);
        this->sorterArray->array[i2] = y;
    }
    else
    {
        // mouse moved more than one index, interpolate
        float y1 = clamp(map(dragPosition_old.y, 0, bs.y, (float)s, 0.f), 0.f, (float)s);
        float y2 = clamp(map(dragPosition.y, 0, bs.y, (float)s, 0.f), 0.f, (float)s);
        if (i2 < i1)
        {
            std::swap(i1, i2);
            std::swap(y1, y2);
        }
        for (int i = i1; i <= i2; i++)
        {
            float y = y1 + map(i, i1, i2, 0.f, 1.0f) * (y2 - y1);
            this->sorterArray->array[i] = y;
        }
    }
}

void SorterArrayWidget::onDragEnd(const DragEndEvent &e)
{
    SorterArrayBase::onDragEnd(e);
    if (e.isConsumed())
        return;
    e.stopPropagating();

    dragging = false;
}