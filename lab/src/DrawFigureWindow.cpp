#include "DrawFigureWindow.h"

#include "ImVecUtils.h"

//
// Construction
//

DrawFigureWindow::DrawFigureWindow(
    const std::string & window_name,
    const ImU32 color
  ) :
    m_WindowName(window_name),
    m_Color(color)
{
  // Empty
}

//
// Interface
//

const std::vector<ImVec2> DrawFigureWindow::GetPoints() const
{
  return m_Points;
}

//
// IWindow
//

std::string DrawFigureWindow::GetWindowName() const
{
  return m_WindowName;
}

void DrawFigureWindow::UpdateFrameData()
{
  ImGui::Text("Points count: %d", m_Points.size());

  ImGui::BeginChild("Viewport", ImVec2(-1, -1), true);

  const auto cursor_pos = ImGui::GetCursorScreenPos();

  const auto NewPoint = ImGui::GetMousePos() - cursor_pos;
  const bool IsMouseDown = ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);

  if (IsMouseDown)
    m_Points.push_back(NewPoint);

  DrawFigure(m_Points, cursor_pos, m_Color, 3);

  if (IsMouseDown)
    m_Points.pop_back();

  if (m_WasMouseDown && !IsMouseDown)
    m_Points.push_back(NewPoint);

  if (IsMouseDown)
  {
    if (!m_WasMouseDown)
      m_Points.clear();

    if (m_Points.empty() || ImVecDistance(m_Points.back(), NewPoint) >= POINTS_INDENT)
    {
      m_Points.push_back(NewPoint);
    }

    m_WasMouseDown = true;
  }
  else
  {
    m_WasMouseDown = false;
  }

  ImGui::EndChild();
}
