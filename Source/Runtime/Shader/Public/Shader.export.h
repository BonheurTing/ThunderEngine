
#ifndef SHADER_API_H
#define SHADER_API_H

#ifdef SHADER_STATIC_DEFINE
#  define SHADER_API
#  define SHADER_NO_EXPORT
#else
#  ifndef SHADER_API
#    ifdef Shader_EXPORTS
        /* We are building this library */
#      define SHADER_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define SHADER_API __declspec(dllimport)
#    endif
#  endif

#  ifndef SHADER_NO_EXPORT
#    define SHADER_NO_EXPORT 
#  endif
#endif

#ifndef SHADER_DEPRECATED
#  define SHADER_DEPRECATED __declspec(deprecated)
#endif

#ifndef SHADER_DEPRECATED_EXPORT
#  define SHADER_DEPRECATED_EXPORT SHADER_API SHADER_DEPRECATED
#endif

#ifndef SHADER_DEPRECATED_NO_EXPORT
#  define SHADER_DEPRECATED_NO_EXPORT SHADER_NO_EXPORT SHADER_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SHADER_NO_DEPRECATED
#    define SHADER_NO_DEPRECATED
#  endif
#endif

#endif /* SHADER_API_H */
