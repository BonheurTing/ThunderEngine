
#ifndef LAUNCH_API_H
#define LAUNCH_API_H

#ifdef LAUNCH_STATIC_DEFINE
#  define LAUNCH_API
#  define LAUNCH_NO_EXPORT
#else
#  ifndef LAUNCH_API
#    ifdef Launch_EXPORTS
        /* We are building this library */
#      define LAUNCH_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define LAUNCH_API __declspec(dllimport)
#    endif
#  endif

#  ifndef LAUNCH_NO_EXPORT
#    define LAUNCH_NO_EXPORT 
#  endif
#endif

#ifndef LAUNCH_DEPRECATED
#  define LAUNCH_DEPRECATED __declspec(deprecated)
#endif

#ifndef LAUNCH_DEPRECATED_EXPORT
#  define LAUNCH_DEPRECATED_EXPORT LAUNCH_API LAUNCH_DEPRECATED
#endif

#ifndef LAUNCH_DEPRECATED_NO_EXPORT
#  define LAUNCH_DEPRECATED_NO_EXPORT LAUNCH_NO_EXPORT LAUNCH_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LAUNCH_NO_DEPRECATED
#    define LAUNCH_NO_DEPRECATED
#  endif
#endif

#endif /* LAUNCH_API_H */
