#ifndef SIO__
#define SIO__

#define SIO_32 1
#define SIO_8 0

#define SIO_MASTER 1
#define SIO_SLAVE 0

void init_sio_normal(int is_master, int is_32);
int sio_normal(int data, int is_master, int is_32);
int timed_sio_normal_master(int data, int is_32, int vCountWait);

#endif