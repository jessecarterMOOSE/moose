//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ElementMaterialRealSampler.h"

registerMooseObject("MooseApp", ElementMaterialRealSampler);
registerMooseObjectRenamed("MooseApp",
                           MaterialVectorPostprocessor,
                           "06/30/2025 24:00",
                           ElementMaterialRealSampler);

InputParameters
ElementMaterialRealSampler::validParams()
{
  InputParameters params = ElementMaterialSamplerBase::validParams();
  params.addClassDescription("Records all Real-valued material properties of a material object, "
                             "or Real-valued material properties of the supplied property names "
                             "on quadrature points on elements at the indicated execution points.");
  return params;
}

ElementMaterialRealSampler::ElementMaterialRealSampler(const InputParameters & parameters)
  : ElementMaterialSamplerBase(parameters)
{
  // Check properties are valid and store references
  for (auto & prop : _prop_names)
  {
    if (hasMaterialProperty<Real>(prop))
      _prop_refs.push_back(&getMaterialProperty<Real>(prop));
    else if (hasMaterialProperty<unsigned int>(prop))
      _prop_refs.push_back(&getMaterialProperty<unsigned int>(prop));
    else if (hasMaterialProperty<int>(prop))
      _prop_refs.push_back(&getMaterialProperty<int>(prop));
    else
    {
      mooseWarning("property " + prop +
                   " is of unsupported type and skipped by ElementMaterialRealSampler");
      continue;
    }
    _prop_vecs.push_back(&declareVector(prop));
  }
}

void
ElementMaterialRealSampler::execute()
{
  // call base class to handle qp data
  ElementMaterialSamplerBase::execute();

  unsigned int nqp = _qrule->n_points();
  for (unsigned int i = 0; i < _prop_names.size(); i++)
  {
    auto prop_name = _prop_names[i];
    auto prop = _prop_vecs[i];
    std::vector<Real> vals;
    if (hasMaterialProperty<Real>(prop_name))
    {
      auto vals = dynamic_cast<const MaterialProperty<Real> *>(_prop_refs[i]);
      for (unsigned int qp = 0; qp < nqp; qp++)
        prop->push_back((*vals)[qp]);
    }
    else if (hasMaterialProperty<unsigned int>(prop_name))
    {
      auto vals = dynamic_cast<const MaterialProperty<unsigned int> *>(_prop_refs[i]);
      for (unsigned int qp = 0; qp < nqp; qp++)
        prop->push_back((*vals)[qp]);
    }
    else if (hasMaterialProperty<int>(prop_name))
    {
      auto vals = dynamic_cast<const MaterialProperty<int> *>(_prop_refs[i]);
      for (unsigned int qp = 0; qp < nqp; qp++)
        prop->push_back((*vals)[qp]);
    }
  }
}