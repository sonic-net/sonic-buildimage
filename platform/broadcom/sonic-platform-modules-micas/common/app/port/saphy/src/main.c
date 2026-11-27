#include "aperta_phy.h"
#include "mdio.h"
#include "common.h"

#define EXTPHY_APIS_NUMBER (1)
extern phy_apis_t extphy_aperta_apis;

static phy_apis_t *phy_apis[EXTPHY_APIS_NUMBER] = {
    &extphy_aperta_apis
};

static bool g_platform_inited[EXTPHY_APIS_NUMBER] = {0};

int phy_platform_init(int unit)
{
    int i, rv;
    int platform_cnt = 0;
    int platform_init_cnt = 0;
    LOG_INFO("phy_platform_init start.");
    for(i = 0; i < EXTPHY_APIS_NUMBER; i++) {
        if (phy_apis[i]->phy_platform_init == NULL) {
            LOG_INFO("chip:%s phy_platform_init is null.", phy_apis[i]->type);
            continue;
        }
        rv = phy_apis[i]->phy_platform_init(unit);
        if (rv != STATUS_NOT_SUPPORTED) {
            platform_cnt++;
        }
        if (rv == STATUS_SUCCESS) {
            platform_init_cnt++;
            g_platform_inited[i] = true;
        } else if (rv == STATUS_FAILURE) {
            LOG_INFO("phy_platform_init failed, platform_id: %d", i);
        }
    }
    LOG_INFO("phy_platform_init end.");
    if (platform_cnt) {
        if (platform_cnt == platform_init_cnt) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_FAILURE;
        }
    } else {
        return STATUS_NOT_SUPPORTED;
    }
}

int main(int argc, char **argv)
{
    int rv = 0;
    common_init();
    mdio_init();
    rv = phy_platform_init(0);
    do {
        sleep(1);
    } while (!rv);
}

