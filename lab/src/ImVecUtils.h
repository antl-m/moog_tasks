#pragma once

#include <imgui.h>
#include <cmath>
#include <random>
#include <functional>
#include <map>

inline ImVec2 operator+(ImVec2 lhs, ImVec2 rhs)
{
  return ImVec2{ lhs.x + rhs.x, lhs.y + rhs.y };
}

inline ImVec2 operator-(ImVec2 lhs, ImVec2 rhs)
{
  return ImVec2{ lhs.x - rhs.x, lhs.y - rhs.y };
}

inline float operator*(ImVec2 lhs, ImVec2 rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y;
}

inline ImVec2 operator*(ImVec2 lhs, float rhs)
{
  return ImVec2{ lhs.x * rhs, lhs.y * rhs };
}

inline ImVec2 operator*(float lhs, ImVec2 rhs)
{
  return ImVec2{ lhs * rhs.x, lhs * rhs.y };
}

inline ImVec2 operator/(ImVec2 lhs, float rhs)
{
  return ImVec2{ lhs.x / rhs, lhs.y / rhs };
}

inline float ImVecDistance(ImVec2 lhs, ImVec2 rhs)
{
  const auto d = rhs - lhs;

  return std::sqrtf(d.x * d.x + d.y * d.y);
}

inline ImVec2 LinearInterpolate(ImVec2 P1, ImVec2 P2, float t)
{
  return P1 * (1 - t) + P2 * t;
}

inline ImVec2 BezierInterpolate(ImVec2 P1, ImVec2 P2, ImVec2 V1, ImVec2 V2, float t)
{
  auto Q2 = LinearInterpolate(V1, V2, t);

  return LinearInterpolate(
             LinearInterpolate(LinearInterpolate(P1, V1, t), Q2, t),
             LinearInterpolate(Q2, LinearInterpolate(V2, P2, t), t),
             t
           );
}

inline ImVec2 CubicInterpolate(ImVec2 P1, ImVec2 P2, float t)
{

  const float t1 = t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;

  return LinearInterpolate(P1, P2, t1);
}

