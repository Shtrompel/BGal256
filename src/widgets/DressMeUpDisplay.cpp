#include "DressMeUpDisplay.hpp"

#include <widget/FramebufferWidget.hpp>
#include <context.hpp>

#include "Utils.hpp"

#include "stb_image_wrapper.h"

#include <unordered_map>

// Helper to convert OpenGL error codes to readable strings
const char *glErrorToString(GLenum error)
{
    switch (error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNKNOWN_ERROR";
    }
}

// Lambda to check OpenGL errors using VCV Rack's WARN
int checkGLError(const char *file, int line)
{
    GLenum err;
    bool errorFound = false;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        WARN("OpenGL Error (%s) at %s:%d", glErrorToString(err), file, line);
        errorFound = true;
    }
    return errorFound;
};

// Convenience macro to insert current file + line
#define GL_CHECK() checkGLError(__FILE__, __LINE__)

static bool loadShader(
    GLuint *program,
    const std::string &pathVert,
    const std::string &pathFrag);

#if 0
// save NVG image to file
static void saveNVGImageToFile(NVGcontext *vg, int imageHandle, const char *filename)
{
    int texId, w, h;
    // clear errors
    glGetError();

    // get texture info
    nvgImageSize(vg, imageHandle, &w, &h);
    // get the OpenGL texture id
    texId = nvglImageHandleGL2(vg, imageHandle);

    // create framebuffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // attach the texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer is not complete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        return;
    }

    // read pixels
    unsigned char *pixels = (unsigned char *)malloc(w * h * 4); // RGBA
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // flip vertically (OpenGL reads from bottom-left)
    for (int y = 0; y < h / 2; ++y)
    {
        for (int x = 0; x < w * 4; ++x)
        {
            unsigned char tmp = pixels[y * w * 4 + x];
            pixels[y * w * 4 + x] = pixels[(h - 1 - y) * w * 4 + x];
            pixels[(h - 1 - y) * w * 4 + x] = tmp;
        }
    }

    // save
    stbi_write_png(filename, w, h, 4, pixels, w * 4);

    // cleanup
    free(pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}
#endif

DressMeUpGLWidget::DressMeUpGLWidget()
{
    // DEBUG("Vendor graphic card: %s\n", glGetString(GL_VENDOR));
    // DEBUG("Renderer: %s\n", glGetString(GL_RENDERER));
    // DEBUG("Version GL: %s\n", glGetString(GL_VERSION));
    // DEBUG("Version GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

DressMeUpGLWidget::~DressMeUpGLWidget()
{
}

t_img_ptr DressMeUpGLWidget::loadImage(const std::string &path)
{
    return loadImage(path, {0.f, 0.f}, {1.f, 1.f}, 0.0f);
}

t_img_ptr DressMeUpGLWidget::loadImage(
    const std::string &path,
    const Vec &pos,
    const Vec &scale,
    float layer)
{
    t_img_ptr imageData = std::make_shared<ImageData>();
    imageData->attributes.pos = pos;
    imageData->layer = layer;

    int channels;
    unsigned char *data = stbi_load(
        path.c_str(),
        &imageData->imageWidth,
        &imageData->imageHeight,
        &channels,
        STBI_rgb_alpha);

    imageData->attributes.size =
        {
            scale.x * imageData->imageWidth,
            scale.y * imageData->imageHeight,
        };

    if (!data)
    {
        DEBUG("Failed to load image: %s", path.c_str());
        return t_img_ptr{};
    }

    imageData->alphaMap.resize(
        imageData->imageWidth * imageData->imageHeight);
    for (int y = 0; y < imageData->imageHeight; y++)
    {
        for (int x = 0; x < imageData->imageWidth; x++)
        {
            int idx = y * imageData->imageWidth + x;
            imageData->alphaMap[idx] = data[idx * 4 + 3];
        }
    }

    {
        // Generate OpenGL texture
        glGenTextures(1, &imageData->textureID);
        GL_CHECK();
        glBindTexture(GL_TEXTURE_2D, imageData->textureID);
        GL_CHECK();

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK();

        // Upload image data
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            imageData->imageWidth, imageData->imageHeight,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        GL_CHECK();

        stbi_image_free(data);

        glBindTexture(GL_TEXTURE_2D, 0);
        GL_CHECK();
    }

    this->textures.push_back(imageData);
    return this->textures.back();
}

void DressMeUpGLWidget::initializeGL(
    const std::string &texVertexPath,
    const std::string &texFragPath,
    const std::string &filterVertexPath,
    const std::string &filterFragPath)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Initialize OpenGL objects
    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);

    // Load Shader
    if (!loadShader(&program, texVertexPath, texFragPath))
    {
        WARN("Problem loading texVertexPath and texFragPath shader");
    }

    if (!loadShader(&filterProgram, filterVertexPath, filterFragPath))
    {
        WARN("Problem loading filterVertexPath and filterFragPath shader");
    }

    // Upload vertex data
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    GL_CHECK();

    // Create instance VBO, for the vertex sahder data
    glGenBuffers(1, &instanceVBO);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float),
        (void *)0);
    GL_CHECK();

    // UV attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float),
        (void *)(2 * sizeof(float)));
    GL_CHECK();

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    GL_CHECK();

    generateSampleTexture();
    GL_CHECK();
}

