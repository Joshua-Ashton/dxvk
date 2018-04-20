d3d10_1_src = [
  'd3d10_1_blend.cpp',
  'd3d10_1_buffer.cpp',
  'd3d10_1_depth_stencil.cpp',
  'd3d10_1_device.cpp',
  'd3d10_1_context.cpp',
  'd3d10_1_enums.cpp',
  'd3d10_1_input_layout.cpp',
  'd3d10_1_main.cpp',
  'd3d10_1_options.cpp',
  'd3d10_1_present.cpp',
  'd3d10_1_query.cpp',
  'd3d10_1_rasterizer.cpp',
  'd3d10_1_sampler.cpp',
  'd3d10_1_shader.cpp',
  'd3d10_1_state.cpp',
  'd3d10_1_texture.cpp',
  'd3d10_1_util.cpp',
  'd3d10_1_view_dsv.cpp',
  'd3d10_1_view_rtv.cpp',
  'd3d10_1_view_srv.cpp',
]


d3d10_1_dll = shared_library('d3d10_1', d3d10_1_src,
  name_prefix         : '',
  link_with           : [ util_lib ],
  dependencies        : [ dxvk_dep, dxgi_dep, dxbc_dep ],
  include_directories : dxvk_include_path,
  install             : true,
  vs_module_defs      : 'd3d10_1.def',
  override_options    : ['cpp_std='+dxvk_cpp_std])

d3d10_1_dep = declare_dependency(
  link_with           : [ d3d10_1_dll ],
  include_directories : [ dxvk_include_path, include_directories('.') ])
