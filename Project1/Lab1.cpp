#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main()
{
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3.\n");
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(512, 512, "Mainwindow", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW\n");
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        // ÷вет фона (белый)
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        // ÷вет эллипса
        glColor3f(0.2f, 1.0f, 1.0f);

        // –исуем эллипс
        glBegin(GL_POLYGON);
        for (int i = 0; i < 100; i++) {
            float angle = 2.0f * 3.1415926f * i / 100;

            float x = 0.5f * cos(angle); // радиус по X
            float y = 0.3f * sin(angle); // радиус по Y

            glVertex2f(x, y);
        }
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    const GLubyte* version_str = glGetString(GL_VERSION);
    const GLubyte* device_str = glGetString(GL_RENDERER);

    printf("OpenGL version: %s\n", version_str);
    printf("Renderer: %s\n", device_str);

    glfwTerminate();
    return 0;
}
