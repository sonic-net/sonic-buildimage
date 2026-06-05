local _cfye = require "ddk._cfye"

local strt = require "struct"

---@module 'ddk.cfye'
local cfye = {}

-- Constants & EnumVals --

---@type integer
cfye.CHANNEL_WORDS = _cfye.CFYE_CHANNEL_WORDS
---@type integer
cfye.CHAN_COUNT_INC_DIS = _cfye.CFYE_CHAN_COUNT_INC_DIS
---@type integer
cfye.DO_NOT_DROP = _cfye.CFYE_DO_NOT_DROP
---@type integer
cfye.DROP_CRC_ERROR = _cfye.CFYE_DROP_CRC_ERROR
---@type integer
cfye.DROP_INTERNAL = _cfye.CFYE_DROP_INTERNAL
---@type integer
cfye.DROP_PKT_ERROR = _cfye.CFYE_DROP_PKT_ERROR
---@type integer
cfye.ECC_NOF_STATUS_COUNTERS = _cfye.CFYE_ECC_NOF_STATUS_COUNTERS
---@type integer
cfye.ERROR_BAD_PARAMETER = _cfye.CFYE_ERROR_BAD_PARAMETER
---@type integer
cfye.ERROR_INTERNAL = _cfye.CFYE_ERROR_INTERNAL
---@type integer
cfye.ERROR_NOT_IMPLEMENTED = _cfye.CFYE_ERROR_NOT_IMPLEMENTED
---@type integer
cfye.EVENT_CHAN_CTRL_PACKET = _cfye.CFYE_EVENT_CHAN_CTRL_PACKET
---@type integer
cfye.EVENT_CHAN_DATA_PACKET = _cfye.CFYE_EVENT_CHAN_DATA_PACKET
---@type integer
cfye.EVENT_CHAN_DROP_PACKET = _cfye.CFYE_EVENT_CHAN_DROP_PACKET
---@type integer
cfye.EVENT_CHAN_ERR_EOP_WO_SOP = _cfye.CFYE_EVENT_CHAN_ERR_EOP_WO_SOP
---@type integer
cfye.EVENT_CHAN_ERR_NOT_B2B = _cfye.CFYE_EVENT_CHAN_ERR_NOT_B2B
---@type integer
cfye.EVENT_CHAN_ERR_SLOT_2PKTS = _cfye.CFYE_EVENT_CHAN_ERR_SLOT_2PKTS
---@type integer
cfye.EVENT_CHAN_ERR_SLOT_CHID = _cfye.CFYE_EVENT_CHAN_ERR_SLOT_CHID
---@type integer
cfye.EVENT_CHAN_ERR_SLOT_SOP = _cfye.CFYE_EVENT_CHAN_ERR_SLOT_SOP
---@type integer
cfye.EVENT_CHAN_ERR_SOP_WO_EOP = _cfye.CFYE_EVENT_CHAN_ERR_SOP_WO_EOP
---@type integer
cfye.EVENT_CHAN_ERR_XFER_WO_SOP = _cfye.CFYE_EVENT_CHAN_ERR_XFER_WO_SOP
---@type integer
cfye.EVENT_CHAN_MTT_MISS = _cfye.CFYE_EVENT_CHAN_MTT_MISS
---@type integer
cfye.EVENT_CHAN_TCAM_HIT_MULT = _cfye.CFYE_EVENT_CHAN_TCAM_HIT_MULT
---@type integer
cfye.EVENT_CHAN_TCAM_HIT_NON_CS = _cfye.CFYE_EVENT_CHAN_TCAM_HIT_NON_CS
---@type integer
cfye.EVENT_CHAN_TCAM_MISS = _cfye.CFYE_EVENT_CHAN_TCAM_MISS
---@type integer
cfye.EVENT_ECC_ERR = _cfye.CFYE_EVENT_ECC_ERR
---@type integer
cfye.EVENT_STAT_CHAN_THR = _cfye.CFYE_EVENT_STAT_CHAN_THR
---@type integer
cfye.EVENT_STAT_MTT_THR = _cfye.CFYE_EVENT_STAT_MTT_THR
---@type integer
cfye.EVENT_STAT_TCAM_THR = _cfye.CFYE_EVENT_STAT_TCAM_THR
---@type integer
cfye.MAC_DA_ET_MATCH_RULES_COUNT = _cfye.CFYE_MAC_DA_ET_MATCH_RULES_COUNT
---@type integer
cfye.MAC_DA_ET_RANGE_MATCH_RULES_COUNT = _cfye.CFYE_MAC_DA_ET_RANGE_MATCH_RULES_COUNT
---@type integer
cfye.MODE_INVALID = _cfye.CFYE_MODE_INVALID
---@type integer
cfye.MODE_IPSEC = _cfye.CFYE_MODE_IPSEC
---@type integer
cfye.MODE_MACSEC = _cfye.CFYE_MODE_MACSEC
---@type integer
cfye.MPLS3_SELECT_LABEL1 = _cfye.CFYE_MPLS3_SELECT_LABEL1
---@type integer
cfye.MPLS3_SELECT_LABEL2 = _cfye.CFYE_MPLS3_SELECT_LABEL2
---@type integer
cfye.MPLS3_SELECT_LABEL3 = _cfye.CFYE_MPLS3_SELECT_LABEL3
---@type integer
cfye.MPLS4_SELECT_LABEL1 = _cfye.CFYE_MPLS4_SELECT_LABEL1
---@type integer
cfye.MPLS4_SELECT_LABEL2 = _cfye.CFYE_MPLS4_SELECT_LABEL2
---@type integer
cfye.MPLS4_SELECT_LABEL3 = _cfye.CFYE_MPLS4_SELECT_LABEL3
---@type integer
cfye.MPLS4_SELECT_LABEL4 = _cfye.CFYE_MPLS4_SELECT_LABEL4
---@type integer
cfye.MPLS5_SELECT_LABEL1 = _cfye.CFYE_MPLS5_SELECT_LABEL1
---@type integer
cfye.MPLS5_SELECT_LABEL2 = _cfye.CFYE_MPLS5_SELECT_LABEL2
---@type integer
cfye.MPLS5_SELECT_LABEL3 = _cfye.CFYE_MPLS5_SELECT_LABEL3
---@type integer
cfye.MPLS5_SELECT_LABEL4 = _cfye.CFYE_MPLS5_SELECT_LABEL4
---@type integer
cfye.MPLS5_SELECT_LABEL5 = _cfye.CFYE_MPLS5_SELECT_LABEL5
---@type integer
cfye.MPLS_ETYPE_MAX_COUNT = _cfye.CFYE_MPLS_ETYPE_MAX_COUNT
---@type integer
cfye.NULL = _cfye.CFYE_NULL
---@type integer
cfye.ROLE_EGRESS = _cfye.CFYE_ROLE_EGRESS
---@type integer
cfye.ROLE_INGRESS = _cfye.CFYE_ROLE_INGRESS
---@type integer
cfye.RULE_CHANNEL_ID_MASK = _cfye.CFYE_RULE_CHANNEL_ID_MASK
---@type integer
cfye.RULE_NON_CTRL_WORD_COUNT = _cfye.CFYE_RULE_NON_CTRL_WORD_COUNT
---@type integer
cfye.RULE_NUMTAGS_MASK = _cfye.CFYE_RULE_NUMTAGS_MASK
---@type integer
cfye.RULE_PKT_TYPE_IPSEC = _cfye.CFYE_RULE_PKT_TYPE_IPSEC
---@type integer
cfye.RULE_PKT_TYPE_IPSEC_MPLS = _cfye.CFYE_RULE_PKT_TYPE_IPSEC_MPLS
---@type integer
cfye.RULE_PKT_TYPE_IPSEC_UDP = _cfye.CFYE_RULE_PKT_TYPE_IPSEC_UDP
---@type integer
cfye.RULE_PKT_TYPE_MACSEC = _cfye.CFYE_RULE_PKT_TYPE_MACSEC
---@type integer
cfye.RULE_PKT_TYPE_MASK = _cfye.CFYE_RULE_PKT_TYPE_MASK
---@type integer
cfye.RULE_PKT_TYPE_MPLS = _cfye.CFYE_RULE_PKT_TYPE_MPLS
---@type integer
cfye.RULE_PKT_TYPE_OTHER = _cfye.CFYE_RULE_PKT_TYPE_OTHER
---@type integer
cfye.STATUS_OK = _cfye.CFYE_STATUS_OK
---@type integer
cfye.TCAM_COUNT_INC_DIS = _cfye.CFYE_TCAM_COUNT_INC_DIS
---@type integer
cfye.VLAN_UP_MAX_COUNT = _cfye.CFYE_VLAN_UP_MAX_COUNT

