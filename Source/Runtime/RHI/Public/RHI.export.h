
#ifndef RHI_API_H
#define RHI_API_H

#ifdef RHI_STATIC_DEFINE
#  define RHI_API
#  define RHI_NO_EXPORT
#else
#  ifndef RHI_API
#    ifdef RHI_EXPORTS
        /* We are building this library */
#      define RHI_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define RHI_API __declspec(dllimport)
#    endif
#  endif

#  ifndef RHI_NO_EXPORT
#    define RHI_NO_EXPORT 
#  endif
#endif

#ifndef RHI_DEPRECATED
#  define RHI_DEPRECATED __declspec(deprecated)
#endif

#ifndef RHI_DEPRECATED_EXPORT
#  define RHI_DEPRECATED_EXPORT RHI_API RHI_DEPRECATED
#endif

#ifndef RHI_DEPRECATED_NO_EXPORT
#  define RHI_DEPRECATED_NO_EXPORT RHI_NO_EXPORT RHI_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RHI_NO_DEPRECATED
#    define RHI_NO_DEPRECATED
#  endif
#endif

#endif /* RHI_API_H */
