#pragma once

#include "IWindow.h"
#include "DrawFigureWindow.h"

#include <imgui.h>
#include <vector>
#include <memory>
#include <map>

class MorphingWindow :
  public IWindow
{
public: // Construction

  MorphingWindow(
      const std::string & window_name,
      const std::shared_ptr<DrawFigureWindow> & first_figure,
      const std::shared_ptr<DrawFigureWindow> & second_figure
    );

protected: // IWindow

  std::string GetWindowName() const override;

  void UpdateFrameData() override;

private: // Constants

  static const std::vector<std::pair<std::string, ImVec2(*)(ImVec2, ImVec2, float)>> INTERPOLATE_METHODS;

private: // Members

  std::string                       m_WindowName;
  std::shared_ptr<DrawFigureWindow> m_FirstFigure;
  std::shared_ptr<DrawFigureWindow> m_SecondFigure;
  float                             m_Parameter = 0;
  bool                              m_IsAnimationActive = false;
  bool                              m_NeedDrawTransitions = false;
  float                             m_Delta = 0.35f;

  const std::pair<std::string, ImVec2(*)(ImVec2, ImVec2, float)> * m_CurrentMethod = nullptr;
};

