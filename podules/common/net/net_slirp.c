#include <slirp/libslirp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "net_slirp.h"
#include "queue.h"

struct net_slirp_timer
{
	struct net_slirp_timer *next;
	uint64_t expire_time;
	SlirpTimerCb handler;
	void *opaque;
};

typedef struct net_slirp_t
{
	Slirp * slirp;
	pthread_t poll_thread;
	volatile int exit_poll;
	queueADT slirpq;
	struct net_slirp_timer * timer_head;
	fd_set rfds, wfds, xfds;
	int maxfd;
} net_slirp_t;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static int net_slirp_add_poll(int fd, int events, void * opaque)
{
	struct net_slirp_t * slirp = opaque;

	if (events & SLIRP_POLL_IN)
		FD_SET(fd, &(slirp->rfds));
	if (events & SLIRP_POLL_OUT)
		FD_SET(fd, &(slirp->wfds));
	if (events & SLIRP_POLL_PRI)
		FD_SET(fd, &(slirp->xfds));
	if (slirp->maxfd < fd)
		slirp->maxfd = fd;

	return fd;
}

static int net_slirp_get_revents(int idx, void * opaque)
{
	struct net_slirp_t * slirp = opaque;
	int event = 0;
	if (FD_ISSET(idx, &(slirp->rfds)))
		event |= SLIRP_POLL_IN;
	if (FD_ISSET(idx, &(slirp->wfds)))
		event |= SLIRP_POLL_OUT;
	if (FD_ISSET(idx, &(slirp->xfds)))
		event |= SLIRP_POLL_PRI;

	return event;
}

static void *net_slirp_poll_thread(void *p)
{
	net_slirp_t *slirp = p;

	while (!slirp->exit_poll)
	{
		int ret2;
		struct timeval tv;
		unsigned int timeout;

		slirp->maxfd = -1;
		FD_ZERO(&(slirp->rfds));
		FD_ZERO(&(slirp->wfds));
		FD_ZERO(&(slirp->xfds));

		timeout = 500;
		slirp_pollfds_fill(slirp->slirp, &timeout, net_slirp_add_poll, slirp);

		tv.tv_sec = 0;
		tv.tv_usec = timeout;

		ret2 = select(slirp->maxfd + 1, &(slirp->rfds), &(slirp->wfds),
					  &(slirp->xfds), &tv);

		slirp_pollfds_poll(slirp->slirp, ret2<0, net_slirp_get_revents, slirp);
	}

	return NULL;
}


static int net_slirp_read(net_t *net, packet_t *packet)
{
	net_slirp_t *slirp = net->p;
	int ret = -1;

	pthread_mutex_lock(&queue_mutex);
	if (QueuePeek(slirp->slirpq) > 0)
	{
		// aeh54_log("> net_slirp_read\n");
		struct queuepacket *qp = QueueDelete(slirp->slirpq);

		packet->p = qp;
		packet->data = qp->data;
		packet->len = qp->len;

		ret = 0;
		// aeh54_log("< net_slirp_read %d @ %p\n", packet->len, packet->data);
	}
	pthread_mutex_unlock(&queue_mutex);

	return ret;
}

static void net_slirp_write(net_t *net, uint8_t *data, int size)
{
	net_slirp_t *slirp = net->p;
	// aeh54_log("> net_slirp_write %d @ %p\n", size, data);
	slirp_input(slirp->slirp, data, size);
	// aeh54_log("< net_slirp_write\n");
}

static void net_slirp_free(net_t *net, packet_t *packet)
{
	packet->data = NULL;
	free(packet->p);
}

ssize_t net_slirp_output(const void *buf, size_t len, void * opaque)
{
	net_slirp_t * slirp = opaque;
	struct queuepacket *p;
	p = (struct queuepacket *)malloc(sizeof(struct queuepacket));
	memcpy(p->data, buf, len);
	p->len = len;
	// aeh54_log("net_slirp recv %ld bytes\n", p->len);
	pthread_mutex_lock(&queue_mutex);
	QueueEnter(slirp->slirpq, p);
	pthread_mutex_unlock(&queue_mutex);

	return len;
}

