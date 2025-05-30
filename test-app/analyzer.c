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
#include <yara.h>
#include "../linux/include/uapi/linux/mcheck.h"

struct nl_sock *sock;
int nl_fam;
static YR_RULES *yr_rules = NULL;
static int send_int_msg(struct nl_sock *sk, int fam, int value);

int (*analyze)(unsigned long addr, unsigned long len) = NULL;

int analyze_print(unsigned long addr, unsigned long len) {
    if (strncmp((char *) addr, "virus", strlen("virus")) == 0) {
        return 1;
    }

    printf("len = %lu; first 10 bytes: ", len);
    for (int i = 0; i < 10 ; i++) {
        printf("%hhx ", ((char *) addr)[i]);
    }
    printf("\n");
    return 0;
}

int analyze_nop(unsigned long addr, unsigned long len)
{
    return 0;
}

int analyze_liniar(unsigned long addr, unsigned long len)
{
    char *data = (char *) addr;
    uint32_t hash = 0x811c9dc5;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x01000193;
    }

    if (hash == 0xdeadbeef) {
        return 1;
    }
    return 0;
}

int yara_callback(YR_SCAN_CONTEXT *context, int message, void *message_data, void *user_data)
{
    if (message == CALLBACK_MSG_RULE_MATCHING) {
        YR_RULE *rule = (YR_RULE *) message_data;
        printf("YARA rule matched: %s\n", rule->identifier);
        *((int *) user_data) = 1;
        return CALLBACK_ABORT;
    }

    return CALLBACK_CONTINUE;
}
int analyze_yara(unsigned long addr, unsigned long len)
{
    int result = 0;
    yr_rules_scan_mem(
        yr_rules,
        (const uint8_t *) addr,
        len,
        0, // flags
        yara_callback,
        &result, // user_data
        0 // timeout
    );
    return result;
}

static int message_handler(struct nl_msg *msg, void *arg)
{
    int		   err	   = 0;
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr	  *tb[LSM_ATTR_MAX + 1];

	/* Parse the attributes */
	err = nla_parse(tb, LSM_ATTR_MAX, genlmsg_attrdata(genlhdr, 0),
			genlmsg_attrlen(genlhdr, 0), NULL);
	if (err) {
		printf("unable to parse message\n");
		return NL_SKIP;
	}
	/* Check that there's actually a payload */
	if (!tb[LSM_ATTR_ADDRESS]) {
		printf("address missing from message\n");
		return NL_SKIP;
	}
    if (!tb[LSM_ATTR_LENGTH]) {
        printf("length missing from message\n");
        return NL_SKIP;
    }

    unsigned long addr = (unsigned long) nla_get_u64(tb[LSM_ATTR_ADDRESS]);
    unsigned long len = (unsigned long) nla_get_u64(tb[LSM_ATTR_LENGTH]);

    int result = analyze(addr, len);

    send_int_msg(sock, nl_fam, result);
    return NL_OK;
}

static int send_int_msg(struct nl_sock *sk, int fam, int value)
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

	err = nla_put_s32(msg, LSM_ATTR_RESPONSE, value);
	if (err < 0) {
		return -err;
	}

	/* Send the message. */
	err = nl_send_auto(sk, msg);
	err = err >= 0 ? 0 : err;

	nlmsg_free(msg);

	return err;
}

int yara_init()
{
    if (yr_initialize() != ERROR_SUCCESS) {
        printf("Failed to initialize YARA\n");
        return -1;
    }
    YR_COMPILER *compiler;
    if (yr_compiler_create(&compiler) != ERROR_SUCCESS) {
        printf("Failed to create YARA compiler\n");
        return -1;
    }

    FILE *rules_file = fopen("rules.yar", "r");
    if (!rules_file) {
        printf("Failed to open rules file\n");
        yr_compiler_destroy(compiler);
        return -1;
    }

    if (yr_compiler_add_file(compiler, rules_file, NULL, NULL) != ERROR_SUCCESS) {
        printf("Failed to add rules file to YARA compiler\n");
        fclose(rules_file);
        yr_compiler_destroy(compiler);
        return -1;
    }
    fclose(rules_file);
    if (yr_compiler_get_rules(compiler, &yr_rules) != ERROR_SUCCESS) {
        printf("Failed to get YARA rules\n");
        yr_compiler_destroy(compiler);
        return -1;
    }
    yr_compiler_destroy(compiler);
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <analyze_type>\n", argv[0]);
        printf("analyze_type: print, nop, liniar, yara\n");
        return -1;
    }
    if (strcmp(argv[1], "print") == 0) {
        analyze = analyze_print;
    } else if (strcmp(argv[1], "nop") == 0) {
        analyze = analyze_nop;
    } else if (strcmp(argv[1], "liniar") == 0) {
        analyze = analyze_liniar;
    } else if (strcmp(argv[1], "yara") == 0) {
        analyze = analyze_yara;
    } else {
        printf("Unknown analyze type: %s\n", argv[1]);
        return -1;
    }

    if (yara_init() != 0) {
        printf("Failed to initialize YARA\n");
        return -1;
    }

    int my_pid = getpid();
    printf("my pid: %d\n", my_pid);
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
    int err = send_int_msg(sock, nl_fam, my_pid);
    if (err) {
        printf("error sending message: %s\n", nl_geterror(err));
    }

    printf("listening for messages\n");

    while (1) {
        nl_recvmsgs_default(sock);
    }
    return 0;
}