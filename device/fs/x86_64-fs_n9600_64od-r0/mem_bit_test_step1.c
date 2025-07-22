
/* 
24-04-19
1、完成初版，优化收包每个阶段10分钟以内
24-05-08
1、报文错误时只打印前10个报文
2、报文错误时，增加延时，规避爆缓存问题
3、增加版本信息打印
 */
cint_reset();


int bbshell(int unit, char *str)                                          /*H*/
{                                                                          /*H*/
    printf("B_CM.%d> %s\n", unit, str);                                      /*H*/
    return bshell(unit, str);                                                       /*H*/
}                                                                          /*H*/

int unit = 0;
bcm_field_group_t fp_group = 5;
bcm_field_entry_t fp_entry = 2048;
const char *platform = "n9600-64od";
const char *ver = "v0.2 24-05-08";

/* 
    配置端口的回环模式,为了避免模块影响. MAC回环,EDB回环
    BCM_PORT_LOOPBACK_NONE = 0, 
    BCM_PORT_LOOPBACK_MAC  = 1, 
    BCM_PORT_LOOPBACK_PHY  = 2, 
    BCM_PORT_LOOPBACK_PHY_REMOTE = 3, 
    BCM_PORT_LOOPBACK_MAC_REMOTE = 4, 
    BCM_PORT_LOOPBACK_EDB  = 5, 
    BCM_PORT_LOOPBACK_NIF = 6, 
    BCM_PORT_LOOPBACK_COUNT = 7  
*/
int test_port_lb = BCM_PORT_LOOPBACK_EDB;
bcm_mac_t dst_mac = {0x00, 0x01, 0x00, 0x00, 0x00, 0x02};
const int pkt_len = 9216;
uint8_t orig_pkt[pkt_len] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x01, 0x81, 0x00, 0x00, 0x64, 
};
const uint32_t smac_start = 6;
const uint32_t vlan_start = 15;
const uint32_t payload_start = 16;
const int base_vlan = 100;
// int     injection_error_count = 1;
// int     injection_error_pos = 200;
// uint8_t injection_error_var = 0x00;
const uint32_t test_duration = 300; // 秒

uint8_t rx_pkt_data[pkt_len];

typedef struct pkt_type_t{
    int tx_port;
    uint32_t payload;
    int pkt_len;
    int pkt_num_tx;
    int pkt_num_rx;
    int pkt_num_rx_err;
    // char* pkt_file;
    // uint8_t pkt_crc[4];
    bcm_mac_t src_mac;
};

int step1_pkt_type = 56;
int step2_pkt_type = 1;
int pkt_type = step1_pkt_type+step2_pkt_type;
int pkt_rx_idx = 0;
pkt_type_t pkt_list[pkt_type] = {
    // {0xFFFFFFFF, 100, 0, 0, "pkt_FF_9216.txt", {0x00, 0x01, 0x21,0xc5,0x4c,0x0c}},
    // {0xA5A5A5A5, 100, 0, 0, "pkt_A5_9216.txt", {0x00, 0x01, 0x4e,0x65,0xeb,0xe1}},
    // {0x5A5A5A5A, 100, 0, 0, "pkt_5A_9216.txt", {0x00, 0x01, 0x87,0xd7,0x13,0xa8}},
    // {0xFFFFFFFF, 1000, 0, 0, "pkt_FF_9216.txt", {0x00, 0x01, 0x21,0xc5,0x4c,0x0c}},
    /* 500个报文30秒 */
    /* tx_port, payload , pkt_len, pkt_num_tx, pkt_num_rx, pkt_num_rx_err, src_mac */
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x02}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x03}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x04}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x05}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x06}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x07}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x08}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x09}},
    {0, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0a}},

    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x11, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x12, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x13, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x14, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x15, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x16, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x17, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x18, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x19, 0xFF}},
    {32, 0xFFFFFFFF, 3758, 500, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0x1a, 0xFF}},

    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x01}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x02}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x03}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x04}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x05}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x06}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x07}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x08}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x09}},
    {0, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0xA5, 0x0a}},

    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x11, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x12, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x13, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x14, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x15, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x16, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x17, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x18, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x19, 0xA5}},
    {32, 0xA5A5A5A5, 3758, 500, 0, 0, {0x00, 0xA5, 0xA5, 0xA5, 0x1a, 0xA5}},

    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x01}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x02}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x03}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x04}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x05}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x06}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x07}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x08}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x09}},
    {0, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x0a}},

    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x01, 0x5A}},
    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x02, 0x5A}},
    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x03, 0x5A}},
    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x04, 0x5A}},
    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x05, 0x5A}},
    {32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x06, 0x5A}},
    //{32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x07, 0x5A}},
    //{32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x08, 0x5A}},
    //{32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x09, 0x5A}},
    //{32, 0x5A5A5A5A, 3758, 500, 0, 0, {0x00, 0x5A, 0x5A, 0x5A, 0x0a, 0x5A}},
    // 319960/32=9998.75
    // 9998/15=666.5个报文
    {0, 0xFFFFFFFF, 3758, 666, 0, 0, {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF1}},
};

