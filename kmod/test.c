#include <linux/kernel.h>
#include <linux/module.h>

/* small hack for prepending module name to dmesg output */
#ifdef pr_fmt
#   undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt "\n"


static int __init
test_init(void)
{
    pr_info("called %s", __FUNCTION__);
    return 0;
}

static void __exit
test_exit(void)
{
    pr_info("called %s", __FUNCTION__);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
