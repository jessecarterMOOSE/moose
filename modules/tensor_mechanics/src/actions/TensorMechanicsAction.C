//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "Conversion.h"
#include "FEProblem.h"
#include "Factory.h"
#include "MooseMesh.h"
#include "MooseObjectAction.h"
#include "TensorMechanicsAction.h"

#include "libmesh/string_to_enum.h"
#include <algorithm>

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "meta_action");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "setup_mesh_complete");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "validate_coordinate_systems");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "add_variable");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "add_aux_variable");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "add_kernel");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "add_aux_kernel");

registerMooseAction("TensorMechanicsApp", TensorMechanicsAction, "add_material");

InputParameters
TensorMechanicsAction::validParams()
{
  InputParameters params = TensorMechanicsActionBase::validParams();
  params.addClassDescription("Set up stress divergence kernels with coordinate system aware logic");

  // parameters specified here only appear in the input file sub-blocks of the
  // Master action, not in the common parameters area
  params.addParam<std::vector<SubdomainName>>("block",
                                              "The list of ids of the blocks (subdomain) "
                                              "that the stress divergence kernels will be "
                                              "applied to");
  params.addParamNamesToGroup("block", "Advanced");

  params.addParam<MultiMooseEnum>("additional_generate_output",
                                  TensorMechanicsActionBase::outputPropertiesType(),
                                  "Add scalar quantity output for stress and/or strain (will be "
                                  "appended to the list in `generate_output`)");
  params.addParamNamesToGroup("additional_generate_output", "Output");
  params.addParam<std::string>(
      "strain_base_name",
      "The base name used for the strain. If not provided, it will be set equal to base_name");
  params.addParam<std::vector<TagName>>(
      "extra_vector_tags",
      "The tag names for extra vectors that residual data should be saved into");
  params.addParam<Real>("scaling", "The scaling to apply to the displacement variables");

  return params;
}

TensorMechanicsAction::TensorMechanicsAction(const InputParameters & params)
  : TensorMechanicsActionBase(params),
    _displacements(getParam<std::vector<VariableName>>("displacements")),
    _ndisp(_displacements.size()),
    _coupled_displacements(_ndisp),
    _save_in(getParam<std::vector<AuxVariableName>>("save_in")),
    _diag_save_in(getParam<std::vector<AuxVariableName>>("diag_save_in")),
    _subdomain_names(getParam<std::vector<SubdomainName>>("block")),
    _subdomain_ids(),
    _strain(getParam<MooseEnum>("strain").getEnum<Strain>()),
    _planar_formulation(getParam<MooseEnum>("planar_formulation").getEnum<PlanarFormulation>()),
    _out_of_plane_direction(
        getParam<MooseEnum>("out_of_plane_direction").getEnum<OutOfPlaneDirection>()),
    _base_name(isParamValid("base_name") ? getParam<std::string>("base_name") + "_" : "")
{
  // determine if incremental strains are to be used
  if (isParamValid("incremental"))
  {
    const bool incremental = getParam<bool>("incremental");
    if (!incremental && _strain == Strain::Small)
      _strain_and_increment = StrainAndIncrement::SmallTotal;
    else if (!incremental && _strain == Strain::Finite)
      _strain_and_increment = StrainAndIncrement::FiniteTotal;
    else if (incremental && _strain == Strain::Small)
      _strain_and_increment = StrainAndIncrement::SmallIncremental;
    else if (incremental && _strain == Strain::Finite)
      _strain_and_increment = StrainAndIncrement::FiniteIncremental;
    else
      mooseError("Internal error");
  }
  else
  {
    if (_strain == Strain::Small)
      _strain_and_increment = StrainAndIncrement::SmallTotal;
    else if (_strain == Strain::Finite)
      _strain_and_increment = StrainAndIncrement::FiniteIncremental;
    else
      mooseError("Internal error");
  }

  // determine if displaced mesh is to be used
  _use_displaced_mesh = (_strain == Strain::Finite);
  if (params.isParamSetByUser("use_displaced_mesh"))
  {
    bool use_displaced_mesh_param = getParam<bool>("use_displaced_mesh");
    if (use_displaced_mesh_param != _use_displaced_mesh && params.isParamSetByUser("strain"))
      mooseError("Wrong combination of use displaced mesh and strain model");
    _use_displaced_mesh = use_displaced_mesh_param;
  }

  // convert vector of VariableName to vector of VariableName
  for (unsigned int i = 0; i < _ndisp; ++i)
    _coupled_displacements[i] = _displacements[i];

  if (_save_in.size() != 0 && _save_in.size() != _ndisp)
    mooseError("Number of save_in variables should equal to the number of displacement variables ",
               _ndisp);

  if (_diag_save_in.size() != 0 && _diag_save_in.size() != _ndisp)
    mooseError(
        "Number of diag_save_in variables should equal to the number of displacement variables ",
        _ndisp);

  // plane strain consistency check
  if (_planar_formulation != PlanarFormulation::None)
  {
    if (params.isParamSetByUser("out_of_plane_strain") &&
        _planar_formulation != PlanarFormulation::WeakPlaneStress)
      mooseError(
          "out_of_plane_strain should only be specified with planar_formulation=WEAK_PLANE_STRESS");
    else if (!params.isParamSetByUser("out_of_plane_strain") &&
             _planar_formulation == PlanarFormulation::WeakPlaneStress)
      mooseError("out_of_plane_strain must be specified with planar_formulation=WEAK_PLANE_STRESS");
  }

  // convert output variable names to lower case
  for (const auto & out : getParam<MultiMooseEnum>("generate_output"))
  {
    std::string lower(out);
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    _generate_output.push_back(lower);
  }

  // Error if volumetric locking correction is true for 1D problems
  if (_ndisp == 1 && getParam<bool>("volumetric_locking_correction"))
    mooseError("Volumetric locking correction should be set to false for 1D problems.");

  if (!getParam<bool>("add_variables") && params.isParamSetByUser("scaling"))
    paramError("scaling",
               "The scaling parameter has no effect unless add_variables is set to true. Did you "
               "mean to set 'add_variables = true'?");
}

