project(
  'ptrace',
  ['cpp', 'c'],
  version: '1.0',
  license: 'GPLv2',
  meson_version : '>=0.47.0',
  default_options : [
    'cpp_std=c++11',
    'buildtype=release'
  ],
)

add_project_arguments(
  '-DPTRACE_VERSION="@0@"'.format(meson.project_version()),
  language : 'cpp',
)

libperfetto = static_library('perfetto',
  'perfetto/perfetto.cc',
)

libperfetto_dep = declare_dependency(
  include_directories : 'perfetto',
  link_with : libperfetto,
)

libptrace = shared_library('ptrace',
  'lib/ptrace.cc',
  cpp_args : '-pthread',
  link_args : '-pthread',
  dependencies : libperfetto_dep,
  install : true,
)

pkgconf_data = configuration_data()
pkgconf_data.set('PACKAGE_VERSION', meson.project_version())
pkgconf_data.set('prefix', get_option('prefix'))
pkgconf_data.set('exec_prefix', '${prefix}')
pkgconf_data.set('libdir', '${prefix}/' + get_option('libdir'))
pkgconf_data.set('includedir', '${prefix}/' + get_option('includedir'))

configure_file(
  input : 'lib/libptrace.pc.in',
  output : 'libptrace.pc',
  configuration : pkgconf_data,
  install_dir: join_paths([get_option('libdir'), 'pkgconfig']),
)

libptrace_test = executable('libptrace_test',
  'lib/test/main.c',
  include_directories : 'lib',
  link_with : libptrace,
)

protoc = find_program('protoc')
proto2cpp = generator(protoc,
  arguments : [
    '--proto_path', '@SOURCE_DIR@/proto',
    '--cpp_out', '@BUILD_DIR@',
    '@INPUT@'
  ],
  output : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
)

config_path = join_paths([
  get_option('prefix'),
  get_option('sysconfdir'),
  'ptrace',
])

ptrace = executable('ptrace',
  'tools/main.cc',
  'tools/ptrace_cmd.cc',
  'tools/ptrace_combine.cc',
  'tools/ptrace_msg.cc',
  proto2cpp.process('proto/perfetto_trace.proto'),
  cpp_args : [
    '-Wno-deprecated-declarations',
    '-DPTRACE_CONFIG_PATH="@0@"'.format(config_path),
  ],
  dependencies : [
    dependency('protobuf'),
    libperfetto_dep
  ],
  install : true,
)

ptrace_combine = custom_target('ptrace-combine',
  output : 'ptrace-combine',
  command : ['ln', '-sf', 'ptrace', '@OUTPUT@'],
  install : true,
  install_dir : get_option('bindir')
)

install_headers('lib/ptrace.h')

install_subdir(
  'config/ptrace',
  install_dir : get_option('sysconfdir'),
)