-- Structs --

---@class cfye.Ch_Mask_t
---@field public ch_bitmask integer[]

---@type fun (): cfye.Ch_Mask_t
cfye.Ch_Mask_t = _cfye.CfyE_Ch_Mask_t

---@class cfye.ControlPacket_t
---@field public CPMatchModeMask integer
---@field public MAC_DA_ET_Rules cfye.MAC_DA_ET_MatchRule_t[]
---@field public CPMatchSubMask integer
---@field public MAC_DA_44Bit_Const_p integer[]|nil
---@field public MAC_DA_Range cfye.MAC_DA_Range_MatchRule_t
---@field public CPMatchEnableMask integer
---@field public MAC_DA_48Bit_Const_p integer[]|nil
---@field public MAC_DA_ET_Range cfye.MAC_DA_ET_Range_MatchRule_t[]

---@type fun (): cfye.ControlPacket_t
cfye.ControlPacket_t = _cfye.CfyE_ControlPacket_t

---@class cfye.DeviceStatus_t
---@field public ECCStatus_p cfye.ECCStatus_t|nil
---@field public fExternalTCAM_p boolean|nil
---@field public PktProcessDebug_p cfye.PktProcessDebug_t|nil

---@type fun (): cfye.DeviceStatus_t
cfye.DeviceStatus_t = _cfye.CfyE_DeviceStatus_t

