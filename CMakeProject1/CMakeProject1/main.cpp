#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// “我要画一个 mesh，
//你需要告诉我：
//用哪个 shader
//用哪个 VAO” 这两个参数，正好是画一个物体的最小必需信息。封装
//把0和3变成参数，好处在于可以自定义可以从第几个顶点画，以及要画几个顶点，如果不给值的话默认是0和3
void drawMesh(GLuint shader_program, GLuint vao, GLint first = 0, GLsizei count = 3) {
    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, first, count); //如果是从0和3的话 永远是从0个顶点开始画三个顶点，也就是说画一个三角形
}

GLuint makeShaderProgram(const char* vertex_shader, const char* fragment_shader) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);//我要创建一个【顶点着色器】的壳子,vs 是一个 GPU 对象的编号
    glShaderSource(vs, 1, &vertex_shader, NULL);//把这段字符串，塞进刚才那个槽位里。这一步只是“拷代码”，不是执行，也不是编译。

    //vs：要往哪个 shader 塞代码

    //1：一段字符串

    //& vertex_shader：代码内容

    //NULL：字符串长度我自己算

    glCompileShader(vs);//让 GPU 把这段 shader 代码编译成它能执行的机器形式。
    //同上
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    GLuint shader_program = glCreateProgram();//创建一个“程序容器好，我给你一个空盒子，你可以往里面装 shader
    glAttachShader(shader_program, fs);//把 fragment shader（像素规则）装进盒子
    glAttachShader(shader_program, vs);//把 vertex shader（点怎么变）也装进去
    glLinkProgram(shader_program);//让 GPU 把这两个 shader “接线 + 检查 + 合成”

    // 链接完就可以删掉 shader 对象（program 已经把结果“复制/固化”进去了）
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return shader_program;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
//这是你自己写的一个“回调函数”（callback）。
//GLFW 会在窗口的 framebuffer 尺寸发生变化时自动调用它。
//width / height 是新的 framebuffer 尺寸（像素单位）。
//告诉 OpenGL：从现在开始，用整个窗口大小作为绘制区域。
//前两个 0, 0：viewport 从左下角开始
//width, height：viewport 宽高
//一句话：窗口怎么变，这个函数就把 OpenGL 的绘图区域同步成同样大小。

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
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);//以后这个窗口 win 只要尺寸变化，就调用 framebuffer_size_callback。
    int w, h;
    glfwGetFramebufferSize(win, &w, &h); //问 GLFW 当前 framebuffer 的尺寸是多少（写到 w,h 里）。
    glViewport(0, 0, w, h); //把 OpenGL 的 viewport 设置成当前尺寸。
    //因为回调只有“发生变化时”才会触发。启动那一刻你也需要设置一次，否则有时 viewport 还是默认值，第一次显示就会不对。
   
     //窗口是否关闭，没有关闭false while (!glfwWindowShouldClose(win))
        float vertices[] = {
            -1.0f, -0.6f, 0.0f,
            -0.5f, -0.6f, 0.0f,
            -0.75f, 0.1f, 0.0f
        };  

        float vertices2[] = {
           -0.2f, -0.6f, 0.0f,
            0.3f, -0.6f, 0.0f,
            0.05f, 0.1f, 0.0f
        };

        float connected[] = {
            0.25f, 0.6f, 0.0f,
            0.25f, 0.2f, 0.0f,
            0.65f, 0.2f, 0.0f,

            0.25f, 0.6f, 0.0f,
            0.65f, 0.2f, 0.0f,
            0.65f, 0.6f, 0.0f
                };

        float tipToTip[] = {
            // upright (正三角) - CCW
            -0.85f,  0.35f, 0.0f,   // shared tip
            -1.00f,  0.0902f, 0.0f,
            -0.70f,  0.0902f, 0.0f,

            // inverted (倒三角) - CCW
            -0.85f,  0.35f, 0.0f,   // shared tip
            -0.70f,  0.6098f, 0.0f,
            -1.00f,  0.6098f, 0.0f
                };
        //如何做到fade背景
        //float vertices[] = {
        //    -1.0f, -1.0f, 0.0f,
        //    3.0f, -1.0f, 0.0f,
        //    -1.0f,  3.0f, 0.0f
        //};
        //这三个点看着奇怪，但它能覆盖整个屏幕（经典技巧）。

        //#version 330 core
        //layout(location = 0) in vec3 vp;
        //void main() { gl_Position = vec4(vp, 1.0); }
        //keep simplest vertex shader 

        //const char* fragment_shader =
        //    "#version 330 core\n"
        //    "out vec4 FragColor;\n" 
        //    "uniform vec2 uResolution;\n"
        //    "void main() {\n"
        //    "    vec2 uv = gl_FragCoord.xy / uResolution; // 0..1\n" gl_FragCoord = 当前像素的屏幕坐标（单位是像素）/ uResolution 归一化到 0~1
        //    "    vec3 top = vec3(0.1, 0.6, 0.9);\n"
        //    "    vec3 bottom = vec3(0.0, 0.0, 0.0);\n"
        //    "    vec3 col = mix(bottom, top, uv.y);\n" mix(a,b,t) = 线性混合（t=0 用 a，t=1 用 b）
        //    "    FragColor = vec4(col, 1.0);\n" uv.y 越大（越靠上）越接近 top
        //    "}\n";

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

        GLuint vbo2 = 0;
        glGenBuffers(1, &vbo2);
        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices2, GL_STATIC_DRAW);

        GLuint vao2 = 0;
        glGenVertexArrays(1, &vao2);
        glBindVertexArray(vao2);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glVertexAttribPointer(
            0,              // 给 shader 的 location 0
            3,              // 每个顶点 3 个分量
            GL_FLOAT,       // 每个分量是 float
            GL_FALSE,       // 不做归一化
            0,              // 紧密排列
            NULL            // 从 buffer 开头读
        );

        GLuint vboC = 0;
        glGenBuffers(1, &vboC);
        glBindBuffer(GL_ARRAY_BUFFER, vboC);
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), connected, GL_STATIC_DRAW);

        GLuint vaoC = 0;
        glGenVertexArrays(1, &vaoC);
        glBindVertexArray(vaoC);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboC);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            NULL
        );

        GLuint vboTip = 0;
        glGenBuffers(1, &vboTip);
        glBindBuffer(GL_ARRAY_BUFFER, vboTip);
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), tipToTip, GL_STATIC_DRAW);

        GLuint vaoTip = 0;
        glGenVertexArrays(1, &vaoTip);
        glBindVertexArray(vaoTip);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboTip);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            NULL
        );

        //必须要写的shader 写死在 C++ 里的字符串 
        // 
        // 顶点着色器可以决定每个顶点在屏幕上的位置
        const char* vertex_shader =
            "#version 330 core\n" //这段代码用的是 GLSL 3.30（对应 OpenGL 3.3）
            "layout (location = 0) in vec3 vp;\n" //我需要一个输入变量 vp，它来自顶点属性 location = 0，是一个 vec3（三个 float）
            "out vec3 vColor;\n"
            //“我要输出一个 vec3 的值，
            //名字叫 vColor，
            //后面的人可以用。”可以使用flat禁止插值 也就是禁止渐变 上下都要
            "void main() {\n" //每个 shader 必须有 main() GPU 会对 每个顶点调用一次
            "    gl_Position = vec4(vp+vec3(0.2,0.0,0.0), 1.0);\n"//把一个 3D 点 (x, y, z)，转成 GPU 必须要的 4D 形式(x, y, z, 1)，作为“这个顶点在屏幕上的位置”。
            "vColor = vp*0.5+0.5;\n" //给每个顶点算一个“颜色值” 颜色的区间[0,1] 坐标[-1,1]
            "}\n"; 

        //vec2 = 两个数(x, y)

        //vec3 = 三个数(x, y, z)

        //vec4 = 四个数(x, y, z, w)

        //vp * 0.5 = 所有顶点都向原点缩一半，三个点都离(0, 0) 更近了
        //gl_Position = vec4(vp+vec3(0.2,0.0,0.0), 1.0)给每个顶点的 x 都加 0.2，所有点整体向右挪，点之间的相对位置没变
        //vertices 是我在 CPU 世界给数据起的名字，
        //vp 是我在 GPU 世界给同一份数据起的名字，
        //它们通过 location 数字连接。
        // 
        //片段着色器所做的是计算像素最后的颜色输出
        const char* fragment_shader =
            "#version 330 core\n"
            "in vec3 vColor;\n" //“我要接收一个 vec3 的输入，名字也叫 vColor。”可以使用flat禁止插值 也就是禁止渐变 上下都要
            "out vec4 FragColor;\n" //这个 fragment shader 的输出，是一个 4 个分量的颜色。 r红色 g绿色 b蓝色 a透明度
            "void main() {\n"
            "    FragColor = vec4(vColor, 1.0);\n"//给与flagcolor参数，给每个像素上色
            "}\n";
        
        const char* fragment_shader_yellow =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            " FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
            "}\n";

        const char* fragment_shader_light_blue =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            " FragColor = vec4(0.0, 0.0, 1.0, 0.2);\n"
            "}\n";

        //“GPU 对屏幕上的每一个像素，都执行一次这个程序，
        //    然后用 FragColor 决定这个像素是什么颜色。”

        GLuint shader_program = makeShaderProgram(vertex_shader, fragment_shader);
        GLuint shader_program_yellow = makeShaderProgram(vertex_shader, fragment_shader_yellow);
        GLuint shader_program_light_blue = makeShaderProgram(vertex_shader, fragment_shader_light_blue);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //因为窗口是实时运行的程序，当渲染的时候每一帧都在运动
        //只要窗口还没被用户关闭，就一直循环。

        bool wireframe = false; //在内存里创建一个开关变量，名字叫 wireframe。默认不是线框模式

        while (!glfwWindowShouldClose(win)) {

            glfwPollEvents();//处理窗口事件 / 输入事件。

            if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { //问 GLFW：窗口 win 里，W 键现在是不是被按着？
                wireframe = true;
                //如果现在你正在按住 W 键，就进入大括号里面执行。
            }
            if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) { //E 键现在是不是被按住？
                wireframe = false;
                //你想退出线框模式，回到实心模式
            }
            
            if (wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //把之后画出来的三角形，改成只画边框。
            }
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //把之后画出来的三角形，改成实心填满。

            //按住 W → 开关变 true → 线框模式
            //按住 E → 开关变 false → 实心模式
            //每一帧根据开关，决定用 LINE 还是 FILL

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            //// 黑色(black)
            //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            //// 白色(white)
            //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            //// 红色(red)
            //glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

            //// 绿色(green)
            //glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

            //// 蓝色(blue)
            //glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

            //// 天蓝色(lighted blue)
            //glClearColor(0.1f, 0.6f, 0.9f, 1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //清屏：把上一帧画的东西擦掉，为新的一帧做准备。不清的话会导致上一帧的画面依然在画面里
            //GL_COLOR_BUFFER_BIT：清掉“颜色缓冲”（上一帧的颜色画面）
            //GL_DEPTH_BUFFER_BIT：清掉“深度缓冲”（上一帧的远近信息）
            
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//线框模式(Wireframe Mode)
            drawMesh(shader_program, vao);
            drawMesh(shader_program_yellow, vao2);
            drawMesh(shader_program, vaoC, 0, 6);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            drawMesh(shader_program, vaoTip, 0, 6);
            if (wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            float oldW = 1.0f;
            glGetFloatv(GL_LINE_WIDTH, &oldW);  

            if (wireframe) {
                glLineWidth(20.0f);            
            }
            drawMesh(shader_program_light_blue, vaoTip, 0, 6);

            glLineWidth(oldW);

            //glUseProgram(shader_program);//告诉 GPU：这一帧画东西要用哪一套 shader 规则。
            //glBindVertexArray(vao);//告诉 GPU：这一帧要从哪个 VAO 读取顶点数据，以及怎么解释这些数据。

            //glDrawArrays(GL_TRIANGLES, 0, 3);
            //真正下达绘制命令：画三角形。
            //GL_TRIANGLES：每 3 个顶点组成一个三角形
            //0：从第 0 个顶点开始
            //3：使用 3 个顶点

            glfwSwapBuffers(win);//把这一帧“画好的后台画面”显示到屏幕上。
        }
        //每一帧：处理输入 → 清屏 → 选规则（program）→ 选数据（VAO）→ 画 → 显示
        //glClearColor(0.1f, 0.6f, 0.9f, 1.0f); //将以往的颜色替代成现在的颜色
        //glClear(GL_COLOR_BUFFER_BIT); //清除颜色缓存
        //glfwSwapBuffers(win);
        //glfwPollEvents();//不断更新信息
        // 没有事件就睡眠等待，更省电  glfwWaitEvents(); 
    //当前面的while循环不结束的时候，不会使用到下面的函数
    glfwDestroyWindow(win);//删除窗口
    glfwTerminate();//删除glfw的库
    return 0;//程序正常结束，没有错误，return -1就是报错
}
