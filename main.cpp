// ====================================================================
//  LINEAS (8 OCTANTES) Y CIRCULO SOBRE UNA GRILLA  -  SIN MOUSE
//
//  IDEA GENERAL
//  ------------
//  La pantalla es una grilla de casillas (COLUMNAS x FILAS). Cada
//  casilla es un cuadrado hecho con 2 triangulos (4 vertices), y cada
//  vertice guarda su POSICION y su COLOR.
//  "Pintar una casilla" = cambiar el color de sus 4 vertices.
//
//  Los algoritmos (Bresenham y punto medio) NO dibujan nada por si
//  solos: solo calculan QUE casillas (columna, fila) hay que pintar.
//  Despues otra funcion se encarga de pintarlas.
//
//  EN ESTA VERSION NO SE USA EL MOUSE
//  ----------------------------------
//  Las figuras se definen directamente en el codigo (ver la
//  SECCION "QUE FIGURAS SE DIBUJAN", justo antes de main). Para
//  cambiar el dibujo basta con editar esos numeros y volver a ejecutar.
// ====================================================================
#include <iostream>
#include <vector>
#include <cmath>     // (por si se necesitan funciones matematicas)
#include <cstdlib>   // abs
#include <cstddef>   // offsetof
#include <utility>   // std::pair, std::swap
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// --------------------------------------------------------------------
// SHADERS
// Son los programas que corren en la tarjeta grafica.
//  - Vertex shader  : recibe posicion (location 0) y color (location 1)
//                     de cada vertice, y pasa el color al siguiente paso.
//  - Fragment shader: pinta cada pixel con el color que le llega.
// --------------------------------------------------------------------
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"    ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(ourColor, 1.0f);\n"
"}\n\0";

// --------------------------------------------------------------------
// CONSTANTES DE LA GRILLA
// --------------------------------------------------------------------
const int   COLUMNAS   = 40;     // casillas en horizontal
const int   FILAS      = 40;     // casillas en vertical
const float SEPARACION = 0.002f; // espacio entre casillas (en NDC), asi se ve la grilla

// --------------------------------------------------------------------
// TIPOS DE DATOS
// --------------------------------------------------------------------

// Un vertice: posicion (x, y, z) y color (r, g, b).
// Es lo que se sube a la GPU (al VBO).
struct Vertex
{
    GLfloat x, y, z;
    GLfloat r, g, b;
};

// Un color RGB (cada valor de 0.0 a 1.0).
struct Color
{
    GLfloat r, g, b;
};

// Una casilla se identifica con (columna, fila).
// La fila 0 es la de ARRIBA y la columna 0 la de la IZQUIERDA.
typedef std::pair<int, int> Celda;

// --------------------------------------------------------------------
// COLORES (los mismos del otro repositorio)
// --------------------------------------------------------------------
const Color COLOR_CELDA   = { 0.212f, 0.278f, 0.463f }; // casilla vacia (azul oscuro)
const Color COLOR_CIRCULO = { 0.200f, 0.800f, 0.900f }; // circulos (cian)
const Color COLOR_LINEA   = { 0.950f, 0.550f, 0.150f }; // lineas (naranja)
const Color COLOR_CENTRO  = { 1.000f, 1.000f, 1.000f }; // casilla central (blanco)

// --------------------------------------------------------------------
// LISTA DE VERTICES E INDICES (la grilla completa)
// --------------------------------------------------------------------
std::vector<Vertex> vertices; // 4 vertices por casilla
std::vector<GLuint> indices;  // 6 indices por casilla (2 triangulos)

// ====================================================================
//  PARTE 1: CONSTRUIR Y PINTAR LA GRILLA
// ====================================================================