---@class cfye.Device_Control_t
---@field public fSparseReg boolean
---@field public fLowLatencyBypass boolean
---@field public fIPsec boolean
---@field public Exceptions_p cfye.Device_Exceptions_t|nil

---@type fun (): cfye.Device_Control_t
cfye.Device_Control_t = _cfye.CfyE_Device_Control_t

---@class cfye.Device_Exceptions_t
---@field public ECCDropAction integer
---@field public fDefaultVPortValid boolean
---@field public DropAction integer
---@field public fForceDrop boolean
---@field public DefaultVPort integer
---@field public fShouldSecure boolean

---@type fun (): cfye.Device_Exceptions_t
cfye.Device_Exceptions_t = _cfye.CfyE_Device_Exceptions_t

---@class cfye.Device_Limits_t
---@field public mtt_count integer
---@field public minor_version integer
---@field public major_version integer
---@field public patch_level integer
---@field public channel_count integer
---@field public vport_count integer
---@field public rule_count integer

---@type fun (): cfye.Device_Limits_t
cfye.Device_Limits_t = _cfye.CfyE_Device_Limits_t

---@class cfye.Device_t
---@field public CP_p cfye.ControlPacket_t|nil
---@field public ECCConf_p cfye.ECCConf_t|nil
---@field public EOPConf_p cfye.EOPConf_t|nil
---@field public Control_p cfye.Device_Control_t|nil
---@field public HeaderParser_p cfye.HeaderParser_t|nil
---@field public StatControl_p cfye.Statistics_Control_t|nil

---@type fun (): cfye.Device_t
cfye.Device_t = _cfye.CfyE_Device_t

---@class cfye.ECCConf_t
---@field public ECCUncorrectableThr integer
---@field public ECCCorrectableThr integer

---@type fun (): cfye.ECCConf_t
cfye.ECCConf_t = _cfye.CfyE_ECCConf_t

---@class cfye.ECCStatus_t
---@field public Counters cfye.ECCStatus_t_Counters[]

---@type fun (): cfye.ECCStatus_t
cfye.ECCStatus_t = _cfye.CfyE_ECCStatus_t

---@class cfye.ECCStatus_t_Counters
---@field public fCorrectableThr boolean
---@field public UncorrectableCount integer
---@field public CorrectableCount integer
---@field public fUncorrectableThr boolean

---@type fun (): cfye.ECCStatus_t_Counters
cfye.ECCStatus_t_Counters = _cfye.CfyE_ECCStatus_t_Counters

---@class cfye.EOPConf_t
---@field public EOPTimeoutVal integer
---@field public EOPTimeoutCtrl cfye.Ch_Mask_t

---@type fun (): cfye.EOPConf_t
cfye.EOPConf_t = _cfye.CfyE_EOPConf_t

---@class cfye.EgressHeader_t
---@field public EgressHeaderEtype integer
---@field public fEnable boolean

---@type fun (): cfye.EgressHeader_t
cfye.EgressHeader_t = _cfye.CfyE_EgressHeader_t

---@class cfye.HeaderParser_t
---@field public VLAN_Parser_p cfye.VLAN_Parser_t|nil
---@field public EgressHeader_p cfye.EgressHeader_t|nil
---@field public IPsec_Parser_p cfye.IPSEC_Parser_t|nil
---@field public MPLS_Parser_p cfye.MPLS_Parser_t|nil
---@field public SecTAG_Parser_p cfye.SecTAG_Parser_t|nil

---@type fun (): cfye.HeaderParser_t
cfye.HeaderParser_t = _cfye.CfyE_HeaderParser_t

---@class cfye.IPSEC_Parser_t
---@field public IKE_Port integer
---@field public fAllowFragments boolean
---@field public fMACDACheck boolean
---@field public fVerifyUDPChkSum boolean
---@field public fSubParseNATIKE boolean
---@field public fParseESP boolean
---@field public NAT_Port integer
---@field public fIgnoreIPv4ChkSum boolean
---@field public fSubParseNATKeepAlive boolean
---@field public MAC_DA integer[]
---@field public fSubParseIKE boolean
---@field public fParseIKE boolean
---@field public fParseIP boolean
---@field public fParseUDP boolean
---@field public fParseNATKeepAlive boolean
---@field public fParseNATIKE boolean
---@field public fParseNAT boolean

---@type fun (): cfye.IPSEC_Parser_t
cfye.IPSEC_Parser_t = _cfye.CfyE_IPSEC_Parser_t

