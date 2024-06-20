//
// Created by starr on 2024/6/20.
//

#ifndef LOFIGFX_H
#define LOFIGFX_H

#ifdef LOFI_GFX_DLL_EXPORT
#define LOFI_API __declspec(dllexport)
#else
#define LOFI_API __declspec(dllimport)
#endif

extern "C"{
      LOFI_API void GStart();
}

#undef LOFI_API
#endif //LOFIGFX_H
