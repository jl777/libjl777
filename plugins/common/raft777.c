/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifdef DEFINES_ONLY
#ifndef raft777_h
#define raft777_h
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "../includes/portable777.h"
#include "hostnet777.c"

typedef struct { void *buf; uint32_t len; } raft_entry_data_t;

/** Message sent from client to server.
 * The client sends this message to a server with the intention of having it
 * applied to the FSM. */
typedef struct
{
    /** the term the entry was created */
    int32_t term;
    
    /** the entry's unique ID
     * The ID is used to prevent duplicate entries from being appended */
    uint32_t id;
    
    raft_entry_data_t data;
} msg_entry_t;

/** Entry message response.
 * Indicates to client if entry was committed or not. */
typedef struct
{
    /** the entry's unique ID */
    uint32_t id;
    
    /** the entry's term */
    int32_t term;
    
    /** the entry's index */
    int32_t idx;
} msg_entry_response_t;

/** Vote request message.
 * Sent to nodes when a server wants to become leader.
 * This message could force a leader/candidate to become a follower. */
typedef struct
{
    /** currentTerm, to force other leader/candidate to step down */
    int32_t term;
    
    /** candidate requesting vote */
    int32_t candidate_id;
    
    /** index of candidate's last log entry */
    int32_t last_log_idx;
    
    /** term of candidate's last log entry */
    int32_t last_log_term;
} msg_requestvote_t;

/** Vote request response message.
 * Indicates if node has accepted the server's vote request. */
typedef struct
{
    /** currentTerm, for candidate to update itself */
    int32_t term;
    
    /** true means candidate received vote */
    int32_t vote_granted;
} msg_requestvote_response_t;

/** Appendentries message.
 * This message is used to tell nodes if it's safe to apply entries to the FSM.
 * Can be sent without any entries as a keep alive message.
 * This message could force a leader/candidate to become a follower. */
typedef struct
{
    /** currentTerm, to force other leader/candidate to step down */
    int32_t term;
    
    /** the index of the log just before the newest entry for the node who
     * receives this message */
    int32_t prev_log_idx;
    
    /** the term of the log just before the newest entry for the node who
     * receives this message */
    int32_t prev_log_term;
    
    /** the index of the entry that has been appended to the majority of the
     * cluster. Entries up to this index will be applied to the FSM */
    int32_t leader_commit;
    
    /** number of entries within this message */
    int32_t n_entries;
    
    /** array of entries within this message */
    msg_entry_t* entries;
} msg_appendentries_t;

/** Appendentries response message.
 * Can be sent without any entries as a keep alive message.
 * This message could force a leader/candidate to become a follower. */
typedef struct
{
    /** currentTerm, to force other leader/candidate to step down */
    int32_t term;
    
    /** true if follower contained entry matching prevLogidx and prevLogTerm */
    int32_t success;
    
    /* Non-Raft fields follow: */
    /* Having the following fields allows us to do less book keeping in
     * regards to full fledged RPC */
    
    /** This is the highest log IDX we've received and appended to our log */
    int32_t current_idx;
    
    /** The first idx that we received within the appendentries message */
    int32_t first_idx;
} msg_appendentries_response_t;

typedef void* raft_server_t;
typedef void* raft_node_t;

/** Entry that is stored in the server's entry log. */
typedef struct
{
    /** the entry's term at the point it was created */
    uint32_t term;
    
    /** the entry's unique ID */
    uint32_t id;
    
    /** number of nodes that have this entry */
    uint32_t num_nodes;
    
    raft_entry_data_t data;
} raft_entry_t;

/** Callback for sending request vote messages.
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] node The node's ID that we are sending this message to
 * @param[in] msg The request vote message to be sent
 * @return 0 on success */
typedef int32_t (
*func_send_requestvote_f
)   (
raft_server_t* raft,
void *user_data,
int32_t node,
msg_requestvote_t* msg
);

/** Callback for sending append entries messages.
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] node The node's ID that we are sending this message to
 * @param[in] msg The appendentries message to be sent
 * @return 0 on success */
typedef int32_t (
*func_send_appendentries_f
)   (
raft_server_t* raft,
void *user_data,
int32_t node,
msg_appendentries_t* msg
);

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
/** Callback for providing debug logging information.
 * This callback is optional
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] buf The buffer that was logged */
typedef void (
*func_log_f
)    (
raft_server_t* raft,
void *user_data,
const char *buf
);
#endif

/** Callback for applying this log entry to the state machine.
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] data Data to be applied to the log
 * @param[in] len Length in bytes of data to be applied
 * @return 0 on success */
typedef int32_t (
*func_applylog_f
)   (
raft_server_t* raft,
void *user_data,
const unsigned char *log_data,
const int32_t log_len
);

/** Callback for saving who we voted for to disk.
 * For safety reasons this callback MUST flush the change to disk.
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] voted_for The node we voted for
 * @return 0 on success */
typedef int32_t (
*func_persist_int_f
)   (
raft_server_t* raft,
void *user_data,
const int32_t voted_for
);

/** Callback for saving log entry changes.
 *
 * This callback is used for:
 * <ul>
 *      <li>Adding entries to the log (ie. offer)
 *      <li>Removing the first entry from the log (ie. polling)
 *      <li>Removing the last entry from the log (ie. popping)
 * </ul>
 *
 * For safety reasons this callback MUST flush the change to disk.
 *
 * @param[in] raft The Raft server making this callback
 * @param[in] user_data User data that is passed from Raft server
 * @param[in] entry The entry that the event is happening to.
 *    The user is allowed to change the memory pointed to in the
 *    raft_entry_data_t struct. This MUST be done if the memory is temporary.
 * @param[in] entry_idx The entries index in the log
 * @return 0 on success */
typedef int32_t (
*func_logentry_event_f
)   (
raft_server_t* raft,
void *user_data,
raft_entry_t *entry,
int32_t entry_idx
);

typedef struct
{
    /** Callback for sending request vote messages */
    func_send_requestvote_f send_requestvote;
    
    /** Callback for sending appendentries messages */
    func_send_appendentries_f send_appendentries;
    
    /** Callback for finite state machine application */
    func_applylog_f applylog;
    
    /** Callback for persisting vote data
     * For safety reasons this callback MUST flush the change to disk. */
    func_persist_int_f persist_vote;
    
    /** Callback for persisting term data
     * For safety reasons this callback MUST flush the change to disk. */
    func_persist_int_f persist_term;
    
    /** Callback for adding an entry to the log
     * For safety reasons this callback MUST flush the change to disk. */
    func_logentry_event_f log_offer;
    
    /** Callback for removing the oldest entry from the log
     * For safety reasons this callback MUST flush the change to disk.
     * @note If memory was malloc'd in log_offer then this should be the right
     *  time to free the memory. */
    func_logentry_event_f log_poll;
    
    /** Callback for removing the youngest entry from the log
     * For safety reasons this callback MUST flush the change to disk.
     * @note If memory was malloc'd in log_offer then this should be the right
     *  time to free the memory. */
    func_logentry_event_f log_pop;
    
    /** Callback for catching debugging log messages
     * This callback is optional */
    func_log_f log;
} raft_cbs_t;

typedef struct
{
    /** User data pointer for addressing.
     * Examples of what this could be:
     * - void* pointing to implementor's networking data
     * - a (IP,Port) tuple */
    void* udata_address;
} raft_node_configuration_t;

/** Initialise a new Raft server.
 *
 * Request timeout defaults to 200 milliseconds
 * Election timeout defaults to 1000 milliseconds
 *
 * @return newly initialised Raft server */
raft_server_t* raft_new();

/** De-initialise Raft server.
 * Frees all memory */
void raft_free(raft_server_t* me);

/** Set callbacks and user data.
 *
 * @param[in] funcs Callbacks
 * @param[in] user_data "User data" - user's context that's included in a callback */
void raft_set_callbacks(raft_server_t* me, raft_cbs_t* funcs, void* user_data);

/** Set configuration.
 *
 * @deprecated This function has been replaced by raft_add_node and
 * raft_remove_node
 *
 * @param[in] nodes Array of nodes. End of array is marked by NULL entry
 * @param[in] my_idx Index of the node that refers to this Raft server */
void raft_set_configuration(raft_server_t* me,raft_node_configuration_t* nodes, int32_t my_idx)
__attribute__ ((deprecated));

