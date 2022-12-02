#include "head_motor.h"
#include <stdio.h>
#include <unistd.h>
#include "head_to_lib.h"

#define EXIT_NUMBER 2

int you_choice()
{
    int choice;
    printf("Выберете действие:\n%d: калибровка;\n%d: шаг\n%d: конец\n", CAM2MOTOR_ACTION_CALIBRATION, CAM2MOTOR_ACTION_STEP, CAM2MOTOR_ACTION_EXIT);
    scanf("%d%*c", &choice);
    switch (choice)
    {
        case CAM2MOTOR_ACTION_CALIBRATION:
        {
            calibration();
            break;
        }
        case CAM2MOTOR_ACTION_STEP:
        {
            step();
            break;
        }
        case CAM2MOTOR_ACTION_EXIT:
        {
            goodbye_motor();
            return EXIT_NUMBER;
        }
        default:
        {
            printf("получено неизвестное сообщение\n");
            break;
        }
    }
    return 0;
}

int main()
{
    int ret;
    init_contol_motor();
    while (ret != EXIT_NUMBER)
    {
        ret = you_choice();
    }
    return 0;
}