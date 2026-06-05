local _secy = require "ddk._secy"

local strt = require "struct"

---@module 'ddk.secy'
local secy = {}

-- Constants & Enums --

---@type integer
secy.CHANNEL_WORDS = _secy.SECY_CHANNEL_WORDS
---@type integer
secy.CHAN_COUNT_INC_DIS = _secy.SECY_CHAN_COUNT_INC_DIS
---@type integer
secy.ECC_NOF_STATUS_COUNTERS = _secy.SECY_ECC_NOF_STATUS_COUNTERS
---@type integer
secy.ERROR_BAD_PARAMETER = _secy.SECY_ERROR_BAD_PARAMETER
---@type integer
secy.ERROR_INTERNAL = _secy.SECY_ERROR_INTERNAL
---@type integer
secy.ERROR_NOT_IMPLEMENTED = _secy.SECY_ERROR_NOT_IMPLEMENTED
---@type integer
secy.EVENT_CHAN_ERR_EOP_WO_SOP = _secy.SECY_EVENT_CHAN_ERR_EOP_WO_SOP
---@type integer
secy.EVENT_CHAN_ERR_NOT_B2B = _secy.SECY_EVENT_CHAN_ERR_NOT_B2B
---@type integer
secy.EVENT_CHAN_ERR_SLOT_CHID = _secy.SECY_EVENT_CHAN_ERR_SLOT_CHID
---@type integer
secy.EVENT_CHAN_ERR_SLOT_SOP = _secy.SECY_EVENT_CHAN_ERR_SLOT_SOP
---@type integer
secy.EVENT_CHAN_ERR_SOP_WO_EOP = _secy.SECY_EVENT_CHAN_ERR_SOP_WO_EOP
---@type integer
secy.EVENT_CHAN_ERR_XFER_WO_SOP = _secy.SECY_EVENT_CHAN_ERR_XFER_WO_SOP
---@type integer
secy.EVENT_CHAN_PKT_CFY_OVERRUN = _secy.SECY_EVENT_CHAN_PKT_CFY_OVERRUN
---@type integer
secy.EVENT_CHAN_PKT_DATA_OVERRUN = _secy.SECY_EVENT_CHAN_PKT_DATA_OVERRUN
---@type integer
secy.EVENT_CHAN_RXCAM_HIT_MULT = _secy.SECY_EVENT_CHAN_RXCAM_HIT_MULT
---@type integer
secy.EVENT_CHAN_RXCAM_MISS = _secy.SECY_EVENT_CHAN_RXCAM_MISS
---@type integer
secy.EVENT_DROP_CLASS = _secy.SECY_EVENT_DROP_CLASS
---@type integer
secy.EVENT_DROP_MTU = _secy.SECY_EVENT_DROP_MTU
---@type integer
secy.EVENT_DROP_PP = _secy.SECY_EVENT_DROP_PP
---@type integer
secy.EVENT_ECC_ERR = _secy.SECY_EVENT_ECC_ERR
---@type integer
secy.EVENT_ENG_IRQ = _secy.SECY_EVENT_ENG_IRQ
---@type integer
secy.EVENT_PE_CTR_ERR = _secy.SECY_EVENT_PE_CTR_ERR
---@type integer
secy.EVENT_PE_CTX_ERR = _secy.SECY_EVENT_PE_CTX_ERR
---@type integer
secy.EVENT_PE_ECC_ERR = _secy.SECY_EVENT_PE_ECC_ERR
---@type integer
secy.EVENT_PE_IB_AUTH_FAIL = _secy.SECY_EVENT_PE_IB_AUTH_FAIL
---@type integer
secy.EVENT_PE_IB_SL = _secy.SECY_EVENT_PE_IB_SL
---@type integer
secy.EVENT_PE_IB_ZERO_PKTNUM = _secy.SECY_EVENT_PE_IB_ZERO_PKTNUM
---@type integer
secy.EVENT_PE_LENGTH_ERR0 = _secy.SECY_EVENT_PE_LENGTH_ERR0
---@type integer
secy.EVENT_PE_LENGTH_ERR1 = _secy.SECY_EVENT_PE_LENGTH_ERR1
---@type integer
secy.EVENT_PE_LENGTH_ERR2 = _secy.SECY_EVENT_PE_LENGTH_ERR2
---@type integer
secy.EVENT_PE_OB_EXPANSION_ERR = _secy.SECY_EVENT_PE_OB_EXPANSION_ERR
---@type integer
secy.EVENT_PE_OB_MTU_CHECK = _secy.SECY_EVENT_PE_OB_MTU_CHECK
---@type integer
secy.EVENT_PE_OB_SEQNR_RO = _secy.SECY_EVENT_PE_OB_SEQNR_RO
---@type integer
secy.EVENT_PE_OB_SEQNR_THR = _secy.SECY_EVENT_PE_OB_SEQNR_THR
---@type integer
secy.EVENT_PE_TOKEN_ERR = _secy.SECY_EVENT_PE_TOKEN_ERR
---@type integer
secy.EVENT_SA_EXPIRED = _secy.SECY_EVENT_SA_EXPIRED
---@type integer
secy.EVENT_SA_PN_THR = _secy.SECY_EVENT_SA_PN_THR
---@type integer
secy.EVENT_STAT_IFC1_THR = _secy.SECY_EVENT_STAT_IFC1_THR
---@type integer
secy.EVENT_STAT_IFC_THR = _secy.SECY_EVENT_STAT_IFC_THR
---@type integer
secy.EVENT_STAT_RXCAM_THR = _secy.SECY_EVENT_STAT_RXCAM_THR
---@type integer
secy.EVENT_STAT_SA_THR = _secy.SECY_EVENT_STAT_SA_THR
---@type integer
secy.EVENT_STAT_SECY_THR = _secy.SECY_EVENT_STAT_SECY_THR
---@type integer
secy.FRAME_VALIDATE_CHECK = _secy.SECY_FRAME_VALIDATE_CHECK
---@type integer
secy.FRAME_VALIDATE_DISABLE = _secy.SECY_FRAME_VALIDATE_DISABLE
---@type integer
secy.FRAME_VALIDATE_STRICT = _secy.SECY_FRAME_VALIDATE_STRICT
---@type integer
secy.IFC1_COUNT_INC_DIS = _secy.SECY_IFC1_COUNT_INC_DIS
---@type integer
secy.IFC_COUNT_INC_DIS = _secy.SECY_IFC_COUNT_INC_DIS
---@type integer
secy.MAX_NOF_CHANNELS = _secy.SECY_MAX_NOF_CHANNELS
---@type integer
secy.MODE_INVALID = _secy.SECY_MODE_INVALID
---@type integer
secy.MODE_IPSEC = _secy.SECY_MODE_IPSEC
---@type integer
secy.MODE_MACSEC = _secy.SECY_MODE_MACSEC
---@type integer
secy.NULL = _secy.SECY_NULL
---@type integer
secy.PORT_COMMON = _secy.SECY_PORT_COMMON
---@type integer
secy.PORT_CONTROLLED = _secy.SECY_PORT_CONTROLLED
---@type integer
secy.PORT_RESERVED = _secy.SECY_PORT_RESERVED
---@type integer
secy.PORT_UNCONTROLLED = _secy.SECY_PORT_UNCONTROLLED
---@type integer
secy.ROLE_EGRESS = _secy.SECY_ROLE_EGRESS
---@type integer
secy.ROLE_EGRESS_INGRESS = _secy.SECY_ROLE_EGRESS_INGRESS
---@type integer
secy.ROLE_INGRESS = _secy.SECY_ROLE_INGRESS
---@type integer
secy.ROLE_IPSEC_EGRESS = _secy.SECY_ROLE_IPSEC_EGRESS
---@type integer
secy.ROLE_IPSEC_INGRESS = _secy.SECY_ROLE_IPSEC_INGRESS
---@type integer
secy.RXCAM_COUNT_INC_DIS = _secy.SECY_RXCAM_COUNT_INC_DIS
---@type integer
secy.SA_ACTION_BYPASS = _secy.SECY_SA_ACTION_BYPASS
---@type integer
secy.SA_ACTION_CRYPT_AUTH = _secy.SECY_SA_ACTION_CRYPT_AUTH
---@type integer
secy.SA_ACTION_DROP = _secy.SECY_SA_ACTION_DROP
---@type integer
secy.SA_ACTION_EGRESS = _secy.SECY_SA_ACTION_EGRESS
---@type integer
secy.SA_ACTION_INGRESS = _secy.SECY_SA_ACTION_INGRESS
---@type integer
secy.SA_ACTION_IPSEC_EGRESS = _secy.SECY_SA_ACTION_IPSEC_EGRESS
---@type integer
secy.SA_ACTION_IPSEC_INGRESS = _secy.SECY_SA_ACTION_IPSEC_INGRESS
---@type integer
secy.SA_COUNT_INC_DIS = _secy.SECY_SA_COUNT_INC_DIS
---@type integer
secy.SA_DROP_CRC_ERROR = _secy.SECY_SA_DROP_CRC_ERROR
---@type integer
secy.SA_DROP_INTERNAL = _secy.SECY_SA_DROP_INTERNAL
---@type integer
secy.SA_DROP_NONE = _secy.SECY_SA_DROP_NONE
---@type integer
secy.SA_DROP_PKT_ERROR = _secy.SECY_SA_DROP_PKT_ERROR
---@type integer
secy.SECY_COUNT_INC_DIS = _secy.SECY_SECY_COUNT_INC_DIS
---@type integer
secy.STATUS_OK = _secy.SECY_STATUS_OK

