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

#define DEFAULT_VALUE 0

#define MOTOR_MOVE		0x3
#define MOTOR_RESET		0x2

#define STEP_SIZE 260
#define MOTOR_ZERO_POINT 0
#define LIMIT_STEP 2600

struct mq_attr attributes_for_motor2pic_queue = 
{
    .mq_flags = DEFAULT_VALUE,
    .mq_maxmsg = MQ_MAX_NUM_OF_MESSAGES,
    .mq_msgsize = sizeof(motor2pic_t),
    .mq_curmsgs = DEFAULT_VALUE
};

struct mq_attr attributes_for_pic2motor_queue = 
{
    .mq_flags = DEFAULT_VALUE,
    .mq_maxmsg = MQ_MAX_NUM_OF_MESSAGES,
    .mq_msgsize = sizeof(pic2motor_t),
    .mq_curmsgs = DEFAULT_VALUE
};

typedef enum _motor_status_t
{
    MOTOR_IS_STOP,
    MOTOR_IS_RUNNING,
} motor_status;

typedef struct  {
    int x;
    int y;
    motor_status status;
    int speed;
} motor_message;

typedef struct _motor_steps_t
{
    int x;
    int y;
} motor_steps_t;

//struct sigevent sigev;
typedef struct _motor_context_t
{
    motor2pic_t msg_m2p;
    pic2motor_t msg_p2m;
	int cur_step;
	int step;
	mqd_t pic2motor_queue;
	mqd_t motor2pic_queue;
} motor_context_t;
motor_context_t motor_ctx = {0};

void here_are_am(const char *asker_func_name)
{
    printf("я зашел в %s\n", asker_func_name);
}

/*void sig_receive_message()
{
    if (mq_receive(chat_queue, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == sizeof(struct_to_receive))
    {
        mq_notify(chat_queue, &sigev);
    }
    if (struct_to_receive.sender.client_pid != client_pid)
    {
        printf("%s: %s\n", struct_to_receive.sender.client_name, struct_to_receive.server_to_client_msg);
    } 
}*/

int send_motor2pic_enum()
{
    here_are_am(__FUNCTION__);
	if (mq_send(motor_ctx.motor2pic_queue, (char *)&motor_ctx.msg_m2p, sizeof(motor_ctx.msg_m2p), PRIORITY_OF_QUEUE) == -1)
    {
        printf("mq_send step not success, errno = %d\n", errno);
        return -1;
    }
    printf("Сообщение №%d с действием %d успешно отправлено\n", motor_ctx.msg_m2p.number_of_comand_m2p, motor_ctx.msg_m2p.action_m2p);
    motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_INVALID_TYPE;
    return 0;
}

int create_queue_pic2motor()
{
    here_are_am(__FUNCTION__);
    motor_ctx.pic2motor_queue = mq_open(PIC2MOTOR_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_pic2motor_queue);
    if (motor_ctx.pic2motor_queue == -1)
    {
    	printf("ошибка открытия очереди pic2motor_queue, errno = %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("очередь pic2motor_queue успешнот открыта, ФД = %d\n", (int)motor_ctx.pic2motor_queue);
    return 0;
}

int create_queue_motor2pic()
{
	here_are_am(__FUNCTION__);
	motor_ctx.motor2pic_queue = mq_open(MOTOR2PIC_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_motor2pic_queue);
    if (motor_ctx.motor2pic_queue == -1)
    {
    	printf("ошибка открытия очереди motor2pic_queue, errno = %d\n", errno);
        exit(EXIT_FAILURE);
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
        close(fd);
    	return -1;
    }
    close(fd);
    return 0;
}

int set_movement()
{
	here_are_am(__FUNCTION__);
    motor_steps_t motor_move;
    motor_move.x = STEP_SIZE;
	motor_move.y = 0;
	motor_ctx.cur_step += STEP_SIZE;
	if (motor_ctx.cur_step >= LIMIT_STEP)
	{
        motor_ctx.msg_m2p.number_of_comand_m2p = motor_ctx.msg_p2m.number_of_comand_p2m;
		motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_END_OF_ENUM;
	    send_motor2pic_enum();
	    return 0;
	}
	if (send_command2motor(MOTOR_MOVE, &motor_move) == 0)
	{
        motor_ctx.msg_m2p.number_of_comand_m2p = motor_ctx.msg_p2m.number_of_comand_p2m;
        motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_STEP;
	    send_motor2pic_enum();
	    return 0;
	}
	else
	{
        motor_ctx.msg_m2p.number_of_comand_m2p = motor_ctx.msg_p2m.number_of_comand_p2m;
		motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_INVALID_TYPE;
	    send_motor2pic_enum();
    	return -1;
	}
}

int calibration()
{
	here_are_am(__FUNCTION__);
    motor_steps_t motor_move;
    motor_move.x = -LIMIT_STEP;
    motor_move.y = 0;
    motor_ctx.cur_step = MOTOR_ZERO_POINT;
    if (send_command2motor(MOTOR_MOVE, &motor_move) == 0)
    {
        motor_ctx.msg_m2p.number_of_comand_m2p = motor_ctx.msg_p2m.number_of_comand_p2m;
    	motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_CALIBRATION;
	    send_motor2pic_enum();
	    return 0;
    }
    else
    {
        motor_ctx.msg_m2p.number_of_comand_m2p = motor_ctx.msg_p2m.number_of_comand_p2m;
    	motor_ctx.msg_m2p.action_m2p = CAM2MOTOR_ACTION_INVALID_TYPE;
	    send_motor2pic_enum();
    	return -1;
    }
}

int receive_pic2motor_enum()
{
	here_are_am(__FUNCTION__); 
    while (1)
    {
        int size_receive = mq_receive(motor_ctx.pic2motor_queue, (char *)&motor_ctx.msg_p2m, sizeof(motor_ctx.msg_p2m), NULL);
        if (size_receive < 0)
        {
            printf("размер сообщения меньше 0 байт. errno = %d\n", errno);
            printf("повтор попытки\n");
            sleep(3);
            continue;
        }
        if (size_receive != sizeof(motor_ctx.msg_p2m))
        {
            printf("размер полученного сообщения меньше ожидаемого. errno = %d\n", errno);
            printf("повтор попытки\n");
            sleep(3);
            continue;
        }

        switch(motor_ctx.msg_p2m.action_p2m)
        {
        	case CAM2MOTOR_ACTION_CALIBRATION:
    		{
    			calibration();
    			break;
    		}
    		case CAM2MOTOR_ACTION_STEP:
			{
				set_movement();
				break;
			}
			case CAM2MOTOR_ACTION_EXIT:
			{
                printf("поступила команда выход\n");
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