// int step2_pkt_type = 1;
// int step2_pkt_rx_idx = 0;
// pkt_type_t step2_pkt_list[step2_pkt_type] = {
//     {0xFFFFFFFF, 100, 0, 0, "pkt_FF_9216.txt", {0x00, 0x01, 0x21,0xc5,0x4c,0x0c}},
// };

bcm_error_t vlan_create(int vid, int ingress_port, int egress_port, int eg_drop)
{
    bcm_pbmp_t pbmp;
    bcm_l2_addr_t addr;

    // printf("Ingress port: %d\n", ingress_port);
    // printf("Egress port: %d\n", egress_port);

    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_PORT_ADD(pbmp, ingress_port);
    BCM_PBMP_PORT_ADD(pbmp, egress_port);
    /* Create a vlan. */
    bcm_vlan_destroy(unit, vid);
    BCM_IF_ERROR_RETURN(bcm_vlan_create(unit, vid));

    /* Add vlan member ports. */
    BCM_IF_ERROR_RETURN(bcm_vlan_port_add(unit, vid, pbmp, pbmp));

    /* Set port default vlan id. */
    BCM_IF_ERROR_RETURN(bcm_port_untagged_vlan_set(unit, ingress_port, vid));
    //BCM_IF_ERROR_RETURN(bcm_port_untagged_vlan_set(unit, egress_port, vid));

    /* Add a L2 address on front panel port. */
    bcm_l2_addr_t_init(&addr, dst_mac, vid);
    addr.port = egress_port;

    // addr.flags |= BCM_L2_STATIC | BCM_L2_COPY_TO_CPU;
    addr.flags |= BCM_L2_STATIC;
    BCM_IF_ERROR_RETURN(bcm_l2_addr_add(unit, &addr));

    /* Drop egress_port received packet to avoid packet being forwarded again. */
    if (eg_drop) {
        BCM_IF_ERROR_RETURN(bcm_port_learn_set(unit, egress_port, 0));
    }

    return BCM_E_NONE;
}

bcm_error_t vlan_set_up()
{
    int i, vlan=base_vlan;

    for (i=0; i<front_port_num-1; i++) {
        vlan_create(vlan, front_port_list[i], front_port_list[i+1], 0);
        vlan+=1;
    }
    // 多一个转CPU的vlan
    vlan_create(vlan+1, front_port_list[i], 0, 1);
    vlan_create(vlan, front_port_list[i], front_port_list[0], 0);
    return BCM_E_NONE;
}

uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    // CRC-32 polynomial: 0xEDB88320
    uint32_t CRC32_POLYNOMIAL=0xEDB88320;
    uint32_t i, j, crc = 0xFFFFFFFF;

    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLYNOMIAL : 0);
        }
    }
    return ~crc; // 取反
}

/* This function compares orig_pkt and rx_pkt_data arrays */
int compareArray(uint32_t len)
{
    // int i, pass = 1;
    // uint8_t payload;

    /* 直接对比当前实际只用了一个字节 */
    /* 这个操作非常费时？？ */
    // payload = pkt_list[pkt_rx_idx].payload & 0xFF;
    // for (i=payload_start; i<len; i++) {
    //     if (rx_pkt_data[i] != payload) {
    //         printf("pkt len=%d, byte[%d]:0x%x != 0x%x(expect)\n",
    //                 len, i, rx_pkt_data[i], payload);
    //         if (pkt_list[pkt_rx_idx].pkt_num_rx_err == 0) {
    //             dumpHex(rx_pkt_data, len);
    //         }
    //         pass = 0;
    //         pkt_list[pkt_rx_idx].pkt_num_rx_err += 1;
    //         break;
    //     }
    // }
    int rv;
    rv = sal_memcmp(&rx_pkt_data[payload_start], &orig_pkt[payload_start], len-payload_start);
    if (rv != 0) {
        if (pkt_list[pkt_rx_idx].pkt_num_rx_err < 10) {
            dumpHex(rx_pkt_data, len);
        }
        //dumpHex(orig_pkt, len);
        pkt_list[pkt_rx_idx].pkt_num_rx_err += 1;
    }

    pkt_list[pkt_rx_idx].pkt_num_rx += 1;

    return rv;
}


