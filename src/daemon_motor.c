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
#include <syslog.h>
#include <signal.h>
#include "motor_daemon.h"

#define MOTOR_DAEMON_LOG_ERR(...) syslog(LOG_ERR, ## __VA_ARGS__)
//#define DEBUG
#ifdef DEBUG
#define MOTOR_DAEMON_LOG_INFO(...) syslog(LOG_INFO, ## __VA_ARGS__)
#define MOTOR_DAEMON_LOG_NOTICE(...) syslog(LOG_NOTICE, ## __VA_ARGS__)
#else
#define MOTOR_DAEMON_LOG_INFO(...)
#define MOTOR_DAEMON_LOG_NOTICE(...)
#endif

#define DEFAULT_VALUE 0
#define MOTOR_MOVE          0x3
#define MOTOR_GET_STATUS    0x4

static int done = 0;
void signal_handler(int signum)
{
    MOTOR_DAEMON_LOG_NOTICE("Catch signal w/ id %d", signum);
    done = 1;
}

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

typedef struct _motor_context_t
{
    mqd_t pic2motor_queue;
    mqd_t motor2pic_queue;
} motor_context_t;
motor_context_t motor_ctx = {0};

#ifdef DEBUG
void here_are_am(const char *asker_func_name)
{
    printf("я зашел в %s\n", asker_func_name);
}
#endif

int create_queue_pic2motor()
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    motor_ctx.pic2motor_queue = mq_open(PIC2MOTOR_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_pic2motor_queue);
    if (motor_ctx.pic2motor_queue == -1)
    {
        MOTOR_DAEMON_LOG_ERR("ошибка открытия очереди pic2motor_queue, errno = %d\n", errno);
        exit(EXIT_FAILURE);
    }
    return 0;
}

int create_queue_motor2pic()
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    motor_ctx.motor2pic_queue = mq_open(MOTOR2PIC_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes_for_motor2pic_queue);
    if (motor_ctx.motor2pic_queue == -1)
    {
        MOTOR_DAEMON_LOG_ERR("ошибка открытия очереди motor2pic_queue, errno = %d\n", errno);
        exit(EXIT_FAILURE);
    }
    return 0;
}

int send_command2motor(int cmd, void *buffer)
{
    int fd = open("/dev/motor", O_WRONLY);
    if (ioctl(fd, cmd, buffer) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("send_command2motor not success, errno = %d\n", errno);
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
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    to_send->motor_status = get_status();
    if (mq_send(motor_ctx.motor2pic_queue, (char *)to_send, sizeof(motor2pic_t), PRIORITY_OF_QUEUE) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("mq_send motor2pic_queue not success, errno = %d\n", errno);
        return -1;
    }
    return 0;
}

int set_movement(pic2motor_t *move_p2m)
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
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
            MOTOR_DAEMON_LOG_INFO("step not success");
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
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
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

int receive_pic2motor_request(pic2motor_t *to_receive)
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    int size_receive = 0;
    while(done != 1)
    {
        size_receive = mq_receive(motor_ctx.pic2motor_queue, (char *)to_receive, sizeof(pic2motor_t), NULL);
        if (size_receive < 0)
        {
            sleep(3);
            continue;
        }
        if (size_receive != sizeof(pic2motor_t))
        {
            sleep(3);
            continue;
        }
        if (size_receive == -1)
        {
            return -1;
        }
        break;
    }
    MOTOR_DAEMON_LOG_INFO("я принял сообщение №%d с действием %d Количество шагов = %d Размер полученного сообщения: %ld ", \
                          to_receive->number_of_comand_p2m, to_receive->action_p2m, to_receive->make_steps, sizeof(size_receive));
    return 0;
}

int pic2motor_request()
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    while (done != 1)
    {
        pic2motor_t receive_request = {-1};
        receive_pic2motor_request(&receive_request);
        switch(receive_request.action_p2m)
        {
            case CAM2MOTOR_ACTION_CALIBRATION:
            {
                if (calibration(&receive_request) != 0)
                {
                    MOTOR_DAEMON_LOG_INFO("Калибровка не состоялась, закрываю очереди");
                    return 0;
                }
                break;
            }
            case CAM2MOTOR_ACTION_STEP:
            {
                if (set_movement(&receive_request) != 0)
                {
                    MOTOR_DAEMON_LOG_INFO("Пошагать не получилось, закрываю очереди");
                    return 0;
                }
                break;
            }
            case CAM2MOTOR_ACTION_EXIT:
            {
                MOTOR_DAEMON_LOG_INFO("поступила команда выход");
                return 0;
            }
            case CAM2MOTOR_ACTION_INVALID_TYPE:
            {
                MOTOR_DAEMON_LOG_ERR("Получен невалиный тип события, закрываю очереди");
                return 0;
            }
            default:
            {
                MOTOR_DAEMON_LOG_INFO("получен неопознанный тип события");
                break;
            }
        }
    }
    return 0;
}

void init_queues()
{
    create_queue_pic2motor();
    create_queue_motor2pic();
}

int close_queue()
{
    #ifdef DEBUG
    here_are_am(__FUNCTION__);
    #endif
    if (mq_close(motor_ctx.pic2motor_queue) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("mq_close serv_queue not success, errno = %d", errno);
        mq_close(motor_ctx.pic2motor_queue);
        mq_unlink(MOTOR2PIC_QUEUE);
        return -1;
    }
    if (mq_unlink(PIC2MOTOR_QUEUE) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("mq_unlink PIC2MOTOR_QUEUE not success, errno = %d", errno);
        mq_close(motor_ctx.pic2motor_queue);
        mq_unlink(MOTOR2PIC_QUEUE);
        return -1;
    }
    if (mq_close(motor_ctx.motor2pic_queue) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("mq_close motor2pic_queue not success, errno = %d", errno);
        return -1;
    }
    if (mq_unlink(MOTOR2PIC_QUEUE) == -1)
    {
        MOTOR_DAEMON_LOG_ERR("mq_unlink MOTOR2PIC_QUEUE not success, errno = %d", errno);
        return -1;
    }
    return 0;
}

void start_log()
{
    openlog("daemon_motor", LOG_PID, LOG_DAEMON);
    MOTOR_DAEMON_LOG_INFO("daemon started.");
}

void set_signal()
{
    MOTOR_DAEMON_LOG_NOTICE("triggered signal");
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    sigaction(SIGTERM, &action, NULL);
}

void daemonization()
{
    setsid();
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    start_log();
    set_signal();
}

void Daemon_motor_main()
{
    daemonization();
    init_queues();
    pic2motor_request();
    close_queue();
    MOTOR_DAEMON_LOG_INFO("daemon terminated.");
    closelog();
    exit(EXIT_SUCCESS);
}

void Daemon_motor_main_i()
{
    start_log();
    init_queues();
    pic2motor_request();
    close_queue();
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    pid_t parpid;

    if (argc < 2)
    {
        printf("Usage ./daemon -d for daemon or ./daemon -i for interactive\n");
        exit(1);
    }
    if (strcmp(argv[1], "-i") == 0)
    {
        printf("Запущен интерактивный режим работы демона\n");
        Daemon_motor_main_i();
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        if ((parpid = fork()) < 0) 
        {                  
            printf("\ncan't fork");
            exit(1);               
        }
        else if (parpid != 0)
        {
            exit(0);
        }
        Daemon_motor_main();
    }
    else
    {
        printf("Usage ./daemon -d for daemon or ./daemon -i for interactive\n");
        exit(1);
    }
    return 0;
}