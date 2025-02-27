#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <stdio.h>

#define GEN_NETLINK_FAMILY_NAME "lsm_netlink"

static int message_handler(struct nl_msg *msg, void *arg) {
    int		   err	   = 0;
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr	  *tb[3];

	/* Parse the attributes */
	err = nla_parse(tb, 2, genlmsg_attrdata(genlhdr, 0),
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

	printf("message received: %lx\n", (unsigned long) nla_get_u32(tb[1]));
    return NL_OK;
}

int main(void) {
    struct nl_sock *sock = nl_socket_alloc();
    if (!sock) {
		return -1;
	}

    if (genl_connect(sock) < 0) {
        printf("failed to connect to generic netlink\n");
        return -1;
    }

    int fam = genl_ctrl_resolve(sock, GEN_NETLINK_FAMILY_NAME);
    if (fam < 0) {
        printf("failed to resolve family name\n");
        return -1;
    }

    nl_socket_disable_seq_check(sock);

    int mcgrp = genl_ctrl_resolve_grp(sock, GEN_NETLINK_FAMILY_NAME, "lsm_mc_group");
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

    printf("listening for messages\n");

    while (1) {
        nl_recvmsgs_default(sock);
    }
    return 0;
}