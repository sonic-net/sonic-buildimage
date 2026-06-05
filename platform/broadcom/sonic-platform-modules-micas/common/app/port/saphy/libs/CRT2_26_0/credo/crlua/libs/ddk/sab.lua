local _sab = require "ddk._sab"
local strt = require "struct"

---@module 'ddk.sab'
local sab = {}

-- Constants & Enums --

---@type integer
sab.DIRECTION_EGRESS = _sab.SAB_DIRECTION_EGRESS
---@type integer
sab.DIRECTION_INGRESS = _sab.SAB_DIRECTION_INGRESS
---@type integer
sab.ERROR = _sab.SAB_ERROR
---@type integer
sab.INVALID_PARAMETER = _sab.SAB_INVALID_PARAMETER
---@type integer
sab.IPSEC_DISABLE_REPLAY_CHECK = _sab.SAB_IPSEC_DISABLE_REPLAY_CHECK
---@type integer
sab.IPSEC_DISABLE_SA_TAG_INSERT = _sab.SAB_IPSEC_DISABLE_SA_TAG_INSERT
---@type integer
sab.IPSEC_FLAG_PAD_CHECK = _sab.SAB_IPSEC_FLAG_PAD_CHECK
---@type integer
sab.IPSEC_FLAG_RETAIN_PADDING = _sab.SAB_IPSEC_FLAG_RETAIN_PADDING
---@type integer
sab.MACSEC_DISABLE_ICV_CHECK = _sab.SAB_MACSEC_DISABLE_ICV_CHECK
---@type integer
sab.MACSEC_DISABLE_UPDATE_SEQ_NUM = _sab.SAB_MACSEC_DISABLE_UPDATE_SEQ_NUM
---@type integer
sab.MACSEC_FLAG_LATE_HDR_CHECK = _sab.SAB_MACSEC_FLAG_LATE_HDR_CHECK
---@type integer
sab.MACSEC_FLAG_LATE_HDR_DROP = _sab.SAB_MACSEC_FLAG_LATE_HDR_DROP
---@type integer
sab.MACSEC_FLAG_LONGSEQ = _sab.SAB_MACSEC_FLAG_LONGSEQ
---@type integer
sab.MACSEC_FLAG_NO_LATE_CHECK = _sab.SAB_MACSEC_FLAG_NO_LATE_CHECK
---@type integer
sab.MACSEC_FLAG_RETAIN_ICV = _sab.SAB_MACSEC_FLAG_RETAIN_ICV
---@type integer
sab.MACSEC_FLAG_RETAIN_SECTAG = _sab.SAB_MACSEC_FLAG_RETAIN_SECTAG
---@type integer
sab.MACSEC_FLAG_ROLLOVER = _sab.SAB_MACSEC_FLAG_ROLLOVER
---@type integer
sab.MACSEC_FLAG_SA_EXPIRED_IRQ = _sab.SAB_MACSEC_FLAG_SA_EXPIRED_IRQ
---@type integer
sab.MACSEC_FLAG_SM4 = _sab.SAB_MACSEC_FLAG_SM4
---@type integer
sab.MACSEC_FLAG_UPDATE_ENABLE = _sab.SAB_MACSEC_FLAG_UPDATE_ENABLE
---@type integer
sab.MACSEC_FLAG_UPDATE_TIME = _sab.SAB_MACSEC_FLAG_UPDATE_TIME
---@type integer
sab.NULL = _sab.SAB_NULL
---@type integer
sab.OP_ENCAUTH_AES_GCM = _sab.SAB_OP_ENCAUTH_AES_GCM
---@type integer
sab.OP_ENC_AES_CTR = _sab.SAB_OP_ENC_AES_CTR
---@type integer
sab.OP_IPSEC = _sab.SAB_OP_IPSEC
---@type integer
sab.OP_MACSEC = _sab.SAB_OP_MACSEC
---@type integer
sab.STATUS_OK = _sab.SAB_STATUS_OK
---@type integer
sab.UNSUPPORTED_FEATURE = _sab.SAB_UNSUPPORTED_FEATURE