void rx_packet_handle(int unit, bcm_pktio_pkt_t *rx_packet)
{
    void *buf = NULL;
    uint32_t len;
    int rv = 0;

    /* Get basic packet info */
    rv = bcm_pktio_pkt_data_get(unit, rx_packet, (void *)&buf, &len);
    if (rv != 0)
    {
        printf("bcm_pktio_pkt_data_get rv =%d\n", rv);
        return BCM_PKTIO_RX_NOT_HANDLED;
    }
    if (len > 0) {
        if (len > pkt_len) {
            printf("len=%d,expect len=%d\n", len, pkt_len);
            len = pkt_len;
        }
        sal_memcpy(rx_pkt_data, buf, len);
        // 会报重复释放 bcm_pktio_free(unit, rx_packet);
    }
    // if (injection_error_count) {
    //     rx_pkt_data[injection_error_pos] = injection_error_var;
    //     injection_error_count--;
    // }
    compareArray(len);

    return;
}

void dumpHex(uint8 *data, uint32 data_len)
{
    uint32 i;

    printf("Pkt data HEX:");
    for (i = 0; i < data_len; i++) {
        if ((i % 16) == 0) {
            printf("\n\tdata[%04x]: ", i);
        }
        printf("%02X ", data[i]);
    }
    printf("\n\n");
    return;
}


/* 通过FP报文送cpu */
bcm_error_t cint_field_group_create(int unit, bcm_field_group_t grp)
{
    int rv;

    bcm_field_group_config_t group_config;

    bcm_field_group_config_t_init(&group_config);
    group_config.flags |= BCM_FIELD_GROUP_CREATE_WITH_ID;                     
    group_config.group = grp;
    group_config.mode = bcmFieldGroupModeAuto;
    group_config.priority = 103;

    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyStageIngress);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifySrcMac);
    BCM_FIELD_QSET_ADD(group_config.qset, bcmFieldQualifyInPort);

    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionCopyToCpu);
    BCM_FIELD_ASET_ADD(group_config.aset, bcmFieldActionDrop);

    rv = bcm_field_group_config_create(unit, &group_config);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_group_config_create failed, rv = %d\r\n", rv);
        return rv;
    }

    bcm_field_group_dump(unit,grp);
    
    return BCM_E_NONE;
}

bcm_error_t cint_field_entry_create(int unit, bcm_field_group_t grp, bcm_field_entry_t entry, bcm_port_t port)
{
    bcm_error_t rv;
    bcm_mac_t src_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    bcm_mac_t mac_mask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    rv = bcm_field_entry_create_id(unit, grp, entry);    
    if (rv != BCM_E_NONE) {
        printf("bcm_field_entry_create_id failed, rv = %d\r\n", rv);
        return rv;
    }

    bcm_field_qualify_InPort(0, entry, port, 0xFFFFFFFF);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_qualify_InPort failed with err %s ,ret = %d\r\n", bcm_errmsg(rv), rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }

    rv =bcm_field_qualify_SrcMac(unit, entry, src_mac, mac_mask);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_qualify_DstMac failed,ret = %d\r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }

    rv = bcm_field_action_add(unit, entry, bcmFieldActionCopyToCpu, 1, 0);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_action_add failed, rv = %d \r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }

    rv = bcm_field_action_add(unit, entry, bcmFieldActionDrop, 1, 0);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_action_add failed, rv = %d \r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }
    
    rv = bcm_field_entry_install(unit, entry);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_entry_install failed,ret = %d\r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }  

    // printf("********************* BEGIN ****************************\r\n");
    // bcm_field_entry_dump(unit, entry);
    // printf("*********************** END ****************************\r\n");
    return BCM_E_NONE;
}

