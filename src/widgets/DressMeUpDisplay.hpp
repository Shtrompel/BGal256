#ifndef _DRESS_ME_UP_DISPLAY
#define _DRESS_ME_UP_DISPLAY

#include "plugin.hpp"
#include "../DressMeUp.hpp"
#include "FramebufferLightWidget.hpp"
#include "../utils/Globals.hpp"
#include "../utils/ClothingManager.hpp"

#include <array>
#include <memory>

#define AAAA() DEBUG("%s %d", __FILE__, __LINE__)

struct InstanceAttributes
{
    Vec pos;
    Vec size;
};

struct ImageData
{
    std::string path;
    int imageWidth;
    int imageHeight;
    GLuint textureID = 0;
    float layer;
    int layerIndex = 0;
    bool enableGlow = false;

    // For mouse detection
    std::vector<uint8_t> alphaMap;

    InstanceAttributes attributes;
};

using t_img_ptr = std::shared_ptr<ImageData>;

struct ClothingImage
{
    struct Part {
        t_img_ptr imageData;
        Vec offset;
        Vec centerPos;
    };
    std::vector<Part> parts;
    t_clothtype type;
    std::string name;
    // The order in the list of the same type
    int id;

    void setHightlight(bool b)
    {
        for (int i = 0; i < (int)parts.size(); ++i)
        {
            t_img_ptr img = parts.at(i).imageData;
            if (img)
            {
                img->enableGlow = b;
            }
        }
    }

};

struct DressMeUp;

typedef FramebufferWidget DressMeUpBase;

/*
* OpenGL backend for rendering
*/
struct DressMeUpGLWidget : DressMeUpBase
{
    DressMeUp *module;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint program = 0;
    GLuint instanceVBO;

    GLuint debugTexture;

    math::Vec lastSize;

    NVGLUframebuffer *fb = nullptr;
    GLuint depthRB = 0;
    GLuint filterProgram = 0;

    bool initialized = false;

    float vertices[10] = {
        0.0f, 0.0f, // Center
        1.0f, 0.0f, // Bottom-left
        1.0f, 1.0f, // Bottom-right
        0.0f, 1.0f, // Top-right
        0.0f, 0.0f  // Top-left (closes the fan)
    };

    std::vector<t_img_ptr> textures;

    DressMeUpGLWidget();

    ~DressMeUpGLWidget();

    t_img_ptr loadImage(const std::string &path);

    //void loadImage(const std::string &path, Vec pos, Vec size);

    t_img_ptr loadImage(
        const std::string &path,
        const Vec& pos, 
        const Vec& scale,
        float layer);

    /*
        static bool loadShader(
            GLuint* program,
            const std::string& pathVert,
            const std::string& pathFrag
        );
    */

    void initializeGL(
        const std::string &texVertexPath,
        const std::string &texFragPath,
        const std::string &filterVertexPath,
        const std::string &filterFragPath);
    
    void createMainBuffer(const math::Vec& size);

    void reorderImages();

    void checkFramebufferContent();

    void step() override;

    void drawFramebuffer() override;

    // void draw(const DrawArgs &args) override;

    void draw(const DrawArgs &args) override;

    void drawLayer(const DrawArgs &args, int layer) override;

    void onResize(const ResizeEvent &e) override;

    void drawScene(const math::Vec &size);

    void generateSampleTexture();

    void onContextDestroy(const ContextDestroyEvent &e) override;
};

static float rangeMap(float x, float a, float b, float A, float B)
{
    if (b == a) return A;
    return (x - a) / (b - a) * (B - A) + A;
}

struct DressMeUpDisplay : Widget
{
    DressMeUpGLWidget *child = nullptr;
	bool enable = false;
    ClothingManager* clothingManager = nullptr;

    float scale = 0.08f;

    std::vector<ClothingImage> clothes;
    t_img_ptr bodyImage = nullptr;

    Vec posMouseStart, posDrag, posNewDrag, posMouse;
    Vec initialImagePos, dragOffset;
    ClothingImage* draggedCloth = nullptr;
    bool isDragging = false;

    ClothingImage* lastHighlighted = nullptr;

    std::array<ClothingImage*, 4> clothCurrent = 
        {nullptr, nullptr, nullptr, nullptr};
    
    DressMeUpDisplay();

    ~DressMeUpDisplay();

    void init(
        DressMeUp *module,
        const math::Vec &pos,
        const math::Vec &size);

    void draw(const DrawArgs &args) override;

    void drawLayer(const DrawArgs &args, int layer) override;

    void sortClothing();

    void updateClothing();

    void snapToBody(ClothingImage* draggedCloth);

    void loadBody(const std::string& name);

    void loadCloth(
        const std::string& name, 
        t_clothtype clothType,
        const math::Vec& centerPos,
        float layer,
        const std::string& connected = "");
    
    void changeCloth(t_clothtype clothType, int clothId);

    void changeCloth(ClothingImage* cloth);

    void highlightWorn(t_clothtype clothType);

    void onButton(const event::Button &e) override;

    void onDragStart(const DragStartEvent &e) override;
 
    void onDragEnd(const DragEndEvent &e) override;
 
    void onDragMove(const DragMoveEvent &e) override;
};

#endif // _DRESS_ME_UP_DISPLAY