-- Structs --

---@class secy.Ch_Mask_t
---@field public ch_bitmask integer[]

---@type fun (): secy.Ch_Mask_t
secy.Ch_Mask_t = _secy.SecY_Ch_Mask_t

---@class secy.ChannelConf_t
---@field public Params secy.ChannelParams_t

---@type fun (): secy.ChannelConf_t
secy.ChannelConf_t = _secy.SecY_ChannelConf_t

---@class secy.ChannelParams_t
---@field public ChannelCount integer
---@field public Channel_p secy.Channel_t[]|nil

---@type fun (): secy.ChannelParams_t
secy.ChannelParams_t = _secy.SecY_ChannelParams_t

---@class secy.Channel_Rule_SecTAG_t
---@field public fCheckV boolean
---@field public fCheckSC boolean
---@field public fCheckPN boolean
---@field public fCompEType boolean
---@field public EtherType integer
---@field public fCheckKay boolean
---@field public fCheckSL boolean
---@field public fCheckSLExt boolean
---@field public fCheckCE boolean

---@type fun (): secy.Channel_Rule_SecTAG_t
secy.Channel_Rule_SecTAG_t = _secy.SecY_Channel_Rule_SecTAG_t

---@class secy.Channel_StatControl_t
---@field public SeqNrThreshold integer
---@field public SeqNrThreshold64 secy.Stat_Counter_t

---@type fun (): secy.Channel_StatControl_t
secy.Channel_StatControl_t = _secy.SecY_Channel_StatControl_t

---@class secy.Channel_t
---@field public fPktNumThrMode boolean
---@field public fPad_NH_Check boolean
---@field public fPadLenCheck boolean
---@field public EtherType integer
---@field public fPadPatternCheck boolean
---@field public Latency integer
---@field public fIPsec boolean
---@field public fBypassMode boolean
---@field public fPadDummyCheck boolean
---@field public BurstLimit integer
---@field public fBypassStatus boolean
---@field public fShouldSecure boolean
---@field public fLowLatencyBypass boolean
---@field public RuleSecTAG secy.Channel_Rule_SecTAG_t
---@field public fLatencyEnable boolean
---@field public StatCtrl secy.Channel_StatControl_t
---@field public ChannelId integer

---@type fun (): secy.Channel_t
secy.Channel_t = _secy.SecY_Channel_t