void net_slirp_close(net_t *net)
{
	net_slirp_t *slirp = net->p;

	slirp->exit_poll = 1;
	pthread_join(slirp->poll_thread, NULL);

	slirp_cleanup(slirp->slirp);

	free(slirp);
	free(net);
}

static int64_t net_slirp_clock_get_ns(void *opaque)
{
	(void) opaque;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void *net_slirp_timer_new(SlirpTimerCb cb, void *cb_opaque, void *opaque)
{
	struct net_slirp_t *slirp = opaque;
	struct net_slirp_timer *qt = malloc(sizeof(*qt));
	if (qt)
	{
		qt->next = slirp->timer_head;
		qt->expire_time = -1;
		qt->handler = cb;
		qt->opaque = cb_opaque;
		slirp->timer_head=qt;
	}
	return qt;
}

static void net_slirp_timer_free(void *timer, void *opaque)
{
	struct net_slirp_t *slirp = opaque;
	struct net_slirp_timer *qt = timer;
	struct net_slirp_timer **scan  = &(slirp->timer_head);
	while (*scan != NULL && *scan != qt) {
		 scan = &((*scan) ->next);
	}

	if (*scan)
	{
		*scan = qt->next;
		free(qt);
	}
}

static void net_slirp_timer_mod(void *timer, int64_t expire_time, void *opaque)
{
	(void) opaque;
	struct net_slirp_timer *qt = timer;
	qt->expire_time = expire_time;
}

static void net_slirp_register_poll_fd(int fd, void *opaque)
{
	(void) fd;
	(void) opaque;
}

static void net_slirp_unregister_poll_fd(int fd, void *opaque)
{
	(void) fd;
	(void) opaque;
}

static void net_slirp_notify(void *opaque)
{
	(void) opaque;
}

SlirpCb callbacks = {
	.send_packet = net_slirp_output,
	.clock_get_ns = net_slirp_clock_get_ns,
	.timer_new = net_slirp_timer_new,
	.timer_free = net_slirp_timer_free,
	.timer_mod = net_slirp_timer_mod,
	.register_poll_fd = net_slirp_register_poll_fd,
	.unregister_poll_fd = net_slirp_unregister_poll_fd,
	.notify = net_slirp_notify,
};

net_t *slirp_net_init(void)
{
	int rc;
	net_t *net = malloc(sizeof(*net));
	net_slirp_t *slirp = malloc(sizeof(*slirp));

	memset(net, 0, sizeof(*net));
	memset(slirp, 0, sizeof(*slirp));

	SlirpConfig config;
	config.version = 1;
	config.in_enabled = true;
	config.restricted = 0;
	config.in_enabled = 1;
	inet_pton(AF_INET,"10.0.2.0", &(config.vnetwork));
	inet_pton(AF_INET,"255.255.255.0", &(config.vnetmask));
	inet_pton(AF_INET,"10.0.2.2", &(config.vhost));
	config.in6_enabled = 0;
	config.vhostname = "slirp";
	config.tftp_server_name = NULL;
	config.tftp_path = NULL;
	config.bootfile = NULL;
	inet_pton(AF_INET,"10.0.2.15", &(config.vdhcp_start));
	inet_pton(AF_INET,"10.0.2.3", &(config.vnameserver));
	config.vdnssearch = NULL;
	config.vdomainname = NULL;
	config.if_mtu = 0; // IF_MTU_DEFAULT
	config.if_mru = 0; // IF_MTU_DEFAULT
	config.disable_host_loopback = 0;

	slirp->slirp = slirp_new(&config, &callbacks, slirp);

	struct in_addr hostaddr = {.s_addr = INADDR_ANY };
	rc = slirp_add_hostfwd(slirp->slirp, true, hostaddr, 32768,
			       config.vdhcp_start, 32768);
	if (rc != 0) {
		printf("Could not forward port 32768 - AUN will not function\n");
	}

	net->close = net_slirp_close;
	net->read = net_slirp_read;
	net->write = net_slirp_write;
	net->free = net_slirp_free;

	net->p = slirp;

	slirp->slirpq = QueueCreate();
	pthread_create(&slirp->poll_thread, 0, net_slirp_poll_thread, slirp);

	return net;
}
