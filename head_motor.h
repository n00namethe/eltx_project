#pragma once

#define PIC2MOTOR_QUEUE "/Queue_pic2motor"
#define MOTOR2PIC_QUEUE "/Queue-motor2pic"
#define MQ_MAX_NUM_OF_MESSAGES 10
#define DEFAULT_VALUE 0
#define PRIORITY_OF_QUEUE 1

typedef enum _motor_action_t
{
    CAM2MOTOR_ACTION_INVALID_TYPE = -1,
    CAM2MOTOR_ACTION_CALIBR = 0,
    CAM2MOTOR_ACTION_STEP,
    CAM2MOTOR_ACTION_EXIT,
    CAM2MOTOR_ACTION_END_OF_ENUM
} motor_action_t;


struct mq_attr attributes_for_motor_queue = 
{
        .mq_flags = DEFAULT_VALUE,
        .mq_maxmsg = MQ_MAX_NUM_OF_MESSAGES,
        .mq_msgsize = sizeof(motor_action_t),
        .mq_curmsgs = DEFAULT_VALUE
};