/** Add node.
 *
 * @note This library does not yet support membership changes.
 *  Once raft_periodic has been run this will fail.
 *
 * @note The order this call is made is important.
 *  This call MUST be made in the same order as the other raft nodes.
 *  This is because the node ID is assigned depending on when this call is made
 *
 * @param[in] user_data The user data for the node.
 *  This is obtained using raft_node_get_udata.
 *  Examples of what this could be:
 *  - void* pointing to implementor's networking data
 *  - a (IP,Port) tuple
 * @param[in] is_self Set to 1 if this "node" is this server
 * @return 0 on success; otherwise -1 */
int32_t raft_add_node(raft_server_t* me, void* user_data, int32_t is_self);

#define raft_add_peer raft_add_node

/** Set election timeout.
 * The amount of time that needs to elapse before we assume the leader is down
 * @param[in] msec Election timeout in milliseconds */
void raft_set_election_timeout(raft_server_t* me, int32_t msec);

/** Set request timeout in milliseconds.
 * The amount of time before we resend an appendentries message
 * @param[in] msec Request timeout in milliseconds */
void raft_set_request_timeout(raft_server_t* me, int32_t msec);

/** Process events that are dependent on time passing.
 * @param[in] msec_elapsed Time in milliseconds since the last call
 * @return 0 on success */
int32_t raft_periodic(raft_server_t* me, int32_t msec_elapsed);

/** Receive an appendentries message.
 *
 * Will block (ie. by syncing to disk) if we need to append a message.
 *
 * Might call malloc once to increase the log entry array size.
 *
 * The log_offer callback will be called.
 *
 * @note The memory pointer (ie. raft_entry_data_t) for each msg_entry_t is
 *   copied directly. If the memory is temporary you MUST either make the
 *   memory permanent (ie. via malloc) OR re-assign the memory within the
 *   log_offer callback.
 *
 * @param[in] node Index of the node who sent us this message
 * @param[in] ae The appendentries message
 * @param[out] r The resulting response
 * @return 0 on success */
int32_t raft_recv_appendentries(raft_server_t* me,
                            int32_t node,
                            msg_appendentries_t* ae,
                            msg_appendentries_response_t *r);

/** Receive a response from an appendentries message we sent.
 * @param[in] node Index of the node who sent us this message
 * @param[in] r The appendentries response message
 * @return 0 on success */
int32_t raft_recv_appendentries_response(raft_server_t* me,
                                     int32_t node,
                                     msg_appendentries_response_t* r);

/** Receive a requestvote message.
 * @param[in] node Index of the node who sent us this message
 * @param[in] vr The requestvote message
 * @param[out] r The resulting response
 * @return 0 on success */
int32_t raft_recv_requestvote(raft_server_t* me,
                          int32_t node,
                          msg_requestvote_t* vr,
                          msg_requestvote_response_t *r);

/** Receive a response from a requestvote message we sent.
 * @param[in] node The node this response was sent by
 * @param[in] r The requestvote response message
 * @return 0 on success */
int32_t raft_recv_requestvote_response(raft_server_t* me,
                                   int32_t node,
                                   msg_requestvote_response_t* r);

/** Receive an entry message from the client.
 *
 * Append the entry to the log and send appendentries to followers.
 *
 * Will block (ie. by syncing to disk) if we need to append a message.
 *
 * Might call malloc once to increase the log entry array size.
 *
 * The log_offer callback will be called.
 *
 * @note The memory pointer (ie. raft_entry_data_t) in msg_entry_t is
 *  copied directly. If the memory is temporary you MUST either make the
 *  memory permanent (ie. via malloc) OR re-assign the memory within the
 *  log_offer callback.
 *
 * Will fail:
 * <ul>
 *      <li>if the server is not the leader
 * </ul>
 *
 * @param[in] node Index of the node who sent us this message
 * @param[in] ety The entry message
 * @param[out] r The resulting response
 * @return 0 on success, -1 on failure */
int32_t raft_recv_entry(raft_server_t* me,
                    int32_t node,
                    msg_entry_t* ety,
                    msg_entry_response_t *r);

/**
 * @return the server's node ID */
int32_t raft_get_nodeid(raft_server_t* me);

/**
 * @return currently configured election timeout in milliseconds */
int32_t raft_get_election_timeout(raft_server_t* me);

/**
 * @return number of nodes that this server has */
int32_t raft_get_num_nodes(raft_server_t* me);

/**
 * @return number of items within log */
int32_t raft_get_log_count(raft_server_t* me);

/**
 * @return current term */
int32_t raft_get_current_term(raft_server_t* me);

/**
 * @return current log index */
int32_t raft_get_current_idx(raft_server_t* me);

/**
 * @return 1 if follower; 0 otherwise */
int32_t raft_is_follower(raft_server_t* me);

/**
 * @return 1 if leader; 0 otherwise */
int32_t raft_is_leader(raft_server_t* me);

/**
 * @return 1 if candidate; 0 otherwise */
int32_t raft_is_candidate(raft_server_t* me);

/**
 * @return currently elapsed timeout in milliseconds */
int32_t raft_get_timeout_elapsed(raft_server_t* me);

/**
 * @return request timeout in milliseconds */
int32_t raft_get_request_timeout(raft_server_t* me);

/**
 * @return index of last applied entry */
int32_t raft_get_last_applied_idx(raft_server_t* me);

/**
 * @return 1 if node is leader; 0 otherwise */
int32_t raft_node_is_leader(raft_node_t* node);

/**
 * @return the node's next index */
int32_t raft_node_get_next_idx(raft_node_t* node);

/**
 * @return this node's user data */
void* raft_node_get_udata(raft_node_t* me);

/**
 * Set this node's user data */
void raft_node_set_udata(raft_node_t* me, void* user_data);

/**
 * @param[in] idx The entry's index
 * @return entry from index */
raft_entry_t* raft_get_entry_from_idx(raft_server_t* me, int32_t idx);

/**
 * @param[in] node The node's index
 * @return node pointed to by node index */
raft_node_t* raft_get_node(raft_server_t *me, int32_t node);

/**
 * @return number of votes this server has received this election */
int32_t raft_get_nvotes_for_me(raft_server_t* me);

/**
 * @return node ID of who I voted for */
int32_t raft_get_voted_for(raft_server_t* me);

/** Get what this node thinks the node ID of the leader is.
 * @return node of what this node thinks is the valid leader;
 *   -1 if the leader is unknown */
int32_t raft_get_current_leader(raft_server_t* me);

/**
 * @return callback user data */
void* raft_get_udata(raft_server_t* me);

/**
 * @return this server's node ID */
int32_t raft_get_my_id(raft_server_t* me);

/** Vote for a server.
 * This should be used to reload persistent state, ie. the voted-for field.
 * @param[in] node The server to vote for */
void raft_vote(raft_server_t* me, const int32_t node);

/** Set the current term.
 * This should be used to reload persistent state, ie. the current_term field.
 * @param[in] term The new current term */
void raft_set_current_term(raft_server_t* me, const int32_t term);

/** Add an entry to the server's log.
 * This should be used to reload persistent state, ie. the commit log.
 * @param[in] ety The entry to be appended */
int32_t raft_append_entry(raft_server_t* me, raft_entry_t* ety);

/** Confirm if a msg_entry_response has been committed.
 * @param[in] r The response we want to check */
int32_t raft_msg_entry_response_committed(raft_server_t* me_,const msg_entry_response_t* r);

//#include "raft_log.h"
typedef void* log_t;

log_t* log_new();

void log_set_callbacks(log_t* me_, raft_cbs_t* funcs, void* raft);

void log_free(log_t* me_);

/**
 * Add entry to log.
 * Don't add entry if we've already added this entry (based off ID)
 * Don't add entries with ID=0
 * @return 0 if unsucessful; 1 otherwise */
int32_t log_append_entry(log_t* me_, raft_entry_t* c);

/**
 * @return number of entries held within log */
int32_t log_count(log_t* me_);

/**
 * Delete all logs from this log onwards */
void log_delete(log_t* me_, int32_t idx);

/**
 * Empty the queue. */
void log_empty(log_t * me_);

/**
 * Remove oldest entry
 * @return oldest entry */
void *log_poll(log_t * me_);

raft_entry_t* log_get_from_idx(log_t* me_, int32_t idx);

/**
 * @return youngest entry */
raft_entry_t *log_peektail(log_t * me_);

void log_mark_node_has_committed(log_t* me_, int32_t idx);

void log_delete(log_t* me_, int32_t idx);

int32_t log_get_current_idx(log_t* me_);

