#include "IWindow.h"

#include <imgui.h>
#include <sstream>

namespace
{

std::string PtrToStr(
    const void const * ptr
  )
{
  std::ostringstream ss;
  ss << ptr;
  return ss.str();
}

} // namespace

//
// Construction / Destruction
//

IWindow::IWindow() :
    m_ThisStr(PtrToStr(this))
{
  // Empty
}

//
// Interface
//

void IWindow::Show()
{
  ImGui::PushID(this);

  auto WindowName = GetWindowName();

  WindowName.append("###").append(m_ThisStr);

  if (ImGui::Begin(WindowName.c_str()))
    UpdateFrameData();

  ImGui::End();

  ImGui::PopID();
}

std::string IWindow::GetWindowNameID() const
{
  return GetWindowName().append("###").append(m_ThisStr);
}