// Construye los vertices e indices de la grilla.
// Cada casilla tiene 4 vertices (las 4 esquinas) y 6 indices (2 triangulos):
//
//      v0 ---- v1          Triangulo 1: v0, v1, v2
//      |  \     |          Triangulo 2: v0, v2, v3
//      |    \   |
//      v3 ---- v2
//
// Las casillas se agregan de izquierda a derecha y de arriba hacia
// abajo, asi que la casilla (columna, fila) empieza en el vertice
// numero  (fila * COLUMNAS + columna) * 4.
void construirGrilla()
{
    for (int fila = 0; fila < FILAS; fila++)
    {
        for (int columna = 0; columna < COLUMNAS; columna++)
        {
            // Esquinas en coordenadas NDC (OpenGL usa -1 a 1 en X e Y).
            // x0 = borde izquierdo, x1 = borde derecho
            float x0 = -1.0f + 2.0f * columna / (float)COLUMNAS + SEPARACION;
            float x1 = -1.0f + 2.0f * (columna + 1) / (float)COLUMNAS - SEPARACION;
            // y0 = borde de arriba, y1 = borde de abajo (la fila 0 esta ARRIBA)
            float y0 = 1.0f - 2.0f * fila / (float)FILAS - SEPARACION;
            float y1 = 1.0f - 2.0f * (fila + 1) / (float)FILAS + SEPARACION;

            // Numero del primer vertice de esta casilla
            GLuint base = static_cast<GLuint>(vertices.size());

            // 4 vertices del cuadrado, todos con el color de casilla vacia
            vertices.push_back({ x0, y0, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // arriba-izquierda
            vertices.push_back({ x1, y0, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // arriba-derecha
            vertices.push_back({ x1, y1, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // abajo-derecha
            vertices.push_back({ x0, y1, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // abajo-izquierda

            // Triangulo 1
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            // Triangulo 2
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
    }
}

// Pinta UNA casilla: cambia el color de sus 4 vertices.
// Si la casilla esta fuera de la grilla (por ejemplo un circulo que se
// sale de la pantalla) simplemente se ignora.
void pintarCelda(int columna, int fila, Color color)
{
    if (columna < 0 || columna >= COLUMNAS || fila < 0 || fila >= FILAS) return;

    int celda = fila * COLUMNAS + columna; // numero de casilla
    int base = celda * 4;                  // primer vertice de esa casilla

    for (int i = 0; i < 4; i++)
    {
        vertices[base + i].r = color.r;
        vertices[base + i].g = color.g;
        vertices[base + i].b = color.b;
    }
}

// Pinta una lista de casillas con un mismo color.
void pintarCeldas(const std::vector<Celda>& celdas, Color color)
{
    for (size_t i = 0; i < celdas.size(); i++)
        pintarCelda(celdas[i].first, celdas[i].second, color);
}

// ====================================================================
//  PARTE 2: ALGORITMOS (solo calculan casillas, no dibujan)
// ====================================================================

// --------------------------------------------------------------------
// LINEA "MAS HORIZONTAL QUE VERTICAL"  (|dx| >= |dy|)
// Bresenham: avanza SIEMPRE una casilla en X, y decide si Y sube/baja
// una casilla o se queda igual. Solo usa numeros enteros.
//
// Cubre 4 octantes: los que estan cerca del eje horizontal
// (derecha-arriba, derecha-abajo, izquierda-arriba, izquierda-abajo).
// --------------------------------------------------------------------
void lineaHorizontal(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    // Queremos ir siempre de izquierda a derecha: si el punto inicial
    // esta a la derecha del final, los intercambiamos.
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;             // cuanto avanza en X (>= 0)
    int dy = y1 - y0;             // cuanto cambia en Y (puede ser negativo)

    int dir = (dy < 0) ? -1 : 1;  // Y baja (-1) o sube (+1)
    dy = dy * dir;                // dejamos dy positivo

    // D es el "parametro de decision". Si D >= 0 el punto ideal de la
    // linea esta mas cerca de la fila siguiente, asi que movemos Y.
    int D = 2 * dy - dx;
    int y = y0;

    for (int x = x0; x <= x1; x++)
    {
        celdas.push_back({ x, y });   // esta casilla es parte de la linea

        if (D >= 0)
        {
            y = y + dir;              // nos movemos a la fila vecina
            D = D - 2 * dx;
        }
        D = D + 2 * dy;               // actualizamos el parametro de decision
    }
}

// --------------------------------------------------------------------
// LINEA "MAS VERTICAL QUE HORIZONTAL"  (|dy| > |dx|)
// Es lo mismo que la anterior pero con los papeles de X e Y
// intercambiados: avanza SIEMPRE una casilla en Y y decide sobre X.
//
// Cubre los otros 4 octantes: los que estan cerca del eje vertical.
// --------------------------------------------------------------------
void lineaVertical(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    // Ahora ordenamos por Y: siempre de menor a mayor.
    if (y0 > y1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;             // cuanto cambia en X (puede ser negativo)
    int dy = y1 - y0;             // cuanto avanza en Y (>= 0)

    int dir = (dx < 0) ? -1 : 1;  // X va a la izquierda (-1) o a la derecha (+1)
    dx = dx * dir;                // dejamos dx positivo

    int D = 2 * dx - dy;
    int x = x0;

    for (int y = y0; y <= y1; y++)
    {
        celdas.push_back({ x, y });

        if (D >= 0)
        {
            x = x + dir;          // nos movemos a la columna vecina
            D = D - 2 * dy;
        }
        D = D + 2 * dx;
    }
}

// --------------------------------------------------------------------
// LINEA EN CUALQUIER DIRECCION (los 8 octantes)
// Elige cual de las dos versiones usar segun la inclinacion:
//   |dx| >= |dy|  -> mas horizontal -> lineaHorizontal
//   |dy| >  |dx|  -> mas vertical   -> lineaVertical
// --------------------------------------------------------------------
void dibujarLinea(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    if (std::abs(dx) >= std::abs(dy))
        lineaHorizontal(x0, y0, x1, y1, celdas);
    else
        lineaVertical(x0, y0, x1, y1, celdas);
}

// --------------------------------------------------------------------
// Agrega los 8 puntos simetricos de (x, y) respecto al centro (cx, cy).
// Un circulo es simetrico en 8 partes (los 8 octantes), asi que basta
// con calcular UN octante y reflejar cada casilla en los otros 7.
// --------------------------------------------------------------------
void puntosSimetricos(int cx, int cy, int x, int y, std::vector<Celda>& celdas)
{
    celdas.push_back({ cx + x, cy + y });
    celdas.push_back({ cx - x, cy + y });
    celdas.push_back({ cx + x, cy - y });
    celdas.push_back({ cx - x, cy - y });
    celdas.push_back({ cx + y, cy + x });
    celdas.push_back({ cx - y, cy + x });
    celdas.push_back({ cx + y, cy - x });
    celdas.push_back({ cx - y, cy - x });
}

// --------------------------------------------------------------------
// CIRCULO POR PUNTO MEDIO  (tambien solo con enteros)
// Centro (cx, cy) y radio r, todo en casillas.
//
// Empieza arriba del circulo (x=0, y=r) y baja hacia la diagonal
// (x == y), que es el final del primer octante. En cada paso:
//   - x avanza una casilla.
//   - D dice si el punto medio entre las dos casillas candidatas queda
//     DENTRO (D < 0: y se queda igual) o FUERA (D >= 0: y baja una).
// --------------------------------------------------------------------
void dibujarCirculo(int cx, int cy, int r, std::vector<Celda>& celdas)
{
    int x = 0;
    int y = r;
    int D = 1 - r;                         // valor inicial del parametro de decision

    puntosSimetricos(cx, cy, x, y, celdas); // primer punto (arriba del circulo)

    while (x < y)
    {
        x = x + 1;                         // x siempre avanza

        if (D < 0)
        {
            D = D + 2 * x + 1;             // el punto medio esta dentro: y no cambia
        }
        else
        {
            y = y - 1;                     // el punto medio esta fuera: y baja
            D = D + 2 * (x - y) + 1;
        }

        puntosSimetricos(cx, cy, x, y, celdas);
    }
}

// ====================================================================
//  PARTE 3: MOSTRAR RESULTADOS POR CONSOLA
// ====================================================================

// Muestra por consola el valor (columna, fila) de cada casilla que
// calculo el algoritmo, en orden. Sirve para explicar paso a paso
// como avanza Bresenham.
void imprimirCeldas(const char* titulo, const std::vector<Celda>& celdas)
{
    std::cout << titulo << " (" << celdas.size() << " casillas):" << std::endl;
    for (size_t k = 0; k < celdas.size(); k++)
    {
        std::cout << "  paso " << k << " -> (col " << celdas[k].first
                  << ", fila " << celdas[k].second << ")" << std::endl;
    }
}

// Ajusta el area de dibujo cuando cambia el tamano de la ventana.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ====================================================================
//  QUE FIGURAS SE DIBUJAN  (aqui se cambian los numeros)
// ====================================================================

// Una linea entre dos casillas, con su color.
struct Linea
{
    int x0, y0;   // casilla inicial (columna, fila)
    int x1, y1;   // casilla final   (columna, fila)
    Color color;
};

// Circulo: centro (columna, fila) y radio en casillas.
const int CIRCULO_CX = 20;
const int CIRCULO_CY = 20;
const int CIRCULO_R  = 19;   // 19 hace que el circulo toque casi los bordes

// Lineas que salen del centro (20, 20), una por cada OCTANTE. Cada una
// tiene una pendiente distinta, asi se prueba que funciona en las 8
// direcciones. Recuerda: la fila crece hacia ABAJO.
//   LARGO = largo de la linea larga, CORTO = desplazamiento corto.
const int LARGO = 17;
const int CORTO = 6;
const Linea LINEAS[] = {
    // x0  y0   x1       y1        color
    { 20, 20, 20 + LARGO,  20 - CORTO,  COLOR_LINEA },  // octante 1: derecha, un poco arriba
    { 20, 20, 20 + CORTO,  20 - LARGO,  COLOR_LINEA },  // octante 2: arriba,  un poco a la derecha
    { 20, 20, 20 - CORTO,  20 - LARGO,  COLOR_LINEA },  // octante 3: arriba,  un poco a la izquierda
    { 20, 20, 20 - LARGO,  20 - CORTO,  COLOR_LINEA },  // octante 4: izquierda, un poco arriba
    { 20, 20, 20 - LARGO,  20 + CORTO,  COLOR_LINEA },  // octante 5: izquierda, un poco abajo
    { 20, 20, 20 - CORTO,  20 + LARGO,  COLOR_LINEA },  // octante 6: abajo,   un poco a la izquierda
    { 20, 20, 20 + CORTO,  20 + LARGO,  COLOR_LINEA },  // octante 7: abajo,   un poco a la derecha
    { 20, 20, 20 + LARGO,  20 + CORTO,  COLOR_LINEA }   // octante 8: derecha, un poco abajo
};
const int CANTIDAD_LINEAS = sizeof(LINEAS) / sizeof(LINEAS[0]);

// ====================================================================
//  PARTE 4: MAIN (configuracion de OpenGL y bucle principal)
// ====================================================================
int main()
{
    glfwInit(); // inicia GLFW

    // Pedimos OpenGL 3.3 en perfil "core"
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Grilla", NULL, NULL);
    if (window == nullptr)
    {
        std::cout << "Fallo en la creacion de la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // esta ventana es el contexto actual
    gladLoadGL();                   // carga las funciones de OpenGL con GLAD
    glViewport(0, 0, 800, 800);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ---- Shaders: compilar y unir en un programa ----
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Ya estan dentro del programa: los objetos sueltos ya no hacen falta
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // ---- Construir la grilla (todas las casillas vacias) ----
    construirGrilla();

    // ---- Calcular y pintar las figuras ----
    // Se hace UNA sola vez, antes de subir los datos a la GPU.

    // 1) El circulo (cian)
    std::vector<Celda> celdasCirculo;
    dibujarCirculo(CIRCULO_CX, CIRCULO_CY, CIRCULO_R, celdasCirculo);
    imprimirCeldas("Casillas del circulo", celdasCirculo);
    pintarCeldas(celdasCirculo, COLOR_CIRCULO);

    // 2) Las lineas (naranja), una por octante
    for (int i = 0; i < CANTIDAD_LINEAS; i++)
    {
        std::vector<Celda> celdasLinea;
        dibujarLinea(LINEAS[i].x0, LINEAS[i].y0, LINEAS[i].x1, LINEAS[i].y1, celdasLinea);

        std::cout << "Linea " << i + 1 << ": de (" << LINEAS[i].x0 << ", " << LINEAS[i].y0
                  << ") a (" << LINEAS[i].x1 << ", " << LINEAS[i].y1 << ")" << std::endl;
        imprimirCeldas("Casillas de la linea (Bresenham)", celdasLinea);

        pintarCeldas(celdasLinea, LINEAS[i].color);
    }

    // 3) La casilla central en blanco (donde nacen todas las lineas)
    pintarCelda(20, 20, COLOR_CENTRO);

    // ---- VAO, VBO y EBO ----
    //  VBO: guarda los vertices (posicion + color) en la GPU.
    //  EBO: guarda los indices (como formar los triangulos).
    //  VAO: recuerda como leer el VBO, para no repetirlo en cada dibujo.
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Subimos los vertices (ya con los colores de las figuras).
    // GL_STATIC_DRAW porque los datos no cambian mientras corre el programa.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Subimos los indices (como formar los triangulos)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Atributo 0 = posicion (3 floats, empieza en el campo x de Vertex)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // Atributo 1 = color (3 floats, empieza en el campo r de Vertex)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    // Soltamos el VBO (el EBO se queda guardado dentro del VAO)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glClearColor(0.204f, 0.216f, 0.251f, 1.0f); // color de fondo

    // ---- Bucle principal ----
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        // Dibuja todos los triangulos de la grilla (indices de 3 en 3)
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents(); // procesa los eventos de la ventana
    }

    // Liberamos todo lo que creamos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