void
TensorMechanicsAction::act()
{
  std::string ad_prepend = "";
  std::string ad_append = "";
  if (_use_ad)
  {
    ad_prepend = "AD";
    ad_append = "<RESIDUAL>";
  }

  //
  // Consistency checks across subdomains
  //
  actSubdomainChecks();

  //
  // Gather info from all other TensorMechanicsAction
  //
  actGatherActionParameters();

  //
  // Deal with the optional AuxVariable based tensor quantity output
  //
  actOutputGeneration();

  //
  // Meta action which optionally spawns other actions
  //
  if (_current_task == "meta_action")
  {
    if (_planar_formulation == PlanarFormulation::GeneralizedPlaneStrain)
    {
      if (_use_ad)
        paramError("use_automatic_differentiation", "AD not setup for use with PlaneStrain");
      // Set the action parameters
      const std::string type = "GeneralizedPlaneStrainAction";
      auto action_params = _action_factory.getValidParams(type);
      action_params.set<bool>("_built_by_moose") = true;
      action_params.set<std::string>("registered_identifier") = "(AutoBuilt)";
      action_params.applyParameters(parameters(), {"use_displaced_mesh"});
      action_params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;
      if (isParamValid("pressure_factor"))
        action_params.set<Real>("factor") = getParam<Real>("pressure_factor");
      // Create and add the action to the warehouse
      auto action = MooseSharedNamespace::static_pointer_cast<MooseObjectAction>(
          _action_factory.create(type, name() + "_gps", action_params));
      _awh.addActionBlock(action);
      if (isParamValid("extra_vector_tags"))
        action_params.set<std::vector<TagName>>("extra_vector_tags") =
            getParam<std::vector<TagName>>("extra_vector_tags");
    }
  }

  //
  // Add variables (optional)
  //
  else if (_current_task == "add_variable" && getParam<bool>("add_variables"))
  {
    auto params = _factory.getValidParams("MooseVariable");
    // determine necessary order
    const bool second = _problem->mesh().hasSecondOrderElements();

    params.set<MooseEnum>("order") = second ? "SECOND" : "FIRST";
    params.set<MooseEnum>("family") = "LAGRANGE";
    if (isParamValid("scaling"))
      params.set<std::vector<Real>>("scaling") = {getParam<Real>("scaling")};

    // Loop through the displacement variables
    for (const auto & disp : _displacements)
    {
      // Create displacement variables
      _problem->addVariable("MooseVariable", disp, params);
    }
  }

  //
  // Add Strain Materials
  //
  else if (_current_task == "add_material")
  {
    std::string type;

    //
    // no plane strain
    //
    if (_planar_formulation == PlanarFormulation::None)
    {
      std::map<std::pair<Moose::CoordinateSystemType, StrainAndIncrement>, std::string> type_map = {
          {{Moose::COORD_XYZ, StrainAndIncrement::SmallTotal}, "ComputeSmallStrain"},
          {{Moose::COORD_XYZ, StrainAndIncrement::SmallIncremental},
           "ComputeIncrementalSmallStrain"},
          {{Moose::COORD_XYZ, StrainAndIncrement::FiniteIncremental}, "ComputeFiniteStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::SmallTotal}, "ComputeAxisymmetricRZSmallStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::SmallIncremental},
           "ComputeAxisymmetricRZIncrementalStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::FiniteIncremental},
           "ComputeAxisymmetricRZFiniteStrain"},
          {{Moose::COORD_RSPHERICAL, StrainAndIncrement::SmallTotal},
           "ComputeRSphericalSmallStrain"},
          {{Moose::COORD_RSPHERICAL, StrainAndIncrement::SmallIncremental},
           "ComputeRSphericalIncrementalStrain"},
          {{Moose::COORD_RSPHERICAL, StrainAndIncrement::FiniteIncremental},
           "ComputeRSphericalFiniteStrain"}};

      auto type_it = type_map.find(std::make_pair(_coord_system, _strain_and_increment));
      if (type_it != type_map.end())
        type = type_it->second;
      else
        mooseError("Unsupported strain formulation");
    }
    else if (_planar_formulation == PlanarFormulation::WeakPlaneStress ||
             _planar_formulation == PlanarFormulation::PlaneStrain ||
             _planar_formulation == PlanarFormulation::GeneralizedPlaneStrain)
    {
      if (_use_ad && (_planar_formulation == PlanarFormulation::PlaneStrain ||
                      _planar_formulation == PlanarFormulation::GeneralizedPlaneStrain))
        paramError("use_automatic_differentiation",
                   "AD not setup for use with PlaneStrain or GeneralizedPlaneStrain");

      std::map<std::pair<Moose::CoordinateSystemType, StrainAndIncrement>, std::string> type_map = {
          {{Moose::COORD_XYZ, StrainAndIncrement::SmallTotal}, "ComputePlaneSmallStrain"},
          {{Moose::COORD_XYZ, StrainAndIncrement::SmallIncremental},
           "ComputePlaneIncrementalStrain"},
          {{Moose::COORD_XYZ, StrainAndIncrement::FiniteIncremental}, "ComputePlaneFiniteStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::SmallTotal}, "ComputeAxisymmetric1DSmallStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::SmallIncremental},
           "ComputeAxisymmetric1DIncrementalStrain"},
          {{Moose::COORD_RZ, StrainAndIncrement::FiniteIncremental},
           "ComputeAxisymmetric1DFiniteStrain"}};

      // choose kernel type based on coordinate system
      auto type_it = type_map.find(std::make_pair(_coord_system, _strain_and_increment));
      if (type_it != type_map.end())
        type = type_it->second;
      else
        mooseError("Unsupported coordinate system for plane strain.");
    }
    else
      mooseError("Unsupported planar formulation");

    // set material parameters
    auto params = _factory.getValidParams(ad_prepend + type + ad_append);
    params.applyParameters(parameters(),
                           {"displacements",
                            "use_displaced_mesh",
                            "out_of_plane_strain",
                            "scalar_out_of_plane_strain"});

    if (isParamValid("strain_base_name"))
      params.set<std::string>("base_name") = getParam<std::string>("strain_base_name");

    params.set<std::vector<VariableName>>("displacements") = _coupled_displacements;
    params.set<bool>("use_displaced_mesh") = false;

    if (isParamValid("scalar_out_of_plane_strain"))
      params.set<std::vector<VariableName>>("scalar_out_of_plane_strain") = {
          getParam<VariableName>("scalar_out_of_plane_strain")};

    if (isParamValid("out_of_plane_strain"))
      params.set<std::vector<VariableName>>("out_of_plane_strain") = {
          getParam<VariableName>("out_of_plane_strain")};

    if (_use_ad)
    {
      _problem->addADResidualMaterial(
          ad_prepend + type + "<RESIDUAL>", name() + "_strain" + "_residual", params);
      _problem->addADJacobianMaterial(
          ad_prepend + type + "<JACOBIAN>", name() + "_strain" + "_jacobian", params);
      _problem->haveADObjects(true);
    }
    else
      _problem->addMaterial(type, name() + "_strain", params);
  }

  //
  // Add Stress Divergence (and optionally WeakPlaneStress) Kernels
  //
  else if (_current_task == "add_kernel")
  {
    for (unsigned int i = 0; i < _ndisp; ++i)
    {
      auto tensor_kernel_type = getKernelType();
      auto params = getKernelParameters(ad_prepend + tensor_kernel_type + ad_append);

      std::string kernel_name = "TM_" + name() + Moose::stringify(i);

      // Set appropriate components for kernels, including in the cases where a planar model is
      // running in planes other than the x-y plane (defined by _out_of_plane_strain_direction).
      if (_out_of_plane_direction == OutOfPlaneDirection::x && i == 0)
        continue;
      else if (_out_of_plane_direction == OutOfPlaneDirection::y && i == 1)
        continue;

      params.set<unsigned int>("component") = i;

      params.set<NonlinearVariableName>("variable") = _displacements[i];

      if (_save_in.size() == _ndisp)
        params.set<std::vector<AuxVariableName>>("save_in") = {_save_in[i]};
      if (_diag_save_in.size() == _ndisp)
        params.set<std::vector<AuxVariableName>>("diag_save_in") = {_diag_save_in[i]};
      if (isParamValid("out_of_plane_strain"))
        params.set<std::vector<VariableName>>("out_of_plane_strain") = {
            getParam<VariableName>("out_of_plane_strain")};

      if (_use_ad)
      {
        _problem->addKernel(
            ad_prepend + tensor_kernel_type + "<RESIDUAL>", kernel_name + "_residual", params);
        _problem->addKernel(
            ad_prepend + tensor_kernel_type + "<JACOBIAN>", kernel_name + "_jacobian", params);
        _problem->haveADObjects(true);
      }
      else
        _problem->addKernel(tensor_kernel_type, kernel_name, params);
    }

    if (_planar_formulation == PlanarFormulation::WeakPlaneStress)
    {
      auto params = getKernelParameters(ad_prepend + "WeakPlaneStress" + ad_append);
      std::string wps_kernel_name = "TM_WPS_" + name();
      params.set<NonlinearVariableName>("variable") = getParam<VariableName>("out_of_plane_strain");

      if (_use_ad)
      {
        _problem->addKernel(
            ad_prepend + "WeakPlaneStress" + "<RESIDUAL>", wps_kernel_name + "_residual", params);
        _problem->addKernel(
            ad_prepend + "WeakPlaneStress" + "<JACOBIAN>", wps_kernel_name + "_jacobian", params);
        _problem->haveADObjects(true);
      }
      else
        _problem->addKernel("WeakPlaneStress", wps_kernel_name, params);
    }
  }
}