bcm_error_t cint_field_entry_update(int unit, bcm_field_entry_t entry, bcm_mac_t src_mac)
{
    int rv;
    bcm_mac_t mac_mask = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    /* 直接修改可能出现误匹配问题 */
    // rv = bcm_field_entry_enable_set(unit, entry, 0);
    // if (rv != BCM_E_NONE) {
    //     printf("bcm_field_entry_enable_set 0 failed,ret = %d\r\n", rv);
    // }

    rv =bcm_field_qualify_SrcMac(unit, entry, src_mac, mac_mask);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_qualify_DstMac failed,ret = %d\r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }
    rv = bcm_field_entry_install(unit, entry);
    if (rv != BCM_E_NONE) {
        printf("bcm_field_entry_install failed,ret = %d\r\n", rv);
        bcm_field_entry_destroy(unit, entry);
        return rv;
    }

    // rv = bcm_field_entry_enable_set(unit, entry, 1);
    // if (rv != BCM_E_NONE) {
    //     printf("bcm_field_entry_enable_set 1 failed,ret = %d\r\n", rv);
    // }
    return BCM_E_NONE;
}

bcm_error_t step1_snake_test_setup(int unit)
{
    char cmd[2*1024];
    bcm_error_t rv;

    rv = cint_field_group_create(unit,fp_group);
    if (rv != BCM_E_NONE) {
        printf("field_group_create failed!\n");
        return rv;
    }
    rv = cint_field_entry_create(unit, fp_group, fp_entry, front_port_list[front_port_num-1]);
    if (rv != BCM_E_NONE) {
        printf("field_entry_create failed!\n");
        bcm_field_group_destroy(unit, fp_group);
        return rv;
    }

    if (test_port_lb == BCM_PORT_LOOPBACK_PHY) {
        /* Traffic test starts here - set port mac loopback mode. */
        // BCM_IF_ERROR_RETURN(bcm_port_loopback_set(unit, ingress_port, test_port_lb));
        sprintf(cmd, "port %s lb=phy", front_port_name);
        bbshell(unit, cmd);
    } else if (test_port_lb == BCM_PORT_LOOPBACK_MAC) {
        sprintf(cmd, "port %s lb=mac", front_port_name);
        bbshell(unit, cmd);
    } else if (test_port_lb == BCM_PORT_LOOPBACK_EDB) {
        sprintf(cmd, "port %s lb=edb", front_port_name);
        bbshell(unit, cmd);
    }

    sprintf(cmd, "stg stp 1 %s forward", front_port_name);
    bbshell(unit, cmd);
    vlan_set_up();
    sal_sleep(1);
    return BCM_E_NONE;
}

/* 先用u8提高效率 */
bcm_error_t pkt_generate(int len, bcm_mac_t src_mac, uint8_t payload)
{
    int i;
    for (i=0; i<6; i++) {
        orig_pkt[smac_start+i] = src_mac[i];
    }
    for (i=payload_start; i<len; i++) {
        orig_pkt[i] = payload;
    }
}

bcm_error_t pkt_tx(int unit, pkt_type_t *pkt)
{
    int i, offset;
    char cmd[8*1024];
    pkt_generate(pkt->pkt_len, pkt->src_mac, pkt->payload&0xff);
    orig_pkt[vlan_start] = base_vlan+pkt->tx_port;
    offset = sprintf(cmd, "tx %d pbm=%s%d data=0x", pkt->pkt_num_tx, front_port_name, pkt->tx_port);
    for (i=0; i<pkt->pkt_len; i++) {
        offset += sprintf(cmd+offset, "%02X", orig_pkt[i]);
    }
    bbshell(unit, cmd);
    return BCM_E_NONE;
}

void print_test_step1_results()
{
    int i;
    int pass = 1;

    printf("=============================================================================================\n");
    printf("                                       snake test result                             \n");
    printf("=============================================================================================\n");
    printf("   payload    |       src mac       | pkt len | tx pkt num | rx pkt num | failed pkt | result \n");
    printf("=============================================================================================\n");
    for (i=0; i<step1_pkt_type; i++) {
        printf("  0x%04x  | 0x%02x:%02x:%02x:%02x:%02x:%02x |  %5d  |     %5d  |     %5d  |     %5d  ",
            pkt_list[i].payload,
            pkt_list[i].src_mac[0],pkt_list[i].src_mac[1],pkt_list[i].src_mac[2],
            pkt_list[i].src_mac[3],pkt_list[i].src_mac[4],pkt_list[i].src_mac[5],
            pkt_list[i].pkt_len,
            pkt_list[i].pkt_num_tx, 
            pkt_list[i].pkt_num_rx, 
            pkt_list[i].pkt_num_rx_err);
        if (pkt_list[i].pkt_num_tx != pkt_list[i].pkt_num_rx
            || pkt_list[i].pkt_num_rx_err != 0) {
                printf("|  Failed\n");
                pass = 0;
        } else {
            printf("|  Pass\n");
        }
    }
    if (pass) {
        printf("\nTest Result: Pass\n");
    } else {
        printf("\nTest Result: Failed\n");
    }
}

