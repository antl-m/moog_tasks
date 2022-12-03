#include "SplineDrawingWindow.h"

#include "ImVecUtils.h"

//
// Construction
//

SplineDrawingWindow::SplineDrawingWindow(
    const std::string & window_name
  ) :
    m_WindowName(window_name)
{
  // Empty
}

//
// IWindow
//

std::string SplineDrawingWindow::GetWindowName() const
{
  return m_WindowName;
}

void SplineDrawingWindow::UpdateFrameData()
{
  ImGui::BeginChild("Viewport", ImVec2(-1, -1), true);

  if (m_IsFirstFrame)
  {
    m_FirstPoint = ImVec2(0, 0);
    m_SecondPoint = ImVec2(50, 50);
    m_FirstControlPoint = ImVec2(0, 50);
    m_SecondControlPoint = ImVec2(50, 0);

    m_PreviousMousePosition = ImGui::GetMousePos();

    m_IsFirstFrame = false;
  }

  const auto CursorPos = ImGui::GetCursorScreenPos();

  ImGui::GetWindowDrawList()->AddCircleFilled(CursorPos + m_FirstPoint, 5, 0xFF00FF00);
  ImGui::GetWindowDrawList()->AddCircleFilled(CursorPos + m_SecondPoint, 5, 0xFF00FF00);

  ImGui::GetWindowDrawList()->AddCircleFilled(CursorPos + m_FirstControlPoint, 5, 0xFF0000FF);
  ImGui::GetWindowDrawList()->AddCircleFilled(CursorPos + m_SecondControlPoint, 5, 0xFF0000FF);

  ImGui::GetWindowDrawList()->AddLine(CursorPos + m_FirstPoint, CursorPos + m_FirstControlPoint, 0xFF0000FF, 2);
  ImGui::GetWindowDrawList()->AddLine(CursorPos + m_SecondPoint, CursorPos + m_SecondControlPoint, 0xFF0000FF, 2);

  std::vector<ImVec2> Spline;

  //for (float t = 0.0f; t <= 1.0f; t += 0.01f)
  //  Spline.push_back(BezierInterpolate(m_FirstPoint, m_SecondPoint, m_FirstControlPoint, m_SecondControlPoint, t));

  Spline.insert(Spline.end(), { m_FirstPoint, m_FirstControlPoint, m_SecondControlPoint, m_SecondPoint });

  DrawFigure(Spline, CursorPos, 0xFF00FF00, 3);

  //ImGui::GetWindowDrawList()->AddBezierCubic(
  //    CursorPos + m_FirstPoint,
  //    CursorPos + m_FirstControlPoint,
  //    CursorPos + m_SecondControlPoint,
  //    CursorPos + m_SecondPoint,
  //    0xFF00FF00, 3
  //  );

  if (ImGui::IsWindowHovered() && !m_WasMouseDown && ImGui::IsMouseDown(ImGuiMouseButton_Left))
  {
    for (auto * PointPtr : { &m_FirstPoint, &m_SecondPoint, &m_FirstControlPoint, &m_SecondControlPoint })
    {
      if (ImVecDistance(CursorPos + *PointPtr, ImGui::GetMousePos()) < 10)
      {
        m_DraggedPoint = PointPtr;
        break;
      }
    }
  }

  if (m_WasMouseDown && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    m_DraggedPoint = nullptr;

  if (m_DraggedPoint)
  {
    const auto Delta = ImGui::GetMousePos() - m_PreviousMousePosition;

    *m_DraggedPoint = *m_DraggedPoint + Delta;
  }

  ImGui::EndChild();

  m_PreviousMousePosition = ImGui::GetMousePos();
  m_WasMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
}