void
TensorMechanicsAction::actSubdomainChecks()
{
  //
  // Do the coordinate system check only once the problem is created
  //
  if (_current_task == "setup_mesh_complete")
  {
    // get subdomain IDs
    for (auto & name : _subdomain_names)
      _subdomain_ids.insert(_mesh->getSubdomainID(name));
  }

  if (_current_task == "validate_coordinate_systems")
  {
    // use either block restriction list or list of all subdomains in the mesh
    const auto & check_subdomains =
        _subdomain_ids.empty() ? _problem->mesh().meshSubdomains() : _subdomain_ids;
    if (check_subdomains.empty())
      mooseError("No subdomains found");

    // make sure all subdomains are using the same coordinate system
    _coord_system = _problem->getCoordSystem(*check_subdomains.begin());
    for (auto subdomain : check_subdomains)
      if (_problem->getCoordSystem(subdomain) != _coord_system)
        mooseError("The TensorMechanics action requires all subdomains to have the same coordinate "
                   "system.");

    if (_coord_system == Moose::COORD_RZ)
    {
      if (_out_of_plane_direction != OutOfPlaneDirection::z)
        mooseError("'out_of_plane_direction' must be 'z' for axisymmetric simulations");
    }
    else if (_planar_formulation != PlanarFormulation::None)
    {
      if (_out_of_plane_direction == OutOfPlaneDirection::z && _ndisp != 2)
        mooseError(
            "Must specify two displacements for plane strain when the out of plane direction is z");
      else if (_out_of_plane_direction != OutOfPlaneDirection::z && _ndisp != 3)
        mooseError("Must specify three displacements for plane strain when the out of plane "
                   "direction is x or y");
    }
  }
}

