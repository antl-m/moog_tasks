#pragma once

#include "IWindow.h"

#include <imgui.h>
#include <vector>

class SplineDrawingWindow :
  public IWindow
{
public: // Construction

  SplineDrawingWindow(
      const std::string & window_name
    );

protected: // IWindow

  std::string GetWindowName() const override;

  void UpdateFrameData() override;

private: // Constants

private: // Members

  std::string m_WindowName;
  bool        m_IsFirstFrame = true;
  ImVec2      m_FirstPoint{ 0, 0 };
  ImVec2      m_SecondPoint{ 0, 0 };
  ImVec2      m_FirstControlPoint{ 0, 0 };
  ImVec2      m_SecondControlPoint{ 0, 0 };
  ImVec2      m_PreviousMousePosition{ 0, 0 };
  ImVec2 *    m_DraggedPoint = nullptr;
  bool        m_WasMouseDown = false;
};

