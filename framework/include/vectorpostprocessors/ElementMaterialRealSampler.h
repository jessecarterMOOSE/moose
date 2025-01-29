//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "ElementMaterialSamplerBase.h"

/// This postprocessor records all scalar material properties of the specified
/// material object on specified elements at the indicated execution points
/// (e.g. initial, timestep_begin, etc.).  Non-scalar properties are ignored
/// with a warning.
class ElementMaterialRealSampler : public ElementMaterialSamplerBase
{
public:
  static InputParameters validParams();

  ElementMaterialRealSampler(const InputParameters & parameters);

  virtual void execute() override;
};