---@class cfye.Init_t
---@field public MTTCountFrameThrLo integer
---@field public fDropControlPkts boolean
---@field public WordOffset integer
---@field public CountIncDisCtrl integer
---@field public ChanCountFrameThrLo integer
---@field public CountFrameThrLo integer
---@field public MaxRuleCount integer
---@field public MTTCountFrameThrHi integer
---@field public fParsePktIPsec boolean
---@field public EOPTimeoutCtrl cfye.Ch_Mask_t
---@field public ECCCorrectableThr integer
---@field public ChanCountFrameThrHi integer
---@field public MaxChannelCount integer
---@field public EOPTimeoutVal integer
---@field public fLowLatencyBypass boolean
---@field public MaxvPortCount integer
---@field public InputTCAM_p integer[]|nil
---@field public ECCUncorrectableThr integer
---@field public CountFrameThrHi integer
---@field public InputTCAM_WordCount integer

---@type fun (): cfye.Init_t
cfye.Init_t = _cfye.CfyE_Init_t

---@class cfye.MAC_DA_ET_MatchRule_t
---@field public MAC_DA_p integer[]|nil
---@field public EtherType integer

---@type fun (): cfye.MAC_DA_ET_MatchRule_t
cfye.MAC_DA_ET_MatchRule_t = _cfye.CfyE_MAC_DA_ET_MatchRule_t

---@class cfye.MAC_DA_ET_Range_MatchRule_t
---@field public Range cfye.MAC_DA_Range_MatchRule_t
---@field public EtherType integer

---@type fun (): cfye.MAC_DA_ET_Range_MatchRule_t
cfye.MAC_DA_ET_Range_MatchRule_t = _cfye.CfyE_MAC_DA_ET_Range_MatchRule_t

---@class cfye.MAC_DA_Range_MatchRule_t
---@field public MAC_DA_End_p integer[]|nil
---@field public MAC_DA_Start_p integer[]|nil

---@type fun (): cfye.MAC_DA_Range_MatchRule_t
cfye.MAC_DA_Range_MatchRule_t = _cfye.CfyE_MAC_DA_Range_MatchRule_t

---@class cfye.MPLS_Etype_t
---@field public MPLS_Etype integer
---@field public fCompare boolean

---@type fun (): cfye.MPLS_Etype_t
cfye.MPLS_Etype_t = _cfye.CfyE_MPLS_Etype_t

---@class cfye.MPLS_Parser_t
---@field public MPLS5_Select2 integer
---@field public MPLS4_Select2 integer
---@field public MPLS5_Select1 integer
---@field public MPLS_Etype cfye.MPLS_Etype_t
---@field public MPLS3_Select2 integer
---@field public MPLS4_Select1 integer
---@field public MPLS3_Select1 integer

---@type fun (): cfye.MPLS_Parser_t
cfye.MPLS_Parser_t = _cfye.CfyE_MPLS_Parser_t

---@class cfye.MTT_KeyMask_t
---@field public fIPv6 boolean
---@field public TagLabel1 integer
---@field public fPacketType boolean
---@field public fIPHdrValid boolean
---@field public TagLabel2 integer
---@field public ChannelMask cfye.Ch_Mask_t

---@type fun (): cfye.MTT_KeyMask_t
cfye.MTT_KeyMask_t = _cfye.CfyE_MTT_KeyMask_t

---@class cfye.MTT_t
---@field public IPAddrMask integer[]
---@field public IPAddr integer[]
---@field public Key cfye.MTT_KeyMask_t
---@field public Mask cfye.MTT_KeyMask_t

---@type fun (): cfye.MTT_t
cfye.MTT_t = _cfye.CfyE_MTT_t

---@class cfye.Notify_t
---@field public CBFunc_p userdata|nil (We are not handling callbacks for now)
---@field public ChannelId integer
---@field public EventMask integer
---@field public fGlobal boolean

---@type fun (): cfye.Notify_t
cfye.Notify_t = _cfye.CfyE_Notify_t

---@class cfye.PktProcessDebug_t
---@field public SAMPPDebug3 integer
---@field public CPMatchDebug integer
---@field public SAMPPDebug1 integer
---@field public ParsedDAHi integer
---@field public SecTAGDebug integer
---@field public ParsedSecTAGHi integer
---@field public SAMPPDebug2 integer
---@field public ParsedSecTAGLo integer
---@field public DebugFlowLookup integer
---@field public TCAMDebug integer
---@field public ParsedSALo integer
---@field public ParsedSAHi integer
---@field public ParsedDALo integer

---@type fun (): cfye.PktProcessDebug_t
cfye.PktProcessDebug_t = _cfye.CfyE_PktProcessDebug_t

---@class cfye.Rule_KeyMask_t
---@field public ChannelID integer
---@field public PacketType integer
---@field public NumTags integer

