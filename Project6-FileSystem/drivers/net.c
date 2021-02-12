#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

#define STEP 1

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // Add BDs
    int _num_packet = num_packet;
    while(_num_packet > 0)
    {
        int num = (_num_packet > STEP) ? STEP : _num_packet;
        EmacPsRecv(&EmacPsInstance, 
                kva2pa(rx_buffers), num); 
        EmacPsWaitRecv(&EmacPsInstance,
                num, rx_len);
        // Copy to user
        for (int i = 0; i < num; i++){
            kmemcpy(addr, rx_buffers + i, rx_len[i]);
            *frLength = rx_len[i];
            frLength ++;
            addr += rx_len[i];
        }
        _num_packet -= STEP;
    }
    return 1;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // Copy to `buffer'
    kmemcpy(&tx_buffer, addr, length);
    // send packet
    EmacPsSend(&EmacPsInstance, kva2pa(&tx_buffer), length);
    EmacPsWaitSend(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    // turn on/off network driver's interrupt mode
    set_irq_mode(&EmacPsInstance, mode);
}
