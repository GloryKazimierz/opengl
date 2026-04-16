#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow* win = glfwCreateWindow(1600, 900, "Hello GLFW", nullptr, nullptr);//创建窗口和返还地址，前边的是保存
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
   
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // 默认就是这个，写上更清楚

    std::cout << "CWD: " << std::filesystem::current_path() << "\n";

    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true); // 可选：让图片上下不倒

    unsigned char* data = stbi_load("../../../texture/blackice.jpg", &width, &height, &channels, 0);
    GLuint tex0 = 0;
    if (!data) {
        std::cout << "Failed to load texture: " << stbi_failure_reason() << "\n";
    }
    else {
        std::cout << "Loaded texture: " << width << "x" << height
            << " channels=" << channels << "\n";

        // 1) 创建 OpenGL 纹理对象
        glGenTextures(1, &tex0);
        glBindTexture(GL_TEXTURE_2D, tex0);

        // 2) 设置纹理参数（先照抄）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 3) 上传像素到 GPU（你现在 channels=3，所以用 GL_RGB）
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 4) 上传完了再 free
        stbi_image_free(data);
    }
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

        float vertices_pos_color[] = {
            0.5f, -0.5f, 0.0f,            1.0f, 0.0f, 0.0f,   // 右下角：红色
            -0.5f, -0.5f, 0.0f,            0.0f, 1.0f, 0.0f,   // 左下角：绿色
            0.0f,  0.5f, 0.0f,            0.0f, 0.0f, 1.0f    // 顶点：蓝色
        };

        float quad_tex[] = {
            // pos(x,y,z)           // uv(u,v)
             0.9f,  0.9f, 0.0f,      1.0f, 1.0f,  // 右上
             0.9f,  0.4f, 0.0f,      1.0f, 0.0f,  // 右下
             0.4f,  0.4f, 0.0f,      0.0f, 0.0f,  // 左下

             0.9f,  0.9f, 0.0f,      1.0f, 1.0f,  // 右上
             0.4f,  0.4f, 0.0f,      0.0f, 0.0f,  // 左下
             0.4f,  0.9f, 0.0f,      0.0f, 1.0f   // 左上
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

        GLuint vbo_pc = 0;
        glGenBuffers(1, &vbo_pc);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_pc);
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), vertices_pos_color, GL_STATIC_DRAW);

        GLuint vao_pc = 0;
        glGenVertexArrays(1, &vao_pc);
        glBindVertexArray(vao_pc);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_pc);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),
            (void*)0
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),
            (void*)(3 * sizeof(float))
        );

        GLuint vbo_tex = 0, vao_tex = 0;
        glGenBuffers(1, &vbo_tex);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_tex), quad_tex, GL_STATIC_DRAW);

        glGenVertexArrays(1, &vao_tex);
        glBindVertexArray(vao_tex);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);

        // location=0：position vec3
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        // location=2：uv vec2（我用2是为了不碰你已有的1）
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

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
            "vec3 p = vp;\n"
            "p.xy *= 0.6;\n"                      // ✅ 整体缩小到 60%
            "    gl_Position = vec4(p+vec3(0.2,0.0,0.0), 1.0);\n"//把一个 3D 点 (x, y, z)，转成 GPU 必须要的 4D 形式(x, y, z, 1)，作为“这个顶点在屏幕上的位置”。
            "vColor = vp*0.5+0.5;\n" //给每个顶点算一个“颜色值” 颜色的区间[0,1] 坐标[-1,1]
            "}\n"; 

        const char* vertex_shader_pos_color =
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"   // 输入1：位置，来自 VAO 的 attrib 0
            "layout (location = 1) in vec3 aColor;\n" // 输入2：颜色，来自 VAO 的 attrib 1
            "out vec3 vColor;\n" 
            // 输出：传给片段着色器（会自动插值）
            "uniform vec2 uOffset;\n"
            "void main() {\n"
            "    vec3 pos = aPos + vec3(0.2, 0.0, 0.0) + vec3(uOffset, 0.0);\n"
            "    pos.xy *= 0.6;\n"                    // ✅ 整体缩小
            "    gl_Position = vec4(pos, 1.0);\n" // 你之前坚持的 +0.2 继续保留
            "    vColor = aColor;\n"                   // 把顶点颜色传下去
            "}\n";

        const char* vertex_shader_tex =
            "#version 330 core\n"
            "layout(location=0) in vec3 aPos;\n"
            "layout(location=2) in vec2 aUV;\n"
            "out vec2 vUV;\n"
            "void main(){\n"
            "  gl_Position = vec4(aPos, 1.0);\n"
            "  vUV = aUV;\n"
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

        const char* fragment_shader_pos_color =
            "#version 330 core\n"
            "in vec3 vColor;\n"                        // 接收插值后的颜色
            "out vec4 FragColor;\n"
            "uniform vec4 ourColor;\n"
            "void main() {\n"
            "    FragColor = vec4(vColor, 1.0) * ourColor;\n"     // 输出颜色（不透明）
            "}\n";

        const char* fragment_shader_tex =
            "#version 330 core\n"
            "in vec2 vUV;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D uTex;\n"
            "void main(){\n"
            "  FragColor = texture(uTex, vUV);\n"
            "}\n";

        //“GPU 对屏幕上的每一个像素，都执行一次这个程序，
        //    然后用 FragColor 决定这个像素是什么颜色。”

        GLuint shader_program = makeShaderProgram(vertex_shader, fragment_shader);
        GLuint shader_program_yellow = makeShaderProgram(vertex_shader, fragment_shader_yellow);
        GLuint shader_program_light_blue = makeShaderProgram(vertex_shader, fragment_shader_light_blue);
        GLuint shader_program_pos_color = makeShaderProgram(vertex_shader_pos_color, fragment_shader_pos_color);
        GLuint shader_program_tex = makeShaderProgram(vertex_shader_tex, fragment_shader_tex);

        glUseProgram(shader_program_pos_color);
        GLint locOffset = glGetUniformLocation(shader_program_pos_color, "uOffset");
        GLint locColor = glGetUniformLocation(shader_program_pos_color, "ourColor");
        // 把 uTex 绑定到纹理单元0（设置一次就行）
        glUseProgram(shader_program_tex);
        glUniform1i(glGetUniformLocation(shader_program_tex, "uTex"), 0);
    

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

            glUseProgram(shader_program_pos_color);

            // （可选但推荐）每帧给 uOffset 一个明确值，避免未初始化
            glUniform2f(locOffset, 0.0f, 0.0f);

            // ---- 按 C 切换模式（你原来的代码）----
            static bool mode = false;
            static bool prevC = false;

            bool curC = (glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
            if (curC && !prevC) {
                mode = !mode;
            }
            prevC = curC;

            // ---- 只在这里设置一次 ourColor ----
            if (!mode) {
                // 模式0：呼吸绿色
                float t = (float)glfwGetTime();
                float g = (sinf(t) * 0.5f) + 0.5f;   // 0~1 变化
                glUniform4f(locColor, g , g, g, 1.0f);
            }
            else {
                // 模式1：固定粉色
                glUniform4f(locColor, 1.0f, 0.2f, 0.6f, 1.0f);
            }

            // 绑定纹理到 0 号槽位
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex0);

            // 画矩形（6个顶点）
            drawMesh(shader_program_tex, vao_tex, 0, 6);
            drawMesh(shader_program, vao);
            drawMesh(shader_program_yellow, vao2);
            drawMesh(shader_program, vaoC, 0, 6);
            drawMesh(shader_program_pos_color, vao_pc);   // 默认 count=3，画一个彩色插值三角形

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                            
            drawMesh(shader_program, vaoTip, 0, 6);
            
            if (wireframe) {
                float oldW = 1.0f;
                glGetFloatv(GL_LINE_WIDTH, &oldW);

                glDisable(GL_DEPTH_TEST);              // ⭐关键：淡蓝层不参与深度测试
                glLineWidth(12.0f);                    // ⭐把 40 改小！先试 8~12
                // 如果你担心此时不一定是 LINE，也可以强制一下：
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                drawMesh(shader_program_light_blue, vaoTip, 0, 6);

                glLineWidth(oldW);
                glEnable(GL_DEPTH_TEST);               // ⭐恢复
            }

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