/* 
    阶段一：持续流
    报文类型：FF 5A A5，同时打
    报文长度：9216(CELL:254, 约35.929个cell)
    持续多久：1分钟（demo暂定）
*/
bcm_error_t step1_snake_test_verify(int unit)
{
    int pri = 100;
    int rv;
    int ingress_port = 26;
    int count_tmp = 0;
    int i, j;

    uint64 sn;
    uint32 dump_flag = 0;
    bcm_pktio_pkt_t *pkt;

    //BCM_IF_ERROR_RETURN( step1_snake_test_setup(unit) );

    printf("Starting sync rx...\n");
    //dump_flag = BCM_PKTIO_SYNC_RX_PKT_DUMP;
    BCM_IF_ERROR_RETURN( bcm_pktio_sync_rx_start(unit, dump_flag, &sn) );

    for (i=0; i<step1_pkt_type; i++) {
        pkt_tx(unit, &pkt_list[i]);
    }
    /* 客户要求持续5分钟 */
    printf("sleep %ds ...\n", test_duration);
    sal_sleep(test_duration);
    /* 打印下MMU缓存占用情况 */
    bbshell(unit, "show c rate d3c0");
    bbshell(unit, "bsh -c 'lt CTR_ING_TM_SERVICE_POOL  traverse -l TOTAL_USAGE_CELLS!=0'");
    bbshell(unit, "bsh -c 'lt CTR_TM_THD_UC_Q  traverse -l TOTAL_USAGE_CELLS_Q_ID_0!=0 | grep =0x'");
    for (pkt_rx_idx=0; pkt_rx_idx<step1_pkt_type; pkt_rx_idx++) {
        cint_field_entry_update(unit, fp_entry, pkt_list[pkt_rx_idx].src_mac);
        //sal_sleep(1);
        sal_memset(&orig_pkt[payload_start], pkt_list[pkt_rx_idx].payload, pkt_list[pkt_rx_idx].pkt_len-payload_start);
        for(j=0; j<pkt_list[pkt_rx_idx].pkt_num_tx+1; j++) {
        // for(j=0; j<10000; j++) {
            if( BCM_SUCCESS( bcm_pktio_sync_rx(unit, sn, &pkt, 1000000) ) )
            {
                // bcm_pktio_pkt_data_get(unit, pkt, &vd, &len);
                rx_packet_handle(unit, pkt);
            } else {
                //print src_mac[pkt_check_idx];
                printf("payload:0x%x, smac=0x%02x:%02x:%02x:%02x:%02x:%02x, len:%d, tx:%d, rx:%d, rx_err:%d\n",
                    pkt_list[pkt_rx_idx].payload,
                    pkt_list[pkt_rx_idx].src_mac[0],pkt_list[pkt_rx_idx].src_mac[1],pkt_list[pkt_rx_idx].src_mac[2],
                    pkt_list[pkt_rx_idx].src_mac[3],pkt_list[pkt_rx_idx].src_mac[4],pkt_list[pkt_rx_idx].src_mac[5],
                    pkt_list[pkt_rx_idx].pkt_len,
                    pkt_list[pkt_rx_idx].pkt_num_tx,
                    pkt_list[pkt_rx_idx].pkt_num_rx,
                    pkt_list[pkt_rx_idx].pkt_num_rx_err);
                break;
            }
        }
        /* 有错误报文打印加一些延时 */
        if (pkt_list[pkt_rx_idx].pkt_num_rx_err) {
            sal_sleep(1);
        }
        // printf("payload:0x%x, smac=0x%02x:%02x:%02x:%02x:%02x:%02x, len:%d, tx:%d, rx:%d, rx_err:%d\n",
        //     pkt_list[pkt_rx_idx].payload,
        //     pkt_list[pkt_rx_idx].src_mac[0],pkt_list[pkt_rx_idx].src_mac[1],pkt_list[pkt_rx_idx].src_mac[2],
        //     pkt_list[pkt_rx_idx].src_mac[3],pkt_list[pkt_rx_idx].src_mac[4],pkt_list[pkt_rx_idx].src_mac[5],
        //     pkt_list[pkt_rx_idx].pkt_len,
        //     pkt_list[pkt_rx_idx].pkt_num_tx,
        //     pkt_list[pkt_rx_idx].pkt_num_rx,
        //     pkt_list[pkt_rx_idx].pkt_num_rx_err);
    }

    if( BCM_FAILURE( bcm_pktio_sync_rx_stop(unit, sn) ) )
    {
        printf("WARN: BCM RX STOP FAILED!\n");
    }


    print_test_step1_results();
    return 0;
}

