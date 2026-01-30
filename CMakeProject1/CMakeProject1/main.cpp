#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }//初始化，并且返还真假值,if后面的true了，才会进入if里
    GLFWwindow* win = glfwCreateWindow(640, 480, "Hello GLFW", nullptr, nullptr);//创建窗口和返还地址，前边的是保存
    //auto win = glfwCreateWindow(640, 480, "Hello GLFW", nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }//如果窗口创建失败（win 是 nullptr），因为win会返回false当创建空窗口的时候
    //就清理 GLFW，然后以错误码退出程序。

    while (!glfwWindowShouldClose(win)) {//窗口是否关闭，没有关闭false
        glfwPollEvents();//不断更新信息
        // 没有事件就睡眠等待，更省电  glfwWaitEvents(); 
    }//当前面的while循环不结束的时候，不会使用到下面的函数
    glfwDestroyWindow(win);//删除窗口
    glfwTerminate();//删除glfw的库
    return 0;//程序正常结束，没有错误，return -1就是报错
}