---@class secy.DeviceStatus_t
---@field public ECCStatus_p secy.ECCStatus_t|nil
---@field public PktProcessDebug_p secy.PktProcessDebug_t|nil

---@type fun (): secy.DeviceStatus_t
secy.DeviceStatus_t = _secy.SecY_DeviceStatus_t

---@class secy.Device_Params_t
---@field public ChConf_p secy.ChannelConf_t|nil
---@field public SARetireConf_p secy.SARetireConf_t|nil
---@field public EOPConf_p secy.EOPConf_t|nil
---@field public StatControl_p secy.Statistics_Control_t|nil
---@field public ECCConf_p secy.ECCConf_t|nil

---@type fun (): secy.Device_Params_t
secy.Device_Params_t = _secy.SecY_Device_Params_t

---@class secy.ECCConf_t
---@field public ECCUncorrectableThr integer
---@field public ECCCorrectableThr integer

---@type fun (): secy.ECCConf_t
secy.ECCConf_t = _secy.SecY_ECCConf_t

---@class secy.ECCStatus_t
---@field public Counters secy.ECCStatus_t_Counters

---@type fun (): secy.ECCStatus_t
secy.ECCStatus_t = _secy.SecY_ECCStatus_t

---@class secy.ECCStatus_t_Counters
---@field public fCorrectableThr boolean
---@field public UncorrectableCount integer
---@field public CorrectableCount integer
---@field public fUncorrectableThr boolean

---@type fun (): secy.ECCStatus_t_Counters
secy.ECCStatus_t_Counters = _secy.SecY_ECCStatus_t_Counters

---@class secy.EOPConf_t
---@field public EOPTimeoutVal integer
---@field public EOPTimeoutCtrl secy.Ch_Mask_t

---@type fun (): secy.EOPConf_t
secy.EOPConf_t = _secy.SecY_EOPConf_t

---@class secy.Ifc_Stat_E_t
---@field public OutOctetsUncontrolled secy.Stat_Counter_t
---@field public OutPktsBroadcastUncontrolled secy.Stat_Counter_t
---@field public OutPktsMulticastControlled secy.Stat_Counter_t
---@field public OutPktsUnicastUncontrolled secy.Stat_Counter_t
---@field public OutOctetsControlled secy.Stat_Counter_t
---@field public OutPktsBroadcastControlled secy.Stat_Counter_t
---@field public OutOctetsCommon secy.Stat_Counter_t
---@field public OutPktsUnicastControlled secy.Stat_Counter_t
---@field public OutPktsMulticastUncontrolled secy.Stat_Counter_t

---@type fun (): secy.Ifc_Stat_E_t
secy.Ifc_Stat_E_t = _secy.SecY_Ifc_Stat_E_t

---@class secy.Ifc_Stat_I_t
---@field public InOctetsControlled secy.Stat_Counter_t
---@field public InOctetsUncontrolled secy.Stat_Counter_t
---@field public InPktsUnicastUncontrolled secy.Stat_Counter_t
---@field public InPktsUnicastControlled secy.Stat_Counter_t
---@field public InPktsBroadcastControlled secy.Stat_Counter_t
---@field public InPktsBroadcastUncontrolled secy.Stat_Counter_t
---@field public InPktsMulticastControlled secy.Stat_Counter_t
---@field public InPktsMulticastUncontrolled secy.Stat_Counter_t

---@type fun (): secy.Ifc_Stat_I_t
secy.Ifc_Stat_I_t = _secy.SecY_Ifc_Stat_I_t

---@class secy.Ifc_Stat_t
---@field public Ingress secy.Ifc_Stat_I_t
---@field public Egress secy.Ifc_Stat_E_t

---@type fun (): secy.Ifc_Stat_t
secy.Ifc_Stat_t = _secy.SecY_Ifc_Stat_t

---@class secy.Notify_t
---@field public CBFunc_p userdata|nil
---@field public ChannelId integer
---@field public EventMask integer
---@field public fGlobal boolean

---@type fun (): secy.Notify_t
secy.Notify_t = _secy.SecY_Notify_t

---@class secy.PktProcessDebug_t
---@field public RxCAMSCIHi integer
---@field public SAMPP_Dbg integer
---@field public ParserInDebug integer
---@field public ParsedDALo integer
---@field public SecTAGDebug integer
---@field public RxCAMSCILo integer
---@field public ParsedDAHi integer
---@field public ParsedSCIHi integer
---@field public ParsedSAHi integer
---@field public ParsedSALo integer
---@field public ParsedSecTAGLo integer
---@field public ParsedSecTAGHi integer
---@field public ParsedSCILo integer

---@type fun (): secy.PktProcessDebug_t
secy.PktProcessDebug_t = _secy.SecY_PktProcessDebug_t

---@class secy.Rules_NonSA_t
---@field public DropType integer
---@field public fBypass boolean
---@field public DestPort integer
---@field public fDropNonReserved boolean

---@type fun (): secy.Rules_NonSA_t
secy.Rules_NonSA_t = _secy.SecY_Rules_NonSA_t

---@class secy.RxCAM_Stat_t
---@field public CAMHit secy.Stat_Counter_t

---@type fun (): secy.RxCAM_Stat_t
secy.RxCAM_Stat_t = _secy.SecY_RxCAM_Stat_t

---@class secy.SAHandle_t
---@field public p lightuserdata|nil

---@type fun (): secy.SAHandle_t
secy.SAHandle_t = _secy.SecY_SAHandle_t

---@type secy.SAHandle_t
secy.SAHandle_NULL = _secy.SecY_SAHandle_NULL

---@class secy.SARetireConf_t
---@field public SATimeout integer
---@field public TimerPrescale integer

---@type fun (): secy.SARetireConf_t
secy.SARetireConf_t = _secy.SecY_SARetireConf_t

---@class secy.SA_BD_t
---@field public fSAInUse boolean

---@type fun (): secy.SA_BD_t
secy.SA_BD_t = _secy.SecY_SA_BD_t