bcm_error_t step2_pkt_keep_test_setup(int unit)
{
    char cmd[2*1024];
    bcm_error_t rv;
    int i, qid;

    /* 报文送cpu */
    cint_field_entry_update(unit, fp_entry, pkt_list[pkt_rx_idx].src_mac);

    printf("mmu config ...\n");
    /* 所有面板口出口队列配置静态缓存 */
    for (i=0; i<front_port_num; i++) {
        for (qid = 0; qid < 8; qid ++) {
            // 319960/32=9998.75
            // 9998/15=666.5个报文
            rv = bcm_cosq_control_set(unit, front_port_list[i], qid, bcmCosqControlEgressUCSharedDynamicEnable, 0);
            if (rv != BCM_E_NONE) {
                printf("bcm_port_control_set bcmCosqControlEgressUCSharedDynamicEnable failed! port[%d]:%d,qid:%d\n",
                        i, front_port_list[i], qid);
                return rv;
            }
            rv = bcm_cosq_control_set(unit, front_port_list[i], qid, bcmCosqControlEgressUCQueueSharedLimitBytes, 25400000);
            if (rv != BCM_E_NONE) {
                printf("bcm_port_control_set bcmCosqControlEgressUCSharedDynamicEnable failed! port[%d]:%d,qid:%d\n",
                        i, front_port_list[i], qid);
                return rv;
            }
        }
    }
    bbshell(unit, "bsh -c 'lt TM_THD_UC_Q traverse -l SHARED_LIMIT_CELLS_STATIC!=0 | grep SHARED_LIMIT_CELLS_STATIC'");

    return BCM_E_NONE;
}

void step2_pkt_keep_test_results()
{
    int i;
    int pass = 1;

    printf("============================================================================================\n");
    printf("                                  packet keep at mmu test                        \n");
    printf("============================================================================================\n");
    printf("   payload   |       src mac       | pkt len | tx pkt num | rx pkt num | failed pkt | result \n");
    printf("============================================================================================\n");
    for (i=step1_pkt_type; i<step1_pkt_type + step2_pkt_type; i++) {
        printf(" 0x%04x  | 0x%02x:%02x:%02x:%02x:%02x:%02x |  %5d  |     %5d  |     %5d  |     %5d  ",
            pkt_list[i].payload,
            pkt_list[i].src_mac[0],pkt_list[i].src_mac[1],pkt_list[i].src_mac[2],
            pkt_list[i].src_mac[3],pkt_list[i].src_mac[4],pkt_list[i].src_mac[5],
            pkt_list[i].pkt_len, 
            pkt_list[i].pkt_num_tx*front_port_num, 
            pkt_list[i].pkt_num_rx, 
            pkt_list[i].pkt_num_rx_err);
        if (pkt_list[i].pkt_num_tx*front_port_num != pkt_list[i].pkt_num_rx
            || pkt_list[i].pkt_num_rx_err != 0) {
                printf("|  Failed\n");
                pass = 0;
        } else {
            printf("|  Pass\n");
        }
    }
    if (pass) {
        printf("\nTest Result: Pass\n");
    } else {
        printf("\nTest Result: Failed\n");
    }
}

/* 
    阶段二：滞留报文后校验
    报文类型：FF
    报文长度：9216
    滞留时间：1分钟（demo暂定）
    重复几轮：1轮（demo暂定）
 */
