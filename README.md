# Solution

Утилита для наложения BMP-изображения на YUV(I420) видеоряд.
Поддерживает преобразование RGB->YUV в однопоточном и многопоточном режиме. Также поддерживается преобразование с помощью SIMD инструкций Intel SSSE3 и AVX2.

## Сборка

Требования: GCC >= 4.9 или Clang >= 3.8, CMake >= 3.16, Make.

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Запуск

Справка по использованию программы:
```bash
Usage:
solution [options]

Required options:
  -i, --yuv-in  <path>  path to input YUV file
  -b, --bmp     <path>  path to overlay BMP file
  -o, --yuv-out <path>  path to output YUV file
  -w, --width   <int>   width of YUV frames (pixels, > 0)
  -H, --height  <int>   height of YUV frames (pixels, > 0)

Optional options:
  -x, --x       <int>   overlay X coordinate (default: 0, may be negative)
  -y, --y       <int>   overlay Y coordinate (default: 0, may be negative)
  -m, --mode    <mode>  conversion mode: single | parallel | simd (default: single)
  -j, --threads <int>   number of threads for parallel mode (default: 0 = auto)

  -h, --help            show this help and exit

Both '--flag value' and '--flag=value' forms are accepted.
```

Пример использования: (конвертация в simd режиме, с указанием начальных координат)
```bash
./build/solution -i input.yuv -b input.bmp -o output.yuv -w 1920 -H 1080 -x 200 -y 200 -m simd
```