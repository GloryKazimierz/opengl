#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }//初始化，并且返还真假值,if后面的true了，才会进入if里
    GLFWwindow* win = glfwCreateWindow(640, 480, "Hello GLFW", nullptr, nullptr);//创建窗口和返还地址，前边的是保存
    //auto win = glfwCreateWindow(640, 480, "Hello GLFW", nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }//如果窗口创建失败（win 是 nullptr），因为win会返回false当创建空窗口的时候
    //就清理 GLFW，然后以错误码退出程序。
    glfwMakeContextCurrent(win);// 告诉 OpenGL：“接下来所有 glXXX 调用，都是针对这个窗口的。”
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { //告诉 GLAD：“如果你想知道某个 OpenGL 函数的地址，就用 GLFW 提供的 glfwGetProcAddress 去问系统。”
        // glfwGetprocAddress 会问你 glClearColor 这个函数真正在哪
      
        std::cerr << "Failed to init GLAD\n";
        glfwDestroyWindow(win);//关掉窗口
        glfwTerminate();//删除库
        return -1;
    }
    while (!glfwWindowShouldClose(win)) {//窗口是否关闭，没有关闭false
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f
        };

        GLuint vbo = 0; //申请一块空的GPU内存，vbo只是一个buffer的名字，代表初始化
        glGenBuffers(1, &vbo); //向OPENGL要一个buffer然后写进vbo
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // 从现在开始,所有针对 GL_ARRAY_BUFFER 的操作都作用在这个 vbo 上，绑定，告诉 OpenGL“我现在用它”
        glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices, GL_STATIC_DRAW);//我要操作的是绑定的这个buffer，指定大小，名字，状态, 把 CPU 数据拷贝进 GPU
        //The last line tells OpenGL that the buffer is the size of 9 floating point numbers, and gives it the address of the first value.

        GLuint vao = 0; //现在还没分配任何 VAO
        glGenVertexArrays(1, &vao); // 向 OpenGL 要 1 个 VAO，并把编号写进 vao
        glBindVertexArray(vao); //从现在开始，所有关于‘顶点属性配置’的操作，都记到这个 VAO 里。
        glEnableVertexAttribArray(0);//顶点属性 location = 0 是启用的。默认是所有 attribute 都是关闭的，所以要开机
        glBindBuffer(GL_ARRAY_BUFFER, vbo);//VAO 不会回忆你“之前 bind 过什么 VBO”。
        glVertexAttribPointer(
            0,              // 给 shader 的 location 0
            3,              // 每个顶点 3 个分量
            GL_FLOAT,       // 每个分量是 float
            GL_FALSE,       // 不做归一化
            0,              // 紧密排列
            NULL            // 从 buffer 开头读
        );

        const char* vertex_shader =
            "#version 330 core\n"
            "layout (location = 0) in vec3 vp;\n"
            "void main() {\n"
            "    gl_Position = vec4(vp, 1.0);\n"
            "}\n";

        const char* fragment_shader =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
            "}\n";

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader, NULL);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader, NULL);
        glCompileShader(fs);

        GLuint shader_program = glCreateProgram();
        glAttachShader(shader_program, fs);
        glAttachShader(shader_program, vs);
        glLinkProgram(shader_program);

        while (!glfwWindowShouldClose(win)) {
            
            glfwPollEvents();

            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            
            glUseProgram(shader_program);
            glBindVertexArray(vao);

            
            glDrawArrays(GL_TRIANGLES, 0, 3);

          
            glfwSwapBuffers(win);
        }

        glClearColor(0.1f, 0.6f, 0.9f, 1.0f); //将以往的颜色替代成现在的颜色
        glClear(GL_COLOR_BUFFER_BIT); //清除颜色缓存
        glfwSwapBuffers(win);
        glfwPollEvents();//不断更新信息
        // 没有事件就睡眠等待，更省电  glfwWaitEvents(); 
    }//当前面的while循环不结束的时候，不会使用到下面的函数
    glfwDestroyWindow(win);//删除窗口
    glfwTerminate();//删除glfw的库
    return 0;//程序正常结束，没有错误，return -1就是报错
}
