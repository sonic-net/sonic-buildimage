/*---------------------------------------------
 * Transform Records defined for rekey test
 */
 
#define uint32_t unsigned int 
#define uint8_t  unsigned short int

/* Transform Records - Ingress */
static uint32_t transform_record_basic_transform_ingress[] =
{
    0xd24be06f, /* CCW [3:0] = 0xf -> Ingress , 32-bit packet numbering, encrypt ICV */
    0x0000c000, /* SA Update CW */
    0xd02b7aad, /* Key0 */ 
    0x5a83ac3e, /* Key1 */
    0xdc0f626f, /* Key2 */
    0x45b306b5, /* Key3 */
    0x00000000, /* Key4 */
    0x00000000, /* Key5 */
    0x00000000, /* Key6 */
    0x00000000, /* Key7 */
    0x803da273, /* H_Key0 */
    0xd5e21d12, /* H_Key1 */
    0x3f2550a8, /* H_Key2 */
    0x0e1243cf, /* H_Key3 */
    0xffffffc1, /* Seq0 */
    0x00000000, /* Seq1*/
    0x00000080, /* Mask  - Replay Window */
    0x00000000, /* (zero) */
    0x00000000, /* (zero) */
    0x00000000, /* (zero) */
    0xefbeadde, /* IV0 (SCI/CtxSalt) */
    0x01005aa5, /* IV1 (SCI/CtxSalt) */
    0x00000000, /* IV2 (CtxSalt) */
};

static const unsigned char srcpacket_basic_transform_ingress_sci[] = 
{
    0xde, 0xad, 0xbe, 0xef, 0xa5, 0x5a, 0x00, 0x01
};
static const unsigned char * sci_basic_transform_ingress_p = &srcpacket_basic_transform_ingress_sci[0];

/* Transform Record - Egress */
/* key and hash key match with ingress to ensure encryption and decryption work */
static uint32_t transform_record_basic_transform_egress[] =
{
    0x924be066, /* CCW */
    0x8000c001, /* SA Update CW */
    0xd02b7aad, /* Key0*/
    0x5a83ac3e, /* Key1*/
    0xdc0f626f, /* Key2*/
    0x45b306b5, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x803da273, /* H_Key0*/
    0xd5e21d12, /* H_Key1*/
    0x3f2550a8, /* H_Key2*/
    0x0e1243cf, /* H_Key3*/
    0xffffffc0, /* Seq0*/
    0x00000000, /* Seq1*/
    0x000005DC, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/

};

/* Transform Records AN1 */
static uint32_t transform_record_basic_transform_egress1[] =
{
    0x964be066, /* CCW*/
    0x8000c002,  /* SA upd ctrl*/
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc0, /* Seq0*/
    0x00000000, /* Seq1*/
    0x000005DC, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/

};

static uint32_t transform_record_basic_transform_ingress1[] =
{
    0xd24be06f, /* CCW*/
    0x2000c001, /* SA update control */
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc1, /* Seq0*/
    0x00000000, /* Seq1*/
    0x00000080, /* Mask  - Replay Window */
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/
    
};

/* Transform Records AN2*/
static uint32_t transform_record_basic_transform_egress2[] =
{
    0x9a4be066, /* CCW*/
    0x8000c003,  /* SA upd ctrl*/
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc0, /* Seq0*/
    0x00000000, /* Seq1*/
    0x000005DC, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/
};

static uint32_t transform_record_basic_transform_ingress2[] =
{
    0xd24be06f, /* CCW*/
    0x4000c002, /* SA update control */
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc1, /* Seq0*/
    0x00000000, /* Seq1*/
    0x00000080, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/
};

/* Transform Records AN3*/
static uint32_t transform_record_basic_transform_egress3[] =
{
    0x9e4be066, /* CCW*/
    0x8000c000,  /* SA upd ctrl*/
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc0, /* Seq0*/
    0x00000000, /* Seq1*/
    0x000005DC, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/

};

static uint32_t transform_record_basic_transform_ingress3[] =
{
    0xd24be06f, /* CCW*/
    0x6000c003, /* SA update control */
    0xdc0f626f, /* Key0*/
    0x45b306b5, /* Key1*/
    0x803da273, /* Key2*/
    0x0e1243cf, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x45b306b5, /* H_Key0*/
    0xdc0f626f, /* H_Key1*/
    0xefbeadde, /* H_Key2*/
    0x01005aa5, /* H_Key3*/
    0xffffffc1, /* Seq0*/
    0x00000000, /* Seq1*/
    0x00000080, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/

};
