# Grilla sin mouse: lineas (8 octantes) y circulo - OpenGL

Grilla de 40x40 casillas. Las figuras se definen en el codigo (no hay mouse):
un circulo (cian) y 8 lineas (naranja) que salen del centro, una por octante.

## Como cambiar el dibujo
En `main.cpp`, seccion **"QUE FIGURAS SE DIBUJAN"**:
- `CIRCULO_CX`, `CIRCULO_CY`, `CIRCULO_R`: centro y radio del circulo (en casillas).
- Arreglo `LINEAS[]`: cada linea es `{ x0, y0, x1, y1, color }`.
- Colores: constantes `COLOR_*` al inicio del archivo.

## Mapa de `main.cpp`
1. **Grilla**: `construirGrilla`, `pintarCelda`, `pintarCeldas`
2. **Algoritmos**: `lineaHorizontal`, `lineaVertical`, `dibujarLinea`, `dibujarCirculo`
3. **Consola**: `imprimirCeldas` (muestra cada casilla que calcula el algoritmo)
4. **main**: configuracion de OpenGL, pintado de figuras y bucle principal

## Compilar
Abrir `GrillaSinMouse.slnx` en Visual Studio y ejecutar (x64).
GLFW y GLAD estan en `librerias/`.
