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
  [exact_fn]
    type = ParsedFunction
    value = 'x*x+y*y'
  []

  [ffn]
    type = ParsedFunction
    value = -4
  []

  [bottom_bc_fn]
    type = ParsedFunction
    value = -2*y
  []

  [right_bc_fn]
    type = ParsedFunction
    value = 2*x
  []

  [top_bc_fn]
    type = ParsedFunction
    value = 2*y
  []

  [left_bc_fn]
    type = ParsedFunction
    value = -2*x
  []
[]

[Variables]
  [u]
    family = LAGRANGE
    order = SECOND
    initial_condition = 1.0
  []
[]

[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
  [ffnk]
    type = BodyForce
    variable = u
    function = ffn
  []
[]

[NodalKernels]
  [penalty]
    type = NodalPPSCoefForce
    variable = u
    postprocessor = nodal_min
    coef = 1e3 # penalty
    boundary = center
  []
[]

[BCs]
  [bottom]
    type = FunctionNeumannBC
    variable = u
    boundary = 'bottom'
    function = bottom_bc_fn
  []
  [right]
    type = FunctionNeumannBC
    variable = u
    boundary = 'right'
    function = right_bc_fn
  []
  [top]
    type = FunctionNeumannBC
    variable = u
    boundary = 'top'
    function = top_bc_fn
  []
  [left]
    type = FunctionNeumannBC
    variable = u
    boundary = 'left'
    function = left_bc_fn
  []
[]

[Postprocessors]
  [integral]
    type = ElementIntegralVariablePostprocessor
    variable = u
    execute_on = 'initial linear'
  []
  [nodal_min]
    type = NodalExtremeValue
    value_type = min
    variable = u
    boundary = center
    execute_on = 'initial linear'
  []
[]

[Preconditioning]
  [pc]
    type = SMP
    full = true
    solve_type = 'NEWTON'
  []
[]

[Executioner]
  type = Steady
  nl_rel_tol = 1e-12
  l_tol = 1.e-5
  nl_max_its = 10
  # This is a linear problem, so we don't need to recompute the
  # Jacobian. This isn't a big deal for a Steady problems, however, as
  # there is only one solve.
  solve_type = 'LINEAR'
[]

[Outputs]
  exodus = true
[]