void DressMeUpGLWidget::createMainBuffer(const math::Vec &size)
{
    if (depthRB)
    {
        glDeleteRenderbuffers(1, &depthRB);
        depthRB = 0;
    }

    this->fb = nvgluCreateFramebuffer(APP->window->vg, size.x, size.y, 0);

    // Delete NanoVG's default stencil buffer (we'll replace it)
    glDeleteRenderbuffers(1, &fb->rbo);

    // Create a combined depth-stencil buffer
    glGenRenderbuffers(1, &depthRB);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);

    // Attach to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRB);

    // Check completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        DEBUG("Framebuffer incomplete: 0x%x", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DressMeUpGLWidget::reorderImages()
{
    if (1)
    {
        auto it = std::remove_if(
            textures.begin(),
            textures.end(),
            [](const t_img_ptr &tex)
            {
                return !tex;
            });
        textures.erase(it, textures.end());
    }

    auto texturesCopy = textures;
    std::sort(texturesCopy.begin(), texturesCopy.end(),
              [](const t_img_ptr &a, const t_img_ptr &b)
              {
                  if (!a || !b)
                      return false;
                  return a->layer < b->layer;
              });
    textures = std::move(texturesCopy);

    int layerIndex = 0;
    for (t_img_ptr &texture : textures)
    {
        if (texture)
        {
            texture->layerIndex = layerIndex++;
        }
    }
}

void DressMeUpGLWidget::step()
{
    DressMeUpBase::step();
    if (!module)
        return;
    if (module->drawDisplayDirty)
        this->dirty = true;
    module->drawDisplayDirty = 1;
}

void DressMeUpGLWidget::drawFramebuffer()
{

    math::Vec renderSize = getFramebufferSize();
    float zoom = APP->scene->rackScroll->getZoom();
    renderSize = renderSize.div(zoom);

    // if (module->enableShader)
    {
        if (!this->fb)
        {
            createMainBuffer(renderSize);
        }
        nvgluBindFramebuffer(this->fb);
        GL_CHECK();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_CHECK();

        glViewport(0, 0, renderSize.x, renderSize.y);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK();

        if (module)
            drawScene(box.size);
        GL_CHECK();

        nvgluBindFramebuffer(0);
        GL_CHECK();

        // saveNVGImageToFile(APP->window->vg, fb->image, "D:\\Projects\\VisualCode\\VCVRackTest\\asdasd.png");
    }
}

