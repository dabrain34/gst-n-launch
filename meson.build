project('gstnlaunch', 'c', version : '1.17.0.1', license : 'LGPL')

cc = meson.get_compiler('c')

gst_dep = dependency('gstreamer-1.0',
    fallback : ['gstreamer', 'gst_dep'])

executable('gst-n-launch-1.0',
    'src/gstnlaunch.c',
    install: true,
    dependencies : [gst_dep]
  )