---@type fun (): cfye.Rule_KeyMask_t
cfye.Rule_KeyMask_t = _cfye.CfyE_Rule_KeyMask_t

---@class cfye.Rule_Policy_t
---@field public fControlPacket boolean
---@field public fControlPacketSub boolean
---@field public Priority integer
---@field public vPortHandle cfye.vPortHandle_t|nil
---@field public fDrop boolean
---@field public AN integer

---@type fun (): cfye.Rule_Policy_t
cfye.Rule_Policy_t = _cfye.CfyE_Rule_Policy_t

---@class cfye.Rule_t
---@field public Data integer[]
---@field public Key cfye.Rule_KeyMask_t
---@field public Mask cfye.Rule_KeyMask_t
---@field public Policy cfye.Rule_Policy_t
---@field public DataMask integer[]

---@type fun (): cfye.Rule_t
cfye.Rule_t = _cfye.CfyE_Rule_t

---@class cfye.SecTAG_Parser_t
---@field public fLookupUseSCI boolean
---@field public fCheckKay boolean
---@field public MACsecTagValue integer
---@field public fCompType boolean
---@field public fCheckVersion boolean

---@type fun (): cfye.SecTAG_Parser_t
cfye.SecTAG_Parser_t = _cfye.CfyE_SecTAG_Parser_t

---@class cfye.Stat_Counter_t
---@field public Hi integer
---@field public Lo integer

---@type fun (): cfye.Stat_Counter_t
cfye.Stat_Counter_t = _cfye.CfyE_Stat_Counter_t

---@class cfye.Statistics_Channel_t
---@field public HeaderParserDroppedPkts cfye.Stat_Counter_t
---@field public MTTMiss cfye.Stat_Counter_t
---@field public PktsErrIn cfye.Stat_Counter_t
---@field public PktsCtrl cfye.Stat_Counter_t
---@field public PktsData cfye.Stat_Counter_t
---@field public TCAMHitMultiple cfye.Stat_Counter_t
---@field public TCAMMiss cfye.Stat_Counter_t
---@field public PktsDropped cfye.Stat_Counter_t

---@type fun (): cfye.Statistics_Channel_t
cfye.Statistics_Channel_t = _cfye.CfyE_Statistics_Channel_t

---@class cfye.Statistics_Control_t
---@field public ChanCountFrameThr cfye.Stat_Counter_t
---@field public CountIncDisCtrl integer
---@field public fAutoStatCntrsReset boolean
---@field public CountFrameThr cfye.Stat_Counter_t
---@field public MTTCountFrameThr cfye.Stat_Counter_t

---@type fun (): cfye.Statistics_Control_t
cfye.Statistics_Control_t = _cfye.CfyE_Statistics_Control_t

---@class cfye.Statistics_MTT_t
---@field public Counter cfye.Stat_Counter_t

---@type fun (): cfye.Statistics_MTT_t
cfye.Statistics_MTT_t = _cfye.CfyE_Statistics_MTT_t

---@class cfye.Statistics_TCAM_t
---@field public Counter cfye.Stat_Counter_t

---@type fun (): cfye.Statistics_TCAM_t
cfye.Statistics_TCAM_t = _cfye.CfyE_Statistics_TCAM_t

---@class cfye.VLAN_ParseTag_t
---@field public fParseStag2 boolean
---@field public fParseStag1 boolean
---@field public fParseQinQ boolean
---@field public fParseStag3 boolean
---@field public fParseQTag boolean

---@type fun (): cfye.VLAN_ParseTag_t
cfye.VLAN_ParseTag_t = _cfye.CfyE_VLAN_ParseTag_t

---@class cfye.VLAN_Parser_t
---@field public fQTagUpEnable boolean
---@field public STag1 integer
---@field public STag3 integer
---@field public CP cfye.VLAN_ParseTag_t
---@field public STag2 integer
---@field public UpTable1 integer[]
---@field public DefaultUp integer
---@field public UpTable2 integer[]
---@field public fSTagUpEnable boolean
---@field public QTag integer

---@type fun (): cfye.VLAN_Parser_t
cfye.VLAN_Parser_t = _cfye.CfyE_VLAN_Parser_t

---@class cfye.vPort_t
---@field public MPLS_Hdr integer
---@field public SecTagOffset integer
---@field public PktExtension integer

---@type fun (): cfye.vPort_t
cfye.vPort_t = _cfye.CfyE_vPort_t

---@class cfye.RuleHandle_t : userdata

---@type cfye.RuleHandle_t
cfye.RuleHandle_NULL = _cfye.CfyE_RuleHandle_NULL

---@class cfye.vPortHandle_t : userdata

---@type cfye.vPortHandle_t
cfye.vPortHandle_NULL = _cfye.CfyE_vPortHandle_NULL

-- ##################### Functions ####################

---@param DeviceId integer
---@param ChannelId integer
---@return boolean
function cfye.Channel_Bypass_Get(DeviceId, ChannelId)
    return _cfye.CfyE_Channel_Bypass_Get(DeviceId, ChannelId)
