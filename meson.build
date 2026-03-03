# SPDX-License-Identifier: LGPL-2.1-or-later
project('libudev', 'c',
        version : '1.0.0',
        license : 'LGPL-2.1-or-later',
        default_options : [
                'c_std=gnu11',
                'warning_level=2',
        ])

cc = meson.get_compiler('c')

# Dependencies
thread_dep = dependency('threads')

# Configuration
conf = configuration_data()
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set('_GNU_SOURCE', 1)

# Generate config header
configure_file(
        output : 'config.h',
        configuration : conf)

# Include directories
inc = include_directories('src')

# libudev source files (based on official systemd structure)
libudev_sources = [
        'src/libudev.c',
        'src/libudev-device.c',
        'src/libudev-enumerate.c',
        'src/libudev-hwdb.c',
        'src/libudev-list.c',
        'src/libudev-monitor.c',
        'src/libudev-queue.c',
        'src/libudev-util.c',
]

# Create shared library
libudev = shared_library('udev',
        libudev_sources,
        include_directories : inc,
        dependencies : [thread_dep],
        version : meson.project_version(),
        soversion : '1',
        install : true,
        install_dir : get_option('libdir'))

# Install headers
install_headers('src/libudev.h')

# Generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(libudev,
        name : 'libudev',
        description : 'Linux device library (Termux compatible implementation)',
        version : meson.project_version(),
        url : 'https://www.freedesktop.org/wiki/Software/systemd/')

# Create UDev target for CMake compatibility
libudev_dep = declare_dependency(
        link_with : libudev,
        include_directories : inc)

# Export for use as subproject
libudev_dep_for_subproject = libudev_dep