//#include "raft_private.h"
enum {
    RAFT_STATE_NONE,
    RAFT_STATE_FOLLOWER,
    RAFT_STATE_CANDIDATE,
    RAFT_STATE_LEADER
};

typedef struct {
    /* Persistent state: */
    
    /* the server's best guess of what the current term is
     * starts at zero */
    int32_t current_term;
    
    /* The candidate the server voted for in its current term,
     * or Nil if it hasn't voted for any.  */
    int32_t voted_for;
    
    /* the log which is replicated */
    void* log;
    
    /* Volatile state: */
    
    /* idx of highest log entry known to be committed */
    int32_t commit_idx;
    
    /* idx of highest log entry applied to state machine */
    int32_t last_applied_idx;
    
    /* follower/leader/candidate indicator */
    int32_t state;
    
    /* amount of time left till timeout */
    int32_t timeout_elapsed;
    
    /* who has voted for me. This is an array with N = 'num_nodes' elements */
    int32_t *votes_for_me;
    
    raft_node_t* nodes;
    int32_t num_nodes;
    
    int32_t election_timeout;
    int32_t request_timeout;
    
    /* what this node thinks is the node ID of the current leader, or -1 if
     * there isn't a known current leader. */
    int32_t current_leader;
    
    /* callbacks */
    raft_cbs_t cb;
    void* udata;
    
    /* my node ID */
    int32_t nodeid;
} raft_server_private_t;

void raft_election_start(raft_server_t* me);

void raft_become_leader(raft_server_t* me);

void raft_become_candidate(raft_server_t* me);

void raft_become_follower(raft_server_t* me);

void raft_vote(raft_server_t* me, int32_t node);

void raft_set_current_term(raft_server_t* me,int32_t term);

/**
 * @return 0 on error */
int32_t raft_send_requestvote(raft_server_t* me, int32_t node);

void raft_send_appendentries(raft_server_t* me, int32_t node);

void raft_send_appendentries_all(raft_server_t* me_);

/**
 * Apply entry at lastApplied + 1. Entry becomes 'committed'.
 * @return 1 if entry committed, 0 otherwise */
int32_t raft_apply_entry(raft_server_t* me_);

/**
 * Appends entry using the current term.
 * Note: we make the assumption that current term is up-to-date
 * @return 0 if unsuccessful */
int32_t raft_append_entry(raft_server_t* me_, raft_entry_t* c);

void raft_set_commit_idx(raft_server_t* me, int32_t commit_idx);
int32_t raft_get_commit_idx(raft_server_t* me_);

void raft_set_last_applied_idx(raft_server_t* me, int32_t idx);

void raft_set_state(raft_server_t* me_, int32_t state);
int32_t raft_get_state(raft_server_t* me_);

raft_node_t* raft_node_new(void* udata);

void raft_node_set_next_idx(raft_node_t* node, int32_t nextIdx);

int32_t raft_votes_is_majority(const int32_t nnodes, const int32_t nvotes);

#endif
#else
#ifndef raft777_c
#define raft777_c

#ifndef raft777_h
#define DEFINES_ONLY
#include "raft777.c"
#undef DEFINES_ONLY
#endif

typedef struct
{
    void* udata;
    int32_t next_idx;
} raft_node_private_t;

#define INITIAL_CAPACITY 10
#define in(x) ((log_private_t*)x)

typedef struct
{
    /* size of array */
    int32_t size;
    
    /* the amount of elements in the array */
    int32_t count;
    
    /* position of the queue */
    int32_t front, back;
    
    /* we compact the log, and thus need to increment the base idx */
    int32_t base_log_idx;
    
    raft_entry_t* entries;
    
    /* callbacks */
    raft_cbs_t *cb;
    void* raft;
} log_private_t;

static void __ensurecapacity(log_private_t * me)
{
    int32_t i, j;
    raft_entry_t *temp;
    
    if (me->count < me->size)
        return;
    
    temp = (raft_entry_t*)calloc(2, sizeof(raft_entry_t) * me->size * 2);
    
    for (i = 0, j = me->front; i < me->count; i++, j++)
    {
        if (j == me->size)
            j = 0;
        memcpy(&temp[i], &me->entries[j], sizeof(raft_entry_t));
    }
    
    /* clean up old entries */
    free(me->entries);
    
    me->size *= 2;
    me->entries = temp;
    me->front = 0;
    me->back = me->count;
}

log_t* log_new()
{
    log_private_t* me = (log_private_t*)calloc(2, sizeof(log_private_t));
    me->size = INITIAL_CAPACITY;
    me->count = 0;
    me->back = in(me)->front = 0;
    me->entries = (raft_entry_t*)calloc(2, sizeof(raft_entry_t) * me->size);
    return (log_t*)me;
}

void log_set_callbacks(log_t* me_, raft_cbs_t* funcs, void* raft)
{
    log_private_t* me = (log_private_t*)me_;
    
    me->raft = raft;
    me->cb = funcs;
}

int32_t log_append_entry(log_t* me_, raft_entry_t* c)
{
    log_private_t* me = (log_private_t*)me_;
    
    if (0 == c->id)
        return -1;
    if ( Debuglevel > 2 )
        printf("count.%d back.%d\n",me->count,me->back);
    __ensurecapacity(me);
    
    memcpy(&me->entries[me->back], c, sizeof(raft_entry_t));
    me->entries[me->back].num_nodes = 0;
    if (me->cb && me->cb->log_offer)
        me->cb->log_offer(me->raft, raft_get_udata(me->raft), c, me->back);
    me->count++;
    me->back++;
    return 0;
}

raft_entry_t* log_get_from_idx(log_t* me_, int32_t idx)
{
    log_private_t* me = (log_private_t*)me_;
    int32_t i;
    assert(0 <= idx - 1);
    if ( Debuglevel > 2 )
        printf("baselog.%d count.%d < idx.%d\n",me->base_log_idx,me->count,idx);
    if (me->base_log_idx + me->count < idx || idx < me->base_log_idx)
        return NULL;
    
    /* idx starts at 1 */
    idx -= 1;
    
    i = (me->front + idx - me->base_log_idx) % me->size;
    return &me->entries[i];
}

int32_t log_count(log_t* me_)
{
    return ((log_private_t*)me_)->count;
}

void log_delete(log_t* me_, int32_t idx)
{
    log_private_t* me = (log_private_t*)me_;
    int32_t end;
    
    /* idx starts at 1 */
    idx -= 1;
    idx -= me->base_log_idx;
    
    for (end = log_count(me_); idx < end; idx++)
    {
        if (me->cb && me->cb->log_pop)
            me->cb->log_pop(me->raft, raft_get_udata(me->raft),
                            &me->entries[me->back], me->back);
        me->back--;
        me->count--;
    }
}

void *log_poll(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;
    
    if (0 == log_count(me_))
        return NULL;
    
    const void *elem = &me->entries[me->front];
    if (me->cb && me->cb->log_poll)
        me->cb->log_poll(me->raft, raft_get_udata(me->raft),&me->entries[me->front], me->front);
    me->front++;
    me->count--;
    me->base_log_idx++;
    return (void*)elem;
}

raft_entry_t *log_peektail(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;
    
    if (0 == log_count(me_))
        return NULL;
    
    if (0 == me->back)
        return &me->entries[me->size - 1];
    else
        return &me->entries[me->back - 1];
}

void log_empty(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;
    
    me->front = 0;
    me->back = 0;
    me->count = 0;
}

void log_free(log_t * me_)
{
    log_private_t* me = (log_private_t*)me_;
    
    free(me->entries);
    free(me);
}

void log_mark_node_has_committed(log_t* me_, int32_t idx)
{
    raft_entry_t* e = log_get_from_idx(me_, idx);
    if (e)
        e->num_nodes += 1;
}

int32_t log_get_current_idx(log_t* me_)
{
    log_private_t* me = (log_private_t*)me_;
    return log_count(me_) + me->base_log_idx;
}

void raft_set_election_timeout(raft_server_t* me_, int32_t millisec)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->election_timeout = millisec;
}

void raft_set_request_timeout(raft_server_t* me_, int32_t millisec)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->request_timeout = millisec;
}

int32_t raft_get_nodeid(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->nodeid;
}

int32_t raft_get_election_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->election_timeout;
}

int32_t raft_get_request_timeout(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->request_timeout;
}

int32_t raft_get_num_nodes(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->num_nodes;
}

int32_t raft_get_timeout_elapsed(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->timeout_elapsed;
}

