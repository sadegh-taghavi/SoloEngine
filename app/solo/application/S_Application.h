#pragma once
#include "solo/platforms/S_BaseApplication.h"
#include "solo/renderer/S_Renderer.h"
#include "solo/resource/S_ResourceManager.h"
#include <memory>

namespace solo
{

class S_Application : public S_BaseApplication
{
public:
    S_Application( unsigned int width, unsigned int height );
    virtual void onCreateEvent();
    virtual void onSpinEvent();
    virtual void onResizeEvent(const S_WindowResizeEvent *event);
    virtual void onFocusEvent(const S_WindowFocusEvent *event);
    S_Renderer *renderer() const;
    S_ResourceManager *resourceManager() const;
    static S_Application *executingApplication();

private:
    char __padding[4];
    std::unique_ptr<S_Renderer> m_renderer;
    std::unique_ptr<S_ResourceManager> m_resourceManager;

};

}