void DressMeUpGLWidget::draw(const DrawArgs &args)
{
    DressMeUpBase::draw(args);

    if (!this->module)
        return;

    float zoom = APP->scene->rackScroll->getZoom();
    math::Vec fbSize = getFramebufferSize();

    nvgluBindFramebuffer(getFramebuffer());
    GL_CHECK();

    if (!module->enableShader)
    {
        glViewport(0, 0, fbSize.x, fbSize.y);
        GL_CHECK();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK();

        drawScene(box.size);
        GL_CHECK();

        // Unbind VAO and shader program when finished
        glBindVertexArray(0);
        glUseProgram(0);
        GL_CHECK();

        nvgluBindFramebuffer(0);
        return;
    }

    if (!this->fb)
    {
        return;
    }

    auto startShader = [](
                           const math::Vec &fbSize,
                           NVGLUframebuffer *framebuffer,
                           int shaderProgram)
    {
        // Set viewport for default framebuffer (using window dimensions or fbSize)
        glViewport(0, 0, fbSize.x, fbSize.y);
        GL_CHECK();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK();

        glUseProgram(shaderProgram);

        // Activate texture unit 0 and bind our FBO texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer->texture); // fb->image
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);
        glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

        // Projection Matrix
        float projectionMat[16] = {
            0.0f, 2.0f, 0.0f, 0.0f,  // X: maps 0→-1, 1→1
            -2.0f, 0.0f, 0.0f, 0.0f, // Y: maps 0→1, 1→-1 (flipped)
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, -1.0f, 1.0f};
        // Set uniforms
        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "projectionMatrix"),
            1, GL_FALSE, projectionMat);
        GL_CHECK();
    };

    auto endShader = [=]()
    {
        // Bind the VAO that defines your fullscreen quad geometry
        glBindVertexArray(vao);
        // Draw the fullscreen quad (assuming 4 vertices using a triangle fan)
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Unbind VAO and shader program when finished
        glBindVertexArray(0);
        glUseProgram(0);

        nvgluBindFramebuffer(0);
        GL_CHECK();
    };

    // fbSize = fbSize.div(zoom);
    static int frameCount = 0;
    ShaderParameters &shaderParams =
        module->shaderParams;

    startShader(fbSize, fb, filterProgram);

    glUniform1f(
        glGetUniformLocation(filterProgram, "uTime"),
        (frameCount++) / 60.0f);

    glUniform2f(
        glGetUniformLocation(filterProgram, "uResolution"),
        fbSize.x, fbSize.y);
    glUniform1f(
        glGetUniformLocation(filterProgram, "uZoom"),
        zoom);

    glUniform1f(
        glGetUniformLocation(filterProgram, "uSpotWidth"),
        shaderParams.spotWidth);
    glUniform1f(
        glGetUniformLocation(filterProgram, "uSpotHeight"),
        shaderParams.spotHeight);
    glUniform1f(
        glGetUniformLocation(filterProgram, "uColorBoost"),
        shaderParams.colorBoost);
    glUniform1f(
        glGetUniformLocation(filterProgram, "uInputGamma"),
        shaderParams.inputGamma);
    glUniform1f(
        glGetUniformLocation(filterProgram, "uOutputGamma"),
        shaderParams.outputGamma);

    glUniform1f(
        glGetUniformLocation(filterProgram, "uEffectScale"),
        shaderParams.effectScale);

    GL_CHECK();

    endShader();
}

void DressMeUpGLWidget::drawLayer(const DrawArgs &args, int layer)
{
}

void DressMeUpGLWidget::onResize(const ResizeEvent &e)
{
    Vec currentSize = getFramebufferSize();
    if (fb)
    {
        if (currentSize == lastSize)
            return;
        nvgluDeleteFramebuffer(fb);
    }
    createMainBuffer(currentSize);
    this->lastSize = currentSize;
}

void DressMeUpGLWidget::drawScene(const math::Vec &size)
{
    float width = size.x;
    float height = size.y;

    // Use shader program
    glUseProgram(program);
    GL_CHECK();

    // Projection Matrix
    float projectionMat[16] = {
        2.0f, 0.0f, 0.0f, 0.0f,  // X: maps 0→-1, 1→1
        0.0f, -2.0f, 0.0f, 0.0f, // Y: maps 0→1, 1→-1 (flipped)
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, -1.0f, 1.0f};
    GL_CHECK();

    // Set uniforms
    glUniformMatrix4fv(
        glGetUniformLocation(program, "projectionMatrix"),
        1, GL_FALSE, projectionMat);

    glUniform3f(
        glGetUniformLocation(program, "uColor"),
        1.0f, 1.0f, 0.0f);

    GL_CHECK();

    for (int i = 0; i < (int)textures.size(); ++i)
    {
        auto imageData = textures.at(i);
        InstanceAttributes &atr = imageData->attributes;

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, imageData->textureID);
        glUniform1i(glGetUniformLocation(program, "uTex"), 0);

        glUniform2f(
            glGetUniformLocation(program, "uTexSize"),
            imageData->imageWidth, imageData->imageHeight);
        glUniform1i(
            glGetUniformLocation(program, "uEnableGlow"),
            imageData->enableGlow);

        // Set position and size uniforms
        GLint posLoc = glGetUniformLocation(program, "uPosition");
        GLint sizeLoc = glGetUniformLocation(program, "uSize");
        // GLint layerLoc = glGetUniformLocation(program, "uLayer");
        glUniform2f(posLoc, atr.pos.x / width, atr.pos.y / height);
        glUniform2f(sizeLoc, atr.size.x / width, atr.size.y / height);
        // glUniform1f(layerLoc*0+(rand()%1000)/1000.0f, atr.layer);

        // Draw one instance
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 5, 1); // only one instance
        glBindVertexArray(0);
    }
    GL_CHECK();

    // Cleanup
    glUseProgram(0);
    GL_CHECK();
}

