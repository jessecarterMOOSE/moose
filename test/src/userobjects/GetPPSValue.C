/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

#include "GetPPSValue.h"

template <>
InputParameters
validParams<GetPPSValue>()
{
  InputParameters params = validParams<GeneralUserObject>();
  params.addParam<PostprocessorName>("pps_1", -1, "Name of the postprocessor to use.");
  params.addParam<PostprocessorName>("pps_2", -1, "Name of the postprocessor to use.");
  return params;
}

GetPPSValue::GetPPSValue(const InputParameters & params)
  : GeneralUserObject(params),
    _pps_value_1(getPostprocessorValue("pps_1")),
    _pps_value_2(getPostprocessorValue("pps_2"))
{
}

void
GetPPSValue::execute()
{
  _console << std::endl;
  _console << "value 1: " << _pps_value_1 << std::endl;
  _console << "value 2: " << _pps_value_2 << std::endl;
  _console << std::endl;
}