---@class secy.SA_CA_t
---@field public IVMode integer
---@field public fZeroLengthMessage boolean
---@field public fConfProtect boolean
---@field public fICVVerify boolean
---@field public ConfidentialityOffset integer
---@field public fICVAppend boolean

---@type fun (): secy.SA_CA_t
secy.SA_CA_t = _secy.SecY_SA_CA_t

---@class secy.SA_E_t
---@field public PreSecTagAuthStart integer
---@field public fProtectFrames boolean
---@field public fUseES boolean
---@field public PreSecTagAuthLength integer
---@field public fAllowDataPkts boolean
---@field public fSAInUse boolean
---@field public fIncludeSCI boolean
---@field public ConfidentialityOffset integer
---@field public fUseSCB boolean
---@field public fConfProtect boolean

---@type fun (): secy.SA_E_t
secy.SA_E_t = _secy.SecY_SA_E_t

---@class secy.SA_IPsec_E_t
---@field public fIgHdrInsert boolean
---@field public fProtectFrames boolean
---@field public fConfProtect boolean
---@field public fSAInUse boolean
---@field public fOuterIPHdr boolean
---@field public fUpdateUDP boolean
---@field public fRollOverMode boolean
---@field public CryptoAlg integer
---@field public fNAT_UDP boolean
---@field public fUpdateIP boolean
---@field public fReplayCheck boolean
---@field public fEncrAuth boolean

---@type fun (): secy.SA_IPsec_E_t
secy.SA_IPsec_E_t = _secy.SecY_SA_IPsec_E_t

---@class secy.SA_IPsec_I_t
---@field public fPadLenFailDrop boolean
---@field public fIgHdrInsert boolean
---@field public fPadNotValidDrop boolean
---@field public fPadCheck boolean
---@field public fSAInUse boolean
---@field public fConfProtect boolean
---@field public fRetainPad boolean
---@field public fUpdateTTL boolean
---@field public fReplayProtect boolean
---@field public fReplayCheck boolean
---@field public fUpdateIP boolean

---@type fun (): secy.SA_IPsec_I_t
secy.SA_IPsec_I_t = _secy.SecY_SA_IPsec_I_t

---@class secy.SA_I_t
---@field public ValidateFramesTagged integer
---@field public PreSecTagAuthStart integer
---@field public fSAInUse boolean
---@field public fAllowTagged boolean
---@field public SCI_p integer[]
---@field public AN integer
---@field public fAllowUntagged boolean
---@field public fRetainSecTAG boolean
---@field public fRetainICV boolean
---@field public PreSecTagAuthLength integer
---@field public fValidateUntagged boolean
---@field public fReplayProtect boolean
---@field public fRetire boolean
---@field public ConfidentialityOffset integer

---@type fun (): secy.SA_I_t
secy.SA_I_t = _secy.SecY_SA_I_t

---@class secy.SA_Stat_E_t
---@field public OutOctetsEncryptedProtected secy.Stat_Counter_t
---@field public OutPktsTooLong secy.Stat_Counter_t
---@field public OutPktsEncryptedProtected secy.Stat_Counter_t
---@field public OutPktsSANotInUse secy.Stat_Counter_t

---@type fun (): secy.SA_Stat_E_t
secy.SA_Stat_E_t = _secy.SecY_SA_Stat_E_t

---@class secy.SA_Stat_IPsec_I_t
---@field public InPktsOK secy.Stat_Counter_t
---@field public InPktsReplayed secy.Stat_Counter_t
---@field public InPktsPadLenFail secy.Stat_Counter_t
---@field public InOctetsDecrypted secy.Stat_Counter_t
---@field public InPktsPadNotValid secy.Stat_Counter_t
---@field public InOctetsValidated secy.Stat_Counter_t
---@field public InPktsLate secy.Stat_Counter_t
---@field public InPktsNotValid secy.Stat_Counter_t
---@field public InPktsNotUsingSA secy.Stat_Counter_t
---@field public InPktsPadDummy secy.Stat_Counter_t

---@type fun (): secy.SA_Stat_IPsec_I_t
secy.SA_Stat_IPsec_I_t = _secy.SecY_SA_Stat_IPsec_I_t

---@class secy.SA_Stat_I_t
---@field public InPktsOK secy.Stat_Counter_t
---@field public InPktsInvalid secy.Stat_Counter_t
---@field public InPktsNotValid secy.Stat_Counter_t
---@field public InPktsUnusedSA secy.Stat_Counter_t
---@field public InOctetsValidated secy.Stat_Counter_t
---@field public InPktsUnchecked secy.Stat_Counter_t
---@field public InPktsLate secy.Stat_Counter_t
---@field public InOctetsDecrypted secy.Stat_Counter_t
---@field public InPktsDelayed secy.Stat_Counter_t
---@field public InPktsNotUsingSA secy.Stat_Counter_t

---@type fun (): secy.SA_Stat_I_t
secy.SA_Stat_I_t = _secy.SecY_SA_Stat_I_t

---@class secy.SA_Stat_t
---@field public Ingress secy.SA_Stat_I_t
---@field public IPsecIngress secy.SA_Stat_IPsec_I_t
---@field public Egress secy.SA_Stat_E_t

---@type fun (): secy.SA_Stat_t
secy.SA_Stat_t = _secy.SecY_SA_Stat_t

---@class secy.SA_t
---@field public SA_WordCount integer
---@field public DestPort integer
---@field public DropType integer
---@field public TransformRecord_p integer[]|nil
---@field public Params secy.SA_t_Params
---@field public ActionType integer

---@type fun (): secy.SA_t
secy.SA_t = _secy.SecY_SA_t

---@class secy.SA_t_Params
---@field public Ingress secy.SA_I_t
---@field public IPsecIngress secy.SA_IPsec_I_t
---@field public BypassDrop secy.SA_BD_t
---@field public CryptAuth secy.SA_CA_t
---@field public IPsecEgress secy.SA_IPsec_E_t
---@field public Egress secy.SA_E_t