void DressMeUpGLWidget::generateSampleTexture()
{
    // Chatgpt

    glGenTextures(1, &debugTexture);
    glBindTexture(GL_TEXTURE_2D, debugTexture);

    // Define a 2x2 image with 4 different colors (each pixel is RGB)
    // Pixel layout (row-major order):
    // +-----------+-----------+
    // |  Red      |  Green    |
    // +-----------+-----------+
    // |  Blue     |  White    |
    // +-----------+-----------+

    unsigned char pixels[] = {
        // First row: (red, green)
        255, 0, 0,    // Red:   (255, 0, 0)
        0, 255, 0,    // Green: (0, 255, 0)
                      // Second row: (blue, white)
        0, 0, 255,    // Blue:  (0, 0, 255)
        255, 255, 255 // White: (255, 255, 255)
    };

    // Upload the pixel data as a 2x2 texture.
    // Note: The ordering of rows depends on how you want to map the texture coordinates.
    // Typically, OpenGL expects the first row to be the bottom row in the image.
    // You might need to adjust the data order if you want a specific mapping.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Set texture parameters for wrapping and filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Optionally, unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DressMeUpGLWidget::onContextDestroy(const ContextDestroyEvent &e)
{
    // Delete resources in reverse creation order
    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (program)
    {
        glDeleteProgram(program);
        program = 0;
    }
    for (auto &tex : textures)
    {
        if (tex->textureID)
        {
            glDeleteTextures(1, &tex->textureID);
        }
    }

    if (depthRB)
    {
        glDeleteRenderbuffers(1, &depthRB);
        depthRB = 0;
    }

    nvgluDeleteFramebuffer(this->fb);

    initialized = false;

    DressMeUpBase::onContextDestroy(e);
}

static bool loadShader(
    GLuint *program,
    const std::string &pathVert,
    const std::string &pathFrag)
{
    bool out = true;

    // error checking
    GLint success;
    char infoLog[512];

    std::string vertShaderSrcStd = loadTextFromPluginFile(
        pathVert);
    const char *vertShaderSrc = vertShaderSrcStd.c_str();

    std::string fragShaderSrcStd = loadTextFromPluginFile(
        pathFrag);
    const char *fragShaderSrc = fragShaderSrcStd.c_str();

    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertShaderSrc, NULL);
    glCompileShader(vertexShader);
    GL_CHECK();

    // After glCompileShader(vertexShader):
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        DEBUG("Vertex shader error: %s", infoLog);
        out = false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSrc, NULL);
    glCompileShader(fragmentShader);
    GL_CHECK();

    // After glCompileShader(vertexShader):
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        DEBUG("Fragment shader error: %s", infoLog);
        out = false;
    }

    // Create shader program
    *program = glCreateProgram();
    glAttachShader(*program, vertexShader);
    glAttachShader(*program, fragmentShader);
    glLinkProgram(*program);
    GL_CHECK();

    return out;
}

DressMeUpDisplay::DressMeUpDisplay()
{
}

DressMeUpDisplay::~DressMeUpDisplay()
{
}

void DressMeUpDisplay::init(
    DressMeUp *module,
    const math::Vec &pos,
    const math::Vec &size)
{
    if (settings::headless)
    {
        this->child = nullptr;
    }
    else
    {
        DressMeUpGLWidget *display =
            createWidget<DressMeUpGLWidget>(
                Vec(0, 0));
        display->box.size = size;
        display->module = module;
        this->child = display;
        addChild(display);

        /*
        bodyImage = child->loadImage(
            rack::asset::plugin(pluginInstance, "res/images/Title.png"),
            Vec{0.f, 0.f},
            Vec{0.2f, 0.2f},
            0.8f);
        Vec centerPos =
            {
                (float)box.size.x / 2.f - 529.f / 2.f * 0.2f,
                5.f};
        // centerPos -= bodyImage->attributes.size / 2;
        bodyImage->attributes.pos = centerPos;
        */
    }
}