int32_t raft_get_log_count(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_count(me->log);
}

int32_t raft_get_voted_for(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->voted_for;
}

void raft_set_current_term(raft_server_t* me_, const int32_t term)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->current_term = term;
    if (me->cb.persist_term)
        me->cb.persist_term(me_, me->udata, term);
}

int32_t raft_get_current_term(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->current_term;
}

int32_t raft_get_current_idx(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_get_current_idx(me->log);
}

void raft_set_commit_idx(raft_server_t* me_, int32_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->commit_idx = idx;
}

void raft_set_last_applied_idx(raft_server_t* me_, int32_t idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->last_applied_idx = idx;
}

int32_t raft_get_last_applied_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->last_applied_idx;
}

int32_t raft_get_commit_idx(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->commit_idx;
}

void raft_set_state(raft_server_t* me_, int32_t state)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    /* if became the leader, then update the current leader entry */
    if (state == RAFT_STATE_LEADER)
        me->current_leader = me->nodeid;
    me->state = state;
}

int32_t raft_get_state(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->state;
}

raft_node_t* raft_get_node(raft_server_t *me_, int32_t nodeid)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    if (nodeid < 0 || me->num_nodes <= nodeid)
        return NULL;
    return (raft_node_t*)me->nodes[nodeid];
}

int32_t raft_get_current_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (void*)me_;
    return me->current_leader;
}

void* raft_get_udata(raft_server_t* me_)
{
    return ((raft_server_private_t*)me_)->udata;
}

int32_t raft_is_follower(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_FOLLOWER;
}

int32_t raft_is_leader(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_LEADER;
}

int32_t raft_is_candidate(raft_server_t* me_)
{
    return raft_get_state(me_) == RAFT_STATE_CANDIDATE;
}

raft_node_t* raft_node_new(void* udata)
{
    raft_node_private_t* me;
    me = (raft_node_private_t*)calloc(1, sizeof(raft_node_private_t));
    me->udata = udata;
    me->next_idx = 1;
    return (raft_node_t*)me;
}

int32_t raft_node_get_next_idx(raft_node_t* me_)
{
    raft_node_private_t* me = (raft_node_private_t*)me_;
    return me->next_idx;
}

void raft_node_set_next_idx(raft_node_t* me_, int32_t nextIdx)
{
    raft_node_private_t* me = (raft_node_private_t*)me_;
    /* log index begins at 1 */
    me->next_idx = nextIdx < 1 ? 1 : nextIdx;
}

void* raft_node_get_udata(raft_node_t* me_)
{
    raft_node_private_t* me = (raft_node_private_t*)me_;
    return me->udata;
}

void raft_node_set_udata(raft_node_t* me_, void* udata)
{
    raft_node_private_t* me = (raft_node_private_t*)me_;
    me->udata = udata;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))

static void __log(raft_server_t *me_, const char *fmt, ...)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    char buf[1024];
    va_list args;
    
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    
    if (me->cb.log)
        me->cb.log(me_, me->udata, buf);
}

raft_server_t* raft_new()
{
    raft_server_private_t* me = (raft_server_private_t*)calloc(1, sizeof(raft_server_private_t));
    if (!me)
        return NULL;
    me->current_term = 0;
    me->voted_for = -1;
    me->timeout_elapsed = 0;
    me->request_timeout = 200;
    me->election_timeout = 1000;
    me->log = log_new();
    raft_set_state((raft_server_t*)me, RAFT_STATE_FOLLOWER);
    me->current_leader = -1;
    return (raft_server_t*)me;
}

void raft_set_callbacks(raft_server_t* me_, raft_cbs_t* funcs, void* udata)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    memcpy(&me->cb, funcs, sizeof(raft_cbs_t));
    me->udata = udata;
    log_set_callbacks(me->log, &me->cb, me_);
}

void raft_free(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    log_free(me->log);
    free(me_);
}

void raft_election_start(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return;
    if ( Debuglevel > 2 )
        printf("start election.%d\n",me->nodeid);
    __log(me_, "election starting: %d %d, term: %d",me->election_timeout, me->timeout_elapsed, me->current_term);
    
    raft_become_candidate(me_);
}

void raft_become_leader(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t i;
    if ( Debuglevel > 2 )
        printf("becoming leader.%d\n",me->nodeid);
 
    __log(me_, "becoming leader");
    
    raft_set_state(me_, RAFT_STATE_LEADER);
    for (i = 0; i < me->num_nodes; i++)
    {
        if (me->nodeid != i)
        {
            raft_node_t* p = raft_get_node(me_, i);
            raft_node_set_next_idx(p, raft_get_current_idx(me_) + 1);
            raft_send_appendentries(me_, i);
        }
    }
}

void raft_become_candidate(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t i;
    return;
    if ( Debuglevel > 2 )
        printf("become candidate.%d\n",me->nodeid);

    __log(me_, "becoming candidate");
    
    memset(me->votes_for_me, 0, sizeof(int32_t) * me->num_nodes);
    me->current_term += 1;
    raft_vote(me_, me->nodeid);
    me->current_leader = -1;
    raft_set_state(me_, RAFT_STATE_CANDIDATE);
    
    /* we need a random factor here to prevent simultaneous candidates */
    me->timeout_elapsed = rand() % 500;
    
    for (i = 0; i < me->num_nodes; i++)
        if (me->nodeid != i)
            raft_send_requestvote(me_, i);
}

void raft_become_follower(raft_server_t* me_)
{
    if ( Debuglevel > 2 )
        printf("become follower.%d\n",((raft_server_private_t *)me_)->nodeid);
    __log(me_, "becoming follower");
    raft_set_state(me_, RAFT_STATE_FOLLOWER);
    raft_vote(me_, -1);
}

int32_t raft_periodic(raft_server_t* me_, int32_t msec_since_last_period)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    switch (me->state)
    {
        case RAFT_STATE_FOLLOWER:
            if (me->last_applied_idx < me->commit_idx)
                if (-1 == raft_apply_entry(me_))
                    return -1;
            break;
    }
    
    me->timeout_elapsed += msec_since_last_period;
    
    if (me->state == RAFT_STATE_LEADER)
    {
        if (me->request_timeout <= me->timeout_elapsed)
        {
            raft_send_appendentries_all(me_);
            me->timeout_elapsed = 0;
        }
    }
    else if (me->election_timeout <= me->timeout_elapsed)
        raft_election_start(me_);
    
    return 0;
}

raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int32_t etyidx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_get_from_idx(me->log, etyidx);
}

int32_t raft_recv_appendentries_response(raft_server_t* me_,int32_t node, msg_appendentries_response_t* r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    __log(me_, "received appendentries response node: %d %s cidx: %d 1stidx: %d",node, r->success == 1 ? "success" : "fail", r->current_idx, r->first_idx);
    
    // TODO: should force invalid leaders to stepdown
    
    raft_node_t* p = raft_get_node(me_, node);
    
    if (0 == r->success)
    {
        /* If AppendEntries fails because of log inconsistency:
         decrement nextIndex and retry (§5.3) */
        assert(0 <= raft_node_get_next_idx(p));
        // TODO can jump back to where node is different instead of iterating
        raft_node_set_next_idx(p, raft_node_get_next_idx(p) - 1);
        
        /* retry */
        raft_send_appendentries(me_, node);
        return 0;
    }
    
    raft_node_set_next_idx(p, r->current_idx + 1);
    
    int32_t i;
    
    for (i = r->first_idx; i <= r->current_idx; i++)
        log_mark_node_has_committed(me->log, i);
    
    while (1)
    {
        raft_entry_t* e = raft_get_entry_from_idx(me_, me->last_applied_idx + 1);
        
        /* majority has this */
        if (e && me->num_nodes / 2 <= e->num_nodes)
        {
            if (-1 == raft_apply_entry(me_))
                break;
        }
        else
            break;
    }
    
    return 0;
}

int32_t raft_recv_appendentries(
                            raft_server_t* me_,
                            const int32_t node,
                            msg_appendentries_t* ae,
                            msg_appendentries_response_t *r
                            )
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    me->timeout_elapsed = 0;
    if ( Debuglevel > 2 )
        printf("hn.%d recv append entries.%d\n",me->nodeid,ae->n_entries);
    if (0 < ae->n_entries)
        __log(me_, "recvd appendentries from: %d, %d %d %d %d #%d",
              node,
              ae->term,
              ae->leader_commit,
              ae->prev_log_idx,
              ae->prev_log_term,
              ae->n_entries);
    
    r->term = me->current_term;
    
    /* Step down - we've found a leader who is legitimate */
    if (raft_is_leader(me_) && me->current_term <= ae->term)
        raft_become_follower(me_);
    
    /* 1. Reply false if term < currentTerm (§5.1) */
    if (ae->term < me->current_term)
    {
        __log(me_, "AE term is less than current term");
        r->success = 0;
        return 0;
    }
    
