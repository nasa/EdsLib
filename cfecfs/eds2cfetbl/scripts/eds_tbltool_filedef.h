#ifndef EDS_TBLTOOL_FILEDEF_H
#define EDS_TBLTOOL_FILEDEF_H

#include <stdint.h>
#include <stddef.h>

#include <cfe_sb_eds_typedefs.h>

CFE_SB_MsgIdValue_Atom_t CFE_SB_CmdTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum);
CFE_SB_MsgIdValue_Atom_t CFE_SB_TlmTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum);

uint16_t EdsTableTool_GetProcessorId(void);
void EdsTableTool_DoExport(void *arg, const void *filedefptr, const void *obj, size_t sz);


static inline CFE_SB_MsgIdValue_Atom_t CFE_SB_GlobalCmdTopicIdToMsgId(uint16_t TopicId)
{
    /* Instance number 0 is used for globals */
    return CFE_SB_CmdTopicIdToMsgId(TopicId, 0);
}

static inline CFE_SB_MsgIdValue_Atom_t CFE_SB_GlobalTlmTopicIdToMsgId(uint16_t TopicId)
{
    /* Instance number 0 is used for globals */
    return CFE_SB_TlmTopicIdToMsgId(TopicId, 0);
}

static inline CFE_SB_MsgIdValue_Atom_t CFE_SB_LocalCmdTopicIdToMsgId(uint16_t TopicId)
{
    /* PSP-reported Instance number is used for locals */
    return CFE_SB_CmdTopicIdToMsgId(TopicId, EdsTableTool_GetProcessorId());
}

static inline CFE_SB_MsgIdValue_Atom_t CFE_SB_LocalTlmTopicIdToMsgId(uint16_t TopicId)
{
    /* PSP-reported Instance number is used for locals */
    return CFE_SB_TlmTopicIdToMsgId(TopicId, EdsTableTool_GetProcessorId());
}


#endif
