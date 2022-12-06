#ifndef HEAD_MOTOR_H
#define HEAD_MOTOR_H

#define PIC2MOTOR_QUEUE "/Queue_pic2motor"
#define MOTOR2PIC_QUEUE "/Queue_motor2pic"
#define MQ_MAX_NUM_OF_MESSAGES 10
#define PRIORITY_OF_QUEUE 1
#define SIZE_OF_MESSAGE 128
#define LIMIT_STEP 2600

typedef enum _motor_action_e
{
    CAM2MOTOR_ACTION_INVALID_TYPE = -1,
    CAM2MOTOR_ACTION_CALIBRATION = 0,
    CAM2MOTOR_ACTION_STEP,
    CAM2MOTOR_ACTION_EXIT,
    CAM2MOTOR_ACTION_END_OF_ENUM
} motor_action_e;

typedef struct _motor2pic_t
{
    motor_action_e action_m2p;
    int number_of_comand_m2p;
    int motor_status;
} motor2pic_t;

typedef struct _pic2motor_t
{
    motor_action_e action_p2m;
    int number_of_comand_p2m;
    int make_steps;
} pic2motor_t;
#endif