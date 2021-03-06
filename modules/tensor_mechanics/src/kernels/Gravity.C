//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "Gravity.h"
#include "Function.h"

registerMooseObject("TensorMechanicsApp", Gravity);

InputParameters
Gravity::validParams()
{
  InputParameters params = Kernel::validParams();
  params.addClassDescription("Apply gravity. Value is in units of acceleration.");
  params.addParam<bool>("use_displaced_mesh", true, "Displaced mesh defaults to true");
  params.addRequiredParam<Real>(
      "value", "Value multiplied against the residual, e.g. gravitational acceleration");
  params.addParam<FunctionName>(
      "function", "1", "A function that describes the gravitational force");
  params.addParam<Real>("alpha", 0.0, "alpha parameter required for HHT time integration scheme");
  return params;
}

Gravity::Gravity(const InputParameters & parameters)
  : Kernel(parameters),
    _density(getMaterialProperty<Real>("density")),
    _value(getParam<Real>("value")),
    _function(getFunction("function")),
    _alpha(getParam<Real>("alpha"))
{
}

Real
Gravity::computeQpResidual()
{
  Real factor = _value * _function.value(_t + _alpha * _dt, _q_point[_qp]);
  return _density[_qp] * _test[_i][_qp] * -factor;
}
