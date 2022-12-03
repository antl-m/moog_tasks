#pragma once

#include "IWindow.h"

class MainEditWindow :
  public IWindow
{
protected: // IWindow

  std::string GetWindowName() const override;

  void UpdateFrameData() override;

};

