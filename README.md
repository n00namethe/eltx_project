Пока не доделал мейк компилировать так:  
  
toolchain/bin/mips-linux-gnu-gcc -c libcontrol_motor.c  -lrt -lm  
ar rc libjust_test.a libcontrol_motor.o  
make motor_app  
make api_motor_mq  

спасибо за внимание.
