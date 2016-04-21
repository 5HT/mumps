# Installations #
I downloaded and installed GT.M V5.3003 from SF.net into my
home directory under /Users/amul/Programming/gtm/V53003. A
word of caution, the gtm tarball dumps itself into the
current working direcotry, not a subdirecttory like any
other tarball.

You will need to get Unicode headers.  There is a project
http://aarone.org/cocoaicu/ that has ICU headers, but I was
too lazy to work with that, so I stole them from my Linux
box.

# Steps to compile #
```

tcsh
setenv gtm_curpro /Users/amul/Programming/gtm/V53003
setenv gtm_exe gtm_curpro
setenv gtm_inc pwd/sr_darwin
setenv gtm_tools pwd/sr_darwin
setenv gtm_version_change 1
source sr_unix/gtm_env.csh
setenv distro ubuntu
make -f sr_unix/comlist.mk -I./sr_unix -I./sr_darwin buildtypes=dbg gtm_ver=pwd clean
```

# Status #
Currently bombs out because I did not update some header for platform support:
#error UNSUPPORTED PLATFORM