//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "NodalPPSCoefForce.h"

registerMooseObject("MooseTestApp", NodalPPSCoefForce);

InputParameters
NodalPPSCoefForce::validParams()
{
  InputParameters params = NodalKernel::validParams();
  params.addClassDescription("Adds force to node(s) based on Postprocessor value.");
  params.addRequiredParam<PostprocessorName>("postprocessor", "The postprocessor which provides the force.");
  params.addParam<Real>("coef", 1.0, "Coefficent multiplier for the coupled force term.");

  return params;
}

NodalPPSCoefForce::NodalPPSCoefForce(const InputParameters & parameters)
  : NodalKernel(parameters),
    _pp_value(getPostprocessorValue("postprocessor")),
    _coef(getParam<Real>("coef"))
{
}

Real
NodalPPSCoefForce::computeQpResidual()
{
  return _coef * _pp_value;
}

Real
NodalPPSCoefForce::computeQpJacobian()
{
  // I doubt this is correct
  return _coef;
}
