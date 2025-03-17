#include <string.h>
#include <unistd.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <stdio.h>
#include <errno.h>
#include "../linux/include/uapi/linux/mcheck.h"


struct nl_sock *sock;
int nl_fam;
static int send_u32_msg(struct nl_sock *sk, int fam, unsigned int value);

unsigned int analyze(unsigned long addr) {
    if (strncmp((char *) addr, "virus", strlen("virus")) == 0) {
        return 1;
    }

    printf("first 10 bytes: ");
    for (int i = 0; i < 10 ; i++) {
        printf("%hhx ", ((char *) addr)[i]);
    }
    printf("\n");
    return 0;
}

static int message_handler(struct nl_msg *msg, void *arg)
{
    int		   err	   = 0;
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr	  *tb[3];

	/* Parse the attributes */
	err = nla_parse(tb, LSM_ATTR_MAX, genlmsg_attrdata(genlhdr, 0),
			genlmsg_attrlen(genlhdr, 0), NULL);
	if (err) {
		printf("unable to parse message\n");
		return NL_SKIP;
	}
	/* Check that there's actually a payload */
	if (!tb[1]) {
		printf("address missing from message\n");
		return NL_SKIP;
	}

    unsigned long addr = (unsigned long) nla_get_u64(tb[1]);

    send_u32_msg(sock, nl_fam, analyze(addr));
    return NL_OK;
}

static int send_u32_msg(struct nl_sock *sk, int fam, unsigned int value)
{
	int	       err = 0;
	struct nl_msg *msg = nlmsg_alloc();
	if (!msg) {
		return -ENOMEM;
	}

	/* Put the genl header inside message buffer */
	void *hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, fam, 0, 0, LSM_CMD_REPLY, 1);
	if (!hdr) {
		return -EMSGSIZE;
	}

	err = nla_put_u32(msg, LSM_ATTR_RESPONSE, value);
	if (err < 0) {
		return -err;
	}

	/* Send the message. */
	err = nl_send_auto(sk, msg);
	err = err >= 0 ? 0 : err;

	nlmsg_free(msg);

	return err;
}

int main(void) {

    unsigned int my_pid =  (unsigned int) getpid();
    printf("my pid: %u\n", my_pid);
    sock = nl_socket_alloc();
    if (!sock) {
		return -1;
	}

    if (genl_connect(sock) < 0) {
        printf("failed to connect to generic netlink\n");
        return -1;
    }

    nl_fam = genl_ctrl_resolve(sock, GEN_NETLINK_FAMILY_NAME);
    if (nl_fam < 0) {
        printf("failed to resolve family name\n");
        return -1;
    }

    nl_socket_disable_seq_check(sock);

    int mcgrp = genl_ctrl_resolve_grp(sock, GEN_NETLINK_FAMILY_NAME, GEN_NETLINK_GROUP_NAME);
    if (mcgrp < 0) {
        printf("failed to resolve multicast group\n");
        return -1;
    }

    if (nl_socket_add_membership(sock, mcgrp) < 0) {
        printf("failed to add membership\n");
        return -1;
    }

    if (nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, message_handler, NULL) < 0) {
        printf("failed to modify callback\n");
        return -1;
    }

    // message for the lsm to register my pid
    int err = send_u32_msg(sock, nl_fam, my_pid);
    if (err) {
        printf("error sending message: %s\n", nl_geterror(err));
    }

    printf("listening for messages\n");

    while (1) {
        nl_recvmsgs_default(sock);
    }
    return 0;
}