void
TensorMechanicsAction::actOutputGeneration()
{
  //
  // Add variables (optional)
  //
  if (_current_task == "add_aux_variable")
  {
    auto params = _factory.getValidParams("MooseVariableConstMonomial");
    params.set<MooseEnum>("order") = "CONSTANT";
    params.set<MooseEnum>("family") = "MONOMIAL";
    // Loop through output aux variables
    for (auto out : _generate_output)
    {
      // Create output helper aux variables
      _problem->addAuxVariable("MooseVariableConstMonomial", _base_name + out, params);
    }
  }

  //
  // Add output AuxKernels
  //
  else if (_current_task == "add_aux_kernel")
  {
    // Loop through output aux variables
    for (auto out : _generate_output)
    {
      std::string type = "";
      InputParameters params = emptyInputParameters();

      // RankTwoAux
      for (const auto & r2a : _ranktwoaux_table)
        for (unsigned int a = 0; a < 3; ++a)
          for (unsigned int b = 0; b < 3; ++b)
            if (r2a.first + '_' + _component_table[a] + _component_table[b] == out)
            {
              type = "RankTwoAux";
              params = _factory.getValidParams(type);
              params.set<MaterialPropertyName>("rank_two_tensor") = _base_name + r2a.second;
              params.set<unsigned int>("index_i") = a;
              params.set<unsigned int>("index_j") = b;
            }

      // RankTwoScalarAux
      for (const auto & r2sa : _ranktwoscalaraux_table)
        for (const auto & t : r2sa.second.second)
          if (r2sa.first + '_' + t == out)
          {
            const auto r2a = _ranktwoaux_table.find(t);
            if (r2a != _ranktwoaux_table.end())
            {
              type = "RankTwoScalarAux";
              params = _factory.getValidParams(type);
              params.set<MaterialPropertyName>("rank_two_tensor") = _base_name + r2a->second;
              params.set<MooseEnum>("scalar_type") = r2sa.second.first;
            }
            else
              mooseError("Internal error. The permitted tensor shortcuts in "
                         "'_ranktwoscalaraux_table' must be keys in the '_ranktwoaux_table'.");
          }

      if (type != "")
      {
        params.applyParameters(parameters());
        params.set<AuxVariableName>("variable") = _base_name + out;
        params.set<ExecFlagEnum>("execute_on") = EXEC_TIMESTEP_END;
        _problem->addAuxKernel(type, _base_name + out + '_' + name(), params);
      }
      else
        mooseError("Unable to add output AuxKernel");
    }
  }
}

