#include "MorphingWindow.h"

#include "ImVecUtils.h"

//
// Constants
//

const std::vector<std::pair<std::string, ImVec2(*)(ImVec2, ImVec2, float)>> MorphingWindow::INTERPOLATE_METHODS {
    { "Linear",      &LinearInterpolate      },
    { "Cubic",       &CubicInterpolate       },
    { "Back",        &BackInterpolate        },
    { "Elastic",     &ElasticInterpolate     },
    { "OutBounce",   &OutBounceInterpolate   },
    { "InOutBounce", &InOutBounceInterpolate },
  };

//
// Construction
//

MorphingWindow::MorphingWindow(
    const std::string & window_name,
    const std::shared_ptr<DrawFigureWindow> & first_figure,
    const std::shared_ptr<DrawFigureWindow> & second_figure
  ) :
    m_WindowName(window_name),
    m_FirstFigure(first_figure),
    m_SecondFigure(second_figure),
    m_CurrentMethod(&INTERPOLATE_METHODS.front())
{
  // Empty
}

//
// IWindow
//

std::string MorphingWindow::GetWindowName() const
{
  return m_WindowName;
}

void MorphingWindow::UpdateFrameData()
{
  if (ImGui::SliderFloat("Time", &m_Parameter, 0, 1))
    m_IsAnimationActive = false;

  ImGui::Checkbox("Animation", &m_IsAnimationActive);
  ImGui::Checkbox("Draw transitions", &m_NeedDrawTransitions);

  if (ImGui::BeginCombo("##combo", m_CurrentMethod->first.c_str()))
  {
    for (int n = 0; n < INTERPOLATE_METHODS.size(); n++)
    {
      bool is_selected = (m_CurrentMethod == &INTERPOLATE_METHODS[n]);

      if (ImGui::Selectable(INTERPOLATE_METHODS[n].first.c_str(), is_selected))
        m_CurrentMethod = &INTERPOLATE_METHODS[n];

      if (is_selected)
        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
    }
    ImGui::EndCombo();
  }

  if (m_IsAnimationActive)
  {
    const float Next = m_Parameter + ImGui::GetIO().DeltaTime * m_Delta;

    if (Next >= 1.f)
    {
      m_Parameter = 1.f;
      m_Delta *= -1;
    }
    else
    if (Next <= 0)
    {
      m_Parameter = 0.f;
      m_Delta *= -1;
    }
    else
    {
      m_Parameter = Next;
    }
  }

  ImGui::BeginChild("Viewport", ImVec2(-1, -1), true);

  const auto CursorPos = ImGui::GetCursorScreenPos();

  auto FirstFigure = m_FirstFigure->GetPoints();
  auto SecondFigure = m_SecondFigure->GetPoints();
  //auto FirstFigure = GetSpline(m_FirstFigure->GetPoints(), 10);
  //auto SecondFigure = GetSpline(m_SecondFigure->GetPoints(), 10);

  if (FirstFigure.size() > 2 && SecondFigure.size() > 2)
    std::tie(FirstFigure, SecondFigure) = FillMissingPoints(FirstFigure, SecondFigure);

  if (m_NeedDrawTransitions && FirstFigure.size() == SecondFigure.size())
  {
    DrawFigure(FirstFigure, CursorPos, 0x8000FF00, 3);

    for (int i = 0; i < FirstFigure.size(); ++i)
    {
      ImGui::GetWindowDrawList()->AddLine(
        CursorPos + FirstFigure[i],
        CursorPos + SecondFigure[i],
        0x80808080, 1
      );
    }

    DrawFigure(SecondFigure, CursorPos, 0x800000FF, 3);
  }

  DrawFigure(
      Morph(FirstFigure, SecondFigure, m_Parameter, m_CurrentMethod->second),
      CursorPos, IM_COL32(255 * m_Parameter, 255 * (1 - m_Parameter), 0, 255), 3
    );

  ImGui::EndChild();
}