-- Structures --

---@class sab.MTU_Params_t
---@field public fDrop boolean
---@field public MTU integer
---@field public fEnable boolean

---@type fun (): sab.MTU_Params_t
sab.MTU_Params_t = _sab.SABuilder_MTU_Params_t

---@class sab.Params_t
---@field public ICVByteCount integer
---@field public SPI integer
---@field public HKey_p integer[]|nil
---@field public WindowSize integer
---@field public operation integer
---@field public SCI_p integer[]|nil
---@field public KeyByteCount integer
---@field public SeqNumLo integer
---@field public direction integer
---@field public flags integer
---@field public SSCI_p integer[]|nil
---@field public AN integer
---@field public SeqNumHi integer
---@field public Salt_p integer[]|nil
---@field public Key_p integer[]|nil

---@type fun (): sab.Params_t
sab.Params_t = _sab.SABuilder_Params_t

---@class sab.UpdCtrl_Params_t -- good
---@field public SAIndex integer
---@field public fUpdateEnable boolean
---@field public fExpiredIRQ boolean
---@field public fSAIndexValid boolean
---@field public SCIndex integer
---@field public AN integer
---@field public fUpdateTime boolean

---@type fun (): sab.UpdCtrl_Params_t
sab.UpdCtrl_Params_t = _sab.SABuilder_UpdCtrl_Params_t

---@class sab.AESCallback : userdata

---@type sab.AESCallback
sab.AES_Encrypt = _sab.AES_Encrypt

---@type sab.AESCallback
sab.SM4_Encrypt = _sab.SM4_Encrypt

-- ################# Functions ######################

---@param SAParams_p sab.Params_t
---@param AES_CB? sab.AESCallback
---@return integer[]
function sab.BuildSA(SAParams_p, AES_CB)
    local bufferSize = sab.GetSize(SAParams_p)
    return _sab.SABuilder_BuildSA(SAParams_p, bufferSize, AES_CB)
end

---@param SAParams_p sab.Params_t
---@return integer SAWord32Count_p
function sab.GetSize(SAParams_p)
    return _sab.SABuilder_GetSize(SAParams_p)
end

---@param SAParams_p sab.Params_t
---@param AN integer
---@param direction integer
---@param operation integer
---@return sab.Params_t
function sab.InitParams(SAParams_p, AN, direction, operation)
    _sab.SABuilder_InitParams(SAParams_p, AN, direction, operation)
    return SAParams_p
end

---@param ControlWord integer
---@return integer
function sab.MTUOffset_Get(ControlWord)
    return _sab.SABuilder_MTUOffset_Get(ControlWord)
end

---@param SAWord integer
---@return sab.MTU_Params_t UpdateParams_p
function sab.MTU_Decode(SAWord)
    local params = sab.MTU_Params_t()
    _sab.SABuilder_MTU_Decode(SAWord, params)
    return params
end

---@param UpdateParams_p sab.MTU_Params_t
---@return integer SAWord_p
function sab.MTU_Update(UpdateParams_p)
    return _sab.SABuilder_MTU_Update()
end

---@param ControlWord any
---@return integer SeqNumOffset_p
---@return boolean fExtPN_p
function sab.SeqNumOffset_Get(ControlWord)
    return _sab.SABuilder_SeqNumOffset_Get(ControlWord)
end

---@param ControlWord integer
---@return integer UpdCtrlOffset_p
function sab.UpdateCtrlOffset_Get(ControlWord)
    return _sab.SABuilder_UpdateCtrlOffset_Get()
end

---@param UpdateParams_p sab.UpdCtrl_Params_t
---@return integer SAWord_p
function sab.UpdateCtrl_Update(UpdateParams_p)
    return _sab.SABuilder_UpdateCtrl_Update(UpdateParams_p)
end

---@param ControlWord integer
---@return integer
function sab.WindowSizeOffset_Get(ControlWord)
    return _sab.SABuilder_WindowSizeOffset_Get(ControlWord)
end

strt.set_struct_tostrings(sab)

return sab
