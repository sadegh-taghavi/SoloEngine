#include "S_Application.h"
#include "solo/debug/S_Debug.h"
#include "solo/file/S_File.h"
#include "solo/stl/S_List.h"

using namespace solo;

solo::S_Application::S_Application(unsigned int width, unsigned int height) : S_BaseApplication ( width, height )
{

}

void solo::S_Application::onCreateEvent()
{
    s_debugLayer( "onCreateEvent" );
    m_resourceManager = std::make_unique<S_ResourceManager>();
    m_renderer = std::make_unique<S_Renderer>();
    S_BaseApplication::onCreateEvent();
}

void solo::S_Application::onSpinEvent()
{
    m_renderer->drawFrame();
    S_BaseApplication::onSpinEvent();
}

void S_Application::onResizeEvent(const S_WindowResizeEvent *event)
{
    s_debugLayer( "onResizeEvent", event->width(), event->height() );
    m_renderer->resize( event->width(), event->height() );
    S_BaseApplication::onResizeEvent( event );
}

void S_Application::onFocusEvent(const S_WindowFocusEvent *event)
{
    s_debugLayer( "onFocusEvent", event->focus() );
    m_renderer->active( event->focus() );
    S_BaseApplication::onFocusEvent( event );
}

S_Renderer* S_Application::renderer() const
{
    return m_renderer.get();
}

S_ResourceManager *S_Application::resourceManager() const
{
    return m_resourceManager.get();
}

S_Application *S_Application::executingApplication()
{
    return static_cast<S_Application *>( S_BaseApplication::executingApplication() );
}

