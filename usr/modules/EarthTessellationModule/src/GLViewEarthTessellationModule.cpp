#include "GLViewEarthTessellationModule.h"

#include "Axes.h" // We can set Axes to on/off with this
#include "ManagerOpenGLState.h" // We can change OpenGL State attributes with this
#include "PhysicsEngineODE.h"
#include "WorldList.h" // This is where we place all of our WOs

// Different WO used by this module
#include "AftrGLRendererBase.h"
#include "Camera.h"
#include "CameraChaseActorAbsNormal.h"
#include "CameraChaseActorRelNormal.h"
#include "CameraChaseActorSmooth.h"
#include "CameraStandard.h"
#include "MGLEarthQuad.h"
#include "Model.h"
#include "ModelDataShared.h"
#include "ModelMesh.h"
#include "ModelMeshDataShared.h"
#include "ModelMeshSkin.h"
#include "WO.h"
#include "WOLight.h"
#include "WOSkyBox.h"

#include "GLSLShader.h"
#include "GLSLShaderDataShared.h"

using namespace Aftr;

const static unsigned int NUM_TILES_X = 180;
const static unsigned int NUM_TILES_Y = 360;

const static float INIT_SCALE_FACTOR = 0.0001f;
const static float INIT_TESS_FACTOR = 20.0f;
const static float INIT_MAX_TESS_FACTOR = 64.0f;

GLViewEarthTessellationModule* GLViewEarthTessellationModule::New(const std::vector<std::string>& args)
{
    GLViewEarthTessellationModule* glv = new GLViewEarthTessellationModule(args);
    glv->init(Aftr::GRAVITY, Vector(0, 0, -1.0f), "aftr.conf", PHYSICS_ENGINE_TYPE::petODE);
    glv->onCreate();

    return glv;
}

GLViewEarthTessellationModule::GLViewEarthTessellationModule(const std::vector<std::string>& args)
    : GLView(args)
{
    // Note: At this point, the Managers are not yet initialized. The Engine initialization
    // occurs immediately after this method returns (see GLViewEarthTessellationModule::New() for
    // reference). Then the engine invoke's GLView::loadMap() for this module.
    // After loadMap() returns, GLView::onCreate is finally invoked.

    // The order of execution of a module startup:
    // GLView::New() is invoked:
    //    calls GLView::init()
    //       calls GLView::loadMap() (as well as initializing the engine's Managers)
    //    calls GLView::onCreate()

    // GLViewEarthTessellationModule::onCreate() is invoked after this module's LoadMap() is completed.

    earth = nullptr;
}

void GLViewEarthTessellationModule::onCreate()
{
    // GLViewEarthTessellationModule::onCreate() is invoked after this module's LoadMap() is completed.
    // At this point, all the managers are initialized. That is, the engine is fully initialized.

    if (this->pe != NULL) {
        //optionally, change gravity direction and magnitude here
        //The user could load these values from the module's aftr.conf
        this->pe->setGravityNormalizedVector(Vector(0, 0, -1.0f));
        this->pe->setGravityScalar(Aftr::GRAVITY);
    }
    this->setActorChaseType(STANDARDEZNAV); // Default is STANDARDEZNAV mode
}

GLViewEarthTessellationModule::~GLViewEarthTessellationModule()
{
    //Implicitly calls GLView::~GLView()
}

void GLViewEarthTessellationModule::updateWorld()
{
    GLView::updateWorld(); // Just call the parent's update world
}

void GLViewEarthTessellationModule::onResizeWindow(GLsizei width, GLsizei height)
{
    GLView::onResizeWindow(width, height); // call parent's resize method
}

void GLViewEarthTessellationModule::onMouseDown(const SDL_MouseButtonEvent& e)
{
    GLView::onMouseDown(e);
}

void GLViewEarthTessellationModule::onMouseUp(const SDL_MouseButtonEvent& e)
{
    GLView::onMouseUp(e);
}

void GLViewEarthTessellationModule::onMouseMove(const SDL_MouseMotionEvent& e)
{
    GLView::onMouseMove(e);
}

