//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// MOOSE includes
#include "LevelSetAdvection.h"

registerADMooseObject("LevelSetApp", LevelSetAdvection);

template <ComputeStage compute_stage>
InputParameters
LevelSetAdvection<compute_stage>::validParams()
{
  InputParameters params = ADKernelValue<compute_stage>::validParams();
  params.addClassDescription("Implements the level set advection equation: $\\vec{v}\\cdot\\nabla "
                             "u = 0$, where the weak form is $(\\psi_i, \\vec{v}\\cdot\\nabla u) = "
                             "0$.");
  params += LevelSetVelocityInterface<ADKernelValue<compute_stage>>::validParams();
  return params;
}

template <ComputeStage compute_stage>
LevelSetAdvection<compute_stage>::LevelSetAdvection(const InputParameters & parameters)
  : LevelSetVelocityInterface<ADKernelValue<compute_stage>>(parameters)
{
}

template <ComputeStage compute_stage>
ADReal
LevelSetAdvection<compute_stage>::precomputeQpResidual()
{
  computeQpVelocity();
  return _velocity * _grad_u[_qp];
}
