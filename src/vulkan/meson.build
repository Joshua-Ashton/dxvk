vkcommon_src = [
  'vulkan_loader.cpp',
  'vulkan_names.cpp',
  'vulkan_presenter.cpp',
]

thread_dep = dependency('threads')

vkcommon_deps = [ thread_dep, lib_vulkan ]

if dxvk_wsi == 'win32'
  vkcommon_src  += 'platform/vulkan_presenter_win32.cpp'
elif dxvk_wsi == 'sdl2'
  vkcommon_src  += 'platform/vulkan_presenter_sdl2.cpp'
  vkcommon_deps += lib_sdl2
else
  error('Unknown platform for vkcommon')
endif

vkcommon_lib = static_library('vkcommon', vkcommon_src,
  dependencies        : vkcommon_deps,
  override_options    : ['cpp_std='+dxvk_cpp_std],
  include_directories : [ dxvk_include_path ])

vkcommon_dep = declare_dependency(
  link_with           : [ vkcommon_lib ],
  include_directories : [ dxvk_include_path ])