#if 0
    if (-1 != ae->prev_log_idx &&
        ae->prev_log_idx < raft_get_current_idx(me_))
    {
        __log(me_, "AE prev_idx is less than current idx");
        r->success = 0;
        return 0;
    }
#endif
    
    /* Not the first appendentries we've received */
    /* NOTE: the log starts at 1 */
    if (0 < ae->prev_log_idx)
    {
        raft_entry_t* e = raft_get_entry_from_idx(me_, ae->prev_log_idx);
        
        if (!e)
        {
            __log(me_, "AE no log at prev_idx %d", ae->prev_log_idx);
            r->success = 0;
            return 0;
        }
        
        /* 2. Reply false if log doesn't contain an entry at prevLogIndex
         whose term matches prevLogTerm (§5.3) */
        if (e->term != ae->prev_log_term)
        {
            __log(me_, "AE term doesn't match prev_idx (ie. %d vs %d)",
                  e->term, ae->prev_log_term);
            r->success = 0;
            return 0;
        }
    }
    
    /* 3. If an existing entry conflicts with a new one (same index
     but different terms), delete the existing entry and all that
     follow it (§5.3) */
    raft_entry_t* e2 = raft_get_entry_from_idx(me_, ae->prev_log_idx + 1);
    if (e2)
        log_delete(me->log, ae->prev_log_idx + 1);
    
    /* 4. If leaderCommit > commitIndex, set commitIndex =
     min(leaderCommit, last log index) */
    if (raft_get_commit_idx(me_) < ae->leader_commit)
    {
        int32_t last_log_idx = max(raft_get_current_idx(me_) - 1, 1);
        raft_set_commit_idx(me_, min(last_log_idx, ae->leader_commit));
        while (0 == raft_apply_entry(me_))
            ;
    }
    
    if (raft_is_candidate(me_))
        raft_become_follower(me_);
    
    raft_set_current_term(me_, ae->term);
    /* update current leader because we accepted appendentries from it */
    me->current_leader = node;
    
    int32_t i;
    
    /* append all entries to log */
    for (i = 0; i < ae->n_entries; i++)
    {
        msg_entry_t* cmd = &ae->entries[i];
        
        raft_entry_t ety;
        if ( Debuglevel > 2 )
            printf("add entry iter\n");
        ety.term = cmd->term;
        ety.id = cmd->id;
        memcpy(&ety.data, &cmd->data, sizeof(raft_entry_data_t));
        int32_t e = raft_append_entry(me_, &ety);
        if (-1 == e)
        {
            __log(me_, "AE failure; couldn't append entry");
            r->success = 0;
            return -1;
        }
    }
    
    r->success = 1;
    r->current_idx = raft_get_current_idx(me_);
    r->first_idx = ae->prev_log_idx + 1;
    return 0;
}

static int32_t __should_grant_vote(raft_server_private_t* me, msg_requestvote_t* vr)
{
    if ( 1 && vr->term < raft_get_current_term((void*)me))
    {
        if ( Debuglevel > 2 )
            printf("__should_grant_vote.%d term less than vr.%d | %d < %d\n",me->nodeid,vr->candidate_id,vr->term,raft_get_current_term((void*)me));
        return 0;
    }
    
    /* we've already voted */
    if (-1 != me->voted_for)
    {
        if ( Debuglevel > 2 )
            printf("__should_grant_vote.%d already voted vr.%d | %d\n",me->nodeid,vr->candidate_id,me->voted_for);
        return 0;
    }
    
    /* we have a more up-to-date log */
    if (vr->last_log_idx < raft_get_current_idx((void*)me))
    {
        if ( Debuglevel > 2 )
            printf("__should_grant_vote.%d last log less than vr.%d\n",me->nodeid,vr->candidate_id);
        return 0;
    }
    return 1;
}

int32_t raft_recv_requestvote(raft_server_t* me_, int32_t node, msg_requestvote_t* vr,msg_requestvote_response_t *r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    if (raft_get_current_term(me_) < vr->term)
    {
        if ( Debuglevel > 2 )
            printf("less than become follower\n");
        raft_become_follower(me_);
    }
    
    if (__should_grant_vote(me, vr))
    {
        /* It shouldn't be possible for a leader or candidate to grant a vote
         * Both states would have voted for themselves */
        assert(!(raft_is_leader(me_) || raft_is_candidate(me_)));
        
        raft_vote(me_, node);
        r->vote_granted = 1;
    }
    else
        r->vote_granted = 0;
    
    /* voted for someone, therefore must be in an election. */
    if (0 <= me->voted_for)
        me->current_leader = -1;
    
    __log(me_, "node requested vote: %d replying: %s",node, r->vote_granted == 1 ? "granted" : "not granted");
    
    r->term = raft_get_current_term(me_);
    return 0;
}

int32_t raft_votes_is_majority(const int32_t num_nodes, const int32_t nvotes)
{
    if (num_nodes < nvotes)
        return 0;
    int32_t half = num_nodes / 2;
    return half + 1 <= nvotes;
}

int32_t raft_recv_requestvote_response(raft_server_t* me_, int32_t node,msg_requestvote_response_t* r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if ( Debuglevel > 2 )
        printf("hn.%d: raft_recv_requestvote_response\n",me->nodeid);
    __log(me_, "node responded to requestvote: %d status: %s",node, r->vote_granted == 1 ? "granted" : "not granted");
    
    if (raft_is_leader(me_))
    {
        printf("i am already leader\n");
        return 0;
    }
    
    assert(node < me->num_nodes);
    
    // TODO: if invalid leader then stepdown
    // if (r->term != raft_get_current_term(me_))
    // return 0;
    
    if (1 == r->vote_granted)
    {
        me->votes_for_me[node] = 1;
        int32_t votes = raft_get_nvotes_for_me(me_);
        if ( Debuglevel > 2 )
            printf("hn.%d: votes.%d (node.%d)\n",me->nodeid,votes,node);
        if (raft_votes_is_majority(me->num_nodes, votes))
            raft_become_leader(me_);
    }
    
    return 0;
}

int32_t raft_recv_entry(raft_server_t* me_, int32_t node, msg_entry_t* e,msg_entry_response_t *r)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t i;
    
    if (!raft_is_leader(me_))
        return -1;
    
    __log(me_, "received entry from: %d", node);
    
    raft_entry_t ety;
    ety.term = me->current_term;
    ety.id = e->id;
    memcpy(&ety.data, &e->data, sizeof(raft_entry_data_t));
    raft_append_entry(me_, &ety);
    for (i = 0; i < me->num_nodes; i++)
        if (me->nodeid != i)
            raft_send_appendentries(me_, i);
    
    r->id = e->id;
    r->idx = raft_get_current_idx(me_);
    r->term = me->current_term;
    if ( Debuglevel > 2 )
        printf("ENTRY.(%d %d %d)\n",r->id,r->idx,r->term);
    return 0;
}

int32_t raft_send_requestvote(raft_server_t* me_, int32_t node)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    msg_requestvote_t rv;
    
    __log(me_, "sending requestvote to: %d", node);
    
    rv.term = me->current_term;
    rv.last_log_idx = raft_get_current_idx(me_);
    rv.last_log_term = raft_get_current_term(me_);
    rv.candidate_id = raft_get_nodeid(me_);
    if (me->cb.send_requestvote)
        me->cb.send_requestvote(me_, me->udata, node, &rv);
    return 0;
}

int32_t raft_append_entry(raft_server_t* me_, raft_entry_t* ety)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    return log_append_entry(me->log, ety);
}

int32_t raft_apply_entry(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    raft_entry_t* e = raft_get_entry_from_idx(me_, me->last_applied_idx + 1);
    if (!e)
        return -1;
    
    __log(me_, "applying log: %d, size: %d", me->last_applied_idx, e->data.len);
    
    me->last_applied_idx++;
    if (me->commit_idx < me->last_applied_idx)
        me->commit_idx = me->last_applied_idx;
    if (me->cb.applylog)
        me->cb.applylog(me_, me->udata, e->data.buf, e->data.len);
    return 0;
}

