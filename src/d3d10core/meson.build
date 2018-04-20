d3d10core_src = [
  'd3d10core_main.cpp',
]

d3d10core_dll = shared_library('d3d10core', d3d10core_src,
  name_prefix         : '',
  link_with           : [ util_lib ],
  dependencies        : [ dxvk_dep, dxgi_dep, dxbc_dep ],
  include_directories : dxvk_include_path,
  install             : true,
  vs_module_defs      : 'd3d10core.def',
  override_options    : ['cpp_std='+dxvk_cpp_std])

d3d10core_dep = declare_dependency(
  link_with           : [ d3d10core_dll ],
  include_directories : [ dxvk_include_path, include_directories('.') ])