bcm_error_t step2_pkt_keep_test_verify(int unit)
{
    int pri = 100;
    int rv;
    int ingress_port = 26;
    int count_tmp = 0;
    int i, j;
    int tx_status;
    int rx_tmp,rx_err_tmp;

    uint64 sn;
    uint32 dump_flag = 0;
    bcm_pktio_pkt_t *pkt;
    // pkt_type_t pkt_list[pkt_type] = {

    //BCM_IF_ERROR_RETURN( step1_snake_test_setup(unit) );

    printf("Starting sync rx...\n");
    //dump_flag = BCM_PKTIO_SYNC_RX_PKT_DUMP;
    BCM_IF_ERROR_RETURN( bcm_pktio_sync_rx_start(unit, dump_flag, &sn) );
    // 清零看下报文丢包位置
    bbshell(unit, "clear c");

    /* 从最后一个口开始堵住报文，发包到第一个口转发出去，避免报文入口全是CPU */
    for (i=front_port_num-1; i>=0; i--) {
        rv = bcm_port_control_set(unit, front_port_list[i], bcmPortControlTxEnable, 0);
        if (rv != BCM_E_NONE) {
            printf("bcm_port_control_set %d disable failed!\n", i);
            return rv;
        }
        pkt_tx(unit, &pkt_list[step1_pkt_type]);
        bshell(unit, "sleep 0 100000");
    }

    // 发包后停流1分钟
    printf("sleep %ds ...\n", test_duration);
    /* 客户要求持续5分钟 */
    sal_sleep(test_duration);

    /* 打印下MMU缓存占用情况 */
    //bbshell(unit, "show c rate d3c0");
    bbshell(unit, "bsh -c 'lt CTR_ING_TM_SERVICE_POOL  traverse -l TOTAL_USAGE_CELLS!=0'");
    bbshell(unit, "bsh -c 'lt CTR_TM_THD_UC_Q  traverse -l TOTAL_USAGE_CELLS_Q_ID_0!=0 | grep =0x'");
    //bbshell(unit, "show c");

    printf("pkt_rx_idx=%d\n", pkt_rx_idx);
    for (i=front_port_num-1; i>=0; i--) {
    // for (i=front_port_num-1; i>=front_port_num-10; i--) {
        /* 恢复报文转发 */
        rv = bcm_port_control_set(unit, front_port_list[i], bcmPortControlTxEnable, 1);
        if (rv != BCM_E_NONE) {
            printf("bcm_port_control_set %d enable failed!\n", i);
            return rv;
        }
        bcm_port_control_get(unit, front_port_list[i], bcmPortControlTxEnable, &tx_status);
        
        // printf("port[%d]:%d, %d \n", i, front_port_list[i], tx_status);
        rx_tmp = pkt_list[step1_pkt_type].pkt_num_rx;
        rx_err_tmp = pkt_list[step1_pkt_type].pkt_num_rx_err;
        for(j=0; j<pkt_list[step1_pkt_type].pkt_num_tx; j++) {
        //for(j=0; j<1000; j++) {
            if( BCM_SUCCESS( bcm_pktio_sync_rx(unit, sn, &pkt, 2000000) ) ) {
                // bcm_pktio_pkt_data_get(unit, pkt, &vd, &len);
                rx_packet_handle(unit, pkt);
            } else {
                //print src_mac[pkt_check_idx];
                printf("payload:0x%x, smac=0x%02x:%02x:%02x:%02x:%02x:%02x, len:%d, port[%d]:%d, tx:%d, rx:%d, rx_err:%d\n",
                    pkt_list[step1_pkt_type].payload,
                    pkt_list[step1_pkt_type].src_mac[0],pkt_list[step1_pkt_type].src_mac[1],pkt_list[step1_pkt_type].src_mac[2],
                    pkt_list[step1_pkt_type].src_mac[3],pkt_list[step1_pkt_type].src_mac[4],pkt_list[step1_pkt_type].src_mac[5],
                    pkt_list[step1_pkt_type].pkt_len,
                    i,
                    front_port_list[i],
                    pkt_list[step1_pkt_type].pkt_num_tx,
                    pkt_list[step1_pkt_type].pkt_num_rx - rx_tmp,
                    pkt_list[step1_pkt_type].pkt_num_rx_err - rx_err_tmp);
                break;
            }
        }
        // printf("payload:0x%x, smac=0x%02x:%02x:%02x:%02x:%02x:%02x, len:%d, port[%d]:%d, tx:%d, rx:%d, rx_err:%d\n",
            // pkt_list[step1_pkt_type].payload,
            // pkt_list[step1_pkt_type].src_mac[0],pkt_list[step1_pkt_type].src_mac[1],pkt_list[step1_pkt_type].src_mac[2],
            // pkt_list[step1_pkt_type].src_mac[3],pkt_list[step1_pkt_type].src_mac[4],pkt_list[step1_pkt_type].src_mac[5],
            // pkt_list[step1_pkt_type].pkt_len,
            // i, front_port_list[i],
            // pkt_list[step1_pkt_type].pkt_num_tx,
            // pkt_list[step1_pkt_type].pkt_num_rx - rx_tmp,
            // pkt_list[step1_pkt_type].pkt_num_rx_err - rx_err_tmp);
    }

    if( BCM_FAILURE( bcm_pktio_sync_rx_stop(unit, sn) ) )
    {
        printf("WARN: BCM RX STOP FAILED!\n");
    }


    step2_pkt_keep_test_results();
    return 0;
}


