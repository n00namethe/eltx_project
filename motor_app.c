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

#define MOTOR_MOVE		    0x3
#define MOTOR_GET_STATUS    0x4

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

typedef enum _motor_status_e
{
    MOTOR_IS_STOP,
    MOTOR_IS_RUNNING,
} motor_status_e;

typedef struct  {
    int x;
    int y;
    motor_status_e status;
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

int get_status() 
{
    motor_message status;
    send_command2motor(MOTOR_GET_STATUS, &status);
    return status.x;
}

int send_motor2pic_reply(motor2pic_t *to_send)
{
    here_are_am(__FUNCTION__);
    to_send->motor_status = get_status();
    if (mq_send(motor_ctx.motor2pic_queue, (char *)to_send, sizeof(motor2pic_t), PRIORITY_OF_QUEUE) == -1)
    {
        printf("mq_send motor2pic_queue not success, errno = %d\n", errno);
        return -1;
    }
    printf("Сообщение №%d с действием %d успешно отправлено\nРазмер отправленного сообщения: %d\n", \
           to_send->number_of_comand_m2p, to_send->action_m2p, sizeof(motor2pic_t));
    return 0;
}

int set_movement(pic2motor_t *move_p2m)
{
	here_are_am(__FUNCTION__);
    motor2pic_t move_m2p = {0};
    motor_steps_t motor_move;
    motor_move.x = 1;
	motor_move.y = 0;
	if (get_status() >= LIMIT_STEP)
	{
        move_m2p.number_of_comand_m2p = move_p2m->number_of_comand_p2m;
		move_m2p.action_m2p = CAM2MOTOR_ACTION_END_OF_ENUM;
	    send_motor2pic_reply(&move_m2p);
	    return 0;
	}
    int i;
    for (i = 0; i < move_p2m->make_steps; i++)
    {
        if (send_command2motor(MOTOR_MOVE, &motor_move) != 0)
        {
            move_m2p.number_of_comand_m2p = move_p2m->number_of_comand_p2m;
            move_m2p.action_m2p = CAM2MOTOR_ACTION_INVALID_TYPE;
            send_motor2pic_reply(&move_m2p);
            return -1;
        }
    }
    move_m2p.number_of_comand_m2p = move_p2m->number_of_comand_p2m;
    move_m2p.action_m2p = CAM2MOTOR_ACTION_STEP;
    send_motor2pic_reply(&move_m2p);
    return 0;
}

int calibration(pic2motor_t *calibration_p2m)
{
	here_are_am(__FUNCTION__);
    motor2pic_t calibration_m2p = {0};
    motor_steps_t motor_move;
    motor_move.x = -1;
    motor_move.y = 0;
    int i;
    for (i = 0; i < LIMIT_STEP; i++)
    {
        if (send_command2motor(MOTOR_MOVE, &motor_move) != 0)
        {
            calibration_m2p.number_of_comand_m2p = calibration_p2m->number_of_comand_p2m;
            calibration_m2p.action_m2p = CAM2MOTOR_ACTION_INVALID_TYPE;
            send_motor2pic_reply(&calibration_m2p);
            return -1;
        }
    }
    calibration_m2p.number_of_comand_m2p = calibration_p2m->number_of_comand_p2m;
    calibration_m2p.action_m2p = CAM2MOTOR_ACTION_CALIBRATION;
    send_motor2pic_reply(&calibration_m2p);
    return 0;
}

void receive_pic2motor_request(pic2motor_t *to_receive)
{
    here_are_am(__FUNCTION__);
    while(1)
    {
        int size_receive = mq_receive(motor_ctx.pic2motor_queue, (char *)to_receive, sizeof(pic2motor_t), NULL);
        if (size_receive < 0)
        {
            printf("размер сообщения меньше 0 байт. errno = %d\n", errno);
            printf("повтор попытки\n");
            sleep(3);
            continue;
        }
        if (size_receive != sizeof(pic2motor_t))
        {
            printf("размер полученного сообщения меньше ожидаемого. errno = %d\n", errno);
            printf("повтор попытки\n");
            sleep(3);
            continue;
        }
        printf("я принял сообщение №%d с действием %d\nКоличество шагов = %d\nРазмер полученного сообщения: %d\n", \
               to_receive->number_of_comand_p2m, to_receive->action_p2m, to_receive->make_steps, sizeof(pic2motor_t));
        break;
    }
}

int pic2motor_request()
{
	here_are_am(__FUNCTION__); 
    while (1)
    {
        pic2motor_t receive_request = {0};
        receive_pic2motor_request(&receive_request);
        switch(receive_request.action_p2m)
        {
        	case CAM2MOTOR_ACTION_CALIBRATION:
    		{
    			if (calibration(&receive_request) != 0)
                {
                    printf("Калибровка не состоялась, закрываю очереди\n");
                    return 0;
                }
    			break;
    		}
    		case CAM2MOTOR_ACTION_STEP:
			{
				if (set_movement(&receive_request) != 0)
                {
                    printf("Пошагать не получилось, закрываю очереди\n");
                    return 0;
                }
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

void init_queues()
{
    create_queue_pic2motor();
    create_queue_motor2pic();
}

int main()
{
	init_queues();
	pic2motor_request();
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