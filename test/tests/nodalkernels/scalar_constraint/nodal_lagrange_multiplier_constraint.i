[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    xmin = -1
    xmax = 1
    ymin = -1
    ymax = 1
    nx = 2
    ny = 2
    elem_type = QUAD9
  []
  [center]
    type = ExtraNodesetGenerator
    input = gen
    new_boundary = center
    coord = '0 0'
  []
[]

[Functions]
  [./exact_fn]
    type = ParsedFunction
    value = 'x*x+y*y'
  [../]

  [./ffn]
    type = ParsedFunction
    value = -4
  [../]

  [./bottom_bc_fn]
    type = ParsedFunction
    value = -2*y
  [../]

  [./right_bc_fn]
    type = ParsedFunction
    value =  2*x
  [../]

  [./top_bc_fn]
    type = ParsedFunction
    value =  2*y
  [../]

  [./left_bc_fn]
    type = ParsedFunction
    value = -2*x
  [../]
[]

[Variables]
  [./u]
    family = LAGRANGE
    order = SECOND
    initial_condition = 1.0
  [../]

  [./lambda]
    family = SCALAR
    order = FIRST
  [../]
[]

[AuxVariables]
  [lambda_aux]
    family = LAGRANGE
    order = FIRST
  []
[]

[AuxKernels]
  [lambda_aux]
    type = CoupledScalarAux
    variable = lambda_aux
    coupled = lambda
    execute_on = linear
  []
[]

[Kernels]
  [./diff]
    type = Diffusion
    variable = u
  [../]

  [./ffnk]
    type = BodyForce
    variable = u
    function = ffn
  [../]
[]

[NodalKernels]
  [force]
    type = CoupledForceNodalKernel
    variable = u
    v = lambda_aux
    coef = -1.0 # residual is -coef*v, so make it positive
    boundary = center
  []
[]

[ScalarKernels]
  [./constraint]
    type = AverageValueConstraint
    variable = lambda
    pp_name = nodal_min
    value = 0.0
  [../]
[]

[BCs]
  [./bottom]
    type = FunctionNeumannBC
    variable = u
    boundary = 'bottom'
    function = bottom_bc_fn
  [../]
  [./right]
    type = FunctionNeumannBC
    variable = u
    boundary = 'right'
    function = right_bc_fn
  [../]
  [./top]
    type = FunctionNeumannBC
    variable = u
    boundary = 'top'
    function = top_bc_fn
  [../]
  [./left]
    type = FunctionNeumannBC
    variable = u
    boundary = 'left'
    function = left_bc_fn
  [../]
[]

[Postprocessors]
  [./integral]
    type = ElementIntegralVariablePostprocessor
    variable = u
    execute_on = 'initial linear'
  [../]
  [./nodal_min]
    type = NodalExtremeValue
    value_type = min
    variable = u
    boundary = center
    execute_on = 'initial linear'
  [../]
[]

[Preconditioning]
  [./pc]
    type = SMP
    full = true
    solve_type = 'NEWTON'
  [../]
[]

[Executioner]
  type = Steady
  nl_rel_tol = 1e-9
  l_tol = 1.e-10
  nl_max_its = 10
  # This is a linear problem, so we don't need to recompute the
  # Jacobian. This isn't a big deal for a Steady problems, however, as
  # there is only one solve.
  solve_type = 'LINEAR'
  petsc_options_iname = '-pc_type -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = ' lu       NONZERO               1e-10'
  # automatic_scaling = true
[]

[Outputs]
  exodus = true
[]
