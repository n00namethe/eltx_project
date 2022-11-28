#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include "head_motor.h"

typedef struct _control_motor_context_t
{
    //int nubmer_of_message_m2p;
    //int nubmer_of_message_p2m;
    motor_action_t action_m2p;
    motor_action_t action_p2m;
	mqd_t pic2motor_queue;
	mqd_t motor2pic_queue;
} control_motor_context_t;
control_motor_context_t cnt_ctx = {0};

void here_are_am(const char *asker_func_name)
{
    printf("я зашел в %s\n", asker_func_name);
}

int send_pic2motor_enum()
{
    here_are_am(__FUNCTION__);
    if (mq_send(cnt_ctx.pic2motor_queue, (char *)&cnt_ctx.action_p2m, sizeof(cnt_ctx.action_p2m), PRIORITY_OF_QUEUE) == -1)
    {
        printf("mq_send step not success, errno = %d\n", errno);
        return -1;
    }
    return 0;
}

int connect_queue_pic2motor()
{
    here_are_am(__FUNCTION__);
    while (cnt_ctx.pic2motor_queue <= 0)
    {
        printf("я в цикле, жду подключения к серверу\n");
        cnt_ctx.pic2motor_queue = mq_open(PIC2MOTOR_QUEUE, O_WRONLY);
        if (cnt_ctx.pic2motor_queue == -1)
        {
            printf("mq_open not success\nnext try 3..2..1..\nerrno = %d\n", errno);
            sleep(3);
        }
    }
    printf("Подкдючение к очереди произошло успешно, queue descriptor: %d\n", (int)cnt_ctx.pic2motor_queue);
    return 0;
}

int connect_queue_motor2pic()
{
    here_are_am(__FUNCTION__);
    while (cnt_ctx.motor2pic_queue <= 0)
    {
        printf("я в цикле, жду подключения к серверу\n");
        cnt_ctx.motor2pic_queue = mq_open(MOTOR2PIC_QUEUE, O_RDONLY);
        if (cnt_ctx.motor2pic_queue == -1)
        {
            printf("mq_open not success\nnext try 3..2..1..\nerrno = %d\n", errno);
            sleep(3);
        }
    }
    printf("Подкдючение к очереди произошло успешно, queue descriptor: %d\n", (int)cnt_ctx.motor2pic_queue);
    return 0;
}

int receive_motor2pic_enum()
{
    here_are_am(__FUNCTION__);
    while(1)
    {
        int size_receive = mq_receive(cnt_ctx.motor2pic_queue, (char *)&cnt_ctx.action_m2p, sizeof(cnt_ctx.action_m2p), NULL);
        if (size_receive < 0)
        {
            printf("размер сообщения меньше 0 байт. errno = %d\n", errno);
            printf("повтор попытки\n");
            continue;
        }
        if (size_receive != sizeof(cnt_ctx.action_m2p))
        {
            printf("размер полученного сообщения меньше ожидаемого. errno = %d\n", errno);
            printf("повтор попытки\n");
            continue;
        }
        break;
    }
    return cnt_ctx.action_m2p;
}

void goodbuy_motor()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.action_p2m = CAM2MOTOR_ACTION_EXIT;
    send_pic2motor_enum();
}

void calibr()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.action_p2m = CAM2MOTOR_ACTION_CALIBR;
    send_pic2motor_enum();
    if (receive_motor2pic_enum() != CAM2MOTOR_ACTION_CALIBR)
    {
        printf("какие-то сложности с калибровкой\n");
        exit(EXIT_FAILURE);
    }
}

int step()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.action_p2m = CAM2MOTOR_ACTION_STEP;
    send_pic2motor_enum();
    if (receive_motor2pic_enum() != CAM2MOTOR_ACTION_STEP)
    {
        printf("какие-то сложности с калибровкой\n");
        return -1;
    }
    else if (receive_motor2pic_enum() == CAM2MOTOR_ACTION_END_OF_ENUM)
    {
        printf("мотор выполнил все шаги\n");
        goodbuy_motor();
        return 1;
    }
    return 0;
}

#if 1 /* just for testing message_queue*/
int switch_action() 
{
    here_are_am(__FUNCTION__);
    printf("Выберете действие:\n%d: калибровка;\n%d: шаг\n%d: конец\n", CAM2MOTOR_ACTION_CALIBR, CAM2MOTOR_ACTION_STEP, CAM2MOTOR_ACTION_EXIT);
    scanf("%d%*c", &cnt_ctx.action_p2m);
    switch (cnt_ctx.action_p2m)
    {
        case CAM2MOTOR_ACTION_CALIBR:
        {
            calibr();
            break;
        }
        case CAM2MOTOR_ACTION_STEP:
        {
            step();
            break;
        }
        case CAM2MOTOR_ACTION_EXIT:
        {
            goodbuy_motor();
            break;
        }
        default:
        {
            printf("получено неизвестное сообщение\n");
            break;
        }
    }
    return 0;
}
#endif

int main()
{
	connect_queue_pic2motor();
    connect_queue_motor2pic();
    switch_action();
    return 0;
}

