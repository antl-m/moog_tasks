#include "MainEditWindow.h"

#include <imgui.h>
#include <cmath>

std::string MainEditWindow::GetWindowName() const
{
  return "Edit";
}

void MainEditWindow::UpdateFrameData()
{
  ImGui::Button("First");
  ImGui::Button("Second");
  ImGui::Button("Third");
  ImGui::Button("Fourth");

  auto size = ImVec2(200, 200);

  ImVec2 p1 = ImGui::GetCursorScreenPos();
  ImVec2 p2 = ImVec2(p1.x + size.x, p1.y + size.y);

  ImGui::BeginChild("##Invisible", size);

  ImGui::GetWindowDrawList()->AddLine(p1, p2, IM_COL32(255, 0, 255, 255));
  ImGui::GetWindowDrawList()->AddCircleFilled(p1, 6.0f, IM_COL32(255, 0, 255, 255));
  ImGui::GetWindowDrawList()->AddCircleFilled(p2, 6.0f, IM_COL32(255, 0, 255, 255));

  const auto mouse_pos = ImGui::GetMousePos();

  if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    ImGui::GetWindowDrawList()->AddCircleFilled(mouse_pos, 10, 0xFFFFFFFF);

  ImGui::EndChild();
}