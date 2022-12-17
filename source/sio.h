#ifndef SIO__
#define SIO__

#define SIO_32 1
#define SIO_8 0

#define SIO_MASTER 1
#define SIO_SLAVE 0

#define SIO_DEFAULT_VALUE 0xFE

void init_sio_normal(int is_master, int is_32);
int sio_normal(int data, int is_master, int is_32);
void sio_normal_prepare_irq_slave(int data, int is_32);
int timed_sio_normal_master(int data, int is_32, int vCountWait);
void sio_handle_irq_slave();
void sio_stop_irq_slave();

#endif
