#pragma once

#include "IWindow.h"

#include <imgui.h>
#include <vector>

class DrawFigureWindow :
  public IWindow
{
public: // Construction

  DrawFigureWindow(
      const std::string & window_name,
      const ImU32 color
    );

public: // Interface

  const std::vector<ImVec2> GetPoints() const;

protected: // IWindow

  std::string GetWindowName() const override;

  void UpdateFrameData() override;

private: // Constants

  static constexpr float POINTS_INDENT = 25.f;

private: // Members

  std::string         m_WindowName;
  ImU32               m_Color;
  std::vector<ImVec2> m_Points;
  bool                m_WasMouseDown = false;
};

