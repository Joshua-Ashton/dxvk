dxgi_res = wrc_generator.process('version.rc')

dxgi_src = [
  'dxgi_adapter.cpp',
  'dxgi_enums.cpp',
  'dxgi_factory.cpp',
  'dxgi_format.cpp',
  'dxgi_main.cpp',
  'dxgi_monitor.cpp',
  'dxgi_options.cpp',
  'dxgi_output.cpp',
  'dxgi_swapchain.cpp',
]

dxgi_src_win32 = [
  'platform/dxgi_monitor_win32.cpp',
  'platform/dxgi_swapchain_win32.cpp',
  'platform/dxgi_output_win32.cpp',
]

dxgi_src_sdl2 = [
  'platform/dxgi_monitor_sdl2.cpp',
  'platform/dxgi_swapchain_sdl2.cpp',
  'platform/dxgi_output_sdl2.cpp',
]

if dxvk_wsi == 'win32'
  dxgi_src += dxgi_src_win32
elif dxvk_wsi == 'sdl2'
  dxgi_src += dxgi_src_sdl2
else
  error('Unknown platform for dxgi')
endif

dxgi_dll = shared_library(so_prefix+'dxgi'+dll_ext, dxgi_src, dxgi_res,
  name_prefix         : '',
  dependencies        : [ dxvk_dep ],
  include_directories : dxvk_include_path,
  install             : true,
  vs_module_defs      : 'dxgi'+def_spec_ext,
  objects             : not dxvk_msvc and not dxvk_native ? 'dxgi'+def_spec_ext : [],
  override_options    : ['cpp_std='+dxvk_cpp_std])

dxgi_dep = declare_dependency(
  link_with           : [ dxgi_dll ],
  include_directories : [ dxvk_include_path ])
