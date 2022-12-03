#pragma once

#include <string>

class IWindow
{
public: // Construction / Destruction

  IWindow();

  virtual ~IWindow() = default;

public: // Interface

  void Show();

  std::string GetWindowNameID() const;

protected: // Service

  virtual std::string GetWindowName() const = 0;

  virtual void UpdateFrameData() = 0;

private:

  std::string m_ThisStr;
};

