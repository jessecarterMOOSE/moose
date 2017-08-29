[Mesh]
  type = GeneratedMesh
  dim = 2
[]

[Variables]
  [./u]
    order = FIRST
    family = LAGRANGE
  [../]
[]

[Kernels]
  [./diff]
    type = Diffusion
    variable = u
  [../]
[]

[BCs]
  [./right]
    type = DirichletBC
    variable = u
    boundary = 2
    value = 1
  [../]
[]

[UserObjects]
  [./volume_uo]
    type = VolumePostprocessor
    execute_on = 'initial timestep_end'
  [../]
  [./get_volume]
    type = GetPPSValue
    pps_1 = volume_pps
    pps_2 = volume_uo
  [../]
[]

[Postprocessors]
  [./volume_pps]
    type = VolumePostprocessor
    execute_on = 'initial timestep_end'
  [../]
[]

[Executioner]
  type = Steady

  # Preconditioned JFNK (default)
  solve_type = 'PJFNK'
[]