void DressMeUpDisplay::draw(const DrawArgs &args)
{
    // Widget::draw(args);
}

static bool isOpenGL33OrAbove()
{
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    if (!version || !*version)
    {
        return false;
    }

    int major = 0;
    int minor = 0;
    const char *ptr = version;

    // OpenGL ES
    if (const char *esToken = strstr(version, "OpenGL ES"))
    {
        ptr = esToken + 9; // Skip "OpenGL ES"
    }

    // Skip to first digit
    while (*ptr && !isdigit(static_cast<unsigned char>(*ptr)))
    {
        ptr++;
    }

    // Parse major version
    if (*ptr)
    {
        major = atoi(ptr);
        while (*ptr && isdigit(static_cast<unsigned char>(*ptr)))
        {
            ptr++;
        }
    }

    // Parse minor version after dot
    if (*ptr == '.')
    {
        ptr++;
        if (*ptr && isdigit(static_cast<unsigned char>(*ptr)))
        {
            minor = atoi(ptr);
        }
    }

    // Check if version >= 3.3
    return (major > 3) || (major == 3 && minor >= 3);
}

void DressMeUpDisplay::drawLayer(const DrawArgs &args, int layer)
{
    // Menu Demo
    if (!child || !child->module)
    {
        auto demoImage = APP->window->loadImage(
            asset::plugin(pluginInstance, "res/images/DressingDemo.png"));

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
    }

    const std::string FONT_PATH = "res/fonts/LCDM2N__.TTF";

    if (!isOpenGL33OrAbove())
    {
        std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(
            pluginInstance, FONT_PATH));

        nvgFontSize(args.vg, 16);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(args.vg, nvgRGB(247, 42, 33));

        char warningText[100];
        snprintf(
            warningText, 100,
            "Requiered OpenGL version\n"
            "is: at minimum \"3.3.0\"\n"
            "Version detected is\n"
            "\"%s\"",
            glGetString(GL_VERSION));

        nvgTextBox(args.vg,
                   50, 50,
                   box.size.x,
                   warningText,
                   NULL);

        return;
    }

    if (child)
    {
        child->draw(args);
    }
}

void DressMeUpDisplay::sortClothing()
{
    if (child)
        child->reorderImages();

    /*
    auto getLayerId = [](const ClothingImage &a)
    {
        int maxLayerId = 0;
        for (auto &part : a.parts)
        {
            maxLayerId = std::max(
                maxLayerId,
                part.imageData->layerIndex);
        }
        return maxLayerId;
    };

        std::sort(clothes.begin(), clothes.end(),
                  [&getLayerId](const ClothingImage &a, const ClothingImage &b)
                  {
                      return getLayerId(a) < getLayerId(b);
                  });
    */

    std::unordered_map<t_clothtype, int> clothesPositions;
    for (ClothingImage &cloth : clothes)
    {
        if (cloth.type < 0 ||
            cloth.type >= CLOTHTYPE_COUNT ||
            cloth.parts.empty())
        {
            continue;
        }

        // count current cloth index from the same type
        auto itr = clothesPositions.find(cloth.type);
        if (itr == clothesPositions.end())
            clothesPositions[cloth.type] = 0;
        else
            itr->second++;

        if (clothCurrent[cloth.type] &&
            clothCurrent[cloth.type]->id == cloth.id)
        {
            continue;
        }

        int pos = clothesPositions[cloth.type];

        // update every part
        for (auto &part : cloth.parts)
        {
            if (!part.imageData)
                continue;

            float x = rangeMap(
                pos,
                -1.f, 4.f, box.size.x / 4.f, box.size.x);
            float y = rangeMap(
                (int)cloth.type,
                -1.f, 4.f, box.size.y / 16.f, box.size.y);

            // align all parts (this code will cancel itself out on index 0)
            t_img_ptr imgData = part.imageData;
            imgData->attributes.pos = {
                x - imgData->attributes.size.x / 2.f,
                y - imgData->attributes.size.y / 2.f};

            Vec sub = part.centerPos - cloth.parts[0].centerPos;
            sub *= scale;
            imgData->attributes.pos += sub;
        }
    }
}

void DressMeUpGLWidget::checkFramebufferContent()
{
    if (!fb)
        return;

    // Read a single pixel from the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
    unsigned char pixel[4];
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    DEBUG("Framebuffer top-left pixel: R=%d G=%d B=%d A=%d",
          pixel[0], pixel[1], pixel[2], pixel[3]);
}