bcm_error_t testCleanup(int unit)
{
    return BCM_E_NONE;
}


bcm_error_t test_step1(void)
{
    bcm_error_t rv;
    int unit = 0;

    bshell(0,"shell 'cat /host/machine.conf | grep onie_platform'"); 
    printf("mem bit test for %s. %s\n", platform, ver);

    rv = step1_snake_test_setup(unit);
    if (BCM_FAILURE(rv)) {
        printf("step1_snake_test_setup() failed %s \n", bcm_errmsg(rv));
        return rv;
    }
    printf("step1_snake_test_setup successfully.\n");

    rv = step1_snake_test_verify(unit);
    if (BCM_FAILURE(rv)) {
        printf("step1_snake_test_verify() FAILED: %s\n", bcm_errmsg(rv));
        return rv;
    }
    return BCM_E_NONE;
}

bcm_error_t test_step2(void)
{
    bcm_error_t rv;
    int unit = 0;
    rv = step2_pkt_keep_test_setup(unit);
    if (BCM_FAILURE(rv)) {
        printf("step2_pkt_keep_test_setup() failed %s \n", bcm_errmsg(rv));
        return rv;
    }

    rv = step2_pkt_keep_test_verify(unit);
    if (BCM_FAILURE(rv)) {
        printf("step2_pkt_keep_test_verify() FAILED: %s\n", bcm_errmsg(rv));
        return rv;
    }

    return BCM_E_NONE;
}

bcm_error_t execute(void)
{
    test_step1();
    test_step2();

    return BCM_E_NONE;
}


/* AS24-128D */
// int front_port_list[128] = {
//     91,89,44,42,48,87,95,99,40,19,46,85,93,97,38,17,53,82,104,108,36,23,51,80,102,106,34,21,57,78,112,116,31,27,55,76,110,114,29,25,61,74,121,125,11,15,59,72,119,123, 9,13,65,70,129,133, 3, 7,63,68,127,131, 1, 5,206,201,138,142,265,269,204,199,136,140,263,267,210,197,146,150,257,261,208,195,144,148,255,259,214,193,155,159,240,235,212,191,153,157,238,233,218,189,163,167,244,231,216,187,161,165,242,229,223,184,172,176,248,227,221,182,170,174,246,225,180,178,252,250,
// };
// int front_port_num = 128;
// char *front_port_name = "cd";

/* n9600-64od */
int front_port_list[128] = {
    26, 22, 37, 33, 48, 44, 59, 55, 70, 66, 81, 77, 92, 88,103, 99,114,110,125,121,136,132,147,143,  1,  5, 11, 15,169,165,158,154,176,180,187,191,345,341,334,330,202,198,213,209,224,220,235,231,246,242,257,253,268,264,279,275,290,286,301,297,312,308,323,319,
};
int front_port_num = 64;
char *front_port_name = "d3c";

/* AS24-128Q */
// int front_port_list[128] = {
//     11, 13, 15, 17, 88, 90, 26, 28, 37, 39, 99,101, 92, 94, 22, 24,110,112,103,105, 33, 35,121,123,114,116, 44, 46,143,145,125,127, 55, 57,132,134,136,138, 66, 68, 81, 83,147,149, 77, 79, 70, 72,154,156,158,160, 59, 61,165,167,169,171, 48, 50,176,178,180,182,297,299,187,189,191,193,286,288,268,270,198,200,275,277,279,281,209,211,264,266,290,292,220,222,213,215,301,303,231,233,202,204,312,314,242,244,224,226,323,325,253,255,235,237,308,310,319,321,246,248,334,336,330,332,257,259,345,347,341,343,  5,  7,  1,  3
// };
// int front_port_num = 128;
// char *front_port_name = "cd";

// execute();
test_step1();
