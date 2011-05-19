/*
 * Testing TCP protocol: server side.
 */
#include <runtime/lib.h>
#include <stream/stream.h>
#include <mem/mem.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/arp.h>
#include <timer/timer.h>
#include <elvees/eth.h>

#ifdef ENABLE_DCACHE
#   define SDRAM_START	0x80000000
#else
#   define SDRAM_START	0xA0000000
#endif
#define SDRAM_SIZE	(64*1024*1024)

ARRAY (stack_tcp, 1500);
ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));
mem_pool_t pool;
arp_t *arp;
eth_t eth_data, *eth = &eth_data;
route_t route;
timer_t timer;
ip_t ip;
tcp_socket_t *user_socket;

#define PORT 		0xBBBB
#define MSG_SIZE	256
int     msg[MSG_SIZE];

void tcp_task (void *data)
{
	tcp_socket_t *lsock;
	int sz;
	int count = 0, old_count = 0, i;
	unsigned long start = 0, end, elapsed;

	lsock = tcp_listen (&ip, 0, PORT);
	if (! lsock) {
		printf (&debug, "Error on listen, aborted\n");
		uos_halt (0);
	}
	for (;;) {
		printf (&debug, "Server waiting on port %d...\n", PORT);
		printf (&debug, "Free memory: %d bytes\n", mem_available (&pool));
		user_socket = tcp_accept (lsock);
		if (! user_socket) {
			printf (&debug, "Error on accept\n");
			uos_halt (0);
		}
		debug_printf ("Client connected\n");

		for (;;) {
			sz = tcp_read (user_socket, msg, sizeof(msg));

			if (sz < 0) break;
//debug_printf ("count = %d, old_count = %d\n", count, old_count);

			sz >>= 2;

			for (i = 0; i < sz; ++i) {
				if (msg[i] != count) {
					debug_printf("bad counter: %d, expected: %d\n", msg[i], count);
					count = msg[i] + 1;
					continue;
				}
				++count;
			}

			if ((count - old_count) >= (256 * sizeof(msg))) {
				end = timer_milliseconds (&timer);
				elapsed = end - start;
				debug_printf ("rcv rate: %ld (bytes/sec)\n", ((count - old_count) << 2) * 1000 / elapsed);
				old_count = count;
				start = end;
			}

		}
	
		tcp_close (user_socket);
		mem_free (user_socket);
		user_socket = 0;
	}
}

void uos_init (void)
{
	/* Configure 16 Mbyte of external Flash memory at nCS3. */
	MC_CSCON3 = MC_CSCON_WS (3);		/* Wait states  */

	/* Configure 64 Mbytes of external 32-bit SDRAM memory at nCS0. */
	MC_CSCON0 = MC_CSCON_E |		/* Enable nCS0 */
		MC_CSCON_WS (0) |		/* Wait states  */
		MC_CSCON_T |			/* Sync memory */
		MC_CSCON_CSBA (0x00000000) |	/* Base address */
		MC_CSCON_CSMASK (0xF8000000);	/* Address mask */

	MC_SDRCON = MC_SDRCON_PS_512 |		/* Page size 512 */
		MC_SDRCON_CL_3 |		/* CAS latency 3 cycles */
		MC_SDRCON_RFR (64000000/8192, MPORT_KHZ); /* Refresh period */

	MC_SDRTMR = MC_SDRTMR_TWR(2) |		/* Write recovery delay */
		MC_SDRTMR_TRP(2) |		/* Минимальный период Precharge */
		MC_SDRTMR_TRCD(2) |		/* Между Active и Read/Write */
		MC_SDRTMR_TRAS(5) |		/* Между * Active и Precharge */
		MC_SDRTMR_TRFC(15);		/* Интервал между Refresh */

	MC_SDRCSR = 1;				/* Initialize SDRAM */
        udelay (2);

#if 0
	/* Используем внешнюю память SRAM. */
	mem_init (&pool, SDRAM_START, SDRAM_START + SDRAM_SIZE);
#else
	/* Используем только внутреннюю память CRAM.
	 * Оставляем 256 байтов для задачи "idle". */
	extern unsigned __bss_end[], _estack[];
	mem_init (&pool, (unsigned) __bss_end, (unsigned) _estack - 256);
#endif
	timer_init (&timer, KHZ, 50);

	/*
	 * Create a group of two locks: timer and eth.
	 */
	mutex_group_t *g = mutex_group_init (group, sizeof(group));
	mutex_group_add (g, &eth->netif.lock);
	mutex_group_add (g, &timer.decisec);

	arp = arp_init (arp_data, sizeof(arp_data), &ip);
	ip_init (&ip, &pool, 70, &timer, arp, g);

	/*
	 * Create interface eth0
	 */
	const unsigned char my_macaddr[] = { 0, 9, 0x94, 0xf1, 0xf2, 0xf3 };
	eth_init (eth, "eth0", 80, &pool, arp, my_macaddr);

	unsigned char my_ip[] = { 192, 168, 1, 20 };
	route_add_netif (&ip, &route, my_ip, 24, &eth->netif);

	task_create (tcp_task, 0, "tcp", 20,
		stack_tcp, sizeof (stack_tcp));
}