void DressMeUpDisplay::updateClothing()
{

    if (!clothingManager)
        return;

    t_clothtype i = 0;

    for (; i < CLOTHTYPE_COUNT; ++i)
    {
        changeCloth(i, clothingManager->clothCurrent[i]);
    }
}

void DressMeUpDisplay::snapToBody(ClothingImage *draggedCloth)
{

    if (!bodyImage)
        return;

    // update all part's positions
    for (auto &part : draggedCloth->parts)
    {
        Vec newCenter = bodyImage->attributes.pos +
                        (scale * part.centerPos);
        newCenter -=
            part.imageData->attributes.size / 2.0f;

        part.imageData->attributes.pos = newCenter;
    }

    bool changeClothing = clothCurrent[draggedCloth->type];

    clothCurrent[draggedCloth->type] =
        draggedCloth;

    if (changeClothing)
        sortClothing();
}

void DressMeUpDisplay::loadBody(
    const std::string &name)
{

    if (child)
    {

        bodyImage = child->loadImage(
            rack::asset::plugin(pluginInstance, name),
            Vec{0.f, 0.f},
            Vec{scale, scale},
            0.4f);
        Vec centerPos = {box.size.x / 16.f * 3, box.size.y / 2.f};
        centerPos -= bodyImage->attributes.size / 2;
        bodyImage->attributes.pos = centerPos;
    }
}

void DressMeUpDisplay::loadCloth(
    const std::string &name,
    t_clothtype clothType,
    const math::Vec &centerPos,
    float layer,
    const std::string &connected)
{
    std::string str = std::string("res/images/clothes/") + name + ".png";

    float x = 0, y = 0;
    t_img_ptr textureId = nullptr;
    if (child)
    {
        if (child->box.size.x == 0)
            x = fmodf(rand(), child->box.size.x);
        if (child->box.size.y == 0)
            y = fmodf(rand(), child->box.size.y);

        textureId = child->loadImage(
            rack::asset::plugin(pluginInstance, str),
            Vec{x, y},
            Vec{scale, scale},
            layer);
    }

    if (connected == "" && clothingManager)
    {
        ClothingImage::Part parts{textureId, {0.0f, 0.0f}, centerPos};
        int newId = clothingManager->clothCounter[(int)clothType] + 1;
        clothes.push_back(
            {{parts},
             clothType,
             name,
             newId});
        clothingManager->loadCloth(clothType);
    }
    else
    {
        auto itr = std::find_if(
            clothes.begin(), clothes.end(),
            [connected](const ClothingImage &image)
            {
                return image.name == connected;
            });

        if (itr != clothes.end())
        {
            ClothingImage::Part part{
                textureId,
                part.centerPos - itr->parts[0].centerPos,
                centerPos};
            itr->parts.push_back(part);
        }
        else
        {
            WARN("Can't find cloth named %s\n", connected.c_str());
        }
    }
}

void DressMeUpDisplay::changeCloth(
    t_clothtype clothType,
    int clothId)
{
    if (!clothingManager)
        return;

    if (clothId == 0 &&
        0 <= clothType &&
        clothType < CLOTHTYPE_COUNT)
    {
        clothCurrent[clothType] = nullptr;
        clothingManager->changeCloth(clothType, 0);
        sortClothing();
        return;
    }

    for (int i = 0; i < (int)clothes.size(); ++i)
    {
        if (clothes[i].type == clothType &&
            clothes[i].id == clothId)
        {
            changeCloth(&clothes[i]);
            clothingManager->changeCloth(clothType, clothId);
            break;
        }
    }
}

void DressMeUpDisplay::changeCloth(ClothingImage *cloth)
{

    snapToBody(cloth);
}

void DressMeUpDisplay::highlightWorn(
    t_clothtype clothType)
{
    if (!child)
        return;

    if (0 > clothType || clothType >= CLOTHTYPE_COUNT)
        return;

    ClothingImage *current = nullptr;
    try
    {
        current = clothCurrent.at(clothType);
    }
    catch (const std::out_of_range &ex)
    {
        return;
    }

    if (this->lastHighlighted)
        this->lastHighlighted->setHightlight(false);
    this->lastHighlighted = nullptr;

    if (this->lastHighlighted &&
        (!current || (this->lastHighlighted != current)))
    {
        if (this->lastHighlighted)
            this->lastHighlighted->setHightlight(false);
        this->lastHighlighted = nullptr;
    }

    if (current)
    {
        current->setHightlight(true);
        this->lastHighlighted = current;
    }
}

