#pragma once

#include "GLView.h"

namespace Aftr {
/**
   This class demonstrates how tessellation on the GPU can be used for rendering a large globe of
   the Earth with a continous level of detail.
*/
class GLViewEarthTessellationModule : public GLView {
public:
    static GLViewEarthTessellationModule* New(const std::vector<std::string>& outArgs);
    virtual ~GLViewEarthTessellationModule();
    virtual void updateWorld(); ///< Called once per frame
    virtual void loadMap(); ///< Called once at startup to build this module's scene
    virtual void onResizeWindow(GLsizei width, GLsizei height);
    virtual void onMouseDown(const SDL_MouseButtonEvent& e);
    virtual void onMouseUp(const SDL_MouseButtonEvent& e);
    virtual void onMouseMove(const SDL_MouseMotionEvent& e);
    virtual void onKeyDown(const SDL_KeyboardEvent& key);
    virtual void onKeyUp(const SDL_KeyboardEvent& key);

protected:
    GLViewEarthTessellationModule(const std::vector<std::string>& args);
    virtual void onCreate();

    WO* earth;
};
} //namespace Aftr
