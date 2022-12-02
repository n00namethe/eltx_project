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
#include "head_to_lib.h"

typedef struct _control_motor_context_t
{
    motor2pic_t msg_m2p;
    pic2motor_t msg_p2m;
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
    if (mq_send(cnt_ctx.pic2motor_queue, (char *)&cnt_ctx.msg_p2m, sizeof(cnt_ctx.msg_p2m), PRIORITY_OF_QUEUE) == -1)
    {
        printf("mq_send pic2motor_queue not success, errno = %d\n", errno);
        return -1;
    }
    printf("Я отправил сообщение № %d с командой %d\n", cnt_ctx.msg_p2m.number_of_comand_p2m, cnt_ctx.msg_p2m.action_p2m);
    cnt_ctx.msg_p2m.number_of_comand_p2m += 1;
    return 0;
}

void connect_queue_pic2motor()
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
}

void connect_queue_motor2pic()
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
}

void receive_motor2pic_enum()
{
    here_are_am(__FUNCTION__);
    while (1)
    {
        int size_receive = mq_receive(cnt_ctx.motor2pic_queue, (char *)&cnt_ctx.msg_m2p, sizeof(cnt_ctx.msg_m2p), NULL);
        if (size_receive != sizeof(cnt_ctx.msg_m2p))
        {
            printf("размер полученного сообщения не соответсвует ожидаемому. errno = %d\n", errno);
            printf("повтор попытки\n");
            sleep(3);
            continue;
        }
        printf("я принял сообщение №%d от действия %d\n", cnt_ctx.msg_m2p.number_of_comand_m2p, cnt_ctx.msg_m2p.action_m2p);
        break;
    }
}

void goodbye_motor()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.msg_p2m.action_p2m = CAM2MOTOR_ACTION_EXIT;
    send_pic2motor_enum();
}

void calibration()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.msg_p2m.action_p2m = CAM2MOTOR_ACTION_CALIBRATION;
    send_pic2motor_enum();
    receive_motor2pic_enum();
    if (cnt_ctx.msg_m2p.number_of_comand_m2p != cnt_ctx.msg_p2m.number_of_comand_p2m - 1)
    {
        printf("Мотор ответил не на то сообщение\nНомер отправленного: %d\nНомер полученного: %d\n", cnt_ctx.msg_p2m.number_of_comand_p2m - 1, cnt_ctx.msg_m2p.number_of_comand_m2p);
    }
    if (cnt_ctx.msg_m2p.action_m2p == CAM2MOTOR_ACTION_INVALID_TYPE)
    {
        printf("Какие-то проблемы с калибровкой\n");
    }
}

void step()
{
    here_are_am(__FUNCTION__);
    cnt_ctx.msg_p2m.action_p2m = CAM2MOTOR_ACTION_STEP;
    send_pic2motor_enum();
    receive_motor2pic_enum();
    if (cnt_ctx.msg_m2p.number_of_comand_m2p != cnt_ctx.msg_p2m.number_of_comand_p2m - 1)
    {
        printf("Мотор ответил не на то сообщение\nНомер отправленного: %d\nНомер полученного: %d\n", cnt_ctx.msg_p2m.number_of_comand_p2m - 1, cnt_ctx.msg_m2p.number_of_comand_m2p);
    }
    else if (cnt_ctx.msg_m2p.action_m2p == CAM2MOTOR_ACTION_END_OF_ENUM)
    {
        printf("мотор выполнил все шаги, запускаю функцию goodbye_motor\n");
        goodbye_motor();
    }
}

void init_contol_motor()
{
    connect_queue_pic2motor();
    connect_queue_motor2pic();
}