---@type fun (): secy.SA_t_Params
secy.SA_t_Params = _secy.SecY_SA_t_Params

---@class secy.SC_Rule_MTUCheck_t
---@field public fOverSizeDrop boolean
---@field public PacketMaxByteCount integer

---@type fun (): secy.SC_Rule_MTUCheck_t
secy.SC_Rule_MTUCheck_t = _secy.SecY_SC_Rule_MTUCheck_t

---@class secy.SecY_Stat_E_t
---@field public OutPktsControl secy.Stat_Counter_t
---@field public OutPktsSANotInUse secy.Stat_Counter_t
---@field public OutPktsUntagged secy.Stat_Counter_t
---@field public OutPktsTransformError secy.Stat_Counter_t

---@type fun (): secy.SecY_Stat_E_t
secy.SecY_Stat_E_t = _secy.SecY_SecY_Stat_E_t

---@class secy.SecY_Stat_IPsec_E_t
---@field public OutPktsSANotInUse secy.Stat_Counter_t
---@field public OutPktsUntagged secy.Stat_Counter_t
---@field public OutPktsTransformError secy.Stat_Counter_t

---@type fun (): secy.SecY_Stat_IPsec_E_t
secy.SecY_Stat_IPsec_E_t = _secy.SecY_SecY_Stat_IPsec_E_t

---@class secy.SecY_Stat_IPsec_I_t
---@field public InPktsTransformError secy.Stat_Counter_t
---@field public InPktsLateHdr secy.Stat_Counter_t
---@field public InPktsSANotInUse secy.Stat_Counter_t
---@field public InPktsIPMismatch secy.Stat_Counter_t

---@type fun (): secy.SecY_Stat_IPsec_I_t
secy.SecY_Stat_IPsec_I_t = _secy.SecY_SecY_Stat_IPsec_I_t

---@class secy.SecY_Stat_I_t
---@field public InPktsNoTag secy.Stat_Counter_t
---@field public InPktsTaggedCtrl secy.Stat_Counter_t
---@field public InPktsControl secy.Stat_Counter_t
---@field public InPktsUntagged secy.Stat_Counter_t
---@field public InPktsSANotInUse secy.Stat_Counter_t
---@field public InPktsTransformError secy.Stat_Counter_t
---@field public InPktsNoSCI secy.Stat_Counter_t
---@field public InPktsBadTag secy.Stat_Counter_t
---@field public InPktsUnknownSCI secy.Stat_Counter_t

---@type fun (): secy.SecY_Stat_I_t
secy.SecY_Stat_I_t = _secy.SecY_SecY_Stat_I_t

---@class secy.SecY_Stat_t
---@field public IPsecEgress secy.SecY_Stat_IPsec_E_t
---@field public Ingress secy.SecY_Stat_I_t
---@field public IPsecIngress secy.SecY_Stat_IPsec_I_t
---@field public Egress secy.SecY_Stat_E_t

---@type fun (): secy.SecY_Stat_t
secy.SecY_Stat_t = _secy.SecY_SecY_Stat_t

---@class secy.Settings_t
---@field public SACountOctetThrHi integer
---@field public IFCCountOctetThrLo integer
---@field public SeqNrThreshold64Hi integer
---@field public SACountFrameThrLo integer
---@field public SecYCountFrameThrLo integer
---@field public RxCAMCountFrameThrHi integer
---@field public SeqNrThreshold integer
---@field public IFCCountOctetThrHi integer
---@field public EOPTimeoutCtrl secy.Ch_Mask_t
---@field public ECCCorrectableThr integer
---@field public SACountFrameThrHi integer
---@field public IFCCountFrameThrLo integer
---@field public ECCUncorrectableThr integer
---@field public IngressHdrEtype integer
---@field public MaxvPortCount integer
---@field public fFlowCyptAuth boolean
---@field public IFCCountFrameThrHi integer
---@field public IFC1CountOctetThrLo integer
---@field public IFC1CountFrameThrHi integer
---@field public MaxSCCount integer
---@field public SARetireTimerPrescale integer
---@field public SACountOctetThrLo integer
---@field public SARetireTimeout integer
---@field public MaxSACount integer
---@field public fDropControlPkts boolean
---@field public MaxChannelCount integer
---@field public EOPTimeoutVal integer
---@field public RxCAMCountFrameThrLo integer
---@field public SeqNrThreshold64Lo integer
---@field public DropBypass secy.Rules_NonSA_t
---@field public CountIncDisCtrl integer
---@field public IFC1CountOctetThrHi integer
---@field public SecYCountFrameThrHi integer
---@field public IFC1CountFrameThrLo integer

---@type fun (): secy.Settings_t
secy.Settings_t = _secy.SecY_Settings_t

---@class secy.Stat_Counter_t
---@field public Hi integer
---@field public Lo integer

---@type fun (): secy.Stat_Counter_t
secy.Stat_Counter_t = _secy.SecY_Stat_Counter_t

---@class secy.Statistics_Control_t
---@field public IFC1CountFrameThr secy.Stat_Counter_t
---@field public fAutoStatCntrsReset boolean
---@field public SeqNrThreshold64 secy.Stat_Counter_t
---@field public RxCAMCountFrameThr secy.Stat_Counter_t
---@field public IFCCountFrameThr secy.Stat_Counter_t
---@field public SecYCountFrameThr secy.Stat_Counter_t
---@field public IFCCountOctetThr secy.Stat_Counter_t
---@field public SACountFrameThr secy.Stat_Counter_t
---@field public SeqNrThreshold integer
---@field public CountIncDisCtrl integer
---@field public IFC1CountOctetThr secy.Stat_Counter_t
---@field public SACountOctetThr secy.Stat_Counter_t

---@type fun (): secy.Statistics_Control_t
secy.Statistics_Control_t = _secy.SecY_Statistics_Control_t

---@param DeviceId integer
---@param ChannelId integer
---@return boolean fBypass_p
function secy.Channel_Bypass_Get(DeviceId, ChannelId)
    ---  _secy.SecY_Channel_Bypass_Get()
    return _secy.SecY_Channel_Bypass_Get(DeviceId, ChannelId)
