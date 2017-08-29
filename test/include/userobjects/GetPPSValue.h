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

#ifndef GETPPSVALUE_H
#define GETPPSVALUE_H

#include "GeneralUserObject.h"

class GetPPSValue;

template <>
InputParameters validParams<GetPPSValue>();

class GetPPSValue : public GeneralUserObject
{
public:
  GetPPSValue(const InputParameters & params);

  virtual void initialize(){};
  virtual void execute();
  virtual void finalize(){};

protected:
  const PostprocessorValue & _pps_value_1;
  const PostprocessorValue & _pps_value_2;
};

#endif /* GETPPSVALUE_H */