end

---@param DeviceId integer
---@param ChannelId integer
---@param fBypass boolean
function cfye.Channel_Bypass_Set(DeviceId, ChannelId, fBypass)
    _cfye.CfyE_Channel_Bypass_Set(DeviceId, ChannelId, fBypass)
end

---@param DeviceId integer
---@param ChannelId integer
---@param Control_p cfye.Device_t
---@return cfye.Device_t
function cfye.Device_Config_Get(DeviceId, ChannelId, Control_p)
    _cfye.CfyE_Device_Config_Get(DeviceId, ChannelId, Control_p)
    return Control_p
end

---@param DeviceId integer
---@param Role integer
---@param Init_p cfye.Init_t
---@return cfye.Init_t
function cfye.Device_Init(DeviceId, Role, Init_p)
    _cfye.CfyE_Device_Init(DeviceId, Role, Init_p)
    return Init_p
end

---@param DeviceId integer
---@param ChannelMask_p cfye.Ch_Mask_t
function cfye.Device_InsertEOP(DeviceId, ChannelMask_p)
    _cfye.CfyE_Device_InsertEOP(DeviceId, ChannelMask_p)
end

---@param DeviceId integer
---@param ChannelMask_p cfye.Ch_Mask_t
function cfye.Device_InsertSOP(DeviceId, ChannelMask_p)
    _cfye.CfyE_Device_InsertSOP(DeviceId, ChannelMask_p)
end

---@param DeviceId integer
---@return integer MaxChannelCount
---@return integer MaxvPortCount
---@return integer MaxRuleCount
function cfye.Device_Limits(DeviceId)
    return _cfye.CfyE_Device_Limits(DeviceId)
end

---@param DeviceId integer
---@return cfye.Device_Limits_t
function cfye.Device_Limits_Get(DeviceId)
    local limits = cfye.Device_Limits_t()
    _cfye.CfyE_Device_Limits_Get(DeviceId, limits)
    return limits
end

---@param DeviceId integer
---@return integer
function cfye.Device_Role_Get(DeviceId)
    return _cfye.CfyE_Device_Role_Get(DeviceId)
end

---@param DeviceId integer
---@param DeviceStatus_p? cfye.DeviceStatus_t
---@return cfye.DeviceStatus_t
function cfye.Device_Status_Get(DeviceId, DeviceStatus_p)
    DeviceStatus_p = DeviceStatus_p or cfye.DeviceStatus_t()
    _cfye.CfyE_Device_Status_Get(DeviceId, DeviceStatus_p)
    return DeviceStatus_p
end

---@param DeviceIndex integer
function cfye.Device_Uninit(DeviceIndex)
    return _cfye.CfyE_Device_Uninit(DeviceIndex)
end

---@param DeviceId integer
---@param ChannelId integer
---@param Control_p cfye.Device_t
function cfye.Device_Update(DeviceId, ChannelId, Control_p)
    _cfye.CfyE_Device_Update(DeviceId, ChannelId, Control_p)
end

---@param DeviceId integer
---@param ChannelId integer
---@param fAllChannels boolean
function cfye.Diag_Channel_Dump(DeviceId, ChannelId, fAllChannels)
    _cfye.CfyE_Diag_Channel_Dump(DeviceId, ChannelId, fAllChannels)
end

---@param DeviceId integer
function cfye.Diag_Device_Dump(DeviceId)
    _cfye.CfyE_Diag_Device_Dump(DeviceId)
end

---@param DeviceId integer
---@param MTTIndex integer
---@param fAllMTT? boolean
function cfye.Diag_MTT_Dump(DeviceId, MTTIndex, fAllMTT)
    fAllMTT = fAllMTT or false
    _cfye.CfyE_Diag_MTT_Dump(DeviceId, MTTIndex, fAllMTT)
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t|nil
---@param fAllRules boolean
function cfye.Diag_Rule_Dump(DeviceId, RuleHandle, fAllRules)
    _cfye.CfyE_Diag_Rule_Dump(DeviceId, RuleHandle, fAllRules)
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t|nil
---@param fAllvPorts boolean
---@param fIncludeRule boolean
function cfye.Diag_vPort_Dump(DeviceId, vPortHandle, fAllvPorts, fIncludeRule)
    _cfye.CfyE_Diag_vPort_Dump(DeviceId, vPortHandle, fAllvPorts, fIncludeRule)
end

---@param DeviceId integer
---@param MTTIndex integer
---@param fSync boolean
function cfye.MTT_Disable(DeviceId, MTTIndex, fSync)
    _cfye.CfyE_MTT_Disable(DeviceId, MTTIndex, fSync)
end

---@param DeviceId integer
---@param MTTIndex integer
---@param fSync boolean
function cfye.MTT_Enable(DeviceId, MTTIndex, fSync)
    _cfye.CfyE_MTT_Enable(DeviceId, MTTIndex, fSync)