void raft_send_appendentries(raft_server_t* me_, int32_t node)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    if ( Debuglevel > 2 )
        printf("raft_send_appendentries\n");
    if (!(me->cb.send_appendentries))
        return;
    
    raft_node_t* p = raft_get_node(me_, node);
    
    msg_appendentries_t ae;
    ae.term = me->current_term;
    ae.leader_commit = raft_get_commit_idx(me_);
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    ae.n_entries = 0;
    ae.entries = NULL;
    
    int32_t next_idx = raft_node_get_next_idx(p);
    
    msg_entry_t mety;
    
    raft_entry_t* ety = raft_get_entry_from_idx(me_, next_idx);
    if (ety)
    {
        mety.term = ety->term;
        mety.id = ety->id;
        mety.data.len = ety->data.len;
        mety.data.buf = ety->data.buf;
        ae.entries = &mety;
        // TODO: we want to send more than 1 at a time
        ae.n_entries = 1;
        if ( Debuglevel > 2 )
            printf("hn.%d sending %d %p\n",me->nodeid,mety.data.len,mety.data.buf);
    }
    if ( Debuglevel > 2 )
        printf("ety.%p next_idx.%d term.%d commit.%d n_entries.%d\n",ety,next_idx,ae.term,ae.leader_commit,ae.n_entries);
    /* previous log is the log just before the new logs */
    if (1 < next_idx)
    {
        raft_entry_t* prev_ety = raft_get_entry_from_idx(me_, next_idx - 1);
        ae.prev_log_idx = next_idx - 1;
        if (prev_ety)
            ae.prev_log_term = prev_ety->term;
    }
    
    __log(me_, "sending appendentries node: %d, %d %d %d %d",
          node,
          ae.term,
          ae.leader_commit,
          ae.prev_log_idx,
          ae.prev_log_term);
    
    me->cb.send_appendentries(me_, me->udata, node, &ae);
}

void raft_send_appendentries_all(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t i;
    
    for (i = 0; i < me->num_nodes; i++)
        if (me->nodeid != i)
            raft_send_appendentries(me_, i);
}

void raft_set_configuration(raft_server_t* me_,
                            raft_node_configuration_t* nodes, int32_t my_idx)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t num_nodes;
    
    /* TODO: one memory allocation only please */
    for (num_nodes = 0; nodes->udata_address; nodes++)
    {
        num_nodes++;
        me->nodes = (raft_node_t*)realloc(me->nodes, sizeof(raft_node_t*) * num_nodes);
        me->num_nodes = num_nodes;
        me->nodes[num_nodes - 1] = raft_node_new(nodes->udata_address);
    }
    me->votes_for_me = (int32_t*)calloc(num_nodes, sizeof(int32_t));
    me->nodeid = my_idx;
}

int32_t raft_add_node(raft_server_t* me_, void* udata, int32_t is_self)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    
    /* TODO: does not yet support dynamic membership changes */
    if (me->current_term != 0 && me->timeout_elapsed != 0 && me->election_timeout != 0)
        return -1;
    
    me->num_nodes++;
    me->nodes = (raft_node_t*)realloc(me->nodes, sizeof(raft_node_t*) * me->num_nodes);
    me->nodes[me->num_nodes - 1] = raft_node_new(udata);
    me->votes_for_me = (int32_t*)realloc(me->votes_for_me, me->num_nodes * sizeof(int32_t));
    me->votes_for_me[me->num_nodes - 1] = 0;
    if (is_self)
        me->nodeid = me->num_nodes - 1;
    return 0;
}

int32_t raft_get_nvotes_for_me(raft_server_t* me_)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    int32_t i, votes;
    
    for (i = 0, votes = 0; i < me->num_nodes; i++)
        if (me->nodeid != i)
            if (1 == me->votes_for_me[i])
                votes += 1;
    
    if (me->voted_for == me->nodeid)
        votes += 1;
    
    return votes;
}

void raft_vote(raft_server_t* me_, const int32_t node)
{
    raft_server_private_t* me = (raft_server_private_t*)me_;
    me->voted_for = node;
    if ( Debuglevel > 2 )
        printf("node.%d votes for %d\n",me->nodeid,me->voted_for);
    if (me->cb.persist_vote)
        me->cb.persist_vote(me_, me->udata, node);
}

int32_t raft_msg_entry_response_committed(raft_server_t* me_,const msg_entry_response_t* r)
{
    raft_entry_t* ety = raft_get_entry_from_idx(me_, r->idx);
    if (!ety)
    {
        printf("raft_msg_entry_response_committed null ety\n");
        return 0;
    }
    
    /* entry from another leader has invalidated this entry message */
    if (r->term != ety->term)
    {
        printf("raft_msg_entry_response_committed term.%d != %d\n",r->term,ety->term);
        return(-1);
    }
    if ( Debuglevel > 2 )
        printf("r->idx.%d <=? commit.%d\n",r->idx,raft_get_commit_idx(me_));
    return r->idx <= raft_get_commit_idx(me_);
}

typedef enum
{
    RAFT_MSG_REQUESTVOTE,
    RAFT_MSG_REQUESTVOTE_RESPONSE,
    RAFT_MSG_APPENDENTRIES,
    RAFT_MSG_APPENDENTRIES_RESPONSE,
    RAFT_MSG_ENTRY,
    RAFT_MSG_ENTRY_RESPONSE,
} raft_message_type_e;

int32_t raft_set_vrequest(msg_requestvote_t *vrequest,cJSON *json)
{
    memset(vrequest,0,sizeof(*vrequest));
    if ( json != 0 )
    {
        vrequest->term = juint(json,"term");
        vrequest->candidate_id = juint(json,"candid");
        vrequest->last_log_idx = juint(json,"idx");
        vrequest->last_log_term = juint(json,"lastterm");
        return(0);
    }
    return(-1);
}

cJSON *raft_vrequest_json(int32_t slot,msg_requestvote_t *vrequest,uint64_t nxt64bits)
{
    cJSON *json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",vrequest->term);
    jaddnum(json,"candid",vrequest->candidate_id);
    jaddnum(json,"idx",vrequest->last_log_idx);
    jaddnum(json,"lastterm",vrequest->last_log_term);
    return(json);
}

cJSON *raft_vresponse_json(int32_t slot,msg_requestvote_response_t *vresponse,uint64_t nxt64bits)
{
    cJSON *json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",vresponse->term);
    jaddnum(json,"granted",vresponse->vote_granted);
    return(json);
}

int32_t raft_set_vresponse(msg_requestvote_response_t *vresponse,cJSON *json)
{
    memset(vresponse,0,sizeof(*vresponse));
    if ( json != 0 )
    {
        vresponse->term = juint(json,"term");
        vresponse->vote_granted = juint(json,"granted");
        return(0);
    }
    return(-1);
}

cJSON *raft_erequest_json(int32_t slot,msg_entry_t *erequest,uint64_t nxt64bits)
{
    char *hexstr; cJSON *json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",erequest->term);
    jaddnum(json,"id",erequest->id);
    jaddnum(json,"len",erequest->data.len);
    hexstr = calloc(1,(erequest->data.len << 1) + 1);
    init_hexbytes_noT(hexstr,erequest->data.buf,erequest->data.len);
    jaddstr(json,"data",hexstr), free(hexstr);
    return(json);
}

int32_t raft_set_erequest(msg_entry_t *erequest,cJSON *json)
{
    char *data;
    memset(erequest,0,sizeof(*erequest));
    if ( json != 0 )
    {
        erequest->term = juint(json,"term");
        erequest->id = juint(json,"id");
        erequest->data.len = juint(json,"len");
        if ( (data= jstr(json,"data")) != 0 )
        {
            erequest->data.buf = malloc(erequest->data.len);
            decode_hex(erequest->data.buf,erequest->data.len,data);
            return(0);
        }
    }
    return(-1);
}

cJSON *raft_arequest_json(int32_t slot,msg_appendentries_t *arequest,uint64_t nxt64bits)
{
    int32_t i; char *hexstr; cJSON *array,*item,*json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",arequest->term);
    jaddnum(json,"previdx",arequest->prev_log_idx);
    jaddnum(json,"prevterm",arequest->prev_log_term);
    jaddnum(json,"commit",arequest->leader_commit);
    jaddnum(json,"n",arequest->n_entries);
    array = cJSON_CreateArray();
    for (i=0; i<arequest->n_entries; i++)
    {
        item = cJSON_CreateObject();
        jaddnum(item,"len",arequest->entries[i].data.len);
        hexstr = calloc(1,(arequest->entries[i].data.len << 1) + 1);
        init_hexbytes_noT(hexstr,arequest->entries[i].data.buf,arequest->entries[i].data.len);
        jaddstr(item,"data",hexstr), free(hexstr);
        jaddi(array,item);
    }
    jadd(json,"entries",array);
    return(json);
}