end

---@param DeviceId integer
---@param ChannelId integer
---@param fBypass boolean
function secy.Channel_Bypass_Set(DeviceId, ChannelId, fBypass)
    _secy.SecY_Channel_Bypass_Set(DeviceId, ChannelId, fBypass)
end

---@param DeviceId integer
---@param ChannelId integer
---@return secy.Channel_t ChannelConfig_p
function secy.Channel_Config_Get(DeviceId, ChannelId)

    local config = secy.Channel_t()
    _secy.SecY_Channel_Config_Get(DeviceId, ChannelId, config)
    return config
end

---@param DeviceId integer
---@param ChannelConfig_p secy.Channel_t
function secy.Channel_Config_Set(DeviceId, ChannelConfig_p)
    _secy.SecY_Channel_Config_Set(DeviceId, ChannelConfig_p)
end

---@param DeviceId integer
---@param ChannelId integer
---@param ChannelConfig_p secy.Channel_t
function secy.Channel_Config_Update(DeviceId, ChannelId, ChannelConfig_p)

    _secy.SecY_Channel_Config_Update(DeviceId, ChannelId, ChannelConfig_p)
end

---@param DeviceId integer
---@param ChannelId integer
---@return boolean fInFlight_p
---@return secy.Ch_Mask_t Mask_p
function secy.Channel_PacketsInflight_Get(DeviceId, ChannelId)
    local mask = secy.Ch_Mask_t()
    local in_flight = _secy.SecY_Channel_PacketsInflight_Get()
    return in_flight, mask
end

---@param DeviceId integer
---@return integer BypassLength_p
function secy.CryptAuth_BypassLen_Get(DeviceId)
    return _secy.SecY_CryptAuth_BypassLen_Get(DeviceId)
end

---@param DeviceId integer
---@param BypassLength integer
function secy.CryptAuth_BypassLen_Update(DeviceId, BypassLength)
    _secy.SecY_CryptAuth_BypassLen_Update(DeviceId, BypassLength)
end

---@param DeviceId integer
---@param Control_p secy.Device_Params_t
---@return secy.Device_Params_t
function secy.Device_Config_Get(DeviceId, Control_p)
    _secy.SecY_Device_Config_Get(DeviceId, Control_p)
    return Control_p
end

---@param DeviceId integer
---@param bound? integer size of buffer to alloc (default 1024)
---@return integer[] IfcIndexes_pp
function secy.Device_CountSummary_PIfc1_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_Device_CountSummary_PIfc1_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param bound? integer size of buffer to alloc (default 1024)
---@return integer[] IfcIndexes_pp
function secy.Device_CountSummary_PIfc_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_Device_CountSummary_PIfc_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param bound? integer size of buffer (default 1024)
---@return integer[] RxCAMIndexes_pp
function secy.Device_CountSummary_PRxCAM_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_Device_CountSummary_PRxCAM_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param bound integer
---@return integer[] SAIndexes_pp
function secy.Device_CountSummary_PSA_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_Device_CountSummary_PSA_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param bound integer
---@return integer[] SecYIndexes_pp
function secy.Device_CountSummary_PSecY_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_Device_CountSummary_PSecY_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param Role integer
---@param Settings_p secy.Settings_t
---@return secy.Settings_t
function secy.Device_Init(DeviceId, Role, Settings_p)
    _secy.SecY_Device_Init(DeviceId, Role, Settings_p)
    return Settings_p
end

---@param DeviceId integer
---@param ChannelMask_p secy.Ch_Mask_t
function secy.Device_InsertEOP(DeviceId, ChannelMask_p)
    _secy.SecY_Device_InsertEOP(DeviceId, ChannelMask_p)
end

---@param DeviceId integer
---@param ChannelMask_p secy.Ch_Mask_t
function secy.Device_InsertSOP(DeviceId, ChannelMask_p)
    _secy.SecY_Device_InsertSOP(DeviceId, ChannelMask_p)
end

---@param DeviceId integer
---@return integer MaxChannelCount_p
---@return integer MaxvPortCount_p
---@return integer MaxSACount_p
---@return integer MaxSCCount_p
function secy.Device_Limits(DeviceId)
    return _secy.SecY_Device_Limits(DeviceId)
end

---@param DeviceId integer
---@return secy.DeviceStatus_t
function secy.Device_Status_Get(DeviceId)
    local status = secy.DeviceStatus_t()
    _secy.SecY_Device_Status_Get(DeviceId, status)
    return status
end

---@param DeviceId integer
function secy.Device_Uninit(DeviceId)
    -- \n
    _secy.SecY_Device_Uninit(DeviceId)
end

---@param DeviceId integer
---@param Control_p secy.Device_Params_t
function secy.Device_Update(DeviceId, Control_p)
    _secy.SecY_Device_Update(DeviceId, Control_p)
end

---@param DeviceId integer
---@param vPort integer
function secy.Ifc_Statistics_Clear(DeviceId, vPort)
    _secy.SecY_Ifc_Statistics_Clear(DeviceId, vPort)
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.Ifc_Stat_E_t
function secy.Ifc_Statistics_E_Get(DeviceId, vPort, fSync)
    local stats = secy.Ifc_Stat_E_t()
    _secy.SecY_Ifc_Statistics_E_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.Ifc_Stat_I_t
function secy.Ifc_Statistics_I_Get(DeviceId, vPort, fSync)
    local stats = secy.Ifc_Stat_I_t()
    _secy.SecY_Ifc_Statistics_I_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param SCIndex integer
---@return secy.SC_Rule_MTUCheck_t
function secy.Rules_MTUCheck_Get(DeviceId, SCIndex)
    local rule = secy.SC_Rule_MTUCheck_t()
    _secy.SecY_Rules_MTUCheck_Get(DeviceId, SCIndex, rule)
    return rule
end

