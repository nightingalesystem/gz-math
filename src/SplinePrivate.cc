/*
 * Copyright (C) 2017 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "ignition/math/Matrix4.hh"

#include "ignition/math/SplinePrivate.hh"

namespace ignition
{
namespace math
{

///////////////////////////////////////////////////////////
Vector4d PolynomialPowers(const unsigned int _order,
                          const double _t)
{
  // It is much faster to go over this table than
  // delving into factorials and power computations.
  double t2 = _t * _t;
  double t3 = t2 * _t;
  switch (_order) {
    case 0:
      return Vector4d(t3, t2, _t, 1.0);
    case 1:
      return Vector4d(3*t2, 2*_t, 1.0, 0.0);
    case 2:
      return Vector4d(6*_t, 2.0, 0.0, 0.0);
    case 3:
      return Vector4d(6.0, 0.0, 0.0, 0.0);
    default:
      return Vector4d(0.0, 0.0, 0.0, 0.0);
  }
}

///////////////////////////////////////////////////////////
void ComputeCubicBernsteinHermiteCoeff(const ControlPoint &_startPoint,
                                       const ControlPoint &_endPoint,
                                       Matrix4d &_coeffs)
{
  // Get values and tangents
  const Vector3d &point0 = _startPoint.MthDerivative(0);
  const Vector3d &point1 = _endPoint.MthDerivative(0);
  const Vector3d &tan0 = _startPoint.MthDerivative(1);
  const Vector3d &tan1 = _endPoint.MthDerivative(1);

  // Hermite basis matrix
  const Matrix4d bmatrix(2.0, -2.0,  1.0,  1.0,
                        -3.0,  3.0, -2.0, -1.0,
                         0.0,  0.0,  1.0,  0.0,
                         1.0,  0.0,  0.0,  0.0);

  // Control vectors matrix
  Matrix4d cmatrix(point0.X(), point0.Y(), point0.Z(), 1.0,
                   point1.X(), point1.Y(), point1.Z(), 1.0,
                   tan0.X(),   tan0.Y(),   tan0.Z(),   1.0,
                   tan1.X(),   tan1.Y(),   tan1.Z(),   1.0);

  // Compute coefficients
  _coeffs = bmatrix * cmatrix;
}

///////////////////////////////////////////////////////////
IntervalCubicSpline::IntervalCubicSpline()
    : startPoint({Vector3d::Zero, Vector3d::Zero}),
      endPoint({Vector3d::Zero, Vector3d::Zero}),
      coeffs(Matrix4d::Zero),
      arcLength(0.0)
{
}

///////////////////////////////////////////////////////////
void IntervalCubicSpline::SetPoints(const ControlPoint &_startPoint,
                                    const ControlPoint &_endPoint)
{
  this->startPoint = _startPoint;
  this->endPoint = _endPoint;

  ComputeCubicBernsteinHermiteCoeff(
      this->startPoint, this->endPoint, this->coeffs);

  this->startPoint.MthDerivative(2) = this->DoInterpolateMthDerivative(2, 0.0);
  this->startPoint.MthDerivative(3) = this->DoInterpolateMthDerivative(3, 0.0);
  this->endPoint.MthDerivative(2) = this->DoInterpolateMthDerivative(2, 1.0);
  this->endPoint.MthDerivative(3) = this->DoInterpolateMthDerivative(3, 1.0);
  this->arcLength = this->ArcLength(1.0);
}

///////////////////////////////////////////////////////////
double IntervalCubicSpline::ArcLength(const double _t) const
{
  // Bound check
  if (_t < 0.0 || _t > 1.0)
    return INF_D;

  // 5 Point Gauss-Legendre quadrature rule for numerical path integration
  // TODO: generalize into a numerical integration toolkit ?
  double w1 = 0.28444444444444444 * _t;
  double w23 = 0.23931433524968326 * _t;
  double w45 = 0.11846344252809456 * _t;
  double x1 = 0.5 * _t;
  double x2 = 0.23076534494715845 * _t;
  double x3 = 0.7692346550528415 * _t;
  double x4 = 0.0469100770306680 * _t;
  double x5 = 0.9530899229693319 * _t;

  double arc_length = w1 * this->InterpolateMthDerivative(1, x1).Length();
  arc_length += w23 * this->InterpolateMthDerivative(1, x2).Length();
  arc_length += w23 * this->InterpolateMthDerivative(1, x3).Length();
  arc_length += w45 * this->InterpolateMthDerivative(1, x4).Length();
  arc_length += w45 * this->InterpolateMthDerivative(1, x5).Length();
  return arc_length;
}

///////////////////////////////////////////////////////////
bool IntervalCubicSpline::HasLoop() const
{
  // Bezier Bernstein polynomial basis.
  const Matrix4d bmatrix(-1.0, 3.0, -3.0, 1.0,
                         3.0, -6.0, 3.0, 0.0,
                         -3.0, 3.0, 0.0, 0.0,
                         1.0, 0.0, 0.0, 0.0);

  // Recover Bezier representation, whose control points
  // and convex hull is defined as follows:
  //
  //     p2 o--------o p3
  //       /          \
  //    b /            \  c
  //     /      a       \
  // p1 o----------------o p4

  const Matrix4d pmatrix = bmatrix.Inverse() * this->coeffs;

  const Vector3d p1(pmatrix(0, 0),
                    pmatrix(0, 1),
                    pmatrix(0, 2));
  const Vector3d p2(pmatrix(1, 0),
                    pmatrix(1, 1),
                    pmatrix(1, 2));
  const Vector3d p3(pmatrix(2, 0),
                    pmatrix(2, 1),
                    pmatrix(2, 2));
  const Vector3d p4(pmatrix(3, 0),
                    pmatrix(3, 1),
                    pmatrix(3, 2));

  const Vector3d a = p4 - p1;
  const Vector3d b = p2 - p1;
  const Vector3d c = p3 - p4;

  const Vector3d axc = a.Cross(c);
  const Vector3d bxc = b.Cross(c);
  const Vector3d axb = a.Cross(b);

  if (bxc == Vector3d::Zero) {
    if (axb != Vector3d::Zero) {
      // Parallel tangents case.
      return false;
    }
    // All collinear points case. If inner
    // control points go past each other
    // loops will ensue.
    const Vector3d d = p3 - p1;
    return d.Length() < b.Length();
  }

  if (!equal<double>(a.Dot(bxc), 0.0)) {
    // TODO: handle non coplanar cases.
    return true;
  }

  bool has_loop = false;

  if (axc != Vector3d::Zero) {
    // The second control point tangent is not collinear
    // with the line that passes through both control points,
    // so intersection with the first control point tangent
    // projection is not at the latter origin.

    // If scale factor is less than 1, the first control
    // point tangent extends beyond the intersection, and
    // thus loops are likely to happen (this IS NOT a necessary
    // condition, but a sufficient one).
    has_loop |= (std::abs(axc.Dot(bxc) / bxc.SquaredLength()) < 1.0);
  }

  if (axb != Vector3d::Zero) {
    // The first control point tangent is not collinear
    // with the line that passes through both control points,
    // so intersection with the second control point tangent
    // projection is not at the latter origin.

    // If scale factor is less than 1, the second control
    // point tangent extends beyond the intersection, and
    // thus loops are likely to happen (this IS NOT a necessary
    // condition, but a sufficient one).
    has_loop |= (std::abs(axb.Dot(bxc) / bxc.SquaredLength()) < 1.0);
  }

  return has_loop;
}

///////////////////////////////////////////////////////////
Vector3d IntervalCubicSpline::DoInterpolateMthDerivative(
    const unsigned int _mth, const double _t) const
{
  Vector4d powers = PolynomialPowers(_mth, _t);
  Vector4d interpolation = powers * this->coeffs;
  return Vector3d(interpolation.X(), interpolation.Y(), interpolation.Z());
}

///////////////////////////////////////////////////////////
Vector3d IntervalCubicSpline::InterpolateMthDerivative(
    const unsigned int _mth, const double _t) const
{
  // Bound check
  if (_t < 0.0 || _t > 1.0)
    return Vector3d(INF_D, INF_D, INF_D);

  if (equal(_t, 0.0))
    return this->startPoint.MthDerivative(_mth);
  else if (equal(_t, 1.0))
    return this->endPoint.MthDerivative(_mth);

  return this->DoInterpolateMthDerivative(_mth, _t);
}



}
}
