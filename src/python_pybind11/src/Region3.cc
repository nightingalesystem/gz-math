/*
 * Copyright (C) 2022 Open Source Robotics Foundation
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

#include "Region3.hh"

namespace ignition
{
namespace math
{
namespace python
{
void defineMathRegion3(py::module &m, const std::string &typestr)
{
  helpDefineMathRegion3<float>(m, typestr + "f");
  helpDefineMathRegion3<double>(m, typestr + "d");
}

}  // namespace python
}  // namespace math
}  // namespace ignition