end

---@param DeviceId integer
---@param MTTEnable integer
---@param MTTDisable integer
---@param EnableSingle boolean
---@param DisableSingle boolean
---@param EnableAll boolean
---@param DisableAll boolean
---@param fSync boolean
function cfye.MTT_EnableDisable(DeviceId, MTTEnable, MTTDisable, EnableSingle, DisableSingle, EnableAll, DisableAll,
                                fSync)
    _cfye.CfyE_MTT_EnableDisable(DeviceId, MTTEnable, MTTDisable, EnableSingle, DisableSingle, EnableAll, DisableAll,
                                 fSync)
end

---@param DeviceId integer
---@param MTTIndex integer
---@return cfye.MTT_t MTT_p
---@return boolean fEnabled_p
function cfye.MTT_Read(DeviceId, MTTIndex)
    local mtt = cfye.MTT_t()
    local fEnabled = _cfye.CfyE_MTT_Read(DeviceId, MTTIndex, mtt)
    return mtt, fEnabled
end

---@param DeviceId integer
---@param MTTIndex integer
---@param MTT_p cfye.MTT_t
function cfye.MTT_Update(DeviceId, MTTIndex, MTT_p)
    _cfye.CfyE_MTT_Update(DeviceId, MTTIndex, MTT_p)
end

---@param DeviceId integer
---@param RuleIndex integer
---@return cfye.RuleHandle_t RuleHandle_p
function cfye.RuleHandle_Get(DeviceId, RuleIndex)
    return _cfye.CfyE_RuleHandle_Get(DeviceId, RuleIndex)
end

---@param Handle1_p cfye.RuleHandle_t
---@param Handle2_p cfye.RuleHandle_t
---@return boolean
function cfye.RuleHandle_IsSame(Handle1_p, Handle2_p)
    return _cfye.CfyE_RuleHandle_IsSame(Handle1_p, Handle2_p)
end

---@param RuleHandle cfye.RuleHandle_t
---@return integer RuleIndex_p
function cfye.RuleIndex_Get(RuleHandle)
    return _cfye.CfyE_RuleIndex_Get(RuleHandle)
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t
---@param Rule_p cfye.Rule_t
---@return cfye.RuleHandle_t RuleHandle_p
function cfye.Rule_Add(DeviceId, vPortHandle, Rule_p)
    return _cfye.CfyE_Rule_Add(DeviceId, vPortHandle, Rule_p)
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t
---@param Rule_p cfye.Rule_t
---@param RuleIndex integer
---@return cfye.RuleHandle_t RuleHandle_p
function cfye.Rule_Add_Index(DeviceId, vPortHandle, Rule_p, RuleIndex)
    return _cfye.CfyE_Rule_Add_Index(DeviceId, vPortHandle, Rule_p, RuleIndex)
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t
---@param fSync boolean
function cfye.Rule_Disable(DeviceId, RuleHandle, fSync)
    _cfye.CfyE_Rule_Disable(DeviceId, RuleHandle, fSync)
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t
---@param fSync boolean
function cfye.Rule_Enable(DeviceId, RuleHandle, fSync)
    _cfye.CfyE_Rule_Enable(DeviceId, RuleHandle, fSync)
end

---@param DeviceId integer
---@param RuleHandleDisable cfye.RuleHandle_t
---@param RuleHandleEnable cfye.RuleHandle_t
---@param EnableAll boolean
---@param DisableAll boolean
---@param fSync boolean
function cfye.Rule_EnableDisable(DeviceId, RuleHandleDisable, RuleHandleEnable, EnableAll, DisableAll, fSync)
    _cfye.CfyE_Rule_EnableDisable(DeviceId, RuleHandleDisable, RuleHandleEnable, EnableAll, DisableAll, fSync)
end

---@param DeviceId integer
---@param CurrentRuleHandle cfye.RuleHandle_t
---@return cfye.RuleHandle_t NextRuleHandle_p
function cfye.Rule_Next_Get(DeviceId, CurrentRuleHandle)
    return _cfye.CfyE_Rule_Next_Get(DeviceId, CurrentRuleHandle)
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t
---@return cfye.Rule_t Rule_p
---@return boolean fEnabled_p
function cfye.Rule_Read(DeviceId, RuleHandle)
    local rule = cfye.Rule_t()
    local enabled = _cfye.CfyE_Rule_Read(DeviceId, RuleHandle, rule)
    return rule, enabled
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t
function cfye.Rule_Remove(DeviceId, RuleHandle)
    _cfye.CfyE_Rule_Remove(DeviceId, RuleHandle)
end