---@param DeviceId integer
---@param SCIndex integer
---@param MTUCheck_Rule_p secy.SC_Rule_MTUCheck_t
function secy.Rules_MTUCheck_Update(DeviceId, SCIndex, MTUCheck_Rule_p)

    _secy.SecY_Rules_MTUCheck_Update(DeviceId, SCIndex, MTUCheck_Rule_p)
end

---@param DeviceId integer
---@param ChannelId integer
---@param SecTag_Rules_p secy.Channel_Rule_SecTAG_t
function secy.Rules_SecTag_Update(DeviceId, ChannelId, SecTag_Rules_p)

    _secy.SecY_Rules_SecTag_Update(DeviceId, ChannelId, SecTag_Rules_p)
end

---@param DeviceId integer
---@param SCIndex integer
---@param fSync boolean
---@return secy.RxCAM_Stat_t
function secy.RxCAM_Statistics_Get(DeviceId, SCIndex, fSync)
    local stats = secy.RxCAM_Stat_t()
    _secy.SecY_RxCAM_Statistics_Get(DeviceId, SCIndex, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param SAIndex integer
---@return secy.SAHandle_t
function secy.SAHandle_Get(DeviceId, SAIndex)
    local handle = secy.SAHandle_t()
    _secy.SecY_SAHandle_Get(DeviceId, SAIndex, handle)
    return handle
end

---@param Handle1_p secy.SAHandle_t
---@param Handle2_p secy.SAHandle_t
---@return boolean
function secy.SAHandle_IsSame(Handle1_p, Handle2_p)
    return _secy.SecY_SAHandle_IsSame(Handle1_p, Handle2_p)
end

---@param SAHandle secy.SAHandle_t
---@param SAIndex integer
---@return boolean
function secy.SAHandle_SAIndex_IsSame(SAHandle, SAIndex)
    return _secy.SecY_SAHandle_SAIndex_IsSame()
end

---@param SAHandle secy.SAHandle_t
---@return integer SAIndex_p
---@return integer SCIndex_p
function secy.SAIndex_Get(SAHandle)
    return _secy.SecY_SAIndex_Get(SAHandle)
end

---@param DeviceId integer
---@param vPort integer
---@return secy.SAHandle_t
function secy.SA_Active_E_Get(DeviceId, vPort)
    local handle = secy.SAHandle_t()
    _secy.SecY_SA_Active_E_Get(DeviceId, vPort, handle)
    return handle
end

---@param DeviceId integer
---@param vPort integer
---@param SCI_p integer[]
---@return secy.SAHandle_t
function secy.SA_Active_I_Get(DeviceId, vPort, SCI_p)
    local handle = secy.SAHandle_t()
    _secy.SecY_SA_Active_I_Get(DeviceId, vPort, SCI_p, handle)
    return handle
end

---@param DeviceId integer
---@param vPort integer
---@param SA_p secy.SA_t
---@return secy.SAHandle_t
function secy.SA_Add(DeviceId, vPort, SA_p)
    local handle = secy.SAHandle_t()
    _secy.SecY_SA_Add(DeviceId, vPort, handle, SA_p)
    return handle
end

---@param DeviceId integer
---@param ActiveSAHandle secy.SAHandle_t
---@param NewSA_p secy.SA_t
---@return secy.SAHandle_t NewSA_p
function secy.SA_Chain(DeviceId, ActiveSAHandle, NewSA_p)
    local handle = secy.SAHandle_t()
    _secy.SecY_SA_Chain(DeviceId, ActiveSAHandle, handle, NewSA_p)
    return handle
end

---@param DeviceId integer
---@param bound? integer default 1024
---@return integer[]
function secy.SA_ExpiredSummary_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_SA_ExpiredSummary_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param NextPN_Lo integer
---@param NextPN_HI integer
---@return boolean
function secy.SA_NextPN_Update(DeviceId, SAHandle, NextPN_Lo, NextPN_HI)
    return _secy.SecY_SA_NextPN_Update(DeviceId, SAHandle, NextPN_Lo, NextPN_HI)
end

---@param DeviceId integer
---@param bound? integer default 1024
---@return integer[]
function secy.SA_PnThrSummary_CheckAndClear(DeviceId, bound)
    local summary, size = _secy.SecY_SA_PnThrSummary_CheckAndClear(DeviceId, bound or 1024)
    return list.slice(summary, 1, size)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param WordOffset integer
---@param WordCount integer
---@return integer[]
function secy.SA_Read(DeviceId, SAHandle, WordOffset, WordCount)
    -- use wordcount twice from swig input
    return _secy.SecY_SA_Read(DeviceId, SAHandle, WordOffset, WordCount, WordCount)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
function secy.SA_Remove(DeviceId, SAHandle)
    _secy.SecY_SA_Remove(DeviceId, SAHandle)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
function secy.SA_Statistics_Clear(DeviceId, SAHandle)
    _secy.SecY_SA_Statistics_Clear(DeviceId, SAHandle)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param fSync boolean
---@return secy.SA_Stat_E_t
function secy.SA_Statistics_E_Get(DeviceId, SAHandle, fSync)
    local stat = secy.SA_Stat_E_t()
    _secy.SecY_SA_Statistics_E_Get(DeviceId, SAHandle, stat, fSync)
    return stat
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param fSync boolean
---@return secy.SA_Stat_IPsec_I_t
function secy.SA_Statistics_IPsec_I_Get(DeviceId, SAHandle, fSync)
    local stat = secy.SA_Stat_IPsec_I_t()
    _secy.SecY_SA_Statistics_IPsec_I_Get(DeviceId, SAHandle, stat, fSync)
    return stat
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param fSync boolean
---@return secy.SA_Stat_I_t
function secy.SA_Statistics_I_Get(DeviceId, SAHandle, fSync)
    local stat = secy.SA_Stat_I_t()
    _secy.SecY_SA_Statistics_I_Get(DeviceId, SAHandle, stat, fSync)
    return stat
end

---@param DeviceId integer
---@param ActiveSAHandle secy.SAHandle_t
---@param NewSAHandle secy.SAHandle_t
---@param NewSA_p secy.SA_t
function secy.SA_Switch(DeviceId, ActiveSAHandle, NewSAHandle, NewSA_p)

    _secy.SecY_SA_Switch(DeviceId, ActiveSAHandle, NewSAHandle, NewSA_p)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param SA_p secy.SA_t
function secy.SA_Update(DeviceId, SAHandle, SA_p)
    _secy.SecY_SA_Update(DeviceId, SAHandle, SA_p)
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param WindowSize integer
function secy.SA_WindowSize_Update(DeviceId, SAHandle, WindowSize)

    _secy.SecY_SA_WindowSize_Update(DeviceId, SAHandle, WindowSize)
end

---@param DeviceId integer
---@param vPort integer
function secy.SecY_Statistics_Clear(DeviceId, vPort)
    secy.SecY_Statistics_Clear(DeviceId, vPort)
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.SecY_Stat_E_t
function secy.SecY_Statistics_E_Get(DeviceId, vPort, fSync)
    local stats = secy.SecY_Stat_E_t()
    _secy.SecY_SecY_Statistics_E_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.SecY_Stat_IPsec_E_t
function secy.SecY_Statistics_IPsec_E_Get(DeviceId, vPort, fSync)
    local stats = secy.SecY_Stat_IPsec_E_t()
    _secy.SecY_SecY_Statistics_IPsec_E_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.SecY_Stat_IPsec_I_t
function secy.SecY_Statistics_IPsec_I_Get(DeviceId, vPort, fSync)
    local stats = secy.SecY_Stat_IPsec_I_t()
    _secy.SecY_SecY_Statistics_IPsec_I_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPort integer
---@param fSync boolean
---@return secy.SecY_Stat_I_t
function secy.SecY_Statistics_I_Get(DeviceId, vPort, fSync)
    local stats = secy.SecY_Stat_I_t()
    _secy.SecY_SecY_Statistics_I_Get(DeviceId, vPort, stats, fSync)
    return stats
end

---@param DeviceId integer
---@param vPort integer
function secy.vPort_Statistics_Clear(DeviceId, vPort)
    _secy.SecY_vPort_Statistics_Clear(DeviceId, vPort)
end

---@param DeviceId integer
---@return integer
function secy.Device_Role_Get(DeviceId)
    return secy.Role_t(DeviceId)
end

---@param DeviceId integer
---@param vPortId integer
---@return secy.SA_t
function secy.vPort_Params_Read(DeviceId, vPortId)
    local sa = secy.SA_t()
    _secy.SecY_vPort_Params_Read(DeviceId, vPortId, sa)
    return sa
end

---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@return secy.SA_t SA_p
---@return integer[] SCI_p
function secy.SA_Params_Read(DeviceId, SAHandle)
    local sa = secy.SA_t()
    local sci = _secy.SecY_SA_Params_Read(DeviceId, SAHandle, sa)
    return sa, sci
end

---comment
---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@return integer
function secy.SA_vPortIndex_Get(DeviceId, SAHandle)
    return _secy.SecY_SA_vPortIndex_Get(DeviceId, SAHandle)
end

---comment
---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@return secy.SAHandle_t|nil
function secy.SA_Next_Get(DeviceId, SAHandle)
    local next_handle = secy.SAHandle_t()
    _secy.SecY_SA_Next_Get(DeviceId, SAHandle, next_handle)
    -- if null handle
    if next_handle.p == nil then
        return nil
    end
    return next_handle
end

---comment
---@param DeviceId integer
---@param CurrentSAHandle secy.SAHandle_t
---@return secy.SAHandle_t|nil
function secy.SA_Chained_Get(DeviceId, CurrentSAHandle)
    local next_handle = secy.SAHandle_t()
    _secy.SecY_SA_Chained_Get(DeviceId, CurrentSAHandle, next_handle)
    -- if null handle
    if next_handle.p == nil then
        return nil
    end
    return next_handle
end

---comment
---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@return integer
function secy.SA_WindowSize_Get(DeviceId, SAHandle)
    return _secy.SecY_SA_WindowSize_Get(DeviceId, SAHandle)
end

---comment
---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@return integer NextPN_Lo_p
---@return integer NextPN_Hi_p
---@return boolean fExtPN_p
function secy.SA_NextPN_Get(DeviceId, SAHandle)
    return _secy.SecY_SA_NextPN_Get(DeviceId, SAHandle)
end

---comment
---@param DeviceId integer
---@param SAHandle secy.SAHandle_t
---@param TmpHandle_p lightuserdata|nil
---@return lightuserdata|nil TmpHandle_p
---@return integer[] NextSCI_p
---@return integer SCIndex_p
function secy.SCI_Next_Get(DeviceId, SAHandle, TmpHandle_p)
    return _secy.SecY_SCI_Next_Get(DeviceId, SAHandle, TmpHandle_p)
end

---comment
---@param DeviceId integer
function secy.Diag_Device_Dump(DeviceId)
    _secy.SecY_Diag_Device_Dump(DeviceId)
end

---comment
---@param DeviceId integer
---@param ChannelId integer
---@param fAllChannels boolean
function secy.Diag_Channel_Dump(DeviceId, ChannelId, fAllChannels)
    _secy.SecY_Diag_Channel_Dump(DeviceId, ChannelId, fAllChannels)
end

---comment
---@param DeviceId integer
---@param vPortId integer
---@param fAllvPorts boolean
---@param fIncludeSA boolean
function secy.Diag_vPort_Dump(DeviceId, vPortId, fAllvPorts, fIncludeSA)
    _secy.SecY_Diag_vPort_Dump(DeviceId, vPortId, fAllvPorts, fIncludeSA)
end

---comment
---@param DevideId integer
---@param SAHandle secy.SAHandle_t
---@param fALLSAs boolean
function secy.Diag_SA_Dump(DevideId, SAHandle, fALLSAs)
    _secy.SecY_Diag_SA_Dump(DevideId, SAHandle, fALLSAs)
end

strt.set_struct_tostrings(_secy)

return secy
