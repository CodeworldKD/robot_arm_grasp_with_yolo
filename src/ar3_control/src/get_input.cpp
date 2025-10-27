
#include <iostream>
#include <ros/ros.h>
#include <std_msgs/String.h>
#include <string>
#include <thread>

using namespace std;


// 储存反馈消息
string BackInform2Interaction;
void doInteractionBackInform(const std_msgs::String::ConstPtr& back_inform)
{
    BackInform2Interaction = back_inform->data;
}


// 启动回调函数，单独线程启动
void run_processInteractionBackInform()
{
    string BackInform2Interaction_last;
    ros::Rate rate(10);
    while (ros::ok())
    {
        if(BackInform2Interaction.size() > 0 && BackInform2Interaction != BackInform2Interaction_last)
        {
            BackInform2Interaction_last = BackInform2Interaction;
            if(BackInform2Interaction == "Init")
            {
                cout << "初始化中，请等待..." << endl;
            }
            else if(BackInform2Interaction == "Ready")
            {
                cout << "控制系统准备完成，开始执行任务" << endl;
            }
            else if(BackInform2Interaction == "Search")
            {
                cout << "正在搜索目标" << endl;
            }
            else if(BackInform2Interaction == "Aim")
            {
                cout << "正在对正目标" << endl;
            }
            else if(BackInform2Interaction == "Grasp")
            {
                cout << "正在抓取目标" << endl;
            }
            else if(BackInform2Interaction == "Move")
            {
                cout << "正在移动目标" << endl;
            }
            else if(BackInform2Interaction == "Release")
            {
                cout << "释放目标" << endl;
            }
        }
        ros::spinOnce();
        rate.sleep();
    }
}


// 标记一下位置的占用情况，在这个程序中也有一个列表标记位置的占用情况，目的是在用户输入的时候对放置位置进行一个预判断，一般情况下，这里的情况和controller程序中的标记位置占用的vector一样
vector<bool> localBeOcc = {false, false, false, false, false, false};
// 获取用户输入并通过话题发送
void get_input_and_pub(ros::Publisher interaction_pub)
{
    // 如果获取到控制程序发送的消息为"Ready"，说明机械臂准备好了，可以允许用户输入
    if(BackInform2Interaction == "Ready")
    {
        // 等待0.5s
        ros::Duration(0.5).sleep();
        // 声明发布的文本消息
        std_msgs::String pub_txt;
        // 设置循环速度为50Hz
        ros::Rate r(50);
        // 首先询问是否需要初始化
        string needInit;
        cout << endl;
        cout << "---------------------------------" << endl;
        cout << "是否初始化场景?(y/n) : ";
        cin >> needInit; //将用户输入传入needInit
        // 如果needInit为y，说明需要初始，发送初始化命令
        if(needInit == "y")
        {
            pub_txt.data = "Init";
            // 占用情况列表清空
            for(int i = 0; i < localBeOcc.size(); i++)
            {
                localBeOcc[i] = false;
            }
        }
        // 如果不需要初始化，则询问需要抓取的模型和放置的位置
        else
        {
            string numberBeMoved;
            // 循环，直到用户输入正确的数字
            while(ros::ok())
            {
                cout << "请输入需要移动的数字(0-9): ";
                cin >> numberBeMoved;
                // 检查是否输入了一个字符
                if(numberBeMoved.size() == 1)
                {
                    // 检查输入的这个字符是否为数字
                    if(isdigit(numberBeMoved[0]))
                    {
                        break;
                    }
                }
                cout << "请输入正确的数字!" << endl;
            }

            string locale;
            // 循环，直到用户输入正确的位置
            while(ros::ok())
            {
                cout << "请输入放置的位置(1-6): ";
                cin >> locale;
                // 判断输入是否是一个字符并且为数字
                if(locale.size() == 1 && isdigit(locale[0]))
                {
                    // 转换为int
                    int local_num = atoi(locale.c_str());
                    // 判断是否为1-6
                    if(local_num > 0 && local_num < 7)
                    {
                        // 检查是否已经被占用了
                        if(localBeOcc[local_num - 1])
                        {
                            cout << "该位置已经被占用，请重新选择放置位置!" << endl;
                        }
                        // 没有被占用，则标记为被占用
                        else
                        {
                            localBeOcc[local_num - 1] = true;
                            break;
                        }
                    }
                    else
                    {
                        cout << "输入的位置标号不在范围内，请重新选择位置!" << endl;
                    }
                }
                cout << "请输入正确的位置标号!" << endl;
            }
            // 组织消息并发送，格式为：#数字$位置
            pub_txt.data = "#" + numberBeMoved + "$" + locale;
            cout << "任务开始执行" << endl;
        }

        // 发送50次，在50Hz的频率下，实际发送1s
        for(int i = 0; i < 50; i++)
        {
            interaction_pub.publish(pub_txt);
            r.sleep();
        }
        // 发送"None"，发送1s
        pub_txt.data = "None";
        for(int i = 0; i < 50; i++)
        {
            interaction_pub.publish(pub_txt);
            r.sleep();
        }
        cout << "---------------------------------" << endl;
        cout << endl;
        ros::Duration(1).sleep();
    }
}


// 主函数
int main(int argc, char** argv)
{
    // 中文显示
    setlocale(LC_ALL, "");
    ros::init(argc, argv, "get_input");
    ros::NodeHandle nh;
    // 发布话题
    ros::Publisher interaction_pub = nh.advertise<std_msgs::String>("/interaction_input", 10);
    // 接收话题
    ros::Subscriber inform_back_interaction_sub = nh.subscribe<std_msgs::String>("/interaction_back", 10, doInteractionBackInform);
    // 单独线程启动回调
    thread run_doInteractionBackInform_thread = thread(run_processInteractionBackInform);
    // 10Hz运行循环，实际是会阻塞在用户输入位置的
    ros::Rate rate(10);
    while (ros::ok())
    {
        // 等待并获取用户输入
        get_input_and_pub(interaction_pub);
        rate.sleep();
    }
    // 程序退出
    ros::shutdown();
    return 0;
}