---@param DeviceId integer
---@param RuleHandle cfye.RuleHandle_t
---@param Rule_p cfye.Rule_t
function cfye.Rule_Update(DeviceId, RuleHandle, Rule_p)
    _cfye.CfyE_Rule_Update(DeviceId, RuleHandle, Rule_p)
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t
---@param CurrentRuleHandle cfye.RuleHandle_t
---@return cfye.RuleHandle_t NextRuleHandle_p
function cfye.Rule_vPort_Next_Get(DeviceId, vPortHandle, CurrentRuleHandle)
    return _cfye.CfyE_Rule_vPort_Next_Get(DeviceId, vPortHandle, CurrentRuleHandle)
end

---@param DeviceId integer
---@param StatIndex integer
function cfye.Statistics_Channel_Clear(DeviceId, StatIndex)
    _cfye.CfyE_Statistics_Channel_Clear(DeviceId, StatIndex)
end

---@param DeviceId integer
---@param StatIndex integer
---@param fSync boolean
---@return cfye.Statistics_Channel_t Stat_p
function cfye.Statistics_Channel_Get(DeviceId, StatIndex, fSync)
    local stats = cfye.Statistics_Channel_t()
    _cfye.CfyE_Statistics_Channel_Get(DeviceId, StatIndex, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param StatIndex integer
---@param fSync boolean
---@return cfye.Statistics_MTT_t Stat_p
function cfye.Statistics_MTT_Get(DeviceId, StatIndex, fSync)
    local stats = cfye.Statistics_MTT_t()
    _cfye.CfyE_Statistics_MTT_Get(DeviceId, StatIndex, fSync)
    return stats
end

---@param DeviceId integer
---@return cfye.Ch_Mask_t ChSummary_p
function cfye.Statistics_Summary_Channel_Read(DeviceId)
    local mask = cfye.Ch_Mask_t()
    _cfye.CfyE_Statistics_Summary_Channel_Read(DeviceId, mask)
    return mask
end

---@param DeviceId integer
---@return integer Summary_p
function cfye.Statistics_Summary_MTT_Read(DeviceId)
    return _cfye.CfyE_Statistics_Summary_MTT_Read(DeviceId)
end

---@param DeviceId integer
---@param StartOffset integer
---@param Count integer
---@return integer[]
function cfye.Statistics_Summary_TCAM_Read(DeviceId, StartOffset, Count)
    return _cfye.CfyE_Statistics_Summary_TCAM_Read(DeviceId, StartOffset, Count, Count)
end

---@param DeviceId integer
---@param StatIndex integer
---@param fSync boolean
---@return cfye.Statistics_TCAM_t Stat_p
function cfye.Statistics_TCAM_Get(DeviceId, StatIndex, fSync)
    local stats = cfye.Statistics_TCAM_t()
    _cfye.CfyE_Statistics_TCAM_Get(DeviceId, StatIndex, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPortIndex integer
---@return cfye.vPortHandle_t vPortHandle_p
function cfye.vPortHandle_Get(DeviceId, vPortIndex)
    return _cfye.CfyE_vPortHandle_Get(DeviceId, vPortIndex)
end

---@param Handle1_p cfye.vPortHandle_t
---@param Handle2_p cfye.vPortHandle_t
---@return boolean
function cfye.vPortHandle_IsSame(Handle1_p, Handle2_p)
    return _cfye.CfyE_vPortHandle_IsSame(Handle1_p, Handle2_p)
end

---@param vPortHandle cfye.vPortHandle_t
---@return integer
function cfye.vPortIndex_Get(vPortHandle)
    return _cfye.CfyE_vPortIndex_Get(vPortHandle)
end

---@param DeviceId integer
---@param vPort_p cfye.vPort_t
---@param ChannelMode integer
---@return cfye.vPortHandle_t vPortHandle_p
function cfye.vPort_Add(DeviceId, vPort_p, ChannelMode)
    return _cfye.CfyE_vPort_Add(DeviceId, vPort_p, ChannelMode)
end

---@param DeviceId integer
---@param CurrentvPortHandle cfye.vPortHandle_t
---@return cfye.vPortHandle_t NextvPortHandle_p
function cfye.vPort_Next_Get(DeviceId, CurrentvPortHandle)
    return _cfye.CfyE_vPort_Next_Get(DeviceId, CurrentvPortHandle)
end

function cfye.vPort_Read(DeviceId, vPortHandle)
    local vport = cfye.vPort_t()
    _cfye.CfyE_vPort_Read(DeviceId, vPortHandle, vport)
    return vport
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t
function cfye.vPort_Remove(DeviceId, vPortHandle)
    _cfye.CfyE_vPort_Remove(DeviceId, vPortHandle)
end

---@param DeviceId integer
---@param vPortHandle cfye.vPortHandle_t
---@param vPort_p cfye.vPort_t
function cfye.vPort_Update(DeviceId, vPortHandle, vPort_p)
    _cfye.CfyE_vPort_Update(DeviceId, vPortHandle, vPort_p)
end

strt.set_struct_tostrings(cfye)
return cfye
