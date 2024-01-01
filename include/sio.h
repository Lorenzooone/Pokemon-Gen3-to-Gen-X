#ifndef SIO__
#define SIO__

#define SIO_32 1
#define SIO_8 0

#define SIO_MASTER 1
#define SIO_SLAVE 0

#define MAX_NUM_SLAVES 3

#define SIO_DEFAULT_VALUE 0xFE

void init_sio_normal(int, int);
void init_sio_multi(int);
int sio_normal(int, int, int, u8*);
void sio_normal_prepare_irq_slave(int);
int timed_sio_normal_master(int, int, int);
void timed_sio_multi_master(int, int, u16*);
void sio_handle_irq_slave(int);
void sio_stop_irq_slave(void);
int sio_read(u8);
u32 sio_send_if_ready_master(u32, u8, u8*);
u32 sio_send_master(u32 data, u8 is_32);

#endif