void GLViewEarthTessellationModule::onKeyDown(const SDL_KeyboardEvent& key)
{
    GLView::onKeyDown(key);
    if (key.keysym.sym == SDLK_0)
        this->setNumPhysicsStepsPerRender(1);

    if (key.keysym.sym == SDLK_1) {
        MGLEarthQuad* mod = earth->getModelT<MGLEarthQuad>();

        // toggle render mode between triangles and lines
        bool useLines = !mod->isUsingLines();
        mod->useLines(useLines);

        std::cout << "Using " << (useLines ? "lines" : "triangles") << std::endl;
    } else if (key.keysym.sym == SDLK_UP || key.keysym.sym == SDLK_DOWN) {
        MGLEarthQuad* mod = earth->getModelT<MGLEarthQuad>();

        // increase/decrease scale factor
        float newSF = mod->getScaleFactor() * (key.keysym.sym == SDLK_UP ? 1.05f : 0.95f);
        mod->setScaleFactor(newSF);

        std::cout << "Scale factor: " << newSF << std::endl;
    } else if (key.keysym.sym == SDLK_RIGHT || key.keysym.sym == SDLK_LEFT) {
        MGLEarthQuad* mod = earth->getModelT<MGLEarthQuad>();

        // increase/decrease tessellation factor
        float newTF = mod->getTessellationFactor() + (key.keysym.sym == SDLK_RIGHT ? 0.5f : -0.5f);
        newTF = std::max(newTF, 0.0f); // don't allow negative TF
        mod->setTessellationFactor(newTF);

        std::cout << "Tessellation factor: " << newTF << std::endl;
    } else if (key.keysym.sym == SDLK_o || key.keysym.sym == SDLK_i) {
        MGLEarthQuad* mod = earth->getModelT<MGLEarthQuad>();

        // increase/decrease max tessellation factor
        float newTF = mod->getMaxTessellationFactor() + (key.keysym.sym == SDLK_o ? 1.0f : -1.0f);
        newTF = std::max(std::min(newTF, 64.0f), 1.0f); // clamp to range [1, 64]
        mod->setMaxTessellationFactor(newTF);

        std::cout << "Max tessellation factor: " << newTF << std::endl;
    } else if (key.keysym.sym == SDLK_SPACE) {
        // Adjust camera axis of movement to match location on earth.
        // This makes the camera match what a person would see if they were
        // standing at that part of the globe.
        Vector pos = this->cam->getPosition() - earth->getPosition();
        this->cam->setCameraAxisOfHorizontalRotationViaMouseMotion(pos.normalizeMe());
    } else if (key.keysym.sym == SDLK_r) {
        // reset camera axis to default (up)
        this->cam->setCameraAxisOfHorizontalRotationViaMouseMotion(Vector(0, 0, 1));
    }
}

void GLViewEarthTessellationModule::onKeyUp(const SDL_KeyboardEvent& key)
{
    GLView::onKeyUp(key);
}

void Aftr::GLViewEarthTessellationModule::loadMap()
{
    this->worldLst = new WorldList(); // WorldList is a 'smart' vector that is used to store WO*'s
    this->actorLst = new WorldList();
    this->netLst = new WorldList();

    ManagerOpenGLState::GL_CLIPPING_PLANE = 10000.0;
    ManagerOpenGLState::GL_NEAR_PLANE = 1.0f;
    ManagerOpenGLState::enableFrustumCulling = false;
    Axes::isVisible = true;
    this->glRenderer->isUsingShadowMapping(false); // set to TRUE to enable shadow mapping, must be using GL 3.2+

    this->cam->setPosition(15, 15, 10);

    // SkyBox Textures readily available
    std::vector<std::string> skyBoxImageNames; // vector to store texture paths
    skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_gray_matter+6.jpg" );

    float ga = 0.1f; // Global Ambient Light level for this module
    ManagerLight::setGlobalAmbientLight(aftrColor4f(ga, ga, ga, 1.0f));
    WOLight* light = WOLight::New();
    light->isDirectionalLight(true);
    light->setPosition(Vector(0, 0, 100));
    // Set the light's display matrix such that it casts light in a direction parallel to the -z axis (ie, downwards as though it was "high noon")
    // for shadow mapping to work, this->glRenderer->isUsingShadowMapping( true ), must be invoked.
    light->getModel()->setDisplayMatrix(Mat4::rotateIdentityMat({ 0, 1, 0 }, 90.0f * Aftr::DEGtoRAD));
    light->setLabel("Light");
    worldLst->push_back(light);

    // Create the SkyBox
    WO* wo = WOSkyBox::New(skyBoxImageNames.at(0), this->getCameraPtrPtr());
    wo->setPosition(Vector(0, 0, 0));
    wo->setLabel("Sky Box");
    wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    worldLst->push_back(wo);

    std::string dataset = ManagerEnvironmentConfiguration::getLMM() + "/images/ETOPO1_Ice_g_geotiff.tif";
    std::string imagery = ManagerEnvironmentConfiguration::getLMM() + "/images/2_no_clouds_16k.jpg";

    // create earth WO
    earth = WO::New();

    // create and use earth model
    earth->setModel(new MGLEarthQuad(earth, Vector(90.0f, -180.0f, 0.0f), Vector(-90.0f, 180.0f, 0.0f),
        NUM_TILES_X, NUM_TILES_Y, INIT_SCALE_FACTOR, INIT_TESS_FACTOR, INIT_MAX_TESS_FACTOR, dataset, imagery));
    earth->setPosition(Vector(0.0, 0.0, 0.0)); // center earth at origin of world

    // add to world
    worldLst->push_back(earth);
}