inline ImVec2 BackInterpolate(ImVec2 P1, ImVec2 P2, float t)
{
  static const float c1 = 1.70158;
  static const float c2 = c1 * 1.525;

  const float t1 = t < 0.5
    ? (std::pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2
    : (std::pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;

  return LinearInterpolate(P1, P2, t1);
}

inline ImVec2 ElasticInterpolate(ImVec2 P1, ImVec2 P2, float t)
{
  static const auto c5 = (2 * 3.141592) / 4.5;

  const auto t1 = t == 0 ? 0 : t == 1 ? 1 : t < 0.5
    ? -(std::pow(2, 20 * t - 10) * std::sin((20 * t - 11.125) * c5)) / 2
    : (std::pow(2, -20 * t + 10) * std::sin((20 * t - 11.125) * c5)) / 2 + 1;

  return LinearInterpolate(P1, P2, t1);
}

inline float OutBounce(float x)
{
  static const float n1 = 7.5625;
  static const float d1 = 2.75;

  if (x < 1 / d1) {
      return n1 * x * x;
  }
  else if (x < 2 / d1) {
    return n1 * (x -= 1.5 / d1) * x + 0.75;
  }
  else if (x < 2.5 / d1) {
    return n1 * (x -= 2.25 / d1) * x + 0.9375;
  }
  else {
    return n1 * (x -= 2.625 / d1) * x + 0.984375;
  }
}

inline float InOutBounce(float x)
{
  return x < 0.5
    ? (1 - OutBounce(1 - 2 * x)) / 2
    : (1 + OutBounce(2 * x - 1)) / 2;
}

inline ImVec2 OutBounceInterpolate(ImVec2 P1, ImVec2 P2, float t)
{
  return LinearInterpolate(P1, P2, OutBounce(t));
}

inline ImVec2 InOutBounceInterpolate(ImVec2 P1, ImVec2 P2, float t)
{
  return LinearInterpolate(P1, P2, InOutBounce(t));
}

inline float GetT(float t, float alpha, const ImVec2 & p0, const ImVec2 & p1)
{
  auto d = p1 - p0;
  float a = d * d;
  float b = std::pow(a, alpha * 0.5f);
  return (b + t);
}

inline ImVec2 CatmullRom(const ImVec2 & p0, const ImVec2 & p1, const ImVec2 & p2, const ImVec2 & p3, float t, float alpha = .5f)
{
  float t0 = 0.0f;
  float t1 = GetT(t0, alpha, p0, p1);
  float t2 = GetT(t1, alpha, p1, p2);
  float t3 = GetT(t2, alpha, p2, p3);
  t = std::lerp(t1, t2, t);
  ImVec2 A1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
  ImVec2 A2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
  ImVec2 A3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;
  ImVec2 B1 = (t2 - t) / (t2 - t0) * A1 + (t - t0) / (t2 - t0) * A2;
  ImVec2 B2 = (t3 - t) / (t3 - t1) * A2 + (t - t1) / (t3 - t1) * A3;
  ImVec2 C = (t2 - t) / (t2 - t1) * B1 + (t - t1) / (t2 - t1) * B2;
  return C;
}

inline std::vector<ImVec2> GetSpline(
    const std::vector<ImVec2> & points,
    const std::size_t           num_points
  )
{
  if (points.size() < 3)
    return points;

  std::vector<ImVec2> Result;
  const float delta = 1.0f / (num_points + 1);
  const std::size_t siz = points.size();

  for (float t = 0; t <= 1.0f; t += delta)
    Result.push_back(CatmullRom(2 * points[0] - points[1], points[0], points[1], points[2], t));

  for (std::size_t i = 1; i < siz - 2; ++i)
    for (float t = 0; t <= 1.0f; t += delta)
      Result.push_back(CatmullRom(points[i - 1], points[i], points[i + 1], points[i + 2], t));


  for (float t = 0; t <= 1.0f; t += delta)
    Result.push_back(CatmullRom(points[siz - 3], points[siz - 2], points[siz - 1], 2 * points[siz - 1] - points[siz - 2], t));

  return Result;
}

inline void DrawFigure(
    const std::vector<ImVec2> & points,
    const ImVec2 pos,
    const ImU32 col = 0xFFFFFFFF,
    const float thickness = 1
  )
{
  if (points.size() < 2)
    return;

  const auto Spline = GetSpline(points, 10);

  for (int i = 0; i < Spline.size() - 1; ++i)
  {
    int next = (i == Spline.size() - 1 ? 0 : i + 1);

    ImGui::GetWindowDrawList()->AddLine(
      pos + Spline[i],
      pos + Spline[i + 1],
      col, thickness
    );
  }
}

inline std::pair<std::vector<ImVec2>, std::vector<ImVec2>> FillMissingPoints(
    const std::vector<ImVec2> & first,
    const std::vector<ImVec2> & second
  )
{
  if (first.size() == second.size() || first.size() == 0 || second.size() == 0)
    return { first, second };

  const auto max_count = std::max(first.size(), second.size());

  std::vector<ImVec2> copy = (first.size() < second.size() ? first : second);

  std::default_random_engine eng;

  while (copy.size() < max_count)
  {
    std::uniform_int_distribution<int> distr(0, copy.size() - 2);
    const int pos = distr(eng);

    if (pos == 0)
      copy.insert(copy.begin() + pos + 1, CatmullRom(2 * copy[0] - copy[1], copy[0], copy[1], copy[2], 0.5));
    else
    if (pos + 2 == copy.size())
      copy.insert(copy.begin() + pos + 1, CatmullRom(copy[pos - 2], copy[pos - 1], copy[pos], 2 * copy[pos + 1] - copy[pos], 0.5));
    else
      copy.insert(copy.begin() + pos + 1, CatmullRom(copy[pos - 1], copy[pos], copy[pos + 1], copy[pos + 2], 0.5));
  }

  if (first.size() < second.size())
    return { std::move(copy), second };

  return { first, std::move(copy) };
}

inline std::vector<ImVec2> Morph(
    const std::vector<ImVec2> &                          _First,
    const std::vector<ImVec2> &                          _Second,
    const float                                          _Time,
    const std::function<ImVec2(ImVec2, ImVec2, float)> & _InterpolateFunc
  )
{
  if (_First.size() < 2 || _Second.size() < 2)
    return {};

  if (_First.size() != _Second.size())
    return {};

  std::vector<ImVec2> Result;
  Result.reserve(_First.size());

  for (int i = 0; i < _First.size(); ++i)
    Result.push_back(_InterpolateFunc(_First[i], _Second[i], _Time));

  return Result;
}