int32_t raft_set_arequest(msg_appendentries_t *arequest,cJSON *json)
{
    int32_t i,n; char *hexstr; cJSON *array,*item;
    memset(arequest,0,sizeof(*arequest));
    if ( json != 0 )
    {
        arequest->term = juint(json,"term");
        arequest->prev_log_idx = juint(json,"previdx");
        arequest->prev_log_term = juint(json,"prevterm");
        arequest->leader_commit = juint(json,"commit");
        arequest->n_entries = juint(json,"n");
        arequest->entries = calloc(arequest->n_entries,sizeof(*arequest->entries));
        if ( (array= jarray(&n,json,"entries")) != 0 && n == arequest->n_entries )
        {
            for (i=0; i<n; i++)
            {
                item = jitem(array,i);
                if ( (hexstr= jstr(json,"data")) != 0 )
                {
                    arequest->entries[i].data.len = juint(json,"len");
                    arequest->entries[i].data.buf = malloc(arequest->entries[i].data.len);
                    decode_hex(arequest->entries[i].data.buf,arequest->entries[i].data.len,hexstr);
                }
            }
            return(0);
        }
    }
    return(-1);
}

cJSON *raft_aresponse_json(int32_t slot,msg_appendentries_response_t *aresponse,uint64_t nxt64bits)
{
    cJSON *json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",aresponse->term);
    jaddnum(json,"success",aresponse->success);
    jaddnum(json,"current_idx",aresponse->current_idx);
    jaddnum(json,"first_idx",aresponse->first_idx);
    return(json);
}

int32_t raft_set_aresponse(msg_appendentries_response_t *aresponse,cJSON *json)
{
    memset(aresponse,0,sizeof(*aresponse));
    if ( json != 0 )
    {
        aresponse->term = juint(json,"term");
        aresponse->success = juint(json,"success");
        aresponse->current_idx = juint(json,"current_idx");
        aresponse->first_idx = juint(json,"first_idx");
        return(0);
    }
    return(-1);
}

cJSON *raft_eresponse_json(int32_t slot,msg_entry_response_t *eresponse,uint64_t nxt64bits)
{
    cJSON *json = cJSON_CreateObject();
    jaddnum(json,"timestamp",time(NULL));
    jadd64bits(json,"sender",nxt64bits);
    jaddnum(json,"slot",slot);
    jaddnum(json,"term",eresponse->term);
    jaddnum(json,"id",eresponse->id);
    jaddnum(json,"idx",eresponse->idx);
    return(json);
}

union hostnet777 HN[3];
int32_t append_msg(union hostnet777 *hn,void *jsonstr,int32_t type,long len,int32_t slot,raft_server_t *raft)
{
    //printf("HN.%p hn.%d append_msg.%d: slot.%d len.%ld (%s)\n",HN,hn->client->H.slot,type,slot,len,jsonstr);
    return(0);
    if ( slot != hn->client->H.slot )
        hostnet777_sendmsg(hn,HN[slot].client->H.pubkey,hn->client->H.privkey,hn->client->H.pubkey,jsonstr,(int32_t)len);
    else queue_enqueue("selfpost",&hn->client->H.Q,queueitem(jsonstr));
    return(0);
}

int sender_requestvote(raft_server_t *raft,void *hn,int32_t peer,msg_requestvote_t *msg)
{
    cJSON *json; char *jsonstr;
    return(0);
    json = raft_vrequest_json(((union hostnet777 *)hn)->client->H.slot,msg,((union hostnet777 *)hn)->client->H.nxt64bits);
    jsonstr = jprint(json,1);
    if ( Debuglevel > 2 )
        printf("hn.%d sender request vote %s\n",((union hostnet777 *)hn)->client->H.slot,jsonstr);
    return(append_msg(hn,jsonstr,RAFT_MSG_REQUESTVOTE,strlen(jsonstr)+1,peer,raft));
}

int sender_requestvote_response(raft_server_t *raft,void *hn,int32_t peer,msg_requestvote_response_t *msg)
{
    cJSON *json; char *jsonstr;
    return(0);
    json = raft_vresponse_json(((union hostnet777 *)hn)->client->H.slot,msg,((union hostnet777 *)hn)->client->H.nxt64bits);
    jsonstr = jprint(json,1);
    if ( Debuglevel > 2 )
        printf("hn.%d sender request vote response %s\n",((union hostnet777 *)hn)->client->H.slot,jsonstr);
    //raft_recv_requestvote_response(raft,peer,msg);
    return(append_msg(hn,jsonstr,RAFT_MSG_REQUESTVOTE_RESPONSE,strlen(jsonstr)+1,peer,raft));
}

int sender_appendentries(raft_server_t *raft,void *_hn,int32_t peer,msg_appendentries_t *msg)
{
    cJSON *json; char *jsonstr; union hostnet777 *hn = _hn;
    json = raft_arequest_json(hn->client->H.slot,msg,hn->client->H.nxt64bits);
    jsonstr = jprint(json,1);
    if ( Debuglevel > 2 )
        printf("sender sender_appendentries %s\n",jsonstr);
    if ( peer != hn->client->H.slot )
        hostnet777_sendmsg(hn,HN[peer].client->H.pubkey,hn->client->H.privkey,hn->client->H.pubkey,(void *)jsonstr,(int32_t)strlen(jsonstr)+1);
    return(append_msg(hn,jsonstr,RAFT_MSG_APPENDENTRIES,strlen(jsonstr)+1,peer,raft));
}

int sender_appendentries_response(raft_server_t *raft,void *hn,int32_t peer,msg_appendentries_response_t *msg)
{
    cJSON *json; char *jsonstr;
    json = raft_aresponse_json(((union hostnet777 *)hn)->client->H.slot,msg,((union hostnet777 *)hn)->client->H.nxt64bits);
    jsonstr = jprint(json,1);
    if ( Debuglevel > 2 )
        printf("sender append entries response.(%s)\n",jsonstr);
    return(append_msg(hn,jsonstr,RAFT_MSG_APPENDENTRIES_RESPONSE,strlen(jsonstr)+1,peer,raft));
}

int sender_entries_response(raft_server_t *raft,void *hn,int32_t peer,msg_entry_response_t *msg)
{
    cJSON *json; char *jsonstr;
    json = raft_eresponse_json(((union hostnet777 *)hn)->client->H.slot,msg,((union hostnet777 *)hn)->client->H.nxt64bits);
    jsonstr = jprint(json,1);
    if ( Debuglevel > 2 )
        printf("sender.%d entries response.(%s) from.%d\n",((union hostnet777 *)hn)->client->H.slot,jsonstr,peer);
    return(append_msg(hn,jsonstr,RAFT_MSG_ENTRY_RESPONSE,strlen(jsonstr)+1,peer,raft));
}

struct eresponse_item { struct queueitem DL; msg_entry_response_t eresponse; uint64_t millistamp; int32_t retval; };
void raft_queue_eresponse(union hostnet777 *hn,msg_entry_response_t *eresponse)
{
    struct eresponse_item *ptr = calloc(1,sizeof(*ptr));
    ptr->eresponse = *eresponse;
    if ( Debuglevel > 2 )
        printf("hn.%d queue response.%d\n",hn->client->H.slot,eresponse->id);
    queue_enqueue("eresponse",&hn->client->H.Q2,&ptr->DL);
}

