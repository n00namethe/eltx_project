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

#define MOTOR_MOVE		0x3
#define MOTOR_RESET		0x2

#define STEP_SIZE 430
#define MOTOR_ZERO_POINT 0
#define LIMIT_STEP 4300

typedef struct _motor_context_t
{
	int cur_step;
	int max_steps;
	mqd_t pic2motor_queue;
	mqd_t motor2pic_queue;
} motor_context_t;
motor_context_t motor_ctx = {0};

void here_are_am(const char *asker_func_name)
{
    printf("я зашел в %s\n", asker_func_name);
}

int send_pic2motor_enum(int action)
{
	if (mq_send(motor_ctx.motor2pic_queue, (char *)&action, sizeof(action), PRIORITY_OF_QUEUE) == -1)
    {
        printf("mq_send step not success, errno = %d\n", errno);
        return -1;
    }
    return 0;
}

int create_queue_pic2motor()
{
    here_are_am(__FUNCTION__);
    motor_ctx.pic2motor_queue = mq_open(PIC2MOTOR_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_motor_queue);
    if (motor_ctx.pic2motor_queue == -1)
    {
        perror("ошибка открытия очереди SERVICE_QUEUE");
        return -1;
    }
    printf("очередь pic2motor_queue успешнот открыта, ФД = %d\n", (int)motor_ctx.pic2motor_queue);
    return 0;
}

int create_queue_motor2pic()
{
	here_are_am(__FUNCTION__);
	motor_ctx.motor2pic_queue = mq_open(MOTOR2PIC_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_motor_queue);
    if (motor_ctx.motor2pic_queue == -1)
    {
        perror("ошибка открытия очереди SERVICE_QUEUE");
        return -1;
    }
    printf("очередь motor2pic_queue успешнот открыта, ФД = %d\n", (int)motor_ctx.motor2pic_queue);
    return 0;
}

int send_command2motor(int cmd, void *buffer) 
{
	here_are_am(__FUNCTION__);
    int fd = open("/dev/motor", O_WRONLY);
    if (ioctl(fd, cmd, buffer) == -1)
    {
    	printf("send_command2motor not success, errno = %d\n", errno);
    	return -1;
    }
    close(fd);
    return 0;
}

int set_movement()
{
	here_are_am(__FUNCTION__);
	motor_action_t action;
	motor_ctx.cur_step += STEP_SIZE;
	if (motor_ctx.cur_step >= LIMIT_STEP)
	{
		action = CAM2MOTOR_ACTION_END_OF_ENUM;
	    send_pic2motor_enum(action);
	    return 0;
	}
	if (send_command2motor(MOTOR_MOVE, &motor_ctx.cur_step) == 0)
	{
		action = CAM2MOTOR_ACTION_STEP;
	    send_pic2motor_enum(action);
	    return 0;
	}
	else
	{
		action = CAM2MOTOR_ACTION_INVALID_TYPE;
	    send_pic2motor_enum(action);
    	return -1;
	}
}

int calibr()
{
	here_are_am(__FUNCTION__);
	motor_action_t action;
    motor_ctx.cur_step = MOTOR_ZERO_POINT;
    if (send_command2motor(MOTOR_RESET, &motor_ctx.cur_step) == 0)
    {
    	action = CAM2MOTOR_ACTION_CALIBR;
	    send_pic2motor_enum(action);
	    return 0;
    }
    else
    {
    	action = CAM2MOTOR_ACTION_INVALID_TYPE;
	    send_pic2motor_enum(action);
    	return -1;
    }
}

int receive_pic2motor_enum()
{
	here_are_am(__FUNCTION__);
	motor_action_t action;
    while (1)
    {
        int size_receive = mq_receive(motor_ctx.pic2motor_queue, (char *)&action, sizeof(action), NULL);
        if (size_receive < 0)
        {
            printf("размер сообщения меньше 0 байт. errno = %d\n", errno);
            printf("повтор попытки\n");
            continue;
        }
        if (size_receive != sizeof(action))
        {
            printf("размер полученного сообщения меньше ожидаемого. errno = %d\n", errno);
            printf("повтор попытки\n");
            continue;
        }

        switch(action)
        {
        	case CAM2MOTOR_ACTION_CALIBR:
    		{
    			calibr();
    			break;
    		}
    		case CAM2MOTOR_ACTION_STEP:
			{
				set_movement();
				break;
			}
			case CAM2MOTOR_ACTION_EXIT:
			{
				return 0;
			}
			default:
			{
				printf("получен неопознанный тип события\n");
				break;
			}
        }
    }
}
int main()
{
	create_queue_pic2motor();
	create_queue_motor2pic();
	receive_pic2motor_enum();
	if (mq_close(motor_ctx.pic2motor_queue) == -1)
    {
        printf("mq_close serv_queue not success, errno = %d\n", errno);
        mq_close(motor_ctx.pic2motor_queue);
        mq_unlink(MOTOR2PIC_QUEUE);
        return -1;
    }
    if (mq_unlink(PIC2MOTOR_QUEUE) == -1)
    {
        printf("mq_unlink PIC2MOTOR_QUEUE not success, errno = %d\n", errno);
        mq_close(motor_ctx.pic2motor_queue);
        mq_unlink(MOTOR2PIC_QUEUE);
        return -1;
    }
    if (mq_close(motor_ctx.motor2pic_queue) == -1)
    {
        printf("mq_close motor2pic_queue not success, errno = %d\n", errno);
        return -1;
    }
    if (mq_unlink(MOTOR2PIC_QUEUE) == -1)
    {
        printf("mq_unlink MOTOR2PIC_QUEUE not success, errno = %d\n", errno);
        return -1;
    }
	return 0;
}