void DressMeUpDisplay::onButton(const event::Button &e)
{
    if (!(e.button == GLFW_MOUSE_BUTTON_LEFT))
    {
        return;
    }

    if (e.action == GLFW_PRESS)
    {
        e.consume(this);
        posMouseStart = e.pos;
        isDragging = true;
    }
}

static bool checkAlphaValue(const Vec &localPos, const t_img_ptr &imgData)
{
    Rect bbox;
    bbox.pos = imgData->attributes.pos;
    bbox.size = imgData->attributes.size;

    int texX = static_cast<int>(
        (localPos.x / bbox.size.x) * imgData->imageWidth);
    int texY = static_cast<int>(
        (localPos.y / bbox.size.y) * imgData->imageHeight);

    int alphaIdx = texY * imgData->imageWidth + texX;
    if (alphaIdx >= 0 && (size_t)alphaIdx < imgData->alphaMap.size())
    {
        uint8_t alpha = imgData->alphaMap[alphaIdx];
        return alpha > 0;
    }
    return false;
}

void DressMeUpDisplay::onDragStart(const DragStartEvent &e)
{
    if (!isDragging)
        return;
    posDrag = APP->scene->rack->getMousePos();

    for (auto it = clothes.rbegin(); it != clothes.rend(); ++it)
    {
        ClothingImage &cloth = *it;
        for (auto &part : cloth.parts)
        {
            Rect bbox;
            bbox.pos = part.imageData->attributes.pos;
            bbox.size = part.imageData->attributes.size;

            if (bbox.size.x == 0.0f || bbox.size.y == 0.0f)
                continue;

            if (!bbox.contains(posMouseStart))
            {
                continue;
            }

            // make sure the mouse is not inside a transparent pixel
            auto &imgData = part.imageData;
            Vec localPos = posMouseStart - bbox.pos;

            if (!checkAlphaValue(localPos, imgData))
                continue;

            draggedCloth = &cloth;
            initialImagePos =
                cloth.parts[0].imageData->attributes.pos;
            dragOffset = posMouseStart - initialImagePos;
        }

        if (draggedCloth)
        {
            break;
        }
    }
}

void DressMeUpDisplay::onDragMove(const DragMoveEvent &e)
{
    posNewDrag = APP->scene->rack->getMousePos();
    if (!isDragging)
        return;

    posMouse = posMouseStart + (posNewDrag - posDrag);
    if (posMouse.x > box.size.x)
        posMouse.x = box.size.x;
    if (posMouse.y > box.size.y)
        posMouse.y = box.size.y;
    if (posMouse.x < 0)
        posMouse.x = 0;
    if (posMouse.y < 0)
        posMouse.y = 0;

    if (draggedCloth)
    {
        t_img_ptr imageData =
            draggedCloth->parts[0].imageData;
        Vec mainPartNewPos =
            posMouse - dragOffset;
        for (int i = 0; i < (int)draggedCloth->parts.size(); ++i)
        {
            auto &part = draggedCloth->parts[i];

            part.imageData->attributes.pos =
                mainPartNewPos +
                imageData->attributes.size / 2 -
                part.imageData->attributes.size / 2;

            Vec sub =
                draggedCloth->parts[i].centerPos -
                draggedCloth->parts[0].centerPos;
            sub *= scale;
            part.imageData->attributes.pos += sub;
        }
        if (child)
            child->dirty = true;
        e.consume(this);
    }
}

void DressMeUpDisplay::onDragEnd(const DragEndEvent &e)
{
    isDragging = false;
    if (!bodyImage)
        return;

    if (draggedCloth)
    {
        Rect bbox;
        bbox.pos = bodyImage->attributes.pos;
        bbox.size = bodyImage->attributes.size;

        if (bbox.contains(posMouse))
        {
            changeCloth(draggedCloth->type, draggedCloth->id);
        }
        else
        {
            if (clothCurrent[(int)draggedCloth->type] == draggedCloth)
            {
                clothCurrent[(t_clothtype)draggedCloth->type] = nullptr;
                clothingManager->changeCloth(
                    (t_clothtype)draggedCloth->type, 0);
            }
            sortClothing();
        }

        draggedCloth = nullptr;
        e.consume(this);
    }
}