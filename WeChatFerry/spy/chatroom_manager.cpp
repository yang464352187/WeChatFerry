﻿#pragma execution_character_set("utf-8")

#include "chatroom_manager.h"
#include "log.hpp"
#include "offsets.h"
#include "pb_util.h"
#include "rpc_helper.h"
#include "util.h"

extern QWORD g_WeChatWinDllAddr;

namespace chatroom
{
namespace OsRoom = Offsets::Chatroom;

using get_mgr_t        = QWORD (*)();
using add_member_t     = QWORD (*)(QWORD, QWORD, WxString *, QWORD);
using delete_member_t  = QWORD (*)(QWORD, QWORD, WxString *);
using invite_members_t = QWORD (*)(const wchar_t *, QWORD, WxString *, QWORD);

template <auto FillFunc, typename Func>
bool rpc_chatroom_common(const MemberMgmt &m, uint8_t *out, size_t *len, Func func)
{
    int status = -1;
    if (m.wxids && m.roomid) {
        status = func(m.roomid, m.wxids);
    } else {
        LOG_ERROR("wxid 和 roomid 不能为空");
    }
    return fill_response<FillFunc>(out, len, [&](Response &rsp) { rsp.msg.status = status; });
}

int add_chatroom_member(const string &roomid, const string &wxids)
{
    auto get_chatroom_mgr = reinterpret_cast<get_mgr_t>(g_WeChatWinDllAddr + OsRoom::MGR);
    auto add_members      = reinterpret_cast<add_member_t>(g_WeChatWinDllAddr + OsRoom::ADD);

    WxString *wx_roomid = util::CreateWxString(roomid);

    QWORD tmp[2]    = { 0 };
    auto wx_members = util::parse_wxids(wxids).wxWxids;
    QWORD p_members = reinterpret_cast<QWORD>(&wx_members);

    return static_cast<int>(add_members(get_chatroom_mgr(), p_members, wx_roomid, reinterpret_cast<QWORD>(tmp)));
}

int del_chatroom_member(const string &roomid, const string &wxids)
{
    auto get_chatroom_mgr = reinterpret_cast<get_mgr_t>(g_WeChatWinDllAddr + OsRoom::MGR);
    auto del_members      = reinterpret_cast<delete_member_t>(g_WeChatWinDllAddr + OsRoom::DEL);

    WxString *wx_roomid = util::CreateWxString(roomid);
    auto wx_members     = util::parse_wxids(wxids).wxWxids;
    QWORD p_members     = reinterpret_cast<QWORD>(&wx_members);

    return static_cast<int>(del_members(get_chatroom_mgr(), p_members, wx_roomid));
}

int invite_chatroom_member(const string &roomid, const string &wxids)
{
    auto invite_members = reinterpret_cast<invite_members_t>(g_WeChatWinDllAddr + OsRoom::INV);

    wstring ws_roomid   = util::s2w(roomid);
    WxString *wx_roomid = util::CreateWxString(roomid);

    QWORD tmp[2]    = { 0 };
    auto wx_members = util::parse_wxids(wxids).wxWxids;
    QWORD p_members = reinterpret_cast<QWORD>(&wx_members);

    return static_cast<int>(invite_members(ws_roomid.c_str(), p_members, wx_roomid, reinterpret_cast<QWORD>(tmp)));
}

bool rpc_add_chatroom_member(const MemberMgmt &m, uint8_t *out, size_t *len)
{
    return rpc_chatroom_common<Functions_FUNC_ADD_ROOM_MEMBERS>(m, out, len, add_chatroom_member);
}

bool rpc_delete_chatroom_member(const MemberMgmt &m, uint8_t *out, size_t *len)
{
    return rpc_chatroom_common<Functions_FUNC_DEL_ROOM_MEMBERS>(m, out, len, del_chatroom_member);
}

bool rpc_invite_chatroom_member(const MemberMgmt &m, uint8_t *out, size_t *len)
{
    return rpc_chatroom_common<Functions_FUNC_INV_ROOM_MEMBERS>(m, out, len, invite_chatroom_member);
}

} // namespace chatroom
