#include "Application.h"

#include "MainEditWindow.h"
#include "DrawFigureWindow.h"
#include "MorphingWindow.h"

#include <exception>
#include <iostream>

int main()
{
  ImGuiVulkanGlfwApplication app;

  auto FirstFigureWindow  = std::make_shared<DrawFigureWindow>("Draw first figure", 0xFF00FF00);
  auto SecondFigureWindow = std::make_shared<DrawFigureWindow>("Draw second figure", 0xFF0000FF);

  app.AddWindow(FirstFigureWindow);
  app.AddWindow(SecondFigureWindow);
  app.AddWindow(std::make_shared<MorphingWindow>(
      "First -> second",
      FirstFigureWindow,
      SecondFigureWindow
    ));

  try
  {
    app.Run();
  }
  catch (const std::exception & ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