int32_t sender_poll_msgs(union hostnet777 *hn)
{
    struct eresponse_item *ptr; raft_server_t *raft; char *jsonstr; cJSON *json; int32_t type,sender,n = 0;  msg_entry_response_t eresponse;
    msg_appendentries_t A; msg_appendentries_response_t aresponse; msg_requestvote_t V; msg_requestvote_response_t vresponse; msg_entry_t E;
    raft = hn->client->H.raft;
    //printf("hn.%d isleader.%d\n",hn->client->H.slot,raft_is_leader(hn->client->H.raft));
    if ( (jsonstr= queue_dequeue(&hn->client->H.Q,1)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) == 0 )
            free_queueitem(jsonstr);
        else
        {
            type = juint(json,"type");
            sender = juint(json,"slot");
            if ( Debuglevel > 2 )
                printf("hn.%d RECV.(%s) slot.%d\n",hn->client->H.slot,jsonstr,sender);
            switch ( type )
            {
                case RAFT_MSG_APPENDENTRIES:
                    raft_set_arequest(&A,jobj(json,"msg"));
                    n += raft_recv_appendentries(raft,sender,&A,&aresponse) == 0;
                    sender_appendentries_response(raft,hn,sender,&aresponse);
                    //append_msg(hn,&aresponse,RAFT_MSG_APPENDENTRIES_RESPONSE,sizeof(aresponse),sender,raft);
                    break;
                case RAFT_MSG_APPENDENTRIES_RESPONSE:
                    raft_set_aresponse(&aresponse,jobj(json,"msg"));
                    n += raft_recv_appendentries_response(raft,sender,&aresponse) == 0;
                    break;
                case RAFT_MSG_REQUESTVOTE:
                    if ( 0 )
                    {
                        raft_set_vrequest(&V,jobj(json,"msg"));
                        n += raft_recv_requestvote(raft,sender,&V,&vresponse) == 0;
                        sender_requestvote_response(raft,hn,sender,&vresponse);
                        //append_msg(hn,&vresponse,RAFT_MSG_REQUESTVOTE_RESPONSE,sizeof(vresponse),sender,raft);
                    }
                    break;
                case RAFT_MSG_REQUESTVOTE_RESPONSE:
                    //printf("request vote response\n");
                    raft_set_vresponse(&vresponse,jobj(json,"msg"));
                    n += raft_recv_requestvote_response(raft,sender,&vresponse) == 0;
                    break;
                case RAFT_MSG_ENTRY:
                    //printf("hn.%d sender.%d type.%d RAFT.(%s)\n",hn->client->H.slot,sender,type,jsonstr);
                    raft_set_erequest(&E,jobj(json,"msg"));
                    n += raft_recv_entry(raft,sender,&E,&eresponse) == 0;
                    if ( sender == hn->client->H.slot )
                        raft_queue_eresponse(hn,&eresponse);
                    sender_entries_response(raft,hn,sender,&eresponse);
                    //append_msg(hn,&eresponse,RAFT_MSG_ENTRY_RESPONSE,sizeof(eresponse),sender,raft);
                    break;
                case RAFT_MSG_ENTRY_RESPONSE:
                    printf("got RAFT_MSG_ENTRY_RESPONSE.(%s)\n",jsonstr);
                    //n = raft_recv_entry_response(raft,sender,&eresponse) == 0;
                    break;
            }
            free_json(json);
            free_queueitem(jsonstr);
        }
    }
    //if ( n == 0 )
    {
        if ( (ptr= queue_dequeue(&hn->client->H.Q2,0)) != 0 )
        {
            //printf("hn.%d deQ id.%d\n",hn->client->H.slot,ptr->eresponse.id);
            if ( (ptr->retval= raft_msg_entry_response_committed(raft,&ptr->eresponse)) != 0 )
            {
                if ( Debuglevel > 2 )
                    printf("hn.%d eresponse.%d completed status.%d\n",hn->client->H.slot,ptr->eresponse.id,ptr->retval);
                ptr->millistamp = ((time(NULL) * 1000) + ((uint64_t)milliseconds() % 1000));
                queue_enqueue("eresponse done",&hn->client->H.Q3[0],&ptr->DL);
            }
            else queue_enqueue("eresponse pending",&hn->client->H.Q2,&ptr->DL);
        }
    }
    return(n);
}

int32_t raft_checkQ(int32_t *retvalp,uint64_t *millistampp,union hostnet777 *hn,int32_t erequest_id)
{
    int32_t iter; struct eresponse_item *ptr; raft_server_private_t *raft;
    for (iter=0; iter<2; iter++)
    {
        while ( (ptr= queue_dequeue(&hn->client->H.Q3[iter],0)) != 0 )
        {
            if ( Debuglevel > 2 )
                printf("hn.%d dequeied Q3 id.%d vs ind.%d\n",hn->client->H.slot,ptr->eresponse.id,erequest_id);
            if ( ptr->eresponse.id == erequest_id )
            {
                *millistampp = ptr->millistamp;
                *retvalp = ptr->retval;
                //raft_node_set_next_idx(hn->client->H.raft,ptr->eresponse.id + 1);
                raft = hn->client->H.raft;
                //log_delete(raft->log,ptr->eresponse.id);
                log_empty(raft->log);
                //raft->log = log_new();
                if ( Debuglevel > 2 )
                    printf("found matched erequest.id %x millistamp %llu retval.%d\n",erequest_id,(long long)ptr->millistamp,ptr->retval);
                free(ptr);
                return(1);
            } else queue_enqueue("requeue",&hn->client->H.Q3[iter ^ 1],&ptr->DL);
        }
    }
    return(0);
}

int32_t raft_newstate(int32_t *retvalp,uint64_t *millistampp,union hostnet777 *hn,char *jsonstr,int32_t blockflag)
{
    msg_entry_t erequest; cJSON *json = cJSON_CreateObject();
    memset(&erequest,0,sizeof(erequest));
    *millistampp = *retvalp = 0;
    //jaddnum(json,"timestamp",time(NULL));
    //jadd64bits(json,"sender",hn->client->H.nxt64bits);
    jaddnum(json,"type",RAFT_MSG_ENTRY);
    jaddnum(json,"slot",hn->client->H.slot);
    erequest.term = raft_get_current_term(hn->client->H.raft);
    erequest.id = ++hn->client->H.ind;
    erequest.data.buf = jsonstr;
    erequest.data.len = (int32_t)strlen(jsonstr)+1;
    jadd(json,"msg",raft_erequest_json(hn->client->H.slot,&erequest,((union hostnet777 *)hn)->client->H.nxt64bits));
    jsonstr = jprint(json,1);
    queue_enqueue("raft",&hn->client->H.Q,queueitem(jsonstr));
    if ( Debuglevel > 2 )
    printf("Q.(%s)\n",jsonstr);
    free(jsonstr);
    if ( blockflag != 0 )
        raft_checkQ(retvalp,millistampp,hn,erequest.id);
    return(erequest.id);
}

void raft777_test()
{
    int32_t i,j,leaders,n,retval,ind = -1; uint64_t millistamp; bits256 privkeys[3]; raft_server_t *r[3];
    randombytes(privkeys[0].bytes,sizeof(privkeys));
    hostnet777_init(HN,privkeys,3,1);
    for (j=0; j<3; j++)
    {
        HN[j].client->H.raft = r[j] = raft_new();
        raft_set_election_timeout(r[j],500);
        raft_add_node(r[j],&HN[j],j==0);
        raft_add_node(r[j],&HN[j],j==1);
        raft_add_node(r[j],&HN[j],j==2);
        /*raft_set_state(r[j],(j == 0) ? RAFT_STATE_LEADER : RAFT_STATE_FOLLOWER);
        if ( j == 0 )
        {
            raft_become_leader(r[j]);
            raft_set_commit_idx(r[j],1);
        }
        else raft_become_follower(r[j]);*/
        raft_set_callbacks(r[j],&((raft_cbs_t) { .send_requestvote = sender_requestvote, .send_appendentries = sender_appendentries, .log = NULL}),&HN[j]);
    }
    raft_periodic(r[0],1000); // NOTE: important for 1st node to send vote request before others
    for (i=0; i<19; i++)
    {
        //sleep(1);
        if ( (i % 1000) == 0 )
            printf("iter.%d\n",i);
        if ( ind >= 0 && raft_checkQ(&retval,&millistamp,&HN[0],ind) != 0 )
            ind = -1;
        if ( i > 10 && ind < 0 )
        {
            char buf[128];
            sprintf(buf,"[%d]",i);
            ind = raft_newstate(&retval,&millistamp,&HN[0],buf,0);
            if ( Debuglevel > 2 )
                printf(">>>>>>>> send.(%s) i.%d ind.%d\n",buf,i,ind);
        }
        //do
        {
            for (j=n=0; j<3; j++)
                n += sender_poll_msgs(&HN[j]);
        } //while ( n != 0 );
        for (j=0; j<3; j++)
            raft_periodic(r[j],100);
    }
    leaders = 0;
    for (j=0; j<3; j++)
        if ( raft_is_leader(r[j]) )
            leaders += 1;
    printf("num leaders.%d\n",leaders), getchar();
}

#endif
#endif