void
TensorMechanicsAction::actGatherActionParameters()
{
  //
  // Gather info about all other master actions when we add variables
  //
  if (_current_task == "validate_coordinate_systems" && getParam<bool>("add_variables"))
  {
    auto actions = _awh.getActions<TensorMechanicsAction>();
    for (const auto & action : actions)
    {
      const auto size_before = _subdomain_id_union.size();
      const auto added_size = action->_subdomain_ids.size();
      _subdomain_id_union.insert(action->_subdomain_ids.begin(), action->_subdomain_ids.end());
      const auto size_after = _subdomain_id_union.size();

      if (size_after != size_before + added_size)
        mooseError("The block restrictions in the TensorMechanics/Master actions must be "
                   "non-overlapping.");

      if (added_size == 0 && actions.size() > 1)
        mooseError("No TensorMechanics/Master action can be block unrestricted if more than one "
                   "TensorMechanics/Master action is specified.");
    }
  }
}

std::string
TensorMechanicsAction::getKernelType()
{
  std::map<Moose::CoordinateSystemType, std::string> type_map = {
      {Moose::COORD_XYZ, "StressDivergenceTensors"},
      {Moose::COORD_RZ, "StressDivergenceRZTensors"},
      {Moose::COORD_RSPHERICAL, "StressDivergenceRSphericalTensors"}};

  // choose kernel type based on coordinate system
  auto type_it = type_map.find(_coord_system);
  if (type_it != type_map.end())
    return type_it->second;
  else
    mooseError("Unsupported coordinate system");
}

InputParameters
TensorMechanicsAction::getKernelParameters(std::string type)
{
  InputParameters params = _factory.getValidParams(type);
  params.applyParameters(
      parameters(),
      {"displacements", "use_displaced_mesh", "save_in", "diag_save_in", "out_of_plane_strain"});

  params.set<std::vector<VariableName>>("displacements") = _coupled_displacements;
  params.set<bool>("use_displaced_mesh") = _use_displaced_mesh;

  return params;
}
